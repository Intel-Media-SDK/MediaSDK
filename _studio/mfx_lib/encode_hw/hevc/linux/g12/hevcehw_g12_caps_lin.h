// Copyright (c) 2019-2020 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)

#include "hevcehw_base.h"
#include "hevcehw_g12_caps.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Gen12
{
class Caps
    : public HEVCEHW::Gen12::Caps
{
public:
    Caps(mfxU32 FeatureId)
        : HEVCEHW::Gen12::Caps(FeatureId)
    {}

protected:
    virtual void SetSpecificCaps(HEVCEHW::Base::EncodeCapsHevc& caps) override
    {
        caps.CodingLimitSet             = 1;
        caps.Color420Only               = 0;
        caps.SliceIPBOnly               = 1;
        caps.NoMinorMVs                 = 1;
        caps.RawReconRefToggle          = 1;
        caps.NoInterlacedField          = 1;
        caps.MaxNumOfROI                = 16;
        caps.ROIDeltaQPSupport          = 1;
        caps.BlockSize                  = 1;
        caps.SliceLevelReportSupport    = 1;
        caps.FrameSizeToleranceSupport  = 1;
        caps.NumScalablePipesMinus1     = 1;
        caps.NoWeightedPred             = 0;
        caps.LumaWeightedPred           = 1;
        caps.ChromaWeightedPred         = 0;
        caps.MaxNum_WeightedPredL0      = 4;
        caps.MaxNum_WeightedPredL1      = 2;
        caps.HRDConformanceSupport      = 1;
        caps.TileSupport                = 1;
        caps.YUV444ReconSupport         = 1;
        caps.IntraRefreshBlockUnitSize  = 2;
    }
};

} //Gen12
} //Linux
} //namespace HEVCEHW

#endif
