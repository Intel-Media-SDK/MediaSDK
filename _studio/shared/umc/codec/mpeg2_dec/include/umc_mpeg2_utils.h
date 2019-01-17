// Copyright (c) 2018 Intel Corporation
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

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"

namespace UMC_MPEG2_DECODER
{
    eMFXPlatform GetPlatform_MPEG2(VideoCORE * core, mfxVideoParam * par);
    mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
    bool CheckVideoParam(mfxVideoParam *in);

    void GetMfxFrameRate(uint8_t frame_rate_value, mfxU32 & frameRateN, mfxU32 & frameRateD);
    mfxU8 GetMfxCodecProfile(uint8_t profile);
    mfxU8 GetMfxCodecLevel(uint8_t level);
    void CalcAspectRatio(uint32_t dar, uint32_t width, uint32_t height, uint16_t & aspectRatioW, uint16_t & aspectRatioH);

    namespace MFX_Utility
    {
        UMC::Status FillVideoParam(const MPEG2SequenceHeader & seq,
                                   const MPEG2SequenceExtension * seqExt,
                                   const MPEG2SequenceDisplayExtension * dispExt,
                                   mfxVideoParam & par);
    }
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
