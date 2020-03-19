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
#include <mfxfei.h>
#include <mfxpak.h>
#include "mfx_pak_ext.h"

#include <mfx_session.h>
#include <mfx_common.h>

// sheduling and threading stuff
#include <mfx_task.h>



#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)
#include "mfx_h264_fei_pak.h"
#endif

template<>
VideoPAK* _mfxSession::Create<VideoPAK>(mfxVideoParam& par)
{
    VideoPAK* pPAK = nullptr;
    mfxStatus mfxRes = MFX_ERR_MEMORY_ALLOC;
    mfxU32 codecId = par.mfx.CodecId;

    switch (codecId)
    {
#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)
    case MFX_CODEC_AVC:
        if (bEnc_PAK(&par))
            pPAK = (VideoPAK*) new VideoPAK_PAK(m_pCORE.get(), &mfxRes);
        break;
#endif // MFX_ENABLE_H264_VIDEO_FEI_PAK

    default:
        break;
    }

    // check error(s)
    if (MFX_ERR_NONE != mfxRes)
    {
        delete pPAK;
        pPAK = nullptr;
    }

    return pPAK;
}

mfxStatus MFXVideoPAK_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    (void)in;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(out, MFX_ERR_NULL_PTR);

    mfxStatus mfxRes;
    try
    {
        switch (out->mfx.CodecId)
        {

#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)
        case MFX_CODEC_AVC:
            if (bEnc_PAK(in))
                mfxRes = VideoPAK_PAK::Query(session->m_pCORE.get(), in, out);
            else
                mfxRes = MFX_ERR_UNSUPPORTED;
            break;
#endif

        default: case 0:
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

mfxStatus MFXVideoPAK_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);
    MFX_CHECK(request, MFX_ERR_NULL_PTR);

    mfxStatus mfxRes;
    try
    {
        switch (par->mfx.CodecId)
        {

#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PAK)
        case MFX_CODEC_AVC:
            if (bEnc_PAK(par))
                mfxRes = VideoPAK_PAK::QueryIOSurf(session->m_pCORE.get(), par, request);
            else
                mfxRes = MFX_ERR_UNSUPPORTED;
            break;
#endif


        default: case 0:
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

mfxStatus MFXVideoPAK_Init(mfxSession session, mfxVideoParam *par)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);

    try
    {
        // create a new instance
        session->m_pPAK.reset(session->Create<VideoPAK>(*par));
        MFX_CHECK(session->m_pPAK.get(), MFX_ERR_INVALID_VIDEO_PARAM);

        mfxRes = session->m_pPAK->Init(par);
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;
}

mfxStatus MFXVideoPAK_Close(mfxSession session)
{
    mfxStatus mfxRes;

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);

    try
    {
        if (!session->m_pPAK)
        {
            return MFX_ERR_NOT_INITIALIZED;
        }

        // wait until all tasks are processed
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_pPAK.get());

        mfxRes = session->m_pPAK->Close();
        // delete the codec's instance
        session->m_pPAK.reset(nullptr);
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    return mfxRes;
}

enum
{
    MFX_NUM_ENTRY_POINTS = 2
};

mfxStatus MFXVideoPAK_ProcessFrameAsync(mfxSession session , mfxPAKInput *in, mfxPAKOutput *out, mfxSyncPoint *syncp)
{
     mfxStatus mfxRes;

     MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
     MFX_CHECK(session->m_pPAK.get(), MFX_ERR_NOT_INITIALIZED);
     MFX_CHECK(syncp, MFX_ERR_NULL_PTR);

     VideoPAK_Ext *pPak = dynamic_cast<VideoPAK_Ext *>(session->m_pPAK.get());
     MFX_CHECK(pPak, MFX_ERR_INVALID_HANDLE);

     try
     {
         mfxSyncPoint syncPoint = NULL;
         MFX_ENTRY_POINT entryPoints[MFX_NUM_ENTRY_POINTS];
         mfxU32 numEntryPoints = MFX_NUM_ENTRY_POINTS;
         memset(&entryPoints, 0, sizeof(entryPoints));

         mfxRes = pPak->RunFramePAKCheck(in,out,entryPoints,numEntryPoints);

         if ((MFX_ERR_NONE == mfxRes) ||
             (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfxRes) ||
             (MFX_WRN_OUT_OF_RANGE == mfxRes) ||
             (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes)) ||
             (MFX_ERR_MORE_BITSTREAM == mfxRes))
         {
             if (1 == numEntryPoints)
             {
                 MFX_TASK task;

                 memset(&task, 0, sizeof(task));
                 task.pOwner = pPak;
                 task.entryPoint = entryPoints[0];
                 task.priority = session->m_priority;
                 task.threadingPolicy = pPak->GetThreadingPolicy();
                 // fill dependencies

                 task.pSrc[0] = in->InSurface;
                 task.pSrc[1] = out ? out->ExtParam : 0; // for LA plugin
                 task.pDst[0] = out;

                 // register input and call the task
                 MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
             }
             else
             {
                 MFX_TASK task;

                 memset(&task, 0, sizeof(task));
                 task.pOwner = pPak;
                 task.entryPoint = entryPoints[0];
                 task.priority = session->m_priority;
                 task.threadingPolicy = pPak->GetThreadingPolicy();
                 // fill dependencies
                 task.pSrc[0] = pPak->GetSrcForSync(entryPoints[0]);
                 task.pDst[0] = entryPoints[0].pParam;

                 // register input and call the task
                 MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));

                 memset(&task, 0, sizeof(task));
                 task.pOwner = pPak;
                 task.entryPoint = entryPoints[1];
                 task.priority = session->m_priority;
                 task.threadingPolicy = pPak->GetThreadingPolicy();
                 // fill dependencies
                 task.pSrc[0] = entryPoints[0].pParam;
                 task.pDst[0] = pPak->GetDstForSync(entryPoints[1]);
                 task.pDst[1] = out ? out->ExtParam : 0; // sync point for LA plugin

                 // register input and call the task
                 MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
             }

             // IT SHOULD BE REMOVED
             if (MFX_ERR_MORE_DATA_SUBMIT_TASK == mfxRes)
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
}

//
// THE OTHER PAK FUNCTIONS HAVE IMPLICIT IMPLEMENTATION
//

FUNCTION_RESET_IMPL(PAK, Reset, (mfxSession session, mfxVideoParam *par), (par))

FUNCTION_IMPL(PAK, GetVideoParam, (mfxSession session, mfxVideoParam *par), (par))
FUNCTION_IMPL(PAK, GetFrameParam, (mfxSession session, mfxFrameParam *par), (par))
