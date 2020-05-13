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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"

namespace HEVCEHW
{
    namespace Base
    {
        class EncodedFrameInfo
            : public FeatureBase
        {
        public:
#define DECL_BLOCK_LIST\
        DECL_BLOCK(CheckMAD)\
        DECL_BLOCK(QueryTask)
#define DECL_FEATURE_NAME "Base_EncodedFrameInfo"
#include "hevcehw_decl_blocks.h"

            EncodedFrameInfo(mfxU32 FeatureId)
                : FeatureBase(FeatureId)
            {}

        protected:
            virtual void SetSupported(ParamSupport& par) override;
            virtual void Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push) override;
            virtual void QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push) override;

            virtual mfxStatus GetDdiInfo(const void* pDdiFeedback, mfxExtAVCEncodedFrameInfo& info) = 0;
        };

    } //Base
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)