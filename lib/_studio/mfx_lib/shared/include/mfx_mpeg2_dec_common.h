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

#ifndef __MFX_MPEG2_DEC_COMMON_H__
#define __MFX_MPEG2_DEC_COMMON_H__

#include "mfx_common.h"
#if defined MFX_ENABLE_MPEG2_VIDEO_DECODE

#include "mfx_common_int.h"
#include "mfxvideo.h"
#include "umc_mpeg2_dec_defs.h"

#define MFX_PROFILE_MPEG1 8

void GetMfxFrameRate(mfxU8 umcFrameRateCode, mfxU32 *frameRateNom, mfxU32 *frameRateDenom);
inline bool IsMpeg2StartCodeEx(const mfxU8* p);

template<class T> inline
T AlignValue(T value, mfxU32 divisor)
{
    return static_cast<T>(((value + divisor - 1) / divisor) * divisor);
}

#endif

#endif //__MFX_MPEG2_DEC_COMMON_H__
