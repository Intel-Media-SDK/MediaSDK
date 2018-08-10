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

#include <cstdarg>
#include "umc_h265_task_broker.h"
#include "umc_h265_task_supplier.h"
#include "umc_h265_frame_list.h"

#include "umc_h265_debug.h"


//#define ECHO
//#define ECHO_DEB

#undef DEBUG_PRINT
#undef DEBUG_PRINT1
#define DEBUG_PRINT(x)
#define DEBUG_PRINT1(x)

namespace UMC_HEVC_DECODER
{
H265Slice* FindSliceByCUAddr(H265DecoderFrameInfo * info, int32_t firstCUToProcess);
TileThreadingInfo * FindTileForProcess(H265DecoderFrameInfo * info, int32_t taskID, bool isDecRec = false);


TaskBroker_H265::TaskBroker_H265(TaskSupplier_H265 * pTaskSupplier)
    : m_pTaskSupplier(pTaskSupplier)
    , m_iConsumerNumber(0)
    , m_FirstAU(0)
    , m_IsShouldQuit(false)
{
    Release();
}

TaskBroker_H265::~TaskBroker_H265()
{
    Release();
}

// Initialize task broker with threads number
bool TaskBroker_H265::Init(int32_t iConsumerNumber)
{
    Release();

    // we keep this variable due some optimizations purposes
    m_iConsumerNumber = iConsumerNumber;
    m_IsShouldQuit = false;
    return true;
}

// Reset to default values, stop all activity
void TaskBroker_H265::Reset()
{
    UMC::AutomaticUMCMutex guard(m_mGuard);
    m_FirstAU = 0;
    m_IsShouldQuit = true;

    m_decodingQueue.clear();
    m_completedQueue.clear();
}

// Release resources
void TaskBroker_H265::Release()
{
    Reset();
}

// Lock synchronization mutex
void TaskBroker_H265::Lock()
{
    m_mGuard.Lock();
}

// Uncock synchronization mutex
void TaskBroker_H265::Unlock()
{
    m_mGuard.Unlock();
}

// Check whether frame is prepared
bool TaskBroker_H265::PrepareFrame(H265DecoderFrame * pFrame)
{
    if (!pFrame || pFrame->prepared)
    {
        return true;
    }

    if (pFrame->prepared)
        return true;

    if (pFrame->GetAU()->GetStatus() == H265DecoderFrameInfo::STATUS_FILLED || pFrame->GetAU()->GetStatus() == H265DecoderFrameInfo::STATUS_STARTED)
    {
        pFrame->prepared = true;
    }

    DEBUG_PRINT((VM_STRING("Prepare frame - %d\n"), pFrame->m_PicOrderCnt));

    return true;
}

// Add frame to decoding queue
bool TaskBroker_H265::AddFrameToDecoding(H265DecoderFrame * frame)
{
    if (!frame || frame->IsDecodingStarted() || !IsExistTasks(frame))
        return false;

    UMC::AutomaticUMCMutex guard(m_mGuard);


    m_decodingQueue.push_back(frame);
    frame->StartDecoding();
    return true;
}

// Remove access unit from the linked list of frames
void TaskBroker_H265::RemoveAU(H265DecoderFrameInfo * toRemove)
{
    H265DecoderFrameInfo * temp = m_FirstAU;

    if (!temp)
        return;

    H265DecoderFrameInfo * reference = 0;

    for (; temp; temp = temp->GetNextAU())
    {
        if (temp == toRemove)
            break;

        if (temp->IsReference())
            reference = temp;
    }

    if (!temp)
        return;

    if (temp->GetPrevAU())
        temp->GetPrevAU()->SetNextAU(temp->GetNextAU());

    if (temp->GetNextAU())
        temp->GetNextAU()->SetPrevAU(temp->GetPrevAU());

    H265DecoderFrameInfo * temp1 = temp->GetNextAU();

    temp->SetPrevAU(0);
    temp->SetNextAU(0);

    if (temp == m_FirstAU)
        m_FirstAU = temp1;

    if (temp1)
    {
        temp = temp1;

        for (; temp; temp = temp->GetNextAU())
        {
            if (temp->GetRefAU())
                temp->SetRefAU(reference);

            if (temp->IsReference())
                break;
        }
    }
}

// Finish frame decoding
void TaskBroker_H265::CompleteFrame(H265DecoderFrame * frame)
{
    if (!frame || m_decodingQueue.empty() || !frame->IsFullFrame())
        return;

    if (!IsFrameCompleted(frame) || frame->IsDecodingCompleted())
        return;

    if (frame == m_decodingQueue.front())
    {
        RemoveAU(frame->GetAU());
        m_decodingQueue.pop_front();
    }
    else
    {
        RemoveAU(frame->GetAU());
        m_decodingQueue.remove(frame);
    }

    frame->CompleteDecoding();
}

// Update access units list finishing completed frames
void TaskBroker_H265::SwitchCurrentAU()
{
    if (!m_FirstAU || !m_FirstAU->IsCompleted())
        return;

    while (m_FirstAU && m_FirstAU->IsCompleted())
    {
        DEBUG_PRINT((VM_STRING("Switch current AU - is_completed %d, poc - %d, m_FirstAU - %X\n"), m_FirstAU->IsCompleted(), m_FirstAU->m_pFrame->m_PicOrderCnt, m_FirstAU));

        H265DecoderFrameInfo* completed = m_FirstAU;
        m_FirstAU = m_FirstAU->GetNextAU();
        CompleteFrame(completed->m_pFrame);
    }

    InitAUs();
}

// Wakes up working threads to start working on new tasks
void TaskBroker_H265::Start()
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    FrameQueue::iterator iter = m_decodingQueue.begin();

    for (; iter != m_decodingQueue.end(); ++iter)
    {
        H265DecoderFrame * frame = *iter;

        if (IsFrameCompleted(frame))
        {
            CompleteFrame(frame);
            iter = m_decodingQueue.begin();
            if (iter == m_decodingQueue.end()) // avoid ++iter operation
                break;
        }
    }

    InitAUs();
    DEBUG_PRINT((VM_STRING("Start\n")));
}

// Find an access unit which has all slices found
H265DecoderFrameInfo * TaskBroker_H265::FindAU()
{
    FrameQueue::iterator iter = m_decodingQueue.begin();
    FrameQueue::iterator end_iter = m_decodingQueue.end();

    for (; iter != end_iter; ++iter)
    {
        H265DecoderFrame * frame = *iter;

        H265DecoderFrameInfo *slicesInfo = frame->GetAU();

        if (slicesInfo->GetSliceCount())
        {
            if (slicesInfo->GetStatus() == H265DecoderFrameInfo::STATUS_FILLED)
                return slicesInfo;
        }
    }

    return 0;
}

// Try to find an access unit which to decode next
void TaskBroker_H265::InitAUs()
{
    H265DecoderFrameInfo * pPrev;
    H265DecoderFrameInfo * refAU;

    if (!m_FirstAU)
    {
        m_FirstAU = FindAU();
        if (!m_FirstAU)
            return;

        if (!PrepareFrame(m_FirstAU->m_pFrame))
        {
            m_FirstAU = 0;
            return;
        }

        m_FirstAU->SetStatus(H265DecoderFrameInfo::STATUS_STARTED);

        pPrev = m_FirstAU;
        m_FirstAU->SetPrevAU(0);
        m_FirstAU->SetNextAU(0);
        m_FirstAU->SetRefAU(0);

        refAU = 0;
        if (m_FirstAU->IsReference())
        {
            refAU = m_FirstAU;
        }
    }
    else
    {
        pPrev = m_FirstAU;
        refAU = 0;

        pPrev->SetPrevAU(0);
        pPrev->SetRefAU(0);

        if (m_FirstAU->IsReference())
        {
            refAU = pPrev;
        }

        while (pPrev->GetNextAU())
        {
            pPrev = pPrev->GetNextAU();

            if (!refAU)
                pPrev->SetRefAU(0);

            if (pPrev->IsReference())
            {
                refAU = pPrev;
            }
        };
    }

    H265DecoderFrameInfo * pTemp = FindAU();
    for (; pTemp; )
    {
        if (!PrepareFrame(pTemp->m_pFrame))
        {
            pPrev->SetNextAU(0);
            break;
        }

        pTemp->SetStatus(H265DecoderFrameInfo::STATUS_STARTED);
        pTemp->SetNextAU(0);
        pTemp->SetPrevAU(pPrev);

        pTemp->SetRefAU(refAU);

        if (pTemp->IsReference())
        {
            refAU = pTemp;
        }

        pPrev->SetNextAU(pTemp);
        pPrev = pTemp;
        pTemp = FindAU();
    }
}

// Returns whether frame decoding is finished
bool TaskBroker_H265::IsFrameCompleted(H265DecoderFrame * pFrame) const
{
    if (!pFrame)
        return true;

    H265DecoderFrameInfo::FillnessStatus status = pFrame->GetAU()->GetStatus();

    bool ret;
    switch (status)
    {
    case H265DecoderFrameInfo::STATUS_NONE:
        ret = true;
        break;
    case H265DecoderFrameInfo::STATUS_NOT_FILLED:
        ret = false;
        break;
    case H265DecoderFrameInfo::STATUS_COMPLETED:
        ret = true;
        break;
    default:
        ret = pFrame->GetAU()->IsCompleted();
        break;
    }

    return ret;
}

// Tries to find a new task for asynchronous processing
bool TaskBroker_H265::GetNextTask(H265Task *pTask)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);


    bool res = GetNextTaskInternal(pTask);

    return res;
} // bool TaskBroker_H265::GetNextTask(H265Task *pTask)

// Calculate frame state after a task has been finished
void TaskBroker_H265::AddPerformedTask(H265Task *)
{
}

#define Check_Status(status)  ((status == H265DecoderFrameInfo::STATUS_FILLED) || (status == H265DecoderFrameInfo::STATUS_STARTED))

// Returns number of access units available in the list but not processed yet
int32_t TaskBroker_H265::GetNumberOfTasks(void)
{
    int32_t au_count = 0;

    H265DecoderFrameInfo * temp = m_FirstAU;

    for (; temp ; temp = temp->GetNextAU())
    {
        if (temp->GetStatus() == H265DecoderFrameInfo::STATUS_COMPLETED)
            continue;

        au_count += 2;
    }

    return au_count;
}

// Returns whether enough bitstream data is evailable to start an asynchronous task
bool TaskBroker_H265::IsEnoughForStartDecoding(bool )
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    InitAUs();
    return m_FirstAU != 0;
}

// Returns whether there is some work available for specified frame
bool TaskBroker_H265::IsExistTasks(H265DecoderFrame * frame)
{
    H265DecoderFrameInfo *slicesInfo = frame->GetAU();

    return Check_Status(slicesInfo->GetStatus());
}


} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
