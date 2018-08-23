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


mfxStatus MFXVideoPAK_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoPAK_Query(mfxSession session=" + ToString(session) + ", mfxVideoParam* in=" + ToString(in) + ", mfxVideoParam *out=" + ToString(out) + ") +");
        mfxLoader *loader = (mfxLoader*)session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoPAK_Query_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*(fMFXVideoPAK_Query)proc)(session, in, out);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoPAK_Query called");

        Log::WriteLog(context.dump("session", session));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));

        Log::WriteLog("function: MFXVideoPAK_Query(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoPAK_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoPAK_QueryIOSurf(mfxSession session=" + ToString(session) + ", mfxVideoParam* par=" + ToString(par) + ", mfxFrameAllocRequest *request=" + ToString(request) + ") +");
        mfxLoader *loader = (mfxLoader*)session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoPAK_QueryIOSurf_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));
        if (request) Log::WriteLog(context.dump("request", *request));

        Timer t;
        mfxStatus status = (*(fMFXVideoPAK_QueryIOSurf)proc)(session, par, request);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoPAK_QueryIOSurf called");

        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));
        if (request) Log::WriteLog(context.dump("request", *request));

        Log::WriteLog("function: MFXVideoPAK_QueryIOSurf(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoPAK_Init(mfxSession session, mfxVideoParam *par)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoPAK_Init(mfxSession session=" + ToString(session) + ", mfxVideoParam* par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*)session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoPAK_Init_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoPAK_Init)proc)(session, par);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoPAK_Init called");

        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));

        Log::WriteLog("function: MFXVideoPAK_Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoPAK_Reset(mfxSession session, mfxVideoParam *par)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoPAK_Reset(mfxSession session=" + ToString(session) + ", mfxVideoParam* par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*)session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoPAK_Reset_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoPAK_Reset)proc)(session, par);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoPAK_Reset called");

        Log::WriteLog(context.dump("session", session));
        if (par) Log::WriteLog(context.dump("par", *par));

        Log::WriteLog("function: MFXVideoPAK_Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoPAK_Close(mfxSession session)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoPAK_Close(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*)session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoPAK_Close_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));

        Timer t;
        mfxStatus status = (*(fMFXVideoPAK_Close)proc)(session);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoPAK_Close called");

        Log::WriteLog(context.dump("session", session));

        Log::WriteLog("function: MFXVideoPAK_Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoPAK_ProcessFrameAsync(mfxSession session, mfxPAKInput *in, mfxPAKOutput *out, mfxSyncPoint *syncp)
{
    try {
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) //call with logging
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
            sp.component = ENC;
            Log::WriteLog("function: MFXVideoPAK_ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp) +");
            mfxLoader *loader = (mfxLoader*)session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoPAK_ProcessFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            if (in) Log::WriteLog(context.dump("in", *in));
            if (out) Log::WriteLog(context.dump("out", *out));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            Timer t;
            mfxStatus status = (*(fMFXVideoPAK_ProcessFrameAsync)proc)(session, in, out, syncp);
            std::string elapsed = TimeToString(t.GetTime());

            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }
            Log::WriteLog(">> MFXVideoPAK_ProcessFrameAsync called");

            Log::WriteLog(context.dump("session", session));
            if (in) Log::WriteLog(context.dump("in", *in));
            if (out) Log::WriteLog(context.dump("out", *out));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            Log::WriteLog("function: MFXVideoPAK_ProcessFrameAsync(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call witout loging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;

            mfxLoader *loader = (mfxLoader*)session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoPAK_ProcessFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            
            mfxStatus status = (*(fMFXVideoPAK_ProcessFrameAsync)proc)(session, in, out, syncp);

            return status;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
