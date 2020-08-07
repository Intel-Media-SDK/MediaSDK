// Copyright (c) 2020 Intel Corporation
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

#ifndef __MFX_ENCTOOLS_BRC_DEFS_H__
#define __MFX_ENCTOOLS_BRC_DEFS_H__

#include "mfxdefs.h"
#include "mfxenctools-int.h"
#include <math.h>
#include <vector>
#include <memory>
#include <algorithm>
#include "mfx_enctools_utils.h"
#include <climits>

constexpr auto BRC_MAX_DQP_LTR = 4;

constexpr auto BRC_BUFK = 3.5;
constexpr auto LTR_BUFK = 4.5;
#define BRC_LTR_BUF(type, dqp, boost, schg, shstrt) \
((type == MFX_FRAMETYPE_IDR) ? (((schg && !boost) || !dqp) ? BRC_BUFK : LTR_BUFK) : (shstrt ? BRC_BUFK : 2.5))

constexpr auto  BRC_DQFF0 = 1.0;
constexpr auto  BRC_DQFF1 = 1.66;
#define BRC_DQF(type, dqp, boost, schg) \
((type == MFX_FRAMETYPE_IDR) ? ((dqp?pow(2, ((mfxF64)dqp / 6.0)) : 1.0) * ((schg && !boost) ? BRC_DQFF0 : BRC_DQFF1)) : 1.0)

#define BRC_FRM_RATIO(type, encorder, shstrt, pyr) \
((((encorder == 0 && !pyr) || type == MFX_FRAMETYPE_I) ? 6.0 : (shstrt || type == MFX_FRAMETYPE_IDR) ? 8.0 : 4.0) * ((pyr) ? 1.5 : 1.0))

constexpr auto BRC_CONST_MUL_P1   = 2.253264596;
constexpr auto BRC_CONST_EXP_R_P1 = 0.42406423;

// RACA = Spatial Complexity measure
// RACA = Row diff Abs + Column diff Abs

constexpr auto BRC_MIN_RACA = 0.25;
constexpr auto BRC_MAX_RACA = 361.0;
constexpr auto BRC_RACA_SCALE = 128.0;
constexpr auto BRC_PWR_RACA = 0.751;

constexpr auto  BRC_SCENE_CHANGE_RATIO2 = 5.0;

#define ltrprintf(...)
//#define ltrprintf printf

const mfxF64 BRC_COEFF_INTRA[2] = { -0.107510, 0.694515 };

inline void get_coeff_intra(mfxF64 *pCoeff)
{
    pCoeff[0] = BRC_COEFF_INTRA[0];
    pCoeff[1] = BRC_COEFF_INTRA[1];
}

const mfxU32 h264_bit_rate_scale = 4;
const mfxU32 h264_cpb_size_scale = 2;

struct BRC_Ctx
{
    mfxI32 QuantIDR;  //currect qp for intra frames
    mfxI32 QuantI;  //currect qp for intra frames
    mfxI32 QuantP;  //currect qp for P frames
    mfxI32 QuantB;  //currect qp for B frames

    mfxI32 Quant;           // qp for last encoded frame
    mfxI32 QuantMin;        // qp Min for last encoded frame (is used for recoding)
    mfxI32 QuantMax;        // qp Max for last encoded frame (is used for recoding)

    bool   bToRecode;       // last frame is needed in recoding
    bool   bPanic;          // last frame is needed in panic
    mfxU32 encOrder;        // encoding order of last encoded frame
    mfxU32 poc;             // poc of last encoded frame
    mfxI32 SceneChange;     // scene change parameter of last encoded frame
    mfxU32 SChPoc;          // poc of frame with scene change
    mfxU32 LastIEncOrder;   // encoded order of last intra frame
    mfxU32 LastIDREncOrder;   // encoded order of last idr frame
    mfxU32 LastIDRSceneChange; // last idr was scene change
    mfxU32 LastIQpAct;      // Qp of last intra frame
    mfxU32 LastIFrameSize; // encoded frame size of last non B frame (is used for sceneChange)
    mfxF64 LastICmplx;      // Qp of last intra frame
    mfxU32 LastIQpSetOrder; // Qp of last intra frame
    mfxU32 LastIQpMin; // Qp of last intra frame
    mfxU32 LastIQpSet;      // Qp of last intra frame

    mfxU32 LastNonBFrameSize; // encoded frame size of last non B frame (is used for sceneChange)

    mfxF64 fAbLong;         // frame abberation (long period)
    mfxF64 fAbShort;        // frame abberation (short period)
    mfxF64 dQuantAb;        // dequant abberation
    mfxF64 totalDeviation;   // deviation from  target bitrate (total)

    mfxF64 eRate;               // eRate of last encoded frame, this parameter is used for scene change calculation
    mfxF64 eRateSH;             // eRate of last encoded scene change frame, this parameter is used for scene change calculation
};

class AVGBitrate
{
public:
    AVGBitrate(mfxU32 windowSize, mfxU32 maxBitPerFrame, mfxU32 avgBitPerFrame, bool bLA = false):
        m_maxWinBits(maxBitPerFrame*windowSize),
        m_maxWinBitsLim(0),
        m_avgBitPerFrame(std::min(avgBitPerFrame, maxBitPerFrame)),
        m_currPosInWindow(windowSize - 1),
        m_lastFrameOrder(mfxU32(-1)),
        m_bLA(bLA)

    {
        windowSize = windowSize > 0 ? windowSize : 1; // kw
        m_slidingWindow.resize(windowSize);
        for (mfxU32 i = 0; i < windowSize; i++)
        {
            m_slidingWindow[i] = maxBitPerFrame / 3; //initial value to prevent big first frames
        }
        m_maxWinBitsLim = GetMaxWinBitsLim();
    }
    virtual ~AVGBitrate()
    {
        //printf("------------ AVG Bitrate: %d ( %d), NumberOfErrors %d\n", m_MaxBitReal, m_MaxBitReal_temp, m_NumberOfErrors);
    }
    void UpdateSlidingWindow(mfxU32  sizeInBits, mfxU32  FrameOrder, bool bPanic, bool bSH, mfxU32 recode, mfxU32 /* qp */)
    {
        mfxU32 windowSize = (mfxU32)m_slidingWindow.size();
        bool   bNextFrame = FrameOrder != m_lastFrameOrder;

        if (bNextFrame)
        {
            m_lastFrameOrder = FrameOrder;
            m_currPosInWindow = (m_currPosInWindow + 1) % windowSize;
        }
        m_slidingWindow[m_currPosInWindow] = sizeInBits;

        if (bNextFrame)
        {
            if (bPanic || bSH)
            {
                m_maxWinBitsLim = mfx::clamp((GetLastFrameBits(windowSize,false) + m_maxWinBits) / 2, GetMaxWinBitsLim(), m_maxWinBits);
            }
            else
            {
                if (recode)
                    m_maxWinBitsLim = mfx::clamp(GetLastFrameBits(windowSize,false) + GetStep() / 2, m_maxWinBitsLim, m_maxWinBits);
                else if ((m_maxWinBitsLim > GetMaxWinBitsLim() + GetStep()) &&
                    (m_maxWinBitsLim - GetStep() > (GetLastFrameBits(windowSize - 1, false) + sizeInBits)))
                    m_maxWinBitsLim -= GetStep();
            }

        }
    }

    mfxU32 GetMaxFrameSize(bool bPanic, bool bSH, mfxU32 recode) const
    {
        mfxU32 winBits = GetLastFrameBits(GetWindowSize() - 1, !bPanic);

        mfxU32 maxWinBitsLim = m_maxWinBitsLim;
        if (bSH)
            maxWinBitsLim = (m_maxWinBits + m_maxWinBitsLim) / 2;
        if (bPanic)
            maxWinBitsLim = m_maxWinBits;
        maxWinBitsLim = std::min(maxWinBitsLim + recode * GetStep() / 2, m_maxWinBits);

        mfxU32 maxFrameSize = winBits >= m_maxWinBitsLim ?
            mfxU32(std::max((mfxI32)m_maxWinBits - (mfxI32)winBits, 1)) :
            maxWinBitsLim - winBits;

        return maxFrameSize;
    }

    mfxU32 GetWindowSize() const
    {
        return (mfxU32)m_slidingWindow.size();
    }

    mfxI32 GetBudget(mfxU32 numFrames) const
    {
        numFrames = std::min(mfxU32(m_slidingWindow.size()), numFrames);
        return ((mfxI32)m_maxWinBitsLim - (mfxI32)GetLastFrameBits((mfxU32)m_slidingWindow.size() - numFrames, true));
    }

protected:

    mfxU32                      m_maxWinBits;
    mfxU32                      m_maxWinBitsLim;
    mfxU32                      m_avgBitPerFrame;

    mfxU32                      m_currPosInWindow;
    mfxU32                      m_lastFrameOrder;
    bool                        m_bLA;
    std::vector<mfxU32>         m_slidingWindow;


    mfxU32 GetLastFrameBits(mfxU32 numFrames, bool bCheckSkip) const
    {
        mfxU32 size = 0;
        numFrames = numFrames < m_slidingWindow.size() ? numFrames : (mfxU32)m_slidingWindow.size();
        for (mfxU32 i = 0; i < numFrames; i++)
        {
            mfxU32 frame_size = m_slidingWindow[(m_currPosInWindow + m_slidingWindow.size() - i) % m_slidingWindow.size()];
            if (bCheckSkip && (frame_size < m_avgBitPerFrame / 3))
                frame_size = m_avgBitPerFrame / 3;
            size += frame_size;
            //printf("GetLastFrames: %d) %d sum %d\n",i,m_slidingWindow[(m_currPosInWindow + m_slidingWindow.size() - i) % m_slidingWindow.size() ], size);
        }
        return size;
    }
    mfxU32 GetStep() const
    {
        return  (m_maxWinBits / GetWindowSize() - m_avgBitPerFrame) / (m_bLA ? 4 : 2);
    }

    mfxU32 GetMaxWinBitsLim() const
    {
        return m_maxWinBits - GetStep() * GetWindowSize();
    }
};

class cBRCParams
{
public:
    mfxU16 rateControlMethod; // CBR or VBR

    mfxU16 HRDConformance;   // is HRD compliance  needed
    mfxU16 bRec;             // is Recoding possible
    mfxU16 bPanic;           // is Panic mode possible

    // HRD params
    mfxU32 bufferSizeInBytes;
    mfxU32 initialDelayInBytes;

    // Sliding window parameters
    mfxU32  WinBRCMaxAvgKbps;
    mfxU16  WinBRCSize;

    // RC params
    mfxU32 targetbps;
    mfxU32 maxbps;
    mfxF64 frameRate;
    mfxF64 inputBitsPerFrame;
    mfxF64 maxInputBitsPerFrame;
    mfxU32 maxFrameSizeInBits;

    // Frame size params
    mfxU16 width;
    mfxU16 height;
    mfxU16 chromaFormat;
    mfxU16 bitDepthLuma;
    mfxU32 mRawFrameSizeInBits;
    mfxU32 mRawFrameSizeInPixs;

    // GOP params
    mfxU16 gopPicSize;
    mfxU16 gopRefDist;
    bool   bPyr;
    bool   bFieldMode;

    //BRC accurancy params
    mfxF64 fAbPeriodLong;   // number on frames to calculate abberation from target frame
    mfxF64 fAbPeriodShort;  // number on frames to calculate abberation from target frame
    mfxF64 dqAbPeriod;      // number on frames to calculate abberation from dequant
    mfxF64 bAbPeriod;       // number of frames to calculate abberation from target bitrate

    //QP parameters
    mfxI32   quantOffset;
    mfxI32   quantMaxI;
    mfxI32   quantMinI;
    mfxI32   quantMaxP;
    mfxI32   quantMinP;
    mfxI32   quantMaxB;
    mfxI32   quantMinB;
    mfxU32   iDQp0;
    mfxU32   iDQp;
    mfxU32   mNumRefsInGop;
    bool     mIntraBoost;
    mfxF64   mMinQstepCmplxKP;
    mfxF64   mMinQstepRateEP;
    mfxI32   mMinQstepCmplxKPUpdt;
    mfxF64   mMinQstepCmplxKPUpdtErr;

    mfxU32  codecId;

public:
    cBRCParams() :
        rateControlMethod(0),
        HRDConformance(MFX_BRC_NO_HRD),
        bRec(0),
        bPanic(0),
        bufferSizeInBytes(0),
        initialDelayInBytes(0),
        WinBRCMaxAvgKbps(0),
        WinBRCSize(0),
        targetbps(0),
        maxbps(0),
        frameRate(0),
        inputBitsPerFrame(0),
        maxInputBitsPerFrame(0),
        maxFrameSizeInBits(0),
        width(0),
        height(0),
        chromaFormat(0),
        bitDepthLuma(0),
        mRawFrameSizeInBits(0),
        mRawFrameSizeInPixs(0),
        gopPicSize(0),
        gopRefDist(0),
        bPyr(0),
        bFieldMode(0),
        fAbPeriodLong(0),
        fAbPeriodShort(0),
        dqAbPeriod(0),
        bAbPeriod(0),
        quantOffset(0),
        quantMaxI(0),
        quantMinI(0),
        quantMaxP(0),
        quantMinP(0),
        quantMaxB(0),
        quantMinB(0),
        iDQp0(0),
        iDQp(0),
        mNumRefsInGop(0),
        mIntraBoost(0),
        mMinQstepCmplxKP(0),
        mMinQstepRateEP(0),
        mMinQstepCmplxKPUpdt(0),
        mMinQstepCmplxKPUpdtErr(0),
        codecId(0)
    {}
#ifdef MFX_ENABLE_ENCTOOLS
    mfxStatus Init(mfxEncToolsCtrl const & ctrl, bool bFieldMode = false);
    mfxStatus GetBRCResetType(mfxEncToolsCtrl const & ctrl, bool bNewSequence, bool &bReset, bool &bSlidingWindowReset);
#endif
    mfxStatus Init(mfxVideoParam* par, bool bFieldMode = false);
    mfxStatus GetBRCResetType(mfxVideoParam* par, bool bNewSequence, bool &bReset, bool &bSlidingWindowReset );
};

struct sHrdInput
{
    bool   m_cbrFlag = false;
    mfxU32 m_bitrate = 0;
    mfxU32 m_maxCpbRemovalDelay = 0;
    mfxF64 m_clockTick = 0.0;
    mfxF64 m_cpbSize90k = 0.0;
    mfxF64 m_initCpbRemovalDelay = 0;

    void Init(cBRCParams par);
};


class HRDCodecSpec
{
private:
    mfxI32   m_overflowQuant  = 999;
    mfxI32   m_underflowQuant = 0;

public:
    mfxI32    GetMaxQuant() const { return m_overflowQuant - 1; }
    mfxI32    GetMinQuant() const { return m_underflowQuant + 1; }
    void      SetOverflowQuant(mfxI32 qp) { m_overflowQuant = qp; }
    void      SetUnderflowQuant(mfxI32 qp) { m_underflowQuant = qp; }
    void      ResetQuant() { m_overflowQuant = 999;  m_underflowQuant = 0;}

    virtual ~HRDCodecSpec() {}
    virtual void Init(cBRCParams const &par) = 0;
    virtual void Reset(cBRCParams const &par) = 0;
    virtual void Update(mfxU32 sizeInbits, mfxU32 encOrder, bool bSEI) = 0;
    virtual mfxU32 GetInitCpbRemovalDelay(mfxU32 encOrder) const = 0;
    virtual mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 encOrder) const = 0;
    virtual mfxU32 GetMaxFrameSizeInBits(mfxU32 encOrder, bool bSEI) const = 0;
    virtual mfxU32 GetMinFrameSizeInBits(mfxU32 encOrder, bool bSEI) const = 0;
    virtual mfxF64 GetBufferDeviation(mfxU32 encOrder) const = 0;
    virtual mfxF64 GetBufferDeviationFactor(mfxU32 encOrder) const = 0;
};

class HEVC_HRD: public HRDCodecSpec
{
public:
    HEVC_HRD() :
          m_prevAuCpbRemovalDelayMinus1(0)
        , m_prevAuCpbRemovalDelayMsb(0)
        , m_prevAuFinalArrivalTime(0)
        , m_prevBpAuNominalRemovalTime(0)
        , m_prevBpEncOrder(0)
    {}
    virtual ~HEVC_HRD() {}
    void Init(cBRCParams const&par) override;
    void Reset(cBRCParams const &par) override;
    void Update(mfxU32 sizeInbits, mfxU32 eo,  bool bSEI) override;
    mfxU32 GetInitCpbRemovalDelay(mfxU32 eo)  const override;
    mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 eo)  const override
    {
        return mfxU32(m_hrdInput.m_cpbSize90k - GetInitCpbRemovalDelay(eo));
    }
    mfxU32 GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI)  const override;
    mfxU32 GetMinFrameSizeInBits(mfxU32 eo, bool bSEI)  const override;
    mfxF64 GetBufferDeviation(mfxU32 eo)  const override;
    mfxF64 GetBufferDeviationFactor(mfxU32 eo)  const override;

protected:
    sHrdInput m_hrdInput;
    mfxI32 m_prevAuCpbRemovalDelayMinus1;
    mfxU32 m_prevAuCpbRemovalDelayMsb;
    mfxF64 m_prevAuFinalArrivalTime;
    mfxF64 m_prevBpAuNominalRemovalTime;
    mfxU32 m_prevBpEncOrder;
};

class H264_HRD: public HRDCodecSpec
{
public:
    H264_HRD();
    virtual ~H264_HRD() {}
    void Init(cBRCParams const &par) override;
    void Reset(cBRCParams const &par) override;
    void Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI) override;
    mfxU32 GetInitCpbRemovalDelay(mfxU32 eo)  const override;
    mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 eo)  const override;
    mfxU32 GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI)  const override;
    mfxU32 GetMinFrameSizeInBits(mfxU32 eo, bool bSEI)  const override;
    mfxF64 GetBufferDeviation(mfxU32 eo)  const override;
    mfxF64 GetBufferDeviationFactor(mfxU32 eo)  const override;

private:
    sHrdInput m_hrdInput;
    double m_trn_cur;   // nominal removal time
    double m_taf_prv;   // final arrival time of prev unit

};

inline mfxU32 hevcBitRateScale(mfxU32 bitrate)
{
    mfxU32 bit_rate_scale = 0;
    while (bit_rate_scale < 16 && (bitrate & ((1 << (6 + bit_rate_scale + 1)) - 1)) == 0)
        bit_rate_scale++;
    return bit_rate_scale;
}

inline mfxU32 hevcCbpSizeScale(mfxU32 cpbSize)
{
    mfxU32 cpb_size_scale = 2;
    while (cpb_size_scale < 16 && (cpbSize & ((1 << (4 + cpb_size_scale + 1)) - 1)) == 0)
        cpb_size_scale++;
    return cpb_size_scale;
}

inline mfxI32 GetRawFrameSize(mfxU32 lumaSize, mfxU16 chromaFormat, mfxU16 bitDepthLuma)
{
    mfxI32 frameSize = lumaSize;

    if (chromaFormat == MFX_CHROMAFORMAT_YUV420)
        frameSize += lumaSize / 2;
    else if (chromaFormat == MFX_CHROMAFORMAT_YUV422)
        frameSize += lumaSize;
    else if (chromaFormat == MFX_CHROMAFORMAT_YUV444)
        frameSize += lumaSize * 2;

    return frameSize * bitDepthLuma; //frame size in bits
}

static const mfxF64 MFX_QSTEP[88] = {
     0.630,  0.707,  0.794,  0.891,  1.000,   1.122,   1.260,   1.414,   1.587,   1.782,   2.000,   2.245,   2.520,
     2.828,  3.175,  3.564,  4.000,  4.490,   5.040,   5.657,   6.350,   7.127,   8.000,   8.980,  10.079,  11.314,
    12.699, 14.254, 16.000, 17.959, 20.159,  22.627,  25.398,  28.509,  32.000,  35.919,  40.317,  45.255,  50.797,
    57.018, 64.000, 71.838, 80.635, 90.510, 101.594, 114.035, 128.000, 143.675, 161.270, 181.019, 203.187, 228.070,
    256.000, 287.350, 322.540, 362.039, 406.375, 456.140, 512.000, 574.701, 645.080, 724.077, 812.749, 912.280,
    1024.000, 1149.401, 1290.159, 1448.155, 1625.499, 1824.561, 2048.000, 2298.802, 2580.318, 2896.309, 3250.997, 3649.121,
    4096.000, 4597.605, 5160.637, 5792.619, 6501.995, 7298.242, 8192.000, 9195.209, 10321.273, 11585.238, 13003.989, 14596.485
};


inline mfxI32 QStep2QpFloor(mfxF64 qstep, mfxI32 qpoffset = 0) // MFX_QSTEP[qp] <= qstep, return 0<=qp<=51+mQuantOffset
{
    mfxU8 qp = mfxU8(std::upper_bound(MFX_QSTEP, MFX_QSTEP + 51 + qpoffset, qstep) - MFX_QSTEP);
    return qp > 0 ? qp - 1 : 0;
}

inline mfxU8 QStep2QpCeil(mfxF64 qstep) // MFX_QSTEP[qp] > qstep
{
    return std::min<mfxU8>(mfxU8(std::lower_bound(MFX_QSTEP, MFX_QSTEP + 52, qstep) - MFX_QSTEP), 51);
}


inline mfxI32 Qstep2QP(mfxF64 qstep, mfxI32 qpoffset = 0) // return 0<=qp<=51+mQuantOffset
{
    mfxI32 qp = QStep2QpFloor(qstep, qpoffset);
    // prevent going MFX_QSTEP index out of bounds
    if (qp >= (mfxI32)(sizeof(MFX_QSTEP)/sizeof(MFX_QSTEP[0])) - 1)
        return 0;
    return (qp == 51 + qpoffset || qstep < (MFX_QSTEP[qp] + MFX_QSTEP[qp + 1]) / 2) ? qp : qp + 1;
}

inline mfxF64 QP2Qstep(mfxI32 qp, mfxI32 qpoffset = 0)
{
    return MFX_QSTEP[std::min(51 + qpoffset, qp)];
}

inline mfxI32 GetNewQP(mfxF64 totalFrameBits, mfxF64 targetFrameSizeInBits, mfxI32 minQP , mfxI32 maxQP, mfxI32 qp , mfxI32 qp_offset, mfxF64 f_pow, bool bStrict = false, bool bLim = true)
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
            qp_new  = std::max(qp_new, (minQP + qp + 1) >> 1);
        if (bStrict)
            qp_new  = std::min(qp_new, qp - 1);
    }
    else // underflow
    {
        if (qp >= maxQP)
        {
            return qp; // QP change is impossible
        }
        if (bLim)
            qp_new  = std::min(qp_new, (maxQP + qp + 1) >> 1);
        if (bStrict)
            qp_new  = std::max(qp_new, qp + 1);
    }
    return mfx::clamp(qp_new, minQP, maxQP);
}

// Reduce AB period before intra and increase it after intra (to avoid intra frame affecting the bottom of hrd)
inline mfxF64 GetAbPeriodCoeff (mfxU32 numInGop, mfxU32 gopPicSize, mfxU32 SC)
{
    const mfxU32 maxForCorrection = 30;
    mfxF64 maxValue = SC ? 1.3 : 1.5;
    const mfxF64 minValue = 1.0;

    mfxU32 numForCorrection = std::min(gopPicSize /2, maxForCorrection);
    mfxF64 k[maxForCorrection] = {0};

    if (numInGop >= gopPicSize || gopPicSize < 2)
        return 1.0;

    for (mfxU32 i = 0; i < numForCorrection; i ++)
    {
        k[i] = maxValue - (maxValue - minValue)*i/numForCorrection;
    }
    if (numInGop < gopPicSize/2)
    {
        return k[std::min(numInGop, numForCorrection - 1)];
    }
    else
    {
        mfxU32 n = gopPicSize - 1 - numInGop;
        return 1.0/ k[std::min(n, numForCorrection - 1)];
    }

}

inline mfxI32 GetNewQPTotal(mfxF64 bo, mfxF64 dQP, mfxI32 minQP , mfxI32 maxQP, mfxI32 qp, bool bPyr, bool bSC)
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


inline mfxStatus SetRecodeParams(mfxU16 brcStatus, mfxI32 qp, mfxI32 qp_new, mfxI32 minQP, mfxI32 maxQP, BRC_Ctx &ctx, mfxBRCFrameStatus* status)
{
    ctx.bToRecode = 1;

    if (brcStatus == MFX_BRC_BIG_FRAME || brcStatus == MFX_BRC_PANIC_BIG_FRAME )
    {
         MFX_CHECK(qp_new >= qp, MFX_ERR_UNDEFINED_BEHAVIOR);
         ctx.Quant = qp_new;
         ctx.QuantMax = maxQP;
         if (brcStatus == MFX_BRC_BIG_FRAME && qp_new > qp)
         {
            ctx.QuantMin = std::max(qp + 1, minQP); //limit QP range for recoding
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
            ctx.QuantMax = std::min(qp - 1, maxQP);
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

inline bool  isFrameBeforeIntra (mfxU32 order, mfxU32 intraOrder, mfxU32 gopPicSize, mfxU32 gopRefDist)
 {
     mfxI32 distance0 = gopPicSize*3/4;
     mfxI32 distance1 = gopPicSize - gopRefDist*3;
     return (order - intraOrder) > (mfxU32)(std::max(distance0, distance1));
 }

inline mfxU16 GetFrameType(mfxU16 m_frameType, mfxU16 level, mfxU16 gopRefDist)
{
    if (m_frameType & MFX_FRAMETYPE_IDR)
        return MFX_FRAMETYPE_IDR;
    else if (m_frameType & MFX_FRAMETYPE_I)
        return MFX_FRAMETYPE_I;
    else if (m_frameType & MFX_FRAMETYPE_P)
        return MFX_FRAMETYPE_P;
    else if ((m_frameType & MFX_FRAMETYPE_REF) && (level == 0 || gopRefDist == 1))
        return MFX_FRAMETYPE_P; //low delay B
    else
        return MFX_FRAMETYPE_B;
}

inline mfxI32 GetFrameTargetSize(mfxU32 brcSts, mfxI32 minFrameSize, mfxI32 maxFrameSize)
{
    if (brcSts != MFX_BRC_BIG_FRAME && brcSts != MFX_BRC_SMALL_FRAME) return 0;
    return (brcSts == MFX_BRC_BIG_FRAME) ? maxFrameSize * 3 / 4 : minFrameSize * 5 / 4;
}

inline mfxF64 getScaledIntraBits(mfxF64 targetBits, mfxF64 rawSize, mfxF64 raca)
{
    if (raca < BRC_MIN_RACA)  raca = BRC_MIN_RACA;
    mfxF64 SC = pow(raca, BRC_PWR_RACA);
    mfxF64 dBits = log((targetBits / rawSize) / SC);

    return dBits;
}

inline mfxI32 compute_first_qp_intra(mfxI32 targetBits, mfxI32 rawSize, mfxF64 raca)
{
    mfxF64 dBits = getScaledIntraBits(targetBits, rawSize, raca);
    mfxF64 coeffIntra[2];
    get_coeff_intra(coeffIntra);

    mfxF64 qpNew = (dBits - coeffIntra[1]) / coeffIntra[0];
    mfxI32 qp = (mfxI32)(qpNew + 0.5);
    if (qp < 1) qp = 1;
    return qp;
}

constexpr auto MAX_MODEL_ERR = 6;

inline mfxI32 compute_new_qp_intra(mfxI32 targetBits, mfxI32 rawSize, mfxF64 raca, mfxI32 iBits, mfxF64 icmplx, mfxI32 iqp)
{
    mfxF64 coeffIntra1[2], coeffIntra2[2];

    mfxF64 qp_hat = getScaledIntraBits(iBits, rawSize, icmplx);
    get_coeff_intra(coeffIntra1);
    qp_hat = (qp_hat - coeffIntra1[1]) / coeffIntra1[0];

    mfxF64 dQp = iqp - qp_hat;
    dQp = mfx::clamp(dQp, (-1.0 * MAX_MODEL_ERR), (1.0 * MAX_MODEL_ERR));

    mfxF64 qp_pred = getScaledIntraBits(targetBits, rawSize, raca);
    get_coeff_intra(coeffIntra2);

    qp_pred = (qp_pred - coeffIntra2[1]) / coeffIntra2[0];

    mfxF64 qpNew = qp_pred + dQp;

    mfxI32 qp = (mfxI32)(qpNew + 0.5);
    if (qp < 1) qp = 1;
    return qp;
}

inline void ResetMinQForMaxFrameSize(cBRCParams* par, mfxU32 type)
{
    if (type == MFX_FRAMETYPE_IDR || type == MFX_FRAMETYPE_I || type == MFX_FRAMETYPE_P) {
        par->mMinQstepCmplxKPUpdt = 0;
        par->mMinQstepCmplxKPUpdtErr = 0.16;
        par->mMinQstepCmplxKP = BRC_CONST_MUL_P1;
        par->mMinQstepRateEP = BRC_CONST_EXP_R_P1;
    }
}

inline mfxI32 GetMinQForMaxFrameSize(const cBRCParams& par, mfxF64 targetBits, mfxU32 type)
{
    mfxI32 qp = 0;
    if (type == MFX_FRAMETYPE_P) {
        if (par.mMinQstepCmplxKPUpdt > 2 && par.mMinQstepCmplxKPUpdtErr < 0.69) {
            mfxI32 rawSize = par.mRawFrameSizeInPixs;
            mfxF64 BitsDesiredFrame = targetBits * (1.0 - 0.165 - std::min(0.115, par.mMinQstepCmplxKPUpdtErr/3.0));
            mfxF64 R = (mfxF64)rawSize / BitsDesiredFrame;
            mfxF64 QstepScale = pow(R, par.mMinQstepRateEP) * par.mMinQstepCmplxKP;
            QstepScale = std::min(128.0, QstepScale);
            mfxF64 minqp = 6.0*log(QstepScale) / log(2.0) + 12.0;
            minqp = std::max(0.0, minqp);
            qp = (mfxU32)(minqp + 0.5);
            qp = mfx::clamp(qp, 1, 51);
        }
    }
    return qp;
}

inline void UpdateMinQForMaxFrameSize(cBRCParams* par, mfxI32 bits, mfxI32 qp, const BRC_Ctx& ctx, mfxU32 type, bool shstrt, mfxU16 brcSts)
{
    if (type == MFX_FRAMETYPE_I || type == MFX_FRAMETYPE_IDR) {
        mfxI32 rawSize = par->mRawFrameSizeInPixs;
        mfxF64 R = (mfxF64)rawSize / (mfxF64)bits;
        mfxF64 QstepScaleComputed = pow(R, par->mMinQstepRateEP) * par->mMinQstepCmplxKP;
        mfxF64 QstepScaleReal = pow(2.0, ((mfxF64)qp - 12.0) / 6.0);
        if (QstepScaleComputed > QstepScaleReal) {
            // Next P Frame atleast as complex as I Frame
            mfxF64 dS = log(QstepScaleReal) - log(QstepScaleComputed);
            par->mMinQstepCmplxKPUpdtErr = std::max<mfxF64>((par->mMinQstepCmplxKPUpdtErr + abs(dS)) / 2, abs(dS));
            mfxF64 upDlt = 0.5;
            dS = mfx::clamp(dS, -0.5, 1.0);
            par->mMinQstepCmplxKP = par->mMinQstepCmplxKP*(1.0 + upDlt*dS);
            //par->mMinQstepCmplxKPUpdt++;
            par->mMinQstepRateEP = mfx::clamp(par->mMinQstepRateEP + mfx::clamp(0.01 * (log(QstepScaleReal) - log(QstepScaleComputed))*log(R), -0.1, 0.2), 0.125, 1.0);

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
        if (ctx.LastIQpSetOrder < ctx.encOrder) {
            mfxI32 rawSize = par->mRawFrameSizeInPixs;
            mfxF64 R = (mfxF64)rawSize / (mfxF64)bits;
            mfxF64 QstepScaleComputed = pow(R, par->mMinQstepRateEP) * par->mMinQstepCmplxKP;
            mfxF64 QstepScaleReal = pow(2.0, ((mfxF64)qp - 12.0) / 6.0);
            mfxF64 dS = log(QstepScaleReal) - log(QstepScaleComputed);
            par->mMinQstepCmplxKPUpdtErr = std::max<mfxF64>((par->mMinQstepCmplxKPUpdtErr + abs(dS)) / 2, abs(dS));
            mfxF64 upDlt = mfx::clamp(1.3042 * pow(R, -0.922), 0.025, 0.5);
            if (shstrt || par->mMinQstepCmplxKPUpdt <= 2 || par->mMinQstepCmplxKPUpdtErr > 0.69) upDlt = 0.5;
            else if (brcSts != MFX_BRC_OK || par->mMinQstepCmplxKPUpdtErr > 0.41) upDlt = std::max(0.125, upDlt);
            dS = mfx::clamp(dS, -0.5, 1.0);
            par->mMinQstepCmplxKP = par->mMinQstepCmplxKP*(1.0 + upDlt*dS);
            par->mMinQstepCmplxKPUpdt++;
            par->mMinQstepRateEP = mfx::clamp(par->mMinQstepRateEP + mfx::clamp(0.01 * (log(QstepScaleReal) - log(QstepScaleComputed))*log(R), -0.1, 0.2), 0.125, 1.0);
        }
    }
}


inline mfxU16 CheckHrdAndUpdateQP(HRDCodecSpec &hrd, mfxU32 frameSizeInBits, mfxU32 eo, bool bIdr, mfxI32 currQP)
{
    if (frameSizeInBits > hrd.GetMaxFrameSizeInBits(eo, bIdr))
    {
        hrd.SetUnderflowQuant(currQP);
        return MFX_BRC_BIG_FRAME;
    }
    else if (frameSizeInBits < hrd.GetMinFrameSizeInBits(eo, bIdr))
    {
        hrd.SetOverflowQuant(currQP);
        return MFX_BRC_SMALL_FRAME;
    }
    return MFX_BRC_OK;
}

mfxI32 GetOffsetAPQ(mfxI32 level, mfxU16 isRef, mfxU16 apqInfo);

// Set all Base QPs (IDR/I/P/B) from given QP for frame of type, level, iRef, and Adaptive Pyramid QP info (apqInfo).
inline void SetQPParams(mfxI32 qp, mfxU32 type, BRC_Ctx  &ctx, mfxU32 /* rec_num */, mfxI32 minQuant, mfxI32 maxQuant, mfxU32 level, mfxU32 iDQp, mfxU16 isRef, mfxU16 apqInfo)
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
        qp -= GetOffsetAPQ(level, isRef, apqInfo);
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

inline void UpdateQPParams(mfxI32 qp, mfxU32 type , BRC_Ctx  &ctx, mfxU32 rec_num, mfxI32 minQuant, mfxI32 maxQuant, mfxU32 level, mfxU32 iDQp, mfxU16 isRef, mfxU16 apqInfo)
{
    ctx.Quant = qp;
    if (ctx.LastIQpSetOrder > ctx.encOrder) return;

    SetQPParams(qp, type, ctx, rec_num, minQuant, maxQuant, level, iDQp, isRef, apqInfo);
}


#endif
