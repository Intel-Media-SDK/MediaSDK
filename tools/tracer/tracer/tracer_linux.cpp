// Copyright (c) 2018 Intel Corporation
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

#if !defined(_WIN32) && !defined(_WIN64)

#include <stdlib.h>
#include <dlfcn.h>
#include "tracer.h"

static const char* g_mfxlib = NULL;

void __attribute__ ((constructor)) dll_init(void)
{
    try {
        tracer_init();

        Log::WriteLog("mfx_tracer: dll_init() +");
#ifdef ANDROID // temporary hardcode for Android
        g_mfxlib = "/system/lib/libmfxhw32.so";
#else
        static std::string mfxlib = Config::GetParam("core", "lib");
        if (mfxlib.empty()) {
            Log::WriteLog("mfx_tracer: No path to real MediaSDK library is specified in the config file");
            return;
        }
        else {
            g_mfxlib = mfxlib.c_str();
        }
#endif
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
            Log::WriteLog(context.dump("session", *session));
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
        if (g_mfxlib) {
#ifdef ANDROID
            loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL);
#else
            loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
#endif
        }
        if (!loader->dlhandle){
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            return MFX_ERR_NOT_FOUND;
        }
        /* Loading functions table */
        int i;
        mfxFunctionPointer proc;
        for (i = 0; i < eFunctionsNum; ++i) {
            proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            /* NOTE: on Android very first call to dlsym may fail */
            if (!proc) proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            //if (!proc) break;
            loader->table[i] = proc;
        }
        if (i < eFunctionsNum) {
            dlclose(loader->dlhandle);
            free(loader);
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
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
            dlclose(loader->dlhandle);
            free(loader);
            Log::WriteLog(context.dump("ver", ver));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", mfx_res));
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
        dlclose(loader->dlhandle);
        free(loader);
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXClose(" + elapsed + ", " + context.dump_mfxStatus("status", mfx_res) + ") - \n\n");
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
            Log::WriteLog(context.dump("session", *session));
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
        if (g_mfxlib) {
#ifdef ANDROID
            loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL);
#else
            loader->dlhandle = dlopen(g_mfxlib, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
#endif
        }
        if (!loader->dlhandle){
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            return MFX_ERR_NOT_FOUND;
        }
        /* Loading functions table */
        int i;
        mfxFunctionPointer proc;
        for (i = 0; i < eFunctionsNum; ++i) {
            proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            /* NOTE: on Android very first call to dlsym may fail */
            if (!proc) proc = (mfxFunctionPointer)dlsym(loader->dlhandle, g_mfxFuncTable[i].name);
            if (!proc) break;
            loader->table[i] = proc;
        }
        if (i < eFunctionsNum) {
            dlclose(loader->dlhandle);
            free(loader);
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
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
            dlclose(loader->dlhandle);
            free(loader);
            Log::WriteLog(context.dump("par", par));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", mfx_res));
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
