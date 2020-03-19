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
#include <mfx_tools.h>
#include <mfx_common.h>
#include <mfx_user_plugin.h>

// sheduling and threading stuff
#include <mfx_task.h>

#ifdef MFX_ENABLE_VPP
// VPP include files here
#include "mfx_vpp_main.h"       // this VideoVPP class builds VPP pipeline and run the VPP pipeline
#endif

template<>
VideoVPP* _mfxSession::Create<VideoVPP>(mfxVideoParam& /*par*/)
{
    VideoVPP *pVPP = nullptr;

#ifdef MFX_ENABLE_VPP
    VideoCORE* core = m_pCORE.get();
    mfxStatus mfxRes = MFX_ERR_MEMORY_ALLOC;

    pVPP = new VideoVPPMain(core, &mfxRes);
    if (MFX_ERR_NONE != mfxRes)
    {
        delete pVPP;
        pVPP = nullptr;
    }
#endif // MFX_ENABLE_VPP

    return pVPP;
}

mfxStatus MFXVideoVPP_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, in);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(out, MFX_ERR_NULL_PTR);

    if ((0 != in) && (MFX_HW_VAAPI == session->m_pCORE->GetVAType()))
    {
        // protected content not supported on Linux
        if(0 != in->Protected)
        {
            out->Protected = 0;
            return MFX_ERR_UNSUPPORTED;
        }
    }

    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;
    try
    {
#ifdef MFX_ENABLE_USER_VPP
        if (session->m_plgVPP.get())
        {
            mfxRes = session->m_plgVPP->Query(session->m_pCORE.get(), in, out);
        }
        else
        {
#endif

#ifdef MFX_ENABLE_VPP
            mfxRes = VideoVPPMain::Query(session->m_pCORE.get(), in, out);
#endif // MFX_ENABLE_VPP

#ifdef MFX_ENABLE_USER_VPP
        }
#endif
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_NULL_PTR;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, out);
    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoVPP_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, par);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);
    MFX_CHECK(request, MFX_ERR_NULL_PTR);

    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;
    try
    {
#ifdef MFX_ENABLE_USER_VPP
        if (session->m_plgVPP.get())
        {
            mfxRes = session->m_plgVPP->QueryIOSurf(session->m_pCORE.get(), par, &(request[0]),  &(request[1]) );
        }
        else
        {
#endif

#ifdef MFX_ENABLE_VPP
            mfxRes = VideoVPPMain::QueryIOSurf(session->m_pCORE.get(), par, request/*, session->m_adapterNum*/);
#endif // MFX_ENABLE_VPP

#ifdef MFX_ENABLE_USER_VPP
        }
#endif
    }
    // handle error(s)
    catch(...)
    {
        mfxRes = MFX_ERR_NULL_PTR;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, request);
    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoVPP_Init(mfxSession session, mfxVideoParam *par)
{
    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, par);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(par, MFX_ERR_NULL_PTR);

    try
    {
        // check existence of component
        if (!session->m_pVPP)
        {
            // create a new instance
            session->m_pVPP.reset(session->Create<VideoVPP>(*par));
#ifdef MFX_ENABLE_VPP
            MFX_CHECK(session->m_pVPP.get(), MFX_ERR_INVALID_VIDEO_PARAM);
#else
            MFX_CHECK(session->m_pVPP.get(), MFX_ERR_UNSUPPORTED);
#endif
        }

        // create a new instance
        mfxRes = session->m_pVPP->Init(par);
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;
}

mfxStatus MFXVideoVPP_Close(mfxSession session)
{
    mfxStatus mfxRes;

    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pScheduler, MFX_ERR_NOT_INITIALIZED);

    try
    {
        if (!session->m_pVPP)
        {
            return MFX_ERR_NOT_INITIALIZED;
        }

        // wait until all tasks are processed
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_pVPP.get());

        mfxRes = session->m_pVPP->Close();
        // delete the codec's instance if not plugin
        if (!session->m_plgVPP)
        {
            session->m_pVPP.reset(nullptr);
        }
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;
}

static
mfxStatus MFXVideoVPPLegacyRoutine(void *pState, void *pParam,
                                   mfxU32 threadNumber, mfxU32 /* callNumber */)
{
    MFX_AUTO_LTRACE_FUNC(MFX_TRACE_LEVEL_API);
    VideoVPP *pVPP = (VideoVPP *) pState;
    MFX_THREAD_TASK_PARAMETERS *pTaskParam = (MFX_THREAD_TASK_PARAMETERS *) pParam;
    mfxStatus mfxRes;

    // check error(s)
    if ((NULL == pState) ||
        (NULL == pParam) ||
        (0 != threadNumber))
    {
        return MFX_ERR_NULL_PTR;
    }

    // call the obsolete method
    mfxRes = pVPP->RunFrameVPP(pTaskParam->vpp.in,
                               pTaskParam->vpp.out,
                               pTaskParam->vpp.aux);

    return mfxRes;

} // mfxStatus MFXVideoVPPLegacyRoutine(void *pState, void *pParam,

enum
{
    MFX_NUM_ENTRY_POINTS = 2
};

mfxStatus MFXVideoVPP_RunFrameVPPAsync(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp)
{
    mfxStatus mfxRes;

    MFX_AUTO_LTRACE_WITHID(MFX_TRACE_LEVEL_API, "MFX_RunFrameVPPAsync");
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, aux);
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, in);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pVPP.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(syncp, MFX_ERR_NULL_PTR);

    try
    {
#ifdef MFX_ENABLE_USER_VPP
      if (session->m_plgVPP.get())
      {
          MFX_TASK task;
          mfxSyncPoint syncPoint = NULL;
          *syncp = NULL;
          memset(&task, 0, sizeof(MFX_TASK));

          mfxRes = session->m_plgVPP->VPPFrameCheck(in, out, aux, &task.entryPoint);

          if (task.entryPoint.pRoutine)
          {
              mfxStatus mfxAddRes;

              task.pOwner = session->m_plgVPP.get();
              task.priority = session->m_priority;
              task.threadingPolicy = session->m_plgVPP->GetThreadingPolicy();
              // fill dependencies
              task.pSrc[0] = in;
              task.pDst[0] = out;
              if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
                task.pDst[0] = NULL;

              #ifdef MFX_TRACE_ENABLE
              task.nParentId = MFX_AUTO_TRACE_GETID();
              task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP;
              #endif

              // register input and call the task
              mfxAddRes = session->m_pScheduler->AddTask(task, &syncPoint);
              if (MFX_ERR_NONE != mfxAddRes)
              {
                  return mfxAddRes;
              }
              *syncp = syncPoint;
          }
      }
      else
      {
#endif
        mfxSyncPoint syncPoint = NULL;
        MFX_ENTRY_POINT entryPoints[MFX_NUM_ENTRY_POINTS];
        mfxU32 numEntryPoints = MFX_NUM_ENTRY_POINTS;

        memset(&entryPoints, 0, sizeof(entryPoints));
        mfxRes = session->m_pVPP->VppFrameCheck(in, out, aux, entryPoints, numEntryPoints);
        // source data is OK, go forward
        if ((MFX_ERR_NONE == mfxRes) ||
            (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes)) ||
            (MFX_ERR_MORE_SURFACE == mfxRes) ||
            (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfxRes))
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
                task.pOwner = session->m_pVPP.get();
                task.entryPoint.pRoutine = &MFXVideoVPPLegacyRoutine;
                task.entryPoint.pState = session->m_pVPP.get();
                task.entryPoint.requiredNumThreads = 1;

                // fill legacy parameters
                task.obsolete_params.vpp.in = in;
                task.obsolete_params.vpp.out = out;
                task.obsolete_params.vpp.aux = aux;
                // END OF OBSOLETE PART

                task.priority = session->m_priority;
                task.threadingPolicy = session->m_pVPP->GetThreadingPolicy();
                // fill dependencies
                task.pSrc[0] = in;
                task.pDst[0] = out;

                if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
                    task.pDst[0] = NULL;

#ifdef MFX_TRACE_ENABLE
                task.nParentId = MFX_AUTO_TRACE_GETID();
                task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP;
#endif // MFX_TRACE_ENABLE

                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
            }
            else if (1 == numEntryPoints)
            {
                MFX_TASK task;

                memset(&task, 0, sizeof(task));
                task.pOwner = session->m_pVPP.get();
                task.entryPoint = entryPoints[0];
                task.priority = session->m_priority;
                task.threadingPolicy = session->m_pVPP->GetThreadingPolicy();
                // fill dependencies
                task.pSrc[0] = in;
                task.pDst[0] = out;
                if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
                    task.pDst[0] = NULL;


#ifdef MFX_TRACE_ENABLE
                task.nParentId = MFX_AUTO_TRACE_GETID();
                task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP;
#endif
                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
            }
            else
            {
                MFX_TASK task;

                memset(&task, 0, sizeof(task));
                task.pOwner = session->m_pVPP.get();
                task.entryPoint = entryPoints[0];
                task.priority = session->m_priority;
                task.threadingPolicy = session->m_pVPP->GetThreadingPolicy();
                // fill dependencies
                task.pSrc[0] = in;
                task.pDst[0] = entryPoints[0].pParam;


#ifdef MFX_TRACE_ENABLE
                task.nParentId = MFX_AUTO_TRACE_GETID();
                task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP;
#endif
                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));

                memset(&task, 0, sizeof(task));
                task.pOwner = session->m_pVPP.get();
                task.entryPoint = entryPoints[1];
                task.priority = session->m_priority;
                task.threadingPolicy = session->m_pVPP->GetThreadingPolicy();

                // fill dependencies
                task.pSrc[0] = entryPoints[0].pParam;
                task.pDst[0] = out;
                task.pDst[1] = aux;
                if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
                {
                    task.pDst[0] = NULL;
                    task.pDst[1] = NULL;
                }

#ifdef MFX_TRACE_ENABLE
                task.nParentId = MFX_AUTO_TRACE_GETID();
                task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP2;
#endif
                // register input and call the task
                MFX_CHECK_STS(session->m_pScheduler->AddTask(task, &syncPoint));
            }

            if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
            {
                mfxRes = MFX_ERR_MORE_DATA;
                syncPoint = NULL;
            }
        }

        // return pointer to synchronization point
        *syncp = syncPoint;
#ifdef MFX_ENABLE_USER_VPP
      }
#endif
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, out);
    if (mfxRes == MFX_ERR_NONE && syncp)
    {
        MFX_LTRACE_P(MFX_TRACE_LEVEL_PARAMS, *syncp);
    }
    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;

} // mfxStatus MFXVideoVPP_RunFrameVPPAsync(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp)

mfxStatus MFXVideoVPP_RunFrameVPPAsyncEx(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp)
{
#if !defined(MFX_ENABLE_USER_VPP)
    (void)in;
    (void)surface_work;
    (void)surface_out;
#endif

    mfxStatus mfxRes;

    MFX_AUTO_LTRACE_WITHID(MFX_TRACE_LEVEL_API, "MFX_RunFrameVPPAsyncEx");
    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, in);

    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pVPP.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(syncp, MFX_ERR_NULL_PTR);

    try
    {
#ifdef MFX_ENABLE_USER_VPP
      if (session->m_plgVPP.get())
      {
          MFX_TASK task;
          mfxSyncPoint syncPoint = NULL;
          *syncp = NULL;
          memset(&task, 0, sizeof(MFX_TASK));

          mfxRes = session->m_plgVPP->VPPFrameCheckEx(in, surface_work, surface_out, &task.entryPoint);

          if (task.entryPoint.pRoutine)
          {
              mfxStatus mfxAddRes;

              task.pOwner = session->m_plgVPP.get();
              task.priority = session->m_priority;
              task.threadingPolicy = session->m_plgVPP->GetThreadingPolicy();
              // fill dependencies
              task.pSrc[0] = in;
              task.pDst[0] = surface_work;
              if (MFX_ERR_MORE_DATA_SUBMIT_TASK == static_cast<int>(mfxRes))
                task.pDst[0] = NULL;

              #ifdef MFX_TRACE_ENABLE
              task.nParentId = MFX_AUTO_TRACE_GETID();
              task.nTaskId = MFX::CreateUniqId() + MFX_TRACE_ID_VPP;
              #endif

              // register input and call the task
              mfxAddRes = session->m_pScheduler->AddTask(task, &syncPoint);
              if (MFX_ERR_NONE != mfxAddRes)
              {
                  return mfxAddRes;
              }
              *syncp = syncPoint;
          }
      }
      else
      {
#endif
        //MediaSDK's VPP should not work through Ex function
        return MFX_ERR_UNDEFINED_BEHAVIOR;

#ifdef MFX_ENABLE_USER_VPP
      }
#endif
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }

    MFX_LTRACE_BUFFER(MFX_TRACE_LEVEL_PARAMS, surface_work);
    if (mfxRes == MFX_ERR_NONE && syncp)
    {
        MFX_LTRACE_P(MFX_TRACE_LEVEL_PARAMS, *syncp);
    }
    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, mfxRes);
    return mfxRes;

} // mfxStatus MFXVideoVPP_RunFrameVPPAsyncEx(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxThreadTask *task);

//
// THE OTHER VPP FUNCTIONS HAVE IMPLICIT IMPLEMENTATION
//

FUNCTION_RESET_IMPL(VPP, Reset, (mfxSession session, mfxVideoParam *par), (par))

FUNCTION_IMPL(VPP, GetVideoParam, (mfxSession session, mfxVideoParam *par), (par))
FUNCTION_IMPL(VPP, GetVPPStat, (mfxSession session, mfxVPPStat *stat), (stat))
