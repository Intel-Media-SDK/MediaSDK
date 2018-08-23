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



mfxStatus MFXQueryIMPL(mfxSession session, mfxIMPL *impl)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXQueryIMPL(mfxSession session=" + ToString(session) + ", mfxIMPL *impl=" + ToString(impl) + ") +");
        Log::WriteLog(context.dump("session", session));
        if (impl) Log::WriteLog(context.dump("impl", *impl));
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXQueryIMPL_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if (impl) Log::WriteLog(context.dump("impl", *impl));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxIMPL *impl)) proc) (session, impl);


        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXQueryIMPL called");
        Log::WriteLog(context.dump("session", session));
        if (impl) Log::WriteLog(context.dump("impl", *impl));
        Log::WriteLog("function: MFXQueryIMPL(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXQueryVersion(mfxSession session, mfxVersion *version)
{

    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXQueryVersion(mfxSession session=" + ToString(session) + ", mfxVersion *version=" + ToString(version) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXQueryVersion_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;

        Log::WriteLog(context.dump("session", session));
        if(version) Log::WriteLog(context.dump("version", *version));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxVersion *version)) proc) (session, version);

        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXQueryVersion called");
        Log::WriteLog(context.dump("session", session));
        if(version) Log::WriteLog(context.dump("version", *version));
        Log::WriteLog("function: MFXQueryVersion(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

/*
 * API version 1.1 functions
 */
mfxStatus MFXJoinSession(mfxSession session, mfxSession child_session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXJoinSession(mfxSession session=" + ToString(session) + ", mfxSession child_session=" + ToString(child_session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;
        mfxLoader *tmp_loader = (mfxLoader*) child_session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXJoinSession_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        child_session = tmp_loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("child_session", child_session));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxSession child_session)) proc) (session, child_session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXJoinSession called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("child_session", child_session));
        Log::WriteLog("function: MFXJoinSession(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXCloneSession(mfxSession session, mfxSession *clone)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXCloneSession(mfxSession session=" + ToString(session) + ", mfxSession *clone=" + ToString(clone) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXCloneSession_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(clone) Log::WriteLog(context.dump("clone", (*clone)));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxSession *clone)) proc) (session, clone);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXCloneSession called");
        Log::WriteLog(context.dump("session", session));
        if(clone) Log::WriteLog(context.dump("clone", (*clone)));
        Log::WriteLog("function: MFXCloneSession(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXDisjoinSession(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXDisjoinSession(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXCloneSession_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session)) proc) (session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXDisjoinSession called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXDisjoinSession(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXSetPriority(mfxSession session, mfxPriority priority)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXSetPriority(mfxSession session=" + ToString(session) + ", mfxPriority priority=" + ToString(priority) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXSetPriority_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("priority", priority));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxPriority priority)) proc) (session, priority);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXSetPriority called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump("priority", priority));
        Log::WriteLog("function: MFXSetPriority(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXGetPriority(mfxSession session, mfxPriority *priority)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXGetPriority(mfxSession session=" + ToString(session) + ", mfxPriority *priority=" + ToString(priority) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXGetPriority_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        if(priority) Log::WriteLog(context.dump("priority", *priority));

        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session, mfxPriority *priority)) proc) (session, priority);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXGetPriority called");
        Log::WriteLog(context.dump("session", session));
        if(priority) Log::WriteLog(context.dump("priority", *priority));
        Log::WriteLog("function: MFXGetPriority(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXDoWork(mfxSession session)
{
    try{
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXDoWork(mfxSession session=" + ToString(session) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXDoWork_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));


        Timer t;
        mfxStatus status = (*(mfxStatus (MFX_CDECL*) (mfxSession session)) proc) (session);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> MFXGetPriority called");
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog("function: MFXDoWorck(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
