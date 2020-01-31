// Copyright (c) 2019-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_g9_task.h"

#include <thread>

using namespace HEVCEHW;
using namespace HEVCEHW::Gen9;

void TaskManager::InitAlloc(const FeatureBlocks& blocks, TPushIA Push)
{
    Push(BLK_Init
        , [this, &blocks](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        auto rdrBuf = Glob::Reorder::Get(strg).BufferSize;

        mfxStatus sts = Reset(par.AsyncDepth + rdrBuf + (par.AsyncDepth > 1));
        MFX_CHECK_STS(sts);

        for (auto& task : m_stages[0])
        {
            sts = RunBlocks(Check<mfxStatus, MFX_ERR_NONE>, FeatureBlocks::BQ<AT>::Get(blocks), strg, task);
            MFX_CHECK_STS(sts);
        }

        m_nPicBuffered       = 0;
        m_bufferSize         = !par.mfx.EncodedOrder * (rdrBuf + (par.AsyncDepth > 1));
        m_maxParallelSubmits = std::min<mfxU16>(2, Glob::AllocBS::Get(strg).GetResponse().NumFrameActual);
        m_nTasksInExecution  = 0;

        return sts;
    });
}

void TaskManager::FrameSubmit(const FeatureBlocks& blocks, TPushFS Push)
{
    Push(BLK_NewTask
        , [this, &blocks](
             mfxEncodeCtrl* pCtrl
            , mfxFrameSurface1* pSurf
            , mfxBitstream& bs
            , StorageW& global
            , StorageRW& local) -> mfxStatus
    {
        MFX_CHECK(pSurf || m_nPicBuffered, MFX_ERR_MORE_DATA);

        auto pBs = &bs;
        auto pTask = MoveTask(S_NEW, S_PREPARE);
        MFX_CHECK(pTask, MFX_WRN_DEVICE_BUSY);

        local.Insert(Tmp::CurrTask::Key, make_unique<StorableRef<StorageW>>(*pTask));

        bool bBufferPic = pSurf && m_nPicBuffered < m_bufferSize;
        if (bBufferPic)
        {
            pBs = nullptr;
            m_nPicBuffered++;
        }

        m_nPicBuffered -= !pSurf && m_nPicBuffered;

        auto sts = RunBlocks(
            CheckGE<mfxStatus, MFX_ERR_NONE>
            , FeatureBlocks::BQ<IT>::Get(blocks)
            , pCtrl, pSurf, pBs, global, *pTask);

        MFX_CHECK(sts >= MFX_ERR_NONE, sts);
        MFX_CHECK(pBs, MFX_ERR_MORE_DATA_SUBMIT_TASK);

        return sts;
    });
}

void TaskManager::AsyncRoutine(const FeatureBlocks& blocks, TPushAR Push)
{
    Push(BLK_PrepareTask
        , [this, &blocks](
            StorageW& global
            , StorageW& task) -> mfxStatus
    {
        std::unique_lock<std::mutex> closeGuard(m_closeMtx);
        auto& tpar = Task::Common::Get(task);

        MFX_CHECK(!m_nRecodeTasks, MFX_ERR_NONE);
        MFX_CHECK(tpar.stage == S_PREPARE, MFX_ERR_NONE);
        MFX_CHECK(tpar.pSurfIn, MFX_ERR_NONE);// leave fake task in "prepare" stage for now

        auto sts = RunBlocks(Check<mfxStatus, MFX_ERR_NONE>, FeatureBlocks::BQ<PreRT>::Get(blocks), global, task);
        MFX_CHECK_STS(sts);

        auto pTask = MoveTask(S_PREPARE, S_REORDER, FixedTask(task));

        MFX_CHECK(&(StorageW&)*pTask == &task, MFX_ERR_UNDEFINED_BEHAVIOR);

        return MFX_ERR_NONE;
    });

    Push(BLK_ReorderTask
        , [this, &blocks](
            StorageW& global
            , StorageW& task) -> mfxStatus
    {
        std::unique_lock<std::mutex> closeGuard(m_closeMtx);
        bool bNeedTask = !m_nRecodeTasks
            && (m_stages.at(S_SUBMIT).size() + m_nTasksInExecution) < m_maxParallelSubmits;
        MFX_CHECK(bNeedTask, MFX_ERR_NONE);

        auto       IsInputTask = [](StorageR& rTask) { return !!Task::Common::Get(rTask).pSurfIn; };
        StorageW*  pTask       = nullptr;
        bool       bBypass     = !!Glob::VideoParam::Get(global).mfx.EncodedOrder;
        bool       bFlush      = !IsInputTask(task) && !GetTask(S_PREPARE, SimpleCheck(IsInputTask));
        TFnGetTask GetNextTask = [&](TTaskIt b, TTaskIt e)
        {
            auto IsIdrTask = [](const StorageR& rTask) { return IsIdr(Task::Common::Get(rTask).FrameType); };
            auto it = std::find_if(b, e, IsIdrTask);
            bFlush |= (it != e);
            return Glob::Reorder::Get(global)(b, it, bFlush);
        };

        SetIf(GetNextTask, bBypass, FirstTask);

        pTask = MoveTask(S_REORDER, S_SUBMIT, GetNextTask);
        MFX_CHECK(pTask, MFX_ERR_NONE);

        return RunBlocks(
            Check<mfxStatus, MFX_ERR_NONE>
            , FeatureBlocks::BQ<PostRT>::Get(blocks)
            , global
            , *pTask);
    });

    Push(BLK_SubmitTask
        , [this, &blocks](
            StorageW& global
            , StorageW& /*task*/) -> mfxStatus
    {
        std::unique_lock<std::mutex> closeGuard(m_closeMtx);
        mfxStatus sts;

        while (StorageW* pTask = GetTask(S_SUBMIT))
        {
            auto& task = Task::Common::Get(*pTask);
            bool bSync =
                (m_maxParallelSubmits <= m_nTasksInExecution)
                || (task.bForceSync && GetTask(S_QUERY));
            MFX_CHECK(!bSync, MFX_ERR_NONE);

            sts = RunBlocks(
                Check<mfxStatus, MFX_ERR_NONE>
                , FeatureBlocks::BQ<ST>::Get(blocks)
                , global
                , *pTask);
            MFX_CHECK_STS(sts);

            MoveTask(S_SUBMIT, S_QUERY, FixedTask(*pTask));
            ++m_nTasksInExecution;
            m_nRecodeTasks -= !!m_nRecodeTasks;
        }

        return MFX_ERR_NONE;
    });

    Push(BLK_QueryTask
        , [this, &blocks](
            StorageW& global
            , StorageW& inTask) -> mfxStatus
    {
        std::unique_lock<std::mutex> closeGuard(m_closeMtx);
        auto pBs = Task::Common::Get(inTask).pBsOut;
        MFX_CHECK(pBs, MFX_ERR_NONE);

        StorageRW*          pTask             = nullptr;
        StorageRW*          pPrevRecode       = nullptr;
        bool                bCallAgain        = false;
        const mfxStatus     NoTaskErr[2]      = { MFX_ERR_UNDEFINED_BEHAVIOR, MFX_TASK_BUSY };
        auto                RanQueryTaskBlock =
            [&](FeatureBlocks::BQ<QT>::TQueue::const_reference block)
        {
            mfxStatus sts = block.Call(global, *pTask);
            bCallAgain = (pPrevRecode && sts == MFX_TASK_BUSY);

            if (bCallAgain)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return true;
            }
            ThrowIf(!!sts, sts);

            auto& task = Task::Common::Get(*pTask);

            return (task.bRecode && task.BsDataLength);
        };
        auto NextToPrevRecode = [&](TTaskIt begin, TTaskIt end)
        {
            auto it = FixedTask(*pPrevRecode)(begin, end);
            return std::next(it, it != end);
        };

        auto& qQueryTask = FeatureBlocks::BQ<QT>::Get(blocks);

        do
        {
            pTask = GetTask(S_QUERY);
            MFX_CHECK(pTask, NoTaskErr[!!pPrevRecode]);

            auto& task        = Task::Common::Get(*pTask);
            task.pBsOut       = pBs;
            task.bRecode      = !!pPrevRecode;
            task.BsDataLength *= !task.bRecode; //reset value from prev. recode if any
            bool  bRecode     = false;

            do
            {
                bRecode = std::any_of(qQueryTask.begin(), qQueryTask.end(), RanQueryTaskBlock);
            } while (bCallAgain);

            task.NumRecode += bRecode && !pPrevRecode;

            SetIf(pPrevRecode, !!pPrevRecode
                , std::mem_fn(&TaskManager::MoveTask)
                , *this, mfxU16(S_QUERY), mfxU16(S_SUBMIT), FixedTask(*pTask), NextToPrevRecode);

            SetIf(pPrevRecode, bRecode && !pPrevRecode
                , std::mem_fn(&TaskManager::MoveTask)
                , *this, mfxU16(S_QUERY), mfxU16(S_SUBMIT), FixedTask(*pTask), FirstTask);

            --m_nTasksInExecution;
            m_nRecodeTasks += bRecode;
        } while (pPrevRecode);

        ThrowAssert(pPrevRecode, "For recode must exit by \"no task for query\" condition");

        auto sts = RunBlocks(
            Check<mfxStatus, MFX_ERR_NONE>
            , FeatureBlocks::BQ<FT>::Get(blocks)
            , global, *pTask);
        MFX_CHECK_STS(sts);

        if (!Task::Common::Get(inTask).pSurfIn)
        {
            MoveTask(S_PREPARE, S_NEW, FixedTask(inTask)); //don't need fake task anymore
        }

        MoveTask(S_QUERY, S_NEW, FixedTask(*pTask));

        return MFX_ERR_NONE;
    });
}

void TaskManager::ResetState(const FeatureBlocks& blocks, TPushRS Push)
{
    Push(BLK_Reset
        , [this, &blocks](
            StorageW& global, StorageW&) -> mfxStatus
    {
        if (Glob::ResetHint::Get(global).Flags & (RF_IDR_REQUIRED | RF_CANCEL_TASKS))
        {
            CancelTasks(Glob::RealState::Get(global), FeatureBlocks::BQ<FT>::Get(blocks));
            m_nTasksInExecution = 0;
            m_nPicBuffered      = 0;
            m_nRecodeTasks      = 0;
        }

        return MFX_ERR_NONE;
    });
}

void TaskManager::Close(const FeatureBlocks& blocks, TPushCLS Push)
{
    Push(BLK_Close
        , [this, &blocks](
            StorageW& global) -> mfxStatus
    {
        CancelTasks(global, FeatureBlocks::BQ<FT>::Get(blocks));
        return MFX_ERR_NONE;
    });
}

void TaskManager::CancelTasks(StorageW& global, const FeatureBlocks::BQ<FT>::TQueue& qFreeTask)
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);

    auto stageIt = m_stages.begin();

    while (++stageIt != m_stages.end())
    {
        std::for_each(stageIt->begin(), stageIt->end()
            , [&](StorageRW& task)
        {
            RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, qFreeTask, global, task);
        });

        m_stages.front().splice(m_stages.front().end(), *stageIt);
    }
}

mfxStatus TaskManager::Reset(mfxU32 numTask)
{
    std::unique_lock<std::mutex> lock(m_mtx);

    MFX_CHECK(m_cv.wait_for(
        lock
        , std::chrono::seconds(600)
        , [&] { return m_stages.back().empty(); })
        , MFX_ERR_GPU_HANG);

    MFX_CHECK(numTask, MFX_ERR_NONE);

    for (size_t i = 1; i < m_stages.size(); i++)
        m_stages.front().splice(m_stages.front().end(), m_stages[i]);

    m_stages.front().resize(numTask);

    return MFX_ERR_NONE;
}

StorageRW* TaskManager::GetTask(
    mfxU16 stage
    , TFnGetTask which)
{
    if (stage >= m_stages.size())
        throw std::out_of_range("Invalid task stage id");

    {
        std::unique_lock<std::mutex> lock(m_mtx);
        auto& src = m_stages[stage];
        auto it = which(src.begin(), src.end());

        if (it != src.end())
            return &*it;
    }

    return nullptr;
}

StorageRW* TaskManager::MoveTask(
    mfxU16 from
    , mfxU16 to
    , TFnGetTask which
    , TFnGetTask where)
{
    StorageRW* pTask = nullptr;
    bool bNotify = false;

    if (from >= m_stages.size() || to >= m_stages.size())
        throw std::out_of_range("Invalid task stage id");

    {
        std::unique_lock<std::mutex> lock(m_mtx);
        auto& src = m_stages[from];
        auto& dst = m_stages[to];

        if (!m_stages[from].empty())
        {
            auto itWhich = which(src.begin(), src.end());
            auto itWhere = where(dst.begin(), dst.end());

            if (itWhich == src.end())
                return nullptr;

            pTask = &*itWhich;
            dst.splice(itWhere, src, itWhich);

            bNotify = (to == 0 && m_stages.back().empty());

            auto& stage = Task::Common::Get(*pTask).stage;
            stage |= (1 << from);
            stage &= ~(0xffffffff << to);
        }
    }

    if (bNotify)
        m_cv.notify_one();

    return pTask;
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)