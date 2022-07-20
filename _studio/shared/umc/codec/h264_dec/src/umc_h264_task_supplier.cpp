// Copyright (c) 2017-2020 Intel Corporation
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
#include <memory>

#include "mfx_trace.h"

#include "umc_h264_task_supplier.h"
#include "umc_h264_frame_list.h"
#include "umc_h264_nal_spl.h"
#include "umc_h264_bitstream_headers.h"
#include "umc_h264_task_broker.h"
#include "umc_h264_dec_defs_dec.h"
#include "vm_sys_info.h"
#include "umc_structures.h"
#include "umc_frame_data.h"
#include "umc_h264_notify.h"

#include "umc_h264_dec_debug.h"

using namespace UMC_H264_DECODER;

namespace UMC
{

#if (MFX_VERSION >= 1025)
inline void SetDecodeErrorTypes(NAL_Unit_Type nalUnit, mfxExtDecodeErrorReport *pDecodeErrorReport)
{
    switch (nalUnit)
    {
        case NAL_UT_SPS: pDecodeErrorReport->ErrorTypes |= MFX_ERROR_SPS; break;
        case NAL_UT_PPS: pDecodeErrorReport->ErrorTypes |= MFX_ERROR_PPS; break;
        default: break;
    };
}
#endif

/****************************************************************************************************/
// DPBOutput class routine
/****************************************************************************************************/
DPBOutput::DPBOutput()
{
    Reset();
}

void DPBOutput::Reset(bool disableDelayFeature)
{
    m_isUseFlags.use_payload_sei_delay = 1;
    m_isUseFlags.use_pic_order_cnt_type = 1;

    if (disableDelayFeature)
    {
        m_isUseFlags.use_payload_sei_delay = 0;
        m_isUseFlags.use_pic_order_cnt_type = 0;
    }
}

bool DPBOutput::IsUseDelayOutputValue() const
{
    return IsUseSEIDelayOutputValue() || IsUsePicOrderCnt();
}

bool DPBOutput::IsUseSEIDelayOutputValue() const
{
    return m_isUseFlags.use_payload_sei_delay != 0;
}

bool DPBOutput::IsUsePicOrderCnt() const
{
    return m_isUseFlags.use_pic_order_cnt_type != 0;
}

void DPBOutput::OnNewSps(H264SeqParamSet * sps)
{
    if (sps && sps->pic_order_cnt_type != 2)
    {
        m_isUseFlags.use_pic_order_cnt_type = 0;
    }
}

int32_t DPBOutput::GetDPBOutputDelay(H264SEIPayLoad * payload)
{
    if (IsUsePicOrderCnt())
        return 0;

    if (!payload)
    {
        m_isUseFlags.use_payload_sei_delay = 0;
        return INVALID_DPB_OUTPUT_DELAY;
    }

    if (IsUseSEIDelayOutputValue())
    {
        if (payload->SEI_messages.pic_timing.dpb_output_delay != 0)
            m_isUseFlags.use_payload_sei_delay = 0;

        return payload->SEI_messages.pic_timing.dpb_output_delay;
    }

    return INVALID_DPB_OUTPUT_DELAY;
}

/****************************************************************************************************/
// SVC extension class routine
/****************************************************************************************************/
SVC_Extension::SVC_Extension()
    : MVC_Extension()
    , m_dependency_id(H264_MAX_DEPENDENCY_ID)
    , m_quality_id(H264_MAX_QUALITY_ID)
{
}

SVC_Extension::~SVC_Extension()
{
    Reset();
}

void SVC_Extension::Reset()
{
    MVC_Extension::Reset();

    m_quality_id = 15;
    m_dependency_id = 7;
}

void SVC_Extension::Close()
{
    SVC_Extension::Reset();
    MVC_Extension::Close();
}

void SVC_Extension::SetSVCTargetLayer(uint32_t dependency_id, uint32_t quality_id, uint32_t temporal_id)
{
    m_dependency_id = dependency_id;
    m_quality_id = quality_id;
    m_temporal_id = temporal_id;
}

bool SVC_Extension::IsShouldSkipSlice(H264NalExtension *nal_ext)
{
    if (!nal_ext)
        return true;

    if (!nal_ext->svc_extension_flag)
        return MVC_Extension::IsShouldSkipSlice(nal_ext);

    //if (nal_ext->svc.discardable_flag)
      //  return true;

    if (nal_ext->svc.temporal_id > m_temporal_id)
        return true;

    if (nal_ext->svc.priority_id > m_priority_id)
        return true;

    if (nal_ext->svc.quality_id > m_quality_id)
        return true;

    if (nal_ext->svc.dependency_id > m_dependency_id)
        return true;

    return false;
}

bool SVC_Extension::IsShouldSkipSlice(H264Slice * slice)
{
    return IsShouldSkipSlice(&slice->GetSliceHeader()->nal_ext);
}

void SVC_Extension::ChooseLevelIdc(const H264SeqParamSetSVCExtension * extension, uint8_t baseViewLevelIDC, uint8_t extensionLevelIdc)
{
    if (m_level_idc)
        return;

    m_level_idc = std::max({baseViewLevelIDC, extensionLevelIdc, extension->level_idc});

    assert(m_level_idc != 0);
}

/****************************************************************************************************/
// DecReferencePictureMarking
/****************************************************************************************************/
DecReferencePictureMarking::DecReferencePictureMarking()
    : m_isDPBErrorFound(0)
    , m_frameCount(0)
{
}

void DecReferencePictureMarking::Reset()
{
    m_frameCount = 0;
    m_isDPBErrorFound = 0;
    m_commandsList.clear();
}

void DecReferencePictureMarking::ResetError()
{
    m_isDPBErrorFound = 0;
}

uint32_t DecReferencePictureMarking::GetDPBError() const
{
    return m_isDPBErrorFound;
}

bool IsDecRefPicMarkingEquals(const H264SEIPayLoadBase::SEIMessages::DecRefPicMarkingRepetition *decRefPicMarking1, const H264SEIPayLoadBase::SEIMessages::DecRefPicMarkingRepetition *decRefPicMarking2)
{
    if (!decRefPicMarking1 || !decRefPicMarking2)
        return true;

    if (decRefPicMarking1->original_idr_flag            != decRefPicMarking2->original_idr_flag ||
        decRefPicMarking1->original_frame_num           != decRefPicMarking2->original_frame_num ||
        decRefPicMarking1->original_field_pic_flag      != decRefPicMarking2->original_field_pic_flag ||
        decRefPicMarking1->original_bottom_field_flag   != decRefPicMarking2->original_bottom_field_flag ||
        decRefPicMarking1->long_term_reference_flag     != decRefPicMarking2->long_term_reference_flag)
        return false;

    if (decRefPicMarking1->adaptiveMarkingInfo.num_entries != decRefPicMarking2->adaptiveMarkingInfo.num_entries)
        return false;

    for (uint32_t i = 0; i < decRefPicMarking1->adaptiveMarkingInfo.num_entries; i++)
    {
        if (decRefPicMarking1->adaptiveMarkingInfo.value[i] != decRefPicMarking2->adaptiveMarkingInfo.value[i] ||
            decRefPicMarking1->adaptiveMarkingInfo.mmco[i]  != decRefPicMarking2->adaptiveMarkingInfo.mmco[i])
            return false;
    }

    return true;
}

Status DecReferencePictureMarking::CheckSEIRepetition(ViewItem &view, H264SEIPayLoad *payload)
{
    Status umcRes = UMC_OK;
    if (!payload || payload->payLoadType != SEI_DEC_REF_PIC_MARKING_TYPE)
        return UMC_OK;

    H264DecoderFrame * pFrame = 0;

    for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
    {
        if (pCurr->FrameNum() == payload->SEI_messages.dec_ref_pic_marking_repetition.original_frame_num)
        {
            pFrame = pCurr;
            break;
        }
    }

    if (!pFrame)
        return UMC_OK;

    const H264SEIPayLoadBase::SEIMessages::DecRefPicMarkingRepetition *forCheck = pFrame->GetAU(0)->GetDecRefPicMarking();
    if (payload->SEI_messages.dec_ref_pic_marking_repetition.original_field_pic_flag)
    {
        forCheck = pFrame->GetAU(pFrame->GetNumberByParity(payload->SEI_messages.dec_ref_pic_marking_repetition.original_bottom_field_flag))->GetDecRefPicMarking();
    }

    if (!IsDecRefPicMarkingEquals(&payload->SEI_messages.dec_ref_pic_marking_repetition, forCheck))
    {
        pFrame->SetErrorFlagged(ERROR_FRAME_DPB);
        m_isDPBErrorFound = 1;
        return UMC_ERR_FAILED;
    }

    return umcRes;
}

void DecReferencePictureMarking::CheckSEIRepetition(ViewItem &view, H264DecoderFrame * frame)
{
    bool isEqual = false;

    if (isEqual)
        return;

    DPBCommandsList::iterator end_iter = m_commandsList.end();
    H264DecoderFrame * temp = 0;

    bool wasFrame = false;

    for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
    {
        pCurr->IncrementReference();
        pCurr->IncrementReference();
    }

    {
    DPBCommandsList::reverse_iterator rev_iter = m_commandsList.rbegin();
    DPBCommandsList::reverse_iterator rev_end_iter = m_commandsList.rend();

    for (; rev_iter != rev_end_iter; ++rev_iter)
    {
        DPBChangeItem& item = *rev_iter;

        if (item.m_pCurrentFrame == frame)
        {
            wasFrame = true;
            Undo(frame);
            break;
        }

        if (temp != item.m_pCurrentFrame)
        {
            temp = 0;
        }

        if (!temp)
        {
            temp = item.m_pCurrentFrame;
            Undo(temp);
        }
    }
    }

    bool start = false;
    temp = 0;
    DPBCommandsList::iterator iter = m_commandsList.begin(); // stl issue. we can't reuse iterator
    for (; iter != end_iter; ++iter)
    {
        DPBChangeItem& item = *iter;

        if (item.m_pCurrentFrame == frame)
        {
            Redo(frame);
            start = true;
            break;
        }

        if (!start && wasFrame)
            continue;

        if (temp != item.m_pCurrentFrame)
        {
            temp = 0;
        }

        if (!temp)
        {
            temp = item.m_pCurrentFrame;
            Redo(temp);
        }
    }

    for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
    {
        pCurr->DecrementReference();
        pCurr->DecrementReference();
    }
}

void DecReferencePictureMarking::Undo(const H264DecoderFrame * frame)
{
    MakeChange(true, frame);
}

void DecReferencePictureMarking::Redo(const H264DecoderFrame * frame)
{
    MakeChange(false, frame);
}

void DecReferencePictureMarking::MakeChange(bool isUndo, const H264DecoderFrame * frame)
{
    DPBCommandsList::iterator iter = m_commandsList.begin();
    DPBCommandsList::iterator end_iter = m_commandsList.end();

    for (; iter != end_iter; ++iter)
    {
        DPBChangeItem& item = *iter;

        if (item.m_pCurrentFrame == frame)
        {
            MakeChange(isUndo, &item);
        }
    }
}

void DecReferencePictureMarking::RemoveOld()
{
    if (!m_commandsList.size())
    {
        m_frameCount = 0;
        return;
    }

    Remove(m_commandsList.front().m_pCurrentFrame);
}

#ifdef _DEBUG
void DecReferencePictureMarking::DebugPrintItems()
{
    printf("==================================\n");

    DPBCommandsList::iterator iter = m_commandsList.begin(); // stl issue. we can't reuse iterator
    for (; iter != m_commandsList.end(); ++iter)
    {
        DPBChangeItem& item = *iter;
        printf("current - %d, ref - %d\n", item.m_pCurrentFrame->m_PicOrderCnt[0], item.m_pRefFrame->m_PicOrderCnt[0]);
    }

    printf("==================================\n");
}
#endif

void DecReferencePictureMarking::Remove(H264DecoderFrame * frame)
{
    //printf("remove frame - %d\n", frame->m_PicOrderCnt[0]);
    //DebugPrintItems();

    DPBCommandsList::iterator iter = m_commandsList.begin();
    DPBCommandsList::iterator end_iter = m_commandsList.end();

    DPBCommandsList::iterator start = m_commandsList.end();
    DPBCommandsList::iterator end = m_commandsList.end();

    for (; iter != end_iter; ++iter)
    {
        DPBChangeItem& item = *iter;

        if (start == m_commandsList.end())
        {
            if (item.m_pCurrentFrame == frame)
            {
                m_frameCount--;
                start = iter;
            }
        }
        else
        {
            if (item.m_pCurrentFrame != frame)
            {
                end = iter;
                break;
            }
        }
    }

    if (start != m_commandsList.end())
        m_commandsList.erase(m_commandsList.begin(), end);


    iter = m_commandsList.begin();

    for (; iter != m_commandsList.end(); ++iter)
    {
        DPBChangeItem& item = *iter;

        if (item.m_pRefFrame == frame)
        {
            m_commandsList.erase(iter);
            iter = m_commandsList.begin();
            if (iter == m_commandsList.end()) // avoid ++iter operation
                break;
        }
    }


    //DebugPrintItems();
}

DecReferencePictureMarking::DPBChangeItem* DecReferencePictureMarking::AddItemAndRun(H264DecoderFrame * currentFrame, H264DecoderFrame * refFrame, uint32_t flags)
{
    DPBChangeItem* item = AddItem(m_commandsList, currentFrame, refFrame, flags);
    if (!item)
        return 0;

    Redo(item);
    return item;
}

DecReferencePictureMarking::DPBChangeItem* DecReferencePictureMarking::AddItem(DPBCommandsList & list, H264DecoderFrame * currentFrame, H264DecoderFrame * refFrame, uint32_t flags)
{
    if (!currentFrame || !refFrame)
        return 0;

    DPBChangeItem item;
    item.m_pCurrentFrame = currentFrame;
    item.m_pRefFrame = refFrame;

    item.m_type.isShortTerm = (flags & SHORT_TERM) ? 1 : 0;
    item.m_type.isFrame     = (flags & FULL_FRAME) ? 1 : 0;
    item.m_type.isSet       = (flags & SET_REFERENCE) ? 1 : 0;
    item.m_type.isBottom    = (flags & BOTTOM_FIELD) ? 1 : 0;

    if (CheckUseless(&item))
        return 0;

    list.push_back(item);
    return &list.back();
}

bool DecReferencePictureMarking::CheckUseless(DPBChangeItem * item)
{
    if (!item)
        return true;

    if (item->m_type.isShortTerm)
    {
        if (item->m_type.isFrame)
        {
            return item->m_type.isSet ? (item->m_pRefFrame->isShortTermRef() == 3) : (!item->m_pRefFrame->isShortTermRef());
        }
        else
        {
            return item->m_type.isSet ? item->m_pRefFrame->isShortTermRef(item->m_type.isBottom) : !item->m_pRefFrame->isShortTermRef(item->m_type.isBottom);
        }
    }
    else
    {
        if (item->m_type.isFrame)
        {
            return item->m_type.isSet ? item->m_pRefFrame->isLongTermRef() == 3 : !item->m_pRefFrame->isLongTermRef();
        }
        else
        {
            return item->m_type.isSet ? item->m_pRefFrame->isLongTermRef(item->m_type.isBottom) : !item->m_pRefFrame->isLongTermRef(item->m_type.isBottom);
        }
    }
}

void DecReferencePictureMarking::MakeChange(bool isUndo, const DPBChangeItem * item)
{
    if (!item)
        return;

    DPBChangeItem temp = *item;

    if (isUndo)
    {
        temp.m_type.isSet ^= 1;
    }

    item = &temp;

    int32_t savePSRef = item->m_pRefFrame->m_PictureStructureForRef;
    item->m_pRefFrame->m_PictureStructureForRef = item->m_pCurrentFrame->m_PictureStructureForDec;

    if (item->m_type.isFrame)
    {
        if (item->m_type.isShortTerm)
        {
            item->m_pRefFrame->SetisShortTermRef(item->m_type.isSet == 1, 0);
            item->m_pRefFrame->SetisShortTermRef(item->m_type.isSet == 1, 1);
        }
        else
        {
            item->m_pRefFrame->SetisLongTermRef(item->m_type.isSet == 1, 0);
            item->m_pRefFrame->SetisLongTermRef(item->m_type.isSet == 1, 1);
        }
    }
    else
    {
        if (item->m_type.isShortTerm)
        {
            item->m_pRefFrame->SetisShortTermRef(item->m_type.isSet == 1, item->m_type.isBottom);
        }
        else
        {
            item->m_pRefFrame->SetisLongTermRef(item->m_type.isSet == 1, item->m_type.isBottom);
        }
    }

    item->m_pRefFrame->m_PictureStructureForRef = savePSRef;
}

void DecReferencePictureMarking::Undo(const DPBChangeItem * item)
{
    MakeChange(true, item);
}

void DecReferencePictureMarking::Redo(const DPBChangeItem * item)
{
    MakeChange(false, item);
}

void DecReferencePictureMarking::SlideWindow(ViewItem &view, H264Slice * pSlice, int32_t field_index)
{
    uint32_t NumShortTermRefs, NumLongTermRefs;
    const H264SeqParamSet* sps = pSlice->GetSeqParam();

    // find out how many active reference frames currently in decoded
    // frames buffer
    view.GetDPBList(0)->countActiveRefs(NumShortTermRefs, NumLongTermRefs);
    while (NumShortTermRefs > 0 &&
        (NumShortTermRefs + NumLongTermRefs >= sps->num_ref_frames) &&
        !field_index)
    {
        // mark oldest short-term reference as unused
        assert(NumShortTermRefs > 0);

        H264DecoderFrame * pFrame = view.GetDPBList(0)->findOldestShortTermRef();

        if (!pFrame)
            break;

        AddItemAndRun(pSlice->GetCurrentFrame(), pFrame, UNSET_REFERENCE | FULL_FRAME | SHORT_TERM);
        NumShortTermRefs--;
    }
}

Status DecReferencePictureMarking::UpdateRefPicMarking(ViewItem &view, H264DecoderFrame * pFrame, H264Slice * pSlice, int32_t field_index)
{
    Status umcRes = UMC_OK;
    bool bCurrentisST = true;

    m_frameCount++;

    H264SliceHeader  * sliceHeader = pSlice->GetSliceHeader();

    // set MVC 'inter view flag'
    pFrame->SetInterViewRef(0 != sliceHeader->nal_ext.mvc.inter_view_flag, field_index);

    // corruption recovery
    if (pFrame->m_bIFlag)
    {
        for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
        {
            if (pCurr->GetError() & ERROR_FRAME_SHORT_TERM_STUCK)
            {
                AddItemAndRun(pFrame, pCurr, UNSET_REFERENCE | FULL_FRAME | SHORT_TERM);
            }
        }
    }

    if (pFrame->m_bIDRFlag)
    {
        // mark all reference pictures as unused
        for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
        {
            if (pCurr->isShortTermRef() || pCurr->isLongTermRef())
            {
                AddItemAndRun(pFrame, pCurr, UNSET_REFERENCE | FULL_FRAME | SHORT_TERM);
                AddItemAndRun(pFrame, pCurr, UNSET_REFERENCE | FULL_FRAME | LONG_TERM);
            }
        }

        if (sliceHeader->long_term_reference_flag)
        {
            AddItemAndRun(pFrame, pFrame, SET_REFERENCE | LONG_TERM | (field_index ? BOTTOM_FIELD : TOP_FIELD));
            //pFrame->SetisLongTermRef(true, field_index);
            pFrame->setLongTermFrameIdx(0);
            view.MaxLongTermFrameIdx[0] = 0;
        }
        else
        {
            AddItemAndRun(pFrame, pFrame, SET_REFERENCE | SHORT_TERM | (field_index ? BOTTOM_FIELD : TOP_FIELD));
            //pFrame->SetisShortTermRef(true, field_index);
            view.MaxLongTermFrameIdx[0] = -1;        // no long term frame indices
        }

        bCurrentisST = false;
    }
    else
    {
        AdaptiveMarkingInfo* pAdaptiveMarkingInfo = pSlice->GetAdaptiveMarkingInfo();
        // adaptive ref pic marking
        if (pAdaptiveMarkingInfo && pAdaptiveMarkingInfo->num_entries > 0)
        {
            for (uint32_t arpmmf_idx = 0; arpmmf_idx < pAdaptiveMarkingInfo->num_entries; arpmmf_idx++)
            {
                int32_t LongTermFrameIdx;
                int32_t picNum;
                H264DecoderFrame * pRefFrame = 0;
                int32_t field = 0;

                switch (pAdaptiveMarkingInfo->mmco[arpmmf_idx])
                {
                case 1:
                    // mark a short-term picture as unused for reference
                    // Value is difference_of_pic_nums_minus1
                    picNum = pFrame->PicNum(field_index) -
                        (pAdaptiveMarkingInfo->value[arpmmf_idx*2] + 1);

                    pRefFrame = view.GetDPBList(0)->findShortTermPic(picNum, &field);
                    AddItemAndRun(pFrame, pRefFrame, UNSET_REFERENCE | SHORT_TERM | (field ? BOTTOM_FIELD : TOP_FIELD));
                    break;
                case 2:
                    // mark a long-term picture as unused for reference
                    // value is long_term_pic_num
                    picNum = pAdaptiveMarkingInfo->value[arpmmf_idx*2];
                    pRefFrame = view.GetDPBList(0)->findLongTermPic(picNum, &field);
                    AddItemAndRun(pFrame, pRefFrame, UNSET_REFERENCE | LONG_TERM | (field ? BOTTOM_FIELD : TOP_FIELD));
                    break;
                case 3:

                    // Assign a long-term frame idx to a short-term picture
                    // Value is difference_of_pic_nums_minus1 followed by
                    // long_term_frame_idx. Only this case uses 2 value entries.
                    picNum = pFrame->PicNum(field_index) -
                        (pAdaptiveMarkingInfo->value[arpmmf_idx*2] + 1);
                    LongTermFrameIdx = pAdaptiveMarkingInfo->value[arpmmf_idx*2+1];

                    pRefFrame = view.GetDPBList(0)->findShortTermPic(picNum, &field);
                    if (!pRefFrame)
                        break;

                    {
                        H264DecoderFrame * longTerm = view.GetDPBList(0)->findLongTermRefIdx(LongTermFrameIdx);
                        if (longTerm != pRefFrame)
                            AddItemAndRun(pFrame, longTerm, UNSET_REFERENCE | LONG_TERM | FULL_FRAME);
                    }

                    AddItemAndRun(pFrame, pRefFrame, SET_REFERENCE | LONG_TERM | (field ? BOTTOM_FIELD : TOP_FIELD));
                    AddItemAndRun(pFrame, pRefFrame, UNSET_REFERENCE | SHORT_TERM | (field ? BOTTOM_FIELD : TOP_FIELD));

                    pRefFrame->setLongTermFrameIdx(LongTermFrameIdx);
                    pRefFrame->UpdateLongTermPicNum(pRefFrame->m_PictureStructureForRef >= FRM_STRUCTURE ? 2 : pRefFrame->m_bottom_field_flag[field]);
                    break;
                case 4:
                    // Specify max long term frame idx
                    // Value is max_long_term_frame_idx_plus1
                    // Set to "no long-term frame indices" (-1) when value == 0.
                    view.MaxLongTermFrameIdx[0] = pAdaptiveMarkingInfo->value[arpmmf_idx*2] - 1;

                    pRefFrame = view.GetDPBList(0)->findOldLongTermRef(view.MaxLongTermFrameIdx[0]);
                    while (pRefFrame)
                    {
                        AddItemAndRun(pFrame, pRefFrame, UNSET_REFERENCE | LONG_TERM | FULL_FRAME);
                        pRefFrame = view.GetDPBList(0)->findOldLongTermRef(view.MaxLongTermFrameIdx[0]);
                    }
                    break;
                case 5:
                    // Mark all as unused for reference
                    // no value
                    for (H264DecoderFrame *pCurr = view.GetDPBList(0)->head(); pCurr; pCurr = pCurr->future())
                    {
                        if (pCurr->isShortTermRef() || pCurr->isLongTermRef())
                        {
                            AddItemAndRun(pFrame, pCurr, UNSET_REFERENCE | FULL_FRAME | SHORT_TERM);
                            AddItemAndRun(pFrame, pCurr, UNSET_REFERENCE | FULL_FRAME | LONG_TERM);
                        }
                    }

                    view.GetDPBList(0)->IncreaseRefPicListResetCount(pFrame);
                    view.MaxLongTermFrameIdx[0] = -1;        // no long term frame indices
                    // set "previous" picture order count vars for future

                    if (pFrame->m_PictureStructureForDec < FRM_STRUCTURE)
                    {
                        pFrame->setPicOrderCnt(0, field_index);
                        pFrame->setPicNum(0, field_index);
                    }
                    else
                    {
                        int32_t poc = pFrame->PicOrderCnt(0, 3);
                        pFrame->setPicOrderCnt(pFrame->PicOrderCnt(0, 1) - poc, 0);
                        pFrame->setPicOrderCnt(pFrame->PicOrderCnt(1, 1) - poc, 1);
                        pFrame->setPicNum(0, 0);
                        pFrame->setPicNum(0, 1);
                    }

                    pFrame->m_bIDRFlag = true;
                    view.GetPOCDecoder(0)->Reset(0);
                    // set frame_num to zero for this picture, for correct
                    // FrameNumWrap
                    pFrame->setFrameNum(0);
                    sliceHeader->frame_num = 0;
                    break;
                case 6:
                    // Assign long term frame idx to current picture
                    // Value is long_term_frame_idx
                    LongTermFrameIdx = pAdaptiveMarkingInfo->value[arpmmf_idx*2];
                    bCurrentisST = false;
                    pRefFrame = view.GetDPBList(0)->findLongTermRefIdx(LongTermFrameIdx);
                    if (pRefFrame != pFrame)
                        AddItemAndRun(pFrame, pRefFrame, UNSET_REFERENCE | LONG_TERM | FULL_FRAME);

                    AddItemAndRun(pFrame, pFrame, SET_REFERENCE | LONG_TERM | (field_index ? BOTTOM_FIELD : TOP_FIELD));
                    pFrame->setLongTermFrameIdx(LongTermFrameIdx);
                    break;
                case 0:
                default:
                    // invalid mmco command in bitstream
                    assert(0);
                    umcRes = UMC_ERR_INVALID_STREAM;
                }    // switch
            }    // for arpmmf_idx
        }
    }    // not IDR picture

    if (bCurrentisST)
    {
        AdaptiveMarkingInfo* pAdaptiveMarkingInfo = pSlice->GetAdaptiveMarkingInfo();
        if (pAdaptiveMarkingInfo && pAdaptiveMarkingInfo->num_entries > 0 && !field_index)
        {
            //with MMCO we could overflow DPB after we add current picture as short term
            //this case is not valid but could occure in broken streams and we try to handle it

            uint32_t NumShortTermRefs, NumLongTermRefs;
            view.GetDPBList(0)->countActiveRefs(NumShortTermRefs, NumLongTermRefs);
            const H264SeqParamSet* sps = pSlice->GetSeqParam();

            if (NumShortTermRefs + NumLongTermRefs + 1 > sps->num_ref_frames)
            {
                DPBCommandsList::iterator
                    f = m_commandsList.begin(),
                    l = m_commandsList.end();
                for (; f != l; ++f)
                    if ((*f).m_type.isSet && !(*f).m_type.isShortTerm)
                        break;

                if (f != l)
                {
                    Undo(&(*f));
                    m_commandsList.erase(f);
                }
            }
        }

        if (sliceHeader->field_pic_flag && field_index)
        {
        }
        else
        {
            SlideWindow(view, pSlice, field_index);
        }

        AddItemAndRun(pFrame, pFrame, SET_REFERENCE | SHORT_TERM | (field_index ? BOTTOM_FIELD : TOP_FIELD));
    }

    return umcRes;
}

/****************************************************************************************************/
//
/****************************************************************************************************/
static
bool IsNeedSPSInvalidate(const H264SeqParamSet *old_sps, const H264SeqParamSet *new_sps)
{
    if (!old_sps || !new_sps)
        return false;

    //if (new_sps->no_output_of_prior_pics_flag)
      //  return true;

    if (old_sps->frame_width_in_mbs != new_sps->frame_width_in_mbs)
        return true;

    if (old_sps->frame_height_in_mbs != new_sps->frame_height_in_mbs)
        return true;

    if (old_sps->vui.max_dec_frame_buffering < new_sps->vui.max_dec_frame_buffering)
        return true;

    if (old_sps->chroma_format_idc != new_sps->chroma_format_idc)
        return true;

    if (old_sps->profile_idc != new_sps->profile_idc)
        return true;

    if (old_sps->bit_depth_luma != new_sps->bit_depth_luma)
        return true;

    if (old_sps->bit_depth_chroma != new_sps->bit_depth_chroma)
        return true;

    return false;
}

/****************************************************************************************************/
// MVC_Extension class routine
/****************************************************************************************************/
MVC_Extension::MVC_Extension()
    : m_temporal_id(H264_MAX_TEMPORAL_ID)
    , m_priority_id(63)
    , m_level_idc(0)
    , m_currentDisplayView(BASE_VIEW)
    , m_currentView((uint32_t)INVALID_VIEW_ID)
    , m_decodingMode(UNKNOWN_DECODING_MODE)
{
    Reset();
}

MVC_Extension::~MVC_Extension()
{
    Close();
}

bool MVC_Extension::IsExtension() const
{
    return m_decodingMode != AVC_DECODING_MODE;
}

Status MVC_Extension::Init()
{
    MVC_Extension::Close();

    Status umcRes = AllocateView((uint32_t)INVALID_VIEW_ID);
    if (UMC_OK != umcRes)
    {
        return umcRes;
    }

    return UMC_OK;
}

void MVC_Extension::Close()
{
    MVC_Extension::Reset();
    m_viewIDsList.clear();
    m_views.clear();
}

void MVC_Extension::Reset()
{
    m_temporal_id = H264_MAX_TEMPORAL_ID;
    m_priority_id = 63;
    m_level_idc = 0;
    m_currentDisplayView = BASE_VIEW;
    m_currentView = (uint32_t)INVALID_VIEW_ID;
    m_decodingMode = UNKNOWN_DECODING_MODE;

    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();
    for (; iter != iter_end; ++iter)
    {
        iter->Reset();
    }
}

ViewItem * MVC_Extension::FindView(int32_t viewId)
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    // run over the list and try to find the corresponding view
    for (; iter != iter_end; ++iter)
    {
        ViewItem & item = *iter;
        if (item.viewId == viewId)
        {
            return &item;
        }
    }

    return NULL;
}

ViewItem & MVC_Extension::GetView(int32_t viewId)
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    // run over the list and try to find the corresponding view
    for (; iter != iter_end; ++iter)
    {
        ViewItem & item = *iter;

        //if (viewId == INVALID_VIEW_ID && item.m_isDisplayable)
          //  return &item;

        if (item.viewId == viewId)
        {
            return item;
        }
    }

    throw h264_exception(UMC_ERR_FAILED);
    //return NULL;
}

void MVC_Extension::MoveViewToHead(int32_t view_id)
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    // run over the list and try to find the corresponding view
    for (; iter != iter_end; ++iter)
    {
        if (iter->viewId == view_id)
        {
            ViewItem item = *iter;
            ViewItem &front = m_views.front();
            *iter = front;
            front = item;
            break;
        }
    }
}

ViewItem & MVC_Extension::GetViewByNumber(int32_t viewNum)
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    // run over the list and try to find the corresponding view
    for (int32_t i = 0; iter != iter_end; ++iter, i++)
    {
        ViewItem & item = *iter;
        if (i == viewNum)
            return item;
    }

    throw h264_exception(UMC_ERR_FAILED);
    //return NULL;
}

int32_t MVC_Extension::GetBaseViewId() const
{
    return BASE_VIEW;
}

int32_t MVC_Extension::GetViewCount() const
{
    return (int32_t)m_views.size();
}

bool MVC_Extension::IncreaseCurrentView()
{
    bool isWrapped = false;
    for (size_t i = 0; i < m_views.size(); ++i)
    {
        m_currentDisplayView++;
        if (m_views.size() == m_currentDisplayView)
        {
            m_currentDisplayView = BASE_VIEW;
            isWrapped = true;
        }

        ViewItem & view = GetViewByNumber(m_currentDisplayView);
        if (view.m_isDisplayable)
            break;
    }

    return isWrapped;
}

void MVC_Extension::SetTemporalId(uint32_t temporalId)
{
    m_temporal_id = temporalId;
}

Status MVC_Extension::SetViewList(const std::vector<uint32_t> & targetView, const std::vector<uint32_t> & dependencyList)
{
    for (size_t i = 0; i < targetView.size(); i++)
    {
        m_viewIDsList.push_back(targetView[i]);
    }

    for (size_t i = 0; i < dependencyList.size(); i++)
    {
        Status umcRes = AllocateView(dependencyList[i]);
        if (UMC_OK != umcRes)
        {
            return umcRes;
        }

        ViewItem & viewItem = GetView(dependencyList[i]);
        viewItem.m_isDisplayable = false;
        m_viewIDsList.push_back(dependencyList[i]);
    }

    m_viewIDsList.sort();
    m_viewIDsList.unique();

    return UMC_OK;
}

uint32_t MVC_Extension::GetLevelIDC() const
{
    return m_level_idc;
}

ViewItem & MVC_Extension::AllocateAndInitializeView(H264Slice * slice)
{
    if (slice == nullptr)
    {
        throw h264_exception(UMC_ERR_NULL_PTR);
    }
    ViewItem * view = FindView(slice->GetSliceHeader()->nal_ext.mvc.view_id);
    if (view)
        return *view;

    Status umcRes = AllocateView(slice->GetSliceHeader()->nal_ext.mvc.view_id);
    if (UMC_OK != umcRes)
    {
        throw h264_exception(umcRes);
    }

    ViewItem &viewRef = GetView(slice->GetSliceHeader()->nal_ext.mvc.view_id);
    viewRef.SetDPBSize(const_cast<H264SeqParamSet*>(slice->m_pSeqParamSet), m_level_idc);
    return viewRef;
}

Status MVC_Extension::AllocateView(int32_t view_id)
{
    // check error(s)
    if (((int32_t)H264_MAX_NUM_VIEW <= view_id) && (view_id != (int32_t)INVALID_VIEW_ID) )
    {
        return UMC_ERR_INVALID_PARAMS;
    }

    // already allocated
    if (FindView(view_id))
    {
        return UMC_OK;
    }

    if (FindView(INVALID_VIEW_ID))
    {
        ViewItem &view = GetView((uint32_t)INVALID_VIEW_ID);
        view.viewId = view_id;
        return UMC_OK;
    }

    ViewItem view;
    try
    {
        // allocate DPB and POC counter
        for (uint32_t i = 0; i < MAX_NUM_LAYERS; i++)
        {
            view.pDPB[i].reset(new H264DBPList());
            view.pPOCDec[i].reset(new POCDecoder());
        }
        view.viewId = view_id;
        view.maxDecFrameBuffering = 16;
    }
    catch(...)
    {
        return UMC_ERR_ALLOC;
    }

    // reset the variables
    m_views.push_back(view);

    return UMC_OK;

} // Status AllocateView(uint32_t view_id)

bool MVC_Extension::IsShouldSkipSlice(H264NalExtension *nal_ext)
{
    // check is view at view_list;
    ViewIDsList::const_iterator iter = std::find(m_viewIDsList.begin(), m_viewIDsList.end(), nal_ext->mvc.view_id);
    if (iter == m_viewIDsList.end() && m_viewIDsList.size())
        return true;

    if (nal_ext->mvc.temporal_id > m_temporal_id)
        return true;

    if (nal_ext->mvc.priority_id > m_priority_id)
        return true;

    return false;
}

bool MVC_Extension::IsShouldSkipSlice(H264Slice * slice)
{
    bool isMVC = slice->GetSeqMVCParam() != 0;

    if (!isMVC)
        return false;

    return IsShouldSkipSlice(&slice->GetSliceHeader()->nal_ext);
}

void MVC_Extension::AnalyzeDependencies(const H264SeqParamSetMVCExtension *)
{
}

void MVC_Extension::ChooseLevelIdc(const H264SeqParamSetMVCExtension * extension, uint8_t baseViewLevelIDC, uint8_t extensionLevelIdc)
{
    if (m_level_idc)
        return;

    assert(extension->viewInfo.size() == extension->num_views_minus1 + 1);

    if (m_viewIDsList.empty())
    {
        for (const auto & viewInfo : extension->viewInfo)
        {
            m_viewIDsList.push_back(viewInfo.view_id);
        }

        ViewIDsList::iterator iter = m_viewIDsList.begin();
        ViewIDsList::iterator iter_end = m_viewIDsList.end();
        for (; iter != iter_end; ++iter)
        {
            uint32_t targetView = *iter;
            ViewItem * view = FindView(targetView);
            if (!view)
                continue;

            view->m_isDisplayable = true;
        }
    }

    AnalyzeDependencies(extension);

    m_level_idc = 0;

    for (const auto & levelInfo : extension->levelInfo)
    {
        for (const auto & operationPoint : levelInfo.opsInfo)
        {
            bool foundAll = true;

            for (const auto & targetView : m_viewIDsList)
            {
                auto it = std::find_if(operationPoint.applicable_op_target_view_id.begin(), operationPoint.applicable_op_target_view_id.end(),
                                       [targetView](const uint16_t & item){ return item == targetView; });

                if (it == operationPoint.applicable_op_target_view_id.end())
                {
                    foundAll = false;
                    break;
                }
            }

            if (!foundAll)
                break;

            if (!m_level_idc || m_level_idc > levelInfo.level_idc)
            {
                m_level_idc = levelInfo.level_idc;
            }
        }
    }

    m_level_idc = std::max({baseViewLevelIDC, extensionLevelIdc, m_level_idc});

    assert(m_level_idc != 0);
}

/****************************************************************************************************/
// Skipping class routine
/****************************************************************************************************/
Skipping::Skipping()
    : m_VideoDecodingSpeed(0)
    , m_SkipCycle(1)
    , m_ModSkipCycle(1)
    , m_PermanentTurnOffDeblocking(0)
    , m_SkipFlag(0)
    , m_NumberOfSkippedFrames(0)
{
}

Skipping::~Skipping()
{
}

void Skipping::Reset()
{
    m_VideoDecodingSpeed = 0;
    m_SkipCycle = 0;
    m_ModSkipCycle = 0;
    m_PermanentTurnOffDeblocking = 0;
    m_NumberOfSkippedFrames = 0;
}

void Skipping::PermanentDisableDeblocking(bool disable)
{
    m_PermanentTurnOffDeblocking = disable ? 3 : 0;
}

bool Skipping::IsShouldSkipDeblocking(H264DecoderFrame * pFrame, int32_t field)
{
    return (IS_SKIP_DEBLOCKING_MODE_PREVENTIVE || IS_SKIP_DEBLOCKING_MODE_PERMANENT ||
        (IS_SKIP_DEBLOCKING_MODE_NON_REF && !pFrame->GetAU(field)->IsReference()));
}

bool Skipping::IsShouldSkipFrame(H264DecoderFrame * pFrame, int32_t /*field*/)
{
    bool isShouldSkip = false;

    //bool isReference = pFrame->GetAU(field)->IsReference();

    bool isReference0 = pFrame->GetAU(0)->IsReference();
    bool isReference1 = pFrame->GetAU(1)->IsReference();

    bool isReference = isReference0 || isReference1;

    if ((m_VideoDecodingSpeed > 0) && !isReference)
    {
        if ((m_SkipFlag % m_ModSkipCycle) == 0)
        {
            isShouldSkip = true;
        }

        m_SkipFlag++;

        if (m_SkipFlag >= m_SkipCycle)
            m_SkipFlag = 0;
    }

    if (isShouldSkip)
        m_NumberOfSkippedFrames++;

    return isShouldSkip;
}

void Skipping::ChangeVideoDecodingSpeed(int32_t & num)
{
    m_VideoDecodingSpeed += num;

    if (m_VideoDecodingSpeed < 0)
        m_VideoDecodingSpeed = 0;
    if (m_VideoDecodingSpeed > 7)
        m_VideoDecodingSpeed = 7;

    num = m_VideoDecodingSpeed;

    int32_t deblocking_off = m_PermanentTurnOffDeblocking;

    if (m_VideoDecodingSpeed > 6)
    {
        m_SkipCycle = 1;
        m_ModSkipCycle = 1;
        m_PermanentTurnOffDeblocking = 2;
    }
    else if (m_VideoDecodingSpeed > 5)
    {
        m_SkipCycle = 1;
        m_ModSkipCycle = 1;
        m_PermanentTurnOffDeblocking = 0;
    }
    else if (m_VideoDecodingSpeed > 4)
    {
        m_SkipCycle = 3;
        m_ModSkipCycle = 2;
        m_PermanentTurnOffDeblocking = 1;
    }
    else if (m_VideoDecodingSpeed > 3)
    {
        m_SkipCycle = 3;
        m_ModSkipCycle = 2;
        m_PermanentTurnOffDeblocking = 0;
    }
    else if (m_VideoDecodingSpeed > 2)
    {
        m_SkipCycle = 2;
        m_ModSkipCycle = 2;
        m_PermanentTurnOffDeblocking = 0;
    }
    else if (m_VideoDecodingSpeed > 1)
    {
        m_SkipCycle = 3;
        m_ModSkipCycle = 3;
        m_PermanentTurnOffDeblocking = 0;
    }
    else if (m_VideoDecodingSpeed == 1)
    {
        m_SkipCycle = 4;
        m_ModSkipCycle = 4;
        m_PermanentTurnOffDeblocking = 0;
    }
    else
    {
        m_PermanentTurnOffDeblocking = 0;
    }

    if (deblocking_off == 3)
        m_PermanentTurnOffDeblocking = 3;
}

Skipping::SkipInfo Skipping::GetSkipInfo() const
{
    Skipping::SkipInfo info;
    info.isDeblockingTurnedOff = (m_VideoDecodingSpeed == 5) || (m_VideoDecodingSpeed == 7);
    info.numberOfSkippedFrames = m_NumberOfSkippedFrames;
    return info;
}

/****************************************************************************************************/
// POCDecoder
/****************************************************************************************************/
POCDecoder::POCDecoder()
{
    Reset();
}

POCDecoder::~POCDecoder()
{
}

void POCDecoder::Reset(int32_t IDRFrameNum)
{
    m_PicOrderCnt = 0;
    m_PicOrderCntMsb = 0;
    m_PicOrderCntLsb = 0;
    m_FrameNum = IDRFrameNum;
    m_PrevFrameRefNum = IDRFrameNum;
    m_FrameNumOffset = 0;
    m_TopFieldPOC = 0;
    m_BottomFieldPOC = 0;
}

void POCDecoder::DecodePictureOrderCount(const H264Slice *slice, int32_t frame_num)
{
    const H264SliceHeader *sliceHeader = slice->GetSliceHeader();
    const H264SeqParamSet* sps = slice->GetSeqParam();

    int32_t uMaxFrameNum = (1<<sps->log2_max_frame_num);

    if (sps->pic_order_cnt_type == 0)
    {
        // pic_order_cnt type 0
        int32_t CurrPicOrderCntMsb;
        int32_t MaxPicOrderCntLsb = sps->MaxPicOrderCntLsb;

        if ((sliceHeader->pic_order_cnt_lsb < m_PicOrderCntLsb) &&
             ((m_PicOrderCntLsb - sliceHeader->pic_order_cnt_lsb) >= (MaxPicOrderCntLsb >> 1)))
            CurrPicOrderCntMsb = m_PicOrderCntMsb + MaxPicOrderCntLsb;
        else if ((sliceHeader->pic_order_cnt_lsb > m_PicOrderCntLsb) &&
                ((sliceHeader->pic_order_cnt_lsb - m_PicOrderCntLsb) > (MaxPicOrderCntLsb >> 1)))
            CurrPicOrderCntMsb = m_PicOrderCntMsb - MaxPicOrderCntLsb;
        else
            CurrPicOrderCntMsb = m_PicOrderCntMsb;

        if (sliceHeader->nal_ref_idc)
        {
            // reference picture
            m_PicOrderCntMsb = CurrPicOrderCntMsb & (~(MaxPicOrderCntLsb - 1));
            m_PicOrderCntLsb = sliceHeader->pic_order_cnt_lsb;
        }
        m_PicOrderCnt = CurrPicOrderCntMsb + sliceHeader->pic_order_cnt_lsb;
        if (sliceHeader->field_pic_flag == 0)
        {
             m_TopFieldPOC = CurrPicOrderCntMsb + sliceHeader->pic_order_cnt_lsb;
             m_BottomFieldPOC = m_TopFieldPOC + sliceHeader->delta_pic_order_cnt_bottom;
        }

    }    // pic_order_cnt type 0
    else if (sps->pic_order_cnt_type == 1)
    {
        // pic_order_cnt type 1
        uint32_t i;
        uint32_t uAbsFrameNum;    // frame # relative to last IDR pic
        uint32_t uPicOrderCycleCnt = 0;
        uint32_t uFrameNuminPicOrderCntCycle = 0;
        int32_t ExpectedPicOrderCnt = 0;
        int32_t ExpectedDeltaPerPicOrderCntCycle;
        uint32_t uNumRefFramesinPicOrderCntCycle = sps->num_ref_frames_in_pic_order_cnt_cycle;

        if (frame_num < m_FrameNum)
            m_FrameNumOffset += uMaxFrameNum;

        if (uNumRefFramesinPicOrderCntCycle != 0)
            uAbsFrameNum = m_FrameNumOffset + frame_num;
        else
            uAbsFrameNum = 0;

        if ((sliceHeader->nal_ref_idc == false)  && (uAbsFrameNum > 0))
            uAbsFrameNum--;

        if (uAbsFrameNum)
        {
            uPicOrderCycleCnt = (uAbsFrameNum - 1) /
                    uNumRefFramesinPicOrderCntCycle;
            uFrameNuminPicOrderCntCycle = (uAbsFrameNum - 1) %
                    uNumRefFramesinPicOrderCntCycle;
        }

        ExpectedDeltaPerPicOrderCntCycle = 0;
        for (i=0; i < uNumRefFramesinPicOrderCntCycle; i++)
        {
            ExpectedDeltaPerPicOrderCntCycle +=
                sps->poffset_for_ref_frame[i];
        }

        if (uAbsFrameNum)
        {
            ExpectedPicOrderCnt = uPicOrderCycleCnt * ExpectedDeltaPerPicOrderCntCycle;
            for (i=0; i<=uFrameNuminPicOrderCntCycle; i++)
            {
                ExpectedPicOrderCnt +=
                    sps->poffset_for_ref_frame[i];
            }
        }
        else
            ExpectedPicOrderCnt = 0;

        if (sliceHeader->nal_ref_idc == false)
            ExpectedPicOrderCnt += sps->offset_for_non_ref_pic;
        m_PicOrderCnt = ExpectedPicOrderCnt + sliceHeader->delta_pic_order_cnt[0];
        if( sliceHeader->field_pic_flag==0)
        {
            m_TopFieldPOC = ExpectedPicOrderCnt + sliceHeader->delta_pic_order_cnt[ 0 ];
            m_BottomFieldPOC = m_TopFieldPOC +
                sps->offset_for_top_to_bottom_field + sliceHeader->delta_pic_order_cnt[ 1 ];
        }
        else if( ! sliceHeader->bottom_field_flag)
            m_PicOrderCnt = ExpectedPicOrderCnt + sliceHeader->delta_pic_order_cnt[ 0 ];
        else
            m_PicOrderCnt  = ExpectedPicOrderCnt + sps->offset_for_top_to_bottom_field + sliceHeader->delta_pic_order_cnt[ 0 ];
    }    // pic_order_cnt type 1
    else if (sps->pic_order_cnt_type == 2)
    {
        // pic_order_cnt type 2
        int32_t iMaxFrameNum = (1<<sps->log2_max_frame_num);
        uint32_t uAbsFrameNum;    // frame # relative to last IDR pic

        if (frame_num < m_FrameNum)
            m_FrameNumOffset += iMaxFrameNum;
        uAbsFrameNum = frame_num + m_FrameNumOffset;
        m_PicOrderCnt = uAbsFrameNum*2;
        if (sliceHeader->nal_ref_idc == false)
            m_PicOrderCnt--;
        m_TopFieldPOC = m_PicOrderCnt;
        m_BottomFieldPOC = m_PicOrderCnt;

    }    // pic_order_cnt type 2

    if (sliceHeader->nal_ref_idc)
    {
        m_PrevFrameRefNum = frame_num;
    }

    m_FrameNum = frame_num;
}    // decodePictureOrderCount

int32_t POCDecoder::DetectFrameNumGap(const H264Slice *slice, bool ignore_gaps_allowed_flag)
{
    const H264SeqParamSet* sps = slice->GetSeqParam();

    if (!ignore_gaps_allowed_flag && sps->gaps_in_frame_num_value_allowed_flag != 1)
        return 0;

    const H264SliceHeader *sliceHeader = slice->GetSliceHeader();

    int32_t uMaxFrameNum = (1<<sps->log2_max_frame_num);
    int32_t frameNumGap;

    if (sliceHeader->IdrPicFlag)
        return 0;

    // Capture any frame_num gap
    if (sliceHeader->frame_num != m_PrevFrameRefNum &&
        sliceHeader->frame_num != (m_PrevFrameRefNum + 1) % uMaxFrameNum)
    {
        // note this could be negative if frame num wrapped

        if (sliceHeader->frame_num > m_PrevFrameRefNum - 1)
        {
            frameNumGap = (sliceHeader->frame_num - m_PrevFrameRefNum - 1) % uMaxFrameNum;
        }
        else
        {
            frameNumGap = (uMaxFrameNum - (m_PrevFrameRefNum + 1 - sliceHeader->frame_num)) % uMaxFrameNum;
        }
    }
    else
    {
        frameNumGap = 0;
    }

    return frameNumGap;
}

void POCDecoder::DecodePictureOrderCountFrameGap(H264DecoderFrame *pFrame,
                                                 const H264SliceHeader *pSliceHeader,
                                                 int32_t frameNum)
{
    m_PrevFrameRefNum = frameNum;
    m_FrameNum = frameNum;

    if (pSliceHeader->field_pic_flag)
    {
        pFrame->setPicOrderCnt(m_PicOrderCnt, 0);
        pFrame->setPicOrderCnt(m_PicOrderCnt, 1);
    }
    else
    {
        pFrame->setPicOrderCnt(m_TopFieldPOC, 0);
        pFrame->setPicOrderCnt(m_BottomFieldPOC, 1);
    }
}

void POCDecoder::DecodePictureOrderCountFakeFrames(H264DecoderFrame *pFrame,
                                                   const H264SliceHeader *pSliceHeader)
{
    if (pSliceHeader->field_pic_flag)
    {
        pFrame->setPicOrderCnt(m_PicOrderCnt, 0);
        pFrame->setPicOrderCnt(m_PicOrderCnt, 1);
    }
    else
    {
        pFrame->setPicOrderCnt(m_TopFieldPOC, 0);
        pFrame->setPicOrderCnt(m_BottomFieldPOC, 1);
    }

}

void POCDecoder::DecodePictureOrderCountInitFrame(H264DecoderFrame *pFrame,
                                                  int32_t fieldIdx)
{
    //transfer previosly calculated PicOrdeCnts into current Frame
    if (pFrame->m_PictureStructureForRef < FRM_STRUCTURE)
    {
        pFrame->setPicOrderCnt(m_PicOrderCnt, fieldIdx);
        if (!fieldIdx) // temporally set same POC for second field
            pFrame->setPicOrderCnt(m_PicOrderCnt, 1);
    }
    else
    {
        pFrame->setPicOrderCnt(m_TopFieldPOC, 0);
        pFrame->setPicOrderCnt(m_BottomFieldPOC, 1);
    }
}
/****************************************************************************************************/
// SEI_Storer
/****************************************************************************************************/
SEI_Storer::SEI_Storer()
{
    m_offset = 0;
    Reset();
}

SEI_Storer::~SEI_Storer()
{
    Close();
}

void SEI_Storer::Init()
{
    Close();
    m_data.resize(MAX_BUFFERED_SIZE);
    m_payloads.resize(START_ELEMENTS);
    m_offset = 0;
    m_lastUsed = 2;
}

void SEI_Storer::Close()
{
    Reset();
    m_data.clear();
    m_payloads.clear();
}

void SEI_Storer::Reset()
{
    m_lastUsed = 2;
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        m_payloads[i].isUsed = 0;
    }
}

void SEI_Storer::SetFrame(H264DecoderFrame * frame, int32_t auIndex)
{
    assert(frame);
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].frame == 0 && m_payloads[i].isUsed && m_payloads[i].auID == auIndex)
        {
            m_payloads[i].frame = frame;
        }
    }
}

void SEI_Storer::SetAUID(int32_t auIndex)
{
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].isUsed && m_payloads[i].auID == -1)
        {
            m_payloads[i].auID = auIndex;
        }
    }
}

void SEI_Storer::SetTimestamp(H264DecoderFrame * frame)
{
    assert(frame);
    double ts = frame->m_dFrameTime;

    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].frame == frame)
        {
            m_payloads[i].timestamp = ts;
            if (m_payloads[i].isUsed)
                m_payloads[i].isUsed = m_lastUsed;
        }
    }

    m_lastUsed++;
}

const SEI_Storer::SEI_Message * SEI_Storer::GetPayloadMessage()
{
    SEI_Storer::SEI_Message * msg = 0;

    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (m_payloads[i].isUsed > 1)
        {
            if (!msg || msg->isUsed > m_payloads[i].isUsed || msg->inputID > m_payloads[i].inputID)
            {
                msg = &m_payloads[i];
            }
        }
    }

    if (msg)
    {
        msg->isUsed = 0;
        msg->frame = 0;
        msg->auID = 0;
        msg->inputID = 0;
    }

    return msg;
}

SEI_Storer::SEI_Message* SEI_Storer::AddMessage(UMC::MediaDataEx *nalUnit, SEI_TYPE type, int32_t auIndex)
{
    size_t sz = nalUnit->GetDataSize();

    if (sz > (m_data.size() >> 2))
        return 0;

    if (m_offset + sz > m_data.size())
    {
        m_offset = 0;
    }

    // clear overwriting messages:
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (!m_payloads[i].isUsed)
            continue;

        SEI_Message & mmsg = m_payloads[i];

        if ((m_offset + sz > mmsg.offset) &&
            (m_offset < mmsg.offset + mmsg.msg_size))
        {
            m_payloads[i].isUsed = 0;
            return 0;
        }
    }

    size_t freeSlot = 0;
    for (uint32_t i = 0; i < m_payloads.size(); i++)
    {
        if (!m_payloads[i].isUsed)
        {
            freeSlot = i;
            break;
        }
    }

    if (m_payloads[freeSlot].isUsed)
    {
        if (m_payloads.size() >= MAX_ELEMENTS)
            return 0;

        m_payloads.push_back(SEI_Message());
        freeSlot = m_payloads.size() - 1;
    }

    m_payloads[freeSlot].msg_size = sz;
    m_payloads[freeSlot].offset = m_offset;
    m_payloads[freeSlot].timestamp = 0;
    m_payloads[freeSlot].frame = 0;
    m_payloads[freeSlot].isUsed = 1;
    m_payloads[freeSlot].inputID = m_lastUsed++;
    m_payloads[freeSlot].data = &(m_data.front()) + m_offset;
    m_payloads[freeSlot].type = type;
    m_payloads[freeSlot].auID = auIndex;

    MFX_INTERNAL_CPY(&m_data[m_offset], (uint8_t*)nalUnit->GetDataPointer(), (uint32_t)sz);

    m_offset += sz;
    return &m_payloads[freeSlot];
}

ViewItem::ViewItem()
{
    viewId = 0;
    maxDecFrameBuffering = 0;
    maxNumReorderFrames  = 16; // Max DPB size

    Reset();

} // ViewItem::ViewItem(void)

ViewItem::ViewItem(const ViewItem &src)
{
    Reset();

    viewId = src.viewId;
    for (uint32_t i = 0; i < MAX_NUM_LAYERS; i++) {
        pDPB[i].reset(src.pDPB[i].release());
        pPOCDec[i].reset(src.pPOCDec[i].release());
        MaxLongTermFrameIdx[i] = src.MaxLongTermFrameIdx[i];
    }
    maxDecFrameBuffering = src.maxDecFrameBuffering;
    maxNumReorderFrames  = src.maxNumReorderFrames;

} // ViewItem::ViewItem(const ViewItem &src)

ViewItem &ViewItem::operator =(const ViewItem &src)
{
    Reset();

    viewId = src.viewId;
    for (uint32_t i = 0; i < MAX_NUM_LAYERS; i++) {
        pDPB[i].reset(src.pDPB[i].release());
        pPOCDec[i].reset(src.pPOCDec[i].release());
        MaxLongTermFrameIdx[i] = src.MaxLongTermFrameIdx[i];
    }
    maxDecFrameBuffering = src.maxDecFrameBuffering;
    maxNumReorderFrames  = src.maxNumReorderFrames;

    return *this;
}

ViewItem::~ViewItem()
{
    Close();

} // ViewItem::ViewItem(void)

Status ViewItem::Init(uint32_t view_id)
{
    // release the object before initialization
    Close();

    try
    {
        for (uint32_t i = 0; i < MAX_NUM_LAYERS; i++) {
            // allocate DPB and POC counter
            pDPB[i].reset(new H264DBPList());
            pPOCDec[i].reset(new POCDecoder());
        }
    }
    catch(...)
    {
        return UMC_ERR_ALLOC;
    }

    // save the ID
    viewId = view_id;
    localFrameTime = 0;
    pCurFrame = 0;
    return UMC_OK;

} // Status ViewItem::Init(uint32_t view_id)

void ViewItem::Close(void)
{
    // Reset the parameters before close
    Reset();

    for (int32_t i = MAX_NUM_LAYERS - 1; i >= 0; i--) {
        if (pDPB[i].get())
        {
            pDPB[i].reset();
        }

        if (pPOCDec[i].get())
        {
            pPOCDec[i].reset();
        }

        MaxLongTermFrameIdx[i] = 0;
    }

    viewId = (uint32_t)INVALID_VIEW_ID;
    maxDecFrameBuffering = 1;
} // void ViewItem::Close(void)

void ViewItem::Reset(void)
{
    for (int32_t i = MAX_NUM_LAYERS - 1; i >= 0; i--)
    {
        if (pDPB[i].get())
        {
            pDPB[i]->Reset();
        }

        if (pPOCDec[i].get())
        {
            pPOCDec[i]->Reset();
        }
    }

    pCurFrame = 0;
    localFrameTime = 0;
    m_isDisplayable = true;

} // void ViewItem::Reset(void)

void ViewItem::SetDPBSize(H264SeqParamSet *pSps, uint8_t & level_idc)
{
    // calculate the new DPB size value
    maxDecFrameBuffering = CalculateDPBSize(level_idc ? level_idc : pSps->level_idc,
                               pSps->frame_width_in_mbs * 16,
                               pSps->frame_height_in_mbs * 16,
                               pSps->num_ref_frames);

    maxDecFrameBuffering = pSps->vui.max_dec_frame_buffering ? pSps->vui.max_dec_frame_buffering : maxDecFrameBuffering;
    if (pSps->vui.max_dec_frame_buffering > maxDecFrameBuffering)
        pSps->vui.max_dec_frame_buffering = (uint8_t)maxDecFrameBuffering;

    // provide the new value to the DPBList
    for (uint32_t i = 0; i < MAX_NUM_LAYERS; i++)
    {
        if (pDPB[i].get())
        {
            pDPB[i]->SetDPBSize(maxDecFrameBuffering);
        }
    }

    if (pSps->vui.bitstream_restriction_flag)
    {
        maxNumReorderFrames = pSps->vui.num_reorder_frames;
    }
    else
    {
        // Regarding H264 Specification - E.2.1 VUI parameters semantics :
        // When the max_num_reorder_frames syntax element is not present,
        // the value of max_num_reorder_frames value shall be inferred as follows:
        if ((1 == pSps->constraint_set3_flag) &&
            (H264VideoDecoderParams::H264_PROFILE_CAVLC444_INTRA == pSps->profile_idc ||
             H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH  == pSps->profile_idc ||
             H264VideoDecoderParams::H264_PROFILE_HIGH           == pSps->profile_idc ||
             H264VideoDecoderParams::H264_PROFILE_HIGH10         == pSps->profile_idc ||
             H264VideoDecoderParams::H264_PROFILE_HIGH422        == pSps->profile_idc ||
             H264VideoDecoderParams::H264_PROFILE_HIGH444_PRED   == pSps->profile_idc)
        )
        {
            maxNumReorderFrames = 0;
        }
        else
        {
            maxNumReorderFrames = maxDecFrameBuffering;
        }
    }
} // void ViewItem::SetDPBSize(const H264SeqParamSet *pSps)

/****************************************************************************************************/
// TaskSupplier
/****************************************************************************************************/
TaskSupplier::TaskSupplier()
    : AU_Splitter(&m_ObjHeap)

    , m_pSegmentDecoder(0)
    , m_iThreadNum(0)
    , m_local_delta_frame_time(0)
    , m_use_external_framerate(false)

    , m_pLastSlice(0)
    , m_pLastDisplayed(0)
    , m_pMemoryAllocator(0)
    , m_pFrameAllocator(0)
    , m_WaitForIDR(false)
    , m_DPBSizeEx(0)
    , m_frameOrder(0)
    , m_pTaskBroker(0)
    , m_UIDFrameCounter(0)
    , m_sei_messages(0)
    , m_isInitialized(false)
    , m_ignoreLevelConstrain(false)
{
}

TaskSupplier::~TaskSupplier()
{
    Close();
}

Status TaskSupplier::Init(VideoDecoderParams *init)
{
    if (NULL == init)
        return UMC_ERR_NULL_PTR;

    Close();

    m_DPBSizeEx = 0;

    m_initializationParams = *init;

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    DPBOutput::Reset(m_iThreadNum != 1);
    AU_Splitter::Init();
    Status umcRes = SVC_Extension::Init();
    if (UMC_OK != umcRes)
    {
        return umcRes;
    }

    switch (m_initializationParams.info.profile) // after MVC_Extension::Init()
    {
    case 0:
        m_decodingMode = UNKNOWN_DECODING_MODE;
        break;
    case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:
        m_decodingMode = MVC_DECODING_MODE;
        break;
    default:
        m_decodingMode = AVC_DECODING_MODE;
        break;
    }

    // create slice decoder(s)
    m_pSegmentDecoder = new H264SegmentDecoderBase *[m_iThreadNum];
    if (NULL == m_pSegmentDecoder)
        return UMC_ERR_ALLOC;
    memset(m_pSegmentDecoder, 0, sizeof(H264SegmentDecoderBase *) * m_iThreadNum);

    CreateTaskBroker();
    m_pTaskBroker->Init(m_iThreadNum);

    for (uint32_t i = 0; i < m_iThreadNum; i += 1)
    {
        if (UMC_OK != m_pSegmentDecoder[i]->Init(i))
            return UMC_ERR_INIT;
    }

    m_local_delta_frame_time = 1.0/30;
    m_frameOrder = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    m_DPBSizeEx = m_iThreadNum;

    m_isInitialized = true;

    m_ignoreLevelConstrain = ((H264VideoDecoderParams *)init)->m_ignore_level_constrain;

    return UMC_OK;
}

Status TaskSupplier::PreInit(VideoDecoderParams *init)
{
    if (m_isInitialized)
        return UMC_OK;

    if (NULL == init)
        return UMC_ERR_NULL_PTR;

    Close();

    m_DPBSizeEx = 0;

    SVC_Extension::Init();

    int32_t nAllowedThreadNumber = init->numThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;

    // calculate number of slice decoders.
    // It should be equal to CPU number
    m_iThreadNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);

    AU_Splitter::Init();
    DPBOutput::Reset(m_iThreadNum != 1);

    m_local_delta_frame_time = 1.0/30;
    m_frameOrder             = 0;
    m_use_external_framerate = 0 < init->info.framerate;

    if (m_use_external_framerate)
    {
        m_local_delta_frame_time = 1 / init->info.framerate;
    }

    m_DPBSizeEx = m_iThreadNum;

    m_ignoreLevelConstrain = ((H264VideoDecoderParams *)init)->m_ignore_level_constrain;

    return UMC_OK;
}

void TaskSupplier::Close()
{
    if (m_pTaskBroker)
    {
        m_pTaskBroker->Release();
    }

// from reset

    {
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();
    for (; iter != iter_end; ++iter)
    {
        for (H264DecoderFrame *pFrame = iter->GetDPBList(0)->head(); pFrame; pFrame = pFrame->future())
        {
            pFrame->FreeResources();
        }
    }
    }

    if (m_pSegmentDecoder)
    {
        for (uint32_t i = 0; i < m_iThreadNum; i += 1)
        {
            delete m_pSegmentDecoder[i];
            m_pSegmentDecoder[i] = 0;
        }
    }

    SVC_Extension::Close();
    AU_Splitter::Close();
    DPBOutput::Reset(m_iThreadNum != 1);
    DecReferencePictureMarking::Reset();
    ErrorStatus::Reset();

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_pLastSlice->DecrementReference();
        m_pLastSlice = 0;
    }

    m_accessUnit.Release();
    m_Headers.Reset(false);
    Skipping::Reset();
    m_ObjHeap.Release();

    m_frameOrder               = 0;

    m_WaitForIDR        = true;

    m_pLastDisplayed = 0;

    delete m_sei_messages;
    m_sei_messages = 0;

// from reset

    delete[] m_pSegmentDecoder;
    m_pSegmentDecoder = 0;

    delete m_pTaskBroker;
    m_pTaskBroker = 0;

    m_iThreadNum = 0;

    m_DPBSizeEx = 1;

    m_isInitialized = false;

} // void TaskSupplier::Close()

void TaskSupplier::Reset()
{
    if (m_pTaskBroker)
        m_pTaskBroker->Reset();

    {
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();
    for (; iter != iter_end; ++iter)
    {
        for (H264DecoderFrame *pFrame = iter->GetDPBList(0)->head(); pFrame; pFrame = pFrame->future())
        {
            pFrame->FreeResources();
        }
    }
    }

    if (m_sei_messages)
        m_sei_messages->Reset();

    SVC_Extension::Reset();
    AU_Splitter::Reset();
    DPBOutput::Reset(m_iThreadNum != 1);
    DecReferencePictureMarking::Reset();
    m_accessUnit.Release();
    ErrorStatus::Reset();

    switch (m_initializationParams.info.profile) // after MVC_Extension::Init()
    {
    case 0:
        m_decodingMode = UNKNOWN_DECODING_MODE;
        break;
    case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:
        m_decodingMode = MVC_DECODING_MODE;
        break;
    default:
        m_decodingMode = AVC_DECODING_MODE;
        break;
    }

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_pLastSlice->DecrementReference();
        m_pLastSlice = 0;
    }

    m_Headers.Reset(true);
    Skipping::Reset();
    m_ObjHeap.Release();

    m_frameOrder               = 0;

    m_WaitForIDR        = true;

    m_pLastDisplayed = 0;

    if (m_pTaskBroker)
        m_pTaskBroker->Init(m_iThreadNum);
}

void TaskSupplier::AfterErrorRestore()
{
    if (m_pTaskBroker)
        m_pTaskBroker->Reset();

    {
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();
    for (; iter != iter_end; ++iter)
    {
        for (H264DecoderFrame *pFrame = iter->GetDPBList(0)->head(); pFrame; pFrame = pFrame->future())
        {
            pFrame->FreeResources();
        }
    }
    }

    if (m_sei_messages)
        m_sei_messages->Reset();

    SVC_Extension::Reset();
    AU_Splitter::Reset();
    DPBOutput::Reset(m_iThreadNum != 1);
    DecReferencePictureMarking::Reset();
    m_accessUnit.Release();
    ErrorStatus::Reset();

    switch (m_initializationParams.info.profile) // after MVC_Extension::Init()
    {
    case 0:
        m_decodingMode = UNKNOWN_DECODING_MODE;
        break;
    case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:
        m_decodingMode = MVC_DECODING_MODE;
        break;
    default:
        m_decodingMode = AVC_DECODING_MODE;
        break;
    }

    if (m_pLastSlice)
    {
        m_pLastSlice->Release();
        m_pLastSlice->DecrementReference();
        m_pLastSlice = 0;
    }

    m_Headers.Reset(true);
    Skipping::Reset();
    m_ObjHeap.Release();

    m_WaitForIDR        = true;
    m_pLastDisplayed = 0;

    if (m_pTaskBroker)
        m_pTaskBroker->Init(m_iThreadNum);
}

Status TaskSupplier::GetInfo(VideoDecoderParams *lpInfo)
{
    H264SeqParamSet *sps = m_Headers.m_SeqParams.GetCurrentHeader();
    if (!sps)
    {
        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    const H264PicParamSet *pps = m_Headers.m_PicParams.GetCurrentHeader();

    lpInfo->info.clip_info.height = sps->frame_height_in_mbs * 16 -
        SubHeightC[sps->chroma_format_idc]*(2 - sps->frame_mbs_only_flag) *
        (sps->frame_cropping_rect_top_offset + sps->frame_cropping_rect_bottom_offset);

    lpInfo->info.clip_info.width = sps->frame_width_in_mbs * 16 - SubWidthC[sps->chroma_format_idc] *
        (sps->frame_cropping_rect_left_offset + sps->frame_cropping_rect_right_offset);

    if (0.0 < m_local_delta_frame_time)
    {
        lpInfo->info.framerate = 1.0 / m_local_delta_frame_time;
    }
    else
    {
        lpInfo->info.framerate = 0;
    }

    lpInfo->info.stream_type     = H264_VIDEO;

    lpInfo->profile = sps->profile_idc;
    lpInfo->level = sps->level_idc;

    lpInfo->numThreads = m_iThreadNum;
    lpInfo->info.color_format = GetUMCColorFormat(sps->chroma_format_idc);

    lpInfo->info.profile = sps->profile_idc;
    lpInfo->info.level = sps->level_idc;

    if (sps->vui.aspect_ratio_idc == 255)
    {
        lpInfo->info.aspect_ratio_width  = sps->vui.sar_width;
        lpInfo->info.aspect_ratio_height = sps->vui.sar_height;
    }

    uint32_t multiplier = 1 << (6 + sps->vui.bit_rate_scale);
    lpInfo->info.bitrate = sps->vui.bit_rate_value[0] * multiplier;

    if (sps->frame_mbs_only_flag)
        lpInfo->info.interlace_type = PROGRESSIVE;
    else
    {
        if (0 <= sps->offset_for_top_to_bottom_field)
            lpInfo->info.interlace_type = INTERLEAVED_TOP_FIELD_FIRST;
        else
            lpInfo->info.interlace_type = INTERLEAVED_BOTTOM_FIELD_FIRST;
    }

    H264VideoDecoderParams *lpH264Info = DynamicCast<H264VideoDecoderParams> (lpInfo);
    if (lpH264Info)
    {
        // calculate the new DPB size value
        lpH264Info->m_DPBSize = CalculateDPBSize(sps->level_idc,
                                   sps->frame_width_in_mbs * 16,
                                   sps->frame_height_in_mbs * 16,
                                   sps->num_ref_frames) + m_DPBSizeEx;

        mfxSize sz;
        sz.width    = sps->frame_width_in_mbs * 16;
        sz.height   = sps->frame_height_in_mbs * 16;
        lpH264Info->m_fullSize = sz;

        if (pps)
        {
           lpH264Info->m_entropy_coding_type = pps->entropy_coding_mode;
        }

        lpH264Info->m_cropArea.top = (int16_t)(SubHeightC[sps->chroma_format_idc] * sps->frame_cropping_rect_top_offset * (2 - sps->frame_mbs_only_flag));
        lpH264Info->m_cropArea.bottom = (int16_t)(SubHeightC[sps->chroma_format_idc] * sps->frame_cropping_rect_bottom_offset * (2 - sps->frame_mbs_only_flag));
        lpH264Info->m_cropArea.left = (int16_t)(SubWidthC[sps->chroma_format_idc] * sps->frame_cropping_rect_left_offset);
        lpH264Info->m_cropArea.right = (int16_t)(SubWidthC[sps->chroma_format_idc] * sps->frame_cropping_rect_right_offset);
    }

    return UMC_OK;
}

Status TaskSupplier::DecodeSEI(NalUnit *nalUnit)
{
    H264HeadersBitstream bitStream;

    try
    {
        H264MemoryPiece mem;
        mem.SetData(nalUnit);

        H264MemoryPiece swappedMem;
        swappedMem.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
        swapper->SwapMemory(&swappedMem, &mem, DEFAULT_NU_HEADER_TAIL_VALUE);

        bitStream.Reset((uint8_t*)swappedMem.GetPointer(), (uint32_t)swappedMem.GetDataSize());
        bitStream.SetTailBsSize(DEFAULT_NU_TAIL_SIZE);

        NAL_Unit_Type nal_unit_type;
        uint32_t nal_ref_idc;

        bitStream.GetNALUnitType(nal_unit_type, nal_ref_idc);

        do
        {
            H264SEIPayLoad    m_SEIPayLoads;

            bitStream.ParseSEI(m_Headers, &m_SEIPayLoads);

            DEBUG_PRINT((VM_STRING("debug headers SEI - %d\n"), m_SEIPayLoads.payLoadType));

            if (m_SEIPayLoads.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
            {
                m_UserData = m_SEIPayLoads;
            }
            else
            {
                m_Headers.m_SEIParams.AddHeader(&m_SEIPayLoads);
            }

        } while (bitStream.More_RBSP_Data());

    } catch(...)
    {
        // nothing to do just catch it
    }

    return UMC_OK;
}

Status TaskSupplier::DecodeHeaders(NalUnit *nalUnit)
{
    ViewItem *view = GetViewCount() ? &GetViewByNumber(BASE_VIEW) : 0;
    Status umcRes = UMC_OK;

    H264HeadersBitstream bitStream;

    try
    {
        H264MemoryPiece mem;
        mem.SetData(nalUnit);

        H264MemoryPiece swappedMem;

        swappedMem.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);

        SwapperBase * swapper = m_pNALSplitter->GetSwapper();
        swapper->SwapMemory(&swappedMem, &mem);

        bitStream.Reset((uint8_t*)swappedMem.GetPointer(), (uint32_t)swappedMem.GetDataSize());
        bitStream.SetTailBsSize(DEFAULT_NU_TAIL_SIZE);

        NAL_Unit_Type nal_unit_type;
        uint32_t nal_ref_idc;

        bitStream.GetNALUnitType(nal_unit_type, nal_ref_idc);

        switch(nal_unit_type)
        {
        // sequence parameter set
        case NAL_UT_SPS:
            {
                H264SeqParamSet sps;
                sps.seq_parameter_set_id = MAX_NUM_SEQ_PARAM_SETS;
                umcRes = bitStream.GetSequenceParamSet(&sps, m_ignoreLevelConstrain);
                if (umcRes != UMC_OK)
                {
                    H264SeqParamSet * old_sps = m_Headers.m_SeqParams.GetHeader(sps.seq_parameter_set_id);
                    if (old_sps)
                        old_sps->errorFlags = 1;
                    return UMC_ERR_INVALID_STREAM;
                }

                uint8_t newDPBsize = (uint8_t)CalculateDPBSize(sps.level_idc,
                                            sps.frame_width_in_mbs * 16,
                                            sps.frame_height_in_mbs * 16,
                                            sps.num_ref_frames);

                bool isNeedClean = sps.vui.max_dec_frame_buffering == 0;
                sps.vui.max_dec_frame_buffering = sps.vui.max_dec_frame_buffering ? sps.vui.max_dec_frame_buffering : newDPBsize;
                if (sps.vui.max_dec_frame_buffering > newDPBsize)
                    sps.vui.max_dec_frame_buffering = newDPBsize;

                if (sps.num_ref_frames > newDPBsize)
                    sps.num_ref_frames = newDPBsize;

                const H264SeqParamSet * old_sps = m_Headers.m_SeqParams.GetCurrentHeader();
                bool newResolution = false;
                if (IsNeedSPSInvalidate(old_sps, &sps))
                {
                    newResolution = true;
                }

                if (isNeedClean)
                    sps.vui.max_dec_frame_buffering = 0;

                DEBUG_PRINT((VM_STRING("debug headers SPS - %d, num_ref_frames - %d \n"), sps.seq_parameter_set_id, sps.num_ref_frames));

                H264SeqParamSet * temp = m_Headers.m_SeqParams.GetHeader(sps.seq_parameter_set_id);
                m_Headers.m_SeqParams.AddHeader(&sps);

                // Validate the incoming bitstream's image dimensions.
                temp = m_Headers.m_SeqParams.GetHeader(sps.seq_parameter_set_id);
                if (!temp)
                {
                    return UMC_ERR_NULL_PTR;
                }
                if (view)
                {
                    uint8_t newDPBsizeL = (uint8_t)CalculateDPBSize(m_level_idc ? m_level_idc : temp->level_idc,
                                                                sps.frame_width_in_mbs * 16,
                                                                sps.frame_height_in_mbs * 16,
                                                                sps.num_ref_frames);

                    temp->vui.max_dec_frame_buffering = temp->vui.max_dec_frame_buffering ? temp->vui.max_dec_frame_buffering : newDPBsizeL;
                    if (temp->vui.max_dec_frame_buffering > newDPBsizeL)
                        temp->vui.max_dec_frame_buffering = newDPBsizeL;
                }

                m_pNALSplitter->SetSuggestedSize(CalculateSuggestedSize(&sps));

                if (!temp->vui.timing_info_present_flag || m_use_external_framerate)
                {
                    temp->vui.num_units_in_tick = 90000;
                    temp->vui.time_scale = (uint32_t)(2*90000 / m_local_delta_frame_time);
                }

                m_local_delta_frame_time = 1 / ((0.5 * temp->vui.time_scale) / temp->vui.num_units_in_tick);

                ErrorStatus::isSPSError = 0;

                if (newResolution)
                    return UMC_NTF_NEW_RESOLUTION;
            }
            break;

        case NAL_UT_SPS_EX:
            {
                H264SeqParamSetExtension sps_ex;
                umcRes = bitStream.GetSequenceParamSetExtension(&sps_ex);

                if (umcRes != UMC_OK)
                    return UMC_ERR_INVALID_STREAM;

                m_Headers.m_SeqExParams.AddHeader(&sps_ex);
            }
            break;

            // picture parameter set
        case NAL_UT_PPS:
            {
                H264PicParamSet pps;
                // set illegal id
                pps.pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;

                // Get id
                umcRes = bitStream.GetPictureParamSetPart1(&pps);
                if (UMC_OK != umcRes)
                {
                    H264PicParamSet * old_pps = m_Headers.m_PicParams.GetHeader(pps.pic_parameter_set_id);
                    if (old_pps)
                        old_pps->errorFlags = 1;
                    return UMC_ERR_INVALID_STREAM;
                }

                H264SeqParamSet *refSps = m_Headers.m_SeqParams.GetHeader(pps.seq_parameter_set_id);
                uint32_t prevActivePPS = m_Headers.m_PicParams.GetCurrentID();

                if (!refSps || refSps->seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                {
                    refSps = m_Headers.m_SeqParamsMvcExt.GetHeader(pps.seq_parameter_set_id);
                    if (!refSps || refSps->seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                    {
                        refSps = m_Headers.m_SeqParamsSvcExt.GetHeader(pps.seq_parameter_set_id);
                        if (!refSps || refSps->seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                            return UMC_ERR_INVALID_STREAM;
                    }
                }

                // Get rest of pic param set
                umcRes = bitStream.GetPictureParamSetPart2(&pps, refSps);
                if (UMC_OK != umcRes)
                {
                    H264PicParamSet * old_pps = m_Headers.m_PicParams.GetHeader(pps.pic_parameter_set_id);
                    if (old_pps)
                        old_pps->errorFlags = 1;
                    return UMC_ERR_INVALID_STREAM;
                }

                DEBUG_PRINT((VM_STRING("debug headers PPS - %d - SPS - %d\n"), pps.pic_parameter_set_id, pps.seq_parameter_set_id));
                m_Headers.m_PicParams.AddHeader(&pps);

                //m_Headers.m_SeqParams.SetCurrentID(pps.seq_parameter_set_id);
                // in case of MVC restore previous active PPS
                if ((H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH == refSps->profile_idc) ||
                    (H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH == refSps->profile_idc))
                {
                    m_Headers.m_PicParams.SetCurrentID(prevActivePPS);
                }

                ErrorStatus::isPPSError = 0;
            }
            break;

        // subset sequence parameter set
        case NAL_UT_SUBSET_SPS:
            {
                if (!IsExtension())
                    break;

                H264SeqParamSet sps;
                umcRes = bitStream.GetSequenceParamSet(&sps);
                if (UMC_OK != umcRes)
                {
                    return UMC_ERR_INVALID_STREAM;
                }

                m_pNALSplitter->SetSuggestedSize(CalculateSuggestedSize(&sps));

                DEBUG_PRINT((VM_STRING("debug headers SUBSET SPS - %d, profile_idc - %d, level_idc - %d, num_ref_frames - %d \n"), sps.seq_parameter_set_id, sps.profile_idc, sps.level_idc, sps.num_ref_frames));

                if ((sps.profile_idc == H264VideoDecoderParams::H264_PROFILE_SCALABLE_BASELINE) ||
                    (sps.profile_idc == H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH))
                {
                    H264SeqParamSetSVCExtension spsSvcExt;
                    H264SeqParamSet * sps_temp = &spsSvcExt;

                    *sps_temp = sps;

                    umcRes = bitStream.GetSequenceParamSetSvcExt(&spsSvcExt);
                    if (UMC_OK != umcRes)
                    {
                        return UMC_ERR_INVALID_STREAM;
                    }

                    const H264SeqParamSetSVCExtension * old_sps = m_Headers.m_SeqParamsSvcExt.GetCurrentHeader();
                    bool newResolution = false;
                    if (old_sps && old_sps->profile_idc != spsSvcExt.profile_idc)
                    {
                        newResolution = true;
                    }

                    m_Headers.m_SeqParamsSvcExt.AddHeader(&spsSvcExt);

                    SVC_Extension::ChooseLevelIdc(&spsSvcExt, sps.level_idc, sps.level_idc);

                    if (view)
                    {
                        view->SetDPBSize(&sps, m_level_idc);
                    }

                    if (newResolution)
                        return UMC_NTF_NEW_RESOLUTION;
                }

                // decode additional parameters
                if ((H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH == sps.profile_idc) ||
                    (H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH == sps.profile_idc))
                {
                    H264SeqParamSetMVCExtension spsMvcExt;
                    H264SeqParamSet * sps_temp = &spsMvcExt;

                    *sps_temp = sps;

                    // skip one bit
                    bitStream.Get1Bit();
                    // decode  MVC extension
                    umcRes = bitStream.GetSequenceParamSetMvcExt(&spsMvcExt);
                    if (UMC_OK != umcRes)
                    {
                        return UMC_ERR_INVALID_STREAM;
                    }

                    const H264SeqParamSetMVCExtension * old_sps = m_Headers.m_SeqParamsMvcExt.GetCurrentHeader();
                    bool newResolution = false;
                    if (old_sps && old_sps->profile_idc != spsMvcExt.profile_idc)
                    {
                        newResolution = true;
                    }

                    DEBUG_PRINT((VM_STRING("debug headers SUBSET SPS MVC ext - %d \n"), sps.seq_parameter_set_id));
                    m_Headers.m_SeqParamsMvcExt.AddHeader(&spsMvcExt);

                    MVC_Extension::ChooseLevelIdc(&spsMvcExt, sps.level_idc, sps.level_idc);

                    if (view)
                    {
                        view->SetDPBSize(&sps, m_level_idc);
                    }

                    if (newResolution)
                        return UMC_NTF_NEW_RESOLUTION;
                }
            }
            break;

        // decode a prefix nal unit
        case NAL_UT_PREFIX:
            {
                umcRes = bitStream.GetNalUnitPrefix(&m_Headers.m_nalExtension, nal_ref_idc);
                if (UMC_OK != umcRes)
                {
                    m_Headers.m_nalExtension.extension_present = 0;
                    return UMC_ERR_INVALID_STREAM;
                }
            }
            break;

        default:
            break;
        }
    }
    catch(const h264_exception & ex)
    {
        return ex.GetStatus();
    }
    catch(...)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    return UMC_OK;

} // Status TaskSupplier::DecodeHeaders(MediaDataEx::_MediaDataEx *pSource, H264MemoryPiece * pMem)

//////////////////////////////////////////////////////////////////////////////
// ProcessFrameNumGap
//
// A non-sequential frame_num has been detected. If the sequence parameter
// set field gaps_in_frame_num_value_allowed_flag is non-zero then the gap
// is OK and "non-existing" frames will be created to correctly fill the
// gap. Otherwise the gap is an indication of lost frames and the need to
// handle in a reasonable way.
//////////////////////////////////////////////////////////////////////////////
Status TaskSupplier::ProcessFrameNumGap(H264Slice *pSlice, int32_t field, int32_t dId, int32_t maxDId)
{
    ViewItem &view = GetView(pSlice->GetSliceHeader()->nal_ext.mvc.view_id);
    Status umcRes = UMC_OK;
    const H264SeqParamSet* sps = pSlice->GetSeqParam();
    H264SliceHeader *sliceHeader = pSlice->GetSliceHeader();

    uint32_t frameNumGap = view.GetPOCDecoder(0)->DetectFrameNumGap(pSlice);

    if (!frameNumGap)
        return UMC_OK;

    if (dId == maxDId)
    {
        if (frameNumGap > view.maxDecFrameBuffering)
        {
            frameNumGap = view.maxDecFrameBuffering;
        }
    }
    else
    {
        if (frameNumGap > sps->num_ref_frames)
        {
            frameNumGap = sps->num_ref_frames;
        }
    }

    int32_t uMaxFrameNum = (1<<sps->log2_max_frame_num);

    DEBUG_PRINT((VM_STRING("frame gap - %d\n"), frameNumGap));

    // Fill the frame_num gap with non-existing frames. For each missing
    // frame:
    //  - allocate a frame
    //  - set frame num and pic num
    //  - update FrameNumWrap for all reference frames
    //  - use sliding window frame marking to free oldest reference
    //  - mark the frame as short-term reference
    // The picture part of the generated frames is unimportant -- it will
    // not be used for reference.

    // set to first missing frame. Note that if frame number wrapped during
    // the gap, the first missing frame_num could be larger than the
    // current frame_num. If that happened, FrameNumGap will be negative.
    //VM_ASSERT((int32_t)sliceHeader->frame_num > frameNumGap);
    int32_t frame_num = sliceHeader->frame_num - frameNumGap;

    while ((frame_num != sliceHeader->frame_num) && (umcRes == UMC_OK))
    {
        H264DecoderFrame *pFrame;

        {
            // allocate a frame
            // Traverse list for next disposable frame
            pFrame = GetFreeFrame(pSlice);

            // Did we find one?
            if (!pFrame)
            {
                return UMC_ERR_NOT_ENOUGH_BUFFER;
            }

            pFrame->IncrementReference();
            m_UIDFrameCounter++;
            pFrame->m_UID = m_UIDFrameCounter;
        }

        frameNumGap--;

        if (sps->pic_order_cnt_type != 0)
        {
            int32_t tmp1 = sliceHeader->delta_pic_order_cnt[0];
            int32_t tmp2 = sliceHeader->delta_pic_order_cnt[1];
            sliceHeader->delta_pic_order_cnt[0] = sliceHeader->delta_pic_order_cnt[1] = 0;

            view.GetPOCDecoder(0)->DecodePictureOrderCount(pSlice, frame_num);

            sliceHeader->delta_pic_order_cnt[0] = tmp1;
            sliceHeader->delta_pic_order_cnt[1] = tmp2;
        }

        // Set frame num and pic num for the missing frame
        pFrame->setFrameNum(frame_num);
        view.GetPOCDecoder(0)->DecodePictureOrderCountFrameGap(pFrame, sliceHeader, frame_num);
        DEBUG_PRINT((VM_STRING("frame gap - frame_num = %d, poc = %d,%d added\n"), frame_num, pFrame->m_PicOrderCnt[0], pFrame->m_PicOrderCnt[1]));

        if (sliceHeader->field_pic_flag == 0)
        {
            pFrame->setPicNum(frame_num, 0);
        }
        else
        {
            pFrame->setPicNum(frame_num*2+1, 0);
            pFrame->setPicNum(frame_num*2+1, 1);
        }

        // Update frameNumWrap and picNum for all decoded frames

        H264DecoderFrame *pFrm;
        H264DecoderFrame * pHead = view.GetDPBList(0)->head();
        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
            // TBD: modify for fields
            pFrm->UpdateFrameNumWrap(frame_num,
                uMaxFrameNum,
                pFrame->m_PictureStructureForRef+
                pFrame->m_bottom_field_flag[field]);
        }

        // sliding window ref pic marking
        // view2 below inicialized exacly as view above.
        // why we need view2 & may it be defferent from view?
        ViewItem &view2 = GetView(pSlice->GetSliceHeader()->nal_ext.mvc.view_id);
        H264DecoderFrame * saveFrame = pFrame;
        std::swap(pSlice->m_pCurrentFrame, saveFrame);
        DecReferencePictureMarking::SlideWindow(view2, pSlice, 0);
        std::swap(pSlice->m_pCurrentFrame, saveFrame);

        pFrame->SetisShortTermRef(true, 0);
        pFrame->SetisShortTermRef(true, 1);

        // next missing frame
        frame_num++;
        if (frame_num >= uMaxFrameNum)
            frame_num = 0;

        pFrame->SetFrameAsNonExist();
        pFrame->DecrementReference();
    }   // while

    return UMC_OK;
}   // ProcessFrameNumGap

void TaskSupplier::PostProcessDisplayFrame(H264DecoderFrame *pFrame)
{
    if (!pFrame || pFrame->post_procces_complete)
        return;

    ViewItem &view = GetView(pFrame->m_viewId);

    pFrame->post_procces_complete = true;

    DEBUG_PRINT((VM_STRING("Outputted POC - %d, busyState - %d, uid - %d, viewId - %d, pppp - %d\n"), pFrame->m_PicOrderCnt[0], pFrame->GetRefCounter(), pFrame->m_UID, pFrame->m_viewId, pppp++));

    if (!pFrame->IsFrameExist())
        return;

    pFrame->m_isOriginalPTS = pFrame->m_dFrameTime > -1.0;
    if (pFrame->m_isOriginalPTS)
    {
        view.localFrameTime = pFrame->m_dFrameTime;
    }
    else
    {
        pFrame->m_dFrameTime = view.localFrameTime;
    }

    pFrame->m_frameOrder = m_frameOrder;
    switch (pFrame->m_displayPictureStruct)
    {
    case UMC::DPS_TOP_BOTTOM_TOP:
    case UMC::DPS_BOTTOM_TOP_BOTTOM:
        if (m_initializationParams.lFlags & UMC::FLAG_VDEC_TELECINE_PTS)
        {
            view.localFrameTime += (m_local_delta_frame_time / 2);
        }
        break;

    case UMC::DPS_FRAME_DOUBLING:
    case UMC::DPS_FRAME_TRIPLING:
    case UMC::DPS_TOP:
    case UMC::DPS_BOTTOM:
    case UMC::DPS_TOP_BOTTOM:
    case UMC::DPS_BOTTOM_TOP:
    case UMC::DPS_FRAME:
    default:
        break;
    }

    view.localFrameTime += m_local_delta_frame_time;

    bool wasWrapped = IncreaseCurrentView();
    if (wasWrapped)
        m_frameOrder++;

    if (view.GetDPBList(0)->GetRecoveryFrameCnt() != -1)
        view.GetDPBList(0)->SetRecoveryFrameCnt(-1);
}

H264DecoderFrame *TaskSupplier::GetFrameToDisplayInternal(bool force)
{
    H264DecoderFrame *pFrame = GetAnyFrameToDisplay(force);
    return pFrame;
}

H264DecoderFrame *TaskSupplier::GetAnyFrameToDisplay(bool force)
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    for (uint32_t i = 0; iter != iter_end; ++i, ++iter)
    {
        ViewItem &view = *iter;

        if (i != m_currentDisplayView || !view.m_isDisplayable)
        {
            if (i == m_currentDisplayView)
            {
                IncreaseCurrentView();
            }
            continue;
        }

        for (;;)
        {
            // show oldest frame

            // Flag SPS.VUI.max_num_reorder_frames may be presented in bitstream headers
            // (if SPS.VUI.bitstream_restriction_flag == 1) and may be used to reduce
            // latency on outputting decoded frames.
            //
            // Due to h264 is a legacy codec and flag SPS.VUI.max_num_reorder_frames
            // is not necessary - there are existed some old encoders and coded videos
            // where flag max_num_reorder_frames is set to 0 by default, but reordering
            // actually exists. For this reason Media SDK ignores this flag by default,
            // because there is a risk to break decoding some old videos.
            //
            // Enabling define ENABLE_MAX_NUM_REORDER_FRAMES_OUTPUT allows to reduce latency
            // (ex: max_num_reorder_frames == 0 -> output frame immediately)

            uint32_t countNumDisplayable = view.GetDPBList(0)->countNumDisplayable();
            if (   countNumDisplayable > view.maxDecFrameBuffering
#ifdef ENABLE_MAX_NUM_REORDER_FRAMES_OUTPUT
                || countNumDisplayable > view.maxNumReorderFrames
#endif
                || force)
            {
                H264DecoderFrame *pTmp = view.GetDPBList(0)->findOldestDisplayable(view.maxDecFrameBuffering);

                if (pTmp)
                {
                    int32_t recovery_frame_cnt = view.GetDPBList(0)->GetRecoveryFrameCnt(); // DEBUG !!! need to think about recovery and MVC

                    if (!pTmp->IsFrameExist() || (recovery_frame_cnt != -1 && pTmp->m_FrameNum != recovery_frame_cnt))
                    {
                        pTmp->SetErrorFlagged(ERROR_FRAME_RECOVERY);
                    }

                    return pTmp;
                }

                break;
            }
            else
            {
                if (DPBOutput::IsUseDelayOutputValue())
                {
                    H264DecoderFrame *pTmp = view.GetDPBList(0)->findDisplayableByDPBDelay();
                    if (pTmp)
                        return pTmp;
                }
                break;
            }
        }
    }

    return 0;
}

void TaskSupplier::SetMBMap(const H264Slice * , H264DecoderFrame *, LocalResources * )
{
}

void TaskSupplier::PreventDPBFullness()
{
    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    for (; iter != iter_end; ++iter)
    {
        ViewItem &view = *iter;
        H264DBPList *pDPB = view.GetDPBList(0);

    try
    {
        for (;;)
        {
            // force Display or ... delete long term
            const H264SeqParamSet* sps = m_Headers.m_SeqParams.GetCurrentHeader();
            if (sps)
            {
                uint32_t NumShortTermRefs, NumLongTermRefs;

                // find out how many active reference frames currently in decoded
                // frames buffer
                pDPB->countActiveRefs(NumShortTermRefs, NumLongTermRefs);

                if (NumLongTermRefs == sps->num_ref_frames)
                {
                    H264DecoderFrame *pFrame = pDPB->findOldestLongTermRef();
                    if (pFrame)
                    {
                        pFrame->SetisLongTermRef(false, 0);
                        pFrame->SetisLongTermRef(false, 1);
                        pFrame->Reset();
                    }
                }

                if (pDPB->IsDisposableExist())
                    break;

                /*while (NumShortTermRefs > 0 &&
                    (NumShortTermRefs + NumLongTermRefs >= (int32_t)sps->num_ref_frames))
                {
                    H264DecoderFrame * pFrame = view.pDPB->findOldestShortTermRef();

                    if (!pFrame)
                        break;

                    pFrame->SetisShortTermRef(false, 0);
                    pFrame->SetisShortTermRef(false, 1);

                    NumShortTermRefs--;
                };*/

                if (pDPB->IsDisposableExist())
                    break;
            }

            break;
        }
    } catch(...)
    {
    }

    if (!pDPB->IsDisposableExist())
        AfterErrorRestore();

    }
}

Status TaskSupplier::CompleteDecodedFrames(H264DecoderFrame ** decoded)
{
    H264DecoderFrame* completed = 0;
    Status sts = UMC_OK;

    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();
    for (; iter != iter_end; ++iter)
    {
        ViewItem &view = *iter;
        H264DBPList *pDPB = view.GetDPBList(0);

        for (;;) //add all ready to decoding
        {
            bool isOneToAdd = true;
            H264DecoderFrame * frameToAdd = 0;

            for (H264DecoderFrame * frame = pDPB->head(); frame; frame = frame->future())
            {
                int32_t const frm_error = frame->GetError();

                //we don't overwrite an error if we already got it
                if (sts == UMC_OK && frm_error < 0)
                    //if we have ERROR_FRAME_DEVICE_FAILURE  bit is set then this error is  UMC::Status code
                    sts = static_cast<Status>(frm_error);

                if (frame->IsFrameExist() && !frame->IsDecoded())
                {
                    if (!frame->IsDecodingStarted() && frame->IsFullFrame())
                    {
                        if (frameToAdd)
                        {
                            isOneToAdd = false;
                            if (frameToAdd->m_UID < frame->m_UID) // add first with min UID
                                continue;
                        }

                        frameToAdd = frame;
                    }

                    if (!frame->IsDecodingCompleted())
                    {
                        continue;
                    }

                    frame->OnDecodingCompleted();
                    completed = frame;
                }
            }

            if (sts != UMC_OK)
                break;

            if (frameToAdd)
            {
                if (!m_pTaskBroker->AddFrameToDecoding(frameToAdd))
                    break;
            }

            if (isOneToAdd)
                break;
        }
    }

    if (decoded)
        *decoded = completed;

    return sts;
}

Status TaskSupplier::AddSource(MediaData * pSource)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "TaskSupplier::AddSource");

    H264DecoderFrame* completed = 0;
    Status umcRes = CompleteDecodedFrames(&completed);
    if (umcRes != UMC_OK)
        return pSource || !completed ? umcRes : UMC_OK;

    umcRes = AddOneFrame(pSource); // construct frame

    if (UMC_ERR_NOT_ENOUGH_BUFFER == umcRes)
    {
        ViewItem &view = GetView(m_currentView);
        H264DBPList *pDPB = view.GetDPBList(0);

        // in the following case(s) we can lay on the base view only,
        // because the buffers are managed synchronously.

        // frame is being processed. Wait for asynchronous end of operation.
        if (pDPB->IsDisposableExist())
        {
            return UMC_WRN_INFO_NOT_READY;
        }

        // frame is finished, but still referenced.
        // Wait for asynchronous complete.
        if (pDPB->IsAlmostDisposableExist())
        {
            return UMC_WRN_INFO_NOT_READY;
        }

        // some more hard reasons of frame lacking.
        if (!m_pTaskBroker->IsEnoughForStartDecoding(true))
        {
            umcRes = CompleteDecodedFrames(&completed);
            if (umcRes != UMC_OK)
                return umcRes;
            else if (completed)
                return UMC_WRN_INFO_NOT_READY;

            if (GetFrameToDisplayInternal(true))
                return UMC_ERR_NEED_FORCE_OUTPUT;

            PreventDPBFullness();
            return UMC_WRN_INFO_NOT_READY;
        }
    }

    return umcRes;
}

#if (MFX_VERSION >= 1025)
Status TaskSupplier::ProcessNalUnit(NalUnit *nalUnit, mfxExtDecodeErrorReport * pDecodeErrorReport)
#else
Status TaskSupplier::ProcessNalUnit(NalUnit *nalUnit)
#endif
{
    Status umcRes = UMC_OK;

    switch (nalUnit->GetNalUnitType())
    {
    case NAL_UT_IDR_SLICE:
    case NAL_UT_SLICE:
    case NAL_UT_AUXILIARY:
    case NAL_UT_CODED_SLICE_EXTENSION:
        {
            H264Slice * pSlice = DecodeSliceHeader(nalUnit);
            if (pSlice)
            {
                umcRes = AddSlice(pSlice, false);
            }
        }
        break;

    case NAL_UT_SPS:
    case NAL_UT_PPS:
    case NAL_UT_SPS_EX:
    case NAL_UT_SUBSET_SPS:
    case NAL_UT_PREFIX:
        umcRes = DecodeHeaders(nalUnit);

#if (MFX_VERSION >= 1025)
        if (pDecodeErrorReport && umcRes == UMC_ERR_INVALID_STREAM)
           SetDecodeErrorTypes(nalUnit->GetNalUnitType(), pDecodeErrorReport);
#endif

        break;

    case NAL_UT_SEI:
        umcRes = DecodeSEI(nalUnit);
        break;
    case NAL_UT_AUD:
        m_accessUnit.CompleteLastLayer();
        break;

    case NAL_UT_DPA: //ignore it
    case NAL_UT_DPB:
    case NAL_UT_DPC:
    case NAL_UT_FD:
    case NAL_UT_UNSPECIFIED:
        break;

    case NAL_UT_END_OF_STREAM:
    case NAL_UT_END_OF_SEQ:
        {
            m_accessUnit.CompleteLastLayer();
            m_WaitForIDR = true;
        }
        break;

    default:
        break;
    };

    return umcRes;
}

Status TaskSupplier::AddOneFrame(MediaData * pSource)
{
    Status umsRes = UMC_OK;

    if (m_pLastSlice)
    {
        Status sts = AddSlice(m_pLastSlice, !pSource);
        if (sts == UMC_ERR_NOT_ENOUGH_BUFFER || sts == UMC_ERR_ALLOC)
        {
            return sts;
        }

        if (sts == UMC_OK)
            return sts;
    }

    do
    {
#if (MFX_VERSION >= 1025)
        MediaData::AuxInfo* aux = (pSource) ? pSource->GetAuxInfo(MFX_EXTBUFF_DECODE_ERROR_REPORT) : NULL;
        mfxExtDecodeErrorReport* pDecodeErrorReport = (aux) ? reinterpret_cast<mfxExtDecodeErrorReport*>(aux->ptr) : NULL;
#endif

        NalUnit *nalUnit = m_pNALSplitter->GetNalUnits(pSource);

        if (!nalUnit && pSource)
        {
            uint32_t flags = pSource->GetFlags();

            if (!(flags & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME))
            {
                assert(!m_pLastSlice);
                return AddSlice(0, true);
            }

            return UMC_ERR_SYNC;
        }

        if (!nalUnit)
        {
            if (!pSource)
                return AddSlice(0, true);

            return UMC_ERR_NOT_ENOUGH_DATA;
        }

        if ((NAL_UT_IDR_SLICE != nalUnit->GetNalUnitType()) &&
            (NAL_UT_SLICE != nalUnit->GetNalUnitType()))
        {
            // Reset last prefix NAL unit
            m_Headers.m_nalExtension.extension_present = 0;
        }

        switch ((NAL_Unit_Type)nalUnit->GetNalUnitType())
        {
        case NAL_UT_IDR_SLICE:
        case NAL_UT_SLICE:
        case NAL_UT_AUXILIARY:
        case NAL_UT_CODED_SLICE_EXTENSION:
            {
            H264Slice * pSlice = DecodeSliceHeader(nalUnit);
            if (pSlice)
            {
                umsRes = AddSlice(pSlice, !pSource);
                if (umsRes == UMC_ERR_NOT_ENOUGH_BUFFER || umsRes == UMC_OK || umsRes == UMC_ERR_ALLOC)
                {

                    return umsRes;
                }
            }
            }
            break;

        case NAL_UT_SPS:
        case NAL_UT_PPS:
        case NAL_UT_SPS_EX:
        case NAL_UT_SUBSET_SPS:
        case NAL_UT_PREFIX:
            umsRes = DecodeHeaders(nalUnit);
            if (umsRes != UMC_OK)
            {
                if (umsRes == UMC_NTF_NEW_RESOLUTION && pSource)
                {
                    int32_t size = (int32_t)nalUnit->GetDataSize();
                    pSource->MoveDataPointer(- size - 3);
                }

#if (MFX_VERSION >= 1025)
                if (pDecodeErrorReport && umsRes == UMC_ERR_INVALID_STREAM)
                    SetDecodeErrorTypes(nalUnit->GetNalUnitType(), pDecodeErrorReport);
#endif

                return umsRes;
            }

            if (nalUnit->GetNalUnitType() == NAL_UT_SPS || nalUnit->GetNalUnitType() == NAL_UT_PPS)
            {
                m_accessUnit.CompleteLastLayer();
            }
            break;

        case NAL_UT_SEI:
            m_accessUnit.CompleteLastLayer();
            DecodeSEI(nalUnit);
            break;
        case NAL_UT_AUD:  //ignore it
            m_accessUnit.CompleteLastLayer();
            break;

        case NAL_UT_END_OF_STREAM:
        case NAL_UT_END_OF_SEQ:
            {
                m_accessUnit.CompleteLastLayer();
                m_WaitForIDR = true;
            }
            break;

        case NAL_UT_DPA: //ignore it
        case NAL_UT_DPB:
        case NAL_UT_DPC:
        case NAL_UT_FD:
        default:
            break;
        };

    } while ((pSource) && (MINIMAL_DATA_SIZE < pSource->GetDataSize()));

    if (!pSource)
    {
        return AddSlice(0, true);
    }
    else
    {
        uint32_t flags = pSource->GetFlags();

        if (!(flags & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME))
        {
            assert(!m_pLastSlice);
            return AddSlice(0, true);
        }
    }

    return UMC_ERR_NOT_ENOUGH_DATA;
}

static
bool IsFieldOfOneFrame(const H264DecoderFrame *pFrame, const H264Slice * pSlice1, const H264Slice *pSlice2)
{
    if (!pFrame)
        return false;

    if (pFrame && pFrame->GetAU(0)->GetStatus() > H264DecoderFrameInfo::STATUS_NOT_FILLED
        && pFrame->GetAU(1)->GetStatus() > H264DecoderFrameInfo::STATUS_NOT_FILLED)
        return false;

    if ((pSlice1->GetSliceHeader()->nal_ref_idc && !pSlice2->GetSliceHeader()->nal_ref_idc) ||
        (!pSlice1->GetSliceHeader()->nal_ref_idc && pSlice2->GetSliceHeader()->nal_ref_idc))
        return false;

    if (pSlice1->GetSliceHeader()->field_pic_flag != pSlice2->GetSliceHeader()->field_pic_flag)
        return false;

    if (pSlice1->GetSliceHeader()->frame_num != pSlice2->GetSliceHeader()->frame_num)
        return false;

    if (pSlice1->GetSliceHeader()->bottom_field_flag == pSlice2->GetSliceHeader()->bottom_field_flag)
        return false;

    return true;
}

H264Slice * TaskSupplier::CreateSlice()
{
    return m_ObjHeap.AllocateObject<H264Slice>();
}

H264Slice * TaskSupplier::DecodeSliceHeader(NalUnit *nalUnit)
{
    if ((0 > m_Headers.m_SeqParams.GetCurrentID()) ||
        (0 > m_Headers.m_PicParams.GetCurrentID()))
    {
        return 0;
    }

    if (m_Headers.m_nalExtension.extension_present)
    {
        if (SVC_Extension::IsShouldSkipSlice(&m_Headers.m_nalExtension))
        {
            m_Headers.m_nalExtension.extension_present = 0;
            return 0;
        }
    }

    H264Slice * pSlice = CreateSlice();
    if (!pSlice)
    {
        return 0;
    }
    pSlice->SetHeap(&m_ObjHeap);
    pSlice->IncrementReference();

    notifier0<H264Slice> memory_leak_preventing_slice(pSlice, &H264Slice::DecrementReference);

    H264MemoryPiece memCopy;
    memCopy.SetData(nalUnit);

    pSlice->m_pSource.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_SLICE_TAIL_SIZE);

    notifier0<H264MemoryPiece> memory_leak_preventing(&pSlice->m_pSource, &H264MemoryPiece::Release);

    SwapperBase * swapper = m_pNALSplitter->GetSwapper();
    swapper->SwapMemory(&pSlice->m_pSource, &memCopy);

    int32_t pps_pid = pSlice->RetrievePicParamSetNumber();
    if (pps_pid == -1)
    {
        ErrorStatus::isPPSError = 1;
        return 0;
    }

    H264SEIPayLoad * spl = m_Headers.m_SEIParams.GetHeader(SEI_RECOVERY_POINT_TYPE);

    if (m_WaitForIDR)
    {
        if (pSlice->GetSliceHeader()->slice_type != INTRASLICE && !spl)
        {
            return 0;
        }
    }

    pSlice->m_pPicParamSet = m_Headers.m_PicParams.GetHeader(pps_pid);
    if (!pSlice->m_pPicParamSet)
    {
        return 0;
    }

    int32_t seq_parameter_set_id = pSlice->m_pPicParamSet->seq_parameter_set_id;

    if (NAL_UT_CODED_SLICE_EXTENSION == pSlice->GetSliceHeader()->nal_unit_type)
    {
        if (pSlice->GetSliceHeader()->nal_ext.svc_extension_flag == 0)
        {
            pSlice->m_pSeqParamSetSvcEx = 0;
            pSlice->m_pSeqParamSet = pSlice->m_pSeqParamSetMvcEx = m_Headers.m_SeqParamsMvcExt.GetHeader(seq_parameter_set_id);

            if (NULL == pSlice->m_pSeqParamSet)
            {
                return 0;
            }

            m_Headers.m_SeqParamsMvcExt.SetCurrentID(pSlice->m_pSeqParamSetMvcEx->seq_parameter_set_id);
            m_Headers.m_PicParams.SetCurrentID(pSlice->m_pPicParamSet->pic_parameter_set_id);
        }
        else
        {
            pSlice->m_pSeqParamSetMvcEx = 0;
            pSlice->m_pSeqParamSet = pSlice->m_pSeqParamSetSvcEx = m_Headers.m_SeqParamsSvcExt.GetHeader(seq_parameter_set_id);

            if (NULL == pSlice->m_pSeqParamSet)
            {
                return 0;
            }

            m_Headers.m_SeqParamsSvcExt.SetCurrentID(pSlice->m_pSeqParamSetSvcEx->seq_parameter_set_id);
            m_Headers.m_PicParams.SetCurrentID(pSlice->m_pPicParamSet->pic_parameter_set_id);
        }
    }
    else
    {
        pSlice->m_pSeqParamSetSvcEx = m_Headers.m_SeqParamsSvcExt.GetCurrentHeader();
        pSlice->m_pSeqParamSetMvcEx = m_Headers.m_SeqParamsMvcExt.GetCurrentHeader();
        pSlice->m_pSeqParamSet = m_Headers.m_SeqParams.GetHeader(seq_parameter_set_id);

        if (!pSlice->m_pSeqParamSet)
        {
            ErrorStatus::isSPSError = 0;
            return 0;
        }

        m_Headers.m_SeqParams.SetCurrentID(pSlice->m_pPicParamSet->seq_parameter_set_id);
        m_Headers.m_PicParams.SetCurrentID(pSlice->m_pPicParamSet->pic_parameter_set_id);
    }

    if (pSlice->m_pSeqParamSet->errorFlags)
        ErrorStatus::isSPSError = 1;

    if (pSlice->m_pPicParamSet->errorFlags)
        ErrorStatus::isPPSError = 1;

    Status sts = InitializePictureParamSet(m_Headers.m_PicParams.GetHeader(pps_pid), pSlice->m_pSeqParamSet, NAL_UT_CODED_SLICE_EXTENSION == pSlice->GetSliceHeader()->nal_unit_type);
    if (sts != UMC_OK)
    {
        return 0;
    }

    pSlice->m_pSeqParamSetEx = m_Headers.m_SeqExParams.GetHeader(seq_parameter_set_id);
    pSlice->m_pCurrentFrame = 0;

    memory_leak_preventing.ClearNotification();
    pSlice->m_dTime = pSlice->m_pSource.GetTime();

    if (!pSlice->Reset(&m_Headers.m_nalExtension))
    {
        return 0;
    }

    if (IsShouldSkipSlice(pSlice))
        return 0;

    if (!IsExtension())
    {
        memset(&pSlice->GetSliceHeader()->nal_ext, 0, sizeof(H264NalExtension));
    }

    if (spl)
    {
        if (m_WaitForIDR)
        {
            AllocateAndInitializeView(pSlice);
            ViewItem &view = GetView(pSlice->GetSliceHeader()->nal_ext.mvc.view_id);
            uint32_t recoveryFrameNum = (spl->SEI_messages.recovery_point.recovery_frame_cnt + pSlice->GetSliceHeader()->frame_num) % (1 << pSlice->m_pSeqParamSet->log2_max_frame_num);
            view.GetDPBList(0)->SetRecoveryFrameCnt(recoveryFrameNum);
        }

        if ((pSlice->GetSliceHeader()->slice_type != INTRASLICE))
        {
            H264SEIPayLoad * splToRemove = m_Headers.m_SEIParams.GetHeader(SEI_RECOVERY_POINT_TYPE);
            m_Headers.m_SEIParams.RemoveHeader(splToRemove);
        }
    }

    m_WaitForIDR = false;
    memory_leak_preventing_slice.ClearNotification();

    //DEBUG_PRINT((VM_STRING("debug headers slice - view - %d \n"), pSlice->GetSliceHeader()->nal_ext.mvc.view_id));

    return pSlice;
}

void TaskSupplier::DPBSanitize(H264DecoderFrame * pDPBHead, const H264DecoderFrame * pFrame)
{
    for (H264DecoderFrame *pFrm = pDPBHead; pFrm; pFrm = pFrm->future())
    {
        if ((pFrm != pFrame) &&
            (pFrm->FrameNum() == pFrame->FrameNum()) &&
             pFrm->isShortTermRef())
        {
            pFrm->SetErrorFlagged(ERROR_FRAME_SHORT_TERM_STUCK);
            AddItemAndRun(pFrm, pFrm, UNSET_REFERENCE | FULL_FRAME | SHORT_TERM);
        }
    }
}

Status TaskSupplier::AddSlice(H264Slice * pSlice, bool force)
{
    if (!m_accessUnit.GetLayersCount() && pSlice)
    {
        if (m_sei_messages)
            m_sei_messages->SetAUID(m_accessUnit.GetAUIndentifier());
    }

    if (!pSlice)
    {
        m_accessUnit.AddSlice(0); // full AU
    }

    if ((!pSlice || m_accessUnit.IsFullAU() || !m_accessUnit.AddSlice(pSlice)) && m_accessUnit.GetLayersCount())
    {
        m_pLastSlice = pSlice;
        if (!m_accessUnit.m_isInitialized)
        {
            InitializeLayers(&m_accessUnit, 0, 0);

            size_t layersCount = m_accessUnit.GetLayersCount();
            uint32_t maxDId = 0;
            if (layersCount && m_accessUnit.GetLayer(layersCount - 1)->GetSliceCount())
            {
                maxDId = m_accessUnit.GetLayer(layersCount - 1)->GetSlice(0)->GetSliceHeader()->nal_ext.svc.dependency_id;
            }
            for (size_t i = 0; i < layersCount; i++)
            {
                SetOfSlices * setOfSlices = m_accessUnit.GetLayer(i);
                H264Slice * slice = setOfSlices->GetSlice(0);
                if (slice == nullptr)
                    continue;

                AllocateAndInitializeView(slice);

                ViewItem &view = GetView(slice->GetSliceHeader()->nal_ext.mvc.view_id);

                if (view.pCurFrame && view.pCurFrame->m_PictureStructureForDec < FRM_STRUCTURE)
                {
                    H264Slice * pFirstFrameSlice = view.pCurFrame->GetAU(0)->GetSlice(0);
                    if (!pFirstFrameSlice || !IsFieldOfOneFrame(view.pCurFrame, pFirstFrameSlice, slice))
                    {
                        ProcessNonPairedField(view.pCurFrame);
                        OnFullFrame(view.pCurFrame);
                        view.pCurFrame = 0;
                    }
                }

                if (view.GetPOCDecoder(0)->DetectFrameNumGap(slice))
                {
                    view.pCurFrame = 0;

                    Status umsRes = ProcessFrameNumGap(slice, 0, 0, maxDId);
                    if (umsRes != UMC_OK)
                        return umsRes;
                }
            }

            m_accessUnit.m_isInitialized = true;
        }

        size_t layersCount = m_accessUnit.GetLayersCount();

        {
            for (size_t i = 0; i < layersCount; i++)
            {
                SetOfSlices * setOfSlices = m_accessUnit.GetLayer(i);
                H264Slice * slice = setOfSlices->GetSlice(0);

                if (setOfSlices->m_frame)
                    continue;

                Status sts = AllocateNewFrame(slice, &setOfSlices->m_frame);
                if (sts != UMC_OK)
                    return sts;

                if (!setOfSlices->m_frame)
                    return UMC_ERR_NOT_ENOUGH_BUFFER;

                setOfSlices->m_frame->m_auIndex = m_accessUnit.GetAUIndentifier();
                ApplyPayloadsToFrame(setOfSlices->m_frame, slice, &setOfSlices->m_payloads);

                if (layersCount == 1)
                    break;

                return UMC_OK;
            }
        }

        for (size_t i = 0; i < layersCount; i++)
        {
            SetOfSlices * setOfSlices = m_accessUnit.GetLayer(i);
            if (setOfSlices->m_isCompleted)
                continue;

            H264Slice * lastSlice = setOfSlices->GetSlice(setOfSlices->GetSliceCount() - 1);

            FrameType Frame_type = SliceTypeToFrameType(lastSlice->GetSliceHeader()->slice_type);
            m_currentView = lastSlice->GetSliceHeader()->nal_ext.mvc.view_id;
            ViewItem &view = GetView(m_currentView);
            view.pCurFrame = setOfSlices->m_frame;

            //1)If frame_num has gap in a stream, for a kind of cases that the frame_num has a jump, the frame_num
            //gap will cause a reference frame to stay in DPB buffer constantly. Need to unmark the reference frame
            //in DPB buffer when the future frame with the same frame_num is coming.
            //2)If a stream has a wrong way to calculate the B frame's frame_num, this will cause DPB buffer confusion.
            //For example: The B frame's frame_num equals to the previous reference frame's frame_num.
            //For these cases, add an another condition (Frame_type != B_PICTURE) that unmark the reference frame when
            //decoding the P frame or I frame rather than B frame.
            if (lastSlice->GetSeqParam()->gaps_in_frame_num_value_allowed_flag != 1 && Frame_type != B_PICTURE)
            {
                // Check if DPB has ST frames with frame_num duplicating frame_num of new slice_type
                // If so, unmark such frames as ST.
                H264DecoderFrame * pHead = view.GetDPBList(0)->head();
                DPBSanitize(pHead, view.pCurFrame);
            }

            const H264SliceHeader *sliceHeader = lastSlice->GetSliceHeader();
            uint32_t field_index = setOfSlices->m_frame->GetNumberByParity(sliceHeader->bottom_field_flag);
            if (!setOfSlices->m_frame->GetAU(field_index)->GetSliceCount())
            {
                size_t count = setOfSlices->GetSliceCount();
                for (size_t sliceId = 0; sliceId < count; sliceId++)
                {
                    H264Slice * slice = setOfSlices->GetSlice(sliceId);
                    slice->m_pCurrentFrame = setOfSlices->m_frame;
                    AddSliceToFrame(setOfSlices->m_frame, slice);

                    if (slice->GetSliceHeader()->slice_type != INTRASLICE)
                    {
                        uint32_t NumShortTermRefs, NumLongTermRefs, NumInterViewRefs = 0;
                        view.GetDPBList(0)->countActiveRefs(NumShortTermRefs, NumLongTermRefs);
                        // calculate number of inter-view references
                        if (slice->GetSliceHeader()->nal_ext.mvc.view_id)
                        {
                            NumInterViewRefs = GetInterViewFrameRefs(m_views,
                                                                     slice->GetSliceHeader()->nal_ext.mvc.view_id,
                                                                     setOfSlices->m_frame->m_auIndex,
                                                                     slice->GetSliceHeader()->bottom_field_flag);
                        }

                        if (NumShortTermRefs + NumLongTermRefs + NumInterViewRefs == 0)
                            AddFakeReferenceFrame(slice);
                    }

                    slice->UpdateReferenceList(m_views, 0);
                }

                if (count && !setOfSlices->GetSlice(0)->IsSliceGroups())
                {
                    H264Slice * slice = setOfSlices->GetSlice(0);
                    if (slice->m_iFirstMB)
                    {
                        m_pSegmentDecoder[0]->RestoreErrorRect(0, slice->m_iFirstMB, slice);
                    }
                }
            }

            Status umcRes;
            if ((umcRes = CompleteFrame(setOfSlices->m_frame, field_index)) != UMC_OK)
            {
                return umcRes;
            }

            if (setOfSlices->m_frame->m_PictureStructureForDec < FRM_STRUCTURE && force && !pSlice)
            {
                if (ProcessNonPairedField(setOfSlices->m_frame))
                {
                    OnFullFrame(setOfSlices->m_frame);
                    view.pCurFrame = 0;
                }
            }

            if (setOfSlices->m_frame->m_PictureStructureForDec >= FRM_STRUCTURE || field_index)
            {
                OnFullFrame(setOfSlices->m_frame);
                view.pCurFrame = 0;
            }

            setOfSlices->m_isCompleted = true;
        }

        m_accessUnit.Reset();
        return layersCount ? UMC_OK : UMC_ERR_NOT_ENOUGH_DATA;
    }

    m_pLastSlice = 0;
    return UMC_ERR_NOT_ENOUGH_DATA;
}

void TaskSupplier::AddFakeReferenceFrame(H264Slice * )
{
}

void TaskSupplier::OnFullFrame(H264DecoderFrame * pFrame)
{
    pFrame->SetFullFrame(true);

    ViewItem &view = GetView(pFrame->m_viewId);

    if (!view.m_isDisplayable)
    {
        pFrame->setWasOutputted();
        pFrame->setWasDisplayed();
    }

    if (pFrame->IsSkipped())
        return;

    if (pFrame->m_bIDRFlag && !(pFrame->GetError() & ERROR_FRAME_DPB))
    {
        DecReferencePictureMarking::ResetError();
    }

    if (DecReferencePictureMarking::GetDPBError())
    {
        pFrame->SetErrorFlagged(ERROR_FRAME_DPB);
    }
}

Status TaskSupplier::InitializeLayers(AccessUnit *accessUnit, H264DecoderFrame * /*pFrame*/, int32_t /*field*/)
{
    accessUnit->SortforASO();
    size_t layersCount = accessUnit->GetLayersCount();

    if (!layersCount)
        return UMC_OK;

    if ((m_decodingMode == MVC_DECODING_MODE) && layersCount > 1)
    {
        H264Slice * sliceExtension = 0;
        for (size_t i = layersCount - 1; i >= 1; i--)
        {
            SetOfSlices * setOfSlices = accessUnit->GetLayer(i);
            size_t sliceCount = setOfSlices->GetSliceCount();
            for (size_t sliceId = 0; sliceId < sliceCount; sliceId++)
            {
                H264Slice * slice = setOfSlices->GetSlice(sliceId);
                if (NAL_UT_CODED_SLICE_EXTENSION == slice->GetSliceHeader()->nal_unit_type)
                {
                    sliceExtension = slice;
                    break;
                }
            }

            if (sliceExtension)
                break;
        }

        if (sliceExtension)
        {
            SetOfSlices * setOfSlices = accessUnit->GetLayer(0);
            size_t sliceCount = setOfSlices->GetSliceCount();
            for (size_t sliceId = 0; sliceId < sliceCount; sliceId++)
            {
                H264Slice * slice = setOfSlices->GetSlice(sliceId);
                if (NAL_UT_CODED_SLICE_EXTENSION != slice->GetSliceHeader()->nal_unit_type)
                {
                    if (!slice->m_pSeqParamSetMvcEx)
                        slice->SetSeqMVCParam(sliceExtension->m_pSeqParamSetMvcEx);
                    if (!slice->m_pSeqParamSetSvcEx)
                        slice->SetSeqSVCParam(sliceExtension->m_pSeqParamSetSvcEx);
                }
            }
        }
    }

    return UMC_OK;
}

Status TaskSupplier::CompleteFrame(H264DecoderFrame * pFrame, int32_t field)
{
    if (!pFrame)
        return UMC_OK;

    H264DecoderFrameInfo * slicesInfo = pFrame->GetAU(field);

    if (slicesInfo->GetStatus() > H264DecoderFrameInfo::STATUS_NOT_FILLED)
        return UMC_OK;

    DEBUG_PRINT((VM_STRING("Complete frame POC - (%d,%d) type - %d, picStruct - %d, field - %d, count - %d, m_uid - %d, IDR - %d, viewId - %d\n"), pFrame->m_PicOrderCnt[0], pFrame->m_PicOrderCnt[1], pFrame->m_FrameType, pFrame->m_displayPictureStruct, field, pFrame->GetAU(field)->GetSliceCount(), pFrame->m_UID, pFrame->m_bIDRFlag, pFrame->m_viewId));

    DBPUpdate(pFrame, field);

    // skipping algorithm
    {
        if (((slicesInfo->IsField() && field) || !slicesInfo->IsField()) &&
            IsShouldSkipFrame(pFrame, field))
        {
            if (slicesInfo->IsField())
            {
                pFrame->GetAU(0)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
                pFrame->GetAU(1)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
            }
            else
            {
                pFrame->GetAU(0)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
            }

            pFrame->SetisShortTermRef(false, 0);
            pFrame->SetisShortTermRef(false, 1);
            pFrame->SetisLongTermRef(false, 0);
            pFrame->SetisLongTermRef(false, 1);
            pFrame->SetSkipped(true);
            pFrame->OnDecodingCompleted();
            return UMC_OK;
        }
        else
        {
            if (IsShouldSkipDeblocking(pFrame, field))
            {
                pFrame->GetAU(field)->SkipDeblocking();
            }
        }
    }

    slicesInfo->SetStatus(H264DecoderFrameInfo::STATUS_FILLED);
    return UMC_OK;
}

void TaskSupplier::InitFreeFrame(H264DecoderFrame * pFrame, const H264Slice *pSlice)
{
    const H264SeqParamSet *pSeqParam = pSlice->GetSeqParam();

    pFrame->m_FrameType = SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type);
    pFrame->m_dFrameTime = pSlice->m_dTime;
    pFrame->m_crop_left = SubWidthC[pSeqParam->chroma_format_idc] * pSeqParam->frame_cropping_rect_left_offset;
    pFrame->m_crop_right = SubWidthC[pSeqParam->chroma_format_idc] * pSeqParam->frame_cropping_rect_right_offset;
    pFrame->m_crop_top = SubHeightC[pSeqParam->chroma_format_idc] * pSeqParam->frame_cropping_rect_top_offset * (2 - pSeqParam->frame_mbs_only_flag);
    pFrame->m_crop_bottom = SubHeightC[pSeqParam->chroma_format_idc] * pSeqParam->frame_cropping_rect_bottom_offset * (2 - pSeqParam->frame_mbs_only_flag);
    pFrame->m_crop_flag = pSeqParam->frame_cropping_flag;

    pFrame->setFrameNum(pSlice->GetSliceHeader()->frame_num);
    if (pSlice->GetSliceHeader()->nal_ext.extension_present &&
        0 == pSlice->GetSliceHeader()->nal_ext.svc_extension_flag)
    {
        pFrame->setViewId(pSlice->GetSliceHeader()->nal_ext.mvc.view_id);
    }

    pFrame->m_aspect_width  = pSeqParam->vui.sar_width;
    pFrame->m_aspect_height = pSeqParam->vui.sar_height;

    if (pSlice->GetSliceHeader()->field_pic_flag)
    {
        pFrame->m_bottom_field_flag[0] = pSlice->GetSliceHeader()->bottom_field_flag;
        pFrame->m_bottom_field_flag[1] = !pSlice->GetSliceHeader()->bottom_field_flag;

        pFrame->m_PictureStructureForRef =
        pFrame->m_PictureStructureForDec = FLD_STRUCTURE;
    }
    else
    {
        pFrame->m_bottom_field_flag[0] = 0;
        pFrame->m_bottom_field_flag[1] = 1;

        if (pSlice->GetSliceHeader()->MbaffFrameFlag)
        {
            pFrame->m_PictureStructureForRef =
            pFrame->m_PictureStructureForDec = AFRM_STRUCTURE;
        }
        else
        {
            pFrame->m_PictureStructureForRef =
            pFrame->m_PictureStructureForDec = FRM_STRUCTURE;
        }
    }

    int32_t iMBCount = pSeqParam->frame_width_in_mbs * pSeqParam->frame_height_in_mbs;
    pFrame->totalMBs = iMBCount;
    if (pFrame->m_PictureStructureForDec < FRM_STRUCTURE)
        pFrame->totalMBs /= 2;

    int32_t chroma_format_idc = pSeqParam->chroma_format_idc;

    uint8_t bit_depth_luma = pSeqParam->bit_depth_luma;
    uint8_t bit_depth_chroma = pSeqParam->bit_depth_chroma;

    int32_t bit_depth = std::max(bit_depth_luma, bit_depth_chroma);

    int32_t iMBWidth = pSeqParam->frame_width_in_mbs;
    int32_t iMBHeight = pSeqParam->frame_height_in_mbs;
    mfxSize dimensions = {iMBWidth * 16, iMBHeight * 16};

    ColorFormat cf = GetUMCColorFormat(chroma_format_idc);

    VideoDataInfo info;
    info.Init(dimensions.width, dimensions.height, cf, bit_depth);

    pFrame->Init(&info);
}

void TaskSupplier::InitFrameCounter(H264DecoderFrame * pFrame, const H264Slice *pSlice)
{
    const H264SliceHeader *sliceHeader = pSlice->GetSliceHeader();
    ViewItem &view = GetView(sliceHeader->nal_ext.mvc.view_id);

    if (view.GetPOCDecoder(0)->DetectFrameNumGap(pSlice, true))
    {
        H264DBPList* dpb = view.GetDPBList(0);
        assert(dpb);

        uint32_t NumShortTerm, NumLongTerm;
        dpb->countActiveRefs(NumShortTerm, NumLongTerm);

        if ((NumShortTerm + NumLongTerm > 0))
        {
            // set error flag only we have some references in DPB
            pFrame->SetErrorFlagged(ERROR_FRAME_REFERENCE_FRAME);

            // Leaving aside a legal frame_num wrapping cases, when a rapid _decrease_ of frame_num occurs due to frame gaps,
            // frames marked as short-term prior the gap may get stuck in DPB for a very long sequence (up to '(1 << log2_max_frame_num) - 1').
            // Reference lists are generated incorrectly. A potential recovery point can be at next I frame (if GOP is closed).
            // So let's mark these potentially dangereous ST frames
            // to remove them later from DPB in UpdateRefPicMarking() (if they're still there) at next I frame or SEI recovery point.
            for (H264DecoderFrame *pFrm = view.GetDPBList(0)->head(); pFrm; pFrm = pFrm->future())
            {
                if ((pFrm->FrameNum() > sliceHeader->frame_num) && pFrm->isShortTermRef())
                {
                    pFrm->SetErrorFlagged(ERROR_FRAME_SHORT_TERM_STUCK);
                }
            }
        }
    }

    if (sliceHeader->IdrPicFlag)
    {
        view.GetPOCDecoder(0)->Reset(sliceHeader->frame_num);
    }

    view.GetPOCDecoder(0)->DecodePictureOrderCount(pSlice, sliceHeader->frame_num);

    pFrame->m_bIDRFlag = (sliceHeader->IdrPicFlag != 0);

    const int32_t recoveryFrameNum = view.GetDPBList(0)->GetRecoveryFrameCnt();
    pFrame->m_bIFlag = (sliceHeader->slice_type == INTRASLICE) || (recoveryFrameNum != -1 && pFrame->FrameNum() == recoveryFrameNum);

    if (pFrame->m_bIDRFlag)
    {
        view.GetDPBList(0)->IncreaseRefPicListResetCount(pFrame);
    }

    pFrame->setFrameNum(sliceHeader->frame_num);

    uint32_t field_index = pFrame->GetNumberByParity(sliceHeader->bottom_field_flag);

    if (sliceHeader->field_pic_flag == 0)
        pFrame->setPicNum(sliceHeader->frame_num, 0);
    else
        pFrame->setPicNum(sliceHeader->frame_num*2+1, field_index);

    view.GetPOCDecoder(0)->DecodePictureOrderCountInitFrame(pFrame, field_index);

    DEBUG_PRINT((VM_STRING("Init frame POC - %d, %d, field - %d, uid - %d, frame_num - %d, viewId - %d\n"), pFrame->m_PicOrderCnt[0], pFrame->m_PicOrderCnt[1], field_index, pFrame->m_UID, pFrame->m_FrameNum, pFrame->m_viewId));

    pFrame->SetInterViewRef(0 != pSlice->GetSliceHeader()->nal_ext.mvc.inter_view_flag, field_index);
    pFrame->InitRefPicListResetCount(field_index);

} // void TaskSupplier::InitFrameCounter(H264DecoderFrame * pFrame, const H264Slice *pSlice)

void TaskSupplier::AddSliceToFrame(H264DecoderFrame *pFrame, H264Slice *pSlice)
{
    const H264SliceHeader *sliceHeader = pSlice->GetSliceHeader();

    if (pFrame->m_FrameType < SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type))
        pFrame->m_FrameType = SliceTypeToFrameType(pSlice->GetSliceHeader()->slice_type);

    uint32_t field_index = pFrame->GetNumberByParity(sliceHeader->bottom_field_flag);

    H264DecoderFrameInfo * au_info = pFrame->GetAU(field_index);
    int32_t iSliceNumber = au_info->GetSliceCount() + 1;

    if (field_index)
    {
        iSliceNumber += pFrame->m_TopSliceCount;
    }
    else
    {
        pFrame->m_TopSliceCount++;
    }

    pFrame->m_iNumberOfSlices++;

    pSlice->SetSliceNumber(iSliceNumber);
    pSlice->m_pCurrentFrame = pFrame;
    au_info->AddSlice(pSlice);
}

void TaskSupplier::DBPUpdate(H264DecoderFrame * pFrame, int32_t field)
{
    H264DecoderFrameInfo *slicesInfo = pFrame->GetAU(field);

    uint32_t count = slicesInfo->GetSliceCount();
    for (uint32_t i = 0; i < count; i++)
    {
        H264Slice * slice = slicesInfo->GetSlice(i);
        if (!slice->IsReference())
            continue;

        ViewItem &view = GetView(slice->GetSliceHeader()->nal_ext.mvc.view_id);

        DecReferencePictureMarking::UpdateRefPicMarking(view, pFrame, slice, field);
        break;
    }
}

H264DecoderFrame * TaskSupplier::FindSurface(FrameMemID id)
{
    AutomaticUMCMutex guard(m_mGuard);

    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    for (; iter != iter_end; ++iter)
    {
        H264DecoderFrame *pFrame = iter->GetDPBList(0)->head();
        for (; pFrame; pFrame = pFrame->future())
        {
            if (pFrame->GetFrameData()->GetFrameMID() == id)
                return pFrame;
        }
    }

    return 0;
}

Status TaskSupplier::RunDecoding()
{
    Status umcRes = CompleteDecodedFrames(0);
    if (umcRes != UMC_OK)
        return umcRes;

    ViewList::iterator iter = m_views.begin();
    ViewList::iterator iter_end = m_views.end();

    H264DecoderFrame *pFrame = 0;
    for (; iter != iter_end; ++iter)
    {
        pFrame = iter->GetDPBList(0)->head();

        for (; pFrame; pFrame = pFrame->future())
        {
            if (!pFrame->IsDecodingCompleted())
            {
                break;
            }
        }

        if (pFrame)
            break;
    }

    m_pTaskBroker->Start();

    if (!pFrame)
        return UMC_OK;

    //DEBUG_PRINT((VM_STRING("Decode POC - %d\n"), pFrame->m_PicOrderCnt[0]));

    return UMC_OK;
}

Status TaskSupplier::GetUserData(MediaData * pUD)
{
    if(!pUD)
        return UMC_ERR_NULL_PTR;

    if (!m_pLastDisplayed)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if (m_pLastDisplayed->m_UserData.user_data.size() && m_pLastDisplayed->m_UserData.payLoadSize &&
        m_pLastDisplayed->m_UserData.payLoadType == SEI_USER_DATA_REGISTERED_TYPE)
    {
        pUD->SetTime(m_pLastDisplayed->m_dFrameTime);
        pUD->SetBufferPointer(&m_pLastDisplayed->m_UserData.user_data[0],
            m_pLastDisplayed->m_UserData.payLoadSize);
        pUD->SetDataSize(m_pLastDisplayed->m_UserData.payLoadSize);
        //m_pLastDisplayed->m_UserData.Reset();
        return UMC_OK;
    }

    return UMC_ERR_NOT_ENOUGH_DATA;
}

void TaskSupplier::ApplyPayloadsToFrame(H264DecoderFrame * frame, H264Slice *slice, SeiPayloadArray * payloads)
{
    if (!payloads || !frame)
        return;

    if (m_sei_messages)
    {
        m_sei_messages->SetFrame(frame, frame->m_auIndex);
    }

    H264SEIPayLoad * payload = payloads->FindPayload(SEI_PIC_TIMING_TYPE);

    if (frame->m_displayPictureStruct == DPS_UNKNOWN)
    {
        if (payload && slice->GetSeqParam()->vui.pic_struct_present_flag)
        {
            frame->m_displayPictureStruct = payload->SEI_messages.pic_timing.pic_struct;
        }
        else
        {
            if (frame->m_PictureStructureForDec == FLD_STRUCTURE)
            {
                frame->m_displayPictureStruct = frame->GetNumberByParity(0) ? DPS_BOTTOM : DPS_TOP;
            }
            else
            {
                if (frame->PicOrderCnt(0, 1) == frame->PicOrderCnt(1, 1))
                {
                    frame->m_displayPictureStruct = DPS_FRAME;
                }
                else
                {
                    frame->m_displayPictureStruct = (frame->PicOrderCnt(0, 1) < frame->PicOrderCnt(1, 1)) ? DPS_TOP_BOTTOM : DPS_BOTTOM_TOP;
                }
            }
        }
    }

    frame->m_dpb_output_delay = DPBOutput::GetDPBOutputDelay(payload);

    if (frame->GetAU(0)->m_isBExist || frame->GetAU(1)->m_isBExist)
        DPBOutput::GetDPBOutputDelay(0);

    payload = payloads->FindPayload(SEI_DEC_REF_PIC_MARKING_TYPE);
    if (payload)
    {
        ViewItem &view = GetViewByNumber(BASE_VIEW);
        DecReferencePictureMarking::CheckSEIRepetition(view, payload);
    }
}

Status TaskSupplier::AllocateNewFrame(const H264Slice *slice, H264DecoderFrame **frame)
{
    if (!frame || !slice)
    {
        return UMC_ERR_NULL_PTR;
    }

    *frame = 0;

    m_currentView = slice->GetSliceHeader()->nal_ext.mvc.view_id;
    ViewItem &view = GetView(m_currentView);

    if (view.pCurFrame && view.pCurFrame->m_PictureStructureForDec < FRM_STRUCTURE)
    {
        H264Slice * pFirstFrameSlice = view.pCurFrame->GetAU(0)->GetSlice(0);
        if (pFirstFrameSlice && IsFieldOfOneFrame(view.pCurFrame, pFirstFrameSlice, slice))
        {
            *frame = view.pCurFrame;
            InitFrameCounter(*frame, slice);
        }

        view.pCurFrame = 0;
    }

    if (*frame)
        return UMC_OK;

    H264DecoderFrame *pFrame = GetFreeFrame(slice);
    if (!pFrame)
    {
        return UMC_ERR_NOT_ENOUGH_BUFFER;
    }

    Status umcRes = AllocateFrameData(pFrame);
    if (umcRes != UMC_OK)
    {
        return umcRes;
    }

    *frame = pFrame;

    if (slice->IsField())
    {
        pFrame->GetAU(1)->SetStatus(H264DecoderFrameInfo::STATUS_NOT_FILLED);
    }

    InitFrameCounter(pFrame, slice);

    return UMC_OK;
} // H264DecoderFrame * TaskSupplier::AddFrame(H264Slice *pSlice)

} // namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
