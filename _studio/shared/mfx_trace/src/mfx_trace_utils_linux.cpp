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
#include <sstream>
#include <iomanip>

#ifdef MFX_TRACE_ENABLE
#include "vm_sys_info.h"

#if defined(ANDROID)
#include "snprintf_s.h"
#endif

extern "C"
{

#include <stdlib.h>

/*------------------------------------------------------------------------------*/

FILE* mfx_trace_open_conf_file(const char* name)
{
    FILE* file = NULL;
    char file_name[MAX_PATH] = {0};

#if defined(ANDROID)
    const char* home = "/data/data/com.intel.vtune/mediasdk";
#else
    const char* home = getenv("HOME");
#endif

    if (home)
    {
        snprintf_s_ss(file_name, MAX_PATH-1, "%s/.%s", home, name);
        file = fopen(file_name, "r");
    }
    if (!file)
    {
        snprintf_s_ss(file_name, MAX_PATH-1, "%s/%s", MFX_TRACE_CONFIG_PATH, name);
        file = fopen(file_name, "r");
    }
    return file;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 mfx_trace_get_value_pos(FILE* file,
                                    const char* pName,
                                    char* line, mfxTraceU32 line_size,
                                    char** value_pos)
{
    char *str = NULL;
    mfxTraceU32 n = 0;
    bool bFound = false;

    if (!file || ! pName || !value_pos) return 1;
    while (NULL != (str = fgets(line, line_size-1,  file)))
    {
        n = strnlen_s(str, line_size-1);
        if ((n > 0) && (str[n-1] == '\n')) str[n-1] = '\0';
        for(; strchr(" \t", *str) && (*str); str++);
        n = strnlen_s(pName, 256);
        if (!strncmp(str, pName, n))
        {
            str += n;
            if (!strchr(" =\t", *str)) continue;
            for(; strchr(" =\t", *str) && (*str); str++);
            bFound = true;
            *value_pos = str;
            break;
        }
    }
    return (bFound)? 0: 1;
}

/*------------------------------------------------------------------------------*/

mfxTraceU32 mfx_trace_get_conf_dword(FILE* file,
                                     const char* pName,
                                     mfxTraceU32* pValue)
{
    char line[MAX_PATH] = {0}, *value_pos = NULL;
    
    if (!mfx_trace_get_value_pos(file, pName, line, sizeof(line), &value_pos))
    {
        if (!strncmp(value_pos, "0x", 2))
        {
            value_pos += 2;
            std::stringstream ss(value_pos);
            std::string s;
            ss >> std::setw(8) >> s;
            std::stringstream(s) >> std::hex >> *pValue;
        }
        else
        {
            *pValue = atoi(value_pos);
        }
        return 0;
    }
    return 1;
}


mfxTraceU32 mfx_trace_get_conf_string(FILE* file,
                                      const char* pName,
                                      mfxTraceChar* pValue, mfxTraceU32 cValueMaxSize)
{
    char line[MAX_PATH] = {0}, *value_pos = NULL;
    
    if (!mfx_trace_get_value_pos(file, pName, line, sizeof(line), &value_pos))
    {
        strncpy(pValue, value_pos, cValueMaxSize-1);
        return 0;
    }
    return 1;
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE

