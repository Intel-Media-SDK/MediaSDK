/******************************************************************************\
Copyright (c) 2017-2018, Intel Corporation
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

FEI_Encode::FEI_Encode(MFXVideoSession* session, mfxHDL hdl,
        MfxVideoParamsWrapper& encode_pars,
        const mfxExtFeiHevcEncFrameCtrl& frameCtrl, const PerFrameTypeCtrl& frametypeCtrl,
        const msdk_char* strDstFile, const msdk_char* mvpInFile,
        const msdk_char* repackctrlFile, const msdk_char* repackstatFile,
        PredictorsRepaking* repacker)
    : m_pmfxSession(session)
    , m_mfxENCODE(*m_pmfxSession)
    , m_buf_allocator(hdl, encode_pars.mfx.FrameInfo.Width, encode_pars.mfx.FrameInfo.Height)
    , m_videoParams(encode_pars)
    , m_syncPoint(0)
    , m_dstFileName(strDstFile)
    , m_defFrameCtrl(frameCtrl)
    , m_ctrlPerFrameType(frametypeCtrl)
    , m_processedFrames(0)
    , m_repackCtrlFileName(repackctrlFile)
    , m_repackStatFileName(repackstatFile)
    , m_mvpInFileName(mvpInFile)
{
    m_encodeCtrl.FrameType = MFX_FRAMETYPE_UNKNOWN;

    m_repacker.reset(repacker);
}

FEI_Encode::~FEI_Encode()
{
    msdk_printf(MSDK_STRING("\nEncode processed %u frames\n"), m_processedFrames);
    m_mfxENCODE.Close();
    m_pmfxSession = NULL;

    try
    {
        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>();
        if (pMVP)
        {
            m_buf_allocator.Free(pMVP);
        }

        mfxExtFeiHevcEncQP* pQP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncQP>();
        if (pQP)
        {
            m_buf_allocator.Free(pQP);
        }

        mfxExtBRC* pBrc = m_videoParams.GetExtBuffer<mfxExtBRC>();
        if (pBrc)
        {
            HEVCExtBRC::Destroy(*pBrc);
        }
    }
    catch(mfxError& ex)
    {
        msdk_printf("Exception raised in FEI Encode destructor sts = %d, msg = %s\n", ex.GetStatus(), ex.what());
    }
    catch(...)
    {
        msdk_printf("Exception raised in FEI Encode destructor\n");
    }

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

    if (!m_mvpInFileName.empty())
    {
        m_pFile_MVP_in.reset(new FileHandler(m_mvpInFileName.c_str(), MSDK_STRING("rb")));
    }

    // allocate ext buffer for input MV predictors required for Encode.
    if (m_repacker.get() || m_pFile_MVP_in.get())
    {
        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncMVPredictors>();
        MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);
        pMVP->VaBufferID = VA_INVALID_ID;
    }

    if (!m_repackCtrlFileName.empty())
    {
        m_pFile_repack_ctrl.reset(new FileHandler(m_repackCtrlFileName.c_str(), MSDK_STRING("rb")));
    }

    if (m_pFile_repack_ctrl.get())
    {
        mfxExtFeiHevcRepackCtrl *pRepackCtrl = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcRepackCtrl>();
        MSDK_CHECK_POINTER(pRepackCtrl, MFX_ERR_NOT_INITIALIZED);

        pRepackCtrl->MaxFrameSize = 0;
        pRepackCtrl->NumPasses = 1;
        MSDK_ZERO_MEMORY(pRepackCtrl->DeltaQP);
    }

    if (!m_repackStatFileName.empty())
    {
        m_pFile_repack_stat.reset(new FileHandler(m_repackStatFileName.c_str(), MSDK_STRING("w")));
    }

    if (m_pFile_repack_stat.get())
    {
        mfxExtFeiHevcRepackStat *pRepackStat = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcRepackStat>();
        MSDK_CHECK_POINTER(pRepackStat, MFX_ERR_NOT_INITIALIZED);
        pRepackStat->NumPasses = 0;
    }

    mfxExtBRC* pBrc = m_videoParams.GetExtBuffer<mfxExtBRC>();
    if (pBrc)
    {
        sts = HEVCExtBRC::Create(*pBrc);
        MSDK_CHECK_STATUS(sts, "HEVCExtBRC Create failed");
    }

    sts = ResetExtBuffers(m_videoParams);
    MSDK_CHECK_STATUS(sts, "FEI Encode ResetExtBuffers failed");

    return MFX_ERR_NONE;
}

mfxStatus FEI_Encode::ResetExtBuffers(const MfxVideoParamsWrapper & videoParams)
{

    BufferAllocRequest request;
    MSDK_ZERO_MEMORY(request);

    request.Width = videoParams.mfx.FrameInfo.CropW;
    request.Height = videoParams.mfx.FrameInfo.CropH;
    try
    {
        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>();
        if (pMVP)
        {
            m_buf_allocator.Free(pMVP);

            m_buf_allocator.Alloc(pMVP, request);
        }

        mfxExtFeiHevcEncQP* pQP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncQP>();
        if (pQP)
        {
            m_buf_allocator.Free(pQP);

            m_buf_allocator.Alloc(pQP, request);
        }
    }
    catch (mfxError& ex)
    {
        MSDK_CHECK_STATUS(ex.GetStatus(), ex.what());
    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

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
    mfxStatus sts = MFX_ERR_NONE;
    if (m_dstFileName.length())
    {
        sts = m_FileWriter.Init(m_dstFileName.c_str());
        MSDK_CHECK_STATUS(sts, "FileWriter Init failed");
    }

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

mfxStatus FEI_Encode::SetRepackCtrl()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxExtFeiHevcRepackCtrl *pRepackCtrl = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcRepackCtrl>();
    MSDK_CHECK_POINTER(pRepackCtrl, MFX_ERR_NOT_INITIALIZED);

    sts = m_pFile_repack_ctrl->Read(&(pRepackCtrl->MaxFrameSize), sizeof(mfxU32), 1);
    MSDK_CHECK_STATUS(sts, "FEI Encode Read repack ctrl MaxFrameSize failed");

    sts = m_pFile_repack_ctrl->Read(&(pRepackCtrl->NumPasses), sizeof(mfxU32), 1);
    MSDK_CHECK_STATUS(sts, "FEI Encode Read repack ctrl NumPasses failed");

    sts = m_pFile_repack_ctrl->Read(pRepackCtrl->DeltaQP, sizeof(mfxU8), 8);
    MSDK_CHECK_STATUS(sts, "FEI Encode Read repack ctrl DeltaQP failed");

    if (pRepackCtrl->NumPasses > 8)
    {
        msdk_printf(MSDK_STRING("ERROR: HEVC NumPasses should be less than or equal to 8\n"));
        sts = MFX_ERR_UNSUPPORTED;
    }

    return sts;
}

mfxStatus FEI_Encode::GetRepackStat()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxI8 repackStatChar[64];

    mfxExtFeiHevcRepackStat *pRepackStat = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcRepackStat>();
    MSDK_CHECK_POINTER(pRepackStat, MFX_ERR_NOT_INITIALIZED);

    snprintf(repackStatChar, sizeof(repackStatChar),
             "%d NumPasses\n", pRepackStat->NumPasses);
    sts = m_pFile_repack_stat->Write(repackStatChar, strlen(repackStatChar), 1);

    pRepackStat->NumPasses = 0;

    return sts;
}

// in encoded order
mfxStatus FEI_Encode::EncodeFrame(HevcTask* task)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL;
    if (task)
    {
        sts = SetCtrlParams(*task);
        MSDK_CHECK_STATUS(sts, "FEI Encode::SetCtrlParams failed");
        pSurf = task->m_surf;
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

        if (m_pFile_repack_ctrl.get() && pSurf)
        {
            sts = SetRepackCtrl();
            MSDK_CHECK_STATUS(sts, "FEI Encode::SetRepackCtrl failed");
        }

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

            if ((MFX_ERR_NONE == sts) && m_pFile_repack_stat.get())
            {
                mfxStatus repack_sts = GetRepackStat();
                MSDK_CHECK_WRN(repack_sts, "FEI Encode::GetRepackStat failed");
            }
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

    return sts;
}

void FillHEVCRefLists(const HevcTask& task, mfxExtHEVCRefLists & refLists)
{
    const mfxU8 (&RPL)[2][MAX_DPB_SIZE] = task.m_refPicList;
    const HevcDpbArray & DPB = task.m_dpb[TASK_DPB_ACTIVE];

    refLists.NumRefIdxL0Active = task.m_numRefActive[0];
    refLists.NumRefIdxL1Active = task.m_numRefActive[1];

    for (mfxU32 direction = 0; direction < 2; ++direction)
    {
        mfxExtHEVCRefLists::mfxRefPic (&refPicList)[32] = (direction == 0) ? refLists.RefPicList0 : refLists.RefPicList1;
        for (mfxU32 i = 0; i < MAX_DPB_SIZE; ++i)
        {
            const mfxU8 & idx = RPL[direction][i];
            if (idx < MAX_DPB_SIZE)
            {
                refPicList[i].FrameOrder = DPB[idx].m_fo;
                refPicList[i].PicStruct  = MFX_PICSTRUCT_UNKNOWN;
            }
        }
    }
}

mfxStatus FEI_Encode::SetCtrlParams(const HevcTask& task)
{
    m_encodeCtrl.FrameType = task.m_frameType;

    mfxExtHEVCRefLists* pRefLists = m_encodeCtrl.GetExtBuffer<mfxExtHEVCRefLists>();
    MSDK_CHECK_POINTER(pRefLists, MFX_ERR_NOT_INITIALIZED);

    FillHEVCRefLists(task, *pRefLists);

    // Get Frame Control buffer
    mfxExtFeiHevcEncFrameCtrl* ctrl = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncFrameCtrl>();
    MSDK_CHECK_POINTER(ctrl, MFX_ERR_NOT_INITIALIZED);

    if (m_repacker.get() || m_pFile_MVP_in.get())
    {
        // 0 - no predictors (for I-frames),
        // 1 - enabled per 16x16 block,
        // 2 - enabled per 32x32 block (default for repacker),
        // 7 - inherit size of block from buffers CTU setting (default for external file with predictors)
        ctrl->MVPredictor = (m_encodeCtrl.FrameType & MFX_FRAMETYPE_I) ? 0 : m_defFrameCtrl.MVPredictor;

        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>();
        MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);
        AutoBufferLocker<mfxExtFeiHevcEncMVPredictors> lock(m_buf_allocator, *pMVP);

        ctrl->NumMvPredictors[0] = 0;
        ctrl->NumMvPredictors[1] = 0;

        if (m_repacker.get())
        {
            mfxStatus sts = m_repacker->RepackPredictors(task, *pMVP, ctrl->NumMvPredictors);
            MSDK_CHECK_STATUS(sts, "FEI Encode::RepackPredictors failed");
        }
        else
        {
            mfxStatus sts = m_pFile_MVP_in->Read(pMVP->Data, pMVP->Pitch * pMVP->Height * sizeof(pMVP->Data[0]), 1);
            MSDK_CHECK_STATUS(sts, "FEI Encode. Read MV predictors failed");

            if (m_encodeCtrl.FrameType & MFX_FRAMETYPE_P)
            {
                ctrl->NumMvPredictors[0] = m_defFrameCtrl.NumMvPredictors[0];
            }
            else if (m_encodeCtrl.FrameType & MFX_FRAMETYPE_B)
            {
                ctrl->NumMvPredictors[0] = m_defFrameCtrl.NumMvPredictors[0];
                ctrl->NumMvPredictors[1] = m_defFrameCtrl.NumMvPredictors[1];
            }
        }
    }

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
            ctrl->FastIntraMode      = m_ctrlPerFrameType.CtrlB.FastIntraMode;
            ctrl->ForceCtuSplit      = m_ctrlPerFrameType.CtrlB.ForceCtuSplit;
            ctrl->NumFramePartitions = m_ctrlPerFrameType.CtrlB.NumFramePartitions;
            break;
        default:
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid m_encodeCtrl.FrameType");
            break;
    }

    return MFX_ERR_NONE;
}

mfxStatus FEI_Encode::SyncOperation()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_pmfxSession->SyncOperation(m_syncPoint, MSDK_WAIT_INTERVAL);
    MSDK_CHECK_STATUS(sts, "FEI Encode: SyncOperation failed");

    m_processedFrames++;

    if (m_dstFileName.length())
    {
        sts = m_FileWriter.WriteNextFrame(&m_bitstream);
        MSDK_CHECK_STATUS(sts, "FEI Encode: WriteNextFrame failed");
    }
    // mark that there's no need bit stream data any more
    m_bitstream.DataLength = 0;

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

mfxStatus FEI_Encode::ResetIOState()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_bitstream.DataOffset = 0;
    m_bitstream.DataLength = 0;
    m_syncPoint = 0;

    if (m_dstFileName.length())
    {
        sts = m_FileWriter.Reset();
        MSDK_CHECK_STATUS(sts, "FEI Encode: file writer reset failed");
    }

    if (m_pFile_MVP_in.get())
    {
        sts = m_pFile_MVP_in->Seek(0, SEEK_SET);
        MSDK_CHECK_STATUS(sts, "FEI Encode: failed to rewind file with MVP");
    }

    return sts;
}
