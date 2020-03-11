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

#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#include "umc_h265_slice_decoding.h"
#include "umc_h265_heap.h"
#include "umc_h265_frame_info.h"
#include "umc_h265_frame_list.h"

#include <algorithm>
#include <numeric>

namespace UMC_HEVC_DECODER
{
H265Slice::H265Slice()
    : m_pSeqParamSet(0)
    , m_context(0)
    , m_tileByteLocation (0)
{
    Reset();
} // H265Slice::H265Slice()

H265Slice::~H265Slice()
{
    Release();

} // H265Slice::~H265Slice(void)

// Initialize slice structure to default values
void H265Slice::Reset()
{
    m_source.Release();

    if (m_pSeqParamSet)
    {
        if (m_pSeqParamSet)
            ((H265SeqParamSet*)m_pSeqParamSet)->DecrementReference();
        if (m_pPicParamSet)
            ((H265PicParamSet*)m_pPicParamSet)->DecrementReference();
        m_pSeqParamSet = 0;
        m_pPicParamSet = 0;
    }

    m_pCurrentFrame = 0;
    m_NumEmuPrevnBytesInSliceHdr = 0;

    m_SliceHeader.nuh_temporal_id = 0;
    m_SliceHeader.m_CheckLDC = false;
    m_SliceHeader.slice_deblocking_filter_disabled_flag = false;
    m_SliceHeader.num_entry_point_offsets = 0;

    m_tileCount = 0;
    delete[] m_tileByteLocation;
    m_tileByteLocation = 0;
}

// Release resources
void H265Slice::Release()
{
    Reset();
} // void H265Slice::Release(void)

// Parse beginning of slice header to get PPS ID
int32_t H265Slice::RetrievePicParamSetNumber()
{
    if (!m_source.GetDataSize())
        return -1;

    m_SliceHeader = {};
    m_BitStream.Reset((uint8_t *) m_source.GetPointer(), (uint32_t) m_source.GetDataSize());

    UMC::Status umcRes = UMC::UMC_OK;

    try
    {
        umcRes = m_BitStream.GetNALUnitType(m_SliceHeader.nal_unit_type,
                                            m_SliceHeader.nuh_temporal_id);
        if (UMC::UMC_OK != umcRes)
            return false;

        // decode first part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart1(&m_SliceHeader);
        if (UMC::UMC_OK != umcRes)
            return -1;
    } catch (...)
    {
        return -1;
    }

    return m_SliceHeader.slice_pic_parameter_set_id;
}

// Decode slice header and initializ slice structure with parsed values
bool H265Slice::Reset(PocDecoding * pocDecoding)
{
    m_BitStream.Reset((uint8_t *) m_source.GetPointer(), (uint32_t) m_source.GetDataSize());

    // decode slice header
    if (m_source.GetDataSize() && false == DecodeSliceHeader(pocDecoding))
        return false;

    m_SliceHeader.m_HeaderBitstreamOffset = (uint32_t)m_BitStream.BytesDecoded();

    m_SliceHeader.m_SeqParamSet = m_pSeqParamSet;
    m_SliceHeader.m_PicParamSet = m_pPicParamSet;

    int32_t iMBInFrame = (GetSeqParam()->WidthInCU * GetSeqParam()->HeightInCU);

    // set slice variables
    m_iFirstMB = m_SliceHeader.slice_segment_address;
    m_iFirstMB = m_iFirstMB > iMBInFrame ? iMBInFrame : m_iFirstMB;
    m_iFirstMB = m_pPicParamSet->m_CtbAddrRStoTS[m_iFirstMB];
    m_iMaxMB = iMBInFrame;

    processInfo.Initialize(m_iFirstMB, GetSeqParam()->WidthInCU);

    m_bError = false;

    // frame is not associated yet
    m_pCurrentFrame = NULL;
    return true;

} // bool H265Slice::Reset(void *pSource, size_t nSourceSize, int32_t iNumber)

// Set current slice number
void H265Slice::SetSliceNumber(int32_t iSliceNumber)
{
    m_iNumber = iSliceNumber;

} // void H265Slice::SetSliceNumber(int32_t iSliceNumber)

// Decoder slice header and calculate POC
bool H265Slice::DecodeSliceHeader(PocDecoding * pocDecoding)
{
    UMC::Status umcRes = UMC::UMC_OK;
    // Locals for additional slice data to be read into, the data
    // was read and saved from the first slice header of the picture,
    // is not supposed to change within the picture, so can be
    // discarded when read again here.
    try
    {
        m_SliceHeader = {};

        umcRes = m_BitStream.GetNALUnitType(m_SliceHeader.nal_unit_type,
                                            m_SliceHeader.nuh_temporal_id);
        if (UMC::UMC_OK != umcRes)
            return false;

        umcRes = m_BitStream.GetSliceHeaderFull(this, m_pSeqParamSet, m_pPicParamSet, pocDecoding);

        if (!GetSliceHeader()->dependent_slice_segment_flag)
        {
            if (!GetSliceHeader()->IdrPicFlag)
            {
                const H265SeqParamSet * sps = m_pSeqParamSet;
                H265SliceHeader * sliceHdr = &m_SliceHeader;

                if (GetSeqParam()->long_term_ref_pics_present_flag)
                {
                    ReferencePictureSet *rps = getRPS();
                    uint32_t offset = rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures();

                    int32_t prevDeltaMSB = 0;
                    int32_t maxPicOrderCntLSB = 1 << sps->log2_max_pic_order_cnt_lsb;
                    int32_t DeltaPocMsbCycleLt = 0;
                    for(uint32_t j = offset, k = 0; k < rps->getNumberOfLongtermPictures(); j++, k++)
                    {
                        int pocLsbLt = rps->poc_lbs_lt[j];
                        if (rps->delta_poc_msb_present_flag[j])
                        {
                            bool deltaFlag = false;

                            if( (j == offset) || (j == (offset + rps->num_long_term_sps)))
                                deltaFlag = true;

                            if(deltaFlag)
                                DeltaPocMsbCycleLt = rps->delta_poc_msb_cycle_lt[j];
                            else
                                DeltaPocMsbCycleLt = rps->delta_poc_msb_cycle_lt[j] + prevDeltaMSB;

                            int32_t pocLTCurr = sliceHdr->m_poc - DeltaPocMsbCycleLt * maxPicOrderCntLSB - sliceHdr->slice_pic_order_cnt_lsb + pocLsbLt;
                            rps->setPOC(j, pocLTCurr);
                            rps->setDeltaPOC(j, - sliceHdr->m_poc + pocLTCurr);
                        }
                        else
                        {
                            rps->setPOC     (j, pocLsbLt);
                            rps->setDeltaPOC(j, - sliceHdr->m_poc + pocLsbLt);
                            if (j == offset + rps->num_long_term_sps)
                                DeltaPocMsbCycleLt = 0;
                        }

                        prevDeltaMSB = DeltaPocMsbCycleLt;
                    }

                    offset += rps->getNumberOfLongtermPictures();
                    rps->num_pics = offset;
                }

                if ( sliceHdr->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_LP
                    || sliceHdr->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_RADL
                    || sliceHdr->nal_unit_type == NAL_UT_CODED_SLICE_BLA_N_LP )
                {
                    ReferencePictureSet *rps = getRPS();
                    rps->num_negative_pics = 0;
                    rps->num_positive_pics = 0;
                    rps->setNumberOfLongtermPictures(0);
                    rps->num_pics = 0;
                }

                if (GetSliceHeader()->nuh_temporal_id == 0 && sliceHdr->nal_unit_type != NAL_UT_CODED_SLICE_RADL_R &&
                    sliceHdr->nal_unit_type != NAL_UT_CODED_SLICE_RASL_R && !IsSubLayerNonReference(sliceHdr->nal_unit_type))
                {
                    pocDecoding->prevPicOrderCntMsb    = sliceHdr->m_poc - sliceHdr->slice_pic_order_cnt_lsb;
                    pocDecoding->prevPocPicOrderCntLsb = sliceHdr->slice_pic_order_cnt_lsb;
                }
            }
            else
            {
                if (GetSliceHeader()->nuh_temporal_id == 0)
                {
                    pocDecoding->prevPicOrderCntMsb = 0;
                    pocDecoding->prevPocPicOrderCntLsb = 0;
                }
            }
        }

   }
    catch(...)
    {
	if (!m_SliceHeader.dependent_slice_segment_flag)
        {
            if (m_SliceHeader.slice_type != I_SLICE)
                m_bError = true;
        }

        return false;
    }

    return (UMC::UMC_OK == umcRes);

} // bool H265Slice::DecodeSliceHeader(bool bFullInitialization)

// Get tile column CTB width
uint32_t H265Slice::getTileColumnWidth(uint32_t col) const
{
    uint32_t tileColumnWidth = 0;
    uint32_t lcuSize = 1 << m_pSeqParamSet->log2_max_luma_coding_block_size;
    uint32_t widthInLcu = (m_pSeqParamSet->pic_width_in_luma_samples + lcuSize - 1) / lcuSize;

    if (m_pPicParamSet->uniform_spacing_flag)
    {
        tileColumnWidth = ((col + 1) * widthInLcu) / m_pPicParamSet->num_tile_columns -
            (col * widthInLcu) / m_pPicParamSet->num_tile_columns;
    }
    else
    {
        if (col == (m_pPicParamSet->num_tile_columns - 1))
        {
            for (uint32_t i = 0; i < (m_pPicParamSet->num_tile_columns - 1); i++)
            {
                tileColumnWidth += m_pPicParamSet->column_width[i];
            }
            tileColumnWidth = widthInLcu - tileColumnWidth;
        }
        else
        {
            tileColumnWidth = m_pPicParamSet->column_width[col];
        }
    }
    return tileColumnWidth;
}

// Get tile row CTB height
uint32_t H265Slice::getTileRowHeight(uint32_t row) const
{
    uint32_t tileRowHeight = 0;
    uint32_t lcuSize = 1 << m_pSeqParamSet->log2_max_luma_coding_block_size;
    uint32_t heightInLcu = (m_pSeqParamSet->pic_height_in_luma_samples + lcuSize - 1) / lcuSize;

    if (m_pPicParamSet->uniform_spacing_flag)
    {
        tileRowHeight = ((row + 1) * heightInLcu) / m_pPicParamSet->num_tile_rows -
            (row * heightInLcu) / m_pPicParamSet->num_tile_rows;
    }
    else
    {
        if (row == (m_pPicParamSet->num_tile_rows - 1))
        {
            for (uint32_t i = 0; i < (m_pPicParamSet->num_tile_rows - 1); i++)
            {
                tileRowHeight += m_pPicParamSet->row_height[i];
            }
            tileRowHeight = heightInLcu - tileRowHeight;
        }
        else
        {
            tileRowHeight = m_pPicParamSet->row_height[row];
        }
    }
    return tileRowHeight;
}

// Get tile XIdx
uint32_t H265Slice::getTileXIdx() const
{
    uint32_t i;
    uint32_t lcuOffset = 0;
    uint32_t lcuSize = 1 << m_pSeqParamSet->log2_max_luma_coding_block_size;
    uint32_t widthInLcu = (m_pSeqParamSet->pic_width_in_luma_samples + lcuSize - 1) / lcuSize;
    uint32_t lcuColumnOffset = m_SliceHeader.slice_segment_address % widthInLcu;

    for (i = 0; i < (m_pPicParamSet->num_tile_columns-1); i++)
    {
        int tileColumnWidth = getTileColumnWidth(i);
        if (lcuColumnOffset >= lcuOffset && lcuColumnOffset < lcuOffset + tileColumnWidth)
            break;
        lcuOffset += tileColumnWidth;
    }
    return i;
}

// Get tile YIdx
uint32_t H265Slice::getTileYIdx() const
{
    uint32_t i;
    uint32_t lcuOffset = 0;
    uint32_t lcuSize = 1 << m_pSeqParamSet->log2_max_luma_coding_block_size;
    uint32_t widthInLcu = (m_pSeqParamSet->pic_width_in_luma_samples + lcuSize - 1) / lcuSize;
    uint32_t lcuRowOffset = m_SliceHeader.slice_segment_address / widthInLcu;

    for (i = 0; i < (m_pPicParamSet->num_tile_rows - 1); i++)
    {
        int tileRowHeight = getTileRowHeight(i);
        if (lcuRowOffset >= lcuOffset && lcuRowOffset < lcuOffset + tileRowHeight)
            break;
        lcuOffset += tileRowHeight;
    }
    return i;
}

// Returns number of used references in RPS
int H265Slice::getNumRpsCurrTempList() const
{
  int numRpsCurrTempList = 0;

  if (GetSliceHeader()->slice_type != I_SLICE)
  {
      const ReferencePictureSet *rps = getRPS();
      bool intra_block_copy_flag = (GetPicParam()->pps_curr_pic_ref_enabled_flag != 0);

      for (uint32_t i=0;i < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures() + rps->getNumberOfLongtermPictures();i++)
      {
          if (rps->getUsed(i))
          {
              numRpsCurrTempList++;
          }
      }

      if (intra_block_copy_flag)
      {
          numRpsCurrTempList++;
      }
  }

  return numRpsCurrTempList;
}

void H265Slice::setRefPOCListSliceHeader()
{
    int32_t  RefPicPOCSetStCurr0[MAX_NUM_REF_PICS] = {};
    int32_t  RefPicPOCSetStCurr1[MAX_NUM_REF_PICS] = {};
    int32_t  RefPicPOCSetLtCurr[MAX_NUM_REF_PICS]  = {};
    uint32_t NumPicStCurr0 = 0;
    uint32_t NumPicStCurr1 = 0;
    uint32_t NumPicLtCurr  = 0;
    uint32_t i;

    if (m_SliceHeader.slice_type == I_SLICE)
        return;

    for (i = 0; i < getRPS()->getNumberOfNegativePictures(); i++)
    {
        if (getRPS()->getUsed(i))
        {
            RefPicPOCSetStCurr0[NumPicStCurr0] = m_SliceHeader.m_poc + getRPS()->getDeltaPOC(i);
            NumPicStCurr0++;
        }
    }
    m_SliceHeader.m_numPicStCurr0 = NumPicStCurr0;

    for (; i < getRPS()->getNumberOfNegativePictures() + getRPS()->getNumberOfPositivePictures(); i++)
    {
        if (getRPS()->getUsed(i))
        {
            RefPicPOCSetStCurr1[NumPicStCurr1] = m_SliceHeader.m_poc + getRPS()->getDeltaPOC(i);
            NumPicStCurr1++;
        }
    }
    m_SliceHeader.m_numPicStCurr1 = NumPicStCurr1;

    for (i = getRPS()->getNumberOfNegativePictures() + getRPS()->getNumberOfPositivePictures();
        i < getRPS()->getNumberOfNegativePictures() + getRPS()->getNumberOfPositivePictures() + getRPS()->getNumberOfLongtermPictures(); i++)
    {
        if (getRPS()->getUsed(i))
        {
            RefPicPOCSetLtCurr[NumPicLtCurr] = getRPS()->getPOC(i);
            NumPicLtCurr++;
        }
    }
    m_SliceHeader.m_numPicLtCurr = NumPicLtCurr;

    H265PicParamSet const* pps = GetPicParam();
    VM_ASSERT(pps);

    int32_t numPicTotalCurr = NumPicStCurr0 + NumPicStCurr1 + NumPicLtCurr;
    if (pps->pps_curr_pic_ref_enabled_flag)
        numPicTotalCurr++;

    int32_t cIdx = 0;
    int32_t rIdx = 0;
    for (i = 0; i < NumPicStCurr0; cIdx++, i++)
    {
        m_SliceHeader.m_RpsPOCCurrList0[cIdx] = RefPicPOCSetStCurr0[i];
    }
    for (i = 0; i < NumPicStCurr1; cIdx++, i++)
    {
        m_SliceHeader.m_RpsPOCCurrList0[cIdx] = RefPicPOCSetStCurr1[i];
    }
    for (i = 0; i < NumPicLtCurr; cIdx++, i++)
    {
        m_SliceHeader.m_RpsPOCCurrList0[cIdx] = RefPicPOCSetLtCurr[i];
    }
    if (pps->pps_curr_pic_ref_enabled_flag)
        m_SliceHeader.m_RpsPOCCurrList0[cIdx++] = m_SliceHeader.m_poc;

    int32_t rpsPOCCurrList1[MAX_NUM_REF_PICS + 1] = {};
    if (m_SliceHeader.slice_type == B_SLICE)
    {
        cIdx = 0;
        for (i = 0; i < NumPicStCurr1; cIdx++, i++)
        {
            rpsPOCCurrList1[cIdx] = RefPicPOCSetStCurr1[i];
        }
        for (i = 0; i < NumPicStCurr0; cIdx++, i++)
        {
            rpsPOCCurrList1[cIdx] = RefPicPOCSetStCurr0[i];
        }
        for (i = 0; i < NumPicLtCurr; cIdx++, i++)
        {
            rpsPOCCurrList1[cIdx] = RefPicPOCSetLtCurr[i];
        }
        if (pps->pps_curr_pic_ref_enabled_flag)
            rpsPOCCurrList1[cIdx++] = m_SliceHeader.m_poc;
    }

    RefPicListModification &refPicListModification = m_SliceHeader.m_RefPicListModification;

    for (rIdx = 0; rIdx <= m_SliceHeader.m_numRefIdx[0] - 1; rIdx++)
    {
        cIdx = refPicListModification.ref_pic_list_modification_flag_l0 ? refPicListModification.list_entry_l0[rIdx] : rIdx % numPicTotalCurr;
        m_SliceHeader.m_RefPOCList[0][rIdx] = m_SliceHeader.m_RpsPOCCurrList0[cIdx];
    }

    int32_t const numRpsCurrTempList0 = (std::max)(m_SliceHeader.m_numRefIdx[0], numPicTotalCurr);
    if (pps->pps_curr_pic_ref_enabled_flag &&
        !refPicListModification.ref_pic_list_modification_flag_l0 &&
        numRpsCurrTempList0 > m_SliceHeader.m_numRefIdx[0])
    {
        m_SliceHeader.m_RefPOCList[0][m_SliceHeader.m_numRefIdx[0] - 1] = m_SliceHeader.m_poc;
    }

    if (m_SliceHeader.slice_type == P_SLICE)
    {
        m_SliceHeader.m_numRefIdx[1] = 0;
    }
    else
    {
        for (rIdx = 0; rIdx <= m_SliceHeader.m_numRefIdx[1] - 1; rIdx++)
        {
            cIdx = refPicListModification.ref_pic_list_modification_flag_l1 ? refPicListModification.list_entry_l1[rIdx] : rIdx % numPicTotalCurr;
            m_SliceHeader.m_RefPOCList[1][rIdx] = rpsPOCCurrList1[cIdx];
        }
    }
}

// For dependent slice copy data from another slice
void H265Slice::CopyFromBaseSlice(const H265Slice * s)
{
    if (!s || !m_SliceHeader.dependent_slice_segment_flag)
        return;

    VM_ASSERT(s);
    m_iNumber = s->m_iNumber;

    const H265SliceHeader * slice = s->GetSliceHeader();

    m_SliceHeader.IdrPicFlag    = slice->IdrPicFlag;
    m_SliceHeader.m_poc         = slice->m_poc;
    m_SliceHeader.nal_unit_type = slice->nal_unit_type;
    m_SliceHeader.SliceQP       = slice->SliceQP;

    m_SliceHeader.slice_deblocking_filter_disabled_flag = slice->slice_deblocking_filter_disabled_flag;
    m_SliceHeader.deblocking_filter_override_flag       = slice->deblocking_filter_override_flag;
    m_SliceHeader.slice_beta_offset = slice->slice_beta_offset;
    m_SliceHeader.slice_tc_offset   = slice->slice_tc_offset;

    for (int32_t i = 0; i < 3; i++)
    {
        m_SliceHeader.m_numRefIdx[i]     = slice->m_numRefIdx[i];
    }

    m_SliceHeader.m_CheckLDC             = slice->m_CheckLDC;
    m_SliceHeader.slice_type             = slice->slice_type;
    m_SliceHeader.slice_qp_delta         = slice->slice_qp_delta;
    m_SliceHeader.slice_cb_qp_offset     = slice->slice_cb_qp_offset;
    m_SliceHeader.slice_cr_qp_offset     = slice->slice_cr_qp_offset;
    m_SliceHeader.slice_act_y_qp_offset  = slice->slice_act_y_qp_offset;
    m_SliceHeader.slice_act_cb_qp_offset = slice->slice_act_cb_qp_offset;
    m_SliceHeader.slice_act_cr_qp_offset = slice->slice_act_cr_qp_offset;

    m_SliceHeader.m_rps                   = slice->m_rps;
    m_SliceHeader.collocated_from_l0_flag = slice->collocated_from_l0_flag;
    m_SliceHeader.collocated_ref_idx      = slice->collocated_ref_idx;
    m_SliceHeader.nuh_temporal_id         = slice->nuh_temporal_id;

    for (int32_t e = 0; e < 2; e++)
    {
        for (int32_t n = 0; n < MAX_NUM_REF_PICS; n++)
        {
            MFX_INTERNAL_CPY(m_SliceHeader.pred_weight_table[e][n], slice->pred_weight_table[e][n], sizeof(wpScalingParam) * 3);
        }
    }

    m_SliceHeader.luma_log2_weight_denom   = slice->luma_log2_weight_denom;
    m_SliceHeader.chroma_log2_weight_denom = slice->chroma_log2_weight_denom;
    m_SliceHeader.slice_sao_luma_flag      = slice->slice_sao_luma_flag;
    m_SliceHeader.slice_sao_chroma_flag    = slice->slice_sao_chroma_flag;
    m_SliceHeader.cabac_init_flag          = slice->cabac_init_flag;

    m_SliceHeader.mvd_l1_zero_flag = slice->mvd_l1_zero_flag;
    m_SliceHeader.slice_loop_filter_across_slices_enabled_flag = slice->slice_loop_filter_across_slices_enabled_flag;
    m_SliceHeader.slice_temporal_mvp_enabled_flag = slice->slice_temporal_mvp_enabled_flag;
    m_SliceHeader.max_num_merge_cand  = slice->max_num_merge_cand;
    m_SliceHeader.use_integer_mv_flag = slice->use_integer_mv_flag;

    m_SliceHeader.cu_chroma_qp_offset_enabled_flag = slice->cu_chroma_qp_offset_enabled_flag;

    // Set the start of real slice, not slice segment
    m_SliceHeader.SliceCurStartCUAddr = slice->SliceCurStartCUAddr;

    m_SliceHeader.m_RefPicListModification = slice->m_RefPicListModification;

    m_SliceHeader.m_numPicStCurr0 = slice->m_numPicStCurr0;
    m_SliceHeader.m_numPicStCurr1 = slice->m_numPicStCurr1;
    m_SliceHeader.m_numPicLtCurr  = slice->m_numPicLtCurr;

    for (int32_t i = 0; i < MAX_NUM_REF_PICS + 1; i++)
    {
        m_SliceHeader.m_RpsPOCCurrList0[i] = slice->m_RpsPOCCurrList0[i];
    }
}

// Build reference lists from slice reference pic set. HEVC spec 8.3.2
UMC::Status H265Slice::UpdateReferenceList(H265DBPList *pDecoderFrameList, H265DecoderFrame* curr_ref)
{
    UMC::Status ps = UMC::UMC_OK;

    if (m_pCurrentFrame == nullptr)
        return UMC::UMC_ERR_NULL_PTR;

    const H265DecoderRefPicList* pH265DecRefPicList0 = m_pCurrentFrame->GetRefPicList(m_iNumber, 0);
    const H265DecoderRefPicList* pH265DecRefPicList1 = m_pCurrentFrame->GetRefPicList(m_iNumber, 1);
    if (pH265DecRefPicList0 == nullptr || pH265DecRefPicList1 == nullptr)
        return UMC::UMC_ERR_NULL_PTR;

    H265DecoderRefPicList::ReferenceInformation* pRefPicList0 = pH265DecRefPicList0->m_refPicList;
    H265DecoderRefPicList::ReferenceInformation* pRefPicList1 = pH265DecRefPicList1->m_refPicList;

    H265SliceHeader* header = GetSliceHeader();
    VM_ASSERT(header);

    if (header->slice_type == I_SLICE)
    {
        for (int32_t number = 0; number < 3; number++)
            header->m_numRefIdx[number] = 0;

        return UMC::UMC_OK;
    }

    H265DecoderFrame *RefPicSetStCurr0[16] = {};
    H265DecoderFrame *RefPicSetStCurr1[16] = {};
    H265DecoderFrame *RefPicSetLtCurr[16]  = {};
    uint32_t NumPicStCurr0 = 0;
    uint32_t NumPicStCurr1 = 0;
    uint32_t NumPicLtCurr  = 0;
    uint32_t i;

    for (i = 0; i < header->m_numPicStCurr0; i++)
    {
        int32_t poc = header->m_RpsPOCCurrList0[i];

        H265DecoderFrame *pFrm = pDecoderFrameList->findShortRefPic(poc);
        m_pCurrentFrame->AddReferenceFrame(pFrm);

        if (pFrm)
            pFrm->SetisLongTermRef(false);
        RefPicSetStCurr0[NumPicStCurr0] = pFrm;
        NumPicStCurr0++;
        if (!pFrm)
        {
            /* Reporting about missed reference */
            m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
            /* And because frame can not be decoded properly set flag "ERROR_FRAME_MAJOR" too*/
            m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_MAJOR);
        }
        // pcRefPic->setCheckLTMSBPresent(false);
    }

    for (i = header->m_numPicStCurr0; i < header->m_numPicStCurr0 + header->m_numPicStCurr1; i++)
    {
        int32_t poc = header->m_RpsPOCCurrList0[i];

        H265DecoderFrame *pFrm = pDecoderFrameList->findShortRefPic(poc);
        m_pCurrentFrame->AddReferenceFrame(pFrm);

        if (pFrm)
            pFrm->SetisLongTermRef(false);
        RefPicSetStCurr1[NumPicStCurr1] = pFrm;
        NumPicStCurr1++;
        if (!pFrm)
        {
            /* Reporting about missed reference */
            m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
            /* And because frame can not be decoded properly set flag "ERROR_FRAME_MAJOR" too*/
            m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_MAJOR);
        }
        // pcRefPic->setCheckLTMSBPresent(false);
    }

    for (i = getRPS()->getNumberOfNegativePictures() + getRPS()->getNumberOfPositivePictures();
        i < getRPS()->getNumberOfNegativePictures() + getRPS()->getNumberOfPositivePictures() + getRPS()->getNumberOfLongtermPictures(); i++)
    {
        if (getRPS()->getUsed(i))
        {
            int32_t poc = getRPS()->getPOC(i);

            H265DecoderFrame *pFrm = pDecoderFrameList->findLongTermRefPic(m_pCurrentFrame, poc, GetSeqParam()->log2_max_pic_order_cnt_lsb, !getRPS()->getCheckLTMSBPresent(i));

            if (!pFrm)
                continue;

            m_pCurrentFrame->AddReferenceFrame(pFrm);

            pFrm->SetisLongTermRef(true);
            RefPicSetLtCurr[NumPicLtCurr] = pFrm;
            NumPicLtCurr++;
            if (!pFrm)
            {
                /* Reporting about missed reference */
                m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
                /* And because frame can not be decoded properly set flag "ERROR_FRAME_MAJOR" too*/
                m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_MAJOR);
            }
        }
        // pFrm->setCheckLTMSBPresent(getRPS()->getCheckLTMSBPresent(i));
    }

    H265PicParamSet const* pps = GetPicParam();
    VM_ASSERT(pps);

    int32_t numPicTotalCurr = NumPicStCurr0 + NumPicStCurr1 + NumPicLtCurr;
    if (pps->pps_curr_pic_ref_enabled_flag)
        numPicTotalCurr++;

    // ref_pic_list_init
    H265DecoderFrame *refPicListTemp0[MAX_NUM_REF_PICS + 1] = {};
    H265DecoderFrame *refPicListTemp1[MAX_NUM_REF_PICS + 1] = {};

    if (header->nal_unit_type >= NAL_UT_CODED_SLICE_BLA_W_LP &&
        header->nal_unit_type <= NAL_UT_CODED_SLICE_CRA &&
        numPicTotalCurr != pps->pps_curr_pic_ref_enabled_flag)
    {
        //8.3.2: If the current picture is a BLA or CRA picture, the value of NumPicTotalCurr shall be equal to pps_curr_pic_ref_enabled_flag ...
        m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
        return UMC::UMC_OK;
    }
    else if (!numPicTotalCurr)
    {
        //8.3.2: ... Otherwise, when the current picture contains a P or B slice, the value of NumPicTotalCurr shall not be equal to 0
        m_pCurrentFrame->SetErrorFlagged(UMC::ERROR_FRAME_REFERENCE_FRAME);
        return UMC::UMC_OK;
    }

    int32_t cIdx = 0;
    for (i = 0; i < NumPicStCurr0; cIdx++, i++)
    {
        refPicListTemp0[cIdx] = RefPicSetStCurr0[i];
    }
    for (i = 0; i < NumPicStCurr1; cIdx++, i++)
    {
        refPicListTemp0[cIdx] = RefPicSetStCurr1[i];
    }
    for (i = 0; i < NumPicLtCurr; cIdx++, i++)
    {
        refPicListTemp0[cIdx] = RefPicSetLtCurr[i];
    }
    if (pps->pps_curr_pic_ref_enabled_flag)
        refPicListTemp0[cIdx++] = curr_ref;

    if (header->slice_type == B_SLICE)
    {
        cIdx = 0;
        for (i = 0; i < NumPicStCurr1; cIdx++, i++)
        {
            refPicListTemp1[cIdx] = RefPicSetStCurr1[i];
        }
        for (i = 0; i < NumPicStCurr0; cIdx++, i++)
        {
            refPicListTemp1[cIdx] = RefPicSetStCurr0[i];
        }
        for (i = 0; i < NumPicLtCurr; cIdx++, i++)
        {
            refPicListTemp1[cIdx] = RefPicSetLtCurr[i];
        }
        if (pps->pps_curr_pic_ref_enabled_flag)
            refPicListTemp1[cIdx++] = curr_ref;
    }

    RefPicListModification &refPicListModification = header->m_RefPicListModification;

    for (cIdx = 0; cIdx <= header->m_numRefIdx[0] - 1; cIdx ++)
    {
        bool isLong = refPicListModification.ref_pic_list_modification_flag_l0 ?
            (refPicListModification.list_entry_l0[cIdx] >= (NumPicStCurr0 + NumPicStCurr1))
            : ((uint32_t)(cIdx % numPicTotalCurr) >= (NumPicStCurr0 + NumPicStCurr1));

        pRefPicList0[cIdx].refFrame = refPicListModification.ref_pic_list_modification_flag_l0 ? refPicListTemp0[refPicListModification.list_entry_l0[cIdx]] : refPicListTemp0[cIdx % numPicTotalCurr];
        pRefPicList0[cIdx].isLongReference = isLong;
    }

    int32_t const numRpsCurrTempList0 = (std::max)(header->m_numRefIdx[0], numPicTotalCurr);
    if (pps->pps_curr_pic_ref_enabled_flag &&
        !refPicListModification.ref_pic_list_modification_flag_l0 &&
        numRpsCurrTempList0 > header->m_numRefIdx[0])
    {
        pRefPicList0[header->m_numRefIdx[0] - 1].refFrame = curr_ref;
        pRefPicList0[header->m_numRefIdx[0] - 1].isLongReference = true;
    }

    if (header->slice_type == P_SLICE)
    {
        header->m_numRefIdx[1] = 0;
    }
    else
    {
        for (cIdx = 0; cIdx <= header->m_numRefIdx[1] - 1; cIdx ++)
        {
            bool isLong = refPicListModification.ref_pic_list_modification_flag_l1 ?
                (refPicListModification.list_entry_l1[cIdx] >= (NumPicStCurr0 + NumPicStCurr1))
                : ((uint32_t)(cIdx % numPicTotalCurr) >= (NumPicStCurr0 + NumPicStCurr1));

            pRefPicList1[cIdx].refFrame = refPicListModification.ref_pic_list_modification_flag_l1 ? refPicListTemp1[refPicListModification.list_entry_l1[cIdx]] : refPicListTemp1[cIdx % numPicTotalCurr];
            pRefPicList1[cIdx].isLongReference = isLong;
        }
    }

    if (header->slice_type == B_SLICE && getNumRefIdx(REF_PIC_LIST_1) == 0)
    {
        int32_t iNumRefIdx = getNumRefIdx(REF_PIC_LIST_0);
        header->m_numRefIdx[REF_PIC_LIST_1] = iNumRefIdx;

        for (int32_t iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
        {
            pRefPicList1[iRefIdx] = pRefPicList0[iRefIdx];
        }
    }

    if (header->slice_type != I_SLICE)
    {
        bool bLowDelay = true;
        int32_t currPOC = header->m_poc;

        H265DecoderFrame *missedReference = 0;

        for (int32_t k = 0; !missedReference && k < numPicTotalCurr; k++)
        {
            missedReference = refPicListTemp0[k];
        }

        for (int32_t k = 0; k < getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; k++)
        {
            if (!pRefPicList0[k].refFrame)
            {
                pRefPicList0[k].refFrame = missedReference;
            }
        }

        for (int32_t k = 0; k < getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; k++)
        {
            if (pRefPicList0[k].refFrame && pRefPicList0[k].refFrame->PicOrderCnt() > currPOC)
            {
                bLowDelay = false;
            }
        }

        if (header->slice_type == B_SLICE)
        {
            for (int32_t k = 0; k < getNumRefIdx(REF_PIC_LIST_1); k++)
            {
                if (!pRefPicList1[k].refFrame)
                {
                    pRefPicList1[k].refFrame = missedReference;
                }
            }

            for (int32_t k = 0; k < getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; k++)
            {
                if (pRefPicList1[k].refFrame && pRefPicList1[k].refFrame->PicOrderCnt() > currPOC)
                {
                    bLowDelay = false;
                }
            }
        }

        m_SliceHeader.m_CheckLDC = bLowDelay;
    }

    return ps;
} // Status H265Slice::UpdateRefPicList(H265DBPList *pDecoderFrameList)

bool H265Slice::GetRapPicFlag() const
{
    return GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_IDR_W_RADL
        || GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_IDR_N_LP
        || GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_BLA_N_LP
        || GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_RADL
        || GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_BLA_W_LP
        || GetSliceHeader()->nal_unit_type == NAL_UT_CODED_SLICE_CRA;
}

// RPS data structure constructor
ReferencePictureSet::ReferencePictureSet()
{
  ::memset(this, 0, sizeof(*this));
}

// Bubble sort RPS by delta POCs placing negative values first, positive values second, increasing POS deltas
// Negative values are stored with biggest absolute value first. See HEVC spec 8.3.2.
void ReferencePictureSet::sortDeltaPOC()
{
    for (uint32_t j = 1; j < num_pics; j++)
    {
        int32_t deltaPOC = m_DeltaPOC[j];
        uint8_t Used = used_by_curr_pic_flag[j];
        for (int32_t k = j - 1; k >= 0; k--)
        {
            int32_t temp = m_DeltaPOC[k];
            if (deltaPOC < temp)
            {
                m_DeltaPOC[k + 1] = temp;
                used_by_curr_pic_flag[k + 1] = used_by_curr_pic_flag[k];
                m_DeltaPOC[k] = deltaPOC;
                used_by_curr_pic_flag[k] = Used;
            }
        }
    }
    int32_t NumNegPics = (int32_t) num_negative_pics;
    for (int32_t j = 0, k = NumNegPics - 1; j < NumNegPics >> 1; j++, k--)
    {
        int32_t deltaPOC = m_DeltaPOC[j];
        uint8_t Used = used_by_curr_pic_flag[j];
        m_DeltaPOC[j] = m_DeltaPOC[k];
        used_by_curr_pic_flag[j] = used_by_curr_pic_flag[k];
        m_DeltaPOC[k] = deltaPOC;
        used_by_curr_pic_flag[k] = Used;
    }
}

uint32_t ReferencePictureSet::getNumberOfUsedPictures() const
{
    uint32_t const total =
        std::accumulate(used_by_curr_pic_flag, used_by_curr_pic_flag + num_pics, 0);

    return total;
}

// RPS list data structure constructor
ReferencePictureSetList::ReferencePictureSetList()
    : m_NumberOfReferencePictureSets(0)
{
}

// Allocate RPS list data structure with a new number of RPS
void ReferencePictureSetList::allocate(unsigned NumberOfReferencePictureSets)
{
    if (m_NumberOfReferencePictureSets == NumberOfReferencePictureSets)
        return;

    m_NumberOfReferencePictureSets = NumberOfReferencePictureSets;
    referencePictureSet.resize(NumberOfReferencePictureSets);
}

} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
