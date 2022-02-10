/******************************************************************************\
Copyright (c) 2005-2020, Intel Corporation
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

#include "pipeline_encode.h"
#include "sysmem_allocator.h"
#include "parameters_dumper.h"

#if D3D_SURFACES_SUPPORT
#include "d3d_allocator.h"
#include "d3d11_allocator.h"

#include "d3d_device.h"
#include "d3d11_device.h"
#endif

#ifdef LIBVA_SUPPORT
#include "vaapi_allocator.h"
#include "vaapi_device.h"
#endif

#include "plugin_loader.h"
#include "sample_utils.h"

#if defined (ENABLE_V4L2_SUPPORT)
#include <pthread.h>
#endif

#include "version.h"

#include <algorithm>

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

/* obtain the clock tick of an uninterrupted master clock */
msdk_tick time_get_tick(void)
{
    return msdk_time_get_tick();
}

/* obtain the clock resolution */
msdk_tick time_get_frequency(void)
{
    return msdk_time_get_frequency();
}

void LockPreEncAuxBuffer(PreEncAuxBuffer* pBuff)
{
    if (!pBuff) return;
    msdk_atomic_inc16(&pBuff->Locked);
}

void UnlockPreEncAuxBuffer(PreEncAuxBuffer* pBuff)
{
    if (!pBuff) return;
    msdk_atomic_dec16(&pBuff->Locked);
}


CEncTaskPool::CEncTaskPool()
{
    m_pTasks  = NULL;
    m_pmfxSession       = NULL;
    m_nTaskBufferStart  = 0;
    m_nPoolSize         = 0;
    m_bGpuHangRecovery = false;
}

CEncTaskPool::~CEncTaskPool()
{
    Close();
}

sTask::sTask()
    : pAux(nullptr)
    , EncSyncP(0)
    , pWriter(NULL)

{
}

mfxStatus CEncTaskPool::Init(MFXVideoSession* pmfxSession, CSmplBitstreamWriter* pWriter, mfxU32 nPoolSize, mfxU32 nBufferSize, CSmplBitstreamWriter *pOtherWriter)
{
    MSDK_CHECK_POINTER(pmfxSession, MFX_ERR_NULL_PTR);

    MSDK_CHECK_ERROR(nPoolSize, 0, MFX_ERR_UNDEFINED_BEHAVIOR);
    MSDK_CHECK_ERROR(nBufferSize, 0, MFX_ERR_UNDEFINED_BEHAVIOR);

    // nPoolSize must be even in case of 2 output bitstreams
    if (pOtherWriter && (0 != nPoolSize % 2))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_pmfxSession = pmfxSession;
    m_nPoolSize = nPoolSize;

    m_pTasks = new sTask [m_nPoolSize];
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_MEMORY_ALLOC);

    mfxStatus sts = MFX_ERR_NONE;

    if (pOtherWriter) // 2 bitstreams on output
    {
        for (mfxU32 i = 0; i < m_nPoolSize; i+=2)
        {
            sts = m_pTasks[i+0].Init(nBufferSize, pWriter);
            sts = m_pTasks[i+1].Init(nBufferSize, pOtherWriter);
            MSDK_CHECK_STATUS(sts, "m_pTasks[i+1].Init failed");
        }
    }
    else
    {
        for (mfxU32 i = 0; i < m_nPoolSize; i++)
        {
            sts = m_pTasks[i].Init(nBufferSize, pWriter);
            MSDK_CHECK_STATUS(sts, "m_pTasks[i].Init failed");
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CEncTaskPool::SynchronizeFirstTask()
{
    m_statOverall.StartTimeMeasurement();
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(m_pmfxSession, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts  = MFX_ERR_NONE;
    bool bGpuHang = false;

    // non-null sync point indicates that task is in execution
    if (NULL != m_pTasks[m_nTaskBufferStart].EncSyncP)
    {
        sts = m_pmfxSession->SyncOperation(m_pTasks[m_nTaskBufferStart].EncSyncP, MSDK_WAIT_INTERVAL);
        if (sts == MFX_ERR_GPU_HANG && m_bGpuHangRecovery)
        {
            bGpuHang = true;
            {
            for (mfxU32 i = 0; i < m_nPoolSize; i++)
                if (m_pTasks[i].EncSyncP != NULL)
                {
                    sts = m_pmfxSession->SyncOperation(m_pTasks[i].EncSyncP, 0);//MSDK_WAIT_INTERVAL
                }
            }
            ClearTasks();
            sts = MFX_ERR_NONE;
            msdk_printf(MSDK_STRING("GPU hang happened\n"));
        }
        MSDK_CHECK_STATUS_NO_RET(sts, "SyncOperation failed");

        if (MFX_ERR_NONE == sts)
        {
            m_statFile.StartTimeMeasurement();
            sts = m_pTasks[m_nTaskBufferStart].WriteBitstream();
            m_statFile.StopTimeMeasurement();
            MSDK_CHECK_STATUS(sts, "m_pTasks[m_nTaskBufferStart].WriteBitstream failed");

            sts = m_pTasks[m_nTaskBufferStart].Reset();
            MSDK_CHECK_STATUS(sts, "m_pTasks[m_nTaskBufferStart].Reset failed");

            if(m_pTasks[m_nTaskBufferStart].pAux)
            {
                UnlockPreEncAuxBuffer(m_pTasks[m_nTaskBufferStart].pAux);
            }
            // move task buffer start to the next executing task
            // the first transform frame to the right with non zero sync point
            for (mfxU32 i = 0; i < m_nPoolSize; i++)
            {
                m_nTaskBufferStart = (m_nTaskBufferStart + 1) % m_nPoolSize;
                if (NULL != m_pTasks[m_nTaskBufferStart].EncSyncP)
                {
                    break;
                }
            }
        }
        else if (MFX_ERR_ABORTED == sts)
        {
            while (!m_pTasks[m_nTaskBufferStart].DependentVppTasks.empty())
            {
                // find out if the error occurred in a VPP task to perform recovery procedure if applicable
                sts = m_pmfxSession->SyncOperation(*m_pTasks[m_nTaskBufferStart].DependentVppTasks.begin(), 0);
                if (sts == MFX_ERR_GPU_HANG && m_bGpuHangRecovery)
                {
                    bGpuHang = true;
                    ClearTasks();
                    sts = MFX_ERR_NONE;
                    msdk_printf(MSDK_STRING("GPU hang happened\n"));
                }

                if (MFX_ERR_NONE == sts)
                {
                    m_pTasks[m_nTaskBufferStart].DependentVppTasks.pop_front();
                    sts = MFX_ERR_ABORTED; // save the status of the encode task
                    continue; // go to next vpp task
                }
                else
                {
                    break;
                }
            }
        }

    }
    else
    {
        sts = MFX_ERR_NOT_FOUND; // no tasks left in task buffer
    }
    m_statOverall.StopTimeMeasurement();
    return bGpuHang ? MFX_ERR_GPU_HANG : sts;
}

mfxU32 CEncTaskPool::GetFreeTaskIndex()
{
    mfxU32 off = 0;

    if (m_pTasks)
    {
        for (off = 0; off < m_nPoolSize; off++)
        {
            if (NULL == m_pTasks[(m_nTaskBufferStart + off) % m_nPoolSize].EncSyncP)
            {
                break;
            }
        }
    }

    if (off >= m_nPoolSize)
        return m_nPoolSize;

    return (m_nTaskBufferStart + off) % m_nPoolSize;
}

mfxStatus CEncTaskPool::GetFreeTask(sTask **ppTask)
{
    MSDK_CHECK_POINTER(ppTask, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_NOT_INITIALIZED);

    mfxU32 index = GetFreeTaskIndex();

    if (index >= m_nPoolSize)
    {
        return MFX_ERR_NOT_FOUND;
    }

    // return the address of the task
    *ppTask = &m_pTasks[index];

    return MFX_ERR_NONE;
}

void CEncTaskPool::Close()
{
    if (m_pTasks)
    {
        for (mfxU32 i = 0; i < m_nPoolSize; i++)
        {
            m_pTasks[i].Close();
        }
    }

    MSDK_SAFE_DELETE_ARRAY(m_pTasks);

    m_pmfxSession = NULL;
    m_nTaskBufferStart = 0;
    m_nPoolSize = 0;
}

void CEncTaskPool::SetGpuHangRecoveryFlag()
{
    m_bGpuHangRecovery = true;
}

void CEncTaskPool::ClearTasks()
{
    for (size_t i = 0; i < m_nPoolSize; i++)
    {
        m_pTasks[i].Reset();
    }
    m_nTaskBufferStart = 0;
}

mfxStatus sTask::Init(mfxU32 nBufferSize, CSmplBitstreamWriter *pwriter)
{
    Close();

    pWriter = pwriter;

    mfxStatus sts = Reset();
    MSDK_CHECK_STATUS(sts, "Reset failed");

    mfxBS.Extend(nBufferSize);

    return sts;
}

mfxStatus sTask::Close()
{
    EncSyncP = 0;
    DependentVppTasks.clear();

    return MFX_ERR_NONE;
}

mfxStatus sTask::WriteBitstream()
{
    if (pWriter)
        return pWriter->WriteNextFrame(&mfxBS);
    else
        return MFX_ERR_NONE;
}

mfxStatus sTask::Reset()
{
    // mark sync point as free
    EncSyncP = NULL;

    // prepare bit stream
    mfxBS.DataOffset = 0;
    mfxBS.DataLength = 0;

    DependentVppTasks.clear();

    return MFX_ERR_NONE;
}

void CEncodingPipeline::InitExtMVCBuffers(mfxExtMVCSeqDesc *mvcBuffer) const
{
    // a simple example of mfxExtMVCSeqDesc structure filling
    // actually equal to the "Default dependency mode" - when the structure fields are left 0,
    // but we show how to properly allocate and fill the fields

    mfxU32 i;

    // mfxMVCViewDependency array
    mvcBuffer->NumView = m_nNumView;
    mvcBuffer->NumViewAlloc = m_nNumView;
    mvcBuffer->View = new mfxMVCViewDependency[mvcBuffer->NumViewAlloc];

    for (i = 0; i < mvcBuffer->NumViewAlloc; ++i)
    {
        mvcBuffer->View[i] = {};
        mvcBuffer->View[i].ViewId = (mfxU16) i; // set view number as view id
    }

    // set up dependency for second view
    mvcBuffer->View[1].NumAnchorRefsL0 = 1;
    mvcBuffer->View[1].AnchorRefL0[0] = 0;     // ViewId 0 - base view

    mvcBuffer->View[1].NumNonAnchorRefsL0 = 1;
    mvcBuffer->View[1].NonAnchorRefL0[0] = 0;  // ViewId 0 - base view

    // viewId array
    mvcBuffer->NumViewId = m_nNumView;
    mvcBuffer->NumViewIdAlloc = m_nNumView;
    mvcBuffer->ViewId = new mfxU16[mvcBuffer->NumViewIdAlloc];

    for (i = 0; i < mvcBuffer->NumViewIdAlloc; ++i)
    {
        mvcBuffer->ViewId[i] = (mfxU16) i;
    }

    // create a single operation point containing all views
    mvcBuffer->NumOP = 1;
    mvcBuffer->NumOPAlloc = 1;
    mvcBuffer->OP = new mfxMVCOperationPoint[mvcBuffer->NumOPAlloc];

    for (i = 0; i < mvcBuffer->NumOPAlloc; ++i)
    {
        mvcBuffer->OP[i] = {};
        mvcBuffer->OP[i].NumViews = (mfxU16) m_nNumView;
        mvcBuffer->OP[i].NumTargetViews = (mfxU16) m_nNumView;
        mvcBuffer->OP[i].TargetViewId = mvcBuffer->ViewId; // points to mfxExtMVCSeqDesc::ViewId
    }
}

mfxStatus CEncodingPipeline::InitVppFilters()
{
    auto vppExtParams = m_mfxVppParams.AddExtBuffer<mfxExtVPPDoNotUse>();

    vppExtParams->NumAlg = 4;

    if(vppExtParams->AlgList)
        delete[] vppExtParams->AlgList;
    vppExtParams->AlgList = new mfxU32 [vppExtParams->NumAlg];

    vppExtParams->AlgList[0] = MFX_EXTBUFF_VPP_DENOISE; // turn off denoising (on by default)
    vppExtParams->AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS; // turn off scene analysis (on by default)
    vppExtParams->AlgList[2] = MFX_EXTBUFF_VPP_DETAIL; // turn off detail enhancement (on by default)
    vppExtParams->AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP; // turn off processing amplified (on by default)

    if(MVC_ENABLED & m_MVCflags)
    {
        auto mvcBuffer = m_mfxVppParams.AddExtBuffer<mfxExtMVCSeqDesc>();
        InitExtMVCBuffers(mvcBuffer);
    }

    return MFX_ERR_NONE;

} // CEncodingPipeline::InitVppFilters()

void CEncodingPipeline::FreeVppFilters()
{
    auto vppExtParams = m_mfxVppParams.GetExtBuffer<mfxExtVPPDoNotUse>();
    if(vppExtParams)
        MSDK_SAFE_DELETE_ARRAY(vppExtParams->AlgList);
}

void CEncodingPipeline::DeallocateExtMVCBuffers()
{
    auto mvcBuffer = m_mfxEncParams.GetExtBuffer<mfxExtMVCSeqDesc>();
    if(mvcBuffer)
    {
        MSDK_SAFE_DELETE_ARRAY(mvcBuffer->View);
        MSDK_SAFE_DELETE_ARRAY(mvcBuffer->ViewId);
        MSDK_SAFE_DELETE_ARRAY(mvcBuffer->OP);
    }

}

PreEncAuxBuffer*  CEncodingPipeline::GetFreePreEncAuxBuffer()
{
    for(mfxU32 i = 0; i < m_PreEncAuxPool.size(); i++)
    {
        if (!m_PreEncAuxPool[i].Locked)
            return &(m_PreEncAuxPool[i]);
    }
    return nullptr;
}

mfxStatus CEncodingPipeline::InitMfxPreEncParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(pInParams->bEnableExtLA, false, MFX_ERR_INVALID_VIDEO_PARAM);

    m_mfxPreEncParams.mfx = {};
    // Plugin needs AVC for init
    m_mfxPreEncParams.mfx.CodecId     = MFX_CODEC_AVC;
    m_mfxPreEncParams.mfx.TargetUsage = pInParams->nTargetUsage;
    m_mfxPreEncParams.mfx.FrameInfo   = m_pmfxVPP ? m_mfxVppParams.vpp.Out : m_mfxEncParams.mfx.FrameInfo;
    m_mfxPreEncParams.AsyncDepth      = pInParams->nAsyncDepth;
    m_mfxPreEncParams.IOPattern       = m_mfxEncParams.IOPattern;

    // configure and attach external parameters
    auto laCtrl                 = m_mfxPreEncParams.AddExtBuffer<mfxExtLAControl>();
    laCtrl->LookAheadDepth      = pInParams->nLADepth ? pInParams->nLADepth : 40;
    laCtrl->NumOutStream        = 1;
    laCtrl->OutStream[0].Width  = m_mfxPreEncParams.mfx.FrameInfo.Width;
    laCtrl->OutStream[0].Height = m_mfxPreEncParams.mfx.FrameInfo.Height;

    m_mfxPreEncParams.mfx.GopPicSize  = pInParams->nGopPicSize ? pInParams->nGopPicSize : 1500;
    m_mfxPreEncParams.mfx.GopRefDist  = pInParams->nGopRefDist;
    m_mfxPreEncParams.mfx.TargetUsage = pInParams->nTargetUsage;

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::InitMfxEncParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    m_mfxEncParams.mfx.CodecId                 = pInParams->CodecId;
    m_mfxEncParams.mfx.TargetUsage             = pInParams->nTargetUsage; // trade-off between quality and speed
    m_mfxEncParams.mfx.RateControlMethod       = pInParams->nRateControlMethod;
    m_mfxEncParams.mfx.GopRefDist              = pInParams->nGopRefDist;
    m_mfxEncParams.mfx.GopPicSize              = pInParams->nGopPicSize;
    m_mfxEncParams.mfx.NumRefFrame             = pInParams->nNumRefFrame;
    m_mfxEncParams.mfx.IdrInterval             = pInParams->nIdrInterval;

    m_mfxEncParams.mfx.CodecProfile            = pInParams->CodecProfile;
    m_mfxEncParams.mfx.CodecLevel              = pInParams->CodecLevel;
    m_mfxEncParams.mfx.MaxKbps                 = (mfxU16)pInParams->MaxKbps;
    m_mfxEncParams.mfx.InitialDelayInKB        = (mfxU16)pInParams->InitialDelayInKB;
    m_mfxEncParams.mfx.GopOptFlag              = pInParams->GopOptFlag;
    m_mfxEncParams.mfx.BufferSizeInKB          = (mfxU16)pInParams->BufferSizeInKB;
    m_mfxEncParams.mfx.BRCParamMultiplier      = pInParams->nBitRateMultiplier;

    if (m_mfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        m_mfxEncParams.mfx.QPI = pInParams->nQPI;
        m_mfxEncParams.mfx.QPP = pInParams->nQPP;
        m_mfxEncParams.mfx.QPB = pInParams->nQPB;
    }
    else if (m_mfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_ICQ ||
             m_mfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ)
    {
        m_mfxEncParams.mfx.ICQQuality = pInParams->ICQQuality;
    }
    else if (m_mfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_AVBR)
    {
        m_mfxEncParams.mfx.Accuracy    = pInParams->Accuracy;
        m_mfxEncParams.mfx.TargetKbps  = (mfxU16)pInParams->nBitRate;
        m_mfxEncParams.mfx.Convergence = pInParams->Convergence;
    }
    else
    {
        m_mfxEncParams.mfx.TargetKbps = (mfxU16)pInParams->nBitRate; // in Kbps
    }

    if(pInParams->enableQSVFF)
    {
        m_mfxEncParams.mfx.LowPower = MFX_CODINGOPTION_ON;
    }

    m_mfxEncParams.mfx.NumSlice = pInParams->nNumSlice;
    ConvertFrameRate(pInParams->dFrameRate, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtN, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtD);

    m_bQPFileMode = pInParams->QPFileMode;
    m_mfxEncParams.mfx.EncodedOrder = m_bQPFileMode || m_pmfxPreENC ? 1 : 0; // binary flag, 0 signals encoder to take frames in display order

    if (m_bQPFileMode)
    {
        mfxStatus sts = m_QPFileReader.Read(pInParams->strQPFilePath, pInParams->CodecId);
        MSDK_CHECK_STATUS(sts, m_QPFileReader.GetErrorMessage().c_str());
    }
    m_bTCBRCFileMode = pInParams->TCBRCFileMode;
    if (m_bTCBRCFileMode)
    {
        mfxStatus sts = m_TCBRCFileReader.Read(pInParams->strTCBRCFilePath, pInParams->CodecId);
        MSDK_CHECK_STATUS(sts, m_TCBRCFileReader.GetErrorMessage().c_str());
        m_mfxEncParams.AsyncDepth = 1;
        m_mfxEncParams.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        m_mfxEncParams.mfx.GopRefDist = 1;
        pInParams->nNalHrdConformance = MFX_CODINGOPTION_OFF;
        pInParams->LowDelayBRC = MFX_CODINGOPTION_ON;
    }

    // specify memory type
    if (D3D9_MEMORY == pInParams->memType || D3D11_MEMORY == pInParams->memType)
    {
        m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }
    else
    {
        m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    }

    // frame info parameters
    m_mfxEncParams.mfx.FrameInfo.FourCC       = pInParams->EncodeFourCC;
    m_mfxEncParams.mfx.FrameInfo.ChromaFormat = FourCCToChroma(pInParams->EncodeFourCC);
    m_mfxEncParams.mfx.FrameInfo.PicStruct    = pInParams->nPicStruct;
    m_mfxEncParams.mfx.FrameInfo.Shift        = pInParams->shouldUseShifted10BitEnc;

    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    m_mfxEncParams.mfx.FrameInfo.Width  = MSDK_ALIGN16(pInParams->nDstWidth);
    m_mfxEncParams.mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxEncParams.mfx.FrameInfo.PicStruct)?
        MSDK_ALIGN16(pInParams->nDstHeight) : MSDK_ALIGN32(pInParams->nDstHeight);

    m_mfxEncParams.mfx.FrameInfo.CropX = 0;
    m_mfxEncParams.mfx.FrameInfo.CropY = 0;
    m_mfxEncParams.mfx.FrameInfo.CropW = pInParams->nDstWidth;
    m_mfxEncParams.mfx.FrameInfo.CropH = pInParams->nDstHeight;

    if (pInParams->nEncTileRows && pInParams->nEncTileCols)
    {
        if (m_mfxEncParams.mfx.CodecId == MFX_CODEC_HEVC)
        {
            auto hevcTiles = m_mfxEncParams.AddExtBuffer<mfxExtHEVCTiles>();
            hevcTiles->NumTileRows = pInParams->nEncTileRows;
            hevcTiles->NumTileColumns = pInParams->nEncTileCols;
        }
#if (MFX_VERSION >= 1029)
        else if (m_mfxEncParams.mfx.CodecId == MFX_CODEC_VP9)
        {
            auto vp9Param = m_mfxEncParams.AddExtBuffer<mfxExtVP9Param>();
            vp9Param->NumTileRows    = pInParams->nEncTileRows;
            vp9Param->NumTileColumns = pInParams->nEncTileCols;
        }
#endif
    }
    if (*pInParams->uSEI && (pInParams->CodecId == MFX_CODEC_AVC || pInParams->CodecId == MFX_CODEC_HEVC))
    {
        auto pl = new mfxPayload;
        mfxU16 size = 0;
        std::vector<mfxU8> data;
        std::vector<mfxU8> uuid;
        std::string msg;
        mfxU16 type = 5; //user data unregister
        pl->CtrlFlags = 0;

        mfxU16 index = 0;
        for(; index < 16; index++)
        {
            mfxU8 bt = Char2Hex(pInParams->uSEI[index * 2]) * 16 +
                Char2Hex(pInParams->uSEI[index * 2 + 1]);
            uuid.push_back(bt);
        }
        index = 32; //32 characters for uuid

        if(msdk_strlen(pInParams->uSEI) > index)
        {
            index++; //skip the delimiter
            if(pInParams->CodecId == MFX_CODEC_HEVC)
            {
                pl->CtrlFlags = pInParams->uSEI[index++] == '0' ? 0 : MFX_PAYLOAD_CTRL_SUFFIX;
            }
            else
            {
                pl->CtrlFlags = pInParams->uSEI[index++] == '0';
            }
        }
        if(msdk_strlen(pInParams->uSEI) > index)
        {
            index++;

            // Converting to byte-string if necessary
            msdk_tstring tstr(pInParams->uSEI+index);
            std::transform(tstr.begin(), tstr.end(), std::back_inserter(msg), [](msdk_char c) {
                return (char)c;
            });
        }
        size = (mfxU16)uuid.size();
        size += (mfxU16)msg.length();
        SEICalcSizeType(data, type, size);
        mfxU16 calc_size = (mfxU16)data.size();
        size += calc_size;
        pl->BufSize = size;
        pl->Data    = new mfxU8[pl->BufSize];
        pl->NumBit  = pl->BufSize * 8;
        pl->Type    = type;

        mfxU8* p = pl->Data;
        memcpy(p, data.data(), calc_size);
        p += calc_size;
        memcpy(p, uuid.data(), uuid.size());
        p += uuid.size();
        memcpy(p, msg.c_str(), msg.length());

        m_UserDataUnregSEI.push_back(pl);
    }

    // we don't specify profile and level and let the encoder choose those basing on parameters
    // we must specify profile only for MVC codec
    if (MVC_ENABLED & m_MVCflags)
    {
        m_mfxEncParams.mfx.CodecProfile = MFX_PROFILE_AVC_STEREO_HIGH;
    }

    // configure and attach external parameters
    if (MVC_ENABLED & pInParams->MVC_flags)
    {
        m_mfxEncParams.AddExtBuffer<mfxExtMVCSeqDesc>();
    }

    if (MVC_VIEWOUTPUT & pInParams->MVC_flags)
    {
        auto codingOption = m_mfxEncParams.AddExtBuffer<mfxExtCodingOption>();
        // ViewOuput option requested
        codingOption->ViewOutput = MFX_CODINGOPTION_ON;
    }

    if (pInParams->nPicTimingSEI || pInParams->nNalHrdConformance || pInParams->nVuiNalHrdParameters)
    {
        auto codingOption = m_mfxEncParams.AddExtBuffer<mfxExtCodingOption>();
        codingOption->PicTimingSEI        = pInParams->nPicTimingSEI;
        codingOption->NalHrdConformance   = pInParams->nNalHrdConformance;
        codingOption->VuiNalHrdParameters = pInParams->nVuiNalHrdParameters;
    }


    // configure the depth of the look ahead BRC if specified in command line
    if (pInParams->nLADepth || pInParams->nMaxSliceSize || pInParams->nMaxFrameSize || pInParams->nBRefType ||
        (pInParams->BitrateLimit && pInParams->CodecId == MFX_CODEC_AVC) ||
        (pInParams->nExtBRC && (pInParams->CodecId == MFX_CODEC_HEVC || pInParams->CodecId == MFX_CODEC_AVC)) ||
        pInParams->IntRefType || pInParams->IntRefCycleSize || pInParams->IntRefQPDelta ||
        pInParams->AdaptiveI || pInParams->AdaptiveB)
    {
        auto codingOption2 = m_mfxEncParams.AddExtBuffer<mfxExtCodingOption2>();

        codingOption2->LookAheadDepth = pInParams->nLADepth;
        codingOption2->MaxSliceSize   = pInParams->nMaxSliceSize;
        codingOption2->MaxFrameSize = pInParams->nMaxFrameSize;
        codingOption2->BRefType = pInParams->nBRefType;
        codingOption2->BitrateLimit = pInParams->BitrateLimit;

        if (pInParams->nExtBRC != EXTBRC_DEFAULT && (pInParams->CodecId == MFX_CODEC_HEVC || pInParams->CodecId == MFX_CODEC_AVC))
        {
            codingOption2->ExtBRC = (mfxU16)(pInParams->nExtBRC == EXTBRC_OFF ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON);
        }
        else
        {
            codingOption2->ExtBRC = 0;
        }

        codingOption2->IntRefType = pInParams->IntRefType;
        codingOption2->IntRefCycleSize = pInParams->IntRefCycleSize;
        codingOption2->IntRefQPDelta = pInParams->IntRefQPDelta;
        codingOption2->AdaptiveI = pInParams->AdaptiveI;
        codingOption2->AdaptiveB = pInParams->AdaptiveB;
    }

#if (MFX_VERSION >= 1024)
    // This is for explicit extbrc only. In case of implicit (built-into-library) version - we don't need this extended buffer
    if (pInParams->nExtBRC == EXTBRC_ON && (pInParams->CodecId == MFX_CODEC_HEVC || pInParams->CodecId == MFX_CODEC_AVC))
    {
        auto extBRC = m_mfxEncParams.AddExtBuffer<mfxExtBRC>();
        HEVCExtBRC::Create(*extBRC);
    }
#endif

    // set up mfxCodingOption3
    if (pInParams->nGPB || pInParams->LowDelayBRC || pInParams->WeightedPred || pInParams->WeightedBiPred
        || pInParams->nPRefType || pInParams->IntRefCycleDist || pInParams->nAdaptiveMaxFrameSize
        || pInParams->nNumRefActiveP || pInParams->nNumRefActiveBL0 || pInParams->nNumRefActiveBL1
        || pInParams->ExtBrcAdaptiveLTR || pInParams->QVBRQuality || pInParams->WinBRCSize
#if (MFX_VERSION >= 1027)
        || pInParams->TargetBitDepthLuma || pInParams->TargetBitDepthChroma
#endif
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        || pInParams->DeblockingAlphaTcOffset || pInParams->DeblockingBetaOffset
#endif
        || pInParams->WinBRCMaxAvgKbps || pInParams->nTransformSkip)
    {
        auto codingOption3 = m_mfxEncParams.AddExtBuffer<mfxExtCodingOption3>();
        if (pInParams->CodecId == MFX_CODEC_HEVC)
        {
            codingOption3->GPB = pInParams->nGPB;
            std::fill(codingOption3->NumRefActiveP,   codingOption3->NumRefActiveP + 8,   pInParams->nNumRefActiveP);
            std::fill(codingOption3->NumRefActiveBL0, codingOption3->NumRefActiveBL0 + 8, pInParams->nNumRefActiveBL0);
            std::fill(codingOption3->NumRefActiveBL1, codingOption3->NumRefActiveBL1 + 8, pInParams->nNumRefActiveBL1);
            #if (MFX_VERSION >= 1026)
                codingOption3->TransformSkip = pInParams->nTransformSkip;
            #endif
        }

        codingOption3->WeightedPred   = pInParams->WeightedPred;
        codingOption3->WeightedBiPred = pInParams->WeightedBiPred;
#if (MFX_VERSION >= 1023)
        codingOption3->LowDelayBRC    = pInParams->LowDelayBRC;
#endif
        codingOption3->PRefType       = pInParams->nPRefType;
        codingOption3->IntRefCycleDist= pInParams->IntRefCycleDist;
        codingOption3->AdaptiveMaxFrameSize = pInParams->nAdaptiveMaxFrameSize;
        codingOption3->QVBRQuality    = pInParams->QVBRQuality;
#if (MFX_VERSION >= 1026)
        codingOption3->ExtBrcAdaptiveLTR = pInParams->ExtBrcAdaptiveLTR;
#endif

#if (MFX_VERSION >= 1027)
        codingOption3->TargetBitDepthLuma = pInParams->TargetBitDepthLuma;
        codingOption3->TargetBitDepthChroma = pInParams->TargetBitDepthChroma;
#endif
        codingOption3->WinBRCSize = pInParams->WinBRCSize;
        codingOption3->WinBRCMaxAvgKbps = pInParams->WinBRCMaxAvgKbps;

#if (MFX_VERSION >= MFX_VERSION_NEXT)
        if (pInParams->DeblockingAlphaTcOffset || pInParams->DeblockingBetaOffset)
        {
            codingOption3->DeblockingAlphaTcOffset = pInParams->DeblockingAlphaTcOffset;
            codingOption3->DeblockingBetaOffset = pInParams->DeblockingBetaOffset;
        }
#endif
    }

    if (pInParams->nAvcTemp)
    {
        auto avcTemporalLayers = m_mfxEncParams.AddExtBuffer<mfxExtAvcTemporalLayers>();

        avcTemporalLayers->BaseLayerPID = pInParams->nBaseLayerPID;
        for (int i = 0; i < 8; i++)
        {
            avcTemporalLayers->Layer[i].Scale = pInParams->nAvcTemporalLayers[i];
        }
    }

    if (pInParams->nSPSId || pInParams->nPPSId)
    {
        if (pInParams->CodecId == MFX_CODEC_HEVC)
        {
            auto codingOptionSPSPPS = m_mfxEncParams.AddExtBuffer<mfxExtCodingOptionSPSPPS>();
            codingOptionSPSPPS->SPSId = pInParams->nSPSId;
            codingOptionSPSPPS->PPSId = pInParams->nPPSId;
        }
    }

    // In case of HEVC when height and/or width divided with 8 but not divided with 16
    // add extended parameter to increase performance
    if ( ( !((m_mfxEncParams.mfx.FrameInfo.CropW & 15 ) ^ 8 ) ||
        !((m_mfxEncParams.mfx.FrameInfo.CropH & 15 ) ^ 8 ) ) &&
        (m_mfxEncParams.mfx.CodecId == MFX_CODEC_HEVC) && !m_bIsFieldSplitting)
    {
        auto encHEVCParam = m_mfxEncParams.AddExtBuffer<mfxExtHEVCParam>();
        encHEVCParam->PicWidthInLumaSamples = m_mfxEncParams.mfx.FrameInfo.CropW;
        encHEVCParam->PicHeightInLumaSamples = m_mfxEncParams.mfx.FrameInfo.CropH;
    }

    if (pInParams->TransferMatrix)
    {
        auto videoSignalInfo = m_mfxEncParams.AddExtBuffer<mfxExtVideoSignalInfo>();
        videoSignalInfo->ColourDescriptionPresent = 1;
        videoSignalInfo->MatrixCoefficients = pInParams->TransferMatrix;
    }

    // JPEG encoder settings overlap with other encoders settings in mfxInfoMFX structure
    if (MFX_CODEC_JPEG == pInParams->CodecId)
    {
        m_mfxEncParams.mfx.Interleaved = 1;
        m_mfxEncParams.mfx.Quality = pInParams->nQuality;
        m_mfxEncParams.mfx.RestartInterval = 0;
        MSDK_ZERO_MEMORY(m_mfxEncParams.mfx.reserved5);
    }

    m_mfxEncParams.AsyncDepth = pInParams->nAsyncDepth;

    return MFX_ERR_NONE;
}

mfxU32 CEncodingPipeline::FileFourCC2EncFourCC(mfxU32 fcc)
{
    // File reader automatically converts I420 and YV12 to NV12
    if (fcc == MFX_FOURCC_I420 || fcc == MFX_FOURCC_YV12)
        return MFX_FOURCC_NV12;
    else
        return fcc;
}

mfxStatus CEncodingPipeline::InitMfxVppParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    // specify memory type
    if (D3D9_MEMORY == pInParams->memType || D3D11_MEMORY == pInParams->memType)
    {
        m_mfxVppParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    }
    else
    {
        m_mfxVppParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }

    m_mfxVppParams.vpp.In.PicStruct = pInParams->nPicStruct;
    ConvertFrameRate(pInParams->dFrameRate, &m_mfxVppParams.vpp.In.FrameRateExtN, &m_mfxVppParams.vpp.In.FrameRateExtD);

    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    m_mfxVppParams.vpp.In.Width     = MSDK_ALIGN16(pInParams->nWidth);
    m_mfxVppParams.vpp.In.Height    = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppParams.vpp.In.PicStruct)?
        MSDK_ALIGN16(pInParams->nHeight) : MSDK_ALIGN32(pInParams->nHeight);

    // set crops in input mfxFrameInfo for correct work of file reader
    // VPP itself ignores crops at initialization
    m_mfxVppParams.vpp.In.CropW = pInParams->nWidth;
    m_mfxVppParams.vpp.In.CropH = pInParams->nHeight;

    // fill output frame info
    MSDK_MEMCPY_VAR(m_mfxVppParams.vpp.Out,&m_mfxVppParams.vpp.In, sizeof(mfxFrameInfo));

    m_mfxVppParams.vpp.In.FourCC    = FileFourCC2EncFourCC(m_InputFourCC);

    m_mfxVppParams.vpp.Out.FourCC    = pInParams->EncodeFourCC;
    m_mfxVppParams.vpp.In.ChromaFormat =  FourCCToChroma(m_mfxVppParams.vpp.In.FourCC);
    m_mfxVppParams.vpp.Out.ChromaFormat = FourCCToChroma(m_mfxVppParams.vpp.Out.FourCC);

    // Fill Shift bit
    m_mfxVppParams.vpp.In.Shift = pInParams->shouldUseShifted10BitVPP;
    m_mfxVppParams.vpp.Out.Shift = pInParams->shouldUseShifted10BitEnc; // This output should correspond to Encoder settings

    // input frame info
#if defined (ENABLE_V4L2_SUPPORT)
    if (isV4L2InputEnabled)
    {
        m_mfxVppParams.vpp.In.FourCC = v4l2Pipeline.ConvertToMFXFourCC(pInParams->v4l2Format);
        m_mfxVppParams.vpp.In.ChromaFormat = FourCCToChroma(m_mfxVppParams.vpp.In.FourCC);

        m_mfxVppParams.vpp.Out.FourCC    = MFX_FOURCC_NV12;
        m_mfxVppParams.vpp.Out.ChromaFormat = FourCCToChroma(m_mfxVppParams.vpp.Out.FourCC);
    }
#endif

    // only resizing is supported
    m_mfxVppParams.vpp.Out.Width = MSDK_ALIGN16(pInParams->nDstWidth);
    m_mfxVppParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppParams.vpp.Out.PicStruct)?
        MSDK_ALIGN16(pInParams->nDstHeight) : MSDK_ALIGN32(pInParams->nDstHeight);

    // configure and attach external parameters
    InitVppFilters();

    m_mfxVppParams.AsyncDepth = pInParams->nAsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::CreateHWDevice()
{
    mfxStatus sts = MFX_ERR_NONE;
#if D3D_SURFACES_SUPPORT
#if MFX_D3D11_SUPPORT
    if (D3D11_MEMORY == m_memType)
        m_hwdev = new CD3D11Device();
    else
#endif // #if MFX_D3D11_SUPPORT
        m_hwdev = new CD3D9Device();

    if (NULL == m_hwdev)
        return MFX_ERR_MEMORY_ALLOC;

    sts = m_hwdev->Init(
        NULL,
        0,
        MSDKAdapter::GetNumber(GetFirstSession()));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");

#elif LIBVA_SUPPORT
    m_hwdev = CreateVAAPIDevice(m_strDevicePath);
    if (NULL == m_hwdev)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    sts = m_hwdev->Init(NULL, 0, MSDKAdapter::GetNumber(GetFirstSession()));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");
#endif
    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::ResetDevice()
{
    if (D3D9_MEMORY == m_memType || D3D11_MEMORY == m_memType)
    {
        return m_hwdev->Reset();
    }
    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocFrames()
{
    MSDK_CHECK_POINTER(GetFirstEncoder(), MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxFrameAllocRequest PreEncRequest;
    mfxFrameAllocRequest VppRequest[2];

    mfxU16 nEncSurfNum = 0; // number of surfaces for encoder
    mfxU16 nVppSurfNum = 0; // number of surfaces for vpp
    mfxU16 nPreEncSurfNum = 0; // number of surfaces for preenc

    MSDK_ZERO_MEMORY(EncRequest);
    MSDK_ZERO_MEMORY(PreEncRequest);
    MSDK_ZERO_MEMORY(VppRequest[0]);
    MSDK_ZERO_MEMORY(VppRequest[1]);

    mfxU16 initialTargetKbps = m_mfxEncParams.mfx.TargetKbps;
    auto co2 = m_mfxEncParams.GetExtBuffer<mfxExtCodingOption2>();

    // Querying encoder
    sts = GetFirstEncoder()->Query(&m_mfxEncParams, &m_mfxEncParams);
    MSDK_CHECK_STATUS(sts, "Query (for encoder) failed");

    if (co2 && co2->BitrateLimit != MFX_CODINGOPTION_OFF && initialTargetKbps != m_mfxEncParams.mfx.TargetKbps)
    {
        msdk_printf(MSDK_STRING("WARNING: -BitrateLimit:on, target bitrate was changed from %d kbps to %d kbps.\n"), initialTargetKbps, m_mfxEncParams.mfx.TargetKbps);
    }

    // Calculate the number of surfaces for components.
    // QueryIOSurf functions tell how many surfaces are required to produce at least 1 output.
    // To achieve better performance we provide extra surfaces.
    // 1 extra surface at input allows to get 1 extra output.
    sts = GetFirstEncoder()->QueryIOSurf(&m_mfxEncParams, &EncRequest);
    MSDK_CHECK_STATUS(sts, "QueryIOSurf (for encoder) failed");

    if (!m_pmfxVPP)
    {
        EncRequest.NumFrameMin = EncRequest.NumFrameSuggested = std::max(EncRequest.NumFrameSuggested, m_nPerfOpt);
    }

    if (EncRequest.NumFrameSuggested < m_mfxEncParams.AsyncDepth)
        return MFX_ERR_MEMORY_ALLOC;

    // The number of surfaces shared by vpp output and encode input.
    nEncSurfNum = EncRequest.NumFrameSuggested;

    if (m_pmfxVPP)
    {
        // Querying VPP
        sts = m_pmfxVPP->Query(&m_mfxVppParams, &m_mfxVppParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Query failed");

        // VppRequest[0] for input frames request, VppRequest[1] for output frames request
        sts = m_pmfxVPP->QueryIOSurf(&m_mfxVppParams, VppRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

        VppRequest[0].NumFrameMin = VppRequest[0].NumFrameSuggested = std::max(VppRequest[0].NumFrameSuggested, m_nPerfOpt);
        // The number of surfaces for vpp input - so that vpp can work at async depth = m_nAsyncDepth
        nVppSurfNum = VppRequest[0].NumFrameSuggested;
        // If surfaces are shared by 2 components, c1 and c2. NumSurf = c1_out + c2_in - AsyncDepth + 1
        nEncSurfNum += nVppSurfNum - m_mfxEncParams.AsyncDepth + 1;
    }

    if (m_pmfxPreENC)
    {
        // Querying PreEnc
        sts = m_pmfxPreENC->Query(&m_mfxPreEncParams, &m_mfxPreEncParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->Query failed");

        sts = m_pmfxPreENC->QueryIOSurf(&m_mfxPreEncParams, &PreEncRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->QueryIOSurf failed");

        nPreEncSurfNum = PreEncRequest.NumFrameSuggested;
        nEncSurfNum += nPreEncSurfNum - m_mfxEncParams.AsyncDepth + 1;
    }

    // prepare allocation requests
    EncRequest.NumFrameSuggested = EncRequest.NumFrameMin = nEncSurfNum;
    MSDK_MEMCPY_VAR(EncRequest.Info, &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
    if (m_pmfxVPP)
    {
        EncRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT; // surfaces are shared between vpp output and encode input
    }

    // alloc frames for encoder
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &EncRequest, &m_EncResponse);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

    // alloc frames for vpp if vpp is enabled
    if (m_pmfxVPP)
    {
        VppRequest[0].NumFrameSuggested = VppRequest[0].NumFrameMin = nVppSurfNum;
        MSDK_MEMCPY_VAR(VppRequest[0].Info, &(m_mfxVppParams.vpp.In), sizeof(mfxFrameInfo));

#if defined (ENABLE_V4L2_SUPPORT)
        if (isV4L2InputEnabled)
        {
            VppRequest[0].Type |= MFX_MEMTYPE_EXPORT_FRAME;
        }
#endif

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(VppRequest[0]), &m_VppResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");
    }

    // prepare mfxFrameSurface1 array for encoder
    m_pEncSurfaces = new mfxFrameSurface1 [m_EncResponse.NumFrameActual];
    MSDK_CHECK_POINTER(m_pEncSurfaces, MFX_ERR_MEMORY_ALLOC);

    for (int i = 0; i < m_EncResponse.NumFrameActual; i++)
    {
        memset(&(m_pEncSurfaces[i]), 0, sizeof(mfxFrameSurface1));
        MSDK_MEMCPY_VAR(m_pEncSurfaces[i].Info, &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

        if (m_bExternalAlloc)
        {
            m_pEncSurfaces[i].Data.MemId = m_EncResponse.mids[i];
        }
        else
        {
            // get YUV pointers
            sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_EncResponse.mids[i], &(m_pEncSurfaces[i].Data));
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
        }
    }

    // prepare mfxFrameSurface1 array for vpp if vpp is enabled
    if (m_pmfxVPP)
    {
        m_pVppSurfaces = new mfxFrameSurface1 [m_VppResponse.NumFrameActual];
        MSDK_CHECK_POINTER(m_pVppSurfaces, MFX_ERR_MEMORY_ALLOC);

        for (int i = 0; i < m_VppResponse.NumFrameActual; i++)
        {
            MSDK_ZERO_MEMORY(m_pVppSurfaces[i]);
            MSDK_MEMCPY_VAR(m_pVppSurfaces[i].Info, &(m_mfxVppParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

            if (m_bExternalAlloc)
            {
                m_pVppSurfaces[i].Data.MemId = m_VppResponse.mids[i];
            }
            else
            {
                sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_VppResponse.mids[i], &(m_pVppSurfaces[i].Data));
                MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
            }
        }
    }

    // prepare Aux buffer for preenc
    if (m_pmfxPreENC)
    {

        m_PreEncAuxPool.resize(m_EncResponse.NumFrameActual);
        auto laCtrl = m_mfxPreEncParams.AddExtBuffer<mfxExtLAControl>();

        int buff_size = sizeof(mfxExtLAFrameStatistics) +
            sizeof(mfxLAFrameInfo)*laCtrl->NumOutStream*laCtrl->LookAheadDepth;
        for (auto& buffer : m_PreEncAuxPool)
        {
            buffer.encInput = {};
            buffer.encOutput = {};

            buffer.encCtrl.NumExtParam = 1;
            buffer.encCtrl.ExtParam = new mfxExtBuffer*[1];

            char *pBuff = new char[buff_size];
            memset(pBuff,0, buff_size);

            buffer.encCtrl.ExtParam[0] = (mfxExtBuffer *)pBuff;

            mfxExtLAFrameStatistics *pExtBuffer = (mfxExtLAFrameStatistics *)pBuff;

            pExtBuffer->Header.BufferId = MFX_EXTBUFF_LOOKAHEAD_STAT;
            pExtBuffer->Header.BufferSz = buff_size;
            pExtBuffer->NumAlloc = laCtrl->NumOutStream*laCtrl->LookAheadDepth;
            pExtBuffer->FrameStat = (mfxLAFrameInfo *)(pBuff + sizeof(mfxExtLAFrameStatistics));

            buffer.encOutput.NumExtParam = 1;
            buffer.encOutput.ExtParam = buffer.encCtrl.ExtParam;
        }
    }

    // prepare mfxEncodeCtrl array for encoder if qpfile mode is enabled
    if (m_bQPFileMode)
    {
        m_EncCtrls.resize(m_EncResponse.NumFrameActual);
        for (auto& ctrl : m_EncCtrls)
        {
            ctrl.Payload    = m_UserDataUnregSEI.data();
            ctrl.NumPayload = (mfxU16)m_UserDataUnregSEI.size();
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (D3D9_MEMORY == m_memType || D3D11_MEMORY == m_memType)
    {
#if D3D_SURFACES_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

        mfxHDL hdl = NULL;
        mfxHandleType hdl_t =
#if MFX_D3D11_SUPPORT
            D3D11_MEMORY == m_memType ? MFX_HANDLE_D3D11_DEVICE :
#endif // #if MFX_D3D11_SUPPORT
            MFX_HANDLE_D3D9_DEVICE_MANAGER;

        sts = m_hwdev->GetHandle(hdl_t, &hdl);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");

        // handle is needed for HW library only
        mfxIMPL impl = 0;
        m_mfxSession.QueryIMPL(&impl);
        if (impl != MFX_IMPL_SOFTWARE)
        {
            sts = m_mfxSession.SetHandle(hdl_t, hdl);
            MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");
        }

        // create D3D allocator
#if MFX_D3D11_SUPPORT
        if (D3D11_MEMORY == m_memType)
        {
            m_pMFXAllocator = new D3D11FrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

            D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(hdl);
            pd3dAllocParams->bUseSingleTexture = m_bSingleTexture;
            m_pmfxAllocatorParams = pd3dAllocParams;
        }
        else
#endif // #if MFX_D3D11_SUPPORT
        {
            m_pMFXAllocator = new D3DFrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

            D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);

            m_pmfxAllocatorParams = pd3dAllocParams;
        }

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to Media SDK */
        sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

        m_bExternalAlloc = true;
#endif
#ifdef LIBVA_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");
        /* It's possible to skip failed result here and switch to SW implementation,
        but we don't process this way */

        mfxHDL hdl = NULL;
        sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
        // provide device manager to MediaSDK
        sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

        // create VAAPI allocator
        m_pMFXAllocator = new vaapiFrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

        vaapiAllocatorParams *p_vaapiAllocParams = new vaapiAllocatorParams;
        MSDK_CHECK_POINTER(p_vaapiAllocParams, MFX_ERR_MEMORY_ALLOC);

        p_vaapiAllocParams->m_dpy = (VADisplay)hdl;
#ifdef ENABLE_V4L2_SUPPORT
        p_vaapiAllocParams->m_export_mode = vaapiAllocatorParams::PRIME;
#endif
        m_pmfxAllocatorParams = p_vaapiAllocParams;

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to mediasdk */
        sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

        m_bExternalAlloc = true;
#endif
    }
    else
    {
#ifdef LIBVA_SUPPORT
        //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library
        mfxIMPL impl;
        m_mfxSession.QueryIMPL(&impl);

        if(MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE(impl))
        {
            sts = CreateHWDevice();
            MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

            mfxHDL hdl = NULL;
            sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
            // provide device manager to MediaSDK
            sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
            MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");
        }
#endif

        // create system memory allocator
        m_pMFXAllocator = new SysMemFrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

        /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, Media SDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/
    }

    // initialize memory allocator
    sts = m_pMFXAllocator->Init(m_pmfxAllocatorParams);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Init failed");

    return MFX_ERR_NONE;
}

void CEncodingPipeline::DeleteFrames()
{
    // delete surfaces array
    MSDK_SAFE_DELETE_ARRAY(m_pEncSurfaces);
    MSDK_SAFE_DELETE_ARRAY(m_pVppSurfaces);

    // delete frames
    if (m_pMFXAllocator)
    {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_EncResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_VppResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_PreEncResponse);
    }

    for (auto& buffer:m_PreEncAuxPool)
    {
        if(buffer.encOutput.ExtParam)
        {
            delete [] buffer.encOutput.ExtParam[0];
            delete [] buffer.encOutput.ExtParam;
        }
    }
    m_PreEncAuxPool.clear();
}

void CEncodingPipeline::DeleteHWDevice()
{
    MSDK_SAFE_DELETE(m_hwdev);
}

void CEncodingPipeline::DeleteAllocator()
{
    // delete allocator
    MSDK_SAFE_DELETE(m_pMFXAllocator);
    MSDK_SAFE_DELETE(m_pmfxAllocatorParams);

    DeleteHWDevice();
}

CEncodingPipeline::CEncodingPipeline()
{
    m_pmfxENC = NULL;
    m_pmfxVPP = NULL;
    m_pmfxPreENC = NULL;
    m_pMFXAllocator = NULL;
    m_pmfxAllocatorParams = NULL;
    m_memType = SYSTEM_MEMORY;
    m_bExternalAlloc = false;
    m_pEncSurfaces = NULL;
    m_pVppSurfaces = NULL;
    m_InputFourCC = 0;

    m_nPerfOpt = 0;
    m_nTimeout = 0;

    m_nFramesRead = 0;
    m_bFileWriterReset = false;

    m_bSoftRobustFlag = false;
    m_bSingleTexture = false;

    m_MVCflags = MVC_DISABLED;
    m_nNumView = 0;

    m_FileWriters.first = m_FileWriters.second = NULL;

    m_hwdev = NULL;

#if (MFX_VERSION >= 1027)
    m_round_in = nullptr;
#endif

    MSDK_ZERO_MEMORY(m_EncResponse);
    MSDK_ZERO_MEMORY(m_VppResponse);
    MSDK_ZERO_MEMORY(m_PreEncResponse);

    isV4L2InputEnabled = false;

    m_nFramesToProcess = 0;
    m_bCutOutput = false;
    m_bTimeOutExceed = false;
    m_bInsertIDR = false;

    m_bIsFieldSplitting = false;

    m_bQPFileMode = false;
    m_bTCBRCFileMode = false;
    m_nEncSurfIdx = 0;
    m_nVppSurfIdx = 0;
}

CEncodingPipeline::~CEncodingPipeline()
{
    Close();
}

mfxStatus CEncodingPipeline::InitFileWriter(CSmplBitstreamWriter **ppWriter, const msdk_char *filename)
{
    MSDK_CHECK_ERROR(ppWriter, NULL, MFX_ERR_NULL_PTR);

    MSDK_SAFE_DELETE(*ppWriter);
    *ppWriter = new CSmplBitstreamWriter;
    MSDK_CHECK_POINTER(*ppWriter, MFX_ERR_MEMORY_ALLOC);
    mfxStatus sts = (*ppWriter)->Init(filename);
    MSDK_CHECK_STATUS(sts, " failed");

    return sts;
}

mfxStatus CEncodingPipeline::InitFileWriters(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    // no output mode
    if (!pParams->dstFileBuff.size())
        return MFX_ERR_NONE;

    // prepare output file writers

    // ViewOutput mode: output in single bitstream
    if ( (MVC_VIEWOUTPUT & pParams->MVC_flags) && (pParams->dstFileBuff.size() <= 1))
    {
        sts = InitFileWriter(&m_FileWriters.first, pParams->dstFileBuff[0]);
        MSDK_CHECK_STATUS(sts, "InitFileWriter failed");

        m_FileWriters.second = m_FileWriters.first;
    }
    // ViewOutput mode: 2 bitstreams in separate files
    else if ( (MVC_VIEWOUTPUT & pParams->MVC_flags) && (pParams->dstFileBuff.size() <= 2))
    {
        sts = InitFileWriter(&m_FileWriters.first, pParams->dstFileBuff[0]);
        MSDK_CHECK_STATUS(sts, "InitFileWriter failed");

        sts = InitFileWriter(&m_FileWriters.second, pParams->dstFileBuff[1]);
        MSDK_CHECK_STATUS(sts, "InitFileWriter failed");
    }
    // ViewOutput mode: 3 bitstreams - 2 separate & 1 merged
    else if ( (MVC_VIEWOUTPUT & pParams->MVC_flags) && (pParams->dstFileBuff.size() <= 3))
    {
        std::unique_ptr<CSmplBitstreamDuplicateWriter> first(new CSmplBitstreamDuplicateWriter);

        // init first duplicate writer
        MSDK_CHECK_POINTER(first.get(), MFX_ERR_MEMORY_ALLOC);
        sts = first->Init(pParams->dstFileBuff[0]);
        MSDK_CHECK_STATUS(sts, "first->Init failed");
        sts = first->InitDuplicate(pParams->dstFileBuff[2]);
        MSDK_CHECK_STATUS(sts, "first->InitDuplicate failed");

        // init second duplicate writer
        std::unique_ptr<CSmplBitstreamDuplicateWriter> second(new CSmplBitstreamDuplicateWriter);
        MSDK_CHECK_POINTER(second.get(), MFX_ERR_MEMORY_ALLOC);
        sts = second->Init(pParams->dstFileBuff[1]);
        MSDK_CHECK_STATUS(sts, "second->Init failed");
        sts = second->JoinDuplicate(first.get());
        MSDK_CHECK_STATUS(sts, "second->JoinDuplicate failed");

        m_FileWriters.first = first.release();
        m_FileWriters.second = second.release();
    }
    // not ViewOutput mode
    else
    {
        sts = InitFileWriter(&m_FileWriters.first, pParams->dstFileBuff[0]);
        MSDK_CHECK_STATUS(sts, "InitFileWriter failed");
    }

    return sts;
}

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
mfxU16 FourCcBitDepth(mfxU32 fourCC)
{
    switch (fourCC)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_YUY2:
    case MFX_FOURCC_AYUV:
        return 8;
        break;

    case MFX_FOURCC_P010:
    case MFX_FOURCC_P210:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y410:
#endif
        return 10;
        break;

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
    case MFX_FOURCC_Y216:
    case MFX_FOURCC_Y416:
        return 12;
        break;
#endif
    default:
        return 0;
    }
}
#endif

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
mfxU32 CEncodingPipeline::GetPreferredAdapterNum(const mfxAdaptersInfo & adapters, const sInputParams & params)
{
    if (adapters.NumActual == 0 || !adapters.Adapters)
        return 0;

    if (params.bPrefferdGfx)
    {
        // Find dGfx adapter in list and return it's index

        auto idx = std::find_if(adapters.Adapters, adapters.Adapters + adapters.NumActual,
            [](const mfxAdapterInfo info)
        {
            return info.Platform.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_DISCRETE;
        });

        // No dGfx in list
        if (idx == adapters.Adapters + adapters.NumActual)
        {
            msdk_printf(MSDK_STRING("Warning: No dGfx detected on machine. Will pick another adapter\n"));
            return 0;
        }

        return static_cast<mfxU32>(std::distance(adapters.Adapters, idx));
    }

    if (params.bPrefferiGfx)
    {
        // Find iGfx adapter in list and return it's index

        auto idx = std::find_if(adapters.Adapters, adapters.Adapters + adapters.NumActual,
            [](const mfxAdapterInfo info)
        {
            return info.Platform.MediaAdapterType == mfxMediaAdapterType::MFX_MEDIA_INTEGRATED;
        });

        // No iGfx in list
        if (idx == adapters.Adapters + adapters.NumActual)
        {
            msdk_printf(MSDK_STRING("Warning: No iGfx detected on machine. Will pick another adapter\n"));
            return 0;
        }

        return static_cast<mfxU32>(std::distance(adapters.Adapters, idx));
    }

    // Other ways return 0, i.e. best suitable detected by dispatcher
    return 0;
}
#endif // (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)

mfxStatus CEncodingPipeline::GetImpl(const sInputParams & params, mfxIMPL & impl)
{
    if (!params.bUseHWLib)
    {
        impl = MFX_IMPL_SOFTWARE;
        return MFX_ERR_NONE;
    }

#if (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)
    mfxU32 num_adapters_available;

    mfxStatus sts = MFXQueryAdaptersNumber(&num_adapters_available);
    MSDK_CHECK_STATUS(sts, "MFXQueryAdaptersNumber failed");

    mfxComponentInfo interface_request = { mfxComponentType::MFX_COMPONENT_ENCODE };

    bool isFourccNeedShift = params.FileInputFourCC == MFX_FOURCC_P010 || params.FileInputFourCC == MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
        || params.FileInputFourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
        || params.FileInputFourCC == MFX_FOURCC_Y216 || params.FileInputFourCC == MFX_FOURCC_P016
#endif
        ;
    mfxU16 Shift = params.IsSourceMSB || (isFourccNeedShift && ((params.memType != SYSTEM_MEMORY && AreGuidsEqual(params.pluginParams.pluginGuid, MFX_PLUGINID_HEVCE_HW)) || params.CodecId == MFX_CODEC_VP9));
    mfxU16 Height   = (MFX_PICSTRUCT_PROGRESSIVE == params.nPicStruct) ? MSDK_ALIGN16(params.nDstHeight) : MSDK_ALIGN32(params.nDstHeight);
    mfxU16 LowPower = mfxU16(params.enableQSVFF ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_UNKNOWN);
    mfxU16 BitDepth = FourCcBitDepth(params.EncodeFourCC);

    interface_request.Requirements                              = {};
    interface_request.Requirements.mfx.LowPower                 = LowPower;
    interface_request.Requirements.mfx.TargetUsage              = params.nTargetUsage;
    interface_request.Requirements.mfx.CodecId                  = params.CodecId;
    interface_request.Requirements.mfx.CodecProfile             = params.CodecProfile;
    interface_request.Requirements.mfx.CodecLevel               = params.CodecLevel;
    interface_request.Requirements.mfx.FrameInfo.BitDepthLuma   = BitDepth;
    interface_request.Requirements.mfx.FrameInfo.BitDepthChroma = BitDepth;
    interface_request.Requirements.mfx.FrameInfo.Shift          = Shift;
    interface_request.Requirements.mfx.FrameInfo.FourCC         = params.EncodeFourCC;
    interface_request.Requirements.mfx.FrameInfo.Width          = MSDK_ALIGN16(params.nDstWidth);
    interface_request.Requirements.mfx.FrameInfo.Height         = Height;
    interface_request.Requirements.mfx.FrameInfo.PicStruct      = params.nPicStruct;
    interface_request.Requirements.mfx.FrameInfo.ChromaFormat   = FourCCToChroma(params.EncodeFourCC);

    /* Note! IOPattern is mandatory according to MSDK manual! */
    if (D3D9_MEMORY == params.memType || D3D11_MEMORY == params.memType)
    {
        interface_request.Requirements.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }
    else
    {
        interface_request.Requirements.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    }

    // JPEG encoder settings overlap with other encoders settings in mfxInfoMFX structure
    if (MFX_CODEC_JPEG == params.CodecId)
    {
        interface_request.Requirements.mfx.Interleaved     = 1;
        interface_request.Requirements.mfx.Quality         = params.nQuality;
        interface_request.Requirements.mfx.RestartInterval = 0;
        MSDK_ZERO_MEMORY(interface_request.Requirements.mfx.reserved5);
    }

    std::vector<mfxAdapterInfo> displays_data(num_adapters_available);
    mfxAdaptersInfo adapters = { displays_data.data(), mfxU32(displays_data.size()), 0u };

    sts = MFXQueryAdapters(&interface_request, &adapters);
    if (sts == MFX_ERR_NOT_FOUND)
    {
        msdk_printf(MSDK_STRING("ERROR: No suitable adapters found for this workload\n"));
    }
    MSDK_CHECK_STATUS(sts, "MFXQueryAdapters failed");

    mfxU32 idx = GetPreferredAdapterNum(adapters, params);
    switch (adapters.Adapters[idx].Number)
    {
    case 0:
        impl = MFX_IMPL_HARDWARE;
        break;
    case 1:
        impl = MFX_IMPL_HARDWARE2;
        break;
    case 2:
        impl = MFX_IMPL_HARDWARE3;
        break;
    case 3:
        impl = MFX_IMPL_HARDWARE4;
        break;

    default:
        // try searching on all display adapters
        impl = MFX_IMPL_HARDWARE_ANY;
        break;
    }
#else
    // Library should pick first available compatible adapter during InitEx call with MFX_IMPL_HARDWARE_ANY
    impl = MFX_IMPL_HARDWARE_ANY;
#endif // (defined(_WIN64) || defined(_WIN32)) && (MFX_VERSION >= 1031)

    // If d3d11 surfaces are used ask the library to run acceleration through D3D11
    // feature may be unsupported due to OS or MSDK API version
    // prefer d3d11 by default
#if D3D_SURFACES_SUPPORT
    if (D3D9_MEMORY != params.memType)
        impl |= MFX_IMPL_VIA_D3D11;
#endif

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::Init(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

#if defined ENABLE_V4L2_SUPPORT
    isV4L2InputEnabled = pParams->isV4L2InputEnabled;
#endif

    m_MVCflags = pParams->MVC_flags;

    // FileReader can convert yv12->nv12 without vpp
    m_InputFourCC = (pParams->FileInputFourCC == MFX_FOURCC_I420) ? MFX_FOURCC_NV12 : pParams->FileInputFourCC;

    m_nTimeout = pParams->nTimeout;
#if defined(LINUX32) || defined(LINUX64)
    m_strDevicePath = pParams->strDevicePath;
#endif
    mfxInitParamlWrap initPar;

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    initPar.Version.Major = 1;
    initPar.Version.Minor = 0;

    initPar.GPUCopy = pParams->gpuCopy;
    m_bSingleTexture = pParams->bSingleTexture;

    mfxStatus sts = GetImpl(*pParams, initPar.Implementation);
    MSDK_CHECK_STATUS(sts, "GetImpl failed");

    sts = m_mfxSession.InitEx(initPar);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.InitEx failed");

    mfxVersion version;
    sts = MFXQueryVersion(m_mfxSession, &version); // get real API version of the loaded library
    MSDK_CHECK_STATUS(sts, "MFXQueryVersion failed");

    if ((pParams->MVC_flags & MVC_ENABLED) != 0 && !CheckVersion(&version, MSDK_FEATURE_MVC)) {
        msdk_printf(MSDK_STRING("error: MVC is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;

    }
    if ((pParams->MVC_flags & MVC_VIEWOUTPUT) != 0 && !CheckVersion(&version, MSDK_FEATURE_MVC_VIEWOUTPUT)) {
        msdk_printf(MSDK_STRING("error: MVC Viewoutput is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }
    if ((pParams->CodecId == MFX_CODEC_JPEG) && !CheckVersion(&version, MSDK_FEATURE_JPEG_ENCODE)) {
        msdk_printf(MSDK_STRING("error: Jpeg is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->nRateControlMethod == MFX_RATECONTROL_LA) && !CheckVersion(&version, MSDK_FEATURE_LOOK_AHEAD)) {
        msdk_printf(MSDK_STRING("error: Look ahead is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if (CheckVersion(&version, MSDK_FEATURE_PLUGIN_API)) {
        /* Here we actually define the following codec initialization scheme:
        *  1. If plugin path or guid is specified: we load user-defined plugin (example: HEVC encoder plugin)
        *  2. If plugin path not specified:
        *    2.a) we check if codec is distributed as a mediasdk plugin and load it if yes
        *    2.b) if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library
        */
        if (pParams->pluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(pParams->pluginParams.strPluginPath, sizeof(pParams->pluginParams.strPluginPath)))
        {
            m_pUserModule.reset(new MFXVideoUSER(m_mfxSession));
            m_pPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, pParams->pluginParams.pluginGuid, 1, pParams->pluginParams.strPluginPath, (mfxU32)msdk_strnlen(pParams->pluginParams.strPluginPath, sizeof(pParams->pluginParams.strPluginPath))));
            if (m_pPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            bool isDefaultPlugin = false;
            if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                mfxIMPL impl = pParams->bUseHWLib ? MFX_IMPL_HARDWARE : MFX_IMPL_SOFTWARE;
                pParams->pluginParams.pluginGuid = msdkGetPluginUID(impl, MSDK_VENCODE, pParams->CodecId);
                isDefaultPlugin = true;
            }
            if (!AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_pPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, pParams->pluginParams.pluginGuid, 1));
                if (m_pPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
            }
            if (sts == MFX_ERR_UNSUPPORTED)
            {
                msdk_printf(MSDK_STRING("%s"), isDefaultPlugin ?
                    MSDK_STRING("Default plugin cannot be loaded (possibly you have to define plugin explicitly)\n")
                    : MSDK_STRING("Explicitly specified plugin cannot be loaded.\n"));
            }
        }
        MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
    }

    if(pParams->bEnableExtLA)
    {
        m_pPreEncPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, MFX_PLUGINID_H264LA_HW, 1));
        MSDK_CHECK_POINTER(m_pPreEncPlugin, MFX_ERR_UNSUPPORTED)
        m_pmfxPreENC = new MFXVideoENC(m_mfxSession);
        MSDK_CHECK_POINTER(m_pmfxPreENC, MFX_ERR_MEMORY_ALLOC);
    }

    // create encoder
    m_pmfxENC = new MFXVideoENCODE(m_mfxSession);
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_MEMORY_ALLOC);

    bool bVpp = false;
    if (pParams->nPicStruct != MFX_PICSTRUCT_PROGRESSIVE && pParams->CodecId == MFX_CODEC_HEVC)
    {
        bVpp = true;
        m_bIsFieldSplitting = true;
    }

    // create preprocessor if resizing was requested from command line
    // or if different FourCC is set
    if (pParams->nWidth != pParams->nDstWidth ||
        pParams->nHeight != pParams->nDstHeight ||
        FileFourCC2EncFourCC(pParams->FileInputFourCC) != pParams->EncodeFourCC)
    {
        bVpp = true;
        if (m_bIsFieldSplitting)
        {
            msdk_printf(MSDK_STRING("ERROR: Field Splitting is enabled according to streams parameters. Other VPP filters cannot be used in this mode, please remove corresponding options.\n"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (bVpp)
    {
        msdk_printf(MSDK_STRING("Note: VPP is enabled.\n"));
        m_pmfxVPP = new MFXVideoVPP(m_mfxSession);
        MSDK_CHECK_POINTER(m_pmfxVPP, MFX_ERR_MEMORY_ALLOC);
    }
    // Determine if we should shift P010 surfaces
    bool readerShift = false;
    if (pParams->FileInputFourCC == MFX_FOURCC_P010
        || pParams->FileInputFourCC == MFX_FOURCC_P210
#if (MFX_VERSION >= 1027)
        || pParams->FileInputFourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1031)
        || pParams->FileInputFourCC == MFX_FOURCC_Y216 || pParams->FileInputFourCC == MFX_FOURCC_P016
#endif
    )
    {
        if (pParams->IsSourceMSB)
        {
            pParams->shouldUseShifted10BitVPP = true;
            pParams->shouldUseShifted10BitEnc = true;
        }
        else
        {
            pParams->shouldUseShifted10BitVPP = m_pmfxVPP && pParams->memType != SYSTEM_MEMORY;

            pParams->shouldUseShifted10BitEnc = (pParams->memType != SYSTEM_MEMORY && AreGuidsEqual(pParams->pluginParams.pluginGuid, MFX_PLUGINID_HEVCE_HW)) || pParams->CodecId == MFX_CODEC_VP9;
            readerShift = m_pmfxVPP ? pParams->shouldUseShifted10BitVPP : pParams->shouldUseShifted10BitEnc;
        }
    }

    if(readerShift)
    {
        msdk_printf(MSDK_STRING("\n10-bit frames data will be shifted to MSB area to be compatible with MSDK 10-bit input format\n"));
    }

    if(m_pmfxVPP && pParams->shouldUseShifted10BitVPP && !pParams->shouldUseShifted10BitEnc && pParams->bUseHWLib)
    {
        msdk_printf(MSDK_STRING("ERROR: Encoder requires P010 LSB format. VPP currently supports only MSB encoding for P010 format.\nSample cannot combine both of them in one pipeline.\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    // Preparing readers and writers
    if (!isV4L2InputEnabled)
    {
        // prepare input file reader
        sts = m_FileReader.Init(pParams->InputFiles,
            pParams->FileInputFourCC,readerShift);
        MSDK_CHECK_STATUS(sts, "m_FileReader.Init failed");
    }

    sts = InitFileWriters(pParams);
    MSDK_CHECK_STATUS(sts, "InitFileWriters failed");


    // set memory type
    m_memType = pParams->memType;
    m_nPerfOpt = pParams->nPerfOpt;
    m_fpsLimiter.Reset(pParams->nMaxFPS);

    m_bSoftRobustFlag = pParams->bSoftRobustFlag;

    // create and init frame allocator
    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    sts = InitMfxEncParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxEncParams failed");

    sts = InitMfxVppParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxVppParams failed");

    if(pParams->bEnableExtLA)
    {
        sts = InitMfxPreEncParams(pParams);
        MSDK_CHECK_STATUS(sts, "InitMfxPreEncParams failed");
    }

    // MVC specific options
    if (MVC_ENABLED & m_MVCflags)
    {
        auto mvcBuffer = m_mfxEncParams.AddExtBuffer<mfxExtMVCSeqDesc>();
        InitExtMVCBuffers(mvcBuffer);
    }

    sts = ResetMFXComponents(pParams);
    MSDK_CHECK_STATUS(sts, "ResetMFXComponents failed");

    sts = OpenRoundingOffsetFile(pParams);
    MSDK_CHECK_STATUS(sts, "Failed to open file");

    InitV4L2Pipeline(pParams);

    m_nFramesToProcess = pParams->nNumFrames;

    // If output isn't specified work in performance mode and do not insert idr
    m_bCutOutput = pParams->dstFileBuff.size() ? !pParams->bUncut : false;

    // Dumping components configuration if required
    if(*pParams->DumpFileName)
    {
        CParametersDumper::DumpLibraryConfiguration(pParams->DumpFileName, NULL, m_pmfxVPP, m_pmfxENC, NULL, &m_mfxVppParams,&m_mfxEncParams);
    }

    return MFX_ERR_NONE;
}

void CEncodingPipeline::InitV4L2Pipeline(sInputParams* /*pParams*/)
{
#if defined (ENABLE_V4L2_SUPPORT)
    if (isV4L2InputEnabled)
    {
        int i;
        v4l2Pipeline.Init(pParams->DeviceName,
            pParams->nWidth, pParams->nHeight,
            m_VppResponse.NumFrameActual,
            pParams->v4l2Format,
            pParams->MipiMode,
            pParams->MipiPort);

        v4l2Pipeline.V4L2Alloc();

        for(i=0; i<m_VppResponse.NumFrameActual; i++)
        {
            buffers[i].index = i;
            struct vaapiMemId *vaapi = (struct vaapiMemId *)m_pVppSurfaces[i].Data.MemId;

            buffers[i].fd = vaapi->m_buffer_info.handle;
            v4l2Pipeline.V4L2QueueBuffer(&buffers[i]);
        }
    }
#endif
}

mfxStatus CEncodingPipeline::CaptureStartV4L2Pipeline()
{
#if defined (ENABLE_V4L2_SUPPORT)
    if (isV4L2InputEnabled)
    {
        v4l2Pipeline.V4L2StartCapture();

        v4l2Device *m_v4l2Display = &v4l2Pipeline;


        if (pthread_create(&m_PollThread, NULL, PollingThread, (void *)m_v4l2Display))
        {
            msdk_printf(MSDK_STRING("Couldn't create v4l2 polling thread\n"));
            return MFX_ERR_UNKNOWN;
        }
    }

#endif
    return MFX_ERR_NONE;
}

void CEncodingPipeline::CaptureStopV4L2Pipeline()
{
#if defined (ENABLE_V4L2_SUPPORT)
    if (isV4L2InputEnabled)
    {
        pthread_join(m_PollThread, NULL);
        v4l2Pipeline.V4L2StopCapture();
    }
#endif
}

void CEncodingPipeline::InsertIDR(mfxEncodeCtrl & ctrl, bool forceIDR)
{
    ctrl.FrameType = forceIDR ? MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF : MFX_FRAMETYPE_UNKNOWN;
}

mfxStatus CEncodingPipeline::InitEncFrameParams(sTask* pTask)
{
    MSDK_CHECK_POINTER(pTask, MFX_ERR_NULL_PTR);

    mfxEncodeCtrlWrap & ctrl = pTask->encCtrl;
    if (m_round_in)
    {
        ctrl.AddExtBuffer<mfxExtAVCRoundingOffset>();

        auto numFields = m_mfxEncParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;

        for (auto fieldId = 0; fieldId < numFields; ++fieldId)
        {
            auto ro = ctrl.GetExtBuffer<mfxExtAVCRoundingOffset>(fieldId);
            MSDK_CHECK_POINTER(ro, MFX_ERR_NULL_PTR);

            SAFE_FREAD(&ro->EnableRoundingIntra, sizeof(mfxU16), 1, m_round_in, MFX_ERR_MORE_DATA);
            SAFE_FREAD(&ro->RoundingOffsetIntra, sizeof(mfxU16), 1, m_round_in, MFX_ERR_MORE_DATA);
            SAFE_FREAD(&ro->EnableRoundingInter, sizeof(mfxU16), 1, m_round_in, MFX_ERR_MORE_DATA);
            SAFE_FREAD(&ro->RoundingOffsetInter, sizeof(mfxU16), 1, m_round_in, MFX_ERR_MORE_DATA);
        }
    }

    ctrl.Payload = m_UserDataUnregSEI.data();
    ctrl.NumPayload = (mfxU16)m_UserDataUnregSEI.size();

    return MFX_ERR_NONE;
}

void CEncodingPipeline::Close()
{
    if (m_FileWriters.first)
    {
        msdk_printf(MSDK_STRING("Frame number: %u\r\n"), m_FileWriters.first->m_nProcessedFramesNum);
        mfxF64 ProcDeltaTime = m_statOverall.GetDeltaTime() - m_statFile.GetDeltaTime() - m_TaskPool.GetFileStatistics().GetDeltaTime();
        msdk_printf(MSDK_STRING("Encoding fps: %.0f\n"), m_FileWriters.first->m_nProcessedFramesNum / ProcDeltaTime);
    }

    std::for_each(m_UserDataUnregSEI.begin(), m_UserDataUnregSEI.end(), [](mfxPayload* payload) { delete[] payload->Data; delete payload; });
    m_UserDataUnregSEI.clear();

    MSDK_SAFE_DELETE(m_pmfxENC);
    MSDK_SAFE_DELETE(m_pmfxVPP);
    MSDK_SAFE_DELETE(m_pmfxPreENC);

#if (MFX_VERSION >= 1024)
    auto extBRC = m_mfxEncParams.GetExtBuffer<mfxExtBRC>();
    if (extBRC)
        HEVCExtBRC::Destroy(*extBRC);
#endif

    DeallocateExtMVCBuffers();
    FreeVppFilters();

    DeleteFrames();

    m_pPlugin.reset();
    m_pPreEncPlugin.reset();

    m_TaskPool.Close();
    m_mfxSession.Close();

    m_FileReader.Close();
    FreeFileWriters();

#if (MFX_VERSION >= 1027)
    if(m_round_in)
    {
        fclose(m_round_in);
        m_round_in = nullptr;
    }
#endif
    // allocator if used as external for MediaSDK must be deleted after SDK components
    DeleteAllocator();
}

void CEncodingPipeline::FreeFileWriters()
{
    if (m_FileWriters.second == m_FileWriters.first)
    {
        m_FileWriters.second = NULL; // second do not own the writer - just forget pointer
    }

    if (m_FileWriters.first)
        m_FileWriters.first->Close();
    MSDK_SAFE_DELETE(m_FileWriters.first);

    if (m_FileWriters.second)
        m_FileWriters.second->Close();
    MSDK_SAFE_DELETE(m_FileWriters.second);
}

mfxStatus CEncodingPipeline::FillBuffers()
{
    if (m_nPerfOpt)
    {
        for (mfxU32 i = 0; i < m_nPerfOpt; i++)
        {
            mfxFrameSurface1* surface = m_pmfxVPP ? &m_pVppSurfaces[i] : &m_pEncSurfaces[i];

            mfxStatus sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, surface->Data.MemId, &surface->Data);
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
            sts = m_FileReader.LoadNextFrame(surface);
            MSDK_CHECK_STATUS(sts, "m_FileReader.LoadNextFrame failed");
            sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, surface->Data.MemId, &surface->Data);
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Unlock failed");
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::ResetMFXComponents(sInputParams* pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;
    sts = m_pmfxENC->Close();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->Close failed");

    if (m_pmfxVPP)
    {
        sts = m_pmfxVPP->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Close failed");
    }

    // free allocated frames
    DeleteFrames();

    m_TaskPool.Close();

    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    m_mfxEncParams.mfx.FrameInfo.FourCC       = m_mfxPreEncParams.mfx.FrameInfo.FourCC       = m_mfxVppParams.vpp.Out.FourCC;
    m_mfxEncParams.mfx.FrameInfo.ChromaFormat = m_mfxPreEncParams.mfx.FrameInfo.ChromaFormat = m_mfxVppParams.vpp.Out.ChromaFormat;

    // LA initialization
    if (m_pmfxPreENC)
    {
        sts = m_pmfxPreENC->Init(&m_mfxPreEncParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->Init failed");
    }

    if (m_bIsFieldSplitting)
    {
        m_mfxEncParams.mfx.FrameInfo.Height = MSDK_ALIGN32((m_mfxEncParams.mfx.FrameInfo.Height/2));
        m_mfxEncParams.mfx.FrameInfo.CropH /= 2;
    }

    sts = m_pmfxENC->Init(&m_mfxEncParams);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->Init failed");

    sts = m_pmfxENC->GetVideoParam(&m_mfxEncParams);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->GetVideoParam failed");

    if (m_bIsFieldSplitting)
    {
        if (pParams->nPicStruct & MFX_PICSTRUCT_FIELD_BFF)
        {
            m_mfxVppParams.vpp.In.PicStruct = MFX_PICSTRUCT_FIELD_BFF;
        }
        else
        {
            m_mfxVppParams.vpp.In.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
        }
        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_SINGLE;
    }
    if (m_pmfxVPP)
    {
        sts = m_pmfxVPP->Init(&m_mfxVppParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }

    mfxU32 nEncodedDataBufferSize = GetSufficientBufferSize();
    if (nEncodedDataBufferSize == 0)
        MSDK_CHECK_STATUS(MFX_ERR_UNKNOWN, "ERROR: GetSufficientBufferSize failed");

    sts = m_TaskPool.Init(&m_mfxSession, m_FileWriters.first, m_mfxEncParams.AsyncDepth, nEncodedDataBufferSize, m_FileWriters.second);
    MSDK_CHECK_STATUS(sts, "m_TaskPool.Init failed");

    if (m_bSoftRobustFlag)
        m_TaskPool.SetGpuHangRecoveryFlag();

    sts = FillBuffers();
    MSDK_CHECK_STATUS(sts, "FillBuffers failed");

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::OpenRoundingOffsetFile(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams, MFX_ERR_NULL_PTR);

#if (MFX_VERSION >= 1027)
    bool enableRoundingOffset = pInParams->RoundingOffsetFile && pInParams->CodecId == MFX_CODEC_AVC;
    if (enableRoundingOffset && m_round_in == nullptr)
    {
        MSDK_FOPEN(m_round_in, pInParams->RoundingOffsetFile, MSDK_CHAR("rb"));
        if (m_round_in == nullptr)
        {
            msdk_printf(MSDK_STRING("ERROR: Can't open file %s\n"), pInParams->RoundingOffsetFile);
            return MFX_ERR_UNSUPPORTED;
        }

        msdk_printf(MSDK_STRING("Using rounding offset input file: %s\n"), pInParams->RoundingOffsetFile);
    }
    else
    {
        return MFX_ERR_NONE;
    }
#endif

    return MFX_ERR_NONE;
}

mfxU32 CEncodingPipeline::GetSufficientBufferSize()
{
    if (!GetFirstEncoder())
        return 0;

    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    mfxStatus sts = GetFirstEncoder()->GetVideoParam(&par);
    if (sts != MFX_ERR_NONE)
        return 0;

    mfxU32 new_size = 0;
    if (par.mfx.CodecId == MFX_CODEC_JPEG)
    {
        new_size = 4 + (par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height * 3 + 1023);
    }
    else
    {
        new_size = par.mfx.BufferSizeInKB * par.mfx.BRCParamMultiplier * 1000u;
    }

    return new_size;
}

mfxStatus CEncodingPipeline::AllocateSufficientBuffer(mfxBitstreamWrapper& bs)
{

    mfxU32 new_size = GetSufficientBufferSize();
    if (new_size == 0)
        MSDK_CHECK_STATUS(MFX_ERR_UNKNOWN, "ERROR: GetSufficientBufferSize failed");

    bs.Extend(new_size);

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::GetFreeTask(sTask **ppTask)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bFileWriterReset)
    {
        if (m_FileWriters.first)
        {
            sts = m_FileWriters.first->Reset();
            MSDK_CHECK_STATUS(sts, "m_FileWriters.first->Reset failed");
        }
        if (m_FileWriters.second)
        {
            sts = m_FileWriters.second->Reset();
            MSDK_CHECK_STATUS(sts, "m_FileWriters.second->Reset failed");
        }
        m_bFileWriterReset = false;
    }

    sts = m_TaskPool.GetFreeTask(ppTask);
    if (MFX_ERR_NOT_FOUND == sts)
    {
        sts = m_TaskPool.SynchronizeFirstTask();
        if (MFX_ERR_NONE == sts)
        {
            m_fpsLimiter.Work();
        }
        if (sts == MFX_ERR_GPU_HANG && m_bSoftRobustFlag)
        {
            m_TaskPool.ClearTasks();
            FreeSurfacePool(m_pEncSurfaces, m_EncResponse.NumFrameActual);
            m_bInsertIDR = true;
            sts = MFX_ERR_NONE;
        }
        MSDK_CHECK_STATUS(sts, "m_TaskPool.SynchronizeFirstTask failed");

        // try again
        sts = m_TaskPool.GetFreeTask(ppTask);
    }

    return sts;
}

mfxStatus CEncodingPipeline::PreEncOneFrame(const ExtendedSurface& In, ExtendedSurface& Out, sTask*& pTask)
{
    mfxStatus sts = MFX_ERR_NONE;
    PreEncAuxBuffer* pAux = NULL;

    for (mfxU32 i = 0; i < MSDK_WAIT_INTERVAL; i += 1)
    {
        pAux = GetFreePreEncAuxBuffer();
        if (pAux)
        {
            break;
        }
        else
        {
            MSDK_SLEEP(1);
        }
    }
    MSDK_CHECK_POINTER(pAux,  MFX_ERR_MEMORY_ALLOC);
    for(;;)
    {
        pAux->encInput.InSurface = In.pSurface;
        // at this point surface for encoder contains either a frame from file or a frame processed by vpp
        sts = m_pmfxPreENC->ProcessFrameAsync(&pAux->encInput, &pAux->encOutput, &Out.Syncp );

        if (MFX_ERR_NONE < sts && !Out.Syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(1); // wait if device is busy
        }
        else if (MFX_ERR_NONE <= sts && Out.Syncp)
        {
            LockPreEncAuxBuffer(pAux);
            Out.pCtrl = &pAux->encCtrl;
            pTask->pAux = pAux;
            MSDK_CHECK_POINTER(pAux->encOutput.ExtParam, MFX_ERR_NULL_PTR);
            MSDK_CHECK_NOT_EQUAL(pAux->encOutput.NumExtParam, 1, MFX_ERR_UNSUPPORTED);
            Out.pSurface = ((mfxExtLAFrameStatistics *) pAux->encOutput.ExtParam[0])->OutSurface;
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else
        {
            break;
        }
    }
    return sts;
}

mfxStatus CEncodingPipeline::VPPOneFrame(const ExtendedSurface& In, ExtendedSurface& Out, const bool& skipFrame)
{
    mfxStatus sts = MFX_ERR_NONE;
    for (;;)
    {
        sts = m_pmfxVPP->RunFrameVPPAsync(skipFrame ?  nullptr : In.pSurface,
                                                  Out.pSurface, NULL, &Out.Syncp);

        if (MFX_ERR_NONE < sts && !Out.Syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(1); // wait if device is busy
        }
        else if (MFX_ERR_NONE < sts && Out.Syncp)
        {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            if (m_nPerfOpt) m_nVppSurfIdx++;
            break;
        }
        else
        {
            if (m_nPerfOpt) m_nVppSurfIdx++;
            break; // not a warning
        }
    }
    return sts;
}
mfxStatus CEncodingPipeline::EncodeOneFrame(const ExtendedSurface& In, sTask*& pTask)
{
    mfxStatus sts = MFX_ERR_NONE;
    for (;;)
    {
        if (!m_bQPFileMode)
            InsertIDR(pTask->encCtrl, m_bInsertIDR);
        m_bInsertIDR = false;

		if (m_bTCBRCFileMode &&  In.pSurface)
		{
            sts = ConfigTCBRCTest(In.pSurface);
            MSDK_CHECK_STATUS(sts, "TCBRC reset failed");
        }


        sts = InitEncFrameParams(pTask);
        MSDK_CHECK_STATUS(sts, "ENCODE: InitEncFrameParams failed");

        // at this point surface for encoder contains either a frame from file or a frame processed by vpp/preenc
        sts = m_pmfxENC->EncodeFrameAsync(In.pCtrl, In.pSurface, &pTask->mfxBS, &pTask->EncSyncP);

        if (MFX_ERR_NONE < sts && !pTask->EncSyncP) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(1); // wait if device is busy
        }
        else if (MFX_ERR_NONE < sts && pTask->EncSyncP)
        {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            if (m_nPerfOpt) m_nEncSurfIdx++;
            break;
        }
        else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
        {
            sts = AllocateSufficientBuffer(pTask->mfxBS);
            MSDK_CHECK_STATUS(sts, "AllocateSufficientBuffer failed");
        }
        else
        {
            // get next surface and new task for 2nd bitstream in ViewOutput mode
            MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
            if (m_nPerfOpt) m_nEncSurfIdx++;
            break;
        }
    }

    return sts;
}
mfxStatus CEncodingPipeline::ConfigTCBRCTest(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_bTCBRCFileMode && pSurf)
    {
        mfxU32 targetFrameSize = m_TCBRCFileReader.GetTargetFrameSize(pSurf->Data.FrameOrder);
        if (targetFrameSize)
        {
            mfxU32 newBitrate = targetFrameSize * 8 * m_mfxEncParams.mfx.FrameInfo.FrameRateExtN / (m_mfxEncParams.mfx.FrameInfo.FrameRateExtD * 1000);
            if (m_mfxEncParams.mfx.TargetKbps != newBitrate)
            {
                m_mfxEncParams.mfx.TargetKbps = (mfxU16)newBitrate;
                sts = m_pmfxENC->Reset(&m_mfxEncParams);
            }
        }
    }
    return sts;
}
mfxStatus CEncodingPipeline::Run()
{
    m_statOverall.StartTimeMeasurement();
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    ExtendedSurface vppSurface = {};
    ExtendedSurface preencSurface = {};
    ExtendedSurface encSurface = {};

    sTask *pCurrentTask = nullptr; // a pointer to the current task
    m_nEncSurfIdx = 0;
    m_nVppSurfIdx = 0;

    bool bVppMultipleOutput = false;  // this flag is true if VPP produces more frames at output
    // than consumes at input. E.g. framerate conversion 30 fps -> 60 fps

    // Since in sample we support just 2 views
    // we will change this value between 0 and 1 in case of MVC
    mfxU16 currViewNum = 0;

    mfxU32 nFramesProcessed = 0;

    bool skipLoadingNextFrame = false;

    sts = MFX_ERR_NONE;

#if defined (ENABLE_V4L2_SUPPORT)
    if (isV4L2InputEnabled)
    {
        msdk_printf(MSDK_STRING("Press Ctrl+C to terminate this application\n"));
    }
#endif

    // main loop, preprocessing and encoding
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        if ((m_nFramesToProcess != 0) && (nFramesProcessed == m_nFramesToProcess))
        {
            break;
        }

#if defined (ENABLE_V4L2_SUPPORT)
        if (isV4L2InputEnabled && v4l2Pipeline.GetV4L2TerminationSignal())
        {
            break;
        }
#endif
        // get a pointer to a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);

        // find free surface for encoder input
        if (m_nPerfOpt && !m_pmfxVPP)
        {
            m_nEncSurfIdx %= m_nPerfOpt;
        }
        else
        {
            m_nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
        }
        MSDK_CHECK_ERROR(m_nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

        // point pSurf to encoder surface
        preencSurface.pSurface = vppSurface.pSurface = &m_pEncSurfaces[m_nEncSurfIdx];

        if (!bVppMultipleOutput)
        {
            if(!skipLoadingNextFrame)
            {
                // if vpp is enabled find free surface for vpp input and point pSurf to vpp surface
                if (m_pmfxVPP)
                {
#if defined (ENABLE_V4L2_SUPPORT)
                    if (isV4L2InputEnabled)
                    {
                        m_nVppSurfIdx = v4l2Pipeline.GetOffQ();
                    }
#else
                    if (m_nPerfOpt)
                    {
                        m_nVppSurfIdx = m_nVppSurfIdx % m_nPerfOpt;
                    }
                    else
                    {
                        m_nVppSurfIdx = GetFreeSurface(m_pVppSurfaces, m_VppResponse.NumFrameActual);
                    }
                    MSDK_CHECK_ERROR(m_nVppSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);
#endif
                    vppSurface.pSurface = &m_pVppSurfaces[m_nVppSurfIdx];
                }

                vppSurface.pSurface->Info.FrameId.ViewId = currViewNum;

                m_statFile.StartTimeMeasurement();
                sts = LoadNextFrame(vppSurface.pSurface);
                m_statFile.StopTimeMeasurement();

                if ( (MFX_ERR_MORE_DATA == sts) && !m_bTimeOutExceed)
                    continue;

                MSDK_BREAK_ON_ERROR(sts);

                if (m_bQPFileMode)
                    LoadNextControl(encSurface.pCtrl, m_nEncSurfIdx);
                else
                    encSurface.pCtrl = &pCurrentTask->encCtrl;

                if (MVC_ENABLED & m_MVCflags)
                {
                    currViewNum ^= 1; // Flip between 0 and 1 for ViewId
                }
            }
        }

        // perform preprocessing if required
        if (m_pmfxVPP)
        {
            bVppMultipleOutput = false; // reset the flag before a call to VPP

            sts = VPPOneFrame(vppSurface, preencSurface, skipLoadingNextFrame);
            // process errors
            if (MFX_ERR_MORE_DATA == sts)
            {
                continue;
            }
            else if(MFX_ERR_MORE_SURFACE == sts)
            {
                skipLoadingNextFrame = true;
                sts = MFX_ERR_NONE;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }
        }
        else
        {
            preencSurface.pSurface = vppSurface.pSurface;
            preencSurface.Syncp    = vppSurface.Syncp;
        }

        if (m_pmfxPreENC)
        {
            sts = PreEncOneFrame(preencSurface, encSurface, pCurrentTask);
            // process errors
            if (MFX_ERR_MORE_DATA == sts)
            {
                continue;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }
        }
        else
        {
            encSurface.pSurface = preencSurface.pSurface;
        }

        // save the id of preceding vpp/preenc task which will produce input data for the encode task
        if (preencSurface.Syncp)
        {
            pCurrentTask->DependentVppTasks.push_back(preencSurface.Syncp);
            preencSurface.Syncp = NULL;
            vppSurface.Syncp = NULL;
        }

        sts = EncodeOneFrame(encSurface, pCurrentTask);

        nFramesProcessed++;
    }

    // means that the input file has ended, need to go to buffering loops
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->EncodeFrameAsync failed");

    if (m_pmfxVPP)
    {
        // loop to get buffered frames from vpp
        while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts)
            // MFX_ERR_MORE_SURFACE can be returned only by RunFrameVPPAsync
            // MFX_ERR_MORE_DATA is accepted only from EncodeFrameAsync
        {
            // get a free task (bit stream and sync point for encoder)
            sts = GetFreeTask(&pCurrentTask);
            MSDK_BREAK_ON_ERROR(sts);
            // find free surface for encoder input (vpp output)
            m_nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
            MSDK_CHECK_ERROR(m_nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);
            preencSurface.pSurface = &m_pEncSurfaces[m_nEncSurfIdx];
            vppSurface.pSurface = nullptr;

            sts = VPPOneFrame(vppSurface, preencSurface, false);

            if (MFX_ERR_MORE_SURFACE == sts)
            {
                continue;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }

            if (m_pmfxPreENC)
            {
                sts = PreEncOneFrame(preencSurface, encSurface, pCurrentTask);

                if (MFX_ERR_MORE_DATA == sts)
                {
                    continue;
                }
                else
                {
                    MSDK_BREAK_ON_ERROR(sts);
                }
            }
            else
            {
                encSurface.pSurface = preencSurface.pSurface;
                encSurface.Syncp    = preencSurface.Syncp;
            }

            // save the id of preceding vpp/preenc task which will produce input data for the encode task
            if (preencSurface.Syncp)
            {
                pCurrentTask->DependentVppTasks.push_back(preencSurface.Syncp);
                preencSurface.Syncp = NULL;
                vppSurface.Syncp = NULL;
            }

            sts = EncodeOneFrame(encSurface, pCurrentTask);
        }

        // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
        // indicates that there are no more buffered frames
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        MSDK_CHECK_STATUS(sts, "m_pmfxENC->EncodeFrameAsync failed");
    }

    if (m_pmfxPreENC && nFramesProcessed != m_nFramesToProcess)
    {
        // loop to get buffered frames from preenc
        while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
            // MFX_ERR_MORE_DATA is accepted only from EncodeFrameAsync
        {
            // get a free task (bit stream and sync point for encoder)
            sts = GetFreeTask(&pCurrentTask);
            MSDK_BREAK_ON_ERROR(sts);
            preencSurface.pSurface = nullptr;
            sts = PreEncOneFrame(preencSurface, encSurface, pCurrentTask);

            MSDK_BREAK_ON_ERROR(sts);

            // save the id of preceding vpp/preenc task which will produce input data for the encode task
            if (preencSurface.Syncp)
            {
                pCurrentTask->DependentVppTasks.push_back(preencSurface.Syncp);
                preencSurface.Syncp = NULL;
                vppSurface.Syncp = NULL;
            }

            sts = EncodeOneFrame(encSurface, pCurrentTask);
        }

        // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
        // indicates that there are no more buffered frames
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        MSDK_CHECK_STATUS(sts, "m_pmfxENC->EncodeFrameAsync failed");
    }

    // loop to get buffered frames from encoder
    while (MFX_ERR_NONE <= sts)
    {
        // get a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);
        encSurface.pSurface = nullptr;
        sts = EncodeOneFrame(encSurface, pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);
    }

    // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
    // indicates that there are no more buffered frames
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->EncodeFrameAsync failed");

    // synchronize all tasks that are left in task pool
    while (MFX_ERR_NONE == sts)
    {
        sts = m_TaskPool.SynchronizeFirstTask();
        if (MFX_ERR_NONE == sts)
        {
            m_fpsLimiter.Work();
        }
        if (sts == MFX_ERR_GPU_HANG && m_bSoftRobustFlag)
        {
            m_bInsertIDR = true;
            m_TaskPool.ClearTasks();//may be not needed
            FreeSurfacePool(m_pEncSurfaces, m_EncResponse.NumFrameActual);
            sts = MFX_ERR_NONE;
        }
    }

    // MFX_ERR_NOT_FOUND is the correct status to exit the loop with
    // EncodeFrameAsync and SyncOperation don't return this status
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    // report any errors that occurred in asynchronous part
    MSDK_CHECK_STATUS(sts, "m_TaskPool.SynchronizeFirstTask failed");
    m_statOverall.StopTimeMeasurement();
    return sts;
}

mfxStatus CEncodingPipeline::LoadNextFrame(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (isV4L2InputEnabled)
        return MFX_ERR_NONE;

    m_bTimeOutExceed = (m_nTimeout < m_statOverall.GetDeltaTime()) ? true : false;

    if (m_nPerfOpt)
    {
        // memory buffer mode. No file reading required
        bool bPerfOptExceed = m_nTimeout ? m_bTimeOutExceed : false;
        if ((m_nPerfOpt == m_nFramesRead && m_nFramesToProcess == 0) || bPerfOptExceed)
        {
            sts = MFX_ERR_MORE_DATA;
            // insert idr frame
            m_bInsertIDR = m_bCutOutput;
        }
    }
    else
    {
        // read frame from file
        if (m_bExternalAlloc)
        {
            mfxStatus sts1 = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
            MSDK_CHECK_STATUS(sts1, "m_pMFXAllocator->Lock failed");

            if (m_bQPFileMode)
            {
                mfxU16 w = pSurf->Info.CropW ? pSurf->Info.CropW : pSurf->Info.Width;
                mfxU16 h = pSurf->Info.CropH ? pSurf->Info.CropH : pSurf->Info.Height;
                mfxU32 vid = pSurf->Info.FrameId.ViewId;
                sts = m_FileReader.SkipNframesFromBeginning(w, h, vid, m_QPFileReader.GetCurrentDisplayOrder());
                MSDK_CHECK_STATUS(sts, "m_FileReader.SkipNframesFromBeginning failed");
            }

            sts = m_FileReader.LoadNextFrame(pSurf);

            sts1 = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
            MSDK_CHECK_STATUS(sts1, "m_pMFXAllocator->Unlock failed");
        }
        else
        {
            if (m_bQPFileMode)
            {
                mfxU16 w = pSurf->Info.CropW ? pSurf->Info.CropW : pSurf->Info.Width;
                mfxU16 h = pSurf->Info.CropH ? pSurf->Info.CropH : pSurf->Info.Height;
                mfxU32 vid = pSurf->Info.FrameId.ViewId;
                sts = m_FileReader.SkipNframesFromBeginning(w, h, vid, m_QPFileReader.GetCurrentDisplayOrder());
                MSDK_CHECK_STATUS(sts, "m_FileReader.SkipNframesFromBeginning failed");
            }

            sts = m_FileReader.LoadNextFrame(pSurf);
        }

        if ( (MFX_ERR_MORE_DATA == sts) && !m_bTimeOutExceed )
        {
            m_FileReader.Reset();
            m_bFileWriterReset = m_bCutOutput;
            // forcedly insert idr frame to make output file readable
            m_bInsertIDR = m_bCutOutput;
            return sts;
        }
    }
    // frameorder required for reflist, dbp, and decrefpicmarking operations
    if (pSurf)
        pSurf->Data.FrameOrder = m_bQPFileMode ? m_QPFileReader.GetCurrentDisplayOrder() : m_nFramesRead;
    m_nFramesRead++;

    return sts;
}

void CEncodingPipeline::LoadNextControl(mfxEncodeCtrl*& pCtrl, mfxU32 encSurfIdx)
{
    pCtrl = &m_EncCtrls[encSurfIdx];
    pCtrl->QP = m_QPFileReader.GetCurrentQP();
    pCtrl->FrameType = m_QPFileReader.GetCurrentFrameType();
    m_QPFileReader.NextFrame();
}

void CEncodingPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("Encoding Sample Version %s\n"), GetMSDKSampleVersion().c_str());
    msdk_printf(MSDK_STRING("\nInput file format\t%s\n"), ColorFormatToStr(m_FileReader.m_ColorFormat));
    msdk_printf(MSDK_STRING("Output video\t\t%s\n"), CodecIdToStr(m_mfxEncParams.mfx.CodecId).c_str());

    mfxFrameInfo SrcPicInfo = m_mfxVppParams.vpp.In;
    mfxFrameInfo DstPicInfo = m_mfxEncParams.mfx.FrameInfo;

    msdk_printf(MSDK_STRING("Source picture:\n"));
    msdk_printf(MSDK_STRING("\tResolution\t%dx%d\n"), SrcPicInfo.Width, SrcPicInfo.Height);
    msdk_printf(MSDK_STRING("\tCrop X,Y,W,H\t%d,%d,%d,%d\n"), SrcPicInfo.CropX, SrcPicInfo.CropY, SrcPicInfo.CropW, SrcPicInfo.CropH);

    msdk_printf(MSDK_STRING("Destination picture:\n"));
    msdk_printf(MSDK_STRING("\tResolution\t%dx%d\n"), DstPicInfo.Width, DstPicInfo.Height);
    msdk_printf(MSDK_STRING("\tCrop X,Y,W,H\t%d,%d,%d,%d\n"), DstPicInfo.CropX, DstPicInfo.CropY, DstPicInfo.CropW, DstPicInfo.CropH);

    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), DstPicInfo.FrameRateExtN * 1.0 / DstPicInfo.FrameRateExtD);
    if (m_mfxEncParams.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        msdk_printf(MSDK_STRING("Bit rate(Kbps)\t%d\n"), m_mfxEncParams.mfx.TargetKbps);
    }
    else
    {
        msdk_printf(MSDK_STRING("QPI\t%d\nQPP\t%d\nQPB\t%d\n"), m_mfxEncParams.mfx.QPI, m_mfxEncParams.mfx.QPP, m_mfxEncParams.mfx.QPB);

    }
    msdk_printf(MSDK_STRING("Gop size\t%d\n"), m_mfxEncParams.mfx.GopPicSize);
    msdk_printf(MSDK_STRING("Ref dist\t%d\n"), m_mfxEncParams.mfx.GopRefDist);
    msdk_printf(MSDK_STRING("Ref number\t%d\n"), m_mfxEncParams.mfx.NumRefFrame);
    msdk_printf(MSDK_STRING("Idr Interval\t%d\n"), m_mfxEncParams.mfx.IdrInterval);
    msdk_printf(MSDK_STRING("Target usage\t%s\n"), TargetUsageToStr(m_mfxEncParams.mfx.TargetUsage));

    const msdk_char* sMemType =
#if defined(_WIN32) || defined(_WIN64)
        m_memType == D3D9_MEMORY  ? MSDK_STRING("d3d")
#else
        m_memType == VAAPI_MEMORY  ? MSDK_STRING("vaapi")
#endif
        : (m_memType == D3D11_MEMORY ? MSDK_STRING("d3d11")
        : MSDK_STRING("system"));
    msdk_printf(MSDK_STRING("Memory type\t%s\n"), sMemType);

    mfxIMPL impl;
    GetFirstSession().QueryIMPL(&impl);

    const msdk_char* sImpl = (MFX_IMPL_VIA_D3D11 == MFX_IMPL_VIA_MASK(impl)) ? MSDK_STRING("hw_d3d11")
                     : (MFX_IMPL_HARDWARE  == MFX_IMPL_BASETYPE(impl)) ? MSDK_STRING("hw")
                     : (MFX_IMPL_HARDWARE2 == MFX_IMPL_BASETYPE(impl)) ? MSDK_STRING("hw2")
                     : (MFX_IMPL_HARDWARE3 == MFX_IMPL_BASETYPE(impl)) ? MSDK_STRING("hw3")
                     : (MFX_IMPL_HARDWARE4 == MFX_IMPL_BASETYPE(impl)) ? MSDK_STRING("hw4")
                                                   : MSDK_STRING("sw");
    msdk_printf(MSDK_STRING("Media SDK impl\t\t%s\n"), sImpl);

    mfxVersion ver;
    GetFirstSession().QueryVersion(&ver);
    msdk_printf(MSDK_STRING("Media SDK version\t%d.%d\n"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));
}
