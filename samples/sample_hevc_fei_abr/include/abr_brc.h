/******************************************************************************\
Copyright (c) 2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/
#ifndef __ABR_BRC_H__
#define __ABR_BRC_H__

#include "task.h"
#include "brc_routines.h"

#include <algorithm>
#include <mutex>

namespace
{
    // LA BRC auxiliary functions
    constexpr mfxU32 BRC3_RATE_BLUR_LENGTH    = 10;
    constexpr mfxF64 BRC3_GAUSS_VARIANCE      = 160.0;
    constexpr mfxF64 BRC3_BLUR_STOP           = 0.001;
    constexpr mfxF64 BRC3_DIST_COMPL_EXPONENT = 0.5;

    constexpr mfxF64 BRC3_PROPAGATION_EXPONENT = 2.0;

    constexpr mfxF64 QSTEP[52] = {
        0.630,  0.707,  0.794,  0.891,  1.000,  1.122,   1.260,   1.414,   1.587,   1.782,   2.000,   2.245,   2.520,
        2.828,  3.175,  3.564,  4.000,  4.490,  5.040,   5.657,   6.350,   7.127,   8.000,   8.980,   10.079,  11.314,
        12.699, 14.254, 16.000, 17.959, 20.159, 22.627,  25.398,  28.509,  32.000,  35.919,  40.317,  45.255,  50.797,
        57.018, 64.000, 71.838, 80.635, 90.510, 101.594, 114.035, 128.000, 143.675, 161.270, 181.019, 203.187, 228.070
    };

    inline mfxU8 QStep2QpFloor(mfxF64 qstep) // QSTEP[qp] <= qstep, return 0<=qp<=51
    {
        mfxU8 qp = mfxU8(std::upper_bound(QSTEP, QSTEP + 52, qstep) - QSTEP);
        return qp > 0 ? qp - 1 : 0;
    }

    inline mfxU8 QStep2QpNearest(mfxF64 qstep) // return 0<=qp<=51
    {
        mfxU8 qp = QStep2QpFloor(qstep);
        return (qp == 51 || qstep < (QSTEP[qp] + QSTEP[qp + 1]) / 2) ? qp : qp + 1;
    }

    inline mfxF64 Qp2QStep(mfxU8 qp)
    {
        return QSTEP[std::min(mfxU8(51), qp)];
    }
}

// BRC interface
class BRC
{
public:
    BRC()                      = default;
    BRC(BRC const&)            = delete;
    BRC(BRC &&)                = default;
    BRC& operator=(BRC const&) = delete;
    BRC& operator=(BRC &&)     = default;

    virtual ~BRC() {}

    virtual void PreEnc(FrameStatData& statData) = 0;

    virtual void Report(mfxU32 dataLength) = 0;

    virtual mfxU8 GetQP() = 0;

    virtual void SubmitNewStat(FrameStatData& stat) = 0;
};

/*
    This is not canonical way of using SW BRC, in sample_encode correct approach is implemented.
    Code below gives for better understanding how actually SW BRC interface works and shows a way
    how to use BRC without letting MSDK lib know it.
*/
class SW_BRC : public BRC
{
public:
    SW_BRC(mfxVideoParam const & video, mfxU16 TargetKbps)
    {
        Zero(m_BRCPar);
        Zero(m_BRCCtrl);

        //prepare mfxVideoParam for BRC Init
        mfxVideoParam tmp = video;
        tmp.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        tmp.mfx.TargetKbps        = TargetKbps;

        mfxExtCodingOption CO;
        Zero(CO);

        CO.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
        CO.Header.BufferSz = sizeof(mfxExtCodingOption);
        CO.NalHrdConformance = MFX_CODINGOPTION_OFF;

        mfxExtBuffer *ext = &CO.Header;
        tmp.ExtParam    = &ext;
        tmp.NumExtParam = 1;

        mfxStatus sts = m_SW_BRC.Init(&tmp);
        if (MFX_ERR_NONE != sts)
            throw mfxError(MFX_ERR_NOT_INITIALIZED, "m_SW_BRC.Init failed");
    }

    SW_BRC(SW_BRC const&)            = delete;
    SW_BRC(SW_BRC &&)                = default;
    SW_BRC& operator=(SW_BRC const&) = delete;
    SW_BRC& operator=(SW_BRC &&)     = default;

    ~SW_BRC() {}

    virtual void PreEnc(FrameStatData& statData) override
    {
        Zero(m_BRCPar);
        Zero(m_BRCCtrl);

        m_BRCPar.EncodedOrder = statData.EncodedOrder;
        m_BRCPar.DisplayOrder = statData.DisplayOrder;
        m_BRCPar.FrameType    = statData.FrameType;

        mfxStatus sts = m_SW_BRC.GetFrameCtrl(&m_BRCPar, &m_BRCCtrl);
        if (MFX_ERR_NONE != sts)
            throw mfxError(sts, "m_SW_BRC.GetFrameCtrl failed");

        m_curQp = m_BRCCtrl.QpY;
    }

    virtual void Report(mfxU32 dataLength) override
    {
        m_BRCPar.CodedFrameSize = dataLength;

        mfxBRCFrameStatus bsts;
        mfxStatus sts = m_SW_BRC.Update(&m_BRCPar, &m_BRCCtrl, &bsts);
        if (MFX_ERR_NONE != sts)
            throw mfxError(sts, "m_SW_BRC.Update failed");

        if (bsts.BRCStatus != MFX_BRC_OK)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "m_SW_BRC.Update failed. Re encode is not supported");
    }

    virtual mfxU8 GetQP() override
    {
        return m_curQp;
    }

    void SubmitNewStat(FrameStatData& /*stat*/) override { return; }

private:
    ExtBRC m_SW_BRC;
    mfxU8  m_curQp = 0xff;

    mfxBRCFrameParam m_BRCPar;
    mfxBRCFrameCtrl  m_BRCCtrl;
};

class LA_Stat_Queue
{
public:
    LA_Stat_Queue(mfxU16 la_depth = 100, mfxU16 lb_depth = 100, mfxU16 adpt_length = 100)
        : m_LookAheadDepth(la_depth)
        , m_LookBackDepth(lb_depth)
        , m_AdaptationLength(adpt_length)
        , m_SizeLimit(1 + std::max(m_LookBackDepth, m_AdaptationLength) + m_LookAheadDepth)
    {
        for (mfxU32 j = 1; j < BRC3_RATE_BLUR_LENGTH; ++j)
        {
            m_normFactors[j - 1] = std::exp(-mfxF64(j*j) / BRC3_GAUSS_VARIANCE);
        }
    }

    void Add(FrameStatData& stat)
    {
        m_frameStatData.emplace_back(std::move(stat));

        // Update pixel counters for reference frames
        for (auto fo_amount : m_frameStatData.back().NumPixelsPredictedFromRef)
        {
            auto it = std::find_if(std::begin(m_frameStatData), std::end(m_frameStatData),
                        [fo_amount](FrameStatData& dat) { return dat.DisplayOrder == fo_amount.first; });

            if (it != std::end(m_frameStatData))
            {
                it->NPixelsPropagated += fo_amount.second;
            }
        }

        if (m_frameStatData.size() > m_SizeLimit)
        {
            RemoveLast();
        }

        m_DistortionAccumulated += m_frameStatData.back().VisualDistortion;
    }

    void StartNewFrameProcessing()
    {
        ++m_curIndex;

        // Store old indexes to detect window bound change
        mfxU16 lb_old = m_lbIndex, al_old = m_alIndex;

        m_lbIndex = std::max(0, mfxI32(m_curIndex) - mfxI32(m_LookBackDepth));
        m_alIndex = std::max(0, mfxI32(m_curIndex) - mfxI32(m_AdaptationLength));

        // Calculate transitive propagation of current frame pixels

        // How many pixels were directly propagated to another frames (+1.0 to guarantee positiveness)
        m_frameStatData[m_curIndex].NPixelsTransitivelyPropagated = 1.0 + mfxF64(m_frameStatData[m_curIndex].NPixelsPropagated) / m_frameStatData[m_curIndex].NPixelsInFrame;

        // Go through future frames and incorporate indirect pixels influence by adding weighted sum which is:
        // share_of_directly_predicted_pixels_from_frame__m_frameStatData[m_curIndex] * total_propagated_pixels_of_frame__m_frameStatData[i] / amount_of_pixels_in_frame
        for (mfxU32 i = m_curIndex + 1; i < m_frameStatData.size(); ++i)
        {
            if (m_frameStatData[i].Contains(m_frameStatData[m_curIndex].DisplayOrder))
            {
                m_frameStatData[m_curIndex].NPixelsTransitivelyPropagated +=
                    (m_frameStatData[i].ShareOfPredictedPixelsFromRef[m_frameStatData[m_curIndex].DisplayOrder] * m_frameStatData[i].NPixelsPropagated) / m_frameStatData[i].NPixelsInFrame;
            }
        }

        // Precalculate temporary values of NPixelsTransitivelyPropagated. We can't guarantee correct values for some of them near the end of LA window but it doesn't harm the algorithm much
        m_PixelsWeightedPropagation_tmp = 0.0;
        for (mfxU32 i = m_curIndex + 1; i < m_frameStatData.size(); ++i)
        {
            m_frameStatData[i].NPixelsTransitivelyPropagated = 1.0 + mfxF64(m_frameStatData[i].NPixelsPropagated) / m_frameStatData[i].NPixelsInFrame;

            for (mfxU32 j = i + 1; j < m_frameStatData.size(); ++j)
            {
                if (m_frameStatData[j].Contains(m_frameStatData[i].DisplayOrder))
                {
                    m_frameStatData[i].NPixelsTransitivelyPropagated +=
                        (m_frameStatData[j].ShareOfPredictedPixelsFromRef[m_frameStatData[i].DisplayOrder] * m_frameStatData[j].NPixelsPropagated) / m_frameStatData[j].NPixelsInFrame;
                }
            }
            m_PixelsWeightedPropagation_tmp += m_frameStatData[i].NPixelsTransitivelyPropagated;
        }

        m_PixelsWeightedPropagation += m_frameStatData[m_curIndex].NPixelsTransitivelyPropagated;

        if (m_curIndex)
        {
            // Update counters if window is shifted - remove frame which falls out of window
            if (lb_old != m_lbIndex)
            {
                m_DistortionAccumulated     -= m_frameStatData[lb_old].VisualDistortion;
                m_PixelsWeightedPropagation -= m_frameStatData[lb_old].NPixelsTransitivelyPropagated;
                m_BitsEncodedAccumulated    -= m_frameStatData[lb_old].BitsEncoded;
            }

            if (al_old != m_alIndex)
            {
                if (m_frameStatData[al_old].FrameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P))
                {
                    m_BitsPredictedAccumulatedIP -= m_frameStatData[al_old].BitsPredicted;
                    m_BitsEncodedAccumulatedIP   -= m_frameStatData[al_old].BitsEncoded;
                }
                else
                {
                    m_BitsPredictedAccumulatedB -= m_frameStatData[al_old].BitsPredicted;
                    m_BitsEncodedAccumulatedB   -= m_frameStatData[al_old].BitsEncoded;
                }
            }

            m_BitsEncodedAccumulated += m_frameStatData[m_curIndex - 1].BitsEncoded;

            // Update counter with size of recently encoded frame
            if (m_frameStatData[m_curIndex - 1].FrameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P))
            {
                m_BitsPredictedAccumulatedIP += m_frameStatData[m_curIndex - 1].BitsPredicted;
                m_BitsEncodedAccumulatedIP   += m_frameStatData[m_curIndex - 1].BitsEncoded;
            }
            else
            {
                m_BitsPredictedAccumulatedB  += m_frameStatData[m_curIndex - 1].BitsPredicted;
                m_BitsEncodedAccumulatedB    += m_frameStatData[m_curIndex - 1].BitsEncoded;
            }
        }
    }

    FrameStatData& GetCurrentStatData()
    {
        return m_frameStatData[m_curIndex];
    }

    mfxU32 GetCurrentIndex()
    {
        return m_curIndex;
    }

    FrameStatData& operator[] (size_t i)
    {
        return m_frameStatData[i];
    }

    void CalcScaledQsteps(mfxF64 scale);

    mfxF64 GetBitrateAdjustmentRatio(mfxU32 frameType);

    void CalcComplexities();

    mfxF64 CalcBitrate();

private:

    void RemoveLast()
    {
        if (m_lbIndex <= m_alIndex)
        {
            // Update accumulated values
            m_DistortionAccumulated     -= m_frameStatData.front().VisualDistortion;
            m_PixelsWeightedPropagation -= m_frameStatData.front().NPixelsTransitivelyPropagated;
            m_BitsEncodedAccumulated    -= m_frameStatData.front().BitsEncoded;
        }

        if (m_alIndex <= m_lbIndex)
        {
            // Update accumulators for Predicted / Encoded bits
            if (m_frameStatData.front().FrameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P))
            {
                m_BitsPredictedAccumulatedIP -= m_frameStatData.front().BitsPredicted;
                m_BitsEncodedAccumulatedIP   -= m_frameStatData.front().BitsEncoded;
            }
            else
            {
                m_BitsPredictedAccumulatedB  -= m_frameStatData.front().BitsPredicted;
                m_BitsEncodedAccumulatedB    -= m_frameStatData.front().BitsEncoded;
            }
        }

        m_frameStatData.pop_front();
        --m_curIndex;

        // Update LookBack and Adaptation regions starting indexes
        m_lbIndex = std::max(0, mfxI32(m_curIndex) - mfxI32(m_LookBackDepth));
        m_alIndex = std::max(0, mfxI32(m_curIndex) - mfxI32(m_AdaptationLength));
    }

    std::deque<FrameStatData> m_frameStatData;

    mfxU16 m_LookAheadDepth   = 100; // Frames to look ahead
    mfxU16 m_LookBackDepth    = 100; // Frames to take into account from past (use already fixed statistics)
    mfxU16 m_AdaptationLength = 100; // Frames from past to calculate adaptation ratio

    mfxU16 m_SizeLimit        = 0;
    mfxI16 m_curIndex         = -1;// Current frame index
    mfxU16 m_lbIndex          = 0; // Starting index for LookBack   region
    mfxU16 m_alIndex          = 0; // Starting index for Adaptation region

    mfxF64 m_normFactors[BRC3_RATE_BLUR_LENGTH - 1]; // Half of Gaussian blurring kernel

    // Bit counters for bitrate adjustment which use Adjustment window
    mfxU64 m_BitsPredictedAccumulatedIP = 0;
    mfxU64 m_BitsEncodedAccumulatedIP   = 0;
    mfxU64 m_BitsPredictedAccumulatedB  = 0;
    mfxU64 m_BitsEncodedAccumulatedB    = 0;

    // Bit counter for bitrate calculation which use LookBack window
    mfxU64 m_BitsEncodedAccumulated     = 0;

    // Accumulator for Distortion
    mfxF64 m_DistortionAccumulated      = 0.0;

    // Frames which propagates more pixels should be encoded with better quality, because for such frames quality changes propagates more
    mfxF64 m_PixelsWeightedPropagation     = 0.0; // Accumulated pixel propagation
    mfxF64 m_PixelsWeightedPropagation_tmp = 0.0; // Accumulated pixel propagation for future frames (contains not final values)
};

class LA_BRC : public BRC
{
public:
    LA_BRC(mfxVideoParam const & video, mfxU16 TargetKbps, mfxU16 la_depth = 100, mfxU16 lb_depth = 100, mfxU16 adpt_length = 100)
        : m_TargetBitrate(mfxF64(1000 * TargetKbps * video.mfx.FrameInfo.FrameRateExtD) / video.mfx.FrameInfo.FrameRateExtN)
        , m_frameStatQueue(la_depth, lb_depth, adpt_length)
    {
    }

    ~LA_BRC() {}

    virtual void PreEnc(FrameStatData& /*statData*/) override;

    virtual void Report(mfxU32 dataLength) override;

    virtual mfxU8 GetQP() override
    {
        return m_curQp;
    }

    virtual void SubmitNewStat(FrameStatData& stat) override
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        // Fill some additional information
        stat.Propagation        = 1.0 - std::pow(stat.ShareIntra, BRC3_PROPAGATION_EXPONENT);
        stat.QstepOriginal      = Qp2QStep(stat.QP);
        stat.ComplexityOriginal = stat.QstepOriginal*stat.FrameSize;

        m_frameStatQueue.Add(stat);
    }

private:
    mfxU8  m_curQp         = 0xff;
    mfxF64 m_TargetBitrate = 0.0; // bits per frame

    LA_Stat_Queue m_frameStatQueue;

    // To protect LA queue from pushing / pulling from different threads
    std::mutex queue_mutex;

    // Update internal state with new frame
    void UpdateStatData();
};

#endif // __ABR_BRC_H__
