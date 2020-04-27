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

#include "mfx_tracer_test_main.h"
#include "mfx_tracer_test_mocks.h"

std::unique_ptr<MockCallObj> g_call_obj_ptr;

extern "C"
{
    void *dlopen(const char *filename, int flag)
    {
        return g_call_obj_ptr->dlopen(filename, flag);
    }

    int dlclose(void* handle)
    {
        return g_call_obj_ptr->dlclose(handle);
    }

    void *dlsym(void *handle, const char *symbol)
    {
        return g_call_obj_ptr->dlsym(handle, symbol);
    }

    mfxStatus MFXInitHookWrap(mfxIMPL implParam, mfxVersion *ver, mfxSession *session)
    {
        return g_call_obj_ptr->MFXInit(implParam, ver, session);
    }

    mfxStatus MFXInitExHookWrap(mfxInitParam par, mfxSession *session)
    {
        return g_call_obj_ptr->MFXInitEx(par, session);
    }

    mfxStatus MFXCloseHookWrap(mfxSession session)
    {
        return g_call_obj_ptr->MFXClose(session);
    }
} // extern "C"


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
