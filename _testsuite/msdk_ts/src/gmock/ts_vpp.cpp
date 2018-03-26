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

#include "ts_vpp.h"




tsVideoVPP::tsVideoVPP(bool useDefaults, mfxU32 plugin_id)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_stat()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pRequest(m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_pSurfIn(0)
    , m_pSurfOut(0)
    , m_pSurfWork(0)
    , m_pStat(&m_stat)
    , m_pSurfPoolIn(&m_spool_in)
    , m_pSurfPoolOut(&m_spool_out)
    , m_surf_in_processor(0)
    , m_surf_out_processor(0)
{
    memset(m_request, 0, sizeof(m_request));

    if(m_default)
    {
        m_par.vpp.In.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.vpp.In.Height = m_par.mfx.FrameInfo.CropH = 480;
        m_par.vpp.In.FourCC          = MFX_FOURCC_NV12;
        m_par.vpp.In.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;
        m_par.vpp.In.PicStruct       = MFX_PICSTRUCT_PROGRESSIVE;
        m_par.vpp.In.FrameRateExtN   = 30;
        m_par.vpp.In.FrameRateExtD   = 1;
        m_par.vpp.Out = m_par.vpp.In;
        m_par.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }
    if(plugin_id)
    {
        m_uid = g_tsPlugin.UID(MFX_PLUGINTYPE_VIDEO_VPP, plugin_id);
        m_loaded = !m_uid;
        if(m_default && (plugin_id == MFX_MAKEFOURCC('C','A','M','R')))
        {
            m_par.vpp.In.FourCC        = MFX_FOURCC_R16;
            m_par.vpp.Out.FourCC       = MFX_FOURCC_RGB4;
            m_par.vpp.In.ChromaFormat  = MFX_CHROMAFORMAT_MONOCHROME;
            m_par.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
            mfxExtCamPipeControl& cam_ctrl = m_par; //accordingly to ashapore, this filter is mandatory
            cam_ctrl.RawFormat         = MFX_CAM_BAYER_BGGR;

            if(g_tsImpl == MFX_IMPL_HARDWARE)
            {
                if(g_tsHWtype < MFX_HW_HSW)
                    m_sw_fallback = true;
            }
        }
        if(m_default && (plugin_id == MFX_MAKEFOURCC('P','T','I','R')))
        {
            // configure for MFX_DEINTERLACING_AUTO_SINGLE
            m_par.vpp.In.PicStruct = MFX_PICSTRUCT_UNKNOWN;
            m_par.vpp.In.FrameRateExtN = 0;
            m_par.vpp.In.FrameRateExtD = 1;
            m_par.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            m_par.vpp.Out.FrameRateExtN = 30;
            m_par.vpp.Out.FrameRateExtD = 1;
            if(g_tsImpl == MFX_IMPL_HARDWARE)
            {
                if(g_tsHWtype < MFX_HW_IVB)
                    m_sw_fallback = true;
            }
        }
    }
}

tsVideoVPP::~tsVideoVPP()
{
    if(m_initialized)
    {
        Close();
    }
}


mfxStatus tsVideoVPP::Init()
{
    bool set_allocator = false;

    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }

        if(!m_pSurfPoolIn)
            m_pSurfPoolIn = &m_spool_in;
        if(!m_pSurfPoolOut)
            m_pSurfPoolOut = &m_spool_out;

        if(    (m_request[0].Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
            || (m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY))
        {
            if(!m_pSurfPoolIn->GetAllocator())
            {
                if(m_pFrameAllocator)
                    m_pSurfPoolIn->SetAllocator(m_pFrameAllocator, true);
                else if(m_pSurfPoolOut->GetAllocator())
                    m_pSurfPoolIn->SetAllocator(m_pSurfPoolOut->GetAllocator(), true);
                else
                    m_pSurfPoolIn->UseDefaultAllocator(false);
            }

            if(!m_pFrameAllocator)
            {
                m_pFrameAllocator = m_pSurfPoolIn->GetAllocator();
                set_allocator = true;
            }
        }

        if(   (m_request[1].Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
           || (m_par.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        {
            if(!m_pSurfPoolOut->GetAllocator())
            {
                if(m_pFrameAllocator)
                    m_pSurfPoolOut->SetAllocator(m_pFrameAllocator, true);
                else if(m_pSurfPoolIn->GetAllocator())
                    m_pSurfPoolOut->SetAllocator(m_pSurfPoolIn->GetAllocator(), true);
                else
                    m_pSurfPoolOut->UseDefaultAllocator(false);
            }

            if(!m_pFrameAllocator)
            {
                m_pFrameAllocator = m_pSurfPoolOut->GetAllocator();
                set_allocator = true;
            }
        }

        if(set_allocator)
            SetFrameAllocator();

        if(m_par.IOPattern & (MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
            QueryIOSurf();

        if(m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            m_pSurfPoolIn->AllocOpaque(m_request[0], m_par);

        if(m_par.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
            m_pSurfPoolOut->AllocOpaque(m_request[1], m_par);

    }
    return Init(m_session, m_pPar);
}

mfxStatus tsVideoVPP::Init(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoVPP_Init, session, par);
    IS_FALLBACK_EXPECTED(m_sw_fallback, g_tsStatus);
    g_tsStatus.check( MFXVideoVPP_Init(session, par) );

    m_initialized = (g_tsStatus.get() >= 0);

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::Close(bool check)
{
    mfxStatus sts = Close(m_session);

    if (!check)
        return sts;

    // check locked counters is disabled by default
    mfxStatus locked_sts = m_pSurfPoolIn->CheckLockedCounter();
    if (locked_sts != MFX_ERR_NONE)
    {
        g_tsLog << "ERROR: there is Locked IN surface after Close()\n";
        g_tsStatus.check(MFX_ERR_ABORTED); TS_CHECK_MFX;
    }

    locked_sts = m_pSurfPoolOut->CheckLockedCounter();
    if (locked_sts != MFX_ERR_NONE)
    {
        g_tsLog << "ERROR: there is Locked OUT surface after Close()\n";
        g_tsStatus.check(MFX_ERR_ABORTED); TS_CHECK_MFX;
    }

    return sts;
}

mfxStatus tsVideoVPP::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoVPP_Close, session);
    g_tsStatus.check( MFXVideoVPP_Close(session) );

    m_initialized = false;

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::Query()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
    }
    return Query(m_session, m_pPar, m_pParOut);
}

mfxStatus tsVideoVPP::Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    TRACE_FUNC3(MFXVideoVPP_Query, session, in, out);
    IS_FALLBACK_EXPECTED(m_sw_fallback, g_tsStatus);
    g_tsStatus.check( MFXVideoVPP_Query(session, in, out) );
    TS_TRACE(out);

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::QueryIOSurf()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
    }
    return QueryIOSurf(m_session, m_pPar, m_pRequest);
}

mfxStatus tsVideoVPP::QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    TRACE_FUNC3(MFXVideoVPP_QueryIOSurf, session, par, request);
    IS_FALLBACK_EXPECTED(m_sw_fallback, g_tsStatus);
    g_tsStatus.check( MFXVideoVPP_QueryIOSurf(session, par, request) );
    TS_TRACE(request);
    if(request)
    {
        TS_TRACE(request[1]);
    }

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::Reset()
{
    return Reset(m_session, m_pPar);
}

mfxStatus tsVideoVPP::Reset(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoVPP_Reset, session, par);
    g_tsStatus.check( MFXVideoVPP_Reset(session, par) );

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::GetVideoParam()
{
    if(m_default && !m_initialized)
    {
        Init();
    }
    return GetVideoParam(m_session, m_pPar);
}

mfxStatus tsVideoVPP::GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoVPP_GetVideoParam, session, par);
    g_tsStatus.check( MFXVideoVPP_GetVideoParam(session, par) );
    TS_TRACE(par);

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::GetVPPStat()
{
    if(m_default && !m_initialized)
    {
        GetVPPStat();
    }
    return GetVPPStat(m_session, m_pStat);
}

mfxStatus tsVideoVPP::GetVPPStat(mfxSession session, mfxVPPStat *stat)
{
    TRACE_FUNC2(MFXVideoVPP_GetVPPStat, session, stat);
    g_tsStatus.check( MFXVideoVPP_GetVPPStat(session, stat) );
    TS_TRACE(stat);

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::AllocSurfaces()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_default && !m_request[0].NumFrameMin)
    {
        QueryIOSurf();
    }

    if(!m_pSurfPoolIn || !m_pSurfPoolOut)
    {
        g_tsStatus.check(sts = MFX_ERR_NULL_PTR);
        return sts;
    }

    sts = m_pSurfPoolIn->AllocSurfaces(m_request[0]);
    if(sts)
        return sts;

    return m_pSurfPoolOut->AllocSurfaces(m_request[1]);
}

mfxStatus tsVideoVPP::RunFrameVPPAsync()
{
    if(m_default)
    {
        if(!m_initialized)
        {
            Init();
        }

        if(!m_pSurfPoolIn->PoolSize() || !m_pSurfPoolOut->PoolSize())
        {
            AllocSurfaces();
        }

        m_pSurfIn  = m_pSurfPoolIn->GetSurface();
        m_pSurfOut = m_pSurfPoolOut->GetSurface();

        if(m_surf_in_processor)
        {
            m_pSurfIn = m_surf_in_processor->ProcessSurface(m_pSurfIn, m_pFrameAllocator);
        }
    }

    RunFrameVPPAsync(m_session, m_pSurfIn, m_pSurfOut, 0, m_pSyncPoint);

    if(g_tsStatus.get() == 0)
    {
        m_surf_out.insert( std::make_pair(*m_pSyncPoint, m_pSurfOut) );
        if(m_pSurfOut)
        {
            msdk_atomic_inc16(&m_pSurfOut->Data.Locked);
        }
    }

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::RunFrameVPPAsync(
    mfxSession  session,
    mfxFrameSurface1 *in,
    mfxFrameSurface1 *out,
    mfxExtVppAuxData *aux,
    mfxSyncPoint *syncp)
{
    TRACE_FUNC5(MFXVideoVPP_RunFrameVPPAsync, session, in, out, aux, syncp);
    mfxStatus mfxRes = MFXVideoVPP_RunFrameVPPAsync(session, in, out, aux, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(out);
    TS_TRACE(syncp);

    return g_tsStatus.m_status = mfxRes;
}

mfxStatus tsVideoVPP::SyncOperation()
{
    if(!m_surf_out.size())
    {
        g_tsStatus.check(MFX_ERR_UNKNOWN);
    }
    return SyncOperation(m_surf_out.begin()->first);
}

mfxStatus tsVideoVPP::SyncOperation(mfxSyncPoint syncp)
{
    mfxFrameSurface1* pS = m_surf_out[syncp];
    mfxStatus res = SyncOperation(m_session, syncp, MFX_INFINITE);

    if(m_default && pS && pS->Data.Locked)
    {

        msdk_atomic_dec16(&pS->Data.Locked);
    }

    if (m_default && m_surf_out_processor && g_tsStatus.get() == MFX_ERR_NONE)
    {
        m_surf_out_processor->ProcessSurface(pS, m_pFrameAllocator);
    }

    return g_tsStatus.m_status = res;
}

mfxStatus tsVideoVPP::SyncOperation(mfxSession session,  mfxSyncPoint syncp, mfxU32 wait)
{
    m_surf_out.erase(syncp);
    return tsSession::SyncOperation(session, syncp, wait);
}


mfxStatus tsVideoVPP::ProcessFrames(mfxU32 n)
{
    mfxU32 processed = 0;
    mfxU32 submitted = 0;
    mfxU32 async = TS_MAX(1, m_par.AsyncDepth);
    mfxStatus res = MFX_ERR_NONE;

    async = TS_MIN(n, async - 1);

    while(processed < n)
    {
        res = RunFrameVPPAsync();

        if(MFX_ERR_MORE_DATA == res)
        {
            if(!m_pSurfIn)
            {
                if(submitted)
                {
                    processed += submitted;

                    while(m_surf_out.size()) SyncOperation();
                }
                break;
            }
            continue;
        }

        if(MFX_ERR_MORE_SURFACE == res || res > 0)
        {
            continue;
        }

        if(res < 0) g_tsStatus.check();

        if(++submitted >= async)
        {
            while(m_surf_out.size()) SyncOperation();
            processed += submitted;
            submitted = 0;
            async = TS_MIN(async, (n - processed));
        }
    }

    g_tsLog << processed << " FRAMES PROCESSED\n";

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::RunFrameVPPAsyncEx(
    mfxSession session,
    mfxFrameSurface1 *in,
    mfxFrameSurface1 *surface_work,
    mfxFrameSurface1 **surface_out,
    mfxSyncPoint *syncp)
{
    TRACE_FUNC5(MFXVideoVPP_RunFrameVPPAsyncEx, session, in, surface_work, surface_out, syncp);
    mfxStatus mfxRes = MFXVideoVPP_RunFrameVPPAsyncEx(session, in, surface_work, surface_out, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(surface_work);
    TS_TRACE(surface_out);
    TS_TRACE(syncp);

    return g_tsStatus.m_status = mfxRes;
}

mfxStatus tsVideoVPP::RunFrameVPPAsyncEx()
{
    if(m_default)
    {
        if(!m_initialized)
        {
            Init();
        }

        if(!m_pSurfPoolIn->PoolSize() || !m_pSurfPoolOut->PoolSize())
        {
            AllocSurfaces();
        }

        m_pSurfIn  = m_pSurfPoolIn->GetSurface();
        m_pSurfWork = m_pSurfPoolOut->GetSurface();

        if(m_surf_in_processor)
        {
            m_pSurfIn = m_surf_in_processor->ProcessSurface(m_pSurfIn, m_pFrameAllocator);
        }
    }

    RunFrameVPPAsyncEx(m_session, m_pSurfIn, m_pSurfWork, &m_pSurfOut, m_pSyncPoint);

    if(g_tsStatus.get() == 0)
    {
        m_surf_out.insert( std::make_pair(*m_pSyncPoint, m_pSurfOut) );
        if(m_pSurfOut)
        {
            msdk_atomic_inc16(&m_pSurfOut->Data.Locked);
        }
    }

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::ProcessFramesEx(mfxU32 n)
{
    mfxU32 processed = 0;
    mfxU32 submitted = 0;
    mfxU32 async = TS_MAX(1, m_par.AsyncDepth);
    mfxStatus res = MFX_ERR_NONE;

    async = TS_MIN(n, async - 1);

    while(processed < n)
    {
        res = RunFrameVPPAsyncEx();

        if(MFX_ERR_MORE_DATA == res)
        {
            if(!m_pSurfIn)
            {
                if(submitted)
                {
                    processed += submitted;

                    while(m_surf_out.size()) SyncOperation();
                }
                break;
            }
            continue;
        }

        if(MFX_ERR_MORE_SURFACE == res || res > 0)
        {
            continue;
        }

        if(res < 0) g_tsStatus.check();

        if(++submitted >= async)
        {
            while(m_surf_out.size()) SyncOperation();
            processed += submitted;
            submitted = 0;
            async = TS_MIN(async, (n - processed));
        }
    }

    g_tsLog << processed << " FRAMES PROCESSED\n";

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::Load()
{
    if(m_default && !m_session)
    {
        MFXInit();
    }

    m_loaded = (0 == tsSession::Load(m_session, m_uid, 1));

    return g_tsStatus.get();
}

mfxStatus tsVideoVPP::SetHandle()
{
    if (!m_is_handle_set)
    {
        mfxHDL hdl;
        mfxHandleType type;

        bool isInSW = !(!!((m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)));
        bool isOutSW = !(!!(m_par.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY));

        if (!isInSW)
        {
            m_spool_in.GetAllocator()->get_hdl(type, hdl);
            return SetHandle(m_session, type, hdl);
        } else if (!isOutSW)
        {
            m_spool_out.GetAllocator()->get_hdl(type, hdl);
            return SetHandle(m_session, type, hdl);
        }

        return MFX_ERR_NONE;    // do not set handle for SW
    }

    return MFX_ERR_NONE;
}

mfxStatus tsVideoVPP::SetHandle(mfxSession session, mfxHandleType type, mfxHDL handle)
{
    return tsSession::SetHandle(session, type, handle);
}

mfxStatus tsVideoVPP::CreateAllocators()
{
    bool isInSW = !(!!((m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)));
    // VA Handle is set by default for Linux HW
    // see tsSession::MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
    if (m_pVAHandle && !isInSW)
    {
        m_spool_in.SetAllocator(m_pVAHandle, true);
    } else
    {
        m_spool_in.UseDefaultAllocator(isInSW);
    }

    bool isOutSW = !(!!(m_par.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY));
    if (!isInSW && !isOutSW)    // reuse device
    {
        m_spool_out.SetAllocator(m_spool_in.GetAllocator(), true);
    } else
    {
        if (m_pVAHandle && !isOutSW)
        {
            m_spool_out.SetAllocator(m_pVAHandle, true);
        } else
        {
            m_spool_out.UseDefaultAllocator(isOutSW);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus tsVideoVPP::SetFrameAllocator()
{
    bool isInSW = !(!!((m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)));
    bool isOutSW = !(!!(m_par.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY));

    if (!isInSW)
    {
        return SetFrameAllocator(m_session, m_spool_in.GetAllocator());
    } else if (!isOutSW)
    {
        return SetFrameAllocator(m_session, m_spool_out.GetAllocator());
    }

    return MFX_ERR_NONE;    // do not set allocator for SW
}

mfxStatus tsVideoVPP::SetFrameAllocator(mfxSession session, mfxFrameAllocator* allocator)
{
    return tsSession::SetFrameAllocator(session, allocator);
}

const tsVPPInfo::CFormat tsVPPInfo::Formats[tsVPPInfo::NumFormats] =
{
    /*00*/{ MFX_FOURCC_NV12, MFX_CHROMAFORMAT_YUV420,  8,  8, 0 },
    /*01*/{ MFX_FOURCC_YV12, MFX_CHROMAFORMAT_YUV420,  8,  8, 0 },
    /*02*/{ MFX_FOURCC_UYVY, MFX_CHROMAFORMAT_YUV422,  8,  8, 0 },
    /*03*/{ MFX_FOURCC_YUY2, MFX_CHROMAFORMAT_YUV422,  8,  8, 0 },
    /*04*/{ MFX_FOURCC_AYUV, MFX_CHROMAFORMAT_YUV444,  8,  8, 0 },
    /*05*/{ MFX_FOURCC_RGB4, MFX_CHROMAFORMAT_YUV444,  8,  8, 0 },
    /*06*/{ MFX_FOURCC_P010, MFX_CHROMAFORMAT_YUV420, 10, 10, 1 },
    /*07*/{ MFX_FOURCC_Y210, MFX_CHROMAFORMAT_YUV422, 10, 10, 1 },
    /*08*/{ MFX_FOURCC_Y410, MFX_CHROMAFORMAT_YUV444, 10, 10, 0 },
    /*09*/{ MFX_FOURCC_A2RGB10, MFX_CHROMAFORMAT_YUV444, 10, 10, 0 },
    /*10*/{ MFX_FOURCC_P016, MFX_CHROMAFORMAT_YUV420, 12, 12, 1 },
    /*11*/{ MFX_FOURCC_Y216, MFX_CHROMAFORMAT_YUV422, 12, 12, 1 },
    /*12*/{ MFX_FOURCC_Y416, MFX_CHROMAFORMAT_YUV444, 12, 12, 1 },
};

const mfxStatus HW = MFX_ERR_NONE;
const mfxStatus SW = MFX_WRN_PARTIAL_ACCELERATION;
const mfxStatus NO = MFX_ERR_UNSUPPORTED;

const tsVPPInfo::TCCSupport tsVPPInfo::CCSupportTable[3] =
{
    {//gen9
    //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB4  P010  Y210  Y410  A2RGB10 P016/12 Y216/12 Y416/12
    /*   NV12*/{  HW,   NO,   NO,   HW,   NO,   HW,   HW,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   YV12*/{  HW,   NO,   NO,   HW,   NO,   HW,   HW,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   UYVY*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   YUY2*/{  HW,   NO,   NO,   HW,   NO,   HW,   HW,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   AYUV*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   RGB4*/{  HW,   NO,   NO,   HW,   NO,   HW,   HW,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   P010*/{  HW,   NO,   NO,   NO,   NO,   HW,   HW,   NO,   NO,      HW,    NO,     NO,     NO },
    /*   Y210*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   Y410*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*A2RGB10*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*P016/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*Y216/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*Y416/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    },
    {//gen11
    //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB4  P010  Y210  Y410  A2RGB10 P016/12 Y216/12 Y416/12
    /*   NV12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   YV12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    NO,     NO,     NO },
    /*   UYVY*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   YUY2*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   AYUV*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   RGB4*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   P010*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   Y210*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*   Y410*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    NO,     NO,     NO },
    /*A2RGB10*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*P016/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*Y216/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*Y416/12*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    },
    {//gen12
    //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB4  P010  Y210  Y410  A2RGB10 P016/12 Y216/12 Y416/12
    /*   NV12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    HW,     HW,     HW },
    /*   YV12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    HW,     HW,     HW },
    /*   UYVY*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*   YUY2*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    HW,     HW,     HW },
    /*   AYUV*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    HW,     HW,     HW },
    /*   RGB4*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      NO,    HW,     HW,     HW },
    /*   P010*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    /*   Y210*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    /*   Y410*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    /*A2RGB10*/{  NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,   NO,      NO,    NO,     NO,     NO },
    /*P016/12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    /*Y216/12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    /*Y416/12*/{  HW,   NO,   NO,   HW,   HW,   HW,   HW,   HW,   HW,      HW,    HW,     HW,     HW },
    },
};
