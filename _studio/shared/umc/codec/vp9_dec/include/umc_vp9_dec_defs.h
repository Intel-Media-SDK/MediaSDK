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

#include "umc_defs.h"
#ifdef MFX_ENABLE_VP9_VIDEO_DECODE

#ifndef __UMC_VP9_DEC_DEFS_DEC_H__
#define __UMC_VP9_DEC_DEFS_DEC_H__

namespace UMC_VP9_DECODER
{
    const uint8_t VP9_SYNC_CODE_0 = 0x49;
    const uint8_t VP9_SYNC_CODE_1 = 0x83;
    const uint8_t VP9_SYNC_CODE_2 = 0x42;

    const uint8_t VP9_FRAME_MARKER = 0x2;

    typedef enum tagVP9_FRAME_TYPE
    {
        KEY_FRAME = 0,
        INTER_FRAME = 1
    } VP9_FRAME_TYPE;

    typedef enum tagCOLOR_SPACE
    {
        UNKNOWN    = 0,
        BT_601     = 1,  // YUV
        BT_709     = 2,  // YUV
        SMPTE_170  = 3,  // YUV
        SMPTE_240  = 4,  // YUV
        RESERVED_1 = 5,
        RESERVED_2 = 6,
        SRGB       = 7   // RGB
    } COLOR_SPACE;

    typedef enum tagMV_REFERENCE_FRAME
    {
        NONE = -1,
        INTRA_FRAME = 0,
        LAST_FRAME = 1,
        GOLDEN_FRAME = 2,
        ALTREF_FRAME = 3,
        MAX_REF_FRAMES = 4
    } MV_REFERENCE_FRAME;

    typedef enum tagINTERP_FILTER
    {
        EIGHTTAP = 0,
        EIGHTTAP_SMOOTH = 1,
        EIGHTTAP_SHARP = 2,
        BILINEAR = 3,
        SWITCHABLE = 4
    } INTERP_FILTER;

    const uint8_t VP9_MAX_NUM_OF_SEGMENTS = 8;
    const uint8_t VP9_NUM_OF_SEGMENT_TREE_PROBS = VP9_MAX_NUM_OF_SEGMENTS - 1;
    const uint8_t VP9_NUM_OF_PREDICTION_PROBS = 3;

    // Segment level features.
    typedef enum tagSEG_LVL_FEATURES
    {
        SEG_LVL_ALT_Q = 0,               // Use alternate Quantizer ....
        SEG_LVL_ALT_LF = 1,              // Use alternate loop filter value...
        SEG_LVL_REF_FRAME = 2,           // Optional Segment reference frame
        SEG_LVL_SKIP = 3,                // Optional Segment (0,0) + skip mode
        SEG_LVL_MAX = 4                  // Number of features supported
    } SEG_LVL_FEATURES;

    const uint8_t REFS_PER_FRAME = 3;
    const uint8_t REF_FRAMES_LOG2 = 3;
    const uint8_t NUM_REF_FRAMES = 1 << REF_FRAMES_LOG2;

    const uint8_t FRAME_CONTEXTS_LOG2 = 2;

    const uint8_t QINDEX_BITS = 8;

    const uint8_t VP9_MAX_PROB = 255;

    typedef struct tagSizeOfFrame{
        uint32_t width;
        uint32_t height;
    } SizeOfFrame;

    const uint8_t MINQ = 0;
    const uint8_t MAXQ = 255;
    const uint16_t QINDEX_RANGE = MAXQ - MINQ + 1;

    const uint8_t SEGMENT_DELTADATA = 0;
    const uint8_t SEGMENT_ABSDATA = 1;

    const uint8_t MAX_REF_LF_DELTAS = 4;
    const uint8_t MAX_MODE_LF_DELTAS = 2;

    const uint8_t MAX_LOOP_FILTER = 63;

    const uint8_t MIN_TILE_WIDTH_B64 = 4;
    const uint8_t MAX_TILE_WIDTH_B64 = 64;
    const uint8_t MI_SIZE_LOG2 = 3;
    const uint8_t MI_BLOCK_SIZE_LOG2 = 6 - MI_SIZE_LOG2; // 64 = 2^6

    const uint8_t SEG_FEATURE_DATA_SIGNED[SEG_LVL_MAX] = { 1, 1, 0, 0 };
    const uint8_t SEG_FEATURE_DATA_MAX[SEG_LVL_MAX] = { MAXQ, MAX_LOOP_FILTER, 3, 0 };

    struct Loopfilter
    {
        int32_t filterLevel;

        int32_t sharpnessLevel;
        int32_t lastSharpnessLevel;

        uint8_t modeRefDeltaEnabled;
        uint8_t modeRefDeltaUpdate;

        // 0 = Intra, Last, GF, ARF
        int8_t refDeltas[MAX_REF_LF_DELTAS];

        // 0 = ZERO_MV, MV
        int8_t modeDeltas[MAX_MODE_LF_DELTAS];
    };

    struct LoopFilterInfo
    {
        uint8_t level[VP9_MAX_NUM_OF_SEGMENTS][MAX_REF_FRAMES][MAX_MODE_LF_DELTAS];
    };

    struct VP9Segmentation
    {
        uint8_t enabled;
        uint8_t updateMap;
        uint8_t updateData;
        uint8_t absDelta;
        uint8_t temporalUpdate;

        uint8_t treeProbs[VP9_NUM_OF_SEGMENT_TREE_PROBS];
        uint8_t predProbs[VP9_NUM_OF_PREDICTION_PROBS];

        int32_t featureData[VP9_MAX_NUM_OF_SEGMENTS][SEG_LVL_MAX];
        uint32_t featureMask[VP9_MAX_NUM_OF_SEGMENTS];

    };

    class vp9_exception
    {
    public:

        vp9_exception(int32_t status = -1)
            : m_Status(status)
        {}

        virtual ~vp9_exception()
        {}

        int32_t GetStatus() const
        {
            return m_Status;
        }

    private:

        int32_t m_Status;
    };
} // end namespace UMC_VP9_DECODER

#endif // __UMC_VP9_DEC_DEFS_DEC_H__
#endif // MFX_ENABLE_VP9_VIDEO_DECODE
