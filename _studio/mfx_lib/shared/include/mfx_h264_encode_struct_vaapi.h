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

//#include "mfx_common.h"
#ifndef __MFX_H264_ENCODE_STRUCT_VAAPI__H
#define __MFX_H264_ENCODE_STRUCT_VAAPI__H

#include "mfx_common.h"

#if defined (MFX_VA_LINUX)

#include "mfx_platform_headers.h"


#define D3DDDIFMT_INTELENCODE_BITSTREAMDATA     (D3DDDIFORMAT)164 // D3DDDIFMT_DXVA_RESERVED14
#define D3DDDIFMT_INTELENCODE_MBDATA            (D3DDDIFORMAT)165 // D3DDDIFMT_DXVA_RESERVED15
#define D3DDDIFMT_INTELENCODE_MBSEGMENTMAP      (D3DDDIFORMAT)181

typedef struct tagENCODE_COMP_BUFFER_INFO
{
    D3DFORMAT    Type;
    D3DFORMAT   CompressedFormats;
    USHORT        CreationWidth;
    USHORT        CreationHeight;
    UCHAR        NumBuffer;
    UCHAR        AllocationType;
} ENCODE_COMP_BUFFER_INFO;

typedef enum tagENCODE_FUNC
{
    ENCODE_ENC        = 0x0001,
    ENCODE_PAK        = 0x0002,
    ENCODE_ENC_PAK    = 0x0004,
} ENCODE_FUNC;

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define frame encoding type
///////////////////////////////////////////////////////////////////////////////
enum
{
    CODING_TYPE_I   = 1,
    CODING_TYPE_P   = 2,
    CODING_TYPE_B   = 3,
};

enum
{
    SLICE_TYPE_P = 0,
    SLICE_TYPE_B = 1,
    SLICE_TYPE_I = 2
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define Intra MB mode
///////////////////////////////////////////////////////////////////////////////
enum    //for IntraMbMode Field
{
    INTRA_MB_MODE_16x16 = 0,
    INTRA_MB_MODE_8x8   = 1,
    INTRA_MB_MODE_4x4   = 2,
    INTRA_MB_MODE_IPCM  = 3,
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define Intra MB Type
///////////////////////////////////////////////////////////////////////////////
enum    //Intra MbType
{
    MBTYPE_I_4x4            = 0,
    MBTYPE_I_8x8            = 0,
    MBTYPE_I_16x16_000      = 1,
    MBTYPE_I_16x16_100      = 2,
    MBTYPE_I_16x16_200      = 3,
    MBTYPE_I_16x16_300      = 4,
    MBTYPE_I_16x16_010      = 5,
    MBTYPE_I_16x16_110      = 6,
    MBTYPE_I_16x16_210      = 7,
    MBTYPE_I_16x16_310      = 8,
    MBTYPE_I_16x16_020      = 0,
    MBTYPE_I_16x16_120      = 0xA,
    MBTYPE_I_16x16_220      = 0xB,
    MBTYPE_I_16x16_320      = 0xC,
    MBTYPE_I_16x16_001      = 0xD,
    MBTYPE_I_16x16_101      = 0xE,
    MBTYPE_I_16x16_201      = 0xF,
    MBTYPE_I_16x16_301      = 0x10,
    MBTYPE_I_16x16_011      = 0x11,
    MBTYPE_I_16x16_111      = 0x12,
    MBTYPE_I_16x16_211      = 0x13,
    MBTYPE_I_16x16_311      = 0x14,
    MBTYPE_I_16x16_021      = 0x15,
    MBTYPE_I_16x16_121      = 0x16,
    MBTYPE_I_16x16_221      = 0x17,
    MBTYPE_I_16x16_321      = 0x18,
    MBTYPE_I_IPCM           = 0x19,
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define Inter MB Type
///////////////////////////////////////////////////////////////////////////////
enum    //Inter MbType
{
    MBTYPE_BP_L0_16x16      = 1,
    MBTYPE_B_L1_16x16       = 2,
    MBTYPE_B_Bi_16x16       = 3,
    MBTYPE_BP_L0_L0_16x8    = 4,
    MBTYPE_BP_L0_L0_8x16    = 5,
    MBTYPE_B_L1_L1_16x8     = 6,
    MBTYPE_B_L1_L1_8x16     = 7,
    MBTYPE_B_L0_L1_16x8     = 8,
    MBTYPE_B_L0_L1_8x16     = 9,
    MBTYPE_B_L1_L0_16x8     = 0xA,
    MBTYPE_B_L1_L0_8x16     = 0xB,
    MBTYPE_B_L0_Bi_16x8     = 0xC,
    MBTYPE_B_L0_Bi_8x16     = 0xD,
    MBTYPE_B_L1_Bi_16x8     = 0xE,
    MBTYPE_B_L1_Bi_8x16     = 0xF,
    MBTYPE_B_Bi_L0_16x8     = 0x10,
    MBTYPE_B_Bi_L0_8x16     = 0x11,
    MBTYPE_B_Bi_L1_16x8     = 0x12,
    MBTYPE_B_Bi_L1_8x16     = 0x13,
    MBTYPE_B_Bi_Bi_16x8     = 0x14,
    MBTYPE_B_Bi_Bi_8x16     = 0x15,
    MBTYPE_BP_8x8           = 0x16,
};
////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define MB Type for MPEG2
///////////////////////////////////////////////////////////////////////////////
enum
{
    MBTYPE_I                    = 0x1A,
    MBTYPE_P_FWD_SKIP           = 0x01,
    MBTYPE_B_BWD_SKIP           = 0x02,
    MBTYPE_B_Bi_SKIP            = 0x03,
    MBTYPE_P_0MV                = 0x01,
    MBTYPE_P_FRM_FRM            = 0x01,
    MBTYPE_P_FRM_FLD            = 0x04,
    MBTYPE_P_FRM_DUALPRIME      = 0x19,
    MBTYPE_P_FLD_1_16x16        = 0x01,
    MBTYPE_P_FLD_2_16x8         = 0x04,
    MBTYPE_P_FLD_DUALPRIME      = 0x19,
    MBTYPE_B_FRM_FRM_FWD        = 0x01,
    MBTYPE_B_FRM_FRM_BWD        = 0x02,
    MBTYPE_B_FRM_FRM_Bi         = 0x03,
    MBTYPE_B_FRM_FLD_FWD        = 0x04,
    MBTYPE_B_FRM_FLD_BWD        = 0x03,
    MBTYPE_B_FRM_FLD_Bi         = 0x14,
    MBTYPE_B_FLD_1_16x16_FWD    = 0x01,
    MBTYPE_B_FLD_1_16x16_BWD    = 0x02,
    MBTYPE_B_FLD_1_16x16_Bi     = 0x03,
    MBTYPE_B_FLD_2_16x8_FWD     = 0x04,
    MBTYPE_B_FLD_2_16x8_BWD     = 0x06,
    MBTYPE_B_FLD_2_16x8_Bi      = 0x14,
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define Inter MB block size
///////////////////////////////////////////////////////////////////////////////
enum
{
    INTER_MB_MODE_16x16         = 0,
    INTER_MB_MODE_16x8          = 1,
    INTER_MB_MODE_8x16          = 2,
    INTER_MB_MODE_8x8           = 3,
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define inter MB sub partition shape in a 8x8 block
///////////////////////////////////////////////////////////////////////////////
enum
{
    SUB_MB_SHAPE_8x8    = 0,
    SUB_MB_SHAPE_8x4    = 1,
    SUB_MB_SHAPE_4x8    = 2,
    SUB_MB_SHAPE_4x4    = 3,
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the Inter MB prediction type
///////////////////////////////////////////////////////////////////////////////
enum
{
    SUB_MB_PRED_MODE_L0     = 0,
    SUB_MB_PRED_MODE_L1     = 1,
    SUB_MB_PRED_MODE_Bi     = 2,
};

////////////////////////////////////////////////////////////////////////////////
// this structure is used to define the control structure for VME.
// in Linux this structure is a proxy between MediaSDK and libVA
///////////////////////////////////////////////////////////////////////////////

typedef struct tagENCODE_CAPS
{
    union
    {
        struct
        {
            UINT    CodingLimitSet              : 1;
            UINT    NoFieldFrame                : 1;
            UINT    NoCabacSupport              : 1;
            UINT    NoGapInFrameCount           : 1;
            UINT    NoGapInPicOrder             : 1;
            UINT    NoUnpairedField             : 1;
            UINT    BitDepth8Only               : 1;
            UINT    ConsecutiveMBs              : 1;
            UINT    SliceStructure              : 3;
            UINT    SliceIPOnly                 : 1;
            UINT    SliceIPBOnly                : 1;
            UINT    NoWeightedPred              : 1;
            UINT    NoMinorMVs                  : 1;
            UINT    ClosedGopOnly               : 1;
            UINT    NoFrameCropping             : 1;
            UINT    FrameLevelRateCtrl          : 1;
            UINT    HeaderInsertion             : 1;
            UINT    RawReconRefToggle           : 1; // doesn't shpport change reference frame (original vs. recon) at a per frame basis.
            UINT    NoPAFF                      : 1; // doesn't support change frame/field coding at per frame basis.
            UINT    NoInterlacedField           : 1;
            UINT    BRCReset                    : 1;
            UINT    EnhancedEncInput            : 1;
            UINT    RollingIntraRefresh         : 1;
            UINT    UserMaxFrameSizeSupport     : 1;
            UINT    LayerLevelRateCtrl          : 1;
            UINT    SliceLevelRateCtrl          : 1;
            UINT    VCMBitrateControl           : 1;
            UINT    NoESS                       : 1;
            UINT    Color420Only                : 1;
            UINT    ICQBRCSupport               : 1;
        };
        UINT CodingLimits;
    };
    union
    {
        struct
        {
            USHORT EncFunc              : 1;
            USHORT PakFunc              : 1;
            USHORT EncodeFunc           : 1;  // Enc+Pak
            USHORT InputAnalysisFunc    : 1;  // Reserved for now
            USHORT reserved             :12;
        };
        USHORT CodingFunction;
    };
    UINT    vaTrellisQuantization;
    UINT    vaRollingIntraRefresh;
    UINT    vaBufferMaxFrameSize;
    UINT    MaxPicWidth;
    UINT    MaxPicHeight;
    UCHAR   MaxNum_Reference;
    UCHAR   MaxNum_SPS_id;
    UCHAR   MaxNum_PPS_id;
    UCHAR   MaxNum_Reference1;
    UCHAR   MaxNum_QualityLayer;
    UCHAR   MaxNum_DependencyLayer;
    UCHAR   MaxNum_DQLayer;
    UCHAR   MaxNum_TemporalLayer;
    UCHAR   MBBRCSupport;

    UCHAR   TrelisQuantization;
    union {
        struct {
            UCHAR MaxNumOfROI                : 5; // [0..16]
            UCHAR                            : 1;
            UCHAR ROIBRCPriorityLevelSupport : 1;
            UCHAR ROIBRCDeltaQPLevelSupport  : 1;
        };
        UCHAR ROICaps;
    };

    union {
        struct {
            UINT    RoundingOffset          : 1;
            UINT    SkipFrame               : 1;
            UINT    MbQpDataSupport         : 1;
            UINT    SliceLevelWeightedPred  : 1;
            UINT    LumaWeightedPred        : 1;
            UINT    ChromaWeightedPred      : 1;
            UINT    QVBRBRCSupport          : 1;
            UINT    SliceLevelReportSupport : 1;
            UINT    HMEOffsetSupport        : 1;
            UINT    DirtyRectSupport        : 1;
            UINT    MoveRectSupport         : 1;
            UINT    FrameSizeToleranceSupport     : 1; // eFrameSizeTolerance_Low (Sliding window) supported
            UINT    HWCounterAutoIncrementSupport : 2;
            UINT    MBControlSupport              : 1;
            UINT    ForceReparationCheckSupport   : 1;
            UINT    CustomRoundingControl         : 1;
            UINT    LLCStreamingBufferSupport     : 1;
            UINT    DDRStreamingBufferSupport     : 1;
            UINT    LowDelayBRCSupport            : 1; // eFrameSizeTolerance_ExtremelyLow (Low delay) supported
            UINT    MaxNumDeltaQPMinus1           : 4;
            UINT    TCBRCSupport                  : 1;
            UINT    HRDConformanceSupport         : 1;
            UINT    PollingModeSupport            : 1;
            UINT                                  : 5;
        };
        UINT      CodingLimits2;
    };
    UCHAR    MaxNum_WeightedPredL0;
    UCHAR    MaxNum_WeightedPredL1;
} ENCODE_CAPS;

// We cannot modify ENCODE_CAPS structure, but need to add new features
struct MFX_ENCODE_CAPS
{
    ENCODE_CAPS ddi_caps;
    //bitrate control capabilities provided by MSDK
    bool CQPSupport;
    bool CBRSupport;
    bool VBRSupport;
    bool AVBRSupport;

    bool AdaptiveMaxFrameSizeSupport;
};

// this enumeration is used to define the block size for intra prediction. they
// are used as bit flags to control what block size will be checked for intra
// prediction.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_INTRA_BLOCK_NONE    = 0,
    ENC_INTRA_BLOCK_4x4     = (1 << 0),
    ENC_INTRA_BLOCK_8x8     = (1 << 1),
    ENC_INTRA_BLOCK_16x16   = (1 << 2),
    ENC_INTRA_BLOCK_PCM     = (1 << 3),
    ENC_INTRA_BLOCK_NOPCM   = (ENC_INTRA_BLOCK_4x4  |
                               ENC_INTRA_BLOCK_8x8  |
                               ENC_INTRA_BLOCK_16x16),
    ENC_INTRA_BLOCK_ALL     = (ENC_INTRA_BLOCK_4x4  |
                               ENC_INTRA_BLOCK_8x8  |
                               ENC_INTRA_BLOCK_16x16|
                               ENC_INTRA_BLOCK_PCM),
};


////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the cost type used in intra/inter prediction.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_COST_TYPE_NONE          = 0,
    ENC_COST_TYPE_SAD           = (1 << 0), //sum of absolute difference
    ENC_COST_TYPE_SSD           = (1 << 1), //sum of squared difference
    ENC_COST_TYPE_SATD_HADAMARD = (1 << 2), //sum of absolute hadamard transformed difference
    ENC_COST_TYPE_SATD_HARR     = (1 << 3), //sum of absolute harr transformed difference
    ENC_COST_TYPE_PROPRIETARY   = (1 << 4), //propriteary cost type
};


////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the entropy coding type used for RDO
// intra/inter prediction.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_ENTROPY_CODING_VLC      = 0,    //vlc
    ENC_ENTROPY_CODING_CABAC    = 1,    //cabac
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the integer pel search algorithm.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_INTER_SEARCH_TYPE_NONE              = 0,
    ENC_INTER_SEARCH_TYPE_FULL              = (1 << 0),
    ENC_INTER_SEARCH_TYPE_UMH               = (1 << 1),
    ENC_INTER_SEARCH_TYPE_LOG               = (1 << 2),
    ENC_INTER_SEARCH_TYPE_TTS               = (1 << 3),
    ENC_INTER_SEARCH_TYPE_SQUARE            = (1 << 4),
    ENC_INTER_SEARCH_TYPE_DIAMOND           = (1 << 5),
    ENC_INTER_SEARCH_TYPE_PROPRIETARY       = (1 << 6),
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the inter prediction block size. they are
// used as bit flags to control what block size should be checked in inter
// prediction.
// this also controls how many MV can exist in one MB.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_INTER_BLOCK_SIZE_NONE       = 0,
    ENC_INTER_BLOCK_SIZE_16x16      = (1 << 0),
    ENC_INTER_BLOCK_SIZE_16x8       = (1 << 1),
    ENC_INTER_BLOCK_SIZE_8x16       = (1 << 2),
    ENC_INTER_BLOCK_SIZE_8x8        = (1 << 3),
    ENC_INTER_BLOCK_SIZE_8x4        = (1 << 4),
    ENC_INTER_BLOCK_SIZE_4x8        = (1 << 5),
    ENC_INTER_BLOCK_SIZE_4x4        = (1 << 6),

    //all possible size
    ENC_INTER_BLOCK_SIZE_ALL        = (ENC_INTER_BLOCK_SIZE_16x16 |
    ENC_INTER_BLOCK_SIZE_16x8  |
    ENC_INTER_BLOCK_SIZE_8x16  |
    ENC_INTER_BLOCK_SIZE_8x8   |
    ENC_INTER_BLOCK_SIZE_8x4   |
    ENC_INTER_BLOCK_SIZE_4x8   |
    ENC_INTER_BLOCK_SIZE_4x4),

    //blocks are bigger than or equal 8x8
    ENC_INTER_BLOCK_SIZE_NO_MINOR   = (ENC_INTER_BLOCK_SIZE_16x16 |
    ENC_INTER_BLOCK_SIZE_16x8  |
    ENC_INTER_BLOCK_SIZE_8x16  |
    ENC_INTER_BLOCK_SIZE_8x8),

    //VC-1 4-MV mode, for P frame only
    ENC_INTER_BLOCK_SIZE_VC1_4MV    = (ENC_INTER_BLOCK_SIZE_16x16 |
    ENC_INTER_BLOCK_SIZE_8x8),

    //VC-1 1-MV mode, for P and B frame
    ENC_INTER_BLOCK_SIZE_VC1_1MV    = ENC_INTER_BLOCK_SIZE_16x16,

    //MPEG2 1-MV mode, for P and B frame
    ENC_INTER_BLOCK_SIZE_MPEG_1MV   = ENC_INTER_BLOCK_SIZE_16x16,

};


////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the interpolation methods used to get the
// sub pixels.
//
// WMV4TAP: for 1/2 pel [-1, 9, 9, -1]/16
//          for 1/4 pel [-4, 53, 18, -3]/64
//          for 3/4 pel [-3, 18, 53, -4]/64
//
// VME4TAP: for 1/2 pel [-1, 5, 5, -1]/8
//          for 1/4 pel [-1, 13, 5, -1]/16
//          for 3/4 pel [-1, 5, 13, -1]/16
//
// BILINEAR: for 1/2 pel [0, 1, 1, 0]/2
//           for 1/4 pel [0, 3, 1, 0]/4
//           for 3/4 pel [0, 1, 3, 0]/4
//
// AVC6TAP:  for 1/2 pel [1, -5, 20, 20, -5, 1]/32
//           for 1/4 pel [1, -5, 52, 20, -5, 1]/64
//           for 3/4 pel [1, -5, 20, 52, -5, 1]/64
//
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_INTERPOLATION_TYPE_NONE          = 0,
    ENC_INTERPOLATION_TYPE_VME4TAP       = (1 << 0),
    ENC_INTERPOLATION_TYPE_BILINEAR      = (1 << 1),
    ENC_INTERPOLATION_TYPE_WMV4TAP       = (1 << 2),
    ENC_INTERPOLATION_TYPE_AVC6TAP       = (1 << 3),
};

////////////////////////////////////////////////////////////////////////////////
// this enumeration is used to define the MV precision.
///////////////////////////////////////////////////////////////////////////////
enum
{
    ENC_MV_PRECISION_NONE           = 0,
    ENC_MV_PRECISION_INTEGER        = (1 << 0),
    ENC_MV_PRECISION_HALFPEL        = (1 << 1),
    ENC_MV_PRECISION_QUARTERPEL     = (1 << 2),
};

typedef struct tagENCODE_ENC_CTRL_CAPS
{
    //DW 0
    UINT    IntraPredBlockSize          : 4;
    UINT    IntraPredCostType           : 5;
    UINT    InterPredBlockSize          : 8;
    UINT    MVPrecision                 : 4;
    UINT    MBZ0                        : 11;

    //DW 1
    UINT    MECostType                  : 5;
    UINT    MESearchType                : 8;
    UINT    MBZ1                        : 19;

    //DW 2
    UINT    MVSearchWindowX             : 16;
    UINT    MVSearchWindowY             : 16;

    //DW 3
    UINT    MEInterpolationMethod       : 4;
    UINT    MaxMVs                      : 6;
    UINT    SkipCheck                   : 1;
    UINT    DirectCheck                 : 1;
    UINT    BiDirSearch                 : 1;
    UINT    MBAFF                       : 1;
    UINT    FieldPrediction             : 1;
    UINT    RefOppositeField            : 1;
    UINT    ChromaInME                  : 1;
    UINT    WeightedPrediction          : 1;
    UINT    RateDistortionOpt           : 1;
    UINT    MVPrediction                : 1;
    UINT    DirectVME                   : 1;
    UINT    MBZ2                        : 5;
    UINT    MAD                         : 1;
    UINT    MBZ3                        : 5;

} ENCODE_ENC_CTRL_CAPS;


////////////////////////////////////////////////////////////////////////////////
// this structure is used to define the sequence level parameters
///////////////////////////////////////////////////////////////////////////////
typedef enum tagENCODE_ARGB_COLOR
{
    eColorSpace_P709 = 0,
    eColorSpace_P601 = 1,
    eColorSpace_P2020 = 2
}ENCODE_ARGB_COLOR;

typedef enum tagENCODE_FRAME_SIZE_TOLERANCE
{
    eFrameSizeTolerance_Normal = 0,//default
    eFrameSizeTolerance_Low = 1,//Sliding Window
    eFrameSizeTolerance_ExtremelyLow = 2//low delay
}ENCODE_FRAME_SIZE_TOLERANCE;

typedef enum tagENCODE_SCENARIO
{
    eScenario_Unknown = 0,
    eScenario_DisplayRemoting = 1,
    eScenario_VideoConference = 2,
    eScenario_Archive = 3,
    eScenario_LiveStreaming = 4,
    eScenario_VideoCapture = 5,
    eScenario_VideoSurveillance = 6
} ENCODE_SCENARIO;

typedef enum tagENCODE_CONTENT
{
    eContent_Unknown = 0,
    eContent_FullScreenVideo = 1,
    eContent_NonVideoScreen = 2
} ENCODE_CONTENT;

typedef struct tagENCODE_SET_SEQUENCE_PARAMETERS_H264
{
    USHORT  FrameWidth;
    USHORT  FrameHeight;
    UCHAR   Profile;
    UCHAR   Level;

    USHORT  GopPicSize;
    UCHAR   GopRefDist;
    UCHAR   GopOptFlag : 2;
    UCHAR   Reserved2  : 6;

    UCHAR   TargetUsage;
    UCHAR   RateControlMethod;
    UINT    TargetBitRate;
    UINT    MaxBitRate;
    UINT    MinBitRate;
    USHORT  FramesPer100Sec;
    ULONG   InitVBVBufferFullnessInBit;
    ULONG   VBVBufferSizeInBit;
    UCHAR   NumRefFrames;

    UCHAR   seq_parameter_set_id;
    UCHAR   chroma_format_idc;
    UCHAR   bit_depth_luma_minus8;
    UCHAR   bit_depth_chroma_minus8;
    UCHAR   log2_max_frame_num_minus4;
    UCHAR   pic_order_cnt_type;
    UCHAR   log2_max_pic_order_cnt_lsb_minus4;
    UCHAR   num_ref_frames_in_pic_order_cnt_cycle;
    INT     offset_for_non_ref_pic;
    INT     offset_for_top_to_bottom_field;
    INT     offset_for_ref_frame[256];
    USHORT  frame_crop_left_offset;
    USHORT  frame_crop_right_offset;
    USHORT  frame_crop_top_offset;
    USHORT  frame_crop_bottom_offset;
    USHORT  seq_scaling_matrix_present_flag         : 1;
    USHORT  seq_scaling_list_present_flag           : 1;
    USHORT  delta_pic_order_always_zero_flag        : 1;
    USHORT  frame_mbs_only_flag                     : 1;
    USHORT  direct_8x8_inference_flag               : 1;
    USHORT  vui_parameters_present_flag             : 1;
    USHORT  frame_cropping_flag                     : 1;
    USHORT  EnableSliceLevelRateCtrl                : 1;
    USHORT  MBZ1                                    : 8;

    union
    {
        struct
        {
            UINT    bResetBRC                       : 1;
            UINT    bNoAccelerationSPSInsertion     : 1;
            UINT    GlobalSearch                    : 2;
            UINT    LocalSearch                     : 4;
            UINT    EarlySkip                       : 2;
            UINT    Trellis                         : 2;
            UINT    MBBRC                           : 4;
            UINT    bReserved                       :16;
        };

        UINT sFlags;
    };

    UINT    UserMaxFrameSize;
    USHORT  ICQQualityFactor;

    ENCODE_ARGB_COLOR ARGB_input_color;
    ENCODE_SCENARIO scenario_info;
    ENCODE_CONTENT  content_info;
    ENCODE_FRAME_SIZE_TOLERANCE frame_size_tolerance;

} ENCODE_SET_SEQUENCE_PARAMETERS_H264;

typedef struct tagENCODE_SET_SEQUENCE_PARAMETERS_MPEG2
{
    USHORT   FrameWidth;
    USHORT   FrameHeight;
    UCHAR    Profile;
    UCHAR    Level;
    UCHAR    ChromaFormat;
    UCHAR    TargetUsage;


    // ENC + PAK related parameters
    union
    {
        USHORT  AratioFrate;
        struct
        {
            USHORT  AspectRatio             : 4;
            USHORT  FrameRateCode           : 4;
            USHORT  FrameRateExtN           : 3;
            USHORT  FrameRateExtD           : 5;
        };
    };

    UINT    bit_rate;           // bits/sec
    UINT    vbv_buffer_size;    // 16 Kbit

    UCHAR   progressive_sequence            : 1;
    UCHAR   low_delay                       : 1;
    UCHAR   bResetBRC                       : 1;
    UCHAR   bNoSPSInsertion                 : 1;
    UCHAR   Reserved                        : 4;
    UCHAR   RateControlMethod;
    USHORT  Reserved16bits;
    UINT    MaxBitRate;
    UINT    MinBitRate;
    UINT    UserMaxFrameSize;
    UINT    InitVBVBufferFullnessInBit;
    USHORT  AVBRAccuracy;
    USHORT  AVBRConvergence;
} ENCODE_SET_SEQUENCE_PARAMETERS_MPEG2;


/* H.264/AVC picture entry data structure */
typedef struct _ENCODE_PICENTRY {
    union {
        struct {
            UCHAR  Index7Bits      : 7;
            UCHAR  AssociatedFlag  : 1;
        };
        UCHAR  bPicEntry;
    };
} ENCODE_PICENTRY;  /* 1 byte */

typedef struct tagENCODE_RECT
{
    USHORT Top;                // [0..(FrameHeight+ M-1)/M -1]
    USHORT Bottom;             // [0..(FrameHeight+ M-1)/M -1]
    USHORT Left;               // [0..(FrameWidth+15)/16-1]
    USHORT Right;              // [0..(FrameWidth+15)/16-1]
} ENCODE_RECT;

typedef struct tagENCODE_ROI
{
    ENCODE_RECT Roi;
    CHAR   PriorityLevelOrDQp; // [-3..3] or [-51..51]
} ENCODE_ROI;

typedef struct tagMOVE_RECT
{
    USHORT SourcePointX;
    USHORT SourcePointY;
    ENCODE_RECT DestRect;
} MOVE_RECT;


typedef struct tagENCODE_SET_PICTURE_PARAMETERS_H264
{
    ENCODE_PICENTRY     CurrOriginalPic;
    ENCODE_PICENTRY     CurrReconstructedPic;
    UCHAR   CodingType;

    UCHAR   FieldCodingFlag         : 1;
    UCHAR   FieldFrameCodingFlag    : 1;
    UCHAR   MBZ0                    : 2;
    UCHAR   InterleavedFieldBFF     : 1;
    UCHAR   ProgressiveField        : 1;
    UCHAR   MBZ1                    : 2;

    UCHAR           NumSlice;
    CHAR            QpY;
    ENCODE_PICENTRY RefFrameList[16];
    UINT            UsedForReferenceFlags;
    INT             CurrFieldOrderCnt[2];
    INT             FieldOrderCntList[16][2];
    USHORT          frame_num;
    BOOL            bLastPicInSeq;
    BOOL            bLastPicInStream;
    union
    {
        struct
        {
            UINT        bUseRawPicForRef                         : 1;
            UINT        bDisableHeaderPacking                    : 1;
            UINT        bEnableDVMEChromaIntraPrediction         : 1;
            UINT        bEnableDVMEReferenceLocationDerivation   : 1;
            UINT        bEnableDVMWSkipCentersDerivation         : 1;
            UINT        bEnableDVMEStartCentersDerivation        : 1;
            UINT        bEnableDVMECostCentersDerivation         : 1;
            UINT        bDisableSubMBPartition                   : 1;
            UINT        bEmulationByteInsertion                  : 1;
            UINT        bEnableRollingIntraRefresh               : 2;
            UINT        bReserved                                : 21;

        };
        BOOL    UserFlags;
    };
    UINT            StatusReportFeedbackNumber;
    UCHAR           bIdrPic;
    UCHAR           pic_parameter_set_id;
    UCHAR           seq_parameter_set_id;
    UCHAR           num_ref_idx_l0_active_minus1;
    UCHAR           num_ref_idx_l1_active_minus1;
    CHAR            chroma_qp_index_offset;
    CHAR            second_chroma_qp_index_offset;
    USHORT          entropy_coding_mode_flag            : 1;
    USHORT          pic_order_present_flag              : 1;
    USHORT          weighted_pred_flag                  : 1;
    USHORT          weighted_bipred_idc                 : 2;
    USHORT          constrained_intra_pred_flag         : 1;
    USHORT          transform_8x8_mode_flag             : 1;
    USHORT          pic_scaling_matrix_present_flag     : 1;
    USHORT          pic_scaling_list_present_flag       : 1;
    USHORT          RefPicFlag                          : 1;
    USHORT          BRCPrecision                        : 2;
    USHORT          MBZ2                                : 4;
    USHORT          IntraInsertionLocation;
    USHORT          IntraInsertionSize;
    CHAR            QpDeltaForInsertedIntra;
    UINT            SliceSizeInBytes;

} ENCODE_SET_PICTURE_PARAMETERS_H264;

typedef struct tagENCODE_SET_PICTURE_PARAMETERS_MPEG2
{
    ENCODE_PICENTRY CurrOriginalPic;
    ENCODE_PICENTRY CurrReconstructedPic;
    UCHAR           picture_coding_type;
    UCHAR           FieldCodingFlag         : 1;
    UCHAR           FieldFrameCodingFlag    : 1;
    UCHAR           MBZ0                    : 2;
    UCHAR           InterleavedFieldBFF     : 1;
    UCHAR           ProgressiveField        : 1;
    UCHAR           MBZ1                    : 2;
    UCHAR           NumSlice;
    UCHAR           bPicBackwardPrediction;
    UCHAR           bBidirectionalAveragingMode;
    UCHAR           bPic4MVallowed;
    ENCODE_PICENTRY RefFrameList[2];
    union
    {
        struct
        {
            UINT        bUseRawPicForRef                         : 1;
            UINT        bDisableHeaderPacking                    : 1;
            UINT        bEnableDVMEChromaIntraPrediction         : 1;
            UINT        bEnableDVMEReferenceLocationDerivation   : 1;
            UINT        bEnableDVMESkipCentersDerivation         : 1;
            UINT        bEnableDVMEStartCentersDerivation        : 1;
            UINT        bEnableDVMECostCentersDerivation         : 1;
            UINT        bReserved                                : 25;
        };
        BOOL    UserFlags;
    };
    UINT            StatusReportFeedbackNumber;
    UINT            alternate_scan              : 1;
    UINT            intra_vlc_format            : 1;
    UINT            q_scale_type                : 1;
    UINT            concealment_motion_vectors  : 1;
    UINT            frame_pred_frame_dct        : 1;



    UINT            DisableMismatchControl      : 1;
    UINT            intra_dc_precision          : 2;
    UINT            f_code00                    : 4;
    UINT            f_code01                    : 4;
    UINT            f_code10                    : 4;
    UINT            f_code11                    : 4;


    UINT            Reserved1                   : 8;

    // ENC + PAK related parameters
    BOOL            bLastPicInStream;
    BOOL            bNewGop;

    USHORT          GopPicSize;
    UCHAR           GopRefDist;
    UCHAR           GopOptFlag                  : 2;
    UCHAR                                       : 6;

    UINT            time_code                   : 25;
    UINT                                        : 7;

    USHORT          temporal_reference          : 10;
    USHORT                                      : 6;

    USHORT          vbv_delay;

    UINT            repeat_first_field          : 1;
    UINT            composite_display_flag      : 1;
    UINT            v_axis                      : 1;
    UINT            field_sequence              : 3;
    UINT            sub_carrier                 : 1;
    UINT            burst_amplitude             : 7;
    UINT            sub_carrier_phase           : 8;
    UINT                                        : 10;


} ENCODE_SET_PICTURE_PARAMETERS_MPEG2;

typedef struct tagENCODE_SET_SLICE_HEADER_H264
{
    UINT                NumMbsForSlice;
    ENCODE_PICENTRY     RefPicList[2][32];
    SHORT               Weights[2][32][3][2];
    UINT                first_mb_in_slice;

    UCHAR               slice_type;
    UCHAR               pic_parameter_set_id;
    USHORT              direct_spatial_mv_pred_flag         : 1;
    USHORT              num_ref_idx_active_override_flag    : 1;
    USHORT              long_term_reference_flag            : 1;
    USHORT              MBZ0                                : 13;
    USHORT              idr_pic_id;
    USHORT              pic_order_cnt_lsb;
    INT                 delta_pic_order_cnt_bottom;
    INT                 delta_pic_order_cnt[2];
    UCHAR               num_ref_idx_l0_active_minus1;
    UCHAR               num_ref_idx_l1_active_minus1;
    UCHAR               luma_log2_weight_denom;
    UCHAR               chroma_log2_weight_denom;
    UCHAR               cabac_init_idc;
    CHAR                slice_qp_delta;
    UCHAR               disable_deblocking_filter_idc;
    CHAR                slice_alpha_c0_offset_div2;
    CHAR                slice_beta_offset_div2;
    UINT                slice_id;

} ENCODE_SET_SLICE_HEADER_H264;

typedef struct tagENCODE_PACKEDHEADER_DATA
{
    UCHAR * pData;
    UINT    BufferSize;
    UINT    DataLength;
    UINT    DataOffset;
    UINT    SkipEmulationByteCount;
    UINT    Reserved;
} ENCODE_PACKEDHEADER_DATA;

typedef struct tagENCODE_SET_SLICE_HEADER_MPEG2
{
    USHORT  NumMbsForSlice;
    USHORT  FirstMbX;
    USHORT  FirstMbY;
    USHORT  IntraSlice;
    UCHAR   quantiser_scale_code;
} ENCODE_SET_SLICE_HEADER_MPEG2;

typedef struct
{
    SHORT  x;
    SHORT  y;
} ENC_MV, ENCODE_MV_DATA;

typedef struct tagENCODE_ENC_MB_DATA_MPEG2
{
    struct
    {
        UINT InterMBMode            : 2;
        UINT SkipMbFlag                : 1;
        UINT MBZ1                    : 5;
        UINT macroblock_type        : 5;
        UINT macroblock_intra        : 1;
        UINT FieldMBFlag            : 1;
        UINT dct_type               : 1;
        UINT MBZ2                    : 1;
        UINT CbpDcV                    : 1;
        UINT CbpDcU                    : 1;
        UINT CbpDcY                    : 1;
        UINT MvFormat                : 3;
        UINT MBZ3                    : 1;
        UINT PackedMvNum            : 3;
        UINT MBZ4                    : 5;
    };

    struct
    {
        UINT MbX  : 16;
        UINT MbY  : 16;
    };

    struct
    {
        UINT coded_block_pattern    : 12;
        UINT MBZ5                     : 4;
        UINT TargetSizeInWord       : 8;
        UINT MaxSizeInWord          : 8;
    };

    struct
    {
        UINT QpScaleCode            : 5;
        UINT MBZ6                    : 11;
        UINT MvFieldSelect          : 4;
        UINT MBZ7                   : 4;
        UINT FirstMbInSG            : 1;
        UINT MbSkipConvDisable      : 1;
        UINT LastMbInSG             : 1;
        UINT EnableCoeffClamp       : 1;
        UINT MBZ8                   : 2;
        UINT FirstMbInSlice         : 1;
        UINT LastMbInSlice          : 1;
    };

    ENC_MV  MVL0[2];
    ENC_MV  MVL1[2];
    UINT reserved[8];
} ENCODE_ENC_MB_DATA_MPEG2;

typedef struct tagENCODE_ENC_MV_H264
{
    ENC_MV  MVL0[16];
    ENC_MV  MVL1[16];
} ENCODE_ENC_MV_H264;

typedef struct tagENCODE_MBDATA_LAYOUT
{
    UCHAR         MB_CODE_size;
    UINT          MB_CODE_offset;
    UINT          MB_CODE_BottomField_offset;
    UINT          MB_CODE_stride;
    UCHAR         MV_number;
    UINT          MV_offset;
    UINT          MV_BottomField_offset;
    UINT          MV_stride;

} ENCODE_MBDATA_LAYOUT;

enum
{
    ENCODE_ENC_ID                           = 0x100,
    ENCODE_PAK_ID                           = 0x101,
    ENCODE_ENC_PAK_ID                       = 0x102,
    ENCODE_VPP_ID                           = 0x103, // reserved for now

    ENCODE_FORMAT_COUNT_ID                  = 0x104,
    ENCODE_FORMATS_ID                       = 0x105,
    ENCODE_ENC_CTRL_CAPS_ID                 = 0x106,
    ENCODE_ENC_CTRL_GET_ID                  = 0x107,
    ENCODE_ENC_CTRL_SET_ID                  = 0x108,
    MBDATA_LAYOUT_ID                        = 0x109,
    ENCODE_INSERT_DATA_ID                   = 0x120,
    ENCODE_QUERY_STATUS_ID                  = 0x121
};

#endif /* __MFX_H264_ENCODE_STRUCT_VAAPI__H */
#endif /* MFX_VA_LINUX */
/* EOF */
