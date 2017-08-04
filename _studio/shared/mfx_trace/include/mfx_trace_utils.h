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

#ifndef __MFX_TRACE_UTILS_H__
#define __MFX_TRACE_UTILS_H__

#include "mfx_trace.h"

#ifdef MFX_TRACE_ENABLE
#include "sys/mfx_trace_utils_linux.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// defines maximum length of output line
#define MFX_TRACE_MAX_LINE_LENGTH 1024

/*------------------------------------------------------------------------------*/

struct mfxTraceCategoryItem
{
    mfxTraceChar m_name[MAX_PATH];
    mfxTraceU32  m_level;
};

/*------------------------------------------------------------------------------*/
// help functions
extern "C"
{
char* mfx_trace_vsprintf(char* str, size_t& str_size, const char* format, va_list args);
char* mfx_trace_sprintf(char* str, size_t& str_size, const char* format, ...);
int mfx_trace_tcmp(const mfxTraceChar* str1, const mfxTraceChar* str2);
}
/*------------------------------------------------------------------------------*/
// help macroses


#define mfx_trace_fopen(FNAME, FMODE) fopen(FNAME, FMODE)
#define mfx_trace_tfopen(FNAME, FMODE) mfx_trace_fopen(FNAME, FMODE)


#endif // #ifdef MFX_TRACE_ENABLE

#endif // #ifndef __MFX_TRACE_UTILS_H__
