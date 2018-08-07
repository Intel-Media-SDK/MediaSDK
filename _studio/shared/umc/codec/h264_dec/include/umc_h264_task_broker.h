// Copyright (c) 2017 Intel Corporation
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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_TASK_BROKER_H
#define __UMC_H264_TASK_BROKER_H

#include <vector>
#include <list>

#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_heap.h"

namespace UMC
{

class TaskSupplier;
class H264Task;

/****************************************************************************************************/
// TaskBroker
/****************************************************************************************************/
class TaskBroker
{
public:
    TaskBroker(TaskSupplier * pTaskSupplier);

    virtual bool Init(int32_t iConsumerNumber);
    virtual ~TaskBroker();

    virtual bool AddFrameToDecoding(H264DecoderFrame * pFrame);

    virtual bool IsEnoughForStartDecoding(bool force);

    // Get next working task
    virtual bool GetNextTask(H264Task *pTask);

    virtual void Reset();
    virtual void Release();

    // Task was performed
    virtual void AddPerformedTask(H264Task *pTask);

    virtual void Start();

    void Lock();
    void Unlock();

    TaskSupplier * m_pTaskSupplier;

protected:

    bool IsFrameCompleted(H264DecoderFrame * pFrame);

    virtual bool GetNextTaskInternal(H264Task *pTask) = 0;
    virtual bool PrepareFrame(H264DecoderFrame * pFrame) = 0;

    void InitAUs();
    H264DecoderFrameInfo * FindAU();
    void SwitchCurrentAU();
    virtual void CompleteFrame(H264DecoderFrame * frame);
    void RemoveAU(H264DecoderFrameInfo * toRemove);

    int32_t m_iConsumerNumber;

    H264DecoderFrameInfo * m_FirstAU;

    bool m_IsShouldQuit;

    typedef std::list<H264DecoderFrame *> FrameQueue;
    FrameQueue m_decodingQueue;
    FrameQueue m_completedQueue;

    Mutex m_mGuard;
};

} // namespace UMC

#endif // __UMC_H264_TASK_BROKER_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
