// Copyright (c) 2019 Intel Corporation
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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION) && defined (MFX_VA_LINUX)
#include "hevcehw_g9_weighted_prediction.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Gen9
{
    class WeightPred
        : public HEVCEHW::Gen9::WeightPred
    {
    public:
        WeightPred(mfxU32 FeatureId)
            : HEVCEHW::Gen9::WeightPred(FeatureId)
        {}

    protected:

        void SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push) override;
    };

} //Gen11Win
} //Linux
} //namespace HEVCEHW

#endif
