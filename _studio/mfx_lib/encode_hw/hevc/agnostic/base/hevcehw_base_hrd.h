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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"

namespace HEVCEHW
{
namespace Base
{

class HRD
    : public FeatureBase
{
public:
#define DECL_BLOCK_LIST\
    DECL_BLOCK(Init) \
    DECL_BLOCK(Reset) \
    DECL_BLOCK(SubmitTask) \
    DECL_BLOCK(QueryTask)
#define DECL_FEATURE_NAME "Base_HRD"
#include "hevcehw_decl_blocks.h"

    HRD(mfxU32 id)
        : FeatureBase(id)
        , m_bIsHrdRequired(false)
        , m_cbrFlag(false)
        , m_bitrate(0)
        , m_maxCpbRemovalDelay(0)
        , m_clockTick(0)
        , m_cpbSize90k(0)
        , m_initCpbRemovalDelay(0)
        , m_prevAuCpbRemovalDelayMinus1(0)
        , m_prevAuCpbRemovalDelayMsb(0)
        , m_prevAuFinalArrivalTime(0)
        , m_prevBpAuNominalRemovalTime(0)
        , m_prevBpEncOrder(0)
        , m_prevBpEncOrderAsync(0)
    {}

protected:
    bool   m_bIsHrdRequired;
    bool   m_cbrFlag;
    mfxU32 m_bitrate;
    mfxU32 m_maxCpbRemovalDelay;
    mfxF64 m_clockTick;
    mfxF64 m_cpbSize90k;
    mfxF64 m_initCpbRemovalDelay;

    mfxI32 m_prevAuCpbRemovalDelayMinus1;
    mfxU32 m_prevAuCpbRemovalDelayMsb;
    mfxF64 m_prevAuFinalArrivalTime;
    mfxF64 m_prevBpAuNominalRemovalTime;
    mfxU32 m_prevBpEncOrder;
    mfxU32 m_prevBpEncOrderAsync;

    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;

    using FeatureBase::Init;
    void Init(const SPS &sps, mfxU32 InitialDelayInKB);
    using FeatureBase::Reset;
    void Reset(SPS const & sps);
    void Update(mfxU32 sizeInbits, mfxU32 encOrder, bool bufferingPeriodPic);
    mfxU32 GetInitCpbRemovalDelay(mfxU32 encOrder);

    mfxU32 GetInitCpbRemovalDelayOffset() const
    {
        return mfxU32(m_cpbSize90k - m_initCpbRemovalDelay);
    }
};

} //Base
} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
