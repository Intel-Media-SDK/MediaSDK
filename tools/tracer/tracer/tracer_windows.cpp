/* ****************************************************************************** *\

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

File Name: tracer_windows.cpp

\* ****************************************************************************** */

#if defined(_WIN32) || defined(_WIN64)

#define MFX_DISPATCHER_LOG 1

#include <windows.h>
#include <string.h>
#include <iostream>
#include "tracer.h"
#include "mfx_dispatcher.h"
#include "mfx_dispatcher_log.h"
#include "../tools/shared_mem_server.h"


//dump messages from dispatcher
class DispatcherLogRecorder : public IMsgHandler
{
    static DispatcherLogRecorder m_Instance;
public:
    static DispatcherLogRecorder& get(){return m_Instance;}
    virtual void Write(int level, int opcode, const char * msg, va_list argptr);
};


class msdk_analyzer_sink : public IMsgHandler
{

public:
    msdk_analyzer_sink()
    {
        DispatchLog::get().AttachSink(DL_SINK_IMsgHandler, this);
        DispatchLog::get().DetachSink(DL_SINK_PRINTF, NULL);
    }
    ~msdk_analyzer_sink()
    {
        DispatchLog::get().DetachSink(DL_SINK_IMsgHandler, this);

    }

    virtual void Write(int level, int /*opcode*/, const char * msg, va_list argptr)
    {
        char message_formated[1024];
        if ((msg) && (argptr) && (level != DL_LOADED_LIBRARY))
        {
            vsprintf_s(message_formated, sizeof(message_formated)/sizeof(message_formated[0]), msg, argptr);

            //todo: eliminate this
            if (message_formated[strnlen_s(message_formated, 1024)-1] == '\n')
            {
                message_formated[strnlen_s(message_formated, 1024)-1] = 0;
            }
            Log::WriteLog(message_formated);
        }
    }

};

static char* g_mfxlib = NULL;
bool is_loaded;



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    lpReserved;
    hinstDLL;

    try{

        std::string slib = "";
        switch( fdwReason ){
            case DLL_PROCESS_ATTACH:
                slib = Config::GetParam("core", "lib");
                g_mfxlib = new char[MAX_PATH];
                strcpy_s(g_mfxlib, MAX_PATH, slib.c_str());
                if (!strcmp(g_mfxlib, "none"))
                {
                    Log::useGUI = true;
                    if (!is_loaded)
                    {
                        run_shared_memory_server();
                    }

                }
                tracer_init();
                Log::WriteLog(std::string("function: DLLMain() DLL_PROCESS_ATTACH +"));
                Log::WriteLog("mfx_tracer: lib=" + std::string(g_mfxlib));
                Log::WriteLog(std::string("function: DLLMain() DLL_PROCESS_ATTACH - \n\n"));
                break;
            case DLL_THREAD_ATTACH:

                break;
            case DLL_THREAD_DETACH:

                break;
            case DLL_PROCESS_DETACH:
                Log::WriteLog(std::string("function: DLLMain() DLL_PROCESS_DETACH +"));
                //delete [] g_mfxlib;
                Log::WriteLog(std::string("function: DLLMain() DLL_PROCESS_DETACH - \n\n"));
                stop_shared_memory_server();
                break;
        }
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
    }
    return TRUE;
}

// external define from Dispatcher compiled with/MFX_DISPATCHER_EXPOSED_PREFIX
extern "C"
{
    mfxStatus MFX_CDECL disp_MFXInitEx(mfxInitParam par, mfxSession *session);
}

mfxStatus TracerInit (mfxInitParam par, mfxSession *session)
{
    DumpContext context;
    context.context = DUMPCONTEXT_MFX;
    if (!session) {
        Log::WriteLog(context.dump("ver", par.Version));
        Log::WriteLog(context.dump("session", NULL));
        Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NULL_PTR));
        return MFX_ERR_NULL_PTR;
    }

    mfxLoader* loader = (mfxLoader*)calloc(1, sizeof(mfxLoader));
    if (!loader) {
        Log::WriteLog(context.dump("ver", par.Version));
        Log::WriteLog(context.dump("session", *session));
        Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_MEMORY_ALLOC));
        return MFX_ERR_MEMORY_ALLOC;
     }

     Log::WriteLog("LoadLibrary: " + std::string(g_mfxlib));
     HINSTANCE h_mfxdll = NULL;
     h_mfxdll = LoadLibrary(g_mfxlib);
     if (h_mfxdll == NULL){
        //dispatcher
        mfxStatus sts;
        msdk_analyzer_sink sink;
        is_loaded = true;
        sts = disp_MFXInitEx(par, session);
        is_loaded = false;
        if (sts != MFX_ERR_NONE)
        {
            Log::WriteLog(context.dump("ver", par.Version));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_NOT_FOUND));
            free(loader);
            return MFX_ERR_NOT_FOUND;
        }
        char libModuleName[MAX_PATH];

        GetModuleFileName((HMODULE)(*(MFX_DISP_HANDLE**)(session))->hModule, libModuleName, MAX_PATH);
        if (GetLastError() != 0)
        {
           Log::WriteLog("GetModuleFileName() reported  error! \n");
           free(loader);
           return MFX_ERR_NOT_FOUND;
        }
        h_mfxdll = LoadLibrary(libModuleName);
    }
    loader->dlhandle = h_mfxdll;

    //Loading functions table
    int i;
    mfxFunctionPointer proc;
    for (i = 0; i < eFunctionsNum; ++i) {
        proc = (mfxFunctionPointer)GetProcAddress((HINSTANCE)loader->dlhandle, g_mfxFuncTable[i].name);
        if (!proc){
            Sleep(100);
            proc = (mfxFunctionPointer)GetProcAddress((HINSTANCE)loader->dlhandle, g_mfxFuncTable[i].name);
        }
        //if (!proc) break;
        if(proc) loader->table[i] = proc;
    }
    // Initializing loaded library
    mfxStatus mfx_res = (*(MFXInitPointer)loader->table[eMFXInit])(par.Implementation, &par.Version, &(loader->session));
    if (MFX_ERR_NONE != mfx_res) {
            FreeLibrary(h_mfxdll);
            Log::WriteLog(context.dump("ver", par.Version));
            Log::WriteLog(context.dump("session", *session));
            Log::WriteLog(context.dump_mfxStatus("status", mfx_res));
            free(loader);
            return mfx_res;
    }
    *session = (mfxSession)loader;
    return MFX_ERR_NONE;
}


mfxStatus MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
{
    if (is_loaded)
    {
        return MFX_ERR_ABORTED;
    }
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog(std::string("function: MFXInit(mfxIMPL impl=" + GetmfxIMPL(impl) + ", mfxVersion *ver=" + ToString(ver) + ", mfxSession *session=" + ToString(session) + ") +"));
        mfxInitParam par = {};

        par.Implementation = impl;
        if (ver)
        {
            par.Version = *ver;
        }
        else
        {
            par.Version.Major = DEFAULT_API_VERSION_MAJOR;
            par.Version.Minor = DEFAULT_API_VERSION_MINOR;
        }
        par.ExternalThreads = 0;
        Timer t;
        mfxStatus sts = TracerInit(par, session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXInit called");

        Log::WriteLog(context.dump("impl", impl));
        if (ver) Log::WriteLog(context.dump("ver", *ver));
        if (session) Log::WriteLog(context.dump("session", *session));
        Log::WriteLog(std::string("function: MFXInit(" + elapsed + ", " + context.dump_mfxStatus("status", sts) + ") - \n\n"));
        return sts;
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
        Log::WriteLog("function: MFXClose(mfxSession session=" + context.toString<mfxSession>(session) + ") +");
        mfxLoader* loader = (mfxLoader*)session;

        if (!loader){
            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump_mfxStatus("status", MFX_ERR_INVALID_HANDLE));
            return MFX_ERR_INVALID_HANDLE;
        }
        Log::WriteLog(context.dump("session", session));
        Timer t;
        mfxStatus mfx_res = (*(MFXClosePointer)loader->table[eMFXClose])(loader->session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXClose called");

        FreeLibrary((HINSTANCE)loader->dlhandle);

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
    if (is_loaded)
    {
        return MFX_ERR_ABORTED;
    }
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog(std::string("function: MFXInitEx(par.Implementation=" + GetmfxIMPL(par.Implementation) + " par.Versin="+ ToString(&par.Version) +", mfxSession *session=" + ToString(session) + ") +"));
        Timer t;
        mfxStatus sts = TracerInit(par, session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXInitEx called");
        Log::WriteLog(context.dump("par", par));
        Log::WriteLog(context.dump("session", *session));
        Log::WriteLog(std::string("function: MFXInitEx(" + elapsed + ", " + context.dump_mfxStatus("status", sts) + ") - \n\n"));
        return sts;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}



#endif // #if defined(_WIN32) || defined(_WIN64)


