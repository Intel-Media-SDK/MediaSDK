// Copyright (c) 2020-2021 Intel Corporation
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

#include "ehw_task_manager.h"
#include <thread>

namespace MfxEncodeHW
{
mfxStatus TaskManager::ManagerInit()
{
    mfxStatus sts = ManagerReset(GetNumTask());
    MFX_CHECK_STS(sts);

    for (auto& task : m_stages[0])
    {
        sts = RunQueueTaskAlloc(task);
        MFX_CHECK_STS(sts);
    }

    m_nPicBuffered       = 0;
    m_bufferSize         = GetBufferSize();
    m_maxParallelSubmits = GetMaxParallelSubmits();
    m_nTasksInExecution  = 0;

    return sts;
}

mfxStatus TaskManager::TaskNew(
    mfxEncodeCtrl* pCtrl
    , mfxFrameSurface1* pSurf
    , mfxBitstream& bs)
{
    MFX_CHECK(pSurf || m_nPicBuffered, MFX_ERR_MORE_DATA);

    auto pBs = &bs;
    auto pTask = MoveTask(Stage(S_NEW), Stage(S_PREPARE));
    MFX_CHECK(pTask, MFX_WRN_DEVICE_BUSY);

    SetActiveTask(*pTask);

    bool bBufferPic = pSurf && m_nPicBuffered < m_bufferSize;
    if (bBufferPic)
    {
        pBs = nullptr;
        ++m_nPicBuffered;
    }

    m_nPicBuffered -= !pSurf && m_nPicBuffered;

    auto sts = RunQueueTaskInit(pCtrl, pSurf, pBs, *pTask);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);
    MFX_CHECK(pBs, MFX_ERR_MORE_DATA_SUBMIT_TASK);

    return sts;
}

mfxStatus TaskManager::TaskPrepare(StorageW& task)
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);

    MFX_CHECK(!m_nRecodeTasks, MFX_ERR_NONE);
    MFX_CHECK(GetStage(task) == Stage(S_PREPARE), MFX_ERR_NONE);
    MFX_CHECK(IsInputTask(task), MFX_ERR_NONE);// leave fake task in "prepare" stage for now

    auto sts = RunQueueTaskPreReorder(task);
    MFX_CHECK_STS(sts);

    auto pTask = MoveTask(Stage(S_PREPARE), Stage(S_REORDER), FixedTask(task));

    MFX_CHECK(&(StorageW&)*pTask == &task, MFX_ERR_UNDEFINED_BEHAVIOR);

    return MFX_ERR_NONE;
}

mfxStatus TaskManager::TaskReorder(StorageW& task)
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);
    bool bNeedTask = !m_nRecodeTasks
        && (m_stages.at(Stage(S_SUBMIT)).size() + m_nTasksInExecution) < m_maxParallelSubmits;
    MFX_CHECK(bNeedTask, MFX_ERR_NONE);

    auto       IsInputTask = [this](StorageR& rTask) { return this->IsInputTask(rTask); };
    StorageW*  pTask       = nullptr;
    bool       bFlush      = !IsInputTask(task) && !GetTask(Stage(S_PREPARE), SimpleCheck(IsInputTask));
    TFnGetTask GetNextTask = [&](TTaskIt b, TTaskIt e) { return GetNextTaskToEncode(b, e, bFlush); };

    if (IsReorderBypass())
    {
        GetNextTask = FirstTask;
    }

    pTask = MoveTask(Stage(S_REORDER), Stage(S_SUBMIT), GetNextTask);
    MFX_CHECK(pTask, MFX_ERR_NONE);

    return RunQueueTaskPostReorder(*pTask);
}

mfxStatus TaskManager::TaskSubmit(StorageW& /*task*/)
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);

    while (StorageW* pTask = GetTask(Stage(S_SUBMIT)))
    {
        bool bSync =
            (m_maxParallelSubmits <= m_nTasksInExecution)
            || (IsForceSync(*pTask) && GetTask(Stage(S_QUERY)));
        MFX_CHECK(!bSync, MFX_ERR_NONE);

        auto sts = RunQueueTaskSubmit(*pTask);
        MFX_CHECK_STS(sts);

        MoveTask(Stage(S_SUBMIT), Stage(S_QUERY), FixedTask(*pTask));
        ++m_nTasksInExecution;
        m_nRecodeTasks -= !!m_nRecodeTasks;
    }

    return MFX_ERR_NONE;
}

mfxStatus TaskManager::TaskQuery(StorageW& inTask)
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);
    auto pBs = GetBS(inTask);
    MFX_CHECK(pBs, MFX_ERR_NONE);
    if (m_bPostponeQuery) // wait for the subsequent task to restore parallel submission
    {
        m_bPostponeQuery = false;
        return MFX_ERR_NONE;
    }

    StorageRW* pTask        = nullptr;
    StorageRW* pPrevRecode  = nullptr;
    bool       bCallAgain   = false;
    mfxStatus  NoTaskErr[2] = { MFX_ERR_UNDEFINED_BEHAVIOR, MFX_TASK_BUSY };
    auto       NeedRecode   = [&](mfxStatus sts)
    {
        bCallAgain = (pPrevRecode && sts == MFX_TASK_BUSY);

        if (bCallAgain)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return true;
        }
        ThrowIf(!!sts, sts);

        return (GetRecode(*pTask) && GetBsDataLength(*pTask));
    };
    auto NextToPrevRecode = [&](TTaskIt begin, TTaskIt end)
    {
        auto it = FixedTask(*pPrevRecode)(begin, end);
        return std::next(it, it != end);
    };

    do
    {
        pTask = GetTask(Stage(S_QUERY));
        MFX_CHECK(pTask, NoTaskErr[!!pPrevRecode]);

        auto& task    = *pTask;
        bool  bRecode = false;

        SetBS(task, pBs);
        SetRecode(task, !!pPrevRecode);
        SetBsDataLength(task, GetBsDataLength(task) * !pPrevRecode); //reset value from prev. recode if any

        do
        {
            bRecode = RunQueueTaskQuery(task, NeedRecode);
        } while (bCallAgain);

        AddNumRecode(task, bRecode && !pPrevRecode);

        if (!!pPrevRecode)
        {
            pPrevRecode = MoveTask(Stage(S_QUERY), Stage(S_SUBMIT), FixedTask(*pTask), NextToPrevRecode);
        }

        if (bRecode && !pPrevRecode)
        {
            pPrevRecode = MoveTask(Stage(S_QUERY), Stage(S_SUBMIT), FixedTask(*pTask), FirstTask);
        }

        --m_nTasksInExecution;
        m_nRecodeTasks += bRecode;
    } while (pPrevRecode);

    ThrowIf(pPrevRecode, std::logic_error("For recode must exit by \"no task for query\" condition"));

    auto sts = RunQueueTaskFree(*pTask);
    MFX_CHECK_STS(sts);

    if (!IsInputTask(inTask))
    {
        MoveTask(Stage(S_PREPARE), Stage(S_NEW), FixedTask(inTask)); //don't need fake task anymore
    }

    MoveTask(Stage(S_QUERY), Stage(S_NEW), FixedTask(*pTask));

    if (m_nRecodeTasks) // when some task is left in submit stage w/o being moved to the next stage
    {
        m_bPostponeQuery = true; // repeat async cycle w/ same parameters except query stage to resubmit the task
        return MFX_TASK_WORKING;
    }
    return MFX_ERR_NONE;
}

void TaskManager::CancelTasks()
{
    std::unique_lock<std::mutex> closeGuard(m_closeMtx);

    auto stageIt = m_stages.begin();

    while (++stageIt != m_stages.end())
    {
        std::for_each(stageIt->begin(), stageIt->end()
            , [&](StorageRW& task)
        {
            RunQueueTaskFree(task);
        });

        m_stages.front().splice(m_stages.front().end(), *stageIt);
    }

    m_nTasksInExecution = 0;
    m_nPicBuffered      = 0;
    m_nRecodeTasks      = 0;
}

mfxStatus TaskManager::ManagerReset(mfxU32 numTask)
{
    std::unique_lock<std::mutex> lock(m_mtx);

    MFX_CHECK(m_cv.wait_for(
        lock
        , std::chrono::seconds(600)
        , [&] { return m_stages.back().empty(); })
        , MFX_ERR_GPU_HANG);

    MFX_CHECK(numTask, MFX_ERR_NONE);

    for (size_t i = 1; i < m_stages.size(); ++i)
        m_stages.front().splice(m_stages.front().end(), m_stages[i]);

    m_stages.front().resize(numTask);

    return MFX_ERR_NONE;
}

StorageRW* TaskManager::MoveTask(
    mfxU16 from
    , mfxU16 to
    , TFnGetTask which
    , TFnGetTask where)
{
    StorageRW* pTask = nullptr;
    bool bNotify = false;

    ThrowIf(from >= m_stages.size() || to >= m_stages.size(), std::out_of_range("Invalid task stage id"));

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

            auto stage = GetStage(*pTask);
            stage |= (1 << from);
            stage &= ~(0xffffffff << to);

            SetStage(*pTask, stage);
        }
    }

    if (bNotify)
        m_cv.notify_one();

    return pTask;
}

StorageRW* TaskManager::GetTask(mfxU16 stage, TFnGetTask which)
{
    ThrowIf(stage >= m_stages.size(), std::out_of_range("Invalid task stage id"));

    {
        std::unique_lock<std::mutex> lock(m_mtx);
        auto& src = m_stages[stage];
        auto it = which(src.begin(), src.end());

        if (it != src.end())
            return &*it;
    }

    return nullptr;
}
} //namespace MfxEncodeHW