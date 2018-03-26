/* /////////////////////////////////////////////////////////////////////////////
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
*/

#include "ts_transcoder.h"

tsTranscoder::tsTranscoder(mfxU32 codecDec, mfxU32 codecEnc)
    : tsVideoVPP()
    , tsVideoDecoder(codecDec)
    , tsVideoEncoder(codecEnc)
    , m_parDEC(tsVideoDecoder::m_par)
    , m_parVPP(tsVideoVPP::m_par)
    , m_parENC(tsVideoEncoder::m_par)
    , m_bsProcIn(tsVideoDecoder::m_bs_processor)
    , m_bsProcOut(tsVideoEncoder::m_bs_processor)
{
}

tsTranscoder::~tsTranscoder()
{
}

void tsTranscoder::InitAlloc()
{
    mfxHandleType type;
    mfxHDLPair hdl;

    if(!m_pSurfPoolIn || !m_pSurfPoolOut)
    {
        g_tsStatus.check(MFX_ERR_NULL_PTR);
        return;
    }

    if ((tsVideoVPP::m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY) && !m_pSurfPoolIn->GetAllocator())
    {
        if (m_pVAHandle)
            m_pSurfPoolIn->SetAllocator(m_pVAHandle, true);
        else
        {
            if (m_pFrameAllocator)
                m_pSurfPoolIn->SetAllocator(m_pFrameAllocator, true);
            else
                m_pSurfPoolIn->UseDefaultAllocator();
        }

        if (!m_is_handle_set)
        {
            m_pSurfPoolIn->GetAllocator()->get_hdl(type, hdl.first);
            tsSession::SetHandle(m_session, type, hdl.first);
        }

        if (!m_pFrameAllocator)
        {
            m_pFrameAllocator = m_pSurfPoolIn->GetAllocator();
            tsSession::SetFrameAllocator();
        }
    }

    if ((tsVideoVPP::m_par.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !m_pSurfPoolOut->GetAllocator())
    {
        if (m_pVAHandle)
            m_pSurfPoolOut->SetAllocator(m_pVAHandle, true);
        else
        {
            if (m_pFrameAllocator)
                m_pSurfPoolOut->SetAllocator(m_pFrameAllocator, true);
            else
                m_pSurfPoolOut->UseDefaultAllocator();
        }

        if (!m_is_handle_set)
        {
            m_pSurfPoolOut->GetAllocator()->get_hdl(type, hdl.first);
            tsSession::SetHandle(m_session, type, hdl.first);
        }

        if (!m_pFrameAllocator)
        {
            m_pFrameAllocator = m_pSurfPoolOut->GetAllocator();
            tsSession::SetFrameAllocator();
        }
    }
}

void tsTranscoder::AllocSurfaces()
{
    if(!m_pSurfPoolIn || !m_pSurfPoolOut)
    {
        g_tsStatus.check(MFX_ERR_NULL_PTR);
        return;
    }

    if (!tsVideoVPP::m_request[0].NumFrameMin || !tsVideoVPP::m_request[1].NumFrameMin)
    {
        tsVideoVPP::QueryIOSurf();
    }

    if (!tsVideoDecoder::m_request.NumFrameMin)
    {
        tsVideoDecoder::QueryIOSurf();
        tsVideoDecoder::m_request.NumFrameMin += tsVideoVPP::m_request[0].NumFrameMin - tsVideoVPP::m_par.AsyncDepth + 1;
        tsVideoDecoder::m_request.NumFrameSuggested += tsVideoVPP::m_request[0].NumFrameSuggested - tsVideoVPP::m_par.AsyncDepth + 1;
    }

    if (tsVideoVPP::m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc& opqDEC = tsVideoDecoder::m_par;
        mfxExtOpaqueSurfaceAlloc& opqVPP = tsVideoVPP::m_par;

        m_pSurfPoolIn->AllocOpaque(tsVideoDecoder::m_request, opqDEC);

        opqVPP.In = opqDEC.Out;
    }
    else
        m_pSurfPoolIn->AllocSurfaces(tsVideoDecoder::m_request);
        
    if (!tsVideoEncoder::m_request.NumFrameMin)
    {
        tsVideoEncoder::QueryIOSurf();
        tsVideoEncoder::m_request.NumFrameMin += tsVideoVPP::m_request[1].NumFrameMin - tsVideoVPP::m_par.AsyncDepth + 1;
        tsVideoEncoder::m_request.NumFrameSuggested += tsVideoVPP::m_request[1].NumFrameSuggested - tsVideoVPP::m_par.AsyncDepth + 1;
    }
        
    if (tsVideoVPP::m_par.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc& opqENC = tsVideoEncoder::m_par;
        mfxExtOpaqueSurfaceAlloc& opqVPP = tsVideoVPP::m_par;

        m_pSurfPoolOut->AllocOpaque(tsVideoEncoder::m_request, opqENC);

        opqVPP.Out = opqENC.In;
    }
    else
        m_pSurfPoolOut->AllocSurfaces(tsVideoEncoder::m_request);
        
    //fake pools
    {
        mfxExtOpaqueSurfaceAlloc opq = {};
        tsVideoDecoder::AllocOpaque(tsVideoDecoder::m_request, opq);
        tsVideoEncoder::AllocOpaque(tsVideoEncoder::m_request, opq);
    }
}

void tsTranscoder::InitPipeline(mfxU16 AsyncDepth)
{
    tsVideoDecoder::m_par.AsyncDepth = AsyncDepth;
    tsVideoVPP::m_par.AsyncDepth     = AsyncDepth;
    tsVideoEncoder::m_par.AsyncDepth = AsyncDepth;

    tsVideoVPP::m_par.IOPattern = ((tsVideoDecoder::m_par.IOPattern >> 4) | (tsVideoEncoder::m_par.IOPattern << 4));

    MFXInit();
    InitAlloc();

    if (!tsVideoDecoder::m_par_set)
        tsVideoDecoder::DecodeHeader();

    tsVideoVPP::m_par.vpp.In  = tsVideoDecoder::m_par.mfx.FrameInfo;
    tsVideoVPP::m_par.vpp.Out = tsVideoEncoder::m_par.mfx.FrameInfo;

    if (!(tsVideoVPP::m_par.vpp.In.FrameRateExtD * tsVideoVPP::m_par.vpp.In.FrameRateExtN))
    {
        tsVideoVPP::m_par.vpp.In.FrameRateExtD = tsVideoVPP::m_par.vpp.Out.FrameRateExtD;
        tsVideoVPP::m_par.vpp.In.FrameRateExtN = tsVideoVPP::m_par.vpp.Out.FrameRateExtN;
    }

    AllocSurfaces();

    m_sp_free.resize(AsyncDepth);

    if (!tsVideoEncoder::m_loaded)
        tsVideoEncoder::Load();

    tsVideoDecoder::Init(m_session, &m_parDEC);
    tsVideoVPP::Init(m_session, &m_parVPP);
    tsVideoEncoder::Init(m_session, &m_parENC);

    tsVideoDecoder::GetVideoParam();
    tsVideoVPP::GetVideoParam();
    tsVideoEncoder::GetVideoParam();

    tsVideoEncoder::m_par.AsyncDepth = 1;
    for (auto& sp : m_sp_free)
    {
        tsVideoEncoder::AllocBitstream();
        sp.bs = tsVideoEncoder::m_bitstream;
    }
    tsVideoEncoder::m_par.AsyncDepth = AsyncDepth;
}

bool tsTranscoder::SubmitDecoding()
{
    m_cId = 0;

    for (mfxStatus res = MFX_ERR_UNKNOWN; res < MFX_ERR_NONE || !*tsVideoDecoder::m_pSyncPoint; m_cId = 0)
    {
        SetPar4_DecodeFrameAsync();
        res = DecodeFrameAsync(m_session,
            tsVideoDecoder::m_pBitstream,
            tsVideoDecoder::m_pSurf,
            &(tsVideoDecoder::m_pSurfOut),
            tsVideoDecoder::m_pSyncPoint);

        if (res == MFX_ERR_MORE_DATA)
        {
            if (!tsVideoDecoder::m_pBitstream)
                return false;
        }
        else if(res != MFX_ERR_MORE_SURFACE && res < MFX_ERR_NONE)
        {
            g_tsStatus.check();
            return false;
        }
    }

    return true;
}

bool tsTranscoder::SubmitVPP()
{
    m_cId = 1;

    for (mfxStatus res = MFX_ERR_MORE_DATA; res < MFX_ERR_NONE || !*tsVideoVPP::m_pSyncPoint; m_cId = 1)
    {
        if (res == MFX_ERR_MORE_DATA)
            tsVideoVPP::m_pSurfIn = SubmitDecoding() ? tsVideoDecoder::m_pSurfOut : 0;
        tsVideoVPP::m_pSurfOut = m_pSurfPoolOut->GetSurface();

        res = RunFrameVPPAsync(m_session, tsVideoVPP::m_pSurfIn, tsVideoVPP::m_pSurfOut, 0, tsVideoVPP::m_pSyncPoint);

        if (res == MFX_ERR_MORE_DATA)
        {
            if (!tsVideoVPP::m_pSurfIn)
                return false;
        }
        else if (res != MFX_ERR_MORE_SURFACE && res < MFX_ERR_NONE)
        {
            g_tsStatus.check();
            return false;
        }
    }

    return true;
}

bool tsTranscoder::SubmitEncoding()
{
    m_cId = 2;

    for (mfxStatus res = MFX_ERR_UNKNOWN; res < MFX_ERR_NONE || !*tsVideoEncoder::m_pSyncPoint; m_cId = 2)
    {
        res = EncodeFrameAsync();

        if (res == MFX_ERR_MORE_DATA)
        {
            if (!tsVideoEncoder::m_pSurf)
                return false;
        }
        else if(res < MFX_ERR_NONE)
        {
            g_tsStatus.check();
            return false;
        }
    }

    return true;
}

void tsTranscoder::TranscodeFrames(mfxU32 n)
{
    bool eos = false;
    mfxU32 submit = n;
    mfxU32 sync = n;

    while (sync)
    {
        while (m_sp_free.size() > 0 && !eos && submit)
        {
            tsVideoEncoder::m_pBitstream = &m_sp_free.front().bs;

            if (!SubmitEncoding())
            {
                eos = true;
                break;
            }

            submit--;

            m_sp_free.front().sp = *tsVideoEncoder::m_pSyncPoint;
            m_sp_working.splice(m_sp_working.end(), m_sp_free, m_sp_free.begin());
        }

        if (m_sp_working.size())
        {
            tsVideoEncoder::m_pBitstream      = &m_sp_working.front().bs;
            tsVideoEncoder::m_frames_buffered = 1;
            tsVideoEncoder::SyncOperation(m_sp_working.front().sp);

            tsVideoEncoder::m_pBitstream->DataLength = 0;
            m_sp_free.splice(m_sp_free.end(), m_sp_working, m_sp_working.begin());
            sync--;
        }
        else if (eos)
            break;
    }

    g_tsLog << (n - sync) << " FRAMES TRANSCODED\n";
}

mfxFrameSurface1* tsTranscoder::GetSurface()
{
    switch (m_cId)
    {
    case 0: return m_pSurfPoolIn->GetSurface();
    case 2: return SubmitVPP() ? tsVideoVPP::m_pSurfOut : 0;
    case 1: 
    default: break;
    }
    return 0;
}
