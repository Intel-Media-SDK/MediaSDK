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

#ifndef __UMC_DEFS_H__
#define __UMC_DEFS_H__

#include "mfx_config.h"
// This file contains defines which switch on/off support of
// codecs and renderers on application level
/*
// Windows
*/

/*
// Linux on IA32
*/

#if defined(LINUX32) && !defined(ANDROID)

    // video decoders
    #define UMC_ENABLE_VP8_VIDEO_DECODER
    #define UMC_ENABLE_VC1_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_H265_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_VP9_VIDEO_DECODER

    // video encoders
    #define UMC_ENABLE_MPEG2_VIDEO_ENCODER

    // audio decoders
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER

    // audio encoders
    #define UMC_ENABLE_AAC_AUDIO_ENCODER
    //#define UMC_ENABLE_AC3_AUDIO_ENCODER
    //#define UMC_ENABLE_MP3_AUDIO_ENCODER

#endif // Linux on IA32

/*
// Android
*/

#if defined(ANDROID)

    // video decoders
    #define UMC_ENABLE_VC1_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_H265_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_VP9_VIDEO_DECODER

    // video encoders

#endif // Android

#ifdef __cplusplus

namespace UMC
{

};

#endif //__cplusplus

#include <stdint.h>

#ifdef __cplusplus
#include <algorithm>
#endif //__cplusplus

#define MFX_INTERNAL_CPY_S(dst, dstsize, src, src_size) memcpy_s((uint8_t *)(dst), (uint8_t)(dstsize), (const uint8_t *)(src), (int)src_size)
#define MFX_INTERNAL_CPY(dst, src, size) std::copy((const uint8_t *)(src), (const uint8_t *)(src) + (int)(size), (uint8_t *)(dst))

#define MFX_MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#define MFX_MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )

#define MFX_MAX_32S    ( 2147483647 )
#define MFX_MAXABS_64F ( 1.7976931348623158e+308 )
#define MFX_MAX_32U    ( 0xFFFFFFFF )

  #define MFX_MAX_64S  ( 9223372036854775807LL )

typedef struct {
    int width;
    int height;
} mfxSize;

  #define __STDCALL
  #define __CDECL


  #define ALIGN_DECL(X) __attribute__ ((aligned(X)))

#define THROWSEXCEPTION
/******************************************************************************/

#endif // __UMC_DEFS_H__
