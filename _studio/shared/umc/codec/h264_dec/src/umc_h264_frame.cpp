// Copyright (c) 2018 Intel Corporation
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

#include <algorithm>
#include "umc_h264_frame.h"
#include "umc_h264_task_supplier.h"
#include "umc_h264_dec_debug.h"

namespace UMC
{

H264DecoderFrame g_GlobalFakeFrame(0, 0);

H264DecoderFrameInfo::FakeFrameInitializer H264DecoderFrameInfo::g_FakeFrameInitializer;

H264DecoderFrameInfo::FakeFrameInitializer::FakeFrameInitializer()
{
    g_GlobalFakeFrame.m_ID[0] = (size_t)-1;
    g_GlobalFakeFrame.m_ID[1] = (size_t)-1;
};

H264DecoderFrame::H264DecoderFrame(MemoryAllocator *pMemoryAllocator, H264_Heap_Objects * pObjHeap)
    : H264DecYUVBufferPadded()
    , m_TopSliceCount(0)
    , m_frameOrder(0)
    , m_ErrorType(0)
    , m_pSlicesInfo(this)
    , m_pSlicesInfoBottom(this)
    , m_pPreviousFrame(0)
    , m_pFutureFrame(0)
    , m_dFrameTime(-1.0)
    , m_isOriginalPTS(false)
    , m_IsFrameExist(0)
    , m_dpb_output_delay(INVALID_DPB_OUTPUT_DELAY)
    , m_PictureStructureForDec(0)
    , m_displayPictureStruct(DPS_UNKNOWN)
    , totalMBs(0)
    , post_procces_complete(false)
    , m_iNumberOfSlices(0)
    , m_auIndex(-1)
    , m_index(-1)
    , m_UID(-1)
    , m_FrameType(NONE_PICTURE)
    , m_MemID(0)
    , m_crop_left(0)
    , m_crop_right(0)
    , m_crop_top(0)
    , m_crop_bottom(0)
    , m_crop_flag(0)
    , m_aspect_width(0)
    , m_aspect_height(0)
    , m_pObjHeap(pObjHeap)
{
    m_isShortTermRef[0] = m_isShortTermRef[1] = false;
    m_isLongTermRef[0] = m_isLongTermRef[1] = false;
    m_isInterViewRef[0] = m_isInterViewRef[1] = false;
    m_LongTermPicNum[0] = m_LongTermPicNum[1] = -1;
    m_FrameNumWrap = m_FrameNum  = -1;
    m_LongTermFrameIdx = -1;
    m_viewId = 0;
    m_RefPicListResetCount[0] = m_RefPicListResetCount[1] = 0;
    m_PicNum[0] = m_PicNum[1] = -1;
    m_LongTermFrameIdx = -1;
    m_FrameNumWrap = m_FrameNum  = -1;
    m_LongTermPicNum[0] = m_PicNum[1] = -1;
    m_PicOrderCnt[0] = m_PicOrderCnt[1] = 0;
    m_bIDRFlag = false;
    m_bIFlag   = false;

    // set memory managment tools
    m_pMemoryAllocator = pMemoryAllocator;

    ResetRefCounter();

    m_ID[0] = (int32_t) ((uint8_t *) this - (uint8_t *) 0);
    m_ID[1] = m_ID[0] + 1;

    m_PictureStructureForRef = FRM_STRUCTURE;

    m_isFull = 0;
    m_isDecoded = 0;
    m_isDecodingStarted = 0;
    m_isDecodingCompleted = 0;
    m_isSkipped = 0;
    m_wasOutputted = 0;
    m_wasDisplayed = 0;
    prepared[0] = false;
    prepared[1] = false;

    m_iResourceNumber = -1;
}

H264DecoderFrame::~H264DecoderFrame()
{
    // Just to be safe.
    m_pPreviousFrame = 0;
    m_pFutureFrame = 0;
    Reset();
    deallocate();
}

void H264DecoderFrame::AddReference(RefCounter * reference)
{
    if (!reference || reference == this)
        return;

    if (std::find(m_references.begin(), m_references.end(), reference) != m_references.end())
        return;

    reference->IncrementReference();
    m_references.push_back(reference);
}

void H264DecoderFrame::FreeReferenceFrames()
{
    ReferenceList::iterator iter = m_references.begin();
    ReferenceList::iterator end_iter = m_references.end();

    for (; iter != end_iter; ++iter)
    {
        RefCounter * reference = *iter;
        reference->DecrementReference();

    }

    m_references.clear();
}

void H264DecoderFrame::Reset()
{
    m_TopSliceCount = 0;

    m_pSlicesInfo.Reset();
    m_pSlicesInfoBottom.Reset();

    ResetRefCounter();

    m_isShortTermRef[0] = m_isShortTermRef[1] = false;
    m_isLongTermRef[0] = m_isLongTermRef[1] = false;
    m_isInterViewRef[0] = m_isInterViewRef[1] = false;

    post_procces_complete = false;
    m_bIDRFlag = false;
    m_bIFlag   = false;

    m_RefPicListResetCount[0] = m_RefPicListResetCount[1] = 0;
    m_PicNum[0] = m_PicNum[1] = -1;
    m_LongTermPicNum[0] = m_LongTermPicNum[1] = -1;
    m_PictureStructureForRef = FRM_STRUCTURE;
    m_PicOrderCnt[0] = m_PicOrderCnt[1] = 0;

    m_viewId = 0;
    m_LongTermFrameIdx = -1;
    m_FrameNumWrap = m_FrameNum  = -1;
    m_isFull = 0;
    m_isDecoded = 0;
    m_isDecodingStarted = 0;
    m_isDecodingCompleted = 0;
    m_isSkipped = 0;
    m_auIndex = -1;
    m_wasOutputted = 0;
    m_wasDisplayed = 0;
    m_dpb_output_delay = INVALID_DPB_OUTPUT_DELAY;

    m_bottom_field_flag[0] = m_bottom_field_flag[1] = -1;

    m_dFrameTime = -1;
    m_isOriginalPTS = false;

    m_IsFrameExist = true;
    m_iNumberOfSlices = 0;

    m_UserData.Reset();

    m_ErrorType = 0;
    m_UID = -1;
    m_index = -1;
    m_auIndex = -1;
    m_iResourceNumber = -1;

    m_MemID = MID_INVALID;

    prepared[0] = false;
    prepared[1] = false;

    m_displayPictureStruct = DPS_UNKNOWN;

    FreeReferenceFrames();

    deallocate();
}

bool H264DecoderFrame::IsFullFrame() const
{
    return (m_isFull == 1);
}

void H264DecoderFrame::SetFullFrame(bool isFull)
{
    m_isFull = (uint8_t) (isFull ? 1 : 0);
}

bool H264DecoderFrame::IsDecoded() const
{
    return m_isDecoded == 1;
}

void H264DecoderFrame::FreeResources()
{
    FreeReferenceFrames();

    if (IsDecoded())
    {
        m_pSlicesInfo.Free();
        m_pSlicesInfoBottom.Free();
    }
}

void H264DecoderFrame::CompleteDecoding()
{
    UpdateErrorWithRefFrameStatus();
    m_isDecodingCompleted = 1;
}

void H264DecoderFrame::UpdateErrorWithRefFrameStatus()
{
    if (m_pSlicesInfo.CheckReferenceFrameError() || m_pSlicesInfoBottom.CheckReferenceFrameError())
    {
        SetErrorFlagged(ERROR_FRAME_REFERENCE_FRAME);
    }
}

void H264DecoderFrame::OnDecodingCompleted()
{
    UpdateErrorWithRefFrameStatus();

    m_isDecoded = 1;
    FreeResources();
    DecrementReference();
}

void H264DecoderFrame::SetisShortTermRef(bool isRef, int32_t WhichField)
{
    if (isRef)
    {
        if (!isShortTermRef() && !isLongTermRef())
            IncrementReference();

        if (m_PictureStructureForRef >= FRM_STRUCTURE)
            m_isShortTermRef[0] = m_isShortTermRef[1] = true;
        else
            m_isShortTermRef[WhichField] = true;
    }
    else
    {
        bool wasRef = isShortTermRef() != 0;

        if (m_PictureStructureForRef >= FRM_STRUCTURE)
        {
            m_isShortTermRef[0] = m_isShortTermRef[1] = false;
        }
        else
            m_isShortTermRef[WhichField] = false;

        if (wasRef && !isShortTermRef() && !isLongTermRef())
        {
            DecrementReference();
        }
    }
}

void H264DecoderFrame::SetisLongTermRef(bool isRef, int32_t WhichField)
{
    if (isRef)
    {
        if (!isShortTermRef() && !isLongTermRef())
            IncrementReference();

        if (m_PictureStructureForRef >= FRM_STRUCTURE)
            m_isLongTermRef[0] = m_isLongTermRef[1] = true;
        else
            m_isLongTermRef[WhichField] = true;
    }
    else
    {
        bool wasRef = isLongTermRef() != 0;

        if (m_PictureStructureForRef >= FRM_STRUCTURE)
        {
            m_isLongTermRef[0] = m_isLongTermRef[1] = false;
        }
        else
            m_isLongTermRef[WhichField] = false;

        if (wasRef && !isShortTermRef() && !isLongTermRef())
        {
            DecrementReference();
        }
    }
}

void H264DecoderFrame::setWasOutputted()
{
    m_wasOutputted = 1;
}

void H264DecoderFrame::Free()
{
    DEBUG_PRINT((VM_STRING("Free reference POC - %d, ref. count - %d, uid - %d, state D.%d O.%d\n"), m_PicOrderCnt[0], m_refCounter, m_UID, m_wasDisplayed, m_wasOutputted));

    if (wasDisplayed() && wasOutputted())
        Reset();
}

H264DecoderRefPicList* H264DecoderFrame::GetRefPicList(int32_t sliceNumber, int32_t list)
{
    H264DecoderRefPicList *pList;
    pList = GetAU(sliceNumber > m_TopSliceCount ? 1 : 0)->GetRefPicList(sliceNumber, list);
    return pList;
}   // RefPicList. Returns pointer to start of specified ref pic list.

//////////////////////////////////////////////////////////////////////////////
// updateFrameNumWrap
//  Updates m_FrameNumWrap and m_PicNum if the frame is a short-term
//  reference and a frame number wrap has occurred.
//////////////////////////////////////////////////////////////////////////////
void H264DecoderFrame::UpdateFrameNumWrap(int32_t  CurrFrameNum, int32_t  MaxFrameNum, int32_t CurrPicStruct)
{
    if (isShortTermRef())
    {
        m_FrameNumWrap = m_FrameNum;
        if (m_FrameNum > CurrFrameNum)
            m_FrameNumWrap -= MaxFrameNum;
        if (CurrPicStruct >= FRM_STRUCTURE)
        {
            setPicNum(m_FrameNumWrap, 0);
            setPicNum(m_FrameNumWrap, 1);
            m_PictureStructureForRef = FRM_STRUCTURE;
        }
        else
        {
            m_PictureStructureForRef = FLD_STRUCTURE;
            if (m_bottom_field_flag[0])
            {
                //1st - bottom, 2nd - top
                if (isShortTermRef(0)) setPicNum((2*m_FrameNumWrap) + (CurrPicStruct == BOTTOM_FLD_STRUCTURE), 0);
                if (isShortTermRef(1)) setPicNum((2*m_FrameNumWrap) + (CurrPicStruct == TOP_FLD_STRUCTURE), 1);
            }
            else
            {
                //1st - top , 2nd - bottom
                if (isShortTermRef(0)) setPicNum((2*m_FrameNumWrap) + (CurrPicStruct == TOP_FLD_STRUCTURE), 0);
                if (isShortTermRef(1)) setPicNum((2*m_FrameNumWrap) + (CurrPicStruct == BOTTOM_FLD_STRUCTURE), 1);
            }
        }
    }

}    // updateFrameNumWrap

//////////////////////////////////////////////////////////////////////////////
// updateLongTermPicNum
//  Updates m_LongTermPicNum for if long term reference, based upon
//  m_LongTermFrameIdx.
//////////////////////////////////////////////////////////////////////////////
void H264DecoderFrame::UpdateLongTermPicNum(int32_t CurrPicStruct)
{
    if (isLongTermRef())
    {
        if (CurrPicStruct >= FRM_STRUCTURE)
        {
            m_LongTermPicNum[0] = m_LongTermFrameIdx;
            m_LongTermPicNum[1] = m_LongTermFrameIdx;
            m_PictureStructureForRef = FRM_STRUCTURE;
        }
        else
        {
            m_PictureStructureForRef = FLD_STRUCTURE;
            if (m_bottom_field_flag[0])
            {
                //1st - bottom, 2nd - top
                m_LongTermPicNum[0] = 2*m_LongTermFrameIdx + (CurrPicStruct == BOTTOM_FLD_STRUCTURE);
                m_LongTermPicNum[1] = 2*m_LongTermFrameIdx + (CurrPicStruct == TOP_FLD_STRUCTURE);
            }
            else
            {
                //1st - top , 2nd - bottom
                m_LongTermPicNum[0] = 2*m_LongTermFrameIdx + (CurrPicStruct == TOP_FLD_STRUCTURE);
                m_LongTermPicNum[1] = 2*m_LongTermFrameIdx + (CurrPicStruct == BOTTOM_FLD_STRUCTURE);
            }
        }
    }
}    // updateLongTermPicNum


bool H264DecoderFrameInfo::CheckReferenceFrameError()
{
    uint32_t checkedErrorMask = ERROR_FRAME_MINOR | ERROR_FRAME_MAJOR | ERROR_FRAME_REFERENCE_FRAME;
    for (size_t i = 0; i < m_refPicList.size(); i++)
    {
        H264DecoderRefPicList* list = &m_refPicList[i].m_refPicList[LIST_0];
        for (size_t k = 0; list->m_RefPicList[k]; k++)
        {
            if (list->m_RefPicList[k]->GetError() & checkedErrorMask)
                return true;
        }

        list = &m_refPicList[i].m_refPicList[LIST_1];
        for (size_t k = 0; list->m_RefPicList[k]; k++)
        {
            if (list->m_RefPicList[k]->GetError() & checkedErrorMask)
                return true;
        }
    }

    return false;
}

} // end namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
