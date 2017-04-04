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
#if defined MFX_ENABLE_MPEG2_VIDEO_DECODE

#include "mfx_mpeg2_dec_common.h"
#include "mfx_enc_common.h"

void GetMfxFrameRate(mfxU8 umcFrameRateCode, mfxU32 *frameRateNom, mfxU32 *frameRateDenom)
{
    switch (umcFrameRateCode)
    {
        case 0: *frameRateNom = 30; *frameRateDenom = 1; break;
        case 1: *frameRateNom = 24000; *frameRateDenom = 1001; break;
        case 2: *frameRateNom = 24; *frameRateDenom = 1; break;
        case 3: *frameRateNom = 25; *frameRateDenom = 1; break;
        case 4: *frameRateNom = 30000; *frameRateDenom = 1001; break;
        case 5: *frameRateNom = 30; *frameRateDenom = 1; break;
        case 6: *frameRateNom = 50; *frameRateDenom = 1; break;
        case 7: *frameRateNom = 60000; *frameRateDenom = 1001; break;
        case 8: *frameRateNom = 60; *frameRateDenom = 1; break;
        default: *frameRateNom = 30; *frameRateDenom = 1;
    }
    return;
}

#endif
