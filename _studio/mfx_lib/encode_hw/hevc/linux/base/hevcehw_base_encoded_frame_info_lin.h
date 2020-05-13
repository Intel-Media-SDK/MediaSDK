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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_VA_LINUX)

#include "hevcehw_base_encoded_frame_info.h"
#include "hevcehw_base_va_packer_lin.h"
#include "va/va.h"

namespace HEVCEHW
{
namespace Linux
{
namespace Base
{
    class EncodedFrameInfo
        : public HEVCEHW::Base::EncodedFrameInfo
    {
    public:

        EncodedFrameInfo(mfxU32 FeatureId)
            : HEVCEHW::Base::EncodedFrameInfo(FeatureId)
        {}

    protected:
        virtual mfxStatus GetDdiInfo(
            const void* pDdiFeedback
            , mfxExtAVCEncodedFrameInfo& info) override
        {
            MFX_CHECK(pDdiFeedback, MFX_ERR_UNDEFINED_BEHAVIOR);
            auto& fb = *(const VACodedBufferSegment*)pDdiFeedback;

            info.QP = mfxU16(fb.status & VA_CODED_BUF_STATUS_PICTURE_AVE_QP_MASK);

            return MFX_ERR_NONE;
        }
    };

} //Base
} //Linux
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
