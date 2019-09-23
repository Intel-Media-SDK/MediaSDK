// Copyright (c) 2017-2019 Intel Corporation
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
#include <mfx_common.h>

// sheduling and threading stuff
#include <mfx_task.h>

#include "mfxenc.h"
#include "mfx_enc_ext.h"



#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_LA_H264_VIDEO_HW)
#include "mfx_h264_la.h"
#endif

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
#include "mfx_h264_preenc.h"
#endif

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)
#include "mfx_h264_enc.h"
#endif

template<>
VideoENC* _mfxSession::Create<VideoENC>(mfxVideoParam& par)
{
    VideoENC* pENC = nullptr;
    mfxStatus mfxRes = MFX_ERR_MEMORY_ALLOC;
    mfxU32 codecId = par.mfx.CodecId;

    switch (codecId)
    {
#if ( defined(MFX_ENABLE_LA_H264_VIDEO_HW) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC) || defined(MFX_ENABLE_H264_VIDEO_FEI_ENC))
    case MFX_CODEC_AVC:
#if defined(MFX_ENABLE_LA_H264_VIDEO_HW)
        if (bEnc_LA(&par))
            pENC = (VideoENC*) new VideoENC_LA(m_pCORE.get(), &mfxRes);
#endif
#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
        if (bEnc_PREENC(&par))
            pENC = (VideoENC*) new VideoENC_PREENC(m_pCORE.get(), &mfxRes);
#endif
#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW)
        if (bEnc_ENC(&par))
            pENC = (VideoENC*) new VideoENC_ENC(m_pCORE.get(), &mfxRes);
#endif
        break;
#endif

    default:
        break;
    }

    // check error(s)
    if (MFX_ERR_NONE != mfxRes)
    {
        delete pENC;
        pENC = nullptr;
    }

    return pENC;
}

mfxStatus MFXVideoENC_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    (void)in;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(out, MFX_ERR_NULL_PTR);

    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;
    try
    {

#ifdef MFX_ENABLE_USER_ENC
        mfxRes = MFX_ERR_UNSUPPORTED;

        _mfxSession_1_10 * versionedSession = (_mfxSession_1_10 *)(session);
        MFXIPtr<MFXISession_1_10> newSession(versionedSession->QueryInterface(MFXISession_1_10_GUID));

        if (newSession && newSession->GetPreEncPlugin().get())
        {
            mfxRes = newSession->GetPreEncPlugin()->Query(session->m_pCORE.get(), in, out);
        }
        // unsupported reserved to codecid != requested codecid
        if (MFX_ERR_UNSUPPORTED == mfxRes)
#endif
        switch (out->mfx.CodecId)
        {
#if  defined(MFX_ENABLE_LA_H264_VIDEO_HW) || defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
        case MFX_CODEC_AVC:
#if defined(MFX_ENABLE_LA_H264_VIDEO_HW)
        if (bEnc_LA(in))
            mfxRes = VideoENC_LA::Query(session->m_pCORE.get(), in, out);
#endif

#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
        if (bEnc_PREENC(out))
            mfxRes = VideoENC_PREENC::Query(session->m_pCORE.get(),in, out);
#endif
            break;
#endif // MFX_ENABLE_H264_VIDEO_ENC || MFX_ENABLE_H264_VIDEO_ENC_H



        case 0:
        default:
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_NULL_PTR;
    }
    return mfxRes;
}

mfxStatus MFXVideoENC_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);
    MFX_CHECK(request, MFX_ERR_NULL_PTR);

    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;
    try
    {
#ifdef MFX_ENABLE_USER_ENC
        mfxRes = MFX_ERR_UNSUPPORTED;
        _mfxSession_1_10 * versionedSession = (_mfxSession_1_10 *)(session);
        MFXIPtr<MFXISession_1_10> newSession(versionedSession->QueryInterface(MFXISession_1_10_GUID));
        if (newSession && newSession->GetPreEncPlugin().get())
        {
            mfxRes = newSession->GetPreEncPlugin()->QueryIOSurf(session->m_pCORE.get(), par, request, 0);
        }
        // unsupported reserved to codecid != requested codecid
        if (MFX_ERR_UNSUPPORTED == mfxRes)
#endif
        switch (par->mfx.CodecId)
        {

#if  ( defined(MFX_ENABLE_LA_H264_VIDEO_HW) ||  defined (MFX_ENABLE_H264_VIDEO_FEI_PREENC) || defined(MFX_ENABLE_H264_VIDEO_FEI_ENC))
        case MFX_CODEC_AVC:

#if defined(MFX_ENABLE_LA_H264_VIDEO_HW)
        if (bEnc_LA(par))
            mfxRes = VideoENC_LA::QueryIOSurf(session->m_pCORE.get(), par, request);
#endif
#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW)
        if (bEnc_ENC(par))
            mfxRes = VideoENC_ENC::QueryIOSurf(session->m_pCORE.get(), par, request);
#endif

            break;
#endif // MFX_ENABLE_H264_VIDEO_ENC || MFX_ENABLE_H264_VIDEO_ENC_HW



        case 0:
        default:
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_NULL_PTR;
    }
    return mfxRes;
}

mfxStatus MFXVideoENC_Init(mfxSession session, mfxVideoParam *par)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);

    try
    {
        // check existence of component
        if (!session->m_pENC)
        {
            // create a new instance
            session->m_pENC.reset(session->Create<VideoENC>(*par));
            MFX_CHECK(session->m_pENC.get(), MFX_ERR_INVALID_VIDEO_PARAM);
        }

        mfxRes = session->m_pENC->Init(par);
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;
}

mfxStatus MFXVideoENC_Close(mfxSession session)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pScheduler, MFX_ERR_NOT_INITIALIZED);

    try
    {
        if (!session->m_pENC)
        {
            return MFX_ERR_NOT_INITIALIZED;
        }

        // wait until all tasks are processed
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_pENC.get());

        mfxRes = session->m_pENC->Close();
        // delete the codec's instance
        session->m_pENC.reset(nullptr);
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;
}

static
mfxStatus MFXVideoENCLegacyRoutineExt(void *pState, void *pParam,
                                   mfxU32 threadNumber, mfxU32 /* callNumber */)
{
    VideoENC_Ext * pENC = (VideoENC_Ext  *) pState;
    MFX_THREAD_TASK_PARAMETERS *pTaskParam = (MFX_THREAD_TASK_PARAMETERS *) pParam;

    // check error(s)
    if ((NULL == pState) ||
        (NULL == pParam) ||
        (0 != threadNumber))
    {
        return MFX_ERR_NULL_PTR;
    }
    return pENC->RunFrameVmeENC(pTaskParam->enc.in, pTaskParam->enc.out);
}

enum
{
    MFX_NUM_ENTRY_POINTS = 2
};


mfxStatus  MFXVideoENC_ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pENC.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(syncp, MFX_ERR_NULL_PTR);

    VideoENC_Ext *pEnc = dynamic_cast<VideoENC_Ext *>(session->m_pENC.get());
    MFX_CHECK(pEnc, MFX_ERR_INVALID_HANDLE);

    try
    {
        mfxSyncPoint syncPoint = NULL;
        MFX_ENTRY_POINT entryPoints[MFX_NUM_ENTRY_POINTS];
        mfxU32 numEntryPoints = MFX_NUM_ENTRY_POINTS;
        memset(&entryPoints, 0, sizeof(entryPoints));

        mfxRes = pEnc->RunFrameVmeENCCheck(in,out,entryPoints,numEntryPoints);

        if ((MFX_ERR_NONE == mfxRes) ||
            (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfxRes) ||
            (MFX_WRN_OUT_OF_RANGE == mfxRes) ||
            (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes)) ||
            (MFX_ERR_MORE_BITSTREAM == mfxRes))
        {
            // prepare the absolete kind of task.
            // it is absolete and must be removed.
            if (NULL == entryPoints[0].pRoutine)
            {
                MFX_TASK task;
                memset(&task, 0, sizeof(task));
                // BEGIN OF OBSOLETE PART
                task.bObsoleteTask = true;
                // fill task info
                task.entryPoint.pRoutine = &MFXVideoENCLegacyRoutineExt;
                task.entryPoint.pState = pEnc;
                task.entryPoint.requiredNumThreads = 1;

                // fill legacy parameters
                task.obsolete_params.enc.in = in;
                task.obsolete_params.enc.out = out;

                task.priority = session->m_priority;
                task.threadingPolicy = pEnc->GetThreadingPolicy();
                // fill dependencies
                task.pSrc[0] = in;
                task.pDst[0] = out;
                task.pOwner= pEnc;
                mfxRes = session->m_pScheduler->AddTask(task, &syncPoint);

            } // END OF OBSOLETE PART
            else if (1 == numEntryPoints)
            {
                MFX_TASK task;

                memset(&task, 0, sizeof(task));
                task.pOwner = pEnc;
                task.entryPoint = entryPoints[0];
                task.priority = session->m_priority;
                task.threadingPolicy = pEnc->GetThreadingPolicy();
                // fill dependencies

                task.pSrc[0] =  out; // for LA plugin
                task.pSrc[1] =  in->InSurface;
                task.pDst[0] =  out? out->ExtParam : 0;

                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
            }
            else
            {
                MFX_TASK task;

                memset(&task, 0, sizeof(task));
                task.pOwner = pEnc;
                task.entryPoint = entryPoints[0];
                task.priority = session->m_priority;
                task.threadingPolicy = pEnc->GetThreadingPolicy();
                // fill dependencies

                task.pSrc[0] = in->InSurface;
                task.pDst[0] = entryPoints[0].pParam;

                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));

                memset(&task, 0, sizeof(task));
                task.pOwner = pEnc;
                task.entryPoint = entryPoints[1];
                task.priority = session->m_priority;
                task.threadingPolicy = pEnc->GetThreadingPolicy();
                // fill dependencies
                task.pSrc[0] = entryPoints[0].pParam;
                task.pDst[0] = (MFX_ERR_NONE == mfxRes) ? out:0; // sync point for LA plugin
                task.pDst[1] = in->InSurface;


                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
            }

            // IT SHOULD BE REMOVED
            if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
            {
                mfxRes = MFX_ERR_MORE_DATA;
                syncPoint = NULL;
            }

        }

        // return pointer to synchronization point
        *syncp = syncPoint;
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;

} // MFXVideoENC_ProcessFrameAsync


//
// THE OTHER ENC FUNCTIONS HAVE IMPLICIT IMPLEMENTATION
//

FUNCTION_RESET_IMPL(ENC, Reset, (mfxSession session, mfxVideoParam *par), (par))

FUNCTION_IMPL(ENC, GetVideoParam, (mfxSession session, mfxVideoParam *par), (par))
FUNCTION_IMPL(ENC, GetFrameParam, (mfxSession session, mfxFrameParam *par), (par))
