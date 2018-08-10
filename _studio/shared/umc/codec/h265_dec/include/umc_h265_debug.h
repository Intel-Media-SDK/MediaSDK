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

#ifndef __UMC_H265_DEC_DEBUG_H
#define __UMC_H265_DEC_DEBUG_H

#include "vm_time.h" /* for vm_time_get_tick on Linux */
#include "umc_h265_dec_defs.h"

//#define ENABLE_TRACE

namespace UMC_HEVC_DECODER
{

#ifdef ENABLE_TRACE
    class H265DecoderFrame;
    void Trace(vm_char * format, ...);
    const vm_char* GetFrameInfoString(H265DecoderFrame * frame);

#if defined _DEBUG
#define DEBUG_PRINT(x) Trace x
static int pppp = 0;
#define DEBUG_PRINT1(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT1(x)
#endif

#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT1(x)
#endif

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_DEC_DEBUG_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
