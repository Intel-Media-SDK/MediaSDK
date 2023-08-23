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

#include "umc_h265_frame_list.h"
#include "umc_h265_debug.h"
#include "umc_h265_task_supplier.h"

namespace UMC_HEVC_DECODER
{

H265DecoderFrameList::H265DecoderFrameList(void)
{
    m_pHead = NULL;
    m_pTail = NULL;
} // H265DecoderFrameList::H265DecoderFrameList(void)

H265DecoderFrameList::~H265DecoderFrameList(void)
{
    Release();

} // H265DecoderFrameList::~H265DecoderFrameList(void)

// Destroy frame list
void H265DecoderFrameList::Release(void)
{
    while (m_pHead)
    {
        H265DecoderFrame *pNext = m_pHead->future();
        delete m_pHead;
        m_pHead = pNext;
    }

    m_pHead = NULL;
    m_pTail = NULL;

} // void H265DecoderFrameList::Release(void)


// Appends a new decoded frame buffer to the "end" of the linked list
void H265DecoderFrameList::append(H265DecoderFrame *pFrame)
{
    // Error check
    if (!pFrame)
    {
        // Sent in a NULL frame
        return;
    }

    // Has a list been constructed - is their a head?
    if (!m_pHead)
    {
        // Must be the first frame appended
        // Set the head to the current
        m_pHead = pFrame;
        m_pHead->setPrevious(0);
    }

    if (m_pTail)
    {
        // Set the old tail as the previous for the current
        pFrame->setPrevious(m_pTail);

        // Set the old tail's future to the current
        m_pTail->setFuture(pFrame);
    }
    else
    {
        // Must be the first frame appended
        // Set the tail to the current
        m_pTail = pFrame;
    }

    // The current is now the new tail
    m_pTail = pFrame;
    m_pTail->setFuture(0);
    //
}

H265DBPList::H265DBPList()
    : m_dpbSize(0)
{
}

// Searches DPB for a reusable frame with biggest POC
H265DecoderFrame * H265DBPList::GetOldestDisposable(void)
{
    H265DecoderFrame *pOldest = NULL;
    int32_t  SmallestPicOrderCnt = 0x7fffffff;    // very large positive
    int32_t  LargestRefPicListResetCount = 0;

    for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
    {
        if (pTmp->isDisposable())
        {
            if (pTmp->RefPicListResetCount() > LargestRefPicListResetCount)
            {
                pOldest = pTmp;
                SmallestPicOrderCnt = pTmp->PicOrderCnt();
                LargestRefPicListResetCount = pTmp->RefPicListResetCount();
            }
            else if ((pTmp->PicOrderCnt() < SmallestPicOrderCnt) &&
                     (pTmp->RefPicListResetCount() == LargestRefPicListResetCount))
            {
                pOldest = pTmp;
                SmallestPicOrderCnt = pTmp->PicOrderCnt();
            }
        }
    }

    return pOldest;
} // H265DecoderFrame *H265DBPList::GetDisposable(void)

// Returns whether DPB contains frames which may be reused
bool H265DBPList::IsDisposableExist()
{
    for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
    {
        if (pTmp->isDisposable())
        {
            return true;
        }
    }

    return false;
}

// Returns whether DPB contains frames which may be reused after asynchronous decoding finishes
bool H265DBPList::IsAlmostDisposableExist()
{
    int32_t count = 0;
    for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
    {
        count++;
        if (isAlmostDisposable(pTmp))
        {
            return true;
        }
    }

    return count < m_dpbSize;
}

// Returns first reusable frame in DPB
H265DecoderFrame * H265DBPList::GetDisposable(void)
{
    for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
    {
        if (pTmp->isDisposable())
        {
            return pTmp;
        }
    }

    // We never found one
    return NULL;
} // H265DecoderFrame *H265DBPList::GetDisposable(void)

// Search through the list for the oldest displayable frame. It must be
// not disposable, not outputted, and have smallest PicOrderCnt.
H265DecoderFrame * H265DBPList::findOldestDisplayable(int32_t /*dbpSize*/ )
{
    H265DecoderFrame *pCurr = m_pHead;
    H265DecoderFrame *pOldest = NULL;
    int32_t  SmallestPicOrderCnt = 0x7fffffff;    // very large positive
    int32_t  LargestRefPicListResetCount = 0;
    int32_t  uid = 0x7fffffff;

    while (pCurr)
    {
        if (pCurr->isDisplayable() && !pCurr->wasOutputted())
        {
            // corresponding frame
            if (pCurr->RefPicListResetCount() > LargestRefPicListResetCount)
            {
                pOldest = pCurr;
                SmallestPicOrderCnt = pCurr->PicOrderCnt();
                LargestRefPicListResetCount = pCurr->RefPicListResetCount();
            }
            else if (pCurr->PicOrderCnt() <= SmallestPicOrderCnt && pCurr->RefPicListResetCount() == LargestRefPicListResetCount)
            {
                pOldest = pCurr;
                SmallestPicOrderCnt = pCurr->PicOrderCnt();
            }
        }

        pCurr = pCurr->future();
    }

    if (!pOldest)
        return 0;

    pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr->isDisplayable() && !pCurr->wasOutputted())
        {
            // corresponding frame
            if (pCurr->RefPicListResetCount() == LargestRefPicListResetCount && pCurr->PicOrderCnt() == SmallestPicOrderCnt && pCurr->m_UID < uid)
            {
                pOldest = pCurr;
                SmallestPicOrderCnt = pCurr->PicOrderCnt();
                LargestRefPicListResetCount = pCurr->RefPicListResetCount();
                uid = pCurr->m_UID;
            }
        }

        pCurr = pCurr->future();
    }

    return pOldest;
}    // findOldestDisplayable

// Returns the number of frames in DPB
uint32_t H265DBPList::countAllFrames()
{
    H265DecoderFrame *pCurr = head();
    uint32_t count = 0;

    while (pCurr)
    {
        count++;
        pCurr = pCurr->future();
    }

    return count;
}

void H265DBPList::calculateInfoForDisplay(uint32_t &countDisplayable, uint32_t &countDPBFullness, int32_t &maxUID)
{
    H265DecoderFrame *pCurr = head();

    countDisplayable = 0;
    countDPBFullness = 0;
    maxUID = 0;

    int resetCounter = -1;

    while (pCurr)
    {
        if (pCurr->isDisplayable() && !pCurr->wasOutputted())
        {
            countDisplayable++;
            if (resetCounter == -1)
                resetCounter = pCurr->RefPicListResetCount();
            else
            {
                if (resetCounter != pCurr->RefPicListResetCount()) // DPB contain new IDR and frames from prev sequence
                    countDisplayable += 16;
            }
        }

        if (((pCurr->isShortTermRef() || pCurr->isLongTermRef()) && pCurr->IsFullFrame()) || (pCurr->isDisplayable() && !pCurr->wasOutputted()))
        {
            countDPBFullness++;
            if (maxUID < pCurr->m_UID)
                maxUID = pCurr->m_UID;
        }

        pCurr = pCurr->future();
    }
}    // calculateInfoForDisplay

// Return number of active short and long term reference frames.
void H265DBPList::countActiveRefs(uint32_t &NumShortTerm, uint32_t &NumLongTerm)
{
    H265DecoderFrame *pCurr = m_pHead;
    NumShortTerm = 0;
    NumLongTerm = 0;

    while (pCurr)
    {
        if (pCurr->isShortTermRef())
            NumShortTerm++;
        else if (pCurr->isLongTermRef())
            NumLongTerm++;
        pCurr = pCurr->future();
    }

}    // countActiveRefs

// Marks all frames as not used as reference frames.
void H265DBPList::removeAllRef()
{
    H265DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr->isShortTermRef() || pCurr->isLongTermRef())
        {
            pCurr->SetisLongTermRef(false);
            pCurr->SetisShortTermRef(false);
        }

        pCurr = pCurr->future();
    }

}    // removeAllRef

// Increase ref pic list reset count except for one frame
void H265DBPList::IncreaseRefPicListResetCount(H265DecoderFrame *ExcludeFrame)
{
    H265DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr!=ExcludeFrame)
        {
            pCurr->IncreaseRefPicListResetCount();
        }
        pCurr = pCurr->future();
    }

}    // IncreaseRefPicListResetCount

// Debug print
void H265DBPList::printDPB()
{
    H265DecoderFrame *pCurr = m_pHead;

    DEBUG_PRINT((VM_STRING("DPB: (")));
    while (pCurr)
    {
        DEBUG_PRINT((VM_STRING("POC = %d %p (%d)"), pCurr->PicOrderCnt(), (RefCounter *)pCurr, pCurr->GetRefCounter()));
        pCurr = pCurr->future();
        DEBUG_PRINT((VM_STRING(", ")));
    }
    DEBUG_PRINT((VM_STRING(")\n")));
}

// Searches DPB for a short term reference frame with specified POC
H265DecoderFrame *H265DBPList::findShortRefPic(int32_t picPOC)
{
    H265DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr->isShortTermRef() && pCurr->PicOrderCnt() == picPOC)
            break;

        pCurr = pCurr->future();
    }

    return pCurr;
}

// Searches DPB for a long term reference frame with specified POC
H265DecoderFrame *H265DBPList::findLongTermRefPic(const H265DecoderFrame *excludeFrame, int32_t picPOC, uint32_t bitsForPOC, bool isUseMask) const
{
    H265DecoderFrame *pCurr = m_pHead;
    uint32_t POCmask = (1 << bitsForPOC) - 1;

    if (!isUseMask)
        POCmask = 0xffffffff;

    int32_t excludeUID = excludeFrame ? excludeFrame->m_UID : 0x7fffffff;
    H265DecoderFrame *correctPic = 0;

    while (pCurr)
    {
        if ((pCurr->PicOrderCnt() & POCmask) == (picPOC & POCmask) && pCurr->m_UID < excludeUID)
        {
            if (pCurr->isLongTermRef() && (!correctPic || correctPic->m_UID < pCurr->m_UID))
                correctPic = pCurr;
        }

        pCurr = pCurr->future();
    }

    return correctPic;
} // findLongTermRefPic

// Try to find a frame closest to specified for error recovery
H265DecoderFrame * H265DBPList::FindClosest(H265DecoderFrame * pFrame)
{
    int32_t originalPOC = pFrame->PicOrderCnt();
    int32_t originalResetCount = pFrame->RefPicListResetCount();

    H265DecoderFrame * pOldest = 0;

    int32_t  SmallestPicOrderCnt = 0;    // very large positive
    int32_t  SmallestRefPicListResetCount = 0x7fffffff;

    for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
    {
        if (pTmp == pFrame || !pTmp->IsDecodingCompleted())
            continue;

        if (pTmp->m_chroma_format != pFrame->m_chroma_format ||
            pTmp->lumaSize().width != pFrame->lumaSize().width ||
            pTmp->lumaSize().height != pFrame->lumaSize().height)
            continue;

        if (pTmp->RefPicListResetCount() < SmallestRefPicListResetCount)
        {
            pOldest = pTmp;
            SmallestPicOrderCnt = pTmp->PicOrderCnt();
            SmallestRefPicListResetCount = pTmp->RefPicListResetCount();
        }
        else if (pTmp->RefPicListResetCount() == SmallestRefPicListResetCount)
        {
            if (pTmp->RefPicListResetCount() == originalResetCount)
            {
                if (SmallestRefPicListResetCount != originalResetCount)
                {
                    SmallestPicOrderCnt = 0x7fff;
                }

                if (abs(pTmp->PicOrderCnt() - originalPOC) < SmallestPicOrderCnt)
                {
                    pOldest = pTmp;
                    SmallestPicOrderCnt = pTmp->PicOrderCnt();
                    SmallestRefPicListResetCount = pTmp->RefPicListResetCount();
                }
            }
            else
            {
                if (pTmp->PicOrderCnt() > SmallestPicOrderCnt)
                {
                    pOldest = pTmp;
                    SmallestPicOrderCnt = pTmp->PicOrderCnt();
                    SmallestRefPicListResetCount = pTmp->RefPicListResetCount();
                }
            }
        }
    }

    return pOldest;
}

// Reset the buffer and reset every single frame of it
void H265DBPList::Reset(void)
{
    H265DecoderFrame *pFrame ;

    for (pFrame = head(); pFrame; pFrame = pFrame->future())
    {
        pFrame->FreeResources();
    }

    for (pFrame = head(); pFrame; pFrame = pFrame->future())
    {
        pFrame->Reset();
    }
} // void H265DBPList::Reset(void)

// Debug print
void H265DBPList::DebugPrint()
{
#ifdef ENABLE_TRACE
    Trace(VM_STRING("-==========================================\n"));
    int32_t curID = -1;
    H265DecoderFrame * minTmp = 0;

    for (;;)
    {
        for (H265DecoderFrame * pTmp = m_pHead; pTmp; pTmp = pTmp->future())
        {
            if (pTmp->m_UID > curID)
            {
                if (minTmp && minTmp->m_UID < pTmp->m_UID)
                    continue;
                minTmp = pTmp;
            }
        }

        if (!minTmp)
            break;

        curID = minTmp->m_UID;

        H265DecoderFrame * pTmp = minTmp;

        Trace(VM_STRING("\n\nPTR = %p UID - %d POC - %d  - resetcount - %d, frame ID - %d\n"), (RefCounter *)pTmp, pTmp->m_UID, pTmp->m_PicOrderCnt, pTmp->RefPicListResetCount(), pTmp->GetFrameData()->GetFrameMID());
        Trace(VM_STRING("Short - %d, Long - %d \n"), pTmp->isShortTermRef(), pTmp->isLongTermRef());
        Trace(VM_STRING("Busy - %d, decoded - %d \n"), pTmp->GetRefCounter(), pTmp->IsDecodingCompleted());
        Trace(VM_STRING("Disp - %d , wasOutput - %d wasDisplayed - %d, m_maxUIDWhenWasDisplayed - %d\n"), pTmp->isDisplayable(), pTmp->wasOutputted(), pTmp->wasDisplayed(), pTmp->m_maxUIDWhenWasDisplayed);
        minTmp = 0;
    }

    Trace(VM_STRING("-==========================================\n"));
    //fflush(stdout);
#endif
}


} // end namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
