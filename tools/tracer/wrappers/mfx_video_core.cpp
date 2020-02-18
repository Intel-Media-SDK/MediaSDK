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

File Name: mfx_video_core.cpp

\* ****************************************************************************** */

#include <exception>
#include <iostream>

#include "../loggers/timer.h"
#include "../tracer/functions_table.h"
#include "mfx_structures.h"

#if TRACE_CALLBACKS
mfxStatus mfxFrameAllocator_Alloc(mfxHDL _pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxFrameAllocator_Alloc_tracer][1];

        Log::WriteLog("callback: mfxFrameAllocator::Alloc(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameAllocRequest *request=" + ToString(request)
            + ", mfxFrameAllocResponse *response=" + ToString(response) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxFrameAllocator_Alloc proc = (fmfxFrameAllocator_Alloc)loader->callbacks[emfxFrameAllocator_Alloc_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (request) Log::WriteLog(context.dump("request", *request));
        if (response) Log::WriteLog(context.dump("response", *response));

        Timer t;
        mfxStatus status = (*proc) (pthis, request, response);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxFrameAllocator::Alloc called");
        if (response) Log::WriteLog(context.dump("response", *response));
        Log::WriteLog("callback: mfxFrameAllocator::Alloc(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus mfxFrameAllocator_Lock(mfxHDL _pthis, mfxMemId mid, mfxFrameData *ptr)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxFrameAllocator_Lock_tracer][1];

        Log::WriteLog("callback: mfxFrameAllocator::Lock(mfxHDL pthis=" + ToString(pthis)
            + ", mfxMemId mid=" + ToString(mid)
            + ", mfxFrameData *ptr=" + ToString(ptr) + ") +");

        fmfxFrameAllocator_Lock proc = (fmfxFrameAllocator_Lock)loader->callbacks[emfxFrameAllocator_Lock_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (ptr) Log::WriteLog(context.dump("ptr", *ptr));

        Timer t;
        mfxStatus status = (*proc) (pthis, mid, ptr);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxFrameAllocator::Lock called");
        if (ptr) Log::WriteLog(context.dump("ptr", *ptr));
        Log::WriteLog("callback: mfxFrameAllocator::Lock(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus mfxFrameAllocator_Unlock(mfxHDL _pthis, mfxMemId mid, mfxFrameData *ptr)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxFrameAllocator_Unlock_tracer][1];

        Log::WriteLog("callback: mfxFrameAllocator::Unlock(mfxHDL pthis=" + ToString(pthis)
            + ", mfxMemId mid=" + ToString(mid)
            + ", mfxFrameData *ptr=" + ToString(ptr) + ") +");

        fmfxFrameAllocator_Unlock proc = (fmfxFrameAllocator_Unlock)loader->callbacks[emfxFrameAllocator_Unlock_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (ptr) Log::WriteLog(context.dump("ptr", *ptr));

        Timer t;
        mfxStatus status = (*proc) (pthis, mid, ptr);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxFrameAllocator::Unlock called");
        if (ptr) Log::WriteLog(context.dump("ptr", *ptr));
        Log::WriteLog("callback: mfxFrameAllocator::Unlock(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus mfxFrameAllocator_GetHDL(mfxHDL _pthis, mfxMemId mid, mfxHDL *handle)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxFrameAllocator_GetHDL_tracer][1];

        Log::WriteLog("callback: mfxFrameAllocator::GetHDL(mfxHDL pthis=" + ToString(pthis)
            + ", mfxMemId mid=" + ToString(mid)
            + ", mfxHDL *handle=" + ToString(handle) + ") +");

        fmfxFrameAllocator_GetHDL proc = (fmfxFrameAllocator_GetHDL)loader->callbacks[emfxFrameAllocator_GetHDL_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (handle) Log::WriteLog(context.dump("handle", *handle));

        Timer t;
        mfxStatus status = (*proc) (pthis, mid, handle);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxFrameAllocator::GetHDL called");
        if (handle) Log::WriteLog(context.dump("handle", *handle));
        Log::WriteLog("callback: mfxFrameAllocator::GetHDL(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus mfxFrameAllocator_Free(mfxHDL _pthis, mfxFrameAllocResponse *response)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxFrameAllocator_Free_tracer][1];

        Log::WriteLog("callback: mfxFrameAllocator::Free(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameAllocResponse *response=" + ToString(response) + ") +");

        fmfxFrameAllocator_Free proc = (fmfxFrameAllocator_Free)loader->callbacks[emfxFrameAllocator_Free_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (response) Log::WriteLog(context.dump("response", *response));

        Timer t;
        mfxStatus status = (*proc) (pthis, response);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxFrameAllocator::Free called");
        if (response) Log::WriteLog(context.dump("response", *response));
        Log::WriteLog("callback: mfxFrameAllocator::Free(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
#endif //TRACE_CALLBACKS

// CORE interface functions
mfxStatus MFXVideoCORE_SetBufferAllocator(mfxSession session, mfxBufferAllocator *allocator)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoCORE_SetBufferAllocator(mfxSession session=" + ToString(session) + ", mfxBufferAllocator *allocator=" + ToString(allocator) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoCORE_SetBufferAllocator_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(allocator) Log::WriteLog(context.dump("allocator", *allocator));

        Timer t;
        mfxStatus status = (*(fMFXVideoCORE_SetBufferAllocator) proc) (session, allocator);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoCORE_SetBufferAllocator called");
        Log::WriteLog(context.dump("session", session));
        if(allocator) Log::WriteLog(context.dump("allocator", *allocator));
        Log::WriteLog("function: MFXVideoCORE_SetBufferAllocator(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoCORE_SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoCORE_SetFrameAllocator(mfxSession session=" + ToString(session) + ", mfxFrameAllocator *allocator=" + ToString(allocator) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoCORE_SetFrameAllocator_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(allocator) Log::WriteLog(context.dump("allocator", *allocator));

#if TRACE_CALLBACKS
        INIT_CALLBACK_BACKUP(loader->callbacks);
        mfxFrameAllocator proxyAllocator;

        if (allocator)
        {
            proxyAllocator = *allocator;
            allocator = &proxyAllocator;
            SET_CALLBACK(mfxFrameAllocator, allocator->, Alloc,  allocator->pthis);
            SET_CALLBACK(mfxFrameAllocator, allocator->, Lock,   allocator->pthis);
            SET_CALLBACK(mfxFrameAllocator, allocator->, Unlock, allocator->pthis);
            SET_CALLBACK(mfxFrameAllocator, allocator->, GetHDL, allocator->pthis);
            SET_CALLBACK(mfxFrameAllocator, allocator->, Free,   allocator->pthis);
            if (allocator->pthis)
                allocator->pthis = loader;
        }
#endif //TRACE_CALLBACKS

        Timer t;
        mfxStatus status = (*(fMFXVideoCORE_SetFrameAllocator) proc) (session, allocator);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoCORE_SetFrameAllocator called");
        //No need to dump input-only parameters twice!!!
        //Log::WriteLog(context.dump("session", session));
        //if(allocator) Log::WriteLog(context.dump("allocator", *allocator));
        Log::WriteLog("function: MFXVideoCORE_SetFrameAllocator(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

#if TRACE_CALLBACKS
        if (status != MFX_ERR_NONE)
            callbacks.RevertAll();
#endif //TRACE_CALLBACKS

        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoCORE_SetHandle(mfxSession session, mfxHandleType type, mfxHDL hdl)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoCORE_SetHandle(mfxSession session=" + ToString(session) + ", mfxHandleType type=" + ToString(type) + ", mfxHDL hdl=" + ToString(hdl) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoCORE_SetHandle_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("type", type));
        Log::WriteLog(context.dump_mfxHDL("hdl", &hdl));

        Timer t;
        mfxStatus status = (*(fMFXVideoCORE_SetHandle) proc) (session, type, hdl);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoCORE_SetHandle called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("type", type));
        Log::WriteLog(context.dump_mfxHDL("hdl", &hdl));
        Log::WriteLog("function: MFXVideoCORE_SetHandle(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoCORE_GetHandle(mfxSession session, mfxHandleType type, mfxHDL *hdl)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoCORE_GetHandle(mfxSession session=" + ToString(session) + ", mfxHandleType type=" + ToString(type) + ", mfxHDL *hdl=" + ToString(hdl) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoCORE_GetHandle_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("type", type));
        Log::WriteLog(context.dump_mfxHDL("hdl", hdl));

        Timer t;
        mfxStatus status = (*(fMFXVideoCORE_GetHandle) proc) (session, type, hdl);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoCORE_GetHandle called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("type", type));
        Log::WriteLog(context.dump_mfxHDL("hdl", hdl));
        Log::WriteLog("function: MFXVideoCORE_GetHandle(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoCORE_QueryPlatform(mfxSession session, mfxPlatform* platform)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoCORE_QueryPlatform(mfxSession session=" + ToString(session) + ", mfxPlatform* platform=" + ToString(platform) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoCORE_QueryPlatform_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("platform", platform));

        Timer t;
        mfxStatus status = (*(fMFXVideoCORE_QueryPlatform) proc) (session, platform);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoCORE_QueryPlatform called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("platform", platform));
        Log::WriteLog("function: MFXVideoCORE_QueryPlatform(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait)
{
    try{
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) //call function with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = syncp;

                Log::WriteLog("function: MFXVideoCORE_SyncOperation(mfxSession session=" + ToString(session) + ", mfxSyncPoint syncp=" + ToString(syncp) + ", mfxU32 wait=" + ToString(wait) + ") +");
                //sp.component == ENCODE ? Log::WriteLog("SyncOperation(ENCODE," + TimeToString(sp.timer.GetTime()) + ")") : (sp.component == DECODE ? Log::WriteLog("SyncOperation(DECODE, " + ToString(sp.timer.GetTime()) + " sec)") : Log::WriteLog("SyncOperation(VPP, "  + ToString(sp.timer.GetTime()) + " sec)"));
            }
            else {
                // already synced
                Log::WriteLog("Already synced");
                return MFX_ERR_NONE;
            }

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoCORE_SyncOperation_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump("syncp", syncp));
            Log::WriteLog(context.dump_mfxU32("wait", wait));

            Timer t;
            mfxStatus status = (*(fMFXVideoCORE_SyncOperation) proc) (session, sp.syncPoint, wait);
            std::string elapsed = TimeToString(t.GetTime());

            Log::WriteLog(">> MFXVideoCORE_SyncOperation called");
            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));
            Log::WriteLog(context.dump_mfxU32("wait", wait));
            Log::WriteLog("function: MFXVideoCORE_SyncOperation(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call function without logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = syncp;

            }
            else {
                // already synced
                return MFX_ERR_NONE;
            }

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoCORE_SyncOperation_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;

            mfxStatus status = (*(fMFXVideoCORE_SyncOperation) proc) (session, sp.syncPoint, wait);

            return status;
        }
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
