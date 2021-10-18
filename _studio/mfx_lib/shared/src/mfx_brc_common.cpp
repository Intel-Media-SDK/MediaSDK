// Copyright (c) 2018-2020 Intel Corporation
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

#include "mfx_brc_common.h"
#include "mfx_enc_common.h"
#include <algorithm>
#include <math.h>
#if defined(MFX_ENABLE_VIDEO_BRC_COMMON)

#define IS_IFRAME(pictype) ((pictype == MFX_FRAMETYPE_I || pictype == MFX_FRAMETYPE_IDR) ? MFX_FRAMETYPE_I: 0)

mfxStatus ConvertVideoParam_Brc(const mfxVideoParam *parMFX, UMC::VideoBrcParams *parUMC)
{
    MFX_CHECK_COND(parMFX != NULL);
    MFX_CHECK_COND((parMFX->mfx.FrameInfo.CropX + parMFX->mfx.FrameInfo.CropW) <= parMFX->mfx.FrameInfo.Width);
    MFX_CHECK_COND((parMFX->mfx.FrameInfo.CropY + parMFX->mfx.FrameInfo.CropH) <= parMFX->mfx.FrameInfo.Height);

    switch (parMFX->mfx.RateControlMethod) {
    case MFX_RATECONTROL_CBR:  parUMC->BRCMode = UMC::BRC_CBR; break;
    case MFX_RATECONTROL_AVBR: parUMC->BRCMode = UMC::BRC_AVBR; break;
    default:
    case MFX_RATECONTROL_VBR:  parUMC->BRCMode = UMC::BRC_VBR; break;
    }

    mfxU16 brcParamMultiplier = parMFX->mfx.BRCParamMultiplier ? parMFX->mfx.BRCParamMultiplier : 1;

    parUMC->targetBitrate = parMFX->mfx.TargetKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;

    if (parUMC->BRCMode == UMC::BRC_AVBR)
    {
        parUMC->accuracy = parMFX->mfx.Accuracy;
        parUMC->convergence = parMFX->mfx.Convergence;
        parUMC->HRDBufferSizeBytes = 0;
        parUMC->HRDInitialDelayBytes = 0;
        parUMC->maxBitrate = parMFX->mfx.TargetKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;
    }
    else
    {
        parUMC->maxBitrate = parMFX->mfx.MaxKbps * brcParamMultiplier * BRC_BITS_IN_KBIT;
        parUMC->HRDBufferSizeBytes = parMFX->mfx.BufferSizeInKB * brcParamMultiplier * BRC_BYTES_IN_KBYTE;
        parUMC->HRDInitialDelayBytes = parMFX->mfx.InitialDelayInKB * brcParamMultiplier * BRC_BYTES_IN_KBYTE;
    }

    parUMC->info.clip_info.width = parMFX->mfx.FrameInfo.Width;
    parUMC->info.clip_info.height = parMFX->mfx.FrameInfo.Height;

    parUMC->GOPPicSize = parMFX->mfx.GopPicSize;
    parUMC->GOPRefDist = parMFX->mfx.GopRefDist;

    parUMC->info.framerate = CalculateUMCFramerate(parMFX->mfx.FrameInfo.FrameRateExtN, parMFX->mfx.FrameInfo.FrameRateExtD);
    parUMC->frameRateExtD = parMFX->mfx.FrameInfo.FrameRateExtD;
    parUMC->frameRateExtN = parMFX->mfx.FrameInfo.FrameRateExtN;
    if (parUMC->info.framerate <= 0) {
        parUMC->info.framerate = 30;
        parUMC->frameRateExtD = 1;
        parUMC->frameRateExtN = 30;
    }

    return MFX_ERR_NONE;
}
#endif

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE) || defined (MFX_ENABLE_H265_VIDEO_ENCODE)

mfxExtBuffer* Hevc_GetExtBuffer(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id)
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

mfxStatus cBRCParams::Init(mfxVideoParam* par, bool bField)
{
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(par->mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
              par->mfx.RateControlMethod == MFX_RATECONTROL_VBR,
              MFX_ERR_UNDEFINED_BEHAVIOR);
    bFieldMode = bField;
    codecId = par->mfx.CodecId;

    mfxU32 k  = par->mfx.BRCParamMultiplier == 0 ?  1: par->mfx.BRCParamMultiplier;
    targetbps = k*par->mfx.TargetKbps * 1000;
    maxbps    = k*par->mfx.MaxKbps * 1000;

    maxbps = (par->mfx.RateControlMethod == MFX_RATECONTROL_CBR) ?
        targetbps : ((maxbps >= targetbps) ? maxbps : targetbps);

    mfxU32 bit_rate_scale = (par->mfx.CodecId == MFX_CODEC_AVC) ?
        h264_bit_rate_scale : hevcBitRateScale(maxbps);
    mfxU32 cpb_size_scale = (par->mfx.CodecId == MFX_CODEC_AVC) ?
        h264_cpb_size_scale : hevcCbpSizeScale(maxbps);

    rateControlMethod  = par->mfx.RateControlMethod;
    maxbps =    ((maxbps >> (6 + bit_rate_scale)) << (6 + bit_rate_scale));

    mfxExtCodingOption * pExtCO = (mfxExtCodingOption*)Hevc_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION);

    HRDConformance = MFX_BRC_NO_HRD;
    if (pExtCO)
    {
        if (!IsOff(pExtCO->NalHrdConformance) && !IsOff(pExtCO->VuiNalHrdParameters))
            HRDConformance = MFX_BRC_HRD_STRONG;
        else if (IsOn(pExtCO->NalHrdConformance) && IsOff(pExtCO->VuiNalHrdParameters))
            HRDConformance = MFX_BRC_HRD_WEAK;
    }

    if (HRDConformance != MFX_BRC_NO_HRD)
    {
        bufferSizeInBytes  = ((k*par->mfx.BufferSizeInKB*1000) >> (cpb_size_scale + 1)) << (cpb_size_scale + 1);
        initialDelayInBytes =((k*par->mfx.InitialDelayInKB*1000) >> (cpb_size_scale + 1)) << (cpb_size_scale + 1);
        bRec = 1;
        bPanic = (HRDConformance == MFX_BRC_HRD_STRONG) ? 1 : 0;
    }
    MFX_CHECK (par->mfx.FrameInfo.FrameRateExtD != 0 &&
               par->mfx.FrameInfo.FrameRateExtN != 0,
               MFX_ERR_UNDEFINED_BEHAVIOR);

    frameRate = (mfxF64)par->mfx.FrameInfo.FrameRateExtN / (mfxF64)par->mfx.FrameInfo.FrameRateExtD;

    width = par->mfx.FrameInfo.Width;
    height =par->mfx.FrameInfo.Height;

    chromaFormat = par->mfx.FrameInfo.ChromaFormat == 0 ?  MFX_CHROMAFORMAT_YUV420 : par->mfx.FrameInfo.ChromaFormat ;
    bitDepthLuma = par->mfx.FrameInfo.BitDepthLuma == 0 ?  8 : par->mfx.FrameInfo.BitDepthLuma;

    quantOffset   = 6 * (bitDepthLuma - 8);

    inputBitsPerFrame    = targetbps / frameRate;
    maxInputBitsPerFrame = maxbps / frameRate;
    gopPicSize = par->mfx.GopPicSize*(bFieldMode ? 2 : 1);
    gopRefDist = par->mfx.GopRefDist*(bFieldMode ? 2 : 1);

    mfxExtCodingOption2 * pExtCO2 = (mfxExtCodingOption2*)Hevc_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION2);
    bPyr = (pExtCO2 && pExtCO2->BRefType == MFX_B_REF_PYRAMID);
    maxFrameSizeInBits  = pExtCO2 ? pExtCO2->MaxFrameSize*8 : 0;

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

    if (pExtCO2
        && pExtCO2->MaxQPI <=51 && pExtCO2->MaxQPI > pExtCO2->MinQPI && pExtCO2->MinQPI >=1
        && pExtCO2->MaxQPP <=51 && pExtCO2->MaxQPP > pExtCO2->MinQPP && pExtCO2->MinQPP >=1
        && pExtCO2->MaxQPB <=51 && pExtCO2->MaxQPB > pExtCO2->MinQPB && pExtCO2->MinQPB >=1 )
    {
        quantMaxI = pExtCO2->MaxQPI + quantOffset;
        quantMinI = pExtCO2->MinQPI;
        quantMaxP = pExtCO2->MaxQPP + quantOffset;
        quantMinP = pExtCO2->MinQPP;
        quantMaxB = pExtCO2->MaxQPB + quantOffset;
        quantMinB = pExtCO2->MinQPB;
    }
    else
    {
        quantMaxI = quantMaxP = quantMaxB = 51 + quantOffset;
        quantMinI = quantMinP = quantMinB = 1;
    }

    mfxExtCodingOption3 * pExtCO3 = (mfxExtCodingOption3*)Hevc_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION3);
    if (pExtCO3)
    {
        WinBRCMaxAvgKbps = static_cast<mfxU16>(pExtCO3->WinBRCMaxAvgKbps * k);
        WinBRCSize = pExtCO3->WinBRCSize;
    }

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

mfxStatus   cBRCParams::GetBRCResetType(mfxVideoParam* par, bool bNewSequence, bool &bBRCReset, bool &bSlidingWindowReset)
{
    bBRCReset = false;
    bSlidingWindowReset = false;

    if (bNewSequence)
        return MFX_ERR_NONE;

    cBRCParams new_par;
    mfxStatus sts = new_par.Init(par);
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
        MFX_CHECK(new_par.bufferSizeInBytes   == bufferSizeInBytes, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
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

// Get QP Offset for given frame and Adaptive Pyramid QP class
// level = Pyramid level or Layer for 8GOP Pyramid, value [1-3]
// isRef = zero for non-reference frame
// clsAPQ = Adaptive Pyramid QP class, value [0-1]
// -----------------------------------
//                Offset Table
// clsAPQ | level1   level2   level3
// -----------------------------------
//        | ref nref ref nref ref nref
// 0      | 0   1    1   2    2   3
// 1      | 2   2    3   3    5   5
// -----------------------------------
// QP Offset is realtive QuantB.
// QuantB = QuantP+1
// clsAPQ=0, can be for used non 8GOP and/or non Pyramid cases.

inline mfxI32 GetOffsetAPQ(mfxI32 level, mfxU16 isRef, mfxU16 clsAPQ)
{
    mfxI32 qp = 0;
    level = std::max(mfxI32(1), std::min(mfxI32(3), level));
    if (clsAPQ == 1) {
        switch (level) {
        case 3:
            qp += 2;
        case 2:
            qp += 0;
        case 1:
        default:
            qp += 3;
            break;
        }
    }
    else {
        qp += (level > 0 ? level - 1 : 0);
        if (level && !isRef) qp += 1;
    }
    return qp;
}

namespace MfxHwH265EncodeBRC
{

bool isFieldMode(mfxVideoParam *par)
{
    return ((par->mfx.CodecId == MFX_CODEC_HEVC) && !(par->mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE));
}

mfxStatus ExtBRC::Init (mfxVideoParam* par)
{
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK(!m_bInit, MFX_ERR_UNDEFINED_BEHAVIOR);
    sts = m_par.Init(par, isFieldMode(par));
    MFX_CHECK_STS(sts);

    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        if (m_par.codecId == MFX_CODEC_AVC)
            m_hrdSpec.reset(new H264_HRD());
        else
            m_hrdSpec.reset(new HEVC_HRD());
        m_hrdSpec->Init(m_par);

    }
    memset(&m_ctx, 0, sizeof(m_ctx));

    m_ctx.fAbLong  = m_par.inputBitsPerFrame;
    m_ctx.fAbShort = m_par.inputBitsPerFrame;

    mfxI32 rawSize = GetRawFrameSize(m_par.width * m_par.height ,m_par.chromaFormat, m_par.bitDepthLuma);
    mfxI32 qp = GetNewQP(rawSize, m_par.inputBitsPerFrame, m_par.quantMinI, m_par.quantMaxI, 1 , m_par.quantOffset, 0.5, false, false);

    UpdateQPParams(qp,MFX_FRAMETYPE_IDR , m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, MFX_FRAMETYPE_REF, 0);

    m_ctx.dQuantAb = qp > 0 ? 1./qp : 1.0; //kw

    if (m_par.WinBRCSize)
    {
        m_avg.reset(new AVGBitrate(m_par.WinBRCSize, (mfxU32)(m_par.WinBRCMaxAvgKbps*1000.0/m_par.frameRate), (mfxU32)m_par.inputBitsPerFrame) );
        MFX_CHECK_NULL_PTR1(m_avg.get());
    }

    m_bInit = true;
    return sts;
}

// Get QP for current frame
mfxI32 ExtBRC::GetCurQP (mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ)
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
        qp += GetOffsetAPQ(layer, isRef, clsAPQ);
        qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }
    //printf("GetCurQP IDR %d I %d P %d B %d, min %d max %d type %d \n", m_ctx.QuantIDR, m_ctx.QuantI, m_ctx.QuantP, m_ctx.QuantB, m_par.quantMinI, m_par.quantMaxI, type);

    return qp;
}

mfxF64 ExtBRC::ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong, mfxU32 eo, bool bIdr, mfxU16 clsAPQ)
{
    mfxI32 seqQP_new = GetSeqQP(qp, type, layer, isRef, clsAPQ);
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

// Get P-QP from QP of given frame
mfxI32 ExtBRC::GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ)
{
    mfxI32 pqp = 0;
    if (type == MFX_FRAMETYPE_IDR) {
        pqp = qp + m_par.iDQp + 1;
    } else if (type == MFX_FRAMETYPE_I) {
        pqp = qp + 1;
    } else if (type == MFX_FRAMETYPE_P) {
        pqp = qp - layer;
    } else {
        qp -= GetOffsetAPQ(layer, isRef, clsAPQ);
        pqp = qp - 1;
    }
    pqp = mfx::clamp(pqp, m_par.quantMinP, m_par.quantMaxP);

    return pqp;
}

// Get QP from P-QP and given frametype, layer, ref and Adaptive Pyramid QP class.
mfxI32 ExtBRC::GetPicQP(mfxI32 pqp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 clsAPQ)
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
        qp = pqp + layer;
        qp = mfx::clamp(qp, m_par.quantMinP, m_par.quantMaxP);
    }
    else
    {
        qp = pqp + 1;
        qp += GetOffsetAPQ(layer, isRef, clsAPQ);
        qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }

    return qp;
}


mfxStatus ExtBRC::Update(mfxBRCFrameParam* frame_par, mfxBRCFrameCtrl* frame_ctrl, mfxBRCFrameStatus* status)
{
    mfxU16 ParClassAPQ = 0; // default
    // Use optimal Pyramid QPs for HEVC 8 GOP Pyramid coding
    if (m_par.gopRefDist == 8 && m_par.bPyr && m_par.codecId == MFX_CODEC_HEVC) ParClassAPQ = 1;

#if (MFX_VERSION >= 1026)
    mfxU16 ParSceneChange = frame_par->SceneChange;
    mfxU32 ParFrameCmplx = frame_par->FrameCmplx;
#else
    mfxU16 ParSceneChange = 0;
    mfxU32 ParFrameCmplx = 0;
#endif
    mfxStatus sts       = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR3(frame_par, frame_ctrl, status);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxU16 &brcSts       = status->BRCStatus;
    status->MinFrameSize  = 0;

    if (frame_par->NumRecode)
       m_ReEncodeCount++;
    if (frame_par->NumRecode > 100)
        m_SkipCount++;
    //printf("ExtBRC::Update:  m_ctx.encOrder %d , frame_par->EncodedOrder %d, frame_par->NumRecode %d, frame_par->CodedFrameSize %d, qp %d ClassAPQ %d\n", m_ctx.encOrder , frame_par->EncodedOrder, frame_par->NumRecode, frame_par->CodedFrameSize, frame_ctrl->QpY, ParClassAPQ);

    mfxI32 bitsEncoded  = frame_par->CodedFrameSize*8;
    mfxU32 picType      = GetFrameType(frame_par->FrameType, frame_par->PyramidLayer, m_par.gopRefDist);
    bool  bIdr          = (picType == MFX_FRAMETYPE_IDR);
    mfxI32 qpY          = frame_ctrl->QpY + m_par.quantOffset;
    mfxI32 layer        = frame_par->PyramidLayer;
    mfxF64 qstep        = QP2Qstep(qpY, m_par.quantOffset);

    mfxF64 fAbLong  = m_ctx.fAbLong   + (bitsEncoded - m_ctx.fAbLong)  / m_par.fAbPeriodLong;
    mfxF64 fAbShort = m_ctx.fAbShort  + (bitsEncoded - m_ctx.fAbShort) / m_par.fAbPeriodShort;
    mfxF64 eRate    = bitsEncoded * sqrt(qstep);

    mfxF64 e2pe     =  0;
    bool bMaxFrameSizeMode = m_par.maxFrameSizeInBits != 0 &&
        m_par.maxFrameSizeInBits < m_par.inputBitsPerFrame * 2 &&
        m_ctx.totalDeviation < (-1)*m_par.inputBitsPerFrame*m_par.frameRate;

    if (IS_IFRAME(picType)) {
        e2pe = (m_ctx.eRateSH == 0) ? (BRC_SCENE_CHANGE_RATIO2 + 1) : eRate / m_ctx.eRateSH;
        if(ParSceneChange && e2pe <= BRC_SCENE_CHANGE_RATIO2 && m_ctx.eRate)
            e2pe = eRate / m_ctx.eRate;
    } else {
        e2pe = (m_ctx.eRate == 0) ? (BRC_SCENE_CHANGE_RATIO2 + 1) : eRate / m_ctx.eRate;
    }
    mfxU32 frameSizeLim    = 0xfffffff ; // sliding window limitation or external frame size limitation

    bool  bSHStart = false;
    bool  bNeedUpdateQP = false;

    brcSts = MFX_BRC_OK;

    if (m_par.bRec && m_ctx.bToRecode &&  (m_ctx.encOrder != frame_par->EncodedOrder || frame_par->NumRecode == 0))
    {
        //printf("++++++++++++++++++++++++++++++++++\n");
        // Frame must be recoded, but encoder calls BR for another frame
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if (frame_par->NumRecode == 0)
    {
        // Set context for new frame
        if (IS_IFRAME(picType)) {
            m_ctx.LastIEncOrder = frame_par->EncodedOrder;
            if (bIdr)
            {
                m_ctx.LastIDREncOrder = frame_par->EncodedOrder;
                m_ctx.LastIDRSceneChange = ParSceneChange;
            }
        }
        m_ctx.encOrder = frame_par->EncodedOrder;
        m_ctx.poc = frame_par->DisplayOrder;
        m_ctx.bToRecode = 0;
        m_ctx.bPanic = 0;

        if (IS_IFRAME(picType))
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

        if (m_ctx.SceneChange && ( m_ctx.poc > m_ctx.SChPoc + 1 || m_ctx.poc == 0))
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
        brcSts = CheckHrdAndUpdateQP(*m_hrdSpec.get(), bitsEncoded, frame_par->EncodedOrder, bIdr, qpY);

        MFX_CHECK(brcSts == MFX_BRC_OK || (!m_ctx.bPanic), MFX_ERR_NOT_ENOUGH_BUFFER);
        if (brcSts == MFX_BRC_OK && !m_ctx.bPanic)
            bNeedUpdateQP = true;

        status->MinFrameSize = m_hrdSpec->GetMinFrameSizeInBits(frame_par->EncodedOrder,bIdr) + 7;

        //printf("%d: poc %d, size %d QP %d (%d %d), HRD sts %d, maxFrameSize %d, type %d \n",frame_par->EncodedOrder, frame_par->DisplayOrder, bitsEncoded, m_ctx.Quant, m_ctx.QuantMin, m_ctx.QuantMax, brcSts,  m_hrd.GetMaxFrameSize(), frame_par->FrameType);
    }
    if ((e2pe > BRC_SCENE_CHANGE_RATIO2  && bitsEncoded > 4 * m_par.inputBitsPerFrame) ||
        (IS_IFRAME(picType) && ParFrameCmplx > 0 && frame_par->EncodedOrder == m_ctx.LastIEncOrder // We could set Qp
          && (ParSceneChange > 0 && m_ctx.LastIQpSet == m_ctx.LastIQpMin))                         // We did set Qp and/or was SceneChange
        )
    {
        // scene change, resetting BRC statistics
        m_ctx.fAbLong  = m_par.inputBitsPerFrame;
        m_ctx.fAbShort = m_par.inputBitsPerFrame;
        fAbLong = m_ctx.fAbLong + (bitsEncoded - m_ctx.fAbLong) / m_par.fAbPeriodLong;
        fAbShort = m_ctx.fAbShort + (bitsEncoded - m_ctx.fAbShort) / m_par.fAbPeriodShort;
        m_ctx.SceneChange |= 1;
        if (picType != MFX_FRAMETYPE_B)
        {
            bSHStart = true;
            m_ctx.dQuantAb = ResetQuantAb(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong, frame_par->EncodedOrder, bIdr, ParClassAPQ);
            m_ctx.SceneChange |= 16;
            m_ctx.eRateSH = eRate;
            m_ctx.SChPoc = frame_par->DisplayOrder;
            //printf("!!!!!!!!!!!!!!!!!!!!! %d m_ctx.SceneChange %d, order %d\n", frame_par->EncodedOrder, m_ctx.SceneChange, frame_par->DisplayOrder);
            if (picType == MFX_FRAMETYPE_P && bitsEncoded > 4 * m_par.inputBitsPerFrame) ResetMinQForMaxFrameSize(&m_par, picType);
        }
    }

    if (m_avg.get())
    {
       frameSizeLim = std::min (frameSizeLim, m_avg->GetMaxFrameSize(m_ctx.bPanic, bSHStart || IS_IFRAME(picType), frame_par->NumRecode));
    }
    if (m_par.maxFrameSizeInBits)
    {
        frameSizeLim = std::min (frameSizeLim, m_par.maxFrameSizeInBits);
    }
    //printf("frameSizeLim %d (%d)\n", frameSizeLim, bitsEncoded);
    if (frame_par->NumRecode < 100)
        UpdateMinQForMaxFrameSize(&m_par, bitsEncoded, qpY, m_ctx, picType, bSHStart, brcSts);

    if (frame_par->NumRecode < 2)
    // Check other condions for recoding (update qp if it is needed)
    {
        mfxF64 targetFrameSize = std::max<mfxF64>(m_par.inputBitsPerFrame, fAbLong);
        mfxF64 dqf = (m_par.bFieldMode) ? 1.0 : BRC_DQF(picType, m_par.iDQp, ((picType == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), (ParSceneChange || m_ctx.encOrder == 0));
        mfxF64 maxFrameSizeByRatio = dqf * BRC_FRM_RATIO(picType, m_ctx.encOrder, bSHStart, m_par.bPyr) * targetFrameSize;
        if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance != MFX_BRC_NO_HRD) {

            mfxF64 bufferDeviation = m_hrdSpec->GetBufferDeviation(frame_par->EncodedOrder);

            //printf("bufferDeviation %f\n", bufferDeviation);
            mfxF64 dev = -1.0*maxFrameSizeByRatio - bufferDeviation;
            if (dev > 0) maxFrameSizeByRatio += (std::min)(maxFrameSizeByRatio, (dev / (IS_IFRAME(picType) ? 2.0 : 4.0)));
        }

        mfxI32 quantMax = m_ctx.QuantMax;
        mfxI32 quantMin = m_ctx.QuantMin;
        mfxI32 quant = qpY;

        mfxF64 maxFrameSize = std::min<mfxF64>(maxFrameSizeByRatio, frameSizeLim);

        if (m_par.HRDConformance != MFX_BRC_NO_HRD)
        {

            mfxF64 maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frame_par->EncodedOrder,bIdr);
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

        if (bitsEncoded >  maxFrameSize && quant < quantMax)
        {
            mfxI32 quant_new = GetNewQP(bitsEncoded, (mfxU32)maxFrameSize, quantMin , quantMax, quant ,m_par.quantOffset, 1);
            if (quant_new > quant)
            {
                bNeedUpdateQP = false;
                //printf("    recode 1-0: %d:  k %5f bitsEncoded %d maxFrameSize %d, targetSize %d, fAbLong %f, inputBitsPerFrame %f, qp %d new %d, layer %d\n",frame_par->EncodedOrder, bitsEncoded/maxFrameSize, (int)bitsEncoded, (int)maxFrameSize,(int)targetFrameSize, fAbLong, m_par.inputBitsPerFrame, quant, quant_new, layer);
                if (quant_new > GetCurQP(picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong, frame_par->EncodedOrder, bIdr, ParClassAPQ);
                }

                if (m_par.bRec)
                {
                    SetRecodeParams(MFX_BRC_BIG_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, status);
                    return sts;
                }
            } //(quant_new > quant)
        } //bitsEncoded >  maxFrameSize

        mfxF64 lFR = std::min(m_par.gopPicSize - 1, 4);
        mfxF64 lowFrameSizeI = std::min(maxFrameSize, lFR *(mfxF64)m_par.inputBitsPerFrame);
        // Did we set the qp?
        if (IS_IFRAME(picType) && ParFrameCmplx > 0                                                     // We could set Qp
            && frame_par->EncodedOrder == m_ctx.LastIEncOrder && m_ctx.LastIQpSet == m_ctx.LastIQpMin   // We did set Qp
            && frame_par->NumRecode == 0 && bitsEncoded <  (lowFrameSizeI/2.0)  && quant > quantMin)    // We can & should recode
        {
            // too small; do something
            mfxI32 quant_new = GetNewQP(bitsEncoded, (mfxU32)lowFrameSizeI, quantMin, quantMax, quant, m_par.quantOffset, 0.78, false, true);
            if (quant_new < quant)
            {
                bNeedUpdateQP = false;
                if (quant_new < GetCurQP(picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong,frame_par->EncodedOrder, bIdr, ParClassAPQ);
                }

                if (m_par.bRec)
                {
                    SetRecodeParams(MFX_BRC_SMALL_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, status);
                    return sts;
                }
            } //(quant_new < quant)
        }

        if (bitsEncoded >  maxFrameSize && quant == quantMax &&
            !IS_IFRAME(picType) && m_par.bPanic &&
            (!m_ctx.bPanic) && isFrameBeforeIntra(m_ctx.encOrder, m_ctx.LastIEncOrder, m_par.gopPicSize, m_par.gopRefDist))
        {
            //skip frames before intra
            SetRecodeParams(MFX_BRC_PANIC_BIG_FRAME,quant,quant, quantMin ,quantMax, m_ctx, status);
            return sts;
        }
        if (m_par.HRDConformance != MFX_BRC_NO_HRD && frame_par->NumRecode == 0 && (quant < quantMax))
        {

            mfxF64 maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frame_par->EncodedOrder, bIdr);

            mfxF64 FAMax = 1./9. * maxFrameSizeHrd + 8./9. * fAbLong;

            if (fAbShort > FAMax)
            {
                mfxI32 quant_new = GetNewQP(fAbShort, FAMax, quantMin , quantMax, quant ,m_par.quantOffset, 0.5);
                //printf("============== recode 2-0: %d:  FAMax %f, fAbShort %f, quant_new %d\n",frame_par->EncodedOrder, FAMax, fAbShort, quant_new);

                if (quant_new > quant)
                {
                   bNeedUpdateQP = false;
                   if (quant_new > GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ))
                   {
                        UpdateQPParams(quant_new ,picType, m_ctx, 0, quantMin , quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
                        fAbLong  = m_ctx.fAbLong  = m_par.inputBitsPerFrame;
                        fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                        m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong, frame_par->EncodedOrder, bIdr, ParClassAPQ);
                    }
                    if (m_par.bRec)
                    {
                        SetRecodeParams(MFX_BRC_BIG_FRAME,quant,quant_new, quantMin, quantMax, m_ctx, status);
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
            quant_new = GetNewQP(bitsEncoded, frameSizeLim, m_ctx.QuantMin , m_ctx.QuantMax,quant,m_par.quantOffset, 1, true);
        }
        else if (brcSts == MFX_BRC_BIG_FRAME || brcSts == MFX_BRC_SMALL_FRAME)
        {
            mfxF64 targetSize = GetFrameTargetSize(brcSts,
                m_hrdSpec->GetMinFrameSizeInBits(frame_par->EncodedOrder, bIdr),
                m_hrdSpec->GetMaxFrameSizeInBits(frame_par->EncodedOrder, bIdr));

            quant_new = GetNewQP(bitsEncoded, targetSize, m_ctx.QuantMin , m_ctx.QuantMax,quant,m_par.quantOffset, 1, true);
        }
        if (quant_new != quant)
        {
            if (brcSts == MFX_BRC_SMALL_FRAME)
            {
               quant_new = std::max(quant_new, quant-2);
               brcSts = MFX_BRC_PANIC_SMALL_FRAME;
            }
            // Idea is to check a sign mismatch, 'true' if both are negative or positive
            if ((quant_new - qpY) * (quant_new - GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ)) > 0)
            {
                UpdateQPParams(quant_new ,picType, m_ctx, 0, m_ctx.QuantMin , m_ctx.QuantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
            }
            bNeedUpdateQP = false;
        }
        SetRecodeParams(brcSts,quant,quant_new, m_ctx.QuantMin , m_ctx.QuantMax, m_ctx, status);
        //printf("===================== recode 1-0: HRD recode: quant_new %d\n", quant_new);
    }
    else
    {
        // no recoding are needed. Save context params

        mfxF64 k = 1./ GetSeqQP(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
        mfxF64 dqAbPeriod = m_par.dqAbPeriod;
        if (m_ctx.bToRecode)
            dqAbPeriod = (k < m_ctx.dQuantAb)? 16:25;

        if (bNeedUpdateQP)
        {
            m_ctx.dQuantAb += (k - m_ctx.dQuantAb)/dqAbPeriod;
            m_ctx.dQuantAb = mfx::clamp(m_ctx.dQuantAb, 1. / m_ctx.QuantMax, 1.);

            m_ctx.fAbLong  = fAbLong;
            m_ctx.fAbShort = fAbShort;
        }

        bool oldScene = false;
        if ((m_ctx.SceneChange & 16) && (m_ctx.poc < m_ctx.SChPoc) && (e2pe < .01) && (mfxF64)bitsEncoded < 1.5*fAbLong)
            oldScene = true;
        //printf("-- m_ctx.eRate %f,  eRate %f, e2pe %f\n", m_ctx.eRate,  eRate, e2pe );

        if (!m_ctx.bPanic && frame_par->NumRecode < 100)
        {
            if (picType != MFX_FRAMETYPE_B)
            {
                m_ctx.LastNonBFrameSize = bitsEncoded;
                if (IS_IFRAME(picType))
                {
                    m_ctx.eRateSH = eRate;
                    if (ParSceneChange)
                        m_ctx.eRate = m_par.inputBitsPerFrame * sqrt(QP2Qstep(GetCurQP(MFX_FRAMETYPE_P, 0, MFX_FRAMETYPE_REF, 0), m_par.quantOffset));
                }
                else
                {
                    m_ctx.eRate = eRate;
                    if (m_ctx.eRate > m_ctx.eRateSH) m_ctx.eRateSH = m_ctx.eRate;
                }
            }

            if (IS_IFRAME(picType))
            {
                m_ctx.LastIFrameSize = bitsEncoded;
                m_ctx.LastIQpAct = qpY;
            }
        }

        if (m_avg.get())
        {
            m_avg->UpdateSlidingWindow(bitsEncoded, m_ctx.encOrder, m_ctx.bPanic, bSHStart || IS_IFRAME(picType),frame_par->NumRecode, qpY);
        }

        m_ctx.totalDeviation += ((mfxF64)bitsEncoded -m_par.inputBitsPerFrame);

        //printf("------------------ %d (%d)) Total deviation %f, old scene %d, bNeedUpdateQP %d, m_ctx.Quant %d, type %d, m_ctx.fAbLong %f m_par.inputBitsPerFrame %f\n", frame_par->EncodedOrder, frame_par->DisplayOrder,m_ctx.totalDeviation, oldScene , bNeedUpdateQP, m_ctx.Quant,picType, m_ctx.fAbLong, m_par.inputBitsPerFrame);

        if (m_par.HRDConformance != MFX_BRC_NO_HRD)
        {
            m_hrdSpec->Update(bitsEncoded, frame_par->EncodedOrder, bIdr);
        }

        if (!m_ctx.bPanic&& (!oldScene) && bNeedUpdateQP)
        {
            mfxI32 quant_new = qpY;

            //Update QP

            mfxF64 totDev = m_ctx.totalDeviation;
            mfxF64 HRDDevFactor = 0.0;
            mfxF64 HRDDev = 0.0;
            mfxF64 maxFrameSizeHrd = 0.0;
            if (m_par.HRDConformance != MFX_BRC_NO_HRD)
            {

                HRDDevFactor = m_hrdSpec->GetBufferDeviationFactor(frame_par->EncodedOrder);
                HRDDev = m_hrdSpec->GetBufferDeviation(frame_par->EncodedOrder);
                maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(frame_par->EncodedOrder, bIdr);
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
            quant_new = GetNewQPTotal(totDev / bAbPreriod / (mfxF64)m_par.inputBitsPerFrame, dequant_new, m_ctx.QuantMin, m_ctx.QuantMax, GetSeqQP(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ), m_par.bPyr && m_par.bRec, bSHStart && m_ctx.bToRecode == 0);
            quant_new = GetPicQP(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
            //printf("    ===%d quant old %d quant_new %d, bitsEncoded %d m_ctx.QuantMin %d m_ctx.QuantMax %d\n", frame_par->EncodedOrder, m_ctx.Quant, quant_new, bitsEncoded, m_ctx.QuantMin, m_ctx.QuantMax);

            if (bMaxFrameSizeMode)
            {
                mfxF64 targetMax = ((mfxF64)m_par.maxFrameSizeInBits*((bSHStart || IS_IFRAME(picType)) ? 0.95 : 0.9));
                mfxF64 targetMin = ((mfxF64)m_par.maxFrameSizeInBits*((bSHStart || IS_IFRAME(picType)) ? 0.9  : 0.8 /*0.75 : 0.5*/));
                mfxI32 QuantNewMin = GetNewQP(bitsEncoded, targetMax, m_ctx.QuantMin, m_ctx.QuantMax, qpY, m_par.quantOffset, 1,false, false);
                mfxI32 QuantNewMax = GetNewQP(bitsEncoded, targetMin, m_ctx.QuantMin, m_ctx.QuantMax, qpY, m_par.quantOffset, 1,false, false);
                mfxI32 quant_corrected = qpY;

                if (quant_corrected < QuantNewMin - 3)
                    quant_corrected += 2;
                if (quant_corrected < QuantNewMin)
                    quant_corrected ++;
                else if (quant_corrected > QuantNewMax + 3)
                    quant_corrected -= 2;
                else if (quant_corrected > QuantNewMax)
                    quant_corrected--;

                //printf("   QuantNewMin %d, QuantNewMax %d, m_ctx.Quant %d, new %d (%d)\n", QuantNewMin, QuantNewMax, m_ctx.Quant, quant_corrected, quant_new);

                quant_new = mfx::clamp(quant_corrected, m_ctx.QuantMin, m_ctx.QuantMax);
            }

            if ((quant_new - qpY)* (quant_new - GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ)) > 0) // this check is actual for async scheme
            {
                //printf("   +++ Update QP %d: totalDeviation %f, bAbPreriod %f (%f), QP %d (%d %d), qp_new %d (qpY %d), type %d, dequant_new %f (%f) , m_ctx.fAbLong %f, m_par.inputBitsPerFrame %f\n",
                //    frame_par->EncodedOrder,totDev , bAbPreriod, GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIEncOrder, m_par.gopPicSize, m_ctx.LastIDRSceneChange), m_ctx.Quant, m_ctx.QuantMin, m_ctx.QuantMax,quant_new, qpY, picType, 1.0/dequant_new, 1.0/m_ctx.dQuantAb, m_ctx.fAbLong, m_par.inputBitsPerFrame);
                UpdateQPParams(quant_new ,picType, m_ctx, 0, m_ctx.QuantMin , m_ctx.QuantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
            }
        }
        m_ctx.bToRecode = 0;
    }
    return sts;

}

mfxStatus ExtBRC::GetFrameCtrl (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
{
    MFX_CHECK_NULL_PTR2(par, ctrl);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
    mfxU16 ParClassAPQ = 0;
    // Use optimal Pyramid QPs for HEVC 8 GOP Pyramid coding
    if (m_par.gopRefDist == 8 && m_par.bPyr && m_par.codecId == MFX_CODEC_HEVC) ParClassAPQ = 1;

#if (MFX_VERSION >= 1026)
    mfxU16 ParSceneChange = par->SceneChange;
    mfxU16 ParLongTerm = par->LongTerm;
    mfxU32 ParFrameCmplx = par->FrameCmplx;
#else
    mfxU16 ParSceneChange = 0;
    mfxU16 ParLongTerm = 0;
    mfxU32 ParFrameCmplx = 0;
#endif
    mfxI32 qp = 0;
    mfxI32 qpMin = 1;
    mfxU16 type = GetFrameType(par->FrameType, par->PyramidLayer, m_par.gopRefDist);
    bool  bIdr = (type == MFX_FRAMETYPE_IDR);


    mfxF64 HRDDev = 0.0;
    mfxF64 maxFrameSizeHrd = 0.0;
    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        m_hrdSpec->GetBufferDeviationFactor(par->EncodedOrder);
        HRDDev = m_hrdSpec->GetBufferDeviation(par->EncodedOrder);
        maxFrameSizeHrd = m_hrdSpec->GetMaxFrameSizeInBits(par->EncodedOrder, bIdr);
    }

    if (!m_bDynamicInit) {
        if (IS_IFRAME(type)) {
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
                if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qp0>3) qp0 -= 3; // use re-encoding for best results (maxFrameSizeGood)
                else if (raca == BRC_MIN_RACA && qp0>3)                           qp0 -= 3; // uncertainty; use re-encoding for best results
                ltrprintf("Qp0 %d\n", qp0);
                UpdateQPParams(qp0, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, par->FrameType & MFX_FRAMETYPE_REF, 0);
                qpMin = qp0;
            }
        }
        m_bDynamicInit = true;

    }

    if (par->EncodedOrder == m_ctx.encOrder || par->NumRecode)
    {
        qp = m_ctx.Quant;
    }
    else
    {
        if (IS_IFRAME(type))
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
                if (!ParLongTerm) {
                    m_par.iDQp = 0;
                } else {
                    mfxF64 maxFrameRatio = 2 * BRC_FRM_RATIO(type, par->EncodedOrder, 0, m_par.bPyr);
                    mfxF64 minFrameRatio = BRC_FRM_RATIO(type, 0, 0, m_par.bPyr);
                    maxFrameRatio = std::min(maxFrameRatio, (maxFrameSize / m_par.inputBitsPerFrame));
                    mfxU32 mNumRefsInGop = m_par.mNumRefsInGop;
                    if (m_ctx.LastIQpSetOrder) {
                        mfxU32 pastRefsInGop = (mfxU32)(std::max(1.0, (!m_par.bPyr ? (mfxF64)(par->EncodedOrder - m_ctx.LastIQpSetOrder) / (mfxF64)m_par.gopRefDist : (mfxF64)(par->EncodedOrder - m_ctx.LastIQpSetOrder) / 2.0)));
                        mNumRefsInGop = std::min(mNumRefsInGop, pastRefsInGop);
                    }
                    maxFrameRatio = std::min<mfxF64>(maxFrameRatio, mNumRefsInGop);
                    mfxF64 dqpmax = std::max(0.0, 6.0 * (log(maxFrameRatio / minFrameRatio) / log(2.0)));
                    mfxU32 iDQpMax = (mfxU32)(dqpmax + 0.5);
                    if (ParSceneChange) {
                        iDQpMax = mfx::clamp<mfxU32>(iDQpMax, 1, m_par.iDQp0);
                    }
                    else {
                        iDQpMax = mfx::clamp<mfxU32>(iDQpMax, 1, BRC_MAX_DQP_LTR);
                    }
                    m_par.iDQp = iDQpMax;
                    ltrprintf("FR %lf DQp %d\n", maxFrameRatio, m_par.iDQp);
                }
            }

            // Determine Min Qp
            if (ParFrameCmplx > 0) {
                mfxF64 raca = (mfxF64)ParFrameCmplx / BRC_RACA_SCALE;
                mfxF64 dqf = BRC_DQF(type, m_par.iDQp, ((type == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), ParSceneChange);
                mfxF64 targetFrameSize = dqf * BRC_FRM_RATIO(type, par->EncodedOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
                if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance != MFX_BRC_NO_HRD) {
                    // CBR HRD Buffer over flow has priority
                    mfxF64 dev = -1.0*targetFrameSize - HRDDev;
                    if (dev > 0) targetFrameSize += std::min(targetFrameSize, (dev/2.0));
                }

                targetFrameSize = std::min(maxFrameSize, targetFrameSize);
                mfxF64 CmplxRatio = 1.0;
                if (m_ctx.LastICmplx) CmplxRatio = ParFrameCmplx / m_ctx.LastICmplx;
                if (!ParSceneChange && m_ctx.LastICmplx && m_ctx.LastIQpAct && m_ctx.LastIFrameSize && CmplxRatio > 0.5 && CmplxRatio < 2.0)
                {
                    qpMin = compute_new_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca, m_ctx.LastIFrameSize, (mfxF64) m_ctx.LastICmplx / BRC_RACA_SCALE, m_ctx.LastIQpAct);
                    if (raca == BRC_MIN_RACA && qpMin>3)                                qpMin -= 3; // uncertainty; use re-encoding for best results
                }
                else
                {
                    qpMin = compute_first_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca);
                    if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qpMin>3) qpMin -= 3; // uncertainty; use re-encoding for best results
                    else if (raca == BRC_MIN_RACA && qpMin>3)                           qpMin -= 3; // uncertainty; use re-encoding for best results
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

            mfxF64 targetFrameSize = BRC_FRM_RATIO(ltype, par->EncodedOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
            if (m_par.bPyr && m_par.gopRefDist == 8)
                targetFrameSize *= ((ParClassAPQ == 1) ? 2.0 : 1.66);

            if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.HRDConformance != MFX_BRC_NO_HRD) {
                mfxF64 dev = -1.0*targetFrameSize - HRDDev;
                if (dev > 0) targetFrameSize += std::min(targetFrameSize, (dev/4.0));
            }
            targetFrameSize = std::min(maxFrameSize, targetFrameSize);
            qpMin = GetMinQForMaxFrameSize(m_par, targetFrameSize, ltype);
        }

        qp = GetCurQP(type, par->PyramidLayer, par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);

        // Max Frame Size recode prevention
        if (qp < qpMin)
        {
            if (type != MFX_FRAMETYPE_B)
            {
                SetQPParams(qpMin, type, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
                qp = GetCurQP(type, par->PyramidLayer, par->FrameType & MFX_FRAMETYPE_REF, ParClassAPQ);
            }
            else
            {
                qp = qpMin;
            }
        }
        else
            qpMin = std::min(qp - 1, qpMin);
    }
    ctrl->QpY = qp - m_par.quantOffset;
    if (m_par.HRDConformance != MFX_BRC_NO_HRD)
    {
        ctrl->InitialCpbRemovalDelay = m_hrdSpec->GetInitCpbRemovalDelay(par->EncodedOrder);
        ctrl->InitialCpbRemovalOffset = m_hrdSpec->GetInitCpbRemovalDelayOffset(par->EncodedOrder);
    }
    //printf("EncOrder %d ctrl->QpY %d, qp %d quantOffset %d Cmplx %lf\n", par->EncodedOrder, ctrl->QpY , qp , m_par.quantOffset, par->FrameCmplx);

    if (IS_IFRAME(type)) {
        m_ctx.LastIQpSetOrder = par->EncodedOrder;
        m_ctx.LastIQpMin = qpMin - m_par.quantOffset;
        m_ctx.LastIQpSet = ctrl->QpY;
        m_ctx.LastIQpAct = 0;
        m_ctx.LastICmplx = ParFrameCmplx;
        m_ctx.LastIFrameSize = 0;
        ResetMinQForMaxFrameSize(&m_par, type);
    }
    return MFX_ERR_NONE;
}


mfxStatus ExtBRC::Reset(mfxVideoParam *par )
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);

    mfxExtEncoderResetOption  * pRO = (mfxExtEncoderResetOption *)Hevc_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCODER_RESET_OPTION);
    if (pRO && pRO->StartNewSequence == MFX_CODINGOPTION_ON)
    {
        Close();
        sts = Init(par);
    }
    else
    {
        bool brcReset = false;
        bool slidingWindowReset = false;

        sts = m_par.GetBRCResetType(par, false, brcReset, slidingWindowReset);
        MFX_CHECK_STS(sts);

        if (brcReset)
        {
            sts = m_par.Init(par, isFieldMode(par));
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


}


#endif // defined(MFX_ENABLE_VIDEO_BRC_COMMON)
