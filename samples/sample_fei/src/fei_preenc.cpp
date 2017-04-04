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

#include "fei_preenc.h"

FEI_PreencInterface::FEI_PreencInterface(MFXVideoSession* session, iTaskPool* task_pool, mfxU32 allocId, bufList* ext_bufs, bufList* enc_ext_bufs, AppConfig* config)
    : m_pmfxSession(session)
    , m_pmfxPREENC(new MFXVideoENC(*session))
    , m_pmfxDS(config->preencDSstrength ? new MFXVideoVPP(*session) : NULL)
    , m_inputTasks(task_pool)
    , m_allocId(allocId)
    , m_pExtBuffers(ext_bufs)
    , m_pEncExtBuffers(enc_ext_bufs)
    , m_pAppConfig(config)
    , m_SyncPoint(0)
    , m_bSingleFieldMode(config->bFieldProcessingMode)
    , m_DSstrength(config->preencDSstrength)
    , m_bMVout(m_pAppConfig->bENCODE || m_pAppConfig->bOnlyENC || m_pAppConfig->bENCPAK)
    , m_bMBStatout(m_pAppConfig->bENCODE || m_pAppConfig->bOnlyENC || m_pAppConfig->bENCPAK)
    , m_pMvPred_in(NULL)
    , m_pMbQP_in(NULL)
    , m_pMBstat_out(NULL)
    , m_pMV_out(NULL)
{
    MSDK_ZERO_MEMORY(m_videoParams);
    MSDK_ZERO_MEMORY(m_DSParams);
    MSDK_ZERO_MEMORY(m_FullResParams);

    m_InitExtParams.reserve(1);
    m_DSExtParams.reserve(1);

    if (m_pAppConfig->bENCODE || m_pAppConfig->bOnlyENC || m_pAppConfig->bENCPAK)
    {
        m_tmpForMedian.resize(16);
    }

    /* Default values for I-frames */
    memset(&m_tmpMVMB, 0x8000, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB));
}

FEI_PreencInterface::~FEI_PreencInterface()
{
    MSDK_SAFE_DELETE(m_pmfxPREENC);
    MSDK_SAFE_DELETE(m_pmfxDS);

    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_PARAM:
            mfxExtFeiParam* ptr = reinterpret_cast<mfxExtFeiParam*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE(ptr);
            break;
        }
    }
    m_InitExtParams.clear();

    for (mfxU32 i = 0; i < m_DSExtParams.size(); ++i)
    {
        switch (m_DSExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_VPP_DONOTUSE:
            mfxExtVPPDoNotUse* ptr = reinterpret_cast<mfxExtVPPDoNotUse*>(m_DSExtParams[i]);
            MSDK_SAFE_DELETE_ARRAY(ptr->AlgList);
            MSDK_SAFE_DELETE(ptr);
            break;
        }
    }
    m_DSExtParams.clear();

    SAFE_FCLOSE(m_pMvPred_in);
    SAFE_FCLOSE(m_pMbQP_in);
    SAFE_FCLOSE(m_pMBstat_out);
    SAFE_FCLOSE(m_pMV_out);

    m_pAppConfig->PipelineCfg.pDownSampleVideoParam = NULL;
    m_pAppConfig->PipelineCfg.pPreencVideoParam     = NULL;
}

mfxStatus FEI_PreencInterface::Init()
{
    if (m_pmfxDS)
    {
        mfxStatus sts = m_pmfxDS->Init(&m_DSParams);
        MSDK_CHECK_STATUS(sts, "FEI PreENC DS: Init failed");
    }
    return m_pmfxPREENC->Init(&m_videoParams);
}

mfxStatus FEI_PreencInterface::Close()
{
    if (m_pmfxDS)
    {
        mfxStatus sts = m_pmfxDS->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "FEI PreENC DS: Close failed");
    }
    return m_pmfxPREENC->Close();
}

mfxStatus FEI_PreencInterface::Reset(mfxU16 width, mfxU16 height, mfxU16 crop_w, mfxU16 crop_h)
{
    if (width && height && crop_w && crop_h)
    {
        if (m_pmfxDS)
        {
            m_DSParams.vpp.In.Width  = width;
            m_DSParams.vpp.In.Height = height;
            m_DSParams.vpp.In.CropW  = crop_w;
            m_DSParams.vpp.In.CropH  = crop_h;

            width  = crop_w = m_DSParams.vpp.Out.CropW = m_DSParams.vpp.Out.Width  = MSDK_ALIGN16(width / m_pAppConfig->preencDSstrength);
            height = crop_h = m_DSParams.vpp.Out.CropH = m_DSParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_DSParams.vpp.Out.PicStruct) ?
                MSDK_ALIGN16(height / m_pAppConfig->preencDSstrength) : MSDK_ALIGN32(height / m_pAppConfig->preencDSstrength);
        }

        m_videoParams.mfx.FrameInfo.Width  = width;
        m_videoParams.mfx.FrameInfo.Height = height;
        m_videoParams.mfx.FrameInfo.CropW  = crop_w;
        m_videoParams.mfx.FrameInfo.CropH  = crop_h;
    }

    if (m_pmfxDS)
    {
        mfxStatus sts = m_pmfxDS->Reset(&m_DSParams);
        MSDK_CHECK_STATUS(sts, "FEI PreENC DS: Reset failed");
    }
    return m_pmfxPREENC->Reset(&m_videoParams);
}

mfxStatus FEI_PreencInterface::QueryIOSurf(mfxFrameAllocRequest* preenc_request, mfxFrameAllocRequest ds_request[2])
{
    mfxStatus sts = MFX_ERR_NULL_PTR;

    if (ds_request)
    {
        if (m_pmfxDS)
        {
            sts = m_pmfxDS->QueryIOSurf(&m_DSParams, ds_request);
            MSDK_CHECK_STATUS(sts, "FEI PreENC DS: QueryIOSurf failed");
        }
        else
            return MFX_ERR_NOT_INITIALIZED;
    }

    if (preenc_request)
    {
        m_pmfxPREENC->QueryIOSurf(&m_videoParams, preenc_request);
    }

    return sts;
}

/* This function return video params that applicable to the subsequent interfaces.
In DS case PreENC videoParam correspondes to downsampled frames which are used
by PreENC only, the following interfaces expects full resolution frames.
*/
mfxVideoParam* FEI_PreencInterface::GetCommonVideoParams()
{
    return &m_FullResParams;
}

mfxStatus FEI_PreencInterface::UpdateVideoParam()
{
    if (m_pmfxDS)
    {
        mfxStatus sts = m_pmfxDS->GetVideoParam(&m_DSParams);
        MSDK_CHECK_STATUS(sts, "FEI PreENC DS: GetVideoParam failed");
    }
    return m_pmfxPREENC->GetVideoParam(&m_videoParams);
}

void FEI_PreencInterface::GetRefInfo(
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
    numRefActiveP   = m_pAppConfig->NumRefActiveP;
    numRefActiveBL0 = m_pAppConfig->NumRefActiveBL0;
    numRefActiveBL1 = m_pAppConfig->NumRefActiveBL1;

    bRefType = m_pAppConfig->bRefType == MFX_B_REF_UNKNOWN ? MFX_B_REF_OFF : m_pAppConfig->bRefType;

    picStruct   = m_videoParams.mfx.FrameInfo.PicStruct;
    refDist     = m_videoParams.mfx.GopRefDist;
    numRefFrame = m_videoParams.mfx.NumRefFrame;
    gopSize     = m_videoParams.mfx.GopPicSize;
    gopOptFlag  = m_videoParams.mfx.GopOptFlag;
    idrInterval = m_videoParams.mfx.IdrInterval;

    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
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
}

mfxStatus FEI_PreencInterface::FillDSVideoParams()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pAppConfig->PipelineCfg.pDownSampleVideoParam = &m_DSParams;

    m_DSParams.AllocId      = m_allocId;
    // specify memory type
    m_DSParams.IOPattern    = mfxU16(m_pAppConfig->bUseHWmemory ? (MFX_IOPATTERN_IN_VIDEO_MEMORY  | MFX_IOPATTERN_OUT_VIDEO_MEMORY)
                                                                : (MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY));
    m_DSParams.AsyncDepth   = 1;

    if (m_pAppConfig->bDECODE && !m_pAppConfig->bVPP)
    {
        /* Decoder present in pipeline */
        MSDK_MEMCPY_VAR(m_DSParams.vpp.In, &m_pAppConfig->PipelineCfg.pDecodeVideoParam->mfx.FrameInfo, sizeof(mfxFrameInfo));
        m_DSParams.vpp.In.PicStruct = m_pAppConfig->nPicStruct;
    }
    else if (m_pAppConfig->bVPP)
    {
        /* There is VPP stage before PreENC DownSampling in pipeline */
        MSDK_CHECK_POINTER(m_pAppConfig->PipelineCfg.pVppVideoParam, MFX_ERR_NULL_PTR);
        MSDK_MEMCPY_VAR(m_DSParams.vpp.In, &m_pAppConfig->PipelineCfg.pVppVideoParam->vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        /* No other instances before PreENC DownSampling. Fill VideoParams */

        // input frame info
        m_DSParams.vpp.In.FourCC       = MFX_FOURCC_NV12;
        m_DSParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_DSParams.vpp.In.PicStruct    = m_pAppConfig->nPicStruct;

        sts = ConvertFrameRate(m_pAppConfig->dFrameRate, &m_DSParams.vpp.In.FrameRateExtN, &m_DSParams.vpp.In.FrameRateExtD);
        MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");

        // width must be a multiple of 16
        // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
        m_DSParams.vpp.In.Width = MSDK_ALIGN16(m_pAppConfig->nWidth);
        m_DSParams.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_DSParams.vpp.In.PicStruct) ?
            MSDK_ALIGN16(m_pAppConfig->nHeight) : MSDK_ALIGN32(m_pAppConfig->nHeight);

        // set crops in input mfxFrameInfo for correct work of file reader
        // VPP itself ignores crops at initialization
        m_DSParams.vpp.In.CropX = m_DSParams.vpp.In.CropY = 0;
        m_DSParams.vpp.In.CropW = m_pAppConfig->nWidth;
        m_DSParams.vpp.In.CropH = m_pAppConfig->nHeight;
    }

    // fill output frame info
    MSDK_MEMCPY_VAR(m_DSParams.vpp.Out, &m_DSParams.vpp.In, sizeof(mfxFrameInfo));

    // only resizing is supported
    m_DSParams.vpp.Out.CropX = m_DSParams.vpp.Out.CropY  = 0;
    m_DSParams.vpp.Out.CropW = m_DSParams.vpp.Out.Width  = MSDK_ALIGN16(m_pAppConfig->nDstWidth / m_pAppConfig->preencDSstrength);
    m_DSParams.vpp.Out.CropH = m_DSParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_DSParams.vpp.Out.PicStruct) ?
        MSDK_ALIGN16(m_pAppConfig->nDstHeight / m_pAppConfig->preencDSstrength) : MSDK_ALIGN32(m_pAppConfig->nDstHeight / m_pAppConfig->preencDSstrength);


    // configure and attach external parameters
    mfxExtVPPDoNotUse* m_VppDoNotUse = new mfxExtVPPDoNotUse;
    MSDK_ZERO_MEMORY(*m_VppDoNotUse);
    m_VppDoNotUse->Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    m_VppDoNotUse->Header.BufferSz = sizeof(mfxExtVPPDoNotUse);
    m_VppDoNotUse->NumAlg = 4;

    m_VppDoNotUse->AlgList    = new mfxU32[m_VppDoNotUse->NumAlg];
    m_VppDoNotUse->AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;        // turn off denoising (on by default)
    m_VppDoNotUse->AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS; // turn off scene analysis (on by default)
    m_VppDoNotUse->AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;         // turn off detail enhancement (on by default)
    m_VppDoNotUse->AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;        // turn off processing amplified (on by default)
    m_DSExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_VppDoNotUse));

    m_DSParams.ExtParam    = &m_DSExtParams[0]; // vector is stored linearly in memory
    m_DSParams.NumExtParam = (mfxU16)m_DSExtParams.size();

    return sts;
}

mfxStatus FEI_PreencInterface::FillParameters()
{
    mfxStatus sts = MFX_ERR_NONE;

    /* Share PreENC video parameters with other interfaces */
    m_pAppConfig->PipelineCfg.pPreencVideoParam = &m_videoParams;

    m_videoParams.AllocId = m_allocId;

    m_videoParams.mfx.CodecId           = m_pAppConfig->CodecId;
    m_videoParams.mfx.TargetUsage       = 0; // FEI doesn't have support of
    m_videoParams.mfx.TargetKbps        = 0; // these features
    m_videoParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP; // For now FEI work with RATECONTROL_CQP only

    sts = ConvertFrameRate(m_pAppConfig->dFrameRate, &m_videoParams.mfx.FrameInfo.FrameRateExtN, &m_videoParams.mfx.FrameInfo.FrameRateExtD);
    MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");

    m_videoParams.mfx.QPI = m_videoParams.mfx.QPP = m_videoParams.mfx.QPB = m_pAppConfig->QP;

    // binary flag, 0 signals encoder to take frames in display order. PREENC, ENCPAK, ENC, PAK interfaces works only in encoded order
    m_videoParams.mfx.EncodedOrder = 1;

    // specify memory type
    m_videoParams.IOPattern = mfxU16(m_pAppConfig->bUseHWmemory ? MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    // Fill DS VPP params
    if (m_pAppConfig->preencDSstrength)
    {
        sts = FillDSVideoParams();
        MSDK_CHECK_STATUS(sts, "FEI PreENC: FillDSVideoParams failed");
    }

    if (m_pAppConfig->bDECODE && !(m_pAppConfig->preencDSstrength || m_pAppConfig->bVPP))
    {
        /* Decoder present in pipeline */
        MSDK_CHECK_POINTER(m_pAppConfig->PipelineCfg.pDecodeVideoParam, MFX_ERR_NULL_PTR);
        // in case of decoder without VPP copy FrameInfo from decoder
        MSDK_MEMCPY_VAR(m_videoParams.mfx.FrameInfo, &m_pAppConfig->PipelineCfg.pDecodeVideoParam->mfx.FrameInfo, sizeof(mfxFrameInfo));
        m_videoParams.mfx.FrameInfo.PicStruct = m_pAppConfig->nPicStruct;

        m_pAppConfig->nWidth  = m_pAppConfig->nDstWidth  = m_videoParams.mfx.FrameInfo.CropW;
        m_pAppConfig->nHeight = m_pAppConfig->nDstHeight = m_videoParams.mfx.FrameInfo.CropH;
    }
    else if (m_pAppConfig->preencDSstrength)
    {
        /* PreEnc uses downsampled input surface */
        MSDK_MEMCPY_VAR(m_videoParams.mfx.FrameInfo, &m_DSParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else if (m_pAppConfig->bVPP)
    {
        /* There is VPP stage before PreENC in pipeline (and no DownSampling stage) */
        MSDK_CHECK_POINTER(m_pAppConfig->PipelineCfg.pVppVideoParam, MFX_ERR_NULL_PTR);
        MSDK_MEMCPY_VAR(m_videoParams.mfx.FrameInfo, &m_pAppConfig->PipelineCfg.pVppVideoParam->vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        /* No other instances before PreENC. Fill VideoParams */

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
    m_videoParams.mfx.NumRefFrame = (std::max)(m_pAppConfig->numRef,    mfxU16(1));
    m_videoParams.mfx.NumSlice    = (std::max)(m_pAppConfig->numSlices, mfxU16(1));

    MSDK_MEMCPY_VAR(m_FullResParams, &m_videoParams, sizeof(mfxVideoParam));
    if (m_pmfxDS)
    {
        MSDK_MEMCPY_VAR(m_FullResParams.mfx.FrameInfo, &m_DSParams.vpp.In, sizeof(mfxFrameInfo));
    }

    /* Create extension buffer to Init FEI PREENC */
    mfxExtFeiParam* pExtBufInit = new mfxExtFeiParam;
    MSDK_ZERO_MEMORY(*pExtBufInit);

    pExtBufInit->Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    pExtBufInit->Header.BufferSz = sizeof(mfxExtFeiParam);
    pExtBufInit->Func            = MFX_FEI_FUNCTION_PREENC;

    pExtBufInit->SingleFieldProcessing = mfxU16(m_bSingleFieldMode ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);
    m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(pExtBufInit));

    if (!m_InitExtParams.empty())
    {
        m_videoParams.ExtParam    = &m_InitExtParams[0]; // vector is stored linearly in memory
        m_videoParams.NumExtParam = (mfxU16)m_InitExtParams.size();
    }

    if (m_pMvPred_in == NULL && m_pAppConfig->mvinFile)
    {
        printf("Using MV input file: %s\n", m_pAppConfig->mvinFile);
        MSDK_FOPEN(m_pMvPred_in, m_pAppConfig->mvinFile, MSDK_CHAR("rb"));
        if (m_pMvPred_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mvinFile);
            exit(-1);
        }
    }

    if (m_pMbQP_in == NULL && m_pAppConfig->mbQpFile != NULL)
    {
        printf("Using QP input file: %s\n", m_pAppConfig->mbQpFile);
        MSDK_FOPEN(m_pMbQP_in, m_pAppConfig->mbQpFile, MSDK_CHAR("rb"));
        if (m_pMbQP_in == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbQpFile);
            exit(-1);
        }
    }

    if (m_pMV_out == NULL && m_pAppConfig->mvoutFile &&
        !(m_pAppConfig->bENCODE || m_pAppConfig->bENCPAK || m_pAppConfig->bOnlyENC))
    {
        m_bMVout = true;

        printf("Using MV output file: %s\n", m_pAppConfig->mvoutFile);
        MSDK_FOPEN(m_pMV_out, m_pAppConfig->mvoutFile, MSDK_CHAR("wb"));
        if (m_pMV_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mvoutFile);
            exit(-1);
        }
    }

    if (m_pMBstat_out == NULL && m_pAppConfig->mbstatoutFile &&
        !(m_pAppConfig->bENCODE || m_pAppConfig->bENCPAK || m_pAppConfig->bOnlyENC))
    {
        m_bMBStatout = true;

        printf("Using MB distortion output file: %s\n", m_pAppConfig->mbstatoutFile);
        MSDK_FOPEN(m_pMBstat_out, m_pAppConfig->mbstatoutFile, MSDK_CHAR("wb"));
        if (m_pMBstat_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->mbstatoutFile);
            exit(-1);
        }
    }

    return sts;
}

mfxStatus FEI_PreencInterface::PreencOneFrame(iTask* eTask)
{
    MFX_ITT_TASK("PreencOneFrame");
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    /* PreENC DownSampling */
    if (m_pmfxDS)
    {
        sts = DownSampleInput(eTask);
        MSDK_CHECK_STATUS(sts, "PreENC DownsampleInput failed");
    }

    sts = ProcessMultiPreenc(eTask);
    if (sts == MFX_ERR_GPU_HANG)
    {
        return MFX_ERR_GPU_HANG;
    }
    MSDK_CHECK_STATUS(sts, "ProcessMultiPreenc failed");

    // Pass MVP to encode,encpak
    if (m_pAppConfig->bENCODE || m_pAppConfig->bENCPAK || m_pAppConfig->bOnlyENC)
    {
        if (eTask->m_fieldPicFlag || !(ExtractFrameType(*eTask) & MFX_FRAMETYPE_I))
        {
            //repack MV predictors
            MFX_ITT_TASK("RepackMVs");
            if (!m_pAppConfig->bPerfMode)
                sts = RepackPredictors(eTask);
            else
                sts = RepackPredictorsPerf(eTask);
            MSDK_CHECK_STATUS(sts, "Predictors Repacking failed");
        }
    }

    return sts;
}

mfxStatus FEI_PreencInterface::DownSampleInput(iTask* eTask)
{
    MFX_ITT_TASK("DownsampleInput");

    MSDK_CHECK_POINTER(eTask,                      MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->PREENC_in.InSurface, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->ENC_in.InSurface,    MFX_ERR_NULL_PTR);


    mfxStatus sts = MFX_ERR_NONE;

    // VPP will use this surface as output, so it should be unlocked
    mfxU16 locker = eTask->PREENC_in.InSurface->Data.Locked;
    eTask->PREENC_in.InSurface->Data.Locked = 0;

    for (;;)
    {
        sts = m_pmfxDS->RunFrameVPPAsync(eTask->ENC_in.InSurface, eTask->PREENC_in.InSurface, NULL, &m_SyncPoint);
        if (sts == MFX_ERR_GPU_HANG)
        {
            return MFX_ERR_GPU_HANG;
        }

        if (!m_SyncPoint)
        {
            if (MFX_WRN_DEVICE_BUSY == sts){
                WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
            }
            else
                return sts;
        }
        else if (MFX_ERR_NONE <= sts) {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else
            MSDK_BREAK_ON_ERROR(sts);
    }
    MSDK_CHECK_STATUS(sts, "PreENC DownsampleInput failed");

    for (;;)
    {
        sts = m_pmfxSession->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);
        if (sts == MFX_ERR_GPU_HANG)
        {
            return MFX_ERR_GPU_HANG;
        }

        if (!m_SyncPoint)
        {
            if (MFX_WRN_DEVICE_BUSY == sts){
                WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
            }
            else
                return sts;
        }
        else if (MFX_ERR_NONE <= sts) {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else
            MSDK_BREAK_ON_ERROR(sts);
    }

    // restore locker
    eTask->PREENC_in.InSurface->Data.Locked = locker;

    return sts;
}

mfxStatus FEI_PreencInterface::InitFrameParams(iTask* eTask, iTask* refTask[2][2], mfxU8 ref_fid[2][2], bool isDownsamplingNeeded)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxFrameSurface1* refSurf0[2] = { NULL, NULL }; // L0 ref surfaces
    mfxFrameSurface1* refSurf1[2] = { NULL, NULL }; // L1 ref surfaces

    eTask->preenc_bufs = m_pExtBuffers->GetFreeSet();
    MSDK_CHECK_POINTER(eTask->preenc_bufs, MFX_ERR_NULL_PTR);

    /* Adjust number of MBs in extension buffers */
    if (m_pAppConfig->PipelineCfg.DRCresetPoint || m_pAppConfig->PipelineCfg.mixedPicstructs)
    {
        mfxU32 n_MB = m_pAppConfig->PipelineCfg.DRCresetPoint ?
                      ((m_videoParams.mfx.FrameInfo.Width*m_videoParams.mfx.FrameInfo.Height)>>8) : // DRC
                      (!eTask->m_fieldPicFlag ? m_pAppConfig->PipelineCfg.numMB_preenc_frame        // Mixed Picstructs : progressive
                                              : m_pAppConfig->PipelineCfg.numMB_preenc_refPic);     // Mixed Picstructs : interlaced

        eTask->preenc_bufs->ResetMBnum(n_MB, m_pAppConfig->PipelineCfg.DRCresetPoint);
    }

    for (mfxU32 fieldId = 0; fieldId < mfxU32(eTask->m_fieldPicFlag ? 2 : 1); fieldId++)
    {
        bool is_I_frame = ExtractFrameType(*eTask, fieldId) & MFX_FRAMETYPE_I;

        refSurf0[fieldId] = refTask[fieldId][0] ? refTask[fieldId][0]->PREENC_in.InSurface : NULL;
        refSurf1[fieldId] = refTask[fieldId][1] ? refTask[fieldId][1]->PREENC_in.InSurface : NULL;

        eTask->PREENC_in.NumFrameL0 = !!refSurf0[fieldId];
        eTask->PREENC_in.NumFrameL1 = !!refSurf1[fieldId];

        //in data
        eTask->PREENC_in.NumExtParam = is_I_frame ? eTask->preenc_bufs->I_bufs.in.NumExtParam() : eTask->preenc_bufs->PB_bufs.in.NumExtParam();
        eTask->PREENC_in.ExtParam    = is_I_frame ? eTask->preenc_bufs->I_bufs.in.ExtParam()    : eTask->preenc_bufs->PB_bufs.in.ExtParam();
        //out data
        eTask->PREENC_out.NumExtParam = is_I_frame ? eTask->preenc_bufs->I_bufs.out.NumExtParam() : eTask->preenc_bufs->PB_bufs.out.NumExtParam();
        eTask->PREENC_out.ExtParam    = is_I_frame ? eTask->preenc_bufs->I_bufs.out.ExtParam()    : eTask->preenc_bufs->PB_bufs.out.ExtParam();
    }

    mfxU32 preENCCtrId = 0, pMvPredId = 0;
    mfxU8 type = MFX_FRAMETYPE_UNKNOWN;
    bool ref0_isFrame = !eTask->m_fieldPicFlag, ref1_isFrame = !eTask->m_fieldPicFlag;

    for (int i = 0; i < eTask->PREENC_in.NumExtParam; i++)
    {
        switch (eTask->PREENC_in.ExtParam[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_PREENC_CTRL:
        {
            mfxExtFeiPreEncCtrl* preENCCtr = reinterpret_cast<mfxExtFeiPreEncCtrl*>(eTask->PREENC_in.ExtParam[i]);

            /* Get type of current field */
            type = ExtractFrameType(*eTask, preENCCtrId);
            /* Disable MV output for I frames / if no reference frames provided / if no mv output requested */
            preENCCtr->DisableMVOutput = (type & MFX_FRAMETYPE_I) || (!refSurf0[preENCCtrId] && !refSurf1[preENCCtrId]) || !m_bMVout;

            preENCCtr->DisableStatisticsOutput = !m_bMBStatout;

            /* Section below sets up reference frames and their types */
            ref0_isFrame = refSurf0[preENCCtrId] ? (refSurf0[preENCCtrId]->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) :
                !eTask->m_fieldPicFlag;
            ref1_isFrame = refSurf1[preENCCtrId] ? (refSurf1[preENCCtrId]->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) :
                !eTask->m_fieldPicFlag;

            preENCCtr->RefPictureType[0] = mfxU16(ref0_isFrame ? MFX_PICTYPE_FRAME :
                ref_fid[preENCCtrId][0] == 0 ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

            preENCCtr->RefPictureType[1] = mfxU16(ref1_isFrame ? MFX_PICTYPE_FRAME :
                ref_fid[preENCCtrId][1] == 0 ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

            preENCCtr->RefFrame[0] = refSurf0[preENCCtrId];
            preENCCtr->RefFrame[1] = refSurf1[preENCCtrId];

            /* Default value is ON */
            preENCCtr->DownsampleInput = mfxU16(isDownsamplingNeeded ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);

            /* Configure MV predictors */
            preENCCtr->MVPredictor = 0;

            if (m_pMvPred_in && !(type & MFX_FRAMETYPE_I))
            {
                /* Prefer user settings (use/not use) for L0/L1 or make a decision
                   depending on whether references are presented */
                if (m_pAppConfig->bPreencPredSpecified_l0)
                    preENCCtr->MVPredictor |= (0x01 * m_pAppConfig->PreencMVPredictors[0]);
                else
                    preENCCtr->MVPredictor |= (0x01 * (!!preENCCtr->RefFrame[0]));

                if (m_pAppConfig->bPreencPredSpecified_l1)
                    preENCCtr->MVPredictor |= (0x02 * m_pAppConfig->PreencMVPredictors[1]);
                else
                    preENCCtr->MVPredictor |= (0x02 * (!!preENCCtr->RefFrame[1]));
            }

            /* In case of mixed picstructs, update type of current frame/field */
            if (m_pAppConfig->PipelineCfg.mixedPicstructs)
            {
                switch (eTask->PREENC_in.InSurface->Info.PicStruct & 0xf)
                {
                case MFX_PICSTRUCT_FIELD_TFF:
                    preENCCtr->PictureType = mfxU16(!preENCCtrId ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);
                    break;

                case MFX_PICSTRUCT_FIELD_BFF:
                    preENCCtr->PictureType = mfxU16(!preENCCtrId ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD);
                    break;

                case MFX_PICSTRUCT_PROGRESSIVE:
                default:
                    preENCCtr->PictureType = MFX_PICTYPE_FRAME;
                    break;
                }
            }

            preENCCtrId++;
        }
        break;

        case MFX_EXTBUFF_FEI_PREENC_MV_PRED:
            if (m_pMvPred_in)
            {
                /* Skip MV predictor data for I-fields/frames */
                if (!(ExtractFrameType(*eTask, pMvPredId) & MFX_FRAMETYPE_I))
                {
                    mfxExtFeiPreEncMVPredictors* pMvPredBuf = reinterpret_cast<mfxExtFeiPreEncMVPredictors*>(eTask->PREENC_in.ExtParam[i]);
                    SAFE_FREAD(pMvPredBuf->MB, sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc, 1, m_pMvPred_in, MFX_ERR_MORE_DATA);
                }
                else{
                    SAFE_FSEEK(m_pMvPred_in, sizeof(mfxExtFeiPreEncMVPredictors::mfxExtFeiPreEncMVPredictorsMB)*m_pAppConfig->PipelineCfg.numMB_preenc_refPic, SEEK_CUR, MFX_ERR_MORE_DATA);
                }
                pMvPredId++;
            }
            break;

        case MFX_EXTBUFF_FEI_ENC_QP:
            if (m_pMbQP_in)
            {
                mfxExtFeiEncQP* pMbQP = reinterpret_cast<mfxExtFeiEncQP*>(eTask->PREENC_in.ExtParam[i]);
                SAFE_FREAD(pMbQP->QP, sizeof(pMbQP->QP[0])*pMbQP->NumQPAlloc, 1, m_pMbQP_in, MFX_ERR_MORE_DATA);
            }
            break;
        }
    } // for (int i = 0; i<eTask->in.NumExtParam; i++)

    mdprintf(stderr, "enc: %d t: %d\n", eTask->m_frameOrder, (eTask->m_type[0] & MFX_FRAMETYPE_IPB));
    return MFX_ERR_NONE;
}

mfxStatus FEI_PreencInterface::ProcessMultiPreenc(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxU16 numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    // max possible candidates to L0 / L1
    mfxU16 total_l0 = (ExtractFrameType(*eTask, eTask->m_fieldPicFlag) & MFX_FRAMETYPE_P) ? ((ExtractFrameType(*eTask) & MFX_FRAMETYPE_IDR) ? 1 : eTask->NumRefActiveP) : ((ExtractFrameType(*eTask) & MFX_FRAMETYPE_I) ? 1 : eTask->NumRefActiveBL0);
    mfxU16 total_l1 = (ExtractFrameType(*eTask) & MFX_FRAMETYPE_B) ? eTask->NumRefActiveBL1 : 1; // just one iteration here for non-B

    total_l0 = (std::min)(total_l0, (mfxU16)(numOfFields*m_videoParams.mfx.NumRefFrame)); // adjust to maximal
    total_l1 = (std::min)(total_l1, (mfxU16)(numOfFields*m_videoParams.mfx.NumRefFrame)); // number of references

    mfxU16 total_mvp_l0 = (std::max)(GetNumL0MVPs(*eTask, 0), GetNumL0MVPs(*eTask, 1));
    mfxU16 total_mvp_l1 = (std::max)(GetNumL1MVPs(*eTask, 0), GetNumL1MVPs(*eTask, 1));
    if (m_pAppConfig->bPerfMode)
    {
        // adjust to maximal number of predictors
        total_l0 = (std::min)(total_l0, total_mvp_l0);
        total_l1 = (std::min)(total_l1, total_mvp_l1);
    }

    mfxU8 preenc_ref_idx[2][2]; // indexes means [fieldId][L0L1]
    mfxU8 ref_fid[2][2];
    iTask* refTask[2][2];
    bool isDownsamplingNeeded = true;

    MSDK_ZERO_ARRAY(eTask->NumMVPredictorsP,   2);
    MSDK_ZERO_ARRAY(eTask->NumMVPredictorsBL0, 2);
    MSDK_ZERO_ARRAY(eTask->NumMVPredictorsBL1, 2);

    for (mfxU8 l0_idx = 0, l1_idx = 0; l0_idx < total_l0 || l1_idx < total_l1; l0_idx++, l1_idx++)
    {
        MFX_ITT_TASK("PreENCpass");
        sts = GetRefTaskEx(eTask, l0_idx, l1_idx, preenc_ref_idx, ref_fid, refTask);

        if (ExtractFrameType(*eTask) & MFX_FRAMETYPE_I)
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_OUT_OF_RANGE);

        if (sts == MFX_WRN_OUT_OF_RANGE){ // some of the indexes exceeds DPB
            sts = MFX_ERR_NONE;
            continue;
        }
        MSDK_BREAK_ON_ERROR(sts);

        for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
        {
            if (refTask[fieldId][0])
            {
                if (eTask->NumMVPredictorsP[fieldId] < total_mvp_l0)
                {
                    eTask->NumMVPredictorsP[fieldId]++;
                    eTask->NumMVPredictorsBL0[fieldId]++;
                }
            }
            if (refTask[fieldId][1])
            {
                if (eTask->NumMVPredictorsBL1[fieldId] < total_mvp_l1)
                {
                    eTask->NumMVPredictorsBL1[fieldId]++;
                }
            }
        }

        sts = InitFrameParams(eTask, refTask, ref_fid, isDownsamplingNeeded);
        MSDK_CHECK_STATUS(sts, "PreENC: InitFrameParams failed");

        // If input surface is not being changed between PreENC calls
        // (including calls for different fields of the same field pair in Single Field processing mode),
        // an application can avoid redundant downsampling on driver side.
        isDownsamplingNeeded = false;

        // Doing PreEnc
        for (;;)
        {
            for (int i = 0; i < 1 + m_bSingleFieldMode; ++i)
            {
                sts = m_pmfxPREENC->ProcessFrameAsync(&eTask->PREENC_in, &eTask->PREENC_out, &m_SyncPoint);
                if (sts == MFX_ERR_GPU_HANG)
                {
                    return MFX_ERR_GPU_HANG;
                }
                MSDK_BREAK_ON_ERROR(sts);

                /*PRE-ENC is running in separate session */
                sts = m_pmfxSession->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);
                if (sts == MFX_ERR_GPU_HANG)
                {
                    return MFX_ERR_GPU_HANG;
                }
                MSDK_BREAK_ON_ERROR(sts);
                mdprintf(stderr, "preenc synced : %d\n", sts);
            }

            if (MFX_ERR_NONE < sts && !m_SyncPoint) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                {
                    WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
                }
            }
            else if (MFX_ERR_NONE < sts && m_SyncPoint)
            {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else
            {
                break;
            }
        }
        MSDK_CHECK_STATUS(sts, "FEI PreENC failed to process frame");

        // Store PreEnc output
        eTask->preenc_output.push_back(PreEncOutput(eTask->preenc_bufs, preenc_ref_idx));

        //drop output data to output file
        sts = FlushOutput(eTask);
        MSDK_CHECK_STATUS(sts, "PreENC: FlushOutput failed");

    } // for (mfxU32 l0_idx = 0, l1_idx = 0; l0_idx < total_l0 || l1_idx < total_l1; l0_idx++, l1_idx++)

    return sts;
}

mfxStatus FEI_PreencInterface::GetRefTaskEx(iTask *eTask, mfxU8 l0_idx, mfxU8 l1_idx, mfxU8 refIdx[2][2], mfxU8 ref_fid[2][2], iTask *outRefTask[2][2])
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus stsL0 = MFX_WRN_OUT_OF_RANGE, stsL1 = MFX_WRN_OUT_OF_RANGE;

    for (int i = 0; i < 2; i++){
        MSDK_ZERO_ARRAY(refIdx[i],     2);
        MSDK_ZERO_ARRAY(outRefTask[i], 2);
        MSDK_ZERO_ARRAY(ref_fid[i],    2);
    }

    mfxU32 numOfFields = eTask->m_fieldPicFlag ? 2 : 1;
    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        mfxU8 type = ExtractFrameType(*eTask, fieldId);
        mfxU32 l0_ref_count = eTask->GetNBackward(fieldId),
               l1_ref_count = eTask->GetNForward(fieldId),
               fid = eTask->m_fid[fieldId];

        /* adjustment for case of equal l0 and l1 lists*/
        if (!l0_ref_count && eTask->m_list0[fid].Size() && !(eTask->m_type[fid] & MFX_FRAMETYPE_I))
        {
            l0_ref_count = eTask->m_list0[fid].Size();
        }

        if (l0_idx < l0_ref_count && eTask->m_list0[fid].Size())
        {
            mfxU8 const * l0_instance = eTask->m_list0[fid].Begin() + l0_idx;
            iTask *L0_ref_task = m_inputTasks->GetTaskByFrameOrder(eTask->m_dpb[fid][*l0_instance & 127].m_frameOrder);
            MSDK_CHECK_POINTER(L0_ref_task, MFX_ERR_NULL_PTR);

            refIdx[fieldId][0]     = l0_idx;
            outRefTask[fieldId][0] = L0_ref_task;
            ref_fid[fieldId][0]    = (*l0_instance > 127) ? 1 : 0; // 1 - bottom field, 0 - top field

            stsL0 = MFX_ERR_NONE;
        }
        else {
            if (eTask->m_list0[fid].Size())
                stsL0 = MFX_WRN_OUT_OF_RANGE;
        }

        if (l1_idx < l1_ref_count && eTask->m_list1[fid].Size())
        {
            if (type & MFX_FRAMETYPE_IP)
            {
                //refIdx[fieldId][1] = 0;        // already zero
                //outRefTask[fieldId][1] = NULL; // No forward ref for P
            }
            else
            {
                mfxU8 const *l1_instance = eTask->m_list1[fid].Begin() + l1_idx;
                iTask *L1_ref_task = m_inputTasks->GetTaskByFrameOrder(eTask->m_dpb[fid][*l1_instance & 127].m_frameOrder);
                MSDK_CHECK_POINTER(L1_ref_task, MFX_ERR_NULL_PTR);

                refIdx[fieldId][1]     = l1_idx;
                outRefTask[fieldId][1] = L1_ref_task;
                ref_fid[fieldId][1]    = (*l1_instance > 127) ? 1 : 0; // for bff ? 0 : 1; (but now preenc supports only tff)

                stsL1 = MFX_ERR_NONE;
            }
        }
        else {
            if (eTask->m_list1[fid].Size())
                stsL1 = MFX_WRN_OUT_OF_RANGE;
        }
    }

    return stsL1 != stsL0 ? MFX_ERR_NONE : stsL0;
}

mfxStatus FEI_PreencInterface::RepackPredictors(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxExtFeiEncMVPredictors* mvp = NULL;
    bufSet*                   set = m_pEncExtBuffers->GetFreeSet();
    MSDK_CHECK_POINTER(set, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    BestMVset BestSet(eTask);

    mfxU32 i, j, numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        MSDK_BREAK_ON_ERROR(sts);

        //adjust n of passed pred
        mfxU32 nPred_actual[2] = { (std::min)(GetNumL0MVPs(*eTask, fieldId), MaxFeiEncMVPNum),
                                   (std::min)(GetNumL1MVPs(*eTask, fieldId), MaxFeiEncMVPNum) };
        if (nPred_actual[0] + nPred_actual[1] == 0 || (ExtractFrameType(*eTask, fieldId) & MFX_FRAMETYPE_I))
            continue; // I-field

        // MVP is predictors Ext Buffer
        mvp = reinterpret_cast<mfxExtFeiEncMVPredictors*>(set->PB_bufs.in.getBufById(MFX_EXTBUFF_FEI_ENC_MV_PRED, fieldId));
        if (mvp == NULL){
            sts = MFX_ERR_NULL_PTR;
            break;
        }

        /* not necessary for pipelines PreENC + FEI_ENCODE / (ENCPAK), if number of predictors for next interface properly set */
        MSDK_ZERO_ARRAY(mvp->MB, mvp->NumMBAlloc);

        for (i = 0; i < m_pAppConfig->PipelineCfg.numMB_preenc_refPic; ++i)
        {
            /* get best L0/L1 PreENC predictors for current MB in terms of distortions */
            sts = GetBestSetsByDistortion(eTask->preenc_output, BestSet, nPred_actual, fieldId, i);
            MSDK_BREAK_ON_ERROR(sts);

            if (!m_pAppConfig->preencDSstrength)
            {
                /* in case of PreENC on original surfaces just get median of MVs for each predictor */
                for (j = 0; j < BestSet.bestL0.size(); ++j)
                {
                    mvp->MB[i].RefIdx[j].RefL0 = BestSet.bestL0[j].refIdx;
                    mvp->MB[i].MV[j][0].x = get16Median(BestSet.bestL0[j].preenc_MVMB, &m_tmpForMedian[0], 0, 0);
                    mvp->MB[i].MV[j][0].y = get16Median(BestSet.bestL0[j].preenc_MVMB, &m_tmpForMedian[0], 1, 0);
                }
                for (j = 0; j < BestSet.bestL1.size(); ++j)
                {
                    mvp->MB[i].RefIdx[j].RefL1 = BestSet.bestL1[j].refIdx;
                    mvp->MB[i].MV[j][1].x = get16Median(BestSet.bestL1[j].preenc_MVMB, &m_tmpForMedian[0], 0, 1);
                    mvp->MB[i].MV[j][1].y = get16Median(BestSet.bestL1[j].preenc_MVMB, &m_tmpForMedian[0], 1, 1);
                }
           }
           else
           {
               /* in case of PreENC on downsampled surfaces we need to calculate umpsampled pridictors */
               for (j = 0; j < BestSet.bestL0.size(); ++j)
               {
                   UpsampleMVP(BestSet.bestL0[j].preenc_MVMB, i, mvp, j, BestSet.bestL0[j].refIdx, 0);
               }
               for (j = 0; j < BestSet.bestL1.size(); ++j)
               {
                   UpsampleMVP(BestSet.bestL1[j].preenc_MVMB, i, mvp, j, BestSet.bestL1[j].refIdx, 1);
               }
           }
        } // for (i = 0; i < numMBpreenc; ++i)

    } // for (mfxU32 fieldId = 0; fieldId < m_numOfFields; fieldId++)

    SAFE_RELEASE_EXT_BUFSET(set);

    eTask->ReleasePreEncOutput();

    return sts;
}

/*  In case of PreENC with downsampling output motion vectors should be upscaled to correspond full-resolution frame

    preenc_MVMB - current MB on downsampled frame
    MBindex_DS  - index of current MB
    mvp         - motion vectors predictors array for full-resolution frame
    predIdx     - index of current predictor [0-3] (up to 4 is supported)
    refIdx      - index of current reference (for which predictor is applicable) in reference list
    L0L1        - indicates list of references being processed [0-1] (0 - L0 list, 1- L1 list)
*/
void FEI_PreencInterface::UpsampleMVP(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB * preenc_MVMB, mfxU32 MBindex_DS, mfxExtFeiEncMVPredictors* mvp, mfxU32 predIdx, mfxU8 refIdx, mfxU32 L0L1)
{
    static mfxI16 MVZigzagOrder[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };

    static mfxU16 widthMB_ds = (m_DSParams.vpp.Out.Width + 15) >> 4, widthMB_full = (m_DSParams.vpp.In.Width + 15) >> 4;

    mfxU16 mv_idx;
    mfxU32 encMBidx;
    mfxI16Pair* mvp_mb;

    for (mfxU16 k = 0; k < m_pAppConfig->preencDSstrength*m_pAppConfig->preencDSstrength; ++k)
    {
        encMBidx = (MBindex_DS % widthMB_ds + MBindex_DS / widthMB_ds *  widthMB_full) * m_pAppConfig->preencDSstrength + k % m_pAppConfig->preencDSstrength + widthMB_full*(k / m_pAppConfig->preencDSstrength);

        if (encMBidx >= m_pAppConfig->PipelineCfg.numMB_refPic)
            continue;

        mvp_mb = &(mvp->MB[encMBidx].MV[predIdx][L0L1]);

        if (!L0L1)
            mvp->MB[encMBidx].RefIdx[predIdx].RefL0 = refIdx;
        else
            mvp->MB[encMBidx].RefIdx[predIdx].RefL1 = refIdx;

        switch (m_pAppConfig->preencDSstrength)
        {
        case 2:
            mvp_mb->x = get4Median(preenc_MVMB, &m_tmpForMedian[0], 0, L0L1, k);
            mvp_mb->y = get4Median(preenc_MVMB, &m_tmpForMedian[0], 1, L0L1, k);
            break;
        case 4:
            mvp_mb->x = preenc_MVMB->MV[MVZigzagOrder[k]][L0L1].x;
            mvp_mb->y = preenc_MVMB->MV[MVZigzagOrder[k]][L0L1].y;
            break;
        case 8:
            mv_idx = MVZigzagOrder[k % 16 % 8 / 2 + k / 16 * 4];
            mvp_mb->x = preenc_MVMB->MV[mv_idx][L0L1].x;
            mvp_mb->y = preenc_MVMB->MV[mv_idx][L0L1].y;
            break;
        default:
            break;
        }

        mvp_mb->x *= m_pAppConfig->preencDSstrength;
        mvp_mb->y *= m_pAppConfig->preencDSstrength;

    } // for (mfxU16 k = 0; k < m_pAppConfig->preencDSstrength*m_pAppConfig->preencDSstrength; ++k)
}

/* Simplified conversion - no sorting by distortion, no median on MV */
mfxStatus FEI_PreencInterface::RepackPredictorsPerf(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtFeiEncMVPredictors* mvp = NULL;
    std::vector<mfxExtFeiPreEncMV*> mvs_v;
    std::vector<mfxU8*>             refIdx_v;
    bufSet* set = m_pEncExtBuffers->GetFreeSet();
    MSDK_CHECK_POINTER(set, MFX_ERR_NULL_PTR);

    mvs_v.reserve(MaxFeiEncMVPNum);
    refIdx_v.reserve(MaxFeiEncMVPNum);

    mfxU32 i, j, preencMBidx, MVrow, MVcolumn, preencMBMVidx;

    mfxU32 nOfPredPairs = 0, numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        nOfPredPairs = (std::min)(MaxFeiEncMVPNum, (std::max)(GetNumL0MVPs(*eTask, fieldId), GetNumL1MVPs(*eTask, fieldId)));

        if (nOfPredPairs == 0 || (ExtractFrameType(*eTask, fieldId) & MFX_FRAMETYPE_I))
            continue; // I-field

        mvs_v.clear();
        refIdx_v.clear();

        mvp = reinterpret_cast<mfxExtFeiEncMVPredictors*>(set->PB_bufs.in.getBufById(MFX_EXTBUFF_FEI_ENC_MV_PRED, fieldId));
        if (mvp == NULL){
            sts = MFX_ERR_NULL_PTR;
            break;
        }

        j = 0;
        for (std::list<PreEncOutput>::iterator it = eTask->preenc_output.begin(); j < nOfPredPairs; ++it, ++j)
        {
            mvs_v.push_back(reinterpret_cast<mfxExtFeiPreEncMV*>((*it).output_bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PREENC_MV, fieldId)));
            refIdx_v.push_back((*it).refIdx[fieldId]);
        }

        for (i = 0; i < m_pAppConfig->PipelineCfg.numMB_refPic; ++i) // get nPred_actual L0/L1 predictors for each MB
        {
            for (j = 0; j < nOfPredPairs; ++j)
            {
                mvp->MB[i].RefIdx[j].RefL0 = refIdx_v[j][0];
                mvp->MB[i].RefIdx[j].RefL1 = refIdx_v[j][1];

                if (!m_pAppConfig->preencDSstrength)
                {
                    MSDK_MEMCPY_VAR(*mvp->MB[i].MV[j], mvs_v[j]->MB[i].MV[0], 2 * sizeof(mfxI16Pair));
                }
                else
                {
                    static mfxI16 MVZigzagOrder[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };

                    static mfxU16 widthMB_ds = ((m_pmfxDS? m_DSParams.vpp.Out.Width : m_videoParams.mfx.FrameInfo.Width) + 15) >> 4,
                                widthMB_full = ((m_pmfxDS? m_DSParams.vpp.In.Width  : m_videoParams.mfx.FrameInfo.Width) + 15) >> 4;

                    preencMBidx = i / widthMB_full / m_pAppConfig->preencDSstrength * widthMB_ds + i % widthMB_full / m_pAppConfig->preencDSstrength;

                    if (preencMBidx >= m_pAppConfig->PipelineCfg.numMB_preenc_refPic)
                        continue;

                    MVrow    = i / widthMB_full % m_pAppConfig->preencDSstrength;
                    MVcolumn = i % widthMB_full % m_pAppConfig->preencDSstrength;

                    switch (m_pAppConfig->preencDSstrength)
                    {
                    case 2:
                        preencMBMVidx = MVrow * 8 + MVcolumn * 2;
                        break;
                    case 4:
                        preencMBMVidx = MVrow * 4 + MVcolumn;
                        break;
                    case 8:
                        preencMBMVidx = MVrow / 2 * 4 + MVcolumn / 2;
                        break;
                    default:
                        preencMBMVidx = 0;
                        break;
                    }

                    MSDK_MEMCPY_VAR(*mvp->MB[i].MV[j], mvs_v[j]->MB[preencMBidx].MV[MVZigzagOrder[preencMBMVidx]], 2 * sizeof(mfxI16Pair));

                    mvp->MB[i].MV[j][0].x *= m_pAppConfig->preencDSstrength;
                    mvp->MB[i].MV[j][0].y *= m_pAppConfig->preencDSstrength;
                    mvp->MB[i].MV[j][1].x *= m_pAppConfig->preencDSstrength;
                    mvp->MB[i].MV[j][1].y *= m_pAppConfig->preencDSstrength;
                }
            }
        }

    } // for (mfxU32 fieldId = 0; fieldId < m_numOfFields; fieldId++)

    SAFE_RELEASE_EXT_BUFSET(set);

    eTask->ReleasePreEncOutput();

    return sts;
}

mfxStatus FEI_PreencInterface::FlushOutput(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    int mvsId = 0;
    for (int i = 0; i < eTask->PREENC_out.NumExtParam; i++)
    {
        switch (eTask->PREENC_out.ExtParam[i]->BufferId){
        case MFX_EXTBUFF_FEI_PREENC_MB:
            if (m_pMBstat_out)
            {
                mfxExtFeiPreEncMBStat* mbdata = reinterpret_cast<mfxExtFeiPreEncMBStat*>(eTask->PREENC_out.ExtParam[i]);
                SAFE_FWRITE(mbdata->MB, sizeof(mbdata->MB[0]) * mbdata->NumMBAlloc, 1, m_pMBstat_out, MFX_ERR_MORE_DATA);
            }
            break;

        case MFX_EXTBUFF_FEI_PREENC_MV:
            if (m_pMV_out)
            {
                mfxExtFeiPreEncMV* mvs = reinterpret_cast<mfxExtFeiPreEncMV*>(eTask->PREENC_out.ExtParam[i]);

                if (ExtractFrameType(*eTask, mvsId) & MFX_FRAMETYPE_I)
                {                                                   // IP pair
                    for (mfxU32 k = 0; k < mvs->NumMBAlloc; k++){   // in progressive case Ext buffer for I frame is detached
                        SAFE_FWRITE(&m_tmpMVMB, sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB), 1, m_pMV_out, MFX_ERR_MORE_DATA);
                    }
                }
                else
                {
                    SAFE_FWRITE(mvs->MB, sizeof(mvs->MB[0]) * mvs->NumMBAlloc, 1, m_pMV_out, MFX_ERR_MORE_DATA);
                }
            }
            mvsId++;
            break;
        } // switch (eTask->PREENC_out.ExtParam[i]->BufferId)
    } //for (int i = 0; i < eTask->PREENC_out.NumExtParam; i++)

    if (!eTask->m_fieldPicFlag && m_pMV_out && (ExtractFrameType(*eTask) & MFX_FRAMETYPE_I)){ // drop 0x8000 for progressive I-frames
        for (mfxU32 k = 0; k < m_pAppConfig->PipelineCfg.numMB_preenc_refPic; k++){
            SAFE_FWRITE(&m_tmpMVMB, sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB), 1, m_pMV_out, MFX_ERR_MORE_DATA);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus FEI_PreencInterface::ResetState()
{
    mfxStatus sts = MFX_ERR_NONE;

    // mark sync point as free
    m_SyncPoint = NULL;

    SAFE_FSEEK(m_pMvPred_in,  0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMbQP_in,    0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMBstat_out, 0, SEEK_SET, MFX_ERR_MORE_DATA);
    SAFE_FSEEK(m_pMV_out,     0, SEEK_SET, MFX_ERR_MORE_DATA);

    return sts;
}
