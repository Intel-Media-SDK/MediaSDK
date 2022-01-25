// Copyright (c) 2017-2019 Intel Corporation
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

#ifndef __UMC_H265_FRAME_H__
#define __UMC_H265_FRAME_H__

#include <stdlib.h>
#include "umc_h265_yuv.h"
#include "umc_h265_notify.h"
#include "umc_h265_heap.h"


namespace UMC_HEVC_DECODER
{
class H265Slice;
class H265DecoderFrameInfo;
class H265CodingUnit;

// Struct containing list 0 and list 1 reference picture lists for one slice.
// Length is plus 1 to provide for null termination.
class H265DecoderRefPicList
{
public:

    struct ReferenceInformation // flags use for reference frames of slice
    {
        H265DecoderFrame * refFrame;
        bool isLongReference;
    };

    ReferenceInformation *m_refPicList;

    H265DecoderRefPicList()
    {
        memset(m_refPicList1, 0, sizeof(m_refPicList1));

        m_refPicList = &(m_refPicList1[1]);

        m_refPicList1[0].refFrame = 0;
        m_refPicList1[0].isLongReference = 0;
    }

    void Reset()
    {
        memset(&m_refPicList1, 0, sizeof(m_refPicList1));
    }

    H265DecoderRefPicList (const H265DecoderRefPicList& copy)
    {
        m_refPicList = &(m_refPicList1[1]);
        MFX_INTERNAL_CPY(&m_refPicList1, &copy.m_refPicList1, sizeof(m_refPicList1));
    }

    H265DecoderRefPicList& operator=(const H265DecoderRefPicList & copy)
    {
        MFX_INTERNAL_CPY(&m_refPicList1, &copy.m_refPicList1, sizeof(m_refPicList1));
        return *this;
    }

private:
    ReferenceInformation m_refPicList1[MAX_NUM_REF_PICS + 3];
};

// HEVC decoder frame class
class H265DecoderFrame : public H265DecYUVBufferPadded, public RefCounter
{
public:

    int32_t  m_PicOrderCnt;    // Display order picture count mod MAX_PIC_ORDER_CNT.

    int32_t  m_frameOrder;
    int32_t  m_ErrorType;

    bool prepared;

    H265DecoderFrameInfo * GetAU() const
    {
        return m_pSlicesInfo;
    }

    H265DecoderFrame *m_pPreviousFrame;
    H265DecoderFrame *m_pFutureFrame;

    H265SEIPayLoad m_UserData;
    H265SEIPayLoad m_mastering_display;
    H265SEIPayLoad m_content_light_level_info;

    double           m_dFrameTime;
    bool             m_isOriginalPTS;

    DisplayPictureStruct_H265  m_DisplayPictureStruct_H265;

    // For type 1 calculation of m_PicOrderCnt. m_FrameNum is needed to
    // be used as previous frame num.

    bool post_procces_complete;

    int32_t m_index;
    int32_t m_UID;
    UMC::FrameType m_FrameType;

    UMC::MemID m_MemID;

    int32_t           m_RefPicListResetCount;
    int32_t           m_crop_left;
    int32_t           m_crop_right;
    int32_t           m_crop_top;
    int32_t           m_crop_bottom;
    int32_t           m_crop_flag;

    int32_t           m_aspect_width;
    int32_t           m_aspect_height;

    bool             m_pic_output;

    bool             m_isShortTermRef;
    bool             m_isLongTermRef;
    bool             m_isUsedAsReference;

    // Returns whether frame has all slices found
    bool IsFullFrame() const;
    // Set frame flag denoting that all slices for it were found
    void SetFullFrame(bool isFull);

    struct
    {
        uint8_t  isFull    : 1;
        uint8_t  isDecoded : 1;
        uint8_t  isDecodingStarted : 1;
        uint8_t  isDecodingCompleted : 1;
    } m_Flags;

    uint8_t  m_isDisplayable;
    uint8_t  m_wasDisplayed;
    uint8_t  m_wasOutputted;
    int32_t m_maxUIDWhenWasDisplayed;

    typedef std::list<RefCounter *>  ReferenceList;
    ReferenceList m_references;

    // Add target frame to the list of reference frames
    void AddReferenceFrame(H265DecoderFrame * frm);
    // Clear all references to other frames
    void FreeReferenceFrames();

    // Reinitialize frame structure before reusing frame
    void Reset();
    // Clean up data after decoding is done
    void FreeResources();

    // Accelerator for getting 'surface Index' FrameMID
    inline int32_t GetFrameMID() const
    {
        return m_frameData.GetFrameMID();
    }
public:
    // Delete unneeded references and set flags after decoding is done
    void OnDecodingCompleted();

    // Free resources if possible
    virtual void Free();

    // Returns whether frame has been decoded
    bool IsDecoded() const;

    H265DecoderFrame(UMC::MemoryAllocator *pMemoryAllocator, Heap_Objects * pObjHeap);

    virtual ~H265DecoderFrame();

    // The following methods provide access to the HEVC Decoder's doubly
    // linked list of H265DecoderFrames.  Note that m_pPreviousFrame can
    // be non-NULL even for an I frame.
    H265DecoderFrame *future() const  { return m_pFutureFrame; }

    void setPrevious(H265DecoderFrame *pPrev)
    {
        m_pPreviousFrame = pPrev;
    }

    void setFuture(H265DecoderFrame *pFut)
    {
        m_pFutureFrame = pFut;
    }

    bool        isDisplayable()    { return m_isDisplayable != 0; }

    void        SetisDisplayable(bool isDisplayable)
    {
        m_isDisplayable = isDisplayable ? 1 : 0;
    }

    bool IsDecodingStarted() const { return m_Flags.isDecodingStarted != 0;}
    void StartDecoding() { m_Flags.isDecodingStarted = 1;}

    bool IsDecodingCompleted() const { return m_Flags.isDecodingCompleted != 0;}
    // Flag frame as completely decoded
    void CompleteDecoding();

    // Check reference frames for error status and flag this frame if error is found
    void UpdateErrorWithRefFrameStatus();

    bool        wasDisplayed()    { return m_wasDisplayed != 0; }
    void        setWasDisplayed() { m_wasDisplayed = 1; }

    bool        wasOutputted()    { return m_wasOutputted != 0; }
    void        setWasOutputted(); // Flag frame after it was output

    bool        isDisposable()    { return (!m_isShortTermRef &&
                                            !m_isLongTermRef &&
                                            ((m_wasOutputted != 0 && m_wasDisplayed != 0) || (m_isDisplayable == 0)) &&
                                            !GetRefCounter()); }

    bool isShortTermRef() const
    {
        return m_isShortTermRef;
    }

    // Mark frame as short term reference frame
    void SetisShortTermRef(bool isRef);

    int32_t PicOrderCnt() const
    {
        return m_PicOrderCnt;
    }
    void setPicOrderCnt(int32_t PicOrderCnt)
    {
        m_PicOrderCnt = PicOrderCnt;
    }

    bool isLongTermRef() const
    {
        return m_isLongTermRef;
    }

    // Mark frame as long term reference frame
    void SetisLongTermRef(bool isRef);

    void IncreaseRefPicListResetCount()
    {
        m_RefPicListResetCount++;
    }

    void InitRefPicListResetCount()
    {
        m_RefPicListResetCount = 0;
    }

    int32_t RefPicListResetCount() const
    {
        return m_RefPicListResetCount;
    }

    // GetRefPicList
    // Returns pointer to start of specified ref pic list.
    H265_FORCEINLINE const H265DecoderRefPicList* GetRefPicList(int32_t sliceNumber, int32_t list) const
    {
        VM_ASSERT(list <= REF_PIC_LIST_1 && list >= 0);

        if (sliceNumber >= (int32_t)m_refPicList.size())
        {
            return 0;
        }

        return &m_refPicList[sliceNumber].m_refPicList[list];
    }

    int32_t GetError() const
    {
        return m_ErrorType;
    }

    void SetError(int32_t errorType)
    {
        m_ErrorType = errorType;
    }

    void SetErrorFlagged(int32_t errorType)
    {
        m_ErrorType |= errorType;
    }



    void AddSlice(H265Slice * pSlice);

protected:
    H265DecoderFrameInfo * m_pSlicesInfo;

    // Declare memory management tools
    UMC::MemoryAllocator *m_pMemoryAllocator;   // FIXME: should be removed because it duplicated in base class

    Heap_Objects * m_pObjHeap;

    struct H265DecoderRefPicListPair
    {
    public:
        H265DecoderRefPicList m_refPicList[2];
    };

    // ML: OPT: TODO: std::vector<> results with relatively slow access code
    std::vector<H265DecoderRefPicListPair> m_refPicList;

    class FakeFrameInitializer
    {
    public:
        FakeFrameInitializer();
    };

    static FakeFrameInitializer g_FakeFrameInitializer;

    bool CheckReferenceFrameError();
};

// Returns if frame is not needed by decoder
inline bool isAlmostDisposable(H265DecoderFrame * pTmp)
{
    return (!pTmp->m_isShortTermRef &&
        !pTmp->m_isLongTermRef &&
        ((pTmp->m_wasOutputted != 0) || (pTmp->m_isDisplayable == 0)) &&
        !pTmp->GetRefCounter());
}

// Returns if frame is not needed by decoder
inline bool isInDisplayngStage(H265DecoderFrame * pTmp)
{
    return (pTmp->m_isDisplayable && pTmp->m_wasOutputted && !pTmp->m_wasDisplayed);
}

} // end namespace UMC_HEVC_DECODER

#endif // __UMC_H265_FRAME_H__
#endif // MFX_ENABLE_H265_VIDEO_DECODE
