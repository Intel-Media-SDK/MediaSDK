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


#ifndef __UMC_VP9_UTILS_H_
#define __UMC_VP9_UTILS_H_

#include "umc_defs.h"
#include "mfxstructures.h"

#ifdef MFX_ENABLE_VP9_VIDEO_DECODE
#include "umc_vp9_dec_defs.h"
#define VP9_CHECK_AND_THROW(exp, err) if(!exp) throw UMC_VP9_DECODER::vp9_exception(MFX_ERR_UNKNOWN)

namespace UMC_VP9_DECODER
{
    constexpr auto VP9_INVALID_REF_FRAME = -1;
    class VP9DecoderFrame;

    inline mfxU32 AlignPowerOfTwo(mfxU32 value, mfxU32 n)
    {
        return (((value) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1));
    }

    inline
    int32_t clamp(int32_t value, int32_t low, int32_t high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    void SetSegData(VP9Segmentation & seg, uint8_t segmentId, SEG_LVL_FEATURES featureId, int32_t seg_data);

    inline
    int32_t GetSegData(VP9Segmentation const& seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        return
            seg.featureData[segmentId][featureId];
    }

    inline
    bool IsSegFeatureActive(VP9Segmentation const& seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        return
            seg.enabled && (seg.featureMask[segmentId] & (1 << featureId));
    }

    inline
    void EnableSegFeature(VP9Segmentation & seg, uint8_t segmentId, SEG_LVL_FEATURES featureId)
    {
        seg.featureMask[segmentId] |= 1 << featureId;
    }

    inline
    uint8_t IsSegFeatureSigned(SEG_LVL_FEATURES featureId)
    {
        return
            SEG_FEATURE_DATA_SIGNED[featureId];
    }

    inline
    void ClearAllSegFeatures(VP9Segmentation & seg)
    {
        memset(&seg.featureData, 0, sizeof(seg.featureData));
        memset(&seg.featureMask, 0, sizeof(seg.featureMask));
    }

    inline
    uint32_t GetMostSignificantBit(uint32_t n)
    {
        uint32_t log = 0;
        uint32_t value = n;

        for (int8_t i = 4; i >= 0; --i)
        {
            const uint32_t shift = (1u << i);
            const uint32_t x = value >> shift;
            if (x != 0)
            {
                value = x;
                log += shift;
            }
        }

        return log;
    }

    inline
    uint32_t GetUnsignedBits(uint32_t numValues)
    {
        return
            numValues > 0 ? GetMostSignificantBit(numValues) + 1 : 0;
    }

    inline mfxU16 GetMinProfile(mfxU16 depth, mfxU16 format)
    {
        return MFX_PROFILE_VP9_0 +
            (depth > 8) * 2 +
            (format > MFX_CHROMAFORMAT_YUV420);
    }

    int32_t GetQIndex(VP9Segmentation const & seg, uint8_t segmentId, int32_t baseQIndex);

    void InitDequantizer(VP9DecoderFrame* info);

    UMC::ColorFormat GetUMCColorFormat_VP9(VP9DecoderFrame const*);

    void SetupPastIndependence(VP9DecoderFrame & info);

    void GetTileNBits(const int32_t miCols, int32_t & minLog2TileCols, int32_t & maxLog2TileCols);

} //UMC_VP9_DECODER

#endif //MFX_ENABLE_VP9_VIDEO_DECODE
#endif //__UMC_VP9_UTILS_H_
