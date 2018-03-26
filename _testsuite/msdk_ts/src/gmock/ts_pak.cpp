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

#include "ts_pak.h"

tsVideoPAK::tsVideoPAK(mfxFeiFunction func, mfxU32 CodecId, bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_bitstream()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pBitstream(&m_bitstream)
    , m_pRequest(m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_filler(0)
    , m_frames_buffered(0)
    , m_uid(0)
{
    memset(m_request, 0, sizeof(m_request));

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

tsVideoPAK::~tsVideoPAK()
{
    if(m_initialized)
    {
        Close();
    }
}

mfxStatus tsVideoPAK::Init()
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
            && (   (m_request[0].Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
                || (m_request[1].Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
                || (m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        {
            if(!GetAllocator())
            {
                UseDefaultAllocator(false);
            }
            m_pFrameAllocator = GetAllocator();
            SetFrameAllocator();TS_CHECK_MFX;
        }
    }
    return Init(m_session, m_pPar);
}

mfxStatus tsVideoPAK::Init(mfxSession session, mfxVideoParam *par)
{
    mfxVideoParam orig_par;
    if (par)
        memcpy(&orig_par, par, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoPAK_Init, session, par);
    g_tsStatus.check( MFXVideoPAK_Init(session, par) );

    m_initialized = (g_tsStatus.get() >= 0);

    if (par)
        EXPECT_EQ(0, memcmp(&orig_par, par, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Init()";

    return g_tsStatus.get();
}

mfxStatus tsVideoPAK::Close()
{
    return Close(m_session);
}

mfxStatus tsVideoPAK::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoPAK_Close, session);
    g_tsStatus.check( MFXVideoPAK_Close(session) );

    m_initialized = false;
    m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoPAK::Query()
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

mfxStatus tsVideoPAK::Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    TRACE_FUNC3(MFXVideoPAK_Query, session, in, out);
    g_tsStatus.check( MFXVideoPAK_Query(session, in, out) );
    TS_TRACE(out);

    return g_tsStatus.get();
}

mfxStatus tsVideoPAK::QueryIOSurf()
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

mfxStatus tsVideoPAK::QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    TRACE_FUNC3(MFXVideoPAK_QueryIOSurf, session, par, request);
    g_tsStatus.check( MFXVideoPAK_QueryIOSurf(session, par, request) );
    TS_TRACE(request);

    return g_tsStatus.get();
}

mfxStatus tsVideoPAK::AllocSurfaces()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_default && (!m_request[0].NumFrameMin || !m_request[1].NumFrameMin))
    {
        QueryIOSurf();TS_CHECK_MFX;
    }

    sts = tsSurfacePool::AllocSurfaces(m_request[0]);
    if (sts)
    {
        return sts;
    }

    return tsSurfacePool::AllocSurfaces(m_request[1]);
}

mfxStatus tsVideoPAK::Reset()
{
    return Reset(m_session, m_pPar);
}

mfxStatus tsVideoPAK::Reset(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoPAK_Reset, session, par);
    g_tsStatus.check( MFXVideoPAK_Reset(session, par) );

    //m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoPAK::GetVideoParam()
{
    if(m_default && !m_initialized)
    {
        Init();TS_CHECK_MFX;
    }
    return GetVideoParam(m_session, m_pPar);
}

mfxStatus tsVideoPAK::GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoPAK_GetVideoParam, session, par);
    g_tsStatus.check( MFXVideoPAK_GetVideoParam(session, par) );
    TS_TRACE(par);

    return g_tsStatus.get();
}
