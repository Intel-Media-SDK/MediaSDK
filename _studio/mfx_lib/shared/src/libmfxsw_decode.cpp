// Copyright (c) 2018-2020 Intel Corporation
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

#include <mfxvideo.h>

#include <mfx_session.h>
#include <mfx_tools.h>
#include <mfx_common.h>

// sheduling and threading stuff
#include <mfx_task.h>

#include <libmfx_core.h>

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)
#include "mfx_vc1_decode.h"
#endif

#if defined (MFX_ENABLE_H264_VIDEO_DECODE)
#include "mfx_h264_dec_decode.h"
#endif

#if defined (MFX_ENABLE_H265_VIDEO_DECODE)
#include "mfx_h265_dec_decode.h"
#endif

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)
#include "mfx_mpeg2_decode.h"
#endif

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "mfx_mjpeg_dec_decode.h"
#endif

#if defined (MFX_ENABLE_VP8_VIDEO_DECODE_HW)
#include "mfx_vp8_dec_decode_common.h"
#if defined (MFX_ENABLE_VP8_VIDEO_DECODE_HW)
#include "mfx_vp8_dec_decode_hw.h"
#else
#include "mfx_vp8_dec_decode.h"
#endif
#endif

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
#include "mfx_vp9_dec_decode.h"
#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
#include "mfx_vp9_dec_decode_hw.h"
#endif
#endif

#if defined (MFX_ENABLE_AV1_VIDEO_DECODE)
#include "mfx_av1_dec_decode.h"
#endif

#ifdef MFX_ENABLE_USER_DECODE
#include "mfx_user_plugin.h"
#endif

template<>
VideoDECODE* _mfxSession::Create<VideoDECODE>(mfxVideoParam& par)
{
    VideoDECODE* pDECODE = nullptr;
    VideoCORE* core = m_pCORE.get();
    mfxStatus mfxRes = MFX_ERR_MEMORY_ALLOC;
    mfxU32 CodecId = par.mfx.CodecId;

    // create a codec instance
    switch (CodecId)
    {
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)
    case MFX_CODEC_MPEG2:
        pDECODE = new VideoDECODEMPEG2(core, &mfxRes);
        break;
#endif

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)
    case MFX_CODEC_VC1:
        pDECODE = new MFXVideoDECODEVC1(core, &mfxRes);
        break;
#endif

#if defined (MFX_ENABLE_H264_VIDEO_DECODE)
    case MFX_CODEC_AVC:
        pDECODE = new VideoDECODEH264(core, &mfxRes);
        break;
#endif

#if defined (MFX_ENABLE_H265_VIDEO_DECODE)
    case MFX_CODEC_HEVC:
        pDECODE = new VideoDECODEH265(core, &mfxRes);
        break;
#endif

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
    case MFX_CODEC_JPEG:
        pDECODE = new VideoDECODEMJPEG(core, &mfxRes);
        break;
#endif

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)
    case MFX_CODEC_VP8:
        pDECODE = new VideoDECODEVP8_HW(core, &mfxRes);
        break;
#endif

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
     case MFX_CODEC_VP9:
        pDECODE = new VideoDECODEVP9_HW(core, &mfxRes);
        break;
#endif

#if defined (MFX_ENABLE_AV1_VIDEO_DECODE)
     case MFX_CODEC_AV1:
         pDECODE = new VideoDECODEAV1(core, &mfxRes);
         break;
#endif

    default:
        break;
    }

    // check error(s)
    if (MFX_ERR_NONE != mfxRes)
    {
        delete pDECODE;
        pDECODE = nullptr;
    }

    return pDECODE;
}

mfxStatus MFXVideoDECODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(out, MFX_ERR_NULL_PTR);

#ifndef ANDROID
    if ((0 != in) && (MFX_HW_VAAPI == session->m_pCORE->GetVAType()))
    {
        // protected content not supported on Linux
        if(0 != in->Protected)
        {
            out->Protected = 0;
            return MFX_ERR_UNSUPPORTED;
        }
    }
#endif

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, in);

    mfxStatus mfxRes;

    try
    {
#ifdef MFX_ENABLE_USER_DECODE
        mfxRes = MFX_ERR_UNSUPPORTED;
        if (session->m_plgDec.get())
        {
            mfxRes = session->m_plgDec->Query(session->m_pCORE.get(), in, out);
        }
        // unsupported reserved to codecid != requested codecid
        if (MFX_ERR_UNSUPPORTED == mfxRes)
#endif
        switch (out->mfx.CodecId)
        {
#ifdef MFX_ENABLE_VC1_VIDEO_DECODE
        case MFX_CODEC_VC1:
            mfxRes = MFXVideoDECODEVC1::Query(session->m_pCORE.get(), in, out);
            break;
#endif

#ifdef MFX_ENABLE_H264_VIDEO_DECODE
        case MFX_CODEC_AVC:
            mfxRes = VideoDECODEH264::Query(session->m_pCORE.get(), in, out);
            break;
#endif

#ifdef MFX_ENABLE_H265_VIDEO_DECODE
        case MFX_CODEC_HEVC:
            mfxRes = VideoDECODEH265::Query(session->m_pCORE.get(), in, out);
            break;
#endif

#ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE
        case MFX_CODEC_MPEG2:
            mfxRes = VideoDECODEMPEG2::Query(session->m_pCORE.get(), in, out);
            break;
#endif

#ifdef MFX_ENABLE_MJPEG_VIDEO_DECODE
        case MFX_CODEC_JPEG:
            mfxRes = VideoDECODEMJPEG::Query(session->m_pCORE.get(), in, out);
            break;
#endif

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)
        case MFX_CODEC_VP8:
#if defined (MFX_ENABLE_VP8_VIDEO_DECODE_HW)
            mfxRes = VideoDECODEVP8_HW::Query(session->m_pCORE.get(), in, out);
#else
            mfxRes = VideoDECODEVP8::Query(session->m_pCORE.get(), in, out);
#endif // MFX_VA && MFX_ENABLE_VP8_VIDEO_DECODE_HW
            break;
#endif

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
        case MFX_CODEC_VP9:
#if defined (MFX_ENABLE_VP9_VIDEO_DECODE_HW)
            mfxRes = VideoDECODEVP9_HW::Query(session->m_pCORE.get(), in, out);
#else
            mfxRes = VideoDECODEVP9::Query(session->m_pCORE.get(), in, out);
#endif // MFX_VA && MFX_ENABLE_VP9_VIDEO_DECODE_HW
            break;
#endif

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE
        case MFX_CODEC_AV1:
            mfxRes = VideoDECODEAV1::Query(session->m_pCORE.get(), in, out);
            break;
#endif

        default:
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, out);
    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoDECODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);
    MFX_CHECK(request, MFX_ERR_NULL_PTR);

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, par);

    mfxStatus mfxRes;
    try
    {
#ifdef MFX_ENABLE_USER_DECODE
        mfxRes = MFX_ERR_UNSUPPORTED;
        if (session->m_plgDec.get())
        {
            mfxRes = session->m_plgDec->QueryIOSurf(session->m_pCORE.get(), par, 0, request);
        }
        // unsupported reserved to codecid != requested codecid
        if (MFX_ERR_UNSUPPORTED == mfxRes)
#endif
        switch (par->mfx.CodecId)
        {
#ifdef MFX_ENABLE_VC1_VIDEO_DECODE
        case MFX_CODEC_VC1:
            mfxRes = MFXVideoDECODEVC1::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

#ifdef MFX_ENABLE_H264_VIDEO_DECODE
        case MFX_CODEC_AVC:
            mfxRes = VideoDECODEH264::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

#ifdef MFX_ENABLE_H265_VIDEO_DECODE
        case MFX_CODEC_HEVC:
            mfxRes = VideoDECODEH265::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

#ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE
        case MFX_CODEC_MPEG2:
            mfxRes = VideoDECODEMPEG2::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

#ifdef MFX_ENABLE_MJPEG_VIDEO_DECODE
        case MFX_CODEC_JPEG:
            mfxRes = VideoDECODEMJPEG::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)
        case MFX_CODEC_VP8:
#if defined (MFX_ENABLE_VP8_VIDEO_DECODE_HW)
            mfxRes = VideoDECODEVP8_HW::QueryIOSurf(session->m_pCORE.get(), par, request);
#else
            mfxRes = VideoDECODEVP8::QueryIOSurf(session->m_pCORE.get(), par, request);
#endif // MFX_VA && MFX_ENABLE_VP8_VIDEO_DECODE_HW
            break;
#endif

#if defined (MFX_ENABLE_VP9_VIDEO_DECODE_HW)
        case MFX_CODEC_VP9:
#if defined (MFX_ENABLE_VP9_VIDEO_DECODE_HW)
            mfxRes = VideoDECODEVP9_HW::QueryIOSurf(session->m_pCORE.get(), par, request);
#else
            mfxRes = VideoDECODEVP9::QueryIOSurf(session->m_pCORE.get(), par, request);
#endif // MFX_VA && MFX_ENABLE_VP9_VIDEO_DECODE_HW
            break;
#endif

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE
        case MFX_CODEC_AV1:
            mfxRes = VideoDECODEAV1::QueryIOSurf(session->m_pCORE.get(), par, request);
            break;
#endif

        default:
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, request);
    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxVideoParam *par)
{
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(bs, MFX_ERR_NULL_PTR);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, bs);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, par);

    mfxStatus mfxRes;
    try
    {
#ifdef MFX_ENABLE_USER_DECODE
        mfxRes = MFX_ERR_UNSUPPORTED;
        if (session->m_plgDec.get())
        {
            mfxRes = session->m_plgDec->DecodeHeader(session->m_pCORE.get(), bs, par);
        }

        // unsupported reserved to codecid != requested codecid
        if (MFX_ERR_UNSUPPORTED == mfxRes)
#endif

        switch (par->mfx.CodecId)
        {
#ifdef MFX_ENABLE_VC1_VIDEO_DECODE
        case MFX_CODEC_VC1:
            mfxRes = MFXVideoDECODEVC1::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#ifdef MFX_ENABLE_H264_VIDEO_DECODE
        case MFX_CODEC_AVC:
            mfxRes = VideoDECODEH264::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#ifdef MFX_ENABLE_H265_VIDEO_DECODE
        case MFX_CODEC_HEVC:
            mfxRes = VideoDECODEH265::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE
        case MFX_CODEC_MPEG2:
            mfxRes = VideoDECODEMPEG2::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#ifdef MFX_ENABLE_MJPEG_VIDEO_DECODE
        case MFX_CODEC_JPEG:
            mfxRes = VideoDECODEMJPEG::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)
        case MFX_CODEC_VP8:
            mfxRes = VP8DecodeCommon::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#if defined (MFX_ENABLE_VP9_VIDEO_DECODE_HW)
        case MFX_CODEC_VP9:
            mfxRes = VideoDECODEVP9_HW::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE
        case MFX_CODEC_AV1:
            mfxRes = VideoDECODEAV1::DecodeHeader(session->m_pCORE.get(), bs, par);
            break;
#endif

        default:
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoDECODE_Init(mfxSession session, mfxVideoParam *par)
{
    mfxStatus mfxRes;

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, par);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);

    try
    {
        // check existence of component
        if (!session->m_pDECODE)
        {
            // create a new instance
            session->m_pDECODE.reset(session->Create<VideoDECODE>(*par));
            MFX_CHECK(session->m_pDECODE.get(), MFX_ERR_INVALID_VIDEO_PARAM);
        }

        mfxRes = session->m_pDECODE->Init(par);
    }
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoDECODE_Close(mfxSession session)
{
    mfxStatus mfxRes = MFX_ERR_NONE;

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pScheduler, MFX_ERR_NOT_INITIALIZED);

    try
    {
        MFX_CHECK(session->m_pDECODE, MFX_ERR_NOT_INITIALIZED);

        // wait until all tasks are processed
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_pDECODE.get());

        mfxRes = session->m_pDECODE->Close();
        // delete the codec's instance if not plugin
        if (!session->m_plgDec)
        {
            session->m_pDECODE.reset(nullptr);
        }
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);

    MFX_CHECK_STS(mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp)
{
    mfxStatus mfxRes;

#ifdef MFX_TRACE_ENABLE
    MFX_AUTO_LTRACE_WITHID(MFX_TRACE_LEVEL_API, "MFX_DecodeFrameAsync");
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, bs);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, surface_work);
#endif

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pScheduler, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(session->m_pDECODE.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(syncp);
    MFX_CHECK_NULL_PTR1(surface_out);

    try
    {
        mfxSyncPoint syncPoint = NULL;
        MFX_TASK task;

        // Wait for the bit stream
        mfxRes = session->m_pScheduler->WaitForDependencyResolved(bs);
        MFX_CHECK_STS(mfxRes);

        // reset the sync point
        *syncp = NULL;
        *surface_out = NULL;

        memset(&task, 0, sizeof(MFX_TASK));
        mfxRes = session->m_pDECODE->DecodeFrameCheck(bs, surface_work, surface_out, &task.entryPoint);
        MFX_CHECK(mfxRes >= 0 || MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes)
                  || MFX_ERR_MORE_DATA == static_cast<int>(mfxRes)
                  || MFX_ERR_MORE_SURFACE == static_cast<int>(mfxRes), mfxRes);

        // source data is OK, go forward
        if (task.entryPoint.pRoutine)
        {
            mfxStatus mfxAddRes;

            task.pOwner = session->m_pDECODE.get();
            task.priority = session->m_priority;
            task.threadingPolicy = session->m_pDECODE->GetThreadingPolicy();
            // fill dependencies
            task.pSrc[0] = *surface_out;
            task.pDst[0] = *surface_out;
            // this is wa to remove external task dependency for HEVC SW decode plugin.
            // need only because SW HEVC decode is pseudo
            {
                mfxPlugin plugin;
                mfxPluginParam par;
                if (session->m_plgDec.get())
                {
                    session->m_plgDec.get()->GetPlugin(plugin);
                    MFX_CHECK_STS(plugin.GetPluginParam(plugin.pthis, &par));
                    if (MFX_PLUGINID_HEVCD_SW == par.PluginUID)
                    {
                        task.pDst[0] = 0;
                    }
                }
            }

#ifdef MFX_TRACE_ENABLE
            task.nParentId = MFX_AUTO_TRACE_GETID();
            task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_DECODE;
#endif

            // register input and call the task
            mfxAddRes = session->m_pScheduler->AddTask(task, &syncPoint);
            MFX_CHECK_STS(mfxAddRes);
        }

        if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
        {
            mfxRes = MFX_WRN_DEVICE_BUSY;
        }

        // return pointer to synchronization point
        if (MFX_ERR_NONE == mfxRes || (mfxRes == MFX_WRN_VIDEO_PARAM_CHANGED && *surface_out != NULL))
        {
            *syncp = syncPoint;
        }
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    if (mfxRes == MFX_ERR_NONE)
    {
        if (surface_out && *surface_out)
        {
            MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_API, *surface_out);
        }
        if (syncp)
        {
            MFX_LTRACE_P(MFX_TRACE_LEVEL_API, *syncp);
        }
    }
    MFX_LTRACE_I(MFX_TRACE_LEVEL_API, mfxRes);

    return mfxRes;

} // mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_dec, mfxFrameSurface1 **surface_disp, mfxSyncPoint *syncp)

//
// THE OTHER DECODE FUNCTIONS HAVE IMPLICIT IMPLEMENTATION
//

FUNCTION_RESET_IMPL(DECODE, Reset, (mfxSession session, mfxVideoParam *par), (par))

FUNCTION_IMPL(DECODE, GetVideoParam, (mfxSession session, mfxVideoParam *par), (par))
FUNCTION_IMPL(DECODE, GetDecodeStat, (mfxSession session, mfxDecodeStat *stat), (stat))
FUNCTION_IMPL(DECODE, SetSkipMode, (mfxSession session, mfxSkipMode mode), (mode))
FUNCTION_IMPL(DECODE, GetPayload, (mfxSession session, mfxU64 *ts, mfxPayload *payload), (ts, payload))
