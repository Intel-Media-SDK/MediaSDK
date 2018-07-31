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

#include "mfxbrc.h"

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
        Close();
        return  Init( video, enableRecode);
    }

    mfxStatus Close() {  return MFX_ERR_NONE;}
        
    mfxI32 GetQP(MfxVideoParam &video, Task &task);
    mfxStatus SetQP(mfxI32 /* qp */, mfxU16 /* frameType */,  bool /*bLowDelay*/) { return MFX_ERR_NONE;  }

    void PreEnc(mfxU32 frameType, std::vector<VmeData *> const & vmeData, mfxU32 encOrder);

    mfxBRCStatus   PostPackFrame(MfxVideoParam & /*video*/, Task &task, mfxI32 bitsEncodedFrame, mfxI32 /*overheadBits*/, mfxI32 /*recode = 0*/)
    {
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



}
namespace MfxHwH265Encode
{
class H265BRCNew : public BrcIface
{

public:
    H265BRCNew():
        m_minSize(0),
        m_pBRC(0)
    {
        memset(&m_BRCLocal,0, sizeof(m_BRCLocal));
    }    
    virtual ~H265BRCNew()
    {
        Close();
    }
    virtual mfxStatus   Init(MfxVideoParam &video, mfxI32 )
    {
        mfxStatus sts = MFX_ERR_NONE;
        if (video.m_ext.extBRC.pthis)
        {
            m_pBRC = &video.m_ext.extBRC;
            MFX_CHECK_NULL_PTR3(m_pBRC->Init, m_pBRC->Close, m_pBRC->Reset);
            MFX_CHECK_NULL_PTR2(m_pBRC->GetFrameCtrl, m_pBRC->Update);        
        }
        else
        {
            sts = HEVCExtBRC::Create(m_BRCLocal);
            MFX_CHECK_STS(sts);
            m_pBRC = &m_BRCLocal;
        }
        return m_pBRC->Init(m_pBRC->pthis, &video);
    }
    virtual mfxStatus   Close()
    {
        mfxStatus sts = MFX_ERR_NONE;
        sts =  m_pBRC->Close(m_pBRC->pthis);
        MFX_CHECK_STS(sts);
        HEVCExtBRC::Destroy(m_BRCLocal);
        return sts;
    }
    virtual mfxStatus   Reset(MfxVideoParam &video, mfxI32 )
    {
        return m_pBRC->Reset(m_pBRC->pthis,&video);    
    }
    virtual mfxBRCStatus PostPackFrame(MfxVideoParam &, Task &task, mfxI32 bitsEncodedFrame, mfxI32 , mfxI32 )
    {
        mfxBRCFrameParam frame_par={};
        mfxBRCFrameCtrl  frame_ctrl={};
        mfxBRCFrameStatus frame_sts = {};
        

        frame_ctrl.QpY = task.m_qpY;
        InitFramePar(task,frame_par);
        frame_par.CodedFrameSize = bitsEncodedFrame/8;  // Size of frame in bytes after encoding


        mfxStatus sts = m_pBRC->Update(m_pBRC->pthis,&frame_par, &frame_ctrl, &frame_sts);
        MFX_CHECK(sts == MFX_ERR_NONE, MFX_BRC_ERROR);

        m_minSize = frame_sts.MinFrameSize;

        switch (frame_sts.BRCStatus)
        {
        case ::MFX_BRC_OK:
            return MFX_BRC_OK;
        case ::MFX_BRC_BIG_FRAME: 
            return MFX_BRC_ERR_BIG_FRAME;
        case ::MFX_BRC_SMALL_FRAME: 
            return MFX_BRC_ERR_SMALL_FRAME;
        case ::MFX_BRC_PANIC_BIG_FRAME: 
            return MFX_BRC_ERR_BIG_FRAME | MFX_BRC_NOT_ENOUGH_BUFFER;
        case ::MFX_BRC_PANIC_SMALL_FRAME:  
            return MFX_BRC_ERR_SMALL_FRAME| MFX_BRC_NOT_ENOUGH_BUFFER;
        }
        return MFX_BRC_OK;    
    }
    virtual mfxI32      GetQP(MfxVideoParam &, Task &task)
    {
        mfxBRCFrameParam frame_par={};
        mfxBRCFrameCtrl  frame_ctrl={};

        InitFramePar(task,frame_par);
        m_pBRC->GetFrameCtrl(m_pBRC->pthis,&frame_par, &frame_ctrl);
        return frame_ctrl.QpY;    
    }
    virtual mfxStatus   SetQP(mfxI32 , uint16_t , bool )
    {
        return MFX_ERR_NONE;    
    }
    //virtual mfxStatus   GetInitialCPBRemovalDelay(mfxU32 *initial_cpb_removal_delay, mfxI32 recode = 0);
    virtual void        GetMinMaxFrameSize(mfxI32 *minFrameSizeInBits, mfxI32 *)
    {
        *minFrameSizeInBits = m_minSize;
    
    }


    virtual void        PreEnc(mfxU32 /* frameType */, std::vector<VmeData *> const & /* vmeData */, mfxU32 /* encOrder */) {}
    virtual mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics*, mfxU32 , mfxU32 )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    virtual bool IsVMEBRC()  {return false;}

private:
    mfxU32      m_minSize;
    mfxExtBRC * m_pBRC;
    mfxExtBRC   m_BRCLocal;

protected:
    void InitFramePar(Task &task, mfxBRCFrameParam & frame_par)
    {
        frame_par.EncodedOrder = task.m_eo;    // Frame number in a sequence of reordered frames starting from encoder Init()
        frame_par.DisplayOrder = task.m_poc;    // Frame number in a sequence of frames in display order starting from last IDR
        frame_par.FrameType = task.m_frameType;       // See FrameType enumerator
        frame_par.PyramidLayer = (mfxU16)task.m_level;    // B-pyramid or P-pyramid layer, frame belongs to
        frame_par.NumRecode = (mfxU16)task.m_recode;       // Number of recodings performed for this frame
        if (task.m_secondField)
            frame_par.PyramidLayer = frame_par.PyramidLayer +1;    
    }

 
};
}

#endif
