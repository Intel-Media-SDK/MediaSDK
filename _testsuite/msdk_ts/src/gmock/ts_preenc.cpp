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

#include "ts_preenc.h"

tsVideoPreENC::tsVideoPreENC(bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pRequest(&m_request)
    , m_pSurf(0)
    , m_filler(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_PreENCInput{0}
    , m_PreENCOutput{0}
    , m_pPreENCInput(&m_PreENCInput)
    , m_pPreENCOutput(&m_PreENCOutput)
    , m_field_processed(0)
{
    if (m_default)
    {
        mfxExtFeiParam& extbuffer = m_par;
        extbuffer.Func = MFX_FEI_FUNCTION_PREENC;

        m_par.mfx.CodecId = MFX_CODEC_AVC;
        m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 576;
        m_par.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;
        m_par.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }
    m_loaded = true;
}

tsVideoPreENC::tsVideoPreENC(mfxFeiFunction func, mfxU32 CodecId, bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pRequest(&m_request)
    , m_pSurf(0)
    , m_pSyncPoint(&m_syncpoint)
    , m_filler(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_PreENCInput{0}
    , m_PreENCOutput{0}
    , m_pPreENCInput(&m_PreENCInput)
    , m_pPreENCOutput(&m_PreENCOutput)
    , m_field_processed(0)
{
    if(m_default)
    {
        m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 576;
        m_par.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;
        m_par.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    mfxExtFeiParam& extbuffer = m_par;
    extbuffer.Func = func;
    m_par.mfx.CodecId = CodecId;

    m_loaded = true;
}

tsVideoPreENC::~tsVideoPreENC()
{
    if (m_initialized)
    {
        Close();
    }
}

mfxStatus tsVideoPreENC::Init()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();TS_CHECK_MFX;
        }
        if(!m_loaded)
        {
            Load();
        }
        if(     !m_pFrameAllocator
            && (   (m_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
                || (m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        {
            m_pFrameAllocator = m_pVAHandle;

            if(!GetAllocator())
            {
                if (m_pFrameAllocator)
                {
                    SetAllocator(m_pFrameAllocator, true);
                }
                else
                {
                    UseDefaultAllocator(false);
                    m_pFrameAllocator = GetAllocator();
                }
            }

            SetFrameAllocator();TS_CHECK_MFX;
        }
        if(m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            QueryIOSurf();
            AllocOpaque(m_request, m_par);
        }
    }

    sts = Init(m_session, m_pPar);
    if(MFX_ERR_NONE == sts)
        m_par.AllocId = (mfxU64)&(m_session) & 0xffffffff;

    return sts;
}

mfxStatus tsVideoPreENC::Init(mfxSession session, mfxVideoParam *par)
{
    mfxVideoParam orig_par;
    if (par) memcpy(&orig_par, m_pPar, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoENC_Init, session, par);
    g_tsStatus.check( MFXVideoENC_Init(session, par) );

    m_initialized = (g_tsStatus.get() >= 0);

    if (par)
    {
        if (g_tsConfig.lowpower != MFX_CODINGOPTION_UNKNOWN)
        {
            EXPECT_EQ(g_tsConfig.lowpower, par->mfx.LowPower)
                << "ERROR: external configuration of LowPower doesn't equal to real value\n";
        }
        EXPECT_EQ(0, memcmp(&orig_par, m_pPar, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Init()";
    }

    return g_tsStatus.get();
}

mfxStatus tsVideoPreENC::Close()
{
    return Close(m_session);
}

mfxStatus tsVideoPreENC::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoENC_Close, session);
    g_tsStatus.check(MFXVideoENC_Close(session));

    m_initialized = false;

    return g_tsStatus.get();
}

mfxStatus tsVideoPreENC::Query()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
        if(!m_loaded)
        {
            Load();
        }
    }
    return Query(m_session, m_pPar, m_pParOut);
}

mfxStatus tsVideoPreENC::Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    TRACE_FUNC3(MFXVideoENC_Query, session, in, out);
    g_tsStatus.check( MFXVideoENC_Query(session, in, out) );
    TS_TRACE(out);

    return g_tsStatus.get();
}

mfxStatus tsVideoPreENC::QueryIOSurf()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
        if(!m_loaded)
        {
            Load();
        }
    }
    return QueryIOSurf(m_session, m_pPar, m_pRequest);
}

mfxStatus tsVideoPreENC::QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{

    TRACE_FUNC3(MFXVideoENC_QueryIOSurf, session, par, request);
    g_tsStatus.check( MFXVideoENC_QueryIOSurf(session, par, request) );
    TS_TRACE(request);

    return g_tsStatus.get();
}

mfxStatus tsVideoPreENC::Reset()
{
    return Reset(m_session, m_pPar);
}

mfxStatus tsVideoPreENC::Reset(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoENC_Reset, session, par);
    g_tsStatus.check( MFXVideoENC_Reset(session, par) );

    //m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoPreENC::ProcessFrameAsync()
{
    if(m_default)
    {
        if(!PoolSize())
        {
            if(m_pFrameAllocator && !GetAllocator())
            {
                SetAllocator(m_pFrameAllocator, true);
            }
            AllocSurfaces();TS_CHECK_MFX;
            if(!m_pFrameAllocator && (m_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)))
            {
                m_pFrameAllocator = GetAllocator();
                SetFrameAllocator();TS_CHECK_MFX;
            }
        }
        if(!m_initialized)
        {
            Init();TS_CHECK_MFX;
        }
        m_pPreENCInput->InSurface = GetSurface();TS_CHECK_MFX;
        if(m_filler)
        {
            m_pPreENCInput->InSurface = m_filler->ProcessSurface(m_pPreENCInput->InSurface, m_pFrameAllocator);
        }
    }
    mfxStatus mfxRes = ProcessFrameAsync(m_session, m_pPreENCInput, m_pPreENCOutput, m_pSyncPoint);

    return mfxRes;
}

mfxStatus tsVideoPreENC::ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp)
{
    TRACE_FUNC4(MFXVideoENC_ProcessFrameAsync, session, in, out, syncp);
    mfxStatus mfxRes = MFXVideoENC_ProcessFrameAsync(session, in, out, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(in);
    TS_TRACE(out);
    TS_TRACE(syncp);

    m_frames_buffered += (mfxRes >= 0);

    return g_tsStatus.m_status = mfxRes;
}


mfxStatus tsVideoPreENC::SyncOperation()
{
    return SyncOperation(m_syncpoint);
}

mfxStatus tsVideoPreENC::SyncOperation(mfxSyncPoint syncp)
{
    mfxU32 nFrames = m_frames_buffered;
    mfxStatus res = SyncOperation(m_session, syncp, MFX_INFINITE);

    return g_tsStatus.m_status = res;
}

mfxStatus tsVideoPreENC::SyncOperation(mfxSession session,  mfxSyncPoint syncp, mfxU32 wait)
{
    m_frames_buffered = 0;
    return tsSession::SyncOperation(session, syncp, wait);
}

mfxStatus tsVideoPreENC::AllocSurfaces()
{
    if(m_default && !m_request.NumFrameMin)
    {
        QueryIOSurf();TS_CHECK_MFX;
    }
    return tsSurfacePool::AllocSurfaces(m_request);
}
