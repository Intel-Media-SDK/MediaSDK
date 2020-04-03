// Copyright (c) 2020 Intel Corporation
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
#include "fast_copy.h"

#define FAFT_COPY_CPU_DISP_INIT_C(func)           (func ## _C)
#define FAFT_COPY_CPU_DISP_INIT_SSE4(func)        (func ## _SSE4)
#define FAFT_COPY_CPU_DISP_INIT_SSE4_C(func)      (m_SSE4_available ? FAFT_COPY_CPU_DISP_INIT_SSE4(func) : FAFT_COPY_CPU_DISP_INIT_C(func))

mfxI32 CpuFeature_SSE41() {
    return((__builtin_cpu_supports("sse4.1")));
}

void copyVideoToSys(const mfxU8* src, mfxU8* dst, int width)
{
    static const int m_SSE4_available = CpuFeature_SSE41();

    static const t_copyVideoToSys copyVideoToSys_impl = FAFT_COPY_CPU_DISP_INIT_SSE4_C(copyVideoToSys);

    copyVideoToSys_impl(src, dst, width);
}

void copyVideoToSysShift(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    static const int m_SSE4_available = CpuFeature_SSE41();

    static const t_copyVideoToSysShift copyVideoToSysShift_impl = FAFT_COPY_CPU_DISP_INIT_SSE4_C(copyVideoToSysShift);

    copyVideoToSysShift_impl(src, dst, width, shift);
}

void copySysToVideoShift(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    static const int m_SSE4_available = CpuFeature_SSE41();

    static const t_copySysToVideoShift copySysToVideoShift_impl = FAFT_COPY_CPU_DISP_INIT_SSE4_C(copySysToVideoShift);

    copySysToVideoShift_impl(src, dst, width, shift);
}
