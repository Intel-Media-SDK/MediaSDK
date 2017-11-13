// Copyright (c) 2017 Intel Corporation
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
#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#pragma once
#include "mfx_common.h"
#include <bitset>

namespace MfxHwH265Encode
{

enum
{
      MAX_NUM_TILE_COLUMNS   = 20
    , MAX_NUM_TILE_ROWS      = 22
    , MAX_NUM_LONG_TERM_PICS = 8
    , MAX_PIC_HEIGHT_IN_CTBS = 270
    , MAX_NUM_ENTRY_POINT_OFFSETS  = ((MAX_NUM_TILE_COLUMNS*MAX_NUM_TILE_ROWS > MAX_PIC_HEIGHT_IN_CTBS) ? MAX_NUM_TILE_COLUMNS*MAX_NUM_TILE_ROWS : MAX_PIC_HEIGHT_IN_CTBS)
};

enum NALU_TYPE
{
    TRAIL_N = 0,
    TRAIL_R,
    TSA_N,
    TSA_R,
    STSA_N,
    STSA_R,
    RADL_N,
    RADL_R,
    RASL_N,
    RASL_R,
    RSV_VCL_N10,
    RSV_VCL_R11,
    RSV_VCL_N12,
    RSV_VCL_R13,
    RSV_VCL_N14,
    RSV_VCL_R15,
    BLA_W_LP,
    BLA_W_RADL,
    BLA_N_LP,
    IDR_W_RADL,
    IDR_N_LP,
    CRA_NUT,
    RSV_IRAP_VCL22,
    RSV_IRAP_VCL23,
    RSV_VCL24,
    RSV_VCL25,
    RSV_VCL26,
    RSV_VCL27,
    RSV_VCL28,
    RSV_VCL29,
    RSV_VCL30,
    RSV_VCL31,
    VPS_NUT,
    SPS_NUT,
    PPS_NUT,
    AUD_NUT,
    EOS_NUT,
    EOB_NUT,
    FD_NUT,
    PREFIX_SEI_NUT,
    SUFFIX_SEI_NUT,
    RSV_NVCL41,
    RSV_NVCL42,
    RSV_NVCL43,
    RSV_NVCL44,
    RSV_NVCL45,
    RSV_NVCL46,
    RSV_NVCL47,
    UNSPEC48,
    UNSPEC49,
    UNSPEC50,
    UNSPEC51,
    UNSPEC52,
    UNSPEC53,
    UNSPEC54,
    UNSPEC55,
    UNSPEC56,
    UNSPEC57,
    UNSPEC58,
    UNSPEC59,
    UNSPEC60,
    UNSPEC61,
    UNSPEC62,
    UNSPEC63,
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

    mfxU16 abs_delta_rps_minus1;

    mfxU8  num_negative_pics : 4;
    mfxU8  num_positive_pics : 4;

    struct
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


struct VUI{
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

    //ScalingListData* sld;

    mfxU8 num_short_term_ref_pic_sets;
    STRPS strps[65];

    mfxU8  long_term_ref_pics_present_flag : 1;
    mfxU8  num_long_term_ref_pics_sps      : 6;

    mfxU16 lt_ref_pic_poc_lsb_sps[32];
    mfxU8  used_by_curr_pic_lt_sps_flag[32];

    mfxU8  temporal_mvp_enabled_flag           : 1;
    mfxU8  strong_intra_smoothing_enabled_flag : 1;
    mfxU8  vui_parameters_present_flag         : 1;
    mfxU8  extension_flag                      : 1;
    mfxU8  extension_data_flag                 : 1;


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
    mfxI16 init_qp_minus26 : 6;
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

    //mfxU32 entry_point_offset_minus1[MAX_NUM_ENTRY_POINT_OFFSETS];
};

struct NALU
{
    mfxU16 long_start_code       : 1;
    mfxU16 nal_unit_type         : 6;
    mfxU16 nuh_layer_id          : 6;
    mfxU16 nuh_temporal_id_plus1 : 3;
};

} //MfxHwH265Encode
#endif
