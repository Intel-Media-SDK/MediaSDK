// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef mfx_tracer_TEST_MAIN_H
#define mfx_tracer_TEST_MAIN_H


#include "mfx_tracer_test_mocks.h"

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


extern std::unique_ptr<MockCallObj> g_call_obj_ptr;

extern "C"
{
    mfxStatus MFXInitHookWrap(mfxIMPL implParam, mfxVersion *ver, mfxSession *session);
    mfxStatus MFXInitExHookWrap(mfxInitParam par, mfxSession* session);
    mfxStatus MFXCloseHookWrap(mfxSession session);
}

inline void ResetMockCallObj()
{
    g_call_obj_ptr.reset(new MockCallObj);
}


#endif /* mfx_tracer_TEST_MAIN_H */
