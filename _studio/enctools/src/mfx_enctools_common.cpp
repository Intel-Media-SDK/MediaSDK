// Copyright (c) 2019-2020 Intel Corporation
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

#include "mfx_enctools.h"
#include <algorithm>
#include <math.h>

mfxExtBuffer* Et_GetExtBuffer(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id)
{
    if (extBuf != 0)
    {
        for (mfxU16 i = 0; i < numExtBuf; i++)
        {
            if (extBuf[i] != 0 && extBuf[i]->BufferId == id) // assuming aligned buffers
                return (extBuf[i]);
        }
    }

    return 0;
}
mfxStatus InitCtrl(mfxVideoParam const & par, mfxEncToolsCtrl *ctrl)
{
    MFX_CHECK_NULL_PTR1(ctrl);

    mfxExtCodingOption *CO = (mfxExtCodingOption *)Et_GetExtBuffer(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_CODING_OPTION);
    mfxExtCodingOption2 *CO2 = (mfxExtCodingOption2 *)Et_GetExtBuffer(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_CODING_OPTION2);
    mfxExtCodingOption3 *CO3 = (mfxExtCodingOption3 *)Et_GetExtBuffer(par.ExtParam, par.NumExtParam, MFX_EXTBUFF_CODING_OPTION3);
    MFX_CHECK_NULL_PTR3(CO, CO2, CO3);

    *ctrl = {};

    ctrl->CodecId = par.mfx.CodecId;
    ctrl->CodecProfile = par.mfx.CodecProfile;
    ctrl->CodecLevel = par.mfx.CodecLevel;

    ctrl->FrameInfo = par.mfx.FrameInfo;
    ctrl->IOPattern = par.IOPattern;
    ctrl->MaxDelayInFrames = CO2->LookAheadDepth;

    ctrl->MaxGopSize = par.mfx.GopPicSize;
    ctrl->MaxGopRefDist = par.mfx.GopRefDist;
    ctrl->MaxIDRDist = par.mfx.GopPicSize * (par.mfx.IdrInterval + 1);
    ctrl->BRefType = CO2->BRefType;

    ctrl->ScenarioInfo = CO3->ScenarioInfo;

    // Rate control info
    mfxU32 mult = par.mfx.BRCParamMultiplier ? par.mfx.BRCParamMultiplier : 1;
    bool   BRC = (par.mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
        par.mfx.RateControlMethod == MFX_RATECONTROL_VBR);

    ctrl->RateControlMethod = par.mfx.RateControlMethod;  //CBR, VBR, CRF,CQP


    if (!BRC)
    {
        ctrl->QPLevel[0] = par.mfx.QPI;
        ctrl->QPLevel[1] = par.mfx.QPP;
        ctrl->QPLevel[2] = par.mfx.QPB;
    }

    else
    {
        ctrl->TargetKbps = par.mfx.TargetKbps*mult;
        ctrl->MaxKbps = par.mfx.MaxKbps*mult;

        ctrl->HRDConformance = MFX_BRC_NO_HRD;
        if (!IsOff(CO->NalHrdConformance) && !IsOff(CO->VuiNalHrdParameters))
            ctrl->HRDConformance = MFX_BRC_HRD_STRONG;
        else if (IsOn(CO->NalHrdConformance) && IsOff(CO->VuiNalHrdParameters))
            ctrl->HRDConformance = MFX_BRC_HRD_WEAK;


        if (ctrl->HRDConformance)
        {
            ctrl->BufferSizeInKB = par.mfx.BufferSizeInKB*mult;      //if HRDConformance is ON
            ctrl->InitialDelayInKB = par.mfx.InitialDelayInKB*mult;    //if HRDConformance is ON
        }
        else
        {
            ctrl->ConvergencePeriod = 0;     //if HRDConformance is OFF, 0 - the period is whole stream,
            ctrl->Accuracy = 10;              //if HRDConformance is OFF
        }
        ctrl->WinBRCMaxAvgKbps = CO3->WinBRCMaxAvgKbps*mult;
        ctrl->WinBRCSize = CO3->WinBRCSize;
        ctrl->MaxFrameSizeInBytes[0] = CO3->MaxFrameSizeI ? CO3->MaxFrameSizeI : CO2->MaxFrameSize;     // MaxFrameSize limitation
        ctrl->MaxFrameSizeInBytes[1] = CO3->MaxFrameSizeP ? CO3->MaxFrameSizeP : CO2->MaxFrameSize;
        ctrl->MaxFrameSizeInBytes[2] = CO2->MaxFrameSize;

        ctrl->MinQPLevel[0] = CO2->MinQPI;       //QP range  limitations
        ctrl->MinQPLevel[1] = CO2->MinQPP;
        ctrl->MinQPLevel[2] = CO2->MinQPB;

        ctrl->MaxQPLevel[0] = CO2->MaxQPI;       //QP range limitations
        ctrl->MaxQPLevel[1] = CO2->MaxQPP;
        ctrl->MaxQPLevel[2] = CO2->MaxQPB;

        ctrl->PanicMode = CO3->BRCPanicMode;

    }
    return MFX_ERR_NONE;

}

inline void SetToolsStatus(mfxExtEncToolsConfig* conf, bool bOn)
{
    conf->SceneChange =
        conf->AdaptiveI =
        conf->AdaptiveB =
        conf->AdaptiveRefP =
        conf->AdaptiveRefB =
        conf->AdaptiveLTR =
        conf->AdaptivePyramidQuantP =
        conf->AdaptivePyramidQuantB =
        conf->AdaptiveQuantMatrices =
        conf->BRCBufferHints =
        conf->BRC = mfxU16(bOn ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);
}

inline void CopyPreEncSCTools(mfxExtEncToolsConfig const & confIn, mfxExtEncToolsConfig* confOut)
{
    confOut->AdaptiveI = confIn.AdaptiveI;
    confOut->AdaptiveB = confIn.AdaptiveB;
    confOut->AdaptiveRefP = confIn.AdaptiveRefP;
    confOut->AdaptiveRefB = confIn.AdaptiveRefB;
    confOut->AdaptiveLTR = confIn.AdaptiveLTR;
    confOut->AdaptivePyramidQuantP = confIn.AdaptivePyramidQuantP;
    confOut->AdaptivePyramidQuantB = confIn.AdaptivePyramidQuantB;
}

inline void OffPreEncSCDTools(mfxExtEncToolsConfig* conf)
{
    mfxExtEncToolsConfig confIn = {};
    SetToolsStatus(&confIn, false);
    CopyPreEncSCTools(confIn, conf);
}

inline void CopyPreEncLATools(mfxExtEncToolsConfig const & confIn, mfxExtEncToolsConfig* confOut)
{
    confOut->AdaptiveQuantMatrices = confIn.AdaptiveQuantMatrices;
    confOut->BRCBufferHints = confIn.BRCBufferHints;
    confOut->AdaptivePyramidQuantP = confIn.AdaptivePyramidQuantP;
}

inline void OffPreEncLATools(mfxExtEncToolsConfig* conf)
{
    mfxExtEncToolsConfig confIn = {};
    SetToolsStatus(&confIn, false);
    CopyPreEncLATools(confIn, conf);
}

inline bool isPreEncSCD(mfxExtEncToolsConfig const & conf, mfxEncToolsCtrl const & ctrl)
{
#ifdef MFX_ENABLE_AENC
    return ((IsOn(conf.AdaptiveI) ||
        IsOn(conf.AdaptiveB) ||
        IsOn(conf.AdaptiveRefP) ||
        IsOn(conf.AdaptiveRefB) ||
        IsOn(conf.AdaptiveLTR) ||
        IsOn(conf.AdaptivePyramidQuantP) ||
        IsOn(conf.AdaptivePyramidQuantB)) && ctrl.ScenarioInfo != MFX_SCENARIO_GAME_STREAMING);
#else
    return false;
#endif
}
inline bool isPreEncLA(mfxExtEncToolsConfig const & conf, mfxEncToolsCtrl const & ctrl)
{
    return (
        ctrl.ScenarioInfo == MFX_SCENARIO_GAME_STREAMING &&
        (IsOn(conf.AdaptiveQuantMatrices) ||
         IsOn(conf.BRCBufferHints) ||
         IsOn(conf.AdaptivePyramidQuantP)));
}

mfxStatus EncTools::GetSupportedConfig(mfxExtEncToolsConfig* config, mfxEncToolsCtrl const * ctrl)
{
    MFX_CHECK_NULL_PTR2(config, ctrl);
    SetToolsStatus(config, false);
    config->BRC = (mfxU16)((ctrl->RateControlMethod == MFX_RATECONTROL_CBR ||
                   ctrl->RateControlMethod == MFX_RATECONTROL_VBR)?
        MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);

    if (ctrl->ScenarioInfo != MFX_SCENARIO_GAME_STREAMING)
    {
#ifdef MFX_ENABLE_AENC
        if (ctrl->MaxGopRefDist == 8 ||
            ctrl->MaxGopRefDist == 4 ||
            ctrl->MaxGopRefDist == 2 ||
            ctrl->MaxGopRefDist == 1)
        {
            config->SceneChange = MFX_CODINGOPTION_ON;
            config->AdaptiveI = MFX_CODINGOPTION_ON;
            config->AdaptiveB = MFX_CODINGOPTION_ON;
            config->AdaptiveRefP = MFX_CODINGOPTION_ON;
            config->AdaptiveRefB = MFX_CODINGOPTION_ON;
            config->AdaptiveLTR = MFX_CODINGOPTION_ON;
            config->AdaptivePyramidQuantP = MFX_CODINGOPTION_ON;
            config->AdaptivePyramidQuantB = MFX_CODINGOPTION_ON;
        }
#endif
    }
#if defined (MFX_ENABLE_ENCTOOLS_LPLA)
    else
    {
        config->AdaptiveQuantMatrices = MFX_CODINGOPTION_ON;
        config->BRCBufferHints = MFX_CODINGOPTION_ON;
        config->SceneChange = MFX_CODINGOPTION_ON;
        config->AdaptivePyramidQuantP = MFX_CODINGOPTION_ON;
        config->AdaptiveI = MFX_CODINGOPTION_ON;
     }
#endif
    return MFX_ERR_NONE;
}
mfxStatus EncTools::GetActiveConfig(mfxExtEncToolsConfig* pConfig)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(pConfig);

    *pConfig = m_config;

    return MFX_ERR_NONE;
}

mfxStatus EncTools::GetDelayInFrames(mfxExtEncToolsConfig const * config, mfxEncToolsCtrl const * ctrl, mfxU32 *numFrames)
{
    MFX_CHECK_NULL_PTR3(config, ctrl, numFrames);
    //to fix: delay should be asked from m_scd
    *numFrames = (isPreEncSCD(*config, *ctrl)) ? 8 : 0; //

#if defined (MFX_ENABLE_ENCTOOLS_LPLA)
    if (isPreEncLA(*config, *ctrl))
    {
        *numFrames = std::max(*numFrames, (mfxU32)ctrl->MaxDelayInFrames);
    }
#endif
    return MFX_ERR_NONE;
}

mfxStatus EncTools::InitVPP(mfxEncToolsCtrl const & ctrl)
{
    MFX_CHECK(!m_bVPPInit, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(m_device && m_pAllocator, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxStatus sts;

    if (mfxSession(m_mfxSession) == 0)
    {
        mfxInitParam initPar = {};
        initPar.Version.Major = 1;
        initPar.Version.Minor = 0;
        initPar.Implementation = MFX_IMPL_HARDWARE;
        initPar.Implementation |= (m_deviceType == MFX_HANDLE_D3D11_DEVICE ? MFX_IMPL_VIA_D3D11 : (m_deviceType == MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9 ? MFX_IMPL_VIA_D3D9 : MFX_IMPL_VIA_VAAPI));
        initPar.GPUCopy = MFX_GPUCOPY_DEFAULT;

        sts = m_mfxSession.InitEx(initPar);
        MFX_CHECK_STS(sts);
    }

    //mfxVersion version;     // real API version with which library is initialized
    //sts = MFXQueryVersion(m_mfxSession, &version); // get real API version of the loaded library
    //MFX_CHECK_STS(sts);

    sts = m_mfxSession.SetFrameAllocator(m_pAllocator);
    MFX_CHECK_STS(sts);

    sts = m_mfxSession.SetHandle((mfxHandleType)m_deviceType, m_device);
    MFX_CHECK_STS(sts);

    m_pmfxVPP.reset(new MFXVideoVPP(m_mfxSession));
    MFX_CHECK(m_pmfxVPP, MFX_ERR_MEMORY_ALLOC);

    sts = InitMfxVppParams(ctrl);
    MFX_CHECK_STS(sts);

    mfxExtVPPScaling vppScalingMode = {};
    vppScalingMode.Header.BufferId = MFX_EXTBUFF_VPP_SCALING;
    vppScalingMode.Header.BufferSz = sizeof(vppScalingMode);
    vppScalingMode.ScalingMode = MFX_SCALING_MODE_LOWPOWER;
    vppScalingMode.InterpolationMethod = MFX_INTERPOLATION_NEAREST_NEIGHBOR;
    std::vector<mfxExtBuffer*> extParams;
    extParams.push_back((mfxExtBuffer *)&vppScalingMode);
    m_mfxVppParams.ExtParam = extParams.data();
    m_mfxVppParams.NumExtParam = (mfxU16)extParams.size();

    sts = m_pmfxVPP->Init(&m_mfxVppParams);
    MFX_CHECK_STS(sts);

    mfxFrameAllocRequest VppRequest[2];
    sts = m_pmfxVPP->QueryIOSurf(&m_mfxVppParams, VppRequest);
    MFX_CHECK_STS(sts);

    VppRequest[1].Type |= MFX_MEMTYPE_FROM_DECODE; // ffmpeg's qsv allocator requires MFX_MEMTYPE_FROM_DECODE or MFX_MEMTYPE_FROM_ENCODE

    sts = m_pAllocator->Alloc(m_pAllocator->pthis, &(VppRequest[1]), &m_VppResponse);
    MFX_CHECK_STS(sts);

    m_pIntSurfaces.resize(m_VppResponse.NumFrameActual);
    for (mfxU32 i = 0; i < (mfxU32)m_pIntSurfaces.size(); i++)
    {
        m_pIntSurfaces[i] = {};
        m_pIntSurfaces[i].Info = m_mfxVppParams.vpp.Out;
        m_pIntSurfaces[i].Data.MemId = m_VppResponse.mids[i];
    }

    m_bVPPInit = true;
    return MFX_ERR_NONE;

}

mfxStatus EncTools::InitMfxVppParams(mfxEncToolsCtrl const & ctrl)
{
    m_mfxVppParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    m_mfxVppParams.vpp.In = ctrl.FrameInfo;
    m_mfxVppParams.vpp.Out = m_mfxVppParams.vpp.In;

    if (isPreEncSCD(m_config, ctrl))
    {
        mfxFrameInfo frameInfo;
        mfxStatus sts = m_scd.GetInputFrameInfo(frameInfo);
        MFX_CHECK_STS(sts);
        m_mfxVppParams.vpp.Out.Width = frameInfo.Width;
        m_mfxVppParams.vpp.Out.Height = frameInfo.Height;
        m_mfxVppParams.vpp.Out.CropW = m_mfxVppParams.vpp.Out.Width;
        m_mfxVppParams.vpp.Out.CropH = m_mfxVppParams.vpp.Out.Height;
    }
    else // LPLA
    {
        mfxU16 crW = m_mfxVppParams.vpp.In.CropW ? m_mfxVppParams.vpp.In.CropW : m_mfxVppParams.vpp.In.Width;
        mfxU16 crH = m_mfxVppParams.vpp.In.CropH ? m_mfxVppParams.vpp.In.CropH : m_mfxVppParams.vpp.In.Height;

        mfxPlatform platform;
        m_mfxSession.QueryPlatform(&platform);

        if (platform.CodeName < MFX_PLATFORM_TIGERLAKE)
        {
            m_mfxVppParams.vpp.In.CropW = m_mfxVppParams.vpp.In.Width  = crW & ~0x3F;
            m_mfxVppParams.vpp.In.CropH = m_mfxVppParams.vpp.In.Height = crH & ~0x3F;
            m_mfxVppParams.vpp.Out.CropW = m_mfxVppParams.vpp.Out.Width  = m_mfxVppParams.vpp.In.CropW >> 2;
            m_mfxVppParams.vpp.Out.CropH = m_mfxVppParams.vpp.Out.Height = m_mfxVppParams.vpp.In.CropH >> 2;
        }
        else
        {
            m_mfxVppParams.vpp.Out.CropW = (crW >> 2);
            m_mfxVppParams.vpp.Out.CropH = (crH >> 2);
            m_mfxVppParams.vpp.Out.Width  = (m_mfxVppParams.vpp.Out.CropW + 15) & ~0xF;
            m_mfxVppParams.vpp.Out.Height = (m_mfxVppParams.vpp.Out.CropH + 15) & ~0xF;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus EncTools::CloseVPP()
{
    MFX_CHECK(m_bVPPInit, MFX_ERR_NOT_INITIALIZED);
    mfxStatus sts;

    if (m_pAllocator)
    {
        m_pAllocator->Free(m_pAllocator->pthis, &m_VppResponse);
        m_pAllocator = nullptr;
    }
    if (m_pIntSurfaces.size())
        m_pIntSurfaces.clear();
    if (m_pmfxVPP)
    {
        m_pmfxVPP->Close();
        m_pmfxVPP.reset();
    }
    sts = m_mfxSession.Close();
    MFX_CHECK_STS(sts);

    m_bVPPInit = false;
    return sts;
}



mfxStatus EncTools::Init(mfxExtEncToolsConfig const * pConfig, mfxEncToolsCtrl const * ctrl)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR2(pConfig, ctrl);
    MFX_CHECK(!m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);

    m_ctrl = *ctrl;
    SetToolsStatus(&m_config, false);
    if (IsOn(pConfig->BRC))
    {
        sts = m_brc.Init(*ctrl);
        MFX_CHECK_STS(sts);
        m_config.BRC = MFX_CODINGOPTION_ON;
    }
    if (isPreEncSCD(*pConfig, *ctrl))
    {
        sts = m_scd.Init(*ctrl, *pConfig);
        MFX_CHECK_STS(sts);
        // to add request to m_scd about supoorted tools
        CopyPreEncSCTools(*pConfig, &m_config);
    }

    mfxU16 crW = ctrl->FrameInfo.CropW ? ctrl->FrameInfo.CropW : ctrl->FrameInfo.Width;
    if ((isPreEncSCD(m_config, *ctrl) || (isPreEncLA(m_config, *ctrl) && crW >= 720)) && (ctrl->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY))
    {
        mfxEncToolsCtrlExtDevice *extDevice = (mfxEncToolsCtrlExtDevice *)Et_GetExtBuffer(ctrl->ExtParam, ctrl->NumExtParam, MFX_EXTBUFF_ENCTOOLS_DEVICE);
        if (extDevice)
        {
            m_device = extDevice->DeviceHdl;
            m_deviceType = extDevice->HdlType;
        }
        mfxEncToolsCtrlExtAllocator *extAlloc = (mfxEncToolsCtrlExtAllocator *)Et_GetExtBuffer(ctrl->ExtParam, ctrl->NumExtParam, MFX_EXTBUFF_ENCTOOLS_ALLOCATOR);
        if (extAlloc)
            m_pAllocator = extAlloc->pAllocator;

        if (!m_device || !m_pAllocator)
        {
            m_scd.Close();
            OffPreEncSCDTools(&m_config);
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        sts = InitVPP(*ctrl);
        MFX_CHECK_STS(sts);
    }

    m_bInit = true;
    return sts;
}

mfxStatus EncTools::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    if (IsOn(m_config.BRC))
    {
        m_brc.Close();
        m_config.BRC = false;
    }
    if (isPreEncSCD(m_config, m_ctrl))
    {
        m_scd.Close();
        OffPreEncSCDTools(&m_config);
    }

    if (m_bVPPInit)
        sts = CloseVPP();


    m_bInit = false;
    return sts;
}

mfxStatus EncTools::Reset(mfxExtEncToolsConfig const * config, mfxEncToolsCtrl const * ctrl)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR2(config,ctrl);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    if (IsOn(config->BRC))
    {
        MFX_CHECK(m_config.BRC, MFX_ERR_UNSUPPORTED);
        sts = m_brc.Reset(*ctrl);
    }
    if (isPreEncSCD(*config, *ctrl))
    {
        // to add check if Close/Init is real needed
        if (isPreEncSCD(m_config, m_ctrl))
            m_scd.Close();
        sts = m_scd.Init(*ctrl, *config);
    }

    return sts;
}

#define MSDK_VPP_WAIT_INTERVAL 300000

mfxStatus EncTools::VPPDownScaleSurface(mfxFrameSurface1 *pInSurface, mfxFrameSurface1 *pOutSurface)
{
    mfxStatus sts;
    MFX_CHECK_NULL_PTR2(pInSurface, pOutSurface);

    mfxSyncPoint vppSyncp;
    sts = m_pmfxVPP->RunFrameVPPAsync(pInSurface, pOutSurface, NULL, &vppSyncp);
    MFX_CHECK_STS(sts);
    sts = m_mfxSession.SyncOperation(vppSyncp, MSDK_VPP_WAIT_INTERVAL);
    MFX_CHECK_STS(sts);

    return MFX_ERR_NONE;
}

mfxStatus EncTools::Submit(mfxEncToolsTaskParam const * par)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    mfxEncToolsFrameToAnalyze  *pFrameData = (mfxEncToolsFrameToAnalyze *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_FRAME_TO_ANALYZE);

    if (pFrameData)
    {
        pFrameData->Surface->Data.FrameOrder = par->DisplayOrder;

        if (m_bVPPInit && (m_ctrl.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY) && (isPreEncSCD(m_config, m_ctrl) || isPreEncLA(m_config, m_ctrl)))
        {
            sts = VPPDownScaleSurface(pFrameData->Surface, m_pIntSurfaces.data());
            MFX_CHECK_STS(sts);
            m_pIntSurfaces[0].Data.FrameOrder = pFrameData->Surface->Data.FrameOrder;

            if (isPreEncSCD(m_config, m_ctrl))
            {
                m_pAllocator->Lock(m_pAllocator->pthis, m_pIntSurfaces[0].Data.MemId, &m_pIntSurfaces[0].Data);
                sts = m_scd.SubmitFrame(m_pIntSurfaces.data());
                m_pAllocator->Unlock(m_pAllocator->pthis, m_pIntSurfaces[0].Data.MemId, &m_pIntSurfaces[0].Data);
            }
        }
        else if (isPreEncSCD(m_config, m_ctrl))
            sts = m_scd.SubmitFrame(pFrameData->Surface);
        return sts;
    }

    mfxEncToolsBRCEncodeResult  *pEncRes = (mfxEncToolsBRCEncodeResult *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_ENCODE_RESULT);
    if (pEncRes) {
        m_scd.ReportEncResult(par->DisplayOrder, *pEncRes);
    }
    if (pEncRes && IsOn(m_config.BRC))
    {
        return m_brc.ReportEncResult(par->DisplayOrder, *pEncRes);
    }

    mfxEncToolsBRCFrameParams  *pFrameStruct = (mfxEncToolsBRCFrameParams *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_FRAME_PARAM );
    if (pFrameStruct && IsOn(m_config.BRC))
    {
        sts = m_brc.SetFrameStruct(par->DisplayOrder, *pFrameStruct);
        MFX_CHECK_STS(sts);
    }

    mfxEncToolsBRCBufferHint  *pBRCHints = (mfxEncToolsBRCBufferHint *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_BUFFER_HINT);
    if (pBRCHints && IsOn(m_config.BRC))
    {
        return m_brc.ReportBufferHints(par->DisplayOrder, *pBRCHints);
    }

    mfxEncToolsHintPreEncodeGOP *pPreEncGOP = (mfxEncToolsHintPreEncodeGOP *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_HINT_GOP);
    if (pPreEncGOP && IsOn(m_config.BRC))
    {
        return m_brc.ReportGopHints(par->DisplayOrder, *pPreEncGOP);
    }

    return sts;
}

mfxStatus EncTools::Query(mfxEncToolsTaskParam* par, mfxU32 /*timeOut*/)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxEncToolsHintPreEncodeSceneChange *pPreEncSC = (mfxEncToolsHintPreEncodeSceneChange *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_HINT_SCENE_CHANGE);
    if (pPreEncSC)
    {
        sts = m_scd.GetSCDecision(par->DisplayOrder, pPreEncSC);
        MFX_CHECK_STS(sts);
    }
    mfxEncToolsHintPreEncodeGOP *pPreEncGOP = (mfxEncToolsHintPreEncodeGOP *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_HINT_GOP);
    if (pPreEncGOP)
    {
        if (isPreEncSCD(m_config, m_ctrl))
        {
            sts = m_scd.GetGOPDecision(par->DisplayOrder, pPreEncGOP);
            MFX_CHECK_STS(sts);
        }
    }
    mfxEncToolsHintPreEncodeARefFrames *pPreEncARef = (mfxEncToolsHintPreEncodeARefFrames *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_HINT_AREF);
    if (pPreEncARef)
    {
        sts = m_scd.GetARefDecision(par->DisplayOrder, pPreEncARef);
        MFX_CHECK_STS(sts);
    }

    mfxEncToolsBRCStatus  *pFrameSts = (mfxEncToolsBRCStatus *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_STATUS);
    if (pFrameSts && IsOn(m_config.BRC))
    {
        return m_brc.UpdateFrame(par->DisplayOrder, pFrameSts);
    }

    mfxEncToolsBRCQuantControl *pFrameQp = (mfxEncToolsBRCQuantControl *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_QUANT_CONTROL);
    if (pFrameQp && IsOn(m_config.BRC))
    {
        sts = m_brc.ProcessFrame(par->DisplayOrder, pFrameQp);
        MFX_CHECK_STS(sts);
    }
    mfxEncToolsBRCHRDPos *pHRDPos = (mfxEncToolsBRCHRDPos *)Et_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCTOOLS_BRC_HRD_POS);
    if (pHRDPos && IsOn(m_config.BRC))
    {
        sts = m_brc.GetHRDPos(par->DisplayOrder, pHRDPos);
        MFX_CHECK_STS(sts);
    }

    return sts;
}


mfxStatus EncTools::Discard(mfxU32 displayOrder)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_scd.CompleteFrame(displayOrder);
    return sts;
}


