// Copyright (c) 2019-2020 Intel Corporation
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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_ddi.h"
#include "ehw_resources_pool.h"
#include "ehw_device.h"
#include <vector>

namespace HEVCEHW
{
namespace Base
{
    static const GUID DXVA2_Intel_Encode_HEVC_Main =
    { 0x28566328, 0xf041, 0x4466,{ 0x8b, 0x14, 0x8f, 0x58, 0x31, 0xe7, 0x8f, 0x8b } };
    static const GUID DXVA2_Intel_Encode_HEVC_Main10 =
    { 0x6b4a94db, 0x54fe, 0x4ae1,{ 0x9b, 0xe4, 0x7a, 0x7d, 0xad, 0x00, 0x46, 0x00 } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main =
    { 0xb8b28e0c, 0xecab, 0x4217,{ 0x8c, 0x82, 0xea, 0xaa, 0x97, 0x55, 0xaa, 0xf0 } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main10 =
    { 0x8732ecfd, 0x9747, 0x4897,{ 0xb4, 0x2a, 0xe5, 0x34, 0xf9, 0xff, 0x2b, 0x7a } };
    static const GUID DXVA2_Intel_Encode_HEVC_Main422 =
    { 0x056a6e36, 0xf3a8, 0x4d00,{ 0x96, 0x63, 0x7e, 0x94, 0x30, 0x35, 0x8b, 0xf9 } };
    static const GUID DXVA2_Intel_Encode_HEVC_Main422_10 =
    { 0xe139b5ca, 0x47b2, 0x40e1,{ 0xaf, 0x1c, 0xad, 0x71, 0xa6, 0x7a, 0x18, 0x36 } };
    static const GUID DXVA2_Intel_Encode_HEVC_Main444 =
    { 0x5415a68c, 0x231e, 0x46f4,{ 0x87, 0x8b, 0x5e, 0x9a, 0x22, 0xe9, 0x67, 0xe9 } };
    static const GUID DXVA2_Intel_Encode_HEVC_Main444_10 =
    { 0x161be912, 0x44c2, 0x49c0,{ 0xb6, 0x1e, 0xd9, 0x46, 0x85, 0x2b, 0x32, 0xa1 } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main422 =
    { 0xcee393ab, 0x1030, 0x4f7b,{ 0x8d, 0xbc, 0x55, 0x62, 0x9c, 0x72, 0xf1, 0x7e } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main422_10 =
    { 0x580da148, 0xe4bf, 0x49b1,{ 0x94, 0x3b, 0x42, 0x14, 0xab, 0x05, 0xa6, 0xff } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main444 =
    { 0x87b2ae39, 0xc9a5, 0x4c53,{ 0x86, 0xb8, 0xa5, 0x2d, 0x7e, 0xdb, 0xa4, 0x88 } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main444_10 =
    { 0x10e19ac8, 0xbf39, 0x4443,{ 0xbe, 0xc3, 0x1b, 0x0c, 0xbf, 0xe4, 0xc7, 0xaa } };

    class EndOfBuffer : public std::exception
    {
    public:
        EndOfBuffer() : std::exception() {}
    };

    constexpr mfxU8  MAX_DPB_SIZE               = 15;
    constexpr mfxU8  IDX_INVALID                = 0xff;
    constexpr mfxU8  HW_SURF_ALIGN_W            = 16;
    constexpr mfxU8  HW_SURF_ALIGN_H            = 16;
    constexpr mfxU16 MAX_SLICES                 = 600; // conforms to level 6 limits
    constexpr mfxU8  DEFAULT_LTR_INTERVAL       = 16;
    constexpr mfxU8  DEFAULT_PPYR_INTERVAL      = 3;
    constexpr mfxU16 GOP_INFINITE               = 0xFFFF;
    constexpr mfxU8  MAX_NUM_TILE_COLUMNS       = 20;
    constexpr mfxU8  MAX_NUM_TILE_ROWS          = 22;
    constexpr mfxU8  MAX_NUM_LONG_TERM_PICS     = 8;
    constexpr mfxU16 MIN_TILE_WIDTH_IN_SAMPLES  = 256;
    constexpr mfxU16 MIN_TILE_HEIGHT_IN_SAMPLES = 64;

    enum NALU_TYPE
    {
        TRAIL_N = 0,
        TRAIL_R,
        TSA_N, TSA_R,
        STSA_N, STSA_R,
        RADL_N, RADL_R,
        RASL_N, RASL_R,
        RSV_VCL_N10, RSV_VCL_R11, RSV_VCL_N12, RSV_VCL_R13, RSV_VCL_N14, RSV_VCL_R15,
        BLA_W_LP, BLA_W_RADL, BLA_N_LP,
        IDR_W_RADL, IDR_N_LP,
        CRA_NUT,
        RSV_IRAP_VCL22, RSV_IRAP_VCL23,
        RSV_VCL24, RSV_VCL25, RSV_VCL26, RSV_VCL27, RSV_VCL28, RSV_VCL29, RSV_VCL30, RSV_VCL31,
        VPS_NUT,
        SPS_NUT,
        PPS_NUT,
        AUD_NUT,
        EOS_NUT,
        EOB_NUT,
        FD_NUT,
        PREFIX_SEI_NUT,
        SUFFIX_SEI_NUT,
        RSV_NVCL41, RSV_NVCL42, RSV_NVCL43, RSV_NVCL44, RSV_NVCL45, RSV_NVCL46, RSV_NVCL47,
        UNSPEC48, UNSPEC49, UNSPEC50, UNSPEC51, UNSPEC52, UNSPEC53, UNSPEC54, UNSPEC55,
        UNSPEC56, UNSPEC57, UNSPEC58, UNSPEC59, UNSPEC60, UNSPEC61, UNSPEC62, UNSPEC63,
        num_NALU_TYPE
    };

    struct PTL
    {
        mfxU8  profile_space : 2;
        mfxU8  tier_flag     : 1;
        mfxU8  profile_idc   : 5;

        mfxU8  progressive_source_flag    : 1;
        mfxU8  interlaced_source_flag     : 1;
        mfxU8  non_packed_constraint_flag : 1;
        mfxU8  frame_only_constraint_flag : 1;
        mfxU8  profile_present_flag       : 1;
        mfxU8  level_present_flag         : 1;
        mfxU8                             : 2;
        mfxU8  level_idc;

        mfxU32 profile_compatibility_flags;

        union
        {
            mfxU32 rext_constraint_flags_0_31;

            struct
            {
                mfxU32 max_12bit        : 1;
                mfxU32 max_10bit        : 1;
                mfxU32 max_8bit         : 1;
                mfxU32 max_422chroma    : 1;
                mfxU32 max_420chroma    : 1;
                mfxU32 max_monochrome   : 1;
                mfxU32 intra            : 1;
                mfxU32 one_picture_only : 1;
                mfxU32 lower_bit_rate   : 1;
                mfxU32                  : 23;
            } constraint;
        };
        mfxU32 rext_constraint_flags_32_42  : 11;
        mfxU32 inbld_flag                   :  1;
        mfxU32                              : 20;
    };

    struct SubLayerOrdering
    {
        mfxU8  max_dec_pic_buffering_minus1 : 4;
        mfxU8  max_num_reorder_pics         : 4;
        mfxU32 max_latency_increase_plus1;
    };

    struct STRPS
    {
        mfxU8  inter_ref_pic_set_prediction_flag : 1;
        mfxU8  delta_idx_minus1                  : 6;
        mfxU8  delta_rps_sign                    : 1;

        mfxU8  num_negative_pics : 4;
        mfxU8  num_positive_pics : 4;

        mfxU16 abs_delta_rps_minus1;
        mfxU16 WeightInGop;

        struct Pic
        {
            mfxU8 used_by_curr_pic_flag   : 1;
            mfxU8 use_delta_flag          : 1;
            mfxI16 DeltaPocSX;

            union
            {
                struct
                {
                    mfxU16 delta_poc_s0_minus1      : 15;
                    mfxU16 used_by_curr_pic_s0_flag : 1;
                };
                struct
                {
                    mfxU16 delta_poc_s1_minus1      : 15;
                    mfxU16 used_by_curr_pic_s1_flag : 1;
                };
                struct
                {
                    mfxU16 delta_poc_sx_minus1      : 15;
                    mfxU16 used_by_curr_pic_sx_flag : 1;
                };
            };
        }pic[16];
    };

    using STRPSPic = STRPS::Pic;

    struct LayersInfo
    {
        mfxU8 sub_layer_ordering_info_present_flag : 1;
        PTL general;
        struct SubLayer : PTL, SubLayerOrdering {} sub_layer[8];
    };


    struct HRDInfo
    {
        mfxU16 nal_hrd_parameters_present_flag              : 1;
        mfxU16 vcl_hrd_parameters_present_flag              : 1;
        mfxU16 sub_pic_hrd_params_present_flag              : 1;
        mfxU16 du_cpb_removal_delay_increment_length_minus1 : 5;
        mfxU16 sub_pic_cpb_params_in_pic_timing_sei_flag    : 1;
        mfxU16 dpb_output_delay_du_length_minus1            : 5;

        mfxU16 tick_divisor_minus2                          : 8;
        mfxU16 bit_rate_scale                               : 4;
        mfxU16 cpb_size_scale                               : 4;

        mfxU8  cpb_size_du_scale                            : 4;
        mfxU16 initial_cpb_removal_delay_length_minus1      : 5;
        mfxU16 au_cpb_removal_delay_length_minus1           : 5;
        mfxU16 dpb_output_delay_length_minus1               : 5;

        struct SubLayer
        {
            mfxU8  fixed_pic_rate_general_flag     :  1;
            mfxU8  fixed_pic_rate_within_cvs_flag  :  1;
            mfxU8  low_delay_hrd_flag              :  1;

            mfxU16 elemental_duration_in_tc_minus1 : 11;
            mfxU16 cpb_cnt_minus1                  :  5;

            struct CPB
            {
                mfxU32 bit_rate_value_minus1;
                mfxU32 cpb_size_value_minus1;
                mfxU32 cpb_size_du_value_minus1;
                mfxU32 bit_rate_du_value_minus1;
                mfxU8  cbr_flag;
            } cpb[32];
        } sl[8];
    };

    struct BufferingPeriodSEI
    {
        mfxU8  seq_parameter_set_id         : 4;
        mfxU8  irap_cpb_params_present_flag : 1;
        mfxU8  concatenation_flag           : 1;

        mfxU32 cpb_delay_offset;
        mfxU32 dpb_delay_offset;
        mfxU32 au_cpb_removal_delay_delta_minus1;

        struct CPB{
            mfxU32 initial_cpb_removal_delay;
            mfxU32 initial_cpb_removal_offset;
            mfxU32 initial_alt_cpb_removal_delay;
            mfxU32 initial_alt_cpb_removal_offset;
        } nal[32], vcl[32];
    };

    struct PicTimingSEI
    {
        mfxU8  pic_struct                       : 4;
        mfxU8  source_scan_type                 : 2;
        mfxU8  duplicate_flag                   : 1;
        mfxU8  du_common_cpb_removal_delay_flag : 1;

        mfxU32 au_cpb_removal_delay_minus1;
        mfxU32 pic_dpb_output_delay;
        mfxU32 pic_dpb_output_du_delay;
        mfxU32 num_decoding_units_minus1;
        mfxU32 du_common_cpb_removal_delay_increment_minus1;

        mfxU32* num_nalus_in_du_minus1;
        mfxU32* du_cpb_removal_delay_increment_minus1;
    };

    struct VPS : LayersInfo
    {
        mfxU16 video_parameter_set_id   : 4;
        mfxU16 reserved_three_2bits     : 2;
        mfxU16 max_layers_minus1        : 6;
        mfxU16 max_sub_layers_minus1    : 3;
        mfxU16 temporal_id_nesting_flag : 1;
        mfxU16 reserved_0xffff_16bits;

        mfxU16 max_layer_id          :  6;
        mfxU16 num_layer_sets_minus1 : 10;

        //mfxU8** layer_id_included_flag; // max [1024][64];

        mfxU32 num_units_in_tick;
        mfxU32 time_scale;
        mfxU32 timing_info_present_flag        : 1;
        mfxU32 poc_proportional_to_timing_flag : 1;
        mfxU32 num_ticks_poc_diff_one_minus1 : 10;
        mfxU32 num_hrd_parameters            : 10;

        //VPSHRD* hrd; //max 1024
    };


    struct VUI
    {
        mfxU8  aspect_ratio_info_present_flag : 1;
        mfxU8  aspect_ratio_idc;
        mfxU16 sar_width;
        mfxU16 sar_height;

        mfxU8  overscan_info_present_flag      : 1;
        mfxU8  overscan_appropriate_flag       : 1;
        mfxU8  video_signal_type_present_flag  : 1;
        mfxU8  video_format                    : 3;
        mfxU8  video_full_range_flag           : 1;
        mfxU8  colour_description_present_flag : 1;
        mfxU8  colour_primaries;
        mfxU8  transfer_characteristics;
        mfxU8  matrix_coeffs;

        mfxU8  chroma_loc_info_present_flag        : 1;
        mfxU8  chroma_sample_loc_type_top_field    : 3;
        mfxU8  chroma_sample_loc_type_bottom_field : 3;

        mfxU8  neutral_chroma_indication_flag : 1;
        mfxU8  field_seq_flag                 : 1;
        mfxU8  frame_field_info_present_flag  : 1;
        mfxU8  default_display_window_flag    : 1;

        mfxU32 def_disp_win_left_offset;
        mfxU32 def_disp_win_right_offset;
        mfxU32 def_disp_win_top_offset;
        mfxU32 def_disp_win_bottom_offset;

        mfxU8  timing_info_present_flag        : 1;
        mfxU8  hrd_parameters_present_flag     : 1;
        mfxU8  poc_proportional_to_timing_flag : 1;

        mfxU8  bitstream_restriction_flag              : 1;
        mfxU8  tiles_fixed_structure_flag              : 1;
        mfxU8  motion_vectors_over_pic_boundaries_flag : 1;
        mfxU8  restricted_ref_pic_lists_flag           : 1;

        mfxU32 num_units_in_tick;
        mfxU32 time_scale;
        mfxU32 num_ticks_poc_diff_one_minus1;

        mfxU32 min_spatial_segmentation_idc : 12;
        mfxU32 max_bytes_per_pic_denom      :  5;
        mfxU32 max_bits_per_min_cu_denom    :  5;
        mfxU16 log2_max_mv_length_horizontal : 5;
        mfxU16 log2_max_mv_length_vertical   : 4;

        HRDInfo hrd;
    };

    struct ScalingList
    {
        mfxU8 scalingLists0[6][16];
        mfxU8 scalingLists1[6][64];
        mfxU8 scalingLists2[6][64];
        mfxU8 scalingLists3[2][64];
        mfxU8 scalingListDCCoefSizeID2[6];
        mfxU8 scalingListDCCoefSizeID3[2];
    };

    struct SPS : LayersInfo
    {
        mfxU8  video_parameter_set_id   : 4;
        mfxU8  max_sub_layers_minus1    : 3;
        mfxU8  temporal_id_nesting_flag : 1;

        mfxU8  seq_parameter_set_id       : 4;
        mfxU8  chroma_format_idc          : 2;
        mfxU8  separate_colour_plane_flag : 1;
        mfxU8  conformance_window_flag    : 1;

        mfxU32 pic_width_in_luma_samples;
        mfxU32 pic_height_in_luma_samples;

        mfxU32 conf_win_left_offset;
        mfxU32 conf_win_right_offset;
        mfxU32 conf_win_top_offset;
        mfxU32 conf_win_bottom_offset;

        mfxU8  bit_depth_luma_minus8                : 3;
        mfxU8  bit_depth_chroma_minus8              : 3;
        mfxU8  log2_max_pic_order_cnt_lsb_minus4    : 4;
        //mfxU8  sub_layer_ordering_info_present_flag : 1;

        mfxU32 log2_min_luma_coding_block_size_minus3;
        mfxU32 log2_diff_max_min_luma_coding_block_size;
        mfxU32 log2_min_transform_block_size_minus2;
        mfxU32 log2_diff_max_min_transform_block_size;
        mfxU32 max_transform_hierarchy_depth_inter;
        mfxU32 max_transform_hierarchy_depth_intra;

        mfxU8  scaling_list_enabled_flag      : 1;
        mfxU8  scaling_list_data_present_flag : 1;

        mfxU8  amp_enabled_flag                    : 1;
        mfxU8  sample_adaptive_offset_enabled_flag : 1;

        mfxU8  pcm_enabled_flag                    : 1;
        mfxU8  pcm_loop_filter_disabled_flag       : 1;
        mfxU8  pcm_sample_bit_depth_luma_minus1    : 4;
        mfxU8  pcm_sample_bit_depth_chroma_minus1  : 4;
        mfxU32 log2_min_pcm_luma_coding_block_size_minus3;
        mfxU32 log2_diff_max_min_pcm_luma_coding_block_size;

        mfxU8  long_term_ref_pics_present_flag : 1;
        mfxU8  num_long_term_ref_pics_sps      : 6;

        mfxU16 lt_ref_pic_poc_lsb_sps[32];
        mfxU8  used_by_curr_pic_lt_sps_flag[32];

        mfxU8  temporal_mvp_enabled_flag           : 1;
        mfxU8  strong_intra_smoothing_enabled_flag : 1;
        mfxU8  vui_parameters_present_flag         : 1;
        mfxU8  extension_flag                      : 1;
        mfxU8  extension_data_flag                 : 1;

        mfxU8 range_extension_flag                    : 1;
        mfxU8 transform_skip_rotation_enabled_flag    : 1;
        mfxU8 transform_skip_context_enabled_flag     : 1;
        mfxU8 implicit_rdpcm_enabled_flag             : 1;
        mfxU8 explicit_rdpcm_enabled_flag             : 1;
        mfxU8 extended_precision_processing_flag      : 1;
        mfxU8 intra_smoothing_disabled_flag           : 1;
        mfxU8 high_precision_offsets_enabled_flag     : 1;
        mfxU8 persistent_rice_adaptation_enabled_flag : 1;
        mfxU8 cabac_bypass_alignment_enabled_flag     : 1;

        mfxU8 low_delay_mode    : 1;
        mfxU8 hierarchical_flag : 1;

        mfxU8 ExtensionFlags;
        mfxU8 num_short_term_ref_pic_sets;
        STRPS strps[65];

        ScalingList scl;
        VUI vui;
    };

    struct PPS
    {
        mfxU16 pic_parameter_set_id : 6;
        mfxU16 seq_parameter_set_id : 4;
        mfxU16 dependent_slice_segments_enabled_flag : 1;
        mfxU16 output_flag_present_flag              : 1;
        mfxU16 num_extra_slice_header_bits           : 3;
        mfxU16 sign_data_hiding_enabled_flag         : 1;
        mfxU16 cabac_init_present_flag               : 1;
        mfxU16 num_ref_idx_l0_default_active_minus1  : 4;
        mfxU16 num_ref_idx_l1_default_active_minus1  : 4;
        mfxU16 constrained_intra_pred_flag           : 1;
        mfxU16 transform_skip_enabled_flag           : 1;
        mfxU16 cu_qp_delta_enabled_flag              : 1;
        mfxU16 slice_segment_header_extension_present_flag : 1;

        mfxU32 diff_cu_qp_delta_depth;
        mfxI32 init_qp_minus26;
        mfxI16 cb_qp_offset : 6;
        mfxI16 cr_qp_offset : 6;

        mfxU8  slice_chroma_qp_offsets_present_flag  : 1;
        mfxU8  weighted_pred_flag                    : 1;
        mfxU8  weighted_bipred_flag                  : 1;
        mfxU8  transquant_bypass_enabled_flag        : 1;
        mfxU8  tiles_enabled_flag                    : 1;
        mfxU8  entropy_coding_sync_enabled_flag      : 1;
        mfxU8  uniform_spacing_flag                  : 1;
        mfxU8  loop_filter_across_tiles_enabled_flag : 1;

        mfxU16 num_tile_columns_minus1;
        mfxU16 num_tile_rows_minus1;

        mfxU16 column_width[MAX_NUM_TILE_COLUMNS - 1];
        mfxU16 row_height[MAX_NUM_TILE_ROWS - 1];

        mfxU8  loop_filter_across_slices_enabled_flag  : 1;
        mfxU8  deblocking_filter_control_present_flag  : 1;
        mfxU8  deblocking_filter_override_enabled_flag : 1;
        mfxU8  deblocking_filter_disabled_flag         : 1;
        mfxU8  scaling_list_data_present_flag          : 1;
        mfxU8  lists_modification_present_flag         : 1;
        mfxU8  extension_flag                          : 1;
        mfxU8  extension_data_flag                     : 1;

        mfxI8  beta_offset_div2 : 4;
        mfxI8  tc_offset_div2   : 4;

        //ScalingListData* sld;

        mfxU16 log2_parallel_merge_level_minus2;

        mfxU32 range_extension_flag                      : 1;
        mfxU32 cross_component_prediction_enabled_flag   : 1;
        mfxU32 chroma_qp_offset_list_enabled_flag        : 1;
        mfxU32 log2_sao_offset_scale_luma                : 3;
        mfxU32 log2_sao_offset_scale_chroma              : 3;
        mfxU32 chroma_qp_offset_list_len_minus1          : 3;
        mfxU32 diff_cu_chroma_qp_offset_depth            : 5;
        mfxU32 log2_max_transform_skip_block_size_minus2 : 5;
        mfxU32                                           : 10;
        mfxI8  cb_qp_offset_list[6];
        mfxI8  cr_qp_offset_list[6];

        mfxU8 ExtensionFlags;
    };

    struct Slice
    {
        mfxU8  no_output_of_prior_pics_flag    : 1;
        mfxU8  pic_parameter_set_id            : 6;
        mfxU8  dependent_slice_segment_flag    : 1;

        mfxU32 segment_address;

        mfxU8  reserved_flags;
        mfxU8  type                       : 2;
        mfxU8  colour_plane_id            : 2;
        mfxU8  short_term_ref_pic_set_idx;

        mfxU8  pic_output_flag                 : 1;
        mfxU8  short_term_ref_pic_set_sps_flag : 1;
        mfxU8  num_long_term_sps               : 6;

        mfxU8  first_slice_segment_in_pic_flag  : 1;
        mfxU8  temporal_mvp_enabled_flag        : 1;
        mfxU8  sao_luma_flag                    : 1;
        mfxU8  sao_chroma_flag                  : 1;
        mfxU8  num_ref_idx_active_override_flag : 1;
        mfxU8  mvd_l1_zero_flag                 : 1;
        mfxU8  cabac_init_flag                  : 1;
        mfxU8  collocated_from_l0_flag          : 1;

        mfxU8  collocated_ref_idx            : 4;
        mfxU8  five_minus_max_num_merge_cand : 3;

        mfxU8  num_ref_idx_l0_active_minus1 : 4;
        mfxU8  num_ref_idx_l1_active_minus1 : 4;

        mfxU32 pic_order_cnt_lsb;
        mfxU16 num_long_term_pics;

        mfxI8  slice_qp_delta;
        mfxI16 slice_cb_qp_offset : 6;
        mfxI16 slice_cr_qp_offset : 6;

        mfxU8  deblocking_filter_override_flag        : 1;
        mfxU8  deblocking_filter_disabled_flag        : 1;
        mfxU8  loop_filter_across_slices_enabled_flag : 1;
        mfxU8  offset_len_minus1                      : 5;

        mfxI8  beta_offset_div2 : 4;
        mfxI8  tc_offset_div2   : 4;

        mfxU32 num_entry_point_offsets;

        STRPS strps;

        struct LongTerm
        {
            mfxU8  lt_idx_sps                 :  5;
            mfxU8  used_by_curr_pic_lt_flag   :  1;
            mfxU8  delta_poc_msb_present_flag :  1;
            mfxU32 poc_lsb_lt;
            mfxU32 delta_poc_msb_cycle_lt;
        } lt[MAX_NUM_LONG_TERM_PICS];

        mfxU8  ref_pic_list_modification_flag_lx[2];
        mfxU8  list_entry_lx[2][16];

        mfxU16 luma_log2_weight_denom   : 3;
        mfxU16 chroma_log2_weight_denom : 3;
        mfxI16 pwt[2][16][3][2]; //[list][list entry][Y, Cb, Cr][weight, offset]
    };

    struct NALU
    {
        mfxU16 long_start_code       : 1;
        mfxU16 nal_unit_type         : 6;
        mfxU16 nuh_layer_id          : 6;
        mfxU16 nuh_temporal_id_plus1 : 3;
    };

    using Resource = MfxEncodeHW::ResPool::Resource;

    struct IAllocation
        : public Storable
    {
        using TAlloc = CallChain<mfxStatus, const mfxFrameAllocRequest&, bool /*isCopyRequired*/>;
        TAlloc Alloc;

        using TAllocOpaque = CallChain<mfxStatus
            , const mfxFrameInfo &
            , mfxU16 /*type*/
            , mfxFrameSurface1 ** /*surfaces*/
            , mfxU16 /*numSurface*/>;
        TAllocOpaque AllocOpaque;

        using TGetResponse = CallChain<mfxFrameAllocResponse>;
        TGetResponse GetResponse;

        using TGetInfo = CallChain<mfxFrameInfo>;
        TGetInfo GetInfo;

        using TAcquire = CallChain<Resource>;
        TAcquire Acquire;

        using TRelease = CallChain<void, mfxU32 /*idx*/>;
        TRelease Release;

        using TClearFlag = CallChain<void, mfxU32 /*idx*/>;
        TClearFlag ClearFlag;

        using TSetFlag = CallChain<void, mfxU32 /*idx*/, mfxU32 /*flag*/>;
        TSetFlag SetFlag;

        using TGetFlag = CallChain<mfxU32, mfxU32 /*idx*/>;
        TGetFlag GetFlag;

        using TUnlockAll = CallChain<void>;
        TUnlockAll UnlockAll;

        std::unique_ptr<Storable> m_pthis;
    };

    class IBsReader
    {
    public:
        virtual ~IBsReader() {}
        virtual mfxU32 GetBit() = 0;
        virtual mfxU32 GetBits(mfxU32 n) = 0;
        virtual mfxU32 GetUE() = 0;
        virtual mfxI32 GetSE() = 0;

        //conditional read
        template<class TDefault>
        mfxU32 CondBits(bool bRead, mfxU32 nBits, TDefault defaultVal)
        {
            return bRead ? GetBits(nBits) : mfxU32(defaultVal);
        }
        template<class TDefault>
        mfxU32 CondBit(bool bRead, TDefault defaultVal)
        {
            return bRead ? GetBit() : mfxU32(defaultVal);
        }
        template<class TDefault>
        mfxU32 CondUE(bool bRead, TDefault defaultVal)
        {
            return bRead ? GetUE() : mfxU32(defaultVal);
        }
        template<class TDefault>
        mfxI32 CondSE(bool bRead, TDefault defaultVal)
        {
            return bRead ? GetSE() : mfxI32(defaultVal);
        }

    };

    class IBsWriter
    {
    public:
        virtual ~IBsWriter() {}
        virtual void PutBits(mfxU32 n, mfxU32 b) = 0;
        virtual void PutBit(mfxU32 b) = 0;
        virtual void PutUE(mfxU32 b) = 0;
        virtual void PutSE(mfxI32 b) = 0;
    };

    struct SliceInfo
    {
        mfxU32 SegmentAddress;
        mfxU32 NumLCU;
    };

    constexpr mfxU8 CODING_TYPE_I  = 1;
    constexpr mfxU8 CODING_TYPE_P  = 2;
    constexpr mfxU8 CODING_TYPE_B  = 3; //regular B, or no reference to other B frames
    constexpr mfxU8 CODING_TYPE_B1 = 4; //B1, reference to only I, P or regular B frames
    constexpr mfxU8 CODING_TYPE_B2 = 5; //B2, references include B1

    // Min frame params required for Reorder, RPL/DPB management
    struct FrameBaseInfo
        : Storable
    {
        mfxI32   POC            = -1;
        mfxU16   FrameType      = 0;
        bool     isLDB          = false; // is "low-delay B"
        mfxU8    TemporalID     = 0;
        mfxU32   BPyramidOrder  = mfxU32(MFX_FRAMEORDER_UNKNOWN);
        mfxU32   PyramidLevel   = 0;
        bool     bBottomField   = false;
        bool     b2ndField      = false;
    };

    typedef struct _DpbFrame
        : FrameBaseInfo
    {
        mfxU32   DisplayOrder   = mfxU32(-1);
        mfxU32   EncodedOrder   = mfxU32(-1);
        bool     isLTR          = false; // is "long-term"
        mfxU8    CodingType     = 0;
        Resource Raw;
        Resource Rec;
        mfxFrameSurface1* pSurfIn = nullptr; //input surface, may be opaque
    } DpbFrame, DpbArray[MAX_DPB_SIZE];

    enum eSkipMode
    {
        SKIPFRAME_NO = 0
        , SKIPFRAME_INSERT_DUMMY
        , SKIPFRAME_INSERT_DUMMY_PROTECTED
        , SKIPFRAME_INSERT_NOTHING
        , SKIPFRAME_BRC_ONLY
        , SKIPFRAME_EXT_PSEUDO
    };

    enum eSkipCmd
    {
        SKIPCMD_NeedInputReplacement        = (1 << 0)
        , SKIPCMD_NeedDriverCall            = (1 << 1)
        , SKIPCMD_NeedSkipSliceGen          = (1 << 2)
        , SKIPCMD_NeedCurrentFrameSkipping  = (1 << 3)
        , SKIPCMD_NeedNumSkipAdding         = (1 << 4)
    };

    struct IntraRefreshState
    {
        mfxU16  refrType;
        mfxU16  IntraLocation;
        mfxU16  IntraSize;
        mfxI16  IntRefQPDelta;
        bool    firstFrameInCycle;
    };

    enum eInsertHeader
    {
        INSERT_AUD      = 0x01,
        INSERT_VPS      = 0x02,
        INSERT_SPS      = 0x04,
        INSERT_PPS      = 0x08,
        INSERT_BPSEI    = 0x10,
        INSERT_PTSEI    = 0x20,
        INSERT_DCVSEI   = 0x40,
        INSERT_LLISEI   = 0x80,
        INSERT_SEI      = (INSERT_BPSEI | INSERT_PTSEI | INSERT_DCVSEI | INSERT_LLISEI)
    };

    enum eRecFlag
    {
        REC_SKIPPED = 1
        , REC_READY = 2
    };

    struct TaskCommonPar
        : DpbFrame
    {
        mfxU32              stage               = 0;
        mfxU32              MinFrameSize        = 0;
        mfxU32              BsDataLength        = 0;
        mfxU32              BsBytesAvailable    = 0;
        mfxU8*              pBsData             = nullptr;
        mfxU32*             pBsDataLength       = nullptr;
        mfxBitstream*       pBsOut              = nullptr;
        mfxFrameSurface1*   pSurfReal           = nullptr;
        mfxEncodeCtrl       ctrl                = {};
        mfxU32              SkipCMD             = 0;
        Resource            BS;
        Resource            CUQP;
        mfxHDLPair          HDLRaw              = {};
        bool                bCUQPMap            = false;
        bool                bForceSync          = false;
        bool                bSkip               = false;
        bool                bResetBRC           = false;
        bool                bDontPatchBS        = false;
        bool                bRecode             = false;
        bool                bForceLongStartCode = false;
        IntraRefreshState   IRState             = {};
        mfxI32              PrevIPoc            = -1;
        mfxI32              PrevRAP             = -1;
        mfxU16              NumRecode           = 0;
        mfxI8               QpY                 = 0;
        mfxI8               m_minQP             = 0;
        mfxI8               m_maxQP             = 0;

        mfxU8               SliceNUT            = 0;
        mfxU32              InsertHeaders       = 0;
        mfxU32              RepackHeaders       = 0;
        mfxU32              StatusReportId      = mfxU32(-1);

        mfxU32              initial_cpb_removal_delay   = 0;
        mfxU32              initial_cpb_removal_offset  = 0;
        mfxU32              cpb_removal_delay           = 0;
        //dpb_output_delay = (DisplayOrder + sps.sub_layer[0].max_num_reorder_pics - EncodedOrder);
        mfxU32              TCBRCTargetFrameSize        = 0;
        mfxU8 RefPicList[2][MAX_DPB_SIZE] =
        {
            {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
            {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
        };
        mfxU8 NumRefActive[2] = {};

        struct DPBs
        {
            DpbArray
                Active   // available during task execution (modified dpb[BEFORE] )
                , After  // after task execution (ACTIVE + curTask if ref)
                , Before // after previous task execution (prevTask dpb[AFTER])
                ;
        } DPB;
        std::list<mfxPayload> PLInternal = {};
    };

    inline void Invalidate(TaskCommonPar& par)
    {
        par = TaskCommonPar();
    }
    inline bool isValid(DpbFrame const & frame) { return IDX_INVALID != frame.Rec.Idx; }
    inline bool isDpbEnd(DpbArray const & dpb, mfxU32 idx) { return idx >= MAX_DPB_SIZE || !isValid(dpb[idx]); }

    class FrameLocker
        : public mfxFrameData
    {
    public:
        mfxU32 Pitch;

        FrameLocker(VideoCORE& core, mfxMemId memId, bool external = false)
            : m_data(*this)
            , m_core(core)
            , m_memId(memId)
        {
            m_data   = {};
            m_status = Lock(external);
            Pitch    = (mfxFrameData::PitchHigh << 16) + mfxFrameData::PitchLow;
        }
        FrameLocker(VideoCORE& core, mfxFrameData& data, bool external = false)
            : m_data(data)
            , m_core(core)
            , m_memId(data.MemId)
            , m_status(Lock(external))
        {
            Pitch = (mfxFrameData::PitchHigh << 16) + mfxFrameData::PitchLow;
        }

        ~FrameLocker()
        {
            Unlock();
        }

        enum { LOCK_NO, LOCK_INT, LOCK_EXT };

        mfxStatus Unlock()
        {
            mfxStatus mfxSts = MFX_ERR_NONE;
            if (m_status == LOCK_INT)
                mfxSts = m_core.UnlockFrame(m_memId, &m_data);
            else if (m_status == LOCK_EXT)
                mfxSts = m_core.UnlockExternalFrame(m_memId, &m_data);
            m_status = LOCK_NO;
            return mfxSts;
        }


        mfxU32 Lock(bool external)
        {
            mfxU32 status = LOCK_NO;
            if (!m_data.Y && !m_data.Y410)
            {
                status = external
                    ? (m_core.LockExternalFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_EXT : LOCK_NO)
                    : (m_core.LockFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_INT : LOCK_NO);
            }
            return status;
        }

    private:
        FrameLocker(FrameLocker const &) = delete;
        FrameLocker & operator =(FrameLocker const &) = delete;

        mfxFrameData& m_data;
        VideoCORE&    m_core;
        mfxMemId      m_memId;
        mfxU32        m_status;
    };

    enum ePackInfo
    {
        PACK_QPDOffset = 0
        , PACK_SAOOffset
        , PACK_VUIOffset
        , PACK_PWTOffset
        , PACK_PWTLength
        , NUM_PACK_INFO
    };

    using MfxEncodeHW::PackedData;

    struct PackedHeaders
    {
        PackedData VPS;
        PackedData SPS;
        PackedData PPS;
        PackedData AUD[3];
        PackedData PrefixSEI;
        PackedData SuffixSEI;
        std::vector<PackedData> SSH;
    };

    using MfxEncodeHW::DDIExecParam;

    template<class T>
    inline T& Deref(const DDIExecParam::Param& par, mfxU32 idx = 0)
    {
        ThrowAssert(
            !par.pData
            || (par.Size * std::max<mfxU32>(par.Num, 1)) < (sizeof(T) * (idx + 1))
            , "Invalid DDIExecParam::Param data");
        return *((T*)par.pData);
    }

    using DDIFeedbackParam = MfxEncodeHW::DDIFeedback;

    struct BsDataInfo
    {
        mfxU8  *Data;
        mfxU32 DataOffset;
        mfxU32 DataLength;
        mfxU32 MaxLength;
    };

    enum eResetFlags
    {
        RF_SPS_CHANGED      = (1 << 0)
        , RF_PPS_CHANGED    = (1 << 1)
        , RF_IDR_REQUIRED   = (1 << 2)
        , RF_BRC_RESET      = (1 << 3)
        , RF_CANCEL_TASKS   = (1 << 4)
    };

    struct ResetHint
    {
        mfxU32 Flags;
    };

    struct VAGUID
    {
        mfxU32 Profile;
        mfxU32 Entrypoint;
    };

    struct LessGUID
    {
        bool operator()(const GUID& l, const GUID& r) const
        {
            bool bEQ = true;
            bool bLT = l.Data1 < r.Data1;

            bEQ &= l.Data1 == r.Data1;
            bLT |= bEQ && l.Data2 < r.Data2;

            bEQ &= l.Data2 == r.Data2;
            bLT |= bEQ && l.Data3 < r.Data3;

            bEQ &= l.Data3 == r.Data3;
            bLT |= bEQ && std::lexicographical_compare(l.Data4, std::end(l.Data4), r.Data4, std::end(r.Data4));

            return bLT;
        }
    };

    /*SliceStructure indicates the restrictions set on the slice structure.*/
    enum SliceStructure
    {
        // Once slice for the whole frame
        ONESLICE              = 0
        /*Slices are composed of a power of 2 number of rows and each slice
          must have the same size (width and height) except for the last one,
          which must be smaller or equal to the previous slices. */
        , POW2ROW             = 1
        /*Slices are composed of any number of rows, but all must have the same
        size (width and height) except for the last one, which must be smaller
        or equal to the previous slices.*/
        , ROWSLICE            = 2
        //Arbitrary number of rows per slice for all slices.
        , ARBITRARY_ROW_SLICE = 3
        //Arbitrary number of macroblocks per slice for all slices.
        , ARBITRARY_MB_SLICE  = 4
    };

    typedef std::list<StorageRW>::iterator TTaskIt;

    template<class T, mfxU32 K>
    struct TaskItWrap
    {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;
        using difference_type = ptrdiff_t;
        using iterator_type = TaskItWrap;

        TaskItWrap(TTaskIt _it) : it(_it) {}

        bool operator==(const TTaskIt& other) const
        {
            return it == other;
        }
        bool operator!=(const TTaskIt& other) const
        {
            return it != other;
        }
        bool operator==(const iterator_type& other) const
        {
            return it == other.it;
        }
        bool operator!=(const iterator_type& other) const
        {
            return it != other.it;
        }
        pointer operator->()
        {
            return &it->Write<value_type>(K);
        }
        reference operator*() const
        {
            return it->Write<value_type>(K);
        }
        iterator_type& operator++()
        {
            ++it; return *this;
        }
        iterator_type operator++(int)
        {
            iterator_type tmp(it); ++it; return tmp;
        }
        iterator_type& operator--()
        {
            --it; return *this;
        }
        iterator_type operator--(int)
        {
            iterator_type tmp(it); --it; return tmp;
        }

        TTaskIt it;
    };

    struct Reorderer
        : CallChain<TTaskIt, const DpbArray&, TTaskIt, TTaskIt, bool>
    {
        mfxU16 BufferSize = 0;
        mfxU16 MaxReorder = 0;
        NotNull<DpbArray*> DPB;

        using TBaseCC = CallChain<TTaskIt, const DpbArray&, TTaskIt, TTaskIt, bool>;

        TTaskIt operator()(TTaskIt itBegin, TTaskIt itEnd, bool bFlush)
        {
            return TBaseCC::operator()((const DpbArray&)*DPB, itBegin, itEnd, bFlush);
        }
    };

    class EncodeCapsHevc
        : public ENCODE_CAPS_HEVC
        , public Storable
    {
    public:
        EncodeCapsHevc()
            : ENCODE_CAPS_HEVC({})
        {}

        struct MsdkExt
        {
            bool bSingleSliceMultiTile = false;
            bool CQPSupport            = false;
            bool CBRSupport            = false;
            bool VBRSupport            = false;
            bool ICQSupport            = false;
            bool PSliceSupport         = false;
        } msdk;
    };

    struct Defaults
    {
        struct Param
        {
            Param(
                const mfxVideoParam& _par
                , const EncodeCapsHevc& _caps
                , eMFXHWType _hw
                , const Defaults& _d)
                : mvp(_par)
                , caps(_caps)
                , hw(_hw)
                , base(_d)
            {}
            const mfxVideoParam&    mvp;
            const EncodeCapsHevc&   caps;
            eMFXHWType              hw;
            const Defaults&         base;
        };

        template<class TRV>
        using TChain = CallChain<TRV, const Param&>;
        std::map<mfxU32, bool> SetForFeature;

        TChain<mfxU16> GetCodedPicWidth;
        TChain<mfxU16> GetCodedPicHeight;
        TChain<mfxU16> GetCodedPicAlignment;
        TChain<mfxU16> GetMaxDPB;
        TChain<mfxU16> GetNumTemporalLayers;
        TChain<mfxU16> GetGopPicSize;
        TChain<mfxU16> GetGopRefDist;
        TChain<mfxU16> GetNumBPyramidLayers;
        TChain<mfxU16> GetNumRefFrames;
        TChain<mfxU16> GetNumRefBPyramid;
        TChain<mfxU16> GetNumRefPPyramid;
        TChain<mfxU16> GetNumRefNoPyramid;
        TChain<mfxU16> GetMinRefForBPyramid;
        TChain<mfxU16> GetMinRefForBNoPyramid;
        TChain<mfxU16> GetBRefType;
        TChain<mfxU16> GetPRefType;
        TChain<mfxU16> GetTargetBitDepthLuma;
        TChain<mfxU16> GetTargetChromaFormat;
        TChain<mfxU16> GetMaxBitDepth;
        TChain<mfxU16> GetMaxBitDepthByFourCC;
        TChain<mfxU16> GetMaxChroma;
        TChain<mfxU16> GetMaxChromaByFourCC;
        TChain<mfxU16> GetRateControlMethod;
        TChain<mfxU16> GetProfile;
        TChain<mfxU16> GetMBBRC;
        TChain<mfxU16> GetAsyncDepth;
        TChain<mfxU16> GetNumSlices;
        TChain<mfxU16> GetLCUSize;
        TChain<bool>   GetHRDConformanceON;
        TChain<mfxU16> GetPicTimingSEI;
        TChain<mfxU16> GetPPyrInterval;
        TChain<mfxU32> GetTargetKbps;
        TChain<mfxU32> GetMaxKbps;
        TChain<mfxU32> GetBufferSizeInKB;
        TChain<std::tuple<mfxU16, mfxU16>> GetNumTiles; // (NumTileColumns, NumTileRows)
        TChain<std::tuple<mfxU16, mfxU16, mfxU16>> GetMaxNumRef;
        TChain<std::tuple<mfxU32, mfxU32>> GetFrameRate;
        TChain<std::tuple<mfxU16, mfxU16, mfxU16>> GetQPMFX; //I,P,B
        TChain<mfxU8> GetMinQPMFX;
        TChain<mfxU8> GetMaxQPMFX;
        TChain<mfxU8> GetHighestTId;
        TChain<mfxU8> GetNumReorderFrames;
        TChain<bool>  GetNonStdReordering;

        using TGetNumRefActive = CallChain<
            bool //bExternal
            , const Defaults::Param&
            , mfxU16(*)[8] //P
            , mfxU16(*)[8] //BL0
            , mfxU16(*)[8]>; //BL1
        TGetNumRefActive GetNumRefActive;

        using TGetQPOffset = CallChain<
            mfxU16 //EnableQPOffset
            , const Defaults::Param&
            , mfxI16(*)[8]>;//QPOffset
        TGetQPOffset GetQPOffset;

        using TGetGUID = CallChain<
            bool //bSupported
            , const Defaults::Param&
            , GUID&>;
        TGetGUID GetGUID;

        using TGetFrameType = CallChain<
            mfxU16 // FrameType
            , const Defaults::Param&
            , mfxU32//DisplayOrder
            , mfxU32>; //LastIDR
        TGetFrameType GetFrameType;

        using TGetPLayer = CallChain<
            mfxU8 //layer
            , const Defaults::Param&
            , mfxU32>; //order
        TGetPLayer GetPLayer;
        using TGetTId = TGetPLayer;
        TGetTId GetTId;

        using TGetRPLFromExt = CallChain<
            std::tuple<mfxU8, mfxU8> //nL0, nL1
            , const Defaults::Param&
            , const DpbArray &
            , mfxU16, mfxU16 //maxL0, maxL1
            , const mfxExtAVCRefLists&
            , mfxU8(&)[2][MAX_DPB_SIZE]>; //out RPL
        TGetRPLFromExt GetRPLFromExt;

        using TGetRPL = CallChain<
            std::tuple<mfxU8, mfxU8> //nL0, nL1
            , const Defaults::Param&
            , const DpbArray &
            , mfxU16, mfxU16 //maxL0, maxL1
            , const FrameBaseInfo&
            , mfxU8(&)[2][MAX_DPB_SIZE]>; //out RPL
        TGetRPL GetRPL;
        TGetRPL GetRPLMod; // apply hw/other specific modifications to RPL (final)

        using TGetRPLFromCtrl = CallChain<
            std::tuple<mfxU8, mfxU8> //nL0, nL1
            , const Defaults::Param&
            , const DpbArray &
            , mfxU16, mfxU16 //maxL0, maxL1
            , const FrameBaseInfo&
            , const mfxExtAVCRefListCtrl&
            , mfxU8(&)[2][MAX_DPB_SIZE]>; //out RPL
        TGetRPLFromCtrl GetRPLFromCtrl;

        using TCmpRef = CallChain<
            bool                      // refA > refB
            , const Defaults::Param&
            , const FrameBaseInfo&    // curFrame
            , const FrameBaseInfo&    // refA
            , const FrameBaseInfo&>;  // refB
        TCmpRef CmpRefLX[2];
        using TGetWeakRef = CallChain<
            const DpbFrame*
            , const Defaults::Param&
            , const FrameBaseInfo&    // curFrame
            , const DpbFrame*         // begin
            , const DpbFrame*>;       // end
        TGetWeakRef GetWeakRef;

        using TGetPreReorderInfo = CallChain<
            mfxStatus
            , const Defaults::Param&
            , FrameBaseInfo&
            , const mfxFrameSurface1*   //pSurfIn
            , const mfxEncodeCtrl*      //pCtrl
            , mfxU32                    //prevIDROrder
            , mfxI32                    //prevIPOC
            , mfxU32>;                  //frameOrder
        TGetPreReorderInfo GetPreReorderInfo; // fill all info available at pre-reorder

        using TGetFrameNumRefActive = CallChain<
            std::tuple<mfxU8,mfxU8>
            , const Defaults::Param&
            , const FrameBaseInfo&>;
        TGetFrameNumRefActive GetFrameNumRefActive;

        using TGetTileSlices = CallChain<
            mfxU16
            , const Defaults::Param&
            , std::vector<SliceInfo>&
            , mfxU32   //SliceStructure
            , mfxU32   //nCol
            , mfxU32   //nRow
            , mfxU32>; //nSlice
        TGetTileSlices GetTileSlices;

        using TGetSlices = CallChain <
            mfxU16
            , const Defaults::Param&
            , std::vector<SliceInfo>&>;
        TGetSlices GetSlices;

        using TGetVPS = CallChain<mfxStatus, const Defaults::Param&, VPS&>;
        TGetVPS GetVPS;
        using TGetSPS = CallChain<mfxStatus, const Defaults::Param&, const VPS&, SPS&>;
        TGetSPS GetSPS;
        using TGetPPS = CallChain<mfxStatus, const Defaults::Param&, const SPS&, PPS&>;
        TGetPPS GetPPS;

        using TGetSHNUT = CallChain<
            mfxU8                       //NUT
            , const Defaults::Param&
            , const TaskCommonPar&      //task
            , bool>;                    //RAPIntra
        TGetSHNUT GetSHNUT;
        using TGetFrameOrder = CallChain<
            mfxU32                 //FrameOrder
            , const Defaults::Param&
            , const StorageR&      //task
            , mfxU32>;             //prevFrameOrder
        TGetFrameOrder GetFrameOrder;

        using TRunFastCopyWrapper = CallChain<
            mfxStatus
            , mfxFrameSurface1 & /* surfDst */
            , mfxU16 /* dstMemType */
            , mfxFrameSurface1 & /* surfSrc */
            , mfxU16 /* srcMemType */
        >;
        TRunFastCopyWrapper RunFastCopyWrapper;

        //for Query w/o caps:
        using TPreCheck = CallChain<mfxStatus, const mfxVideoParam&>;
        TPreCheck PreCheckCodecId;
        TPreCheck PreCheckChromaFormat;

        template<class TRV>
        using TGetHWDefault = CallChain <TRV, const mfxVideoParam&, eMFXHWType>;
        TGetHWDefault<mfxU16> GetLowPower;

        //for Query w/caps (check + fix)
        using TCheckAndFix = CallChain<mfxStatus, const Defaults::Param&, mfxVideoParam&>;
        TCheckAndFix CheckLCUSize;
        TCheckAndFix CheckLowDelayBRC;
        TCheckAndFix CheckLevel;
        TCheckAndFix CheckSurfSize;
        TCheckAndFix CheckProfile;
        TCheckAndFix CheckFourCC;
        TCheckAndFix CheckInputFormatByFourCC;
        TCheckAndFix CheckTargetChromaFormat;
        TCheckAndFix CheckTargetBitDepth;
        TCheckAndFix CheckFourCCByTargetFormat;
        TCheckAndFix CheckWinBRC;
        TCheckAndFix CheckSAO;
        TCheckAndFix CheckNumRefActive;
    };

    enum eFeatureId
    {
          FEATURE_LEGACY = 0
        , FEATURE_DDI
        , FEATURE_DDI_PACKER
        , FEATURE_PARSER
        , FEATURE_HRD
        , FEATURE_ALLOCATOR
        , FEATURE_TASK_MANAGER
        , FEATURE_PACKER
        , FEATURE_BLOCKING_SYNC
        , FEATURE_EXT_BRC
        , FEATURE_DIRTY_RECT
        , FEATURE_DPB_REPORT
        , FEATURE_DUMP_FILES
        , FEATURE_EXTDDI
        , FEATURE_HDR_SEI
        , FEATURE_MAX_FRAME_SIZE
        , FEATURE_PROTECTED
        , FEATURE_GPU_HANG
        , FEATURE_ROI
        , FEATURE_INTERLACE
        , FEATURE_BRC_SLIDING_WINDOW
        , FEATURE_MB_PER_SEC
        , FEATURE_ENCODED_FRAME_INFO
        , FEATURE_WEIGHTPRED
        , FEATURE_QMATRIX
        , FEATURE_UNITS_INFO
        , FEATURE_FEI
        , FEATURE_RECON_INFO
        , NUM_FEATURES
    };

    enum eStages
    {
        S_NEW = 0
        , S_PREPARE
        , S_REORDER
        , S_SUBMIT
        , S_QUERY
        , NUM_STAGES
    };

    struct PriorityParam
    {
        mfxU32        m_MaxContextPriority;
    };

    struct Glob
    {
        static const StorageR::TKey _KD = __LINE__ + 1;
        using VideoCore           = StorageVar<__LINE__ - _KD, VideoCORE>;
        using RTErr               = StorageVar<__LINE__ - _KD, mfxStatus>;
        using GUID                = StorageVar<__LINE__ - _KD, ::GUID>;
        using EncodeCaps          = StorageVar<__LINE__ - _KD, EncodeCapsHevc>;
        using VideoParam          = StorageVar<__LINE__ - _KD, ExtBuffer::Param<mfxVideoParam>>;
        using VPS                 = StorageVar<__LINE__ - _KD, Base::VPS>;
        using SPS                 = StorageVar<__LINE__ - _KD, Base::SPS>;
        using PPS                 = StorageVar<__LINE__ - _KD, Base::PPS>;
        using SliceInfo           = StorageVar<__LINE__ - _KD, std::vector<Base::SliceInfo>>;
        using AllocRaw            = StorageVar<__LINE__ - _KD, IAllocation>;
        using AllocOpq            = StorageVar<__LINE__ - _KD, IAllocation>;
        using AllocRec            = StorageVar<__LINE__ - _KD, IAllocation>;
        using AllocBS             = StorageVar<__LINE__ - _KD, IAllocation>;
        using AllocMBQP           = StorageVar<__LINE__ - _KD, IAllocation>;
        using PackedHeaders       = StorageVar<__LINE__ - _KD, Base::PackedHeaders>;
        using DDI_Resources       = StorageVar<__LINE__ - _KD, std::list<DDIExecParam>>;
        using DDI_SubmitParam     = StorageVar<__LINE__ - _KD, std::list<DDIExecParam>>;
        using DDI_Feedback        = StorageVar<__LINE__ - _KD, DDIFeedbackParam>;
        using DDI_Execute         = StorageVar<__LINE__ - _KD, CallChain<mfxStatus, const DDIExecParam&>>;
        using RealState           = StorageVar<__LINE__ - _KD, StorageW>;//available during Reset
        using ResetHint           = StorageVar<__LINE__ - _KD, Base::ResetHint>; //available during Reset
        using Reorder             = StorageVar<__LINE__ - _KD, Reorderer>;
        using NeedRextConstraints = StorageVar<__LINE__ - _KD, std::function<bool(const Base::PTL&)>>;
        using ReadSpsExt          = StorageVar<__LINE__ - _KD, std::function<bool(const Base::SPS&, mfxU8, IBsReader&)>>;
        using ReadPpsExt          = StorageVar<__LINE__ - _KD, std::function<bool(const Base::PPS&, mfxU8, IBsReader&)>>;
        using PackSpsExt          = StorageVar<__LINE__ - _KD, std::function<bool(const Base::SPS&, mfxU8, IBsWriter&)>>;
        using PackPpsExt          = StorageVar<__LINE__ - _KD, std::function<bool(const Base::PPS&, mfxU8, IBsWriter&)>>;
        using GuidToVa            = StorageVar<__LINE__ - _KD, std::map<::GUID, VAGUID, LessGUID>>;
        using Defaults            = StorageVar<__LINE__ - _KD, Base::Defaults>;
        using PriorityPar         = StorageVar<__LINE__ - _KD, Base::PriorityParam>;
        static const StorageR::TKey ReservedKey0 = __LINE__ - _KD;
        static const StorageR::TKey NUM_KEYS = __LINE__ - _KD;
    };

    struct Tmp
    {
        static const StorageR::TKey _KD = __LINE__ + 1;
        using MakeAlloc     = StorageVar<__LINE__ - _KD, std::function<IAllocation*(VideoCORE&)>>;
        using BSAllocInfo   = StorageVar<__LINE__ - _KD, mfxFrameAllocRequest>;
        using MBQPAllocInfo = StorageVar<__LINE__ - _KD, mfxFrameAllocRequest>;
        using RecInfo       = StorageVar<__LINE__ - _KD, mfxFrameAllocRequest>;
        using RawInfo       = StorageVar<__LINE__ - _KD, mfxFrameAllocRequest>;
        using CurrTask      = StorageVar<__LINE__ - _KD, StorageW>;
        using BsDataInfo    = StorageVar<__LINE__ - _KD, Base::BsDataInfo>;
        using DDI_InitParam = StorageVar<__LINE__ - _KD, std::list<DDIExecParam>>;
        static const StorageR::TKey NUM_KEYS = __LINE__ - _KD;
    };

    struct Task
    {
        static const StorageR::TKey _KD = __LINE__ + 1;
        using Common = StorageVar<__LINE__ - _KD, TaskCommonPar>;
        using SSH    = StorageVar<__LINE__ - _KD, Base::Slice>;
        static const StorageR::TKey ReservedKey0 = __LINE__ - _KD;
        static const StorageR::TKey NUM_KEYS = __LINE__ - _KD;
    };

    template<class TEB>
    inline const TEB& GetRTExtBuffer(const StorageR& glob, const StorageR& task)
    {
        const TEB* pEB = ExtBuffer::Get(Task::Common::Get(task).ctrl);
        if (pEB)
            return *pEB;
        return ExtBuffer::Get(Glob::VideoParam::Get(glob));
    }

} //namespace Base
} //namespace HEVCEHW

#endif
