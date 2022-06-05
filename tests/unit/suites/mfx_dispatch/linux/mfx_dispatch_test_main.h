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

#ifndef MFX_DISPATCH_TEST_MAIN_H
#define MFX_DISPATCH_TEST_MAIN_H


#include "mfx_dispatch_test_mocks.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <list>
#include <algorithm>

#ifndef MFX_MODULES_DIR
#define MFX_MODULES_DIR "ERROR: MFX_MODULES_DIR was not defined!"
#endif

#ifndef MFX_PLUGINS_CONF_DIR
#define MFX_PLUGINS_CONF_DIR "ERROR: MFX_PLUGINS_CONF_DIR was not defined!"
#endif

#define MOCK_DLOPEN_HANDLE (void*)0x0BADCAFE
#define MOCK_FUNC_PTR (void*)0xDEADBEEF
#define MOCK_SESSION_HANDLE (mfxSession)0xFADEBABE
#define MOCK_FILE_DESCRIPTOR (FILE*) 0x31337FAD
#define MOCK_PLUGIN_UID_BYTE_BASE 0xB0


extern std::unique_ptr<MockCallObj> g_call_obj_ptr;

extern "C"
{
    mfxStatus MFXInitExHookWrap(mfxInitParam par, mfxSession* session);
    mfxStatus MFXQueryVersionHookWrap(mfxSession session, mfxVersion* version);
    mfxStatus MFXQueryIMPLHookWrap(mfxSession session, mfxIMPL* impl);
    mfxStatus MFXCloseHookWrap(mfxSession session);
    mfxStatus CreatePluginWrap(mfxPluginUID uid, mfxPlugin* pPlugin);
    mfxStatus GetPluginParamWrap(mfxHDL pthis, mfxPluginParam *par);
    mfxStatus MFXVideoUSER_RegisterWrap(mfxSession session, mfxU32 type, const mfxPlugin *par);
    mfxStatus MFXVideoUSER_UnregisterWrap(mfxSession session, mfxU32 type);
}


inline void ResetMockCallObj()
{
    g_call_obj_ptr.reset(new MockCallObj);
}

inline bool operator==(mfxPluginUID lhs, mfxPluginUID rhs)
{
    return std::equal(std::begin(lhs.Data), std::end(lhs.Data), std::begin(rhs.Data));
}

inline std::ostream& operator<<(std::ostream& stream, mfxPluginUID uid)
{
    for (size_t i = 0; i < sizeof(uid.Data); ++i)
    {
        stream << std::hex << (mfxU32) uid.Data[i];
    }
    return stream;
}


#endif /* MFX_DISPATCH_TEST_MAIN_H */
