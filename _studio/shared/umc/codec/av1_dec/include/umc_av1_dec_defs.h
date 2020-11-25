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

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#ifndef __UMC_AV1_DEC_DEFS_DEC_H__
#define __UMC_AV1_DEC_DEFS_DEC_H__

#include <stdexcept>
#include <vector>
#include "umc_structures.h"
#include "umc_vp9_dec_defs.h"

namespace UMC_AV1_DECODER
{
    class AV1DecoderFrame;
    typedef std::vector<AV1DecoderFrame*> DPBType;

    using UMC_VP9_DECODER::NUM_REF_FRAMES;

    const uint8_t SYNC_CODE_0 = 0x49;
    const uint8_t SYNC_CODE_1 = 0x83;
    const uint8_t SYNC_CODE_2 = 0x43;

    const uint8_t FRAME_MARKER = 0x2;

    const uint8_t MINIMAL_DATA_SIZE = 3;

    const uint8_t INTER_REFS                    = 7;
    const uint8_t TOTAL_REFS                    = 8;

    const uint8_t QM_LEVEL_BITS                 = 4;

    const uint8_t LOG2_SWITCHABLE_FILTERS       = 2;

    const uint8_t CDEF_MAX_STRENGTHS            = 8;

    const uint8_t CDEF_STRENGTH_BITS            = 6;

    const uint8_t  MAX_MB_PLANE                 = 3;
    const uint8_t MAX_LEB128_SIZE               = 8;
    const uint8_t LEB128_BYTE_MASK              = 0x7f;
    const uint8_t MAX_SB_SIZE_LOG2              = 7;

    const uint16_t RESTORATION_UNITSIZE_MAX     = 256;

    const uint8_t MI_SIZE_LOG2 = 2;
    const uint8_t MAX_MIB_SIZE_LOG2 = MAX_SB_SIZE_LOG2 - MI_SIZE_LOG2;

    const uint8_t SCALE_NUMERATOR = 8;
    const uint8_t SUPERRES_SCALE_BITS = 3;
    const uint8_t SUPERRES_SCALE_DENOMINATOR_MIN = SCALE_NUMERATOR + 1;
    const uint8_t PRIMARY_REF_BITS = 3;
    const uint8_t PRIMARY_REF_NONE = 7;

    const uint32_t MAX_TILE_WIDTH = 4096;        // Max Tile width in pixels
    const uint32_t MAX_TILE_AREA  = 4096 * 2304;  // Maximum tile area in pixels
    const uint32_t MAX_TILE_ROWS  = 64;
    const uint32_t MAX_TILE_COLS  = 64;

    const uint8_t FRAME_CONTEXTS_LOG2           = 3;
    const uint8_t MAX_MODE_LF_DELTAS            = 2;

    const uint8_t WARPEDMODEL_PREC_BITS         = 16;
    const uint8_t WARP_PARAM_REDUCE_BITS        = 6;

    const uint8_t MAX_NUM_TEMPORAL_LAYERS       = 8;
    const uint8_t MAX_NUM_SPATIAL_LAYERS        = 4;
    const uint8_t MAX_NUM_OPERATING_POINTS      = MAX_NUM_TEMPORAL_LAYERS * MAX_NUM_SPATIAL_LAYERS;
    const uint8_t SELECT_SCREEN_CONTENT_TOOLS   = 2;
    const uint8_t SELECT_INTEGER_MV             = 2;

    const uint8_t MAX_POINTS_IN_SCALING_FUNCTION_LUMA    = 14;
    const uint8_t MAX_POINTS_IN_SCALING_FUNCTION_CHROMA  = 10;
    const uint8_t MAX_AUTOREG_COEFFS_LUMA                = 24;
    const uint8_t MAX_AUTOREG_COEFFS_CHROMA              = MAX_AUTOREG_COEFFS_LUMA + 1;

    enum AV1_OBU_TYPE
    {
        OBU_SEQUENCE_HEADER = 1,
        OBU_TEMPORAL_DELIMITER = 2,
        OBU_FRAME_HEADER = 3,
        OBU_TILE_GROUP = 4,
        OBU_METADATA = 5,
        OBU_FRAME = 6,
        OBU_REDUNDANT_FRAME_HEADER = 7,
        OBU_PADDING = 15,
    };

    enum AOM_COLOR_PRIMARIES
    {
        AOM_CICP_CP_RESERVED_0 = 0,  /**< For future use */
        AOM_CICP_CP_BT_709 = 1,      /**< BT.709 */
        AOM_CICP_CP_UNSPECIFIED = 2, /**< Unspecified */
        AOM_CICP_CP_RESERVED_3 = 3,  /**< For future use */
        AOM_CICP_CP_BT_470_M = 4,    /**< BT.470 System M (historical) */
        AOM_CICP_CP_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
        AOM_CICP_CP_BT_601 = 6,      /**< BT.601 */
        AOM_CICP_CP_SMPTE_240 = 7,   /**< SMPTE 240 */
        AOM_CICP_CP_GENERIC_FILM =
        8, /**< Generic film (color filters using illuminant C) */
        AOM_CICP_CP_BT_2020 = 9,      /**< BT.2020, BT.2100 */
        AOM_CICP_CP_XYZ = 10,         /**< SMPTE 428 (CIE 1921 XYZ) */
        AOM_CICP_CP_SMPTE_431 = 11,   /**< SMPTE RP 431-2 */
        AOM_CICP_CP_SMPTE_432 = 12,   /**< SMPTE EG 432-1  */
        AOM_CICP_CP_RESERVED_13 = 13, /**< For future use (values 13 - 21)  */
        AOM_CICP_CP_EBU_3213 = 22,    /**< EBU Tech. 3213-E  */
        AOM_CICP_CP_RESERVED_23 = 23  /**< For future use (values 23 - 255)  */
    };

    enum AOM_TRANSFER_CHARACTERISTICS
    {
        AOM_CICP_TC_RESERVED_0 = 0,  /**< For future use */
        AOM_CICP_TC_BT_709 = 1,      /**< BT.709 */
        AOM_CICP_TC_UNSPECIFIED = 2, /**< Unspecified */
        AOM_CICP_TC_RESERVED_3 = 3,  /**< For future use */
        AOM_CICP_TC_BT_470_M = 4,    /**< BT.470 System M (historical)  */
        AOM_CICP_TC_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
        AOM_CICP_TC_BT_601 = 6,      /**< BT.601 */
        AOM_CICP_TC_SMPTE_240 = 7,   /**< SMPTE 240 M */
        AOM_CICP_TC_LINEAR = 8,      /**< Linear */
        AOM_CICP_TC_LOG_100 = 9,     /**< Logarithmic (100 : 1 range) */
        AOM_CICP_TC_LOG_100_SQRT10 =
        10,                     /**< Logarithmic (100 * Sqrt(10) : 1 range) */
        AOM_CICP_TC_IEC_61966 = 11, /**< IEC 61966-2-4 */
        AOM_CICP_TC_BT_1361 = 12,   /**< BT.1361 */
        AOM_CICP_TC_SRGB = 13,      /**< sRGB or sYCC*/
        AOM_CICP_TC_BT_2020_10_BIT = 14, /**< BT.2020 10-bit systems */
        AOM_CICP_TC_BT_2020_12_BIT = 15, /**< BT.2020 12-bit systems */
        AOM_CICP_TC_SMPTE_2084 = 16,     /**< SMPTE ST 2084, ITU BT.2100 PQ */
        AOM_CICP_TC_SMPTE_428 = 17,      /**< SMPTE ST 428 */
        AOM_CICP_TC_HLG = 18,            /**< BT.2100 HLG, ARIB STD-B67 */
        AOM_CICP_TC_RESERVED_19 = 19     /**< For future use (values 19-255) */
    };

    enum  AOM_MATRIX_COEFFICIENTS
    {
        AOM_CICP_MC_IDENTITY = 0,    /**< Identity matrix */
        AOM_CICP_MC_BT_709 = 1,      /**< BT.709 */
        AOM_CICP_MC_UNSPECIFIED = 2, /**< Unspecified */
        AOM_CICP_MC_RESERVED_3 = 3,  /**< For future use */
        AOM_CICP_MC_FCC = 4,         /**< US FCC 73.628 */
        AOM_CICP_MC_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
        AOM_CICP_MC_BT_601 = 6,      /**< BT.601 */
        AOM_CICP_MC_SMPTE_240 = 7,   /**< SMPTE 240 M */
        AOM_CICP_MC_SMPTE_YCGCO = 8, /**< YCgCo */
        AOM_CICP_MC_BT_2020_NCL =
        9, /**< BT.2020 non-constant luminance, BT.2100 YCbCr  */
        AOM_CICP_MC_BT_2020_CL = 10, /**< BT.2020 constant luminance */
        AOM_CICP_MC_SMPTE_2085 = 11, /**< SMPTE ST 2085 YDzDx */
        AOM_CICP_MC_CHROMAT_NCL =
        12, /**< Chromaticity-derived non-constant luminance */
        AOM_CICP_MC_CHROMAT_CL = 13, /**< Chromaticity-derived constant luminance */
        AOM_CICP_MC_ICTCP = 14,      /**< BT.2100 ICtCp */
        AOM_CICP_MC_RESERVED_15 = 15 /**< For future use (values 15-255)  */
    };

    enum AOM_COLOR_RANGE
    {
        AOM_CR_STUDIO_RANGE = 0, /**< Y [16..235], UV [16..240] */
        AOM_CR_FULL_RANGE = 1    /**< YUV/RGB [0..255] */
    };

    enum AOM_CHROMA_SAMPLE_POSITION
    {
        AOM_CSP_UNKNOWN = 0,          /**< Unknown */
        AOM_CSP_VERTICAL = 1,         /**< Horizontally co-located with luma(0, 0)*/
                                      /**< sample, between two vertical samples */
        AOM_CSP_COLOCATED = 2,        /**< Co-located with luma(0, 0) sample */
        AOM_CSP_RESERVED = 3          /**< Reserved value */
    };

    enum FRAME_TYPE
    {
        KEY_FRAME = 0,
        INTER_FRAME = 1,
        INTRA_ONLY_FRAME = 2,  // replaces intra-only
        SWITCH_FRAME = 3,
        FRAME_TYPES,
    };

    enum SB_SIZE
    {
        BLOCK_64X64 = 0,
        BLOCK_128X128 = 1,
    };

    using UMC_VP9_DECODER::VP9_MAX_NUM_OF_SEGMENTS;
    using UMC_VP9_DECODER::MAX_LOOP_FILTER;

    enum SEG_LVL_FEATURES {
        SEG_LVL_ALT_Q,       // Use alternate Quantizer ....
        SEG_LVL_ALT_LF_Y_V,  // Use alternate loop filter value on y plane vertical
        SEG_LVL_ALT_LF_Y_H,  // Use alternate loop filter value on y plane horizontal
        SEG_LVL_ALT_LF_U,    // Use alternate loop filter value on u plane
        SEG_LVL_ALT_LF_V,    // Use alternate loop filter value on v plane
        SEG_LVL_REF_FRAME,   // Optional Segment reference frame
        SEG_LVL_SKIP,        // Optional Segment (0,0) + skip mode
        SEG_LVL_GLOBALMV,
        SEG_LVL_MAX
    };

    const uint8_t MAX_REF_IDX_FOR_SEGMENT = 7;

    const uint8_t SEG_FEATURE_DATA_SIGNED[SEG_LVL_MAX] = { 1, 1, 1, 1, 1, 0, 0};
    const uint8_t SEG_FEATURE_DATA_MAX[SEG_LVL_MAX] = { UMC_VP9_DECODER::MAXQ,
                                                      MAX_LOOP_FILTER, MAX_LOOP_FILTER, MAX_LOOP_FILTER, MAX_LOOP_FILTER,
                                                      MAX_REF_IDX_FOR_SEGMENT,
                                                      0 };

    enum {
        RESET_FRAME_CONTEXT_NONE = 0,
        RESET_FRAME_CONTEXT_CURRENT = 1,
        RESET_FRAME_CONTEXT_ALL = 2
    };

    enum INTERP_FILTER{
        EIGHTTAP_REGULAR,
        EIGHTTAP_SMOOTH,
        MULTITAP_SHARP,
        BILINEAR,
        INTERP_FILTERS_ALL,
        SWITCHABLE_FILTERS = BILINEAR,
        SWITCHABLE = SWITCHABLE_FILTERS + 1, /* the last switchable one */
        EXTRA_FILTERS = INTERP_FILTERS_ALL - SWITCHABLE_FILTERS,
    };

    enum TX_MODE{
        ONLY_4X4 = 0,     // only 4x4 transform used
        TX_MODE_LARGEST,  // transform size is the largest possible for pu size
        TX_MODE_SELECT,   // transform specified for each block
        TX_MODES,
    };

    enum REFERENCE_MODE {
        SINGLE_REFERENCE = 0,
        COMPOUND_REFERENCE = 1,
        REFERENCE_MODE_SELECT = 2,
        REFERENCE_MODES = 3,
    };

    // TODO: [Rev0.85] remove this enum once "refresh_frame_context" field will be removed from DDI
    enum REFRESH_FRAME_CONTEXT_MODE {
        REFRESH_FRAME_CONTEXT_DISABLED,
        REFRESH_FRAME_CONTEXT_BACKWARD,
    };

    enum TRANSFORMATION_TYPE {
        IDENTITY = 0,      // identity transformation, 0-parameter
        TRANSLATION = 1,   // translational motion 2-parameter
        ROTZOOM = 2,       // simplified affine with rotation + zoom only, 4-parameter
        AFFINE = 3,        // affine, 6-parameter
        HORTRAPEZOID = 4,  // constrained homography, hor trapezoid, 6-parameter
        VERTRAPEZOID = 5,  // constrained homography, ver trapezoid, 6-parameter
        HOMOGRAPHY = 6,    // homography, 8-parameter
        TRANS_TYPES = 7,
    };

    enum MV_REFERENCE_FRAME
    {
        NONE = -1,
        INTRA_FRAME = 0,
        LAST_FRAME = 1,
        LAST2_FRAME = 2,
        LAST3_FRAME = 3,
        GOLDEN_FRAME = 4,
        BWDREF_FRAME = 5,
        ALTREF2_FRAME = 6,
        ALTREF_FRAME = 7,
        MAX_REF_FRAMES = 8
    };

    struct TimingInfo
    {
        uint32_t num_units_in_display_tick;
        uint32_t time_scale;
        uint32_t equal_picture_interval;
        uint32_t num_ticks_per_picture_minus_1;
    };

    struct DecoderModelInfo
    {
        uint32_t buffer_delay_length_minus_1;
        uint32_t num_units_in_decoding_tick;
        uint32_t buffer_removal_time_length_minus_1;
        uint32_t frame_presentation_time_length_minus_1;
    };

    struct OperatingParametersInfo
    {
        uint32_t decoder_buffer_delay;
        uint32_t encoder_buffer_delay;
        uint32_t low_delay_mode_flag;
    };

    struct ColorConfig
    {
        uint32_t BitDepth;
        uint32_t mono_chrome;
        uint32_t color_primaries;
        uint32_t transfer_characteristics;
        uint32_t matrix_coefficients;
        uint32_t color_range;
        uint32_t chroma_sample_position;
        uint32_t subsampling_x;
        uint32_t subsampling_y;
        uint32_t separate_uv_delta_q;
    };

    struct SequenceHeader
    {
        //Rev 0.85 parameters (AV1 spec version 1.0) in order of appearance/calculation in sequence_header_obu()
        uint32_t seq_profile;
        uint32_t still_picture;
        uint32_t reduced_still_picture_header;

        uint32_t timing_info_present_flag;
        TimingInfo timing_info;

        uint32_t decoder_model_info_present_flag;
        DecoderModelInfo decoder_model_info;

        uint32_t operating_points_cnt_minus_1;
        uint32_t operating_point_idc[MAX_NUM_OPERATING_POINTS];
        uint32_t seq_level_idx[MAX_NUM_OPERATING_POINTS];
        uint32_t seq_tier[MAX_NUM_OPERATING_POINTS];
        uint32_t decoder_model_present_for_this_op[MAX_NUM_OPERATING_POINTS];
        OperatingParametersInfo operating_parameters_info[MAX_NUM_OPERATING_POINTS];
        uint32_t initial_display_delay_minus_1[MAX_NUM_OPERATING_POINTS];

        uint32_t frame_width_bits;
        uint32_t frame_height_bits;
        uint32_t max_frame_width;
        uint32_t max_frame_height;
        uint32_t frame_id_numbers_present_flag;
        uint32_t delta_frame_id_length;
        uint32_t idLen;
        uint32_t sbSize;
        uint32_t enable_filter_intra;
        uint32_t enable_intra_edge_filter;
        uint32_t enable_interintra_compound;
        uint32_t enable_masked_compound;
        uint32_t enable_warped_motion;
        uint32_t enable_dual_filter;
        uint32_t enable_order_hint;
        uint32_t enable_jnt_comp;
        uint32_t enable_ref_frame_mvs;
        uint32_t seq_force_screen_content_tools;
        uint32_t seq_force_integer_mv;
        int32_t order_hint_bits_minus1;
        uint32_t enable_superres;
        uint32_t enable_cdef;
        uint32_t enable_restoration;

        ColorConfig color_config;

        uint32_t film_grain_param_present;
    };

    struct TileInfo
    {
        uint32_t uniform_tile_spacing_flag;
        uint32_t TileColsLog2;
        uint32_t TileRowsLog2;
        uint32_t TileCols;
        uint32_t TileRows;
        uint32_t SbColStarts[MAX_TILE_COLS + 1];  // valid for 0 <= i <= TileCols
        uint32_t SbRowStarts[MAX_TILE_ROWS + 1];  // valid for 0 <= i <= TileRows
        uint32_t context_update_tile_id;
        uint32_t TileSizeBytes;

        //Rev 0.5 parameters
        uint32_t loop_filter_across_tiles_v_enabled;
        uint32_t loop_filter_across_tiles_h_enabled;
    };

    struct QuantizationParams
    {
        uint32_t base_q_idx;
        int32_t DeltaQYDc;
        int32_t DeltaQUDc;
        int32_t DeltaQUAc;
        int32_t DeltaQVDc;
        int32_t DeltaQVAc;
        uint32_t using_qmatrix;
        uint32_t qm_y;
        uint32_t qm_u;
        uint32_t qm_v;
    };

    struct SegmentationParams
    {
        uint8_t segmentation_enabled;
        uint8_t segmentation_update_map;
        uint8_t segmentation_temporal_update;
        uint8_t segmentation_update_data;

        int32_t FeatureData[VP9_MAX_NUM_OF_SEGMENTS][SEG_LVL_MAX];
        uint32_t FeatureMask[VP9_MAX_NUM_OF_SEGMENTS];

    };

    struct LoopFilterParams
    {
        int32_t loop_filter_level[4];
        int32_t loop_filter_sharpness;
        uint8_t loop_filter_delta_enabled;
        uint8_t loop_filter_delta_update;
        // 0 = Intra, Last, Last2, Last3, GF, BWD, ARF
        int8_t loop_filter_ref_deltas[TOTAL_REFS];
        // 0 = ZERO_MV, MV
        int8_t loop_filter_mode_deltas[MAX_MODE_LF_DELTAS];
    };

    struct CdefParams
    {
        uint32_t cdef_damping;
        uint32_t cdef_bits;
        uint32_t cdef_y_pri_strength[CDEF_MAX_STRENGTHS];
        uint32_t cdef_y_sec_strength[CDEF_MAX_STRENGTHS];
        uint32_t cdef_uv_pri_strength[CDEF_MAX_STRENGTHS];
        uint32_t cdef_uv_sec_strength[CDEF_MAX_STRENGTHS];

        //Rev 0.5 parameters
        uint32_t cdef_y_strength[CDEF_MAX_STRENGTHS];
        uint32_t cdef_uv_strength[CDEF_MAX_STRENGTHS];
    };

    enum RestorationType
    {
        RESTORE_NONE,
        RESTORE_WIENER,
        RESTORE_SGRPROJ,
        RESTORE_SWITCHABLE,
        RESTORE_SWITCHABLE_TYPES = RESTORE_SWITCHABLE,
        RESTORE_TYPES = 4,
    };

    struct LRParams
    {
        RestorationType lr_type[MAX_MB_PLANE];
        uint32_t lr_unit_shift;
        uint32_t lr_uv_shift;
    };

    struct GlobalMotionParams
    {
        TRANSFORMATION_TYPE wmtype;
        int32_t wmmat[8];
        int16_t alpha;
        int16_t beta;
        int16_t gamma;
        int16_t delta;
        int8_t invalid;
    };

    struct FilmGrainParams{
        uint32_t apply_grain;
        uint32_t grain_seed;
        uint32_t update_grain;

        uint32_t film_grain_params_ref_idx;

        // 8 bit values

        int32_t num_y_points;  // value: 0..14
        int32_t point_y_value[MAX_POINTS_IN_SCALING_FUNCTION_LUMA];
        int32_t point_y_scaling[MAX_POINTS_IN_SCALING_FUNCTION_LUMA];

        int32_t chroma_scaling_from_luma;

        // 8 bit values
        int32_t num_cb_points;  // value: 0..10
        int32_t point_cb_value[MAX_POINTS_IN_SCALING_FUNCTION_CHROMA];
        int32_t point_cb_scaling[MAX_POINTS_IN_SCALING_FUNCTION_CHROMA];

        // 8 bit values
        int32_t num_cr_points;  // value: 0..10
        int32_t point_cr_value[MAX_POINTS_IN_SCALING_FUNCTION_CHROMA];
        int32_t point_cr_scaling[MAX_POINTS_IN_SCALING_FUNCTION_CHROMA];

        int32_t grain_scaling;

        int32_t ar_coeff_lag;  // values:  0..3

        // 8 bit values
        int32_t ar_coeffs_y[MAX_AUTOREG_COEFFS_LUMA];
        int32_t ar_coeffs_cb[MAX_AUTOREG_COEFFS_CHROMA];
        int32_t ar_coeffs_cr[MAX_AUTOREG_COEFFS_CHROMA];

        // Shift value: AR coeffs range
        // 6: [-2, 2)
        // 7: [-1, 1)
        // 8: [-0.5, 0.5)
        // 9: [-0.25, 0.25)
        int32_t ar_coeff_shift;  // values : 6..9
        int32_t grain_scale_shift;

        int32_t cb_mult;       // 8 bits
        int32_t cb_luma_mult;  // 8 bits
        int32_t cb_offset;     // 9 bits

        int32_t cr_mult;       // 8 bits
        int32_t cr_luma_mult;  // 8 bits
        int32_t cr_offset;     // 9 bits

        int32_t overlap_flag;

        int32_t clip_to_restricted_range;

        int32_t BitDepth;  // video bit depth
    };

    struct  SizeOfFrame{
        uint32_t FrameWidth;
        uint32_t FrameHeight;
    };

    struct FrameHeader
    {
        //Rev 0.85 parameters (AV1 spec version 1.0) in order of appearance/calculation in uncompressed_header()
        uint32_t show_existing_frame;
        uint32_t frame_to_show_map_idx;
        uint64_t frame_presentation_time;
        uint32_t display_frame_id;
        FRAME_TYPE frame_type;
        uint32_t show_frame;
        uint32_t showable_frame;
        uint32_t error_resilient_mode;
        uint32_t disable_cdf_update;
        uint32_t allow_screen_content_tools;
        uint32_t force_integer_mv;
        uint32_t current_frame_id;
        uint32_t frame_size_override_flag;
        uint32_t order_hint;
        uint32_t primary_ref_frame;

        uint8_t refresh_frame_flags;
        uint32_t ref_order_hint[NUM_REF_FRAMES];

        uint32_t FrameWidth;
        uint32_t FrameHeight;
        uint32_t SuperresDenom;
        uint32_t UpscaledWidth;
        uint32_t MiCols;
        uint32_t MiRows;
        uint32_t RenderWidth;
        uint32_t RenderHeight;

        uint32_t allow_intrabc;
        int32_t ref_frame_idx[INTER_REFS];
        uint32_t allow_high_precision_mv;
        INTERP_FILTER interpolation_filter;
        uint32_t is_motion_mode_switchable;
        uint32_t use_ref_frame_mvs;
        uint32_t disable_frame_end_update_cdf;

        uint32_t sbCols;
        uint32_t sbRows;
        uint32_t sbSize;

        TileInfo tile_info;
        QuantizationParams quantization_params;
        SegmentationParams segmentation_params;

        uint32_t delta_q_present;
        uint32_t delta_q_res;

        uint32_t delta_lf_present;
        uint32_t delta_lf_res;
        uint32_t delta_lf_multi;

        uint32_t CodedLossless;
        uint32_t AllLossless;

        LoopFilterParams loop_filter_params;
        CdefParams cdef_params;
        LRParams lr_params;

        TX_MODE TxMode;
        uint32_t reference_mode;
        uint32_t skip_mode_present;
        uint32_t allow_warped_motion;
        uint32_t reduced_tx_set;

        GlobalMotionParams global_motion_params[TOTAL_REFS];

        FilmGrainParams film_grain_params;

        uint32_t NumPlanes;

        uint32_t large_scale_tile;

        //Rev 0.5 parameters
        uint32_t enable_interintra_compound;
        uint32_t enable_masked_compound;
        uint32_t enable_intra_edge_filter;
        uint32_t enable_filter_intra;
    };

    struct OBUHeader
    {
        AV1_OBU_TYPE obu_type;
        uint32_t obu_has_size_field;
        uint32_t temporal_id;
        uint32_t spatial_id;
    };

    struct OBUInfo
    {
        OBUHeader header;
        size_t size;
    };

    struct TileGroupInfo
    {
        uint32_t numTiles;
        uint32_t startTileIdx;
        uint32_t endTileIdx;
    };

    class av1_exception
        : public std::runtime_error
    {
    public:
        av1_exception(int32_t status)
            : std::runtime_error("AV1 error")
            , m_status(status)
        {}
        int32_t GetStatus() const
        {
            return m_status;
        }
    private:
        int32_t m_status;
    };
}

#endif // __UMC_AV1_DEC_DEFS_DEC_H__
#endif // MFX_ENABLE_AV1_VIDEO_DECODE
