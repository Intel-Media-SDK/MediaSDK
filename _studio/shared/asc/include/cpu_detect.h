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
#ifndef _CPUDETECT_H_
#define _CPUDETECT_H_

#include "asc_structures.h"
    #include <cpuid.h>
//
// CPU Dispatcher
// 1) global CPU flags are initialized at startup
// 2) each stub configures a function pointer on first call
//

#ifdef _MSVC_LANG
#pragma warning(disable:4505)
#endif

static mfxI32 CpuFeature_SSE41() {
    return((__builtin_cpu_supports("sse4.1")));
}

static mfxI32 CpuFeature_AVX2() {
#if defined(__AVX2__)
        return((__builtin_cpu_supports("avx2")));
#else
    return 0;
#endif //defined(__AVX2__)
}

#ifdef _MSVC_LANG
#pragma warning(default:4505)
#endif

//
// end Dispatcher
//


#endif
