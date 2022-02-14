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

#ifndef __UMC_H264_DEC_DEFS_DEC_H__
#define __UMC_H264_DEC_DEFS_DEC_H__


#include <vector>
#include "umc_structures.h"

namespace UMC
{

// NAL unit definitions
enum
{
    NAL_STORAGE_IDC_BITS   = 0x60,
    NAL_UNITTYPE_BITS      = 0x1f
};


// Although the standard allows for a minimum width or height of 4, this
// implementation restricts the minimum value to 32.

enum {
    FLD_STRUCTURE       = 0,
    TOP_FLD_STRUCTURE   = 0,
    BOTTOM_FLD_STRUCTURE    = 1,
    FRM_STRUCTURE   = 2,
    AFRM_STRUCTURE  = 3
};

enum DisplayPictureStruct {
    DPS_UNKNOWN   = 100,
    DPS_FRAME     = 0,
    DPS_TOP,         // one field
    DPS_BOTTOM,      // one field
    DPS_TOP_BOTTOM,
    DPS_BOTTOM_TOP,
    DPS_TOP_BOTTOM_TOP,
    DPS_BOTTOM_TOP_BOTTOM,
    DPS_FRAME_DOUBLING,
    DPS_FRAME_TRIPLING
};

enum    // Valid QP range
{
    QP_MAX = 51,
    QP_MIN = 0
};

enum
{
    LIST_0 = 0,     // Ref/mvs list 0 (L0)
    LIST_1 = 1      // Ref/mvs list 1 (L1)
};

// default plane & coeffs types:
typedef uint8_t PlaneYCommon;
typedef uint8_t PlaneUVCommon;

typedef PlaneYCommon * PlanePtrYCommon;
typedef PlaneUVCommon * PlanePtrUVCommon;

typedef enum {
    NAL_UT_UNSPECIFIED  = 0, // Unspecified
    NAL_UT_SLICE     = 1, // Coded Slice - slice_layer_no_partioning_rbsp
    NAL_UT_DPA       = 2, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_DPB       = 3, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_DPC       = 4, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_IDR_SLICE = 5, // Coded Slice of a IDR Picture - slice_layer_no_partioning_rbsp
    NAL_UT_SEI       = 6, // Supplemental Enhancement Information - sei_rbsp
    NAL_UT_SPS       = 7, // Sequence Parameter Set - seq_parameter_set_rbsp
    NAL_UT_PPS       = 8, // Picture Parameter Set - pic_parameter_set_rbsp
    NAL_UT_AUD        = 9, // Access Unit Delimiter - access_unit_delimiter_rbsp
    NAL_UT_END_OF_SEQ   = 10, // End of sequence end_of_seq_rbsp()
    NAL_UT_END_OF_STREAM = 11, // End of stream end_of_stream_rbsp
    NAL_UT_FD        = 12, // Filler Data - filler_data_rbsp
    NAL_UT_SPS_EX    = 13, // Sequence Parameter Set Extension - seq_parameter_set_extension_rbsp
    NAL_UT_PREFIX  = 14, // Prefix NAL unit in scalable extension - prefix_nal_unit_rbsp
    NAL_UT_SUBSET_SPS = 15, // Subset Sequence Parameter Set - subset_seq_parameter_set_rbsp
    NAL_UT_AUXILIARY = 19, // Auxiliary coded picture
    NAL_UT_CODED_SLICE_EXTENSION = 20 // Coded slice in scalable/multiview extension
} NAL_Unit_Type;

// Note!  The Picture Code Type values below are no longer used in the
// core encoder.   It only knows about slice types, and whether or not
// the frame is IDR, Reference or Disposable.  See enum above.

enum EnumSliceCodType        // Permitted MB Prediction Types
{                        // ------------------------------------
    PREDSLICE      = 0,    // I (Intra), P (Pred)
    BPREDSLICE     = 1, // I, P, B (BiPred)
    INTRASLICE     = 2,    // I
    S_PREDSLICE    = 3,    // SP (SPred), I
    S_INTRASLICE   = 4    // SI (SIntra), I
};

inline
FrameType SliceTypeToFrameType(EnumSliceCodType slice_type)
{
    switch(slice_type)
    {
    case PREDSLICE:
    case S_PREDSLICE:
        return P_PICTURE;
    case BPREDSLICE:
        return B_PICTURE;
    case INTRASLICE:
    case S_INTRASLICE:
        return I_PICTURE;
    }

    return NONE_PICTURE;
}

typedef enum
{
    SEI_BUFFERING_PERIOD_TYPE                   = 0,
    SEI_PIC_TIMING_TYPE                         = 1,
    SEI_PAN_SCAN_RECT_TYPE                      = 2,
    SEI_FILLER_TYPE                             = 3,
    SEI_USER_DATA_REGISTERED_TYPE               = 4,
    SEI_USER_DATA_UNREGISTERED_TYPE             = 5,
    SEI_RECOVERY_POINT_TYPE                     = 6,
    SEI_DEC_REF_PIC_MARKING_TYPE                = 7,
    SEI_SPARE_PIC_TYPE                          = 8,
    SEI_SCENE_INFO_TYPE                         = 9,
    SEI_SUB_SEQ_INFO_TYPE                       = 10,
    SEI_SUB_SEQ_LAYER_TYPE                      = 11,
    SEI_SUB_SEQ_TYPE                            = 12,
    SEI_FULL_FRAME_FREEZE_TYPE                  = 13,
    SEI_FULL_FRAME_FREEZE_RELEASE_TYPE          = 14,
    SEI_FULL_FRAME_SNAPSHOT_TYPE                = 15,
    SEI_PROGRESSIVE_REF_SEGMENT_START_TYPE      = 16,
    SEI_PROGRESSIVE_REF_SEGMENT_END_TYPE        = 17,
    SEI_MOTION_CONSTRAINED_SG_SET_TYPE          = 18,
    SEI_FILM_GRAIN_CHARACTERISTICS              = 19,

    SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE    = 20,
    SEI_STEREO_VIDEO_INFO                       = 21,
    SEI_POST_FILTER_HINT                        = 22,
    SEI_TONE_MAPPING_INFO                       = 23,
    SEI_SCALABILITY_INFO                        = 24,
    SEI_SUB_PIC_SCALABLE_LAYER                  = 25,
    SEI_NON_REQUIRED_LAYER_REP                  = 26,
    SEI_PRIORITY_LAYER_INFO                     = 27,
    SEI_LAYERS_NOT_PRESENT                      = 28,
    SEI_LAYER_DEPENDENCY_CHANGE                 = 29,
    SEI_SCALABLE_NESTING                        = 30,
    SEI_BASE_LAYER_TEMPORAL_HRD                 = 31,
    SEI_QUALITY_LAYER_INTEGRITY_CHECK           = 32,
    SEI_REDUNDANT_PIC_PROPERTY                  = 33,
    SEI_TL0_DEP_REP_INDEX                       = 34,
    SEI_TL_SWITCHING_POINT                      = 35,
    SEI_PARALLEL_DECODING_INFO                  = 36,
    SEI_MVC_SCAOLABLE_NESTING                   = 37,
    SEI_VIEW_SCALABILITY_INFO                   = 38,
    SEI_MULTIVIEW_SCENE_INFO                    = 39,
    SEI_MULTIVIEW_ACQUISITION_INFO              = 40,
    SEI_NONT_REQUERED_VIEW_COMPONENT            = 41,
    SEI_VIEW_DEPENDENCY_CHANGE                  = 42,
    SEI_OPERATION_POINTS_NOT_PRESENT            = 43,
    SEI_BASE_VIEW_TEMPORAL_HRD                  = 44,
    SEI_FRAME_PACKING_ARRANGEMENT               = 45,

    SEI_RESERVED                                = 50,

    SEI_NUM_MESSAGES

} SEI_TYPE;

#define IS_SKIP_DEBLOCKING_MODE_NON_REF (m_PermanentTurnOffDeblocking == 1)
#define IS_SKIP_DEBLOCKING_MODE_PERMANENT (m_PermanentTurnOffDeblocking == 2)
#define IS_SKIP_DEBLOCKING_MODE_PREVENTIVE (m_PermanentTurnOffDeblocking == 3)

enum //  unsigned enum
{
    INVALID_DPB_OUTPUT_DELAY = 0xffffffff,
};

enum // usual int
{
    MAX_NUM_SEQ_PARAM_SETS = 32,
    MAX_NUM_PIC_PARAM_SETS = 256,

    MAX_NUM_REF_FRAMES  = 32,
    MAX_NUM_REF_FIELDS  = 32,

    MAX_REF_FRAMES_IN_POC_CYCLE = 256,

    MAX_NUM_SLICE_GROUPS        = 8,
    MAX_SLICE_GROUP_MAP_TYPE    = 6,

    NUM_INTRA_TYPE_ELEMENTS     = 16,

    COEFFICIENTS_BUFFER_SIZE    = 16 * 51,

    MINIMAL_DATA_SIZE           = 4,
    MAX_NUM_LAYERS              = 16,

    DEFAULT_NU_HEADER_TAIL_VALUE   = 0, // for sei
    DEFAULT_NU_TAIL_VALUE       = 0,
    DEFAULT_NU_TAIL_SIZE        = 128,
    DEFAULT_NU_SLICE_TAIL_SIZE  = 512,
};

// Possible values for disable_deblocking_filter_idc:
enum DeblockingModes_t
{
    DEBLOCK_FILTER_ON                   = 0,
    DEBLOCK_FILTER_OFF                  = 1,
    DEBLOCK_FILTER_ON_NO_SLICE_EDGES             = 2,
    DEBLOCK_FILTER_ON_2_PASS                     = 3,
    DEBLOCK_FILTER_ON_NO_CHROMA                  = 4,
    DEBLOCK_FILTER_ON_NO_SLICE_EDGES_NO_CHROMA   = 5,
    DEBLOCK_FILTER_ON_2_PASS_NO_CHROMA           = 6
};

enum
{
    H264_MAX_DEPENDENCY_ID      = 7, // svc decoder
    H264_MAX_QUALITY_ID         = 15,// svc decoder
    H264_MAX_TEMPORAL_ID        = 7, // svc decoder

    H264_MAX_NUM_VIEW           = 1024,
    H264_MAX_NUM_OPS            = 64,
    H264_MAX_VUI_EXT_NUM        = 1024,
    H264_MAX_NUM_VIEW_REF       = 16
};

struct H264ScalingList4x4
{
    uint8_t ScalingListCoeffs[16];
};

struct H264ScalingList8x8
{
    uint8_t ScalingListCoeffs[64];
};

struct H264WholeQPLevelScale4x4
{
    int16_t LevelScaleCoeffs[88]/*since we do not support 422 and 444*/[16];
};
struct H264WholeQPLevelScale8x8
{
    int16_t LevelScaleCoeffs[88]/*since we do not support 422 and 444*/[64];
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RefCounter
{
public:

    RefCounter() : m_refCounter(0)
    {
    }

    void IncrementReference() const;

    void DecrementReference();

    void ResetRefCounter() {m_refCounter = 0;}

    uint32_t GetRefCounter() {return m_refCounter;}

protected:
    volatile mutable int32_t m_refCounter;

    virtual ~RefCounter()
    {
    }

    virtual void Free()
    {
    }
};

class HeapObject : public RefCounter
{
public:

    virtual ~HeapObject() {}

    virtual void Reset()
    {
    }

    virtual void Free();
};

}  // end namespace UMC

namespace UMC_H264_DECODER
{

struct H264VUI
{
    void Reset()
    {
        H264VUI vui = {};
        // set some parameters by default
        vui.video_format = 5; // unspecified
        vui.video_full_range_flag = 0;
        vui.colour_primaries = 2; // unspecified
        vui.transfer_characteristics = 2; // unspecified
        vui.matrix_coefficients = 2; // unspecified

        *this = vui;
    }

    uint8_t        aspect_ratio_info_present_flag;
    uint8_t        aspect_ratio_idc;
    uint16_t       sar_width;
    uint16_t       sar_height;
    uint8_t        overscan_info_present_flag;
    uint8_t        overscan_appropriate_flag;
    uint8_t        video_signal_type_present_flag;
    uint8_t        video_format;
    uint8_t        video_full_range_flag;
    uint8_t        colour_description_present_flag;
    uint8_t        colour_primaries;
    uint8_t        transfer_characteristics;
    uint8_t        matrix_coefficients;
    uint8_t        chroma_loc_info_present_flag;
    uint8_t        chroma_sample_loc_type_top_field;
    uint8_t        chroma_sample_loc_type_bottom_field;
    uint8_t        timing_info_present_flag;
    uint32_t       num_units_in_tick;
    uint32_t       time_scale;
    uint8_t        fixed_frame_rate_flag;
    uint8_t        nal_hrd_parameters_present_flag;
    uint8_t        vcl_hrd_parameters_present_flag;
    uint8_t        low_delay_hrd_flag;
    uint8_t        pic_struct_present_flag;
    uint8_t        bitstream_restriction_flag;
    uint8_t        motion_vectors_over_pic_boundaries_flag;
    uint8_t        max_bytes_per_pic_denom;
    uint8_t        max_bits_per_mb_denom;
    uint8_t        log2_max_mv_length_horizontal;
    uint8_t        log2_max_mv_length_vertical;
    uint8_t        num_reorder_frames;
    uint8_t        max_dec_frame_buffering;
    //hrd_parameters
    uint8_t        cpb_cnt;
    uint8_t        bit_rate_scale;
    uint8_t        cpb_size_scale;
    uint32_t       bit_rate_value[32];
    uint32_t       cpb_size_value[32];
    uint8_t        cbr_flag[32];
    uint8_t        initial_cpb_removal_delay_length;
    uint8_t        cpb_removal_delay_length;
    uint8_t        dpb_output_delay_length;
    uint8_t        time_offset_length;
};

// Sequence parameter set structure, corresponding to the H.264 bitstream definition.
struct H264SeqParamSetBase
{
    uint8_t        profile_idc;                        // baseline, main, etc.
    uint8_t        level_idc;
    uint8_t        constraint_set0_flag;
    uint8_t        constraint_set1_flag;
    uint8_t        constraint_set2_flag;
    uint8_t        constraint_set3_flag;
    uint8_t        constraint_set4_flag;
    uint8_t        constraint_set5_flag;
    uint8_t        chroma_format_idc;
    uint8_t        residual_colour_transform_flag;
    uint8_t        bit_depth_luma;
    uint8_t        bit_depth_chroma;
    uint8_t        qpprime_y_zero_transform_bypass_flag;
    uint8_t        type_of_scaling_list_used[8];
    uint8_t        seq_scaling_matrix_present_flag;
    UMC::H264ScalingList4x4 ScalingLists4x4[6];
    UMC::H264ScalingList8x8 ScalingLists8x8[2];
    uint8_t        gaps_in_frame_num_value_allowed_flag;
    uint8_t        frame_cropping_flag;
    uint32_t       frame_cropping_rect_left_offset;
    uint32_t       frame_cropping_rect_right_offset;
    uint32_t       frame_cropping_rect_top_offset;
    uint32_t       frame_cropping_rect_bottom_offset;

    uint8_t        seq_parameter_set_id;                // id of this sequence parameter set
    uint8_t        log2_max_frame_num;                  // Number of bits to hold the frame_num
    uint8_t        pic_order_cnt_type;                  // Picture order counting method

    uint8_t        delta_pic_order_always_zero_flag;    // If zero, delta_pic_order_cnt fields are
                                                      // present in slice header.
    uint8_t        frame_mbs_only_flag;                 // Nonzero indicates all pictures in sequence
                                                      // are coded as frames (not fields).

    uint8_t        mb_adaptive_frame_field_flag;        // Nonzero indicates frame/field switch
                                                      // at macroblock level
    uint8_t        direct_8x8_inference_flag;           // Direct motion vector derivation method
    uint8_t        vui_parameters_present_flag;         // Zero indicates default VUI parameters
    uint32_t       log2_max_pic_order_cnt_lsb;          // Value of MaxPicOrderCntLsb.
    int32_t       offset_for_non_ref_pic;

    int32_t       offset_for_top_to_bottom_field;      // Expected pic order count difference from
                                                      // top field to bottom field.

    uint32_t       num_ref_frames_in_pic_order_cnt_cycle;
    uint32_t       num_ref_frames;                      // total number of pics in decoded pic buffer
    uint32_t       frame_width_in_mbs;
    uint32_t       frame_height_in_mbs;

    // These fields are calculated from values above.  They are not written to the bitstream
    uint32_t       MaxPicOrderCntLsb;

    // vui part
    H264VUI      vui;

    int32_t       poffset_for_ref_frame[UMC::MAX_REF_FRAMES_IN_POC_CYCLE];  // for pic order cnt type 1

    uint8_t        errorFlags;

    void Reset()
    {
        H264SeqParamSetBase sps = {};
        *this = sps;
    }

};    // H264SeqParamSet

// Sequence parameter set structure, corresponding to the H.264 bitstream definition.
struct H264SeqParamSet : public UMC::HeapObject, public H264SeqParamSetBase
{
    H264SeqParamSet()
        : HeapObject()
        , H264SeqParamSetBase()
    {
        Reset();
    }

    ~H264SeqParamSet()
    {
    }

    int32_t GetID() const
    {
        return seq_parameter_set_id;
    }

    virtual void Reset()
    {
        H264SeqParamSetBase::Reset();

        seq_parameter_set_id = UMC::MAX_NUM_SEQ_PARAM_SETS;
        vui.Reset();
    }
};    // H264SeqParamSet

// Sequence parameter set extension structure, corresponding to the H.264 bitstream definition.
struct H264SeqParamSetExtension : public UMC::HeapObject
{
    uint8_t       seq_parameter_set_id;
    uint8_t       aux_format_idc;
    uint8_t       bit_depth_aux;
    uint8_t       alpha_incr_flag;
    uint8_t       alpha_opaque_value;
    uint8_t       alpha_transparent_value;
    uint8_t       additional_extension_flag;

    H264SeqParamSetExtension()
    {
        Reset();
    }

    virtual void Reset()
    {
        aux_format_idc = 0;
        seq_parameter_set_id = UMC::MAX_NUM_SEQ_PARAM_SETS;    // illegal id
        bit_depth_aux = 0;
        alpha_incr_flag = 0;
        alpha_opaque_value = 0;
        alpha_transparent_value = 0;
        additional_extension_flag = 0;
    }

    int32_t GetID() const
    {
        return seq_parameter_set_id;
    }
};    // H264SeqParamSetExtension

// MVC view's reference info structure
struct H264ViewRefInfo
{
    // ue(v) specifies the view_id of the view with VOIdx equal to i.
    uint32_t view_id;

    // ue(v) specifies the number of view components for inter-view prediction
    // in the initialised RefPicListX in decoding anchor view components with
    // VOIdx equal to i. The value of num_anchor_refs_lx[ i ] shall not
    // be greater than 15.
    uint8_t num_anchor_refs_lx[2];
    // ue(v) specifies the view_id of the j-th view component for inter-view
    // prediction in the initialised RefPicListX in decoding anchor view
    // components with VOIdx equal to i.
    uint16_t anchor_refs_lx[2][UMC::H264_MAX_NUM_VIEW_REF];

    // ue(v) specifies the number of view components for inter-view prediction
    // in the initialised RefPicListX in decoding non-anchor view components
    // with VOIdx equal to i. The value of num_non_anchor_refs_lx[ i ]
    // shall not be greater than 15
    uint8_t num_non_anchor_refs_lx[2];
    // ue(v) specifies the view_id of the j-th view component for inter-view
    // prediction in the initialised RefPicListX in decoding non-anchor view
    // components with VOIdx equal to i.
    uint16_t non_anchor_refs_lx[2][UMC::H264_MAX_NUM_VIEW_REF];

};

struct H264ApplicableOp
{
    // u(3) specifies the temporal_id of the j-th operation point to which the
    // the level indicated by level_idc[ i ] applies.
    uint8_t applicable_op_temporal_id;
    // ue(v) plus 1 specifies the number of target output views for the j-th
    // operation point to which the level indicated by level_idc[ i ] applies.
    // The value of applicable_op_num_target_views_minus1[ i ][ j ] shall be in
    // the range of 0 to 1023, inclusive.
    uint16_t applicable_op_num_target_views_minus1;
    // ue(v) specifies the k-th target output view for the j-th operation point
    // to which the the level indicated by level_idc[ i ] applies. The value of
    // applicable_op_num_target_views_minus1[ i ][ j ] shall be in the
    // range of 0 to 1023, inclusive.
    std::vector<uint16_t> applicable_op_target_view_id;
    // ue(v) plus 1 specifies the number of views required for decoding the
    // target output views corresponding to the j-th operation point to which
    // the level indicated by level_idc[ i ] applies.
    uint16_t applicable_op_num_views_minus1;

};

struct H264LevelValueSignaled
{
    // u(8) specifies the i-th level value signalled for the coded video sequence.
    uint8_t level_idc;
    // ue(v) plus 1 specifies the number of operation points to which the level
    // indicated by level_idc[ i ] applies. The value of
    // num_applicable_ops_minus1[ i ] shall be in the range of 0 to 1023, inclusive.
    uint16_t num_applicable_ops_minus1;

    // Array of applicable operation points for the current level value signalled
    std::vector<H264ApplicableOp> opsInfo;

};

// MVC extensions for sequence parameter set
struct H264SeqParamSetMVCExtension : public H264SeqParamSet
{
    // ue(v) plus 1 specifies the maximum number of coded views in the coded
    // video sequence. The value of num_view_minus1 shall be in the range of
    // 0 to 1023, inclusive
    uint32_t num_views_minus1;

    // Array of views reference info structures.
    std::vector<H264ViewRefInfo> viewInfo;

    // ue(v) plus 1 specifies the number of level values signalled for
    // the coded video sequence. The value of num_level_values_signalled_minus1
    // shall be in the range of 0 to 63, inclusive.
    uint32_t num_level_values_signalled_minus1;

    // Array of level value signaled structures
    std::vector<H264LevelValueSignaled> levelInfo;

    virtual void Reset()
    {
        //H264SeqParamSetBase::Reset();

        num_views_minus1 = 0;
        num_level_values_signalled_minus1 = 0;
        //viewInfo.Clean();
        //levelInfo.Clean();
    }
};

// SVC extensions for sequence parameter set
struct H264SeqParamSetSVCExtension : public H264SeqParamSet
{
    /* For scalable stream */
    uint8_t inter_layer_deblocking_filter_control_present_flag;
    uint8_t extended_spatial_scalability;
    int8_t chroma_phase_x;
    int8_t chroma_phase_y;
    int8_t seq_ref_layer_chroma_phase_x;
    int8_t seq_ref_layer_chroma_phase_y;

    uint8_t seq_tcoeff_level_prediction_flag;
    uint8_t adaptive_tcoeff_level_prediction_flag;
    uint8_t slice_header_restriction_flag;

    int32_t seq_scaled_ref_layer_left_offset;
    int32_t seq_scaled_ref_layer_top_offset;
    int32_t seq_scaled_ref_layer_right_offset;
    int32_t seq_scaled_ref_layer_bottom_offset;

    virtual void Reset()
    {
        //H264SeqParamSetBase::Reset();

        inter_layer_deblocking_filter_control_present_flag = 0;
        extended_spatial_scalability = 0;
        chroma_phase_x = 0;
        chroma_phase_y = 0;
        seq_ref_layer_chroma_phase_x = 0;
        seq_ref_layer_chroma_phase_y = 0;

        seq_tcoeff_level_prediction_flag = 0;
        adaptive_tcoeff_level_prediction_flag = 0;
        slice_header_restriction_flag = 0;

        seq_scaled_ref_layer_left_offset = 0;
        seq_scaled_ref_layer_top_offset = 0;
        seq_scaled_ref_layer_right_offset = 0;
        seq_scaled_ref_layer_bottom_offset = 0;
    }
};

struct H264ScalingPicParams
{
    UMC::H264ScalingList4x4 ScalingLists4x4[6];
    UMC::H264ScalingList8x8 ScalingLists8x8[2];

    // Level Scale addition
    UMC::H264WholeQPLevelScale4x4        m_LevelScale4x4[6];
    UMC::H264WholeQPLevelScale8x8        m_LevelScale8x8[2];
};

// Picture parameter set structure, corresponding to the H.264 bitstream definition.
struct H264PicParamSetBase
{
// Flexible macroblock order structure, defining the FMO map for a picture
// paramter set.

    struct SliceGroupInfoStruct
    {
        uint8_t        slice_group_map_type;                // 0..6

        // The additional slice group data depends upon map type
        union
        {
            // type 0
            uint32_t    run_length[UMC::MAX_NUM_SLICE_GROUPS];

            // type 2
            struct
            {
                uint32_t top_left[UMC::MAX_NUM_SLICE_GROUPS-1];
                uint32_t bottom_right[UMC::MAX_NUM_SLICE_GROUPS-1];
            }t1;

            // types 3-5
            struct
            {
                uint8_t  slice_group_change_direction_flag;
                uint32_t slice_group_change_rate;
            }t2;

            // type 6
            struct
            {
                uint32_t pic_size_in_map_units;     // number of macroblocks if no field coding
            }t3;
        };

        std::vector<uint8_t> pSliceGroupIDMap;          // Id for each slice group map unit (for t3 struct of)
    };    // SliceGroupInfoStruct

    uint16_t       pic_parameter_set_id;            // of this picture parameter set
    uint8_t        seq_parameter_set_id;            // of seq param set used for this pic param set
    uint8_t        entropy_coding_mode;             // zero: CAVLC, else CABAC

    uint8_t        bottom_field_pic_order_in_frame_present_flag; // Zero indicates only delta_pic_order_cnt[0] is
                                                  // present in slice header; nonzero indicates
                                                  // delta_pic_order_cnt[1] is also present.

    uint8_t        weighted_pred_flag;              // Nonzero indicates weighted prediction applied to
                                                  // P and SP slices
    uint8_t        weighted_bipred_idc;             // 0: no weighted prediction in B slices
                                                  // 1: explicit weighted prediction
                                                  // 2: implicit weighted prediction
    int8_t        pic_init_qp;                     // default QP for I,P,B slices
    int8_t        pic_init_qs;                     // default QP for SP, SI slices

    int8_t        chroma_qp_index_offset[2];       // offset to add to QP for chroma

    uint8_t        deblocking_filter_variables_present_flag;    // If nonzero, deblock filter params are
                                                  // present in the slice header.
    uint8_t        constrained_intra_pred_flag;     // Nonzero indicates constrained intra mode

    uint8_t        redundant_pic_cnt_present_flag;  // Nonzero indicates presence of redundant_pic_cnt
                                                  // in slice header
    uint32_t       num_slice_groups;                // One: no FMO
    uint32_t       num_ref_idx_l0_active;           // num of ref pics in list 0 used to decode the picture
    uint32_t       num_ref_idx_l1_active;           // num of ref pics in list 1 used to decode the picture
    uint8_t        transform_8x8_mode_flag;

    uint8_t pic_scaling_matrix_present_flag;
    uint8_t pic_scaling_list_present_flag[8];

    uint8_t        type_of_scaling_list_used[8];

    SliceGroupInfoStruct SliceGroupInfo;    // Used only when num_slice_groups > 1

    H264ScalingPicParams scaling[2];

    uint8_t initialized[2];

    uint8_t errorFlags;

    void Reset()
    {
        H264PicParamSetBase pps = {};
        *this = pps;
    }
};

// Picture parameter set structure, corresponding to the H.264 bitstream definition.
struct H264PicParamSet : public UMC::HeapObject, public H264PicParamSetBase
{
    H264PicParamSet()
        : H264PicParamSetBase()
    {
        Reset();
    }

    void Reset()
    {
        H264PicParamSetBase::Reset();

        pic_parameter_set_id = UMC::MAX_NUM_PIC_PARAM_SETS;
        seq_parameter_set_id = UMC::MAX_NUM_SEQ_PARAM_SETS;
        num_slice_groups = 0;
        SliceGroupInfo.pSliceGroupIDMap.clear();
    }

    ~H264PicParamSet()
    {
    }

    int32_t GetID() const
    {
        return pic_parameter_set_id;
    }

};    // H264PicParamSet

struct RefPicListReorderInfo
{
    uint32_t       num_entries;                 // number of currently valid idc,value pairs
    uint8_t        reordering_of_pic_nums_idc[UMC::MAX_NUM_REF_FRAMES];
    uint32_t       reorder_value[UMC::MAX_NUM_REF_FRAMES];    // abs_diff_pic_num or long_term_pic_num
};

struct AdaptiveMarkingInfo
{
    uint32_t       num_entries;                 // number of currently valid mmco,value pairs
    uint8_t        mmco[UMC::MAX_NUM_REF_FRAMES];    // memory management control operation id
    uint32_t       value[UMC::MAX_NUM_REF_FRAMES*2]; // operation-dependent data, max 2 per operation
};

struct PredWeightTable
{
    uint8_t        luma_weight_flag;            // nonzero: luma weight and offset in bitstream
    uint8_t        chroma_weight_flag;          // nonzero: chroma weight and offset in bitstream
    int8_t        luma_weight;                 // luma weighting factor
    int8_t        luma_offset;                 // luma weighting offset
    int8_t        chroma_weight[2];            // chroma weighting factor (Cb,Cr)
    int8_t        chroma_offset[2];            // chroma weighting offset (Cb,Cr)
};    // PredWeightTable

typedef int32_t H264DecoderMBAddr;
// NAL unit SVC extension structure
struct H264NalSvcExtension
{
    uint8_t idr_flag;
    uint8_t priority_id;
    uint8_t no_inter_layer_pred_flag;
    uint8_t dependency_id;
    uint8_t quality_id;
    uint8_t temporal_id;
    uint8_t use_ref_base_pic_flag;
    uint8_t discardable_flag;
    uint8_t output_flag;

    uint8_t store_ref_base_pic_flag;
    uint8_t adaptive_ref_base_pic_marking_mode_flag;
    AdaptiveMarkingInfo adaptiveMarkingInfo;

    void Reset()
    {
        idr_flag = 0;
        priority_id = 0;
        no_inter_layer_pred_flag = 0;
        dependency_id = 0;
        quality_id = 0;
        temporal_id = 0;
        use_ref_base_pic_flag = 0;
        discardable_flag = 0;
        output_flag = 0;

        store_ref_base_pic_flag = 0;
        adaptive_ref_base_pic_marking_mode_flag = 0;
    }
};

// NAL unit SVC extension structure
struct H264NalMvcExtension
{
    uint8_t non_idr_flag;
    uint16_t priority_id;
    // view_id variable is duplicated to the slice header itself
    uint16_t view_id;
    uint8_t temporal_id;
    uint8_t anchor_pic_flag;
    uint8_t inter_view_flag;

    uint8_t padding[3];

    void Reset()
    {
        non_idr_flag = 0;
        priority_id = 0;
        view_id = 0;
        temporal_id = 0;
        anchor_pic_flag = 0;
        inter_view_flag = 0;
    }
};

// NAL unit extension structure
struct H264NalExtension
{
    uint8_t extension_present;
    uint8_t svc_extension_flag; // equal to 1 specifies that NAL extension contains SVC related parameters

    H264NalSvcExtension svc;
    H264NalMvcExtension mvc;

    void Reset()
    {
        memset(this, 0, sizeof(H264NalExtension));
    }
};

// Slice header structure, corresponding to the H.264 bitstream definition.
struct H264SliceHeader
{
    // flag equal 1 means that the slice belong to IDR or anchor access unit
    uint32_t IdrPicFlag;

    // specified that NAL unit contains any information accessed from
    // the decoding process of other NAL units.
    uint32_t nal_ref_idc;
    // specifies the type of RBSP data structure contained in the NAL unit as
    // specified in Table 7-1 of h264 standard
    UMC::NAL_Unit_Type nal_unit_type;

    // NAL unit extension parameters
    H264NalExtension nal_ext;

    uint16_t        pic_parameter_set_id;                 // of pic param set used for this slice
    uint8_t         field_pic_flag;                       // zero: frame picture, else field picture
    uint8_t         MbaffFrameFlag;
    uint8_t         bottom_field_flag;                    // zero: top field, else bottom field
    uint8_t         direct_spatial_mv_pred_flag;          // zero: temporal direct, else spatial direct
    uint8_t         num_ref_idx_active_override_flag;     // nonzero: use ref_idx_active from slice header
                                                        // instead of those from pic param set
    uint8_t         no_output_of_prior_pics_flag;         // nonzero: remove previously decoded pictures
                                                        // from decoded picture buffer
    uint8_t         long_term_reference_flag;             // How to set MaxLongTermFrameIdx
    uint32_t        cabac_init_idc;                      // CABAC initialization table index (0..2)
    uint8_t         adaptive_ref_pic_marking_mode_flag;   // Ref pic marking mode of current picture
    int32_t        slice_qp_delta;                       // to calculate default slice QP
    uint8_t         sp_for_switch_flag;                   // SP slice decoding control
    int32_t        slice_qs_delta;                       // to calculate default SP,SI slice QS
    uint32_t        disable_deblocking_filter_idc;       // deblock filter control, 0=filter all edges
    int32_t        slice_alpha_c0_offset;               // deblock filter c0, alpha table offset
    int32_t        slice_beta_offset;                   // deblock filter beta table offset
    H264DecoderMBAddr first_mb_in_slice;
    int32_t        frame_num;
    UMC::EnumSliceCodType slice_type;
    uint32_t        idr_pic_id;                           // ID of an IDR picture
    int32_t        pic_order_cnt_lsb;                    // picture order count (mod MaxPicOrderCntLsb)
    int32_t        delta_pic_order_cnt_bottom;           // Pic order count difference, top & bottom fields
    uint32_t        difference_of_pic_nums;               // Ref pic memory mgmt
    uint32_t        long_term_pic_num;                    // Ref pic memory mgmt
    uint32_t        long_term_frame_idx;                  // Ref pic memory mgmt
    uint32_t        max_long_term_frame_idx;              // Ref pic memory mgmt
    int32_t        delta_pic_order_cnt[2];               // picture order count differences
    uint32_t        redundant_pic_cnt;                    // for redundant slices
    int32_t        num_ref_idx_l0_active;                // num of ref pics in list 0 used to decode the slice,
                                                        // see num_ref_idx_active_override_flag
    int32_t        num_ref_idx_l1_active;                // num of ref pics in list 1 used to decode the slice
                                                        // see num_ref_idx_active_override_flag
    uint32_t        slice_group_change_cycle;             // for FMO
    uint8_t         luma_log2_weight_denom;               // luma weighting denominator
    uint8_t         chroma_log2_weight_denom;             // chroma weighting denominator

    bool          is_auxiliary;

    /* for scalable stream */
    uint8_t         scan_idx_start;
    uint8_t         scan_idx_end;

    uint32_t        hw_wa_redundant_elimination_bits[3]; // [0] - start redundant element, [1] - end redundant element [2] - header size in bits
}; // H264SliceHeader

typedef struct
{
    uint32_t  layer_id;
    uint8_t   priority_id;
    uint8_t   discardable_flag;
    uint8_t   dependency_id;
    uint8_t   quality_id;
    uint8_t   temporal_id;

    uint32_t  frm_width_in_mbs;
    uint32_t  frm_height_in_mbs;

    // frm_rate_info_present_flag section
    uint32_t  constant_frm_rate_idc;
    uint32_t  avg_frm_rate;

    // layer_dependency_info_present_flag
    uint32_t num_directly_dependent_layers;
    uint32_t directly_dependent_layer_id_delta_minus1[256];
    uint32_t layer_dependency_info_src_layer_id_delta;

} scalability_layer_info;

struct H264SEIPayLoadBase
{
    UMC::SEI_TYPE payLoadType;
    uint32_t   payLoadSize;
    uint8_t    isValid;

    union SEIMessages
    {
        struct BufferingPeriod
        {
            uint32_t initial_cpb_removal_delay[2][32];
            uint32_t initial_cpb_removal_delay_offset[2][32];
        }buffering_period;

        struct PicTiming
        {
            int32_t cbp_removal_delay;
            int32_t dpb_output_delay;
            UMC::DisplayPictureStruct pic_struct;
            uint8_t  clock_timestamp_flag[16];
            struct ClockTimestamps
            {
                uint8_t ct_type;
                uint8_t nunit_field_based_flag;
                uint8_t counting_type;
                uint8_t full_timestamp_flag;
                uint8_t discontinuity_flag;
                uint8_t cnt_dropped_flag;
                uint8_t n_frames;
                uint8_t seconds_value;
                uint8_t minutes_value;
                uint8_t hours_value;
                uint8_t time_offset;
            }clock_timestamps[16];
        }pic_timing;

        struct PanScanRect
        {
            uint8_t  pan_scan_rect_id;
            uint8_t  pan_scan_rect_cancel_flag;
            uint8_t  pan_scan_cnt;
            uint32_t pan_scan_rect_left_offset[32];
            uint32_t pan_scan_rect_right_offset[32];
            uint32_t pan_scan_rect_top_offset[32];
            uint32_t pan_scan_rect_bottom_offset[32];
            uint8_t  pan_scan_rect_repetition_period;
        }pan_scan_rect;

        struct UserDataRegistered
        {
            uint8_t itu_t_t35_country_code;
            uint8_t itu_t_t35_country_code_extension_byte;
        } user_data_registered;

        struct RecoveryPoint
        {
            uint8_t recovery_frame_cnt;
            uint8_t exact_match_flag;
            uint8_t broken_link_flag;
            uint8_t changing_slice_group_idc;
        }recovery_point;

        struct DecRefPicMarkingRepetition
        {
            uint8_t original_idr_flag;
            uint8_t original_frame_num;
            uint8_t original_field_pic_flag;
            uint8_t original_bottom_field_flag;
            uint8_t long_term_reference_flag;
            AdaptiveMarkingInfo adaptiveMarkingInfo;
        }dec_ref_pic_marking_repetition;

        struct SparePic
        {
            uint32_t target_frame_num;
            uint8_t  spare_field_flag;
            uint8_t  target_bottom_field_flag;
            uint8_t  num_spare_pics;
            uint8_t  delta_spare_frame_num[16];
            uint8_t  spare_bottom_field_flag[16];
            uint8_t  spare_area_idc[16];
            uint8_t  *spare_unit_flag[16];
            uint8_t  *zero_run_length[16];
        }spare_pic;

        struct SceneInfo
        {
            uint8_t scene_info_present_flag;
            uint8_t scene_id;
            uint8_t scene_transition_type;
            uint8_t second_scene_id;
        }scene_info;

        struct SubSeqInfo
        {
            uint8_t sub_seq_layer_num;
            uint8_t sub_seq_id;
            uint8_t first_ref_pic_flag;
            uint8_t leading_non_ref_pic_flag;
            uint8_t last_pic_flag;
            uint8_t sub_seq_frame_num_flag;
            uint8_t sub_seq_frame_num;
        }sub_seq_info;

        struct SubSeqLayerCharacteristics
        {
            uint8_t  num_sub_seq_layers;
            uint8_t  accurate_statistics_flag[16];
            uint16_t average_bit_rate[16];
            uint16_t average_frame_rate[16];
        }sub_seq_layer_characteristics;

        struct SubSeqCharacteristics
        {
            uint8_t  sub_seq_layer_num;
            uint8_t  sub_seq_id;
            uint8_t  duration_flag;
            uint8_t  sub_seq_duration;
            uint8_t  average_rate_flag;
            uint8_t  accurate_statistics_flag;
            uint16_t average_bit_rate;
            uint16_t average_frame_rate;
            uint8_t  num_referenced_subseqs;
            uint8_t  ref_sub_seq_layer_num[16];
            uint8_t  ref_sub_seq_id[16];
            uint8_t  ref_sub_seq_direction[16];
        }sub_seq_characteristics;

        struct FullFrameFreeze
        {
            uint32_t full_frame_freeze_repetition_period;
        }full_frame_freeze;

        struct FullFrameSnapshot
        {
            uint8_t snapshot_id;
        }full_frame_snapshot;

        struct ProgressiveRefinementSegmentStart
        {
            uint8_t progressive_refinement_id;
            uint8_t num_refinement_steps;
        }progressive_refinement_segment_start;

        struct MotionConstrainedSliceGroupSet
        {
            uint8_t num_slice_groups_in_set;
            uint8_t slice_group_id[8];
            uint8_t exact_sample_value_match_flag;
            uint8_t pan_scan_rect_flag;
            uint8_t pan_scan_rect_id;
        }motion_constrained_slice_group_set;

        struct FilmGrainCharacteristics
        {
            uint8_t film_grain_characteristics_cancel_flag;
            uint8_t model_id;
            uint8_t separate_colour_description_present_flag;
            uint8_t film_grain_bit_depth_luma;
            uint8_t film_grain_bit_depth_chroma;
            uint8_t film_grain_full_range_flag;
            uint8_t film_grain_colour_primaries;
            uint8_t film_grain_transfer_characteristics;
            uint8_t film_grain_matrix_coefficients;
            uint8_t blending_mode_id;
            uint8_t log2_scale_factor;
            uint8_t comp_model_present_flag[3];
            uint8_t num_intensity_intervals[3];
            uint8_t num_model_values[3];
            uint8_t intensity_interval_lower_bound[3][256];
            uint8_t intensity_interval_upper_bound[3][256];
            uint8_t comp_model_value[3][3][256];
            uint8_t film_grain_characteristics_repetition_period;
        }film_grain_characteristics;

        struct DeblockingFilterDisplayPreference
        {
            uint8_t deblocking_display_preference_cancel_flag;
            uint8_t display_prior_to_deblocking_preferred_flag;
            uint8_t dec_frame_buffering_constraint_flag;
            uint8_t deblocking_display_preference_repetition_period;
        }deblocking_filter_display_preference;

        struct StereoVideoInfo
        {
            uint8_t field_views_flag;
            uint8_t top_field_is_left_view_flag;
            uint8_t current_frame_is_left_view_flag;
            uint8_t next_frame_is_second_view_flag;
            uint8_t left_view_self_contained_flag;
            uint8_t right_view_self_contained_flag;
        }stereo_video_info;

        struct ScalabilityInfo
        {
            uint8_t temporal_id_nesting_flag;
            uint8_t priority_layer_info_present_flag;
            uint8_t priority_id_setting_flag;

            uint32_t num_layers;
        } scalability_info;

    }SEI_messages;

    void Reset()
    {
        memset(this, 0, sizeof(H264SEIPayLoadBase));

        payLoadType = UMC::SEI_RESERVED;
        payLoadSize = 0;
    }
};

struct H264SEIPayLoad : public UMC::HeapObject, public H264SEIPayLoadBase
{
    std::vector<uint8_t> user_data; // for UserDataRegistered or UserDataUnRegistered

    H264SEIPayLoad()
        : H264SEIPayLoadBase()
    {
    }

    virtual void Reset()
    {
        H264SEIPayLoadBase::Reset();
        user_data.clear();
    }

    int32_t GetID() const
    {
        return payLoadType;
    }
};
} // end of namespace UMC_H264_DECODER

namespace UMC
{

enum
{
    CHROMA_FORMAT_400       = 0,
    CHROMA_FORMAT_420       = 1,
    CHROMA_FORMAT_422       = 2,
    CHROMA_FORMAT_444       = 3
};

///////////////// New structures

struct ReferenceFlags // flags use for reference frames of slice
{
    char field : 3;
    unsigned char isShortReference : 1;
    unsigned char isLongReference : 1;
};

class h264_exception
{
public:
    h264_exception(int32_t status = -1)
        : m_Status(status)
    {
    }

    virtual ~h264_exception()
    {
    }

    int32_t GetStatus() const
    {
        return m_Status;
    }

private:
    int32_t m_Status;
};

template <typename T>
inline T * h264_new_array_throw(int32_t size)
{
    T * t = new T[size];
    if (!t)
        throw h264_exception(UMC_ERR_ALLOC);
    return t;
}

template <typename T>
inline T * h264_new_throw()
{
    T * t = new T();
    if (!t)
        throw h264_exception(UMC_ERR_ALLOC);
    return t;
}

inline ColorFormat GetUMCColorFormat(int32_t color_format)
{
    ColorFormat format;
    switch(color_format)
    {
    case 0:
        format = GRAY;
        break;
    case 2:
        format = NV16;
        break;
    case 3:
        format = YUV444;
        break;
    case 1:
    default:
        format = NV12;
        break;
    }

    return format;
}

inline int32_t GetH264ColorFormat(ColorFormat color_format)
{
    int32_t format;
    switch(color_format)
    {
    case GRAY:
    case GRAYA:
        format = 0;
        break;
    case YUV422A:
    case YUV422:
    case NV16:
        format = 2;
        break;
    case YUV444:
    case YUV444A:
        format = 3;
        break;
    case YUV420:
    case YUV420A:
    case NV12:
    case YV12:
    default:
        format = 1;
        break;
    }

    return format;
}

inline size_t CalculateSuggestedSize(const UMC_H264_DECODER::H264SeqParamSet * sps)
{
    size_t base_size = sps->frame_width_in_mbs * sps->frame_height_in_mbs * 256;
    size_t size = 0;

    switch (sps->chroma_format_idc)
    {
    case 0:  // YUV400
        size = base_size;
        break;
    case 1:  // YUV420
        size = (base_size * 3) / 2;
        break;
    case 2: // YUV422
        size = base_size + base_size;
        break;
    case 3: // YUV444
        size = base_size + base_size + base_size;
        break;
    };

    return size;
}

extern const uint32_t SubWidthC[4];
extern const uint32_t SubHeightC[4];

} // end namespace UMC


#endif // __UMC_H264_DEC_DEFS_DEC_H__
#endif // MFX_ENABLE_H264_VIDEO_DECODE
