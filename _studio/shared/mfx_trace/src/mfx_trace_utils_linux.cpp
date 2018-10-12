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
#include <cstring>

#ifdef MFX_TRACE_ENABLE
#include "vm_sys_info.h"

inline std::string& trim_string(std::string&& s)
{
    // Remove leading spaces, tabulation, = character
    size_t pos = s.find_first_not_of(" =\t");
    if (pos != std::string::npos)
        s.erase(0, pos);

    // Remove trailing spaces, tabulation, newline symbol, spaces
    pos = s.find_last_not_of(" \t\n\r\f\v");
    if (pos != std::string::npos)
        s.erase(pos + 1);

    return s;
}

std::string mfx_trace_get_value(FILE* file, const char* pName)
{
    if (!file || !pName)
        return std::string("");

    char line[MAX_PATH] = {0}, *s = nullptr;
    std::string str, str_name(pName);

    while ((s = fgets(line, MAX_PATH,  file)))
    {
        str = s;

        size_t pos = str.find(str_name);
        if (pos != std::string::npos)
        {
            pos += str_name.size();
            return trim_string(str.substr(pos));
        }
    }

    return std::string("");
}

extern "C"
{

#include <stdlib.h>

FILE* mfx_trace_open_conf_file(const char* name)
{
    FILE* file = nullptr;
    std::stringstream ss;

#if defined(ANDROID)
    const char* home = "/data/data/com.intel.vtune/mediasdk";
#else
    const char* home = getenv("HOME");
#endif

    if (home)
    {
        ss << home << "/." << name;
        file = fopen(ss.str().c_str(), "r");
    }
    if (!file)
    {
        ss << MFX_TRACE_CONFIG_PATH << "/" << name;
        file = fopen(ss.str().c_str(), "r");
    }
    return file;
}

mfxTraceU32 mfx_trace_get_conf_dword(FILE* file,
                                     const char* pName,
                                     mfxTraceU32* pValue)
{
    std::string value_from_file = mfx_trace_get_value(file, pName);

    if (value_from_file.empty())
        return 1;

    if (!value_from_file.compare(0, 2, "0x"))
    {
        std::stringstream(value_from_file.substr(2)) >> std::hex >> *pValue;
    }
    else
    {
        *pValue = std::stoi(value_from_file);
    }

    return 0;
}


mfxTraceU32 mfx_trace_get_conf_string(FILE* file,
                                      const char* pName,
                                      mfxTraceChar* pValue, mfxTraceU32 cValueMaxSize)
{
    std::string value_from_file = mfx_trace_get_value(file, pName);

    if (value_from_file.empty())
        return 1;

    if (cValueMaxSize > 0)
        std::strncpy(pValue, value_from_file.c_str(), cValueMaxSize-1);

    return 0;
}

} // extern "C"
#endif // #ifdef MFX_TRACE_ENABLE

