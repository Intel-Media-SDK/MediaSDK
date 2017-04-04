/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "mfx_samples_config.h"

#include <stdio.h>
#include "plugin_rotate.h"

// disable "unreferenced formal parameter" warning -
// not all formal parameters of interface functions will be used by sample plugin
#pragma warning(disable : 4100)

#define SWAP_BYTES(a, b) {mfxU8 tmp; tmp = a; a = b; b = tmp;}

//defining module template for generic plugin
#include "mfx_plugin_module.h"
PluginModuleTemplate g_PluginModule = {
    NULL,
    NULL,
    Rotate::CreateGenericPlugin,
    NULL
};

/* Rotate class implementation */
Rotate::Rotate() :
    m_bInited(false),
    m_pTasks(NULL),
    m_bIsInOpaque(false),
    m_bIsOutOpaque(false)
{
    m_MaxNumTasks = 0;
    m_pChunks = 0;
    m_NumChunks = 0;

    memset(&m_VideoParam, 0, sizeof(m_VideoParam));
    memset(&m_Param, 0, sizeof(m_Param));

    memset(&m_PluginParam, 0, sizeof(m_PluginParam));
    m_PluginParam.MaxThreadNum = 1;
    m_PluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
}

Rotate::~Rotate()
{
    PluginClose();
    Close();
}

/* Methods required for integration with Media SDK */
mfxStatus Rotate::PluginInit(mfxCoreInterface *core)
{
    MSDK_CHECK_POINTER(core, MFX_ERR_NULL_PTR);
    m_mfxCore = MFXCoreInterface(*core);
    return MFX_ERR_NONE;
}

mfxStatus Rotate::PluginClose()
{
    return MFX_ERR_NONE;
}

mfxStatus Rotate::GetPluginParam(mfxPluginParam *par)
{
    MSDK_CHECK_POINTER(par, MFX_ERR_NULL_PTR);

    *par = m_PluginParam;

    return MFX_ERR_NONE;
}

mfxStatus Rotate::Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
{
    MSDK_CHECK_POINTER(in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(out, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(*in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(*out, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(task, MFX_ERR_NULL_PTR);
    MSDK_CHECK_NOT_EQUAL(in_num, 1, MFX_ERR_UNSUPPORTED);
    MSDK_CHECK_NOT_EQUAL(out_num, 1, MFX_ERR_UNSUPPORTED);
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);

    mfxFrameSurface1 *surface_in = (mfxFrameSurface1 *)in[0];
    mfxFrameSurface1 *surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surface_in = surface_in;
    mfxFrameSurface1 *real_surface_out = surface_out;

    mfxStatus sts = MFX_ERR_NONE;

    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.GetRealSurface(surface_in, &real_surface_in);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.GetRealSurface failed");
    }

    if (m_bIsOutOpaque)
    {
        sts = m_mfxCore.GetRealSurface(surface_out, &real_surface_out);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.GetRealSurface failed");
    }

    // check validity of parameters
    sts = CheckInOutFrameInfo(&real_surface_in->Info, &real_surface_out->Info);
    MSDK_CHECK_STATUS(sts, "CheckInOutFrameInfo failed");

    mfxU32 ind = FindFreeTaskIdx();

    if (ind >= m_MaxNumTasks)
    {
        return MFX_WRN_DEVICE_BUSY; // currently there are no free tasks available
    }

    m_mfxCore.IncreaseReference(&(real_surface_in->Data));
    m_mfxCore.IncreaseReference(&(real_surface_out->Data));

    m_pTasks[ind].In = real_surface_in;
    m_pTasks[ind].Out = real_surface_out;
    m_pTasks[ind].bBusy = true;

    switch (m_Param.Angle)
    {
    case 180:
        m_pTasks[ind].pProcessor = new Rotator180;
        MSDK_CHECK_POINTER(m_pTasks[ind].pProcessor, MFX_ERR_MEMORY_ALLOC);
        break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

    m_pTasks[ind].pProcessor->SetAllocator(&m_mfxCore.FrameAllocator());
    m_pTasks[ind].pProcessor->Init(real_surface_in, real_surface_out);

    *task = (mfxThreadTask)&m_pTasks[ind];

    return MFX_ERR_NONE;
}

mfxStatus Rotate::Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;
    RotateTask *current_task = (RotateTask *)task;

    // 0,...,NumChunks - 2 calls return TASK_WORKING, NumChunks - 1,.. return TASK_DONE
    if (uid_a < m_NumChunks)
    {
        // there's data to process
        sts = current_task->pProcessor->Process(&m_pChunks[uid_a]);
        MSDK_CHECK_STATUS(sts, "current_task->pProcessor->Process failed");
        // last call?
        sts = ((m_NumChunks - 1) == uid_a) ? MFX_TASK_DONE : MFX_TASK_WORKING;
    }
    else
    {
        // no data to process
        sts =  MFX_TASK_DONE;
    }

    return sts;
}

mfxStatus Rotate::FreeResources(mfxThreadTask task, mfxStatus sts)
{
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);

    RotateTask *current_task = (RotateTask *)task;

    m_mfxCore.DecreaseReference(&(current_task->In->Data));
    m_mfxCore.DecreaseReference(&(current_task->Out->Data));

    MSDK_SAFE_DELETE(current_task->pProcessor);
    current_task->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus Rotate::Init(mfxVideoParam *mfxParam)
{
    MSDK_CHECK_POINTER(mfxParam, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;
    m_VideoParam = *mfxParam;

    // map opaque surfaces array in case of opaque surfaces
    m_bIsInOpaque = (m_VideoParam.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY) ? true : false;
    m_bIsOutOpaque = (m_VideoParam.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) ? true : false;
    mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = NULL;

    if (m_bIsInOpaque || m_bIsOutOpaque)
    {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(m_VideoParam.ExtParam,
            m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        MSDK_CHECK_POINTER(pluginOpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);
    }

    // check existence of corresponding allocs
    if ((m_bIsInOpaque && ! pluginOpaqueAlloc->In.Surfaces) || (m_bIsOutOpaque && !pluginOpaqueAlloc->Out.Surfaces))
       return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.MapOpaqueSurface(pluginOpaqueAlloc->In.NumSurface,
            pluginOpaqueAlloc->In.Type, pluginOpaqueAlloc->In.Surfaces);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.MapOpaqueSurface failed");
    }

    if (m_bIsOutOpaque)
    {
        sts = m_mfxCore.MapOpaqueSurface(pluginOpaqueAlloc->Out.NumSurface,
            pluginOpaqueAlloc->Out.Type, pluginOpaqueAlloc->Out.Surfaces);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.MapOpaqueSurface failed");
    }

    m_MaxNumTasks = m_VideoParam.AsyncDepth;
    if (m_MaxNumTasks < 2) m_MaxNumTasks = 2;

    m_pTasks = new RotateTask [m_MaxNumTasks];
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_MEMORY_ALLOC);
    memset(m_pTasks, 0, sizeof(RotateTask) * m_MaxNumTasks);

    m_NumChunks = m_PluginParam.MaxThreadNum;
    m_pChunks = new DataChunk [m_NumChunks];
    MSDK_CHECK_POINTER(m_pChunks, MFX_ERR_MEMORY_ALLOC);
    memset(m_pChunks, 0, sizeof(DataChunk) * m_NumChunks);

    // divide frame into data chunks
    mfxU32 num_lines_in_chunk = mfxParam->vpp.In.CropH / m_NumChunks; // integer division
    mfxU32 remainder_lines = mfxParam->vpp.In.CropH % m_NumChunks; // get remainder
    // remaining lines are distributed among first chunks (+ extra 1 line each)
    for (mfxU32 i = 0; i < m_NumChunks; i++)
    {
        m_pChunks[i].StartLine = (i == 0) ? 0 : m_pChunks[i-1].EndLine + 1;
        m_pChunks[i].EndLine = (i < remainder_lines) ? (i + 1) * num_lines_in_chunk : (i + 1) * num_lines_in_chunk - 1;
    }

    m_bInited = true;

    return MFX_ERR_NONE;
}

mfxStatus Rotate::SetAuxParams(void* auxParam, int auxParamSize)
{
    RotateParam *pRotatePar = (RotateParam *)auxParam;
    MSDK_CHECK_POINTER(pRotatePar, MFX_ERR_NULL_PTR);

    // check validity of parameters
    mfxStatus sts = CheckParam(&m_VideoParam, pRotatePar);
    MSDK_CHECK_STATUS(sts, "CheckParam failed");
    m_Param = *pRotatePar;
    return MFX_ERR_NONE;
}

mfxStatus Rotate::Close()
{

    if (!m_bInited)
        return MFX_ERR_NONE;

    memset(&m_Param, 0, sizeof(RotateParam));

    MSDK_SAFE_DELETE_ARRAY(m_pTasks);
    MSDK_SAFE_DELETE_ARRAY(m_pChunks);

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = NULL;

    if (m_bIsInOpaque || m_bIsOutOpaque)
    {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)
            GetExtBuffer(m_VideoParam.ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        MSDK_CHECK_POINTER(pluginOpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);
    }

    // check existence of corresponding allocs
    if ((m_bIsInOpaque && ! pluginOpaqueAlloc->In.Surfaces) || (m_bIsOutOpaque && !pluginOpaqueAlloc->Out.Surfaces))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.UnmapOpaqueSurface(pluginOpaqueAlloc->In.NumSurface,
            pluginOpaqueAlloc->In.Type, pluginOpaqueAlloc->In.Surfaces);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.UnmapOpaqueSurface failed");
    }

    if (m_bIsOutOpaque)
    {
        sts = m_mfxCore.UnmapOpaqueSurface(pluginOpaqueAlloc->Out.NumSurface,
            pluginOpaqueAlloc->Out.Type, pluginOpaqueAlloc->Out.Surfaces);
        MSDK_CHECK_STATUS(sts, "m_mfxCore.UnmapOpaqueSurface failed");
    }

    m_bInited = false;

    return MFX_ERR_NONE;
}

mfxStatus Rotate::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    MSDK_CHECK_POINTER(par, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(out, MFX_ERR_NULL_PTR);

    in->Info = par->vpp.In;
    in->NumFrameSuggested = in->NumFrameMin = par->AsyncDepth + 1;

    out->Info = par->vpp.Out;
    out->NumFrameSuggested = out->NumFrameMin = par->AsyncDepth + 1;

    return MFX_ERR_NONE;
}

/* Internal methods */
mfxU32 Rotate::FindFreeTaskIdx()
{
    mfxU32 i;
    for (i = 0; i < m_MaxNumTasks; i++)
    {
        if (false == m_pTasks[i].bBusy)
        {
            break;
        }
    }

    return i;
}

mfxStatus Rotate::CheckParam(mfxVideoParam *mfxParam, RotateParam *pRotatePar)
{
    MSDK_CHECK_POINTER(mfxParam, MFX_ERR_NULL_PTR);

    mfxInfoVPP *pParam = &mfxParam->vpp;

    // only NV12 color format is supported
    if (MFX_FOURCC_NV12 != pParam->In.FourCC || MFX_FOURCC_NV12 != pParam->Out.FourCC)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus Rotate::CheckInOutFrameInfo(mfxFrameInfo *pIn, mfxFrameInfo *pOut)
{
    MSDK_CHECK_POINTER(pIn, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pOut, MFX_ERR_NULL_PTR);

    if (pIn->CropW != m_VideoParam.vpp.In.CropW || pIn->CropH != m_VideoParam.vpp.In.CropH ||
        pIn->FourCC != m_VideoParam.vpp.In.FourCC ||
        pOut->CropW != m_VideoParam.vpp.Out.CropW || pOut->CropH != m_VideoParam.vpp.Out.CropH ||
        pOut->FourCC != m_VideoParam.vpp.Out.FourCC)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

/* Processor class implementation */
Processor::Processor()
    : m_pIn(NULL)
    , m_pOut(NULL)
    , m_pAlloc(NULL)
{
}

Processor::~Processor()
{
}

mfxStatus Processor::SetAllocator(mfxFrameAllocator *pAlloc)
{
    m_pAlloc = pAlloc;
    return MFX_ERR_NONE;
}

mfxStatus Processor::Init(mfxFrameSurface1 *frame_in, mfxFrameSurface1 *frame_out)
{
    MSDK_CHECK_POINTER(frame_in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(frame_out, MFX_ERR_NULL_PTR);

    m_pIn = frame_in;
    m_pOut = frame_out;

    return MFX_ERR_NONE;
}

mfxStatus Processor::LockFrame(mfxFrameSurface1 *frame)
{
    MSDK_CHECK_POINTER(frame, MFX_ERR_NULL_PTR);

    //no allocator used, no need to do lock
    if (frame->Data.Y != 0 && !frame->Data.MemId)
        return MFX_ERR_NONE;
    //lock required
    MSDK_CHECK_POINTER(m_pAlloc, MFX_ERR_NULL_PTR);
    return m_pAlloc->Lock(m_pAlloc->pthis, frame->Data.MemId, &frame->Data);
}

mfxStatus Processor::UnlockFrame(mfxFrameSurface1 *frame)
{
    MSDK_CHECK_POINTER(frame, MFX_ERR_NULL_PTR);
    //unlock not possible, no allocator used
    if (frame->Data.Y != 0 && frame->Data.MemId ==0)
        return MFX_ERR_NONE;
    //already unlocked
    if (frame->Data.Y == 0)
        return MFX_ERR_NONE;
    //unlock required
    MSDK_CHECK_POINTER(m_pAlloc, MFX_ERR_NULL_PTR);
    return m_pAlloc->Unlock(m_pAlloc->pthis, frame->Data.MemId, &frame->Data);
}


/* 180 degrees rotator class implementation */
Rotator180::Rotator180() : Processor()
{
}

Rotator180::~Rotator180()
{
}

mfxStatus Rotator180::Process(DataChunk *chunk)
{
    MSDK_CHECK_POINTER(chunk, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;
    if (MFX_ERR_NONE != (sts = LockFrame(m_pIn)))return sts;
    if (MFX_ERR_NONE != (sts = LockFrame(m_pOut)))
    {
        UnlockFrame(m_pIn);
        return sts;
    }

    mfxU32 i, j, in_pitch, out_pitch, h, w;

    in_pitch = m_pIn->Data.Pitch;
    out_pitch = m_pOut->Data.Pitch;
    h = m_pIn->Info.CropH;
    w = m_pIn->Info.CropW;

    m_YIn.assign(m_pIn->Data.Y, m_pIn->Data.Y + h * in_pitch);
    m_UVIn.assign(m_pIn->Data.UV, m_pIn->Data.UV + h * in_pitch / 2);

    m_YOut.resize(m_pOut->Info.Height * out_pitch);
    m_UVOut.resize(m_pOut->Info.Height * out_pitch / 2);

    sts = UnlockFrame(m_pIn);
    MSDK_CHECK_STATUS(sts, "UnlockFrame(m_pIn) failed");
    sts = UnlockFrame(m_pOut);
    MSDK_CHECK_STATUS(sts, "UnlockFrame(m_pOut) failed");

    mfxU8 *in_luma = &m_YIn.front() + m_pIn->Info.CropY * in_pitch + m_pIn->Info.CropX;
    mfxU8 *out_luma = &m_YOut.front() + m_pOut->Info.CropY * out_pitch + m_pOut->Info.CropX;

    mfxU8 *in_chroma = &m_UVIn.front() + m_pIn->Info.CropY / 2 * in_pitch + m_pIn->Info.CropX;
    mfxU8 *out_chroma = &m_UVOut.front() + m_pOut->Info.CropY / 2 * out_pitch + m_pOut->Info.CropX;

    mfxU8 *cur_line = 0; // current line in the destination image

    switch (m_pIn->Info.FourCC)
    {
    case MFX_FOURCC_NV12:
          for (i = chunk->StartLine; i <= chunk->EndLine; i++)
          {
              // rotate Y plane
              cur_line = out_luma + (h-1-i) * out_pitch;

              // i-th line images into h-1-i-th line, w bytes in line
              MSDK_MEMCPY_BUF(cur_line, 0, w,  in_luma + i * in_pitch, w);

              // mirror line's elements with respect to the middle element, element=Yj
              for (j = 0; j < w / 2; j++)
              {
                  SWAP_BYTES(cur_line[j], cur_line[w-1-j]);
              }

              // rotate VU plane, contains h/2 lines
              cur_line = out_chroma + (h/2-1-i/2) * out_pitch;

              // i-th line images into h-1-i-th line, w bytes in line
              MSDK_MEMCPY_BUF(cur_line, 0, w, in_chroma + i/2 * in_pitch, w);

              // mirror line's elements with respect to the middle element, element=VjUj
              for (j = 0; j < w/2 - 1; j = j + 2)
              {
                  SWAP_BYTES(cur_line[j], cur_line[w-1-j-1]); // 0 -> -1
                  SWAP_BYTES(cur_line[j+1], cur_line[w-1-j]); // 1 -> -0
              }
          }
          break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

    // copy data from temporary buffer to output surface
    sts = LockFrame(m_pOut);
   MSDK_CHECK_STATUS(sts, "LockFrame(m_pOut) failed");
    MSDK_MEMCPY_BUF(m_pOut->Data.Y, chunk->StartLine * out_pitch, m_YOut.size(), &m_YOut.front(), m_YOut.size());
    MSDK_MEMCPY_BUF(m_pOut->Data.UV, chunk->StartLine * out_pitch, m_UVOut.size(), &m_UVOut.front(), m_UVOut.size());
    sts = UnlockFrame(m_pOut);

    return sts;
}
