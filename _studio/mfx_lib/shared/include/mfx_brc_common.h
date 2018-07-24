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

#ifndef __MFX_BRC_COMMON_H__
#define __MFX_BRC_COMMON_H__

#include "mfx_common.h"
#include "mfxbrc.h"
#include <vector>
#include <memory>

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE) || defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)
#define UMC_ENABLE_VIDEO_BRC
#define MFX_ENABLE_VIDEO_BRC_COMMON

#include "umc_video_brc.h"

mfxStatus ConvertVideoParam_Brc(const mfxVideoParam *parMFX, UMC::VideoBrcParams *parUMC);
#endif

class AVGBitrate
{
public:
    AVGBitrate(mfxU32 windowSize, mfxU32 maxBitPerFrame, mfxU32 avgBitPerFrame, bool bLA = false):
        m_maxWinBits(maxBitPerFrame*windowSize),
        m_maxWinBitsLim(0),
        m_avgBitPerFrame(MFX_MIN(avgBitPerFrame, maxBitPerFrame)),
        m_currPosInWindow(0),
        m_lastFrameOrder(0),
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
                m_maxWinBitsLim = MFX_MAX(MFX_MIN((GetLastFrameBits(windowSize,false) + m_maxWinBits) / 2, m_maxWinBits), GetMaxWinBitsLim());
            }
            else
            {
                if (recode)
                    m_maxWinBitsLim = MFX_MIN(MFX_MAX(GetLastFrameBits(windowSize,false) + GetStep() / 2, m_maxWinBitsLim), m_maxWinBits);
                else if ((m_maxWinBitsLim > GetMaxWinBitsLim() + GetStep()) &&
                    (m_maxWinBitsLim - GetStep() > (GetLastFrameBits(windowSize - 1,false) + sizeInBits)))
                    m_maxWinBitsLim -= GetStep();
            }

        }
    }
    mfxU32 GetMaxFrameSize(bool bPanic, bool bSH, mfxU32 recode)
    {
        mfxU32 winBits = GetLastFrameBits(GetWindowSize() - 1, recode < 1);

        mfxU32 maxWinBitsLim = m_maxWinBitsLim;
        if (bSH)
            maxWinBitsLim = (m_maxWinBits + m_maxWinBitsLim) / 2;
        if (bPanic)
            maxWinBitsLim = m_maxWinBits;
        maxWinBitsLim = MFX_MIN(maxWinBitsLim + recode*GetStep() / 2, m_maxWinBits);

        mfxU32 maxFrameSize = winBits >= m_maxWinBitsLim ?
            m_maxWinBits - winBits :
            maxWinBitsLim - winBits;

        return maxFrameSize;
    }
    mfxU32 GetWindowSize()
    {
        return (mfxU32)m_slidingWindow.size();
    }
    mfxI32 GetBudget(mfxU32 numFrames)
    {
        numFrames = MFX_MIN((mfxU32)m_slidingWindow.size(), numFrames);
        return ((mfxI32)m_maxWinBitsLim - (mfxI32)GetLastFrameBits((mfxU32)m_slidingWindow.size() - numFrames, false));
    }


protected:

    mfxU32                      m_maxWinBits;
    mfxU32                      m_maxWinBitsLim;
    mfxU32                      m_avgBitPerFrame;

    mfxU32                      m_currPosInWindow;
    mfxU32                      m_lastFrameOrder;
    bool                        m_bLA;
    std::vector<mfxU32>         m_slidingWindow;



    mfxU32 GetLastFrameBits(mfxU32 numFrames, bool bCheckSkip)
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
    mfxU32 GetStep()
    {
        return  (m_maxWinBits / GetWindowSize() - m_avgBitPerFrame) / (m_bLA ? 4 : 2);
    }

    mfxU32 GetMaxWinBitsLim()
    {
        return m_maxWinBits - GetStep() * GetWindowSize();
    }
};

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE) || defined (MFX_ENABLE_H265_VIDEO_ENCODE)

#define MIN_RACA 0.25
#define MAX_RACA 361.0
#define RACA_SCALE 128.0


namespace MfxHwH265EncodeBRC
{
class cBRCParams
{
public:
    mfxU16 rateControlMethod; // CBR or VBR

    mfxU16 bHRDConformance;  // is HRD compliance  needed
    mfxU16 bRec;             // is Recoding possible
    mfxU16 bPanic;           // is Panic mode possible

    // HRD params
    mfxU32 bufferSizeInBytes;
    mfxU32 initialDelayInBytes;

    // Sliding window parameters
    mfxU16  WinBRCMaxAvgKbps;
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

public:
    cBRCParams() :
        rateControlMethod(0),
        bHRDConformance(0),
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
        mMinQstepCmplxKPUpdtErr(0)
    {}

    mfxStatus Init(mfxVideoParam* par, bool bFieldMode = false);
    mfxStatus GetBRCResetType(mfxVideoParam* par, bool bNewSequence, bool &bReset, bool &bSlidingWindowReset );
};
class cHRD
{
public:
    cHRD():
        m_bufFullness(0),
        m_prevBufFullness(0),
        m_frameNum(0),
        m_minFrameSize(0),
        m_maxFrameSize(0),
        m_underflowQuant(0),
        m_overflowQuant(0),
        m_buffSizeInBits(0),
        m_delayInBits(0),
        m_inputBitsPerFrame(0),
        m_bCBR (false)
    {}

    void      Init(mfxU32 buffSizeInBytes, mfxU32 delayInBytes, mfxF64 inputBitsPerFrame, bool cbr);
    mfxU16    UpdateAndCheckHRD(mfxI32 frameBits, mfxI32 recode, mfxI32 minQuant, mfxI32 maxQuant);
    mfxStatus UpdateMinMaxQPForRec(mfxU32 brcSts, mfxI32 qp);
    mfxI32    GetTargetSize(mfxU32 brcSts);
    mfxI32    GetMaxFrameSize() { return m_maxFrameSize;}
    mfxI32    GetMinFrameSize() { return m_minFrameSize;}
    mfxI32    GetMaxQuant() { return m_overflowQuant -  1;}
    mfxI32    GetMinQuant() { return m_underflowQuant + 1;}
    mfxF64    GetBufferDiviation(mfxU32 targetBitrate);
    mfxF64    GetBufferDiviation();
    mfxF64    GetBufferDiviationFactor();



private:
    mfxF64 m_bufFullness;
    mfxF64 m_prevBufFullness;
    mfxI32 m_frameNum;
    mfxI32 m_minFrameSize;
    mfxI32 m_maxFrameSize;
    mfxI32 m_underflowQuant;
    mfxI32 m_overflowQuant;

private:
    mfxI32 m_buffSizeInBits;
    mfxI32 m_delayInBits;
    mfxF64 m_inputBitsPerFrame;
    bool   m_bCBR;

};
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
    mfxF64 totalDiviation;   // divation from  target bitrate (total)

    mfxF64 eRate;               // eRate of last encoded frame, this parameter is used for scene change calculation
    mfxF64 eRateSH;             // eRate of last encoded scene change frame, this parameter is used for scene change calculation
};

class ExtBRC
{
private:
    cBRCParams m_par;
    cHRD       m_hrd;
    bool       m_bInit;
    bool       m_bDynamicInit;
    BRC_Ctx    m_ctx;
    std::unique_ptr<AVGBitrate> m_avg;
    mfxU32     m_SkipCount;
    mfxU32     m_ReEncodeCount;

public:
    ExtBRC():
        m_par(),
        m_hrd(),
        m_bInit(false),
        m_bDynamicInit(false),
        m_SkipCount(0),
        m_ReEncodeCount(0)
    {
        memset(&m_ctx, 0, sizeof(m_ctx));

    }
    mfxStatus Init (mfxVideoParam* par);
    mfxStatus Reset(mfxVideoParam* par);
    mfxStatus Close () {
        //printf("\nFrames skipped: %i \n", m_SkipCount);
        //printf("\nNumber of re-encodes: %i \n", m_ReEncodeCount);
        m_bInit = false;
        return MFX_ERR_NONE;
    }
    mfxStatus GetFrameCtrl (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl);
    mfxStatus Update (mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status);

protected:
    mfxI32 GetCurQP (mfxU32 type, mfxI32 layer, mfxU16 isRef);
    mfxI32 GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef);
    mfxI32 GetPicQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef);
    mfxF64 ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong);
};
}
namespace HEVCExtBRC
{
    inline mfxStatus Init  (mfxHDL pthis, mfxVideoParam* par)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Init(par) ;
    }
    inline mfxStatus Reset (mfxHDL pthis, mfxVideoParam* par)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Reset(par) ;
    }
    inline mfxStatus Close (mfxHDL pthis)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Close() ;
    }
    inline mfxStatus GetFrameCtrl (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
    {
       MFX_CHECK_NULL_PTR1(pthis);
       return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->GetFrameCtrl(par,ctrl) ;
    }
    inline mfxStatus Update       (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((MfxHwH265EncodeBRC::ExtBRC*)pthis)->Update(par,ctrl, status) ;
    }
    inline mfxStatus Create(mfxExtBRC & m_BRC)
    {
        MFX_CHECK(m_BRC.pthis == NULL, MFX_ERR_UNDEFINED_BEHAVIOR);
        m_BRC.pthis = new MfxHwH265EncodeBRC::ExtBRC;
        m_BRC.Init = Init;
        m_BRC.Reset = Reset;
        m_BRC.Close = Close;
        m_BRC.GetFrameCtrl = GetFrameCtrl;
        m_BRC.Update = Update;
        return MFX_ERR_NONE;
    }
    inline mfxStatus Destroy(mfxExtBRC & m_BRC)
    {

        MFX_CHECK(m_BRC.pthis != NULL, MFX_ERR_NONE);
        delete (MfxHwH265EncodeBRC::ExtBRC*)m_BRC.pthis;

        m_BRC.pthis = 0;
        m_BRC.Init = 0;
        m_BRC.Reset = 0;
        m_BRC.Close = 0;
        m_BRC.GetFrameCtrl = 0;
        m_BRC.Update = 0;
        return MFX_ERR_NONE;
    }
}

#endif
#endif



