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

#include "abr_brc.h"
#include <numeric>

inline mfxU8 QPclamp(mfxU8 qp) { return Clip3(mfxU8(1), mfxU8(51), qp); }

constexpr mfxF64 BRC3_RATE_RATIO_ACCURACY    = 0.001;

constexpr mfxF64 BRC3_QSTEP_COMPL_EXPONENT   = 0.4;
constexpr mfxF64 BRC3_BITRATE_QSTEP_EXPONENT = 1.0;

constexpr mfxF64 BRC3_INTRA_QSTEP_FACTOR = 1.4;
const     mfxF64 BRC3_INTRA_QP_DELTA     = 6.0 * std::log2(BRC3_INTRA_QSTEP_FACTOR);


constexpr mfxF64 BRC3_B_QSTEP_EXPONENT = 0.35;
constexpr mfxF64 BRC3_B_QP_FACTOR      = 6.0 * BRC3_B_QSTEP_EXPONENT;
constexpr mfxF64 BRC3_B_QP_DELTA_MIN   = 0.0;
constexpr mfxF64 BRC3_B_QP_DELTA_MAX   = 6.0;


constexpr mfxF64 BRC3_B_QSTEP_FACTOR   = 1.4;
const     mfxF64 BRC3_B_QP_DELTA       = 6.0 * std::log2(BRC3_B_QSTEP_FACTOR);

void LA_Stat_Queue::CalcScaledQsteps(mfxF64 scale)
{
    // First - Set the P frames at correct level
    for (mfxU32 i = m_curIndex; i < m_frameStatData.size(); ++i)
    {
        m_frameStatData[i].QstepCalculated = std::pow(m_frameStatData[i].ComplexitySmoothed, BRC3_QSTEP_COMPL_EXPONENT) / scale;
    }

    // Second - Set I and B frames to correct levels from P frames baseline
    mfxF64 norm = 0.0, logq = 0.0;
    mfxI32 lastrefPos = m_frameStatData.size();

    for (mfxI32 i = m_frameStatData.size() - 1; i >= mfxI32(m_curIndex); --i)
    {

        if (m_frameStatData[i].FrameType & MFX_FRAMETYPE_B)
            continue;

        mfxF64 qstep = m_frameStatData[i].QstepCalculated;

        if (m_frameStatData[i].FrameType & MFX_FRAMETYPE_I)
        {
            if (norm > 0.0)
            {
                mfxF64 qstep_propagated = std::exp(logq / norm) / BRC3_INTRA_QSTEP_FACTOR;

                if (norm >= 1.0)
                    qstep = qstep_propagated;
                else
                    qstep = qstep_propagated * norm + qstep * (1.0 - norm);
            }
            logq = 0.0;
            norm = 0.0;
        }
        else // P-frame
        {
            logq = (logq + std::log(qstep))*m_frameStatData[i].Propagation;
            norm = (norm + 1.0)*m_frameStatData[i].Propagation;
        }

        // Update qpstep of B-frames
        for (mfxI32 j = i + 1; j < lastrefPos; ++j) // if (i + 1 < lastrefPos) => there are B-frames
        {
            m_frameStatData[j].QstepCalculated = Clip3(qstep, qstep*2.0, qstep * std::pow(m_frameStatData[i].ComplexityOriginal / m_frameStatData[j].ComplexityOriginal, BRC3_B_QSTEP_EXPONENT));
        }


        lastrefPos = i;
        m_frameStatData[i].QstepCalculated = qstep;
    }
}

mfxF64 LA_Stat_Queue::GetBitrateAdjustmentRatio(mfxU32 frameType)
{
    return (frameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P)) ?
        (m_BitsPredictedAccumulatedIP ? mfxF64(m_BitsEncodedAccumulatedIP) / m_BitsPredictedAccumulatedIP : 1.0) :
        (m_BitsPredictedAccumulatedB  ? mfxF64(m_BitsEncodedAccumulatedB)  / m_BitsPredictedAccumulatedB  : 1.0);
}

void LA_Stat_Queue::CalcComplexities()
{
    mfxF64 average_Distortion       = m_DistortionAccumulated                                         / (m_frameStatData.size() - m_lbIndex),
           average_PixelPropagation = (m_PixelsWeightedPropagation_tmp + m_PixelsWeightedPropagation) / (m_frameStatData.size() - m_lbIndex);

    for (mfxU32 i = m_curIndex; i < m_frameStatData.size(); ++i)
    {
        m_frameStatData[i].ComplexityOriginal = m_frameStatData[i].QstepOriginal * m_frameStatData[i].FrameSize *                                 // Incorporate dependence between size and QpStep.
                                         std::pow(average_Distortion / (m_frameStatData[i].VisualDistortion + 1.0), BRC3_DIST_COMPL_EXPONENT) *   // Reflects relative subjective quality of frame.
                                         (m_frameStatData[i].NPixelsTransitivelyPropagated != 1.0 ?
                                                            m_frameStatData[i].NPixelsTransitivelyPropagated / average_PixelPropagation : 1.0);   // Relative propagation of current frame.
    }

    // Smooth complexities with Gaussian
    for (mfxU32 i = m_curIndex; i < m_frameStatData.size(); ++i)
    {
        mfxF64 wl = 1.0, wr = 1.0, wnorm;
        mfxF64 complexity = m_frameStatData[i].ComplexityOriginal;
        mfxF64 w = 1.0;

        // Right tail
        mfxU32 jLimit = std::min(BRC3_RATE_BLUR_LENGTH, mfxU32(m_frameStatData.size() - i));
        for (mfxU32 j = 1; j < jLimit; ++j)
        {
            wr *= m_frameStatData[i + j].Propagation;
            if (wr < BRC3_BLUR_STOP)
                break;
            wnorm = wr * m_normFactors[j - 1];
            complexity += m_frameStatData[i + j].ComplexityOriginal * wnorm;
            w += wnorm;
        }

        // Left tail
        jLimit = std::min(BRC3_RATE_BLUR_LENGTH, i + 1);
        for (mfxU32 j = 1; j < jLimit; ++j)
        {
            wl *= m_frameStatData[i - j + 1].Propagation;
            if (wl < BRC3_BLUR_STOP || (m_frameStatData[i - j].FrameType & MFX_FRAMETYPE_I))
                break;
            wnorm = wl * m_normFactors[j - 1];
            complexity += m_frameStatData[i - j].ComplexityOriginal * wnorm;
            w += wnorm;
        }

        m_frameStatData[i].ComplexitySmoothed = complexity / w;
    }
}

mfxF64 LA_Stat_Queue::CalcBitrate()
{
    for (mfxU32 i = m_curIndex; i < m_frameStatData.size(); ++i)
    {
        m_frameStatData[i].BitsPredicted = std::pow(m_frameStatData[i].QstepOriginal / m_frameStatData[i].QstepCalculated, BRC3_BITRATE_QSTEP_EXPONENT) * m_frameStatData[i].FrameSize;
    }

    return (std::accumulate(std::begin(m_frameStatData) + m_curIndex, std::end(m_frameStatData), 0.0,
                [](mfxF64 a, FrameStatData& b) { return a + b.BitsPredicted; }) + // Future frames, so use predicted size.
            m_BitsEncodedAccumulated)                                             // Already encoded frames, so use actual encoded size.
                / (m_frameStatData.size() - m_lbIndex);
}

void LA_BRC::UpdateStatData()
{
    // Precalculate complexities for frames in LA window
    m_frameStatQueue.CalcComplexities();

    mfxF64 scale, ratio;

    mfxF64 rS = Qp2QStep(51) / Qp2QStep(1); // Right bound of Scale parameter
    mfxF64 lS = 1.0 / rS;                   // Left  bound of Scale parameter

    // Left bound of possible bitrate for LA window
    m_frameStatQueue.CalcScaledQsteps(lS);                         // Set up Qsteps for left Scale
    mfxF64 lR = m_frameStatQueue.CalcBitrate() / m_TargetBitrate;  // Get left bound for ratio of Predicted / Target bitrate

    if (lR >= 1.0)
        return;

    m_frameStatQueue.CalcScaledQsteps(rS);                        // Set up Qsteps for right Scale
    mfxF64 rR = m_frameStatQueue.CalcBitrate() / m_TargetBitrate; // Get right bound for ratio of Predicted / Target bitrate

    if (rR <= 1.0)
        return;

    // Start iterative fork search to find optimal quantization steps
    for (mfxU32 i = 0; i < 30; ++i)
    {
        scale = lS + (1.0 - lR)*(rS - lS) / (rR - lR);

        m_frameStatQueue.CalcScaledQsteps(scale);

        ratio = m_frameStatQueue.CalcBitrate() / m_TargetBitrate;

        if (std::abs(ratio - 1.0) < BRC3_RATE_RATIO_ACCURACY)
            break;

        if (ratio < 1.0)
        {
            if (ratio <= lR)
                break;

            lR = ratio;
            lS = scale;
        }
        else
        {
            if (ratio >= rR)
                break;

            rR = ratio;
            rS = scale;
        }
    }

    return;
}

void LA_BRC::PreEnc(FrameStatData& /*statData*/)
{
    std::lock_guard<std::mutex> lock(queue_mutex);

    // Shift index of current frame
    m_frameStatQueue.StartNewFrameProcessing();

    // Precalculate optimal parameters
    UpdateStatData();

    FrameStatData& statData = m_frameStatQueue.GetCurrentStatData();

    // This ratio is used for correction of final bitrate on base of AdaptationDepth frames statistics
    mfxF64 corr_ratio = m_frameStatQueue.GetBitrateAdjustmentRatio(statData.FrameType);

    // Additional refinement for B frames
    if (statData.FrameType & MFX_FRAMETYPE_B)
    {
        mfxI32 poc = statData.POC;
        mfxF64 complexity = statData.ComplexitySmoothed;
        mfxI32 typeR = MFX_FRAMETYPE_UNKNOWN, typeL = MFX_FRAMETYPE_UNKNOWN, pocR = poc + 1, pocL = poc - 1;
        mfxF64 qp, qpR = 0.0, qpL = 0.0;
        mfxF64 complR = complexity, complL = complexity, complP = complexity;

        for (mfxI32 i = mfxI32(m_frameStatQueue.GetCurrentIndex()) - 1; i >= 0; --i)
        {
            if (!(m_frameStatQueue[i].FrameType & MFX_FRAMETYPE_B))
            {
                if (typeR == MFX_FRAMETYPE_UNKNOWN)
                {
                    qpR    = m_frameStatQueue[i].QP;
                    typeR  = m_frameStatQueue[i].FrameType;
                    pocR   = m_frameStatQueue[i].POC;
                    complR = m_frameStatQueue[i].ComplexityOriginal;
                }
                else
                {
                    qpL    = m_frameStatQueue[i].QP;
                    typeL  = m_frameStatQueue[i].FrameType;
                    pocL   = m_frameStatQueue[i].POC;
                    complL = m_frameStatQueue[i].ComplexityOriginal;
                    break;
                }
            }
        }

        if ((typeL & MFX_FRAMETYPE_P) && (typeR & MFX_FRAMETYPE_P))
        {
            mfxF64 crL = (complexity + 1.0) / (complL + 1.0),
                   crR = (complexity + 1.0) / (complR + 1.0);

            if (crL > 1.0)
                crL = 1.0 / crL;

            if (crR > 1.0)
                crR = 1.0 / crR;

            qp = (qpL * (pocR - poc) * crL + qpR * (poc - pocL) * crR) / ((pocR - poc)*crL + (poc - pocL)*crR);

            complP = (complL * crL * (pocR - poc) + complR * crR * (poc - pocL)) / ((pocR - poc)*crL + (poc - pocL)*crR);
        }
        else if (typeR & MFX_FRAMETYPE_P)
        {
            qp     = qpR;
            complP = complR;
        }
        else if (typeL & MFX_FRAMETYPE_P)
        {
            qp     = qpL;
            complP = complL;
        }
        else
        {
            qp = (qpL + qpR)*0.5 + BRC3_INTRA_QP_DELTA;
        }

        // qp_B_delta
        qp += Clip3(BRC3_B_QP_DELTA_MIN, BRC3_B_QP_DELTA_MAX, std::log2((complP / complexity)*corr_ratio) * BRC3_B_QP_FACTOR);

        m_curQp = QPclamp(mfxU8(qp + 0.5));

        statData.QstepCalculated = Qp2QStep(m_curQp);
        statData.QP              = m_curQp;
    }
    else
    {
        m_curQp = QPclamp(QStep2QpNearest(statData.QstepCalculated * corr_ratio));

        statData.QstepCalculated = Qp2QStep(m_curQp);
        statData.QP              = m_curQp;
    }
}

void LA_BRC::Report(mfxU32 dataLength)
{
    FrameStatData& statData = m_frameStatQueue.GetCurrentStatData();

    statData.BitsPredicted = statData.FrameSize * statData.QstepOriginal / statData.QstepCalculated;
    statData.BitsEncoded   = dataLength << 3;
}
