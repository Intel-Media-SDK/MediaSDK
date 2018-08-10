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

//  MPEG-2 is a international standard promoted by ISO/IEC and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.

#pragma once

#include "umc_defs.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_video_decoder.h"

//#define OTLAD

namespace UMC
{

// profile and level definitions for mpeg2 video
// values for profile and level fields in BaseCodecParams
#define MPEG2_PROFILE_SIMPLE  5
#define MPEG2_PROFILE_MAIN    4
#define MPEG2_PROFILE_SNR     3
#define MPEG2_PROFILE_SPATIAL 2
#define MPEG2_PROFILE_HIGH    1

#define MPEG2_LEVEL_LOW   10
#define MPEG2_LEVEL_MAIN   8
#define MPEG2_LEVEL_H14    6
#define MPEG2_LEVEL_HIGH   4

    class MPEG2VideoDecoderBase;
    
}

#endif // ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE
