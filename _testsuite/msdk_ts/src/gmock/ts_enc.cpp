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

#include "ts_enc.h"

tsVideoENC::tsVideoENC(mfxU32 CodecId, bool useDefaults, tsVideoEncoder* enc)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pRequest(&m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_filler(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_ENCInput(0)
    , m_ENCOutput(0)
    , m_pEncode(0)
{
    m_par.mfx.CodecId = CodecId;
    if(m_default)
    {
        m_par.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;

        if (CodecId == MFX_CODEC_VP8)
            m_par.mfx.QPB = 0;

        m_par.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 480;
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_par.IOPattern        = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        m_par.mfx.EncodedOrder = 1;
        m_par.AsyncDepth       = 1;
    }

    if (enc) m_pEncode = enc;

    m_uid = g_tsPlugin.UID(MFX_PLUGINTYPE_VIDEO_ENCODE, CodecId);
    m_loaded = !m_uid;
}

tsVideoENC::tsVideoENC(mfxFeiFunction func, mfxU32 CodecId, bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pRequest(&m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_filler(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_ENCInput(0)
    , m_ENCOutput(0)
    , m_pEncode(0)
{
    if(m_default)
    {
        m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 576;
        m_par.mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
        m_par.mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;
        m_par.IOPattern        = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        m_par.mfx.EncodedOrder = 1;
        m_par.AsyncDepth       = 1;
    }

    mfxExtFeiParam& extbuffer = m_par;
    extbuffer.Func = func;
    m_par.mfx.CodecId = CodecId;

    mfxExtFeiSPS& extsps = m_par;
    extsps.Log2MaxPicOrderCntLsb = 4;

    m_loaded = true;
}

tsVideoENC::~tsVideoENC()
{
    if(m_initialized)
    {
        Close();
    }
}

mfxStatus tsVideoENC::Init()
{
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
            if(!GetAllocator())
            {
                UseDefaultAllocator(false);
            }
            m_pFrameAllocator = GetAllocator();
            SetFrameAllocator();TS_CHECK_MFX;
        }
        if(m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            QueryIOSurf();
            AllocOpaque(m_request, m_par);
        }
    }
    return Init(m_session, m_pPar);
}

mfxStatus tsVideoENC::Init(mfxSession session, mfxVideoParam *par)
{
    mfxVideoParam orig_par;
    if (par)
        memcpy(&orig_par, par, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoENC_Init, session, par);
    g_tsStatus.check( MFXVideoENC_Init(session, par) );

    m_initialized = (g_tsStatus.get() >= 0);

    if (par)
        EXPECT_EQ(0, memcmp(&orig_par, par, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Init()";

    return g_tsStatus.get();
}

mfxStatus tsVideoENC::Close()
{
    return Close(m_session);
}

mfxStatus tsVideoENC::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoENC_Close, session);
    g_tsStatus.check( MFXVideoENC_Close(session) );

    m_initialized = false;
    m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoENC::Query()
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

mfxStatus tsVideoENC::Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    TRACE_FUNC3(MFXVideoENC_Query, session, in, out);
    g_tsStatus.check( MFXVideoENC_Query(session, in, out) );
    TS_TRACE(out);

    return g_tsStatus.get();
}

mfxStatus tsVideoENC::QueryIOSurf()
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

mfxStatus tsVideoENC::QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    TRACE_FUNC3(MFXVideoENC_QueryIOSurf, session, par, request);
    g_tsStatus.check( MFXVideoENC_QueryIOSurf(session, par, request) );
    TS_TRACE(request);

    return g_tsStatus.get();
}

mfxStatus tsVideoENC::Reset()
{
    return Reset(m_session, m_pPar);
}

mfxStatus tsVideoENC::Reset(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoENC_Reset, session, par);
    g_tsStatus.check( MFXVideoENC_Reset(session, par) );

    //m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoENC::ProcessFrameAsync()
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
        m_ENCInput->InSurface = GetSurface();TS_CHECK_MFX;
        if(m_filler)
        {
            m_ENCInput->InSurface = m_filler->ProcessSurface(m_ENCInput->InSurface, m_pFrameAllocator);
        }
    }
    mfxStatus mfxRes = ProcessFrameAsync(m_session, m_ENCInput, m_ENCOutput, m_pSyncPoint);

    return mfxRes;
}

mfxStatus tsVideoENC::ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp)
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


mfxStatus tsVideoENC::SyncOperation()
{
    return SyncOperation(m_syncpoint);
}

mfxStatus tsVideoENC::SyncOperation(mfxSyncPoint syncp)
{
    mfxU32 nFrames = m_frames_buffered;
    mfxStatus res = SyncOperation(m_session, syncp, MFX_INFINITE);

    return g_tsStatus.m_status = res;
}

mfxStatus tsVideoENC::SyncOperation(mfxSession session,  mfxSyncPoint syncp, mfxU32 wait)
{
    m_frames_buffered = 0;
    return tsSession::SyncOperation(session, syncp, wait);
}

mfxStatus tsVideoENC::AllocSurfaces()
{
    if(m_default && !m_request.NumFrameMin)
    {
        QueryIOSurf();TS_CHECK_MFX;
    }
    return tsSurfacePool::AllocSurfaces(m_request);
}

mfxStatus tsVideoENC::Encode1N(mfxU32 n)
{
    mfxU32 processed = 0;
    mfxU32 encoded = 0;
    mfxU32 submitted = 0;
    mfxU32 async = TS_MAX(1, m_par.AsyncDepth);
    mfxSyncPoint sp;

    async = TS_MIN(n, async - 1);

    while(encoded < n)
    {
        if(MFX_ERR_MORE_DATA == ProcessFrameAsync())
        {
            if(!m_ENCInput->InSurface)
            {
                if(submitted)
                {
                    processed += submitted;
                    SyncOperation(sp);
                }
                break;
            }
            continue;
        }

        if(MFX_ERR_MORE_DATA == m_pEncode->EncodeFrameAsync())
        {
            if(!m_pEncode->m_pSurf)
            {
                if(submitted)
                {
                    encoded += submitted;
                    m_pEncode->SyncOperation(sp);
                }
                break;
            }
            continue;
        }

        g_tsStatus.check();TS_CHECK_MFX;
        sp = m_syncpoint;

        if(++submitted >= async)
        {
            SyncOperation();TS_CHECK_MFX;
            encoded += submitted;
            processed += submitted;
            submitted = 0;
            async = TS_MIN(async, (n - encoded));
        }
    }

    g_tsLog << processed << '/' << encoded << " FRAMES Processed/Encoded\n";

    return g_tsStatus.get();
}
