// Copyright (c) 2017 Intel Corporation
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

#ifndef __MFX_H265_BRC_HW_H__
#define __MFX_H265_BRC_HW_H__

#include "mfx_common.h"
#include "mfxla.h"
#include <vector>
#include "mfx_h265_encode_hw_utils.h"
#include <memory>
#include <mfx_brc_common.h>


namespace MfxHwH265Encode
{

enum eMfxBRCStatus
{
    MFX_BRC_ERROR                   = -1,
    MFX_BRC_OK                      = 0x0,
    MFX_BRC_ERR_BIG_FRAME           = 0x1,
    MFX_BRC_BIG_FRAME               = 0x2,
    MFX_BRC_ERR_SMALL_FRAME         = 0x4,
    MFX_BRC_SMALL_FRAME             = 0x8,
    MFX_BRC_NOT_ENOUGH_BUFFER       = 0x10
};
struct MbData
{
    mfxU32      intraCost;
    mfxU32      interCost;
    mfxU32      propCost;
    mfxU8       w0;
    mfxU8       w1;
    mfxU16      dist;
    mfxU16      rate;
    mfxU16      lumaCoeffSum[4];
    mfxU8       lumaCoeffCnt[4];
    mfxI16Pair  costCenter0;
    mfxI16Pair  costCenter1;
    struct
    {
        mfxU32  intraMbFlag     : 1;
        mfxU32  skipMbFlag      : 1;
        mfxU32  mbType          : 5;
        mfxU32  reserved0       : 1;
        mfxU32  subMbShape      : 8;
        mfxU32  subMbPredMode   : 8;
        mfxU32  reserved1       : 8;
    };
    mfxI16Pair  mv[2]; // in sig-sag scan
};
struct VmeData
{
    VmeData()
        : used(false)
        , poc(mfxU32(-1))
        , pocL0(mfxU32(-1))
        , pocL1(mfxU32(-1))
        , intraCost(0)
        , interCost(0)
        , propCost(0) { }

    bool                used;
    mfxU32              poc;
    mfxU32              pocL0;
    mfxU32              pocL1;
    mfxU32              encOrder;
    mfxU32              intraCost;
    mfxU32              interCost;
    mfxU32              propCost;
    std::vector   <MbData> mb;
};

template <size_t N>
class Regression
{
public:
    static const mfxU32 MAX_WINDOW = N;

    Regression() {
        Zero(x);
        Zero(y);
    }
    void Reset(mfxU32 size, mfxF64 initX, mfxF64 initY) {
        windowSize = size;
        normX = initX;
        std::fill_n(x, windowSize, initX);
        std::fill_n(y, windowSize, initY);
        sumxx = initX * initX * windowSize;
        sumxy = initX * initY * windowSize;
    }
    void Add(mfxF64 newx, mfxF64 newy) {
        newy = newy / newx * normX;
        newx = normX;
        sumxy += newx * newy - x[0] * y[0];
        sumxx += newx * newx - x[0] * x[0];
        std::copy(x + 1, x + windowSize, x);
        std::copy(y + 1, y + windowSize, y);
        x[windowSize - 1] = newx;
        y[windowSize - 1] = newy;
    }

    mfxF64 GetCoeff() const {
        return sumxy / sumxx;
    }

//protected:
public: // temporary for debugging and dumping
    mfxF64 x[N];
    mfxF64 y[N];
    mfxU32 windowSize;
    mfxF64 normX;
    mfxF64 sumxy;
    mfxF64 sumxx;
};

typedef mfxI32 mfxBRCStatus;


class BrcIface
{
public:
    virtual ~BrcIface() {};
    virtual mfxStatus Init(MfxVideoParam &video, mfxI32 enableRecode = 1) = 0;
    virtual mfxStatus Reset(MfxVideoParam &video, mfxI32 enableRecode = 1) = 0;
    virtual mfxStatus Close() = 0;
    virtual void PreEnc(mfxU32 frameType, std::vector<VmeData *> const & vmeData, mfxU32 encOrder) = 0;    
    virtual mfxI32 GetQP(MfxVideoParam &video, Task &task)=0;
    virtual mfxStatus SetQP(mfxI32 qp, mfxU16 frameType, bool bLowDelay) = 0;
    virtual mfxBRCStatus   PostPackFrame(MfxVideoParam &video, Task &task, mfxI32 bitsEncodedFrame, mfxI32 overheadBits, mfxI32 recode = 0) =0;
    virtual mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics*, mfxU32 , mfxU32 ) = 0;
    virtual void GetMinMaxFrameSize(mfxI32 *minFrameSizeInBits, mfxI32 *maxFrameSizeInBits) = 0;
    virtual bool IsVMEBRC() = 0;

};

BrcIface * CreateBrc(MfxVideoParam &video);

class VMEBrc : public BrcIface
{
public:
    virtual ~VMEBrc() { Close(); }

    mfxStatus Init( MfxVideoParam &video, mfxI32 enableRecode = 1);
    mfxStatus Reset(MfxVideoParam &video, mfxI32 enableRecode = 1) 
    { 
        return  Init( video, enableRecode);
    }

    mfxStatus Close() {  return MFX_ERR_NONE;}
        
    mfxI32 GetQP(MfxVideoParam &video, Task &task);
    mfxStatus SetQP(mfxI32 /* qp */, mfxU16 /* frameType */,  bool /*bLowDelay*/) { return MFX_ERR_NONE;  }

    void PreEnc(mfxU32 frameType, std::vector<VmeData *> const & vmeData, mfxU32 encOrder);

    mfxBRCStatus   PostPackFrame(MfxVideoParam &video, Task &task, mfxI32 bitsEncodedFrame, mfxI32 overheadBits, mfxI32 recode = 0)
    {
        recode;
        overheadBits;
        video;
        Report(task.m_frameType, bitsEncodedFrame >> 3, 0, 0, task.m_eo, 0, 0); 
        return MFX_ERR_NONE;    
    }
    bool IsVMEBRC()  {return true;}
    mfxU32          Report(mfxU32 frameType, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 repack, mfxU32 picOrder, mfxU32 maxFrameSize, mfxU32 qp); 
    mfxStatus       SetFrameVMEData(const mfxExtLAFrameStatistics *, mfxU32 widthMB, mfxU32 heightMB );
    void            GetMinMaxFrameSize(mfxI32 *minFrameSizeInBits, mfxI32 *maxFrameSizeInBits) {*minFrameSizeInBits = 0; *maxFrameSizeInBits = 0;}
        

public:
    struct LaFrameData
    {
        mfxU32  encOrder;
        mfxU32  dispOrder;
        mfxI32  poc;
        mfxI32  deltaQp;
        mfxF64  estRate[52];
        mfxF64  estRateTotal[52];
        mfxU32  interCost;
        mfxU32  intraCost;
        mfxU32  propCost;
        mfxU32  bframe;
        mfxI32  qp;
        mfxU16   layer;
        bool    bNotUsed;
    };

protected:
    mfxU32  m_lookAheadDep;
    mfxU32  m_totNumMb;
    mfxF64  m_initTargetRate;
    mfxF64  m_targetRateMin;
    mfxF64  m_targetRateMax;
    mfxU32  m_framesBehind;
    mfxF64  m_bitsBehind;
    mfxI32  m_curBaseQp;
    mfxI32  m_curQp;
    mfxU16  m_qpUpdateRange;

    std::list <LaFrameData> m_laData;
    Regression<20>   m_rateCoeffHistory[52];
    UMC::Mutex    m_mutex;

};

enum eMfxBRCRecode
{
    MFX_BRC_RECODE_NONE           = 0,
    MFX_BRC_RECODE_QP             = 1,
    MFX_BRC_RECODE_PANIC          = 2,
    MFX_BRC_RECODE_EXT_QP         = 3,
    MFX_BRC_RECODE_EXT_PANIC      = 4,
    MFX_BRC_EXT_FRAMESKIP         = 16
};

struct HRDState
{
    mfxF64 bufFullness;
    mfxF64 prevBufFullness;
    mfxI32 frameNum;
    mfxI32 minFrameSize;
    mfxI32 maxFrameSize;
    mfxI32 underflowQuant;
    mfxI32 overflowQuant;
};
struct BRCParams
{
    mfxU16 rateControlMethod;
    mfxU32 bufferSizeInBytes;
    mfxU32 initialDelayInBytes;
    mfxU32 targetbps;
    mfxU32 maxbps;
    mfxF64 frameRate;
    mfxU16 width;
    mfxU16 height;
    mfxU16 chromaFormat;
    mfxU16 bitDepthLuma;
    mfxF64 inputBitsPerFrame;
    mfxU16 gopPicSize;
    mfxU16 gopRefDist;

};

class H265BRC : public BrcIface
{

public:

    H265BRC()
    {
        m_IsInit = false;
        memset(&m_par, 0, sizeof(m_par));
        memset(&m_hrdState, 0, sizeof(m_hrdState));
        ResetParams();
    }
    virtual ~H265BRC()
    {
        Close();
    }




    virtual mfxStatus   Init(MfxVideoParam &video, mfxI32 enableRecode = 1);
    virtual mfxStatus   Close();
    virtual mfxStatus   Reset(MfxVideoParam &video, mfxI32 enableRecode = 1);
    virtual mfxBRCStatus PostPackFrame(MfxVideoParam &video, Task &task, mfxI32 bitsEncodedFrame, mfxI32 overheadBits, mfxI32 recode = 0);
    virtual mfxI32      GetQP(MfxVideoParam &video, Task &task);
    virtual mfxStatus   SetQP(mfxI32 qp, uint16_t frameType, bool bLowDelay);
    //virtual mfxStatus   GetInitialCPBRemovalDelay(mfxU32 *initial_cpb_removal_delay, mfxI32 recode = 0);
    virtual void        GetMinMaxFrameSize(mfxI32 *minFrameSizeInBits, mfxI32 *maxFrameSizeInBits);


    virtual void        PreEnc(mfxU32 /* frameType */, std::vector<VmeData *> const & /* vmeData */, mfxU32 /* encOrder */) {}
    virtual mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics*, mfxU32 , mfxU32 )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual bool IsVMEBRC()  {return false;}

protected:

    bool   m_IsInit;

    BRCParams m_par;
    HRDState  m_hrdState;

    mfxI32  mQuantI, mQuantP, mQuantB, mQuantMax, mQuantMin, mQuantPrev, mQuantOffset, mQPprev;
    mfxI32  mMinQp;
    mfxI32  mMaxQp;

    //mfxI64  mBF, mBFsaved; // buffer fullness
    mfxI32  mBitsDesiredFrame; //average frame size
    uint16_t  mQuantUpdated;
    mfxU32  mRecodedFrame_encOrder;
    bool    m_bRecodeFrame;
    mfxU32  mPicType;

    mfxI32 mRecode;
    mfxI64  mBitsEncodedTotal, mBitsDesiredTotal;
    mfxI32  mQp;
    mfxI32  mRCfap, mRCqap, mRCbap, mRCq;
    mfxF64  mRCqa, mRCfa, mRCqa0;
    mfxF64  mRCfa_short;
    mfxI32  mQuantRecoded;
    mfxI32  mQuantIprev, mQuantPprev, mQuantBprev;
    mfxI32  mBitsEncoded;
    mfxI32  mSceneChange;
    mfxI32  mBitsEncodedP, mBitsEncodedPrev;
    mfxI32  mPoc, mSChPoc;
    mfxU32  mEOLastIntra;

    void   ResetParams();
    mfxU32 GetInitQP();
    mfxBRCStatus UpdateAndCheckHRD(mfxI32 frameBits, mfxF64 inputBitsPerFrame, mfxI32 recode);
    void  SetParamsForRecoding (mfxI32 encOrder);

    mfxBRCStatus UpdateQuant(mfxI32 bEncoded, mfxI32 totalPicBits, mfxI32 layer = 0, mfxI32 recode = 0);
    mfxBRCStatus UpdateQuantHRD(mfxI32 bEncoded, mfxBRCStatus sts, mfxI32 overheadBits = 0, mfxI32 layer = 0, mfxI32 recode = 0);
    mfxStatus   SetParams(MfxVideoParam &video);

    bool        isFrameBeforeIntra (mfxU32 order);


    mfxStatus InitHRD();
 
};
}

#endif
