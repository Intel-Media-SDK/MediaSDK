// Copyright (c) 2012-2020 Intel Corporation
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

#include "umc_defs.h"
#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#ifndef __UMC_AV1_UTILS_H_
#define __UMC_AV1_UTILS_H_

#include "umc_av1_dec_defs.h"
#include "umc_av1_frame.h"

namespace UMC_AV1_DECODER
{
    inline uint32_t CeilLog2(uint32_t x) { uint32_t l = 0; while (x > (1U << l)) l++; return l; }

    // we stop using UMC_VP9_DECODER namespace starting from Rev 0.25.2
    // because after switch to AV1-specific segmentation stuff there are only few definitions we need to re-use from VP9
    void SetSegData(SegmentationParams & seg, uint8_t segmentId, SEG_LVL_FEATURES featureId, int32_t seg_data);

    inline int32_t GetSegData(SegmentationParams const& seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        return
            seg.FeatureData[segmentId][featureId];
    }

    inline bool IsSegFeatureActive(SegmentationParams const& seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        return
            seg.segmentation_enabled && (seg.FeatureMask[segmentId] & (1 << featureId));
    }

    inline void EnableSegFeature(SegmentationParams & seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        seg.FeatureMask[segmentId] |= 1 << featureId;
    }

    inline uint8_t IsSegFeatureSigned(SEG_LVL_FEATURES featureId)
    {
        return
            SEG_FEATURE_DATA_SIGNED[featureId];
    }

    inline void ClearAllSegFeatures(SegmentationParams & seg)
    {
        std::fill_n(reinterpret_cast<int32_t*>(seg.FeatureData),
            std::extent<decltype(seg.FeatureData), 0>::value * std::extent<decltype(seg.FeatureData), 1>::value, 0);
        std::fill_n(seg.FeatureMask, std::extent<decltype(seg.FeatureMask)>::value, 0);
    }

    void SetupPastIndependence(FrameHeader & info);

    inline bool FrameIsIntraOnly(FrameHeader const& fh)
    {
        return fh.frame_type == INTRA_ONLY_FRAME;
    }

    inline bool FrameIsIntra(FrameHeader const& fh)
    {
        return (fh.frame_type == KEY_FRAME || FrameIsIntraOnly(fh));
    }

    inline bool FrameIsResilient(FrameHeader const& fh)
    {
        return FrameIsIntra(fh) || fh.error_resilient_mode;
    }

    inline uint32_t NumTiles(TileInfo const & info)
    {
        return info.TileCols * info.TileRows;
    }

    int IsCodedLossless(FrameHeader const&);

    inline void Av1LoadPrevious(FrameHeader& fh, DPBType const& frameDpb)
    {
        const int prevFrame = fh.ref_frame_idx[fh.primary_ref_frame];
        FrameHeader const& prevFH = frameDpb[prevFrame]->GetFrameHeader();

        for (uint32_t i = 0; i < TOTAL_REFS; i++)
            fh.loop_filter_params.loop_filter_ref_deltas[i] = prevFH.loop_filter_params.loop_filter_ref_deltas[i];

        for (uint32_t i = 0; i < MAX_MODE_LF_DELTAS; i++)
            fh.loop_filter_params.loop_filter_mode_deltas[i] = prevFH.loop_filter_params.loop_filter_mode_deltas[i];

        for (uint32_t i = 0; i < VP9_MAX_NUM_OF_SEGMENTS; i++)
        {
            fh.segmentation_params.FeatureMask[i] = prevFH.segmentation_params.FeatureMask[i];
            for (uint32_t j = 0; j < SEG_LVL_MAX; j++)
                fh.segmentation_params.FeatureData[i][j] = prevFH.segmentation_params.FeatureData[i][j];
        }
    }

    inline bool FrameMightUsePrevFrameMVs(FrameHeader const& fh, SequenceHeader const& sh)
    {
        return !FrameIsResilient(fh) && sh.enable_order_hint && sh.enable_ref_frame_mvs;
    }

    inline void SetDefaultLFParams(LoopFilterParams& par)
    {
        par.loop_filter_delta_enabled = 1;
        par.loop_filter_delta_update = 1;

        par.loop_filter_ref_deltas[INTRA_FRAME] = 1;
        par.loop_filter_ref_deltas[LAST_FRAME] = 0;
        par.loop_filter_ref_deltas[LAST2_FRAME] = par.loop_filter_ref_deltas[LAST_FRAME];
        par.loop_filter_ref_deltas[LAST3_FRAME] = par.loop_filter_ref_deltas[LAST_FRAME];
        par.loop_filter_ref_deltas[BWDREF_FRAME] = par.loop_filter_ref_deltas[LAST_FRAME];
        par.loop_filter_ref_deltas[GOLDEN_FRAME] = -1;
        par.loop_filter_ref_deltas[ALTREF2_FRAME] = -1;
        par.loop_filter_ref_deltas[ALTREF_FRAME] = -1;

        par.loop_filter_mode_deltas[0] = 0;
        par.loop_filter_mode_deltas[1] = 0;
    }

    inline uint8_t GetNumPlanes(SequenceHeader const& sh)
    {
        return sh.color_config.mono_chrome ? 1 : MAX_MB_PLANE;
    }

    inline unsigned GetNumArrivedTiles(AV1DecoderFrame const& frame)
    {
        auto &tileSets = frame.GetTileSets();
        return CalcTilesInTileSets(tileSets);
    }

    inline unsigned GetNumMissingTiles(AV1DecoderFrame const& frame)
    {
        FrameHeader const& fh = frame.GetFrameHeader();
        return NumTiles(fh.tile_info) - GetNumArrivedTiles(frame);
    }
}

#endif //__UMC_AV1_UTILS_H_
#endif //MFX_ENABLE_AV1_VIDEO_DECODE
