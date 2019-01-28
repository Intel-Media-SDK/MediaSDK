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

#include <algorithm>
#include "umc_h265_frame.h"
#include "umc_h265_task_supplier.h"
#include "umc_h265_debug.h"



namespace UMC_HEVC_DECODER
{

H265DecoderFrame::H265DecoderFrame(UMC::MemoryAllocator *pMemoryAllocator, Heap_Objects * pObjHeap)
    : H265DecYUVBufferPadded(pMemoryAllocator)
    , m_ErrorType(0)
    , m_pPreviousFrame(nullptr)
    , m_pFutureFrame(nullptr)
    , m_dFrameTime(-1.0)
    , m_isOriginalPTS(false)
    , post_procces_complete(false)
    , m_index(-1)
    , m_UID(-1)
    , m_pSlicesInfo(nullptr)
    , m_pObjHeap(pObjHeap)
{
    m_isShortTermRef = false;
    m_isLongTermRef = false;
    m_RefPicListResetCount = 0;
    m_PicOrderCnt = 0;

    // set memory managment tools
    m_pMemoryAllocator = pMemoryAllocator;

    ResetRefCounter();

    m_pSlicesInfo = new H265DecoderFrameInfo(this, m_pObjHeap);

    m_Flags.isFull = 0;
    m_Flags.isDecoded = 0;
    m_Flags.isDecodingStarted = 0;
    m_Flags.isDecodingCompleted = 0;
    m_isDisplayable = 0;
    m_wasOutputted = 0;
    m_wasDisplayed = 0;
    m_maxUIDWhenWasDisplayed = 0;
    m_crop_flag = 0;
    m_crop_left = 0;
    m_crop_top = 0;
    m_crop_bottom = 0;
    m_pic_output = 0;
    m_FrameType = UMC::NONE_PICTURE;
    m_aspect_width = 0;
    m_MemID = 0;
    m_aspect_height = 0;
    m_isUsedAsReference = 0;
    m_crop_right = 0;
    m_DisplayPictureStruct_H265 = DPS_FRAME_H265;
    m_frameOrder = 0;
    prepared = false;
}

H265DecoderFrame::~H265DecoderFrame()
{
    if (m_pSlicesInfo)
    {
        delete m_pSlicesInfo;
        m_pSlicesInfo = 0;
    }

    // Just to be safe.
    m_pPreviousFrame = 0;
    m_pFutureFrame = 0;
    Reset();
    deallocate();
}

// Add target frame to the list of reference frames
void H265DecoderFrame::AddReferenceFrame(H265DecoderFrame * frm)
{
    if (!frm || frm == this)
        return;

    if (std::find(m_references.begin(), m_references.end(), frm) != m_references.end())
        return;

    frm->IncrementReference();
    m_references.push_back(frm);
}

// Clear all references to other frames
void H265DecoderFrame::FreeReferenceFrames()
{
    ReferenceList::iterator iter = m_references.begin();
    ReferenceList::iterator end_iter = m_references.end();

    for (; iter != end_iter; ++iter)
    {
        RefCounter *reference = *iter;
        reference->DecrementReference();
    }

    m_references.clear();
}

// Reinitialize frame structure before reusing frame
void H265DecoderFrame::Reset()
{
    if (m_pSlicesInfo)
        m_pSlicesInfo->Reset();

    ResetRefCounter();

    m_isShortTermRef = false;
    m_isLongTermRef = false;

    post_procces_complete = false;

    m_RefPicListResetCount = 0;
    m_PicOrderCnt = 0;

    m_Flags.isFull = 0;
    m_Flags.isDecoded = 0;
    m_Flags.isDecodingStarted = 0;
    m_Flags.isDecodingCompleted = 0;
    m_isDisplayable = 0;
    m_wasOutputted = 0;
    m_wasDisplayed = 0;
    m_pic_output = true;
    m_maxUIDWhenWasDisplayed = 0;

    m_dFrameTime = -1;
    m_isOriginalPTS = false;

    m_isUsedAsReference = false;

    m_UserData.Reset();

    m_ErrorType = 0;
    m_UID = -1;
    m_index = -1;

    m_MemID = MID_INVALID;

    prepared = false;

    FreeReferenceFrames();

    deallocate();

    m_refPicList.clear();
}

// Returns whether frame has all slices found
bool H265DecoderFrame::IsFullFrame() const
{
    return (m_Flags.isFull == 1);
}

// Set frame flag denoting that all slices for it were found
void H265DecoderFrame::SetFullFrame(bool isFull)
{
    m_Flags.isFull = (uint8_t) (isFull ? 1 : 0);
}

// Returns whether frame has been decoded
bool H265DecoderFrame::IsDecoded() const
{
    return m_Flags.isDecoded == 1;
}

// Clean up data after decoding is done
void H265DecoderFrame::FreeResources()
{
    FreeReferenceFrames();

    if (m_pSlicesInfo && IsDecoded())
        m_pSlicesInfo->Free();
}

// Flag frame as completely decoded
void H265DecoderFrame::CompleteDecoding()
{
    m_Flags.isDecodingCompleted = 1;
    UpdateErrorWithRefFrameStatus();
}

// Check reference frames for error status and flag this frame if error is found
void H265DecoderFrame::UpdateErrorWithRefFrameStatus()
{
    if (CheckReferenceFrameError())
    {
        SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
    }
}

// Delete unneeded references and set flags after decoding is done
void H265DecoderFrame::OnDecodingCompleted()
{
    UpdateErrorWithRefFrameStatus();

    m_Flags.isDecoded = 1;
    DEBUG_PRINT1((VM_STRING("On decoding complete decrement for POC %d, reference = %d\n"), m_PicOrderCnt, m_refCounter));
    DecrementReference();

    FreeResources();
}

// Mark frame as short term reference frame
void H265DecoderFrame::SetisShortTermRef(bool isRef)
{
    if (isRef)
    {
        if (!isShortTermRef() && !isLongTermRef())
            IncrementReference();

        m_isShortTermRef = true;
    }
    else
    {
        bool wasRef = isShortTermRef() != 0;

        m_isShortTermRef = false;

        if (wasRef && !isShortTermRef() && !isLongTermRef())
        {
            DecrementReference();
            DEBUG_PRINT1((VM_STRING("On was short term ref decrement for POC %d, reference = %d\n"), m_PicOrderCnt, m_refCounter));
        }
    }
}

// Mark frame as long term reference frame
void H265DecoderFrame::SetisLongTermRef(bool isRef)
{
    if (isRef)
    {
        if (!isShortTermRef() && !isLongTermRef())
            IncrementReference();

        m_isLongTermRef = true;
    }
    else
    {
        bool wasRef = isLongTermRef() != 0;

        m_isLongTermRef = false;

        if (wasRef && !isShortTermRef() && !isLongTermRef())
        {
            DEBUG_PRINT1((VM_STRING("On was long term reft decrement for POC %d, reference = %d\n"), m_PicOrderCnt, m_refCounter));
            DecrementReference();
        }
    }
}

// Flag frame after it was output
void H265DecoderFrame::setWasOutputted()
{
    m_wasOutputted = 1;
}

// Free resources if possible
void H265DecoderFrame::Free()
{
    if (wasDisplayed() && wasOutputted())
        Reset();
}

void H265DecoderFrame::AddSlice(H265Slice * pSlice)
{
    int32_t iSliceNumber = m_pSlicesInfo->GetSliceCount() + 1;

    pSlice->SetSliceNumber(iSliceNumber);
    pSlice->m_pCurrentFrame = this;
    m_pSlicesInfo->AddSlice(pSlice);

    m_refPicList.resize(pSlice->GetSliceNum() + 1);
}

bool H265DecoderFrame::CheckReferenceFrameError()
{
    uint32_t checkedErrorMask = UMC::ERROR_FRAME_MINOR | UMC::ERROR_FRAME_MAJOR | UMC::ERROR_FRAME_REFERENCE_FRAME;
    for (size_t i = 0; i < m_refPicList.size(); i ++)
    {
        H265DecoderRefPicList* list = &m_refPicList[i].m_refPicList[REF_PIC_LIST_0];
        for (size_t k = 0; list->m_refPicList[k].refFrame; k++)
        {
            if (list->m_refPicList[k].refFrame->GetError() & checkedErrorMask)
                return true;
        }

        list = &m_refPicList[i].m_refPicList[REF_PIC_LIST_1];
        for (size_t k = 0; list->m_refPicList[k].refFrame; k++)
        {
            if (list->m_refPicList[k].refFrame->GetError() & checkedErrorMask)
                return true;
        }
    }

    return false;
}


} // end namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
