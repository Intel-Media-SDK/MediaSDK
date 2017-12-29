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
#include "fei_encode.h"


FEI_EncodeInterface::FEI_EncodeInterface(MFXVideoSession* session, mfxU32 allocId, bufList* ext_bufs, AppConfig* config)
    : m_pmfxSession(session)
    , m_pmfxENCODE(new MFXVideoENCODE(*session))
    , m_allocId(allocId)
    , m_pExtBuffers(ext_bufs)
    , m_pAppConfig(config)
    , m_SyncPoint(0)
    , m_bSingleFieldMode(config->bFieldProcessingMode)
    , m_pMvPred_in(NULL)
    , m_pENC_MBCtrl_in(NULL)
    , m_pMbQP_in(NULL)
    , m_pRepackCtrl_in(NULL)
    , m_pWeights_in(NULL)
    , m_pMBstat_out(NULL)
    , m_pMV_out(NULL)
    , m_pMBcode_out(NULL)
#if (MFX_VERSION >= 1025)
    , m_pRepackStat_out(NULL)
#endif
{
    MSDK_ZERO_MEMORY(m_mfxBS);
    MSDK_ZERO_MEMORY(m_videoParams);
    MSDK_ZERO_MEMORY(m_encodeControl);
    m_encodeControl.FrameType = MFX_FRAMETYPE_UNKNOWN;
    m_encodeControl.QP = m_pAppConfig->QP;

    m_InitExtParams.reserve(5);

    /* Default values for I-frames */
    memset(&m_tmpMBencMV, 0x8000, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB));
}

FEI_EncodeInterface::~FEI_EncodeInterface()
{
    MSDK_SAFE_DELETE(m_pmfxENCODE);

    mfxExtFeiSliceHeader* pSlices = NULL;

    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_PARAM:
        {
            mfxExtFeiParam* ptr = reinterpret_cast<mfxExtFeiParam*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE(ptr);
        }
        break;

        case MFX_EXTBUFF_CODING_OPTION2:
        {
            mfxExtCodingOption2* ptr = reinterpret_cast<mfxExtCodingOption2*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE(ptr);
        }
        break;

        case MFX_EXTBUFF_CODING_OPTION3:
        {
            mfxExtCodingOption3* ptr = reinterpret_cast<mfxExtCodingOption3*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE(ptr);
        }
        break;

        case MFX_EXTBUFF_FEI_SLICE:
        {
            mfxExtFeiSliceHeader* ptr = reinterpret_cast<mfxExtFeiSliceHeader*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE_ARRAY(ptr->Slice);
            if (!pSlices) { pSlices = ptr; }
        }
        break;
        }
    }
    MSDK_SAFE_DELETE_ARRAY(pSlices);
    m_InitExtParams.clear();

    WipeMfxBitstream(&m_mfxBS);

    SAFE_FCLOSE(m_pMvPred_in);
    SAFE_FCLOSE(m_pENC_MBCtrl_in);
    SAFE_FCLOSE(m_pMbQP_in);
    SAFE_FCLOSE(m_pRepackCtrl_in);
    SAFE_FCLOSE(m_pWeights_in);
    SAFE_FCLOSE(m_pMBstat_out);
    SAFE_FCLOSE(m_pMV_out);
    SAFE_FCLOSE(m_pMBcode_out);
#if (MFX_VERSION >= 1025)
    SAFE_FCLOSE(m_pRepackStat_out);
#endif

    m_pAppConfig->PipelineCfg.pEncodeVideoParam = NULL;
}

void FEI_EncodeInterface::GetRefInfo(
    mfxU16 & picStruct,
    mfxU16 & refDist,
    mfxU16 & numRefFrame,
    mfxU16 & gopSize,
    mfxU16 & gopOptFlag,
    mfxU16 & idrInterval,
    mfxU16 & numRefActiveP,
    mfxU16 & numRefActiveBL0,
    mfxU16 & numRefActiveBL1,
    mfxU16 & bRefType,
    bool   & bSigleFieldProcessing)
{
    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
            case MFX_EXTBUFF_CODING_OPTION2:
            {
                mfxExtCodingOption2* ptr = reinterpret_cast<mfxExtCodingOption2*>(m_InitExtParams[i]);
                bRefType = ptr->BRefType;
            }
            break;

            case MFX_EXTBUFF_CODING_OPTION3:
            {
                mfxExtCodingOption3* ptr = reinterpret_cast<mfxExtCodingOption3*>(m_InitExtParams[i]);
                numRefActiveP   = ptr->NumRefActiveP[0];
                numRefActiveBL0 = ptr->NumRefActiveBL0[0];
                numRefActiveBL1 = ptr->NumRefActiveBL1[0];
            }
            break;

            case MFX_EXTBUFF_FEI_PARAM:
            {
                mfxExtFeiParam* ptr = reinterpret_cast<mfxExtFeiParam*>(m_InitExtParams[i]);
                m_bSingleFieldMode = bSigleFieldProcessing = ptr->SingleFieldProcessing == MFX_CODINGOPTION_ON;
            }
            break;

            default:
                break;
        }
    }

    picStruct   = m_videoParams.mfx.FrameInfo.PicStruct;
    refDist     = m_videoParams.mfx.GopRefDist;
    numRefFrame = m_videoParams.mfx.NumRefFrame;
    gopSize     = m_videoParams.mfx.GopPicSize;
    gopOptFlag  = m_videoParams.mfx.GopOptFlag;
    idrInterval = m_videoParams.mfx.IdrInterval;
}

mfxStatus FEI_EncodeInterface::FillParameters()
{
    mfxStatus sts = MFX_ERR_NONE;

    /* Share ENCODE video parameters with other interfaces */
    m_pAppConfig->PipelineCfg.pEncodeVideoParam = &m_videoParams;

    m_videoParams.AllocId = m_allocId;

    m_videoParams.mfx.CodecId           = m_pAppConfig->CodecId;
    m_videoParams.mfx.TargetUsage       = 0; // FEI doesn't have support of
    m_videoParams.mfx.TargetKbps        = 0; // these features
    m_videoParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP; // For now FEI work with RATECONTROL_CQP only

    sts = ConvertFrameRate(m_pAppConfig->dFrameRate, &m_videoParams.mfx.FrameInfo.FrameRateExtN, &m_videoParams.mfx.FrameInfo.FrameRateExtD);
    MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");

    /* Binary flag, 0 signals encoder to take frames in display order. PREENC, ENCPAK, ENC, PAK interfaces works only in encoded order */
    m_videoParams.mfx.EncodedOrder = mfxU16(m_pAppConfig->EncodedOrder || (m_pAppConfig->bPREENC || m_pAppConfig->bENCPAK || m_pAppConfig->bOnlyENC || m_pAppConfig->bOnlyPAK));

    m_videoParams.mfx.QPI = m_videoParams.mfx.QPP = m_videoParams.mfx.QPB = m_pAppConfig->QP;

    /* Specify memory type */
    m_videoParams.IOPattern = mfxU16(m_pAppConfig->bUseHWmemory ? MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    if (m_pAppConfig->bDECODE && !m_pAppConfig->bVPP)
    {
        MSDK_CHECK_POINTER(m_pAppConfig->PipelineCfg.pDecodeVideoParam, MFX_ERR_NULL_PTR);
        /* in case of decoder without VPP copy FrameInfo from decoder */
        MSDK_MEMCPY_VAR(m_videoParams.mfx.FrameInfo, &m_pAppConfig->PipelineCfg.pDecodeVideoParam->mfx.FrameInfo, sizeof(mfxFrameInfo));
        m_videoParams.mfx.FrameInfo.PicStruct = m_pAppConfig->nPicStruct;

        m_pAppConfig->nWidth  = m_pAppConfig->nDstWidth  = m_videoParams.mfx.FrameInfo.CropW;
        m_pAppConfig->nHeight = m_pAppConfig->nDstHeight = m_videoParams.mfx.FrameInfo.CropH;
    }
    else if (m_pAppConfig->bVPP)
    {
        MSDK_CHECK_POINTER(m_pAppConfig->PipelineCfg.pVppVideoParam, MFX_ERR_NULL_PTR);
        /* in case of VPP copy FrameInfo from VPP output */
        MSDK_MEMCPY_VAR(m_videoParams.mfx.FrameInfo, &m_pAppConfig->PipelineCfg.pVppVideoParam->vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        // frame info parameters
        m_videoParams.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_videoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_videoParams.mfx.FrameInfo.PicStruct    = m_pAppConfig->nPicStruct;

        // Set frame size and crops
        // Width must be a multiple of 16
        // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
        m_videoParams.mfx.FrameInfo.Width  = MSDK_ALIGN16(m_pAppConfig->nDstWidth);
        m_videoParams.mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_videoParams.mfx.FrameInfo.PicStruct) ?
                                             MSDK_ALIGN16(m_pAppConfig->nDstHeight) : MSDK_ALIGN32(m_pAppConfig->nDstHeight);

        m_videoParams.mfx.FrameInfo.CropX = 0;
        m_videoParams.mfx.FrameInfo.CropY = 0;
        m_videoParams.mfx.FrameInfo.CropW = m_pAppConfig->nDstWidth;
        m_videoParams.mfx.FrameInfo.CropH = m_pAppConfig->nDstHeight;
    }

    m_videoParams.AsyncDepth       = 1; //current limitation
    m_videoParams.mfx.GopRefDist   = (std::max)(m_pAppConfig->refDist, mfxU16(1));
    m_videoParams.mfx.GopPicSize   = (std::max)(m_pAppConfig->gopSize, mfxU16(1));
    m_videoParams.mfx.IdrInterval  = m_pAppConfig->nIdrInterval;
    m_videoParams.mfx.GopOptFlag   = m_pAppConfig->GopOptFlag;
    m_videoParams.mfx.CodecProfile = m_pAppConfig->CodecProfile;
    m_videoParams.mfx.CodecLevel   = m_pAppConfig->CodecLevel;

    /* Multi references and multi slices*/
    m_videoParams.mfx.NumRefFrame = (std::max)(m_pAppConfig->numRef, mfxU16(1));
    m_videoParams.mfx.NumSlice    = (std::max)(m_pAppConfig->numSlices, mfxU16(1));

    // configure trellis, B-pyramid, RAW-reference settings
    mfxExtCodingOption2* pCodingOption2 = new mfxExtCodingOption2;
    MSDK_ZERO_MEMORY(*pCodingOption2);
    pCodingOption2->Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    pCodingOption2->Header.BufferSz = sizeof(mfxExtCodingOption2);
    pCodingOption2->UseRawRef = mfxU16(m_pAppConfig->bRawRef ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);
    pCodingOption2->BRefType  = m_pAppConfig->bRefType;
    pCodingOption2->Trellis   = m_pAppConfig->Trellis;
    m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(pCodingOption2));

    // configure P/B reference number, explicit weighted prediction
    mfxExtCodingOption3* pCodingOption3 = new mfxExtCodingOption3;
    MSDK_ZERO_MEMORY(*pCodingOption3);
    pCodingOption3->Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    pCodingOption3->Header.BufferSz = sizeof(mfxExtCodingOption3);
    pCodingOption3->NumRefActiveP[0]   = m_pAppConfig->NumRefActiveP;
    pCodingOption3->NumRefActiveBL0[0] = m_pAppConfig->NumRefActiveBL0;
    pCodingOption3->NumRefActiveBL1[0] = m_pAppConfig->NumRefActiveBL1;
    // weight table file is provided, so explicit weight prediction is enabled.
    pCodingOption3->WeightedPred       = m_pAppConfig->weightsFile ? MFX_WEIGHTED_PRED_EXPLICIT : MFX_WEIGHTED_PRED_UNKNOWN;
    pCodingOption3->WeightedBiPred     = m_pAppConfig->weightsFile ? MFX_WEIGHTED_PRED_EXPLICIT : MFX_WEIGHTED_PRED_UNKNOWN;
    pCodingOption3->WeightedBiPred     = m_pAppConfig->bImplicitWPB ? MFX_WEIGHTED_PRED_IMPLICIT : pCodingOption3->WeightedBiPred;

    /* values stored in m_CodingOption3 required to fill encoding task for PREENC/ENC/PAK*/
    m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(pCodingOption3));


    /* Create extension buffer to Init FEI ENCODE */
    mfxExtFeiParam* pExtBufInit = new mfxExtFeiParam;
    MSDK_ZERO_MEMORY(*pExtBufInit);

    pExtBufInit->Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    pExtBufInit->Header.BufferSz = sizeof(mfxExtFeiParam);
    pExtBufInit->Func = MFX_FEI_FUNCTION_ENCODE;

    pExtBufInit->SingleFieldProcessing = mfxU16(m_bSingleFieldMode ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);
    m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(pExtBufInit));

    /* Create extension buffer to init deblocking parameters */
    if (m_pAppConfig->DisableDeblockingIdc || m_pAppConfig->SliceAlphaC0OffsetDiv2 || m_pAppConfig->SliceBetaOffsetDiv2)
    {
        mfxU16 numFields = (m_videoParams.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;

        mfxExtFeiSliceHeader* pSliceHeader = new mfxExtFeiSliceHeader[numFields];
        MSDK_ZERO_ARRAY(pSliceHeader, numFields);

        for (mfxU16 fieldId = 0; fieldId < numFields; ++fieldId)
        {
            pSliceHeader[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_SLICE;
            pSliceHeader[fieldId].Header.BufferSz = sizeof(mfxExtFeiSliceHeader);

            pSliceHeader[fieldId].NumSlice = m_videoParams.mfx.NumSlice;

            pSliceHeader[fieldId].Slice = new mfxExtFeiSliceHeader::mfxSlice[pSliceHeader[fieldId].NumSlice];
            MSDK_ZERO_ARRAY(pSliceHeader[fieldId].Slice, pSliceHeader[fieldId].NumSlice);

            for (mfxU16 sliceNum = 0; sliceNum < pSliceHeader[fieldId].NumSlice; ++sliceNum)
            {
                pSliceHeader[fieldId].Slice[sliceNum].DisableDeblockingFilterIdc = m_pAppConfig->DisableDeblockingIdc;
                pSliceHeader[fieldId].Slice[sliceNum].SliceAlphaC0OffsetDiv2     = m_pAppConfig->SliceAlphaC0OffsetDiv2;
                pSliceHeader[fieldId].Slice[sliceNum].SliceBetaOffsetDiv2        = m_pAppConfig->SliceBetaOffsetDiv2;
            }
            m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pSliceHeader[fieldId]));
        }
    }

    if (!m_InitExtParams.empty())
    {
        m_videoParams.ExtParam    = m_InitExtParams.data();
        m_videoParams.NumExtParam = (mfxU16)m_InitExtParams.size();
    }

    /* Init file pointers if some input buffers are specified */

    if (!m_pAppConfig->bPREENC && m_pMvPred_in == NULL && m_pAppConfig->mvinFile != NULL) //not load if we couple with PREENC
    {
        printf("Using MV input file: %s\n", m_pAppConfig->mvinFile);
        MSDK_FOPEN(m_pMvPred_in, m_pAppConfig->mvinFile, MSDK_CHAR("rb"));
        if (m_pMvPred_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mvinFile);
            exit(-1);
        }

        /* Alloc temporal buffers */
        if (m_pAppConfig->bRepackPreencMV)
        {
            m_tmpForMedian.resize(16);

            mfxU32 n_MB = m_pAppConfig->bDynamicRC ? m_pAppConfig->PipelineCfg.numMB_drc_max : m_pAppConfig->PipelineCfg.numMB_frame;
            m_tmpForReading.resize(n_MB);
        }
    }

    mfxU32 nEncodedDataBufferSize = m_videoParams.mfx.FrameInfo.Width * m_videoParams.mfx.FrameInfo.Height * 4;
    sts = InitMfxBitstream(&m_mfxBS, nEncodedDataBufferSize);
    MSDK_CHECK_STATUS_SAFE(sts, "InitMfxBitstream failed", WipeMfxBitstream(&m_mfxBS));

    if (m_pRepackCtrl_in == NULL && m_pAppConfig->repackctrlFile != NULL)
    {
        printf("Using Frame Size control input file: %s\n", m_pAppConfig->repackctrlFile);
        MSDK_FOPEN(m_pRepackCtrl_in, m_pAppConfig->repackctrlFile, MSDK_CHAR("rb"));
        if (m_pRepackCtrl_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->repackctrlFile);
            exit(-1);
        }
    }

#if (MFX_VERSION >= 1025)
    if (m_pRepackStat_out == NULL && m_pAppConfig->repackstatFile != NULL)
    {
        printf("Using Repack status output file: %s\n", m_pAppConfig->repackstatFile);
        MSDK_FOPEN(m_pRepackStat_out, m_pAppConfig->repackstatFile, MSDK_CHAR("w"));
        if (m_pRepackStat_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->repackstatFile);
            exit(-1);
        }
    }
#endif

    if (m_pENC_MBCtrl_in == NULL && m_pAppConfig->mbctrinFile != NULL)
    {
        printf("Using MB control input file: %s\n", m_pAppConfig->mbctrinFile);
        MSDK_FOPEN(m_pENC_MBCtrl_in, m_pAppConfig->mbctrinFile, MSDK_CHAR("rb"));
        if (m_pENC_MBCtrl_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbctrinFile);
            exit(-1);
        }
    }

    if (m_pMbQP_in == NULL && m_pAppConfig->mbQpFile != NULL)
    {
        printf("Use MB QP input file: %s\n", m_pAppConfig->mbQpFile);
        MSDK_FOPEN(m_pMbQP_in, m_pAppConfig->mbQpFile, MSDK_CHAR("rb"));
        if (m_pMbQP_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbQpFile);
            exit(-1);
        }
    }

    if (m_pMBstat_out == NULL && m_pAppConfig->mbstatoutFile != NULL)
    {
        printf("Use MB distortion output file: %s\n", m_pAppConfig->mbstatoutFile);
        MSDK_FOPEN(m_pMBstat_out, m_pAppConfig->mbstatoutFile, MSDK_CHAR("wb"));
        if (m_pMBstat_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbstatoutFile);
            exit(-1);
        }
    }

    if (m_pMV_out == NULL && m_pAppConfig->mvoutFile != NULL)
    {
        printf("Use MV output file: %s\n", m_pAppConfig->mvoutFile);

        MSDK_FOPEN(m_pMV_out, m_pAppConfig->mvoutFile, MSDK_CHAR("wb"));
        if (m_pMV_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mvoutFile);
            exit(-1);
        }
    }

    if (m_pMBcode_out == NULL && m_pAppConfig->mbcodeoutFile != NULL)
    {
        printf("Use MB code output file: %s\n", m_pAppConfig->mbcodeoutFile);

        MSDK_FOPEN(m_pMBcode_out, m_pAppConfig->mbcodeoutFile, MSDK_CHAR("wb"));
        if (m_pMBcode_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbcodeoutFile);
            exit(-1);
        }
    }

    if (m_pWeights_in == NULL && m_pAppConfig->weightsFile != NULL)
    {
        printf("Using Prediction Weights Table input file: %s\n", m_pAppConfig->weightsFile);

        MSDK_FOPEN(m_pWeights_in, m_pAppConfig->weightsFile, MSDK_CHAR("rb"));
        if (m_pWeights_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->weightsFile);
            exit(-1);
        }
    }

    sts = m_FileWriter.Init(m_pAppConfig->dstFileBuff[0]);
    MSDK_CHECK_STATUS(sts, "FEI ENCODE: FileWriter.Init failed");

    return sts;
}

mfxStatus FEI_EncodeInterface::InitFrameParams(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* encodeSurface = eTask->ENC_in.InSurface;

    /* Alloc temporal buffers */
    if (m_pAppConfig->bRepackPreencMV && !m_tmpForReading.size())
    {
        mfxU32 n_MB = m_pAppConfig->bDynamicRC ? m_pAppConfig->PipelineCfg.numMB_drc_max : m_pAppConfig->PipelineCfg.numMB_frame;
        m_tmpForReading.resize(n_MB);
    }

    // Store current set of Ext Buffers
    eTask->bufs = m_pExtBuffers->GetFreeSet();
    MSDK_CHECK_POINTER(eTask->bufs, MFX_ERR_NULL_PTR);

    /* Adjust number of MBs in extension buffers */
    if (m_pAppConfig->PipelineCfg.DRCresetPoint || m_pAppConfig->PipelineCfg.mixedPicstructs)
    {
        mfxU32 n_MB = m_pAppConfig->PipelineCfg.DRCresetPoint ? m_pAppConfig->PipelineCfg.numMB_drc_curr : // DRC
            (!eTask->m_fieldPicFlag ? m_pAppConfig->PipelineCfg.numMB_frame                              : // Mixed Picstructs : progressive
            m_pAppConfig->PipelineCfg.numMB_refPic);                                                       // Mixed Picstructs : interlaced

        eTask->bufs->ResetMBnum(n_MB, m_pAppConfig->PipelineCfg.DRCresetPoint);
    }

    mfxU8 ffid = eTask->m_fieldPicFlag && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF);
    if (m_videoParams.mfx.EncodedOrder)
    {
        // We have to force frame type through control in case of ENCODE in EncodedOrder mode
        m_encodeControl.FrameType = eTask->m_fieldPicFlag ? createType(*eTask) : ExtractFrameType(*eTask);
    }
    else if (eTask->m_type[ffid] & MFX_FRAMETYPE_IDR)
    {
        // As well as IDR frame in case of loop mode
        m_encodeControl.FrameType = !eTask->m_fieldPicFlag ? eTask->m_type[ffid] : (((mfxU16(eTask->m_type[1 - ffid])) << 8) | eTask->m_type[ffid]);
    }
    else
    {
        // No forced frametype otherwise
        m_encodeControl.FrameType = 0;
    }

    /* Load input Buffer for FEI ENCODE */
    mfxU32 feiEncCtrlId = ffid, pMvPredId = ffid, pWeightsId = ffid, encMBID = 0, mbQPID = 0, fieldId = 0;
    for (std::vector<mfxExtBuffer*>::iterator it = eTask->bufs->PB_bufs.in.buffers.begin();
        it != eTask->bufs->PB_bufs.in.buffers.end(); ++it)
    {
        switch ((*it)->BufferId)
        {
        case MFX_EXTBUFF_FEI_ENC_MV_PRED:
            if (!m_pAppConfig->bPREENC && m_pMvPred_in)
            {
                if (m_pAppConfig->PipelineCfg.mixedPicstructs && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && pMvPredId != ffid)
                {
                    continue;
                }

                mfxExtFeiEncMVPredictors* pMvPredBuf = reinterpret_cast<mfxExtFeiEncMVPredictors*>(*it);

                if (!(eTask->m_type[pMvPredId] & MFX_FRAMETYPE_I))
                {
                    if (m_pAppConfig->bRepackPreencMV)
                    {
                        SAFE_FREAD(&m_tmpForReading[0], sizeof(m_tmpForReading[0])*pMvPredBuf->NumMBAlloc, 1, m_pMvPred_in, MFX_ERR_MORE_DATA);
                        repackPreenc2Enc(&m_tmpForReading[0], pMvPredBuf->MB, pMvPredBuf->NumMBAlloc, &m_tmpForMedian[0]);
                    }
                    else {
                        SAFE_FREAD(pMvPredBuf->MB, sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc, 1, m_pMvPred_in, MFX_ERR_MORE_DATA);
                    }
                }
                else{
                    int shft = m_pAppConfig->bRepackPreencMV ? sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB) : sizeof(mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB);
                    SAFE_FSEEK(m_pMvPred_in, shft*pMvPredBuf->NumMBAlloc, SEEK_CUR, MFX_ERR_MORE_DATA);
                }
            }
            pMvPredId = 1 - pMvPredId; // set to sfid
            break;

        case MFX_EXTBUFF_FEI_ENC_MB:
            if (m_pENC_MBCtrl_in){
                if (m_pAppConfig->PipelineCfg.mixedPicstructs && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && !!encMBID)
                    continue;
                mfxExtFeiEncMBCtrl* pMbEncCtrl = reinterpret_cast<mfxExtFeiEncMBCtrl*>(*it);
                SAFE_FREAD(pMbEncCtrl->MB, sizeof(pMbEncCtrl->MB[0])*pMbEncCtrl->NumMBAlloc, 1, m_pENC_MBCtrl_in, MFX_ERR_MORE_DATA);
                encMBID++;
            }
            break;

        case MFX_EXTBUFF_FEI_ENC_QP:
            if (m_pMbQP_in){
                if (m_pAppConfig->PipelineCfg.mixedPicstructs && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && !!mbQPID)
                    continue;
                mfxExtFeiEncQP* pMbQP = reinterpret_cast<mfxExtFeiEncQP*>(*it);
#if MFX_VERSION >= 1023
                SAFE_FREAD(pMbQP->MB, sizeof(pMbQP->MB[0])*pMbQP->NumMBAlloc, 1, m_pMbQP_in, MFX_ERR_MORE_DATA);
#else
                SAFE_FREAD(pMbQP->QP, sizeof(pMbQP->QP[0])*pMbQP->NumQPAlloc, 1, m_pMbQP_in, MFX_ERR_MORE_DATA);
#endif
                mbQPID++;
            }
            break;

        case MFX_EXTBUFF_FEI_ENC_CTRL:
        {
            mfxExtFeiEncFrameCtrl* feiEncCtrl = reinterpret_cast<mfxExtFeiEncFrameCtrl*>(*it);

            if (m_pAppConfig->PipelineCfg.mixedPicstructs && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && feiEncCtrlId != ffid)
                continue;

            // adjust ref window size if search window is 0
            if (feiEncCtrl->SearchWindow == 0)
            {
                bool adjust_window = (eTask->m_type[feiEncCtrlId] & MFX_FRAMETYPE_B) && m_pAppConfig->RefHeight * m_pAppConfig->RefWidth > 1024;

                feiEncCtrl->RefHeight = adjust_window ? 32 : m_pAppConfig->RefHeight;
                feiEncCtrl->RefWidth  = adjust_window ? 32 : m_pAppConfig->RefWidth;
            }

            feiEncCtrl->MVPredictor = (eTask->m_type[feiEncCtrlId] & MFX_FRAMETYPE_I) ? 0 : (m_pAppConfig->mvinFile != NULL || m_pAppConfig->bPREENC);

            /* Set number to actual ref number for each field.
            Driver requires these fields to be zero in case of feiEncCtrl->MVPredictor == false
            but MSDK lib will adjust them to zero if application doesn't
            */
            feiEncCtrl->NumMVPredictors[0] = feiEncCtrl->NumMVPredictors[1] = 0;

            if (feiEncCtrl->MVPredictor)
            {
                // feiEncCtrlId tracks id in term of field parity, GetNumMVP accepts fieldId
                feiEncCtrl->NumMVPredictors[0] = GetNumL0MVPs(*eTask, feiEncCtrlId != ffid);
                feiEncCtrl->NumMVPredictors[1] = GetNumL1MVPs(*eTask, feiEncCtrlId != ffid);
            }

            fieldId++;

            feiEncCtrlId = 1 - feiEncCtrlId; // set to sfid
        }
        break;

        case MFX_EXTBUFF_FEI_REPACK_CTRL:
            if (m_pRepackCtrl_in)
            {
                mfxExtFeiRepackCtrl* feiRepackCtrl = reinterpret_cast<mfxExtFeiRepackCtrl*>(*it);
                SAFE_FREAD(&(feiRepackCtrl->MaxFrameSize), sizeof(mfxU32),    1, m_pRepackCtrl_in, MFX_ERR_MORE_DATA);
                SAFE_FREAD(&(feiRepackCtrl->NumPasses),    sizeof(mfxU32),    1, m_pRepackCtrl_in, MFX_ERR_MORE_DATA);
                SAFE_FREAD(feiRepackCtrl->DeltaQP,         sizeof(mfxU8) * 8, 1, m_pRepackCtrl_in, MFX_ERR_MORE_DATA);
                if (feiRepackCtrl->NumPasses > 4) {
                    msdk_printf(MSDK_STRING("WARNING: NumPasses should be less than or equal to 4\n"));
                }
            }
            break;

        case MFX_EXTBUFF_PRED_WEIGHT_TABLE:
            if (m_pWeights_in)
            {
                if (m_pAppConfig->PipelineCfg.mixedPicstructs && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && pWeightsId != ffid)
                    continue;

                mfxExtPredWeightTable* feiWeightTable = reinterpret_cast<mfxExtPredWeightTable*>(*it);
                if ((eTask->m_type[pWeightsId] & MFX_FRAMETYPE_P) ||
                    ((eTask->m_type[pWeightsId] & MFX_FRAMETYPE_B) && !m_pAppConfig->bImplicitWPB))
                {
                    SAFE_FREAD(&(feiWeightTable->LumaLog2WeightDenom),   sizeof(mfxU16),
                               1, m_pWeights_in, MFX_ERR_MORE_DATA);
                    SAFE_FREAD(&(feiWeightTable->ChromaLog2WeightDenom), sizeof(mfxU16),
                               1, m_pWeights_in, MFX_ERR_MORE_DATA);
                    SAFE_FREAD(feiWeightTable->LumaWeightFlag,           sizeof(mfxU16) * 2 * 32,         //[list][list entry]
                               1, m_pWeights_in, MFX_ERR_MORE_DATA);
                    SAFE_FREAD(feiWeightTable->ChromaWeightFlag,         sizeof(mfxU16) * 2 * 32,         //[list][list entry]
                               1, m_pWeights_in, MFX_ERR_MORE_DATA);
                    SAFE_FREAD(feiWeightTable->Weights,                  sizeof(mfxI16) * 2 * 32 * 3 * 2, //[list][list entry][Y, Cb, Cr][weight, offset]
                               1, m_pWeights_in, MFX_ERR_MORE_DATA);
                }
                else{
                    unsigned int seek_size = sizeof(mfxU16) + sizeof(mfxU16) + sizeof(mfxU16) * 2 * 32
                                             + sizeof(mfxU16) * 2 * 32 + sizeof(mfxI16) * 2 * 32 * 3 * 2;
                    SAFE_FSEEK(m_pWeights_in, seek_size, SEEK_CUR, MFX_ERR_MORE_DATA);
                }
            }
            pWeightsId = 1 - pWeightsId; // set to sfid
            break;
        } // switch (eTask->bufs->PB_bufs.in.ExtParam[i]->BufferId)
    } // for (int i = 0; i<eTask->bufs->PB_bufs.in.NumExtParam; i++)

    // Add input buffers
    bool is_I_frame = !eTask->m_fieldPicFlag && (eTask->m_type[ffid] & MFX_FRAMETYPE_I);

    // Initialize controller for Extension buffers
    sts = eTask->ExtBuffersController.InitializeController(eTask->bufs, bufSetController::ENCODE, is_I_frame, !eTask->m_fieldPicFlag);
    MSDK_CHECK_STATUS(sts, "eTask->ExtBuffersController.InitializeController failed");

    return sts;
}

mfxStatus FEI_EncodeInterface::AllocateSufficientBuffer()
{
    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    mfxStatus sts = m_pmfxENCODE->GetVideoParam(&par);
    MSDK_CHECK_STATUS(sts, "m_pmfxENCODE->GetVideoParam failed");

    // reallocate bigger buffer for output
    sts = ExtendMfxBitstream(&m_mfxBS, par.mfx.BufferSizeInKB * 1000);
    MSDK_CHECK_STATUS_SAFE(sts, "ExtendMfxBitstream failed", WipeMfxBitstream(&m_mfxBS));

    return sts;
}

mfxStatus FEI_EncodeInterface::EncodeOneFrame(iTask* eTask)
{
    MFX_ITT_TASK("EncodeOneFrame");

    mfxStatus sts = MFX_ERR_NONE;

    // ENC_in.InSurface always holds full-res surface
    mfxFrameSurface1* encodeSurface = eTask ? eTask->ENC_in.InSurface : NULL;
    if (encodeSurface) // no need to do this for buffered frames
    {
        sts = InitFrameParams(eTask);
        MSDK_CHECK_STATUS(sts, "FEI ENCODE: InitFrameParams failed");
    }

    for (int i = 0; i < 1 + m_bSingleFieldMode; ++i)
    {
        for (;;) {
            // at this point surface for encoder contains either a frame from file or a frame processed by vpp

            if (encodeSurface)
            {
                // Attach extension buffers for current field
                // (in double-field mode both calls will return equal sets, holding buffers for both fields)

                std::vector<mfxExtBuffer *> * in_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::ENCODE, i, true);
                MSDK_CHECK_POINTER(in_buffers, MFX_ERR_NULL_PTR);

                std::vector<mfxExtBuffer *> * out_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::ENCODE, i, false);
                MSDK_CHECK_POINTER(out_buffers, MFX_ERR_NULL_PTR);

                // Input buffers
                m_encodeControl.NumExtParam = mfxU16(in_buffers->size());
                m_encodeControl.ExtParam    = !in_buffers->empty() ? in_buffers->data() : NULL;

                // Output buffers
                m_mfxBS.NumExtParam = mfxU16(out_buffers->size());
                m_mfxBS.ExtParam    = !out_buffers->empty() ? out_buffers->data() : NULL;
            }

            // Encoding goes below
            sts = m_pmfxENCODE->EncodeFrameAsync(&m_encodeControl, encodeSurface, &m_mfxBS, &m_SyncPoint);
            MSDK_CHECK_WRN(sts, "WRN during EncodeFrameAsync");

            if (MFX_ERR_NONE < sts && !m_SyncPoint) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                {
                    WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
                }
            }
            else if (MFX_ERR_NONE < sts && m_SyncPoint)
            {
                // ignore warnings if output is available
                sts = m_pmfxSession->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);
                MSDK_CHECK_STATUS(sts, "FEI ENCODE: SyncOperation failed");
                break;
            }
            else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
            {
                sts = AllocateSufficientBuffer();
                MSDK_CHECK_STATUS(sts, "FEI ENCODE: AllocateSufficientBuffer failed");
            }
            else
            {
                if (m_SyncPoint)
                {
                    sts = m_pmfxSession->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);
                    MSDK_CHECK_STATUS(sts, "FEI ENCODE: SyncOperation failed");
                }
                break;
            }
        } // for(;;)

        MSDK_BREAK_ON_ERROR(sts);

    } // for (int i = 0; i < 1 + m_bSingleFieldMode; ++i)

    if (sts == MFX_ERR_MORE_DATA)
    {
        // MFX_ERR_MORE_DATA is correct status to finish encoding of buffered frames (for which encodeSurface == NULL).
        // Otherwise, ignore it
        return encodeSurface ? MFX_ERR_NONE : MFX_ERR_MORE_DATA;
    }

    MSDK_CHECK_STATUS(sts, "FEI ENCODE: EncodeFrameAsync failed");

    sts = m_FileWriter.WriteNextFrame(&m_mfxBS);
    MSDK_CHECK_STATUS(sts, "FEI ENCODE: WriteNextFrame failed");

    sts = FlushOutput(eTask);
    MSDK_CHECK_STATUS(sts, "FEI ENCODE: FlushOutput failed");

    return sts;
}

mfxStatus FEI_EncodeInterface::FlushOutput(iTask* eTask)
{
    mfxExtBuffer** output_buffers   = NULL;
    mfxU32         n_output_buffers = 0;

    if (eTask)
    {
        MSDK_CHECK_POINTER(eTask->bufs, MFX_ERR_NULL_PTR);
        output_buffers   = eTask->bufs->PB_bufs.out.buffers.data();
        n_output_buffers = eTask->bufs->PB_bufs.out.buffers.size();
    }
    else
    {
        // In case of draining frames from encoder in display-order mode
        output_buffers   = m_mfxBS.ExtParam;
        n_output_buffers = m_mfxBS.NumExtParam;
    }

    mfxU32 mvBufCounter = 0;

    for (mfxU32 i = 0; i < n_output_buffers; ++i)
    {
        MSDK_CHECK_POINTER(output_buffers[i], MFX_ERR_NULL_PTR);

        switch (output_buffers[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_ENC_MV:
            if (m_pMV_out)
            {
                mfxExtFeiEncMV* mvBuf = reinterpret_cast<mfxExtFeiEncMV*>(output_buffers[i]);
                if (!(extractType(m_mfxBS.FrameType, mvBufCounter) & MFX_FRAMETYPE_I)){
                    SAFE_FWRITE(mvBuf->MB, sizeof(mvBuf->MB[0])*mvBuf->NumMBAlloc, 1, m_pMV_out, MFX_ERR_MORE_DATA);
                }
                else
                {
                    for (mfxU32 k = 0; k < mvBuf->NumMBAlloc; k++){
                        SAFE_FWRITE(&m_tmpMBencMV, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB), 1, m_pMV_out, MFX_ERR_MORE_DATA);
                    }
                }
                ++mvBufCounter;
            }
            break;

        case MFX_EXTBUFF_FEI_ENC_MB_STAT:
            if (m_pMBstat_out)
            {
                mfxExtFeiEncMBStat* mbstatBuf = reinterpret_cast<mfxExtFeiEncMBStat*>(output_buffers[i]);
                SAFE_FWRITE(mbstatBuf->MB, sizeof(mbstatBuf->MB[0])*mbstatBuf->NumMBAlloc, 1, m_pMBstat_out, MFX_ERR_MORE_DATA);
            }
            break;

        case MFX_EXTBUFF_FEI_PAK_CTRL:
            if (m_pMBcode_out)
            {
                mfxExtFeiPakMBCtrl* mbcodeBuf = reinterpret_cast<mfxExtFeiPakMBCtrl*>(output_buffers[i]);
                SAFE_FWRITE(mbcodeBuf->MB, sizeof(mbcodeBuf->MB[0])*mbcodeBuf->NumMBAlloc, 1, m_pMBcode_out, MFX_ERR_MORE_DATA);
            }
            break;

#if (MFX_VERSION >= 1025)
        case MFX_EXTBUFF_FEI_REPACK_STAT:
            if (m_pRepackStat_out)
            {
                mfxExtFeiRepackStat* repackStat = reinterpret_cast<mfxExtFeiRepackStat*>
                                                  (output_buffers[i]);
                mfxI8 repackStatChar[64];
                snprintf(repackStatChar, sizeof(repackStatChar),
                         "FrameOrder %d: %d NumPasses\n",
                         eTask?eTask->m_frameOrder:0, repackStat->NumPasses);
                SAFE_FWRITE(repackStatChar, strlen(repackStatChar), 1,
                            m_pRepackStat_out, MFX_ERR_MORE_DATA);
                repackStat->NumPasses = 0;
            }
            break;
#endif

        } // switch (output_buffers[i]->BufferId)
    } // for (mfxU32 i = 0; i < n_output_buffers; ++i)

    return MFX_ERR_NONE;
}

mfxStatus FEI_EncodeInterface::ResetState()
{
    mfxStatus sts = MFX_ERR_NONE;

    //while (MFX_ERR_NONE == sts) {
    //    sts = EncodeOneFrame(NULL, NULL, PairU8(NULL,NULL));
    //}

    // mark sync point as free
    m_SyncPoint = NULL;

    // prepare bit stream
    m_mfxBS.DataOffset = 0;
    m_mfxBS.DataLength = 0;

    // reset FileWriter
    sts = m_FileWriter.Init(m_pAppConfig->dstFileBuff[0]);
    MSDK_CHECK_STATUS(sts, "FEI ENCODE: FileWriter.Init failed");

    SAFE_FSEEK(m_pMvPred_in,     0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pENC_MBCtrl_in, 0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMbQP_in,       0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pRepackCtrl_in, 0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pWeights_in,    0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMBstat_out,    0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMV_out,        0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMBcode_out,    0, SEEK_SET, MFX_ERR_MORE_DATA);
#if (MFX_VERSION >= 1025)
    SAFE_FSEEK(m_pRepackStat_out,0, SEEK_SET, MFX_ERR_MORE_DATA);
#endif

    return sts;
}
