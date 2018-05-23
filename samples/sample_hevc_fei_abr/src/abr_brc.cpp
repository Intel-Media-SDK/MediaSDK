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

#if 0
namespace
{
    constexpr mfxF64 INTRA_QSTEP_COEFF     = 2.0;
    constexpr mfxF64 INTRA_MODE_BITCOST    = 0.0;
    constexpr mfxF64 INTER_MODE_BITCOST    = 0.0;
    constexpr mfxI32 MAX_QP_CHANGE         = 2;
    constexpr mfxF64 LOG2_64               = 3;
    constexpr mfxF64 MIN_EST_RATE          = 0.3;
    constexpr mfxF64 NORM_EST_RATE         = 100.0;

    constexpr mfxF64 MIN_RATE_COEFF_CHANGE = 0.5;
    constexpr mfxF64 MAX_RATE_COEFF_CHANGE = 2.0;
    constexpr mfxF64 INIT_RATE_COEFF[] = {
        1.109, 1.196, 1.225, 1.309, 1.369, 1.428, 1.490, 1.588, 1.627, 1.723, 1.800, 1.851, 1.916,
        2.043, 2.052, 2.140, 2.097, 2.096, 2.134, 2.221, 2.084, 2.153, 2.117, 2.014, 1.984, 2.006,
        1.801, 1.796, 1.682, 1.549, 1.485, 1.439, 1.248, 1.221, 1.133, 1.045, 0.990, 0.987, 0.895,
        0.921, 0.891, 0.887, 0.896, 0.925, 0.917, 0.942, 0.964, 0.997, 1.035, 1.098, 1.170, 1.275
    };

    mfxF64 const DEP_STRENTH = 2.0;

    mfxU8 GetSkippedQp(MbData const & mb)
    {
        if (mb.intraMbFlag)
            return 52; // never skipped
        if (abs(mb.mv[0].x - mb.costCenter0.x) >= 4 ||
            abs(mb.mv[0].y - mb.costCenter0.y) >= 4 ||
            abs(mb.mv[1].x - mb.costCenter1.x) >= 4 ||
            abs(mb.mv[1].y - mb.costCenter1.y) >= 4)
            return 52; // never skipped

        mfxU16 const * sumc = mb.lumaCoeffSum;
        mfxU8  const * nzc = mb.lumaCoeffCnt;

        if (nzc[0] + nzc[1] + nzc[2] + nzc[3] == 0)
            return 0; // skipped at any qp

        mfxF64 qoff = 1.0 / 6;
        mfxF64 norm = 0.1666;

        mfxF64 qskip = IPP_MAX(IPP_MAX(IPP_MAX(
            nzc[0] ? (sumc[0] * norm / nzc[0]) / (1 - qoff) * LOG2_64 : 0,
            nzc[1] ? (sumc[1] * norm / nzc[1]) / (1 - qoff) * LOG2_64 : 0),
            nzc[2] ? (sumc[2] * norm / nzc[2]) / (1 - qoff) * LOG2_64 : 0),
            nzc[3] ? (sumc[3] * norm / nzc[3]) / (1 - qoff) * LOG2_64 : 0);

        return QStep2QpCeil(qskip);
    }
}
#endif

mfxU8 QPclamp(mfxU8 qp) { return Clip3(mfxU8(1), mfxU8(51), qp); }


mfxF64 modifyComplexities(std::vector<FrameStatData> &statData, mfxF64 avMSE, mfxU32 start_index = 0, mfxU32 LookAheadDepth = 100)
{
    mfxU32 last_index_plus1 = std::min(mfxU32(statData.size()), start_index + LookAheadDepth);

    for (mfxU32 i = start_index; i < last_index_plus1; ++i)
    {
        statData[i].complexity0 = statData[i].qstep0 * statData[i].frame_size * std::pow(avMSE / (statData[i].mseY + 1.0), BRC3_DIST_COMPL_EXPONENT);
    }

    return std::accumulate(statData.begin() + start_index, statData.begin() + last_index_plus1, 0.0,
        [](mfxF64 a, FrameStatData& b) { return a + b.complexity0; }) / (last_index_plus1 - start_index);
}


mfxF64 averageComplexities(std::vector<FrameStatData> &statData, mfxU32 start_index = 0, mfxU32 LookAheadDepth = 100)
{
    mfxF64 m_normFactors[BRC3_RATE_BLUR_LENGTH - 1];

    for (mfxU32 j = 1; j < BRC3_RATE_BLUR_LENGTH; ++j)
    {
        m_normFactors[j - 1] = exp(-(mfxF64)j*j / BRC3_GAUSS_VARIANCE);
    }

    mfxU32 last_index_plus1 = std::min(mfxU32(statData.size()), start_index + LookAheadDepth);

    for (size_t i = start_index; i < last_index_plus1; i++)
    {
        mfxF64 wl = 1.0, wr = 1.0, wnorm;
        mfxF64 complexity = statData[i].complexity0;
        mfxF64 w = 1.0;

        // Right tail
        size_t jLimit = std::min(size_t(BRC3_RATE_BLUR_LENGTH), statData.size() - i);
        for (size_t j = 1; j < jLimit; j++)
        {
            //wr *= 1. - pow(statData[i+j].shareI, BRC3_RATE_BLUR_EXPONENT);
            wr *= statData[i + j].propagation;
            if (wr < BRC3_BLUR_STOP)
                break;
            wnorm = wr * m_normFactors[j - 1];
            complexity += statData[i + j].complexity0 * wnorm;
            w += wnorm;
        }

        // Left tail
        jLimit = std::min(size_t(BRC3_RATE_BLUR_LENGTH), i + 1);
        for (size_t j = 1; j < jLimit; j++)
        {
            //wl *= 1. - pow(statData[i-j+1].shareI, BRC3_RATE_BLUR_EXPONENT);
            wl *= statData[i - j + 1].propagation;
            if (wl < BRC3_BLUR_STOP || (statData[i - j].frame_type & MFX_FRAMETYPE_I))
                break;
            wnorm = wl * m_normFactors[j - 1];
            complexity += statData[i - j].complexity0 * wnorm;
            w += wnorm;
        }


        statData[i].complexity = complexity / w;
    }

    return std::accumulate(statData.begin() + start_index, statData.begin() + last_index_plus1, 0.0,
        [](mfxF64 a, FrameStatData& b) { return a + b.complexity; }) / (last_index_plus1 - start_index);
}

mfxF64 calcAverageMSE(std::vector<FrameStatData> &statData, mfxU32 start_index = 0, mfxU32 LookAheadDepth = 100)
{
    /*
    for (size_t i = start_index; i < statData.size(); ++i)
    {
        statData[i].mseY = 255 * 255 / std::pow(10.0, statData[i].psnrY * 0.1);
    } */

    mfxU32 last_index_plus1 = std::min(mfxU32(statData.size()), start_index + LookAheadDepth);

    return std::accumulate(statData.begin() + start_index, statData.begin() + last_index_plus1, 0.0,
        [](mfxF64 a, FrameStatData& b) { return a + b.mseY; }) / (last_index_plus1 - start_index);
}

constexpr mfxF64 BRC3_RATE_RATIO_ACCURACY    = 0.001;

constexpr mfxF64 BRC3_QSTEP_COMPL_EXPONENT   = 0.4;
constexpr mfxF64 BRC3_BITRATE_QSTEP_EXPONENT = 1.0;

constexpr mfxF64 BRC3_INTRA_QSTEP_FACTOR = 1.4;
constexpr mfxF64 BRC3_INTRA_QP_DELTA     = 6.0 * std::log2(BRC3_INTRA_QSTEP_FACTOR);


constexpr mfxF64 BRC3_B_QSTEP_EXPONENT = 0.35;
constexpr mfxF64 BRC3_B_QP_FACTOR      = 6.0 * BRC3_B_QSTEP_EXPONENT;
constexpr mfxF64 BRC3_B_QP_DELTA_MIN   = 0.0;
constexpr mfxF64 BRC3_B_QP_DELTA_MAX   = 6.0;


constexpr mfxF64 BRC3_B_QSTEP_FACTOR   = 1.4;
constexpr mfxF64 BRC3_B_QP_DELTA       = 6.0 * std::log2(BRC3_B_QSTEP_FACTOR);


void calcQsteps(std::vector<FrameStatData> &statData, mfxF64 scale, mfxU32 start_index = 0, mfxU32 LookAheadDepth = 100)
{
    mfxU32 last_index_plus1 = std::min(mfxU32(statData.size()), start_index + LookAheadDepth);

    for (mfxU32 i = start_index; i < last_index_plus1; ++i)
    {
#if 0
        statData[i].qstep = std::max(std::pow(statData[i].complexity0, BRC3_QSTEP_COMPL_EXPONENT) / scale, statData[i].qstep0);
#else
#if 1
        statData[i].qstep = std::pow(statData[i].complexity, BRC3_QSTEP_COMPL_EXPONENT) / scale;
#else
        statData[i].qstep = std::max(std::pow(statData[i].complexity,  BRC3_QSTEP_COMPL_EXPONENT) / scale, statData[i].qstep0);
#endif
#endif
    }
}


void calcQsteps_IB(std::vector<FrameStatData> &statData, mfxU32 start_index = 0)
{
    mfxF64 norm = 0.0;
    mfxF64 logq = 0.0;
    //    mfxF64 refqstep = statData[statData.size() - 1].qstep; // tmp kta
    mfxI32 lastrefPos = mfxI32(statData.size());

    for (mfxI32 i = mfxI32(statData.size()) - 1; i >= mfxI32(start_index); --i)
    {

        if (statData[i].frame_type & MFX_FRAMETYPE_B)
            continue;

        mfxF64 qstep = statData[i].qstep;

        if (statData[i].frame_type & MFX_FRAMETYPE_I)
        {
            if (norm > 0.0)
            {
                // mfxF64 qstep_propagated = pow(q, 1./norm) / BRC3_INTRA_QSTEP_FACTOR;
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
            logq = (logq + std::log(qstep))*statData[i].propagation;
            norm = (norm + 1.0)*statData[i].propagation;
        }

        // Update qpstep of B-frames
        if (i + 1 < lastrefPos) // there are B-frames
        {
            for (mfxI32 j = i + 1; j < lastrefPos; ++j)
            {
                statData[j].qstep = Clip3(qstep, qstep*2.0, qstep * std::pow(statData[i].complexity0 / statData[j].complexity0, BRC3_B_QSTEP_EXPONENT));
            }
        }

        //if (statData[i].frame_type & MFX_FRAMETYPE_B) {
        //    qstep = refqstep * BRC_B_QSTEP_FACTOR;
        //} else
        //    refqstep = qstep;

        lastrefPos = i;
        statData[i].qstep = qstep;
    }

    return;
}


mfxF64 calcBitrate(std::vector<FrameStatData> &statData, mfxU32 start_index = 0, mfxU32 LookAheadDepth = 100)
{
    mfxU32 last_index_plus1 = std::min(mfxU32(statData.size()), start_index + LookAheadDepth);

    for (mfxU32 i = start_index; i < last_index_plus1; ++i)
    {
        statData[i].predicted_size = std::pow(statData[i].qstep0 / statData[i].qstep, BRC3_BITRATE_QSTEP_EXPONENT) * statData[i].frame_size;
    }

    return std::accumulate(statData.begin() + start_index, statData.begin() + last_index_plus1, 0.0,
        [](mfxF64 a, FrameStatData& b) { return a + b.predicted_size; }) / (last_index_plus1 - start_index);
}


void LA_BRC::UpdateStatData(mfxU32 curFrameIndex)
{
    m_avMSE = calcAverageMSE(m_frameStatData, curFrameIndex);

    modifyComplexities(m_frameStatData, m_avMSE, curFrameIndex);

    averageComplexities(m_frameStatData, curFrameIndex);

    mfxF64 scale = 1.0;
    mfxF64 ratio = 999.9; // just to suppress a stupid warning
    mfxF64 predictedBitrate;
    mfxF64 lR, rR;
    mfxF64 lS, rS;
    mfxF64 ratio_accuracy = BRC3_RATE_RATIO_ACCURACY;

#if 0
    // no HRD for now
    if (video.mfx.BufferSizeInKB > 0)
    {
        ratio_accuracy = std::min(ratio_accuracy, video.mfx.BufferSizeInKB * 1600.0 / (m_TargetBitrate * m_frameStatData.size())); // video.mfx.BufferSizeInKB * 8000.0 * 0.2
    }
#endif

    rS = Qp2QStep(51) / Qp2QStep(1);
    lS = 1.0 / rS;

    calcQsteps(m_frameStatData, lS, curFrameIndex);
    lR = calcBitrate(m_frameStatData, curFrameIndex) / m_TargetBitrate;

    if (lR >= 1.0)
        return;

    calcQsteps(m_frameStatData, rS, curFrameIndex);
    rR = calcBitrate(m_frameStatData, curFrameIndex) / m_TargetBitrate;

    if (rR <= 1.0)
        return;

    for (mfxU32 i = 0; i < 30; ++i)
    {
        scale = lS + (1.0 - lR)*(rS - lS) / (rR - lR);

        calcQsteps(m_frameStatData, scale, curFrameIndex);

        calcQsteps_IB(m_frameStatData, curFrameIndex);

        predictedBitrate = calcBitrate(m_frameStatData, curFrameIndex);

        ratio = predictedBitrate / m_TargetBitrate;

        if (std::abs(ratio - 1.0) < ratio_accuracy)
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

constexpr mfxF64 BRC3_QSTEP_CORRECTION_EXPONENT = 1.0;

void LA_BRC::PreEnc(HevcTaskDSO& task)
{
#ifdef DEBUG_OUTPUT
    std::cout << std::endl << std::endl << "Frame #" << task.m_statData.DisplayOrder << std::endl;
    std::cout << "initial qp:   " << mfxI32(task.m_statData.qp);
#endif

    mfxU32 curFrameIndex = std::distance(m_frameStatData.begin(), std::find_if(m_frameStatData.begin(), m_frameStatData.end(),
        [&task](FrameStatData& dat) { return dat.EncodedOrder == task.m_statData.EncodedOrder; }));

    if (curFrameIndex == m_frameStatData.size())
        throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Current stat wasn't found in LookAhead cache");

    FrameStatData& statData = m_frameStatData[curFrameIndex];

    UpdateStatData(curFrameIndex);

    mfxF64 qstep;

#ifdef SEPARATE_IP_B_PROCESSING
    if (statData.frame_type & MFX_FRAMETYPE_B)
    {
        mfxI32 poc = statData.poc;
        mfxF64 complexity = statData.complexity0;
        mfxI32 typeR = MFX_FRAMETYPE_UNKNOWN, typeL = MFX_FRAMETYPE_UNKNOWN, pocR = poc + 1, pocL = poc - 1;
        mfxF64 qp, qpR = 0.0, qpL = 0.0;
        mfxF64 complR = complexity, complL = complexity, complP = complexity;

        for (mfxI32 i = mfxI32(curFrameIndex) - 1; i >= 0; --i)
        {
            if (!(m_frameStatData[i].frame_type & MFX_FRAMETYPE_B))
            {
                if (typeR == MFX_FRAMETYPE_UNKNOWN)
                {
                    qpR    = m_frameStatData[i].qp;
                    typeR  = m_frameStatData[i].frame_type;
                    pocR   = m_frameStatData[i].poc;
                    complR = m_frameStatData[i].complexity0;
                }
                else
                {
                    qpL    = m_frameStatData[i].qp;
                    typeL  = m_frameStatData[i].frame_type;
                    pocL   = m_frameStatData[i].poc;
                    complL = m_frameStatData[i].complexity0;
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

            //            qp = (qpL * (pocR - poc) + qpR * (poc - pocL)) / (pocR - pocL);
            //            complP = (complL * (pocR - poc) + complR * (poc - pocL)) / (pocR - pocL);
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
#if 1
        // qp_B_delta
        qp += Clip3(BRC3_B_QP_DELTA_MIN, BRC3_B_QP_DELTA_MAX, std::log2((complP / complexity)*((m_B_BitsEncoded + 1.0) / (m_B_BitsPredicted + 1.0))) * BRC3_B_QP_FACTOR);
#else
        qp += BRC3_B_QP_DELTA;
#endif

        m_curQp = QPclamp(mfxU8(qp + 0.5));

        statData.qstep = Qp2QStep(m_curQp);
        statData.qp    = m_curQp;
    }
    else
#endif
    {
        qstep = statData.qstep;

        mfxF64 expnnt = BRC3_QSTEP_CORRECTION_EXPONENT;

        if (m_curFrame < m_LookAheadDepth)
            expnnt *= 0.01 * m_curFrame;

#ifdef SEPARATE_IP_B_PROCESSING
        mfxF64 qstep_n = qstep * std::pow((m_IP_BitsEncoded + 1.0) / (m_IP_BitsPredicted + 1.0), expnnt);
#else
        mfxF64 qstep_n = qstep * std::pow((m_BitsEncoded + 1.0) / (m_BitsPredicted + 1.0), expnnt);
#endif

        m_curQp = QPclamp(QStep2QpNearest(qstep_n));

        statData.qstep = qstep_n;
        statData.qp    = m_curQp;
    }

#ifdef DEBUG_OUTPUT
    std::cout << "    qp set:    " << mfxI32(m_curQp) << std::endl;
    std::cout << "initial size: " << task.m_statData.frame_size << " result size: ";
#endif
}

void LA_BRC::Report(mfxU32 dataLength)
{
#ifdef DEBUG_OUTPUT
    std::cout << dataLength << std::endl;
#endif

#ifdef SEPARATE_IP_B_PROCESSING
    switch (m_frameStatData[m_curFrame].frame_type & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B))
    {
    case MFX_FRAMETYPE_I:
    case MFX_FRAMETYPE_P:
        m_IP_BitsPredicted += m_frameStatData[m_curFrame].predicted_size;
        m_IP_BitsEncoded   += dataLength << 3;
        break;
    case MFX_FRAMETYPE_B:
        m_B_BitsPredicted += m_frameStatData[m_curFrame].predicted_size;
        m_B_BitsEncoded   += dataLength << 3;
        break;

    default:
        throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Invalid m_frameStatData[m_curFrame].frame_type");
        break;
    }
#else
    m_BitsPredicted += m_frameStatData[m_curFrame].predicted_size;
    m_BitsEncoded   += dataLength << 3;
#endif

    ++m_curFrame;

    return;
}
