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

#ifndef __HEVC_STRUCT_H
#define __HEVC_STRUCT_H

#include <bs_def.h>

typedef class HEVC_BitStream* BS_HEVC_hdl;

namespace BS_HEVC{

enum INIT_MODE {
    INIT_MODE_DEFAULT        = 0,
    INIT_MODE_CABAC          = 1,
    INIT_MODE_STORE_SEI_DATA = 2,
};

enum TRACE_LEVEL {
     TRACE_LEVEL_NALU     = 0x00000001,
     TRACE_LEVEL_SPS      = 0x00000002,
     TRACE_LEVEL_PPS      = 0x00000004,
     TRACE_LEVEL_SLICE    = 0x00000008,
     TRACE_LEVEL_SEI      = 0x00000010,
     TRACE_LEVEL_VPS      = 0x00000020,
     TRACE_LEVEL_REF_LIST = 0x00000040,
     TRACE_LEVEL_AU       = 0x00000080,
     TRACE_LEVEL_CABAC    = 0x00000100,
     TRACE_LEVEL_FULL     = 0xFFFFFFFF
};

enum NALU_TYPE{
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

enum PROFILE {
    MAIN     = 1,
    MAIN_10  = 2,
    MAIN_SP  = 3,
    REXT     = 4,
    REXT_HT  = 5,
    MAIN_MV  = 6,
    MAIN_SC  = 7,
    MAIN_3D  = 8,
    SCC      = 9,
};

enum ASPECT_RATIO{
    AR_Unspecified = 0,
    AR_1_1,
    AR_SQUARE = AR_1_1,
    AR_12_11,
    AR_10_11,
    AR_16_11,
    AR_40_33,
    AR_24_11,
    AR_20_11,
    AR_32_11,
    AR_80_33,
    AR_18_11,
    AR_15_11,
    AR_64_33,
    AR_160_99,
    AR_4_3,
    AR_3_2,
    AR_2_1,
    //Reserved
    EXTENDED_SAR = 255,
    num_ASPECT_RATIO
};

enum HASH_TYPE{
    HASH_MD5 = 0,
    HASH_CRC,
    HASH_CHECKSUM,
    num_HASH_TYPE
};

enum SLICE_TYPE{
    B = 0,
    P,
    I,
    num_SLICE_TYPE
};

enum PICTURE_MARKING{
    UNUSED_FOR_REFERENCE         = 0x00,
    USED_FOR_SHORTTERM_REFERENCE = 0x01,
    USED_FOR_LONGTERM_REFERENCE  = 0x02,
    USED_FOR_REFERENCE           = 0x03,
};

struct PTLEntry{
    Bs8u  profile_space : 2;
    Bs8u  tier_flag     : 1;
    Bs8u  profile_idc   : 5;

    union{
        Bs32u profile_compatibility_flags;
        struct{
            Bs32u profile_compatibility_flag31 : 1;
            Bs32u profile_compatibility_flag30 : 1;
            Bs32u profile_compatibility_flag29 : 1;
            Bs32u profile_compatibility_flag28 : 1;
            Bs32u profile_compatibility_flag27 : 1;
            Bs32u profile_compatibility_flag26 : 1;
            Bs32u profile_compatibility_flag25 : 1;
            Bs32u profile_compatibility_flag24 : 1;
            Bs32u profile_compatibility_flag23 : 1;
            Bs32u profile_compatibility_flag22 : 1;
            Bs32u profile_compatibility_flag21 : 1;
            Bs32u profile_compatibility_flag20 : 1;
            Bs32u profile_compatibility_flag19 : 1;
            Bs32u profile_compatibility_flag18 : 1;
            Bs32u profile_compatibility_flag17 : 1;
            Bs32u profile_compatibility_flag16 : 1;
            Bs32u profile_compatibility_flag15 : 1;
            Bs32u profile_compatibility_flag14 : 1;
            Bs32u profile_compatibility_flag13 : 1;
            Bs32u profile_compatibility_flag12 : 1;
            Bs32u profile_compatibility_flag11 : 1;
            Bs32u profile_compatibility_flag10 : 1;
            Bs32u profile_compatibility_flag9  : 1;
            Bs32u profile_compatibility_flag8  : 1;
            Bs32u profile_compatibility_flag7  : 1;
            Bs32u profile_compatibility_flag6  : 1;
            Bs32u profile_compatibility_flag5  : 1;
            Bs32u profile_compatibility_flag4  : 1;
            Bs32u profile_compatibility_flag3  : 1;
            Bs32u profile_compatibility_flag2  : 1;
            Bs32u profile_compatibility_flag1  : 1;
            Bs32u profile_compatibility_flag0  : 1;
        };
    };

    Bs8u  progressive_source_flag    : 1;
    Bs8u  interlaced_source_flag     : 1;
    Bs8u  non_packed_constraint_flag : 1;
    Bs8u  frame_only_constraint_flag : 1;
    Bs8u  profile_present_flag       : 1;
    Bs8u  level_present_flag         : 1;
    Bs8u  level_idc;
};

struct ProfileTierLevel{
    PTLEntry general;
    PTLEntry sub_layer[8];
};

struct sub_layer_ordering_info{
    Bs8u  max_dec_pic_buffering_minus1 : 4;
    Bs8u  max_num_reorder_pics         : 4;
    Bs32u max_latency_increase_plus1;
};

struct SubLayerHRD{
    Bs8u  fixed_pic_rate_general_flag    : 1;
    Bs8u  fixed_pic_rate_within_cvs_flag : 1;
    Bs8u  low_delay_hrd_flag             : 1;
    Bs16u elemental_duration_in_tc_minus1 : 11;
    Bs16u cpb_cnt_minus1                  :  5;

    struct{
        Bs32u bit_rate_value_minus1    : 31;
        Bs32u cpb_size_value_minus1    : 31;
        Bs32u cpb_size_du_value_minus1 : 31;
        Bs32u bit_rate_du_value_minus1 : 31;
        Bs32u cbr_flag                 :  1;
    }cpb[32];
};

struct HRD{
    Bs8u  nal_hrd_parameters_present_flag : 1;
    Bs8u  vcl_hrd_parameters_present_flag : 1;
    Bs8u  sub_pic_hrd_params_present_flag : 1;

    Bs8u  tick_divisor_minus2;
    Bs16u du_cpb_removal_delay_increment_length_minus1 : 5;
    Bs16u sub_pic_cpb_params_in_pic_timing_sei_flag    : 1;
    Bs16u dpb_output_delay_du_length_minus1            : 5;

    Bs16u bit_rate_scale : 4;
    Bs8u  cpb_size_scale : 4;

    Bs8u  cpb_size_du_scale : 4;
    Bs16u initial_cpb_removal_delay_length_minus1 : 5;
    Bs16u au_cpb_removal_delay_length_minus1      : 5;
    Bs16u dpb_output_delay_length_minus1          : 5;

    SubLayerHRD sl[8];
};

struct VPSHRD : HRD{
    Bs16u hrd_layer_set_idx  : 10;
    Bs16u cprms_present_flag :  1;
};

struct VPS{
    Bs16u video_parameter_set_id   : 4;
    Bs16u reserved_three_2bits     : 2;
    Bs16u max_layers_minus1        : 6;
    Bs16u max_sub_layers_minus1    : 3;
    Bs16u temporal_id_nesting_flag : 1;
    Bs16u reserved_0xffff_16bits;

    ProfileTierLevel ptl;

    Bs8u sub_layer_ordering_info_present_flag : 1;

    sub_layer_ordering_info sub_layer[8];

    Bs16u max_layer_id          :  6;
    Bs16u num_layer_sets_minus1 : 10;

    Bs8u** layer_id_included_flag; // max [1024][64];

    Bs32u num_units_in_tick;
    Bs32u time_scale;
    Bs32u timing_info_present_flag        : 1;
    Bs32u poc_proportional_to_timing_flag : 1;
    Bs32u num_ticks_poc_diff_one_minus1 : 10;
    Bs32u num_hrd_parameters            : 10;

    Bs8u extension_flag                  : 1;
    Bs8u extension_data_flag             : 1;

    VPSHRD* hrd; //max 1024
};

struct ScalingListData{
    struct{
        Bs16s pred_mode_flag       :  2;
        Bs16s pred_matrix_id_delta :  4;
        Bs16s dc_coef_minus8       : 10;
        Bs8s  delta_coef[64];
    }entry[4][6];
    Bs16s ScalingList[4][6][64];
};

struct ShortTermRefPicSet{
    Bs8u  inter_ref_pic_set_prediction_flag : 1;
    Bs8u  delta_idx_minus1                  : 6;
    Bs8u  delta_rps_sign                    : 1;

    Bs16u abs_delta_rps_minus1;

    Bs8u  num_negative_pics : 4;
    Bs8u  num_positive_pics : 4;

    struct{
        struct{
            Bs8u  used_by_curr_pic_flag;
            Bs8u  use_delta_flag;
        };
        union{
            struct {
                Bs16u delta_poc_s0_minus1      : 15;
                Bs16u used_by_curr_pic_s0_flag : 1;
                Bs16s DeltaPocS0;
            };
            struct {
                Bs16u delta_poc_s1_minus1      : 15;
                Bs16u used_by_curr_pic_s1_flag : 1;
                Bs16s DeltaPocS1;
            };
        };
    }pic[16];
};

struct ShortTermRefPicSets{
    Bs8u num_short_term_ref_pic_sets; //0-64
    ShortTermRefPicSet* set;
};

struct VUI{
    Bs8u  aspect_ratio_info_present_flag : 1;
    Bs8u  aspect_ratio_idc;
    Bs16u sar_width;
    Bs16u sar_height;

    Bs8u  overscan_info_present_flag      : 1;
    Bs8u  overscan_appropriate_flag       : 1;
    Bs8u  video_signal_type_present_flag  : 1;
    Bs8u  video_format                    : 3;
    Bs8u  video_full_range_flag           : 1;
    Bs8u  colour_description_present_flag : 1;
    Bs8u  colour_primaries;
    Bs8u  transfer_characteristics;
    Bs8u  matrix_coeffs;

    Bs8u  chroma_loc_info_present_flag        : 1;
    Bs8u  chroma_sample_loc_type_top_field    : 3;
    Bs8u  chroma_sample_loc_type_bottom_field : 3;

    Bs8u  neutral_chroma_indication_flag : 1;
    Bs8u  field_seq_flag                 : 1;
    Bs8u  frame_field_info_present_flag  : 1;
    Bs8u  default_display_window_flag    : 1;

    Bs32u def_disp_win_left_offset;
    Bs32u def_disp_win_right_offset;
    Bs32u def_disp_win_top_offset;
    Bs32u def_disp_win_bottom_offset;

    Bs8u  timing_info_present_flag        : 1;
    Bs8u  hrd_parameters_present_flag     : 1;
    Bs8u  poc_proportional_to_timing_flag : 1;

    Bs8u  bitstream_restriction_flag              : 1;
    Bs8u  tiles_fixed_structure_flag              : 1;
    Bs8u  motion_vectors_over_pic_boundaries_flag : 1;
    Bs8u  restricted_ref_pic_lists_flag           : 1;

    Bs32u num_units_in_tick;
    Bs32u time_scale;
    Bs32u num_ticks_poc_diff_one_minus1;

    Bs32u min_spatial_segmentation_idc : 12;
    Bs32u max_bytes_per_pic_denom      :  5;
    Bs32u max_bits_per_min_cu_denom    :  5;
    Bs16u log2_max_mv_length_horizontal : 5;
    Bs16u log2_max_mv_length_vertical   : 4;

    HRD hrd;
};

struct SpsRangeExtension{
    Bs8u transform_skip_rotation_enabled_flag : 1;
    Bs8u transform_skip_context_enabled_flag  : 1;
    Bs8u implicit_residual_dpcm_enabled_flag  : 1;
    Bs8u explicit_residual_dpcm_enabled_flag  : 1;
    Bs8u extended_precision_processing_flag   : 1;
    Bs8u intra_smoothing_disabled_flag        : 1;
    Bs8u high_precision_offsets_enabled_flag  : 1;
    Bs8u fast_rice_adaptation_enabled_flag    : 1;
    Bs8u cabac_bypass_alignment_enabled_flag  : 1;
};

struct SpsSccExtension{
    Bs8u  sps_curr_pic_ref_enabled_flag : 1;
    Bs8u  palette_mode_enabled_flag     : 1;

    Bs32u max_bits_ppalette_max_size;
    Bs32u delta_palette_max_predictor_size;

    Bs8u  sps_palette_predictor_initializer_present_flag : 1;

    Bs32u sps_num_palette_predictor_initializer_minus1;

    Bs8u  motion_vector_resolution_control_idc   : 2;
    Bs8u  intra_boundary_filtering_disabled_flag : 1;

    Bs32u* paletteInitializers;
};

struct SPS{
    Bs8u  video_parameter_set_id   : 4;
    Bs8u  max_sub_layers_minus1    : 3;
    Bs8u  temporal_id_nesting_flag : 1;

    ProfileTierLevel ptl;

    Bs8u  seq_parameter_set_id       : 4;
    Bs8u  chroma_format_idc          : 2;
    Bs8u  separate_colour_plane_flag : 1;
    Bs8u  conformance_window_flag    : 1;

    Bs32u pic_width_in_luma_samples;
    Bs32u pic_height_in_luma_samples;

    Bs32u conf_win_left_offset;
    Bs32u conf_win_right_offset;
    Bs32u conf_win_top_offset;
    Bs32u conf_win_bottom_offset;

    Bs8u  bit_depth_luma_minus8                : 3;
    Bs8u  bit_depth_chroma_minus8              : 3;
    Bs8u  log2_max_pic_order_cnt_lsb_minus4    : 4;
    Bs8u  sub_layer_ordering_info_present_flag : 1;

    sub_layer_ordering_info sub_layer[8];

    Bs32u log2_min_luma_coding_block_size_minus3;
    Bs32u log2_diff_max_min_luma_coding_block_size;
    Bs32u log2_min_transform_block_size_minus2;
    Bs32u log2_diff_max_min_transform_block_size;
    Bs32u max_transform_hierarchy_depth_inter;
    Bs32u max_transform_hierarchy_depth_intra;

    Bs8u  scaling_list_enabled_flag      : 1;
    Bs8u  scaling_list_data_present_flag : 1;

    Bs8u  amp_enabled_flag                    : 1;
    Bs8u  sample_adaptive_offset_enabled_flag : 1;

    Bs8u  pcm_enabled_flag                    : 1;
    Bs8u  pcm_loop_filter_disabled_flag       : 1;
    Bs8u  pcm_sample_bit_depth_luma_minus1    : 4;
    Bs8u  pcm_sample_bit_depth_chroma_minus1  : 4;
    Bs32u log2_min_pcm_luma_coding_block_size_minus3;
    Bs32u log2_diff_max_min_pcm_luma_coding_block_size;

    ScalingListData* sld;

    ShortTermRefPicSets strps;

    Bs8u  long_term_ref_pics_present_flag : 1;
    Bs8u  num_long_term_ref_pics_sps      : 6;

    Bs16u lt_ref_pic_poc_lsb_sps[32];
    Bs8u  used_by_curr_pic_lt_sps_flag[32];

    Bs8u  temporal_mvp_enabled_flag           : 1;
    Bs8u  strong_intra_smoothing_enabled_flag : 1;
    Bs8u  vui_parameters_present_flag         : 1;
    Bs8u  extension_flag                      : 1;
    Bs8u  sps_range_extension_flag            : 1;
    Bs8u  sps_scc_extension_flag              : 1;

    VUI vui;
    SpsRangeExtension range_extension;
    SpsSccExtension   scc_extension;
};

struct PpsRangeExtension{
    Bs32u  log2_max_transform_skip_block_size_minus2;
    Bs8u   cross_component_prediction_enabled_flag;

    Bs8u   chroma_qp_offset_list_enabled_flag;
    Bs8u   diff_cu_chroma_qp_offset_depth;

    Bs32u  chroma_qp_offset_list_len_minus1;
    Bs32u* cb_qp_offset_list;
    Bs32u* cr_qp_offset_list;

    Bs32u  log2_sao_offset_scale_luma;
    Bs32u  log2_sao_offset_scale_chroma;
};

struct PpsSccExtension{
    Bs8u   pps_curr_pic_ref_enabled_flag;
    Bs8u   residual_adaptive_colour_transform_enabled_flag;

    Bs8u   pps_slice_act_qp_offsets_present_flag;
    Bs32u  pps_act_y_qp_offset_plus5;
    Bs32u  pps_act_cb_qp_offset_plus5;
    Bs32u  pps_act_cr_qp_offset_plus3;

    Bs8u   pps_palette_predictor_initializer_present_flag;

    Bs32u  pps_num_palette_predictor_initializer;

    Bs8u   monochrome_palette_flag;
    Bs32u  luma_bit_depth_entry_minus8;
    Bs32u  chroma_bit_depth_entry_minus8;

    Bs32u* paletteInitializers;
};

struct PPS{
    Bs16u pic_parameter_set_id : 6;
    Bs16u seq_parameter_set_id : 4;
    Bs16u dependent_slice_segments_enabled_flag : 1;
    Bs16u output_flag_present_flag              : 1;
    Bs16u num_extra_slice_header_bits           : 3;
    Bs16u sign_data_hiding_enabled_flag         : 1;
    Bs16u cabac_init_present_flag               : 1;
    Bs16u num_ref_idx_l0_default_active_minus1  : 4;
    Bs16u num_ref_idx_l1_default_active_minus1  : 4;
    Bs16u constrained_intra_pred_flag           : 1;
    Bs16u transform_skip_enabled_flag           : 1;
    Bs16u cu_qp_delta_enabled_flag              : 1;
    Bs16u slice_segment_header_extension_present_flag : 1;

    Bs32u diff_cu_qp_delta_depth;
    Bs8s  init_qp_minus26 : 6;
    Bs16s cb_qp_offset : 6;
    Bs16s cr_qp_offset : 6;

    Bs8u  slice_chroma_qp_offsets_present_flag  : 1;
    Bs8u  weighted_pred_flag                    : 1;
    Bs8u  weighted_bipred_flag                  : 1;
    Bs8u  transquant_bypass_enabled_flag        : 1;
    Bs8u  tiles_enabled_flag                    : 1;
    Bs8u  entropy_coding_sync_enabled_flag      : 1;
    Bs8u  uniform_spacing_flag                  : 1;
    Bs8u  loop_filter_across_tiles_enabled_flag : 1;

    Bs16u num_tile_columns_minus1;
    Bs16u num_tile_rows_minus1;

    Bs16u* column_width_minus1;
    Bs16u* row_height_minus1;

    Bs8u  loop_filter_across_slices_enabled_flag  : 1;
    Bs8u  deblocking_filter_control_present_flag  : 1;
    Bs8u  deblocking_filter_override_enabled_flag : 1;
    Bs8u  deblocking_filter_disabled_flag         : 1;
    Bs8u  scaling_list_data_present_flag          : 1;
    Bs8u  lists_modification_present_flag         : 1;
    Bs8u  extension_flag                          : 1;
    Bs8u  range_extension_flag                    : 1;
    Bs8u  pps_scc_extension_flag                  : 1;

    Bs8s  beta_offset_div2 : 4;
    Bs8s  tc_offset_div2   : 4;

    ScalingListData* sld;

    Bs16u log2_parallel_merge_level_minus2;

    PpsRangeExtension range_extension;
    PpsSccExtension   scc_extension;
};

struct BufferingPeriod{
    Bs8u  seq_parameter_set_id         : 4;
    Bs8u  irap_cpb_params_present_flag : 1;
    Bs8u  concatenation_flag           : 1;

    Bs32u cpb_delay_offset;
    Bs32u dpb_delay_offset;
    Bs32u au_cpb_removal_delay_delta_minus1;

    struct CPB{
        Bs32u initial_cpb_removal_delay;
        Bs32u initial_cpb_removal_offset;
        Bs32u initial_alt_cpb_removal_delay;
        Bs32u initial_alt_cpb_removal_offset;
    } nal[32], vcl[32];
};

struct PicTiming{
    Bs8u  pic_struct                       : 4;
    Bs8u  source_scan_type                 : 2;
    Bs8u  duplicate_flag                   : 1;
    Bs8u  du_common_cpb_removal_delay_flag : 1;

    Bs32u au_cpb_removal_delay_minus1;
    Bs32u pic_dpb_output_delay;
    Bs32u pic_dpb_output_du_delay;
    Bs32u num_decoding_units_minus1;
    Bs32u du_common_cpb_removal_delay_increment_minus1;

    Bs32u *num_nalus_in_du_minus1;
    Bs32u *du_cpb_removal_delay_increment_minus1;

    //////////////////////////
    VUI* vui;
};

struct ActiveParameterSets{
    Bs16u active_video_parameter_set_id : 4;
    Bs16u self_contained_cvs_flag       : 1;
    Bs16u no_parameter_set_update_flag  : 1;
    Bs16u num_sps_ids_minus1            : 4;

    Bs8u  active_seq_parameter_set_id[16];
};

struct RecoveryPoint{
    Bs32s recovery_poc_cnt;
    Bs8u  exact_match_flag : 1;
    Bs8u  broken_link_flag : 1;
};

struct DecodedPictureHash{
    Bs8u  hash_type;
    union ColorPlane{
        Bs8u  picture_md5[16];
        Bs16u picture_crc;
        Bs32u picture_checksum;
    }plane[3];
};

static const Bs16u MaxSEIPayloads = 64;

struct SEI{
    Bs16u NumMessage;

    struct MSG{
        Bs32u payloadType;
        Bs32u payloadSize;

        union {
            void                *payload;
            ActiveParameterSets *aps;
            BufferingPeriod     *bp;
            PicTiming           *pt;
            RecoveryPoint       *rp;
            DecodedPictureHash  *dph;
            Bs8u                *rawData;
        };

    } message[MaxSEIPayloads];
};

struct AUD{
    Bs8u pic_type : 3;
};

struct LongTerm{
    Bs8u  lt_idx_sps                 :  5;
    Bs8u  used_by_curr_pic_lt_flag   :  1;
    Bs8u  delta_poc_msb_present_flag :  1;
    Bs32u poc_lsb_lt;
    Bs32u delta_poc_msb_cycle_lt;
};

struct RefPicListsMod{
    Bs8u  ref_pic_list_modification_flag_l0 : 1;
    Bs8u  ref_pic_list_modification_flag_l1 : 1;
    Bs16u list_entry_l0[16];
    Bs16u list_entry_l1[16];
};

struct PredWeightTable{
    Bs8u  luma_log2_weight_denom         : 3;
    Bs8s  delta_chroma_log2_weight_denom : 4;

    struct {
        Bs8u  luma_weight_lx_flag   : 1;
        Bs8u  chroma_weight_lx_flag : 1;
        Bs8s delta_luma_weight_lx;
        Bs8s luma_offset_lx;
        Bs8s delta_chroma_weight_lx[2];
        Bs8s delta_chroma_offset_lx[2];
    } l0[16], l1[16];
};

struct SAO{
    Bs8u  type_idx: 2;
    union{
        Bs8u  band_position : 5;
        Bs8u  eo_class      : 2;
    };
    Bs8s  offset[4];
};

struct Residual{
    Bs8u transform_skip_flag : 1;

    Bs8u last_sig_coeff_x_prefix;
    Bs8u last_sig_coeff_y_prefix;
    Bs8u last_sig_coeff_x_suffix;
    Bs8u last_sig_coeff_y_suffix;

    //rest data skipped for now
    //Bs16u coded_sub_block_flags;
};

struct PredUnit{
    Bs16u x;
    Bs16u y;

    Bs8u merge_idx      : 3;
    Bs8u merge_flag     : 1;
    Bs8u inter_pred_idc : 2;
    Bs8u mvp_l0_flag    : 1;
    Bs8u mvp_l1_flag    : 1;
    Bs8u ref_idx_l0     : 4;
    Bs8u ref_idx_l1     : 4;

    Bs16s MvdLX[2][2];
};

struct TransTree{
    Bs16u x;
    Bs16u y;
    Bs8u QpY;
    Bs8u trafoDepth;
    Bs8s cu_qp_delta;
    Bs8u split_transform_flag : 1;
    Bs8u cbf_cb               : 1;
    Bs8u cbf_cr               : 1;
    Bs8u cbf_luma             : 1;

    //Residual   rc[3];
    TransTree* split;
};

struct CQT{
    Bs16u x;
    Bs16u y;
    Bs16u CtDepth;

    Bs8u  split_cu_flag             : 1;
    Bs8u  cu_transquant_bypass_flag : 1;
    Bs8u  cu_skip_flag              : 1;
    Bs8u  pred_mode_flag            : 1;
    Bs8u  part_mode                 : 3;
    Bs8u  pcm_flag                  : 1;

    struct LumaPred{
        Bs8u prev_intra_luma_pred_flag : 1;
        Bs8u mpm_idx                   : 2;
        Bs8u rem_intra_luma_pred_mode  : 5;
    }luma_pred[2][2];

    Bs8u intra_chroma_pred_mode : 4;
    Bs8u rqt_root_cbf           : 1;

    TransTree* tu;
    PredUnit* pu;
    CQT* split;
};

struct CTU{
    CQT cqt;
    Bs16u CtbAddrInRs;
    Bs16u CtbAddrInTs;
    Bs16u RawBits;

    Bs8u  end_of_slice_segment_flag : 1;
    Bs8u  sao_merge_left_flag : 1;
    Bs8u  sao_merge_up_flag : 1;

    SAO sao[3];
};

struct Slice{
    Bs8u  no_output_of_prior_pics_flag    : 1;
    Bs8u  pic_parameter_set_id            : 6;
    Bs8u  dependent_slice_segment_flag    : 1;

    Bs32u segment_address;

    Bs8u  reserved_flags;
    Bs8u  type                       : 2;
    Bs8u  colour_plane_id            : 2;
    Bs8u  short_term_ref_pic_set_idx;

    Bs8u  pic_output_flag                 : 1;
    Bs8u  short_term_ref_pic_set_sps_flag : 1;
    Bs8u  num_long_term_sps               : 6;

    Bs8u  first_slice_segment_in_pic_flag  : 1;
    Bs8u  temporal_mvp_enabled_flag        : 1;
    Bs8u  sao_luma_flag                    : 1;
    Bs8u  sao_chroma_flag                  : 1;
    Bs8u  num_ref_idx_active_override_flag : 1;
    Bs8u  mvd_l1_zero_flag                 : 1;
    Bs8u  cabac_init_flag                  : 1;
    Bs8u  collocated_from_l0_flag          : 1;

    Bs8u  collocated_ref_idx            : 4;
    Bs8u  five_minus_max_num_merge_cand : 3;
    Bs8u  use_integer_mv_flag           : 1;

    Bs8u  num_ref_idx_l0_active_minus1 : 4;
    Bs8u  num_ref_idx_l1_active_minus1 : 4;

    Bs32u pic_order_cnt_lsb;
    Bs16u num_long_term_pics;

    Bs16s slice_qp_delta     : 6;
    Bs16s slice_cb_qp_offset : 5;
    Bs16s slice_cr_qp_offset : 5;

    Bs16s act_y_qp_offset    : 5;
    Bs16s act_cb_qp_offset   : 5;
    Bs16s act_cr_qp_offset   : 5;

    Bs8u  cu_chroma_qp_offset_enabled_flag       : 1;
    Bs8u  deblocking_filter_override_flag        : 1;
    Bs8u  deblocking_filter_disabled_flag        : 1;
    Bs8u  loop_filter_across_slices_enabled_flag : 1;
    Bs8u  offset_len_minus1                      : 5;

    Bs8s  beta_offset_div2 : 4;
    Bs8s  tc_offset_div2   : 4;

    Bs32u num_entry_point_offsets;

    ShortTermRefPicSet strps;

    LongTerm        *lt;
    RefPicListsMod  *rplm;
    PredWeightTable *pwt;
    Bs32u           *entry_point_offset_minus1;

    //////////////////////////
    Bs16u NumPocTotalCurr;
    Bs32s PicOrderCntVal;

    SPS *sps;
    PPS *pps;

    Bs32u NumCU;
    CTU **CuInTs; //coding units in tile scan
};

struct NALU{
    Bs64u StartOffset;
    Bs32u NumBytesInNalUnit;
    Bs32u NumBytesInRbsp;

    Bs16u forbidden_zero_bit    : 1;
    Bs16u nal_unit_type         : 6;
    Bs16u nuh_layer_id          : 6;
    Bs16u nuh_temporal_id_plus1 : 3;

    union{
        void  *unit;
        VPS   *vps;
        SPS   *sps;
        PPS   *pps;
        SEI   *sei;
        AUD   *aud;
        Slice *slice;
    };
};

struct Picture{
    union {
        Bs8u used; //used for reference
        struct{
            Bs8u st : 1; // used for short-term reference
            Bs8u lt : 1; // used for long-term reference
            Bs8u reserved0 : 1;
            Bs8u reserved1 : 1;
            Bs8u reserved2 : 1;
            Bs8u reserved3 : 1;
            Bs8u reserved4 : 1;
            Bs8u reserved5 : 1;
        };
    };
    Bs8s  TId;
    Bs32s PicOrderCntVal;
    Bs32u pic_order_cnt_lsb;

    Bs32u NumSlice;
    NALU** slice;

    Picture* RefPicList0[16];
    Picture* RefPicList1[16];

    Bs32u NumCU;
    CTU*  CuInRs; //coding units in raster scan
};

struct AU{
    Bs32u       NumUnits;
    NALU**      nalu; //NALU* nalu[NumUnits];
    Picture*    pic;
};

};

#endif //#ifndef __HEVC_STRUCT_H
