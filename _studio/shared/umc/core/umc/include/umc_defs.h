// Copyright (c) 2018-2019 Intel Corporation
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

/*
// Android
*/

#if defined(ANDROID)

    // video decoders

    // video encoders

#endif // Android

#ifdef __cplusplus

namespace UMC
{

};

#endif //__cplusplus

#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
#include <cstring>
#include <algorithm>

#endif //__cplusplus

#define MFX_INTERNAL_CPY(dst, src, size) std::copy((const uint8_t *)(src), (const uint8_t *)(src) + (int)(size), (uint8_t *)(dst))

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

/******************************************************************************/

#endif // __UMC_DEFS_H__
