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

#pragma once
#include "mfxdefs.h"
#include <cstring>

#if defined(MFX_DISPATCHER_LOG)
#include <string>
#include <string.h>
#endif

#define MAX_PLUGIN_PATH 4096
#define MAX_PLUGIN_NAME 4096

typedef char msdk_disp_char;
//#define msdk_disp_char_cpy_s(to, to_size, from) strcpy(to, from)

inline void msdk_disp_char_cpy_s(char * to, size_t to_size, const char * from)
{
    size_t source_len = strlen(from);
    size_t num_chars = (to_size - 1) < source_len ? (to_size - 1) : source_len;
    strncpy(to, from, num_chars);
    to[num_chars] = 0;
}

#if defined(MFX_DISPATCHER_LOG)
#define MSDK2WIDE(x) getWideString(x).c_str()

inline std::wstring getWideString(const char * string)
{
    size_t len = strlen(string);
    return std::wstring(string, string + len);
}
#else
    #define MSDK2WIDE(x) x  
#endif


#if defined(__GNUC__)
#define  sscanf_s  sscanf
#define  swscanf_s swscanf
#endif


// declare library module's handle
typedef void * mfxModuleHandle;

typedef void (MFX_CDECL * mfxFunctionPointer)(void);
