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

#include "mfx_common.h"

#include "umc_defs.h"

#ifndef _MFX_VP9_DEC_DECODE_H_
#define _MFX_VP9_DEC_DECODE_H_

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
#include "mfx_common_int.h"
#include "umc_video_decoder.h"

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_vp9_dec_decode_utils.h"


namespace MFX_VP9_Utility
{
    mfxStatus DecodeHeader(VideoCORE * /*core*/, mfxBitstream *bs, mfxVideoParam *params);
    void FillVideoParam(eMFXPlatform platform, UMC_VP9_DECODER::VP9DecoderFrame const& frame, mfxVideoParam &params);
};

#endif // #if defined(MFX_ENABLE_VP9_VIDEO_DECODE) || defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)

#endif // _MFX_VP9_DEC_DECODE_H_

