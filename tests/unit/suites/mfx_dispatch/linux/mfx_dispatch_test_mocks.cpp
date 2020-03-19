// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfx_dispatch_test_mocks.h"

#include "mfx_dispatch_test_main.h"

#include <mfxcommon.h>
#include <mfxdefs.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
  e##func_name,

enum Function
{
  eMFXInit,
  eMFXInitEx,
  eMFXClose,
  eMFXJoinSession,
#include "mfxvideo_functions.h"
  eFunctionsNum,
  eNoMoreFunctions = eFunctionsNum
};

struct FunctionsTable
{
  Function id;
  const char* name;
  mfxVersion version;
};

#define VERSION(major, minor) {{minor, major}}

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    { e##func_name, #func_name, API_VERSION },


static const FunctionsTable g_mfxFuncTable[] =
{
    { eMFXInit, "MFXInit", VERSION(1, 0) },
    { eMFXInitEx, "MFXInitEx", VERSION(1, 14) },
    { eMFXClose, "MFXClose", VERSION(1, 0) },
    { eMFXJoinSession, "MFXJoinSession", VERSION(1, 1) },
#include "mfxvideo_functions.h"
    { eNoMoreFunctions }
};
#undef FUNCTION


void* MockCallObj::EmulateAPI(void *handle, const char *symbol)
{
    std::string requested_symbol(symbol);
    std::string mfxinitex_symbol("MFXInitEx");
    std::string mfxqueryimpl_symbol("MFXQueryIMPL");
    std::string mfxqueryversion_symbol("MFXQueryVersion");
    std::string mfxclose_symbol("MFXClose");
    std::string createplugin_symbol("CreatePlugin");
    std::string mfxvideoregister_symbol("MFXVideoUSER_Register");
    std::string mfxvideounregister_symbol("MFXVideoUSER_Unregister");

    for (int i = 0; i < eFunctionsNum; ++i)
    {
        std::string available_symbol(g_mfxFuncTable[i].name);
        mfxVersion symbol_api_version = g_mfxFuncTable[i].version;
        if (requested_symbol == available_symbol && emulated_api_version.Version > symbol_api_version.Version)
        {
            if (symbol == mfxinitex_symbol)
            {
                return reinterpret_cast<void*>(MFXInitExHookWrap);
            }
            else if (symbol == mfxqueryimpl_symbol)
            {
                return reinterpret_cast<void*>(MFXQueryIMPLHookWrap);
            }
            else if (symbol == mfxqueryversion_symbol)
            {
                return reinterpret_cast<void*>(MFXQueryVersionHookWrap);
            }
            else if (symbol == mfxclose_symbol)
            {
                return reinterpret_cast<void*>(MFXCloseHookWrap);
            }
            else if (symbol == createplugin_symbol)
            {
                return reinterpret_cast<void*>(CreatePluginWrap);
            }
            else if (symbol == mfxvideoregister_symbol)
            {
                return reinterpret_cast<void*>(MFXVideoUSER_RegisterWrap);
            }
            else if (symbol == mfxvideounregister_symbol)
            {
                return reinterpret_cast<void*>(MFXVideoUSER_UnregisterWrap);
            }
            else
            {
                return MOCK_FUNC_PTR;
            }
        }
    }
    return nullptr;
}



char* MockCallObj::FeedEmulatedPluginsCfgLines(char * str, int num, FILE * stream)
{
    if (m_next_plugin_cfg_line_idx < m_emulated_plugins_cfg.size())
    {
        strncpy(str, m_emulated_plugins_cfg[m_next_plugin_cfg_line_idx].c_str(), num);
        ++m_next_plugin_cfg_line_idx;
        return str;
    }
    return nullptr;
}

