// Copyright (c) 2017-2018 Intel Corporation
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

/*  DEBUG: this file is requred to changing name
    to umc_h264_dec_slice_decode_pic */

#include "umc_h264_slice_decoding.h"
#include "umc_h264_dec.h"
#include "vm_debug.h"
#include "umc_h264_frame_list.h"

#include "umc_h264_task_supplier.h"

//#define ENABLE_LIST_TRACE
#define LIST_TRACE_FILE_NAME "d:/ipp.ref"

namespace UMC
{

static H264DecoderFrame *FindLastValidReference(H264DecoderFrame **pList, int32_t iLength)
{
    int32_t i;
    H264DecoderFrame *pLast = NULL;

    for (i = 0; i < iLength; i += 1)
    {
        if (pList[i])
            pLast = pList[i];
    }

    return pLast;

} // H264DecoderFrame *FindLastValidReference(H264DecoderFrame **pList,


Status H264Slice::UpdateReferenceList(ViewList &views,
                                      int32_t dIdIndex)
{
    UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo_L0 = &ReorderInfoL0;
    UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo_L1 = &ReorderInfoL1;
    const UMC_H264_DECODER::H264SeqParamSet *sps = GetSeqParam();
    uint32_t uMaxFrameNum;
    uint32_t uMaxPicNum;
    H264DecoderFrame *pFrm;
    H264DBPList *pDecoderFrameList = GetDPB(views, m_SliceHeader.nal_ext.mvc.view_id, dIdIndex);
    H264DecoderFrame *pHead = pDecoderFrameList->head();
    //uint32_t i;
    H264DecoderFrame **pRefPicList0;
    H264DecoderFrame **pRefPicList1;
    ReferenceFlags *pFields0;
    ReferenceFlags *pFields1;
    uint32_t NumShortTermRefs, NumLongTermRefs;
    H264RefListInfo rli;
    H264DecoderFrame *pLastInList[2] = {NULL, NULL};

    if (m_pCurrentFrame == nullptr)
        return UMC_ERR_NULL_PTR;

    const H264DecoderRefPicList* pH264DecRefPicList0 = m_pCurrentFrame->GetRefPicList(m_iNumber, 0);
    const H264DecoderRefPicList* pH264DecRefPicList1 = m_pCurrentFrame->GetRefPicList(m_iNumber, 1);
    if (pH264DecRefPicList0 == nullptr || pH264DecRefPicList1 == nullptr)
        return UMC_ERR_NULL_PTR;

    pRefPicList0 = pH264DecRefPicList0->m_RefPicList;
    pRefPicList1 = pH264DecRefPicList1->m_RefPicList;
    pFields0 = pH264DecRefPicList0->m_Flags;
    pFields1 = pH264DecRefPicList1->m_Flags;

    // Spec reference: 8.2.4, "Decoding process for reference picture lists
    // construction"

    // get pointers to the picture and sequence parameter sets currently in use
    uMaxFrameNum = (1<<sps->log2_max_frame_num);
    uMaxPicNum = (m_SliceHeader.field_pic_flag == 0) ? uMaxFrameNum : uMaxFrameNum<<1;

    for (pFrm = pHead; pFrm; pFrm = pFrm->future())
    {
        // update FrameNumWrap and picNum if frame number wrap occurred,
        // for short-term frames
        // TBD: modify for fields
        pFrm->UpdateFrameNumWrap((int32_t)m_SliceHeader.frame_num,
            uMaxFrameNum,
            m_pCurrentFrame->m_PictureStructureForDec +
            m_SliceHeader.bottom_field_flag);

        // For long-term references, update LongTermPicNum. Note this
        // could be done when LongTermFrameIdx is set, but this would
        // only work for frames, not fields.
        // TBD: modify for fields
        pFrm->UpdateLongTermPicNum(m_pCurrentFrame->m_PictureStructureForDec +
                                   m_SliceHeader.bottom_field_flag);
    }

    for (uint32_t number = 0; number <= MAX_NUM_REF_FRAMES + 1; number++)
    {
        pRefPicList0[number] = 0;
        pRefPicList1[number] = 0;
        pFields0[number].field = 0;
        pFields0[number].isShortReference = 0;
        pFields1[number].field = 0;
        pFields1[number].isShortReference = 0;
    }

    if ((m_SliceHeader.slice_type != INTRASLICE) && (m_SliceHeader.slice_type != S_INTRASLICE))
    {
        uint32_t NumInterViewRefs = 0;

        // Detect and report no available reference frames
        pDecoderFrameList->countActiveRefs(NumShortTermRefs, NumLongTermRefs);
        // calculate number of inter-view references
        if (m_SliceHeader.nal_ext.mvc.view_id != views.front().viewId)
        {
            NumInterViewRefs = GetInterViewFrameRefs(views, m_SliceHeader.nal_ext.mvc.view_id, m_pCurrentFrame->m_auIndex, m_SliceHeader.bottom_field_flag);
        }

        if ((NumShortTermRefs + NumLongTermRefs + NumInterViewRefs) == 0)
        {
            return UMC_ERR_INVALID_STREAM;
        }


        // Initialize the reference picture lists
        // Note the slice header get function always fills num_ref_idx_lx_active
        // fields with a valid value; either the override from the slice
        // header in the bitstream or the values from the pic param set when
        // there is no override.

        if (!m_SliceHeader.IdrPicFlag)
        {
            if ((m_SliceHeader.slice_type == PREDSLICE) ||
                (m_SliceHeader.slice_type == S_PREDSLICE))
            {
                pDecoderFrameList->InitPSliceRefPicList(this, pRefPicList0);
                pLastInList[0] = FindLastValidReference(pRefPicList0,
                                                        m_SliceHeader.num_ref_idx_l0_active);
            }
            else
            {
                pDecoderFrameList->InitBSliceRefPicLists(this, pRefPicList0, pRefPicList1, rli);
                pLastInList[0] = FindLastValidReference(pRefPicList0,
                                                        m_SliceHeader.num_ref_idx_l0_active);
                pLastInList[1] = FindLastValidReference(pRefPicList1,
                                                        m_SliceHeader.num_ref_idx_l1_active);
            }

            // Reorder the reference picture lists
            if (m_pCurrentFrame->m_PictureStructureForDec < FRM_STRUCTURE)
            {
                rli.m_iNumFramesInL0List = AdjustRefPicListForFields(pRefPicList0, pFields0, rli);
            }

            if (BPREDSLICE == m_SliceHeader.slice_type)
            {
                if (m_pCurrentFrame->m_PictureStructureForDec < FRM_STRUCTURE)
                {
                    rli.m_iNumFramesInL1List = AdjustRefPicListForFields(pRefPicList1, pFields1, rli);
                }

                if ((rli.m_iNumFramesInL0List == rli.m_iNumFramesInL1List) &&
                    (rli.m_iNumFramesInL0List > 1))
                {
                    bool isNeedSwap = true;
                    for (int32_t i = 0; i < rli.m_iNumFramesInL0List; i++)
                    {
                        if (pRefPicList1[i] != pRefPicList0[i] ||
                            pFields1[i].field != pFields0[i].field)
                        {
                            isNeedSwap = false;
                            break;
                        }
                    }

                    if (isNeedSwap)
                    {
                        std::swap(pRefPicList1[0], pRefPicList1[1]);
                        std::swap(pFields1[0], pFields1[1]);
                    }
                }
            }
        }

        // MVC extension. Add inter-view references
        if ((H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH == m_pSeqParamSet->profile_idc) ||
            (H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH == m_pSeqParamSet->profile_idc))
        {
            pDecoderFrameList->AddInterViewRefs(this, pRefPicList0, pFields0, LIST_0, views);
            if (m_SliceHeader.IdrPicFlag)
            {
                pLastInList[0] = FindLastValidReference(pRefPicList0, m_SliceHeader.num_ref_idx_l0_active);
            }

            if (BPREDSLICE == m_SliceHeader.slice_type)
            {
                pDecoderFrameList->AddInterViewRefs(this, pRefPicList1, pFields1, LIST_1, views);
                if (m_SliceHeader.IdrPicFlag)
                    pLastInList[1] = FindLastValidReference(pRefPicList1, m_SliceHeader.num_ref_idx_l1_active);
            }
        }


        if (pReorderInfo_L0->num_entries > 0)
            ReOrderRefPicList(pRefPicList0, pFields0, pReorderInfo_L0, uMaxPicNum, views, dIdIndex, 0);

        pRefPicList0[m_SliceHeader.num_ref_idx_l0_active] = 0;

        if (BPREDSLICE == m_SliceHeader.slice_type)
        {
            if (pReorderInfo_L1->num_entries > 0)
                ReOrderRefPicList(pRefPicList1, pFields1, pReorderInfo_L1, uMaxPicNum, views, dIdIndex, 1);

            pRefPicList1[m_SliceHeader.num_ref_idx_l1_active] = 0;
        }



        // set absent references
        {
            int32_t i;
            int32_t iCurField = 1;

            if (!pLastInList[0])
            {
                for (;;)
                {
                    for (H264DecoderFrame *pFrame = pDecoderFrameList->head(); pFrame; pFrame = pFrame->future())
                    {
                        if (pFrame->isShortTermRef()==3)
                            pLastInList[0] = pFrame;
                    }

                    if (pLastInList[0])
                        break;

                    for (H264DecoderFrame *pFrame = pDecoderFrameList->head(); pFrame; pFrame = pFrame->future())
                    {
                        if (pFrame->isLongTermRef()==3)
                            pLastInList[0] = pFrame;
                    }
                    break;
                }
            }

            if (BPREDSLICE == m_SliceHeader.slice_type && !pLastInList[1])
            {
                pLastInList[1] = pLastInList[0];
            }

            for (i = 0; i < m_SliceHeader.num_ref_idx_l0_active; i += 1)
            {
                if (NULL == pRefPicList0[i])
                {
                    pRefPicList0[i] = pLastInList[0];
                    m_pCurrentFrame->AddReference(pLastInList[0]);
                    pFields0[i].field = (int8_t) (iCurField ^= 1);
                    if (pRefPicList0[i])
                        pFields0[i].isShortReference = (pRefPicList0[i]->m_viewId == m_pCurrentFrame->m_viewId) ? 1 : 0;
                    else
                        pFields0[i].isShortReference = 1;
                    pFields0[i].isLongReference = 0;
                    m_pCurrentFrame->SetErrorFlagged(ERROR_FRAME_MAJOR);
                }
                else
                {
                    m_pCurrentFrame->AddReference(pRefPicList0[i]);
                    m_pCurrentFrame->SetErrorFlagged(pRefPicList0[i]->IsFrameExist() ? 0 : ERROR_FRAME_MAJOR);
                    pFields0[i].isShortReference =
                        pRefPicList0[i]->isShortTermRef(pRefPicList0[i]->GetNumberByParity(pFields0[i].field));
                    pFields0[i].isLongReference =
                        pRefPicList0[i]->isLongTermRef(pRefPicList0[i]->GetNumberByParity(pFields0[i].field));

                    if (pRefPicList0[i]->m_viewId != m_pCurrentFrame->m_viewId)
                        pFields0[i].isShortReference = pFields0[i].isLongReference = 0;
                }
            }

            if (BPREDSLICE == m_SliceHeader.slice_type)
            {
                iCurField = 1;

                for (i = 0; i < m_SliceHeader.num_ref_idx_l1_active; i += 1)
                {
                    if (NULL == pRefPicList1[i])
                    {
                        pRefPicList1[i] = pLastInList[1];
                        m_pCurrentFrame->AddReference(pLastInList[1]);
                        pFields1[i].field = (int8_t) (iCurField ^= 1);
                        pFields1[i].isShortReference = 1;
                        if (pRefPicList1[i])
                            pFields1[i].isShortReference = (pRefPicList1[i]->m_viewId == m_pCurrentFrame->m_viewId) ? 1 : 0;
                        else
                            pFields1[i].isShortReference = 1;
                        pFields1[i].isLongReference = 0;
                        m_pCurrentFrame->SetErrorFlagged(ERROR_FRAME_MAJOR);
                    }
                    else
                    {
                        m_pCurrentFrame->AddReference(pRefPicList1[i]);
                        m_pCurrentFrame->SetErrorFlagged(pRefPicList1[i]->IsFrameExist() ? 0 : ERROR_FRAME_MAJOR);
                        pFields1[i].isShortReference =
                            pRefPicList1[i]->isShortTermRef(pRefPicList1[i]->GetNumberByParity(pFields1[i].field));
                        pFields1[i].isLongReference =
                            pRefPicList1[i]->isLongTermRef(pRefPicList1[i]->GetNumberByParity(pFields1[i].field));

                        if (pRefPicList1[i]->m_viewId != m_pCurrentFrame->m_viewId)
                            pFields1[i].isShortReference = pFields1[i].isLongReference = 0;
                    }
                }
            }
        }
    }

    return UMC_OK;

} // Status H264Slice::UpdateRefPicList(H264DecoderFrameList *pDecoderFrameList)

int32_t H264Slice::AdjustRefPicListForFields(H264DecoderFrame **pRefPicList,
                                          ReferenceFlags *pFields,
                                          H264RefListInfo &rli)
{
    H264DecoderFrame *TempList[MAX_NUM_REF_FRAMES+1];
    uint8_t TempFields[MAX_NUM_REF_FRAMES+1];
    //walk through list and set correct indices
    int32_t i=0,j=0,numSTR=0,numLTR=0;
    int32_t num_same_parity = 0, num_opposite_parity = 0;
    int8_t current_parity = m_SliceHeader.bottom_field_flag;
    //first scan the list to determine number of shortterm and longterm reference frames
    while ((numSTR < 16) && pRefPicList[numSTR] && pRefPicList[numSTR]->isShortTermRef()) numSTR++;
    while ((numSTR + numLTR < 16) && pRefPicList[numSTR+numLTR] && pRefPicList[numSTR+numLTR]->isLongTermRef()) numLTR++;

    while (num_same_parity < numSTR ||  num_opposite_parity < numSTR)
    {
        //try to fill shorttermref fields with the same parity first
        if (num_same_parity < numSTR)
        {
            while (num_same_parity < numSTR)
            {
                int32_t ref_field = pRefPicList[num_same_parity]->GetNumberByParity(current_parity);
                if (pRefPicList[num_same_parity]->isShortTermRef(ref_field))
                    break;

                num_same_parity++;

            }

            if (num_same_parity<numSTR)
            {
                TempList[i] = pRefPicList[num_same_parity];
                TempFields[i] = current_parity;
                i++;
                num_same_parity++;
            }
        }
        //now process opposite parity
        if (num_opposite_parity < numSTR)
        {
            while (num_opposite_parity < numSTR)
            {
                int32_t ref_field = pRefPicList[num_opposite_parity]->GetNumberByParity(!current_parity);
                if (pRefPicList[num_opposite_parity]->isShortTermRef(ref_field))
                    break;
                num_opposite_parity++;
            }

            if (num_opposite_parity<numSTR) //selected field is reference
            {
                TempList[i] = pRefPicList[num_opposite_parity];
                TempFields[i] = !current_parity;
                i++;
                num_opposite_parity++;
            }
        }
    }

    rli.m_iNumShortEntriesInList = (uint8_t) i;
    num_same_parity = num_opposite_parity = 0;
    //now processing LongTermRef
    while(num_same_parity<numLTR ||  num_opposite_parity<numLTR)
    {
        //try to fill longtermref fields with the same parity first
        if (num_same_parity < numLTR)
        {
            while (num_same_parity < numLTR)
            {
                int32_t ref_field = pRefPicList[num_same_parity+numSTR]->GetNumberByParity(current_parity);
                if (pRefPicList[num_same_parity+numSTR]->isLongTermRef(ref_field))
                    break;
                num_same_parity++;
            }
            if (num_same_parity<numLTR)
            {
                TempList[i] = pRefPicList[num_same_parity+numSTR];
                TempFields[i] = current_parity;
                i++;
                num_same_parity++;
            }
        }
        //now process opposite parity
        if (num_opposite_parity<numLTR)
        {
            while (num_opposite_parity < numLTR)
            {
                int32_t ref_field = pRefPicList[num_opposite_parity+numSTR]->GetNumberByParity(!current_parity);

                if (pRefPicList[num_opposite_parity+numSTR]->isLongTermRef(ref_field))
                    break;
                num_opposite_parity++;
            }

            if (num_opposite_parity<numLTR) //selected field is reference
            {
                TempList[i] = pRefPicList[num_opposite_parity+numSTR];
                TempFields[i] = !current_parity;
                i++;
                num_opposite_parity++;
            }
        }
    }

    rli.m_iNumLongEntriesInList = (uint8_t) (i - rli.m_iNumShortEntriesInList);
    j = 0;
    while(j < i)//copy data back to list
    {
        pRefPicList[j] = TempList[j];
        pFields[j].field = TempFields[j];
        pFields[j].isShortReference = (unsigned char) (pRefPicList[j]->isShortTermRef(pRefPicList[j]->GetNumberByParity(TempFields[j])) ? 1 : 0);
        j++;
    }

    while(j < (int32_t)MAX_NUM_REF_FRAMES)//fill remaining entries
    {
        pRefPicList[j] = NULL;
        pFields[j].field = 0;
        pFields[j].isShortReference = 0;
        j++;
    }

    return i;
} // int32_t H264Slice::AdjustRefPicListForFields(H264DecoderFrame **pRefPicList, int8_t *pFields)

void H264Slice::ReOrderRefPicList(H264DecoderFrame **pRefPicList,
                                  ReferenceFlags *pFields,
                                  UMC_H264_DECODER::RefPicListReorderInfo *pReorderInfo,
                                  int32_t MaxPicNum,
                                  ViewList &views,
                                  int32_t dIdIndex,
                                  uint32_t listNum)
{
    bool bIsFieldSlice = (m_SliceHeader.field_pic_flag != 0);
    uint32_t NumRefActive = (0 == listNum) ?
                          m_SliceHeader.num_ref_idx_l0_active :
                          m_SliceHeader.num_ref_idx_l1_active;
    uint32_t i;
    int32_t picNumNoWrap;
    int32_t picNum;
    int32_t picNumPred;
    int32_t picNumCurr;
    int32_t picViewIdxLXPred = -1;
    H264DBPList *pDecoderFrameList = GetDPB(views, m_SliceHeader.nal_ext.mvc.view_id, dIdIndex);

    int32_t pocForce = bIsFieldSlice ? 0 : 3;

    // Reference: Reordering process for reference picture lists, 8.2.4.3
    picNumCurr = m_pCurrentFrame->PicNum(m_pCurrentFrame->GetNumberByParity(m_SliceHeader.bottom_field_flag), pocForce);
    picNumPred = picNumCurr;

    for (i=0; i<pReorderInfo->num_entries; i++)
    {
        if (pReorderInfo->reordering_of_pic_nums_idc[i] < 2)
        {
            // short-term reorder
            if (pReorderInfo->reordering_of_pic_nums_idc[i] == 0)
            {
                picNumNoWrap = picNumPred - pReorderInfo->reorder_value[i];
                if (picNumNoWrap < 0)
                    picNumNoWrap += MaxPicNum;
            }
            else
            {
                picNumNoWrap = picNumPred + pReorderInfo->reorder_value[i];
                if (picNumNoWrap >= MaxPicNum)
                    picNumNoWrap -= MaxPicNum;
            }
            picNumPred = picNumNoWrap;

            picNum = picNumNoWrap;
            if (picNum > picNumCurr)
                picNum -= MaxPicNum;

            int32_t frameField;
            H264DecoderFrame* pFrame = pDecoderFrameList->findShortTermPic(picNum, &frameField);

            for (uint32_t k = NumRefActive; k > i; k--)
            {
                pRefPicList[k] = pRefPicList[k - 1];
                pFields[k] = pFields[k - 1];
            }

            // Place picture with picNum on list, shifting pictures
            // down by one while removing any duplication of picture with picNum.
            pRefPicList[i] = pFrame;
            pFields[i].field = (char) ((pFrame && bIsFieldSlice) ? pFrame->m_bottom_field_flag[frameField] : 0);
            pFields[i].isShortReference = 1;
            int32_t refIdxLX = i + 1;

            for(uint32_t kk = i + 1; kk <= NumRefActive; kk++)
            {
                if (pRefPicList[kk])
                {
                    if (!(pRefPicList[kk]->isShortTermRef(pRefPicList[kk]->GetNumberByParity(pFields[kk].field)) &&
                        pRefPicList[kk]->PicNum(pRefPicList[kk]->GetNumberByParity(pFields[kk].field), 1) == picNum))
                    {
                        pRefPicList[refIdxLX] = pRefPicList[kk];
                        pFields[refIdxLX] = pFields[kk];
                        refIdxLX++;
                    }
                }
            }
        }    // short-term reorder
        else if (2 == pReorderInfo->reordering_of_pic_nums_idc[i])
        {
            // long term reorder
            picNum = pReorderInfo->reorder_value[i];

            int32_t frameField;
            H264DecoderFrame* pFrame = pDecoderFrameList->findLongTermPic(picNum, &frameField);

            for (uint32_t k = NumRefActive; k > i; k--)
            {
                pRefPicList[k] = pRefPicList[k - 1];
                pFields[k] = pFields[k - 1];
            }

            // Place picture with picNum on list, shifting pictures
            // down by one while removing any duplication of picture with picNum.
            pRefPicList[i] = pFrame;
            pFields[i].field = (char) ((pFrame && bIsFieldSlice) ? pFrame->m_bottom_field_flag[frameField] : 0);
            pFields[i].isShortReference = 0;
            int32_t refIdxLX = i + 1;

            for(uint32_t kk = i + 1; kk <= NumRefActive; kk++)
            {
                if (pRefPicList[kk])
                {
                    if (!(pRefPicList[kk]->isLongTermRef(pRefPicList[kk]->GetNumberByParity(pFields[kk].field)) &&
                        pRefPicList[kk]->LongTermPicNum(pRefPicList[kk]->GetNumberByParity(pFields[kk].field), 1) == picNum))
                    {
                        pRefPicList[refIdxLX] = pRefPicList[kk];
                        pFields[refIdxLX] = pFields[kk];
                        refIdxLX++;
                    }
                }
            }
        }    // long term reorder
        // MVC reordering
        else if ((4 == pReorderInfo->reordering_of_pic_nums_idc[i]) ||
                    (5 == pReorderInfo->reordering_of_pic_nums_idc[i]))
        {
            int32_t abs_diff_view_idx = pReorderInfo->reorder_value[i];
            int32_t maxViewIdx;
            int32_t targetViewID;

            if (!m_pSeqParamSetMvcEx)
            {
                m_pCurrentFrame->SetErrorFlagged(ERROR_FRAME_MAJOR);
                continue;
            }

            uint32_t picViewIdxLX;
            H264DecoderFrame* pFrame = NULL;
            int32_t curPOC = m_pCurrentFrame->PicOrderCnt(m_pCurrentFrame->GetNumberByParity(m_SliceHeader.bottom_field_flag), pocForce);

            // set current VO index
            const auto currView = std::find_if(m_pSeqParamSetMvcEx->viewInfo.begin(), m_pSeqParamSetMvcEx->viewInfo.end(),
                                         [this](const UMC_H264_DECODER::H264ViewRefInfo &item){ return item.view_id == m_SliceHeader.nal_ext.mvc.view_id; });
            if (currView == m_pSeqParamSetMvcEx->viewInfo.end())
            {
                throw h264_exception(UMC_ERR_INVALID_STREAM);
            }

            // get the maximum number of reference
            if (m_SliceHeader.nal_ext.mvc.anchor_pic_flag)
            {
                maxViewIdx = currView->num_anchor_refs_lx[listNum];
            }
            else
            {
                maxViewIdx = currView->num_non_anchor_refs_lx[listNum];
            }

            // get the index of view reference
            if (4 == pReorderInfo->reordering_of_pic_nums_idc[i])
            {
                if (picViewIdxLXPred - abs_diff_view_idx < 0)
                {
                    picViewIdxLX = picViewIdxLXPred - abs_diff_view_idx + maxViewIdx;
                }
                else
                {
                    picViewIdxLX = picViewIdxLXPred - abs_diff_view_idx;
                }
            }
            else
            {
                if (picViewIdxLXPred + abs_diff_view_idx >= maxViewIdx)
                {
                    picViewIdxLX = picViewIdxLXPred + abs_diff_view_idx - maxViewIdx;
                }
                else
                {
                    picViewIdxLX = picViewIdxLXPred + abs_diff_view_idx;
                }
            }
            picViewIdxLXPred = picViewIdxLX;
            if (picViewIdxLX >= UMC::H264_MAX_NUM_VIEW_REF)
            {
                m_pCurrentFrame->SetErrorFlagged(ERROR_FRAME_MAJOR);
                continue;
            }
            // get the target view
            if (m_SliceHeader.nal_ext.mvc.anchor_pic_flag)
            {
                targetViewID = currView->anchor_refs_lx[listNum][picViewIdxLX];
            }
            else
            {
                targetViewID = currView->non_anchor_refs_lx[listNum][picViewIdxLX];
            }
            ViewList::iterator iter = views.begin();
            ViewList::iterator iter_end = views.end();
            for (; iter != iter_end; ++iter)
            {
                if (targetViewID == iter->viewId)
                {
                    pFrame = iter->GetDPBList(dIdIndex)->findInterViewRef(m_pCurrentFrame->m_auIndex, m_SliceHeader.bottom_field_flag);
                    break;
                }
            }
            // corresponding frame is found
            if (!pFrame)
                continue;

            // make some space to insert the reference
            for (uint32_t k = NumRefActive; k > i; k--)
            {
                pRefPicList[k] = pRefPicList[k - 1];
                pFields[k] = pFields[k - 1];
            }

            // Place picture with picNum on list, shifting pictures
            // down by one while removing any duplication of picture with cuuPOC.
            pRefPicList[i] = pFrame;
            pFields[i].field = m_SliceHeader.bottom_field_flag;
            pFields[i].isShortReference = 1;
            uint32_t refIdxLX = i + 1;

            for (uint32_t kk = i + 1; kk <= NumRefActive; kk++)
            {
                if (pRefPicList[kk])
                {
                    uint32_t refFieldIdx = pRefPicList[kk]->GetNumberByParity(m_SliceHeader.bottom_field_flag);

                    if ((pRefPicList[kk]->m_viewId != targetViewID) ||
                        (pRefPicList[kk]->PicOrderCnt(refFieldIdx, pocForce) != curPOC))
                    {
                        pRefPicList[refIdxLX] = pRefPicList[kk];
                        pFields[refIdxLX] = pFields[kk];
                        refIdxLX++;
                    }
                }
            }
        }
    }    // for i
} // void H264Slice::ReOrderRefPicList(H264DecoderFrame **pRefPicList,

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE
