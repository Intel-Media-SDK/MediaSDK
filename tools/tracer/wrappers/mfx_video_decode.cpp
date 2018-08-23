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

#include <exception>
#include <iostream>

#include "../loggers/timer.h"
#include "../tracer/functions_table.h"
#include "mfx_structures.h"

// DECODE interface functions
mfxStatus MFXVideoDECODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_Query(mfxSession session=" + ToString(session) + ", mfxVideoParam *in=" + ToString(in) + ", mfxVideoParam *out=" + ToString(out) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_Query_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_Query) proc) (session, in, out);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_Query called");
        Log::WriteLog(context.dump("session", session));
        if(in) Log::WriteLog(context.dump("in", *in));
        if(out) Log::WriteLog(context.dump("out", *out));
        Log::WriteLog("function: MFXVideoDECODE_Query(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_DecodeHeader(mfxSession session=" + ToString(session) + ", mfxBitstream *bs=" + ToString(bs) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_DecodeHeader_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(bs) Log::WriteLog(context.dump("bs", *bs));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_DecodeHeader) proc) (session, bs, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_DecodeHeader called");
        Log::WriteLog(context.dump("session", session));
        if(bs) Log::WriteLog(context.dump("bs", *bs));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoDECODE_DecodeHeader(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_QueryIOSurf(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ", mfxFrameAllocRequest *request=" + ToString(request) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_QueryIOSurf_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_QueryIOSurf) proc) (session, par, request);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_QueryIOSurf called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        if(request) Log::WriteLog(context.dump("request", *request));
        Log::WriteLog("function: MFXVideoDECODE_QueryIOSurf(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_Init(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_Init(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_Init_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_Init) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_Init called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoDECODE_Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_Reset(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_Reset(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_Reset_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_Reset) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_Reset called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoDECODE_Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_Close(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_Close(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_Close_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_Close) proc) (session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_Close called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXVideoDECODE_Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_GetVideoParam(mfxSession session=" + ToString(session) + ", mfxVideoParam *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_GetVideoParam_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_GetVideoParam) proc) (session, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_GetVideoParam called");
        Log::WriteLog(context.dump("session", session));
        if(par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("function: MFXVideoDECODE_GetVideoParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_GetDecodeStat(mfxSession session, mfxDecodeStat *stat)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_GetDecodeStat(mfxSession session=" + ToString(session) + ", mfxDecodeStat *stat=" + ToString(stat) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_GetDecodeStat_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_GetDecodeStat) proc) (session, stat);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_GetDecodeStat called");
        Log::WriteLog(context.dump("session", session));
        if(stat) Log::WriteLog(context.dump("stat", *stat));
        Log::WriteLog("function: MFXVideoDECODE_GetDecodeStat(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_SetSkipMode(mfxSession session, mfxSkipMode mode)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_SetSkipMode(mfxSession session=" + ToString(session) + ", mfxSkipMode mode=" + ToString(mode) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_SetSkipMode_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("mode", mode));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_SetSkipMode) proc) (session, mode);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_SetSkipMode called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("mode", mode));
        Log::WriteLog("function: MFXVideoDECODE_SetSkipMode(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_GetPayload(mfxSession session, mfxU64 *ts, mfxPayload *payload)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoDECODE_GetPayload(mfxSession session=" + ToString(session) + ", mfxU64 *ts=" + ToString(ts) + ", mfxPayload *payload=" + ToString(payload) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_GetPayload_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(ts) Log::WriteLog(context.dump_mfxU64("ts", (*ts)));
        if(payload) Log::WriteLog(context.dump("payload", *payload));

        Timer t;
        mfxStatus status = (*(fMFXVideoDECODE_GetPayload) proc) (session, ts, payload);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXVideoDECODE_GetPayload called");
        Log::WriteLog(context.dump("session", session));
        if(ts) Log::WriteLog(context.dump_mfxU64("ts", (*ts)));
        if(payload) Log::WriteLog(context.dump("payload", *payload));
        Log::WriteLog("function: MFXVideoDECODE_GetPayload(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp)
{
    try{
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) // call with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;
            TracerSyncPoint sp;
            if (syncp)
            {
                sp.syncPoint = (*syncp);
            }
            else
            {
                sp.syncPoint = NULL;
            }

            sp.component = DECODE;
            Log::WriteLog("function: MFXVideoDECODE_DecodeFrameAsync(mfxSession session=" + ToString(session) + ", mfxBitstream *bs=" + ToString(bs) + ", mfxFrameSurface1 *surface_work=" + ToString(surface_work) + ", mfxFrameSurface1 **surface_out=" + ToString(surface_out) + ", mfxSyncPoint *syncp=" + ToString(syncp) + ") +");
            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_DecodeFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            if(bs) Log::WriteLog(context.dump("bs", *bs));
            Log::WriteLog(context.dump("surface_work", *surface_work));
            if(surface_out) {
                if (*surface_out)
                    Log::WriteLog(context.dump("surface_out", (**surface_out)));
            }
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            sp.timer.Restart();
            Timer t;
            mfxStatus status = (*(fMFXVideoDECODE_DecodeFrameAsync) proc) (session, bs, surface_work, surface_out, syncp);

            std::string elapsed = TimeToString(t.GetTime());

            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }

            Log::WriteLog(">> MFXVideoDECODE_DecodeFrameAsync called");
            Log::WriteLog(context.dump("session", session));
            if(bs) Log::WriteLog(context.dump("bs", *bs));
            Log::WriteLog(context.dump("surface_work", *surface_work));
            if(surface_out) {
               if (*surface_out)
                Log::WriteLog(context.dump("surface_out", (**surface_out)));
            }
            Log::WriteLog(context.dump("syncp", sp.syncPoint));
            Log::WriteLog("function: MFXVideoDECODE_DecodeFrameAsync(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call without logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoDECODE_DecodeFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;

            mfxStatus status = (*(fMFXVideoDECODE_DecodeFrameAsync) proc) (session, bs, surface_work, surface_out, syncp);

            return status;
        }

    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
