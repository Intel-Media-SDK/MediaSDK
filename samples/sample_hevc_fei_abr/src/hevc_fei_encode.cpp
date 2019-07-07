/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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

#include "fei_utils.h"
#include "hevc_fei_encode.h"

FEI_Encode::FEI_Encode(MFXVideoSession* session, MfxVideoParamsWrapper& par,
        const mfxExtFeiHevcEncFrameCtrl& frame_ctrl, const PerFrameTypeCtrl& frametype_ctrl,
        const msdk_char* outFile, const sBrcParams& brc_params)
    : m_pmfxSession(session)
    , m_mfxENCODE(*m_pmfxSession)
    , m_videoParams(par)
    , m_syncPoint(0)
    , m_dstFileName(outFile)
    , m_defFrameCtrl(frame_ctrl)
    , m_ctrlPerFrameType(frametype_ctrl)
    , m_pBRC(CreateBRC(brc_params, m_videoParams))
{
    m_encodeCtrl.FrameType = MFX_FRAMETYPE_UNKNOWN;

    m_working_queue.Start();

}

FEI_Encode::~FEI_Encode()
{
    m_working_queue.Stop();

    m_mfxENCODE.Close();
    m_pmfxSession = nullptr;
}

mfxStatus FEI_Encode::PreInit()
{
    // call Query to check that Encode's parameters are valid
    mfxStatus sts = Query();
    MSDK_CHECK_STATUS(sts, "FEI Encode Query failed");

    m_bitstream.Extend(m_videoParams.mfx.FrameInfo.Width * m_videoParams.mfx.FrameInfo.Height * 4);

    // add FEI frame ctrl with default values
    mfxExtFeiHevcEncFrameCtrl* ctrl = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncFrameCtrl>();
    MSDK_CHECK_POINTER(ctrl, MFX_ERR_NOT_INITIALIZED);
    *ctrl = m_defFrameCtrl;

    mfxExtHEVCRefLists* pRefLists = m_encodeCtrl.AddExtBuffer<mfxExtHEVCRefLists>();
    MSDK_CHECK_POINTER(pRefLists, MFX_ERR_NOT_INITIALIZED);

    mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncMVPredictors>();
    MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);

    mfxExtFeiHevcEncCtuCtrl* pCtuCtrl = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncCtuCtrl>();
    MSDK_CHECK_POINTER(pCtuCtrl, MFX_ERR_NOT_INITIALIZED);

    return MFX_ERR_NONE;
}

mfxStatus FEI_Encode::Query()
{
    mfxStatus sts = m_mfxENCODE.Query(&m_videoParams, &m_videoParams);
    MSDK_CHECK_WRN(sts, "FEI Encode Query");

    return sts;
}

mfxStatus FEI_Encode::Init()
{
    mfxStatus sts = m_FileWriter.Init(m_dstFileName.c_str());
    MSDK_CHECK_STATUS(sts, "FileWriter Init failed");

    sts = m_mfxENCODE.Init(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode Init failed");
    MSDK_CHECK_WRN(sts, "FEI Encode Init");

    // just in case, call GetVideoParam to get current Encoder state
    sts = m_mfxENCODE.GetVideoParam(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode GetVideoParam failed");

    return sts;
}

mfxStatus FEI_Encode::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_mfxENCODE.QueryIOSurf(&m_videoParams, request);
}

MfxVideoParamsWrapper FEI_Encode::GetVideoParam()
{
    return m_videoParams;
}

mfxStatus FEI_Encode::Reset(mfxVideoParam& par)
{
    m_videoParams = par;
    mfxStatus sts = m_mfxENCODE.Reset(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode Reset failed");
    MSDK_CHECK_WRN(sts, "FEI Encode Reset");

    // just in case, call GetVideoParam to get current Encoder state
    sts = m_mfxENCODE.GetVideoParam(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode GetVideoParam failed");

    return sts;
}

mfxStatus FEI_Encode::DoWork(std::shared_ptr<HevcTaskDSO> & task)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL;
    if (task.get())
    {
        sts = SetCtrlParams(*task);
        MSDK_CHECK_STATUS(sts, "FEI Encode::SetCtrlParams failed");
        pSurf = task->m_surf;

        // Set up BRC frame QP
        if (m_pBRC.get())
        {
            m_pBRC->PreEnc(task->m_statData);
            m_encodeCtrl.QP = m_pBRC->GetQP();
        }
    }

    sts = EncodeFrame(pSurf);
    MSDK_CHECK_STATUS(sts, "FEI EncodeFrame failed");

    return sts;
}

mfxStatus FEI_Encode::EncodeFrame(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NONE;

    for (;;)
    {
        sts = m_mfxENCODE.EncodeFrameAsync(&m_encodeCtrl, pSurf, &m_bitstream, &m_syncPoint);
        MSDK_CHECK_WRN(sts, "FEI EncodeFrameAsync");

        if (MFX_ERR_NONE < sts && !m_syncPoint) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                WaitForDeviceToBecomeFree(*m_pmfxSession, m_syncPoint, sts);
            }
            continue;
        }

        if (MFX_ERR_NONE <= sts && m_syncPoint) // ignore warnings if output is available
        {
            sts = SyncOperation();
            break;
        }

        if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
        {
            sts = AllocateSufficientBuffer();
            MSDK_CHECK_STATUS(sts, "FEI Encode: AllocateSufficientBuffer failed");
        }

        MSDK_BREAK_ON_ERROR(sts);

    } // for (;;)

    // when pSurf==NULL, MFX_ERR_MORE_DATA indicates all cached frames within encoder were drained, return as is
    // otherwise encoder need more input surfaces, ignore status
    if (MFX_ERR_MORE_DATA == sts && pSurf)
    {
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    }
    msdk_printf(MSDK_STRING("."));

    return sts;
}

void FillHEVCRefLists(const HevcTaskDSO& task, mfxExtHEVCRefLists & refLists)
{
    refLists.NumRefIdxL0Active = task.m_refListActive[0].size();
    refLists.NumRefIdxL1Active = task.m_refListActive[1].size();

    for (mfxU32 direction = 0; direction < 2; ++direction)
    {
        mfxExtHEVCRefLists::mfxRefPic (&refPicList)[32] = (direction == 0) ? refLists.RefPicList0 : refLists.RefPicList1;
        for (mfxU32 i = 0; i < task.m_refListActive[direction].size(); ++i)
        {
            refPicList[i].FrameOrder = task.m_refListActive[direction][i];
            refPicList[i].PicStruct  = MFX_PICSTRUCT_UNKNOWN;
        }
    }
}

mfxStatus FEI_Encode::SetCtrlParams(const HevcTaskDSO& task)
{
    m_encodeCtrl.FrameType = task.m_frameType;

    mfxExtHEVCRefLists* pRefLists = m_encodeCtrl.GetExtBuffer<mfxExtHEVCRefLists>();
    MSDK_CHECK_POINTER(pRefLists, MFX_ERR_NOT_INITIALIZED);

    FillHEVCRefLists(task, *pRefLists);

    mfxExtFeiHevcEncFrameCtrl* ctrl = m_encodeCtrl;
    MSDK_CHECK_POINTER(ctrl, MFX_ERR_NOT_INITIALIZED);

    // 0 - no predictors (for I-frames),
    // 1 - enabled per 16x16 block,
    // 2 - enabled per 32x32 block (default for repacker),
    // 7 - inherit size of block from buffers CTU setting (default for external file with predictors)
    ctrl->MVPredictor = (m_encodeCtrl.FrameType & MFX_FRAMETYPE_I) ? 0 : m_defFrameCtrl.MVPredictor;

    if (ctrl->MVPredictor)
    {
        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>();
        MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);

        *pMVP = *task.m_mvp; // shallow copy
    }

    ctrl->NumMvPredictors[0] = task.m_nMvPredictors[0];
    ctrl->NumMvPredictors[1] = task.m_nMvPredictors[1];

    switch (m_encodeCtrl.FrameType  & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B))
    {
        case MFX_FRAMETYPE_I:
            ctrl->FastIntraMode      = m_ctrlPerFrameType.CtrlI.FastIntraMode;
            ctrl->ForceCtuSplit      = m_ctrlPerFrameType.CtrlI.ForceCtuSplit;
            ctrl->NumFramePartitions = m_ctrlPerFrameType.CtrlI.NumFramePartitions;
            break;
        case MFX_FRAMETYPE_P:
            ctrl->FastIntraMode      = m_ctrlPerFrameType.CtrlP.FastIntraMode;
            ctrl->ForceCtuSplit      = m_ctrlPerFrameType.CtrlP.ForceCtuSplit;
            ctrl->NumFramePartitions = m_ctrlPerFrameType.CtrlP.NumFramePartitions;
            break;
        case MFX_FRAMETYPE_B:
            if (task.m_isGPBFrame)
            {
                ctrl->FastIntraMode      = m_ctrlPerFrameType.CtrlP.FastIntraMode;
                ctrl->ForceCtuSplit      = m_ctrlPerFrameType.CtrlP.ForceCtuSplit;
                ctrl->NumFramePartitions = m_ctrlPerFrameType.CtrlP.NumFramePartitions;
            }
            else
            {
                ctrl->FastIntraMode      = m_ctrlPerFrameType.CtrlB.FastIntraMode;
                ctrl->ForceCtuSplit      = m_ctrlPerFrameType.CtrlB.ForceCtuSplit;
                ctrl->NumFramePartitions = m_ctrlPerFrameType.CtrlB.NumFramePartitions;
            }
            break;
        default:
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid m_encodeCtrl.FrameType");
            break;
    }

    if (task.m_ctuCtrl.get())
    {
        ctrl->PerCtuInput = (m_encodeCtrl.FrameType & MFX_FRAMETYPE_I) ? 0 : m_defFrameCtrl.PerCtuInput;

        if (ctrl->PerCtuInput)
        {
            mfxExtFeiHevcEncCtuCtrl* pCtuCtrl = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncCtuCtrl>();
            MSDK_CHECK_POINTER(pCtuCtrl, MFX_ERR_NOT_INITIALIZED);

            *pCtuCtrl = *task.m_ctuCtrl; // shallow copy
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus FEI_Encode::SyncOperation()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_pmfxSession->SyncOperation(m_syncPoint, MSDK_WAIT_INTERVAL);
    MSDK_CHECK_STATUS(sts, "FEI Encode: SyncOperation failed");

    mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>();
    MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);

    // Update BRC with coded segment size
    if (m_pBRC.get())
    {
        m_pBRC->Report(m_bitstream.DataLength);
    }

    sts = m_FileWriter.WriteNextFrame(&m_bitstream, false);
    MSDK_CHECK_STATUS(sts, "FEI Encode: WriteNextFrame failed");

    return sts;
}

mfxStatus FEI_Encode::AllocateSufficientBuffer()
{
    // find out the required buffer size
    // call GetVideoParam to get current Encoder state
    mfxStatus sts = m_mfxENCODE.GetVideoParam(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode GetVideoParam failed");

    m_bitstream.Extend(m_videoParams.mfx.BufferSizeInKB * 1000);

    return sts;
}

mfxStatus MVPOverlay::Init()
{
    mfxStatus sts = m_pBase->Init();
    MSDK_CHECK_STATUS(sts, "m_pBase->Init failed");

    m_yuvWriter.Init(m_yuvName.c_str(), 1);
    MSDK_CHECK_STATUS(sts, "FileWriter Init failed");

    MfxVideoParamsWrapper par = m_pBase->GetVideoParam();
    m_width  = par.mfx.FrameInfo.Width;
    m_height = par.mfx.FrameInfo.Height;
    sts = m_surface.Alloc(par.mfx.FrameInfo.FourCC, m_width, m_height);
    MSDK_CHECK_STATUS(sts, "m_surface.Alloc failed");

    return sts;
}

mfxStatus MVPOverlay::SubmitFrame(std::shared_ptr<HevcTaskDSO> & task)
{
    if (task.get() && task->m_surf)
    {
        mfxFrameSurface1 & s = *task->m_surf;
        mfxStatus sts = m_allocator->Lock(m_allocator->pthis, s.Data.MemId, &s.Data);
        MSDK_CHECK_STATUS(sts, "m_allocator->Lock failed");

        sts = CopySurface(m_surface, s);
        MSDK_CHECK_STATUS(sts, "CopySurface failed");

        sts = m_allocator->Unlock(m_allocator->pthis, s.Data.MemId, &s.Data);
        MSDK_CHECK_STATUS(sts, "m_allocator->Unlock failed");

        if (task->m_mvp.get())
        {
            mfxExtFeiHevcEncMVPredictors & mvp = *task->m_mvp.get();
            AutoBufferLocker<mfxExtFeiHevcEncMVPredictors> lock(*m_buffAlloc.get(), mvp);

            for (mfxU32 tileIdx = 0; tileIdx < mvp.Pitch * mvp.Height; ++tileIdx)
            {
                mfxFeiHevcEncMVPredictors & block = mvp.Data[tileIdx];
                if (block.BlockSize)
                {
                    mfxU32 rasterScanIdx = (tileIdx & 1) + (((tileIdx & 3) >> 1 ) * mvp.Pitch) +
                                            (((tileIdx % (2 * mvp.Pitch)) >> 2 ) << 1) +
                                             ((tileIdx / (2 * mvp.Pitch)) * 2 * mvp.Pitch);

                    mfxU32 x = (rasterScanIdx % mvp.Pitch) << 4; // left top point
                    mfxU32 y = (rasterScanIdx / mvp.Pitch) << 4;

                    x += 8; // center
                    y += 8;

                    for (mfxU32 i = 0; i < 4; ++i)
                    {
                        // ">> 2" below since MVs are in q-pixels
                        if (block.RefIdx[i].RefL0 != 0xf)
                            DrawLine(x, y, block.MV[i][0].x >> 2, block.MV[i][0].y >> 2, m_surface.Data.Y, m_width, m_height, 0);   // L0 - black

                        if (block.RefIdx[i].RefL1 != 0xf)
                            DrawLine(x, y, block.MV[i][1].x >> 2, block.MV[i][1].y >> 2, m_surface.Data.Y, m_width, m_height, 200); // L1 - white
                    }
                }
            }
        }

        sts = m_yuvWriter.WriteNextFrame(&m_surface);
        MSDK_CHECK_STATUS(sts, "WriteNextFrame failed");
    }

    return m_pBase->SubmitFrame(task);
}
