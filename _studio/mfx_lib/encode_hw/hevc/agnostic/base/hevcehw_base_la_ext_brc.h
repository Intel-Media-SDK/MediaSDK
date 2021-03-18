// Copyright (c) 2021 Intel Corporation
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

#pragma once

#include "mfx_brc_common.h"
#include "mfxla.h"

namespace HEVCEHW
{
namespace Base
{


template <size_t N>
class Regression
{
public:
    static const mfxU32 MAX_WINDOW = N;

    Regression() {}
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
    mfxF64 x[N] = {};
    mfxF64 y[N] = {};
    mfxU32 windowSize = 0.0;
    mfxF64 normX = 0.0;
    mfxF64 sumxy = 0.0;
    mfxF64 sumxx = 0.0;
};

class LAExtBrc
{
public:
    virtual ~LAExtBrc() { Close(); }

    mfxStatus Init(mfxVideoParam &video, mfxI32 enableRecode = 1);

    mfxStatus Close() { return MFX_ERR_NONE; }

    mfxI32 GetQP(mfxU32 encodedOrder);

    mfxU32 Report(mfxU32 frameType, mfxU32 dataLength, mfxU32 userDataLength, mfxU32 repack, mfxU32 picOrder, mfxU32 maxFrameSize, mfxU32 qp);

    mfxStatus SetFrameVMEData(const mfxExtLAFrameStatistics *, mfxU32 widthMB, mfxU32 heightMB);

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


}//namespaceBase
}//namespace HEVCEHW
