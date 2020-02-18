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

File Name: mfx_video_vpp.cpp

\* ****************************************************************************** */

#include <exception>
#include <iostream>

#include "../loggers/timer.h"
#include "../tracer/functions_table.h"
#include "mfx_structures.h"

// VPP interface functions
mfxStatus MFXVideoVPP_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_Query(mfxSession session=" + ToString(session) + ", mfxVideoParam *in=" + ToString(in) + ", mfxVideoParam *out=" + ToString(out) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_Query_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_Query) proc) (session, in, out);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_Query called");
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));
        Log::WriteLog("function: MFXVideoVPP_Query(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_QueryIOSurf(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ", mfxFrameAllocRequest *request=" + ToString(request) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_QueryIOSurf_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_QueryIOSurf) proc) (session, par, request);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_QueryIOSurf called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));
        Log::WriteLog("function: MFXVideoVPP_QueryIOSurf(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_Init(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_Init(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_Init_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_Init) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_Init called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoVPP_Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_Reset(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_Reset(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_Reset_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_Reset) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_Reset called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoVPP_Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_Close(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_Close(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_Close_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_Close) proc) (session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_Close called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXVideoVPP_Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_GetVideoParam(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_GetVideoParam_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_GetVideoParam) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_GetVideoParam called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoVPP_GetVideoParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_GetVPPStat(mfxSession session, mfxVPPStat *stat)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_VPP;
        Log::WriteLog("function: MFXVideoVPP_GetVPPStat(mfxSession session=" + ToString(session) + ", mfxVPPStat *stat=" + ToString(stat) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoVPP_GetVPPStat_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));

        Timer t;
        mfxStatus status = (*(fMFXVideoVPP_GetVPPStat) proc) (session, stat);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoVPP_GetVPPStat called");
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));
        Log::WriteLog("function: MFXVideoVPP_GetVPPStat(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_RunFrameVPPAsync(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp)
{
    try{
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) // call with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_VPP;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }
            sp.component = VPP;

            Log::WriteLog("function: MFXVideoVPP_RunFrameVPPAsync(mfxSession session=" + ToString(session) + ", mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp) +");
            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoVPP_RunFrameVPPAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            if(in) Log::WriteLog(context.dump("in", *in));
            if(out) Log::WriteLog(context.dump("out", *out));
            if(aux) Log::WriteLog(context.dump("aux", *aux));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            sp.timer.Restart();
            Timer t;
            mfxStatus status = (*(fMFXVideoVPP_RunFrameVPPAsync) proc) (session, in, out, aux, syncp);

            std::string elapsed = TimeToString(t.GetTime());

            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }

            Log::WriteLog(">> MFXVideoVPP_RunFrameVPPAsync called");
            Log::WriteLog(context.dump("session", session));
            if(in) Log::WriteLog(context.dump("in", *in));
            if(out) Log::WriteLog(context.dump("out", *out));
            if(aux) Log::WriteLog(context.dump("aux", *aux));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));
            Log::WriteLog("function: MFXVideoVPP_RunFrameVPPAsync(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call withot logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_VPP;

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoVPP_RunFrameVPPAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;

            mfxStatus status = (*(fMFXVideoVPP_RunFrameVPPAsync) proc) (session, in, out, aux, syncp);

            return status;
        }
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoVPP_RunFrameVPPAsyncEx(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *work, mfxFrameSurface1 **out, mfxSyncPoint *syncp)
{
    try{
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) // call with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_VPP;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }
            sp.component = VPP;
            Log::WriteLog("function: MFXVideoVPP_RunFrameVPPAsyncEx(mfxSession session=" + ToString(session) + ", mfxFrameSurface1 *in, mfxFrameSurface1 *work, mfxExtVppAuxData *aux, mfxSyncPoint *syncp) +");
            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoVPP_RunFrameVPPAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            if(in) Log::WriteLog(context.dump("in", *in));
            if(work) Log::WriteLog(context.dump("work", *work));
            if (out) {
                if (*out)
                    Log::WriteLog(context.dump("out", **out));
            }
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            sp.timer.Restart();
            Timer t;
            mfxStatus status = (*(fMFXVideoVPP_RunFrameVPPAsyncEx) proc) (session, in, work, out, syncp);

            std::string elapsed = TimeToString(t.GetTime());

            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }

            Log::WriteLog(">> MFXVideoVPP_RunFrameVPPAsyncEx called");
            Log::WriteLog(context.dump("session", session));
            if(in) Log::WriteLog(context.dump("in", *in));
            if(work) Log::WriteLog(context.dump("work", *work));
            if (out) {
                if (*out)
                    Log::WriteLog(context.dump("out", **out));
            }
            Log::WriteLog(context.dump("syncp", sp.syncPoint));
            Log::WriteLog("function: MFXVideoVPP_RunFrameVPPAsyncEx(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else //call without logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_VPP;

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoVPP_RunFrameVPPAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;

            mfxStatus status = (*(fMFXVideoVPP_RunFrameVPPAsyncEx) proc) (session, in, work, out, syncp);

            return status;
        }
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
