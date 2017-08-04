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

#ifndef __MFX_TRACE_UTILS_LINUX_H__
#define __MFX_TRACE_UTILS_LINUX_H__


#ifdef MFX_TRACE_ENABLE

#include <stdio.h>

#define MFX_TRACE_CONFIG          "mfx_trace"
#define MFX_TRACE_CONFIG_PATH     "/etc"
#define MFX_TRACE_CONF_OMODE_TYPE "Output"
#define MFX_TRACE_CONF_LEVEL      "Level"

/*------------------------------------------------------------------------------*/
extern "C"
{
FILE* mfx_trace_open_conf_file(const char* name);

mfxTraceU32 mfx_trace_get_conf_dword(FILE* file,
                                     const char* pName,
                                     mfxTraceU32* pValue);
mfxTraceU32 mfx_trace_get_conf_string(FILE* file,
                                      const char* pName,
                                      char* pValue, mfxTraceU32 cValueMaxSize);
}

#endif // #ifdef MFX_TRACE_ENABLE


#endif // #ifndef __MFX_TRACE_UTILS_LINUX_H__
