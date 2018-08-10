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

#ifndef __UMC_H264_FRAME_LIST_H__
#define __UMC_H264_FRAME_LIST_H__

#include "umc_h264_frame.h"

namespace UMC
{

class H264DecoderFrameList
{
public:
    // Default constructor
    H264DecoderFrameList(void);
    // Destructor
    virtual
    ~H264DecoderFrameList(void);

    H264DecoderFrame   *head() { return m_pHead; }
    H264DecoderFrame   *tail() { return m_pTail; }

    const H264DecoderFrame   *head() const { return m_pHead; }
    const H264DecoderFrame   *tail() const { return m_pTail; }

    bool isEmpty() { return !m_pHead; }

    void swapFrames(H264DecoderFrame *pFrame1, H264DecoderFrame *pFrame2);

    void append(H264DecoderFrame *pFrame);

    int32_t GetFreeIndex()
    {
        for (int32_t i = 0; i < 127; i++)
        {
            H264DecoderFrame *pFrm;

            for (pFrm = head(); pFrm && pFrm->m_index != i; pFrm = pFrm->future())
            {
            }

            if (pFrm == NULL)
            {
                return i;
            }
        }

        VM_ASSERT(false);
        return -1;
    };
protected:

    // Release object
    void Release(void);

    H264DecoderFrame *m_pHead;                          // (H264DecoderFrame *) pointer to first frame in list
    H264DecoderFrame *m_pTail;                          // (H264DecoderFrame *) pointer to last frame in list
};

class H264DBPList : public H264DecoderFrameList
{
public:

    H264DBPList();

    bool IsDisposableExist();

    bool IsAlmostDisposableExist();

    H264DecoderFrame *GetDisposable();
    // Search through the list for the disposable frame to decode into
    // Move disposable frame to tail

    void IncreaseRefPicListResetCount(H264DecoderFrame *excludeFrame);
    // Mark all frames as not used as reference frames.


    H264DecoderFrame * findLongTermRefIdx(int32_t LongTermFrameIdx);

    H264DecoderFrame * findOldLongTermRef(int32_t MaxLongTermFrameIdx);
    // Mark any long-term reference frame with LongTermFrameIdx greater
    // than MaxLongTermFrameIdx as not used.

    H264DecoderFrame * findOldestShortTermRef();

    H264DecoderFrame * findOldestLongTermRef();

    H264DecoderFrame *findShortTermPic(int32_t  picNum, int32_t * field);

    H264DecoderFrame *findLongTermPic(int32_t  picNum, int32_t * field);

    H264DecoderFrame *findInterViewRef(int32_t auIndex, uint32_t bottomFieldFlag);

    uint32_t countAllFrames();

    void countActiveRefs(uint32_t &numShortTerm, uint32_t &numLongTerm);
    // Return number of active int16_t and long term reference frames.

    H264DecoderFrame * findDisplayableByDPBDelay();

    H264DecoderFrame *findOldestDisplayable(int32_t dbpSize);
    // Search through the list for the oldest displayable frame.

    uint32_t countNumDisplayable();
    // Return number of displayable frames.

    H264DecoderFrame * FindClosest(H264DecoderFrame * pFrame);

    H264DecoderFrame * FindByIndex(int32_t index);

    int32_t GetDPBSize() const
    {
        return m_dpbSize;
    }

    void SetDPBSize(int32_t dpbSize)
    {
        m_dpbSize = dpbSize;
    }

    int32_t GetRecoveryFrameCnt() const;

    void SetRecoveryFrameCnt(int32_t recovery_frame_cnt);

    // Reset the buffer and reset every single frame of it
    void Reset(void);

    void DebugPrint();

    // reference list functions
    void InitPSliceRefPicList(H264Slice *slice, H264DecoderFrame **pRefPicList);
    void InitBSliceRefPicLists(H264Slice *slice, H264DecoderFrame **pRefPicList0, H264DecoderFrame **pRefPicList1, H264RefListInfo &rli);
    void AddInterViewRefs(H264Slice *slice, H264DecoderFrame **pRefPicList, ReferenceFlags *pFields, uint32_t listNum, ViewList &views);

protected:
    int32_t m_dpbSize;
    int32_t m_recovery_frame_cnt;
    bool   m_wasRecoveryPointFound;
};

} // end namespace UMC

#endif // __UMC_H264_FRAME_LIST_H__
#endif // MFX_ENABLE_H264_VIDEO_DECODE
