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

#pragma once

#include "bs_def.h"

namespace BS_HEVC2
{

typedef class Parser* HDL;

enum INIT_MODE
{
    INIT_MODE_DEFAULT   = 0x00,
    PARSE_SSD           = 0x01,
    PARALLEL_AU         = 0x02,
    PARALLEL_SD         = 0x04,
    PARALLEL_TILES      = 0x08,
    PARSE_SSD_TC        = 0x10 | PARSE_SSD,

    ASYNC               = (PARALLEL_AU | PARALLEL_SD | PARALLEL_TILES)
};

enum TRACE_LEVEL
{
    TRACE_NUH       = 0x00000001,
    TRACE_AUD       = 0x00000002,
    TRACE_VPS       = 0x00000004,
    TRACE_SPS       = 0x00000008,
    TRACE_PPS       = 0x00000010,
    TRACE_SEI       = 0x00000020,
    TRACE_SSH       = 0x00000040,
    TRACE_REF       = 0x00000080,
    TRACE_CTU       = 0x00000100,
    TRACE_SAO       = 0x00000200,
    TRACE_CQT       = 0x00000400,
    TRACE_CU        = 0x00000800,
    TRACE_PU        = 0x00001000,
    TRACE_TT        = 0x00002000,
    TRACE_TU        = 0x00004000,
    TRACE_RESIDUAL  = 0x00008000,
    TRACE_PRED      = 0x04000000,
    TRACE_QP        = 0x08000000,
    TRACE_COEF      = 0x10000000,
    TRACE_SIZE      = 0x40000000,
    TRACE_OFFSET    = 0x80000000,
    TRACE_DEFAULT =
          TRACE_NUH
        | TRACE_AUD
        | TRACE_VPS
        | TRACE_SPS
        | TRACE_PPS
        | TRACE_SEI
        | TRACE_SSH
        | TRACE_REF
        | TRACE_SIZE
        //| TRACE_OFFSET
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
    RSV_VCL_N10, RSV_VCL_R11, RSV_VCL_N12,
    RSV_VCL_R13, RSV_VCL_N14, RSV_VCL_R15,
    BLA_W_LP,
    BLA_W_RADL,
    BLA_N_LP,
    IDR_W_RADL,
    IDR_N_LP,
    CRA_NUT,
    RSV_IRAP_VCL22, RSV_IRAP_VCL23,
    RSV_VCL24, RSV_VCL25, RSV_VCL26, RSV_VCL27,
    RSV_VCL28, RSV_VCL29, RSV_VCL30, RSV_VCL31,
    VPS_NUT,
    SPS_NUT,
    PPS_NUT,
    AUD_NUT,
    EOS_NUT,
    EOB_NUT,
    FD_NUT,
    PREFIX_SEI_NUT,
    SUFFIX_SEI_NUT,
    RSV_NVCL41, RSV_NVCL42, RSV_NVCL43, RSV_NVCL44,
    RSV_NVCL45, RSV_NVCL46, RSV_NVCL47,
    UNSPEC48, UNSPEC49, UNSPEC50, UNSPEC51,
    UNSPEC52, UNSPEC53, UNSPEC54, UNSPEC55,
    UNSPEC56, UNSPEC57, UNSPEC58, UNSPEC59,
    UNSPEC60, UNSPEC61, UNSPEC62, UNSPEC63,
    num_NALU_TYPE
};

enum PROFILE
{
    MAIN     = 1,
    MAIN_10  = 2,
    MAIN_SP  = 3,
    REXT     = 4,
    REXT_HT  = 5,
    MAIN_MV  = 6,
    MAIN_SC  = 7,
    MAIN_3D  = 8,
    SCC      = 9,
    REXT_SC  = 10
};

enum PIC_TYPE
{
    PIC_I = 0,
    PIC_PI,
    PIC_BPI
};

enum CHROMA_FORMAT
{
    CHROMA_400 = 0,
    CHROMA_420 = 1,
    CHROMA_422 = 2,
    CHROMA_444 = 3
};

enum AR_IDC
{
    AR_IDC_1_1    =   1,
    AR_IDC_square =   1,
    AR_IDC_12_11  =   2,
    AR_IDC_10_11  =   3,
    AR_IDC_16_11  =   4,
    AR_IDC_40_33  =   5,
    AR_IDC_24_11  =   6,
    AR_IDC_20_11  =   7,
    AR_IDC_32_11  =   8,
    AR_IDC_80_33  =   9,
    AR_IDC_18_11  =  10,
    AR_IDC_15_11  =  11,
    AR_IDC_64_33  =  12,
    AR_IDC_160_99 =  13,
    AR_IDC_4_3    =  14,
    AR_IDC_3_2    =  15,
    AR_IDC_2_1    =  16,
    Extended_SAR  = 255,
};

enum VIDEO_FORMAT
{
    Component = 0,
    PAL       = 1,
    NTSC      = 2,
    SECAM     = 3,
    MAC       = 4
};

enum SEI_TYPE
{
    SEI_BUFFERING_PERIOD                     = 0,
    SEI_PICTURE_TIMING                       = 1,
    SEI_PAN_SCAN_RECT                        = 2,
    SEI_FILLER_PAYLOAD                       = 3,
    SEI_USER_DATA_REGISTERED_ITU_T_T35       = 4,
    SEI_USER_DATA_UNREGISTERED               = 5,
    SEI_RECOVERY_POINT                       = 6,
    SEI_SCENE_INFO                           = 9,
    SEI_FULL_FRAME_SNAPSHOT                  = 15,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END   = 17,
    SEI_FILM_GRAIN_CHARACTERISTICS           = 19,
    SEI_POST_FILTER_HINT                     = 22,
    SEI_TONE_MAPPING_INFO                    = 23,
    SEI_FRAME_PACKING                        = 45,
    SEI_DISPLAY_ORIENTATION                  = 47,
    SEI_SOP_DESCRIPTION                      = 128,
    SEI_ACTIVE_PARAMETER_SETS                = 129,
    SEI_DECODING_UNIT_INFO                   = 130,
    SEI_TEMPORAL_LEVEL0_INDEX                = 131,
    SEI_DECODED_PICTURE_HASH                 = 132,
    SEI_SCALABLE_NESTING                     = 133,
    SEI_REGION_REFRESH_INFO                  = 134,
    SEI_NO_DISPLAY                           = 135,
    SEI_TIME_CODE                            = 136,
    SEI_MASTERING_DISPLAY_COLOUR_VOLUME      = 137,
    SEI_SEGM_RECT_FRAME_PACKING              = 138,
    SEI_TEMP_MOTION_CONSTRAINED_TILE_SETS    = 139,
    SEI_CHROMA_RESAMPLING_FILTER_HINT        = 140,
    SEI_KNEE_FUNCTION_INFO                   = 141,
    SEI_COLOUR_REMAPPING_INFO                = 142,
};

enum PIC_STRUCT
{
    FRAME = 0,
    TOP,
    BOT,
    TOP_BOT,
    BOT_TOP,
    TOP_BOT_TOP,
    BOT_TOP_BOT,
    FRAME_x2,
    FRAME_x3,
    TOP_PREVBOT,
    BOT_PREVTOP,
    TOP_NEXTBOT,
    BOT_NEXTTOP
};

enum SOURCE_SCAN_TYPE
{
    SCAN_INTERLACED = 0,
    SCAN_PROGRESSIVE,
    SCAN_UNKNOWN,
    SCAN_RESERVED
};

enum SLICE_TYPE
{
    B = 0,
    P,
    I
};

enum PRED_MODE
{
      MODE_INTER = 0
    , MODE_INTRA
    , MODE_SKIP
};

enum PART_MODE
{
      PART_2Nx2N = 0
    , PART_2NxN
    , PART_Nx2N
    , PART_NxN
    , PART_2NxnU
    , PART_2NxnD
    , PART_nLx2N
    , PART_nRx2N
};

enum INTER_PRED
{
      PRED_L0 = 0
    , PRED_L1
    , PRED_BI
};

enum SAO_TYPE
{
      NOT_APPLIED = 0
    , BAND_OFFSET
    , EDGE_OFFSET
};

enum SAO_EO_CLASS
{
      EO_0  = 0
    , EO_90
    , EO_135
    , EO_45
};

struct AUD
{
    Bs8u pic_type;
};

struct PTL
{
    union
    {
        struct
        {
            Bs32u profile_space                    : 2;
            Bs32u tier_flag                        : 1;
            Bs32u profile_idc                      : 5;
            Bs32u progressive_source_flag          : 1;
            Bs32u interlaced_source_flag           : 1;
            Bs32u non_packed_constraint_flag       : 1;
            Bs32u frame_only_constraint_flag       : 1;
            Bs32u max_12bit_constraint_flag        : 1;
            Bs32u max_10bit_constraint_flag        : 1;
            Bs32u max_8bit_constraint_flag         : 1;
            Bs32u max_422chroma_constraint_flag    : 1;
            Bs32u max_420chroma_constraint_flag    : 1;
            Bs32u max_monochrome_constraint_flag   : 1;
            Bs32u intra_constraint_flag            : 1;
            Bs32u one_picture_only_constraint_flag : 1;
            Bs32u lower_bit_rate_constraint_flag   : 1;
            Bs32u reserved_zero_34bits_0_10        : 11;

            Bs32u reserved_zero_34bits_11_33       : 23;
            Bs32u inbld_flag                       : 1;
            Bs32u level_idc                        : 8;
        };
        struct
        {
            Bs32u                            : 21;
            Bs32u max_14bit_constraint_flag  : 1;
            Bs32u reserved_zero_33bits_0_9   : 10;

            Bs32u reserved_zero_33bits_10_32 : 23;
            Bs32u                            : 9;
        };
        struct
        {
            Bs32u                            : 12;
            Bs32u reserved_zero_43bits_0_19  : 20;

            Bs32u reserved_zero_43bits_20_42 : 23;
            Bs32u reserved_zero_bit          : 1;
            Bs32u                            : 8;
        };
    };

    Bs32u profile_compatibility_flags;

    Bs32u profile_present_flag             : 1;
    Bs32u level_present_flag               : 1;
    Bs32u                                  : 30;
};

struct SubLayerOrderingInfo
{
    Bs32u max_dec_pic_buffering_minus1 : 4;
    Bs32u max_num_reorder_pics         : 4;
    Bs32u                              : 24;
    Bs32u max_latency_increase_plus1;
};

struct HRD
{
    Bs32u nal_hrd_parameters_present_flag                : 1;
    Bs32u vcl_hrd_parameters_present_flag                : 1;
    Bs32u sub_pic_hrd_params_present_flag                : 1;
    Bs32u du_cpb_removal_delay_increment_length_minus1   : 5;
    Bs32u dpb_output_delay_du_length_minus1              : 5;
    Bs32u bit_rate_scale                                 : 4;
    Bs32u initial_cpb_removal_delay_length_minus1        : 5;
    Bs32u au_cpb_removal_delay_length_minus1             : 5;
    Bs32u dpb_output_delay_length_minus1                 : 5;

    Bs32u tick_divisor_minus2                            : 8;
    Bs32u cpb_size_scale                                 : 4;
    Bs32u cpb_size_du_scale                              : 4;
    Bs32u sub_pic_cpb_params_in_pic_timing_sei_flag      : 1;
    Bs32u                                                : 15;

    struct SubLayer
    {
        Bs32u fixed_pic_rate_general_flag       : 1;
        Bs32u fixed_pic_rate_within_cvs_flag    : 1;
        Bs32u elemental_duration_in_tc_minus1   : 11;
        Bs32u low_delay_hrd_flag                : 1;
        Bs32u cpb_cnt_minus1                    : 5;
        Bs32u                                   : 14;

        struct CPB
        {
            Bs32u bit_rate_value_minus1;
            Bs32u cpb_size_value_minus1;
            Bs32u cpb_size_du_value_minus1;
            Bs32u bit_rate_du_value_minus1;
            Bs32u cbr_flag : 1;
            Bs32u          : 31;
        } *nal, *vcl;
    } sl[8];
};

struct SubLayers
{
    SubLayerOrderingInfo slo[8];

    union
    {
        struct
        {
            PTL general;
            PTL sub_layer[7];
        };
        PTL ptl[8];
    };
};

struct VPS : SubLayers
{
    Bs32u video_parameter_set_id    : 4;
    Bs32u base_layer_internal_flag  : 1;
    Bs32u base_layer_available_flag : 1;
    Bs32u max_layers_minus1         : 6;
    Bs32u max_sub_layers_minus1     : 3;
    Bs32u temporal_id_nesting_flag  : 1;
    Bs32u reserved_0xffff_16bits    : 16;

    Bs32u num_units_in_tick;
    Bs32u time_scale;

    Bs32u timing_info_present_flag             : 1;
    Bs32u poc_proportional_to_timing_flag      : 1;
    Bs32u sub_layer_ordering_info_present_flag : 1;
    Bs32u extension_flag                       : 1;
    Bs32u num_ticks_poc_diff_one_minus1        : 10;
    Bs32u num_hrd_parameters                   : 10;
    Bs32u                                      : 8;

    Bs32u max_layer_id                         : 6;
    Bs32u num_layer_sets_minus1                : 10;
    Bs32u                                      : 16;

    Bs32u ExtBits;

    Bs8u* layer_id_included_flags;

    struct VPSHRD : HRD
    {
        Bs16u hrd_layer_set_idx  : 10;
        Bs16u cprms_present_flag : 1;
        Bs16u                    : 5;
    } *hrd;

    Bs8u* ExtData;
};


struct VUI
{
    Bs32u sar_width                               : 16;
    Bs32u sar_height                              : 16;

    Bs32u aspect_ratio_info_present_flag          : 1;
    Bs32u timing_info_present_flag                : 1;
    Bs32u hrd_parameters_present_flag             : 1;
    Bs32u poc_proportional_to_timing_flag         : 1;
    Bs32u bitstream_restriction_flag              : 1;
    Bs32u tiles_fixed_structure_flag              : 1;
    Bs32u motion_vectors_over_pic_boundaries_flag : 1;
    Bs32u restricted_ref_pic_lists_flag           : 1;
    Bs32u aspect_ratio_idc                        : 8;
    Bs32u overscan_info_present_flag              : 1;
    Bs32u overscan_appropriate_flag               : 1;
    Bs32u video_signal_type_present_flag          : 1;
    Bs32u video_full_range_flag                   : 1;
    Bs32u colour_description_present_flag         : 1;
    Bs32u video_format                            : 3;
    Bs32u chroma_loc_info_present_flag            : 1;
    Bs32u neutral_chroma_indication_flag          : 1;
    Bs32u chroma_sample_loc_type_top_field        : 3;
    Bs32u chroma_sample_loc_type_bottom_field     : 3;

    Bs32u field_seq_flag                          : 1;
    Bs32u frame_field_info_present_flag           : 1;
    Bs32u default_display_window_flag             : 1;
    Bs32u max_bytes_per_pic_denom                 : 5;
    Bs32u colour_primaries                        : 8;
    Bs32u transfer_characteristics                : 8;
    Bs32u matrix_coeffs                           : 8;

    Bs32u log2_max_mv_length_horizontal           : 4;
    Bs32u log2_max_mv_length_vertical             : 4;
    Bs32u max_bits_per_min_cu_denom               : 5;
    Bs32u                                         : 7;
    Bs32u min_spatial_segmentation_idc            : 12;

    Bs32u def_disp_win_left_offset;
    Bs32u def_disp_win_right_offset;
    Bs32u def_disp_win_top_offset;
    Bs32u def_disp_win_bottom_offset;

    Bs32u num_units_in_tick;
    Bs32u time_scale;
    Bs32u num_ticks_poc_diff_one_minus1;

    HRD* hrd;
};

enum QM_IDX
{
    QM_IntraY = 0,
    QM_IntraCb,
    QM_IntraCr,
    QM_InterY,
    QM_InterCb,
    QM_InterCr,
};

struct QM
{
    Bs8u ScalingFactor0[6][4][4];
    Bs8u ScalingFactor1[6][8][8];
    Bs8u ScalingFactor2[6][16][16];
    Bs8u ScalingFactor3[6][32][32];
};

struct STRPS
{
    Bs16u NumDeltaPocs;
    Bs16u UsedByCurrPicFlags;
    Bs16s DeltaPoc[16];
};

struct SPS : SubLayers
{
    Bs32u seq_parameter_set_id                      : 4;
    Bs32u video_parameter_set_id                    : 4;
    Bs32u temporal_id_nesting_flag                  : 1;
    Bs32u num_short_term_ref_pic_sets               : 7;
    Bs32u separate_colour_plane_flag                : 1;
    Bs32u conformance_window_flag                   : 1;
    Bs32u sub_layer_ordering_info_present_flag      : 1;
    Bs32u chroma_format_idc                         : 2;
    Bs32u max_sub_layers_minus1                     : 3;
    Bs32u scaling_list_enabled_flag                 : 1;
    Bs32u scaling_list_data_present_flag            : 1;
    Bs32u amp_enabled_flag                          : 1;
    Bs32u sample_adaptive_offset_enabled_flag       : 1;
    Bs32u pcm_sample_bit_depth_luma_minus1          : 4;

    Bs32u pcm_enabled_flag                          : 1;
    Bs32u pcm_loop_filter_disabled_flag             : 1;
    Bs32u long_term_ref_pics_present_flag           : 1;
    Bs32u temporal_mvp_enabled_flag                 : 1;
    Bs32u pcm_sample_bit_depth_chroma_minus1        : 4;
    Bs32u strong_intra_smoothing_enabled_flag       : 1;
    Bs32u bit_depth_chroma_minus8                   : 4;
    Bs32u log2_max_pic_order_cnt_lsb_minus4         : 4;
    Bs32u vui_parameters_present_flag               : 1;
    Bs32u bit_depth_luma_minus8                     : 4;
    Bs32u num_long_term_ref_pics                    : 7;
    Bs32u motion_vector_resolution_control_idc      : 2;
    Bs32u intra_boundary_filtering_disabled_flag    : 1;

    Bs32u extension_present_flag                    : 1;
    Bs32u range_extension_flag                      : 1;
    Bs32u multilayer_extension_flag                 : 1;
    Bs32u extension_3d_flag                         : 1;
    Bs32u scc_extension_flag                        : 1;
    Bs32u extension_xbits                           : 4;
    Bs32u transform_skip_rotation_enabled_flag      : 1;
    Bs32u transform_skip_context_enabled_flag       : 1;
    Bs32u implicit_rdpcm_enabled_flag               : 1;
    Bs32u explicit_rdpcm_enabled_flag               : 1;
    Bs32u extended_precision_processing_flag        : 1;
    Bs32u intra_smoothing_disabled_flag             : 1;
    Bs32u high_precision_offsets_enabled_flag       : 1;
    Bs32u persistent_rice_adaptation_enabled_flag   : 1;
    Bs32u cabac_bypass_alignment_enabled_flag       : 1;
    Bs32u inter_view_mv_vert_constraint_flag        : 1;
    Bs32u log2_min_luma_transform_block_size_minus2     : 5;
    Bs32u log2_diff_max_min_luma_transform_block_size   : 5;
    Bs32u curr_pic_ref_enabled_flag                  : 1;
    Bs32u palette_mode_enabled_flag                  : 1;
    Bs32u palette_predictor_initializer_present_flag : 1;

    Bs32u log2_min_luma_coding_block_size_minus3        : 5;
    Bs32u log2_diff_max_min_luma_coding_block_size      : 5;
    Bs32u log2_min_pcm_luma_coding_block_size_minus3    : 5;
    Bs32u log2_diff_max_min_pcm_luma_coding_block_size  : 5;
    Bs32u max_transform_hierarchy_depth_inter           : 6;
    Bs32u max_transform_hierarchy_depth_intra           : 6;

    Bs16u pic_width_in_luma_samples;
    Bs16u pic_height_in_luma_samples;

    Bs16u conf_win_left_offset;
    Bs16u conf_win_right_offset;

    Bs16u conf_win_top_offset;
    Bs16u conf_win_bottom_offset;

    Bs32u used_by_curr_pic_lt_flags;

    Bs8u palette_max_size;
    Bs8s delta_palette_max_predictor_size;
    Bs16u num_palette_predictor_initializer;

    Bs32u ExtBits;

    Bs8u    *ExtData;
    STRPS   *strps;
    Bs16u   *lt_ref_pic_poc_lsb;
    QM      *qm;
    VUI     *vui;
    Bs16u   *palette_predictor_initializers[3];
};

struct PPS
{
    Bs32u pic_parameter_set_id                        : 6;
    Bs32u seq_parameter_set_id                        : 4;
    Bs32u dependent_slice_segments_enabled_flag       : 1;
    Bs32u output_flag_present_flag                    : 1;
    Bs32u sign_data_hiding_enabled_flag               : 1;
    Bs32u cabac_init_present_flag                     : 1;
    Bs32u constrained_intra_pred_flag                 : 1;
    Bs32u transform_skip_enabled_flag                 : 1;
    Bs32u cu_qp_delta_enabled_flag                    : 1;
    Bs32u slice_chroma_qp_offsets_present_flag        : 1;
    Bs32u weighted_pred_flag                          : 1;
    Bs32u weighted_bipred_flag                        : 1;
    Bs32u transquant_bypass_enabled_flag              : 1;
    Bs32u tiles_enabled_flag                          : 1;
    Bs32u entropy_coding_sync_enabled_flag            : 1;
    Bs32u uniform_spacing_flag                        : 1;
    Bs32u loop_filter_across_tiles_enabled_flag       : 1;
    Bs32u loop_filter_across_slices_enabled_flag      : 1;
    Bs32u deblocking_filter_control_present_flag      : 1;
    Bs32u deblocking_filter_override_enabled_flag     : 1;
    Bs32u deblocking_filter_disabled_flag             : 1;
    Bs32u scaling_list_data_present_flag              : 1;
    Bs32u lists_modification_present_flag             : 1;
    Bs32u slice_segment_header_extension_present_flag : 1;

    Bs32u num_extra_slice_header_bits                 : 3;
    Bs32u num_ref_idx_l0_default_active_minus1        : 4;
    Bs32u num_ref_idx_l1_default_active_minus1        : 4;
    Bs32u extension_present_flag                      : 1;
    Bs32u range_extension_flag                        : 1;
    Bs32u multilayer_extension_flag                   : 1;
    Bs32u extension_3d_flag                           : 1;
    Bs32u scc_extension_flag                          : 1;
    Bs32u extension_xbits                             : 4;
    Bs32u                                             : 12;

    Bs32u curr_pic_ref_enabled_flag                         : 1;
    Bs32u residual_adaptive_colour_transform_enabled_flag   : 1;
    Bs32u slice_act_qp_offsets_present_flag                 : 1;
    Bs32u palette_predictor_initializer_present_flag        : 1;
    Bs32u monochrome_palette_flag                           : 1;
    Bs32u luma_bit_depth_entry_minus8                       : 4;
    Bs32u chroma_bit_depth_entry_minus8                     : 4;
    Bs32u num_palette_predictor_initializer                 : 16;
    Bs32u                                                   : 3;
    Bs32s ActQpOffsetY  : 5; //-12 to +12,
    Bs32s ActQpOffsetCb : 5; //-12 to +12,
    Bs32s ActQpOffsetCr : 5; //-12 to +12,
    Bs32s               : 17;

    Bs8s init_qp_minus26;
    Bs8s cb_qp_offset;
    Bs8s cr_qp_offset;
    Bs8s beta_offset_div2 : 4;
    Bs8s tc_offset_div2   : 4;

    Bs32u cross_component_prediction_enabled_flag   : 1;
    Bs32u chroma_qp_offset_list_enabled_flag        : 1;
    Bs32u log2_sao_offset_scale_luma                : 3;
    Bs32u log2_sao_offset_scale_chroma              : 3;
    Bs32u chroma_qp_offset_list_len_minus1          : 3;
    Bs32u diff_cu_chroma_qp_offset_depth            : 5;
    Bs32u log2_max_transform_skip_block_size_minus2 : 5;
    Bs32u                                           : 11;

    Bs8s cb_qp_offset_list[6];
    Bs8s cr_qp_offset_list[6];

    Bs16u diff_cu_qp_delta_depth;
    Bs16u num_tile_columns_minus1;
    Bs16u num_tile_rows_minus1;
    Bs16u log2_parallel_merge_level_minus2;

    Bs32u ExtBits;

    Bs8u  *ExtData;
    Bs16u *column_width_minus1;
    Bs16u *row_height_minus1;
    Bs16u *CtbAddrRsToTs;
    QM    *qm;
    Bs16u *palette_predictor_initializers[3];
};

struct APS_SEI
{
    Bs32u active_video_parameter_set_id : 4;
    Bs32u self_contained_cvs_flag       : 1;
    Bs32u no_parameter_set_update_flag  : 1;
    Bs32u num_sps_ids_minus1            : 4;
    Bs32u                               : 22;
    Bs8u* active_seq_parameter_set_id;
    Bs8u* layer_sps_idx;
};

struct BP_SEI
{
    Bs32u seq_parameter_set_id               : 4;
    Bs32u irap_cpb_params_present_flag       : 1;
    Bs32u concatenation_flag                 : 1;
    Bs32u use_alt_cpb_params_flag            : 1;
    Bs32u au_cpb_removal_delay_delta_minus1;
    Bs32u cpb_delay_offset;
    Bs32u dpb_delay_offset;

    struct CPB
    {
        Bs32u initial_cpb_removal_delay;
        Bs32u initial_cpb_removal_offset;
        Bs32u initial_alt_cpb_removal_delay;
        Bs32u initial_alt_cpb_removal_offset;
    } *nal, *vcl;
};

struct PT_SEI
{
    Bs32u pic_struct                       : 4;
    Bs32u source_scan_type                 : 2;
    Bs32u duplicate_flag                   : 1;
    Bs32u du_common_cpb_removal_delay_flag : 1;

    Bs32u au_cpb_removal_delay_minus1;
    Bs32u pic_dpb_output_delay;
    Bs32u pic_dpb_output_du_delay;
    Bs32u num_decoding_units_minus1;
    Bs32u du_common_cpb_removal_delay_increment_minus1;

    Bs32u *num_nalus_in_du_minus1;
    Bs32u *du_cpb_removal_delay_increment_minus1;

    VUI* vui;
};

struct RP_SEI
{
    Bs16s recovery_poc_cnt;
    Bs16u exact_match_flag : 1;
    Bs16u broken_link_flag : 1;
};

struct SEI
{
    Bs32u payloadType;
    Bs32u payloadSize;

    Bs8u* rawData;

    union
    {
        APS_SEI *aps;
        BP_SEI  *bp;
        PT_SEI  *pt;
        RP_SEI  *rp;
    };

    SEI* next;
};

typedef Bs16s PWT[16][3][2];

struct RefPic
{
    Bs32u long_term :  1;
    Bs32u lost      :  1;
    Bs32u used      :  1;
    Bs32u           : 29;
    Bs32s POC;
};

struct PU
{
    Bs16u x;
    Bs16u y;
    Bs16u w : 8;
    Bs16u h : 8;

    Bs16u merge_idx      : 3;
    Bs16u merge_flag     : 1;
    Bs16u inter_pred_idc : 2;
    Bs16u mvp_l0_flag    : 1;
    Bs16u mvp_l1_flag    : 1;
    Bs16u ref_idx_l0     : 4;
    Bs16u ref_idx_l1     : 4;

    Bs16s MvLX[2][2];

    PU* Next;
};

struct TU
{
    Bs16u x;
    Bs16u y;

    Bs32u log2TrafoSize       : 8;
    Bs32u transform_skip_flag : 3;
    Bs32u cbf_cb              : 1;
    Bs32u cbf_cb1             : 1;
    Bs32u cbf_cr              : 1;
    Bs32u cbf_cr1             : 1;
    Bs32u cbf_luma            : 1;

    Bs32u log2_res_scale_abs_plus1_0 : 3;
    Bs32u res_scale_sign_flag_0      : 1;
    Bs32u log2_res_scale_abs_plus1_1 : 3;
    Bs32u res_scale_sign_flag_1      : 1;
    Bs32u tu_residual_act_flag       : 1;
    Bs32u                            : 7;

    Bs16s QP[3];

    Bs32s* tc_levels_luma;

    TU* Next;
};

struct CU
{
    Bs16u x;
    Bs16u y;

    Bs32u log2CbSize             : 8;
    Bs32u transquant_bypass_flag : 1;
    Bs32u PredMode               : 2;
    Bs32u PartMode               : 4;
    Bs32u pcm_flag               : 1;
    Bs32u palette_mode_flag      : 1;
    Bs32u chroma_qp_offset_flag  : 1;
    Bs32u chroma_qp_offset_idx   : 3;
    Bs32u                        : 11;

    Bs8u IntraPredModeY[2][2];
    Bs8u IntraPredModeC[2][2];

    Bs16s QpY;
    Bs16s QpCb;
    Bs16s QpCr;

    PU* Pu;
    TU* Tu;
    CU* Next;
};

struct SAO
{
    Bs16u type_idx      : 2;
    Bs16u band_position : 5;
    Bs16u eo_class      : 2;
    Bs16u               : 7;
    Bs8s  offset[4];
};

struct CTU
{
    Bs16u CtbAddrInRs;
    Bs16u CtbAddrInTs;

    Bs16u end_of_slice_segment_flag :  1;
    Bs16u sao_merge_left_flag       :  1;
    Bs16u sao_merge_up_flag         :  1;
    Bs16u                           : 13;

    SAO  sao[3];
    CU*  Cu;
    CTU* Next;
};

struct Slice
{
    Bs32u first_slice_segment_in_pic_flag           : 1;
    Bs32u no_output_of_prior_pics_flag              : 1;
    Bs32u dependent_slice_segment_flag              : 1;
    Bs32u sao_luma_flag                             : 1;
    Bs32u sao_chroma_flag                           : 1;
    Bs32u colour_plane_id                           : 2;
    Bs32u num_ref_idx_l0_active                     : 4;
    Bs32u num_ref_idx_l1_active                     : 4;
    Bs32u num_long_term_pics                        : 4;
    Bs32u collocated_ref_idx                        : 4;
    Bs32u short_term_ref_pic_set_idx                : 6;
    Bs32u ref_pic_list_modification_flag_l0         : 1;
    Bs32u ref_pic_list_modification_flag_l1         : 1;
    Bs32u Split                                     : 1;

    Bs32u temporal_mvp_enabled_flag                 : 1;
    Bs32u num_ref_idx_active_override_flag          : 1;
    Bs32u mvd_l1_zero_flag                          : 1;
    Bs32u cabac_init_flag                           : 1;
    Bs32u collocated_from_l0_flag                   : 1;
    Bs32u cu_chroma_qp_offset_enabled_flag          : 1;
    Bs32u deblocking_filter_override_flag           : 1;
    Bs32u deblocking_filter_disabled_flag           : 1;
    Bs32u loop_filter_across_slices_enabled_flag    : 1;
    Bs32u pic_output_flag                           : 1;
    Bs32u short_term_ref_pic_set_sps_flag           : 1;
    Bs32u type                                      : 2;
    Bs32u MaxNumMergeCand                           : 3;
    Bs32u num_long_term_sps                         : 5;
    Bs32u offset_len_minus1                         : 5;
    Bs32u pic_parameter_set_id                      : 6;

    Bs32u pic_order_cnt_lsb         : 16;
    Bs32u reserved_flags            : 8;
    Bs32u luma_log2_weight_denom    : 3;
    Bs32u chroma_log2_weight_denom  : 3;
    Bs32u use_integer_mv_flag       : 1;
    Bs32u                           : 1;

    Bs32u slice_segment_address;

    Bs32u num_entry_point_offsets;
    Bs32u ExtBits;

    Bs32s qp_delta         : 8;
    Bs32s cb_qp_offset     : 8;
    Bs32s cr_qp_offset     : 8;
    Bs32s beta_offset_div2 : 4;
    Bs32s tc_offset_div2   : 4;

    Bs32u used_by_curr_pic_lt_flags;

    Bs32s POC;
    Bs32u NumCTU;

    Bs32s act_y_qp_offset  : 6;
    Bs32s act_cb_qp_offset : 6;
    Bs32s act_cr_qp_offset : 6;
    Bs32s                  : 14;

    STRPS strps;
    Bs8u  list_entry_lx[2][16];

    Bs8u   *ExtData;
    Bs16u  *poc_lsb_lt;
    Bs16u  *DeltaPocMsbCycleLt;
    PWT    *pwt; //[list][entry][Y, Cb, Cr][Weight, Offset]
    Bs32u  *entry_point_offset_minus1;

    RefPic *DPB;
    RefPic *L0;
    RefPic *L1;

    PPS    *pps;
    SPS    *sps;
    CTU    *ctu;
};

struct NALU
{
    Bs64u StartOffset;
    Bs32u NumBytesInNalUnit;
    Bs32u NumBytesInRbsp;

    Bs16u forbidden_zero_bit    : 1;
    Bs16u nal_unit_type         : 6;
    Bs16u nuh_layer_id          : 6;
    Bs16u nuh_temporal_id_plus1 : 3;

    union
    {
        void  *unit;
        AUD   *aud;
        VPS   *vps;
        SPS   *sps;
        PPS   *pps;
        SEI   *sei;
        Slice *slice;
    };

    NALU* next;
};

};
