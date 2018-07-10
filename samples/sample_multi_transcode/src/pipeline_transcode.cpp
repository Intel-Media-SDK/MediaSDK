/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
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

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include "pipeline_transcode.h"
#include "transcode_utils.h"
#include "sample_utils.h"
#include "mfx_vpp_plugin.h"
#include "mfx_itt_trace.h"
#include <algorithm>
#include <cstring>

#include "plugin_loader.h"
#include "parameters_dumper.h"

#include "sample_utils.h"

// let's use std::max and std::min instead
#undef max
#undef min

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

using namespace TranscodingSample;

mfxU32 MFX_STDCALL TranscodingSample::TranscodeRoutine(void   *pObj)
{
    mfxU64 start = TranscodingSample::GetTick();
    ThreadTranscodeContext *pContext = (ThreadTranscodeContext*)pObj;
    pContext->transcodingSts = MFX_ERR_NONE;
    for(;;)
    {
        while (MFX_ERR_NONE == pContext->transcodingSts)
        {
            pContext->transcodingSts = pContext->pPipeline->Run();
        }
        break; // exit loop
    }

    MSDK_IGNORE_MFX_STS(pContext->transcodingSts, MFX_WRN_VALUE_NOT_CHANGED);

    pContext->working_time = TranscodingSample::GetTime(start);
    pContext->numTransFrames = pContext->pPipeline->GetProcessFrames();

    return 0;
} // mfxU32 __stdcall TranscodeRoutine(void   *pObj)

#ifdef ENABLE_MCTF
namespace TranscodingSample
{
    const sMctfRunTimeParam* sMctfRunTimeParams::GetCurParam()
    {
        if (CurIdx >= RunTimeParams.size())
            return NULL;
        else
            return &(RunTimeParams[CurIdx]);
    }

    void sMctfRunTimeParams::MoveForward()
    {
        if(CurIdx < RunTimeParams.size())
        ++CurIdx;
    }

    void sMctfRunTimeParams::Restart()
    {
        CurIdx = 0;
    }

    void sMctfRunTimeParams::Reset()
    {
        CurIdx = 0;
        RunTimeParams.clear();
    }
}
#endif


// set structure to define values
sInputParams::sInputParams() : __sInputParams()
{
#ifdef ENABLE_MCTF
    mctfParam.mode = VPP_FILTER_DISABLED;
    mctfParam.params.FilterStrength = 0;
    mctfParam.rtParams.Reset();
#ifdef ENABLE_MCTF_EXT
    mctfParam.params.BitsPerPixelx100k = 0;
    mctfParam.params.Deblocking = MFX_CODINGOPTION_OFF;
    mctfParam.params.Overlap = MFX_CODINGOPTION_OFF;
    mctfParam.params.TemporalMode = MFX_MCTF_TEMPORAL_MODE_2REF;
    mctfParam.params.MVPrecision = MFX_MVPRECISION_INTEGER;
#endif
#endif
    priority = MFX_PRIORITY_NORMAL;
    libType = MFX_IMPL_SOFTWARE;
    MaxFrameNumber = MFX_INFINITE;
    pVppCompDstRects = NULL;
    m_hwdev = NULL;
    DenoiseLevel=-1;
    DetailLevel=-1;

#if (MFX_VERSION >= 1025)
    MFMode = MFX_MF_DEFAULT;
    numMFEFrames = 0;
    mfeTimeout = 0;
#endif

#if MFX_VERSION >= 1022
    bDecoderPostProcessing = false;
    bROIasQPMAP = false;
#endif //MFX_VERSION >= 1022
}

CTranscodingPipeline::CTranscodingPipeline():
    m_pmfxBS(NULL),
    m_pMFXAllocator(NULL),
    m_hdl(NULL),
    m_encoderFourCC(0),
    m_vppCompDumpRenderMode(0),
    m_hwdev4Rendering(NULL),
    m_EncSurfaceType(0),
    m_DecSurfaceType(0),
    m_bIsVpp(false),
    m_bIsPlugin(false),
    m_nTimeout(0),
    m_bOwnMVCSeqDescMemory(true),
    m_nID(0),
    m_AsyncDepth(0),
    m_nProcessedFramesNum(0),
    m_bIsJoinSession(false),
    m_bDecodeEnable(true),
    m_bEncodeEnable(true),
    m_nVPPCompEnable(0),
    m_bUseOpaqueMemory(false),
    m_LastDecSyncPoint(0),
    m_pBuffer(NULL),
    m_pParentPipeline(NULL),
    m_bIsInit(false),
    m_NumFramesForReset(0),
    isHEVCSW(false),
    m_bInsertIDR(false),
    m_FrameNumberPreference(0xFFFFFFFF),
    m_MaxFramesForTranscode(0xFFFFFFFF),
    m_pBSProcessor(NULL),
    m_nReqFrameTime(0),
    m_nOutputFramesNum(0),
    shouldUseGreedyFormula(false),
    m_nRotationAngle(0)
{
    MSDK_ZERO_MEMORY(m_mfxDecParams);
    MSDK_ZERO_MEMORY(m_mfxVppParams);
    MSDK_ZERO_MEMORY(m_mfxEncParams);
    MSDK_ZERO_MEMORY(m_mfxPluginParams);
    MSDK_ZERO_MEMORY(m_RotateParam);
    MSDK_ZERO_MEMORY(m_mfxPreEncParams);

    MSDK_ZERO_MEMORY(m_threadsPar);
    MSDK_ZERO_MEMORY(m_initPar);

    MSDK_ZERO_MEMORY(m_mfxDecResponse);
    MSDK_ZERO_MEMORY(m_mfxEncResponse);

    MSDK_ZERO_MEMORY(m_Request);

    MSDK_ZERO_MEMORY(m_VppDoNotUse);
    MSDK_ZERO_MEMORY(m_MVCSeqDesc);
    MSDK_ZERO_MEMORY(m_EncOpaqueAlloc);
    MSDK_ZERO_MEMORY(m_VppOpaqueAlloc);
    MSDK_ZERO_MEMORY(m_DecOpaqueAlloc);
    MSDK_ZERO_MEMORY(m_PluginOpaqueAlloc);
    MSDK_ZERO_MEMORY(m_PreEncOpaqueAlloc);
    MSDK_ZERO_MEMORY(m_ExtLAControl);
    MSDK_ZERO_MEMORY(m_CodingOption2);
    MSDK_ZERO_MEMORY(m_CodingOption3);
    MSDK_ZERO_MEMORY(m_ExtHEVCParam);
#if MFX_VERSION >= 1022
    MSDK_ZERO_MEMORY(m_decPostProcessing);
    m_decPostProcessing.Header.BufferId = MFX_EXTBUFF_DEC_VIDEO_PROCESSING;
    m_decPostProcessing.Header.BufferSz = sizeof(mfxExtDecVideoProcessing);
#endif //MFX_VERSION >= 1022

    m_MVCSeqDesc.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
    m_MVCSeqDesc.Header.BufferSz = sizeof(mfxExtMVCSeqDesc);

    m_VppDoNotUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    m_VppDoNotUse.Header.BufferSz = sizeof(mfxExtVPPDoNotUse);

    m_ExtHEVCParam.Header.BufferId = MFX_EXTBUFF_HEVC_PARAM;
    m_ExtHEVCParam.Header.BufferSz = sizeof(mfxExtHEVCParam);

#if (MFX_VERSION >= 1024)
    MSDK_ZERO_MEMORY(m_ExtBRC);
    m_ExtBRC.Header.BufferId = MFX_EXTBUFF_BRC;
    m_ExtBRC.Header.BufferSz = sizeof(m_ExtBRC);
#endif

#if (MFX_VERSION >= 1025)

    MSDK_ZERO_MEMORY(m_ExtMFEParam);
    MSDK_ZERO_MEMORY(m_ExtMFEControl);

    m_ExtMFEControl.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_CONTROL;
    m_ExtMFEControl.Header.BufferSz = sizeof(mfxExtMultiFrameControl);

    m_ExtMFEParam.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_PARAM;
    m_ExtMFEParam.Header.BufferSz = sizeof(mfxExtMultiFrameParam);

#endif

#if MFX_VERSION >= 1022
    m_bUseQPMap = 0;
    m_QPmapWidth = 0;
    m_QPmapHeight = 0;
    m_GOPSize = 0;
    m_QPforI = 0;
    m_QPforP = 0;
    m_nSubmittedFramesNum = 0;
#endif //MFX_VERSION >= 1022

    m_EncOpaqueAlloc.Header.BufferId = m_VppOpaqueAlloc.Header.BufferId =
        m_DecOpaqueAlloc.Header.BufferId = m_PluginOpaqueAlloc.Header.BufferId =
        m_PreEncOpaqueAlloc.Header.BufferId =
        MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
    m_EncOpaqueAlloc.Header.BufferSz = m_VppOpaqueAlloc.Header.BufferSz =
        m_DecOpaqueAlloc.Header.BufferSz = m_PluginOpaqueAlloc.Header.BufferSz =
        m_PreEncOpaqueAlloc.Header.BufferSz =
        sizeof(mfxExtOpaqueSurfaceAlloc);

    m_VppCompParams.InputStream = NULL;
    m_VppCompParams.NumInputStream = 0;
#if MFX_VERSION >= 1023
    m_VppCompParams.NumTiles = 0;
#endif
    m_CodingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    m_CodingOption2.Header.BufferSz = sizeof(m_CodingOption2);

    m_CodingOption3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    m_CodingOption3.Header.BufferSz = sizeof(m_CodingOption3);

    m_VppCompParams.InputStream = NULL;

    inputStatistics.SetDirection(MSDK_STRING("Input"));
    outputStatistics.SetDirection(MSDK_STRING("Output"));

    m_numEncoders = 0;
    m_libvaBackend = 0;
    statisticsWindowSize=0;
    m_bIsFieldWeaving = false;
    m_bIsFieldSplitting = false;
    m_bUseOverlay = false;
    m_bForceStop = false;
    m_bIsInterOrJoined = false;
    m_bRobustFlag = false;
    m_nRotationAngle = 0;
    m_bROIasQPMAP = false;
    m_bExtMBQP = false;
} //CTranscodingPipeline::CTranscodingPipeline()

CTranscodingPipeline::~CTranscodingPipeline()
{
    Close();
} //CTranscodingPipeline::CTranscodingPipeline()

mfxStatus CTranscodingPipeline::CheckRequiredAPIVersion(mfxVersion& version, sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    if (pParams->bIsMVC && !CheckVersion(&version, MSDK_FEATURE_MVC)) {
        msdk_printf(MSDK_STRING("error: MVC is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;

    }
    if ((pParams->DecodeId == MFX_CODEC_JPEG) && !CheckVersion(&version, MSDK_FEATURE_JPEG_DECODE)) {
        msdk_printf(MSDK_STRING("error: Jpeg decoder is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }
    if ((pParams->EncodeId == MFX_CODEC_JPEG) && !CheckVersion(&version, MSDK_FEATURE_JPEG_ENCODE)) {
        msdk_printf(MSDK_STRING("error: Jpeg encoder is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->bLABRC || pParams->nLADepth) && !CheckVersion(&version, MSDK_FEATURE_LOOK_AHEAD)) {
        msdk_printf(MSDK_STRING("error: Look Ahead is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

// initialize decode part
mfxStatus CTranscodingPipeline::DecodePreInit(sInputParams *pParams)
{
    // initialize decode pert
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bDecodeEnable)
    {
        if (CheckVersion(&m_Version, MSDK_FEATURE_PLUGIN_API))
        {
            /* Here we actually define the following codec initialization scheme:
            *  1. If plugin path or guid is specified: we load user-defined plugin (example: VP9 sample decoder plugin)
            *  2. If plugin path not specified:
            *    2.a) we check if codec is distributed as a mediasdk plugin and load it if yes
            *    2.b) if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library
            */
            if (pParams->decoderPluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(pParams->decoderPluginParams.strPluginPath,sizeof(pParams->decoderPluginParams.strPluginPath)))
            {
                m_pUserDecoderModule.reset(new MFXVideoUSER(*m_pmfxSession.get()));
                m_pUserDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, *m_pmfxSession.get(), pParams->decoderPluginParams.pluginGuid, 1, pParams->decoderPluginParams.strPluginPath, (mfxU32)msdk_strnlen(pParams->decoderPluginParams.strPluginPath,sizeof(pParams->decoderPluginParams.strPluginPath))));
                if (m_pUserDecoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
            }
            else
            {
                bool isDefaultPlugin = false;
                if (AreGuidsEqual(pParams->decoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
                {
                    pParams->decoderPluginParams.pluginGuid = msdkGetPluginUID(pParams->libType, MSDK_VDECODE, pParams->DecodeId);
                    isDefaultPlugin=true;
                }
                if (!AreGuidsEqual(pParams->decoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
                {
                    m_pUserDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, *m_pmfxSession.get(), pParams->decoderPluginParams.pluginGuid, 1));
                    if (m_pUserDecoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
                }
                if(sts==MFX_ERR_UNSUPPORTED)
                {
                    msdk_printf(isDefaultPlugin ?
                        MSDK_STRING("Default plugin cannot be loaded (possibly you have to define plugin explicitly)\n")
                        : MSDK_STRING("Explicitly specified plugin cannot be loaded.\n"));
                }
            }
            MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
        }

        // create decoder
        if (!m_bUseOverlay)
        {
            m_pmfxDEC.reset(new MFXVideoDECODE(*m_pmfxSession.get()));
        }

        // set video type in parameters
        m_mfxDecParams.mfx.CodecId = pParams->DecodeId;

        // configure specific decoder parameters
        sts = InitDecMfxParams(pParams);
        if (MFX_ERR_MORE_DATA == sts)
        {
            m_pmfxDEC.reset(NULL);
            return sts;
        }
        else
        {
            MSDK_CHECK_STATUS(sts, "InitDecMfxParams failed");
        }
    }
    else
    {
        m_mfxDecParams = m_pParentPipeline->GetDecodeParam();
        m_MVCSeqDesc = m_pParentPipeline->GetDecMVCSeqDesc();
        m_bOwnMVCSeqDescMemory = false;
    }

    if (pParams->nFPS)
    {
        this->m_nReqFrameTime = 1000000 / pParams->nFPS;
    }

    return sts;

} //mfxStatus CTranscodingPipeline::Init(sInputParams *pParams)

mfxStatus CTranscodingPipeline::VPPPreInit(sInputParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bVppCompInitRequire = false;
    m_bIsFieldWeaving = false;
    m_bIsFieldSplitting = false;

    if (((pParams->eModeExt == VppComp) || (pParams->eModeExt == VppCompOnly)) &&
        (pParams->eMode == Source))
        bVppCompInitRequire = true;

    // Obtaining decoder output FourCC - in case of inter-session, just take it from params, in intra-session case, take it from parent session
    // In inter-session case, we'll enable chroma-changing VPP only in encoding session, and only if decoderFourCC!=encoderFourCC
    mfxU32 decoderFourCC = m_bDecodeEnable ? m_mfxDecParams.mfx.FrameInfo.FourCC : m_pParentPipeline->GetDecodeParam().mfx.FrameInfo.FourCC;

    if (m_bEncodeEnable || m_bDecodeEnable)
    {
        if (m_mfxDecParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_SINGLE &&
            pParams->EncodeId != MFX_CODEC_HEVC && !pParams->bEnableDeinterlacing)
        {
            m_bIsFieldWeaving = true;
            m_bIsVpp = true;
        }

        if ((m_mfxDecParams.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_TFF || m_mfxDecParams.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BFF || m_mfxDecParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_UNKNOWN)
                    && pParams->EncodeId == MFX_CODEC_HEVC && pParams->DecodeId != MFX_CODEC_HEVC && !pParams->bEnableDeinterlacing)
        {
            m_bIsFieldSplitting = true;
            m_bIsVpp = true;
            m_mfxVppParams.vpp.In.PicStruct = MFX_PICSTRUCT_UNKNOWN;
        }

        if ( (m_mfxDecParams.mfx.FrameInfo.CropW != pParams->nDstWidth && pParams->nDstWidth) ||
             (m_mfxDecParams.mfx.FrameInfo.CropH != pParams->nDstHeight && pParams->nDstHeight) ||
             (pParams->bEnableDeinterlacing) || (pParams->DenoiseLevel!=-1) || (pParams->DetailLevel!=-1) || (pParams->FRCAlgorithm) ||
             (bVppCompInitRequire) || (pParams->fieldProcessingMode) ||
#ifdef ENABLE_MCTF
            (VPP_FILTER_DISABLED != pParams->mctfParam.mode) ||
#endif
             (pParams->EncoderFourCC && decoderFourCC && pParams->EncoderFourCC != decoderFourCC && m_bEncodeEnable))
        {
            if (m_bIsFieldWeaving || m_bIsFieldSplitting)
            {
                msdk_printf(MSDK_STRING("ERROR: Field weaving or Field Splitting is enabled according to streams parameters. Other VPP filters cannot be used in this mode, please remove corresponding options.\n"));
                return MFX_ERR_UNSUPPORTED;
            }

            m_bIsVpp = true;
        }

        if (m_bIsVpp)
        {
            sts = InitVppMfxParams(pParams);
            MSDK_CHECK_STATUS(sts, "InitVppMfxParams failed");
        }

        if (pParams->nRotationAngle) // plugin was requested
        {
            m_bIsPlugin = true;
            sts = InitPluginMfxParams(pParams);
            MSDK_CHECK_STATUS(sts, "InitPluginMfxParams failed");
            sts = LoadGenericPlugin();
            MSDK_CHECK_STATUS(sts, "LoadGenericPlugin failed");
        }

        if (!m_bIsPlugin && m_bIsVpp) // only VPP was requested
        {
            m_pmfxVPP.reset(new MFXVideoMultiVPP(*m_pmfxSession.get()));
        }
    }

    return sts;

} //mfxStatus CTranscodingPipeline::VPPInit(sInputParams *pParams)

mfxStatus CTranscodingPipeline::EncodePreInit(sInputParams *pParams)
{

    mfxStatus sts = MFX_ERR_NONE;

    if (m_bEncodeEnable)
    {
        if(pParams->EncodeId != MFX_CODEC_DUMP)
        {
            if (CheckVersion(&m_Version, MSDK_FEATURE_PLUGIN_API) && (m_pUserEncPlugin.get() == NULL))
            {
                /* Here we actually define the following codec initialization scheme:
                *  1. If plugin path or guid is specified: we load user-defined plugin (example: HEVC encoder plugin)
                *  2. If plugin path not specified:
                *    2.a) we check if codec is distributed as a mediasdk plugin and load it if yes
                *    2.b) if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library
                */
                if (pParams->encoderPluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(pParams->encoderPluginParams.strPluginPath,sizeof(pParams->encoderPluginParams.strPluginPath)))
                {
                    m_pUserEncoderModule.reset(new MFXVideoUSER(*m_pmfxSession.get()));
                    m_pUserEncoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), pParams->encoderPluginParams.pluginGuid, 1, pParams->encoderPluginParams.strPluginPath, (mfxU32)msdk_strnlen(pParams->encoderPluginParams.strPluginPath,sizeof(pParams->encoderPluginParams.strPluginPath))));
                    if (m_pUserEncoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
                }
                else
                {
                    bool isDefaultPlugin = false;
                    if (AreGuidsEqual(pParams->encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
                    {
                        pParams->encoderPluginParams.pluginGuid = msdkGetPluginUID(pParams->libType, MSDK_VENCODE, pParams->EncodeId);
                        isDefaultPlugin = true;
                    }
                    if (!AreGuidsEqual(pParams->encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
                    {
                        m_pUserEncoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), pParams->encoderPluginParams.pluginGuid, 1));
                        if (m_pUserEncoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
                    }
                    if(sts==MFX_ERR_UNSUPPORTED)
                    {
                        msdk_printf(isDefaultPlugin ?
                            MSDK_STRING("Default plugin cannot be loaded (possibly you have to define plugin explicitly)\n")
                            : MSDK_STRING("Explicitly specified plugin cannot be loaded.\n"));
                    }
                }
                MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
            }

            // create encoder
            m_pmfxENC.reset(new MFXVideoENCODE(*m_pmfxSession.get()));

            if (m_nVPPCompEnable == VppCompOnlyEncode)
            {
                pParams->EncoderFourCC  = MFX_FOURCC_NV12;
            }

            sts = InitEncMfxParams(pParams);
            MSDK_CHECK_STATUS(sts, "InitEncMfxParams failed");

            // Querying parameters
            mfxU16 ioPattern = m_mfxEncParams.IOPattern;

            msdk_stringstream str1, str2;
            CParametersDumper().SerializeVideoParamStruct(str1, MSDK_STRING(""), m_mfxEncParams);

            sts = m_pmfxENC->Query(&m_mfxEncParams, &m_mfxEncParams);

            CParametersDumper().SerializeVideoParamStruct(str2, MSDK_STRING(""), m_mfxEncParams);

            m_mfxEncParams.IOPattern=ioPattern; // Workaround for a problem: Query changes IOPattern incorrectly

            if (sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM)
            {
                msdk_printf(MSDK_STRING("[WARNING] Configuration changed on the Query() call\n"));

                CParametersDumper().ShowConfigurationDiff(str1, str2);
                MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
            }

            MSDK_CHECK_STATUS(sts, "m_pmfxENC->Query failed");
        }
        else
        {
            //--- This one is required for YUV output
            m_mfxEncParams.mfx.CodecId = pParams->EncodeId;
        }

    }
    return sts;

} // mfxStatus CTranscodingPipeline::EncodeInit(sInputParams *pParams)

mfxStatus CTranscodingPipeline::PreEncPreInit(sInputParams *pParams)
{
     mfxStatus sts = MFX_ERR_NONE;
     // PreInit is allowed in decoder session only
     if (pParams->bEnableExtLA && m_bDecodeEnable)
     {
        /* Here we actually define the following codec initialization scheme:
         *    a) we check if codec is distributed as a user plugin and load it if yes
         *    b) we check if codec is distributed as a mediasdk plugin and load it if yes
         *    c) if codec is not in the list of user plugins or mediasdk plugins, we assume, that it is supported inside mediasdk library
         */

        m_pUserEncPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), MFX_PLUGINID_H264LA_HW, 1));
        if (m_pUserEncPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;

         // create encoder
        m_pmfxPreENC.reset(new MFXVideoENC(*m_pmfxSession.get()));

        sts = InitPreEncMfxParams(pParams);
        MSDK_CHECK_STATUS(sts, "InitPreEncMfxParams failed");
    }
    return sts;

}

mfxVideoParam CTranscodingPipeline::GetDecodeParam() {
    if (m_bIsVpp)
     {
         mfxVideoParam tmp = m_mfxDecParams;
         tmp.mfx.FrameInfo = m_mfxVppParams.vpp.Out;
         return tmp;
     }
    else if (m_bIsPlugin)
    {
        mfxVideoParam tmp = m_mfxDecParams;
        tmp.mfx.FrameInfo = m_mfxPluginParams.mfx.FrameInfo;
        return tmp;
    }

     return m_mfxDecParams;
 };
// 1 ms provides better result in range [0..5] ms
enum
{
    TIME_TO_SLEEP = 1
};

mfxStatus CTranscodingPipeline::DecodeOneFrame(ExtendedSurface *pExtSurface)
{
    MFX_ITT_TASK("DecodeOneFrame");
    MSDK_CHECK_POINTER(pExtSurface,  MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_MORE_SURFACE;
    mfxFrameSurface1    *pmfxSurface = NULL;
    pExtSurface->pSurface = NULL;

    //--- Time measurements
    if (statisticsWindowSize)
    {
        inputStatistics.StopTimeMeasurementWithCheck();
        inputStatistics.StartTimeMeasurement();
    }

    CTimer DevBusyTimer;
    DevBusyTimer.Start();
    while (MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE < sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(*m_pmfxSession,m_LastDecSyncPoint,sts);
        }
        else if (MFX_ERR_MORE_DATA == sts)
        {
            sts = m_pBSProcessor->GetInputBitstream(&m_pmfxBS); // read more data to input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }
        else if (MFX_ERR_MORE_SURFACE == sts)
        {
            // Find new working surface
            pmfxSurface = GetFreeSurface(true, MSDK_SURFACE_WAIT_INTERVAL);
            AutomaticMutex guard(m_mStopSession);
            if (m_bForceStop)
            {
                NoMoreFramesSignal();
                return MFX_WRN_VALUE_NOT_CHANGED;
            }
            MSDK_CHECK_POINTER_SAFE(pmfxSurface, MFX_ERR_MEMORY_ALLOC, msdk_printf(MSDK_STRING("ERROR: No free surfaces in decoder pool (during long period)\n"))); // return an error if a free surface wasn't found
        }

        sts = m_pmfxDEC->DecodeFrameAsync(m_pmfxBS, pmfxSurface, &pExtSurface->pSurface, &pExtSurface->Syncp);

        if ( (MFX_WRN_DEVICE_BUSY == sts) &&
             (DevBusyTimer.GetTime() > MSDK_DEVICE_FREE_WAIT_INTERVAL/1000) )
        {
            msdk_printf(MSDK_STRING("ERROR: Decoder device busy (during long period)\n"));
            return MFX_ERR_DEVICE_FAILED;
        }

        if (sts==MFX_ERR_NONE)
        {
            m_LastDecSyncPoint = pExtSurface->Syncp;
        }
        // ignore warnings if output is available,
        if (MFX_ERR_NONE < sts && pExtSurface->Syncp)
        {
            sts = MFX_ERR_NONE;
        }

    } //while processing

    // HEVC SW requires additional sychronization
    if( MFX_ERR_NONE == sts && isHEVCSW)
    {
        sts = m_pmfxSession->SyncOperation(pExtSurface->Syncp, MSDK_WAIT_INTERVAL);
        HandlePossibleGpuHang(sts);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
    }
    return sts;

} // mfxStatus CTranscodingPipeline::DecodeOneFrame(ExtendedSurface *pExtSurface)
mfxStatus CTranscodingPipeline::DecodeLastFrame(ExtendedSurface *pExtSurface)
{
    MFX_ITT_TASK("DecodeLastFrame");
    mfxFrameSurface1    *pmfxSurface = NULL;
    mfxStatus sts = MFX_ERR_MORE_SURFACE;

    //--- Time measurements
    if (statisticsWindowSize)
    {
        inputStatistics.StopTimeMeasurementWithCheck();
        inputStatistics.StartTimeMeasurement();
    }

    CTimer DevBusyTimer;
    DevBusyTimer.Start();
    // retrieve the buffered decoded frames
    while (MFX_ERR_MORE_SURFACE == sts || MFX_WRN_DEVICE_BUSY == sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(*m_pmfxSession,m_LastDecSyncPoint,sts);
        }

        // find new working surface
        pmfxSurface = GetFreeSurface(true, MSDK_SURFACE_WAIT_INTERVAL);

        MSDK_CHECK_POINTER_SAFE(pmfxSurface, MFX_ERR_MEMORY_ALLOC, msdk_printf(MSDK_STRING("ERROR: No free surfaces in decoder pool (during long period)\n"))); // return an error if a free surface wasn't found
        sts = m_pmfxDEC->DecodeFrameAsync(NULL, pmfxSurface, &pExtSurface->pSurface, &pExtSurface->Syncp);

        if ( (MFX_WRN_DEVICE_BUSY == sts) &&
             (DevBusyTimer.GetTime() > MSDK_DEVICE_FREE_WAIT_INTERVAL/1000) )
        {
            msdk_printf(MSDK_STRING("ERROR: Decoder device busy (during long period)\n"));
            return MFX_ERR_DEVICE_FAILED;
        }
    }

    // HEVC SW requires additional sychronization
    if( MFX_ERR_NONE == sts && isHEVCSW)
    {
        sts = m_pmfxSession->SyncOperation(pExtSurface->Syncp,  MSDK_WAIT_INTERVAL);
        HandlePossibleGpuHang(sts);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
    }

    return sts;
}

mfxStatus CTranscodingPipeline::VPPOneFrame(ExtendedSurface *pSurfaceIn, ExtendedSurface *pExtSurface)
{
    MFX_ITT_TASK("VPPOneFrame");
    MSDK_CHECK_POINTER(pExtSurface,  MFX_ERR_NULL_PTR);
    mfxFrameSurface1 *pmfxSurface = NULL;
    // find/wait for a free working surface
    pmfxSurface = GetFreeSurface(false, MSDK_SURFACE_WAIT_INTERVAL);
    MSDK_CHECK_POINTER_SAFE(pmfxSurface, MFX_ERR_MEMORY_ALLOC, msdk_printf(MSDK_STRING("ERROR: No free surfaces for VPP in encoder pool (during long period)\n"))); // return an error if a free surface wasn't found

    // make sure picture structure has the initial value
    // surfaces are reused and VPP may change this parameter in certain configurations
    pmfxSurface->Info.PicStruct = m_mfxVppParams.vpp.Out.PicStruct ? m_mfxVppParams.vpp.Out.PicStruct : (m_bEncodeEnable ? m_mfxEncParams : m_mfxDecParams).mfx.FrameInfo.PicStruct;

    pExtSurface->pSurface = pmfxSurface;
    mfxStatus sts = MFX_ERR_NONE;

#ifdef ENABLE_MCTF
    bool bAttachMctfBuffer = false;
    mfxExtVppMctf * MctfRTParams = NULL;

    const sMctfRunTimeParam* pMctfCurParam = m_MctfRTParams.GetCurParam();
    bAttachMctfBuffer = NULL != pMctfCurParam;
    if (bAttachMctfBuffer && pSurfaceIn->pSurface)
    {
        // get a new (or existing) Mctf control buffer
        MctfRTParams = GetMctfParamBuffer<mfxExtVppMctf, MFX_EXTBUFF_VPP_MCTF>(pSurfaceIn->pSurface);
        if (MctfRTParams)
        {
            // suppose the following is going to to be pass:
            MctfRTParams->FilterStrength = pMctfCurParam->FilterStrength;
#if defined ENABLE_MCTF_EXT
            MctfRTParams->BitsPerPixelx100k = mfxU32(MCTF_LOSSLESS_BPP * MCTF_BITRATE_MULTIPLIER);
            MctfRTParams->Deblocking = MFX_CODINGOPTION_OFF;
#endif
            WipeOutExtParams(pSurfaceIn->pSurface, true, MAX_NUM_OF_ATTACHED_BUFFERS_FOR_IN_SUFACE);


            if (pSurfaceIn->pSurface->Data.NumExtParam >= MAX_NUM_OF_ATTACHED_BUFFERS_FOR_IN_SUFACE) {
                msdk_printf(MSDK_STRING("number of attached buffers to an input surface >= maximum allowed\n"));
                sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                return sts;
            }
            else
                pSurfaceIn->pSurface->Data.ExtParam[pSurfaceIn->pSurface->Data.NumExtParam++] = reinterpret_cast<mfxExtBuffer*>(MctfRTParams);
            m_MctfRTParams.MoveForward();
        }
        else
        {
            msdk_printf(MSDK_STRING("the extended buffer is not created; nothing can be attached; exit.\n"));
            sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            return(sts);
        }
    }
    else if (!bAttachMctfBuffer && pSurfaceIn->pSurface)
    {
        pSurfaceIn->pSurface->Data.NumExtParam = 0;
    }
#endif

    for(;;)
    {
        sts = m_pmfxVPP->RunFrameVPPAsync(pSurfaceIn->pSurface, pmfxSurface, NULL, &pExtSurface->Syncp);

        if (MFX_ERR_NONE < sts && !pExtSurface->Syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(1); // wait if device is busy
        }
        else if (MFX_ERR_NONE < sts && pExtSurface->Syncp)
        {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else
        {
            break;
        }
    }
    return sts;

} // mfxStatus CTranscodingPipeline::DecodeOneFrame(ExtendedSurface *pExtSurface)

mfxStatus CTranscodingPipeline::EncodeOneFrame(ExtendedSurface *pExtSurface, mfxBitstream *pBS)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!pBS->Data)
    {
        sts = AllocateSufficientBuffer(pBS);
        MSDK_CHECK_STATUS(sts, "AllocateSufficientBuffer failed");
    }

    for (;;)
    {
        // at this point surface for encoder contains either a frame from file or a frame processed by vpp
        sts = m_pmfxENC->EncodeFrameAsync(pExtSurface->pEncCtrl, pExtSurface->pSurface, pBS, &pExtSurface->Syncp);

        if (MFX_ERR_NONE < sts && !pExtSurface->Syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(TIME_TO_SLEEP); // wait if device is busy
        }
        else if (MFX_ERR_NONE < sts && pExtSurface->Syncp)
        {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
        {
            sts = AllocateSufficientBuffer(pBS);
            MSDK_CHECK_STATUS(sts, "AllocateSufficientBuffer failed");
        }
        else
        {
            break;
        }
    }
    return sts;

} //CTranscodingPipeline::EncodeOneFrame(ExtendedSurface *pExtSurface)

mfxStatus CTranscodingPipeline::PreEncOneFrame(ExtendedSurface *pInSurface, ExtendedSurface *pOutSurface)
{
    mfxStatus sts = MFX_ERR_NONE;
    PreEncAuxBuffer* pAux = NULL;

    for (mfxU32 i = 0; i < MSDK_WAIT_INTERVAL; i += TIME_TO_SLEEP)
    {
        pAux = GetFreePreEncAuxBuffer();
        if (pAux)
        {
            break;
        }
        else
        {
            MSDK_SLEEP(TIME_TO_SLEEP);
        }
    }
    MSDK_CHECK_POINTER(pAux,  MFX_ERR_MEMORY_ALLOC);
    for (;;)
    {
        pAux->encInput.InSurface = pInSurface->pSurface;
        // at this point surface for encoder contains either a frame from file or a frame processed by vpp
        sts = m_pmfxPreENC->ProcessFrameAsync(&pAux->encInput, &pAux->encOutput, &pOutSurface->Syncp );

        if (MFX_ERR_NONE < sts && !pOutSurface->Syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
                MSDK_SLEEP(TIME_TO_SLEEP); // wait if device is busy
        }
        else if (MFX_ERR_NONE <= sts && pOutSurface->Syncp)
        {
            LockPreEncAuxBuffer(pAux);
            pOutSurface->pAuxCtrl = pAux;
            MSDK_CHECK_POINTER(pAux->encOutput.ExtParam, MFX_ERR_NULL_PTR);
            MSDK_CHECK_NOT_EQUAL(pAux->encOutput.NumExtParam, 1, MFX_ERR_UNSUPPORTED);
            pOutSurface->pSurface =  ((mfxExtLAFrameStatistics *) pAux->encOutput.ExtParam[0])->OutSurface;
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

// signal that there are no more frames
void CTranscodingPipeline::NoMoreFramesSignal()
{
    ExtendedSurface surf={};
    SafetySurfaceBuffer   *pNextBuffer = m_pBuffer;
    pNextBuffer->AddSurface(surf);
    /*if 1_to_N mode */
    if (0 == m_nVPPCompEnable)
    {
        while (pNextBuffer->m_pNext)
        {
            pNextBuffer = pNextBuffer->m_pNext;
            pNextBuffer->AddSurface(surf);
        }
    }
}

void CTranscodingPipeline::StopSession()
{
    AutomaticMutex guard(m_mStopSession);
    m_bForceStop = true;
}

bool CTranscodingPipeline::IsOverlayUsed()
{
    return m_bUseOverlay;
}

mfxStatus CTranscodingPipeline::Decode()
{
    mfxStatus sts = MFX_ERR_NONE;

    ExtendedSurface DecExtSurface    = {0};
    ExtendedSurface VppExtSurface    = {0};
    ExtendedSurface PreEncExtSurface = {0};
    bool shouldReadNextFrame=true;

    SafetySurfaceBuffer   *pNextBuffer = m_pBuffer;
    bool bEndOfFile = false;
    bool bLastCycle = false;
    time_t start = time(0);

    m_mStopSession.Lock();
    if (m_bForceStop)
    {
        m_mStopSession.Unlock();
        // add surfaces in queue for all sinks
        NoMoreFramesSignal();
        return MFX_WRN_VALUE_NOT_CHANGED;
    }
    m_mStopSession.Unlock();

    if (m_bUseOverlay)
    {
        PreEncExtSurface.pSurface = m_pSurfaceDecPool[0];

        if (pNextBuffer->GetLength() == 0)
        {
            // add surfaces in queue for all sinks
            pNextBuffer->AddSurface(PreEncExtSurface);
            m_nProcessedFramesNum++;
        }
        return MFX_ERR_NONE;
    }

    while (MFX_ERR_NONE == sts)
    {
        pNextBuffer = m_pBuffer;

        if (time(0) - start >= m_nTimeout)
            bLastCycle = true;

        if (bLastCycle)
            SetNumFramesForReset(0);

        msdk_tick nBeginTime = msdk_time_get_tick(); // microseconds.

        if(shouldReadNextFrame)
        {
            if (!bEndOfFile)
            {
                if (!m_bUseOverlay)
                {
                    sts = DecodeOneFrame(&DecExtSurface);
                    if (MFX_ERR_MORE_DATA == sts)
                    {
                        sts = bLastCycle ? DecodeLastFrame(&DecExtSurface) : MFX_ERR_MORE_DATA;
                        bEndOfFile = bLastCycle ? true : false;
                    }
                }
                else
                {
                    // Use preloaded overlay frame
                    DecExtSurface.pSurface = m_pSurfaceDecPool[0];
                    sts = MFX_ERR_NONE;
                }
            }
            else
            {
                sts = DecodeLastFrame(&DecExtSurface);
            }

            if (sts == MFX_ERR_NONE)
            {
                // print statistics if m_nProcessedFramesNum is multiple of
                // statisticsWindowSize OR we at the end of file AND
                // statisticsWindowSize is not zero
                if (statisticsWindowSize && m_nProcessedFramesNum &&
                    ((0 == m_nProcessedFramesNum % statisticsWindowSize) || bEndOfFile)
                    )
                {
                    inputStatistics.PrintStatistics(GetPipelineID());
                    inputStatistics.ResetStatistics();
                }
            }
            if (sts == MFX_ERR_MORE_DATA && (m_pmfxVPP.get() || m_pmfxPreENC.get()))
            {
                DecExtSurface.pSurface = NULL;  // to get buffered VPP or ENC frames
                sts = MFX_ERR_NONE;
            }
            if (!bLastCycle && (DecExtSurface.pSurface == NULL) )
            {
                m_pBSProcessor->ResetInput();

                if (!GetNumFramesForReset())
                    SetNumFramesForReset(m_nProcessedFramesNum);
                sts = MFX_ERR_NONE;
                continue;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (m_pmfxVPP.get())
        {
            if (m_bIsFieldWeaving)
            {
                // We might have 2 cases: decoder gives us pairs (TF BF)... or (BF)(TF). In first case we should set TFF for output, in second - BFF.
                // So, if even input surface is BF, we set TFF for output and vise versa. For odd input surface - no matter what we set.
                if (!(m_nProcessedFramesNum % 2))
                {
                    if (DecExtSurface.pSurface)
                    {
                        if ((DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF))
                        {
                            m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_BFF;
                        }
                        if (DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                        {
                            m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
                        }
                    }
                }
                sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
            }
            else
            {
                if (m_bIsFieldSplitting)
                {
                    if (DecExtSurface.pSurface)
                    {
                        if (DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF || DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                        {
                            m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_SINGLE;
                            sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                        }
                        else
                        {
                            VppExtSurface.pSurface = DecExtSurface.pSurface;
                            VppExtSurface.pAuxCtrl = DecExtSurface.pAuxCtrl;
                            VppExtSurface.Syncp = DecExtSurface.Syncp;
                        }
                    }
                    else
                    {
                        sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                    }
                }
                else
                {
                    sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                }
            }
            // check for interlaced stream
        }
        else // no VPP - just copy pointers
        {
            VppExtSurface.pSurface = DecExtSurface.pSurface;
            VppExtSurface.Syncp = DecExtSurface.Syncp;
        }

        //--- Sometimes VPP may return 2 surfaces on output, for the first one it'll return status MFX_ERR_MORE_SURFACE - we have to call VPPOneFrame again in this case
        if(MFX_ERR_MORE_SURFACE == sts)
        {
            shouldReadNextFrame=false;
            sts=MFX_ERR_NONE;
        }
        else
        {
            shouldReadNextFrame=true;
        }

        if (sts == MFX_ERR_MORE_DATA || !VppExtSurface.pSurface)
        {
            if (!bEndOfFile )
            {
                sts = MFX_ERR_NONE;
                continue; // go get next frame from Decode
            }
        }
        if (sts == MFX_ERR_MORE_DATA && m_pmfxPreENC.get())
        {
           VppExtSurface.pSurface = NULL;  // to get buffered VPP or ENC frames
           sts = MFX_ERR_NONE;
        }

        MSDK_BREAK_ON_ERROR(sts);

        if (m_pmfxPreENC.get())
        {
            sts = PreEncOneFrame(&VppExtSurface, &PreEncExtSurface);
        }
        else // no VPP - just copy pointers
        {
            PreEncExtSurface.pSurface = VppExtSurface.pSurface;
            PreEncExtSurface.Syncp = VppExtSurface.Syncp;
        }

        if (sts == MFX_ERR_MORE_DATA || !PreEncExtSurface.pSurface)
        {
            if (!bEndOfFile )
            {
                sts = MFX_ERR_NONE;
                continue; // go get next frame from Decode
            }
        }
        if (!bLastCycle)
        {
            sts = MFX_ERR_NONE;
        }
        MSDK_BREAK_ON_ERROR(sts);

        // If session is not joined and it is not parent - synchronize.
        // If there was PreENC plugin in the pipeline - synchronize, because
        // plugin will output data to the extended buffers and mediasdk can't
        // track such dependency on its own.
        if ((!m_bIsJoinSession && m_pParentPipeline) || m_pmfxPreENC.get())
        {
            MFX_ITT_TASK("SyncOperation");
            sts = m_pmfxSession->SyncOperation(PreEncExtSurface.Syncp, MSDK_WAIT_INTERVAL);
            HandlePossibleGpuHang(sts);
            PreEncExtSurface.Syncp = NULL;
            MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
        }

        size_t i = 0;
        // Do not exceed buffer length (it should be not longer than AsyncDepth after adding newly processed surface)
        while(pNextBuffer->GetLength()>=m_AsyncDepth)
        {
            m_mStopSession.Lock();
            if (m_bForceStop)
            {
                m_mStopSession.Unlock();
                NoMoreFramesSignal();
                return MFX_WRN_VALUE_NOT_CHANGED;
            }
            m_mStopSession.Unlock();

            pNextBuffer->WaitForSurfaceRelease(MSDK_SURFACE_WAIT_INTERVAL / 1000);

            if (++i > 1000)
            {
                msdk_printf(MSDK_STRING("ERROR: timed out waiting surface release by downstream component\n"));
                return MFX_ERR_NOT_FOUND;
            }
        }

        // add surfaces in queue for all sinks
        pNextBuffer->AddSurface(PreEncExtSurface);
        /* one of key parts for N_to_1 mode:
        * decoded frame should be in one buffer only as we have only 1 (one!) sink
        * */
        if (0 == m_nVPPCompEnable)
        {
            while (pNextBuffer->m_pNext)
            {
                pNextBuffer = pNextBuffer->m_pNext;
                pNextBuffer->AddSurface(PreEncExtSurface);
            }
        }

        // We need to synchronize oldest stored surface if we've already stored enough surfaces in buffer (buffer length >= AsyncDepth)
        // Because we have to wait for decoder to finish processing and free some internally used surfaces
        //mfxU32 len = pNextBuffer->GetLength();
        if(pNextBuffer->GetLength()>=m_AsyncDepth)
        {
            ExtendedSurface frontSurface;
            pNextBuffer->GetSurface(frontSurface);

            if(frontSurface.Syncp)
            {
                sts = m_pmfxSession->SyncOperation(frontSurface.Syncp, MSDK_WAIT_INTERVAL);
                HandlePossibleGpuHang(sts);
                MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
                frontSurface.Syncp=NULL;
            }
        }

        if (!statisticsWindowSize && 0 == (m_nProcessedFramesNum - 1) % 100)
        {
            msdk_printf(MSDK_STRING("."));
        }

        if (bEndOfFile && m_nTimeout)
        {
            break;
        }

        msdk_tick nFrameTime = msdk_time_get_tick() - nBeginTime;
        if (nFrameTime < m_nReqFrameTime)
        {
            MSDK_USLEEP((mfxU32)(m_nReqFrameTime - nFrameTime));
        }
        if (++m_nProcessedFramesNum >= m_MaxFramesForTranscode)
        {
            break;
        }
    }

    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    NoMoreFramesSignal();

    if (MFX_ERR_NONE == sts)
        sts = MFX_WRN_VALUE_NOT_CHANGED;

    return sts;
} // mfxStatus CTranscodingPipeline::Decode()

mfxStatus CTranscodingPipeline::Encode()
{
    mfxStatus sts = MFX_ERR_NONE;
    ExtendedSurface DecExtSurface = {};
    ExtendedSurface VppExtSurface = {};
    ExtendedBS      *pBS = NULL;
    bool isQuit = false;
    bool bPollFlag = false;
    int nFramesAlreadyPut = 0;
    SafetySurfaceBuffer   *curBuffer = m_pBuffer;

    bool shouldReadNextFrame=true;
    while (MFX_ERR_NONE == sts ||  MFX_ERR_MORE_DATA == sts)
    {
        msdk_tick nBeginTime = msdk_time_get_tick(); // microseconds
        if(shouldReadNextFrame)
        {
            if(isQuit)
            {
                // We're here because one of decoders has reported that there're no any more frames ready.
                //So, let's pass null surface to extract data from the VPP and encoder caches.

                MSDK_ZERO_MEMORY(DecExtSurface);
            }
            else
            {
                // Getting next frame
                while (MFX_ERR_MORE_SURFACE == curBuffer->GetSurface(DecExtSurface))
                {
                    if (MFX_ERR_NONE != curBuffer->WaitForSurfaceInsertion(MSDK_SURFACE_WAIT_INTERVAL)) {
                        msdk_printf(MSDK_STRING("ERROR: timed out waiting surface from upstream component\n"));
                        return MFX_ERR_NOT_FOUND;
                    }
                }
            }

            // if session is not joined and it is not parent - synchronize
            if (!m_bIsJoinSession && m_pParentPipeline)
            {
                // if it is not already synchronized
                if (DecExtSurface.Syncp)
                {
                    MFX_ITT_TASK("SyncOperation");
                    sts = m_pParentPipeline->m_pmfxSession->SyncOperation(DecExtSurface.Syncp, MSDK_WAIT_INTERVAL);
                    HandlePossibleGpuHang(sts);
                    MSDK_CHECK_STATUS(sts, "m_pParentPipeline->m_pmfxSession->SyncOperation failed");
                }
            }

            mfxU32 NumFramesForReset = m_pParentPipeline ? m_pParentPipeline->GetNumFramesForReset() : 0;
            if (NumFramesForReset && !(nFramesAlreadyPut % NumFramesForReset) )
            {
                m_bInsertIDR = true;
            }

            if (NULL == DecExtSurface.pSurface)
            {
                isQuit = true;
            }
        }

        if (m_pmfxVPP.get())
        {
            sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
            VppExtSurface.pAuxCtrl = DecExtSurface.pAuxCtrl;
        }
        else // no VPP - just copy pointers
        {
            VppExtSurface.pSurface = DecExtSurface.pSurface;
            VppExtSurface.pAuxCtrl = DecExtSurface.pAuxCtrl;
            VppExtSurface.Syncp = DecExtSurface.Syncp;
        }

        if(MFX_ERR_MORE_SURFACE == sts)
        {
            shouldReadNextFrame=false;
            sts=MFX_ERR_NONE;
        }
        else
        {
            shouldReadNextFrame=true;
        }

        if (MFX_ERR_MORE_DATA == sts)
        {
            if(isQuit)
            {
                // to get buffered VPP or ENC frames
                VppExtSurface.pSurface = NULL;
                sts = MFX_ERR_NONE;
            }
            else
            {
                curBuffer->ReleaseSurface(DecExtSurface.pSurface);

                //--- We should switch to another buffer ONLY in case of Composition
                if (curBuffer->m_pNext != NULL && m_nVPPCompEnable > 0)
                {
                    curBuffer = curBuffer->m_pNext;
                    continue;
                }
                else
                {
                    curBuffer = m_pBuffer;
                    continue; /* No more buffer from decoders */
                }
            }
        }

        MSDK_CHECK_STATUS(sts, "Unexpected error!!");

        if (m_nVPPCompEnable > 0)
            curBuffer->ReleaseSurface(DecExtSurface.pSurface);

        // Do RenderFrame before Encode to improves on-screen performance
        // Presentation packet would now precedes "ENC" packet within the EU
        if ((m_nVPPCompEnable == VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
        {
            if(VppExtSurface.pSurface)
            {
                // Sync to ensure VPP is completed to avoid flicker
                sts = m_pmfxSession->SyncOperation(VppExtSurface.Syncp, MSDK_WAIT_INTERVAL);
                HandlePossibleGpuHang(sts);
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                /* in case if enabled dumping into file for after VPP composition */
                if (DUMP_FILE_VPP_COMP == m_vppCompDumpRenderMode)
                {
                    sts = DumpSurface2File(VppExtSurface.pSurface);
                    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
                }
                /* Rendering may be explicitly disabled for performance measurements */
                if (NULL_RENDER_VPP_COMP != m_vppCompDumpRenderMode)
                {

#if defined(_WIN32) || defined(_WIN64)
                sts = m_hwdev4Rendering->RenderFrame(VppExtSurface.pSurface, m_pMFXAllocator);
#else
                sts = m_hwdev4Rendering->RenderFrame(VppExtSurface.pSurface, NULL);
#endif
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
                } // if (NULL_RENDER_VPP_COMP != m_vppCompDumpRenderMode)
            }
        }

        curBuffer = m_pBuffer;

        pBS = m_pBSStore->GetNext();
        if (!pBS)
            return MFX_ERR_NOT_FOUND;

        m_BSPool.push_back(pBS);

        mfxU32 NumFramesForReset = m_pParentPipeline ? m_pParentPipeline->GetNumFramesForReset() : 0;
        if (NumFramesForReset && !(m_nProcessedFramesNum % NumFramesForReset))
        {
            m_pBSProcessor->ResetOutput();
        }

        SetEncCtrlRT(VppExtSurface, m_bInsertIDR);
        m_bInsertIDR = false;

        if ((m_nVPPCompEnable != VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
        {
            if(m_mfxEncParams.mfx.CodecId != MFX_CODEC_DUMP)
            {
                if (bPollFlag)
                {
                    VppExtSurface.pSurface = 0;
                }
                sts = EncodeOneFrame(&VppExtSurface, &m_BSPool.back()->Bitstream);

                // Count only real surfaces
                if (VppExtSurface.pSurface)
                {
                    m_nProcessedFramesNum++;
                }

                if (m_nProcessedFramesNum >= m_MaxFramesForTranscode)
                {
                    bPollFlag = true;
                }

                if (!sts)
                    nFramesAlreadyPut++;
            }
            else
            {
                sts = Surface2BS(&VppExtSurface, &m_BSPool.back()->Bitstream, m_encoderFourCC);
            }
        }

        if(shouldReadNextFrame) // Release current decoded surface only if we're going to read next one during next iteration
        {
            m_pBuffer->ReleaseSurface(DecExtSurface.pSurface);
        }

        // check if we need one more frame from decode
        if (MFX_ERR_MORE_DATA == sts)
        {
            // the task in not in Encode queue
            m_BSPool.pop_back();
            m_pBSStore->Release(pBS);

            if (NULL == VppExtSurface.pSurface ) // there are no more buffered frames in encoder
            {
                break;
            }
            else
            {
                // get next frame from Decode
                sts = MFX_ERR_NONE;
                continue;
            }
        }

        // check encoding result
        MSDK_CHECK_STATUS(sts, "<EncodeOneFrame|Surface2BS> failed");

        // output statistics if several conditions are true OR we've approached
        // the end, and statisticsWindowSize is not 0, but number of frames is
        // not multiple of statisticsWindowSize; should use m_nProcessedFramesNum
        // as it simplifies conditions
        if ( (statisticsWindowSize && m_nOutputFramesNum && 0 == m_nProcessedFramesNum % statisticsWindowSize) ||
             (statisticsWindowSize && (m_nProcessedFramesNum >= m_MaxFramesForTranscode)))
        {
            outputStatistics.PrintStatistics(GetPipelineID());
            outputStatistics.ResetStatistics();
        }

        m_BSPool.back()->Syncp = VppExtSurface.Syncp;
        m_BSPool.back()->pCtrl =  VppExtSurface.pAuxCtrl;

        /* Actually rendering... if enabled
         * SYNC have not done by driver !!! */
        if ((m_nVPPCompEnable == VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
        {
            if(m_BSPool.size())
            {
                ExtendedBS *pBitstreamEx_temp  = m_BSPool.front();

                // get result coded stream
////Note! Better to do rendering before encode
//                if(VppExtSurface.pSurface)
//                {
//                    if(m_nVPPCompEnable != VppCompOnlyEncode)
//                    {
//                        sts = m_pmfxSession->SyncOperation(VppExtSurface.Syncp, MSDK_WAIT_INTERVAL);
//                        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
//                    }
//#if defined(_WIN32) || defined(_WIN64)
//                    sts = m_hwdev4Rendering->RenderFrame(VppExtSurface.pSurface, m_pMFXAllocator);
//#else
//                    sts = m_hwdev4Rendering->RenderFrame(VppExtSurface.pSurface, NULL);
//#endif
//                    MSDK_CHECK_STATUS(sts, "m_hwdev4Rendering->RenderFrame failed");
//                }

                UnPreEncAuxBuffer(pBitstreamEx_temp->pCtrl);

                if (m_nVPPCompEnable != VppCompOnlyEncode)
                {
                    pBitstreamEx_temp->Bitstream.DataLength = 0;
                    pBitstreamEx_temp->Bitstream.DataOffset = 0;

                    m_BSPool.pop_front();
                    m_pBSStore->Release(pBitstreamEx_temp);
                }
            }

            //--- If there's no data coming out from VPP and there's no data coming from decoders (isQuit==true),
            // then we should quit, otherwise we may stuck here forever (cause there's no new data coming)
            if(!VppExtSurface.pSurface && isQuit)
            {
                break;
            }
        }

        if ((m_nVPPCompEnable != VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
        {
            if (m_BSPool.size() == m_AsyncDepth)
            {
                sts = PutBS();
                MSDK_CHECK_STATUS(sts, "PutBS failed");
            }
            else
            {
                continue;
            }
        } // if (m_nVPPCompEnable != VppCompOnly)

        msdk_tick nFrameTime = msdk_time_get_tick() - nBeginTime;
        if (nFrameTime < m_nReqFrameTime)
        {
            MSDK_USLEEP((mfxU32)(m_nReqFrameTime - nFrameTime));
        }
    }
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    if (m_nVPPCompEnable != VppCompOnly || (m_nVPPCompEnable == VppCompOnlyEncode))
    {
        // need to get buffered bitstream
        if (MFX_ERR_NONE == sts)
        {
            while (m_BSPool.size())
            {
                sts = PutBS();
                MSDK_CHECK_STATUS(sts, "PutBS failed");
            }
        }
    }

    // Clean up decoder buffers to avoid locking them (if some decoder still have some data to decode, but does not have enough surfaces)
    if(m_nVPPCompEnable!=0)
    {
        // Composition case - we have to clean up all buffers (all of them have data from decoders)
        for(SafetySurfaceBuffer * buf = m_pBuffer;buf!=NULL; buf=buf->m_pNext)
        {
            while (buf->GetSurface(DecExtSurface)!=MFX_ERR_MORE_SURFACE)
            {
                buf->ReleaseSurface(DecExtSurface.pSurface);
                buf->CancelBuffering();
            }
        }
    }
    else
    {
        // Clean up only current buffer
        m_pBuffer->CancelBuffering();
        while (m_pBuffer->GetSurface(DecExtSurface)!=MFX_ERR_MORE_SURFACE)
        {
            m_pBuffer->ReleaseSurface(DecExtSurface.pSurface);
        }
    }

    // Close encoder and decoder and clean up buffers
    if (m_pmfxENC.get())
        m_pmfxENC->Close();

    if (m_pmfxVPP.get())
        m_pmfxVPP->Close();


    if (MFX_ERR_NONE == sts)
        sts = MFX_WRN_VALUE_NOT_CHANGED;
    return sts;

} // mfxStatus CTranscodingPipeline::Encode()

#if MFX_VERSION >= 1022
void CTranscodingPipeline::FillMBQPBuffer(mfxExtMBQP &qpMap, mfxU16 pictStruct)
{
    // External MBQP case
    if (m_bExtMBQP)
    {
        // Use simplistic approach to fill in QP buffer
        for (size_t i = 0; i < qpMap.NumQPAlloc; i++)
        {
            qpMap.QP[i] = i % 52;
        }
        return;
    }

    // External MBQP with ROI case
    if (pictStruct == MFX_PICSTRUCT_PROGRESSIVE)
    {
        mfxI8 fQP = (m_nSubmittedFramesNum % m_GOPSize) ? (mfxI8)m_QPforP : (mfxI8)m_QPforI;
        std::memset(qpMap.QP, fQP, qpMap.NumQPAlloc);

//        printf("QP expectation: %d, map PTR %lld\n",fQP,(long long int)qpMap.QP);

        if (m_ROIData.size() > m_nSubmittedFramesNum)
        {
            mfxExtEncoderROI &roi = m_ROIData[m_nSubmittedFramesNum];
            for (mfxI32 i=roi.NumROI-1; i>=0; i--)
            {
                mfxU32  l = (roi.ROI[i].Left) >> 4,
                        t = (roi.ROI[i].Top) >> 4,
                        r = (roi.ROI[i].Right + 15) >> 4,
                        b = (roi.ROI[i].Bottom + 15) >> 4;

                //Additional 32x32 block alignment for HEVC
                if(m_mfxEncParams.mfx.CodecId == MFX_CODEC_HEVC)
                {
                    l = ((roi.ROI[i].Left) >> 5) << 1;
                    t = ((roi.ROI[i].Top) >> 5) << 1;
                    r = ((roi.ROI[i].Right + 31) >> 5) << 1;
                    b = ((roi.ROI[i].Bottom + 31) >> 5) << 1;
                }

                if(l > m_QPmapWidth) l = m_QPmapWidth;
                if(r > m_QPmapWidth) r = m_QPmapWidth;
                if(t > m_QPmapHeight) t = m_QPmapHeight;
                if(b > m_QPmapHeight) b = m_QPmapHeight;

                mfxI8   qp_value = (mfxI8)std::min(std::max(fQP + (mfxI8)roi.ROI[i].DeltaQP, 0), 51);

                for (mfxU32 k=t; k<b; k++)
                    std::memset(qpMap.QP + k*m_QPmapWidth + l, qp_value, r-l);
            }
        }
    }
    else if (pictStruct == MFX_PICSTRUCT_FIELD_TFF || pictStruct == MFX_PICSTRUCT_FIELD_BFF)
    {
        mfxU32  fQP[2] = {(m_nSubmittedFramesNum % m_GOPSize) ? m_QPforP : m_QPforI,
                          (m_GOPSize > 1) ? m_QPforP : m_QPforI};
        mfxU32 fIdx[2] = {2*m_nSubmittedFramesNum, 2*m_nSubmittedFramesNum+1};
        mfxU32 fOff[2] = {0, 0};

        fOff[(pictStruct == MFX_PICSTRUCT_FIELD_BFF) ? 0 : 1] = qpMap.NumQPAlloc/2;

        for (int fld=0; fld<=1; fld++)
        {
            std::memset(qpMap.QP + fOff[fld], fQP[fld], qpMap.NumQPAlloc/2);
            if (m_ROIData.size() > fIdx[fld])
            {
                mfxExtEncoderROI &roi = m_ROIData[fIdx[fld]];
                for (mfxI32 i=roi.NumROI-1; i>=0; i--)
                {
                    mfxU32  l = (roi.ROI[i].Left) >> 4,
                            t = (roi.ROI[i].Top) >> 5,
                            r = (roi.ROI[i].Right + 15) >> 4,
                            b = (roi.ROI[i].Bottom + 31) >> 5;
                    mfxI8   qp_delta = (mfxI8)roi.ROI[i].DeltaQP;
                    mfxU8   roi_qp = (mfxU8)std::min(std::max(mfxI8(fQP[fld]) + qp_delta, 0), 51);

                    if(l > m_QPmapWidth) l = m_QPmapWidth;
                    if(r > m_QPmapWidth) r = m_QPmapWidth;
                    if(t > m_QPmapHeight/2) t = m_QPmapHeight/2;
                    if(b > m_QPmapHeight/2) b = m_QPmapHeight/2;

                    for (mfxU32 k=t; k<b; k++)
                        std::memset(qpMap.QP + fOff[fld] + k*m_QPmapWidth + l, roi_qp, r-l);
                }
            }
        }
    }
    else
    {
        mfxI8 fQP = (m_nSubmittedFramesNum % m_GOPSize) ? (mfxI8)m_QPforP : (mfxI8)m_QPforI;
        std::memset(qpMap.QP, fQP, qpMap.NumQPAlloc);
    }
}
#endif //MFX_VERSION >= 1022

void CTranscodingPipeline::SetEncCtrlRT(ExtendedSurface& extSurface, bool bInsertIDR)
{
    extSurface.pEncCtrl = NULL;
    if (extSurface.pAuxCtrl) {
        extSurface.pEncCtrl = &extSurface.pAuxCtrl->encCtrl;
    }

#if MFX_VERSION >= 1022

    if (extSurface.pSurface) {
        void* keyId = (void*)extSurface.pSurface;

        // Use encoded surface pointer to find placeholders for run-time structures in maps
        if (m_bUseQPMap && m_bufExtMBQP.find(keyId) == m_bufExtMBQP.end()) {
            m_extBuffPtrStorage[keyId] = std::vector<mfxExtBuffer*>();

            m_qpMapStorage[keyId] = std::vector<mfxU8>();
            m_qpMapStorage[keyId].resize(m_QPmapWidth*m_QPmapHeight);

            m_bufExtMBQP[keyId] = mfxExtMBQP();
            m_bufExtMBQP[keyId].Header.BufferId = MFX_EXTBUFF_MBQP;
            m_bufExtMBQP[keyId].Header.BufferSz = sizeof(mfxExtMBQP);
            m_bufExtMBQP[keyId].NumQPAlloc = m_QPmapWidth*m_QPmapHeight;
            m_bufExtMBQP[keyId].QP = m_QPmapWidth && m_QPmapHeight ? &(m_qpMapStorage[keyId][0]) : NULL;
        }

        // Initialize *pCtrl optionally copying content of the pExtSurface.pAuxCtrl.encCtrl
        mfxEncodeCtrl& ctrl = encControlStorage[keyId];
        MSDK_ZERO_MEMORY(ctrl);

        if (extSurface.pEncCtrl) {
            ctrl = *extSurface.pEncCtrl;
        }

        // Copy all extended buffer pointers from pExtSurface.pAuxCtrl.encCtrl
        m_extBuffPtrStorage[keyId].clear();
        if (extSurface.pAuxCtrl) {
            for (unsigned int i = 0; i < ctrl.NumExtParam; i++) {
                 m_extBuffPtrStorage[keyId].push_back(extSurface.pAuxCtrl->encCtrl.ExtParam[i]);
            }
        }

        // Attach additional buffer with either MBQP or ROI information
        if (m_bUseQPMap) {
#if (MFX_VERSION >= 1022)
            FillMBQPBuffer(m_bufExtMBQP[keyId], extSurface.pSurface->Info.PicStruct);
            m_extBuffPtrStorage[keyId].push_back((mfxExtBuffer *)&m_bufExtMBQP[keyId]);
#endif
        } else {
            if (m_ROIData.size() > m_nSubmittedFramesNum)
                m_extBuffPtrStorage[keyId].push_back((mfxExtBuffer *)&m_ROIData[m_nSubmittedFramesNum]);
        }

        // Replace the buffers pointer to pre-allocated storage
        ctrl.NumExtParam = (mfxU16)m_extBuffPtrStorage[keyId].size();
        if (ctrl.NumExtParam) {
            ctrl.ExtParam = &(m_extBuffPtrStorage[keyId][0]);
        }

        extSurface.pEncCtrl = &ctrl;
        m_nSubmittedFramesNum++;
    }
#endif //MFX_VERSION >= 1022

    if (bInsertIDR && extSurface.pSurface)
    {
        if (extSurface.pEncCtrl == NULL)
        {
            mfxEncodeCtrl& ctrl = encControlStorage[(void*)extSurface.pSurface];
            MSDK_ZERO_MEMORY(ctrl);
            extSurface.pEncCtrl = &ctrl;
        }
        extSurface.pEncCtrl->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
    }
    else
    {
        if (extSurface.pEncCtrl)
        {
            extSurface.pEncCtrl->FrameType = 0;
        }
    }
}

mfxStatus CTranscodingPipeline::Transcode()
{
    mfxStatus sts = MFX_ERR_NONE;
    ExtendedSurface DecExtSurface = {0};
    ExtendedSurface VppExtSurface = {0};
    ExtendedBS *pBS = NULL;
    bool bNeedDecodedFrames = true; // indicates if we need to decode frames
    bool bEndOfFile = false;
    bool bLastCycle = false;
    bool shouldReadNextFrame=true;

    time_t start = time(0);
    while (MFX_ERR_NONE == sts )
    {
        msdk_tick nBeginTime = msdk_time_get_tick(); // microseconds.

        if (time(0) - start >= m_nTimeout)
            bLastCycle = true;
        if (m_MaxFramesForTranscode == m_nProcessedFramesNum)
        {
            DecExtSurface.pSurface = NULL;  // to get buffered VPP or ENC frames
            bNeedDecodedFrames = false; // no more decoded frames needed
        }

        // if need more decoded frames
        // decode a frame
        if (bNeedDecodedFrames && shouldReadNextFrame)
        {
            if (!bEndOfFile)
            {
                sts = DecodeOneFrame(&DecExtSurface);
                if (MFX_ERR_MORE_DATA == sts)
                {
                    if (!bLastCycle)
                    {
                        m_bInsertIDR = true;

                        m_pBSProcessor->ResetInput();
                        m_pBSProcessor->ResetOutput();
                        bNeedDecodedFrames = true;

                        bEndOfFile = false;
                        sts = MFX_ERR_NONE;
                        continue;
                    }
                    else
                    {
                        bEndOfFile = true;
                    }
                }
            }

            if (bEndOfFile)
            {
                sts = DecodeLastFrame(&DecExtSurface);
            }

            if (sts == MFX_ERR_MORE_DATA)
            {
                DecExtSurface.pSurface = NULL;  // to get buffered VPP or ENC frames
                sts = MFX_ERR_NONE;
            }
            MSDK_CHECK_STATUS(sts, "Decode<One|Last>Frame failed");
        }
        if (m_bIsFieldWeaving && DecExtSurface.pSurface != NULL)
        {
            m_mfxDecParams.mfx.FrameInfo.PicStruct = DecExtSurface.pSurface->Info.PicStruct;
        }
        if (m_bIsFieldSplitting && DecExtSurface.pSurface != NULL)
        {
            m_mfxDecParams.mfx.FrameInfo.PicStruct = DecExtSurface.pSurface->Info.PicStruct;
        }
        // pre-process a frame
        if (m_pmfxVPP.get() && bNeedDecodedFrames)
        {
            if (m_bIsFieldWeaving)
            {
                // In case of field weaving output surface's parameters for ODD calls to VPPOneFrame will be ignored (because VPP will return ERR_MORE_DATA).
                // So, we need to set output surface picstruct properly for EVEN calls (no matter what will be set for ODD calls).
                // We might have 2 cases: decoder gives us pairs (TF BF)... or (BF)(TF). In first case we should set TFF for output, in second - BFF.
                // So, if even input surface is BF, we set TFF for output and vise versa. For odd input surface - no matter what we set.
                if (DecExtSurface.pSurface)
                {
                    if ((DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF))  // Incoming Top Field in a single surface
                    {
                        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_BFF;
                    }
                    if (DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF)    // Incoming Bottom Field in a single surface
                    {
                        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
                    }
                }
                sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
            }
            else
            {
                if (m_bIsFieldSplitting)
                {
                    if (DecExtSurface.pSurface)
                    {
                        if (DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF || DecExtSurface.pSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF)
                        {
                            m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_SINGLE;
                            sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                        }
                        else
                        {
                            VppExtSurface.pSurface = DecExtSurface.pSurface;
                            VppExtSurface.pAuxCtrl = DecExtSurface.pAuxCtrl;
                            VppExtSurface.Syncp = DecExtSurface.Syncp;
                        }
                    }
                    else
                    {
                        sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                    }
                }
                else
                {
                    sts = VPPOneFrame(&DecExtSurface, &VppExtSurface);
                }
            }
            // check for interlaced stream
        }
        else // no VPP - just copy pointers
        {
            VppExtSurface.pSurface = DecExtSurface.pSurface;
            VppExtSurface.pAuxCtrl = DecExtSurface.pAuxCtrl;
            VppExtSurface.Syncp = DecExtSurface.Syncp;
        }

        if(MFX_ERR_MORE_SURFACE == sts)
        {
            shouldReadNextFrame=false;
            sts=MFX_ERR_NONE;
        }
        else
        {
            shouldReadNextFrame=true;
        }

        if (sts == MFX_ERR_MORE_DATA)
        {
            sts = MFX_ERR_NONE;
            if (NULL == DecExtSurface.pSurface) // there are no more buffered frames in VPP
            {
                VppExtSurface.pSurface = NULL; // to get buffered ENC frames
            }
            else
            {
                continue; // go get next frame from Decode
            }
        }

        MSDK_CHECK_STATUS(sts, "Unexpected error!!");

        // encode frame
        pBS = m_pBSStore->GetNext();
        if (!pBS)
            return MFX_ERR_NOT_FOUND;

        m_BSPool.push_back(pBS);

        // Set Encoding control if it is required.

        SetEncCtrlRT(VppExtSurface, m_bInsertIDR);
        m_bInsertIDR = false;

        if (bNeedDecodedFrames)
            m_nProcessedFramesNum++;

        if(m_mfxEncParams.mfx.CodecId != MFX_CODEC_DUMP)
        {
            sts = EncodeOneFrame(&VppExtSurface, &m_BSPool.back()->Bitstream);
        }
        else
        {

            sts = Surface2BS(&VppExtSurface, &m_BSPool.back()->Bitstream, m_encoderFourCC);
        }

        // check if we need one more frame from decode
        if (MFX_ERR_MORE_DATA == sts)
        {
            // the task in not in Encode queue
            m_BSPool.pop_back();
            m_pBSStore->Release(pBS);

            if (NULL == VppExtSurface.pSurface) // there are no more buffered frames in encoder
            {
                break;
            }
            sts = MFX_ERR_NONE;
            continue;
        }

        // check encoding result
        MSDK_CHECK_STATUS(sts, "<EncodeOneFrame|Surface2BS> failed");

        if(statisticsWindowSize)
        {
        if ( (statisticsWindowSize && m_nOutputFramesNum && 0 == m_nProcessedFramesNum % statisticsWindowSize) ||
             (statisticsWindowSize && (m_nProcessedFramesNum >= m_MaxFramesForTranscode)))
            {
                inputStatistics.PrintStatistics(GetPipelineID());
                outputStatistics.PrintStatistics(
                    GetPipelineID(),
                    (m_mfxEncParams.mfx.FrameInfo.FrameRateExtD)?
                    (mfxF64)m_mfxEncParams.mfx.FrameInfo.FrameRateExtN/(mfxF64)m_mfxEncParams.mfx.FrameInfo.FrameRateExtD: -1);
                inputStatistics.ResetStatistics();
                outputStatistics.ResetStatistics();
            }
        }
        else if (0 == (m_nProcessedFramesNum - 1) % 100)
        {
            msdk_printf(MSDK_STRING("."));
        }

        m_BSPool.back()->Syncp = VppExtSurface.Syncp;

        if (m_BSPool.size() == m_AsyncDepth)
        {
            sts = PutBS();
            MSDK_CHECK_STATUS(sts, "PutBS failed");
        }

        msdk_tick nFrameTime = msdk_time_get_tick() - nBeginTime;
        if (nFrameTime < m_nReqFrameTime)
        {
            MSDK_USLEEP((mfxU32)(m_nReqFrameTime - nFrameTime));
        }
    }
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    // need to get buffered bitstream
    if (MFX_ERR_NONE == sts)
    {
        while(m_BSPool.size())
        {
            sts = PutBS();
            MSDK_CHECK_STATUS(sts, "PutBS failed");
        }
    }

    if (MFX_ERR_NONE == sts)
        sts = MFX_WRN_VALUE_NOT_CHANGED;

    return sts;
} // mfxStatus CTranscodingPipeline::Transcode()

mfxStatus CTranscodingPipeline::PutBS()
{
    mfxStatus       sts = MFX_ERR_NONE;
    ExtendedBS *pBitstreamEx  = m_BSPool.front();
    // get result coded stream, synchronize only if we still have sync point
    if(pBitstreamEx->Syncp)
    {
        sts = m_pmfxSession->SyncOperation(pBitstreamEx->Syncp, MSDK_WAIT_INTERVAL);
        HandlePossibleGpuHang(sts);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
    }

    m_nOutputFramesNum++;

    //--- Time measurements
    if (statisticsWindowSize)
    {
        outputStatistics.StopTimeMeasurementWithCheck();
        outputStatistics.StartTimeMeasurement();
    }

    sts = m_pBSProcessor->ProcessOutputBitstream(&pBitstreamEx->Bitstream);
    MSDK_CHECK_STATUS(sts, "m_pBSProcessor->ProcessOutputBitstream failed");

    UnPreEncAuxBuffer(pBitstreamEx->pCtrl);

    pBitstreamEx->Bitstream.DataLength = 0;
    pBitstreamEx->Bitstream.DataOffset = 0;

    m_BSPool.pop_front();
    m_pBSStore->Release(pBitstreamEx);

    return sts;
} //mfxStatus CTranscodingPipeline::PutBS()

mfxStatus CTranscodingPipeline::DumpSurface2File(mfxFrameSurface1* pSurf)
{
    mfxStatus       sts = MFX_ERR_NONE;
    sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis,pSurf->Data.MemId,&pSurf->Data);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");

    sts =  m_dumpVppCompFileWriter.WriteNextFrame(pSurf);
    MSDK_CHECK_STATUS(sts, "m_dumpVppCompFileWriter.WriteNextFrame failed");

    sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis,pSurf->Data.MemId,&pSurf->Data);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Unlock failed");

    return sts;
} // mfxStatus CTranscodingPipeline::DumpSurface2File(ExtendedSurface* pSurf)


mfxStatus CTranscodingPipeline::Surface2BS(ExtendedSurface* pSurf,mfxBitstream* pBS, mfxU32 fourCC)
{
    mfxStatus       sts = MFX_ERR_MORE_DATA;
    // get result coded stream
    if (!pSurf->pSurface)
    {
        return MFX_ERR_MORE_DATA;
    }

    if(pSurf->Syncp)
    {
        sts = m_pmfxSession->SyncOperation(pSurf->Syncp, MSDK_WAIT_INTERVAL);
        HandlePossibleGpuHang(sts);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SyncOperation failed");
        pSurf->Syncp=0;

        //--- Copying data from surface to bitstream
        sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis,pSurf->pSurface->Data.MemId,&pSurf->pSurface->Data);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");

        switch(fourCC)
        {
        case 0: // Default value is MFX_FOURCC_I420
        case MFX_FOURCC_I420:
            sts = NV12asI420toBS(pSurf->pSurface, pBS);
            break;
        case MFX_FOURCC_NV12:
            sts=NV12toBS(pSurf->pSurface,pBS);
            break;
        case MFX_FOURCC_RGB4:
            sts=RGB4toBS(pSurf->pSurface,pBS);
            break;
        case MFX_FOURCC_YUY2:
            sts=YUY2toBS(pSurf->pSurface,pBS);
            break;
        }
        MSDK_CHECK_STATUS(sts, "<FourCC>toBS failed");

        sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis,pSurf->pSurface->Data.MemId,&pSurf->pSurface->Data);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Unlock failed");
    }

    return sts;
}

mfxStatus CTranscodingPipeline::NV12asI420toBS(mfxFrameSurface1* pSurface, mfxBitstream* pBS)
{
    mfxFrameInfo& info = pSurface->Info;
    mfxFrameData& data = pSurface->Data;
    if ((int)pBS->MaxLength - (int)pBS->DataLength < (int)(info.CropH*info.CropW * 3 / 2))
    {
        mfxStatus sts = ExtendMfxBitstream(pBS, pBS->DataLength + (int)(info.CropH*info.CropW * 3 / 2));
        MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
    }

    for (mfxU16 i = 0; i < info.CropH; i++)
    {
        MSDK_MEMCPY(pBS->Data + pBS->DataLength, data.Y + (info.CropY * data.Pitch + info.CropX) + i * data.Pitch, info.CropW);
        pBS->DataLength += info.CropW;
    }

    mfxU16 h = info.CropH / 2;
    mfxU16 w = info.CropW;

    for (mfxU16 offset = 0; offset<2; offset++)
    {
        for (mfxU16 i = 0; i < h; i++)
        {
            for (mfxU16 j = offset; j < w; j += 2)
            {
                pBS->Data[pBS->DataLength] = *(data.UV + (info.CropY * data.Pitch / 2 + info.CropX) + i * data.Pitch + j);
                pBS->DataLength++;
            }
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::NV12toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS)
{
    mfxFrameInfo& info = pSurface->Info;
    mfxFrameData& data = pSurface->Data;
    if((int)pBS->MaxLength-(int)pBS->DataLength < (int)(info.CropH*info.CropW*3/2))
    {
        mfxStatus sts = ExtendMfxBitstream(pBS, pBS->DataLength+(int)(info.CropH*info.CropW*3/2));
        MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
    }

    for (mfxU16 i = 0; i < info.CropH; i++)
    {
        MSDK_MEMCPY(pBS->Data+pBS->DataLength, data.Y + (info.CropY * data.Pitch + info.CropX)+ i * data.Pitch, info.CropW);
        pBS->DataLength += info.CropW;
    }

    for(mfxU16 i = 0; i < info.CropH / 2;i++)
    {
        MSDK_MEMCPY(pBS->Data + pBS->DataLength, data.UV + (info.CropY * data.Pitch + info.CropX) + i * data.Pitch, info.CropW);
        pBS->DataLength += info.CropW;
    }

    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::RGB4toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS)
{
    mfxFrameInfo& info = pSurface->Info;
    mfxFrameData& data = pSurface->Data;
    if((int)pBS->MaxLength-(int)pBS->DataLength < (int)(info.CropH*info.CropW*4))
    {
        mfxStatus sts = ExtendMfxBitstream(pBS, pBS->DataLength+(int)(info.CropH*info.CropW*4));
        MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
    }

    for (mfxU16 i = 0; i < info.CropH; i++)
    {
        MSDK_MEMCPY(pBS->Data+pBS->DataLength, data.B + (info.CropY * data.Pitch + info.CropX*4)+ i * data.Pitch, info.CropW*4);
        pBS->DataLength += info.CropW*4;
    }

    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::YUY2toBS(mfxFrameSurface1* pSurface,mfxBitstream* pBS)
{
    mfxFrameInfo& info = pSurface->Info;
    mfxFrameData& data = pSurface->Data;
    if((int)pBS->MaxLength-(int)pBS->DataLength < (int)(info.CropH*info.CropW*4))
    {
        mfxStatus sts = ExtendMfxBitstream(pBS, pBS->DataLength+(int)(info.CropH*info.CropW*4));
        MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
    }

    for (mfxU16 i = 0; i < info.CropH; i++)
    {
        MSDK_MEMCPY(pBS->Data+pBS->DataLength, data.Y + (info.CropY * data.Pitch + info.CropX/2*4)+ i * data.Pitch, info.CropW*2);
        pBS->DataLength += info.CropW*2;
    }

    return MFX_ERR_NONE;
}


mfxStatus CTranscodingPipeline::AllocMVCSeqDesc()
{
    mfxU32 i;

    m_MVCSeqDesc.View = new mfxMVCViewDependency[m_MVCSeqDesc.NumView];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.View, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumView; ++i)
    {
        MSDK_ZERO_MEMORY(m_MVCSeqDesc.View[i]);
    }
    m_MVCSeqDesc.NumViewAlloc = m_MVCSeqDesc.NumView;

    m_MVCSeqDesc.ViewId = new mfxU16[m_MVCSeqDesc.NumViewId];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.ViewId, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumViewId; ++i)
    {
        MSDK_ZERO_MEMORY(m_MVCSeqDesc.ViewId[i]);
    }
    m_MVCSeqDesc.NumViewIdAlloc = m_MVCSeqDesc.NumViewId;

    m_MVCSeqDesc.OP = new mfxMVCOperationPoint[m_MVCSeqDesc.NumOP];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.OP, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumOP; ++i)
    {
        MSDK_ZERO_MEMORY(m_MVCSeqDesc.OP[i]);
    }
    m_MVCSeqDesc.NumOPAlloc = m_MVCSeqDesc.NumOP;

    return MFX_ERR_NONE;
}

void CTranscodingPipeline::FreeMVCSeqDesc()
{
    if (m_bOwnMVCSeqDescMemory)
    {
        MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.View);
        MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.ViewId);
        MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.OP);
    }
}

mfxStatus CTranscodingPipeline::InitDecMfxParams(sInputParams *pInParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    MSDK_CHECK_POINTER(pInParams, MFX_ERR_NULL_PTR);

    m_mfxDecParams.AsyncDepth = m_AsyncDepth;

    // configure and attach external parameters
    if (m_bUseOpaqueMemory)
        m_DecExtParams.push_back((mfxExtBuffer *)&m_DecOpaqueAlloc);

    if (pInParams->bIsMVC)
        m_DecExtParams.push_back((mfxExtBuffer *)&m_MVCSeqDesc);

    if (!m_DecExtParams.empty())
    {
        m_mfxDecParams.ExtParam = &m_DecExtParams[0]; // vector is stored linearly in memory
        m_mfxDecParams.NumExtParam = (mfxU16)m_DecExtParams.size();
    }

    if (!m_bUseOverlay)
    {

        // read a portion of data for DecodeHeader function
        sts = m_pBSProcessor->GetInputBitstream(&m_pmfxBS);
        if (MFX_ERR_MORE_DATA == sts)
            return sts;
        else
            MSDK_CHECK_STATUS(sts, "m_pBSProcessor->GetInputBitstream failed");

        // try to find a sequence header in the stream
        // if header is not found this function exits with error (e.g. if device was lost and there's no header in the remaining stream)
        for (;;)
        {
            // trying to find PicStruct information in AVI headers
            if (pInParams->DecodeId == MFX_CODEC_JPEG)
                MJPEG_AVI_ParsePicStruct(m_pmfxBS);

            // parse bit stream and fill mfx params
            sts = m_pmfxDEC->DecodeHeader(m_pmfxBS, &m_mfxDecParams);

            if (MFX_ERR_MORE_DATA == sts)
            {
                if (m_pmfxBS->MaxLength == m_pmfxBS->DataLength)
                {
                    sts = ExtendMfxBitstream(m_pmfxBS, m_pmfxBS->MaxLength * 2);
                    MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
                }

                // read a portion of data for DecodeHeader function
                sts = m_pBSProcessor->GetInputBitstream(&m_pmfxBS);
                if (MFX_ERR_MORE_DATA == sts)
                    return sts;
                else
                    MSDK_CHECK_STATUS(sts, "m_pBSProcessor->GetInputBitstream failed");


                continue;
            }
            else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts && pInParams->bIsMVC)
            {
                sts = AllocMVCSeqDesc();
                MSDK_CHECK_STATUS(sts, "m_pmfxDEC->DecodeHeader failed");

                continue;
            }
            else
                break;
        }

        // to enable decorative flags, has effect with 1.3 API libraries only
        // (in case of JPEG decoder - it is not valid to use this field)
        if (m_mfxDecParams.mfx.CodecId != MFX_CODEC_JPEG)
            m_mfxDecParams.mfx.ExtendedPicStruct = 1;

        // check DecodeHeader status
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC->DecodeHeader failed");

        // if input is interlaced JPEG stream
        if (((pInParams->DecodeId == MFX_CODEC_JPEG) && (m_pmfxBS->PicStruct == MFX_PICSTRUCT_FIELD_TFF))
            || (m_pmfxBS->PicStruct == MFX_PICSTRUCT_FIELD_BFF))
        {
            m_mfxDecParams.mfx.FrameInfo.CropH *= 2;
            m_mfxDecParams.mfx.FrameInfo.Height = MSDK_ALIGN16(m_mfxDecParams.mfx.FrameInfo.CropH);
            m_mfxDecParams.mfx.FrameInfo.PicStruct = m_pmfxBS->PicStruct;
        }
    }
    else
    {
        m_mfxDecParams.mfx.FrameInfo.Width = MSDK_ALIGN32(pInParams->nVppCompSrcW);
        m_mfxDecParams.mfx.FrameInfo.Height = MSDK_ALIGN32(pInParams->nVppCompSrcH);
        m_mfxDecParams.mfx.FrameInfo.CropW = pInParams->nVppCompSrcW;
        m_mfxDecParams.mfx.FrameInfo.CropH = pInParams->nVppCompSrcH;

        m_mfxDecParams.mfx.FrameInfo.AspectRatioW = m_mfxDecParams.mfx.FrameInfo.AspectRatioH = 1;
        m_mfxDecParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        m_mfxDecParams.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
    }

    // set memory pattern
    if (m_bUseOpaqueMemory)
        m_mfxDecParams.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    else if (pInParams->bForceSysMem || (MFX_IMPL_SOFTWARE == pInParams->libType))
        m_mfxDecParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    else
        m_mfxDecParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    // if frame rate specified by user set it for decoder output
    if (pInParams->dDecoderFrameRateOverride)
    {
        ConvertFrameRate(pInParams->dDecoderFrameRateOverride, &m_mfxDecParams.mfx.FrameInfo.FrameRateExtN, &m_mfxDecParams.mfx.FrameInfo.FrameRateExtD);
    }
    // if frame rate not specified and input stream header doesn't contain valid values use default (30.0)
    else if (0 == (m_mfxDecParams.mfx.FrameInfo.FrameRateExtN * m_mfxDecParams.mfx.FrameInfo.FrameRateExtD))
    {
        m_mfxDecParams.mfx.FrameInfo.FrameRateExtN = 30;
        m_mfxDecParams.mfx.FrameInfo.FrameRateExtD = 1;
    }
    else
    {
        // use the value from input stream header
    }

    //--- Force setting fourcc type if required
    if(pInParams->DecoderFourCC)
    {
        m_mfxDecParams.mfx.FrameInfo.FourCC=pInParams->DecoderFourCC;
        m_mfxDecParams.mfx.FrameInfo.ChromaFormat=FourCCToChroma(pInParams->DecoderFourCC);
    }
#if MFX_VERSION >= 1022
    /* SFC usage if enabled */
    if ((pInParams->bDecoderPostProcessing) &&
        (MFX_CODEC_AVC == m_mfxDecParams.mfx.CodecId) && /* Only for AVC */
        (MFX_PICSTRUCT_PROGRESSIVE == m_mfxDecParams.mfx.FrameInfo.PicStruct)) /* ...And only for progressive!*/
    {
        m_decPostProcessing.In.CropX = 0;
        m_decPostProcessing.In.CropY = 0;
        m_decPostProcessing.In.CropW = m_mfxDecParams.mfx.FrameInfo.CropW;
        m_decPostProcessing.In.CropH = m_mfxDecParams.mfx.FrameInfo.CropH;

        m_decPostProcessing.Out.FourCC = m_mfxDecParams.mfx.FrameInfo.FourCC;
        m_decPostProcessing.Out.ChromaFormat = m_mfxDecParams.mfx.FrameInfo.ChromaFormat;
        m_decPostProcessing.Out.Width = MSDK_ALIGN16(pInParams->nVppCompDstW);
        m_decPostProcessing.Out.Height = MSDK_ALIGN16(pInParams->nVppCompDstH);
        m_decPostProcessing.Out.CropX = 0;
        m_decPostProcessing.Out.CropY = 0;
        m_decPostProcessing.Out.CropW = pInParams->nVppCompDstW;
        m_decPostProcessing.Out.CropH = pInParams->nVppCompDstH;

        m_DecExtParams.push_back((mfxExtBuffer *)&m_decPostProcessing);
        m_mfxDecParams.ExtParam = &m_DecExtParams[0]; // vector is stored linearly in memory
        m_mfxDecParams.NumExtParam = (mfxU16)m_DecExtParams.size();
    }
#endif //MFX_VERSION >= 1022
    return MFX_ERR_NONE;
}// mfxStatus CTranscodingPipeline::InitDecMfxParams()

mfxStatus CTranscodingPipeline::InitEncMfxParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);
    m_mfxEncParams.mfx.CodecId                 = pInParams->EncodeId;
    m_mfxEncParams.mfx.TargetUsage             = pInParams->nTargetUsage; // trade-off between quality and speed
    m_mfxEncParams.AsyncDepth                  = m_AsyncDepth;
#if (MFX_VERSION >= 1025)
    if(pInParams->numMFEFrames || pInParams->MFMode)
    {
        m_ExtMFEParam.MaxNumFrames = pInParams->numMFEFrames;
        m_ExtMFEParam.MFMode = pInParams->MFMode;

        m_EncExtParams.push_back((mfxExtBuffer*)&m_ExtMFEParam);
    }

    if(pInParams->mfeTimeout)
    {
        m_ExtMFEControl.Timeout = pInParams->mfeTimeout;
        m_EncExtParams.push_back((mfxExtBuffer*)&m_ExtMFEControl);
    }
#endif
    if (m_pParentPipeline && m_pParentPipeline->m_pmfxPreENC.get())
    {
        m_mfxEncParams.mfx.RateControlMethod       = MFX_RATECONTROL_LA_EXT;
        m_mfxEncParams.mfx.EncodedOrder            = 1; // binary flag, 0 signals encoder to take frames in display order
        m_mfxEncParams.AsyncDepth = m_mfxEncParams.AsyncDepth == 0 ? 2: m_mfxEncParams.AsyncDepth;
    }
    else
    {
        m_mfxEncParams.mfx.RateControlMethod   = pInParams->nRateControlMethod;
    }
    m_mfxEncParams.mfx.NumSlice                = pInParams->nSlices;

    if (pInParams->nRateControlMethod == MFX_RATECONTROL_CQP)
    {
        m_mfxEncParams.mfx.QPI = pInParams->nQPI;
        m_mfxEncParams.mfx.QPP = pInParams->nQPP;
        m_mfxEncParams.mfx.QPB = pInParams->nQPB;
    }

    if(pInParams->enableQSVFF)
    {
        m_mfxEncParams.mfx.LowPower = MFX_CODINGOPTION_ON;
    }

    if (m_bIsVpp)
    {
        MSDK_MEMCPY_VAR(m_mfxEncParams.mfx.FrameInfo, &m_mfxVppParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else if (m_bIsPlugin)
    {
        MSDK_MEMCPY_VAR(m_mfxEncParams.mfx.FrameInfo, &m_mfxPluginParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        MSDK_MEMCPY_VAR(m_mfxEncParams.mfx.FrameInfo, &m_mfxDecParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
    }

    // leave PAR unset to avoid MPEG2 encoder rejecting streams with unsupported DAR
    m_mfxEncParams.mfx.FrameInfo.AspectRatioW = m_mfxEncParams.mfx.FrameInfo.AspectRatioH = 0;

    // calculate default bitrate based on resolution and framerate

    if (pInParams->dEncoderFrameRateOverride)
    {
        ConvertFrameRate(pInParams->dEncoderFrameRateOverride, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtN, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtD);
    }
    else if(pInParams->dVPPOutFramerate)
    {
        ConvertFrameRate(pInParams->dVPPOutFramerate, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtN, &m_mfxEncParams.mfx.FrameInfo.FrameRateExtD);
    }

    MSDK_CHECK_ERROR(m_mfxEncParams.mfx.FrameInfo.FrameRateExtN * m_mfxEncParams.mfx.FrameInfo.FrameRateExtD,
        0, MFX_ERR_INVALID_VIDEO_PARAM);

    if (pInParams->nRateControlMethod != MFX_RATECONTROL_CQP)
    {
        if (pInParams->nBitRate == 0)
        {
            pInParams->nBitRate = CalculateDefaultBitrate(pInParams->EncodeId,
                pInParams->nTargetUsage, m_mfxEncParams.mfx.FrameInfo.Width, m_mfxEncParams.mfx.FrameInfo.Height,
                1.0 * m_mfxEncParams.mfx.FrameInfo.FrameRateExtN / m_mfxEncParams.mfx.FrameInfo.FrameRateExtD);
        }
        m_mfxEncParams.mfx.TargetKbps = pInParams->nBitRate; // in Kbps
        m_mfxEncParams.mfx.BRCParamMultiplier = pInParams->nBitRateMultiplier;
    }

    // In case of HEVC when height and/or width divided with 8 but not divided with 16
    // add extended parameter to increase performance
    if ( ( !((m_mfxEncParams.mfx.FrameInfo.CropW & 15 ) ^ 8 ) ||
           !((m_mfxEncParams.mfx.FrameInfo.CropH & 15 ) ^ 8 ) ) &&
             (m_mfxEncParams.mfx.CodecId == MFX_CODEC_HEVC) )
    {
        m_ExtHEVCParam.PicWidthInLumaSamples = m_mfxEncParams.mfx.FrameInfo.CropW;
        m_ExtHEVCParam.PicHeightInLumaSamples = m_mfxEncParams.mfx.FrameInfo.CropH;
        m_EncExtParams.push_back((mfxExtBuffer*)&m_ExtHEVCParam);
    }
#if (MFX_VERSION >= 1024)
    // This is for explicit extbrc only. In case of implicit (built-into-library) version - we don't need this extended buffer
    if (pInParams->nExtBRC == EXTBRC_ON &&
        (pInParams->EncodeId == MFX_CODEC_HEVC || pInParams->EncodeId == MFX_CODEC_AVC))
    {
        HEVCExtBRC::Create(m_ExtBRC);
        m_EncExtParams.push_back((mfxExtBuffer *)&m_ExtBRC);
    }
#endif

    m_mfxEncParams.mfx.FrameInfo.CropX = 0;
    m_mfxEncParams.mfx.FrameInfo.CropY = 0;

    mfxU16 InPatternFromParent = (mfxU16) ((MFX_IOPATTERN_OUT_VIDEO_MEMORY == m_mfxDecParams.IOPattern) ?
MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    // set memory pattern
    if (m_bUseOpaqueMemory)
        m_mfxEncParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
    else
        m_mfxEncParams.IOPattern = InPatternFromParent;

    // we don't specify profile and level and let the encoder choose those basing on parameters
    // we must specify profile only for MVC codec
    if (pInParams->bIsMVC)
    {
        m_mfxEncParams.mfx.CodecProfile = m_mfxDecParams.mfx.CodecProfile;
    }

    // JPEG encoder settings overlap nasc other encoders settings in mfxInfoMFX structure
    if (MFX_CODEC_JPEG == pInParams->EncodeId)
    {
        m_mfxEncParams.mfx.Interleaved = 1;
        m_mfxEncParams.mfx.Quality = pInParams->nQuality;
        m_mfxEncParams.mfx.RestartInterval = 0;
        MSDK_ZERO_MEMORY(m_mfxEncParams.mfx.reserved5);
    }

    // configure and attach external parameters
    if (pInParams->bLABRC || pInParams->nMaxSliceSize || pInParams->nBRefType
        || (pInParams->nExtBRC && (pInParams->EncodeId == MFX_CODEC_HEVC || pInParams->EncodeId == MFX_CODEC_AVC)))
    {
        m_CodingOption2.LookAheadDepth = pInParams->nLADepth;
        m_CodingOption2.MaxSliceSize = pInParams->nMaxSliceSize;
        m_CodingOption2.BRefType = pInParams->nBRefType;

        if (pInParams->nExtBRC != EXTBRC_DEFAULT && (pInParams->EncodeId == MFX_CODEC_HEVC || pInParams->EncodeId == MFX_CODEC_AVC))
        {
            m_CodingOption2.ExtBRC =(mfxU16)(pInParams->nExtBRC == EXTBRC_OFF ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON);
        }
        else
        {
            m_CodingOption2.ExtBRC = 0;
        }

        m_EncExtParams.push_back((mfxExtBuffer *)&m_CodingOption2);
    }

    bool addCodingOpt3 = false;

    if (pInParams->WinBRCMaxAvgKbps || pInParams->WinBRCSize)
    {
        m_CodingOption3.WinBRCMaxAvgKbps = pInParams->WinBRCMaxAvgKbps;
        m_CodingOption3.WinBRCSize = pInParams->WinBRCSize;
        addCodingOpt3 = true;
    }
#if MFX_VERSION >= 1022
    if (pInParams->bROIasQPMAP || pInParams->bExtMBQP)
    {
        switch(m_mfxEncParams.mfx.CodecId)
        {
        case MFX_CODEC_AVC:
        case MFX_CODEC_HEVC:
            // For AVC codec QP map should define QP value for every 16x16 sub-block of a frame
            m_QPmapWidth = (m_mfxEncParams.mfx.FrameInfo.Width + 15) >> 4;
            m_QPmapHeight = (m_mfxEncParams.mfx.FrameInfo.Height + 15) >> 4;
            m_CodingOption3.EnableMBQP = MFX_CODINGOPTION_ON;
            addCodingOpt3 = true;
            break;
        default:
            m_QPmapWidth = 0;
            m_QPmapHeight = 0;
            break;
        }
    }
#endif

    if (pInParams->WeightedPred || pInParams->WeightedBiPred)
    {
        m_CodingOption3.WeightedPred = pInParams->WeightedPred;
        m_CodingOption3.WeightedBiPred = pInParams->WeightedBiPred;
        addCodingOpt3 = true;
    }
#if (MFX_VERSION >= 1026)
    if (pInParams->ExtBrcAdaptiveLTR)
    {
        m_CodingOption3.ExtBrcAdaptiveLTR = pInParams->ExtBrcAdaptiveLTR;
        addCodingOpt3 = true;
    }
#endif
#if MFX_VERSION >= 1023
    if (pInParams->RepartitionCheckMode)
    {
        m_CodingOption3.RepartitionCheckEnable = pInParams->RepartitionCheckMode;
        addCodingOpt3 = true;
    }
#endif

    if (pInParams->bDisableQPOffset)
    {
        m_CodingOption3.EnableQPOffset = MFX_CODINGOPTION_OFF;
        addCodingOpt3 = true;
    }

    if (addCodingOpt3)
        m_EncExtParams.push_back((mfxExtBuffer *)&m_CodingOption3);

    if (m_bUseOpaqueMemory)
        m_EncExtParams.push_back((mfxExtBuffer *)&m_EncOpaqueAlloc);

    if (pInParams->bIsMVC)
        m_EncExtParams.push_back((mfxExtBuffer *)&m_MVCSeqDesc);

    if (!m_EncExtParams.empty())
    {
        m_mfxEncParams.ExtParam = &m_EncExtParams[0]; // vector is stored linearly in memory
        m_mfxEncParams.NumExtParam = (mfxU16)m_EncExtParams.size();
    }
    if (m_pParentPipeline)
    {
        m_pParentPipeline->AddLaStreams(m_mfxEncParams.mfx.FrameInfo.Width,m_mfxEncParams.mfx.FrameInfo.Height);
    }

    //--- Settings HRD buffer size
    if (pInParams->BufferSizeInKB)
    {
        m_mfxEncParams.mfx.BufferSizeInKB = pInParams->BufferSizeInKB;
    }

    //--- Force setting fourcc type if required
    if (pInParams->EncoderFourCC)
    {
        m_mfxEncParams.mfx.FrameInfo.FourCC=pInParams->EncoderFourCC;
        m_mfxEncParams.mfx.FrameInfo.ChromaFormat=FourCCToChroma(pInParams->EncoderFourCC);
    }

    if (pInParams->GopPicSize)
    {
        m_mfxEncParams.mfx.GopPicSize = pInParams->GopPicSize;
    }

    if (pInParams->GopRefDist)
    {
        m_mfxEncParams.mfx.GopRefDist = pInParams->GopRefDist;
    }

    if (pInParams->NumRefFrame)
    {
        m_mfxEncParams.mfx.NumRefFrame = pInParams->NumRefFrame;
    }

    if (pInParams->CodecLevel)
    {
        m_mfxEncParams.mfx.CodecLevel = pInParams->CodecLevel;
    }

    if (pInParams->CodecProfile)
    {
        m_mfxEncParams.mfx.CodecProfile = pInParams->CodecProfile;
    }

    if (pInParams->GopOptFlag)
    {
        m_mfxEncParams.mfx.GopOptFlag = pInParams->GopOptFlag;
    }

    if (pInParams->MaxKbps)
    {
        m_mfxEncParams.mfx.MaxKbps = pInParams->MaxKbps;
    }

    if (pInParams->InitialDelayInKB)
    {
        m_mfxEncParams.mfx.InitialDelayInKB = pInParams->InitialDelayInKB;
    }

    return MFX_ERR_NONE;
}// mfxStatus CTranscodingPipeline::InitEncMfxParams(sInputParams *pInParams)

mfxStatus CTranscodingPipeline::CorrectPreEncAuxPool(mfxU32 num_frames_in_pool)
{
    if (!m_pmfxPreENC.get()) return MFX_ERR_NONE;

    if (m_pPreEncAuxPool.size() < num_frames_in_pool)
    {
        m_pPreEncAuxPool.resize(num_frames_in_pool);
    }
    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::AllocPreEncAuxPool()
{
    if (!m_pmfxPreENC.get()) return MFX_ERR_NONE;

    mfxU16 num_resolutions = m_ExtLAControl.NumOutStream;
    int buff_size = sizeof(mfxExtLAFrameStatistics) +
        sizeof(mfxLAFrameInfo)*num_resolutions*m_ExtLAControl.LookAheadDepth;

    for (size_t i = 0; i < m_pPreEncAuxPool.size(); i++)
    {
        memset(&m_pPreEncAuxPool[i],0,sizeof(m_pPreEncAuxPool[i]));

        m_pPreEncAuxPool[i].encCtrl.NumExtParam = 1;
        m_pPreEncAuxPool[i].encCtrl.ExtParam = new mfxExtBuffer*[1];

        char *pBuff = new char[buff_size];
        memset(pBuff,0, buff_size);

        m_pPreEncAuxPool[i].encCtrl.ExtParam[0] = (mfxExtBuffer *)pBuff;
        mfxExtLAFrameStatistics *pExtBuffer = (mfxExtLAFrameStatistics *)pBuff;

        pExtBuffer = (mfxExtLAFrameStatistics*) pBuff;
        pExtBuffer->Header.BufferId = MFX_EXTBUFF_LOOKAHEAD_STAT;
        pExtBuffer->Header.BufferSz = buff_size;
        pExtBuffer->NumAlloc = num_resolutions*m_ExtLAControl.LookAheadDepth;
        pExtBuffer->FrameStat = (mfxLAFrameInfo *)(pBuff + sizeof(mfxExtLAFrameStatistics));

        m_pPreEncAuxPool[i].encOutput.NumExtParam = 1;
        m_pPreEncAuxPool[i].encOutput.ExtParam = m_pPreEncAuxPool[i].encCtrl.ExtParam;
    }
    return MFX_ERR_NONE;
}

void CTranscodingPipeline::FreePreEncAuxPool()
{
     for (size_t i = 0; i < m_pPreEncAuxPool.size(); i++)
     {
         if(m_pPreEncAuxPool[i].encCtrl.ExtParam)
         {
             delete [] m_pPreEncAuxPool[i].encCtrl.ExtParam[0];
             delete [] m_pPreEncAuxPool[i].encCtrl.ExtParam;
         }
     }
     m_pPreEncAuxPool.resize(0);
}

mfxStatus TranscodingSample::CTranscodingPipeline::LoadStaticSurface()
{
    if (m_bUseOverlay)
    {
        mfxFrameSurface1* pSurf = m_pSurfaceDecPool[0];
        mfxStatus sts = m_pMFXAllocator->LockFrame(pSurf->Data.MemId, &pSurf->Data);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->LockFrame failed");
        sts = m_pBSProcessor->GetInputFrame(pSurf);
        MSDK_CHECK_STATUS(sts, "m_YUVReader->LoadNextFrame failed");
        sts = m_pMFXAllocator->UnlockFrame(pSurf->Data.MemId, &pSurf->Data);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->UnlockFrame failed");
    }
    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::InitPreEncMfxParams(sInputParams *pInParams)
{
    MSDK_CHECK_ERROR(pInParams->bEnableExtLA, false, MFX_ERR_INVALID_VIDEO_PARAM);
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    mfxVideoParam & param = m_mfxPreEncParams;

    param.AsyncDepth = m_AsyncDepth;

    MSDK_ZERO_MEMORY(param.mfx);
    param.mfx.CodecId= MFX_CODEC_AVC;
    param.mfx.TargetUsage= pInParams->nTargetUsage;

    if (m_bIsVpp)
    {
        MSDK_MEMCPY_VAR(param.mfx.FrameInfo, &m_mfxVppParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else if (m_bIsPlugin)
    {
        MSDK_MEMCPY_VAR(param.mfx.FrameInfo, &m_mfxPluginParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        MSDK_MEMCPY_VAR(param.mfx.FrameInfo, &m_mfxDecParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
    }

    mfxU16 InPatternFromParent = (mfxU16) ((MFX_IOPATTERN_OUT_VIDEO_MEMORY == m_mfxDecParams.IOPattern) ?
                                                        MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    // set memory pattern
    if (m_bUseOpaqueMemory)
        param.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
    else
        param.IOPattern = InPatternFromParent;

    // configure and attach external parameters
    if (m_bUseOpaqueMemory)
        m_PreEncExtParams.push_back((mfxExtBuffer *)&m_PreEncOpaqueAlloc);

    MSDK_ZERO_MEMORY(m_ExtLAControl);
    m_ExtLAControl.Header.BufferId = MFX_EXTBUFF_LOOKAHEAD_CTRL;
    m_ExtLAControl.Header.BufferSz = sizeof(m_ExtLAControl);
    m_ExtLAControl.LookAheadDepth = pInParams->nLADepth? pInParams->nLADepth : 40;
    m_ExtLAControl.NumOutStream = 0;
    m_ExtLAControl.BPyramid = (mfxU16)(pInParams->bEnableBPyramid ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);

    m_mfxPreEncParams.mfx.GopPicSize = pInParams->GopPicSize ? pInParams->GopPicSize : 1500;

    if (pInParams->GopRefDist)
    {
        m_mfxPreEncParams.mfx.GopRefDist = pInParams->GopRefDist;
    }

    if (pInParams->nTargetUsage)
    {
        m_mfxPreEncParams.mfx.TargetUsage = pInParams->nTargetUsage;
    }

    m_PreEncExtParams.push_back((mfxExtBuffer *)&m_ExtLAControl);

    if (!m_PreEncExtParams.empty())
    {
        m_mfxPreEncParams.ExtParam = &m_PreEncExtParams[0]; // vector is stored linearly in memory
        m_mfxPreEncParams.NumExtParam = (mfxU16)m_PreEncExtParams.size();
    }
    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::AddLaStreams(mfxU16 width, mfxU16 height)
{
    if (m_pmfxPreENC.get() != NULL)
    {
        mfxU32 num = m_ExtLAControl.NumOutStream;
        m_numEncoders ++;
        for (mfxU32 i = 0; i < num; i++)
        {
            if (m_ExtLAControl.OutStream[i].Width  == width && m_ExtLAControl.OutStream[i].Height == height)
                return MFX_ERR_NONE;
        }
        if ((sizeof(m_ExtLAControl.OutStream) / sizeof(m_ExtLAControl.OutStream[0])) < (num + 1))
            return MFX_ERR_UNSUPPORTED;

        m_ExtLAControl.OutStream[num].Width  = width;
        m_ExtLAControl.OutStream[num].Height = height;
        m_ExtLAControl.NumOutStream = (mfxU16)num + 1;
    }
    return MFX_ERR_NONE;
}

 mfxStatus CTranscodingPipeline::InitVppMfxParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);
    m_mfxVppParams.AsyncDepth = m_AsyncDepth;

    mfxU16 InPatternFromParent = (mfxU16)((MFX_IOPATTERN_OUT_VIDEO_MEMORY == m_mfxDecParams.IOPattern) ?
        MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    // set memory pattern
    if (pInParams->VppOutPattern)
    {
        m_mfxVppParams.IOPattern = (mfxU16)(InPatternFromParent | pInParams->VppOutPattern);
    }
    else if (m_bUseOpaqueMemory)
    {
        m_mfxVppParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY|MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    }
    else if (pInParams->bForceSysMem || (MFX_IMPL_SOFTWARE == pInParams->libType))
    {
        m_mfxVppParams.IOPattern = (mfxU16)(InPatternFromParent|MFX_IOPATTERN_OUT_SYSTEM_MEMORY);
    }
    else
    {
        m_mfxVppParams.IOPattern = (mfxU16)(InPatternFromParent|MFX_IOPATTERN_OUT_VIDEO_MEMORY);
    }

    // input frame info
    MSDK_MEMCPY_VAR(m_mfxVppParams.vpp.In, &m_mfxDecParams.mfx.FrameInfo, sizeof(mfxFrameInfo));

    // fill output frame info
    MSDK_MEMCPY_VAR(m_mfxVppParams.vpp.Out, &m_mfxVppParams.vpp.In, sizeof(mfxFrameInfo));


    if(m_bIsFieldWeaving)
    {
        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_UNKNOWN;
        m_mfxVppParams.vpp.Out.Height = m_mfxVppParams.vpp.In.Height << 1;
        m_mfxVppParams.vpp.Out.CropH = m_mfxVppParams.vpp.In.CropH << 1;
    }

    if(m_bIsFieldSplitting)
    {
        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_FIELD_SINGLE;
        m_mfxVppParams.vpp.Out.Height = m_mfxVppParams.vpp.In.Height >> 1;
        m_mfxVppParams.vpp.Out.CropH = m_mfxVppParams.vpp.In.CropH >> 1;
    }
    if (pInParams->bEnableDeinterlacing)
        m_mfxVppParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;


    // Resizing
    if (pInParams->nDstWidth)
    {
        m_mfxVppParams.vpp.Out.CropW = pInParams->nDstWidth;
        m_mfxVppParams.vpp.Out.Width     = MSDK_ALIGN16(pInParams->nDstWidth);
    }

    // Framerate conversion
    if(pInParams->dVPPOutFramerate)
    {
        ConvertFrameRate(pInParams->dVPPOutFramerate, &m_mfxVppParams.vpp.Out.FrameRateExtN, &m_mfxVppParams.vpp.Out.FrameRateExtD);
    }

    if (pInParams->nDstHeight)
    {
        m_mfxVppParams.vpp.Out.CropH = pInParams->nDstHeight;
        m_mfxVppParams.vpp.Out.Height    = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppParams.vpp.Out.PicStruct) ?
                                             MSDK_ALIGN16(pInParams->nDstHeight) : MSDK_ALIGN32(pInParams->nDstHeight);
    }


    if (pInParams->bEnableDeinterlacing)
    {
        // If stream were interlaced before then 32 bit alignment were applied.
        // Discard 32 bit alignment as progressive doesn't require it.
        m_mfxVppParams.vpp.Out.Height = MSDK_ALIGN16(m_mfxVppParams.vpp.Out.CropH);
        m_mfxVppParams.vpp.Out.Width  = MSDK_ALIGN16(m_mfxVppParams.vpp.Out.CropW);
    }


    // configure and attach external parameters
    mfxStatus sts = AllocAndInitVppDoNotUse(pInParams);
    MSDK_CHECK_STATUS(sts, "AllocAndInitVppDoNotUse failed");
    if(m_VppDoNotUse.NumAlg)
    {
        m_VppExtParamsStorage.ExtBuffers.push_back((mfxExtBuffer *)&m_VppDoNotUse);
    }

    //--- Setting output FourCC type (input type is taken from m_mfxDecParams)
    if (pInParams->EncoderFourCC)
    {
        m_mfxVppParams.vpp.Out.FourCC = pInParams->EncoderFourCC;
        m_mfxVppParams.vpp.Out.ChromaFormat = FourCCToChroma(pInParams->EncoderFourCC);
    }

    /* VPP Comp Init */
    if (((pInParams->eModeExt == VppComp) || (pInParams->eModeExt == VppCompOnly)) &&
        (pInParams->numSurf4Comp != 0))
    {
        if(m_nVPPCompEnable != VppCompOnlyEncode)
            m_nVPPCompEnable = pInParams->eModeExt;
        m_VppCompParams.Header.BufferId = MFX_EXTBUFF_VPP_COMPOSITE;
        m_VppCompParams.Header.BufferSz = sizeof(mfxExtVPPComposite);
        m_VppCompParams.NumInputStream = (mfxU16)pInParams->numSurf4Comp;
        m_VppCompParams.InputStream = (mfxVPPCompInputStream *)malloc(sizeof(mfxVPPCompInputStream)*
            m_VppCompParams.NumInputStream);
        MSDK_CHECK_POINTER(m_VppCompParams.InputStream,MFX_ERR_NULL_PTR);

        // stream params
        /* if input streams in NV12 format background color should be in YUV format too
        * The same for RGB4 input, background color should be in ARGB format
        * */

        switch(pInParams->EncoderFourCC)
        {
        case MFX_FOURCC_RGB4:
            /* back color in RGB */
            m_VppCompParams.R = 0x00;
            m_VppCompParams.G = 0x00;
            m_VppCompParams.B = 0x00;
            break;
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_P010:
        case MFX_FOURCC_NV16:
        case MFX_FOURCC_P210:
        case MFX_FOURCC_YUY2:
        default:
            /* back color in YUV */
            m_VppCompParams.Y = 0x10;
            m_VppCompParams.U = 0x80;
            m_VppCompParams.V = 0x80;
            break;
        }

#if MFX_VERSION >= 1023
        m_VppCompParams.NumTiles = pInParams->numTiles4Comp;
#endif

        MSDK_CHECK_POINTER(pInParams->pVppCompDstRects,MFX_ERR_NULL_PTR);
        for (mfxU32 i = 0; i < pInParams->numSurf4Comp; i++)
        {
            m_VppCompParams.InputStream[i].DstX = pInParams->pVppCompDstRects[i].DstX;
            m_VppCompParams.InputStream[i].DstY = pInParams->pVppCompDstRects[i].DstY;
            m_VppCompParams.InputStream[i].DstW = pInParams->pVppCompDstRects[i].DstW;
            m_VppCompParams.InputStream[i].DstH = pInParams->pVppCompDstRects[i].DstH;
#if MFX_VERSION >= 1023
            m_VppCompParams.InputStream[i].TileId = pInParams->pVppCompDstRects[i].TileId;
#endif
            m_VppCompParams.InputStream[i].GlobalAlpha = 0;
            m_VppCompParams.InputStream[i].GlobalAlphaEnable = 0;
            m_VppCompParams.InputStream[i].PixelAlphaEnable = 0;

            m_VppCompParams.InputStream[i].LumaKeyEnable = 0;
            m_VppCompParams.InputStream[i].LumaKeyMin = 0;
            m_VppCompParams.InputStream[i].LumaKeyMax = 0;
        }

        m_VppExtParamsStorage.ExtBuffers.push_back((mfxExtBuffer *)&m_VppCompParams);
    } // if ( ((pInParams->eModeExt == VppComp) || (pInParams->eModeExt == VppCompOnly)) &&

    if (m_bUseOpaqueMemory)
        m_VppExtParamsStorage.ExtBuffers.push_back((mfxExtBuffer *)&m_VppOpaqueAlloc);
    if (pInParams->bIsMVC)
        m_VppExtParamsStorage.ExtBuffers.push_back((mfxExtBuffer *)&m_MVCSeqDesc);

    // Initializing m_VppExtParamsStorage here, to put all extra filters (created in Init function) to the end of buffer
    m_VppExtParamsStorage.Init(pInParams);

    // Adding

    m_mfxVppParams.ExtParam = &m_VppExtParamsStorage.ExtBuffers[0]; // vector is stored linearly in memory
    m_mfxVppParams.NumExtParam = (mfxU16)m_VppExtParamsStorage.ExtBuffers.size();

#ifdef ENABLE_MCTF
    // would it be more efficienct to get a pointer?
    m_MctfRTParams = pInParams->mctfParam.rtParams;
    m_MctfRTParams.Restart();
#endif

    return MFX_ERR_NONE;

} //mfxStatus CTranscodingPipeline::InitMfxVppParams(sInputParams *pInParams)

mfxStatus CTranscodingPipeline::InitPluginMfxParams(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    mfxU16 parentPattern = m_bIsVpp ? m_mfxVppParams.IOPattern : m_mfxDecParams.IOPattern;
    mfxU16 InPatternFromParent = (mfxU16)((MFX_IOPATTERN_OUT_VIDEO_MEMORY & parentPattern) ?
        MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY);

    // set memory pattern
    if (m_bUseOpaqueMemory)
        m_mfxPluginParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY|MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    else if (pInParams->bForceSysMem || (MFX_IMPL_SOFTWARE == pInParams->libType))
        m_mfxPluginParams.IOPattern = (mfxU16)(InPatternFromParent|MFX_IOPATTERN_OUT_SYSTEM_MEMORY);
    else
        m_mfxPluginParams.IOPattern = (mfxU16)(InPatternFromParent|MFX_IOPATTERN_OUT_VIDEO_MEMORY);

    m_mfxPluginParams.AsyncDepth = m_AsyncDepth;

    // input frame info
    if (m_bIsVpp)
    {
        MSDK_MEMCPY_VAR(m_mfxPluginParams.vpp.In, &m_mfxVppParams.vpp.Out, sizeof(mfxFrameInfo));
    }
    else
    {
        MSDK_MEMCPY_VAR(m_mfxPluginParams.vpp.In, &m_mfxDecParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
    }

    // fill output frame info
    // in case of rotation plugin sample output frameinfo is same as input
    MSDK_MEMCPY_VAR(m_mfxPluginParams.vpp.Out, &m_mfxPluginParams.vpp.In, sizeof(mfxFrameInfo));

    // configure and attach external parameters
    if (m_bUseOpaqueMemory)
        m_PluginExtParams.push_back((mfxExtBuffer *)&m_PluginOpaqueAlloc);

    if (!m_PluginExtParams.empty())
    {
        m_mfxPluginParams.ExtParam = &m_PluginExtParams[0]; // vector is stored linearly in memory
        m_mfxPluginParams.NumExtParam = (mfxU16)m_PluginExtParams.size();
    }

    return MFX_ERR_NONE;

} //mfxStatus CTranscodingPipeline::InitMfxVppParams(sInputParams *pInParams)

mfxStatus CTranscodingPipeline::AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxU16 nSurfNum = 0; // number of surfaces
    mfxU16 i;

    nSurfNum = pRequest->NumFrameMin = pRequest->NumFrameSuggested;
    msdk_printf(MSDK_STRING("Pipeline surfaces number (%s): %d\n"), isDecAlloc ? MSDK_STRING("DecPool") : MSDK_STRING("EncPool"),nSurfNum);

    mfxFrameAllocResponse *pResponse = isDecAlloc ? &m_mfxDecResponse : &m_mfxEncResponse;

    // no actual memory is allocated if opaque memory type is used
    if (!m_bUseOpaqueMemory)
    {
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, pRequest, pResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");
    }

    for (i = 0; i < nSurfNum; i++)
    {
        mfxFrameSurface1 *surface = new mfxFrameSurface1;
        MSDK_CHECK_POINTER(surface, MFX_ERR_MEMORY_ALLOC);

        MSDK_ZERO_MEMORY(*surface);
        MSDK_MEMCPY_VAR(surface->Info, &(pRequest->Info), sizeof(mfxFrameInfo));

        // no actual memory is allocated if opaque memory type is used (surface pointers and MemId field remain 0)
        if (!m_bUseOpaqueMemory)
        {
            surface->Data.MemId = pResponse->mids[i];
        }
#ifdef ENABLE_MCTF
        if (isDecAlloc)
        {
            // to allocate a buffer of pointers to buffer and initialize it 
            surface->Data.ExtParam = new mfxExtBuffer*[MAX_NUM_OF_ATTACHED_BUFFERS_FOR_IN_SUFACE];
            for (size_t k = 0; k < MAX_NUM_OF_ATTACHED_BUFFERS_FOR_IN_SUFACE; ++k)
                surface->Data.ExtParam[k] = NULL;
            surface->Data.NumExtParam = 0;
        }
#endif

        (isDecAlloc) ? m_pSurfaceDecPool.push_back(surface):m_pSurfaceEncPool.push_back(surface);
    }

    (isDecAlloc) ? m_DecSurfaceType = pRequest->Type : m_EncSurfaceType = pRequest->Type;

    return MFX_ERR_NONE;

} // mfxStatus CTranscodingPipeline::AllocFrames(Component* pComp, mfxFrameAllocResponse* pMfxResponse, mfxVideoParam* pMfxVideoParam)

//return true if correct
static bool CheckAsyncDepth(mfxFrameAllocRequest &curReq, mfxU16 asyncDepth)
{
    return (curReq.NumFrameSuggested >= asyncDepth);
}

static mfxStatus CorrectAsyncDepth(mfxFrameAllocRequest &curReq, mfxU16 asyncDepth)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!CheckAsyncDepth(curReq, asyncDepth))
    {
        sts = MFX_ERR_MEMORY_ALLOC;
    }
    else
    {
        // The request holds summary of required surfaces numbers from 2 components and
        // asyncDepth is included twice. Here we patch surfaces number removing
        // one asyncDepth.
        curReq.NumFrameSuggested = curReq.NumFrameSuggested - asyncDepth;
        curReq.NumFrameMin = curReq.NumFrameSuggested;
    }

    return sts;
}

static void SumAllocRequest(mfxFrameAllocRequest  &curReq, mfxFrameAllocRequest  &newReq)
{
    curReq.NumFrameSuggested = curReq.NumFrameSuggested + newReq.NumFrameSuggested;
    curReq.NumFrameMin = curReq.NumFrameSuggested;
    curReq.Type = curReq.Type | newReq.Type;

    if ((curReq.Type & MFX_MEMTYPE_SYSTEM_MEMORY) && ((curReq.Type & 0xf0) != MFX_MEMTYPE_SYSTEM_MEMORY))
        curReq.Type = (mfxU16)(curReq.Type & (~ MFX_MEMTYPE_SYSTEM_MEMORY));
    if ((curReq.Type & MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET) && ((curReq.Type & 0xf0) != MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
        curReq.Type = (mfxU16)(curReq.Type & (~MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET));

    if (curReq.Info.Width == 0)
    {
        curReq.Info = newReq.Info;
    }
    else
    {
        curReq.Info.Width  = curReq.Info.Width < newReq.Info.Width ? newReq.Info.Width : curReq.Info.Width ;
        curReq.Info.Height = curReq.Info.Height < newReq.Info.Height ? newReq.Info.Height : curReq.Info.Height ;
    }
}

mfxStatus CTranscodingPipeline::AllocFrames()
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bAddFrames = true;   // correct shared pool between session

    mfxFrameAllocRequest DecOut;
    mfxFrameAllocRequest VPPOut;

    MSDK_ZERO_MEMORY(DecOut);
    MSDK_ZERO_MEMORY(VPPOut);

    sts = CalculateNumberOfReqFrames(DecOut,VPPOut);
    MSDK_CHECK_STATUS(sts, "CalculateNumberOfReqFrames failed");

    if (VPPOut.NumFrameSuggested)
    {
        if (bAddFrames)
        {
            SumAllocRequest(VPPOut, m_Request);
            bAddFrames = false;
        }

        // Do not correct anything if we're using raw output - we'll need those surfaces for storing data for writer
        if(m_mfxEncParams.mfx.CodecId != MFX_CODEC_DUMP)
        {
           // In case of rendering enabled we need to add 1 additional surface for renderer
           if((m_nVPPCompEnable == VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
           {
               VPPOut.NumFrameSuggested++;
               VPPOut.NumFrameMin++;
           }
           sts = CorrectAsyncDepth(VPPOut, m_AsyncDepth);
           MSDK_CHECK_STATUS(sts, "CorrectAsyncDepth failed");
           /* WA for rendering: VPP should have at least 2 frames on output (for front & back buffer accordinally) */
           if ((VPPOut.NumFrameSuggested == 1) || (VPPOut.NumFrameMin == 1))
           {
               VPPOut.NumFrameSuggested = 2;
               VPPOut.NumFrameMin = 2;
           }
        }

#ifdef LIBVA_SUPPORT
        if (((m_nVPPCompEnable == VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode)) &&
            ((m_libvaBackend == MFX_LIBVA_DRM_MODESET) ||
#if defined(X11_DRI3_SUPPORT)
            (m_libvaBackend == MFX_LIBVA_X11) ||
#endif // X11_DRI3_SUPPORT
            (m_libvaBackend == MFX_LIBVA_WAYLAND)))
        {
            VPPOut.Type |= MFX_MEMTYPE_EXPORT_FRAME;
        }
#endif
        sts = AllocFrames(&VPPOut, false);
        MSDK_CHECK_STATUS(sts, "AllocFrames failed");
    }

    if (DecOut.NumFrameSuggested)
    {
        if (bAddFrames)
        {
            SumAllocRequest(DecOut, m_Request);
            bAddFrames = false;
        }

        if (m_bDecodeEnable)
        {
            if(0 == m_nVPPCompEnable && m_mfxEncParams.mfx.CodecId != MFX_CODEC_DUMP)
            {
                //--- Make correction to number of surfaces only if composition is not enabled. In case of composition we need all the surfaces QueryIOSurf has requested to pass them to another session's VPP
                // In other inter-session cases, other sessions request additional surfaces using additional calls to AllocFrames
                sts = CorrectAsyncDepth(DecOut, m_AsyncDepth);
                MSDK_CHECK_STATUS(sts, "CorrectAsyncDepth failed");
            }

            // AllocId just opaque handle which allow separate decoder requests in case of VPP Composition with external allocator
            static mfxU32 mark_alloc = 0;
            m_mfxDecParams.AllocId = mark_alloc;
            DecOut.AllocId = mark_alloc;
            if(m_nVPPCompEnable) // WORKAROUND: Remove this if clause after problem with AllocID is fixed in library (mark_alloc++ should be left here)
            {
                mark_alloc++;
            }

            sts = AllocFrames(&DecOut, true);
            MSDK_CHECK_STATUS(sts, "AllocFrames failed");
            sts = CorrectPreEncAuxPool((VPPOut.NumFrameSuggested ? VPPOut.NumFrameSuggested : DecOut.NumFrameSuggested) + m_AsyncDepth);
            MSDK_CHECK_STATUS(sts, "CorrectPreEncAuxPool failed");
            sts = AllocPreEncAuxPool();
            MSDK_CHECK_STATUS(sts, "AllocPreEncAuxPool failed");
        }
        else if((m_nVPPCompEnable==VppComp || m_nVPPCompEnable==VppCompOnly || m_nVPPCompEnable==VppCompOnlyEncode) && m_bUseOpaqueMemory)
        {
            //--- N->1 case, allocating empty pool for opaque only
            sts = AllocFrames(&DecOut, true);
            MSDK_CHECK_STATUS(sts, "AllocFrames failed");
        }
        else
        {
            if ((m_pParentPipeline) &&
                (0 == m_nVPPCompEnable) /* case if 1_to_N  */)
            {
                m_pParentPipeline->CorrectNumberOfAllocatedFrames(&DecOut);
                sts = m_pParentPipeline->CorrectPreEncAuxPool(VPPOut.NumFrameSuggested + DecOut.NumFrameSuggested + m_AsyncDepth);
                MSDK_CHECK_STATUS(sts, "m_pParentPipeline->CorrectPreEncAuxPool failed");
            }
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CTranscodingPipeline::CalculateNumberOfReqFrames(mfxFrameAllocRequest  &pDecOut, mfxFrameAllocRequest  &pVPPOut)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest *pSumRequest = &pDecOut;

    memset(&pDecOut,0,sizeof(pDecOut));
    memset(&pVPPOut,0,sizeof(pVPPOut));

    mfxFrameAllocRequest DecRequest;
    MSDK_ZERO_MEMORY(DecRequest);

    if (m_pmfxDEC.get())
    {
        sts = m_pmfxDEC.get()->QueryIOSurf(&m_mfxDecParams, &DecRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC.get failed");

        if (!CheckAsyncDepth(DecRequest, m_mfxDecParams.AsyncDepth))
            return MFX_ERR_MEMORY_ALLOC;
        SumAllocRequest(*pSumRequest, DecRequest);
    }
    else if (m_bUseOverlay)
    {
        // It takes 1 surface for overlay
        DecRequest.NumFrameMin = DecRequest.NumFrameSuggested = 1;
        DecRequest.Type = MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME;
        MSDK_MEMCPY(&DecRequest.Info, &m_mfxDecParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
        SumAllocRequest(*pSumRequest, DecRequest);
    }

    if (m_pmfxVPP.get())
    {
        mfxFrameAllocRequest VppRequest[2];

        MSDK_ZERO_MEMORY(VppRequest);
        if (m_bIsPlugin && m_bIsVpp)
        {
            sts = m_pmfxVPP.get()->QueryIOSurf(&m_mfxPluginParams, &(VppRequest[0]), &m_mfxVppParams);
            if (!CheckAsyncDepth(VppRequest[0], m_mfxPluginParams.AsyncDepth) ||
                !CheckAsyncDepth(VppRequest[1], m_mfxPluginParams.AsyncDepth) ||
                !CheckAsyncDepth(VppRequest[0], m_mfxVppParams.AsyncDepth) ||
                !CheckAsyncDepth(VppRequest[1], m_mfxVppParams.AsyncDepth))
                return MFX_ERR_MEMORY_ALLOC;
        }
        else if (m_bIsPlugin)
        {
            sts = m_pmfxVPP.get()->QueryIOSurf(&m_mfxPluginParams, &(VppRequest[0]));
            if (!CheckAsyncDepth(VppRequest[0], m_mfxPluginParams.AsyncDepth) ||
                !CheckAsyncDepth(VppRequest[1], m_mfxPluginParams.AsyncDepth))
                return MFX_ERR_MEMORY_ALLOC;
        }
        else
        {
            sts = m_pmfxVPP.get()->QueryIOSurf(&m_mfxVppParams, &(VppRequest[0]));
            if (!CheckAsyncDepth(VppRequest[0], m_mfxVppParams.AsyncDepth) ||
                !CheckAsyncDepth(VppRequest[1], m_mfxVppParams.AsyncDepth))
                return MFX_ERR_MEMORY_ALLOC;
        }

        MSDK_CHECK_STATUS(sts, "m_pmfxVPP.get()->QueryIOSurf failed");

        SumAllocRequest(*pSumRequest, VppRequest[0]);
        pSumRequest = &pVPPOut;
        SumAllocRequest(*pSumRequest, VppRequest[1]);
    }
    if (m_pmfxPreENC.get())
    {
        mfxFrameAllocRequest PreEncRequest;

        MSDK_ZERO_MEMORY(PreEncRequest);
        sts = m_pmfxPreENC.get()->QueryIOSurf(&m_mfxPreEncParams, &PreEncRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC.get()->QueryIOSurf failed");

        if (!CheckAsyncDepth(PreEncRequest, m_mfxPreEncParams.AsyncDepth))
            return MFX_ERR_MEMORY_ALLOC;
        SumAllocRequest(*pSumRequest, PreEncRequest);
    }
    if (m_pmfxENC.get())
    {
        mfxFrameAllocRequest EncRequest;

        MSDK_ZERO_MEMORY(EncRequest);

        sts = m_pmfxENC.get()->QueryIOSurf(&m_mfxEncParams, &EncRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxENC.get()->QueryIOSurf failed");

        if (!CheckAsyncDepth(EncRequest, m_mfxEncParams.AsyncDepth))
            return MFX_ERR_MEMORY_ALLOC;
        SumAllocRequest(*pSumRequest, EncRequest);
    }

    if(!pSumRequest->Type && m_pmfxDEC.get())
    {
        //--- If noone has set type to VPP request type yet, set it now basing on decoder request type
        pSumRequest->Type = MFX_MEMTYPE_BASE(DecRequest.Type) | MFX_MEMTYPE_FROM_VPPOUT;
    }

    return MFX_ERR_NONE;
}
void CTranscodingPipeline::CorrectNumberOfAllocatedFrames(mfxFrameAllocRequest  *pNewReq)
{
    if(shouldUseGreedyFormula)
    {
        m_Request.NumFrameSuggested+=pNewReq->NumFrameSuggested;
    }
    else
    {
        m_Request.NumFrameSuggested = std::max(m_Request.NumFrameSuggested,pNewReq->NumFrameSuggested);
    }

    m_Request.NumFrameMin = m_Request.NumFrameSuggested;
    m_Request.Type = m_Request.Type | pNewReq->Type;

    if ((m_Request.Type & MFX_MEMTYPE_SYSTEM_MEMORY) && ((m_Request.Type & 0xf0) != MFX_MEMTYPE_SYSTEM_MEMORY))
        m_Request.Type = (mfxU16)(m_Request.Type & (~ MFX_MEMTYPE_SYSTEM_MEMORY));
    if ((m_Request.Type & MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET) && ((m_Request.Type & 0xf0) != MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET))
        m_Request.Type = (mfxU16)(m_Request.Type & (~ MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET));

    if (m_Request.Info.Width == 0)
    {
        m_Request.Info = pNewReq->Info;
    }
    else
    {
        m_Request.Info.Width  = m_Request.Info.Width < pNewReq->Info.Width ? pNewReq->Info.Width : m_Request.Info.Width ;
        m_Request.Info.Height = m_Request.Info.Height < pNewReq->Info.Height ? pNewReq->Info.Height : m_Request.Info.Height ;
    }
}

mfxStatus CTranscodingPipeline::InitOpaqueAllocBuffers()
{
    if (m_pmfxDEC.get() ||
        (m_bUseOpaqueMemory && (m_nVPPCompEnable==VppComp || m_nVPPCompEnable==VppCompOnly || m_nVPPCompEnable==VppCompOnlyEncode) && m_pSurfaceDecPool.size()))
    {
        m_DecOpaqueAlloc.Out.Surfaces = &m_pSurfaceDecPool[0]; // vestor is stored linearly in memory
        m_DecOpaqueAlloc.Out.NumSurface = (mfxU16)m_pSurfaceDecPool.size();
        if (m_DecSurfaceType & MFX_MEMTYPE_FROM_VPPOUT)
            m_DecOpaqueAlloc.Out.Type = (mfxU16)(MFX_MEMTYPE_BASE(m_DecSurfaceType) | MFX_MEMTYPE_FROM_VPPOUT);
        else
            m_DecOpaqueAlloc.Out.Type = (mfxU16)(MFX_MEMTYPE_BASE(m_DecSurfaceType) | MFX_MEMTYPE_FROM_DECODE);
    }
    else
    {
        // if no decoder in the pipeline we need to query m_DecOpaqueAlloc structure from parent sink pipeline
        m_DecOpaqueAlloc = m_pParentPipeline->GetDecOpaqueAlloc();
    }

    if (m_pmfxVPP.get())
    {
        m_EncOpaqueAlloc.In.Surfaces = &m_pSurfaceEncPool[0];
        m_EncOpaqueAlloc.In.NumSurface = (mfxU16)m_pSurfaceEncPool.size();
        m_EncOpaqueAlloc.In.Type = (mfxU16)(MFX_MEMTYPE_BASE(m_EncSurfaceType) | MFX_MEMTYPE_FROM_ENCODE);

        // decode will be connected with either VPP or Plugin
        if (m_bIsVpp)
        {
            m_VppOpaqueAlloc.In = m_DecOpaqueAlloc.Out;
        }
        else if (m_bIsPlugin)
        {
            m_PluginOpaqueAlloc.In = m_DecOpaqueAlloc.Out;
        }
        else
            return MFX_ERR_UNSUPPORTED;

        // encode will be connected with either Plugin or VPP
        if (m_bIsPlugin)
            m_PluginOpaqueAlloc.Out = m_EncOpaqueAlloc.In;
        else if (m_bIsVpp)
            m_VppOpaqueAlloc.Out = m_EncOpaqueAlloc.In;
        else
            return MFX_ERR_UNSUPPORTED;
    }
    else if (m_pmfxENC.get() || m_pmfxPreENC.get())
    {
        m_EncOpaqueAlloc.In = m_DecOpaqueAlloc.Out;
    }
    if (m_pmfxPreENC.get())
    {
        m_PreEncOpaqueAlloc.In = m_EncOpaqueAlloc.In;
    }


    return MFX_ERR_NONE;
}

void CTranscodingPipeline::FreeFrames()
{
    // free mfxFrameSurface structures and arrays of pointers
    mfxU32 i;
    for (i = 0; i < m_pSurfaceDecPool.size(); i++)
    {
#ifdef ENABLE_MCTF
        if (m_pSurfaceDecPool[i]->Data.ExtParam) {
            delete[]m_pSurfaceDecPool[i]->Data.ExtParam;
            m_pSurfaceDecPool[i]->Data.ExtParam = NULL;
        }
#endif
        MSDK_SAFE_DELETE(m_pSurfaceDecPool[i]);
    }

    m_pSurfaceDecPool.clear();

    for (i = 0; i < m_pSurfaceEncPool.size(); i++)
    {
        MSDK_SAFE_DELETE(m_pSurfaceEncPool[i]);
    }

    m_pSurfaceEncPool.clear();

    if (m_pMFXAllocator && !m_bUseOpaqueMemory)
    {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxEncResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecResponse);
    }
#ifdef ENABLE_MCTF
    GetMctfParamBuffer<mfxExtVppMctf, MFX_EXTBUFF_VPP_MCTF>((mfxFrameSurface1*)NULL, true);
#endif
} // CTranscodingPipeline::FreeFrames()

mfxStatus CTranscodingPipeline::Init(sInputParams *pParams,
                                     MFXFrameAllocator *pMFXAllocator,
                                     void* hdl,
                                     CTranscodingPipeline *pParentPipeline,
                                     SafetySurfaceBuffer  *pBuffer,
                                     FileBitstreamProcessor   *pBSProc)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pMFXAllocator, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pBSProc, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;
    m_MaxFramesForTranscode = pParams->MaxFrameNumber;
    // if no number of frames for a particular session is undefined, default
    // value is 0xFFFFFFFF. Thus, use it as a marker to assign parent
    // MaxFramesForTranscode to m_MaxFramesForTranscode
    if(pParentPipeline && (0xFFFFFFFF == m_MaxFramesForTranscode) && pParentPipeline->m_MaxFramesForTranscode)
        m_MaxFramesForTranscode = pParentPipeline->m_MaxFramesForTranscode;

    // use external allocator
    m_pMFXAllocator = pMFXAllocator;
    m_pBSProcessor = pBSProc;
    m_hdl = hdl;

    m_pParentPipeline = pParentPipeline;
    shouldUseGreedyFormula = pParams->shouldUseGreedyFormula;

    m_nTimeout = pParams->nTimeout;

    m_AsyncDepth = (0 == pParams->nAsyncDepth)? 1: pParams->nAsyncDepth;
    m_FrameNumberPreference = pParams->FrameNumberPreference;
    m_numEncoders = 0;
    m_bUseOverlay = pParams->DecodeId == MFX_CODEC_RGB4 ? true : false;
    m_bRobustFlag = pParams->bRobustFlag;
    m_nRotationAngle = pParams->nRotationAngle;
    m_sGenericPluginPath = pParams->strVPPPluginDLLPath;
    m_decoderPluginParams = pParams->decoderPluginParams;
    m_encoderPluginParams = pParams->encoderPluginParams;

    m_encoderFourCC = pParams->EncoderFourCC;

    m_bExtMBQP = pParams->bExtMBQP;
    m_bROIasQPMAP = pParams->bROIasQPMAP;

#if MFX_VERSION >= 1022
    m_ROIData = pParams->m_ROIData;
#endif //MFX_VERSION >= 1022

    statisticsWindowSize = pParams->statisticsWindowSize;
    if (statisticsWindowSize > m_MaxFramesForTranscode)
        statisticsWindowSize = m_MaxFramesForTranscode;

    m_strMfxParamsDumpFile.assign(pParams->strMfxParamsDumpFile);

    if (pParams->statisticsLogFile)
    {
        //same log file for intput/output
        inputStatistics.SetOutputFile(pParams->statisticsLogFile);
        outputStatistics.SetOutputFile(pParams->statisticsLogFile);
    }

    if(!pParams->DumpLogFileName.empty())
    {
        inputStatistics.SetDumpName((pParams->DumpLogFileName + MSDK_STRING("_input")).c_str());
        outputStatistics.SetDumpName((pParams->DumpLogFileName + MSDK_STRING("_output")).c_str());
    }

// if no statistic-window is passed but overall stat-log exist:
    // is requested, set statisticsWindowSize to m_MaxFramesForTranscode
    if ((pParams->statisticsLogFile || !pParams->DumpLogFileName.empty()) && 
        0 == statisticsWindowSize)
        statisticsWindowSize = m_MaxFramesForTranscode;

    if (m_bEncodeEnable)
    {
        m_pBSStore.reset(new ExtendedBSStore(m_AsyncDepth));
    }

    // Determine processing mode
    switch(pParams->eMode)
    {
    case Native:
        break;
    case Sink:
        if ((VppComp == pParams->eModeExt) || (VppCompOnly == pParams->eModeExt))
        {
            if ((NULL != pParentPipeline) && (NULL != pParentPipeline->m_pMFXAllocator))
                m_pMFXAllocator = pParentPipeline->m_pMFXAllocator;
        }

        m_bEncodeEnable = false; // decode only
        break;
    case Source:
        // for heterogeneous pipeline use parent allocator
        MSDK_CHECK_POINTER(pParentPipeline, MFX_ERR_NULL_PTR);
        m_pMFXAllocator = pParentPipeline->m_pMFXAllocator;
        m_bDecodeEnable = false; // encode only
        /**/
        if ((pParams->m_hwdev != NULL) && (VppCompOnly == pParams->eModeExt))
        {
#if defined(_WIN32) || defined(_WIN64)
            m_hwdev4Rendering = new CDecodeD3DRender;

            m_hwdev4Rendering->SetHWDevice(pParams->m_hwdev);

            sWindowParams RenderParam;

            memset(&RenderParam, 0, sizeof(sWindowParams));

            RenderParam.lpWindowName = MSDK_STRING("sample_multi_transcode");
            RenderParam.nx           = 0;
            RenderParam.ny           = 0;
            RenderParam.nWidth       = pParams->nDstWidth;
            RenderParam.nHeight      = pParams->nDstHeight;
            RenderParam.ncell        = 0;
            RenderParam.nAdapter     = 0;

            RenderParam.lpClassName  = MSDK_STRING("Render Window Class");
            RenderParam.dwStyle      = WS_OVERLAPPEDWINDOW;
            RenderParam.hWndParent   = NULL;
            RenderParam.hMenu        = NULL;
            RenderParam.hInstance    = GetModuleHandle(NULL);
            RenderParam.lpParam      = NULL;
            RenderParam.bFullScreen  = TRUE;

            m_hwdev4Rendering->Init(RenderParam);
#else
            if(pParams->EncodeId)
            {
                m_nVPPCompEnable = VppCompOnlyEncode;
            }
            m_hwdev4Rendering = pParams->m_hwdev;
#endif
        }
        break;
    default:
        // unknown mode
        return MFX_ERR_UNSUPPORTED;
    }

    if ((VppComp == pParams->eModeExt) || (VppCompOnly == pParams->eModeExt))
    {
        if(m_nVPPCompEnable != VppCompOnlyEncode)
            m_nVPPCompEnable = pParams->eModeExt;
    }

#ifdef LIBVA_SUPPORT
    m_libvaBackend = pParams->libvaBackend;
#endif

    m_pBuffer = pBuffer;

    mfxExtBuffer* extBufs[1];

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    m_initPar.Version.Major = 1;
    m_initPar.Version.Minor = 0;
    m_initPar.Implementation = pParams->libType;

    init_ext_buffer(m_threadsPar);

    bool needInitExtPar = false;

    if (pParams->nThreadsNum) {
        m_threadsPar.NumThread = pParams->nThreadsNum;
        needInitExtPar = true;
    }
    if (needInitExtPar) {
        extBufs[0] = (mfxExtBuffer*)&m_threadsPar;
        m_initPar.ExtParam = extBufs;
        m_initPar.NumExtParam = 1;
    }

    //--- GPU Copy settings
    m_initPar.GPUCopy = pParams->nGpuCopyMode;

    // init session
    m_pmfxSession.reset(new MFXVideoSession);

    // Initializing library
    sts = m_pmfxSession->InitEx(m_initPar);
    MSDK_CHECK_STATUS(sts, "m_pmfxSession->InitEx failed");
    // check the API version of actually loaded library
    sts = m_pmfxSession->QueryVersion(&m_Version);
    MSDK_CHECK_STATUS(sts, "m_pmfxSession->QueryVersion failed");

    sts = CheckRequiredAPIVersion(m_Version, pParams);
    MSDK_CHECK_STATUS(sts, "CheckRequiredAPIVersion failed");

    // opaque memory feature is available starting with API 1.3 and
    // can be used within 1 intra session or joined inter sessions only
    if (m_Version.Major >= 1 && m_Version.Minor >= 3 &&
        (pParams->eMode == Native || pParams->bIsJoin))
    {
        m_bUseOpaqueMemory = true;
    }
    if (!pParams->bUseOpaqueMemory || pParams->EncodeId == MFX_CODEC_DUMP) // Don't use opaque in case of yuv output or if it was specified explicitly
        m_bUseOpaqueMemory = false;

    m_bIsInterOrJoined = pParams->eMode == Sink || pParams->eMode == Source || pParams->bIsJoin;

    sts = SetAllocatorAndHandleIfRequired();
    MSDK_CHECK_STATUS(sts, "SetAllocatorAndHandleIfRequired failed");

    // Joining sessions if required
    if (pParams->bIsJoin && pParentPipeline)
    {
        sts = pParentPipeline->Join(m_pmfxSession.get());
        MSDK_CHECK_STATUS(sts, "pParentPipeline->Join failed");
        m_bIsJoinSession = true;
    }

    // Initialize pipeline components following downstream direction
    // Pre-init methods fill parameters and create components

    // Decode component initialization
    sts = DecodePreInit(pParams);
    if (MFX_ERR_MORE_DATA == sts)
        return sts;
    else
        MSDK_CHECK_STATUS(sts,"DecodePreInit failed");

    // VPP component initialization
    sts = VPPPreInit(pParams);
    MSDK_CHECK_STATUS(sts, "VPPPreInit failed");

    // LA component initialization
    sts = PreEncPreInit(pParams);
    MSDK_CHECK_STATUS(sts, "PreEncPreInit failed");

    // Encode component initialization
    if ((m_nVPPCompEnable != VppCompOnly) || (m_nVPPCompEnable == VppCompOnlyEncode))
    {
        sts = EncodePreInit(pParams);
        MSDK_CHECK_STATUS(sts, "EncodePreInit failed");
    }

    if ((pParams->eMode == Source) &&
        ((m_nVPPCompEnable == VppCompOnly) ||
          (m_nVPPCompEnable == VppCompOnlyEncode) ||
          (m_nVPPCompEnable == VppComp))  )
    {
        if ((0 == msdk_strncmp(MSDK_STRING("null_render"), pParams->strDumpVppCompFile, msdk_strlen(MSDK_STRING("null_render")))))
            m_vppCompDumpRenderMode = NULL_RENDER_VPP_COMP; // null_render case
        else if (0 != msdk_strlen(pParams->strDumpVppCompFile))
        {
            sts = m_dumpVppCompFileWriter.Init(pParams->strDumpVppCompFile, 0);
            MSDK_CHECK_STATUS(sts, "VPP COMP DUMP File Init failed");
            m_vppCompDumpRenderMode = DUMP_FILE_VPP_COMP;
        }
    }

    // Frames allocation for all component
    if (Native == pParams->eMode)
    {
        sts = AllocFrames();
        MSDK_CHECK_STATUS(sts, "AllocFrames failed");
    }
    else if (Source == pParams->eMode)// need allocate frames only for VPP and Encode if VPP exist
    {
        if (!m_bDecodeEnable)
        {
            sts = AllocFrames();
            MSDK_CHECK_STATUS(sts, "AllocFrames failed");
        }
    }

    isHEVCSW = AreGuidsEqual(pParams->decoderPluginParams.pluginGuid, MFX_PLUGINID_HEVCD_SW);

    // if sink - suspended allocation

    // common session settings
    if (m_Version.Major >= 1 && m_Version.Minor >= 1)
        sts = m_pmfxSession->SetPriority(pParams->priority);

    // if sink - suspended allocation
    if (Native !=  pParams->eMode)
        return sts;

    // after surfaces arrays are allocated configure mfxOpaqueAlloc buffers to be passed to components' Inits
    if (m_bUseOpaqueMemory)
    {
        sts = InitOpaqueAllocBuffers();
        MSDK_CHECK_STATUS(sts, "InitOpaqueAllocBuffers failed");
    }

    // Init decode
    if (m_pmfxDEC.get())
    {
        sts = m_pmfxDEC->Init(&m_mfxDecParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Init failed");
    }

    // Init VPP
    if (m_pmfxVPP.get())
    {
        if (m_bIsPlugin && m_bIsVpp)
            sts = m_pmfxVPP->Init(&m_mfxPluginParams, &m_mfxVppParams);
        else if (m_bIsPlugin)
            sts = m_pmfxVPP->Init(&m_mfxPluginParams);
        else
            sts = m_pmfxVPP->Init(&m_mfxVppParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");

        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }
    // LA initialization
    if (m_pmfxPreENC.get())
    {
        sts = m_pmfxPreENC->Init(&m_mfxPreEncParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->Init failed");
    }
    // Init encode
    if (m_pmfxENC.get())
    {
        sts = m_pmfxENC->Init(&m_mfxEncParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxENC->Init failed");

#if MFX_VERSION >= 1022
        if (pParams->bExtMBQP)
        {
            m_bUseQPMap = true;
        }

        if(pParams->bROIasQPMAP)
        {
            mfxVideoParam enc_par;
            MSDK_ZERO_MEMORY(enc_par);
            mfxStatus sts1 = m_pmfxENC->GetVideoParam(&enc_par);
            MSDK_CHECK_STATUS(sts1, "m_pmfxENC->GetVideoParam failed");

            if(enc_par.mfx.GopRefDist != 1)
            {
                printf("INFO: Sample implementation of ROI through MBQP map require B-frames to be disabled.\n");
                m_bUseQPMap = false;
            }
            else if(enc_par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
            {
                printf("INFO: MBQP map require ConstQP mode to operate.\n");
                m_bUseQPMap = false;
            }
            else
            {
                m_GOPSize = (enc_par.mfx.GopPicSize) ? enc_par.mfx.GopPicSize : 1;
                m_QPforI  = enc_par.mfx.QPI;
                m_QPforP  = enc_par.mfx.QPP;
                m_bUseQPMap = true;
            }
        }
#endif //MFX_VERSION >= 1022
    }

    // Dumping components configuration if required
    if (m_strMfxParamsDumpFile.size())
    {
        CParametersDumper::DumpLibraryConfiguration(m_strMfxParamsDumpFile, m_pmfxDEC.get(), m_pmfxVPP.get(), m_pmfxENC.get(), &m_mfxDecParams, &m_mfxVppParams, &m_mfxEncParams);
    }

    m_bIsInit = true;

    return sts;
} //mfxStatus CTranscodingPipeline::Init(sInputParams *pParams)


mfxStatus CTranscodingPipeline::CompleteInit()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bIsInit)
        return MFX_ERR_NONE;

    // need to allocate remaining frames
    if (m_bDecodeEnable)
    {
        sts = AllocFrames();
        MSDK_CHECK_STATUS(sts, "AllocFrames failed");
        LoadStaticSurface();
        MSDK_CHECK_STATUS(sts, "LoadStaticSurface failed");
    }

    // after surfaces arrays are allocated configure mfxOpaqueAlloc buffers to be passed to components' Inits
    if (m_bUseOpaqueMemory)
    {
        sts = InitOpaqueAllocBuffers();
        MSDK_CHECK_STATUS(sts, "InitOpaqueAllocBuffers failed");
    }

        // Init decode
    if (m_pmfxDEC.get())
    {
        sts = m_pmfxDEC->Init(&m_mfxDecParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Init failed");
    }

    // Init vpp
    if (m_pmfxVPP.get())
    {
        if (m_bIsPlugin && m_bIsVpp)
            sts = m_pmfxVPP->Init(&m_mfxPluginParams, &m_mfxVppParams);
        else if (m_bIsPlugin)
            sts = m_pmfxVPP->Init(&m_mfxPluginParams);
        else
            sts = m_pmfxVPP->Init(&m_mfxVppParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }
        // Pre init encode
    if (m_pmfxPreENC.get())
    {
        sts = m_pmfxPreENC->Init(&m_mfxPreEncParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->Init failed");
    }
    // Init encode
    if (m_pmfxENC.get())
    {
        sts = m_pmfxENC->Init(&m_mfxEncParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxENC->Init failed");
    }

    m_bIsInit = true;

    return sts;
} // mfxStatus CTranscodingPipeline::CompleteInit()
mfxFrameSurface1* CTranscodingPipeline::GetFreeSurface(bool isDec, mfxU64 timeout)
{
    mfxFrameSurface1* pSurf = NULL;

    CTimer t;
    t.Start();
    do
    {
        m_mStopSession.Lock();
        if (m_bForceStop)
        {
            m_mStopSession.Unlock();
            break;
        }
        m_mStopSession.Unlock();

        SurfPointersArray & workArray = isDec ? m_pSurfaceDecPool : m_pSurfaceEncPool;

        for (mfxU32 i = 0; i < workArray.size(); i++)
        {
            if (!workArray[i]->Data.Locked)
            {
                pSurf = workArray[i];
                break;
            }
        }
        if (pSurf)
        {
            break;
        }
        else
        {
            MSDK_SLEEP(TIME_TO_SLEEP);
        }
    } while ( t.GetTime() < timeout / 1000 );

    return pSurf;
} // mfxFrameSurface1* CTranscodingPipeline::GetFreeSurface(bool isDec)

mfxU32 CTranscodingPipeline::GetFreeSurfacesCount(bool isDec)
{
    SurfPointersArray& workArray = isDec ? m_pSurfaceDecPool : m_pSurfaceEncPool;
    mfxU32 count=0;
    for (mfxU32 i = 0; i < workArray.size(); i++)
    {
        if (!workArray[i]->Data.Locked)
        {
            count++;
        }
    }
    return count;
}


PreEncAuxBuffer*  CTranscodingPipeline::GetFreePreEncAuxBuffer()
{
    for(mfxU32 i = 0; i < m_pPreEncAuxPool.size(); i++)
    {
        if (!m_pPreEncAuxPool[i].Locked)
            return &(m_pPreEncAuxPool[i]);
    }
    return NULL;
}

void CTranscodingPipeline::LockPreEncAuxBuffer(PreEncAuxBuffer* pBuff)
{
    if (!pBuff) return;
    for (mfxU32 i=0; i < m_numEncoders; i++)
    {
        msdk_atomic_inc16(&pBuff->Locked);
    }
}

void CTranscodingPipeline::UnPreEncAuxBuffer(PreEncAuxBuffer* pBuff)
{
    if (!pBuff) return;
    msdk_atomic_dec16(&pBuff->Locked);
}

mfxU32 CTranscodingPipeline::GetNumFramesForReset()
{
    AutomaticMutex guard(m_mReset);
    return m_NumFramesForReset;
}

void CTranscodingPipeline::SetNumFramesForReset(mfxU32 nFrames)
{
    AutomaticMutex guard(m_mReset);
    m_NumFramesForReset = nFrames;
}

void CTranscodingPipeline::HandlePossibleGpuHang(mfxStatus & sts)
{
    if (sts == MFX_ERR_GPU_HANG && !m_bRobustFlag)
    {
        msdk_printf(MSDK_STRING("GPU hang happened\n"));
        sts = MFX_ERR_NONE;
        m_bInsertIDR = true;
    }
}

mfxStatus CTranscodingPipeline::SetAllocatorAndHandleIfRequired()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxIMPL impl = 0;
    m_pmfxSession->QueryIMPL(&impl);

    // Media SDK session doesn't require external allocator if the application uses opaque memory
    if (!m_bUseOpaqueMemory)
    {
        sts = m_pmfxSession->SetFrameAllocator(m_pMFXAllocator);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SetFrameAllocator failed");
    }

    mfxHandleType handleType = (mfxHandleType)0;
    bool bIsMustSetExternalHandle = 0;

    if (MFX_IMPL_VIA_D3D11 == MFX_IMPL_VIA_MASK(impl))
    {
        handleType = MFX_HANDLE_D3D11_DEVICE;
        bIsMustSetExternalHandle = false;
    }
    else if (MFX_IMPL_VIA_D3D9 == MFX_IMPL_VIA_MASK(impl))
    {
        handleType = MFX_HANDLE_D3D9_DEVICE_MANAGER;
        bIsMustSetExternalHandle = false;
    }
#ifdef LIBVA_SUPPORT
    else if (MFX_IMPL_VIA_VAAPI == MFX_IMPL_VIA_MASK(impl))
    {
        handleType = MFX_HANDLE_VA_DISPLAY;
        bIsMustSetExternalHandle = true;
    }
#endif

    if (m_hdl && (bIsMustSetExternalHandle || (m_bIsInterOrJoined || !m_bUseOpaqueMemory)))
    {
        sts = m_pmfxSession->SetHandle(handleType, m_hdl);
        MSDK_CHECK_STATUS(sts, "m_pmfxSession->SetHandle failed");
    }
    return sts;
}

mfxStatus CTranscodingPipeline::LoadGenericPlugin()
{
    mfxStatus sts = MFX_ERR_NONE;

    std::unique_ptr<MFXVideoVPPPlugin> pVPPPlugin(new MFXVideoVPPPlugin(*m_pmfxSession.get()));
    MSDK_CHECK_POINTER(pVPPPlugin.get(), MFX_ERR_NULL_PTR);

    sts = pVPPPlugin->LoadDLL((msdk_char*)m_sGenericPluginPath.c_str());
    MSDK_CHECK_STATUS(sts, "pVPPPlugin->LoadDLL failed");

    m_RotateParam.Angle = m_nRotationAngle;
    sts = pVPPPlugin->SetAuxParam(&m_RotateParam, sizeof(m_RotateParam));
    MSDK_CHECK_STATUS(sts, "pVPPPlugin->SetAuxParam failed");

    if (!m_bUseOpaqueMemory)
    {
        sts = pVPPPlugin->SetFrameAllocator(m_pMFXAllocator);
        MSDK_CHECK_STATUS(sts, "pVPPPlugin->SetFrameAllocator failed");
    }

    m_pmfxVPP.reset(pVPPPlugin.release());
    return MFX_ERR_NONE;
}

size_t CTranscodingPipeline::GetRobustFlag()
{
    return m_bRobustFlag;
}

void CTranscodingPipeline::Close()
{
    if (m_pmfxDEC.get())
        m_pmfxDEC->Close();

    if (m_pmfxENC.get())
        m_pmfxENC->Close();

    if (m_pmfxVPP.get())
        m_pmfxVPP->Close();

    if (m_pUserDecoderPlugin.get())
        m_pUserDecoderPlugin.reset();

    if (m_pUserEncoderPlugin.get())
        m_pUserEncoderPlugin.reset();

    if (m_pUserEncPlugin.get())
        m_pUserEncPlugin.reset();


    FreeVppDoNotUse();
    FreeMVCSeqDesc();
    FreePreEncAuxPool();

    if (m_VppCompParams.InputStream != NULL)
        free(m_VppCompParams.InputStream);

    m_EncExtParams.clear();
    m_DecExtParams.clear();
    m_VppExtParamsStorage.Clear();
    m_PreEncExtParams.clear();

    if (m_bIsJoinSession)
    {
        //m_pmfxSession->DisjoinSession();
        m_bIsJoinSession = false;
    }

    //Destroy renderer
#if defined(_WIN32) || defined(_WIN64)
    if(m_hwdev4Rendering)
    {
        delete m_hwdev4Rendering;
        m_hwdev4Rendering=NULL;
    }
#endif

    // free allocated surfaces AFTER closing components
    FreeFrames();

    m_bIsInit = false;

} // void CTranscodingPipeline::Close()

mfxStatus CTranscodingPipeline::Reset()
{
    mfxStatus sts = MFX_ERR_NONE;
    bool isDec = m_pmfxDEC.get() ? true : false,
        isEnc = m_pmfxENC.get() ? true : false,
        isVPP = m_pmfxVPP.get() ? true : false,
        isPreEnc = m_pmfxPreENC.get() ? true : false,
        isGenericPLugin = m_nRotationAngle ? true : false,
        isDecoderPlugin = m_pUserDecoderPlugin.get() ? true : false,
        isEncoderPlugin = m_pUserEncoderPlugin.get() ? true : false,
        isPreEncPlugin = m_pUserEncPlugin.get() ? true : false;

    // Close components being used
    if (isDec)
    {
        m_pmfxDEC->Close();
        m_pmfxDEC.reset();
    }

    if (isVPP)
    {
        m_pmfxVPP->Close();
        m_pmfxVPP.reset();
    }

    if (isPreEnc)
    {
        m_pmfxPreENC->Close();
        m_pmfxPreENC.reset();
    }

    if (isEnc)
    {
        m_pmfxENC->Close();
        m_pmfxENC.reset();
    }

    if (isDecoderPlugin)
    {
        m_pUserDecoderPlugin.reset();
        m_pUserDecoderModule.reset();
    }

    if (isEncoderPlugin)
    {
        m_pUserEncoderPlugin.reset();
        m_pUserEncoderModule.reset();
    }
    m_pmfxSession->Close();
    m_pmfxSession.reset();

    m_pmfxSession.reset(new MFXVideoSession());
    sts = m_pmfxSession->InitEx(m_initPar);
    MSDK_CHECK_STATUS(sts, "m_pmfxSession->InitEx failed");

    // Release dec and enc surface pools
    for (size_t i = 0; i < m_pSurfaceDecPool.size(); i++)
    {
        m_pSurfaceDecPool[i]->Data.Locked = 0;
    }
    for (size_t i = 0; i < m_pSurfaceEncPool.size(); i++)
    {
        m_pSurfaceEncPool[i]->Data.Locked = 0;
    }

    // Release all safety buffers
    SafetySurfaceBuffer* sptr = m_pBuffer;
    while (sptr)
    {
        sptr->ReleaseSurfaceAll();
        sptr = sptr->m_pNext;
    }

    // Release output bitstram pools
    m_BSPool.clear();
    m_pBSStore->ReleaseAll();
    m_pBSStore->FlushAll();

    // Load external decoder plugin
    if (isDecoderPlugin)
    {
        if (m_decoderPluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(m_decoderPluginParams.strPluginPath, sizeof(m_decoderPluginParams.strPluginPath)))
        {
            m_pUserDecoderModule.reset(new MFXVideoUSER(*m_pmfxSession.get()));
            m_pUserDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, *m_pmfxSession.get(), m_decoderPluginParams.pluginGuid, 1, m_decoderPluginParams.strPluginPath, (mfxU32)msdk_strnlen(m_decoderPluginParams.strPluginPath, sizeof(m_decoderPluginParams.strPluginPath))));
            if (m_pUserDecoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            if (AreGuidsEqual(m_decoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_decoderPluginParams.pluginGuid = msdkGetPluginUID(m_initPar.Implementation, MSDK_VDECODE, m_mfxDecParams.mfx.CodecId);
            }
            if (!AreGuidsEqual(m_decoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_pUserDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, *m_pmfxSession.get(), m_decoderPluginParams.pluginGuid, 1));
                if (m_pUserDecoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
            }
        }
        MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
    }

    // Load external encoder plugin
    if (isEncoderPlugin)
    {
        if (m_encoderPluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(m_encoderPluginParams.strPluginPath, sizeof(m_encoderPluginParams.strPluginPath)))
        {
            m_pUserEncoderModule.reset(new MFXVideoUSER(*m_pmfxSession.get()));
            m_pUserEncoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), m_encoderPluginParams.pluginGuid, 1, m_encoderPluginParams.strPluginPath, (mfxU32)msdk_strnlen(m_encoderPluginParams.strPluginPath, sizeof(m_encoderPluginParams.strPluginPath))));
            if (m_pUserEncoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            if (AreGuidsEqual(m_encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_decoderPluginParams.pluginGuid = msdkGetPluginUID(m_initPar.Implementation, MSDK_VENCODE, m_mfxEncParams.mfx.CodecId);
            }
            if (!AreGuidsEqual(m_encoderPluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_pUserEncoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), m_encoderPluginParams.pluginGuid, 1));
                if (m_pUserEncoderPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
            }
        }
        MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
    }

    // Load external pre-enc plugin
    if (isPreEncPlugin)
    {
        m_pUserEncPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, *m_pmfxSession.get(), MFX_PLUGINID_H264LA_HW, 1));
        if (m_pUserEncPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        MSDK_CHECK_STATUS(sts, "LoadPlugin failed");
    }

    sts = SetAllocatorAndHandleIfRequired();
    MSDK_CHECK_STATUS(sts, "SetAllocatorAndHandleIfRequired failed");

    if (isDec)
        m_pmfxDEC.reset(new MFXVideoDECODE((mfxSession)*m_pmfxSession));
    if (isVPP)
        m_pmfxVPP.reset(new MFXVideoMultiVPP((mfxSession)*m_pmfxSession));
    if (isPreEnc)
        m_pmfxPreENC.reset(new MFXVideoENC((mfxSession)*m_pmfxSession));
    if (isEnc)
        m_pmfxENC.reset(new MFXVideoENCODE((mfxSession)*m_pmfxSession));

    if (isDec)
    {
        sts = m_pmfxDEC->Init(&m_mfxDecParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Init failed");
    }

    if (isGenericPLugin)
    {
        sts = LoadGenericPlugin();
        MSDK_CHECK_STATUS(sts, "LoadGenericPlugin failed");
    }

    if (isVPP)
    {
        if (m_bIsPlugin && m_bIsVpp)
        {
            mfxFrameAllocRequest request[2] = { 0 };
            sts = m_pmfxVPP->QueryIOSurf(&m_mfxPluginParams, request, &m_mfxVppParams);
            MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

            sts = m_pmfxVPP->Init(&m_mfxPluginParams, &m_mfxVppParams);
        }
        else if (m_bIsPlugin)
            sts = m_pmfxVPP->Init(&m_mfxPluginParams);
        else
            sts = m_pmfxVPP->Init(&m_mfxVppParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }

    if (isPreEnc)
    {
        sts = m_pmfxPreENC->Init(&m_mfxPreEncParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxPreENC->Init failed");
    }

    if (isEnc)
    {
        sts = m_pmfxENC->Init(&m_mfxEncParams);
        MSDK_CHECK_STATUS(sts, "m_pmfxENC->Init failed");
    }

    // Joining sessions if required
    if (m_bIsJoinSession && m_pParentPipeline)
    {
        sts = m_pParentPipeline->Join(m_pmfxSession.get());
        MSDK_CHECK_STATUS(sts, "m_pParentPipeline->Join failed");
        m_bIsJoinSession = true;
    }
    return sts;
}

mfxStatus CTranscodingPipeline::AllocAndInitVppDoNotUse(sInputParams *pInParams)
{
    std::vector<mfxU32> filtersDisabled;
    if(pInParams->DenoiseLevel==-1)
    {
        filtersDisabled.push_back(MFX_EXTBUFF_VPP_DENOISE); // turn off denoising (on by default)
    }
    filtersDisabled.push_back(MFX_EXTBUFF_VPP_SCENE_ANALYSIS); // turn off scene analysis (on by default)

    m_VppDoNotUse.NumAlg = (mfxU32)filtersDisabled.size();

    if(m_VppDoNotUse.AlgList)
        delete[] m_VppDoNotUse.AlgList;
    m_VppDoNotUse.AlgList = new mfxU32 [m_VppDoNotUse.NumAlg];
    MSDK_CHECK_POINTER(m_VppDoNotUse.AlgList,  MFX_ERR_MEMORY_ALLOC);
    MSDK_MEMCPY(m_VppDoNotUse.AlgList,&filtersDisabled[0],sizeof(mfxU32)*filtersDisabled.size());

    return MFX_ERR_NONE;

} // CTranscodingPipeline::AllocAndInitVppDoNotUse()

void CTranscodingPipeline::FreeVppDoNotUse()
{
    MSDK_SAFE_DELETE_ARRAY(m_VppDoNotUse.AlgList);
}

mfxStatus CTranscodingPipeline::AllocateSufficientBuffer(mfxBitstream* pBS)
{
    MSDK_CHECK_POINTER(pBS, MFX_ERR_NULL_PTR);

    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    mfxStatus sts = m_pmfxENC->GetVideoParam(&par);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->GetVideoParam failed");

    mfxU32 new_size = 0;

    // if encoder provided us information about buffer size
    if (0 != par.mfx.BufferSizeInKB)
    {
        new_size = par.mfx.BufferSizeInKB * 1000;
    }
    else
    {
        // trying to guess the size (e.g. for JPEG encoder)
        new_size = (0 == pBS->MaxLength)
            // some heuristic init value
            ? 4 + (par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height * 3 + 1023)
            // double existing size
            : 2 * pBS->MaxLength;
    }

    sts = ExtendMfxBitstream(pBS, new_size);
    MSDK_CHECK_STATUS_SAFE(sts, "ExtendMfxBitstream failed", WipeMfxBitstream(pBS));

    return MFX_ERR_NONE;
} // CTranscodingPipeline::AllocateSufficientBuffer(mfxBitstream* pBS)

mfxStatus CTranscodingPipeline::Join(MFXVideoSession *pChildSession)
{
    mfxStatus sts = MFX_ERR_NONE;
    MSDK_CHECK_POINTER(pChildSession, MFX_ERR_NULL_PTR);
    sts = m_pmfxSession->JoinSession(*pChildSession);
    m_bIsJoinSession = (MFX_ERR_NONE == sts);
    return sts;
} // CTranscodingPipeline::Join(MFXVideoSession *pChildSession)

mfxStatus CTranscodingPipeline::Run()
{
    if (m_bDecodeEnable && m_bEncodeEnable)
        return Transcode();
    else if (m_bDecodeEnable)
        return Decode();
    else if (m_bEncodeEnable)
        return Encode();
    else
        return MFX_ERR_UNSUPPORTED;
}

void IncreaseReference(mfxFrameData *ptr)
{
    msdk_atomic_inc16((volatile mfxU16 *)(&ptr->Locked));
}

void DecreaseReference(mfxFrameData *ptr)
{
    msdk_atomic_dec16((volatile mfxU16 *)&ptr->Locked);
}

SafetySurfaceBuffer::SafetySurfaceBuffer(SafetySurfaceBuffer *pNext)
    :m_pNext(pNext),
     m_IsBufferingAllowed(true)
{
    mfxStatus sts=MFX_ERR_NONE;
    pRelEvent = new MSDKEvent(sts,false,false);
    MSDK_CHECK_POINTER_NO_RET(pRelEvent);

    pInsEvent = new MSDKEvent(sts,false,false);
    MSDK_CHECK_POINTER_NO_RET(pInsEvent);

} // SafetySurfaceBuffer::SafetySurfaceBuffer

SafetySurfaceBuffer::~SafetySurfaceBuffer()
{
    delete pRelEvent;
    delete pInsEvent;
} //SafetySurfaceBuffer::~SafetySurfaceBuffer()

mfxU32 SafetySurfaceBuffer::GetLength()
{
    AutomaticMutex guard(m_mutex);
    return (mfxU32)m_SList.size();
}

mfxStatus SafetySurfaceBuffer::WaitForSurfaceRelease(mfxU32 msec)
{
    return pRelEvent->TimedWait(msec);
}

mfxStatus SafetySurfaceBuffer::WaitForSurfaceInsertion(mfxU32 msec)
{
    return pInsEvent->TimedWait(msec);
}

void SafetySurfaceBuffer::AddSurface(ExtendedSurface Surf)
{
    bool isBufferingAllowed = false;

    {
        AutomaticMutex  guard(m_mutex);

        isBufferingAllowed = m_IsBufferingAllowed;
        if (isBufferingAllowed)
        {
            SurfaceDescriptor sDescriptor;
            // Locked is used to signal when we can free surface
            sDescriptor.Locked = 1;
            sDescriptor.ExtSurface = Surf;

            if (Surf.pSurface)
            {
                IncreaseReference(&Surf.pSurface->Data);
            }

            m_SList.push_back(sDescriptor);
        }
    }

    if (isBufferingAllowed)
    {
        pInsEvent->Signal();
    }

} // SafetySurfaceBuffer::AddSurface(mfxFrameSurface1 *pSurf)

mfxStatus SafetySurfaceBuffer::GetSurface(ExtendedSurface &Surf)
{
    AutomaticMutex guard(m_mutex);

    // no ready surfaces
    if (0 == m_SList.size())
    {
        MSDK_ZERO_MEMORY(Surf)
        return MFX_ERR_MORE_SURFACE;
    }

    SurfaceDescriptor sDescriptor = m_SList.front();

    Surf = sDescriptor.ExtSurface;


    return MFX_ERR_NONE;

} // SafetySurfaceBuffer::GetSurface()

mfxStatus SafetySurfaceBuffer::ReleaseSurface(mfxFrameSurface1* pSurf)
{
    m_mutex.Lock();
    std::list<SurfaceDescriptor>::iterator it;
    for (it = m_SList.begin(); it != m_SList.end(); it++)
    {
        if (pSurf == it->ExtSurface.pSurface)
        {
            it->Locked--;
            if (it->ExtSurface.pSurface)
                DecreaseReference(&it->ExtSurface.pSurface->Data);
            if (0 == it->Locked)
            {
                m_SList.erase(it);
                m_mutex.Unlock();

                // event operation should be out of synced context
                pRelEvent->Signal();
            }
            else
            {
                m_mutex.Unlock();
            }
            return MFX_ERR_NONE;
        }
    }
    m_mutex.Unlock();
    return MFX_ERR_UNKNOWN;

} // mfxStatus SafetySurfaceBuffer::ReleaseSurface(mfxFrameSurface1* pSurf)
mfxStatus SafetySurfaceBuffer::ReleaseSurfaceAll()
{
    AutomaticMutex guard(m_mutex);

    m_SList.clear();
    m_IsBufferingAllowed = true;
    return MFX_ERR_NONE;

} // mfxStatus SafetySurfaceBuffer::ReleaseSurface(mfxFrameSurface1* pSurf)

void SafetySurfaceBuffer::CancelBuffering()
{
    AutomaticMutex guard(m_mutex);
    m_IsBufferingAllowed=false;
}

FileBitstreamProcessor::FileBitstreamProcessor()
{
    MSDK_ZERO_MEMORY(m_Bitstream);
    m_Bitstream.TimeStamp=(mfxU64)-1;
} // FileBitstreamProcessor::FileBitstreamProcessor()

FileBitstreamProcessor::~FileBitstreamProcessor()
{
    if (m_pFileReader.get())
        m_pFileReader->Close();
    if (m_pFileWriter.get())
        m_pFileWriter->Close();
    WipeMfxBitstream(&m_Bitstream);
} // FileBitstreamProcessor::~FileBitstreamProcessor()

mfxStatus FileBitstreamProcessor::SetReader(std::unique_ptr<CSmplYUVReader>& reader)
{
    m_pYUVFileReader = std::move(reader);

    return MFX_ERR_NONE;
}

mfxStatus FileBitstreamProcessor::SetReader(std::unique_ptr<CSmplBitstreamReader>& reader)
{
    m_pFileReader = std::move(reader);
    mfxStatus sts = InitMfxBitstream(&m_Bitstream, 1024 * 1024);
    MSDK_CHECK_STATUS(sts, "InitMfxBitstream failed");

    return MFX_ERR_NONE;
}

mfxStatus FileBitstreamProcessor::SetWriter(std::unique_ptr<CSmplBitstreamWriter>& writer)
{
    m_pFileWriter = std::move(writer);

    return MFX_ERR_NONE;
}

mfxStatus FileBitstreamProcessor::GetInputBitstream(mfxBitstream **pBitstream)
{
    if (!m_pFileReader.get())
    {
        return MFX_ERR_UNSUPPORTED;
    }
    mfxStatus sts = m_pFileReader->ReadNextFrame(&m_Bitstream);
    if (MFX_ERR_NONE == sts)
    {
        *pBitstream = &m_Bitstream;
        return sts;
    }
    return sts;

} //  FileBitstreamProcessor::GetInputBitstream(mfxBitstream* pBitstream)

mfxStatus FileBitstreamProcessor::GetInputFrame(mfxFrameSurface1 * pSurface)
{
    //MSDK_CHECK_POINTER(pSurface);
    if (!m_pYUVFileReader.get())
    {
        return MFX_ERR_UNSUPPORTED;
    }
    return m_pYUVFileReader->LoadNextFrame(pSurface);
}

mfxStatus FileBitstreamProcessor::ProcessOutputBitstream(mfxBitstream* pBitstream)
{
    if (m_pFileWriter.get())
        return m_pFileWriter->WriteNextFrame(pBitstream, false);
    else
        return MFX_ERR_NONE;

} // mfxStatus FileBitstreamProcessor::ProcessOutputBitstream(mfxBitstream* pBitstream)

mfxStatus FileBitstreamProcessor::ResetInput()
{
    if (m_pFileReader.get())
    {
        m_pFileReader->Reset();
    }
    if (m_pYUVFileReader.get())
    {
        m_pYUVFileReader->Reset();
    }
    return MFX_ERR_NONE;
}

mfxStatus FileBitstreamProcessor::ResetOutput()
{
    if (m_pFileWriter.get())
    {
        m_pFileWriter.reset();
    }
    return MFX_ERR_NONE;
}
