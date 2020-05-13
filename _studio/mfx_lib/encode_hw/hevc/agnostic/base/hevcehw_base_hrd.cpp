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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_hrd.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void HRD::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_Init
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        Init(
            Glob::SPS::Get(strg)
            , InitialDelayInKB(Glob::VideoParam::Get(strg).mfx)
        );
        m_prevBpEncOrderAsync = 0;

        return MFX_ERR_NONE;
    });
}

void HRD::ResetState(const FeatureBlocks& /*blocks*/, TPushRS Push)
{
    Push(BLK_Reset
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        Reset(Glob::SPS::Get(strg));
        return MFX_ERR_NONE;
    });

}

void HRD::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SubmitTask
        , [this](StorageW&, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        if (task.NumRecode == 0)
            task.cpb_removal_delay = !!task.EncodedOrder * (task.EncodedOrder - m_prevBpEncOrderAsync);

        bool bBPSEI    = (task.InsertHeaders & INSERT_BPSEI);
        bool bSetBPPar =
            bBPSEI
            && !task.initial_cpb_removal_delay
            && !task.initial_cpb_removal_offset;

        if (bSetBPPar)
        {
            task.initial_cpb_removal_delay = GetInitCpbRemovalDelay(task.EncodedOrder);
            task.initial_cpb_removal_offset = GetInitCpbRemovalDelayOffset();
        }

        SetIf(m_prevBpEncOrderAsync, bBPSEI, task.EncodedOrder);

        return MFX_ERR_NONE;
    });
}

void HRD::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_QueryTask
        , [this](StorageW&, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        Update(task.BsDataLength * 8, task.EncodedOrder, !!(task.InsertHeaders & INSERT_BPSEI));

        return MFX_ERR_NONE;
    });
}

void HRD::Init(const SPS &sps, mfxU32 InitialDelayInKB)
{
    VUI const & vui = sps.vui;
    HRDInfo const & hrd = sps.vui.hrd;
    HRDInfo::SubLayer::CPB const & cpb0 = hrd.sl[0].cpb[0];

    m_bIsHrdRequired =
        !(     !sps.vui_parameters_present_flag
            || !sps.vui.hrd_parameters_present_flag
            || !(hrd.nal_hrd_parameters_present_flag || hrd.vcl_hrd_parameters_present_flag));

    if (!m_bIsHrdRequired)
        return;

    mfxU32 cpbSize = (cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale);
    m_cbrFlag = !!cpb0.cbr_flag;
    m_bitrate = (cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale);
    m_maxCpbRemovalDelay = 1 << (hrd.au_cpb_removal_delay_length_minus1 + 1);

    m_cpbSize90k = mfxU32(90000. * cpbSize / m_bitrate);
    m_initCpbRemovalDelay = mfxU32(90000. * 8000. * InitialDelayInKB / m_bitrate);
    m_clockTick = (mfxF64)vui.num_units_in_tick * 90000 / vui.time_scale;

    m_prevAuCpbRemovalDelayMinus1 = -1;
    m_prevAuCpbRemovalDelayMsb = 0;
    m_prevAuFinalArrivalTime = 0;
    m_prevBpAuNominalRemovalTime = m_initCpbRemovalDelay;
    m_prevBpEncOrder = 0;
}

void HRD::Reset(SPS const & sps)
{
    HRDInfo const & hrd = sps.vui.hrd;
    HRDInfo::SubLayer::CPB const & cpb0 = hrd.sl[0].cpb[0];

    if (m_bIsHrdRequired == false)
        return;

    mfxU32 cpbSize = (cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale);
    m_bitrate = (cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale);
    m_cpbSize90k = mfxU32(90000. * cpbSize / m_bitrate);
}

void HRD::Update(mfxU32 sizeInbits, mfxU32 encOrder, bool bufferingPeriodPic)
{
    mfxF64 auNominalRemovalTime;

    if (m_bIsHrdRequired == false)
        return;

    // (C-9)
    auNominalRemovalTime = m_initCpbRemovalDelay;

    if (encOrder > 0)
    {
        mfxU32  auCpbRemovalDelayMinus1 = (encOrder - m_prevBpEncOrder) - 1;
        bool    bSetDelayMsb            = !bufferingPeriodPic && auCpbRemovalDelayMinus1;
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb =
            bSetDelayMsb
            * (m_prevAuCpbRemovalDelayMsb
                + m_maxCpbRemovalDelay * (mfxI32(auCpbRemovalDelayMinus1) <= m_prevAuCpbRemovalDelayMinus1));

        m_prevAuCpbRemovalDelayMsb      = auCpbRemovalDelayMsb;
        m_prevAuCpbRemovalDelayMinus1   = auCpbRemovalDelayMinus1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_clockTick * (auCpbRemovalDelayValMinus1 + 1);
    }

    // (C-3)
    mfxF64 initArrivalTime = m_prevAuFinalArrivalTime;

    if (!m_cbrFlag)
    {
        mfxF64 initArrivalEarliestTime = auNominalRemovalTime;
        // (C-7)
        initArrivalEarliestTime -= !!bufferingPeriodPic * m_initCpbRemovalDelay;
        // (C-6)
        initArrivalEarliestTime -= !bufferingPeriodPic * m_cpbSize90k;
        // (C-4)
        initArrivalTime = std::max<mfxF64>(m_prevAuFinalArrivalTime, initArrivalEarliestTime * m_bitrate);
    }
    // (C-8)
    mfxF64 auFinalArrivalTime = initArrivalTime + (mfxF64)sizeInbits * 90000;

    m_prevAuFinalArrivalTime = auFinalArrivalTime;

    if (bufferingPeriodPic)
    {
        m_prevBpAuNominalRemovalTime = auNominalRemovalTime;
        m_prevBpEncOrder            = encOrder;
    }

    /*fprintf (stderr, "FO: %d\ninitArrivalTime = %f\nauFinalArrivalTime = %f\nauNominalRemovalTime = %f\n",
        encOrder, initArrivalTime / 90000 / m_bitrate, auFinalArrivalTime / 90000 / m_bitrate, auNominalRemovalTime / 90000);
    fflush(stderr);*/
}

mfxU32 HRD::GetInitCpbRemovalDelay(mfxU32 encOrder)
{
    mfxF64 auNominalRemovalTime;

    if (m_bIsHrdRequired == false)
        return 0;

    if (encOrder > 0)
    {
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb = 0;
        mfxU32 auCpbRemovalDelayMinus1 = encOrder - m_prevBpEncOrder - 1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_clockTick * (auCpbRemovalDelayValMinus1 + 1);

        // (C-17)
        mfxF64 deltaTime90k = auNominalRemovalTime - m_prevAuFinalArrivalTime / m_bitrate;

        m_initCpbRemovalDelay = m_cbrFlag
            // (C-19)
            ? (mfxU32)(deltaTime90k)
            // (C-18)
            : (mfxU32)std::min<mfxF64>(deltaTime90k, m_cpbSize90k);
    }

    return (mfxU32)m_initCpbRemovalDelay;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
