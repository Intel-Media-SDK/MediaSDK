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

#include "umc_h265_debug.h"
#include <cstdarg>


#ifdef ENABLE_TRACE
#include "umc_h265_frame.h"
#endif

namespace UMC_HEVC_DECODER
{

#ifdef ENABLE_TRACE
// Debug output
void Trace(vm_char * format, ...)
{
    va_list(arglist);
    va_start(arglist, format);

    vm_char cStr[256];
    vm_string_vsprintf(cStr, format, arglist);

    //OutputDebugString(cStr);
    vm_string_printf(VM_STRING("%s"), cStr);
    //fflush(stdout);
}

const vm_char* GetFrameInfoString(H265DecoderFrame * frame)
{
    static vm_char str[256];
    vm_string_sprintf(str, VM_STRING("POC - %d, uid - %d"), frame->m_PicOrderCnt, frame->m_UID);
    return str;
}

#endif // ENABLE_TRACE

} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE

