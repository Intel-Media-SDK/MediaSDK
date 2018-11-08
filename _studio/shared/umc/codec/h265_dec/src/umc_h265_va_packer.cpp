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

#include "umc_h265_va_packer.h"
#include "umc_va_base.h"

#ifdef UMC_VA
#include "umc_h265_task_supplier.h"
#endif

#include "umc_h265_tables.h"
#include "umc_va_linux_protected.h"
#include "mfx_ext_buffers.h"

using namespace UMC;

namespace UMC_HEVC_DECODER
{
    int const s_quantTSDefault4x4[16] =
    {
      16,16,16,16,
      16,16,16,16,
      16,16,16,16,
      16,16,16,16
    };

    int const s_quantIntraDefault8x8[64] =
    {
      16,16,16,16,17,18,21,24,  // 10 10 10 10 11 12 15 18
      16,16,16,16,17,19,22,25,
      16,16,17,18,20,22,25,29,
      16,16,18,21,24,27,31,36,
      17,17,20,24,30,35,41,47,
      18,19,22,27,35,44,54,65,
      21,22,25,31,41,54,70,88,
      24,25,29,36,47,65,88,115
    };

    int const s_quantInterDefault8x8[64] =
    {
      16,16,16,16,17,18,20,24,
      16,16,16,17,18,20,24,25,
      16,16,17,18,20,24,25,28,
      16,17,18,20,24,25,28,33,
      17,18,20,24,25,28,33,41,
      18,20,24,25,28,33,41,54,
      20,24,25,28,33,41,54,71,
      24,25,28,33,41,54,71,91
    };

    //the tables used to restore original scan order of scaling lists (req. by drivers since ci-main-49045)
    uint16_t const* SL_tab_up_right[] =
    {
        ScanTableDiag4x4,
        g_sigLastScanCG32x32,
        g_sigLastScanCG32x32,
        g_sigLastScanCG32x32
    };

#if defined(MFX_ENABLE_CPLIB)
    extern Packer * CreatePackerCENC(VideoAccelerator*);
#endif

Packer * Packer::CreatePacker(VideoAccelerator * va)
{
    Packer * packer = 0;

#ifdef MFX_ENABLE_CPLIB
    if (va->GetProtectedVA() && IS_PROTECTION_CENC(va->GetProtectedVA()->GetProtected()))
        packer = CreatePackerCENC(va);
    else
#endif
    packer = new PackerVA(va);

    return packer;
}

Packer::Packer(VideoAccelerator * va)
    : m_va(va)
{
}

Packer::~Packer()
{
}

/****************************************************************************************************/
// VA linux packer implementation
/****************************************************************************************************/

void PackerVA::PackPicParams(const H265DecoderFrame *pCurrentFrame, TaskSupplier_H265 *supplier)
{
    H265DecoderFrameInfo const* pSliceInfo = pCurrentFrame->GetAU();
    if (!pSliceInfo)
        throw h265_exception(UMC_ERR_FAILED);

    auto pSlice = pSliceInfo->GetSlice(0);
    if (!pSlice)
        throw h265_exception(UMC_ERR_FAILED);

    H265SliceHeader const* sliceHeader =  pSlice->GetSliceHeader();
    H265SeqParamSet const* pSeqParamSet = pSlice->GetSeqParam();
    H265PicParamSet const* pPicParamSet = pSlice->GetPicParam();
    VAPictureParameterBufferHEVC *picParam = nullptr;
#if (MFX_VERSION >= 1027)
    VAPictureParameterBufferHEVCExtension* picParamExt = nullptr;
    VAPictureParameterBufferHEVCRext *picParamRext = nullptr;
#endif
    UMCVACompBuffer *picParamBuf = nullptr;

#if (MFX_VERSION >= 1027)
    if (m_va->m_Profile & VA_PROFILE_REXT)
    {
        picParamExt = (VAPictureParameterBufferHEVCExtension*)m_va->GetCompBuffer(VAPictureParameterBufferType, &picParamBuf, sizeof(VAPictureParameterBufferHEVCExtension));
        if (!picParamExt)
            throw h265_exception(UMC_ERR_FAILED);

        picParam = &picParamExt->base;
        if (!picParam)
            throw h265_exception(UMC_ERR_FAILED);

        picParamRext = &picParamExt->rext;
        if (!picParamRext)
            throw h265_exception(UMC_ERR_FAILED);

        picParamBuf->SetDataSize(sizeof(VAPictureParameterBufferHEVCExtension));
        memset(picParam, 0, sizeof(VAPictureParameterBufferHEVCExtension));

    }
    else
#endif
    {
        picParam = (VAPictureParameterBufferHEVC*)m_va->GetCompBuffer(VAPictureParameterBufferType, &picParamBuf, sizeof(VAPictureParameterBufferHEVC));
        if (!picParam)
            throw h265_exception(UMC_ERR_FAILED);
        picParamBuf->SetDataSize(sizeof(VAPictureParameterBufferHEVC));
        memset(picParam, 0, sizeof(VAPictureParameterBufferHEVC));
    }

    picParam->CurrPic.picture_id = m_va->GetSurfaceID(pCurrentFrame->GetFrameMID());
    picParam->CurrPic.pic_order_cnt = pCurrentFrame->m_PicOrderCnt;
    picParam->CurrPic.flags = 0;

    size_t count = 0;
    H265DBPList *dpb = supplier->GetDPBList();
    if (!dpb)
        throw h265_exception(UMC_ERR_FAILED);
    for(H265DecoderFrame* frame = dpb->head() ; frame && count < sizeof(picParam->ReferenceFrames)/sizeof(picParam->ReferenceFrames[0]) ; frame = frame->future())
    {
        if (frame == pCurrentFrame)
            continue;

        int refType = frame->isShortTermRef() ? SHORT_TERM_REFERENCE : (frame->isLongTermRef() ? LONG_TERM_REFERENCE : NO_REFERENCE);

        if (refType != NO_REFERENCE)
        {
            picParam->ReferenceFrames[count].pic_order_cnt = frame->m_PicOrderCnt;
            picParam->ReferenceFrames[count].picture_id = m_va->GetSurfaceID(frame->GetFrameMID());;
            picParam->ReferenceFrames[count].flags = refType == LONG_TERM_REFERENCE ? VA_PICTURE_HEVC_LONG_TERM_REFERENCE : 0;
            count++;
        }
    }

    for (size_t n = count; n < sizeof(picParam->ReferenceFrames)/sizeof(picParam->ReferenceFrames[0]); n++)
    {
        picParam->ReferenceFrames[n].picture_id = VA_INVALID_SURFACE;
        picParam->ReferenceFrames[n].flags = VA_PICTURE_HEVC_INVALID;
    }

    auto rps = pSlice->getRPS();
    uint32_t index;
    int32_t pocList[3*8];
    int32_t numRefPicSetStCurrBefore = 0,
        numRefPicSetStCurrAfter  = 0,
        numRefPicSetLtCurr       = 0;
    for(index = 0; index < rps->getNumberOfNegativePictures(); index++)
            pocList[numRefPicSetStCurrBefore++] = picParam->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);

    for(; index < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures(); index++)
            pocList[numRefPicSetStCurrBefore + numRefPicSetStCurrAfter++] = picParam->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);

    for(; index < rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures() + rps->getNumberOfLongtermPictures(); index++)
    {
        int32_t poc = rps->getPOC(index);
        H265DecoderFrame *pFrm = supplier->GetDPBList()->findLongTermRefPic(pCurrentFrame, poc, pSeqParamSet->log2_max_pic_order_cnt_lsb, !rps->getCheckLTMSBPresent(index));

        if (pFrm)
        {
            pocList[numRefPicSetStCurrBefore + numRefPicSetStCurrAfter + numRefPicSetLtCurr++] = pFrm->PicOrderCnt();
        }
        else
        {
            pocList[numRefPicSetStCurrBefore + numRefPicSetStCurrAfter + numRefPicSetLtCurr++] = picParam->CurrPic.pic_order_cnt + rps->getDeltaPOC(index);
        }
    }

    for(int32_t n=0 ; n < numRefPicSetStCurrBefore + numRefPicSetStCurrAfter + numRefPicSetLtCurr ; n++)
    {
        if (!rps->getUsed(n))
            continue;

        for(size_t k=0;k < count;k++)
        {
            if(pocList[n] == picParam->ReferenceFrames[k].pic_order_cnt)
            {
                if(n < numRefPicSetStCurrBefore)
                    picParam->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE;
                else if(n < numRefPicSetStCurrBefore + numRefPicSetStCurrAfter)
                    picParam->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_ST_CURR_AFTER;
                else if(n < numRefPicSetStCurrBefore + numRefPicSetStCurrAfter + numRefPicSetLtCurr)
                    picParam->ReferenceFrames[k].flags |= VA_PICTURE_HEVC_RPS_LT_CURR;
            }
        }
    }

    picParam->pic_width_in_luma_samples = (uint16_t)pSeqParamSet->pic_width_in_luma_samples;
    picParam->pic_height_in_luma_samples = (uint16_t)pSeqParamSet->pic_height_in_luma_samples;

    picParam->pic_fields.bits.chroma_format_idc = pSeqParamSet->chroma_format_idc;
    picParam->pic_fields.bits.separate_colour_plane_flag = pSeqParamSet->separate_colour_plane_flag;

    picParam->pic_fields.bits.pcm_enabled_flag = pSeqParamSet->pcm_enabled_flag;
    picParam->pic_fields.bits.scaling_list_enabled_flag = pSeqParamSet->scaling_list_enabled_flag;
    picParam->pic_fields.bits.transform_skip_enabled_flag = pPicParamSet->transform_skip_enabled_flag;
    picParam->pic_fields.bits.amp_enabled_flag = pSeqParamSet->amp_enabled_flag;
    picParam->pic_fields.bits.strong_intra_smoothing_enabled_flag = pSeqParamSet->sps_strong_intra_smoothing_enabled_flag;

    picParam->pic_fields.bits.sign_data_hiding_enabled_flag = pPicParamSet->sign_data_hiding_enabled_flag;
    picParam->pic_fields.bits.constrained_intra_pred_flag = pPicParamSet->constrained_intra_pred_flag;
    picParam->pic_fields.bits.cu_qp_delta_enabled_flag = pPicParamSet->cu_qp_delta_enabled_flag;
    picParam->pic_fields.bits.weighted_pred_flag = pPicParamSet->weighted_pred_flag;
    picParam->pic_fields.bits.weighted_bipred_flag = pPicParamSet->weighted_bipred_flag;

    picParam->pic_fields.bits.transquant_bypass_enabled_flag = pPicParamSet->transquant_bypass_enabled_flag;
    picParam->pic_fields.bits.tiles_enabled_flag = pPicParamSet->tiles_enabled_flag;
    picParam->pic_fields.bits.entropy_coding_sync_enabled_flag = pPicParamSet->entropy_coding_sync_enabled_flag;
    picParam->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag = pPicParamSet->pps_loop_filter_across_slices_enabled_flag;
    picParam->pic_fields.bits.loop_filter_across_tiles_enabled_flag = pPicParamSet->loop_filter_across_tiles_enabled_flag;

    picParam->pic_fields.bits.pcm_loop_filter_disabled_flag = pSeqParamSet->pcm_loop_filter_disabled_flag;
    picParam->pic_fields.bits.NoPicReorderingFlag = pSeqParamSet->sps_max_num_reorder_pics[pSlice->GetSliceHeader()->nuh_temporal_id] == 0 ? 1 : 0;
    picParam->pic_fields.bits.NoBiPredFlag = 0;

    picParam->sps_max_dec_pic_buffering_minus1 = (uint8_t)(pSeqParamSet->sps_max_dec_pic_buffering[pSlice->GetSliceHeader()->nuh_temporal_id] - 1);
    picParam->bit_depth_luma_minus8 = (uint8_t)(pSeqParamSet->bit_depth_luma - 8);
    picParam->bit_depth_chroma_minus8 = (uint8_t)(pSeqParamSet->bit_depth_chroma - 8);
    picParam->pcm_sample_bit_depth_luma_minus1 = (uint8_t)(pSeqParamSet->pcm_sample_bit_depth_luma - 1);
    picParam->pcm_sample_bit_depth_chroma_minus1 = (uint8_t)(pSeqParamSet->pcm_sample_bit_depth_chroma - 1);
    picParam->log2_min_luma_coding_block_size_minus3 = (uint8_t)(pSeqParamSet->log2_min_luma_coding_block_size- 3);
    picParam->log2_diff_max_min_luma_coding_block_size = (uint8_t)(pSeqParamSet->log2_max_luma_coding_block_size - pSeqParamSet->log2_min_luma_coding_block_size);
    picParam->log2_min_transform_block_size_minus2 = (uint8_t)(pSeqParamSet->log2_min_transform_block_size - 2);
    picParam->log2_diff_max_min_transform_block_size = (uint8_t)(pSeqParamSet->log2_max_transform_block_size - pSeqParamSet->log2_min_transform_block_size);
    picParam->log2_min_pcm_luma_coding_block_size_minus3 = (uint8_t)(pSeqParamSet->log2_min_pcm_luma_coding_block_size - 3);
    picParam->log2_diff_max_min_pcm_luma_coding_block_size = (uint8_t)(pSeqParamSet->log2_max_pcm_luma_coding_block_size - pSeqParamSet->log2_min_pcm_luma_coding_block_size);
    picParam->max_transform_hierarchy_depth_intra = (uint8_t)pSeqParamSet->max_transform_hierarchy_depth_intra;
    picParam->max_transform_hierarchy_depth_inter = (uint8_t)pSeqParamSet->max_transform_hierarchy_depth_inter;
    picParam->init_qp_minus26 = pPicParamSet->init_qp - 26;
    picParam->diff_cu_qp_delta_depth = (uint8_t)pPicParamSet->diff_cu_qp_delta_depth;
    picParam->pps_cb_qp_offset = (uint8_t)pPicParamSet->pps_cb_qp_offset;
    picParam->pps_cr_qp_offset = (uint8_t)pPicParamSet->pps_cr_qp_offset;
    picParam->log2_parallel_merge_level_minus2 = (uint8_t)(pPicParamSet->log2_parallel_merge_level - 2);

    if (pPicParamSet->tiles_enabled_flag)
    {
        picParam->num_tile_columns_minus1 = std::min<uint8_t>(sizeof(picParam->column_width_minus1)/sizeof(picParam->column_width_minus1[0]) - 1, pPicParamSet->num_tile_columns - 1);
        picParam->num_tile_rows_minus1    = std::min<uint8_t>(sizeof(picParam->row_height_minus1)  /sizeof(picParam->row_height_minus1[0])   - 1, pPicParamSet->num_tile_rows    - 1);

        for (uint32_t i = 0; i <= picParam->num_tile_columns_minus1; i++)
            picParam->column_width_minus1[i] = (uint16_t)(pPicParamSet->column_width[i] - 1);

        for (uint32_t i = 0; i <= picParam->num_tile_rows_minus1; i++)
            picParam->row_height_minus1[i] = (uint16_t)(pPicParamSet->row_height[i] - 1);
    }

    picParam->slice_parsing_fields.bits.lists_modification_present_flag = pPicParamSet->lists_modification_present_flag;
    picParam->slice_parsing_fields.bits.long_term_ref_pics_present_flag = pSeqParamSet->long_term_ref_pics_present_flag;
    picParam->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag = pSeqParamSet->sps_temporal_mvp_enabled_flag;
    picParam->slice_parsing_fields.bits.cabac_init_present_flag = pPicParamSet->cabac_init_present_flag;
    picParam->slice_parsing_fields.bits.output_flag_present_flag = pPicParamSet->output_flag_present_flag;
    picParam->slice_parsing_fields.bits.dependent_slice_segments_enabled_flag = pPicParamSet->dependent_slice_segments_enabled_flag;
    picParam->slice_parsing_fields.bits.pps_slice_chroma_qp_offsets_present_flag = pPicParamSet->pps_slice_chroma_qp_offsets_present_flag;
    picParam->slice_parsing_fields.bits.sample_adaptive_offset_enabled_flag = pSeqParamSet->sample_adaptive_offset_enabled_flag;
    picParam->slice_parsing_fields.bits.deblocking_filter_override_enabled_flag = pPicParamSet->deblocking_filter_override_enabled_flag;
    picParam->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag = pPicParamSet->pps_deblocking_filter_disabled_flag;
    picParam->slice_parsing_fields.bits.slice_segment_header_extension_present_flag = pPicParamSet->slice_segment_header_extension_present_flag;

    picParam->slice_parsing_fields.bits.RapPicFlag = (sliceHeader->nal_unit_type >= NAL_UT_CODED_SLICE_BLA_W_LP && sliceHeader->nal_unit_type <= NAL_UT_CODED_SLICE_CRA) ? 1 : 0;
    picParam->slice_parsing_fields.bits.IdrPicFlag = sliceHeader->IdrPicFlag;
    picParam->slice_parsing_fields.bits.IntraPicFlag = pSliceInfo->IsIntraAU() ? 1 : 0;

    picParam->log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)(pSeqParamSet->log2_max_pic_order_cnt_lsb - 4);
    picParam->num_short_term_ref_pic_sets = (uint8_t)pSeqParamSet->getRPSList()->getNumberOfReferencePictureSets();
    picParam->num_long_term_ref_pic_sps = (uint8_t)pSeqParamSet->num_long_term_ref_pics_sps;
    picParam->num_ref_idx_l0_default_active_minus1 = (uint8_t)(pPicParamSet->num_ref_idx_l0_default_active - 1);
    picParam->num_ref_idx_l1_default_active_minus1 = (uint8_t)(pPicParamSet->num_ref_idx_l1_default_active - 1);
    picParam->pps_beta_offset_div2 = (int8_t)(pPicParamSet->pps_beta_offset >> 1);
    picParam->pps_tc_offset_div2 = (int8_t)(pPicParamSet->pps_tc_offset >> 1);
    picParam->num_extra_slice_header_bits = (uint8_t)pPicParamSet->num_extra_slice_header_bits;

    picParam->st_rps_bits = pSlice->GetSliceHeader()->wNumBitsForShortTermRPSInSlice;

#if (MFX_VERSION >= 1027)
    /* RExt structures */
    if (nullptr != picParamRext)
    {
        picParamRext->range_extension_pic_fields.bits.transform_skip_rotation_enabled_flag        = pSeqParamSet->transform_skip_rotation_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.transform_skip_context_enabled_flag         = pSeqParamSet->transform_skip_context_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.implicit_rdpcm_enabled_flag                 = pSeqParamSet->implicit_residual_dpcm_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.explicit_rdpcm_enabled_flag                 = pSeqParamSet->explicit_residual_dpcm_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.extended_precision_processing_flag          = pSeqParamSet->extended_precision_processing_flag;
        picParamRext->range_extension_pic_fields.bits.intra_smoothing_disabled_flag               = pSeqParamSet->intra_smoothing_disabled_flag;
        picParamRext->range_extension_pic_fields.bits.high_precision_offsets_enabled_flag         = pSeqParamSet->high_precision_offsets_enabled_flag;;
        picParamRext->range_extension_pic_fields.bits.persistent_rice_adaptation_enabled_flag     = pSeqParamSet->fast_rice_adaptation_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.cabac_bypass_alignment_enabled_flag         = pPicParamSet->cabac_init_present_flag;
        picParamRext->range_extension_pic_fields.bits.cross_component_prediction_enabled_flag     = pPicParamSet->cross_component_prediction_enabled_flag;
        picParamRext->range_extension_pic_fields.bits.chroma_qp_offset_list_enabled_flag          = pPicParamSet->chroma_qp_offset_list_enabled_flag;

        picParamRext->diff_cu_chroma_qp_offset_depth = (uint8_t)pPicParamSet->diff_cu_chroma_qp_offset_depth;
        picParamRext->chroma_qp_offset_list_len_minus1 = pPicParamSet->chroma_qp_offset_list_enabled_flag ? (uint8_t)pPicParamSet->chroma_qp_offset_list_len - 1 : 0;
        picParamRext->log2_sao_offset_scale_luma = (uint8_t)pPicParamSet->log2_sao_offset_scale_luma;
        picParamRext->log2_sao_offset_scale_chroma = (uint8_t)pPicParamSet->log2_sao_offset_scale_chroma;
        picParamRext->log2_max_transform_skip_block_size_minus2 = pPicParamSet->pps_range_extensions_flag && pPicParamSet->transform_skip_enabled_flag ? (uint8_t)pPicParamSet->log2_max_transform_skip_block_size_minus2 : 0;

        for (uint32_t i = 0; i < pPicParamSet->chroma_qp_offset_list_len; i++)
        {
            picParamRext->cb_qp_offset_list[i] = (int8_t)pPicParamSet->cb_qp_offset_list[i + 1];
            picParamRext->cr_qp_offset_list[i] = (int8_t)pPicParamSet->cr_qp_offset_list[i + 1];
        }
    } // if (nullptr != picParamRext)
#endif // #if (MFX_VERSION >= 1027)
}

void PackerVA::CreateSliceParamBuffer(H265DecoderFrameInfo const* sliceInfo)
{
    int32_t count = sliceInfo->GetSliceCount();

    UMCVACompBuffer *pSliceParamBuf;
    size_t sizeOfStruct = m_va->IsLongSliceControl() ? sizeof(VASliceParameterBufferHEVC) : sizeof(VASliceParameterBufferBase);
#if (MFX_VERSION >= 1027)
    if (m_va->m_Profile & VA_PROFILE_REXT) /* RExt cases*/
    {
        sizeOfStruct = m_va->IsLongSliceControl() ? sizeof(VASliceParameterBufferHEVCExtension) : sizeof(VASliceParameterBufferBase);
    }
#endif
    m_va->GetCompBuffer(VASliceParameterBufferType, &pSliceParamBuf, sizeOfStruct*(count));
    if (!pSliceParamBuf)
        throw h265_exception(UMC_ERR_FAILED);

    pSliceParamBuf->SetNumOfItem(count);
}

void PackerVA::CreateSliceDataBuffer(H265DecoderFrameInfo const* sliceInfo)
{
    int32_t count = sliceInfo->GetSliceCount();

    int32_t size = 0;
    int32_t AlignedNalUnitSize = 0;

    for (int32_t i = 0; i < count; i++)
    {
        H265Slice  * pSlice = sliceInfo->GetSlice(i);

        uint8_t *pNalUnit; //ptr to first byte of start code
        uint32_t NalUnitSize; // size of NAL unit in byte
        H265HeadersBitstream *pBitstream = pSlice->GetBitStream();

        pBitstream->GetOrg((uint32_t**)&pNalUnit, &NalUnitSize);
        size += NalUnitSize + 3;
    }

    AlignedNalUnitSize = mfx::align2_value(size, 128);

    UMCVACompBuffer* compBuf;
    m_va->GetCompBuffer(VASliceDataBufferType, &compBuf, AlignedNalUnitSize);
    if (!compBuf)
        throw h265_exception(UMC_ERR_FAILED);

    memset((uint8_t*)compBuf->GetPtr() + size, 0, AlignedNalUnitSize - size);

    compBuf->SetDataSize(0);
}

void PackerVA::PackSliceParams(H265Slice const* pSlice, size_t sliceNum, bool isLastSlice)
{
    static uint8_t start_code_prefix[] = {0, 0, 1};

    H265DecoderFrame *pCurrentFrame = pSlice->GetCurrentFrame();
    const H265SliceHeader *sliceHeader = pSlice->GetSliceHeader();
    const H265PicParamSet *pPicParamSet = pSlice->GetPicParam();

    VAPictureParameterBufferHEVC* picParams = nullptr;
    VASliceParameterBufferHEVC* sliceParams = nullptr;

#if (MFX_VERSION >= 1027)
    VAPictureParameterBufferHEVCExtension* picParamsExt = nullptr;
    VASliceParameterBufferHEVCExtension* sliceParamsExt = nullptr;
    VASliceParameterBufferHEVCRext* sliceParamsRExt = nullptr;
#endif

    UMCVACompBuffer* compBuf = nullptr;

#if (MFX_VERSION >= 1027)
    if (m_va->m_Profile & VA_PROFILE_REXT)
    {
        picParamsExt = (VAPictureParameterBufferHEVCExtension*)m_va->GetCompBuffer(VAPictureParameterBufferType);
        if (!picParamsExt)
            throw h265_exception(UMC_ERR_FAILED);

        sliceParamsExt = (VASliceParameterBufferHEVCExtension*)m_va->GetCompBuffer(VASliceParameterBufferType, &compBuf);

        if (!sliceParamsExt)
            throw h265_exception(UMC_ERR_FAILED);

        if (m_va->IsLongSliceControl())
        {
            sliceParamsExt += sliceNum;
            memset(sliceParamsExt, 0, sizeof(VASliceParameterBufferHEVCExtension));
        }
        else
        {
            sliceParamsExt = (VASliceParameterBufferHEVCExtension*)((VASliceParameterBufferBase*)sliceParamsExt + sliceNum);
            memset(sliceParamsExt, 0, sizeof(VASliceParameterBufferHEVCExtension));
        }

        picParams = &picParamsExt->base;
        if (!picParams)
            throw h265_exception(UMC_ERR_FAILED);

        sliceParams = &sliceParamsExt->base;
        if (!sliceParams)
            throw h265_exception(UMC_ERR_FAILED);

        sliceParamsRExt = &sliceParamsExt->rext;
        if (!sliceParamsRExt)
            throw h265_exception(UMC_ERR_FAILED);
    }
    else
#endif
    {
        picParams = (VAPictureParameterBufferHEVC*)m_va->GetCompBuffer(VAPictureParameterBufferType);
        if (!picParams)
            throw h265_exception(UMC_ERR_FAILED);

        sliceParams = (VASliceParameterBufferHEVC*)m_va->GetCompBuffer(VASliceParameterBufferType, &compBuf);
        if (!sliceParams)
            throw h265_exception(UMC_ERR_FAILED);

        if (m_va->IsLongSliceControl())
        {
            sliceParams += sliceNum;
            memset(sliceParams, 0, sizeof(VASliceParameterBufferHEVC));
        }
        else
        {
            sliceParams = (VASliceParameterBufferHEVC*)((VASliceParameterBufferBase*)sliceParams + sliceNum);
            memset(sliceParams, 0, sizeof(VASliceParameterBufferHEVC));
        }
    }

    uint32_t  rawDataSize = 0;
    const void*   rawDataPtr = 0;

    pSlice->m_BitStream.GetOrg((uint32_t**)&rawDataPtr, &rawDataSize);

    sliceParams->slice_data_size = rawDataSize + sizeof(start_code_prefix);
    sliceParams->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;//chopping == CHOPPING_NONE ? VA_SLICE_DATA_FLAG_ALL : VA_SLICE_DATA_FLAG_END;;

    uint8_t *sliceDataBuf = (uint8_t*)m_va->GetCompBuffer(VASliceDataBufferType, &compBuf);
    if (!sliceDataBuf)
        throw h265_exception(UMC_ERR_FAILED);

    sliceParams->slice_data_offset = compBuf->GetDataSize();
    sliceDataBuf += sliceParams->slice_data_offset;
    MFX_INTERNAL_CPY(sliceDataBuf, start_code_prefix, sizeof(start_code_prefix));
    MFX_INTERNAL_CPY(sliceDataBuf + sizeof(start_code_prefix), rawDataPtr, rawDataSize);
    compBuf->SetDataSize(sliceParams->slice_data_offset + sliceParams->slice_data_size);

    if (!m_va->IsLongSliceControl())
        return;

    sliceParams->slice_data_byte_offset = pSlice->m_BitStream.BytesDecoded() + sizeof(start_code_prefix);
    sliceParams->slice_segment_address = sliceHeader->slice_segment_address;

    for(int32_t iDir = 0; iDir < 2; iDir++)
    {
        const H265DecoderRefPicList::ReferenceInformation* pRefPicList = pCurrentFrame->GetRefPicList(pSlice->GetSliceNum(), iDir)->m_refPicList;

        EnumRefPicList eRefPicList = ( iDir == 1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
        int32_t const num_active_ref = pSlice->getNumRefIdx(eRefPicList);
        int32_t const max_num_ref =
            sizeof(picParams->ReferenceFrames) / sizeof(picParams->ReferenceFrames[0]);

        int32_t index = 0;
        for (int32_t i = 0; i < num_active_ref; i++)
        {
            const H265DecoderRefPicList::ReferenceInformation &frameInfo = pRefPicList[i];
            if (!frameInfo.refFrame)
                break;
            else
            {
                bool isFound = false;
                for (uint8_t j = 0; j < max_num_ref; j++)
                {
                    //VASurfaceID
                    if (picParams->ReferenceFrames[j].picture_id == (VASurfaceID)m_va->GetSurfaceID(frameInfo.refFrame->GetFrameMID()))
                    {
                        sliceParams->RefPicList[iDir][index] = j;
                        index++;
                        isFound = true;
                        break;
                    }
                }

                VM_ASSERT(isFound);
            }
        }

        for (int32_t i = index; i < max_num_ref; ++i)
            sliceParams->RefPicList[iDir][i] = 0xff;

        //fill gaps between required active references and actual 'Before/After' references we have in DPB
        //NOTE: try to use only 'Foll's
        for (int32_t i = index; i < num_active_ref; ++i)
        {
            int32_t j = 0;
            //try to find 'Foll' reference
            for (; j < max_num_ref; ++j)
                if (picParams->ReferenceFrames[j].flags == 0 && picParams->ReferenceFrames[j].picture_id != VA_INVALID_SURFACE)
                    break;

            if (j == max_num_ref)
                //no 'Foll' reference found, can't do amymore
                break;

            //make 'Foll' to be 'Before/After' reference
            picParams->ReferenceFrames[j].flags |=
                eRefPicList == REF_PIC_LIST_0 ? VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE : VA_PICTURE_HEVC_RPS_ST_CURR_AFTER;
        }
    }

    sliceParams->LongSliceFlags.fields.LastSliceOfPic = isLastSlice ? 1 : 0;
    // the first slice can't be a dependent slice
    sliceParams->LongSliceFlags.fields.dependent_slice_segment_flag = sliceHeader->dependent_slice_segment_flag;
    sliceParams->LongSliceFlags.fields.slice_type = sliceHeader->slice_type;
    sliceParams->LongSliceFlags.fields.color_plane_id = sliceHeader->colour_plane_id;
    sliceParams->LongSliceFlags.fields.slice_sao_luma_flag = sliceHeader->slice_sao_luma_flag;
    sliceParams->LongSliceFlags.fields.slice_sao_chroma_flag = sliceHeader->slice_sao_chroma_flag;
    sliceParams->LongSliceFlags.fields.mvd_l1_zero_flag = sliceHeader->mvd_l1_zero_flag;
    sliceParams->LongSliceFlags.fields.cabac_init_flag = sliceHeader->cabac_init_flag;
    sliceParams->LongSliceFlags.fields.slice_temporal_mvp_enabled_flag = sliceHeader->slice_temporal_mvp_enabled_flag;
    sliceParams->LongSliceFlags.fields.slice_deblocking_filter_disabled_flag = sliceHeader->slice_deblocking_filter_disabled_flag;
    sliceParams->LongSliceFlags.fields.collocated_from_l0_flag = sliceHeader->collocated_from_l0_flag;
    sliceParams->LongSliceFlags.fields.slice_loop_filter_across_slices_enabled_flag = sliceHeader->slice_loop_filter_across_slices_enabled_flag;

    sliceParams->collocated_ref_idx = (uint8_t)((sliceHeader->slice_type != I_SLICE) ?  sliceHeader->collocated_ref_idx : -1);
    sliceParams->num_ref_idx_l0_active_minus1 = (uint8_t)(pSlice->getNumRefIdx(REF_PIC_LIST_0) - 1);
    sliceParams->num_ref_idx_l1_active_minus1 = (uint8_t)(pSlice->getNumRefIdx(REF_PIC_LIST_1) - 1);
    sliceParams->slice_qp_delta = (int8_t)(sliceHeader->SliceQP - pPicParamSet->init_qp);
    sliceParams->slice_cb_qp_offset = (int8_t)sliceHeader->slice_cb_qp_offset;
    sliceParams->slice_cr_qp_offset = (int8_t)sliceHeader->slice_cr_qp_offset;
    sliceParams->slice_beta_offset_div2 = (int8_t)(sliceHeader->slice_beta_offset >> 1);
    sliceParams->slice_tc_offset_div2 = (int8_t)(sliceHeader->slice_tc_offset >> 1);

    sliceParams->luma_log2_weight_denom = (uint8_t)sliceHeader->luma_log2_weight_denom;
    sliceParams->delta_chroma_log2_weight_denom = (uint8_t)(sliceHeader->chroma_log2_weight_denom - sliceHeader->luma_log2_weight_denom);

    for (int32_t l = 0; l < 2; l++)
    {
        const wpScalingParam *wp;
        EnumRefPicList eRefPicList = ( l == 1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
        for (int32_t iRefIdx = 0; iRefIdx < pSlice->getNumRefIdx(eRefPicList); iRefIdx++)
        {
            wp = sliceHeader->pred_weight_table[eRefPicList][iRefIdx];

            if (eRefPicList == REF_PIC_LIST_0)
            {
                sliceParams->luma_offset_l0[iRefIdx]       = (int8_t)wp[0].offset;
                sliceParams->delta_luma_weight_l0[iRefIdx] = (int8_t)wp[0].delta_weight;
                for(int chroma=0;chroma < 2;chroma++)
                {
                    sliceParams->delta_chroma_weight_l0[iRefIdx][chroma] = (int8_t)wp[1 + chroma].delta_weight;
                    sliceParams->ChromaOffsetL0        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                }
            }
            else
            {
                sliceParams->luma_offset_l1[iRefIdx]       = (int8_t)wp[0].offset;
                sliceParams->delta_luma_weight_l1[iRefIdx] = (int8_t)wp[0].delta_weight;
                for(int chroma=0;chroma < 2;chroma++)
                {
                    sliceParams->delta_chroma_weight_l1[iRefIdx][chroma] = (int8_t)wp[1 + chroma].delta_weight;
                    sliceParams->ChromaOffsetL1        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                }
            }
        }
    }

    sliceParams->five_minus_max_num_merge_cand = (uint8_t)(5 - sliceHeader->max_num_merge_cand);

#if (MFX_VERSION >= 1027)
    /* RExt */
    if (nullptr != sliceParamsRExt)
    {
        for (int32_t l = 0; l < 2; l++)
        {
            const wpScalingParam *wp;
            EnumRefPicList eRefPicList = ( l == 1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
            for (int32_t iRefIdx = 0; iRefIdx < pSlice->getNumRefIdx(eRefPicList); iRefIdx++)
            {
                wp = sliceHeader->pred_weight_table[eRefPicList][iRefIdx];

                if (eRefPicList == REF_PIC_LIST_0)
                {
                    sliceParamsRExt->luma_offset_l0[iRefIdx]   = (int8_t)wp[0].offset;
                    for(int chroma=0;chroma < 2;chroma++)
                        sliceParamsRExt->ChromaOffsetL0        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                }
                else
                {
                    sliceParamsRExt->luma_offset_l1[iRefIdx]       = (int8_t)wp[0].offset;
                    for(int chroma=0;chroma < 2;chroma++)
                        sliceParamsRExt->ChromaOffsetL1        [iRefIdx][chroma] = (int8_t)wp[1 + chroma].offset;
                }
            }
        }

        sliceParamsRExt->slice_ext_flags.bits.cu_chroma_qp_offset_enabled_flag = sliceHeader->cu_chroma_qp_offset_enabled_flag;
        sliceParamsRExt->slice_ext_flags.bits.use_integer_mv_flag    = (uint8_t)sliceHeader->use_integer_mv_flag;

        sliceParamsRExt->slice_act_y_qp_offset  = (int8_t)sliceHeader->slice_act_y_qp_offset;
        sliceParamsRExt->slice_act_cb_qp_offset = (int8_t)sliceHeader->slice_act_cb_qp_offset;
        sliceParamsRExt->slice_act_cr_qp_offset = (int8_t)sliceHeader->slice_act_cr_qp_offset;
    }
#endif // #if (MFX_VERSION >= 1027)
}

void PackerVA::PackQmatrix(const H265Slice *pSlice)
{
    UMCVACompBuffer *quantBuf;
    VAIQMatrixBufferHEVC* qmatrix = (VAIQMatrixBufferHEVC*)m_va->GetCompBuffer(VAIQMatrixBufferType, &quantBuf, sizeof(VAIQMatrixBufferHEVC));
    if (!qmatrix)
        throw h265_exception(UMC_ERR_FAILED);
    quantBuf->SetDataSize(sizeof(VAIQMatrixBufferHEVC));

    const H265ScalingList *scalingList = 0;

    if (pSlice->GetPicParam()->pps_scaling_list_data_present_flag)
    {
        scalingList = pSlice->GetPicParam()->getScalingList();
    }
    else if (pSlice->GetSeqParam()->sps_scaling_list_data_present_flag)
    {
        scalingList = pSlice->GetSeqParam()->getScalingList();
    }
    else
    {
        // TODO: build default scaling list in target buffer location
        static bool doInit = true;
        static H265ScalingList sl;

        if(doInit)
        {
            for(uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
            {
                for(uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
                {
                    const int *src = getDefaultScalingList(sizeId, listId);
                          int *dst = sl.getScalingListAddress(sizeId, listId);
                    int count = std::min(MAX_MATRIX_COEF_NUM, (int32_t)g_scalingListSize[sizeId]);
                    ::MFX_INTERNAL_CPY(dst, src, sizeof(int32_t) * count);
                    sl.setScalingListDC(sizeId, listId, SCALING_LIST_DC);
                }
            }
            doInit = false;
        }

        scalingList = &sl;
    }

    initQMatrix<16>(scalingList, SCALING_LIST_4x4,   qmatrix->ScalingList4x4);    // 4x4
    initQMatrix<64>(scalingList, SCALING_LIST_8x8,   qmatrix->ScalingList8x8);    // 8x8
    initQMatrix<64>(scalingList, SCALING_LIST_16x16, qmatrix->ScalingList16x16);    // 16x16
    initQMatrix(scalingList, SCALING_LIST_32x32, qmatrix->ScalingList32x32);    // 32x32

    for(int sizeId = SCALING_LIST_16x16; sizeId <= SCALING_LIST_32x32; sizeId++)
    {
        for(unsigned listId = 0; listId <  g_scalingListNum[sizeId]; listId++)
        {
            if(sizeId == SCALING_LIST_16x16)
                qmatrix->ScalingListDC16x16[listId] = (uint8_t)scalingList->getScalingListDC(sizeId, listId);
            else if(sizeId == SCALING_LIST_32x32)
                qmatrix->ScalingListDC32x32[listId] = (uint8_t)scalingList->getScalingListDC(sizeId, listId);
        }
    }
}

void PackerVA::PackAU(const H265DecoderFrame *frame, TaskSupplier_H265 * supplier)
{
    if (!frame || !supplier)
        throw h265_exception(UMC_ERR_NULL_PTR);

    PackPicParams(frame, supplier);

    H265DecoderFrameInfo const* sliceInfo = frame->GetAU();
    if (!sliceInfo)
        throw h265_exception(UMC_ERR_FAILED);

    auto pSlice = sliceInfo->GetSlice(0);
    if (!pSlice)
        throw h265_exception(UMC_ERR_FAILED);

    H265SeqParamSet const* pSeqParamSet = pSlice->GetSeqParam();
    if (pSeqParamSet->scaling_list_enabled_flag)
    {
        PackQmatrix(pSlice);
    }

    CreateSliceParamBuffer(sliceInfo);
    CreateSliceDataBuffer(sliceInfo);

    const size_t sliceCount = sliceInfo->GetSliceCount();
    for (size_t sliceNum = 0; sliceNum < sliceCount; sliceNum++)
    {
        PackSliceParams(sliceInfo->GetSlice(sliceNum), sliceNum, sliceNum == sliceCount - 1);
    }

    Status s = m_va->Execute();
    if(s != UMC_OK)
        throw h265_exception(s);
}

void PackerVA::BeginFrame(H265DecoderFrame* /* frame */)
{
}

} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE
