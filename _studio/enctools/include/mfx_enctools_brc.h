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

#ifndef __MFX_ENCTOOLS_BRC_H__
#define __MFX_ENCTOOLS_BRC_H__

#include "mfx_enctools_brc_defs.h"
#include "mfxdefs.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "mfx_enctools_utils.h"
#include <climits>
namespace EncToolsBRC
{


/*
NalHrdConformance | VuiNalHrdParameters   |  Result
--------------------------------------------------------------
    off                  any                => MFX_BRC_NO_HRD
    default              off                => MFX_BRC_NO_HRD
    on                   off                => MFX_BRC_HRD_WEAK
    on (or default)      on (or default)    => MFX_BRC_HRD_STRONG
--------------------------------------------------------------
*/



struct BRC_FrameStruct
{
    BRC_FrameStruct() :
        frameType(0),
        pyrLayer(0),
        encOrder(0),
        dispOrder(0),
        qp(0),
        frameSize(0),
        numRecode(0),
        sceneChange(0),
        longTerm(0),
        frameCmplx(0),
        OptimalFrameSizeInBytes(0),
        optimalBufferFullness(0),
        qpDelta(MFX_QP_UNDEFINED),
        qpModulation(MFX_QP_MODULATION_NOT_DEFINED)
    {}
    mfxU16 frameType;
    mfxU16 pyrLayer;
    mfxU32 encOrder;
    mfxU32 dispOrder;
    mfxI32 qp;
    mfxI32 frameSize;
    mfxI32 numRecode;
    mfxU16 sceneChange;
    mfxU16 longTerm;
    mfxU32 frameCmplx;
    mfxU32 OptimalFrameSizeInBytes;
    mfxU32 optimalBufferFullness;
    mfxI16 qpDelta;
    mfxU16 qpModulation;
};



class BRC_EncTool
{
public:
    BRC_EncTool() :
        m_bInit(false),
        m_par(),
        m_hrdSpec(),
        m_bDynamicInit(false),
        m_ctx(),
        m_SkipCount(0),
        m_ReEncodeCount(0)
    {}
    ~BRC_EncTool() { Close(); }

    mfxStatus Init(mfxEncToolsCtrl const & ctrl);
    mfxStatus Reset(mfxEncToolsCtrl const & ctrl);
    void Close()
    {
        m_bInit = false;
    }

    mfxStatus ReportEncResult(mfxU32 dispOrder, mfxEncToolsBRCEncodeResult const & pEncRes);
    mfxStatus SetFrameStruct(mfxU32 dispOrder, mfxEncToolsBRCFrameParams  const & pFrameStruct);
    mfxStatus ReportBufferHints(mfxU32 dispOrder, mfxEncToolsBRCBufferHint const & pBufHints);
    mfxStatus ReportGopHints(mfxU32 dispOrder, mfxEncToolsHintPreEncodeGOP const & pGopHints);
    mfxStatus ProcessFrame(mfxU32 dispOrder, mfxEncToolsBRCQuantControl *pFrameQp);
    mfxStatus UpdateFrame(mfxU32 dispOrder, mfxEncToolsBRCStatus *pFrameSts);
    mfxStatus GetHRDPos(mfxU32 dispOrder, mfxEncToolsBRCHRDPos *pHRDPos);


protected:
    bool m_bInit;
    cBRCParams m_par;
    std::unique_ptr < HRDCodecSpec> m_hrdSpec;
    bool       m_bDynamicInit;
    BRC_Ctx    m_ctx;
    std::unique_ptr<AVGBitrate> m_avg;
    mfxU32     m_SkipCount;
    mfxU32     m_ReEncodeCount;
    std::vector<BRC_FrameStruct> m_FrameStruct;

    mfxI32 GetCurQP(mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const;
    mfxI32 GetSeqQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const;
    mfxI32 GetPicQP(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxU16 qpMod) const;
    mfxF64 ResetQuantAb(mfxI32 qp, mfxU32 type, mfxI32 layer, mfxU16 isRef, mfxF64 fAbLong, mfxU32 eo, bool bIdr, mfxU16 qpMod) const;
};

};


#endif


