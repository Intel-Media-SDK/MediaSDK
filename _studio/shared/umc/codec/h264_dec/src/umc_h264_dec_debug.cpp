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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#include "umc_h264_dec_debug.h"
//#include "umc_h264_timing.h"
#include <cstdarg>

namespace UMC
{

void Trace(vm_char const* format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    vm_char cStr[256];
    vm_string_vsnprintf(cStr, sizeof(cStr)-1, format, arglist);

    //OutputDebugString(cStr);
    vm_string_printf(VM_STRING("%s"), cStr);
    //fflush(stdout);
}

#ifdef USE_DETAILED_H264_TIMING
TimingInfo* GetGlobalTimingInfo()
{
    static TimingInfo info;
    return &info;
}
#endif

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE

