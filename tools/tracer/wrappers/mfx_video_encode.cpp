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

File Name: mfx_video_encode.cpp

\* ****************************************************************************** */

#include <exception>
#include <iostream>

#include "../loggers/timer.h"
#include "../tracer/functions_table.h"
#include "mfx_structures.h"
#include <vector>

#if TRACE_CALLBACKS
// BRC callbacs
mfxStatus mfxExtBRC_Init(mfxHDL _pthis, mfxVideoParam* par)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxExtBRC_Init_tracer][1];

        Log::WriteLog("callback: mfxExtBRC::Init(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam* par=" + ToString(par) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxExtBRC_Init proc = (fmfxExtBRC_Init)loader->callbacks[emfxExtBRC_Init_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxExtBRC::Init called");
        Log::WriteLog("callback: mfxExtBRC::Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxExtBRC_Reset(mfxHDL _pthis, mfxVideoParam* par)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxExtBRC_Reset_tracer][1];

        Log::WriteLog("callback: mfxExtBRC::Reset(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam* par=" + ToString(par) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxExtBRC_Reset proc = (fmfxExtBRC_Reset)loader->callbacks[emfxExtBRC_Reset_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxExtBRC::Reset called");
        Log::WriteLog("callback: mfxExtBRC::Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxExtBRC_Close(mfxHDL _pthis)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxExtBRC_Close_tracer][1];

        Log::WriteLog("callback: mfxExtBRC::Close(mfxHDL pthis=" + ToString(pthis) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxExtBRC_Close proc = (fmfxExtBRC_Close)loader->callbacks[emfxExtBRC_Close_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxExtBRC::Close called");
        Log::WriteLog("callback: mfxExtBRC::Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxExtBRC_GetFrameCtrl(mfxHDL _pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxExtBRC_GetFrameCtrl_tracer][1];

        Log::WriteLog("callback: mfxExtBRC::GetFrameCtrl(mfxHDL pthis=" + ToString(pthis)
            + ", mfxBRCFrameParam* par" + ToString(par)
            + ", mfxBRCFrameCtrl* ctrl" + ToString(ctrl) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxExtBRC_GetFrameCtrl proc = (fmfxExtBRC_GetFrameCtrl)loader->callbacks[emfxExtBRC_GetFrameCtrl_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));
        if (ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));

        Timer t;
        mfxStatus status = (*proc) (pthis, par, ctrl);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxExtBRC::GetFrameCtrl called");
        if (ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));
        Log::WriteLog("callback: mfxExtBRC::GetFrameCtrl(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxExtBRC_Update(mfxHDL _pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;

        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxExtBRC_Update_tracer][1];

        Log::WriteLog("callback: mfxExtBRC::Update(mfxHDL pthis=" + ToString(pthis)
            + ", mfxBRCFrameParam* par" + ToString(par)
            + ", mfxBRCFrameCtrl* ctrl" + ToString(ctrl)
            + ", mfxBRCFrameStatus* status" + ToString(status) + ") +");

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        fmfxExtBRC_Update proc = (fmfxExtBRC_Update)loader->callbacks[emfxExtBRC_Update_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));
        if (ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));
        if (status) Log::WriteLog(context.dump("ctrl", *status));

        Timer t;
        mfxStatus sts = (*proc) (pthis, par, ctrl, status);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxExtBRC::Update called");
        if (status) Log::WriteLog(context.dump("ctrl", *status));
        Log::WriteLog("callback: mfxExtBRC::Update(" + elapsed + ", " + context.dump_mfxStatus("status", sts) + ") - \n\n");
        return sts;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
#endif //TRACE_CALLBACKS

// ENCODE interface functions
mfxStatus MFXVideoENCODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_Query(mfxSession session=" + ToString(session) + ", mfxVideoParam *in=" + ToString(in) + ", mfxVideoParam *out=" + ToString(out) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_Query_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump/*_mfxVideoParam*/("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_Query) proc) (session, in, out);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_Query called");
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump/*_mfxVideoParam*/("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));
        Log::WriteLog("function: MFXVideoENCODE_Query(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_QueryIOSurf(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ", mfxFrameAllocRequest *request=" + ToString(request) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_QueryIOSurf_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_QueryIOSurf) proc) (session, par, request);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_QueryIOSurf called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));
        Log::WriteLog("function: MFXVideoENCODE_QueryIOSurf(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_Init(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_Init(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_Init_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

#if TRACE_CALLBACKS
        INIT_CALLBACK_BACKUP(loader->callbacks);
        mfxExtBRC *pBRC = 0, BRC;
        mfxExtCodingOption2* pCO2 = 0;
        mfxVideoParam proxyPar;
        std::vector<mfxExtBuffer*> ExtParam;

        if (par && par->NumExtParam)
        {
            proxyPar = *par;
            ExtParam.resize(proxyPar.NumExtParam);
            proxyPar.ExtParam = &ExtParam[0];

            for (mfxU16 i = 0; i < proxyPar.NumExtParam; i++)
            {
                proxyPar.ExtParam[i] = par->ExtParam[i];

                if (proxyPar.ExtParam[i]->BufferId == MFX_EXTBUFF_BRC)
                {
                    BRC = *(mfxExtBRC*)proxyPar.ExtParam[i];
                    pBRC = &BRC;
                    proxyPar.ExtParam[i] = &BRC.Header;
                }
                if (proxyPar.ExtParam[i]->BufferId == MFX_EXTBUFF_CODING_OPTION2)
                    pCO2 = (mfxExtCodingOption2*)proxyPar.ExtParam[i];
            }
        }

        if (pBRC && pCO2 && pCO2->ExtBRC == MFX_CODINGOPTION_ON)
        {
            par = &proxyPar;
            SET_CALLBACK(mfxExtBRC, pBRC->, Init,           pBRC->pthis);
            SET_CALLBACK(mfxExtBRC, pBRC->, Reset,          pBRC->pthis);
            SET_CALLBACK(mfxExtBRC, pBRC->, Close,          pBRC->pthis);
            SET_CALLBACK(mfxExtBRC, pBRC->, GetFrameCtrl,   pBRC->pthis);
            SET_CALLBACK(mfxExtBRC, pBRC->, Update,         pBRC->pthis);
            if (pBRC->pthis)
                pBRC->pthis = loader;
        }
#endif //TRACE_CALLBACKS

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_Init) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_Init called");
        //No need to dump input-only parameters twice!!!
        //Log::WriteLog(context.dump("session", session));
        //if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoENCODE_Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

#if TRACE_CALLBACKS
        if (status < MFX_ERR_NONE)
            callbacks.RevertAll();
#endif //TRACE_CALLBACKS

        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_Reset(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_Reset(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");

        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_Reset_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_Reset) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_Reset called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoENCODE_Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_Close(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_Close(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_Close_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_Close) proc) (session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_Close called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXVideoENCODE_Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_GetVideoParam(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_GetVideoParam_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_GetVideoParam) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_GetVideoParam called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoENCODE_GetVideoParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_GetEncodeStat(mfxSession session, mfxEncodeStat *stat)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoENCODE_GetEncodeStat(mfxSession session=" + ToString(session) + ", mfxEncodeStat *stat=" + ToString(stat) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_GetEncodeStat_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));

        Timer t;
        mfxStatus status = (*(fMFXVideoENCODE_GetEncodeStat) proc) (session, stat);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoENCODE_GetEncodeStat called");
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));
        Log::WriteLog("function: MFXVideoENCODE_GetEncodeStat(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoENCODE_EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
    try{
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) // call with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }
            sp.component = ENCODE;
            Log::WriteLog("function: MFXVideoENCODE_EncodeFrameAsync(mfxSession session=" + ToString(session) + ", mfxEncodeCtrl *ctrl=" + ToString(ctrl) + ", mfxFrameSurface1 *surface=" + ToString(surface) + ", mfxBitstream *bs=" + ToString(bs) + ", mfxSyncPoint *syncp=" + ToString(syncp) + ") +");
            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_EncodeFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            if(ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));
            if(surface) Log::WriteLog(context.dump("surface", *surface));
            if(bs) Log::WriteLog(context.dump("bs", *bs));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            sp.timer.Restart();
            Timer t;
            mfxStatus status = (*(fMFXVideoENCODE_EncodeFrameAsync) proc) (session, ctrl, surface, bs, syncp);

            std::string elapsed = TimeToString(t.GetTime());

            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }

            Log::WriteLog(">> MFXVideoENCODE_EncodeFrameAsync called");
            Log::WriteLog(context.dump("session", session));
            if(ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));
            if(surface) Log::WriteLog(context.dump("surface", *surface));
            if(bs) Log::WriteLog(context.dump("bs", *bs));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));
            Log::WriteLog("function: MFXVideoENCODE_EncodeFrameAsync(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call without loging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoENCODE_EncodeFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;

            mfxStatus status = (*(fMFXVideoENCODE_EncodeFrameAsync) proc) (session, ctrl, surface, bs, syncp);

            return status;
        }
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
