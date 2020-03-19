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

#include "mfx_dispatch_test_main.h"
#include "mfx_dispatch_test_mocks.h"

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

    FILE * fopen(const char *filename, const char *opentype)
    {
        return g_call_obj_ptr->fopen(filename, opentype);
    }

    char * fgets(char* out, int n, FILE* fd)
    {
        return g_call_obj_ptr->fgets(out, n, fd);
    }

    int fclose(FILE * fd)
    {
        return g_call_obj_ptr->fclose(fd);
    }

    mfxStatus MFXInitExHookWrap(mfxInitParam par, mfxSession *session)
    {
        return g_call_obj_ptr->MFXInitEx(par, session);
    }

    mfxStatus MFXQueryVersionHookWrap(mfxSession session, mfxVersion *version)
    {
        return g_call_obj_ptr->MFXQueryVersion(session, version);
    }

    mfxStatus MFXQueryIMPLHookWrap(mfxSession session, mfxIMPL *impl)
    {
        return g_call_obj_ptr->MFXQueryIMPL(session, impl);
    }

    mfxStatus MFXCloseHookWrap(mfxSession session)
    {
        return g_call_obj_ptr->MFXClose(session);
    }

    mfxStatus CreatePluginWrap(mfxPluginUID uid, mfxPlugin* pPlugin)
    {
        return g_call_obj_ptr->CreatePlugin(uid, pPlugin);
    }
    mfxStatus GetPluginParamWrap(mfxHDL pthis, mfxPluginParam *par)
    {
        return g_call_obj_ptr->GetPluginParam(pthis, par);
    }
    mfxStatus MFXVideoUSER_RegisterWrap(mfxSession session, mfxU32 type, const mfxPlugin *par)
    {
        return g_call_obj_ptr->MFXVideoUSER_Register(session, type, par);
    }
    mfxStatus MFXVideoUSER_UnregisterWrap(mfxSession session, mfxU32 type)
    {
        return g_call_obj_ptr->MFXVideoUSER_Unregister(session, type);
    }

} // extern "C"



int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
