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

#include "mfx_common.h"

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)

#ifndef _MFX_VP8_DEC_DECODE_VP8_DEFS_H_
#define _MFX_VP8_DEC_DECODE_VP8_DEFS_H_

#include "umc_structures.h"

namespace VP8Defs
{
#define VP8_START_CODE_FOUND(ptr) ((ptr)[0] == 0x9d && (ptr)[1] == 0x01 && (ptr)[2] == 0x2a)

#define VP8_MAX_NUM_OF_TOKEN_PARTITIONS 8
#define VP8_MIN_QP 0
#define VP8_MAX_QP 127

#define VP8_MAX_LF_LEVEL 63

enum
{
  VP8_MB_DC_PRED = 0, /* predict DC using row above and column to the left */
  VP8_MB_V_PRED,      /* predict rows using row above */
  VP8_MB_H_PRED,      /* predict columns using column to the left */
  VP8_MB_TM_PRED,     /* propagate second differences a la "true motion" */
  VP8_MB_B_PRED,      /* each Y block is independently predicted */
  VP8_NUM_MB_MODES_UV = VP8_MB_B_PRED, /* first four modes apply to chroma */
  VP8_NUM_MB_MODES_Y  /* all modes apply to luma */
};


enum
{
  VP8_MV_NEAREST   = VP8_NUM_MB_MODES_Y, /* use "nearest" motion vector for entire MB */
  VP8_MV_NEAR,                           /* use "next nearest" */
  VP8_MV_ZERO,                           /* use zero */
  VP8_MV_NEW,                            /* use explicit offset from implicit */
  VP8_MV_SPLIT,                          /* use multiple motion vectors */
  VP8_NUM_MV_MODES = VP8_MV_SPLIT + 1 - VP8_MV_NEAREST
};

enum
{
  VP8_B_DC_PRED = 0, /* predict DC using row above and column to the left */
  VP8_B_TM_PRED,     /* propagate second differences a la "true motion" */
  VP8_B_VE_PRED,     /* predict rows using row above */
  VP8_B_HE_PRED,     /* predict columns using column to the left */
  VP8_B_LD_PRED,     /* southwest (left and down) 45 degree diagonal prediction */
  VP8_B_RD_PRED,     /* southeast (right and down) "" */
  VP8_B_VR_PRED,     /* SSE (vertical right) diagonal prediction */
  VP8_B_VL_PRED,     /* SSW (vertical left) "" */
  VP8_B_HD_PRED,     /* ESE (horizontal down) "" */
  VP8_B_HU_PRED,     /* ENE (horizontal up) "" */
  VP8_NUM_INTRA_BLOCK_MODES,

  VP8_B_MV_LEFT = VP8_NUM_INTRA_BLOCK_MODES,
  VP8_B_MV_ABOVE,
  VP8_B_MV_ZERO,
  VP8_B_MV_NEW,
  VP8_NUM_B_MV_MODES = VP8_B_MV_NEW + 1 - VP8_NUM_INTRA_BLOCK_MODES
};


enum
{
  VP8_COEFF_PLANE_Y_AFTER_Y2 = 0,
  VP8_COEFF_PLANE_Y2,
  VP8_COEFF_PLANE_UV,
  VP8_COEFF_PLANE_Y,
  VP8_NUM_COEFF_PLANES
};


#define VP8_NUM_COEFF_BANDS 8
#define VP8_NUM_LOCAL_COMPLEXITIES 3


enum
{
  VP8_COEFF_NODE_EOB = 0,
  VP8_COEFF_NODE_0,
  VP8_COEFF_NODE_1,
};


#define VP8_NUM_COEFF_NODES 11


enum
{
  VP8_MV_IS_SHORT  = 0,
  VP8_MV_SIGN,
  VP8_MV_SHORT,
  VP8_MV_LONG      = 9,
  VP8_MV_LONG_BITS = 10,
  VP8_NUM_MV_PROBS = VP8_MV_LONG + VP8_MV_LONG_BITS
};


extern const uint8_t  vp8_range_normalization_shift[64];

extern const uint8_t  vp8_kf_mb_mode_y_probs[VP8_NUM_MB_MODES_Y - 1];
extern const uint8_t  vp8_kf_mb_mode_uv_probs[VP8_NUM_MB_MODES_UV - 1];
extern const uint8_t  vp8_kf_block_mode_probs[VP8_NUM_INTRA_BLOCK_MODES][VP8_NUM_INTRA_BLOCK_MODES][VP8_NUM_INTRA_BLOCK_MODES-1];

extern const uint8_t  vp8_mb_mode_y_probs[VP8_NUM_MB_MODES_Y - 1];
extern const uint8_t  vp8_mb_mode_uv_probs[VP8_NUM_MB_MODES_UV - 1];
extern const uint8_t  vp8_block_mode_probs [VP8_NUM_INTRA_BLOCK_MODES - 1];

extern const uint8_t  vp8_default_coeff_probs[VP8_NUM_COEFF_PLANES][VP8_NUM_COEFF_BANDS][VP8_NUM_LOCAL_COMPLEXITIES][VP8_NUM_COEFF_NODES];
extern const uint8_t  vp8_coeff_update_probs[VP8_NUM_COEFF_PLANES][VP8_NUM_COEFF_BANDS][VP8_NUM_LOCAL_COMPLEXITIES][VP8_NUM_COEFF_NODES];

extern const int8_t  vp8_mb_mode_y_tree[2*(VP8_NUM_MB_MODES_Y - 1)];
extern const int8_t  vp8_kf_mb_mode_y_tree[2*(VP8_NUM_MB_MODES_Y - 1)];
extern const int8_t  vp8_mb_mode_uv_tree[2*(VP8_NUM_MB_MODES_UV - 1)];
extern const int8_t  vp8_block_mode_tree[2*(VP8_NUM_INTRA_BLOCK_MODES - 1)];

extern const uint32_t  vp8_mbmode_2_blockmode_u32[VP8_NUM_MB_MODES_Y];

extern const uint8_t  vp8_mv_update_probs[2][VP8_NUM_MV_PROBS];
extern const uint8_t  vp8_default_mv_contexts[2][VP8_NUM_MV_PROBS];


// get this from h264 sources - need test?????
extern const uint8_t vp8_ClampTbl[768];

#define vp8_GetClampTblValue(x) vp8_ClampTbl[256 + (x)]
#define vp8_ClampTblLookup(x, y) vp8_GetClampTblValue((x) + mfx::clamp(y, -256, 256))

#define vp8_clamp8s(x) vp8_ClampTblLookup((x), 128) - 128
#define vp8_clamp8u(x) vp8_ClampTblLookup((x),  0)


enum {
  VP8_ALT_QUANT = 0,
  VP8_ALT_LOOP_FILTER,
  VP8_NUM_OF_MB_FEATURES
};

#define VP8_MAX_NUM_OF_SEGMENTS 4

enum {
  VP8_ALT_LOOP_FILTER_BITS = 6,
  VP8_ALT_QUANT_BITS = 7
};


enum {
  VP8_LOOP_FILTER_NORMAL = 0,
  VP8_LOOP_FILTER_SIMPLE
};


#define VP8_NUM_OF_SEGMENT_TREE_PROBS 3


enum {
  VP8_INTRA_FRAME = 0,
  VP8_LAST_FRAME,
  VP8_GOLDEN_FRAME,
  VP8_ALTREF_FRAME,
  VP8_NUM_OF_REF_FRAMES
};


#define VP8_NUM_OF_MODE_LF_DELTAS 4


#define VP8_BILINEAR_INTERP 1
#define VP8_CHROMA_FULL_PEL 2

typedef struct _vp8_FrameInfo
{
  UMC::FrameType frameType;

  uint8_t showFrame;

  uint8_t interpolationFlags; // bit0: bilinear interpolation (1 - on, 0 - off);
                            // bit1: chroma full pel (1 - on, 0 - off)

  uint8_t segmentationEnabled;
  uint8_t updateSegmentMap;
  uint8_t updateSegmentData;
  uint8_t segmentAbsMode;

  int8_t segmentFeatureData[VP8_NUM_OF_MB_FEATURES][VP8_MAX_NUM_OF_SEGMENTS];
  uint8_t segmentTreeProbabilities[VP8_NUM_OF_SEGMENT_TREE_PROBS];

  uint8_t loopFilterType;
  uint8_t loopFilterLevel;
  uint8_t sharpnessLevel;

  uint8_t mbLoopFilterAdjust;
  uint8_t modeRefLoopFilterDeltaUpdate;

  int8_t refLoopFilterDeltas[VP8_NUM_OF_REF_FRAMES];
  int8_t modeLoopFilterDeltas[VP8_NUM_OF_MODE_LF_DELTAS];

  uint8_t numTokenPartitions;

  uint8_t mbSkipEnabled;
  uint8_t skipFalseProb;
  uint8_t intraProb;
  uint8_t goldProb;
  uint8_t lastProb;

  int32_t numPartitions;
  int32_t partitionSize[VP8_MAX_NUM_OF_TOKEN_PARTITIONS];
  uint8_t *partitionStart[VP8_MAX_NUM_OF_TOKEN_PARTITIONS];

  int32_t m_DPBSize;

  uint8_t color_space_type;
  uint8_t clamping_type;

  uint8_t h_scale;  // not used by the decoder
  uint8_t v_scale;  // not used by the decoder

  int16_t frameWidth;  // width  + cols_padd
  int16_t frameHeight; // height + rows_padd

  mfxSize frameSize; // actual width/height without padding cols/rows

  uint32_t firstPartitionSize; // Header info bits rounded up to byte
  uint16_t version;

  uint32_t entropyDecSize; // Header info consumed bits
  uint32_t entropyDecOffset;

} vp8_FrameInfo;

typedef struct _vp8_QuantInfo
{
  int32_t yacQP; // abs value, always specified

  int32_t y2acQ[VP8_MAX_NUM_OF_SEGMENTS];
  int32_t y2dcQ[VP8_MAX_NUM_OF_SEGMENTS];
  int32_t yacQ[VP8_MAX_NUM_OF_SEGMENTS];
  int32_t ydcQ[VP8_MAX_NUM_OF_SEGMENTS];
  int32_t uvacQ[VP8_MAX_NUM_OF_SEGMENTS];
  int32_t uvdcQ[VP8_MAX_NUM_OF_SEGMENTS];

  // q deltas
  int32_t ydcDeltaQP;
  int32_t y2acDeltaQP;
  int32_t y2dcDeltaQP;
  int32_t uvacDeltaQP;
  int32_t uvdcDeltaQP;

  int32_t lastGoldenKeyQP;

  const int32_t *pYacQ;
  const int32_t *pY2acQ;
  const int32_t *pUVacQ;
  const int32_t *pYdcQ;
  const int32_t *pY2dcQ;
  const int32_t *pUVdcQ;

} vp8_QuantInfo;


typedef struct _vp8_RefreshInfo
{
  uint8_t refreshRefFrame; // (refreshRefFrame & 2) - refresh golden frame, (refreshRefFrame & 1) - altref frame
  uint8_t copy2Golden;
  uint8_t copy2Altref;
  uint8_t refFrameBiasTable[4];

  uint8_t refreshProbabilities;
  uint8_t refreshLastFrame;

} vp8_RefreshInfo;


typedef struct _vp8_FrameProbabilities
{
  uint8_t mbModeProbY[VP8_NUM_MB_MODES_Y - 1];   // vp8_mb_mode_y_probs
  uint8_t mbModeProbUV[VP8_NUM_MB_MODES_UV - 1]; // vp8_mb_mode_uv_probs
  uint8_t mvContexts[2][VP8_NUM_MV_PROBS];       // vp8_default_mv_contexts


  uint8_t coeff_probs[VP8_NUM_COEFF_PLANES][VP8_NUM_COEFF_BANDS][VP8_NUM_LOCAL_COMPLEXITIES][VP8_NUM_COEFF_NODES];
} vp8_FrameProbabilities;


enum {
  VP8_FIRST_PARTITION = 0,
  VP8_TOKEN_PARTITION_0,
  VP8_TOKEN_PARTITION_1,
  VP8_TOKEN_PARTITION_2,
  VP8_TOKEN_PARTITION_3,
  VP8_TOKEN_PARTITION_4,
  VP8_TOKEN_PARTITION_5,
  VP8_TOKEN_PARTITION_6,
  VP8_TOKEN_PARTITION_7,
  VP8_TOKEN_PARTITION_8,
  VP8_MAX_NUMBER_OF_PARTITIONS
};


typedef struct _vp8_FrameData
{
  uint8_t* data_y;
  uint8_t* data_u;
  uint8_t* data_v;

  uint8_t* base_y;
  uint8_t* base_u;
  uint8_t* base_v;

  mfxSize size;

  int32_t step_y;
  int32_t step_uv;

  int32_t border_size;

} vp8_FrameData;

} // namespace UMC

#endif // __VP8_DEC_H__

#endif // _MFX_VP8_DEC_DECODE_VP8_DEFS_H_
