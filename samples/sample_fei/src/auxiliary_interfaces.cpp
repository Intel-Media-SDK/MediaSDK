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

#include "auxiliary_interfaces.h"

MFX_VppInterface::MFX_VppInterface(MFXVideoSession* session, mfxU32 allocId, AppConfig* config)
    : m_pmfxSession(session)
    , m_pmfxVPP(new MFXVideoVPP(*session))
    , m_allocId(allocId)
    , m_pAppConfig(config)
    , m_SyncPoint(0)
{
    MSDK_ZERO_MEMORY(m_videoParams);

    m_InitExtParams.reserve(1);
}

MFX_VppInterface::~MFX_VppInterface()
{
    MSDK_SAFE_DELETE(m_pmfxVPP);

    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_VPP_DONOTUSE:
            mfxExtVPPDoNotUse* ptr = reinterpret_cast<mfxExtVPPDoNotUse*>(m_InitExtParams[i]);
            MSDK_SAFE_DELETE_ARRAY(ptr->AlgList);
            MSDK_SAFE_DELETE(ptr);
            break;
        }
    }
    m_InitExtParams.clear();

    m_pAppConfig->PipelineCfg.pVppVideoParam = NULL;
}

mfxStatus MFX_VppInterface::Init()
{
    return m_pmfxVPP->Init(&m_videoParams);
}

mfxStatus MFX_VppInterface::Close()
{
    return m_pmfxVPP->Close();
}

mfxVideoParam* MFX_VppInterface::GetCommonVideoParams()
{
    return &m_videoParams;
}

mfxStatus MFX_VppInterface::Reset(mfxU16 width, mfxU16 height, mfxU16 crop_w, mfxU16 crop_h)
{
    if (width && height && crop_w && crop_h)
    {
        m_videoParams.vpp.Out.Width  = width;
        m_videoParams.vpp.Out.Height = height;
        m_videoParams.vpp.Out.CropW  = crop_w;
        m_videoParams.vpp.Out.CropH  = crop_h;
    }
    return m_pmfxVPP->Reset(&m_videoParams);
}

mfxStatus MFX_VppInterface::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_pmfxVPP->QueryIOSurf(&m_videoParams, request);
}

mfxStatus MFX_VppInterface::FillParameters()
{
    mfxStatus sts = MFX_ERR_NONE;

    /* Share VPP video parameters with other interfaces */
    m_pAppConfig->PipelineCfg.pVppVideoParam = &m_videoParams;

    m_videoParams.AllocId   = m_allocId;
    // specify memory type
    m_videoParams.IOPattern = mfxU16(m_pAppConfig->bUseHWmemory ? (MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        : (MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY));
    m_videoParams.AsyncDepth = 1;

    if (m_pAppConfig->bDECODE)
    {
        /* Decoder present in pipeline */
        MSDK_MEMCPY_VAR(m_videoParams.vpp.In, &m_pAppConfig->PipelineCfg.pDecodeVideoParam->mfx.FrameInfo, sizeof(mfxFrameInfo));
        m_videoParams.vpp.In.PicStruct = m_pAppConfig->nPicStruct;
    }
    else
    {
        /* No other instances before PreENC DownSampling. Fill VideoParams */

        // input frame info
        m_videoParams.vpp.In.FourCC       = MFX_FOURCC_NV12;
        m_videoParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_videoParams.vpp.In.PicStruct    = m_pAppConfig->nPicStruct;

        sts = ConvertFrameRate(m_pAppConfig->dFrameRate, &m_videoParams.vpp.In.FrameRateExtN, &m_videoParams.vpp.In.FrameRateExtD);
        MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");

        // width must be a multiple of 16
        // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
        m_videoParams.vpp.In.Width = MSDK_ALIGN16(m_pAppConfig->nWidth);
        m_videoParams.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_videoParams.vpp.In.PicStruct) ?
            MSDK_ALIGN16(m_pAppConfig->nHeight) : MSDK_ALIGN32(m_pAppConfig->nHeight);

        // set crops in input mfxFrameInfo for correct work of file reader
        // VPP itself ignores crops at initialization
        m_videoParams.vpp.In.CropX = m_videoParams.vpp.In.CropY = 0;
        m_videoParams.vpp.In.CropW = m_pAppConfig->nWidth;
        m_videoParams.vpp.In.CropH = m_pAppConfig->nHeight;
    }

    // fill output frame info
    MSDK_MEMCPY_VAR(m_videoParams.vpp.Out, &m_videoParams.vpp.In, sizeof(mfxFrameInfo));

    // only resizing is supported
    m_videoParams.vpp.Out.CropX = m_videoParams.vpp.Out.CropY = 0;
    m_videoParams.vpp.Out.Width = MSDK_ALIGN16(m_pAppConfig->nDstWidth);
    m_videoParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_videoParams.vpp.Out.PicStruct) ?
        MSDK_ALIGN16(m_pAppConfig->nDstHeight) : MSDK_ALIGN32(m_pAppConfig->nDstHeight);
    m_videoParams.vpp.Out.CropW = m_pAppConfig->nDstWidth;
    m_videoParams.vpp.Out.CropH = m_pAppConfig->nDstHeight;



    // configure and attach external parameters
    mfxExtVPPDoNotUse* m_VppDoNotUse = new mfxExtVPPDoNotUse;
    MSDK_ZERO_MEMORY(*m_VppDoNotUse);
    m_VppDoNotUse->Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    m_VppDoNotUse->Header.BufferSz = sizeof(mfxExtVPPDoNotUse);
    m_VppDoNotUse->NumAlg = 4;

    m_VppDoNotUse->AlgList = new mfxU32[m_VppDoNotUse->NumAlg];
    m_VppDoNotUse->AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;        // turn off denoising (on by default)
    m_VppDoNotUse->AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS; // turn off scene analysis (on by default)
    m_VppDoNotUse->AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;         // turn off detail enhancement (on by default)
    m_VppDoNotUse->AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;        // turn off processing amplified (on by default)
    m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_VppDoNotUse));

    m_videoParams.ExtParam    = &m_InitExtParams[0]; // vector is stored linearly in memory
    m_videoParams.NumExtParam = (mfxU16)m_InitExtParams.size();

    return sts;
}

mfxStatus MFX_VppInterface::VPPoneFrame(mfxFrameSurface1* pSurf_in, mfxFrameSurface1* pSurf_out)
{
    mfxStatus sts = MFX_ERR_NONE;

    for (;;)
    {
        sts = m_pmfxVPP->RunFrameVPPAsync(pSurf_in, pSurf_out, NULL, &m_SyncPoint);
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

    return sts;
}


MFX_DecodeInterface::MFX_DecodeInterface(MFXVideoSession* session, mfxU32 allocId, AppConfig* config, ExtSurfPool* surf_pool)
    : m_pmfxSession(session)
    , m_pmfxDECODE(new MFXVideoDECODE(*session))
    , m_allocId(allocId)
    , m_pAppConfig(config)
    , m_SyncPoint(0)
    , m_pSurfPool(surf_pool)
    , m_bEndOfFile(false)
    , m_DecStremout_out(NULL)
{
    MSDK_ZERO_MEMORY(m_mfxBS);
    MSDK_ZERO_MEMORY(m_videoParams);

    m_InitExtParams.reserve(1);
}

MFX_DecodeInterface::~MFX_DecodeInterface()
{
    MSDK_SAFE_DELETE(m_pmfxDECODE);

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

    WipeMfxBitstream(&m_mfxBS);

    SAFE_FCLOSE(m_DecStremout_out);

    m_pAppConfig->PipelineCfg.pDecodeVideoParam = NULL;
}

mfxStatus MFX_DecodeInterface::Init()
{
    return m_pmfxDECODE->Init(&m_videoParams);
}

mfxStatus MFX_DecodeInterface::Close()
{
    return m_pmfxDECODE->Close();
}

mfxStatus MFX_DecodeInterface::Reset()
{
    return m_pmfxDECODE->Reset(&m_videoParams);
}

mfxStatus MFX_DecodeInterface::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_pmfxDECODE->QueryIOSurf(&m_videoParams, request);
}

mfxVideoParam* MFX_DecodeInterface::GetCommonVideoParams()
{
    return &m_videoParams;
}

mfxStatus MFX_DecodeInterface::UpdateVideoParam()
{
    return m_pmfxDECODE->GetVideoParam(&m_videoParams);
}

mfxStatus MFX_DecodeInterface::FillParameters()
{
    mfxStatus sts = MFX_ERR_NONE;

    /* Share DECODE video parameters with other interfaces */
    m_pAppConfig->PipelineCfg.pDecodeVideoParam = &m_videoParams;

    m_videoParams.AsyncDepth = 1;
    m_videoParams.AllocId = m_allocId;

    // set video type in parameters
    m_videoParams.mfx.CodecId = m_pAppConfig->DecodeId;
    m_videoParams.mfx.DecodedOrder = mfxU16(m_pAppConfig->DecodedOrder);

    sts = m_BSReader.Init(m_pAppConfig->strSrcFile);
    MSDK_CHECK_STATUS(sts, "m_BSReader.Init failed");

    sts = InitMfxBitstream(&m_mfxBS, 1024 * 1024);
    MSDK_CHECK_STATUS(sts, "InitMfxBitstream failed");

    // read a portion of data for DecodeHeader function
    sts = m_BSReader.ReadNextFrame(&m_mfxBS);
    if (MFX_ERR_MORE_DATA == sts)
        return sts;
    MSDK_CHECK_STATUS(sts, "m_BSReader.ReadNextFrame failed");

    // try to find a sequence header in the stream
    // if header is not found this function exits with error (e.g. if device was lost and there's no header in the remaining stream)
    for (;;)
    {
        // parse bit stream and fill mfx params
        sts = m_pmfxDECODE->DecodeHeader(&m_mfxBS, &m_videoParams);

        if (MFX_ERR_MORE_DATA == sts)
        {
            if (m_mfxBS.MaxLength == m_mfxBS.DataLength)
            {
                sts = ExtendMfxBitstream(&m_mfxBS, m_mfxBS.MaxLength * 2);
                MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
            }

            // read a portion of data for DecodeHeader function
            sts = m_BSReader.ReadNextFrame(&m_mfxBS);
            if (MFX_ERR_MORE_DATA == sts)
                return sts;
            MSDK_CHECK_STATUS(sts, "m_BSReader.ReadNextFrame failed");

            continue;
        }
        else
            break;
    }

    // if input stream header doesn't contain valid values use default (30.0)
    if (!(m_videoParams.mfx.FrameInfo.FrameRateExtN * m_videoParams.mfx.FrameInfo.FrameRateExtD))
    {
        m_videoParams.mfx.FrameInfo.FrameRateExtN = 30;
        m_videoParams.mfx.FrameInfo.FrameRateExtD = 1;
    }

    if (m_pAppConfig->nPicStruct == MFX_PICSTRUCT_UNKNOWN)
    {
        // In this case we acquire picture structure from the input stream
        m_videoParams.mfx.ExtendedPicStruct = 1;
    }

    // specify memory type
    m_videoParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    if (m_pAppConfig->bDECODESTREAMOUT)
    {
        mfxExtFeiParam* decStreamoutParams = new mfxExtFeiParam;
        MSDK_ZERO_MEMORY(*decStreamoutParams);
        decStreamoutParams->Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
        decStreamoutParams->Header.BufferSz = sizeof(mfxExtFeiParam);
        decStreamoutParams->Func = MFX_FEI_FUNCTION_DEC;
        decStreamoutParams->SingleFieldProcessing = MFX_CODINGOPTION_OFF;

        m_InitExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(decStreamoutParams));

        m_videoParams.NumExtParam = (mfxU16)m_InitExtParams.size();
        m_videoParams.ExtParam = &m_InitExtParams[0];
    }

    if (m_pAppConfig->decodestreamoutFile)
    {
        MSDK_FOPEN(m_DecStremout_out, m_pAppConfig->decodestreamoutFile, MSDK_CHAR("wb"));
        if (m_DecStremout_out == NULL) {
            mdprintf(stderr, "Can't open file %s\n", m_pAppConfig->decodestreamoutFile);
            exit(-1);
        }
    }

    return sts;
}

mfxStatus MFX_DecodeInterface::GetOneFrame(mfxFrameSurface1* & pSurf)
{
    MFX_ITT_TASK("GetOneFrame");

    mfxStatus sts = MFX_ERR_NONE;

    if (!m_bEndOfFile)
    {
        sts = DecodeOneFrame(pSurf);
        if (MFX_ERR_MORE_DATA == sts)
        {
            sts = DecodeLastFrame(pSurf);
            m_bEndOfFile = true;
        }
    }
    else
    {
        sts = DecodeLastFrame(pSurf);
    }

    if (sts == MFX_ERR_GPU_HANG)
    {
        return sts;
    }

    if (sts == MFX_ERR_MORE_DATA)
    {
        if (m_pAppConfig->nTimeout)
        {
            // infinite loop mode, need to proceed from the beginning
            return MFX_ERR_MORE_DATA;
        }
        pSurf = NULL;
        return sts;
    }
    MSDK_CHECK_STATUS(sts, "Decode<One|Last>Frame failed");

    if (m_pAppConfig->bDECODESTREAMOUT || m_pAppConfig->PipelineCfg.mixedPicstructs || !m_pAppConfig->bVPP)
    {
        for (;;)
        {
            sts = m_pmfxSession->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);

            if (sts == MFX_ERR_GPU_HANG)
            {
                return MFX_ERR_GPU_HANG;
            }

            if (MFX_ERR_NONE < sts && !m_SyncPoint) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts){
                    WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts); // wait if device is busy
                }
            }
            else if (MFX_ERR_NONE <= sts && m_SyncPoint) {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }
        }
        MSDK_CHECK_STATUS(sts, "DECODE: SyncOperation failed");

        if (m_pAppConfig->bDECODESTREAMOUT)
        {
            sts = FlushOutput(pSurf);
            MSDK_CHECK_STATUS(sts, "DECODE: FlushOutput failed");
        }
    }

    return sts;
}

mfxStatus MFX_DecodeInterface::DecodeOneFrame(mfxFrameSurface1 * & pSurf_out)
{
    MFX_ITT_TASK("DecodeOneFrame");

    mfxStatus sts = MFX_ERR_MORE_SURFACE;
    mfxFrameSurface1 *pDecSurf = NULL;

    while (MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE < sts)
    {
        switch (sts)
        {
        case MFX_WRN_DEVICE_BUSY:
            WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
            break;

        case MFX_ERR_MORE_DATA:
            sts = m_BSReader.ReadNextFrame(&m_mfxBS); // read more data to input bit stream
            if (sts == MFX_ERR_MORE_DATA) { return sts; } // Reached end of bitstream
            break;

        case MFX_ERR_MORE_SURFACE:
            // find free surface for decoder input
            pDecSurf = m_pSurfPool->GetFreeSurface_FEI();
            MSDK_CHECK_POINTER(pDecSurf, MFX_ERR_MEMORY_ALLOC); // return an error if a free surface wasn't found
            break;

        default:
            break;
        }

        sts = m_pmfxDECODE->DecodeFrameAsync(&m_mfxBS, pDecSurf, &pSurf_out, &m_SyncPoint);
        if (sts == MFX_ERR_GPU_HANG)
        {
            return MFX_ERR_GPU_HANG;
        }

        // ignore warnings if output is available,
        if (MFX_ERR_NONE < sts && m_SyncPoint)
        {
            sts = MFX_ERR_NONE;
        }

    } // while (MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE < sts)

    return sts;
}

mfxStatus MFX_DecodeInterface::DecodeLastFrame(mfxFrameSurface1 * & pSurf_out)
{
    MFX_ITT_TASK("DecodeLastFrame");

    mfxFrameSurface1 *pDecSurf = NULL;
    mfxStatus sts = MFX_ERR_MORE_SURFACE;

    // retrieve the buffered decoded frames
    while (MFX_ERR_MORE_SURFACE == sts || MFX_WRN_DEVICE_BUSY == sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(*m_pmfxSession, m_SyncPoint, sts);
        }
        // find free surface for decoder input
        pDecSurf = m_pSurfPool->GetFreeSurface_FEI();
        MSDK_CHECK_POINTER(pDecSurf, MFX_ERR_MEMORY_ALLOC); // return an error if a free surface wasn't found

        sts = m_pmfxDECODE->DecodeFrameAsync(NULL, pDecSurf, &pSurf_out, &m_SyncPoint);
        if (sts == MFX_ERR_GPU_HANG)
        {
            return MFX_ERR_GPU_HANG;
        }
    }
    return sts;
}

mfxStatus MFX_DecodeInterface::FlushOutput(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NONE;

    for (int i = 0; i < pSurf->Data.NumExtParam; ++i)
        if (pSurf->Data.ExtParam[i]->BufferId == MFX_EXTBUFF_FEI_DEC_STREAM_OUT)
        {
            mfxExtFeiDecStreamOut* decodeStreamout = reinterpret_cast<mfxExtFeiDecStreamOut*>(pSurf->Data.ExtParam[i]);
            MSDK_CHECK_POINTER(decodeStreamout, MFX_ERR_NULL_PTR);

            if (m_DecStremout_out)
            {
                /* NOTE: streamout holds data for both fields in MB array (first NumMBAlloc for first field data, second NumMBAlloc for second field) */
                SAFE_FWRITE(decodeStreamout->MB, sizeof(mfxFeiDecStreamOutMBCtrl)*decodeStreamout->NumMBAlloc, 1, m_DecStremout_out, MFX_ERR_MORE_DATA);
            }
            break;
        }

    return sts;
}

mfxStatus MFX_DecodeInterface::ResetState()
{
    mfxStatus sts = MFX_ERR_NONE;

    // mark sync point as free
    m_SyncPoint = NULL;

    // prepare bit stream
    m_mfxBS.DataOffset = 0;
    m_mfxBS.DataLength = 0;

    sts = m_BSReader.Init(m_pAppConfig->strSrcFile);
    MSDK_CHECK_STATUS(sts, "m_BSReader.Init failed");

    m_bEndOfFile = false;

    SAFE_FSEEK(m_DecStremout_out, 0, SEEK_SET, MFX_ERR_MORE_DATA);

    return sts;
}


mfxStatus YUVreader::GetOneFrame(mfxFrameSurface1* & pSurf)
{
    MFX_ITT_TASK("GetOneFrame");
    mfxStatus sts = MFX_ERR_NONE;

    // point pSurf to encoder surface
    pSurf = m_pSurfPool->GetFreeSurface_FEI();
    MSDK_CHECK_POINTER(pSurf, MFX_ERR_MEMORY_ALLOC);

    // load frame from file to surface data
    // if we share allocator with Media SDK we need to call Lock to access surface data and...
    if (m_bExternalAlloc)
    {
        // get YUV pointers
        sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
    }

    sts = m_FileReader.LoadNextFrame(pSurf);
    if (sts == MFX_ERR_MORE_DATA && m_pAppConfig->nTimeout)
    {
        // infinite loop mode, need to proceed from the beginning
        return MFX_ERR_MORE_DATA;
    }
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, sts);

    // ... after we're done call Unlock
    if (m_bExternalAlloc)
    {
        sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Unlock failed");
    }

    return sts;
}
