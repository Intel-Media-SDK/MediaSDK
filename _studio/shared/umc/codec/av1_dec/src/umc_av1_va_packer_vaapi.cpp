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



#include "umc_vp9_dec_defs.h"

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#include <algorithm>
#include "umc_structures.h"
#include "umc_vp9_bitstream.h"
#include "umc_vp9_frame.h"
#include "umc_av1_utils.h"

#include "umc_av1_frame.h"
#include "umc_av1_va_packer_vaapi.h"

using namespace UMC;

namespace UMC_AV1_DECODER
{
    Packer * Packer::CreatePacker(UMC::VideoAccelerator * va)
    {
        return new PackerVA(va);
    }

    Packer::Packer(UMC::VideoAccelerator * va)
        : m_va(va)
    {}

    Packer::~Packer()
    {}

/****************************************************************************************************/
// Linux VAAPI packer implementation
/****************************************************************************************************/

    PackerVA::PackerVA(UMC::VideoAccelerator * va)
        : Packer(va)
    {}

    UMC::Status PackerVA::GetStatusReport(void * pStatusReport, size_t size)
    {
        return UMC_OK;
    }

    void PackerVA::BeginFrame() {}
    void PackerVA::EndFrame() {}

    void PackerVA::PackAU(std::vector<TileSet>& tileSets, AV1DecoderFrame const& info, bool firstSubmission)
    {
        if (firstSubmission)
        {
            // it's first submission for current frame - need to fill and submit picture parameters
            UMC::UMCVACompBuffer* compBufPic = nullptr;
            VADecPictureParameterBufferAV1 *picParam =
                (VADecPictureParameterBufferAV1*)m_va->GetCompBuffer(VAPictureParameterBufferType, &compBufPic, sizeof(VADecPictureParameterBufferAV1));

            if (!picParam || !compBufPic || (compBufPic->GetBufferSize() < static_cast<int32_t>(sizeof(VADecPictureParameterBufferAV1))))
                throw av1_exception(MFX_ERR_MEMORY_ALLOC);

            compBufPic->SetDataSize(sizeof(VADecPictureParameterBufferAV1));
            *picParam = VADecPictureParameterBufferAV1{};
            PackPicParams(*picParam, info);
        }

        UMC::UMCVACompBuffer* compBufBs = nullptr;
        uint8_t* const bitstreamData = (uint8_t *)m_va->GetCompBuffer(VASliceDataBufferType, &compBufBs, CalcSizeOfTileSets(tileSets), tileSets.size());
        if (!bitstreamData || !compBufBs)
            throw av1_exception(MFX_ERR_MEMORY_ALLOC);

        std::vector<VASliceParameterBufferAV1> tileControlParams;

        for (auto& tileSet : tileSets)
        {
            const size_t offsetInBuffer = compBufBs->GetDataSize();
            const size_t spaceInBuffer = compBufBs->GetBufferSize() - offsetInBuffer;
            TileLayout layout;
            const size_t bytesSubmitted = tileSet.Submit(bitstreamData, spaceInBuffer, offsetInBuffer, layout);

            if (bytesSubmitted)
            {
                compBufBs->SetDataSize(static_cast<uint32_t>(offsetInBuffer + bytesSubmitted));

                for (auto& loc : layout)
                {
                    tileControlParams.emplace_back();
                    PackTileControlParams(tileControlParams.back(), loc);
                }
            }
        }

        UMCVACompBuffer* compBufTile = nullptr;
        const int32_t tileControlInfoSize = static_cast<int32_t>(sizeof(VASliceParameterBufferAV1) * tileControlParams.size());
        VASliceParameterBufferAV1 *tileControlParam = (VASliceParameterBufferAV1*)m_va->GetCompBuffer(VASliceParameterBufferType, &compBufTile, tileControlInfoSize, tileSets.size());
        if (!tileControlParam || !compBufTile || (compBufTile->GetBufferSize() < tileControlInfoSize))
            throw av1_exception(MFX_ERR_MEMORY_ALLOC);

        std::copy(tileControlParams.begin(), tileControlParams.end(), tileControlParam);
        compBufTile->SetDataSize(tileControlInfoSize);

        if(m_va->m_MaxContextPriority)
            PackPriorityParams();
    }

    void PackerVA::PackPriorityParams()
    {
        mfxPriority priority = m_va->m_ContextPriority;
        UMCVACompBuffer *GpuPriorityBuf;
        VAContextParameterUpdateBuffer* GpuPriorityBuf_Av1Decode = (VAContextParameterUpdateBuffer *)m_va->GetCompBuffer(VAContextParameterUpdateBufferType, &GpuPriorityBuf, sizeof(VAContextParameterUpdateBuffer));
        if (!GpuPriorityBuf_Av1Decode)
            throw av1_exception(MFX_ERR_MEMORY_ALLOC);

        memset(GpuPriorityBuf_Av1Decode, 0, sizeof(VAContextParameterUpdateBuffer));
        GpuPriorityBuf_Av1Decode->flags.bits.context_priority_update = 1;

        if(priority == MFX_PRIORITY_LOW)
        {
            GpuPriorityBuf_Av1Decode->context_priority.bits.priority = 0;
        }
        else if (priority == MFX_PRIORITY_HIGH)
        {
            GpuPriorityBuf_Av1Decode->context_priority.bits.priority = m_va->m_MaxContextPriority;
        }
        else
        {
            GpuPriorityBuf_Av1Decode->context_priority.bits.priority = m_va->m_MaxContextPriority/2;
        }

        GpuPriorityBuf->SetDataSize(sizeof(VAContextParameterUpdateBuffer));
    }

    void PackerVA::PackPicParams(VADecPictureParameterBufferAV1& picParam, AV1DecoderFrame const& frame)
    {
        SequenceHeader const& sh = frame.GetSeqHeader();

        picParam.frame_width_minus1 = (uint16_t)frame.GetUpscaledWidth() - 1;
        picParam.frame_height_minus1 = (uint16_t)frame.GetFrameHeight() - 1;

        FrameHeader const& info =
            frame.GetFrameHeader();

        picParam.output_frame_width_in_tiles_minus_1 = 0;
        picParam.output_frame_height_in_tiles_minus_1 = 0;

        // fill seq parameters
        picParam.profile = (uint8_t)sh.seq_profile;

        auto& seqInfo = picParam.seq_info_fields.fields;

        seqInfo.still_picture = 0;

        seqInfo.use_128x128_superblock = (sh.sbSize == BLOCK_128X128) ? 1 : 0;
        seqInfo.enable_filter_intra = sh.enable_filter_intra;
        seqInfo.enable_intra_edge_filter = sh.enable_intra_edge_filter;
        seqInfo.enable_interintra_compound = sh.enable_interintra_compound;
        seqInfo.enable_masked_compound = sh.enable_masked_compound;
        seqInfo.enable_cdef = sh.enable_cdef;

        seqInfo.enable_dual_filter = sh.enable_dual_filter;
        seqInfo.enable_order_hint = sh.enable_order_hint;
        seqInfo.enable_jnt_comp = sh.enable_jnt_comp;
        seqInfo.mono_chrome = sh.color_config.mono_chrome;
        seqInfo.color_range = sh.color_config.color_range;
        seqInfo.subsampling_x = sh.color_config.subsampling_x;
        seqInfo.subsampling_y = sh.color_config.subsampling_y;
        seqInfo.chroma_sample_position = sh.color_config.chroma_sample_position;
        seqInfo.film_grain_params_present = sh.film_grain_param_present;

        picParam.matrix_coefficients = sh.color_config.matrix_coefficients;
        picParam.bit_depth_idx = (sh.color_config.BitDepth == 10) ? 1 :
            (sh.color_config.BitDepth == 12) ? 2 : 0;
        picParam.order_hint_bits_minus_1 = (uint8_t)sh.order_hint_bits_minus1;

        // fill pic params
        auto& picInfo = picParam.pic_info_fields.bits;

        picInfo.frame_type = info.frame_type;
        picInfo.show_frame = info.show_frame;
        picInfo.showable_frame = info.showable_frame;
        picInfo.error_resilient_mode = info.error_resilient_mode;
        picInfo.disable_cdf_update = info.disable_cdf_update;
        picInfo.allow_screen_content_tools = info.allow_screen_content_tools;
        picInfo.force_integer_mv = info.force_integer_mv;
        picInfo.allow_intrabc = info.allow_intrabc;
        picInfo.use_superres = (info.SuperresDenom == SCALE_NUMERATOR) ? 0 : 1;
        picInfo.allow_high_precision_mv = info.allow_high_precision_mv;
        picInfo.is_motion_mode_switchable = info.is_motion_mode_switchable;
        picInfo.use_ref_frame_mvs = info.use_ref_frame_mvs;
        picInfo.disable_frame_end_update_cdf = info.disable_frame_end_update_cdf;
        picInfo.uniform_tile_spacing_flag = info.tile_info.uniform_tile_spacing_flag;
        picInfo.allow_warped_motion = info.allow_warped_motion;

        picParam.order_hint = (uint8_t)info.order_hint;
        picParam.superres_scale_denominator = (uint8_t)info.SuperresDenom;

        picParam.interp_filter = (uint8_t)info.interpolation_filter;

        // fill segmentation params and map
        auto& seg = picParam.seg_info;
        seg.segment_info_fields.bits.enabled = info.segmentation_params.segmentation_enabled;;
        seg.segment_info_fields.bits.temporal_update = info.segmentation_params.segmentation_temporal_update;
        seg.segment_info_fields.bits.update_map = info.segmentation_params.segmentation_update_map;
        seg.segment_info_fields.bits.update_data = info.segmentation_params.segmentation_update_data;

        // set current and reference frames
        picParam.current_frame = (VASurfaceID)m_va->GetSurfaceID(frame.GetMemID(SURFACE_RECON));
        if (!frame.FilmGrainDisabled())
            picParam.current_display_picture = (VASurfaceID)m_va->GetSurfaceID(frame.GetMemID());

        if (seg.segment_info_fields.bits.enabled)
        {
            for (uint8_t i = 0; i < VP9_MAX_NUM_OF_SEGMENTS; i++)
            {
                seg.feature_mask[i] = (uint8_t)info.segmentation_params.FeatureMask[i];
                for (uint8_t j = 0; j < SEG_LVL_MAX; j++)
                    seg.feature_data[i][j] = (int16_t)info.segmentation_params.FeatureData[i][j];
            }
        }

        for (uint8_t ref = 0; ref < NUM_REF_FRAMES; ++ref)
            picParam.ref_frame_map[ref] = (VASurfaceID)m_va->GetSurfaceID(frame.frame_dpb[ref]->GetMemID(SURFACE_RECON));

        for (uint8_t ref_idx = 0; ref_idx < INTER_REFS; ref_idx++)
        {
            const uint8_t idxInDPB = (uint8_t)info.ref_frame_idx[ref_idx];
            picParam.ref_frame_idx[ref_idx] = idxInDPB;
        }

        picParam.primary_ref_frame = (uint8_t)info.primary_ref_frame;

        // fill loop filter params
        picParam.filter_level[0] = (uint8_t)info.loop_filter_params.loop_filter_level[0];
        picParam.filter_level[1] = (uint8_t)info.loop_filter_params.loop_filter_level[1];
        picParam.filter_level_u = (uint8_t)info.loop_filter_params.loop_filter_level[2];
        picParam.filter_level_v = (uint8_t)info.loop_filter_params.loop_filter_level[3];

        auto& lfInfo = picParam.loop_filter_info_fields.bits;
        lfInfo.sharpness_level = (uint8_t)info.loop_filter_params.loop_filter_sharpness;
        lfInfo.mode_ref_delta_enabled = info.loop_filter_params.loop_filter_delta_enabled;
        lfInfo.mode_ref_delta_update = info.loop_filter_params.loop_filter_delta_update;


        for (uint8_t i = 0; i < TOTAL_REFS; i++)
        {
            picParam.ref_deltas[i] = info.loop_filter_params.loop_filter_ref_deltas[i];
        }
        for (uint8_t i = 0; i < UMC_VP9_DECODER::MAX_MODE_LF_DELTAS; i++)
        {
            picParam.mode_deltas[i] = info.loop_filter_params.loop_filter_mode_deltas[i];
        }

        // fill quantization params
        picParam.base_qindex = (int16_t)info.quantization_params.base_q_idx;
        picParam.y_dc_delta_q = (int8_t)info.quantization_params.DeltaQYDc;
        picParam.u_dc_delta_q = (int8_t)info.quantization_params.DeltaQUDc;
        picParam.v_dc_delta_q = (int8_t)info.quantization_params.DeltaQVDc;
        picParam.u_ac_delta_q = (int8_t)info.quantization_params.DeltaQUAc;
        picParam.v_ac_delta_q = (int8_t)info.quantization_params.DeltaQVAc;

        // fill CDEF
        picParam.cdef_damping_minus_3 = (uint8_t)(info.cdef_params.cdef_damping - 3);
        picParam.cdef_bits = (uint8_t)info.cdef_params.cdef_bits;

        for (uint8_t i = 0; i < CDEF_MAX_STRENGTHS; i++)
        {
            picParam.cdef_y_strengths[i] = (uint8_t)((info.cdef_params.cdef_y_pri_strength[i] << 2) + info.cdef_params.cdef_y_sec_strength[i]);
            picParam.cdef_uv_strengths[i] = (uint8_t)((info.cdef_params.cdef_uv_pri_strength[i] << 2) + info.cdef_params.cdef_uv_sec_strength[i]);
        }

        // fill quantization matrix params
        auto& qmFlags = picParam.qmatrix_fields.bits;
        qmFlags.using_qmatrix = info.quantization_params.using_qmatrix;
        qmFlags.qm_y = info.quantization_params.qm_y;
        qmFlags.qm_u = info.quantization_params.qm_u;
        qmFlags.qm_v = info.quantization_params.qm_v;


        auto& modeCtrl = picParam.mode_control_fields.bits;
        modeCtrl.delta_q_present_flag = info.delta_q_present;
        modeCtrl.log2_delta_q_res = CeilLog2(info.delta_q_res);
        modeCtrl.delta_lf_present_flag = info.delta_lf_present;
        modeCtrl.log2_delta_lf_res = CeilLog2(info.delta_lf_res);
        modeCtrl.delta_lf_multi = info.delta_lf_multi;
        modeCtrl.tx_mode = info.TxMode;
        modeCtrl.reference_select = (info.reference_mode == SINGLE_REFERENCE)?0:1;
        modeCtrl.reduced_tx_set_used = info.reduced_tx_set;
        modeCtrl.skip_mode_present = info.skip_mode_present;

        // fill loop restoration params
        auto& lrInfo = picParam.loop_restoration_fields.bits;
        lrInfo.yframe_restoration_type = info.lr_params.lr_type[0];
        lrInfo.cbframe_restoration_type = info.lr_params.lr_type[1];
        lrInfo.crframe_restoration_type = info.lr_params.lr_type[2];;
        lrInfo.lr_unit_shift = info.lr_params.lr_unit_shift;
        lrInfo.lr_uv_shift = info.lr_params.lr_uv_shift;

        // fill global motion params
        for (uint8_t i = 0; i < INTER_REFS; i++)
        {
            picParam.wm[i].invalid = info.global_motion_params[i + 1].invalid;
            picParam.wm[i].wmtype = static_cast<VAAV1TransformationType>(info.global_motion_params[i + 1].wmtype);
            for (uint8_t j = 0; j < 8; j++)
            {
                picParam.wm[i].wmmat[j] = info.global_motion_params[i + 1].wmmat[j];
                for (uint8_t j = 0; j < 8; j++)
                    picParam.wm[i].wmmat[j] = info.global_motion_params[i + 1].wmmat[j];
            }
        }

        // fill film grain params
        auto& fg = picParam.film_grain_info;
        auto& fgInfo = fg.film_grain_info_fields.bits;

        if (!frame.FilmGrainDisabled())
        {
            fgInfo.apply_grain = info.film_grain_params.apply_grain;
            fgInfo.chroma_scaling_from_luma = info.film_grain_params.chroma_scaling_from_luma;
            fgInfo.grain_scaling_minus_8 = info.film_grain_params.grain_scaling - 8;
            fgInfo.ar_coeff_lag = info.film_grain_params.ar_coeff_lag;
            fgInfo.ar_coeff_shift_minus_6 = info.film_grain_params.ar_coeff_shift - 6;
            fgInfo.grain_scale_shift = info.film_grain_params.grain_scale_shift;
            fgInfo.overlap_flag = info.film_grain_params.overlap_flag;
            fgInfo.clip_to_restricted_range = info.film_grain_params.clip_to_restricted_range;

            fg.grain_seed = (uint16_t)info.film_grain_params.grain_seed;
            fg.num_y_points = (uint8_t)info.film_grain_params.num_y_points;

            for (uint8_t i = 0; i < MAX_POINTS_IN_SCALING_FUNCTION_LUMA; i++)
            {
                fg.point_y_value[i] = (uint8_t)info.film_grain_params.point_y_value[i];
                fg.point_y_scaling[i] = (uint8_t)info.film_grain_params.point_y_scaling[i];
            }

            fg.num_cb_points = (uint8_t)info.film_grain_params.num_cb_points;
            fg.num_cr_points = (uint8_t)info.film_grain_params.num_cr_points;

            for (uint8_t i = 0; i < MAX_POINTS_IN_SCALING_FUNCTION_CHROMA; i++)
            {
                fg.point_cb_value[i] = (uint8_t)info.film_grain_params.point_cb_value[i];
                fg.point_cb_scaling[i] = (uint8_t)info.film_grain_params.point_cb_scaling[i];
                fg.point_cr_value[i] = (uint8_t)info.film_grain_params.point_cr_value[i];
                fg.point_cr_scaling[i] = (uint8_t)info.film_grain_params.point_cr_scaling[i];
            }

            for (uint8_t i = 0; i < MAX_AUTOREG_COEFFS_LUMA; i++)
                fg.ar_coeffs_y[i] = (int8_t)info.film_grain_params.ar_coeffs_y[i];

            for (uint8_t i = 0; i < MAX_AUTOREG_COEFFS_CHROMA; i++)
            {
                fg.ar_coeffs_cb[i] = (int8_t)info.film_grain_params.ar_coeffs_cb[i];
                fg.ar_coeffs_cr[i] = (int8_t)info.film_grain_params.ar_coeffs_cr[i];
            }

            fg.cb_mult = (uint8_t)info.film_grain_params.cb_mult;
            fg.cb_luma_mult = (uint8_t)info.film_grain_params.cb_luma_mult;
            fg.cb_offset = (uint16_t)info.film_grain_params.cb_offset;
            fg.cr_mult = (uint8_t)info.film_grain_params.cr_mult;
            fg.cr_luma_mult = (uint8_t)info.film_grain_params.cr_luma_mult;
            fg.cr_offset = (uint16_t)info.film_grain_params.cr_offset;
        }

        // fill tile params
        picParam.tile_cols = (uint8_t)info.tile_info.TileCols;
        picParam.tile_rows = (uint8_t)info.tile_info.TileRows;

        for (uint32_t i = 0; i < picParam.tile_cols; i++)
        {
            picParam.width_in_sbs_minus_1[i] =
                (uint16_t)(info.tile_info.SbColStarts[i + 1] - info.tile_info.SbColStarts[i] - 1);
        }

        for (int i = 0; i < picParam.tile_rows; i++)
        {
            picParam.height_in_sbs_minus_1[i] =
                (uint16_t)(info.tile_info.SbRowStarts[i + 1] - info.tile_info.SbRowStarts[i] - 1);
        }

        picParam.context_update_tile_id = (uint16_t)info.tile_info.context_update_tile_id;
        picParam.tile_count_minus_1 = 0;
    }

    void PackerVA::PackTileControlParams(VASliceParameterBufferAV1& tileControlParam, TileLocation const& loc)
    {
        tileControlParam.slice_data_offset = (uint32_t)loc.offset;
        tileControlParam.slice_data_size = (uint32_t)loc.size;
        tileControlParam.slice_data_flag = 0;
        tileControlParam.tile_row = (uint16_t)loc.row;
        tileControlParam.tile_column = (uint16_t)loc.col;
        tileControlParam.tg_start = (uint16_t)loc.startIdx;
        tileControlParam.tg_end = (uint16_t)loc.endIdx;
    }

} // namespace UMC_AV1_DECODER

#endif // MFX_ENABLE_AV1_VIDEO_DECODE

