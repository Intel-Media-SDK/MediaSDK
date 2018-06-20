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

#include <cmath>
#include <algorithm>
#include <vector>

#ifdef DEBUG_OUTPUT
#include<cstdio>
#endif

namespace
{
    // Magic part
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

#if 0
    mfxU8 QStep2QpCeil(mfxF64 qstep) // QSTEP[qp] > qstep, may return qp=52
    {
        return mfxU8(std::lower_bound(QSTEP, QSTEP + 52, qstep) - QSTEP);
    }
#endif

    mfxU8 QStep2QpFloor(mfxF64 qstep) // QSTEP[qp] <= qstep, return 0<=qp<=51
    {
        mfxU8 qp = mfxU8(std::upper_bound(QSTEP, QSTEP + 52, qstep) - QSTEP);
        return qp > 0 ? qp - 1 : 0;
    }

    mfxU8 QStep2QpNearest(mfxF64 qstep) // return 0<=qp<=51
    {
        mfxU8 qp = QStep2QpFloor(qstep);
        return (qp == 51 || qstep < (QSTEP[qp] + QSTEP[qp + 1]) / 2) ? qp : qp + 1;
    }

    mfxF64 Qp2QStep(mfxU8 qp)
    {
        return QSTEP[std::min(mfxU8(51), qp)];
    }

#if 0
    mfxF64 fQP2Qstep(mfxF64 fqp)
    {
        return std::pow(2.0, (fqp - 4.0) / 6.0);
    }
#endif

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

    virtual void PreEnc(HevcTaskDSO& task /*FrameStatData& statData*/) = 0;

    virtual void Report(mfxU32 dataLength) = 0;

    virtual mfxU8 GetQP() = 0;

    virtual void SubmitNewStat(FrameStatData& stat) = 0;
};

class SW_BRC : public BRC
{
public:
    SW_BRC(mfxVideoParam const & video, mfxU16 TargetKbps)
    {
        //prepare mfxVideoParam for BRC
        mfxVideoParam tmp = video;
        tmp.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        tmp.mfx.TargetKbps        = TargetKbps;

        mfxExtCodingOption CO;
        MSDK_ZERO_MEMORY(CO);

        CO.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
        CO.Header.BufferSz = sizeof(mfxExtCodingOption);
        CO.NalHrdConformance = MFX_CODINGOPTION_OFF;

        mfxExtBuffer *ext = &CO.Header;
        tmp.ExtParam = &ext;
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

    void PreEnc(HevcTaskDSO& task /*FrameStatData& statData*/)
    {
        FrameStatData& statData = task.m_statData;

        MSDK_ZERO_MEMORY(m_BRCPar);
        MSDK_ZERO_MEMORY(m_BRCCtrl);

        m_BRCPar.EncodedOrder = statData.EncodedOrder;
        m_BRCPar.DisplayOrder = statData.DisplayOrder;
        m_BRCPar.FrameType    = statData.frame_type;

        mfxStatus sts = m_SW_BRC.GetFrameCtrl(&m_BRCPar, &m_BRCCtrl);
        if (MFX_ERR_NONE != sts)
            throw mfxError(sts, "m_SW_BRC.GetFrameCtrl failed");

        m_curQp = m_BRCCtrl.QpY;

#ifdef DEBUG_OUTPUT
        std::cout << std::endl << std::endl << "Frame #" << task.m_statData.DisplayOrder << std::endl;
        std::cout << "initial qp:   " << mfxI32(task.m_statData.qp) << "    qp set:    " << mfxI32(m_curQp) << std::endl;
        std::cout << "initial size: " << task.m_statData.frame_size << " result size: ";
#endif
    }

    void Report(mfxU32 dataLength)
    {
#ifdef DEBUG_OUTPUT
        std::cout << dataLength << std::endl;
#endif

        m_BRCPar.CodedFrameSize = dataLength;

        mfxBRCFrameStatus bsts;
        mfxStatus sts = m_SW_BRC.Update(&m_BRCPar, &m_BRCCtrl, &bsts);
        if (MFX_ERR_NONE != sts)
            throw mfxError(sts, "m_SW_BRC.Update failed");

        if (bsts.BRCStatus != MFX_BRC_OK)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "m_SW_BRC.Update failed. Re encode is not supported");
    }

    mfxU8 GetQP()
    {
        return m_curQp;
    }

    void SubmitNewStat(FrameStatData& /*stat*/) { return; }

private:
    ExtBRC m_SW_BRC;
    mfxU8  m_curQp         = 0xff;
    mfxU64 m_BitsEncoded   = 0;
    mfxF64 m_TargetBitrate = 0.0; // it is bits per frame, actually
    mfxU32 m_curFrame      = 0;

    mfxBRCFrameParam m_BRCPar;
    mfxBRCFrameCtrl  m_BRCCtrl;
};

class LA_BRC : public BRC
{
public:
    LA_BRC(mfxVideoParam const & video, mfxU16 TargetKbps, mfxU16 la_depth = 100, msdk_char* yuv_file = nullptr)
        : m_LookAheadDepth(la_depth)
        , m_TargetBitrate(mfxF64(1000 * TargetKbps * video.mfx.FrameInfo.FrameRateExtD) / video.mfx.FrameInfo.FrameRateExtN)
    {
        /*
        for (mfxU32 j = 1; j < BRC3_RATE_BLUR_LENGTH; ++j)
        {
            m_normFactors[j - 1] = exp(-(mfxF64)j*j / BRC3_GAUSS_VARIANCE);
        }*/

        m_frameStatData.reserve(m_LookAheadDepth);
    }

    LA_BRC(LA_BRC const&)            = delete;
    LA_BRC(LA_BRC &&)                = default;
    LA_BRC& operator=(LA_BRC const&) = delete;
    LA_BRC& operator=(LA_BRC &&)     = default;

    ~LA_BRC() {}

    void PreEnc(HevcTaskDSO& task /*FrameStatData& statData*/);

    void Report(mfxU32 dataLength);

    mfxU8 GetQP()
    {
        return m_curQp;
    }

    void SubmitNewStat(FrameStatData& stat)
    {
        // Fill some additional information
        stat.propagation = 1.0 - std::pow(stat.share_intra, BRC3_PROPAGATION_EXPONENT);
        stat.qstep0      = Qp2QStep(stat.qp);
        stat.complexity0 = stat.qstep0*stat.frame_size;

        m_frameStatData.push_back(stat);
    }

private:
    mfxU16 m_LookAheadDepth = 100;
    mfxU8  m_curQp          = 0xff;
    mfxF64 m_avMSE          = 0.0;
    mfxF64 m_TargetBitrate  = 0.0; // it is bits per frame, actually
    mfxU32 m_curFrame       = 0;

#ifdef SEPARATE_IP_B_PROCESSING
    mfxU64 m_IP_BitsEncoded   = 0;
    mfxU64 m_IP_BitsPredicted = 0;

    mfxU64 m_B_BitsEncoded    = 0;
    mfxU64 m_B_BitsPredicted  = 0;
#else
    mfxU64 m_BitsEncoded   = 0;
    mfxU64 m_BitsPredicted = 0;
#endif

    //mfxF64 m_normFactors[BRC3_RATE_BLUR_LENGTH - 1];

    std::vector<FrameStatData> m_frameStatData;

    // Update internal state with new frame
    void UpdateStatData(mfxU32 frame_number);

    bool PossibleEncodeAsIs(mfxU32 start_index = 0);
};

#endif // __ABR_BRC_H__
