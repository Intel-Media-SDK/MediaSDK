// Copyright (c) 2009-2020 Intel Corporation
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

#ifdef MFX_ENABLE_ENCTOOLS

mfxStatus cBRCParams::Init(mfxEncToolsCtrl const & ctrl, bool fieldMode)
{
    MFX_CHECK(ctrl.RateControlMethod == MFX_RATECONTROL_CBR ||
              ctrl.RateControlMethod == MFX_RATECONTROL_VBR,
              MFX_ERR_UNDEFINED_BEHAVIOR);

    bFieldMode= fieldMode;
    codecId   = ctrl.CodecId;
    targetbps = ctrl.TargetKbps * 1000;
    maxbps    = ctrl.MaxKbps * 1000;

    maxbps = (ctrl.RateControlMethod == MFX_RATECONTROL_CBR) ?
        targetbps : ((maxbps >= targetbps) ? maxbps : targetbps);

    mfxU32 bit_rate_scale = (ctrl.CodecId == MFX_CODEC_AVC) ?
        h264_bit_rate_scale : hevcBitRateScale(maxbps);
    mfxU32 cpb_size_scale = (ctrl.CodecId == MFX_CODEC_AVC) ?
        h264_cpb_size_scale : hevcCbpSizeScale(maxbps);

    rateControlMethod  = ctrl.RateControlMethod;
    maxbps =    ((maxbps >> (6 + bit_rate_scale)) << (6 + bit_rate_scale));

    HRDConformance = ctrl.HRDConformance;

   if (HRDConformance != MFX_BRC_NO_HRD)
    {
        bufferSizeInBytes  = ((ctrl.BufferSizeInKB*1000) >> (cpb_size_scale + 1)) << (cpb_size_scale + 1);
        initialDelayInBytes =((ctrl.InitialDelayInKB*1000) >> (cpb_size_scale + 1)) << (cpb_size_scale + 1);
        bRec = 1;
        bPanic = (HRDConformance == MFX_BRC_HRD_STRONG) ? 1 : 0;
    }
    MFX_CHECK (ctrl.FrameInfo.FrameRateExtD != 0 &&
               ctrl.FrameInfo.FrameRateExtN != 0,
               MFX_ERR_UNDEFINED_BEHAVIOR);

    frameRate = (mfxF64)ctrl.FrameInfo.FrameRateExtN / (mfxF64)ctrl.FrameInfo.FrameRateExtD;

    width  = ctrl.FrameInfo.Width;
    height = ctrl.FrameInfo.Height;

    chromaFormat = ctrl.FrameInfo.ChromaFormat == 0 ?  MFX_CHROMAFORMAT_YUV420 : ctrl.FrameInfo.ChromaFormat ;
    bitDepthLuma = ctrl.FrameInfo.BitDepthLuma == 0 ?  8 : ctrl.FrameInfo.BitDepthLuma;

    quantOffset   = 6 * (bitDepthLuma - 8);

    inputBitsPerFrame    = targetbps / frameRate;
    maxInputBitsPerFrame = maxbps / frameRate;
    gopPicSize = ctrl.MaxGopSize*(bFieldMode ? 2 : 1);
    gopRefDist = ctrl.MaxGopRefDist*(bFieldMode ? 2 : 1);

    bPyr = (ctrl.BRefType == MFX_B_REF_PYRAMID);
    maxFrameSizeInBits  = std::max(std::max (ctrl.MaxFrameSizeInBytes[0], ctrl.MaxFrameSizeInBytes[1]),
        ctrl.MaxFrameSizeInBytes[2])*8 ;

    fAbPeriodLong = 120;
    if (gopRefDist <= 3) {
        fAbPeriodShort = 6;
    } else {
        fAbPeriodShort = 16;
    }
    dqAbPeriod = 120;
    bAbPeriod = 120;

    if (maxFrameSizeInBits)
    {
        bRec = 1;
        bPanic = 1;
    }
    quantMinI = (ctrl.MinQPLevel[0] != 0) ?
        ctrl.MinQPLevel[0] : 1;
    quantMaxI = (ctrl.MaxQPLevel[0] != 0) ?
        ctrl.MaxQPLevel[0] : 51 + quantOffset;

    quantMinP = (ctrl.MinQPLevel[1] != 0) ?
        ctrl.MinQPLevel[1] : 1;
    quantMaxP = (ctrl.MaxQPLevel[1] != 0) ?
        ctrl.MaxQPLevel[1] : 51 + quantOffset;

    quantMinB = (ctrl.MinQPLevel[2] != 0) ?
        ctrl.MinQPLevel[2] : 1;
    quantMaxB = (ctrl.MaxQPLevel[2] != 0) ?
        ctrl.MaxQPLevel[2] : 51 + quantOffset;

    WinBRCMaxAvgKbps = ctrl.WinBRCMaxAvgKbps;
    WinBRCSize = ctrl.WinBRCSize;


    mRawFrameSizeInBits = GetRawFrameSize(width * height, chromaFormat, bitDepthLuma);
    mRawFrameSizeInPixs = mRawFrameSizeInBits / bitDepthLuma;
    iDQp0 = 0;

    mNumRefsInGop = (mfxU32)(std::max(1.0, (!bPyr ? (mfxF64)gopPicSize / (mfxF64)gopRefDist : (mfxF64)gopPicSize / 2.0)));

    mfxF64 maxFrameRatio = 1.5874 * BRC_FRM_RATIO(MFX_FRAMETYPE_IDR, 0, 0, bPyr);

    mIntraBoost = (mNumRefsInGop > maxFrameRatio * 8.0) ? 1 : 0;

    mfxF64 maxFrameSize = mRawFrameSizeInBits;
    if (maxFrameSizeInBits) {
        maxFrameSize = std::min<mfxF64>(maxFrameSize, maxFrameSizeInBits);
    }
    if (HRDConformance != MFX_BRC_NO_HRD) {
        mfxF64 bufOccupy = BRC_LTR_BUF(MFX_FRAMETYPE_IDR, 1, mIntraBoost, 1, 0);
        maxFrameSize = std::min(maxFrameSize, bufOccupy / 9.* (initialDelayInBytes * 8.0) + (9.0 - bufOccupy) / 9.*inputBitsPerFrame);
    }

    mfxF64 minFrameRatio = BRC_FRM_RATIO(MFX_FRAMETYPE_IDR, 0, 0, bPyr);
    maxFrameRatio = std::min({maxFrameRatio, maxFrameSize / inputBitsPerFrame, mfxF64(mNumRefsInGop)});
    mfxF64 dqp = std::max(0.0, 6.0 * (log(maxFrameRatio / minFrameRatio) / log(2.0)));
    iDQp0 = (mfxU32)(dqp + 0.5);
    if (iDQp0 < 1) iDQp0 = 1;
    if (iDQp0 > BRC_MAX_DQP_LTR) iDQp0 = BRC_MAX_DQP_LTR;

    // MaxFrameSize violation prevention
    mMinQstepCmplxKP = BRC_CONST_MUL_P1;
    mMinQstepRateEP = BRC_CONST_EXP_R_P1;
    mMinQstepCmplxKPUpdt = 0;
    mMinQstepCmplxKPUpdtErr = 0.16;
    return MFX_ERR_NONE;
}

mfxStatus cBRCParams::GetBRCResetType(mfxEncToolsCtrl const &  ctrl,  bool bNewSequence, bool &bBRCReset, bool &bSlidingWindowReset)
{
    bBRCReset = false;
    bSlidingWindowReset = false;

    if (bNewSequence)
        return MFX_ERR_NONE;

    cBRCParams new_par;
    mfxStatus sts = new_par.Init(ctrl);
    MFX_CHECK_STS(sts);

    MFX_CHECK(new_par.rateControlMethod == rateControlMethod, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM) ;
    MFX_CHECK(new_par.HRDConformance == HRDConformance, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM) ;
    MFX_CHECK(new_par.frameRate == frameRate, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.width == width, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.height == height, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.chromaFormat == chromaFormat, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.bitDepthLuma == bitDepthLuma, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    if (HRDConformance == MFX_BRC_HRD_STRONG)
    {
        MFX_CHECK(new_par.bufferSizeInBytes == bufferSizeInBytes, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.initialDelayInBytes == initialDelayInBytes, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.targetbps == targetbps, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.maxbps == maxbps, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }
    else if (new_par.targetbps != targetbps || new_par.maxbps != maxbps)
    {
        bBRCReset = true;
    }

    if (new_par.WinBRCMaxAvgKbps != WinBRCMaxAvgKbps)
    {
        bBRCReset = true;
        bSlidingWindowReset = true;
    }

    if (new_par.maxFrameSizeInBits != maxFrameSizeInBits) bBRCReset = true;
    if (new_par.gopPicSize != gopPicSize) bBRCReset = true;
    if (new_par.gopRefDist != gopRefDist) bBRCReset = true;
    if (new_par.bPyr != bPyr) bBRCReset = true;
    if (new_par.quantMaxI != quantMaxI) bBRCReset = true;
    if (new_par.quantMinI != quantMinI) bBRCReset = true;
    if (new_par.quantMaxP != quantMaxP) bBRCReset = true;
    if (new_par.quantMinP != quantMinP) bBRCReset = true;
    if (new_par.quantMaxB != quantMaxB) bBRCReset = true;
    if (new_par.quantMinB != quantMinB) bBRCReset = true;

    return MFX_ERR_NONE;
}


inline mfxI32 GetOffsetAPQ(mfxI32 level, mfxU16 isRef, mfxU16 qpMod)
{
    mfxI32 qp = 0;
    level = std::max(mfxI32(1), std::min(mfxI32(3), level));
    if (qpMod == MFX_QP_MODULATION_HIGH) {
        switch (level) {
        case 3:
            qp += 2;
        case 2:
            qp += 1;
        case 1:
        default:
            qp += 2;
            break;
        }
    }
    else if (qpMod == MFX_QP_MODULATION_MEDIUM) {
        switch (level) {
        case 3:
            qp += 2;
        case 2:
            qp += 1;
        case 1:
        default:
            qp += 1;
            break;
        }
    }
    else if (qpMod == MFX_QP_MODULATION_LOW) {
        switch (level) {
        case 3:
            qp += 1;
        case 2:
            qp += 1;
        case 1:
        default:
            qp -= 1;
            break;
        }
    }
    else {
        qp += (level > 0 ? level - 1 : 0);
        if (level && !isRef) qp += 1;
    }
    return qp;
}

mfxI32 BRC_EncTool::GetCurQP (mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const
{
    mfxI32 qp = 0;
    if (type == MFX_FRAMETYPE_IDR)
    {
        qp = m_ctx.QuantIDR;
        qp = mfx::clamp(qp, m_par.quantMinI, m_par.quantMaxI);
    }
    else if (type == MFX_FRAMETYPE_I)
    {
        qp = m_ctx.QuantI;
        qp = mfx::clamp(qp, m_par.quantMinI, m_par.quantMaxI);
    }
    else if (type == MFX_FRAMETYPE_P)
    {
        qp = m_ctx.QuantP + layer;
        qp = mfx::clamp(qp, m_par.quantMinP, m_par.quantMaxP);
    }
    else
    {
        qp = m_ctx.QuantB;
        qp += GetOffsetAPQ(layer, isRef, qpMod);
        qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }
    //printf("GetCurQP IDR %d I %d P %d B %d, min %d max %d type %d \n", m_ctx.QuantIDR, m_ctx.QuantI, m_ctx.QuantP, m_ctx.QuantB, m_par.quantMinI, m_par.quantMaxI, type);

    return qp;
}

mfxF64 BRC_EncTool::ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong, mfxU32 eo, bool bIdr, mfxU16 qpMod) const
{
    mfxI32 seqQP_new = GetSeqQP(qp, type, layer, isRef, qpMod);
    mfxF64 dQuantAb_new = 1.0 / seqQP_new;
    mfxF64 bAbPreriod = m_par.bAbPeriod;

    mfxF64 totDev = m_ctx.totalDeviation;

    mfxF64 HRDDevFactor = 0.0;
    mfxF64 maxFrameSizeHrd = 0.0;
    mfxF64 HRDDev = 0.0;
    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        HRDDevFactor = m_hrdSpec->GetBufferDeviationFactor(eo);
        HRDDev = m_hrdSpec->GetBufferDeviation(eo);
        maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(eo, bIdr);
    }

    mfxF64 lf = 1.0 / pow(m_par.inputBitsPerFrame / fAbLong, 1.0 + HRDDevFactor);

    if (m_par.HRDConformance != MFX_BRC_NO_HRD && totDev > 0)
    {
        if (m_par.rateControlMethod == MFX_RATECONTROL_VBR)
        {
            totDev = std::max(totDev, HRDDev);
        }
        bAbPreriod = (mfxF64)(m_par.bPyr ? 4 : 3)*(mfxF64)maxFrameSizeHrd / m_par.inputBitsPerFrame*GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIDREncOrder, m_par.gopPicSize, m_ctx.LastIDRSceneChange);
        bAbPreriod = mfx::clamp(bAbPreriod, m_par.bAbPeriod / 10, m_par.bAbPeriod);
    }

    mfxI32 quant_new = GetNewQPTotal(totDev / bAbPreriod / (mfxF64)m_par.inputBitsPerFrame, dQuantAb_new, m_ctx.QuantMin, m_ctx.QuantMax, seqQP_new, m_par.bPyr && m_par.bRec, false);
    seqQP_new += (seqQP_new - quant_new);
    mfxF64 dQuantAb =  lf * (1.0 / seqQP_new);
    return dQuantAb;
}

mfxI32 BRC_EncTool::GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const
{
    mfxI32 pqp = 0;
    if (type == MFX_FRAMETYPE_IDR) {
        pqp = qp + m_par.iDQp + 1;
    } else if (type == MFX_FRAMETYPE_I) {
        pqp = qp + 1;
    } else if (type == MFX_FRAMETYPE_P) {
        pqp = qp - layer;
    } else {
        qp -= GetOffsetAPQ(layer, isRef, qpMod);
        pqp = qp - 1;
    }
    pqp = mfx::clamp(pqp, m_par.quantMinP, m_par.quantMaxP);

    return pqp;
}

mfxI32 BRC_EncTool::GetPicQP(mfxI32 pqp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const
{
    mfxI32 qp = 0;

    if (type == MFX_FRAMETYPE_IDR)
    {
        qp = pqp - 1 - m_par.iDQp;
        qp = mfx::clamp(qp, m_par.quantMinI, m_par.quantMaxI);
    }
    else if (type == MFX_FRAMETYPE_I)
    {
        qp = pqp - 1;
        qp = mfx::clamp(qp, m_par.quantMinI, m_par.quantMaxI);
    }
    else if (type == MFX_FRAMETYPE_P)
    {
        qp =pqp + layer;
        qp = mfx::clamp(qp, m_par.quantMinP, m_par.quantMaxP);
    }
    else
    {
        qp = pqp + 1;
        qp += GetOffsetAPQ(layer, isRef, qpMod);
        qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }

    return qp;
}


mfxStatus BRC_EncTool::Init(mfxEncToolsCtrl const & ctrl)
{
    MFX_CHECK(!m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_par.Init(ctrl, isFieldMode(ctrl));
    MFX_CHECK_STS(sts);

    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        if (m_par.codecId == MFX_CODEC_AVC)
            m_hrdSpec.reset(new H264_HRD());
        else
            m_hrdSpec.reset(new HEVC_HRD());
        m_hrdSpec->Init(m_par);

    }
    m_ctx = {};

    m_ctx.fAbLong = m_par.inputBitsPerFrame;
    m_ctx.fAbShort = m_par.inputBitsPerFrame;

    mfxI32 rawSize = GetRawFrameSize(m_par.width * m_par.height, m_par.chromaFormat, m_par.bitDepthLuma);
    mfxI32 qp = GetNewQP(rawSize, m_par.inputBitsPerFrame, m_par.quantMinI, m_par.quantMaxI, 1, m_par.quantOffset, 0.5, false, false);

    UpdateQPParams(qp, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, MFX_FRAMETYPE_REF, 0);

    m_ctx.dQuantAb = qp > 0 ? 1.0 / qp : 1.0; //kw

    if (m_par.WinBRCSize)
    {
        m_avg.reset(new AVGBitrate(m_par.WinBRCSize, (mfxU32)(m_par.WinBRCMaxAvgKbps*1000.0 / m_par.frameRate), (mfxU32)m_par.inputBitsPerFrame));
        MFX_CHECK_NULL_PTR1(m_avg.get());
    }

    m_bInit = true;
    return sts;
}

mfxStatus BRC_EncTool::Reset(mfxEncToolsCtrl const & ctrl)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxExtEncoderResetOption  * pRO = (mfxExtEncoderResetOption *)Et_GetExtBuffer(ctrl.ExtParam, ctrl.NumExtParam, MFX_EXTBUFF_ENCODER_RESET_OPTION);
    if (pRO && pRO->StartNewSequence == MFX_CODINGOPTION_ON)
    {
        Close();
        sts = Init(ctrl);
    }
    else
    {
        bool brcReset = false;
        bool slidingWindowReset = false;

        sts = m_par.GetBRCResetType(ctrl, false, brcReset, slidingWindowReset);
        MFX_CHECK_STS(sts);

        if (brcReset)
        {
            sts = m_par.Init(ctrl, isFieldMode(ctrl));
            MFX_CHECK_STS(sts);

            m_ctx.Quant = (mfxI32)(1. / m_ctx.dQuantAb * pow(m_ctx.fAbLong / m_par.inputBitsPerFrame, 0.32) + 0.5);
            m_ctx.Quant = mfx::clamp(m_ctx.Quant, m_par.quantMinI, m_par.quantMaxI);

            UpdateQPParams(m_ctx.Quant, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, MFX_FRAMETYPE_REF, 0);

            m_ctx.dQuantAb = 1. / m_ctx.Quant;
            m_ctx.fAbLong = m_par.inputBitsPerFrame;
            m_ctx.fAbShort = m_par.inputBitsPerFrame;

            if (slidingWindowReset)
            {
                m_avg.reset(new AVGBitrate(m_par.WinBRCSize, (mfxU32)(m_par.WinBRCMaxAvgKbps*1000.0 / m_par.frameRate), (mfxU32)m_par.inputBitsPerFrame));
                MFX_CHECK_NULL_PTR1(m_avg.get());
            }
        }
    }
    return sts;
}


mfxStatus BRC_EncTool::UpdateFrame(mfxU32 dispOrder, mfxEncToolsBRCStatus *pFrameSts)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(pFrameSts);

    auto frameStructItr = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStructItr == m_FrameStruct.end())
        return MFX_ERR_UNDEFINED_BEHAVIOR; // BRC hasn't processed the frame

    BRC_FrameStruct frameStruct = *frameStructItr;

    mfxI32 bitsEncoded = frameStruct.frameSize * 8;
    mfxI32 qpY = frameStruct.qp + m_par.quantOffset;
    mfxI32 layer = frameStruct.pyrLayer;
    mfxU32 picType = GetFrameType(frameStruct.frameType, frameStruct.pyrLayer, m_par.gopRefDist);
    mfxU16 isRef = frameStruct.frameType & MFX_FRAMETYPE_REF;
    mfxU16 isIntra = frameStruct.frameType & (MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_I);
    bool bIdr = (picType ==  MFX_FRAMETYPE_IDR);
    mfxF64 qstep = QP2Qstep(qpY, m_par.quantOffset);

#if (MFX_VERSION >= 1026)
    mfxU16 ParSceneChange = frameStruct.sceneChange;
    mfxU32 ParFrameCmplx = frameStruct.frameCmplx;
#else
    mfxU16 ParSceneChange = 0;
    mfxU32 ParFrameCmplx = 0;
#endif
    mfxU16 ParQpModulation = (mfxU16) frameStruct.qpModulation;

    mfxF64 fAbLong = m_ctx.fAbLong + (bitsEncoded - m_ctx.fAbLong) / m_par.fAbPeriodLong;
    mfxF64 fAbShort = m_ctx.fAbShort + (bitsEncoded - m_ctx.fAbShort) / m_par.fAbPeriodShort;
    mfxF64 eRate = bitsEncoded * sqrt(qstep);

    mfxF64 e2pe = 0;
    bool bMaxFrameSizeMode = m_par.maxFrameSizeInBits != 0 &&
        m_par.maxFrameSizeInBits < m_par.inputBitsPerFrame * 2 &&
        m_ctx.totalDeviation < (-1)*m_par.inputBitsPerFrame*m_par.frameRate;

    if (isIntra) {
        e2pe = (m_ctx.eRateSH == 0) ? (BRC_SCENE_CHANGE_RATIO2 + 1) : eRate / m_ctx.eRateSH;
        if (ParSceneChange && e2pe <= BRC_SCENE_CHANGE_RATIO2 && m_ctx.eRate)
            e2pe = eRate / m_ctx.eRate;
    }
    else {
        e2pe = (m_ctx.eRate == 0) ? (BRC_SCENE_CHANGE_RATIO2 + 1) : eRate / m_ctx.eRate;
    }
    mfxU32 frameSizeLim = 0xfffffff; // sliding window limitation or external frame size limitation

    bool  bSHStart = false;
    bool  bNeedUpdateQP = false;
    mfxU16 &brcSts = pFrameSts->FrameStatus.BRCStatus;

    brcSts = MFX_BRC_OK;

    if (m_par.bRec && m_ctx.bToRecode && (m_ctx.encOrder != frameStruct.encOrder)) // || frameStruct.numRecode == 0))
    {
        //printf("++++++++++++++++++++++++++++++++++\n");
        // Frame must be recoded, but encoder calls BR for another frame
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if (frameStruct.numRecode == 0)
    {
        // Set context for new frame
        if (isIntra) {
            m_ctx.LastIEncOrder = frameStruct.encOrder;
            if (bIdr)
            {
                m_ctx.LastIDREncOrder = frameStruct.encOrder;
                m_ctx.LastIDRSceneChange = ParSceneChange;
            }
        }
        m_ctx.encOrder = frameStruct.encOrder;
        m_ctx.poc = frameStruct.dispOrder;
        m_ctx.bToRecode = 0;
        m_ctx.bPanic = 0;

        if (isIntra)
        {
            m_ctx.QuantMin = m_par.quantMinI;
            m_ctx.QuantMax = m_par.quantMaxI;
        }
        else if (picType == MFX_FRAMETYPE_P)
        {
            m_ctx.QuantMin = m_par.quantMinP;
            m_ctx.QuantMax = m_par.quantMaxP;
        }
        else
        {
            m_ctx.QuantMin = m_par.quantMinB;
            m_ctx.QuantMax = m_par.quantMaxB;
        }
        m_ctx.Quant = qpY;

        if (m_ctx.SceneChange && (m_ctx.poc > m_ctx.SChPoc + 1 || m_ctx.poc == 0))
            m_ctx.SceneChange &= ~16;

        bNeedUpdateQP = true;

        if (m_par.HRDConformance != MFX_BRC_NO_HRD)
        {
            m_hrdSpec->ResetQuant();
        }

        //printf("m_ctx.SceneChange %d, m_ctx.poc %d, m_ctx.SChPoc, m_ctx.poc %d \n", m_ctx.SceneChange, m_ctx.poc, m_ctx.SChPoc, m_ctx.poc);
    }
    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        brcSts = CheckHrdAndUpdateQP(*m_hrdSpec.get(), bitsEncoded, frameStruct.encOrder, bIdr, qpY);

        MFX_CHECK(brcSts == MFX_BRC_OK || (!m_ctx.bPanic), MFX_ERR_NOT_ENOUGH_BUFFER);
        if (brcSts == MFX_BRC_OK && !m_ctx.bPanic)
            bNeedUpdateQP = true;
        pFrameSts->FrameStatus.MinFrameSize = (m_hrdSpec->GetMinFrameSizeInBits(frameStruct.encOrder,bIdr) + 7) >> 3;
        //printf("%d: poc %d, size %d QP %d (%d %d), HRD sts %d, maxFrameSize %d, type %d \n",frame_par->EncodedOrder, frame_par->DisplayOrder, bitsEncoded, m_ctx.Quant, m_ctx.QuantMin, m_ctx.QuantMax, brcSts,  m_hrd.GetMaxFrameSize(), frame_par->FrameType);
    }
    if ((e2pe > BRC_SCENE_CHANGE_RATIO2  && bitsEncoded > 4 * m_par.inputBitsPerFrame) ||
        (isIntra && ParFrameCmplx > 0 && frameStruct.encOrder == m_ctx.LastIEncOrder // We could set Qp
            && (ParSceneChange > 0 && m_ctx.LastIQpSet == m_ctx.LastIQpMin))                         // We did set Qp and/or was SceneChange
        )
    {
        // scene change, resetting BRC statistics
        m_ctx.fAbLong = m_par.inputBitsPerFrame;
        m_ctx.fAbShort = m_par.inputBitsPerFrame;
        fAbLong = m_ctx.fAbLong + (bitsEncoded - m_ctx.fAbLong) / m_par.fAbPeriodLong;
        fAbShort = m_ctx.fAbShort + (bitsEncoded - m_ctx.fAbShort) / m_par.fAbPeriodShort;
        m_ctx.SceneChange |= 1;
        if (picType != MFX_FRAMETYPE_B)
        {
            bSHStart = true;
            m_ctx.dQuantAb = ResetQuantAb(qpY, picType, layer, isRef, fAbLong, frameStruct.encOrder, bIdr, ParQpModulation);
            m_ctx.SceneChange |= 16;
            m_ctx.eRateSH = eRate;
            m_ctx.SChPoc = frameStruct.dispOrder;
            //printf("!!!!!!!!!!!!!!!!!!!!! %d m_ctx.SceneChange %d, order %d\n", frame_par->EncodedOrder, m_ctx.SceneChange, frame_par->DisplayOrder);
            if (picType == MFX_FRAMETYPE_P && bitsEncoded > 4 * m_par.inputBitsPerFrame) ResetMinQForMaxFrameSize(&m_par, picType);
        }
    }

    if (m_avg.get())
    {
        frameSizeLim = std::min(frameSizeLim, m_avg->GetMaxFrameSize(m_ctx.bPanic, bSHStart || isIntra, frameStruct.numRecode));
    }
    if (m_par.maxFrameSizeInBits)
    {
        frameSizeLim = std::min(frameSizeLim, m_par.maxFrameSizeInBits);
    }
    //printf("frameSizeLim %d (%d)\n", frameSizeLim, bitsEncoded);
    if (frameStruct.numRecode < 100)
        UpdateMinQForMaxFrameSize(&m_par, bitsEncoded, qpY, m_ctx, picType, bSHStart, brcSts);

    if (frameStruct.numRecode < 2)
    // Check other conditions for recoding (update qp if it is needed)
    {
        mfxF64 targetFrameSize = std::max<mfxF64>(m_par.inputBitsPerFrame, fAbLong);
        mfxF64 dqf = (m_par.bFieldMode) ? 1.0 : BRC_DQF(picType, m_par.iDQp, ((picType == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), (ParSceneChange || m_ctx.encOrder == 0));
        mfxF64 maxFrameSizeByRatio = dqf * BRC_FRM_RATIO(picType, m_ctx.encOrder, bSHStart, m_par.bPyr) * targetFrameSize;
        if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance != MFX_BRC_NO_HRD) {
            mfxF64 bufferDeviation = m_hrdSpec->GetBufferDeviation(frameStruct.encOrder);
            mfxF64 dev = -1.0*maxFrameSizeByRatio - bufferDeviation;
            if (dev > 0) maxFrameSizeByRatio += (std::min)(maxFrameSizeByRatio, (dev / (isIntra ? 2.0 : 4.0)));
        }

        mfxI32 quantMax = m_ctx.QuantMax;
        mfxI32 quantMin = m_ctx.QuantMin;
        mfxI32 quant = qpY;

        mfxF64 maxFrameSize = std::min<mfxF64>(maxFrameSizeByRatio, frameSizeLim);

        if (m_par.HRDConformance != MFX_BRC_NO_HRD)
        {
            mfxF64 maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frameStruct.encOrder,bIdr);
            mfxF64 bufOccupy = BRC_LTR_BUF(picType, m_par.iDQp, ((picType == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), ParSceneChange, bSHStart);
            mfxF64 maxFrameSizeHRDBalanced = bufOccupy / 9.* maxFrameSizeHrd + (9.0 - bufOccupy) / 9.*targetFrameSize;
            if (m_ctx.encOrder == 0)
            {
                // modify buf limits for VCM like encode for init only
                mfxF64 maxFrameSizeGood = 6.5 * m_par.inputBitsPerFrame;
                mfxF64 maxFrameSizeHighMark = 8.0 / 9.* maxFrameSizeHrd + 1.0 / 9.*m_par.inputBitsPerFrame;
                mfxF64 maxFrameSizeInit = mfx::clamp(maxFrameSizeGood, maxFrameSizeHRDBalanced, maxFrameSizeHighMark);
                maxFrameSize = std::min(maxFrameSize, maxFrameSizeInit);
            }
            else
                maxFrameSize = std::min(maxFrameSize, maxFrameSizeHRDBalanced);

            quantMax = std::min(m_hrdSpec->GetMaxQuant(), quantMax);
            quantMin = std::max(m_hrdSpec->GetMinQuant(), quantMin);

        }
        maxFrameSize = std::max(maxFrameSize, targetFrameSize);

        if (bitsEncoded > maxFrameSize && quant < quantMax)
        {
            mfxI32 quant_new = GetNewQP(bitsEncoded, (mfxU32)maxFrameSize, quantMin, quantMax, quant, m_par.quantOffset, 1);
            if (quant_new > quant)
            {
                bNeedUpdateQP = false;
                //printf("    recode 1-0: %d:  k %5f bitsEncoded %d maxFrameSize %d, targetSize %d, fAbLong %f, inputBitsPerFrame %f, qp %d new %d, layer %d\n",frame_par->EncodedOrder, bitsEncoded/maxFrameSize, (int)bitsEncoded, (int)maxFrameSize,(int)targetFrameSize, fAbLong, m_par.inputBitsPerFrame, quant, quant_new, layer);
                if (quant_new > GetCurQP(picType, layer, isRef, ParQpModulation))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, isRef, ParQpModulation);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, isRef, fAbLong, frameStruct.encOrder, bIdr, ParQpModulation);
                }

                if (m_par.bRec)
                {
                    SetRecodeParams(MFX_BRC_BIG_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, &pFrameSts->FrameStatus);
                    frameStruct.numRecode++;
                    return sts;
                }
            } //(quant_new > quant)
        } //bitsEncoded >  maxFrameSize

        mfxF64 lFR = std::min(m_par.gopPicSize - 1, 4);
        mfxF64 lowFrameSizeI = std::min(maxFrameSize, lFR *(mfxF64)m_par.inputBitsPerFrame);
        // Did we set the qp?
        if (isIntra && ParFrameCmplx > 0                                                     // We could set Qp
            && frameStruct.encOrder == m_ctx.LastIEncOrder && m_ctx.LastIQpSet == m_ctx.LastIQpMin      // We did set Qp
            && frameStruct.numRecode == 0 && bitsEncoded <  (lowFrameSizeI / 2.0) && quant > quantMin)    // We can & should recode
        {
            // too small; do something
            mfxI32 quant_new = GetNewQP(bitsEncoded, (mfxU32)lowFrameSizeI, quantMin, quantMax, quant, m_par.quantOffset, 0.78, false, true);
            if (quant_new < quant)
            {
                bNeedUpdateQP = false;
                if (quant_new < GetCurQP(picType, layer, isRef, ParQpModulation))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, isRef, ParQpModulation);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, isRef, fAbLong, frameStruct.encOrder, bIdr, ParQpModulation);
                }

                if (m_par.bRec)
                {
                    SetRecodeParams(MFX_BRC_SMALL_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, &pFrameSts->FrameStatus);
                    frameStruct.numRecode++;
                    return sts;
                }
            } //(quant_new < quant)
        }

        if (bitsEncoded > maxFrameSize && quant == quantMax &&
            !isIntra && m_par.bPanic &&
            (!m_ctx.bPanic) && isFrameBeforeIntra(m_ctx.encOrder, m_ctx.LastIEncOrder, m_par.gopPicSize, m_par.gopRefDist))
        {
            //skip frames before intra
            SetRecodeParams(MFX_BRC_PANIC_BIG_FRAME, quant, quant, quantMin, quantMax, m_ctx, &pFrameSts->FrameStatus);
            frameStruct.numRecode++;
            return sts;
        }
        if (m_par.HRDConformance != MFX_BRC_NO_HRD && frameStruct.numRecode == 0 && (quant < quantMax))
        {
            mfxF64 maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frameStruct.encOrder, bIdr);
            mfxF64 FAMax = 1./9. * maxFrameSizeHrd + 8./9. * fAbLong;

            if (fAbShort > FAMax)
            {
                mfxI32 quant_new = GetNewQP(fAbShort, FAMax, quantMin, quantMax, quant, m_par.quantOffset, 0.5);
                //printf("============== recode 2-0: %d:  FAMax %f, fAbShort %f, quant_new %d\n",frame_par->EncodedOrder, FAMax, fAbShort, quant_new);

                if (quant_new > quant)
                {
                    bNeedUpdateQP = false;
                    if (quant_new > GetCurQP(picType, layer, isRef, ParQpModulation))
                    {
                        UpdateQPParams(quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, isRef, ParQpModulation);
                        fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                        fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                        m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, isRef, fAbLong, frameStruct.encOrder, bIdr, ParQpModulation);
                    }
                    if (m_par.bRec)
                    {
                        SetRecodeParams(MFX_BRC_BIG_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, &pFrameSts->FrameStatus);
                        frameStruct.numRecode++;
                        return sts;
                    }
                } //quant_new > quant
            }
        }//m_par.HRDConformance
    }
    if (((m_par.HRDConformance != MFX_BRC_NO_HRD && brcSts != MFX_BRC_OK) || (bitsEncoded > (mfxI32)frameSizeLim)) && m_par.bRec)
    {
        mfxI32 quant = qpY;
        mfxI32 quant_new = quant;
        if (bitsEncoded > (mfxI32)frameSizeLim)
        {
            brcSts = MFX_BRC_BIG_FRAME;
            quant_new = GetNewQP(bitsEncoded, frameSizeLim, m_ctx.QuantMin, m_ctx.QuantMax, quant, m_par.quantOffset, 1, true);
        }
        else if (brcSts == MFX_BRC_BIG_FRAME || brcSts == MFX_BRC_SMALL_FRAME)
        {
            mfxF64 targetSize = GetFrameTargetSize(brcSts,
                m_hrdSpec->GetMinFrameSizeInBits(frameStruct.encOrder, bIdr),
                m_hrdSpec->GetMaxFrameSizeInBits(frameStruct.encOrder, bIdr));
            quant_new = GetNewQP(bitsEncoded, targetSize, m_ctx.QuantMin , m_ctx.QuantMax,quant,m_par.quantOffset, 1, true);
        }
        if (quant_new != quant)
        {
            if (brcSts == MFX_BRC_SMALL_FRAME)
            {
                quant_new = std::max(quant_new, quant - 2);
                brcSts = MFX_BRC_PANIC_SMALL_FRAME;
            }
            // Idea is to check a sign mismatch, 'true' if both are negative or positive
            if ((quant_new - qpY) * (quant_new - GetCurQP(picType, layer, isRef, ParQpModulation)) > 0)
            {
                UpdateQPParams(quant_new, picType, m_ctx, 0, m_ctx.QuantMin, m_ctx.QuantMax, layer, m_par.iDQp, isRef, ParQpModulation);
            }
            bNeedUpdateQP = false;
        }
        SetRecodeParams(brcSts, quant, quant_new, m_ctx.QuantMin, m_ctx.QuantMax, m_ctx, &pFrameSts->FrameStatus);
        //printf("===================== recode 1-0: HRD recode: quant_new %d\n", quant_new);
    }
    else
    {
        // no recoding is needed. Save context params

        mfxF64 k = 1. / GetSeqQP(qpY, picType, layer, isRef, ParQpModulation);
        mfxF64 dqAbPeriod = m_par.dqAbPeriod;
        if (m_ctx.bToRecode)
            dqAbPeriod = (k < m_ctx.dQuantAb) ? 16 : 25;

        if (bNeedUpdateQP)
        {
            m_ctx.dQuantAb += (k - m_ctx.dQuantAb) / dqAbPeriod;
            m_ctx.dQuantAb = mfx::clamp(m_ctx.dQuantAb, 1. / m_ctx.QuantMax, 1.);

            m_ctx.fAbLong = fAbLong;
            m_ctx.fAbShort = fAbShort;
        }

        bool oldScene = false;
        if ((m_ctx.SceneChange & 16) && (m_ctx.poc < m_ctx.SChPoc) && (e2pe < .01) && (mfxF64)bitsEncoded < 1.5*fAbLong)
            oldScene = true;
        //printf("-- m_ctx.eRate %f,  eRate %f, e2pe %f\n", m_ctx.eRate,  eRate, e2pe );

        if (!m_ctx.bPanic && frameStruct.numRecode < 100)
        {
            if (picType != MFX_FRAMETYPE_B)
            {
                m_ctx.LastNonBFrameSize = bitsEncoded;
                if (isIntra)
                {
                    m_ctx.eRateSH = eRate;
                    if (ParSceneChange)
                        m_ctx.eRate = m_par.inputBitsPerFrame * sqrt(QP2Qstep(GetCurQP(MFX_FRAMETYPE_P, 0, MFX_FRAMETYPE_REF, ParQpModulation), m_par.quantOffset));
                }
                else
                {
                    m_ctx.eRate = eRate;
                    if (m_ctx.eRate > m_ctx.eRateSH) m_ctx.eRateSH = m_ctx.eRate;
                }
            }

            if (isIntra)
            {
                m_ctx.LastIFrameSize = bitsEncoded;
                m_ctx.LastIQpAct = qpY;
            }
        }

        if (m_avg.get())
        {
            m_avg->UpdateSlidingWindow(bitsEncoded, m_ctx.encOrder, m_ctx.bPanic, bSHStart || isIntra, frameStruct.numRecode, qpY);
        }

        m_ctx.totalDeviation += ((mfxF64)bitsEncoded - m_par.inputBitsPerFrame);

        //printf("------------------ %d (%d)) Total deviation %f, old scene %d, bNeedUpdateQP %d, m_ctx.Quant %d, type %d, m_ctx.fAbLong %f m_par.inputBitsPerFrame %f\n", frame_par->EncodedOrder, frame_par->DisplayOrder,m_ctx.totalDeviation, oldScene , bNeedUpdateQP, m_ctx.Quant,picType, m_ctx.fAbLong, m_par.inputBitsPerFrame);

        if (!m_ctx.bPanic && (!oldScene) && bNeedUpdateQP)
        {
            mfxI32 quant_new = qpY;

            //Update QP

            mfxF64 totDev = m_ctx.totalDeviation;
            mfxF64 HRDDevFactor = 0.0;
            mfxF64 HRDDev = 0.0;
            mfxF64 maxFrameSizeHrd = 0.0;
            if (m_par.HRDConformance != MFX_BRC_NO_HRD)
            {

                HRDDevFactor = m_hrdSpec->GetBufferDeviationFactor(frameStruct.encOrder);
                HRDDev = m_hrdSpec->GetBufferDeviation(frameStruct.encOrder);
                maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frameStruct.encOrder, bIdr);
            }

            mfxF64 dequant_new = m_ctx.dQuantAb*pow(m_par.inputBitsPerFrame / m_ctx.fAbLong, 1.0 + HRDDevFactor);

            mfxF64 bAbPreriod = m_par.bAbPeriod;

            if (m_par.HRDConformance != MFX_BRC_NO_HRD)
            {
                if (m_par.rateControlMethod == MFX_RATECONTROL_VBR && m_par.maxbps > m_par.targetbps )
                {
                    totDev = std::max(totDev, HRDDev);
                }
                else
                {
                    totDev = HRDDev;
                }
                if (totDev > 0)
                {
                    bAbPreriod = (mfxF64)(m_par.bPyr ? 4 : 3)*(mfxF64)maxFrameSizeHrd / fAbShort * GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIDREncOrder, m_par.gopPicSize, m_ctx.LastIDRSceneChange);
                    bAbPreriod = mfx::clamp(bAbPreriod, m_par.bAbPeriod / 10, m_par.bAbPeriod);
                }
            }
            quant_new = GetNewQPTotal(totDev / bAbPreriod / (mfxF64)m_par.inputBitsPerFrame, dequant_new, m_ctx.QuantMin, m_ctx.QuantMax, GetSeqQP(qpY, picType, layer, isRef, ParQpModulation), m_par.bPyr && m_par.bRec, bSHStart && m_ctx.bToRecode == 0);
            quant_new = GetPicQP(quant_new, picType, layer, isRef,ParQpModulation);
            //printf("    ===%d quant old %d quant_new %d, bitsEncoded %d m_ctx.QuantMin %d m_ctx.QuantMax %d\n", frame_par->EncodedOrder, m_ctx.Quant, quant_new, bitsEncoded, m_ctx.QuantMin, m_ctx.QuantMax);

            if (bMaxFrameSizeMode)
            {
                mfxF64 targetMax = ((mfxF64)m_par.maxFrameSizeInBits*((bSHStart || isIntra) ? 0.95 : 0.9));
                mfxF64 targetMin = ((mfxF64)m_par.maxFrameSizeInBits*((bSHStart || isIntra) ? 0.9 : 0.8 /*0.75 : 0.5*/));
                mfxI32 QuantNewMin = GetNewQP(bitsEncoded, targetMax, m_ctx.QuantMin, m_ctx.QuantMax, qpY, m_par.quantOffset, 1, false, false);
                mfxI32 QuantNewMax = GetNewQP(bitsEncoded, targetMin, m_ctx.QuantMin, m_ctx.QuantMax, qpY, m_par.quantOffset, 1, false, false);
                mfxI32 quant_corrected = qpY;

                if (quant_corrected < QuantNewMin - 3)
                    quant_corrected += 2;
                if (quant_corrected < QuantNewMin)
                    quant_corrected++;
                else if (quant_corrected > QuantNewMax + 3)
                    quant_corrected -= 2;
                else if (quant_corrected > QuantNewMax)
                    quant_corrected--;

                //printf("   QuantNewMin %d, QuantNewMax %d, m_ctx.Quant %d, new %d (%d)\n", QuantNewMin, QuantNewMax, m_ctx.Quant, quant_corrected, quant_new);

                quant_new = mfx::clamp(quant_corrected, m_ctx.QuantMin, m_ctx.QuantMax);
            }

            if ((quant_new - qpY)* (quant_new - GetCurQP(picType, layer, isRef, ParQpModulation)) > 0) // this check is actual for async scheme
            {
                //printf("   +++ Update QP %d: totalDeviation %f, bAbPreriod %f (%f), QP %d (%d %d), qp_new %d (qpY %d), type %d, dequant_new %f (%f) , m_ctx.fAbLong %f, m_par.inputBitsPerFrame %f (%f)\n",frame_par->EncodedOrder,totDiv , bAbPreriod, GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIEncOrder, m_par.gopPicSize), m_ctx.Quant, m_ctx.QuantMin, m_ctx.QuantMax,quant_new, qpY, picType, 1.0/dequant_new, 1.0/m_ctx.dQuantAb, m_ctx.fAbLong, m_par.inputBitsPerFrame);
                UpdateQPParams(quant_new, picType, m_ctx, 0, m_ctx.QuantMin, m_ctx.QuantMax, layer, m_par.iDQp, isRef, ParQpModulation);
            }
        }
        m_ctx.bToRecode = 0;
        if (m_par.HRDConformance != MFX_BRC_NO_HRD)
        {
            m_hrdSpec->Update(bitsEncoded, frameStruct.encOrder, bIdr);
        }
    }

    return sts;
}

mfxStatus BRC_EncTool::GetHRDPos(mfxU32 dispOrder, mfxEncToolsBRCHRDPos *pHRDPos)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(m_par.HRDConformance != MFX_BRC_NO_HRD, MFX_ERR_NONE);
    MFX_CHECK_NULL_PTR1(pHRDPos);

    auto frameStructItr = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStructItr == m_FrameStruct.end())
        return MFX_ERR_UNDEFINED_BEHAVIOR; // BRC hasn't processed the frame

    pHRDPos->InitialCpbRemovalDelay = m_hrdSpec->GetInitCpbRemovalDelay(frameStructItr->encOrder);
    pHRDPos->InitialCpbRemovalDelayOffset = m_hrdSpec->GetInitCpbRemovalDelayOffset(frameStructItr->encOrder);
    return MFX_ERR_NONE;

}

mfxStatus BRC_EncTool::ProcessFrame(mfxU32 dispOrder, mfxEncToolsBRCQuantControl *pFrameQp)
{
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(pFrameQp);

    auto frameStructItr = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStructItr == m_FrameStruct.end())
        return MFX_ERR_UNDEFINED_BEHAVIOR; // BRC hasn't processed the frame
    BRC_FrameStruct frameStruct = *frameStructItr;

#if (MFX_VERSION >= 1026)
    mfxU16 ParSceneChange = frameStruct.sceneChange;
    mfxU16 ParLongTerm = frameStruct.longTerm;
    mfxU32 ParFrameCmplx = frameStruct.frameCmplx;
#else
    mfxU16 ParSceneChange = 0;
    mfxU16 ParLongTerm = 0;
    mfxU32 ParFrameCmplx = 0;
#endif
    mfxU16 ParQpModulation = frameStruct.qpModulation;

    mfxI32 qp = 0;
    mfxI32 qpMin = 1;
    mfxU16 type = GetFrameType(frameStruct.frameType, frameStruct.pyrLayer, m_par.gopRefDist);
    bool  bIdr = (type == MFX_FRAMETYPE_IDR);
    mfxU16 isRef = frameStruct.frameType & MFX_FRAMETYPE_REF;
    mfxU16 isIntra = frameStruct.frameType & (MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_I);

    mfxF64 HRDDevFactor = 0.0;
    mfxF64 HRDDev = 0.0;
    mfxF64 maxFrameSizeHrd = 0.0;

    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        HRDDevFactor = m_hrdSpec->GetBufferDeviationFactor(frameStruct.encOrder);
        HRDDev = m_hrdSpec->GetBufferDeviation(frameStruct.encOrder);
        maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frameStruct.encOrder, bIdr);
    }

    if (!m_bDynamicInit) {
        if (isIntra) {
            // Init DQP
            if (ParLongTerm) {
                m_par.iDQp = m_par.iDQp0;
                ltrprintf("DQp0 %d\n", m_par.iDQp);
            }
            // Init Qp
            if (ParFrameCmplx > 0) {
                mfxF64 raca = (mfxF64)ParFrameCmplx / BRC_RACA_SCALE;
                // MaxFrameSize
                mfxF64 maxFrameSize = m_par.mRawFrameSizeInBits;
                if (m_par.maxFrameSizeInBits) {
                    maxFrameSize = std::min<mfxF64>(maxFrameSize, m_par.maxFrameSizeInBits);
                }
                if (m_par.HRDConformance != MFX_BRC_NO_HRD) {
                    mfxF64 bufOccupy = BRC_LTR_BUF(type, m_par.iDQp, m_par.mIntraBoost, 1, 0);
                    maxFrameSize = std::min(maxFrameSize, (bufOccupy / 9.* (m_par.initialDelayInBytes * 8.0) + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
                }
                // Set Intra QP
                mfxF64 dqf = BRC_DQF(type, m_par.iDQp, m_par.mIntraBoost, 1);
                mfxF64 targetFrameSize = dqf * BRC_FRM_RATIO(type, 0, 0, m_par.bPyr) * (mfxF64)m_par.inputBitsPerFrame;
                targetFrameSize = std::min(maxFrameSize, targetFrameSize);
                mfxI32 qp0 = compute_first_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca);
                if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qp0 > 3)
                    qp0 -= 3; // use re-encoding for best results (maxFrameSizeGood)
                else if (raca == BRC_MIN_RACA && qp0 > 3)
                    qp0 -= 3; // uncertainty; use re-encoding for best results
                ltrprintf("Qp0 %d\n", qp0);
                UpdateQPParams(qp0, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, isRef, ParQpModulation);
                qpMin = qp0;
            }
        }
        m_bDynamicInit = true;
    }

    if (frameStruct.encOrder == m_ctx.encOrder || frameStruct.numRecode)
    {
        qp = m_ctx.Quant;
    }
    else
    {
        if (isIntra)
        {
            if (type == MFX_FRAMETYPE_IDR) {
                if (!ParLongTerm) {
                    m_par.iDQp = 0;
                }
                else if (ParSceneChange) {
                    m_par.iDQp = m_par.iDQp0;
                }
            }

            mfxF64 maxFrameSize = m_par.mRawFrameSizeInBits;
            if (m_par.maxFrameSizeInBits) {
                maxFrameSize = std::min<mfxF64>(maxFrameSize, m_par.maxFrameSizeInBits);
            }
            if (m_par.HRDConformance != MFX_BRC_NO_HRD) {
                mfxF64 hrdMaxFrameSize = m_par.initialDelayInBytes * 8;
                if (maxFrameSizeHrd > 0)
                    hrdMaxFrameSize =  std::min(hrdMaxFrameSize, maxFrameSizeHrd);
                mfxF64 bufOccupy = BRC_LTR_BUF(type, m_par.iDQp, ((type == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), (ParSceneChange || (m_ctx.LastIQpSet && m_ctx.QuantP > ((mfxI32)m_ctx.LastIQpSet + (mfxI32)m_par.iDQp + 1))), 0);
                maxFrameSize = std::min(maxFrameSize, (bufOccupy / 9.* hrdMaxFrameSize + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
            }

            if (type == MFX_FRAMETYPE_IDR) {
                // Re-Determine LTR  iDQP
                if (!ParLongTerm)
                    m_par.iDQp = 0;
                else
                {
                    mfxF64 maxFrameRatio = 2 * BRC_FRM_RATIO(type, frameStruct.encOrder, 0, m_par.bPyr);
                    mfxF64 minFrameRatio = BRC_FRM_RATIO(type, 0, 0, m_par.bPyr);
                    maxFrameRatio = std::min(maxFrameRatio, (maxFrameSize / m_par.inputBitsPerFrame));
                    mfxU32 mNumRefsInGop = m_par.mNumRefsInGop;
                    if (m_ctx.LastIQpSetOrder) {
                        mfxU32 pastRefsInGop = (mfxU32)(std::max(1.0, (!m_par.bPyr ? (mfxF64)(frameStruct.encOrder - m_ctx.LastIQpSetOrder) / (mfxF64)m_par.gopRefDist : (mfxF64)(frameStruct.encOrder - m_ctx.LastIQpSetOrder) / 2.0)));
                        mNumRefsInGop = std::min(mNumRefsInGop, pastRefsInGop);
                    }
                    maxFrameRatio = std::min<mfxF64>(maxFrameRatio, mNumRefsInGop);
                    mfxF64 dqpmax = std::max(0.0, 6.0 * (log(maxFrameRatio / minFrameRatio) / log(2.0)));
                    mfxU32 iDQpMax = (mfxU32)(dqpmax + 0.5);
                    if (ParSceneChange)
                        iDQpMax = mfx::clamp(iDQpMax, 1u, m_par.iDQp0);
                    else
                        iDQpMax = mfx::clamp<mfxU32>(iDQpMax, 1u, BRC_MAX_DQP_LTR);
                    m_par.iDQp = iDQpMax;
                    ltrprintf("FR %lf DQp %d\n", maxFrameRatio, m_par.iDQp);
                }
            }

            // Determine Min Qp
            if (ParFrameCmplx > 0)
            {
                mfxF64 raca = (mfxF64)ParFrameCmplx / BRC_RACA_SCALE;
                mfxF64 dqf = BRC_DQF(type, m_par.iDQp, ((type == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), ParSceneChange);
                mfxF64 targetFrameSize = dqf * BRC_FRM_RATIO(type, frameStruct.encOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
                if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance) {
                    // CBR HRD Buffer overflow has priority
                    mfxF64 dev = -1.0*targetFrameSize - HRDDev;
                    if (dev > 0)
                        targetFrameSize += std::min(targetFrameSize, (dev / 2.0));
                }

                targetFrameSize = std::min(maxFrameSize, targetFrameSize);
                mfxF64 CmplxRatio = 1.0;
                if (m_ctx.LastICmplx) CmplxRatio = ParFrameCmplx / m_ctx.LastICmplx;
                if (!ParSceneChange && m_ctx.LastICmplx && m_ctx.LastIQpAct && m_ctx.LastIFrameSize && CmplxRatio > 0.5 && CmplxRatio < 2.0)
                {
                    qpMin = compute_new_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca, m_ctx.LastIFrameSize, (mfxF64)m_ctx.LastICmplx / BRC_RACA_SCALE, m_ctx.LastIQpAct);
                    if (raca == BRC_MIN_RACA && qpMin > 3)
                        qpMin -= 3; // uncertainty; use re-encoding for best results
                }
                else
                {
                    qpMin = compute_first_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca);
                    if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qpMin>3)
                        qpMin -= 3; // uncertainty; use re-encoding for best results
                    else if (raca == BRC_MIN_RACA && qpMin > 3)
                        qpMin -= 3; // uncertainty; use re-encoding for best results
                }

                ltrprintf("Min QpI %d\n", qpMin);
            }
        }
        else //if (type == MFX_FRAMETYPE_P)
        {
            mfxU16 ltype = MFX_FRAMETYPE_P;
            mfxF64 maxFrameSize = m_par.mRawFrameSizeInBits;
            if (m_par.maxFrameSizeInBits) {
                maxFrameSize = std::min<mfxF64>(maxFrameSize, m_par.maxFrameSizeInBits);
            }
            if (m_par.HRDConformance != MFX_BRC_NO_HRD) {
                mfxF64 hrdMaxFrameSize = m_par.initialDelayInBytes * 8;
                if (maxFrameSizeHrd > 0) hrdMaxFrameSize = std::min(hrdMaxFrameSize, (mfxF64)maxFrameSizeHrd);

                mfxF64 bufOccupy = BRC_LTR_BUF(ltype, m_par.iDQp, false, ParSceneChange, ParSceneChange);
                maxFrameSize = std::min(maxFrameSize, (bufOccupy / 9.* hrdMaxFrameSize + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
            }

            mfxF64 targetFrameSize = BRC_FRM_RATIO(ltype, frameStruct.encOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
            if (m_par.bPyr && m_par.gopRefDist == 8)
                targetFrameSize *= ((ParQpModulation == MFX_QP_MODULATION_HIGH) ? 2.0 : ((ParQpModulation != MFX_QP_MODULATION_LOW) ? 1.66 : 1.0));
            if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance != MFX_BRC_NO_HRD) {
                mfxF64 dev = -1.0*targetFrameSize - HRDDev;
                if (dev > 0)
                    targetFrameSize += std::min(targetFrameSize, (dev / 4.0));
            }
            targetFrameSize = std::min(maxFrameSize, targetFrameSize);
            qpMin = GetMinQForMaxFrameSize(m_par, targetFrameSize, ltype);
        }

        qp = GetCurQP(type, frameStruct.pyrLayer, isRef, ParQpModulation);

        // Max Frame Size recode prevention
        if (qp < qpMin)
        {
            if (type != MFX_FRAMETYPE_B)
            {
                SetQPParams(qpMin, type, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, isRef, ParQpModulation);
                qp = GetCurQP(type, frameStruct.pyrLayer, isRef, ParQpModulation);
            }
            else
            {
                qp = qpMin;
            }
        }
        else
            qpMin = std::min(qp - 1, qpMin);
    }
    frameStruct.qp = qp - m_par.quantOffset;

    if (isIntra) {
        m_ctx.LastIQpSetOrder = frameStruct.encOrder;
        m_ctx.LastIQpMin = qpMin - m_par.quantOffset;
        m_ctx.LastIQpSet = frameStruct.qp;
        m_ctx.LastIQpAct = 0;
        m_ctx.LastICmplx = ParFrameCmplx;
        m_ctx.LastIFrameSize = 0;
        ResetMinQForMaxFrameSize(&m_par, type);
    }

    pFrameQp->QpY = frameStruct.qp;
    pFrameQp->NumDeltaQP = 0;

    return MFX_ERR_NONE;
}

mfxStatus BRC_EncTool::ReportEncResult(mfxU32 dispOrder, mfxEncToolsBRCEncodeResult const & pEncRes)
{
    auto frameStruct = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStruct == m_FrameStruct.end())
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR; // BRC gets encoding results for the frame it hasn't set QP for
    }
    (*frameStruct).frameSize = pEncRes.CodedFrameSize;
    (*frameStruct).qp = pEncRes.QpY;
    (*frameStruct).numRecode = pEncRes.NumRecodesDone;
    return MFX_ERR_NONE;
}


mfxStatus BRC_EncTool::SetFrameStruct(mfxU32 dispOrder, mfxEncToolsBRCFrameParams  const & pFrameStruct)
{
    auto frameStruct = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStruct == m_FrameStruct.end())
    {
        BRC_FrameStruct frStruct;
        frStruct.dispOrder = dispOrder;
        frStruct.frameType = pFrameStruct.FrameType;
        frStruct.pyrLayer = pFrameStruct.PyramidLayer;
        frStruct.encOrder = pFrameStruct.EncodeOrder;
        m_FrameStruct.push_back(frStruct);
        frameStruct = m_FrameStruct.end() - 1;
    }
    else
    {
        (*frameStruct).frameType = pFrameStruct.FrameType;
        (*frameStruct).pyrLayer = pFrameStruct.PyramidLayer;
        (*frameStruct).encOrder = pFrameStruct.EncodeOrder; // or check if it's the same, otherwise - error ?
        (*frameStruct).numRecode++;  // ??? check
    }
    return MFX_ERR_NONE;
}

mfxStatus BRC_EncTool::ReportBufferHints(mfxU32 dispOrder, mfxEncToolsBRCBufferHint const & pBufHints)
{
    auto frameStruct = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStruct == m_FrameStruct.end())
    {
        BRC_FrameStruct frStruct;
        frStruct.dispOrder = dispOrder;
        frStruct.OptimalFrameSizeInBytes = pBufHints.OptimalFrameSizeInBytes;
        //frStruct.optimalBufferFullness = pBufHints.OptimalBufferFullness;
        m_FrameStruct.push_back(frStruct);
        frameStruct = m_FrameStruct.end() - 1;
    }
    else
    {
        (*frameStruct).OptimalFrameSizeInBytes = pBufHints.OptimalFrameSizeInBytes;
        //(*frameStruct).optimalBufferFullness = pBufHints.OptimalBufferFullness;
    }
    return MFX_ERR_NONE;
}

mfxStatus BRC_EncTool::ReportGopHints(mfxU32 dispOrder, mfxEncToolsHintPreEncodeGOP const & pGopHints)
{
    auto frameStruct = std::find_if(m_FrameStruct.begin(), m_FrameStruct.end(), CompareByDisplayOrder(dispOrder));
    if (frameStruct == m_FrameStruct.end())
    {
        BRC_FrameStruct frStruct;
        frStruct.dispOrder = dispOrder;
        frStruct.qpDelta = pGopHints.QPDelta;
        frStruct.qpModulation = pGopHints.QPModulation;
        m_FrameStruct.push_back(frStruct);
        frameStruct = m_FrameStruct.end() - 1;
    }
    else
    {
        (*frameStruct).qpDelta = pGopHints.QPDelta;
        (*frameStruct).qpModulation = pGopHints.QPModulation;
    }
    return MFX_ERR_NONE;

}

#endif

void HEVC_HRD::Init(cBRCParams const &par)
{
    m_hrdInput.Init(par);
    m_prevAuCpbRemovalDelayMinus1 = -1;
    m_prevAuCpbRemovalDelayMsb = 0;
    m_prevAuFinalArrivalTime = 0;
    m_prevBpAuNominalRemovalTime = (mfxU32)m_hrdInput.m_initCpbRemovalDelay;
    m_prevBpEncOrder = 0;
}

void HEVC_HRD::Reset(cBRCParams const &par)
{
    sHrdInput hrdInput;
    hrdInput.Init(par);
    m_hrdInput.m_bitrate = hrdInput.m_bitrate;
    m_hrdInput.m_cpbSize90k = hrdInput.m_cpbSize90k;
}

void HEVC_HRD::Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI)
{
    mfxF64 auNominalRemovalTime = 0.0;
    mfxF64 initCpbRemovalDelay = GetInitCpbRemovalDelay(eo);
    if (eo > 0)
    {
        mfxU32 auCpbRemovalDelayMinus1 = (eo - m_prevBpEncOrder) - 1;
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb = 0;

        if (!bSEI && (eo - m_prevBpEncOrder) != 1)
        {
            auCpbRemovalDelayMsb = ((mfxI32)auCpbRemovalDelayMinus1 <= m_prevAuCpbRemovalDelayMinus1)
                ? m_prevAuCpbRemovalDelayMsb + m_hrdInput.m_maxCpbRemovalDelay
                : m_prevAuCpbRemovalDelayMsb;
        }

        m_prevAuCpbRemovalDelayMsb = auCpbRemovalDelayMsb;
        m_prevAuCpbRemovalDelayMinus1 = auCpbRemovalDelayMinus1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_hrdInput.m_clockTick * (auCpbRemovalDelayValMinus1 + 1);
    }
    else // (C-9)
        auNominalRemovalTime = m_hrdInput.m_initCpbRemovalDelay;

    // (C-3)
    mfxF64 initArrivalTime = m_prevAuFinalArrivalTime;

    if (!m_hrdInput.m_cbrFlag)
    {
        mfxF64 initArrivalEarliestTime = (bSEI)
            // (C-7)
            ? auNominalRemovalTime - initCpbRemovalDelay
            // (C-6)
            : auNominalRemovalTime - m_hrdInput.m_cpbSize90k;
        // (C-4)
        initArrivalTime = std::max<mfxF64>(m_prevAuFinalArrivalTime, initArrivalEarliestTime * m_hrdInput.m_bitrate);
    }
    // (C-8)
    mfxF64 auFinalArrivalTime = initArrivalTime + (mfxF64)sizeInbits * 90000;

    m_prevAuFinalArrivalTime = auFinalArrivalTime;

    if (bSEI)
    {
        m_prevBpAuNominalRemovalTime = auNominalRemovalTime;
        m_prevBpEncOrder = eo;
    }

}

mfxU32 HEVC_HRD::GetInitCpbRemovalDelay(mfxU32 eo) const
{
    mfxF64 auNominalRemovalTime;

    if (eo > 0)
    {
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb = 0;
        mfxU32 auCpbRemovalDelayMinus1 = eo - m_prevBpEncOrder - 1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_hrdInput.m_clockTick * (auCpbRemovalDelayValMinus1 + 1);

        // (C-17)
        mfxF64 deltaTime90k = auNominalRemovalTime - m_prevAuFinalArrivalTime / m_hrdInput.m_bitrate;

        return (m_hrdInput.m_cbrFlag
            // (C-19)
            ? (mfxU32)(deltaTime90k)
            // (C-18)
            : (mfxU32)std::min(deltaTime90k, m_hrdInput.m_cpbSize90k));
    }

    return  (mfxU32)m_hrdInput.m_initCpbRemovalDelay;
}
inline mfxF64 GetTargetDelay(mfxF64 cpbSize90k, mfxF64 initCpbRemovalDelay, bool bVBR)
{
    return  bVBR ?
        std::max(std::min(3.0*cpbSize90k / 4.0, initCpbRemovalDelay), cpbSize90k / 2.0) :
        std::min(cpbSize90k / 2.0, initCpbRemovalDelay);
}
mfxF64 HEVC_HRD::GetBufferDeviation(mfxU32 eo)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    mfxF64 targetDelay = GetTargetDelay(m_hrdInput.m_cpbSize90k, m_hrdInput.m_initCpbRemovalDelay, !m_hrdInput.m_cbrFlag);
    return (targetDelay - delay) / 90000.0*m_hrdInput.m_bitrate;
}
mfxF64 HEVC_HRD::GetBufferDeviationFactor(mfxU32 eo)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    mfxF64 targetDelay = GetTargetDelay(m_hrdInput.m_cpbSize90k, m_hrdInput.m_initCpbRemovalDelay, !m_hrdInput.m_cbrFlag);
    return abs((targetDelay - delay) / targetDelay);
}
mfxU32 HEVC_HRD::GetMaxFrameSizeInBits(mfxU32 eo, bool /*bSEI*/)  const
{
    return (mfxU32)(GetInitCpbRemovalDelay(eo) / 90000.0*m_hrdInput.m_bitrate);
}
mfxU32 HEVC_HRD::GetMinFrameSizeInBits(mfxU32 eo, bool /*bSEI*/)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    if ((!m_hrdInput.m_cbrFlag) || ((delay + m_hrdInput.m_clockTick + 16.0) < m_hrdInput.m_cpbSize90k))
        return 0;
    return (mfxU32)((delay + m_hrdInput.m_clockTick + 16.0 - m_hrdInput.m_cpbSize90k) / 90000.0*m_hrdInput.m_bitrate + 0.99999);
}


H264_HRD::H264_HRD() :
    m_trn_cur(0)
    , m_taf_prv(0)
{
}

void H264_HRD::Init(cBRCParams const &par)
{
    m_hrdInput.Init(par);
    m_hrdInput.m_clockTick *= (1.0 / 90000.0);

    m_taf_prv = 0.0;
    m_trn_cur = m_hrdInput.m_initCpbRemovalDelay / 90000.0;
    m_trn_cur = GetInitCpbRemovalDelay(0) / 90000.0;
}

void H264_HRD::Reset(cBRCParams const &par)
{
    sHrdInput hrdInput;
    hrdInput.Init(par);
    m_hrdInput.m_bitrate = hrdInput.m_bitrate;
    m_hrdInput.m_cpbSize90k = hrdInput.m_cpbSize90k;
}

void H264_HRD::Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI)
{
    // const bool interlace = false; //BRC is frame level only
    mfxU32 initDelay = GetInitCpbRemovalDelay(eo);

    double tai_earliest = bSEI
        ? m_trn_cur - (initDelay / 90000.0)
        : m_trn_cur - (m_hrdInput.m_cpbSize90k / 90000.0);

    double tai_cur = (!m_hrdInput.m_cbrFlag)
        ? std::max(m_taf_prv, tai_earliest)
        : m_taf_prv;

    m_taf_prv = tai_cur + (mfxF64)sizeInbits / m_hrdInput.m_bitrate;
    m_trn_cur += m_hrdInput.m_clockTick;

}

mfxU32 H264_HRD::GetInitCpbRemovalDelay(mfxU32 /* eo */)  const
{

    double delay = std::max(0.0, m_trn_cur - m_taf_prv);
    mfxU32 initialCpbRemovalDelay = mfxU32(90000 * delay + 0.5);

    return (mfxU32)(initialCpbRemovalDelay == 0
        ? 1 // should not be equal to 0
        : initialCpbRemovalDelay > m_hrdInput.m_cpbSize90k && (!m_hrdInput.m_cbrFlag)
        ? m_hrdInput.m_cpbSize90k  // should not exceed hrd buffer
        : initialCpbRemovalDelay);
}
mfxF64 H264_HRD::GetBufferDeviation(mfxU32 eo)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    mfxF64 targetDelay = GetTargetDelay(m_hrdInput.m_cpbSize90k, m_hrdInput.m_initCpbRemovalDelay, !m_hrdInput.m_cbrFlag);
    //printf("%d) GetBufferDeviation %f (%d, target %d)\n", eo, (targetDelay - delay) / 90000.0*m_hrdInput.m_bitrate, delay, (int)targetDelay);

    return (targetDelay - delay) / 90000.0*m_hrdInput.m_bitrate;
}
mfxF64 H264_HRD::GetBufferDeviationFactor(mfxU32 eo)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    mfxF64 targetDelay = GetTargetDelay(m_hrdInput.m_cpbSize90k, m_hrdInput.m_initCpbRemovalDelay, !m_hrdInput.m_cbrFlag);
    return abs((targetDelay - delay) / targetDelay);
}


mfxU32 H264_HRD::GetInitCpbRemovalDelayOffset(mfxU32 eo)  const
{
    // init_cpb_removal_delay + init_cpb_removal_delay_offset should be constant
    return mfxU32(m_hrdInput.m_cpbSize90k - GetInitCpbRemovalDelay(eo));
}
mfxU32 H264_HRD::GetMinFrameSizeInBits(mfxU32 eo, bool /*bSEI*/)  const
{
    mfxU32 delay = GetInitCpbRemovalDelay(eo);
    if ((!m_hrdInput.m_cbrFlag) || ((delay + m_hrdInput.m_clockTick * 90000) < m_hrdInput.m_cpbSize90k))
        return 0;

    return (mfxU32)((delay + m_hrdInput.m_clockTick*90000.0 - m_hrdInput.m_cpbSize90k) / 90000.0*m_hrdInput.m_bitrate) + 16;

}
mfxU32 H264_HRD::GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI)  const
{
    mfxU32 initDelay = GetInitCpbRemovalDelay(eo);

    double tai_earliest = (bSEI)
        ? m_trn_cur - (initDelay / 90000.0)
        : m_trn_cur - (m_hrdInput.m_cpbSize90k / 90000.0);

    double tai_cur = (!m_hrdInput.m_cbrFlag)
        ? std::max(m_taf_prv, tai_earliest)
        : m_taf_prv;

    mfxU32 maxFrameSize = (mfxU32)((m_trn_cur - tai_cur)*m_hrdInput.m_bitrate);

    return  maxFrameSize;
}

#ifdef MFX_ENABLE_ENCTOOLS

mfxStatus ExtBRC::GetFrameCtrl(mfxBRCFrameParam* frame_par, mfxBRCFrameCtrl* ctrl)
{
    mfxEncToolsTaskParam par;
    par.DisplayOrder = frame_par->DisplayOrder;

    std::vector<mfxExtBuffer*> extParams;

    mfxEncToolsBRCFrameParams extFrameStruct;

    extFrameStruct.Header.BufferId = MFX_EXTBUFF_ENCTOOLS_BRC_FRAME_PARAM ;
    extFrameStruct.Header.BufferSz = sizeof(extFrameStruct);
    extFrameStruct.EncodeOrder = frame_par->EncodedOrder;
    extFrameStruct.FrameType = frame_par->FrameType;
    extFrameStruct.PyramidLayer = frame_par->PyramidLayer;

    extParams.push_back((mfxExtBuffer *)&extFrameStruct);

    par.ExtParam = &extParams[0];
    par.NumExtParam = (mfxU16)extParams.size();

    mfxStatus sts;
    sts = Submit(&par);
    MFX_CHECK_STS(sts);


    extParams.clear();

    mfxEncToolsBRCQuantControl extFrameQP;
    extFrameQP.Header.BufferId = MFX_EXTBUFF_ENCTOOLS_BRC_QUANT_CONTROL;
    extFrameQP.Header.BufferSz = sizeof(extFrameQP);

    extParams.push_back((mfxExtBuffer *)&extFrameQP);
    par.ExtParam = &extParams[0];
    par.NumExtParam = (mfxU16)extParams.size();

    sts = Query(&par, 5000);
    MFX_CHECK_STS(sts);

    ctrl->QpY = extFrameQP.QpY;
    return sts;

}

#endif

const mfxU32 h264_h265_au_cpb_removal_delay_length_minus1 = 23;

void sHrdInput::Init(cBRCParams par)
{
    m_cbrFlag = (par.rateControlMethod == MFX_RATECONTROL_CBR);
    m_bitrate = par.maxbps;
    m_maxCpbRemovalDelay = 1 << (h264_h265_au_cpb_removal_delay_length_minus1 + 1);
    m_clockTick = 90000. / par.frameRate;
    m_cpbSize90k = mfxU32(90000. * par.bufferSizeInBytes*8.0 / m_bitrate);
    m_initCpbRemovalDelay = 90000. * 8. * par.initialDelayInBytes / m_bitrate;
}

#ifdef MFX_ENABLE_ENCTOOLS

mfxStatus ExtBRC::Update(mfxBRCFrameParam* frame_par, mfxBRCFrameCtrl* frame_ctrl, mfxBRCFrameStatus* status)
{
    mfxEncToolsTaskParam par;
    par.DisplayOrder = frame_par->DisplayOrder;

    std::vector<mfxExtBuffer*> extParams;
    mfxEncToolsBRCEncodeResult extEncRes;
    extEncRes.Header.BufferId = MFX_EXTBUFF_ENCTOOLS_BRC_ENCODE_RESULT;
    extEncRes.Header.BufferSz = sizeof(extEncRes);
    extEncRes.CodedFrameSize = frame_par->CodedFrameSize;
    extEncRes.QpY = (mfxU16)frame_ctrl->QpY;
    extEncRes.NumRecodesDone = frame_par->NumRecode;

    extParams.push_back((mfxExtBuffer *)&extEncRes);

    par.ExtParam = &extParams[0];
    par.NumExtParam = (mfxU16)extParams.size();

    mfxStatus sts;
    sts = Submit(&par);
    MFX_CHECK_STS(sts);

    extParams.clear();
    mfxEncToolsBRCStatus extSts;
    extSts.Header.BufferId = MFX_EXTBUFF_ENCTOOLS_BRC_STATUS;
    extSts.Header.BufferSz = sizeof(extSts);
    extSts.FrameStatus = *status;

    extParams.push_back((mfxExtBuffer *)&extSts);
    par.ExtParam = &extParams[0];
    par.NumExtParam = (mfxU16)extParams.size();

    sts = Query(&par, 5000);

    return sts;
}

#endif

