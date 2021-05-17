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

#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include "feature_blocks/mfx_feature_blocks_utils.h"

namespace MfxEncodeHW
{
    using namespace MfxFeatureBlocks;

    class TaskManager
    {
    public:
        using TTaskList  = std::list<StorageRW>;
        using TTaskIt    = TTaskList::iterator;
        using TFnGetTask = std::function<TTaskIt(TTaskIt, TTaskIt)>;

        virtual ~TaskManager() {}

        virtual mfxU32        GetNumTask           () const = 0;
        virtual mfxU16        GetBufferSize        () const = 0;
        virtual mfxU16        GetMaxParallelSubmits() const = 0;
        virtual void          SetActiveTask        (StorageW& /*task*/) = 0;
        virtual bool          IsInputTask          (const StorageR& /*task*/) const = 0;
        virtual mfxU32        GetStage             (const StorageR& /*task*/) const = 0;
        virtual void          SetStage             (StorageW& /*task*/, mfxU32) const = 0;
        virtual bool          IsReorderBypass      () const = 0;
        virtual TTaskIt       GetNextTaskToEncode  (TTaskIt /*begin*/, TTaskIt /*end*/, bool /*bFlush*/) = 0;
        virtual bool          IsForceSync          (const StorageR& /*task*/) const = 0;
        virtual mfxBitstream* GetBS                (const StorageR& /*task*/) const = 0;
        virtual void          SetBS                (StorageW& /*task*/, mfxBitstream* /*pBS*/) const = 0;
        virtual bool          GetRecode            (const StorageR& /*task*/) const = 0;
        virtual void          SetRecode            (StorageW& /*task*/, bool) const = 0;
        virtual mfxU32        GetBsDataLength      (const StorageR& /*task*/) const = 0;
        virtual void          SetBsDataLength      (StorageW& /*task*/, mfxU32) const = 0;
        virtual void          AddNumRecode         (StorageW& /*task*/, mfxU16) const = 0;

        virtual mfxStatus RunQueueTaskAlloc(StorageRW& /*task*/) = 0;
        virtual mfxStatus RunQueueTaskInit(
            mfxEncodeCtrl* /*pCtrl*/
            , mfxFrameSurface1* /*pSurf*/
            , mfxBitstream* /*pBs*/
            , StorageW& /*task*/) = 0;
        virtual mfxStatus RunQueueTaskPreReorder(StorageW& /*task*/) = 0;
        virtual mfxStatus RunQueueTaskPostReorder(StorageW& /*task*/) = 0;
        virtual mfxStatus RunQueueTaskSubmit(StorageW& /*task*/) = 0;
        virtual bool RunQueueTaskQuery(
            StorageW& /*task*/
            , std::function<bool(const mfxStatus&)> /*stopAt*/) = 0;
        virtual mfxStatus RunQueueTaskFree(StorageW& /*task*/) = 0;

        virtual mfxStatus ManagerInit();
        virtual mfxStatus TaskNew(
            mfxEncodeCtrl* /*pCtrl*/
            , mfxFrameSurface1* /*pSurf*/
            , mfxBitstream& /*bs*/);
        virtual mfxStatus TaskPrepare(StorageW& /*task*/);
        virtual mfxStatus TaskReorder(StorageW& /*task*/);
        virtual mfxStatus TaskSubmit(StorageW& /*task*/);
        virtual mfxStatus TaskQuery(StorageW& /*task*/);
        virtual void CancelTasks();

    protected:

        static constexpr mfxU16 S_NEW      = 0;
        static constexpr mfxU16 S_PREPARE  = 1;
        static constexpr mfxU16 S_REORDER  = 2;
        static constexpr mfxU16 S_SUBMIT   = 3;
        static constexpr mfxU16 S_QUERY    = 4;

        std::map<mfxU16,mfxU16> m_stageID =
        {
              {S_NEW,       S_NEW}
            , {S_PREPARE,   S_PREPARE}
            , {S_REORDER,   S_REORDER}
            , {S_SUBMIT,    S_SUBMIT}
            , {S_QUERY,     S_QUERY}
        };
        std::vector<TTaskList>  m_stages             = std::vector<TTaskList>(5);
        mfxU16                  m_nPicBuffered       = 0;
        mfxU16                  m_bufferSize         = 0;
        mfxU16                  m_maxParallelSubmits = 0;
        mfxU16                  m_nTasksInExecution  = 0;
        mfxU16                  m_nRecodeTasks       = 0;
        bool                    m_bPostponeQuery     = false;
        std::mutex              m_mtx, m_closeMtx;
        std::condition_variable m_cv;

        static TTaskIt    FirstTask     (TTaskIt begin, TTaskIt) { return begin; }
        static TTaskIt    EndTask       (TTaskIt, TTaskIt end) { return end; }
        static TFnGetTask SimpleCheck   (std::function<bool(StorageR&)> cond)
        {
            using namespace std::placeholders;
            return std::bind(std::find_if<TTaskIt, std::function<bool(StorageR&)>>, _1, _2, cond);
        }
        static TFnGetTask FixedTask(const StorageR& task)
        {
            auto pTask = &task;
            return SimpleCheck([pTask](StorageR& b) { return &b == pTask; });
        }
        mfxU16 Stage(mfxU16 s) { return m_stageID.at(s); }

        //blocking
        mfxStatus ManagerReset(mfxU32 numTask);
        //non-blocking
        StorageRW* MoveTask(
            mfxU16 from
            , mfxU16 to
            , TFnGetTask which = FirstTask
            , TFnGetTask where = EndTask);
        StorageRW* GetTask(mfxU16 stage, TFnGetTask which = FirstTask);
    };
}