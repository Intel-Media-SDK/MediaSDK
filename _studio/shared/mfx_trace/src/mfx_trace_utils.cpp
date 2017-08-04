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

#include "mfx_trace_utils.h"

#ifdef MFX_TRACE_ENABLE
extern "C"
{

/*------------------------------------------------------------------------------*/

char* mfx_trace_vsprintf(char* str, size_t& str_size, const char* format, va_list args)
{
    char* p_str = str;

    if (str_size > 0)
    {
        p_str += vsnprintf(str, str_size, format, args);
        str_size -= p_str-str;
    }
    return p_str;
}

/*------------------------------------------------------------------------------*/

char* mfx_trace_sprintf(char* str, size_t& str_size, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    str = mfx_trace_vsprintf(str, str_size, format, args);
    va_end(args);
    return str;
}

/*------------------------------------------------------------------------------*/

int mfx_trace_tcmp(const mfxTraceChar* str1, const mfxTraceChar* str2)
{
    return strcmp(str1, str2);
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE
