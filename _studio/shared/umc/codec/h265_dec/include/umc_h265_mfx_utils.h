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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_MFX_UTILS_H
#define __UMC_H265_MFX_UTILS_H

namespace UMC_HEVC_DECODER
{

// MFX utility API functions
namespace MFX_Utility
{
    // Initialize mfxVideoParam structure based on decoded bitstream header values
    UMC::Status FillVideoParam(const H265VideoParamSet * vps, const H265SeqParamSet * seq, mfxVideoParam *par, bool full);

    // Returns implementation platform
    eMFXPlatform GetPlatform_H265(VideoCORE * core, mfxVideoParam * par);

    // Find bitstream header NAL units, parse them and fill application parameters structure
    UMC::Status DecodeHeader(TaskSupplier_H265 * supplier, UMC::VideoDecoderParams* params, mfxBitstream *bs, mfxVideoParam *out);

    // MediaSDK DECODE_Query API function
    mfxStatus Query_H265(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type);
    // Validate input parameters
    bool CheckVideoParam_H265(mfxVideoParam *in, eMFXHWType type);

    bool IsBugSurfacePoolApplicable(eMFXHWType hwtype, mfxVideoParam * par);

    // Check HW capabilities
    bool IsNeedPartialAcceleration_H265(mfxVideoParam * par, eMFXHWType type);
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_MFX_UTILS_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
