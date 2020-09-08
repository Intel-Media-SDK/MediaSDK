/*********************************************************************************

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*********************************************************************************/

#if !defined(_WIN32) && !defined(_WIN64)

#include <stdlib.h>
#include <dlfcn.h>
#include "tracer.h"

#if defined(__i386__)
    #define LIBMFXHW "libmfxhw32.so.1"
#elif defined(__x86_64__)
    #define LIBMFXHW "libmfxhw64.so.1"
#else
    #error Unsupported architecture
#endif

static const char* g_mfxlib;
static const char* g_mfxlib_in_dir;

void __attribute__ ((constructor)) dll_init(void)
{
    try {
        tracer_init();

        Log::WriteLog("mfx_tracer: dll_init() +");
        g_mfxlib = LIBMFXHW;
        g_mfxlib_in_dir = MFX_MODULES_DIR "/" LIBMFXHW;
        Log::WriteLog("mfx_tracer: lib=" + string(g_mfxlib));
        Log::WriteLog("mfx_tracer: dll_init() - \n\n");
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
    }
}

mfxStatus MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog(std::string("function: MFXInit(mfxIMPL impl=" + ToString(impl) + ", mfxVersion *ver=" + ToString(ver) + ", mfxSession *session=" + ToString(session) + ") +"));
        if (!session) {
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NULL_PTR));
            return MFX_ERR_NULL_PTR;
        }

        mfxLoader* loader = (mfxLoader*)calloc(1, sizeof(mfxLoader));
        if (!loader) {
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_MEMORY_ALLOC));
            return MFX_ERR_MEMORY_ALLOC;
        }
        loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
        if(!loader->dlhandle)
            loader->dlhandle = dlopen(g_mfxlib_in_dir, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
        if (!loader->dlhandle){
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            free(loader);
            return MFX_ERR_NOT_FOUND;
        }
        /* Loading functions table */
        int i;
        mfxFunctionPointer proc;
        for (i = 0; i < eFunctionsNum; ++i) {
            proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            if (!proc) proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            if (!proc) break;
            loader->table[i] = proc;
        }
        if (i < eFunctionsNum) {
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            dlclose(loader->dlhandle);
            free(loader);
            return MFX_ERR_NOT_FOUND;
        }

        Log::WriteLog(context.dump("impl", impl));
        Log::WriteLog(context.dump("ver", ver));
        Log::WriteLog(context.dump("session", loader->session));
        /* Initializing loaded library */
        Timer t;
        mfxStatus mfx_res = (*(MFXInitPointer)loader->table[eMFXInit_tracer])(impl, ver, &(loader->session));
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXInit called");
        if (MFX_ERR_NONE != mfx_res) {
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", mfx_res));
            dlclose(loader->dlhandle);
            free(loader);
            return mfx_res;
        }
        *session = (mfxSession)loader;
        Log::WriteLog(context.dump("impl", impl));
        Log::WriteLog(context.dump("ver", ver));
        Log::WriteLog(context.dump("session", loader->session));
        Log::WriteLog(std::string("function: MFXInit(" + elapsed + ", " + context.dump_mfxStatus("status", mfx_res) + ") - \n\n"));
        return MFX_ERR_NONE;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXClose(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXClose(mfxSession session=" + ToString(session) + ") +");
        mfxLoader* loader = (mfxLoader*)session;

        if (!loader){
            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_INVALID_HANDLE));
            return MFX_ERR_INVALID_HANDLE;
        }
        Log::WriteLog(context.dump("session", session));
        Timer t;
        mfxStatus mfx_res = (*(MFXClosePointer)loader->table[eMFXClose_tracer])(loader->session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXClose called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXClose(" + elapsed + ", " + context.dump_mfxStatus("status", mfx_res) + ") - \n\n");
        dlclose(loader->dlhandle);
        free(loader);
        return mfx_res;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog(std::string("function: MFXInitEx(mfxInitParam par={.Implementation=" + GetmfxIMPL(par.Implementation) + ", .Version="+ ToString(&par.Version) +"}, mfxSession *session=" + ToString(session) + ") +"));
        if (!session) {
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NULL_PTR));
            return MFX_ERR_NULL_PTR;
        }

        mfxLoader* loader = (mfxLoader*)calloc(1, sizeof(mfxLoader));
        if (!loader) {
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_MEMORY_ALLOC));
            return MFX_ERR_MEMORY_ALLOC;
        }
        loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
        if(!loader->dlhandle)
            loader->dlhandle = dlopen(g_mfxlib_in_dir, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
        if (!loader->dlhandle){
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            free(loader);
            return MFX_ERR_NOT_FOUND;
        }
        /* Loading functions table */
        int i;
        mfxFunctionPointer proc;
        for (i = 0; i < eFunctionsNum; ++i) {
            proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            if (!proc) proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            if (!proc) break;
            loader->table[i] = proc;
        }
        if (i < eFunctionsNum) {
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            dlclose(loader->dlhandle);
            free(loader);
            return MFX_ERR_NOT_FOUND;
        }

        Log::WriteLog(context.dump("par", par));
        Log::WriteLog(context.dump("session", loader->session));
        /* Initializing loaded library */
        Timer t;
        mfxStatus mfx_res = (*(MFXInitExPointer)loader->table[eMFXInitEx_tracer])(par, &(loader->session));
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXInitEx called");
        if (MFX_ERR_NONE != mfx_res) {
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", mfx_res));
            dlclose(loader->dlhandle);
            free(loader);
            return mfx_res;
        }
        *session = (mfxSession)loader;
        Log::WriteLog(context.dump("par", par));
        Log::WriteLog(context.dump("session", loader->session));
        Log::WriteLog(std::string("function: MFXInitEx(" + elapsed + ", " + context.dump_mfxStatus("status", mfx_res) + ") - \n\n"));
        return MFX_ERR_NONE;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

#endif // #if !defined(_WIN32) && !defined(_WIN64)
