// Copyright (c) 2018 Intel Corporation
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
namespace MfxHwH265EncodeBRC
{

#define IS_IFRAME(pictype) ((pictype == MFX_FRAMETYPE_I || pictype == MFX_FRAMETYPE_IDR) ? MFX_FRAMETYPE_I: 0)
#define MAX_DQP_LTR 4
#define MAX_MODEL_ERR 6
#define BRC_BUFK 3.5
#define LTR_BUFK 4.5
#define LTR_BUF(type, dqp, boost, schg, shstrt) \
((type == MFX_FRAMETYPE_IDR) ? (((schg && !boost) || !dqp) ? BRC_BUFK : LTR_BUFK) : (shstrt ? BRC_BUFK : 2.5))

#define DQFF0 1.0
#define DQFF1 1.66
#define DQF(type, dqp, boost, schg) \
((type == MFX_FRAMETYPE_IDR) ? ((dqp?pow(2, ((mfxF64)dqp / 6.0)) : 1.0) * ((schg && !boost) ? DQFF0 : DQFF1)) : 1.0)

#define FRM_RATIO(type, encorder, shstrt, pyr) \
((((encorder == 0 && !pyr) || type == MFX_FRAMETYPE_I) ? 6.0 : (shstrt || type == MFX_FRAMETYPE_IDR) ? 8.0 : 4.0) * ((pyr) ? 1.5 : 1.0))

#define BRC_CONST_MUL_P1 2.253264596
#define BRC_CONST_EXP_R_P1 0.42406423

#define ltrprintf(...)
//#define ltrprintf printf

#define BRC_SCENE_CHANGE_RATIO1 20.0
#define BRC_SCENE_CHANGE_RATIO2 5.0


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

mfxI32 GetRawFrameSize(mfxU32 lumaSize, mfxU16 chromaFormat, mfxU16 bitDepthLuma)
{
    mfxI32 frameSize = lumaSize;

    if (chromaFormat == MFX_CHROMAFORMAT_YUV420)
        frameSize += lumaSize / 2;
    else if (chromaFormat == MFX_CHROMAFORMAT_YUV422)
        frameSize += lumaSize;
    else if (chromaFormat == MFX_CHROMAFORMAT_YUV444)
        frameSize += lumaSize * 2;

    frameSize = frameSize * bitDepthLuma / 8;
    return frameSize * 8; //frame size in bits
}

mfxStatus cBRCParams::Init(mfxVideoParam* par, bool fieldMode)
{
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(par->mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
              par->mfx.RateControlMethod == MFX_RATECONTROL_VBR,
              MFX_ERR_UNDEFINED_BEHAVIOR);
    bFieldMode = fieldMode;
    mfxU32 k = par->mfx.BRCParamMultiplier == 0 ?  1: par->mfx.BRCParamMultiplier;
    mfxU32 bpsScale  = (par->mfx.CodecId == MFX_CODEC_AVC) ? 10 : 6;

    rateControlMethod  = par->mfx.RateControlMethod;
    targetbps = (((k*par->mfx.TargetKbps*1000) >> bpsScale) << bpsScale);
    maxbps =    (((k*par->mfx.MaxKbps*1000) >> bpsScale) << bpsScale);

    maxbps = (par->mfx.RateControlMethod == MFX_RATECONTROL_CBR) ?
        targetbps : ((maxbps >= targetbps) ? maxbps : targetbps);

    mfxExtCodingOption * pExtCO = (mfxExtCodingOption*)Hevc_GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION);

    bHRDConformance =  (pExtCO && pExtCO->NalHrdConformance == MFX_CODINGOPTION_OFF) ? 0 : 1;

    if (bHRDConformance)
    {
        bufferSizeInBytes  = ((k*par->mfx.BufferSizeInKB*1000) >> 3) << 3;
        initialDelayInBytes =((k*par->mfx.InitialDelayInKB*1000) >> 3) << 3;
        bRec = 1;
        bPanic = 1;
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

    fAbPeriodLong = 100;
    fAbPeriodShort = 6;
    dqAbPeriod = 100;
    bAbPeriod = 100;

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

    mNumRefsInGop = (mfxU32)(MFX_MAX(1.0, (!bPyr ? (mfxF64)gopPicSize / (mfxF64)gopRefDist : (mfxF64)gopPicSize / 2.0)));

    mfxF64 maxFrameRatio = 1.5874 * FRM_RATIO(MFX_FRAMETYPE_IDR, 0, 0, bPyr);

    mIntraBoost = (mNumRefsInGop > maxFrameRatio * 8.0) ? 1 : 0;

    mfxF64 maxFrameSize = mRawFrameSizeInBits;
    if (maxFrameSizeInBits) {
        maxFrameSize = MFX_MIN(maxFrameSize, maxFrameSizeInBits);
    }
    if (bHRDConformance) {
        mfxF64 bufOccupy = LTR_BUF(MFX_FRAMETYPE_IDR, 1, mIntraBoost, 1, 0);
        maxFrameSize = MFX_MIN(maxFrameSize, bufOccupy / 9.* (initialDelayInBytes * 8.0) + (9.0 - bufOccupy) / 9.*inputBitsPerFrame);
    }

    mfxF64 minFrameRatio = FRM_RATIO(MFX_FRAMETYPE_IDR, 0, 0, bPyr);
    maxFrameRatio = MFX_MIN(maxFrameRatio, maxFrameSize / inputBitsPerFrame);
    maxFrameRatio = MFX_MIN(maxFrameRatio, mNumRefsInGop);
    mfxF64 dqp = MFX_MAX(0, 6.0 * (log(maxFrameRatio / minFrameRatio) / log(2.0)));
    iDQp0 = (mfxU32)(dqp + 0.5);
    if (iDQp0 < 1) iDQp0 = 1;
    if (iDQp0 > MAX_DQP_LTR) iDQp0 = MAX_DQP_LTR;

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
    MFX_CHECK(new_par.bHRDConformance == bHRDConformance, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM) ;
    MFX_CHECK(new_par.frameRate == frameRate, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.width == width, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.height == height, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.chromaFormat == chromaFormat, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    MFX_CHECK(new_par.bitDepthLuma == bitDepthLuma, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    if (bHRDConformance)
    {
        MFX_CHECK(new_par.bufferSizeInBytes   == bufferSizeInBytes, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.initialDelayInBytes == initialDelayInBytes, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.targetbps == targetbps, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(new_par.maxbps == maxbps, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }
    else
    {
        if (new_par.targetbps != targetbps || new_par.maxbps != maxbps)
        {
            MFX_CHECK(new_par.rateControlMethod == MFX_RATECONTROL_VBR, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            bBRCReset = true;
        }
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



enum
{
    MFX_BRC_RECODE_NONE           = 0,
    MFX_BRC_RECODE_QP             = 1,
    MFX_BRC_RECODE_PANIC          = 2,
};

   mfxF64 const QSTEP[88] = {
         0.630,  0.707,  0.794,  0.891,  1.000,   1.122,   1.260,   1.414,   1.587,   1.782,   2.000,   2.245,   2.520,
         2.828,  3.175,  3.564,  4.000,  4.490,   5.040,   5.657,   6.350,   7.127,   8.000,   8.980,  10.079,  11.314,
        12.699, 14.254, 16.000, 17.959, 20.159,  22.627,  25.398,  28.509,  32.000,  35.919,  40.317,  45.255,  50.797,
        57.018, 64.000, 71.838, 80.635, 90.510, 101.594, 114.035, 128.000, 143.675, 161.270, 181.019, 203.187, 228.070,
        256.000, 287.350, 322.540, 362.039, 406.375, 456.140, 512.000, 574.701, 645.080, 724.077, 812.749, 912.280,
        1024.000, 1149.401, 1290.159, 1448.155, 1625.499, 1824.561, 2048.000, 2298.802, 2580.318, 2896.309, 3250.997, 3649.121,
        4096.000, 4597.605, 5160.637, 5792.619, 6501.995, 7298.242, 8192.000, 9195.209, 10321.273, 11585.238, 13003.989, 14596.485
    };


mfxI32 QStep2QpFloor(mfxF64 qstep, mfxI32 qpoffset = 0) // QSTEP[qp] <= qstep, return 0<=qp<=51+mQuantOffset
{
    uint8_t qp = uint8_t(std::upper_bound(QSTEP, QSTEP + 51 + qpoffset, qstep) - QSTEP);
    return qp > 0 ? qp - 1 : 0;
}

mfxI32 Qstep2QP(mfxF64 qstep, mfxI32 qpoffset = 0) // return 0<=qp<=51+mQuantOffset
{
    mfxI32 qp = QStep2QpFloor(qstep, qpoffset);
    return (qp == 51 + qpoffset || qstep < (QSTEP[qp] + QSTEP[qp + 1]) / 2) ? qp : qp + 1;
}
mfxF64 QP2Qstep(mfxI32 qp, mfxI32 qpoffset = 0)
{
    return QSTEP[MFX_MIN(51 + qpoffset, qp)];
}

mfxF64  cHRD::GetBufferDiviationFactor()
{
    if (m_buffSizeInBits == 0)
    {
        return 0.0;
    }
    mfxF64 targetFullness = m_buffSizeInBits / 2;
    mfxF64 factor = abs((targetFullness - (mfxF64) m_bufFullness) / targetFullness);
    return factor;
}

mfxF64  cHRD::GetBufferDiviation(mfxU32 targetBitrate)
{
    mfxI64 targetFullness = MFX_MIN(m_delayInBits, m_buffSizeInBits / 2);
    mfxI64 minTargetFullness = MFX_MIN(mfxU32(m_buffSizeInBits / 2),targetBitrate * 2); // half bufsize or 2 sec
    targetFullness = MFX_MAX(targetFullness , minTargetFullness);
     return targetFullness - m_bufFullness;
}

mfxF64  cHRD::GetBufferDiviation()
{
    mfxI64 targetFullness = MFX_MIN(m_delayInBits, m_buffSizeInBits / 2);
    return targetFullness - m_bufFullness;
}

mfxU16 cHRD::UpdateAndCheckHRD(mfxI32 frameBits, mfxI32 recode, mfxI32 minQuant, mfxI32 maxQuant)
{
    mfxU16 brcStatus = MFX_BRC_OK ;

    if (recode  == 0)
    {
        m_prevBufFullness = m_bufFullness;
        m_underflowQuant = minQuant - 1;
        m_overflowQuant  = maxQuant + 1;
    }
    else
    { // frame is being recoded - restore buffer state
        m_bufFullness = m_prevBufFullness;
        m_frameNum--;
    }

    m_maxFrameSize = (mfxI32)(m_bufFullness - 1);
    m_minFrameSize = (!m_bCBR)? 0 : (mfxI32)(m_bufFullness + 1 + 1 + m_inputBitsPerFrame - m_buffSizeInBits);
    if (m_minFrameSize < 0)
        m_minFrameSize = 0;

   mfxF64  bufFullness = m_bufFullness - frameBits;

    if (bufFullness < 2)
    {
        bufFullness = m_inputBitsPerFrame;
        brcStatus = MFX_BRC_BIG_FRAME;
        if (bufFullness > m_buffSizeInBits)
            bufFullness = m_buffSizeInBits;
    }
    else
    {
        bufFullness += m_inputBitsPerFrame;
        if (bufFullness > m_buffSizeInBits - 1)
        {
            bufFullness = m_buffSizeInBits - 1;
            if (m_bCBR)
                brcStatus = MFX_BRC_SMALL_FRAME;
        }
    }
    //printf("--- m_frameNum %d, m_bufFullness %f (%f), frameBits %d, m_buffSizeInBits %d, m_inputBitsPerFrame %f, brcStatus %d\n", m_frameNum, m_bufFullness, bufFullness, frameBits, m_buffSizeInBits, m_inputBitsPerFrame, brcStatus);
    m_frameNum++;
    if ( MFX_BRC_RECODE_PANIC == recode) // no use in changing QP
    {
        if (brcStatus == MFX_BRC_SMALL_FRAME)
            brcStatus =  MFX_BRC_PANIC_SMALL_FRAME ;
        if (brcStatus == MFX_BRC_BIG_FRAME)
            brcStatus =  MFX_BRC_PANIC_BIG_FRAME ;    }

    m_bufFullness = bufFullness;
    return brcStatus;
}

mfxStatus cHRD::UpdateMinMaxQPForRec( mfxU32 brcSts, mfxI32 qp)
{
    MFX_CHECK(brcSts == MFX_BRC_BIG_FRAME || brcSts == MFX_BRC_SMALL_FRAME, MFX_ERR_UNDEFINED_BEHAVIOR);
    if (brcSts == MFX_BRC_BIG_FRAME)
        m_underflowQuant = qp;
    else
        m_overflowQuant = qp;
    return MFX_ERR_NONE;
}
mfxI32 cHRD::GetTargetSize(mfxU32 brcSts)
{
     if (brcSts != MFX_BRC_BIG_FRAME && brcSts != MFX_BRC_SMALL_FRAME) return 0;
     return (brcSts == MFX_BRC_BIG_FRAME) ? m_maxFrameSize * 3 / 4 : m_minFrameSize * 5 / 4;
}

mfxI32 GetNewQP(mfxF64 totalFrameBits, mfxF64 targetFrameSizeInBits, mfxI32 minQP , mfxI32 maxQP, mfxI32 qp , mfxI32 qp_offset, mfxF64 f_pow, bool bStrict = false, bool bLim = true)
{
    mfxF64 qstep = 0, qstep_new = 0;
    mfxI32 qp_new = qp;

    qstep = QP2Qstep(qp, qp_offset);
    qstep_new = qstep * pow(totalFrameBits / targetFrameSizeInBits, f_pow);
    qp_new = Qstep2QP(qstep_new, qp_offset);

    if (totalFrameBits < targetFrameSizeInBits) // overflow
    {
        if (qp <= minQP)
        {
            return qp; // QP change is impossible
        }
        if (bLim)
            qp_new  = MFX_MAX (qp_new , (minQP + qp + 1) >> 1);
        if (bStrict)
            qp_new  = MFX_MIN (qp_new, qp - 1);
    }
    else // underflow
    {
        if (qp >= maxQP)
        {
            return qp; // QP change is impossible
        }
        if (bLim)
            qp_new  = MFX_MIN (qp_new , (maxQP + qp + 1) >> 1);
        if (bStrict)
            qp_new  = MFX_MAX (qp_new, qp + 1);
    }
    return mfx::clamp(qp_new, minQP, maxQP);
}


void cHRD::Init(mfxU32 buffSizeInBytes, mfxU32 delayInBytes, mfxF64 inputBitsPerFrame, bool bCBR)
{
    m_bufFullness = m_prevBufFullness= delayInBytes << 3;
    m_delayInBits = delayInBytes << 3;
    m_buffSizeInBits = buffSizeInBytes << 3;
    m_inputBitsPerFrame =inputBitsPerFrame;
    m_bCBR = bCBR;

    m_underflowQuant = 0;
    m_overflowQuant = 999;
    m_frameNum = 0;
    m_minFrameSize = 0;
    m_maxFrameSize = 0;

}

void SetQPParams(mfxI32 qp, mfxU32 type, BRC_Ctx  &ctx, mfxU32 /* rec_num */, mfxI32 minQuant, mfxI32 maxQuant, mfxU32 level, mfxU32 iDQp, mfxU16 isRef)
{
    if (type == MFX_FRAMETYPE_IDR)
    {
        ctx.QuantIDR = qp;
        ctx.QuantI = qp + iDQp;
        ctx.QuantP = qp + 1 + iDQp;
        ctx.QuantB = qp + 2 + iDQp;
    }
    else if (type == MFX_FRAMETYPE_I)
    {
        ctx.QuantIDR = qp - iDQp;
        ctx.QuantI = qp;
        ctx.QuantP = qp + 1;
        ctx.QuantB = qp + 2;
    }
    else if (type == MFX_FRAMETYPE_P)
    {
        qp -= level;
        ctx.QuantIDR = qp - 1 - iDQp;
        ctx.QuantI = qp - 1;
        ctx.QuantP = qp;
        ctx.QuantB = qp + 1;
    }
    else if (type == MFX_FRAMETYPE_B)
    {
        level = level > 0 ? level - 1 : 0;
        qp -= level;
        if (level && !isRef) qp -= 1;
        ctx.QuantIDR = qp - 2 - iDQp;
        ctx.QuantI = qp - 2;
        ctx.QuantP = qp - 1;
        ctx.QuantB = qp;
    }
    ctx.QuantIDR = mfx::clamp(ctx.QuantIDR, minQuant, maxQuant);
    ctx.QuantI   = mfx::clamp(ctx.QuantI,   minQuant, maxQuant);
    ctx.QuantP   = mfx::clamp(ctx.QuantP,   minQuant, maxQuant);
    ctx.QuantB   = mfx::clamp(ctx.QuantB,   minQuant, maxQuant);
    //printf("ctx.QuantIDR %d, QuantI %d, ctx.QuantP %d, ctx.QuantB  %d, level %d\n", ctx.QuantIDR, ctx.QuantI, ctx.QuantP, ctx.QuantB, level);
}

void UpdateQPParams(mfxI32 qp, mfxU32 type , BRC_Ctx  &ctx, mfxU32 rec_num, mfxI32 minQuant, mfxI32 maxQuant, mfxU32 level, mfxU32 iDQp, mfxU16 isRef)
{
    ctx.Quant = qp;
    if (ctx.LastIQpSetOrder > ctx.encOrder) return;

    SetQPParams(qp, type, ctx, rec_num, minQuant, maxQuant, level, iDQp, isRef);
}
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

    if (m_par.bHRDConformance)
    {
        m_hrd.Init(m_par.bufferSizeInBytes, m_par.initialDelayInBytes, m_par.maxInputBitsPerFrame, m_par.rateControlMethod == MFX_RATECONTROL_CBR);
    }
    memset(&m_ctx, 0, sizeof(m_ctx));

    m_ctx.fAbLong  = m_par.inputBitsPerFrame;
    m_ctx.fAbShort = m_par.inputBitsPerFrame;

    mfxI32 rawSize = GetRawFrameSize(m_par.width * m_par.height ,m_par.chromaFormat, m_par.bitDepthLuma);
    mfxI32 qp = GetNewQP(rawSize, m_par.inputBitsPerFrame, m_par.quantMinI, m_par.quantMaxI, 1 , m_par.quantOffset, 0.5, false, false);

    UpdateQPParams(qp,MFX_FRAMETYPE_IDR , m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, MFX_FRAMETYPE_REF);

    m_ctx.dQuantAb = qp > 0 ? 1./qp : 1.0; //kw

    if (m_par.WinBRCSize)
    {
        m_avg.reset(new AVGBitrate(m_par.WinBRCSize, (mfxU32)(m_par.WinBRCMaxAvgKbps*1000.0/m_par.frameRate), (mfxU32)m_par.inputBitsPerFrame) );
        MFX_CHECK_NULL_PTR1(m_avg.get());
    }

    m_bInit = true;
    return sts;
}

mfxU16 GetFrameType(mfxU16 m_frameType, mfxU16 level, mfxU16 gopRegDist)
{
    if (m_frameType & MFX_FRAMETYPE_IDR)
        return MFX_FRAMETYPE_IDR;
    else if (m_frameType & MFX_FRAMETYPE_I)
        return MFX_FRAMETYPE_I;
    else if (m_frameType & MFX_FRAMETYPE_P)
        return MFX_FRAMETYPE_P;
    else if ((m_frameType & MFX_FRAMETYPE_REF) && (level == 0 || gopRegDist == 1))
        return MFX_FRAMETYPE_P; //low delay B
    else
        return MFX_FRAMETYPE_B;
}


bool  isFrameBeforeIntra (mfxU32 order, mfxU32 intraOrder, mfxU32 gopPicSize, mfxU32 gopRefDist)
 {
     mfxI32 distance0 = gopPicSize*3/4;
     mfxI32 distance1 = gopPicSize - gopRefDist*3;
     return (order - intraOrder) > (mfxU32)(MFX_MAX(distance0, distance1));
 }
mfxStatus SetRecodeParams(mfxU16 brcStatus, mfxI32 qp, mfxI32 qp_new, mfxI32 minQP, mfxI32 maxQP, BRC_Ctx &ctx, mfxBRCFrameStatus* status)
{
    ctx.bToRecode = 1;

    if (brcStatus == MFX_BRC_BIG_FRAME || brcStatus == MFX_BRC_PANIC_BIG_FRAME )
    {
         MFX_CHECK(qp_new >= qp, MFX_ERR_UNDEFINED_BEHAVIOR);
         ctx.Quant = qp_new;
         ctx.QuantMax = maxQP;
         if (brcStatus == MFX_BRC_BIG_FRAME && qp_new > qp)
         {
            ctx.QuantMin = MFX_MAX(qp + 1, minQP); //limit QP range for recoding
            status->BRCStatus = MFX_BRC_BIG_FRAME;

         }
         else
         {
             ctx.QuantMin = minQP;
             ctx.bPanic = 1;
             status->BRCStatus = MFX_BRC_PANIC_BIG_FRAME;
         }

    }
    else if (brcStatus == MFX_BRC_SMALL_FRAME || brcStatus == MFX_BRC_PANIC_SMALL_FRAME)
    {
         MFX_CHECK(qp_new <= qp, MFX_ERR_UNDEFINED_BEHAVIOR);

         ctx.Quant = qp_new;
         ctx.QuantMin = minQP; //limit QP range for recoding

         if (brcStatus == MFX_BRC_SMALL_FRAME && qp_new < qp)
         {
            ctx.QuantMax = MFX_MIN (qp - 1, maxQP);
            status->BRCStatus = MFX_BRC_SMALL_FRAME;
         }
         else
         {
            ctx.QuantMax = maxQP;
            status->BRCStatus = MFX_BRC_PANIC_SMALL_FRAME;
            ctx.bPanic = 1;
         }
    }
    //printf("recode %d, qp %d new %d, status %d\n", ctx.encOrder, qp, qp_new, status->BRCStatus);
    return MFX_ERR_NONE;
}
mfxI32 GetNewQPTotal(mfxF64 bo, mfxF64 dQP, mfxI32 minQP , mfxI32 maxQP, mfxI32 qp, bool bPyr, bool bSC)
{
    mfxU8 mode = (!bPyr) ;

    bo  = mfx::clamp(bo, -1.0, 1.0);
    dQP = mfx::clamp(dQP, 1./maxQP, 1./minQP);

    mfxF64 ndQP = dQP + (1. / maxQP - dQP) * bo;
    ndQP = mfx::clamp(ndQP, 1. / maxQP, 1. / minQP);
    mfxI32 quant_new = (mfxI32) (1. / ndQP + 0.5);

    //printf("   GetNewQPTotal: bo %f, quant %d, quant_new %d, mode %d\n", bo, qp, quant_new, mode);
    if (!bSC)
    {
        if (mode == 0) // low: qp_diff [-2; 2]
        {
            if (quant_new >= qp + 5)
                quant_new = qp + 2;
            else if (quant_new > qp + 1)
                quant_new = qp + 1;
            else if (quant_new <= qp - 5)
                quant_new = qp - 2;
            else if (quant_new < qp - 1)
                quant_new = qp - 1;
        }
        else // (mode == 1) midle: qp_diff [-3; 3]
        {
            if (quant_new >= qp + 5)
                quant_new = qp + 3;
            else if (quant_new > qp + 2)
                quant_new = qp + 2;
            else if (quant_new <= qp - 5)
                quant_new = qp - 3;
            else if (quant_new < qp - 2)
                quant_new = qp - 2;
        }
    }
    else
    {
        quant_new = mfx::clamp(quant_new, qp - 5, qp + 5);
    }
    return mfx::clamp(quant_new, minQP, maxQP);
}

// Reduce AB period before intra and increase it after intra (to avoid intra frame affect on the bottom of hrd)
mfxF64 GetAbPeriodCoeff (mfxU32 numInGop, mfxU32 gopPicSize, mfxU32 SC)
{
    const mfxU32 maxForCorrection = 30;
    mfxF64 maxValue = (SC) ? 1.3 : 1.5;
    const mfxF64 minValue = 1.0;

    mfxU32 numForCorrection = MFX_MIN (gopPicSize /2, maxForCorrection);
    mfxF64 k[maxForCorrection] = {0};

    if (numInGop >= gopPicSize || gopPicSize < 2)
        return 1.0;

    for (mfxU32 i = 0; i < numForCorrection; i ++)
    {
        k[i] = maxValue - (maxValue - minValue)*i/numForCorrection;
    }
    if (numInGop < gopPicSize/2)
    {
        return k [numInGop < numForCorrection ? numInGop : numForCorrection - 1];
    }
    else
    {
        mfxU32 n = gopPicSize - 1 - numInGop;
        return 1.0/ k[n < numForCorrection ? n : numForCorrection - 1];
    }

}

void ResetMinQForMaxFrameSize(cBRCParams* par, mfxU32 type)
{
    if (type == MFX_FRAMETYPE_IDR || type == MFX_FRAMETYPE_I || type == MFX_FRAMETYPE_P) {
        par->mMinQstepCmplxKPUpdt = 0;
        par->mMinQstepCmplxKPUpdtErr = 0.16;
        par->mMinQstepCmplxKP = BRC_CONST_MUL_P1;
        par->mMinQstepRateEP = BRC_CONST_EXP_R_P1;
    }
}

mfxI32 GetMinQForMaxFrameSize(cBRCParams* par, mfxF64 targetBits, mfxU32 type)
{
    mfxI32 qp = 0;
    if (type == MFX_FRAMETYPE_P) {
        if (par->mMinQstepCmplxKPUpdt > 2 && par->mMinQstepCmplxKPUpdtErr < 0.69) {
            mfxI32 rawSize = par->mRawFrameSizeInPixs;
            mfxF64 BitsDesiredFrame = targetBits * (1.0 - 0.165 - MFX_MIN(0.115, par->mMinQstepCmplxKPUpdtErr/3.0));
            mfxF64 R = (mfxF64)rawSize / BitsDesiredFrame;
            mfxF64 QstepScale = pow(R, par->mMinQstepRateEP) * par->mMinQstepCmplxKP;
            QstepScale = MFX_MIN(128, QstepScale);
            mfxF64 minqp = 6.0*log(QstepScale) / log(2.0) + 12.0;
            minqp = MFX_MAX(0, minqp);
            qp = (mfxU32)(minqp + 0.5);
            qp = mfx::clamp(qp, 1, 51);
        }
    }
    return qp;
}

void UpdateMinQForMaxFrameSize(cBRCParams* par, mfxI32 bits, mfxI32 qp, BRC_Ctx *ctx, mfxU32 type, bool shstrt, mfxU16 brcSts)
{
    if (IS_IFRAME(type)) {
        mfxI32 rawSize = par->mRawFrameSizeInPixs;
        mfxF64 R = (mfxF64)rawSize / (mfxF64)bits;
        mfxF64 QstepScaleComputed = pow(R, par->mMinQstepRateEP) * par->mMinQstepCmplxKP;
        mfxF64 QstepScaleReal = pow(2.0, ((mfxF64)qp - 12.0) / 6.0);
        if (QstepScaleComputed > QstepScaleReal) {
            // Next P Frame atleast as complex as I Frame
            mfxF64 dS = log(QstepScaleReal) - log(QstepScaleComputed);
            par->mMinQstepCmplxKPUpdtErr = MFX_MAX((par->mMinQstepCmplxKPUpdtErr + abs(dS)) / 2, abs(dS));
            mfxF64 upDlt = 0.5;
            dS = mfx::clamp(dS, -0.5, 1.0);
            par->mMinQstepCmplxKP = par->mMinQstepCmplxKP*(1.0 + upDlt*dS);
            //par->mMinQstepCmplxKPUpdt++;
            par->mMinQstepRateEP = MFX_MIN(1.0, MFX_MAX(0.125, par->mMinQstepRateEP + MFX_MAX(-0.1, MFX_MIN(0.2, 0.01 * (log(QstepScaleReal) - log(QstepScaleComputed))*log(R)))));

            // Sanity Check / Force
            if (qp < 50) {
                mfxF64 rateQstepNew = pow(R, par->mMinQstepRateEP);
                mfxF64 QstepScaleUpdtComputed = rateQstepNew * par->mMinQstepCmplxKP;
                mfxI32 qp_now = (mfxI32)(6.0*log(QstepScaleUpdtComputed) / log(2.0) + 12.0);
                if (qp < qp_now -1) {
                    qp_now = qp + 2;
                    QstepScaleUpdtComputed = pow(2.0, ((mfxF64)qp_now - 12.0) / 6.0);
                    par->mMinQstepCmplxKP = QstepScaleUpdtComputed / rateQstepNew;
                    par->mMinQstepCmplxKPUpdtErr = 0.16;
                }
            }
        }
    } else if (type == MFX_FRAMETYPE_P) {
        if (ctx->LastIQpSetOrder < ctx->encOrder) {
            mfxI32 rawSize = par->mRawFrameSizeInPixs;
            mfxF64 R = (mfxF64)rawSize / (mfxF64)bits;
            mfxF64 QstepScaleComputed = pow(R, par->mMinQstepRateEP) * par->mMinQstepCmplxKP;
            mfxF64 QstepScaleReal = pow(2.0, ((mfxF64)qp - 12.0) / 6.0);
            mfxF64 dS = log(QstepScaleReal) - log(QstepScaleComputed);
            par->mMinQstepCmplxKPUpdtErr = MFX_MAX((par->mMinQstepCmplxKPUpdtErr + abs(dS)) / 2, abs(dS));
            mfxF64 upDlt = MFX_MIN(0.5, MFX_MAX(0.025, 1.3042 * pow(R, -0.922)));
            if (shstrt || par->mMinQstepCmplxKPUpdt <= 2 || par->mMinQstepCmplxKPUpdtErr > 0.69) upDlt = 0.5;
            else if (brcSts != MFX_BRC_OK || par->mMinQstepCmplxKPUpdtErr > 0.41) upDlt = MFX_MAX(0.125, upDlt);
            dS = mfx::clamp(dS, -0.5, 1.0);
            par->mMinQstepCmplxKP = par->mMinQstepCmplxKP*(1.0 + upDlt*dS);
            par->mMinQstepCmplxKPUpdt++;
            par->mMinQstepRateEP = MFX_MIN(1.0, MFX_MAX(0.125, par->mMinQstepRateEP + MFX_MAX(-0.1, MFX_MIN(0.2, 0.01 * (log(QstepScaleReal) - log(QstepScaleComputed))*log(R)))));
        }
    }
}

mfxI32 ExtBRC::GetCurQP (mfxU32 type, mfxI32 layer, mfxU16 isRef)
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
            qp =  m_ctx.QuantB + (layer > 0 ? layer - 1 : 0);
            if (layer && !isRef) qp += 1;
            qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }
    //printf("GetCurQP IDR %d I %d P %d B %d, min %d max %d type %d \n", m_ctx.QuantIDR, m_ctx.QuantI, m_ctx.QuantP, m_ctx.QuantB, m_par.quantMinI, m_par.quantMaxI, type);

    return qp;
}

mfxF64 ExtBRC::ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong)
{
    mfxI32 seqQP_new = GetSeqQP(qp, type, layer, isRef);
    mfxF64 dQuantAb_new = 1.0 / seqQP_new;
    mfxF64 bAbPreriod = m_par.bAbPeriod;

    mfxF64 totDiv = m_ctx.totalDiviation;
    const mfxF64 factor = m_par.bHRDConformance ? m_hrd.GetBufferDiviationFactor() : 0.0;
    mfxF64 lf = 1.0 / pow(m_par.inputBitsPerFrame / fAbLong, 1.0 + factor);

    if (m_par.bHRDConformance && totDiv > 0)
    {
        if (m_par.rateControlMethod == MFX_RATECONTROL_VBR)
        {
            totDiv = MFX_MAX(totDiv, m_hrd.GetBufferDiviation(m_par.targetbps));
        }
        bAbPreriod = (mfxF64)(m_par.bPyr ? 4 : 3)*(mfxF64)m_hrd.GetMaxFrameSize() / m_par.inputBitsPerFrame*GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIDREncOrder, m_par.gopPicSize, m_ctx.LastIDRSceneChange);
        bAbPreriod = mfx::clamp(bAbPreriod, m_par.bAbPeriod / 10, m_par.bAbPeriod);
    }

    mfxI32 quant_new = GetNewQPTotal(totDiv / bAbPreriod / (mfxF64)m_par.inputBitsPerFrame, dQuantAb_new, m_ctx.QuantMin, m_ctx.QuantMax, seqQP_new, m_par.bPyr && m_par.bRec, false);
    seqQP_new += (seqQP_new - quant_new);
    mfxF64 dQuantAb =  lf * (1.0 / seqQP_new);
    return dQuantAb;
}

mfxI32 ExtBRC::GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef)
{
    mfxI32 pqp = 0;
    if (type == MFX_FRAMETYPE_IDR) {
        pqp = qp + m_par.iDQp + 1;
    } else if (type == MFX_FRAMETYPE_I) {
        pqp = qp + 1;
    } else if (type == MFX_FRAMETYPE_P) {
        pqp = qp - layer;
    } else {
        pqp = qp -1 -(layer > 0 ? layer - 1 : 0);
        if (layer && !isRef) pqp -= 1;
    }

    return mfx::clamp(pqp, m_par.quantMinP, m_par.quantMaxP);
}

mfxI32 ExtBRC::GetPicQP(mfxI32 pqp, mfxU32 type, mfxI32 layer, mfxU16 isRef)
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
        qp = pqp + 1 + (layer > 0 ? layer - 1 : 0);
        if (layer && !isRef) qp += 1;
        qp = mfx::clamp(qp, m_par.quantMinB, m_par.quantMaxB);
    }

    return qp;
}

mfxStatus ExtBRC::Update(mfxBRCFrameParam* frame_par, mfxBRCFrameCtrl* frame_ctrl, mfxBRCFrameStatus* status)
{
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
    //printf("ExtBRC::Update:  m_ctx.encOrder %d , frame_par->EncodedOrder %d, frame_par->NumRecode %d, frame_par->CodedFrameSize %d, qp %d\n", m_ctx.encOrder , frame_par->EncodedOrder, frame_par->NumRecode, frame_par->CodedFrameSize, frame_ctrl->QpY);

    mfxI32 bitsEncoded  = frame_par->CodedFrameSize*8;
    mfxU32 picType      = GetFrameType(frame_par->FrameType, frame_par->PyramidLayer, m_par.gopRefDist);
    mfxI32 qpY          = frame_ctrl->QpY + m_par.quantOffset;
    mfxI32 layer        = frame_par->PyramidLayer;
    mfxF64 qstep        = QP2Qstep(qpY, m_par.quantOffset);

    mfxF64 fAbLong  = m_ctx.fAbLong   + (bitsEncoded - m_ctx.fAbLong)  / m_par.fAbPeriodLong;
    mfxF64 fAbShort = m_ctx.fAbShort  + (bitsEncoded - m_ctx.fAbShort) / m_par.fAbPeriodShort;
    mfxF64 eRate    = bitsEncoded * sqrt(qstep);

    mfxF64 e2pe     =  0;
    bool bMaxFrameSizeMode = m_par.maxFrameSizeInBits != 0 &&
        m_par.maxFrameSizeInBits < m_par.inputBitsPerFrame * 2 &&
        m_ctx.totalDiviation < (-1)*m_par.inputBitsPerFrame*m_par.frameRate;

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
            if (picType == MFX_FRAMETYPE_IDR)
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

        //printf("m_ctx.SceneChange %d, m_ctx.poc %d, m_ctx.SChPoc, m_ctx.poc %d \n", m_ctx.SceneChange, m_ctx.poc, m_ctx.SChPoc, m_ctx.poc);
    }
    if (m_par.bHRDConformance)
    {
        //check hrd
        brcSts = m_hrd.UpdateAndCheckHRD(bitsEncoded, frame_par->NumRecode, m_ctx.QuantMin, m_ctx.QuantMax);
        //printf("--UpdateAndCheckHRD (%d) brcSts %d, panic %d\n", frame_par->EncodedOrder,brcSts, m_ctx.bPanic);
        MFX_CHECK(brcSts == MFX_BRC_OK || (!m_ctx.bPanic), MFX_ERR_NOT_ENOUGH_BUFFER);
        if (brcSts == MFX_BRC_BIG_FRAME || brcSts == MFX_BRC_SMALL_FRAME)
            m_hrd.UpdateMinMaxQPForRec(brcSts, qpY);
        else if (!m_ctx.bPanic)
            bNeedUpdateQP = true;
        status->MinFrameSize = m_hrd.GetMinFrameSize() + 7;
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
            m_ctx.dQuantAb = ResetQuantAb(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong);
            m_ctx.SceneChange |= 16;
            m_ctx.eRateSH = eRate;
            m_ctx.SChPoc = frame_par->DisplayOrder;
            //printf("!!!!!!!!!!!!!!!!!!!!! %d m_ctx.SceneChange %d, order %d\n", frame_par->EncodedOrder, m_ctx.SceneChange, frame_par->DisplayOrder);
            if (picType == MFX_FRAMETYPE_P && bitsEncoded > 4 * m_par.inputBitsPerFrame) ResetMinQForMaxFrameSize(&m_par, picType);
        }
    }

    if (m_avg.get())
    {
       frameSizeLim = MFX_MIN (frameSizeLim, m_avg->GetMaxFrameSize(m_ctx.bPanic, bSHStart || IS_IFRAME(picType), frame_par->NumRecode));
    }
    if (m_par.maxFrameSizeInBits)
    {
        frameSizeLim = MFX_MIN (frameSizeLim, m_par.maxFrameSizeInBits);
    }
    //printf("frameSizeLim %d (%d)\n", frameSizeLim, bitsEncoded);
    if (frame_par->NumRecode < 100)
        UpdateMinQForMaxFrameSize(&m_par, bitsEncoded, qpY, &m_ctx, picType, bSHStart, brcSts);

    if (frame_par->NumRecode < 2)
    // Check other condions for recoding (update qp is it is needed)
    {
        mfxF64 targetFrameSize = MFX_MAX((mfxF64)m_par.inputBitsPerFrame, fAbLong);
        mfxF64 dqf = (m_par.bFieldMode) ? 1.0 : DQF(picType, m_par.iDQp, ((picType == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), (ParSceneChange || m_ctx.encOrder == 0));
        mfxF64 maxFrameSizeByRatio = dqf * FRM_RATIO(picType, m_ctx.encOrder, bSHStart, m_par.bPyr) * targetFrameSize;
        if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.bHRDConformance) {
            mfxF64 dev = -1.0*maxFrameSizeByRatio - m_hrd.GetBufferDiviation();
            if (dev > 0) maxFrameSizeByRatio += MFX_MIN(maxFrameSizeByRatio, (dev / (IS_IFRAME(picType) ? 2.0 : 4.0)));
        }

        mfxI32 quantMax = m_ctx.QuantMax;
        mfxI32 quantMin = m_ctx.QuantMin;
        mfxI32 quant = qpY;

        mfxF64 maxFrameSize = MFX_MIN(maxFrameSizeByRatio,frameSizeLim);

        if (m_par.bHRDConformance)
        {
            mfxF64 bufOccupy = LTR_BUF(picType, m_par.iDQp, ((picType == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), ParSceneChange, bSHStart);
            mfxF64 maxFrameSizeHRD = bufOccupy / 9.* m_hrd.GetMaxFrameSize() + (9.0 - bufOccupy) / 9.*targetFrameSize;
            if (m_ctx.encOrder == 0)
            {
                // modify buf limits for VCM like encode for init only
                mfxF64 maxFrameSizeGood = 6.5 * m_par.inputBitsPerFrame;
                mfxF64 maxFrameSizeHighMark = 8.0 / 9.* m_hrd.GetMaxFrameSize() + 1.0 / 9.*m_par.inputBitsPerFrame;
                mfxF64 maxFrameSizeInit = MFX_MIN(maxFrameSizeHighMark, MFX_MAX(maxFrameSizeHRD, maxFrameSizeGood));
                maxFrameSize = MFX_MIN(maxFrameSize, maxFrameSizeInit);
            }
            else
                maxFrameSize = MFX_MIN(maxFrameSize, maxFrameSizeHRD);

            quantMax     = MFX_MIN(m_hrd.GetMaxQuant(), quantMax);
            quantMin     = MFX_MAX(m_hrd.GetMinQuant(), quantMin);
        }
        maxFrameSize = MFX_MAX(maxFrameSize, targetFrameSize);

        if (bitsEncoded >  maxFrameSize && quant < quantMax)
        {
            mfxI32 quant_new = GetNewQP(bitsEncoded, (mfxU32)maxFrameSize, quantMin , quantMax, quant ,m_par.quantOffset, 1);
            if (quant_new > quant)
            {
                bNeedUpdateQP = false;
                //printf("    recode 1-0: %d:  k %5f bitsEncoded %d maxFrameSize %d, targetSize %d, fAbLong %f, inputBitsPerFrame %f, qp %d new %d, layer %d\n",frame_par->EncodedOrder, bitsEncoded/maxFrameSize, (int)bitsEncoded, (int)maxFrameSize,(int)targetFrameSize, fAbLong, m_par.inputBitsPerFrame, quant, quant_new, layer);
                if (quant_new > GetCurQP(picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong);
                }

                if (m_par.bRec)
                {
                    SetRecodeParams(MFX_BRC_BIG_FRAME, quant, quant_new, quantMin, quantMax, m_ctx, status);
                    return sts;
                }
            } //(quant_new > quant)
        } //bitsEncoded >  maxFrameSize

        mfxF64 lFR = MFX_MIN(m_par.gopPicSize - 1, 4);
        mfxF64 lowFrameSizeI = MFX_MIN(maxFrameSize, lFR *(mfxF64)m_par.inputBitsPerFrame);
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
                if (quant_new < GetCurQP(picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF))
                {
                    UpdateQPParams(bMaxFrameSizeMode ? quant_new - 1 : quant_new, picType, m_ctx, 0, quantMin, quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF);
                    fAbLong = m_ctx.fAbLong = m_par.inputBitsPerFrame;
                    fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                    m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong);
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
        if (m_par.bHRDConformance && frame_par->NumRecode == 0 && (quant < quantMax))
        {
            mfxF64 FAMax = 1./9. * m_hrd.GetMaxFrameSize() + 8./9. * fAbLong;

            if (fAbShort > FAMax)
            {
                mfxI32 quant_new = GetNewQP(fAbShort, FAMax, quantMin , quantMax, quant ,m_par.quantOffset, 0.5);
                //printf("============== recode 2-0: %d:  FAMax %f, fAbShort %f, quant_new %d\n",frame_par->EncodedOrder, FAMax, fAbShort, quant_new);

                if (quant_new > quant)
                {
                   bNeedUpdateQP = false;
                   if (quant_new > GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF))
                   {
                        UpdateQPParams(quant_new ,picType, m_ctx, 0, quantMin , quantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF);
                        fAbLong  = m_ctx.fAbLong  = m_par.inputBitsPerFrame;
                        fAbShort = m_ctx.fAbShort = m_par.inputBitsPerFrame;
                        m_ctx.dQuantAb = ResetQuantAb(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF, fAbLong);
                    }
                    if (m_par.bRec)
                    {
                        SetRecodeParams(MFX_BRC_BIG_FRAME,quant,quant_new, quantMin, quantMax, m_ctx, status);
                        return sts;
                    }
                } //quant_new > quant
            }
        }//m_par.bHRDConformance
    }
    if (((m_par.bHRDConformance && brcSts != MFX_BRC_OK) || (bitsEncoded > (mfxI32)frameSizeLim)) && m_par.bRec)
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
            quant_new = GetNewQP(bitsEncoded, m_hrd.GetTargetSize(brcSts), m_ctx.QuantMin , m_ctx.QuantMax,quant,m_par.quantOffset, 1, true);
        }
        if (quant_new != quant)
        {
            if (brcSts == MFX_BRC_SMALL_FRAME)
            {
               quant_new = MFX_MAX(quant_new, quant-2);
               brcSts = MFX_BRC_PANIC_SMALL_FRAME;
            }
            // Idea is to check a sign mismatch, 'true' if both are negative or positive
            if ((quant_new - qpY) * (quant_new - GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF)) > 0)
            {
                UpdateQPParams(quant_new ,picType, m_ctx, 0, m_ctx.QuantMin , m_ctx.QuantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF);
            }
            bNeedUpdateQP = false;
        }
        SetRecodeParams(brcSts,quant,quant_new, m_ctx.QuantMin , m_ctx.QuantMax, m_ctx, status);
        //printf("===================== recode 1-0: HRD recode: quant_new %d\n", quant_new);
    }
    else
    {
        // no recoding are needed. Save context params

        mfxF64 k = 1./ GetSeqQP(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF);
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
                        m_ctx.eRate = m_par.inputBitsPerFrame * sqrt(QP2Qstep(GetCurQP(MFX_FRAMETYPE_P, 0, MFX_FRAMETYPE_REF), m_par.quantOffset));
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

        m_ctx.totalDiviation += ((mfxF64)bitsEncoded -m_par.inputBitsPerFrame);

        //printf("------------------ %d (%d)) Total diviation %d, old scene %d, bNeedUpdateQP %d, m_ctx.Quant %d, type %d, m_ctx.fAbLong %f m_par.inputBitsPerFrame %f\n", frame_par->EncodedOrder, frame_par->DisplayOrder,m_ctx.totalDiviation, oldScene , bNeedUpdateQP, m_ctx.Quant,picType, m_ctx.fAbLong, m_par.inputBitsPerFrame);

        if (!m_ctx.bPanic&& (!oldScene) && bNeedUpdateQP)
        {
            mfxI32 quant_new = qpY;

            //Update QP

            mfxF64 totDiv = m_ctx.totalDiviation;

            const mfxF64 factor = m_par.bHRDConformance ? m_hrd.GetBufferDiviationFactor() : 0.0;
            mfxF64 dequant_new = m_ctx.dQuantAb*pow(m_par.inputBitsPerFrame / m_ctx.fAbLong, 1.0 + factor);

            mfxF64 bAbPreriod = m_par.bAbPeriod;

            if (m_par.bHRDConformance && totDiv > 0)
            {
                if (m_par.rateControlMethod == MFX_RATECONTROL_VBR)
                {
                    totDiv = MFX_MAX(totDiv, m_hrd.GetBufferDiviation(m_par.targetbps));
                }

                bAbPreriod = (mfxF64)(m_par.bPyr ? 4 : 3)*(mfxF64)m_hrd.GetMaxFrameSize() / fAbShort*GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIDREncOrder, m_par.gopPicSize, m_ctx.LastIDRSceneChange);
                bAbPreriod = mfx::clamp(bAbPreriod, m_par.bAbPeriod / 10, m_par.bAbPeriod);
            }
            quant_new = GetNewQPTotal(totDiv / bAbPreriod / (mfxF64)m_par.inputBitsPerFrame, dequant_new, m_ctx.QuantMin, m_ctx.QuantMax, GetSeqQP(qpY, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF), m_par.bPyr && m_par.bRec, bSHStart && m_ctx.bToRecode == 0);
            quant_new = GetPicQP(quant_new, picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF);
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

            if ((quant_new - qpY)* (quant_new - GetCurQP (picType, layer, frame_par->FrameType & MFX_FRAMETYPE_REF)) > 0) // this check is actual for async scheme
            {
                //printf("   +++ Update QP %d: totalDiviation %f, bAbPreriod %f (%f), QP %d (%d %d), qp_new %d (qpY %d), type %d, dequant_new %f (%f) , m_ctx.fAbLong %f, m_par.inputBitsPerFrame %f (%f)\n",frame_par->EncodedOrder,totDiv , bAbPreriod, GetAbPeriodCoeff(m_ctx.encOrder - m_ctx.LastIEncOrder, m_par.gopPicSize), m_ctx.Quant, m_ctx.QuantMin, m_ctx.QuantMax,quant_new, qpY, picType, 1.0/dequant_new, 1.0/m_ctx.dQuantAb, m_ctx.fAbLong, m_par.inputBitsPerFrame);
                UpdateQPParams(quant_new ,picType, m_ctx, 0, m_ctx.QuantMin , m_ctx.QuantMax, layer, m_par.iDQp, frame_par->FrameType & MFX_FRAMETYPE_REF);
            }
        }
        m_ctx.bToRecode = 0;
    }
    return sts;

}

const mfxF64 COEFF_INTRA[2] = { -0.107510, 0.694515 };

void get_coeff_intra(mfxF64 /*rscs*/, mfxF64 *pCoeff)
{
    pCoeff[0] = COEFF_INTRA[0];
    pCoeff[1] = COEFF_INTRA[1];
}

#define PWR_RSCS 0.751

mfxF64 getScaledIntraBits(mfxF64 targetBits, mfxF64 rawSize, mfxF64 rscs)
{
    if (rscs < MIN_RACA)  rscs = MIN_RACA;
    mfxF64 SC = pow(rscs, PWR_RSCS);
    mfxF64 dBits = log((targetBits / rawSize) / SC);

    return dBits;
}

mfxI32 compute_first_qp_intra(mfxI32 targetBits, mfxI32 rawSize, mfxF64 rscs)
{
    mfxF64 dBits = getScaledIntraBits(targetBits, rawSize, rscs);
    mfxF64 coeffIntra[2];
    get_coeff_intra(rscs, coeffIntra);

    mfxF64 qpNew = (dBits - coeffIntra[1]) / coeffIntra[0];
    mfxI32 qp = (mfxI32)(qpNew + 0.5);
    if (qp < 1) qp = 1;
    return qp;
}

mfxI32 compute_new_qp_intra(mfxI32 targetBits, mfxI32 rawSize, mfxF64 raca, mfxI32 iBits, mfxF64 icmplx, mfxI32 iqp)
{
    mfxF64 coeffIntra1[2], coeffIntra2[2];

    mfxF64 qp_hat = getScaledIntraBits(iBits, rawSize, icmplx);
    get_coeff_intra(icmplx, coeffIntra1);
    qp_hat = (qp_hat - coeffIntra1[1]) / coeffIntra1[0];

    mfxF64 dQp = iqp - qp_hat;
    dQp = mfx::clamp(dQp, (-1.0 * MAX_MODEL_ERR), (1.0 * MAX_MODEL_ERR));

    mfxF64 qp_pred = getScaledIntraBits(targetBits, rawSize, raca);
    get_coeff_intra(raca, coeffIntra2);

    qp_pred = (qp_pred - coeffIntra2[1]) / coeffIntra2[0];

    mfxF64 qpNew = qp_pred + dQp;

    mfxI32 qp = (mfxI32)(qpNew + 0.5);
    if (qp < 1) qp = 1;
    return qp;
}


mfxStatus ExtBRC::GetFrameCtrl (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
{
    MFX_CHECK_NULL_PTR2(par, ctrl);
    MFX_CHECK(m_bInit, MFX_ERR_NOT_INITIALIZED);
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

    if (!m_bDynamicInit) {
        if (IS_IFRAME(type)) {
            // Init DQP
            if (ParLongTerm) {
                m_par.iDQp = m_par.iDQp0;
                ltrprintf("DQp0 %d\n", m_par.iDQp);
            }
            // Init Qp
            if (ParFrameCmplx > 0) {
                mfxF64 raca = (mfxF64)ParFrameCmplx / RACA_SCALE;
                // MaxFrameSize
                mfxF64 maxFrameSize = m_par.mRawFrameSizeInBits;
                if (m_par.maxFrameSizeInBits) {
                    maxFrameSize = MFX_MIN(maxFrameSize, m_par.maxFrameSizeInBits);
                }
                if (m_par.bHRDConformance) {
                    mfxF64 bufOccupy = LTR_BUF(type, m_par.iDQp, m_par.mIntraBoost, 1, 0);
                    maxFrameSize = MFX_MIN(maxFrameSize, (bufOccupy / 9.* (m_par.initialDelayInBytes * 8.0) + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
                }
                // Set Intra QP
                mfxF64 dqf = DQF(type, m_par.iDQp, m_par.mIntraBoost, 1);
                mfxF64 targetFrameSize = dqf * FRM_RATIO(type, 0, 0, m_par.bPyr) * (mfxF64)m_par.inputBitsPerFrame;
                targetFrameSize = MFX_MIN(maxFrameSize, targetFrameSize);
                mfxI32 qp0 = compute_first_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca);
                if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qp0>3) qp0 -= 3; // use re-encoding for best results (maxFrameSizeGood)
                else if (raca == MIN_RACA && qp0>3)                           qp0 -= 3; // uncertainty; use re-encoding for best results
                ltrprintf("Qp0 %d\n", qp0);
                UpdateQPParams(qp0, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, par->FrameType & MFX_FRAMETYPE_REF);
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
                maxFrameSize = MFX_MIN(maxFrameSize, m_par.maxFrameSizeInBits);
            }
            if (m_par.bHRDConformance) {
                mfxF64 hrdMaxFrameSize = m_par.initialDelayInBytes * 8;
                if (m_hrd.GetMaxFrameSize() > 0) hrdMaxFrameSize = MFX_MIN(hrdMaxFrameSize, (mfxF64) m_hrd.GetMaxFrameSize());
                mfxF64 bufOccupy = LTR_BUF(type, m_par.iDQp, ((type == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), (ParSceneChange || (m_ctx.LastIQpSet && m_ctx.QuantP > ((mfxI32)m_ctx.LastIQpSet + (mfxI32)m_par.iDQp + 1))), 0);
                maxFrameSize = MFX_MIN(maxFrameSize, (bufOccupy / 9.* hrdMaxFrameSize + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
            }

            if (type == MFX_FRAMETYPE_IDR) {
                // Re-Determine LTR  iDQP
                if (!ParLongTerm) {
                    m_par.iDQp = 0;
                } else {
                    mfxF64 maxFrameRatio = 2 * FRM_RATIO(type, par->EncodedOrder, 0, m_par.bPyr);
                    mfxF64 minFrameRatio = FRM_RATIO(type, 0, 0, m_par.bPyr);
                    maxFrameRatio = MFX_MIN(maxFrameRatio, (maxFrameSize / m_par.inputBitsPerFrame));
                    mfxU32 mNumRefsInGop = m_par.mNumRefsInGop;
                    if (m_ctx.LastIQpSetOrder) {
                        mfxU32 pastRefsInGop = (mfxU32)(MFX_MAX(1.0, (!m_par.bPyr ? (mfxF64)(par->EncodedOrder - m_ctx.LastIQpSetOrder) / (mfxF64)m_par.gopRefDist : (mfxF64)(par->EncodedOrder - m_ctx.LastIQpSetOrder) / 2.0)));
                        mNumRefsInGop = MFX_MIN(mNumRefsInGop, pastRefsInGop);
                    }
                    maxFrameRatio = MFX_MIN(maxFrameRatio, mNumRefsInGop);
                    mfxF64 dqpmax = MFX_MAX(0.0, 6.0 * (log(maxFrameRatio / minFrameRatio) / log(2.0)));
                    mfxU32 iDQpMax = (mfxU32)(dqpmax + 0.5);
                    if (ParSceneChange) {
                        iDQpMax = mfx::clamp<mfxU32>(iDQpMax, 1, m_par.iDQp0);
                    }
                    else {
                        iDQpMax = mfx::clamp<mfxU32>(iDQpMax, 1, MAX_DQP_LTR);
                    }
                    m_par.iDQp = iDQpMax;
                    ltrprintf("FR %lf DQp %d\n", maxFrameRatio, m_par.iDQp);
                }
            }

            // Determine Min Qp
            if (ParFrameCmplx > 0) {
                mfxF64 raca = (mfxF64)ParFrameCmplx / RACA_SCALE;
                mfxF64 dqf = DQF(type, m_par.iDQp, ((type == MFX_FRAMETYPE_IDR) ? m_par.mIntraBoost : false), ParSceneChange);
                mfxF64 targetFrameSize = dqf * FRM_RATIO(type, par->EncodedOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
                if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.bHRDConformance) {
                    // CBR HRD Buffer over flow has priority
                    mfxF64 dev = -1.0*targetFrameSize - m_hrd.GetBufferDiviation();
                    if (dev > 0) targetFrameSize += MFX_MIN(targetFrameSize, (dev/2.0));
                }

                targetFrameSize = MFX_MIN(maxFrameSize, targetFrameSize);
                mfxF64 CmplxRatio = 1.0;
                if (m_ctx.LastICmplx) CmplxRatio = ParFrameCmplx / m_ctx.LastICmplx;
                if (!ParSceneChange && m_ctx.LastICmplx && m_ctx.LastIQpAct && m_ctx.LastIFrameSize && CmplxRatio > 0.5 && CmplxRatio < 2.0)
                {
                    qpMin = compute_new_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca, m_ctx.LastIFrameSize, (mfxF64) m_ctx.LastICmplx / RACA_SCALE, m_ctx.LastIQpAct);
                    if (raca == MIN_RACA && qpMin>3)                                qpMin -= 3; // uncertainty; use re-encoding for best results
                }
                else
                {
                    qpMin = compute_first_qp_intra((mfxI32)targetFrameSize, m_par.mRawFrameSizeInPixs, raca);
                    if (targetFrameSize < 6.5 * m_par.inputBitsPerFrame && qpMin>3) qpMin -= 3; // uncertainty; use re-encoding for best results
                    else if (raca == MIN_RACA && qpMin>3)                           qpMin -= 3; // uncertainty; use re-encoding for best results
                }

                ltrprintf("Min QpI %d\n", qpMin);
            }
        }
        else //if (type == MFX_FRAMETYPE_P)
        {
            mfxU16 ltype = MFX_FRAMETYPE_P;
            mfxF64 maxFrameSize = m_par.mRawFrameSizeInBits;
            if (m_par.maxFrameSizeInBits) {
                maxFrameSize = MFX_MIN(maxFrameSize, m_par.maxFrameSizeInBits);
            }
            if (m_par.bHRDConformance) {
                mfxF64 hrdMaxFrameSize = m_par.initialDelayInBytes * 8;
                if (m_hrd.GetMaxFrameSize() > 0) hrdMaxFrameSize = MFX_MIN(hrdMaxFrameSize, (mfxF64)m_hrd.GetMaxFrameSize());

                mfxF64 bufOccupy = LTR_BUF(ltype, m_par.iDQp, false, ParSceneChange, ParSceneChange);
                maxFrameSize = MFX_MIN(maxFrameSize, (bufOccupy / 9.* hrdMaxFrameSize + (9.0 - bufOccupy) / 9.*m_par.inputBitsPerFrame));
            }

            mfxF64 targetFrameSize = FRM_RATIO(ltype, par->EncodedOrder, 0, m_par.bPyr) * m_par.inputBitsPerFrame;
            if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && m_par.bHRDConformance) {
                mfxF64 dev = -1.0*targetFrameSize - m_hrd.GetBufferDiviation();
                if (dev > 0) targetFrameSize += MFX_MIN(targetFrameSize, (dev/4.0));
            }
            targetFrameSize = MFX_MIN(maxFrameSize, targetFrameSize);
            qpMin = GetMinQForMaxFrameSize(&m_par, targetFrameSize, ltype);
        }

        qp = GetCurQP(type, par->PyramidLayer, par->FrameType & MFX_FRAMETYPE_REF);

        // Max Frame Size recode prevention
        if (qp < qpMin)
        {
            if (type != MFX_FRAMETYPE_B)
            {
                SetQPParams(qpMin, type, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, par->FrameType & MFX_FRAMETYPE_REF);
                qp = GetCurQP(type, par->PyramidLayer, par->FrameType & MFX_FRAMETYPE_REF);
            }
            else
            {
                qp = qpMin;
            }
        }
        else
            qpMin = MFX_MIN(qp - 1, qpMin);
    }
    ctrl->QpY = qp - m_par.quantOffset;

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

            UpdateQPParams(m_ctx.Quant, MFX_FRAMETYPE_IDR, m_ctx, 0, m_par.quantMinI, m_par.quantMaxI, 0, m_par.iDQp, MFX_FRAMETYPE_REF);

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
