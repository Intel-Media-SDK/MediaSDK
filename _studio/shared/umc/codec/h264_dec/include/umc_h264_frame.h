// Copyright (c) 2018-2019 Intel Corporation
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

#ifndef __UMC_H264_FRAME_H__
#define __UMC_H264_FRAME_H__

#include "umc_h264_dec_defs_yuv.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_frame_info.h"

namespace UMC
{
class H264DecoderFrameInfo;
class H264StreamOut;

enum BusyStates
{
    BUSY_STATE_FREE = 0,
    BUSY_STATE_LOCK1 = 1,
    BUSY_STATE_LOCK2 = 2,
    BUSY_STATE_NOT_DECODED = 3
};

class H264DecoderFrame
    : public H264DecYUVBufferPadded
    , public RefCounter
{
    DYNAMIC_CAST_DECL(H264DecoderFrame, H264DecYUVBufferPadded)

    int32_t  m_PictureStructureForRef;
    int32_t  m_PicOrderCnt[2];    // Display order picture count mod MAX_PIC_ORDER_CNT.
    int32_t  m_bottom_field_flag[2];
    int32_t  m_PicNum[2];
    int32_t  m_LongTermPicNum[2];
    int32_t  m_FrameNum;
    int32_t  m_FrameNumWrap;
    int32_t  m_LongTermFrameIdx;
    // ID of view the frame belongs to
    int32_t m_viewId;

    int32_t  m_TopSliceCount;

    int32_t  m_frameOrder;
    int32_t  m_ErrorType;

    H264DecoderFrameInfo m_pSlicesInfo;
    H264DecoderFrameInfo m_pSlicesInfoBottom;

    bool prepared[2];

    H264DecoderFrameInfo * GetAU(int32_t field = 0)
    {
        return (field) ? &m_pSlicesInfoBottom : &m_pSlicesInfo;
    }

    const H264DecoderFrameInfo * GetAU(int32_t field = 0) const
    {
        return (field) ? &m_pSlicesInfoBottom : &m_pSlicesInfo;
    }

    H264DecoderFrame *m_pPreviousFrame;
    H264DecoderFrame *m_pFutureFrame;

    UMC_H264_DECODER::H264SEIPayLoad m_UserData;

    double           m_dFrameTime;
    bool             m_isOriginalPTS;

    bool             m_IsFrameExist;

    int32_t           m_dpb_output_delay;
    int32_t           m_PictureStructureForDec;
    DisplayPictureStruct  m_displayPictureStruct;

    int32_t           totalMBs;

    // For type 1 calculation of m_PicOrderCnt. m_FrameNum is needed to
    // be used as previous frame num.

    bool post_procces_complete;

    int32_t m_iNumberOfSlices;
    int32_t m_iResourceNumber;

    int32_t m_auIndex;
    int32_t m_index;
    int32_t m_UID;
    size_t m_ID[2];
    FrameType m_FrameType;

    MemID m_MemID;

    int32_t           m_RefPicListResetCount[2];
    int32_t           m_crop_left;
    int32_t           m_crop_right;
    int32_t           m_crop_top;
    int32_t           m_crop_bottom;
    int32_t           m_crop_flag;

    int32_t           m_aspect_width;
    int32_t           m_aspect_height;

    bool             m_isShortTermRef[2];
    bool             m_isLongTermRef[2];
    bool             m_isInterViewRef[2];

    bool             m_bIDRFlag;
    bool             m_bIFlag;

    bool IsFullFrame() const;
    void SetFullFrame(bool isFull);

    uint8_t  m_isFull;
    uint8_t  m_isDecoded;
    uint8_t  m_isDecodingStarted;
    uint8_t  m_isDecodingCompleted;
    uint8_t  m_isSkipped;

    uint8_t  m_wasDisplayed;
    uint8_t  m_wasOutputted;

    typedef std::list<RefCounter *>  ReferenceList;
    ReferenceList m_references;

    void FreeReferenceFrames();

    void Reset();
    virtual void FreeResources();

    H264DecoderFrame( const H264DecoderFrame &s );                 // no copy CTR
    H264DecoderFrame & operator=(const H264DecoderFrame &s );

public:

    void AddReference(RefCounter * reference);

    void OnDecodingCompleted();

    virtual void Free();

    void SetSkipped(bool isSkipped)
    {
        m_isSkipped = (uint8_t) (isSkipped ? 1 : 0);
    }

    bool IsSkipped() const
    {
        return m_isSkipped != 0;
    }

    bool IsDecoded() const;

    bool IsFrameExist() const
    {
        return m_IsFrameExist;
    }

    void SetFrameAsNonExist()
    {
        m_IsFrameExist = false;
        m_isFull = true;
        m_isSkipped = true;
        m_isDecoded = 1;
        m_wasOutputted = 1;
        m_wasDisplayed = 1;
    }

    // m_pParsedFrameData's allocated size is remembered so that a
    // re-allocation is done only if size requirements exceed the
    // existing allocation.
    // m_paddedParsedFrameDataSize contains the image dimensions,
    // rounded up to a multiple of 16, that were used.

    H264DecoderFrame(MemoryAllocator *pMemoryAllocator, H264_Heap_Objects * pObjHeap);

    virtual ~H264DecoderFrame();

    // The following methods provide access to the H264Decoder's doubly
    // linked list of H264DecoderFrames.  Note that m_pPreviousFrame can
    // be non-NULL even for an I frame.
    H264DecoderFrame *previous() { return m_pPreviousFrame; }
    H264DecoderFrame *future()   { return m_pFutureFrame; }

    const H264DecoderFrame *previous() const { return m_pPreviousFrame; }
    const H264DecoderFrame *future() const { return m_pFutureFrame; }

    void setPrevious(H264DecoderFrame *pPrev)
    {
        m_pPreviousFrame = pPrev;
    }

    void setFuture(H264DecoderFrame *pFut)
    {
        m_pFutureFrame = pFut;
    }

    bool IsDecodingStarted() const { return m_isDecodingStarted != 0;}
    void StartDecoding() { m_isDecodingStarted = 1;}

    bool IsDecodingCompleted() const { return m_isDecodingCompleted != 0;}
    void CompleteDecoding();

    void UpdateErrorWithRefFrameStatus();

    bool        wasDisplayed()    { return m_wasDisplayed != 0; }
    void        setWasDisplayed() { m_wasDisplayed = 1; }

    bool        wasOutputted()    { return m_wasOutputted != 0; }
    void        setWasOutputted();

    bool        isDisposable()    { return (!IsFullFrame() || (m_wasOutputted && m_wasDisplayed)) && !GetRefCounter(); }

    // A decoded frame can be "disposed" if it is not an active reference
    // and it is not locked by the calling application and it has been
    // output for display.
    int32_t PicNum(int32_t f, int32_t force=0) const
    {
        if ((m_PictureStructureForRef >= FRM_STRUCTURE && force==0) || force==3)
        {
            return std::min(m_PicNum[0],m_PicNum[1]);
        }

        return m_PicNum[f];
    }

    void setPicNum(int32_t picNum, int32_t f)
    {
        if (m_PictureStructureForRef >= FRM_STRUCTURE)
        {
            m_PicNum[0] = m_PicNum[1] = picNum;
        }
        else
            m_PicNum[f] = picNum;
    }

    // Updates m_LongTermPicNum for if long term reference, based upon
    // m_LongTermFrameIdx.
    int32_t FrameNum() const
    {
        return m_FrameNum;
    }

    void setFrameNum(int32_t FrameNum)
    {
        m_FrameNum = FrameNum;
    }

    int32_t FrameNumWrap() const
    {
        return m_FrameNumWrap;
    }

    void setFrameNumWrap(int32_t FrameNumWrap)
    {
        m_FrameNumWrap = FrameNumWrap;
    }

    void UpdateFrameNumWrap(int32_t CurrFrameNum, int32_t MaxFrameNum, int32_t CurrPicStruct);
    // Updates m_FrameNumWrap and m_PicNum if the frame is a short-term
    // reference and a frame number wrap has occurred.

    int32_t LongTermFrameIdx() const
    {
        return m_LongTermFrameIdx;
    }

    void setLongTermFrameIdx(int32_t LongTermFrameIdx)
    {
        m_LongTermFrameIdx = LongTermFrameIdx;
    }

    bool isShortTermRef(int32_t WhichField) const
    {
        if (m_PictureStructureForRef>=FRM_STRUCTURE )
            return m_isShortTermRef[0] && m_isShortTermRef[1];
        else
            return m_isShortTermRef[WhichField];
    }

    int32_t isShortTermRef() const
    {
        return m_isShortTermRef[0] + m_isShortTermRef[1]*2;
    }

    void SetisShortTermRef(bool isRef, int32_t WhichField);

    int32_t PicOrderCnt(int32_t index, int32_t force=0) const
    {
        if ((m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
        {
            return std::min(m_PicOrderCnt[0],m_PicOrderCnt[1]);
        }
        else if (force==2)
        {
            if (isShortTermRef(0) && isShortTermRef(1))
                return std::min(m_PicOrderCnt[0],m_PicOrderCnt[1]);
            else if (isShortTermRef(0))
                return m_PicOrderCnt[0];
            else
                return m_PicOrderCnt[1];
        }
        return m_PicOrderCnt[index];
    }

    size_t DeblockPicID(int32_t index) const
    {
        return m_ID[index];
    }

    void setPicOrderCnt(int32_t PicOrderCnt, int32_t index) {m_PicOrderCnt[index] = PicOrderCnt;}
    bool isLongTermRef(int32_t WhichField) const
    {
        if (m_PictureStructureForRef>=FRM_STRUCTURE)
            return m_isLongTermRef[0] && m_isLongTermRef[1];
        else
            return m_isLongTermRef[WhichField];
    }

    int32_t isLongTermRef() const
    {
        return m_isLongTermRef[0] + m_isLongTermRef[1]*2;
    }

    void SetisLongTermRef(bool isRef, int32_t WhichField);

    int32_t LongTermPicNum(int32_t f, int32_t force=0) const
    {
        if ((m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
        {
            return std::min(m_LongTermPicNum[0],m_LongTermPicNum[1]);
        }
        else if (force==2)
        {
            if (isLongTermRef(0) && isLongTermRef(1))
                return std::min(m_LongTermPicNum[0],m_LongTermPicNum[1]);
            else if (isLongTermRef(0))
                return m_LongTermPicNum[0];
            else return m_LongTermPicNum[1];
        }
        return m_LongTermPicNum[f];
    }

    void setLongTermPicNum(int32_t LongTermPicNum,int32_t f) {m_LongTermPicNum[f] = LongTermPicNum;}
    void UpdateLongTermPicNum(int32_t CurrPicStruct);

    bool isInterViewRef(uint32_t WhichField) const
    {
        if (m_PictureStructureForDec>=FRM_STRUCTURE )
            return m_isInterViewRef[0];
        else
            return m_isInterViewRef[WhichField];
    }

    void SetInterViewRef(bool bInterViewRef, uint32_t WhichField)
    {
        if (m_PictureStructureForDec >= FRM_STRUCTURE)
            m_isInterViewRef[0] = m_isInterViewRef[1] = bInterViewRef;
        else
            m_isInterViewRef[WhichField] = bInterViewRef;
    }

    // set the view_id the frame belongs to
    void setViewId(uint32_t view_id)
    {
        m_viewId = view_id;
    }

    void IncreaseRefPicListResetCount(uint32_t f)
    {
        m_RefPicListResetCount[f]++;
    }

    void InitRefPicListResetCount(uint32_t f)
    {
        if (m_PictureStructureForRef >= FRM_STRUCTURE)
            m_RefPicListResetCount[0]=m_RefPicListResetCount[1]=0;
        else
            m_RefPicListResetCount[f]=0;
    }

    uint32_t RefPicListResetCount(int32_t f) const
    {
        if (m_PictureStructureForRef >= FRM_STRUCTURE)
            return std::max(m_RefPicListResetCount[0], m_RefPicListResetCount[1]);
        else
            return m_RefPicListResetCount[f];
    }

    int8_t GetNumberByParity(int32_t parity) const
    {
        VM_ASSERT(!parity || parity == 1);
        return m_bottom_field_flag[1]==parity ? 1 : 0;
    }

    //////////////////////////////////////////////////////////////////////////////
    // GetRefPicList
    // Returns pointer to start of specified ref pic list.
    //////////////////////////////////////////////////////////////////////////////
    H264DecoderRefPicList* GetRefPicList(int32_t sliceNumber, int32_t list);

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

    int32_t GetTotalMBs() const
    { return totalMBs; }

protected:
    // Declare memory management tools
    MemoryAllocator *m_pMemoryAllocator;
    H264_Heap_Objects * m_pObjHeap;
};

inline bool isAlmostDisposable(H264DecoderFrame * pTmp)
{
    return (pTmp->m_wasOutputted || !pTmp->IsFullFrame())&& !pTmp->GetRefCounter();
}

} // end namespace UMC

#endif // __UMC_H264_FRAME_H__
#endif // MFX_ENABLE_H264_VIDEO_DECODE
