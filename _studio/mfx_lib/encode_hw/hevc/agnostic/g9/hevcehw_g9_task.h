// Copyright (c) 2019 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_g9_data.h"
#include <mutex>
#include <condition_variable>

namespace HEVCEHW
{
namespace Gen9
{

    class TaskManager
        : public FeatureBase
    {
    public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(Init) \
    DECL_BLOCK(Reset) \
    DECL_BLOCK(NewTask) \
    DECL_BLOCK(PrepareTask) \
    DECL_BLOCK(ReorderTask) \
    DECL_BLOCK(SubmitTask) \
    DECL_BLOCK(QueryTask) \
    DECL_BLOCK(Close)
#define DECL_FEATURE_NAME "G9_TaskManager"
#include "hevcehw_decl_blocks.h"

        TaskManager(mfxU32 id)
            : FeatureBase(id)
            , m_stages(NUM_STAGES)
        {}

    protected:
        std::vector<std::list<StorageRW>> m_stages;
        std::mutex m_mtx, m_closeMtx;
        std::condition_variable m_cv;
        mfxU16 m_nPicBuffered       = 0;
        mfxU16 m_bufferSize         = 0;
        mfxU16 m_maxParallelSubmits = 0;
        mfxU16 m_nTasksInExecution  = 0;
        mfxU16 m_nRecodeTasks       = 0;

        virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
        virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;
        virtual void FrameSubmit(const FeatureBlocks& blocks, TPushFS Push) override;
        virtual void AsyncRoutine(const FeatureBlocks& blocks, TPushAR Push) override;
        virtual void Close(const FeatureBlocks& blocks, TPushCLS Push) override;

        void CancelTasks(StorageW& global, const FeatureBlocks::BQ<FT>::TQueue& qFreeTask);

        static TTaskIt FirstTask(TTaskIt begin, TTaskIt) { return begin; }
        static TTaskIt EndTask(TTaskIt, TTaskIt end) { return end; }
        static TTaskIt CheckTask(TTaskIt b, TTaskIt e, std::function<bool(StorageR&)> cond)
        {
            while (b != e && !cond(*b))
                b++;
            return b;
        }

        using TFnGetTask = std::function<TTaskIt(TTaskIt, TTaskIt)>;

        static TFnGetTask SimpleCheck(std::function<bool(StorageR&)> cond)
        {
            using namespace std::placeholders;
            return std::bind(CheckTask, _1, _2, cond);
        }
        static TFnGetTask FixedTask(const StorageR& task)
        {
            auto pTask = &task;
            return SimpleCheck([pTask](StorageR& b) { return &b == pTask; });
        }

        using FeatureBase::Reset;
        //blocking
        mfxStatus Reset(mfxU32 numTask);
        //non-blocking
        StorageRW* MoveTask(
            mfxU16 from
            , mfxU16 to
            , TFnGetTask which = FirstTask
            , TFnGetTask where = EndTask);
        StorageRW* GetTask(mfxU16 stage, TFnGetTask which = FirstTask);
    };

} //Gen9
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
