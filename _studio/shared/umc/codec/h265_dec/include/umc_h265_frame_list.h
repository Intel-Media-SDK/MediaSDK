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

#ifndef __UMC_H265_FRAME_LIST_H__
#define __UMC_H265_FRAME_LIST_H__

#include "umc_h265_frame.h"

namespace UMC_HEVC_DECODER
{

class H265DecoderFrameList
{
public:
    // Default constructor
    H265DecoderFrameList(void);
    // Destructor
    virtual
    ~H265DecoderFrameList(void);

    H265DecoderFrame   *head() { return m_pHead; }
    H265DecoderFrame   *tail() { return m_pTail; }

    const H265DecoderFrame   *head() const { return m_pHead; }
    const H265DecoderFrame   *tail() const { return m_pTail; }

    bool isEmpty() { return !m_pHead; }

    void append(H265DecoderFrame *pFrame);
    // Append the given frame to our tail

    int32_t GetFreeIndex()
    {
        for(int32_t i = 0; i < 128; i++)
        {
            H265DecoderFrame *pFrm;

            for (pFrm = head(); pFrm && pFrm->m_index != i; pFrm = pFrm->future())
            {}

            if(pFrm == NULL)
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

    H265DecoderFrame *m_pHead;                          // (H265DecoderFrame *) pointer to first frame in list
    H265DecoderFrame *m_pTail;                          // (H265DecoderFrame *) pointer to last frame in list
};

class H265DBPList : public H265DecoderFrameList
{
public:

    H265DBPList();

    // Searches DPB for a reusable frame with biggest POC
    H265DecoderFrame * GetOldestDisposable();

    // Returns whether DPB contains frames which may be reused
    bool IsDisposableExist();

    // Returns whether DPB contains frames which may be reused after asynchronous decoding finishes
    bool IsAlmostDisposableExist();

    // Returns first reusable frame in DPB
    H265DecoderFrame *GetDisposable(void);

    // Marks all frames as not used as reference frames.
    void removeAllRef();

    // Increase ref pic list reset count except for one frame
    void IncreaseRefPicListResetCount(H265DecoderFrame *excludeFrame);

    // Searches DPB for a short term reference frame with specified POC
    H265DecoderFrame *findShortRefPic(int32_t picPOC);

    // Searches DPB for a long term reference frame with specified POC
    H265DecoderFrame *findLongTermRefPic(const H265DecoderFrame *excludeFrame, int32_t picPOC, uint32_t bitsForPOC, bool isUseMask) const;

    // Returns the number of frames in DPB
    uint32_t countAllFrames();

    // Return number of active short and long term reference frames.
    void countActiveRefs(uint32_t &numShortTerm, uint32_t &numLongTerm);

    // Search through the list for the oldest displayable frame.
    H265DecoderFrame *findOldestDisplayable(int32_t dbpSize);

    void calculateInfoForDisplay(uint32_t &countDisplayable, uint32_t &countDPBFullness, int32_t &maxUID);

    // Try to find a frame closest to specified for error recovery
    H265DecoderFrame * FindClosest(H265DecoderFrame * pFrame);

    int32_t GetDPBSize() const
    {
        return m_dpbSize;
    }

    void SetDPBSize(int32_t dpbSize)
    {
        m_dpbSize = dpbSize;
    }

    // Reset the buffer and reset every single frame of it
    void Reset(void);

    // Debug print
    void DebugPrint();
    // Debug print
    void printDPB();

protected:
    int32_t m_dpbSize;
};

} // end namespace UMC_HEVC_DECODER

#endif // __UMC_H265_FRAME_LIST_H__
#endif // MFX_ENABLE_H265_VIDEO_DECODE
