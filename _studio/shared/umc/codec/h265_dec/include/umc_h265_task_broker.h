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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_TASK_BROKER_H
#define __UMC_H265_TASK_BROKER_H

#include <vector>
#include <list>

#include "umc_h265_dec_defs.h"
#include "umc_h265_heap.h"
#include "umc_h265_segment_decoder_base.h"

namespace UMC_HEVC_DECODER
{
class H265DecoderFrameInfo;
class H265DecoderFrameList;
class H265Slice;
class TaskSupplier_H265;

class DecodingContext;
struct TileThreadingInfo;

class H265Task;

// Decoder task scheduler class
class TaskBroker_H265
{
public:
    TaskBroker_H265(TaskSupplier_H265 * pTaskSupplier);

    // Initialize task broker with threads number
    virtual bool Init(int32_t iConsumerNumber);
    virtual ~TaskBroker_H265();

    // Add frame to decoding queue
    virtual bool AddFrameToDecoding(H265DecoderFrame * pFrame);

    // Returns whether enough bitstream data is evailable to start an asynchronous task
    virtual bool IsEnoughForStartDecoding(bool force);
    // Returns whether there is some work available for specified frame
    bool IsExistTasks(H265DecoderFrame * frame);

    // Tries to find a new task for asynchronous processing
    virtual bool GetNextTask(H265Task *pTask);

    // Reset to default values, stop all activity
    virtual void Reset();
    // Release resources
    virtual void Release();

    // Calculate frame state after a task has been finished
    virtual void AddPerformedTask(H265Task *pTask);

    // Wakes up working threads to start working on new tasks
    virtual void Start();

    // Check whether frame is prepared
    virtual bool PrepareFrame(H265DecoderFrame * pFrame);

    // Lock synchronization mutex
    void Lock();
    // Unlock synchronization mutex
    void Unlock();

    TaskSupplier_H265 * m_pTaskSupplier;

protected:

    // Returns number of access units available in the list but not processed yet
    int32_t GetNumberOfTasks(void);
    // Returns whether frame decoding is finished
    bool IsFrameCompleted(H265DecoderFrame * pFrame) const;

    virtual bool GetNextTaskInternal(H265Task *)
    {
        return false;
    }

    // Try to find an access unit which to decode next
    void InitAUs();
    // Find an access unit which has all slices found
    H265DecoderFrameInfo * FindAU();
    void SwitchCurrentAU();
    // Finish frame decoding
    virtual void CompleteFrame(H265DecoderFrame * frame);
    // Remove access unit from the linked list of frames
    void RemoveAU(H265DecoderFrameInfo * toRemove);

    int32_t m_iConsumerNumber;

    H265DecoderFrameInfo * m_FirstAU;

    bool m_IsShouldQuit;

    typedef std::list<H265DecoderFrame *> FrameQueue;
    FrameQueue m_decodingQueue;
    FrameQueue m_completedQueue;

    UMC::Mutex m_mGuard;
};


} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_TASK_BROKER_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
