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

//test_blocks.cpp - MFX functions only (one block = one function)
//prefix "b_" for each block
#include "stdafx.h"
#include "test_common.h"
#include "mfx_plugin_loader.h"
#include <cassert>

#define TRACE_FUNCN(n, name, p1, p2, p3, p4, p5, p6, p7)\
    if(var_def<bool>("trace", false)){\
        INC_PADDING();\
        if((n) > 0) std::cout << print_param.padding << #p1 " = " << p1 << '\n';\
        if((n) > 1) std::cout << print_param.padding << #p2 " = " << p2 << '\n';\
        if((n) > 2) std::cout << print_param.padding << #p3 " = " << p3 << '\n';\
        if((n) > 3) std::cout << print_param.padding << #p4 " = " << p4 << '\n';\
        if((n) > 4) std::cout << print_param.padding << #p5 " = " << p5 << '\n';\
        if((n) > 5) std::cout << print_param.padding << #p6 " = " << p6 << '\n';\
        if((n) > 8) std::cout << print_param.padding << #p7 " = " << p6 << '\n';\
        std::cout << print_param.padding << "expectedRes = " << var_def<mfxStatus>("expectedRes", MFX_ERR_NONE) << "\n\n";\
        std::cout << print_param.padding << "mfxRes = " << #name << '(';\
        if((n) > 0) std::cout << #p1;\
        if((n) > 1) std::cout << ", " << #p2;\
        if((n) > 2) std::cout << ", " << #p3;\
        if((n) > 3) std::cout << ", " << #p4;\
        if((n) > 4) std::cout << ", " << #p5;\
        if((n) > 5) std::cout << ", " << #p6;\
        if((n) > 6) std::cout << ", " << #p7;\
        std::cout << ");\n";\
        DEC_PADDING();\
    }
#define TRACE_FUNC6(name, p1, p2, p3, p4, p5, p6) TRACE_FUNCN(6, name, p1, p2, p3, p4, p5, p6, 0)
#define TRACE_FUNC5(name, p1, p2, p3, p4, p5) TRACE_FUNCN(5, name, p1, p2, p3, p4, p5, 0, 0)
#define TRACE_FUNC4(name, p1, p2, p3, p4) TRACE_FUNCN(4, name, p1, p2, p3, p4, 0, 0, 0)
#define TRACE_FUNC3(name, p1, p2, p3) TRACE_FUNCN(3, name, p1, p2, p3, 0, 0, 0, 0)
#define TRACE_FUNC2(name, p1, p2) TRACE_FUNCN(2, name, p1, p2, 0, 0, 0, 0, 0)
#define TRACE_FUNC1(name, p1) TRACE_FUNCN(1, name, p1, 0, 0, 0, 0, 0, 0)
#define TRACE_FUNC0(name) TRACE_FUNCN(0, name, 0, 0, 0, 0, 0, 0, 0)

// Global Functions ///////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXInit){
    mfxVersion default_ver;
    default_ver.Major = MFX_VERSION_MAJOR;
    default_ver.Minor = MFX_VERSION_MINOR;
    mfxIMPL default_impl =  MFX_IMPL_AUTO;
    std::string platform = var_def<const char*>("platform", "");
    if(platform.size()){
        TRACE_PAR(platform);

        if(platform.find("_sw_") != platform.npos)
            default_impl = MFX_IMPL_SOFTWARE;
        else
            default_impl = MFX_IMPL_HARDWARE;

        if(platform.find("d3d11") != platform.npos)
            default_impl |= MFX_IMPL_VIA_D3D11;
    }
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxSession*&    pSession    = var_def<mfxSession*>  ("p_session",   &session);
    mfxIMPL&        impl        = var_def<mfxIMPL>      ("impl",        default_impl);
    mfxVersion&     version     = var_def<mfxVersion>   ("version",     default_ver);
    mfxVersion*&    pVersion    = var_def<mfxVersion*>  ("p_version",   &version);

    TRACE_FUNC3(MFXInit, impl, pVersion, pSession);
    mfxRes = MFXInit(impl, pVersion, pSession);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pVersion);
    TRACE_PAR(pSession);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}


msdk_ts_BLOCK(b_MFXClose){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXClose, session);
    mfxRes = MFXClose(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXQueryIMPL){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxIMPL&        impl        = var_def<mfxIMPL>      ("impl",        MFX_IMPL_AUTO);
    mfxIMPL*&       pImpl       = var_def<mfxIMPL*>     ("p_impl",      &impl);

    TRACE_FUNC2(MFXQueryIMPL, session, pImpl);
    mfxRes = MFXQueryIMPL(session, pImpl);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pImpl);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXQueryVersion){
    mfxVersion default_ver = {0};
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVersion&     version     = var_def<mfxVersion>   ("version",     default_ver);
    mfxVersion*&    pVersion    = var_def<mfxVersion*>  ("p_version",   &version);

    TRACE_FUNC2(MFXQueryVersion, session, pVersion);
    mfxRes = MFXQueryVersion(session, pVersion);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pVersion);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXJoinSession){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",          MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes",     MFX_ERR_NONE);
    mfxSession&     parent      = var_def<mfxSession>   ("session_parent",  0);
    mfxSession&     child       = var_def<mfxSession>   ("session_child",   0);

    TRACE_FUNC2(MFXJoinSession, parent, child);
    mfxRes = MFXJoinSession(parent, child);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXDisjoinSession){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXDisjoinSession, session);
    mfxRes = MFXDisjoinSession(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXCloneSession){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxSession&     clone       = var_def<mfxSession>   ("clone",       0);
    mfxSession*&    pClone      = var_def<mfxSession*>  ("p_clone",     &clone);

    TRACE_FUNC2(MFXCloneSession, session, pClone);
    mfxRes = MFXCloneSession(session, pClone);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pClone);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXSetPriority){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxPriority&    priority    = var_def<mfxPriority>  ("priority",    MFX_PRIORITY_NORMAL);

    TRACE_FUNC2(MFXSetPriority, session, priority);
    mfxRes = MFXSetPriority(session, priority);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXGetPriority){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxPriority&    priority    = var_def<mfxPriority>  ("priority",    MFX_PRIORITY_NORMAL);
    mfxPriority*&   pPriority   = var_def<mfxPriority*> ("p_priority",  &priority);

    TRACE_FUNC2(MFXGetPriority, session, pPriority);
    mfxRes = MFXGetPriority(session, pPriority);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pPriority);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

// VideoCORE //////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXVideoCORE_SetBufferAllocator){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxBufferAllocator*& pAllocator = var_def<mfxBufferAllocator*>("p_buf_allocator", NULL);

    TRACE_FUNC2(MFXVideoCORE_SetBufferAllocator, session, pAllocator);
    mfxRes = MFXVideoCORE_SetBufferAllocator(session, pAllocator);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoCORE_SetFrameAllocator){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxFrameAllocator*& pAllocator = var_def<mfxFrameAllocator*>("p_frame_allocator", NULL);

    TRACE_FUNC2(MFXVideoCORE_SetFrameAllocator, session, pAllocator);
    mfxRes = MFXVideoCORE_SetFrameAllocator(session, pAllocator);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoCORE_SetHandle){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxHandleType&  type        = var_def<mfxHandleType>("hdl_type",    MFX_HANDLE_D3D9_DEVICE_MANAGER);
    mfxHDL&         hdl         = var_def<mfxHDL>       ("hdl",         NULL);

    TRACE_FUNC3(MFXVideoCORE_SetHandle, session, type, hdl);
    mfxRes = MFXVideoCORE_SetHandle(session, type, hdl);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoCORE_GetHandle){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxHandleType&  type        = var_def<mfxHandleType>("hdl_type",    MFX_HANDLE_D3D9_DEVICE_MANAGER);
    mfxHDL&         hdl         = var_def<mfxHDL>       ("hdl",         NULL);
    mfxHDL*&        pHdl        = var_def<mfxHDL*>      ("p_hdl",       &hdl);

    TRACE_FUNC3(MFXVideoCORE_GetHandle, session, type, pHdl);
    mfxRes = MFXVideoCORE_GetHandle(session, type, pHdl);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pHdl);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoCORE_SyncOperation){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxSyncPoint&   syncp       = var_def<mfxSyncPoint> ("syncp",       NULL);
    mfxU32&         wait        = var_def<mfxU32>       ("wait",        MFX_INFINITE);

    TRACE_FUNC3(MFXVideoCORE_SyncOperation, session, syncp, wait);
    mfxRes = MFXVideoCORE_SyncOperation(session, syncp, wait);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

// VideoENCODE ////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXVideoENCODE_Query){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in",  NULL);
    mfxVideoParam*& pOut        = var_def<mfxVideoParam*>("p_mfxvp_out", NULL);

    TRACE_FUNC3(MFXVideoENCODE_Query, session, pIn, pOut);
    mfxRes = MFXVideoENCODE_Query(session, pIn, pOut);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    TRACE_PAR(pOut);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_QueryIOSurf){
    mfxFrameAllocRequest d = {0};
    mfxStatus&             mfxRes      = var_def<mfxStatus>             ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&             expectedRes = var_def<mfxStatus>             ("expectedRes", MFX_ERR_NONE);
    mfxSession&            session     = var_def<mfxSession>            ("session",     0);
    mfxVideoParam*&        pIn         = var_def<mfxVideoParam*>        ("p_mfxvp_in",  NULL);
    mfxFrameAllocRequest&  request     = var_def<mfxFrameAllocRequest>  ("request",     d);
    mfxFrameAllocRequest*& pRequest    = var_def<mfxFrameAllocRequest*> ("p_request",   &request);

    TRACE_FUNC3(MFXVideoENCODE_QueryIOSurf, session, pIn, pRequest);
    mfxRes = MFXVideoENCODE_QueryIOSurf(session, pIn, pRequest);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    TRACE_PAR(pRequest);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_Init){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoENCODE_Init, session, pIn);
    mfxRes = MFXVideoENCODE_Init(session, pIn);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_Reset){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoENCODE_Reset, session, pIn);
    mfxRes = MFXVideoENCODE_Reset(session, pIn);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_Close){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXVideoENCODE_Close, session);
    mfxRes = MFXVideoENCODE_Close(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_GetVideoParam){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoENCODE_GetVideoParam, session, pIn);
    mfxRes = MFXVideoENCODE_GetVideoParam(session, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_GetEncodeStat){
    mfxEncodeStat d = {0};
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxEncodeStat&  stat        = var_def<mfxEncodeStat> ("enc_stat",    d);
    mfxEncodeStat*& pStat       = var_def<mfxEncodeStat*>("p_enc_stat",  &stat);

    TRACE_FUNC2(MFXVideoENCODE_GetEncodeStat, session, pStat);
    mfxRes = MFXVideoENCODE_GetEncodeStat(session, pStat);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pStat);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoENCODE_EncodeFrameAsync){
    mfxStatus&          mfxRes      = var_def<mfxStatus>        ("mfxRes",      MFX_ERR_NONE);
    mfxSession&         session     = var_def<mfxSession>       ("session",     0);
    mfxEncodeCtrl*&     pCtrl       = var_def<mfxEncodeCtrl*>   ("p_enc_ctrl",  NULL);
    mfxFrameSurface1*&  pSurf       = var_def<mfxFrameSurface1*>("p_free_surf", NULL);
    mfxBitstream*&      pBs         = var_def<mfxBitstream*>    ("p_enc_bs",    NULL);
    mfxSyncPoint&       syncp       = var_def<mfxSyncPoint>     ("syncp",       NULL);
    mfxSyncPoint*&      pSyncp      = var_def<mfxSyncPoint*>    ("p_syncp",     &syncp);

    TRACE_FUNC5(MFXVideoENCODE_EncodeFrameAsync, session, pCtrl, pSurf, pBs, pSyncp);
    mfxRes = MFXVideoENCODE_EncodeFrameAsync(session, pCtrl, pSurf, pBs, pSyncp);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pCtrl);
    TRACE_PAR(pSurf);
    TRACE_PAR(pBs);
    TRACE_PAR(pSyncp);

    return msdk_ts::resOK;
}

// VideoDECODE ////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXVideoDECODE_Query){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in",  NULL);
    mfxVideoParam*& pOut        = var_def<mfxVideoParam*>("p_mfxvp_out", NULL);

    TRACE_FUNC3(MFXVideoDECODE_Query, session, pIn, pOut);
    mfxRes = MFXVideoDECODE_Query(session, pIn, pOut);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pOut);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_DecodeHeader){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in",  NULL);
    mfxBitstream*&  pBs         = var_def<mfxBitstream*> ("p_dec_bs",    NULL);

    TRACE_FUNC3(MFXVideoDECODE_DecodeHeader, session, pBs, pIn);
    mfxRes = MFXVideoDECODE_DecodeHeader(session, pBs, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pBs);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_QueryIOSurf){
    mfxFrameAllocRequest d = {0};
    mfxStatus&             mfxRes      = var_def<mfxStatus>             ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&             expectedRes = var_def<mfxStatus>             ("expectedRes", MFX_ERR_NONE);
    mfxSession&            session     = var_def<mfxSession>            ("session",     0);
    mfxVideoParam*&        pIn         = var_def<mfxVideoParam*>        ("p_mfxvp_in",  NULL);
    mfxFrameAllocRequest&  request     = var_def<mfxFrameAllocRequest>  ("request",     d);
    mfxFrameAllocRequest*& pRequest    = var_def<mfxFrameAllocRequest*> ("p_request",   &request);

    TRACE_FUNC3(MFXVideoDECODE_QueryIOSurf, session, pIn, pRequest);
    mfxRes = MFXVideoDECODE_QueryIOSurf(session, pIn, pRequest);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pRequest);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_Init){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoDECODE_Init, session, pIn);
    mfxRes = MFXVideoDECODE_Init(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_Reset){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoDECODE_Reset, session, pIn);
    mfxRes = MFXVideoDECODE_Reset(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_Close){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXVideoDECODE_Close, session);
    mfxRes = MFXVideoDECODE_Close(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_GetVideoParam){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoDECODE_GetVideoParam, session, pIn);
    mfxRes = MFXVideoDECODE_GetVideoParam(session, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_GetDecodeStat){
    mfxDecodeStat d = {0};
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxDecodeStat&  stat        = var_def<mfxDecodeStat> ("dec_stat",    d);
    mfxDecodeStat*& pStat       = var_def<mfxDecodeStat*>("p_dec_stat",  &stat);

    TRACE_FUNC2(MFXVideoDECODE_GetDecodeStat, session, pStat);
    mfxRes = MFXVideoDECODE_GetDecodeStat(session, pStat);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pStat);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_SetSkipMode){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxSkipMode&    skip_mode   = var_def<mfxSkipMode>   ("skip_mode",   MFX_SKIPMODE_NOSKIP);

    TRACE_FUNC2(MFXVideoDECODE_SetSkipMode, session, skip_mode);
    mfxRes = MFXVideoDECODE_SetSkipMode(session, skip_mode);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoDECODE_GetPayload){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxU64*&        pTS         = var_def<mfxU64*>       ("p_ts",        NULL);
    mfxPayload*&    pPayload    = var_def<mfxPayload*>   ("p_payload",   NULL);

    TRACE_FUNC3(MFXVideoDECODE_GetPayload, session, pTS, pPayload);
    mfxRes = MFXVideoDECODE_GetPayload(session, pTS, pPayload);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pPayload);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}


msdk_ts_BLOCK(b_MFXVideoDECODE_DecodeFrameAsync){
    mfxStatus&          mfxRes      = var_def<mfxStatus>         ("mfxRes",      MFX_ERR_NONE);
    mfxSession&         session     = var_def<mfxSession>        ("session",     0);
    mfxBitstream*&      pBs         = var_def<mfxBitstream*>     ("p_dec_bs",    NULL);
    mfxFrameSurface1*&  pSurf       = var_def<mfxFrameSurface1*> ("p_free_surf", NULL);
    mfxFrameSurface1**& ppSurfOut   = var_def<mfxFrameSurface1**>("p_p_dec_out_surf", &var_def<mfxFrameSurface1*>("p_dec_out_surf", NULL));
    mfxSyncPoint&       syncp       = var_def<mfxSyncPoint>      ("syncp",       NULL);
    mfxSyncPoint*&      pSyncp      = var_def<mfxSyncPoint*>     ("p_syncp",     &syncp);

    TRACE_FUNC5(MFXVideoDECODE_DecodeFrameAsync, session, pBs, pSurf, ppSurfOut, pSyncp);
    mfxRes = MFXVideoDECODE_DecodeFrameAsync(session, pBs, pSurf, ppSurfOut, pSyncp);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pBs);
    TRACE_PAR(pSurf);
    TRACE_PAR(ppSurfOut);
    TRACE_PAR(pSyncp);

    return msdk_ts::resOK;
}
// VideoVPP ///////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXVideoVPP_Query){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in",  NULL);
    mfxVideoParam*& pOut        = var_def<mfxVideoParam*>("p_mfxvp_out", NULL);

    TRACE_FUNC3(MFXVideoVPP_Query, session, pIn, pOut);
    mfxRes = MFXVideoVPP_Query(session, pIn, pOut);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pOut);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_QueryIOSurf){
    mfxFrameAllocRequest* d = NULL;
    mfxStatus&             mfxRes      = var_def<mfxStatus>             ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&             expectedRes = var_def<mfxStatus>             ("expectedRes", MFX_ERR_NONE);
    mfxSession&            session     = var_def<mfxSession>            ("session",     0);
    mfxVideoParam*&        pIn         = var_def<mfxVideoParam*>        ("p_mfxvp_in",  NULL);
    if(!is_defined("vpp_request")) 
        d = (mfxFrameAllocRequest*)alloc(2*sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest*& pRequest    = var_def<mfxFrameAllocRequest*> ("vpp_request",   d);

    TRACE_FUNC3(MFXVideoVPP_QueryIOSurf, session, pIn, pRequest);
    mfxRes = MFXVideoVPP_QueryIOSurf(session, pIn, pRequest);
    TRACE_PAR(mfxRes);
    if(pRequest){
        TRACE_PAR(pRequest[0]);
        TRACE_PAR(pRequest[1]);
    }else{
        TRACE_PAR(pRequest);
    }
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_Init){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoVPP_Init, session, pIn);
    mfxRes = MFXVideoVPP_Init(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_Reset){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoVPP_Reset, session, pIn);
    mfxRes = MFXVideoVPP_Reset(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_Close){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXVideoVPP_Close, session);
    mfxRes = MFXVideoVPP_Close(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_GetVideoParam){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVideoParam*& pIn         = var_def<mfxVideoParam*>("p_mfxvp_in", NULL);

    TRACE_FUNC2(MFXVideoVPP_GetVideoParam, session, pIn);
    mfxRes = MFXVideoVPP_GetVideoParam(session, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_GetVPPStat){
    mfxVPPStat d = {0};
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxVPPStat&     stat        = var_def<mfxVPPStat>    ("vpp_stat",    d);
    mfxVPPStat*&    pStat       = var_def<mfxVPPStat*>   ("p_vpp_stat",  &stat);

    TRACE_FUNC2(MFXVideoVPP_GetVPPStat, session, pStat);
    mfxRes = MFXVideoVPP_GetVPPStat(session, pStat);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pStat);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoVPP_RunFrameVPPAsync){
    mfxStatus&          mfxRes      = var_def<mfxStatus>         ("mfxRes",         MFX_ERR_NONE);
    mfxSession&         session     = var_def<mfxSession>        ("session",        0);
    mfxFrameSurface1*&  pSurIn      = var_def<mfxFrameSurface1*> ("p_free_surf",    NULL);
    mfxFrameSurface1*&  pSurfOut    = var_def<mfxFrameSurface1*> ("p_vpp_out_surf", NULL);
    mfxExtVppAuxData*&  pAUX        = var_def<mfxExtVppAuxData*> ("p_aux",          NULL);
    mfxSyncPoint&       syncp       = var_def<mfxSyncPoint>      ("syncp",          NULL);
    mfxSyncPoint*&      pSyncp      = var_def<mfxSyncPoint*>     ("p_syncp",        &syncp);

    TRACE_FUNC5(MFXVideoVPP_RunFrameVPPAsync, session, pSurIn, pSurfOut, pAUX, pSyncp);
    mfxRes = MFXVideoVPP_RunFrameVPPAsync(session, pSurIn, pSurfOut, pAUX, pSyncp);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pSurIn);
    TRACE_PAR(pSurfOut);
    TRACE_PAR(pAUX);
    TRACE_PAR(pSyncp);

    return msdk_ts::resOK;
}

#ifdef __MFXPLUGIN_H__
msdk_ts_BLOCK(b_MFXVideoUSER_Register){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxU32&         type        = var_def<mfxU32>       ("type",        0);
    mfxPlugin*&     pPlugin     = var_def<mfxPlugin*>   ("p_plugin",    NULL);
    
    TRACE_FUNC3(MFXVideoUSER_Register, session, type, pPlugin);
    mfxRes = MFXVideoUSER_Register(session, type, pPlugin);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoUSER_Unregister){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxU32&         type        = var_def<mfxU32>       ("type",        0);

    TRACE_FUNC2(MFXVideoUSER_Unregister, session, type);
    mfxRes = MFXVideoUSER_Unregister(session, type);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoUSER_ProcessFrameAsync){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxHDL&         pIn         = var_def<mfxHDL>       ("up_in",    NULL); //up_in stands for "User plugin in"
    mfxHDL&         pOut        = var_def<mfxHDL>       ("up_out",   NULL);
    mfxU32&         in_num      = var_def<mfxU32>       ("in_num",      1);
    mfxU32&         out_num     = var_def<mfxU32>       ("out_num",     1);
    mfxSyncPoint&   syncp       = var_def<mfxSyncPoint> ("syncp",    NULL);
    mfxSyncPoint*&  pSyncp      = var_def<mfxSyncPoint*>("p_syncp",&syncp);

    TRACE_FUNC6(MFXVideoUSER_ProcessFrameAsync, session, pIn, in_num, pOut, out_num, pSyncp);
    mfxRes = MFXVideoUSER_ProcessFrameAsync(session, &pIn, in_num, &pOut, out_num, pSyncp);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoUSER_Load){
    mfxStatus&     mfxRes      = var_def<mfxStatus>    ("mfxRes",       MFX_ERR_NONE);
    mfxStatus&     expectedRes = var_def<mfxStatus>    ("expectedRes",  MFX_ERR_NONE);
    mfxSession&    session     = var_def<mfxSession>   ("session",      0);
    mfxU32&        version     = var_def<mfxU32>       ("version",      0);
    mfxPluginUID*& p_uid       = var_def<mfxPluginUID*>("p_plugin_uid",NULL);

    TRACE_FUNC3(MFXVideoUSER_Load, session, p_uid, version);
    mfxRes = MFXVideoUSER_Load(session, p_uid, version);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXVideoUSER_UnLoad){
    mfxStatus&     mfxRes      = var_def<mfxStatus>    ("mfxRes",       MFX_ERR_NONE);
    mfxStatus&     expectedRes = var_def<mfxStatus>    ("expectedRes",  MFX_ERR_NONE);
    mfxSession&    session     = var_def<mfxSession>   ("session",      0);
    mfxPluginUID*& p_uid       = var_def<mfxPluginUID*> ("p_plugin_uid",NULL);

    TRACE_FUNC2(MFXVideoUSER_UnLoad, session, p_uid);
    mfxRes = MFXVideoUSER_UnLoad(session, p_uid);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}
#endif //#ifdef __MFXPLUGIN_H__

#ifdef __MFXAUDIO_H__
// audio core
msdk_ts_BLOCK(b_MFXInitAudio){
    mfxVersion default_ver;
    default_ver.Major = MFX_VERSION_MAJOR;
    default_ver.Minor = MFX_VERSION_MINOR;
    mfxIMPL default_impl =  MFX_IMPL_AUDIO;
    std::string platform = var_def<const char*>("platform", "");
    if(platform.size()){
        TRACE_PAR(platform);

        if(platform.find("_sw_") != platform.npos)
            default_impl = MFX_IMPL_SOFTWARE;
        else
            default_impl = MFX_IMPL_HARDWARE;

        if(platform.find("d3d11") != platform.npos)
            default_impl |= MFX_IMPL_VIA_D3D11;
    }
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxSession*&    pSession    = var_def<mfxSession*>  ("p_session",   &session);
    mfxIMPL&        impl        = var_def<mfxIMPL>      ("impl",        default_impl);
    mfxVersion&     version     = var_def<mfxVersion>   ("version",     default_ver);
    mfxVersion*&    pVersion    = var_def<mfxVersion*>  ("p_version",   &version);

    TRACE_FUNC3(MFXInit, impl, pVersion, pSession);
    mfxRes = MFXInit(impl, pVersion, pSession);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pVersion);
    TRACE_PAR(pSession);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXCloseAudio){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXClose, session);
    mfxRes = MFXClose(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioQueryIMPL){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxIMPL&        impl        = var_def<mfxIMPL>      ("impl",        MFX_IMPL_AUTO);
    mfxIMPL*&       pImpl       = var_def<mfxIMPL*>     ("p_impl",      &impl);

    TRACE_FUNC2(MFXQueryIMPL, session, pImpl);
    mfxRes = MFXQueryIMPL(session, pImpl);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pImpl);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioQueryVersion){
    mfxVersion default_ver = {0};
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxVersion&     version     = var_def<mfxVersion>   ("version",     default_ver);
    mfxVersion*&    pVersion    = var_def<mfxVersion*>  ("p_version",   &version);

    TRACE_FUNC2(MFXQueryVersion, session, pVersion);
    mfxRes = MFXQueryVersion(session, pVersion);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pVersion);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

// AudioENCODE ////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXAudioENCODE_Query){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in",  NULL);
    mfxAudioParam*& pOut        = var_def<mfxAudioParam*>("p_mfxap_out", NULL);

    TRACE_FUNC3(MFXAudioENCODE_Query, session, pIn, pOut);
    mfxRes = MFXAudioENCODE_Query(session, pIn, pOut);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    TRACE_PAR(pOut);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_QueryIOSize){
    mfxAudioAllocRequest d = {0};
    mfxStatus&             mfxRes      = var_def<mfxStatus>             ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&             expectedRes = var_def<mfxStatus>             ("expectedRes", MFX_ERR_NONE);
    mfxSession&            session     = var_def<mfxSession>            ("session",     0);
    mfxAudioParam*&        pIn         = var_def<mfxAudioParam*>        ("p_mfxap_in",  NULL);
    mfxAudioAllocRequest&  request     = var_def<mfxAudioAllocRequest>  ("request",     d);
    mfxAudioAllocRequest*& pRequest    = var_def<mfxAudioAllocRequest*> ("p_request",   &request);

    TRACE_FUNC3(MFXAudioENCODE_QueryIOSize, session, pIn, pRequest);
    mfxRes = MFXAudioENCODE_QueryIOSize(session, pIn, pRequest);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    TRACE_PAR(pRequest);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_Init){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioENCODE_Init, session, pIn);
    mfxRes = MFXAudioENCODE_Init(session, pIn);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_Reset){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioENCODE_Reset, session, pIn);
    mfxRes = MFXAudioENCODE_Reset(session, pIn);
    TRACE_PAR(mfxRes);
    //TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_Close){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXAudioENCODE_Close, session);
    mfxRes = MFXAudioENCODE_Close(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_QueryIOSize){
    mfxAudioAllocRequest d = {0};
    mfxStatus&             mfxRes      = var_def<mfxStatus>             ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&             expectedRes = var_def<mfxStatus>             ("expectedRes", MFX_ERR_NONE);
    mfxSession&            session     = var_def<mfxSession>            ("session",     0);
    mfxAudioParam*&        pIn         = var_def<mfxAudioParam*>        ("p_mfxap_in",  NULL);
    mfxAudioAllocRequest&  request     = var_def<mfxAudioAllocRequest>  ("request",     d);
    mfxAudioAllocRequest*& pRequest    = var_def<mfxAudioAllocRequest*> ("p_request",   &request);

    TRACE_FUNC3(MFXAudioDECODE_QueryIOSize, session, pIn, pRequest);
    mfxRes = MFXAudioDECODE_QueryIOSize(session, pIn, pRequest);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pRequest);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_GetAudioParam){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioENCODE_GetAudioParam, session, pIn);
    mfxRes = MFXAudioENCODE_GetAudioParam(session, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioENCODE_EncodeFrameAsync){
    mfxStatus&          mfxRes      = var_def<mfxStatus>        ("mfxRes",      MFX_ERR_NONE);
    mfxSession&         session     = var_def<mfxSession>       ("session",     0);
    mfxBitstream*&      pBsIn       = var_def<mfxBitstream*>    ("p_bs_in",    NULL);
    mfxBitstream*&      pBsOut      = var_def<mfxBitstream*>    ("p_bs_out",    NULL);
    mfxSyncPoint&       syncp       = var_def<mfxSyncPoint>     ("syncp",       NULL);
    mfxSyncPoint*&      pSyncp      = var_def<mfxSyncPoint*>    ("p_syncp",     &syncp);

    TRACE_FUNC4(MFXAudioENCODE_EncodeFrameAsync, session, pBsIn, pBsOut, pSyncp);
    assert(!"Need to update beh test!!");
    //mfxRes = MFXAudioENCODE_EncodeFrameAsync(session, pBsIn, pBsOut, pSyncp);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pBsIn);
    TRACE_PAR(pBsOut);
    TRACE_PAR(pSyncp);

    return msdk_ts::resOK;
}

// AudioDECODE ////////////////////////////////////////////////////////////////
msdk_ts_BLOCK(b_MFXAudioDECODE_Query){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in",  NULL);
    mfxAudioParam*& pOut        = var_def<mfxAudioParam*>("p_mfxap_out", NULL);

    TRACE_FUNC3(MFXAudioDECODE_Query, session, pIn, pOut);
    mfxRes = MFXAudioDECODE_Query(session, pIn, pOut);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pOut);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_DecodeHeader){
    mfxStatus&      mfxRes      = var_def<mfxStatus>     ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>     ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>    ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in",  NULL);
    mfxBitstream*&  pBs         = var_def<mfxBitstream*> ("p_bs_in",    NULL);

    TRACE_FUNC3(MFXAudioDECODE_DecodeHeader, session, pBs, pIn);
    mfxRes = MFXAudioDECODE_DecodeHeader(session, pBs, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pBs);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_Init){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioDECODE_Init, session, pIn);
    mfxRes = MFXAudioDECODE_Init(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_Reset){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioDECODE_Reset, session, pIn);
    mfxRes = MFXAudioDECODE_Reset(session, pIn);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_Close){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);

    TRACE_FUNC1(MFXAudioDECODE_Close, session);
    mfxRes = MFXAudioDECODE_Close(session);
    TRACE_PAR(mfxRes);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_GetAudioParam){
    mfxStatus&      mfxRes      = var_def<mfxStatus>    ("mfxRes",      MFX_ERR_NONE);
    mfxStatus&      expectedRes = var_def<mfxStatus>    ("expectedRes", MFX_ERR_NONE);
    mfxSession&     session     = var_def<mfxSession>   ("session",     0);
    mfxAudioParam*& pIn         = var_def<mfxAudioParam*>("p_mfxap_in", NULL);

    TRACE_FUNC2(MFXAudioDECODE_GetAudioParam, session, pIn);
    mfxRes = MFXAudioDECODE_GetAudioParam(session, pIn);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pIn);
    CHECK_STS(mfxRes, expectedRes);

    return msdk_ts::resOK;
}

msdk_ts_BLOCK(b_MFXAudioDECODE_DecodeFrameAsync){
    mfxStatus&          mfxRes      = var_def<mfxStatus>        ("mfxRes",      MFX_ERR_NONE);
    mfxSession&         session     = var_def<mfxSession>       ("session",     0);
    mfxBitstream*&      pBsIn       = var_def<mfxBitstream*>    ("p_bs_in",    NULL);
    mfxBitstream*&      pBsOut      = var_def<mfxBitstream*>    ("p_bs_out",    NULL);
    mfxSyncPoint&       syncp       = var_def<mfxSyncPoint>     ("syncp",       NULL);
    mfxSyncPoint*&      pSyncp      = var_def<mfxSyncPoint*>    ("p_syncp",     &syncp);

    TRACE_FUNC4(MFXAudioDECODE_DecodeFrameAsync, session, pBsIn, pBsOut, pSyncp);
    assert(!"Need to update beh test!!");
    //mfxRes = MFXAudioDECODE_DecodeFrameAsync(session, pBsIn, pBsOut, pSyncp);
    TRACE_PAR(mfxRes);
    TRACE_PAR(pBsIn);
    TRACE_PAR(pBsOut);
    TRACE_PAR(pSyncp);

    return msdk_ts::resOK;
}

#endif //#ifdef __MFXAUDIO_H__