// Copyright (c) 2020-2021 Intel Corporation
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

#include "hevcehw_g12_scc.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Gen12;

const GUID SCC::DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main =
{ 0x2dec00c7, 0x21ee, 0x4bf8,{ 0x8f, 0x0e, 0x77, 0x3f, 0x11, 0xf1, 0x26, 0xa2 } };
const GUID SCC::DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main10 =
{ 0xc35153a0, 0x23c0, 0x4a81,{ 0xb3, 0xbb, 0x6a, 0x13, 0x26, 0xf2, 0xb7, 0x6b } };
const GUID SCC::DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444 =
{ 0xa33fd0ec, 0xa9d3, 0x4c21,{ 0x92, 0x76, 0xc2, 0x41, 0xcc, 0x90, 0xf6, 0xc7 } };
const GUID SCC::DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444_10 =
{ 0x310e59d2, 0x7ea4, 0x47bb,{ 0xb3, 0x19, 0x50, 0x0e, 0x78, 0x85, 0x53, 0x36 } };

constexpr mfxU8 SCC_EXT_ID = 3;

bool SCC::ReadSpsExt(StorageRW& strg, const Base::SPS&, mfxU8 id, Base::IBsReader& bs)
{
    if (id != SCC_EXT_ID)
        return false;

    SccSpsExt& sps = SpsExt::GetOrConstruct(strg, SccSpsExt{});

    sps.curr_pic_ref_enabled_flag = bs.GetBit();
    sps.palette_mode_enabled_flag = bs.GetBit();

    if (sps.palette_mode_enabled_flag)
    {
        sps.palette_max_size = bs.GetUE();
        sps.delta_palette_max_predictor_size = bs.GetUE();
        sps.palette_predictor_initializer_present_flag = bs.GetBit();

        if (sps.palette_predictor_initializer_present_flag)
        {
            return false;
        }
    }

    sps.motion_vector_resolution_control_idc = bs.GetBits(2);
    sps.intra_boundary_filtering_disabled_flag = bs.GetBit();
    sps.scc_extension_flag = 1;

    return true;
}

bool SCC::ReadPpsExt(StorageRW& strg, const Base::PPS&, mfxU8 id, Base::IBsReader& bs)
{
    if (id != SCC_EXT_ID)
        return false;

    auto pExt = make_storable<SccPpsExt>(SccPpsExt{});
    SccPpsExt& pps = *pExt;

    pps.curr_pic_ref_enabled_flag = bs.GetBit();
    pps.residual_adaptive_colour_transform_enabled_flag = bs.GetBit();  // MBZ for Gen12

    if (pps.residual_adaptive_colour_transform_enabled_flag)
    {
        return false;
    }
    pps.palette_predictor_initializer_present_flag = bs.GetBit();   // MBZ for Gen12

    if (pps.palette_predictor_initializer_present_flag)
    {
        return false;
    }

    pps.scc_extension_flag = 1;

    strg.Insert(PpsExt::Key, std::move(pExt));

    return true;
}

bool SCC::PackSpsExt(StorageRW& strg, const Base::SPS&, mfxU8 id, Base::IBsWriter& bs)
{
    if (id != SCC_EXT_ID)
        return false;

    auto& sps = SpsExt::Get(strg);

    bs.PutBit(sps.curr_pic_ref_enabled_flag);
    bs.PutBit(sps.palette_mode_enabled_flag);

    if (sps.palette_mode_enabled_flag)
    {
        bs.PutUE(sps.palette_max_size);
        bs.PutUE(sps.delta_palette_max_predictor_size);
        bs.PutBit(0); // Gen12: palette_predictor_initializer_present_flag - MBZ
    }

    bs.PutBits(2, 0); // Gen12: motion_vector_resolution_control_idc - MBZ
    bs.PutBit(0); // Gen12: intra_boundary_filtering_disabled_flag - MBZ

    return true;
}

bool SCC::PackPpsExt(StorageRW& strg, const Base::PPS&, mfxU8 id, Base::IBsWriter& bs)
{
    if (id != SCC_EXT_ID)
        return false;

    auto& pps = PpsExt::Get(strg);

    bs.PutBit(pps.curr_pic_ref_enabled_flag);
    bs.PutBit(0); // Gen12: pps.residual_adaptive_colour_transform_enabled_flag - MBZ
    bs.PutBit(0); // Gen12: pps.palette_predictor_initializer_present_flag - MBZ

    return true;
}

void SCC::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_SetCallChains
        , [this](const mfxVideoParam& par, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        MFX_CHECK(par.mfx.CodecProfile == MFX_PROFILE_HEVC_SCC, MFX_ERR_NONE);
        MFX_CHECK(!IsOff(par.mfx.LowPower), MFX_ERR_NONE);

        auto& defaults = Glob::Defaults::GetOrConstruct(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        MFX_CHECK(!bSet, MFX_ERR_NONE);

        defaults.CheckProfile.Push(
            [](Base::Defaults::TCheckAndFix::TExt prev
                , const Base::Defaults::Param& dpar
                , mfxVideoParam& par)
        {
            OnExit reveretProfile([&](){ SetIf(par.mfx.CodecProfile, !!par.mfx.CodecProfile, MFX_PROFILE_HEVC_SCC); });
            par.mfx.CodecProfile = MFX_PROFILE_HEVC_REXT;

            return prev(dpar, par);
        });
        defaults.GetVPS.Push([](
            Defaults::TGetVPS::TExt prev
            , const Defaults::Param& defpar
            , Base::VPS& vps)
        {
            auto sts = prev(defpar, vps);

            vps.general.constraint.max_12bit = 1;
            vps.general.constraint.max_10bit = 1;
            vps.general.constraint.max_8bit = (defpar.mvp.mfx.FrameInfo.BitDepthLuma != 10);
            vps.general.constraint.max_422chroma = (defpar.mvp.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444);
            vps.general.constraint.max_420chroma = (defpar.mvp.mfx.FrameInfo.ChromaFormat != MFX_CHROMAFORMAT_YUV444);
            vps.general.constraint.lower_bit_rate = 1;

            return sts;
        });
        defaults.GetSPS.Push(
            [](Defaults::TGetSPS::TExt prev
                , const Defaults::Param& defPar
                , const Base::VPS& vps
                , Base::SPS& sps)
        {
            auto sts = prev(defPar, vps, sps);
            MFX_CHECK_STS(sts);

            sps.extension_flag = 1;
            sps.ExtensionFlags |= (0x80 >> SCC_EXT_ID);

            return sts;
        });
        defaults.GetPPS.Push(
            [](Defaults::TGetPPS::TExt prev
                , const Defaults::Param& defPar
                , const Base::SPS& sps
                , Base::PPS& pps)
        {
            auto sts = prev(defPar, sps, pps);
            MFX_CHECK_STS(sts);

            pps.extension_flag = 1;
            pps.ExtensionFlags |= (0x80 >> SCC_EXT_ID);

            // Disable ref-list modification
            pps.lists_modification_present_flag = 0;

            return sts;
        });

        bSet = true;

        return MFX_ERR_NONE;

    });

    Push(BLK_SetLowPowerDefault
        , [](const mfxVideoParam&, mfxVideoParam& par, StorageW& strg) -> mfxStatus
    {
        bool bLowPower =
            Glob::VideoCore::Get(strg).GetHWType() >= MFX_HW_TGL_LP
            && par.mfx.CodecProfile == MFX_PROFILE_HEVC_SCC;

        SetDefault(par.mfx.LowPower, mfxU16(bLowPower * MFX_CODINGOPTION_ON));

        return MFX_ERR_NONE;
    });


    Push(BLK_LoadSPSPPS
        , [](const mfxVideoParam&, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        Glob::NeedRextConstraints::GetOrConstruct(strg) = [](const Base::PTL& ptl)
        {
            return ((ptl.profile_idc == 9) || (ptl.profile_compatibility_flags & (0x1 << 9)));
        };

        using namespace std::placeholders;
        Glob::ReadSpsExt::GetOrConstruct(strg) = std::bind(ReadSpsExt, std::ref(strg), _1, _2, _3);
        Glob::ReadPpsExt::GetOrConstruct(strg) = std::bind(ReadPpsExt, std::ref(strg), _1, _2, _3);
        Glob::PackSpsExt::GetOrConstruct(strg) = std::bind(PackSpsExt, std::ref(strg), _1, _2, _3);
        Glob::PackPpsExt::GetOrConstruct(strg) = std::bind(PackPpsExt, std::ref(strg), _1, _2, _3);

        return MFX_ERR_NONE;
    });

    Push(BLK_SetGUID
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        MFX_CHECK(par.mfx.CodecProfile == MFX_PROFILE_HEVC_SCC, MFX_ERR_NONE);
        MFX_CHECK(!IsOff(par.mfx.LowPower), MFX_ERR_NONE);
        //don't change GUID in Reset
        MFX_CHECK(!strg.Contains(Glob::RealState::Key), MFX_ERR_NONE);

        SetExtraGUIDs(strg);

        static const std::map<mfxU16, std::map<mfxU16, GUID>> GUIDSupported =
        {
            {
                mfxU16(8),
                {
                    {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main}
                    , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444}
                }
            }
            , {
                mfxU16(10),
                {
                    {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main10}
                    , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_LowpowerEncode_HEVC_SCC_Main444_10}
                }
            }
        };
        auto& fi = par.mfx.FrameInfo;
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        mfxU16 bd = 8, cf = fi.ChromaFormat;

        bd = std::max<mfxU16>(bd, fi.BitDepthLuma);
        bd = std::max<mfxU16>(bd, fi.BitDepthChroma);

        if (pCO3)
        {
            bd = std::max<mfxU16>(bd, pCO3->TargetBitDepthLuma);
            bd = std::max<mfxU16>(bd, pCO3->TargetBitDepthChroma);

            if (pCO3->TargetChromaFormatPlus1)
                cf = std::max<mfxU16>(cf, pCO3->TargetChromaFormatPlus1 - 1);
        }

        MFX_CHECK(GUIDSupported.count(bd) && GUIDSupported.at(bd).count(cf), MFX_ERR_NONE);
        Glob::GUID::GetOrConstruct(strg) = GUIDSupported.at(bd).at(cf);

        return MFX_ERR_NONE;
    });
}

void SCC::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_Init
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        m_bPatchNextDDITask = (Glob::VideoParam::Get(strg).mfx.CodecProfile == MFX_PROFILE_HEVC_SCC);
        m_bPatchDDISlices = m_bPatchNextDDITask;
        return MFX_ERR_NONE;
    });

    Push(BLK_SetSPSExt
        , [](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        MFX_CHECK(Glob::VideoParam::Get(strg).mfx.CodecProfile == MFX_PROFILE_HEVC_SCC, MFX_ERR_NONE);

        auto& spsExt = SpsExt::GetOrConstruct(strg, SccSpsExt{});

        spsExt = {};
        spsExt.curr_pic_ref_enabled_flag = 1;
        spsExt.palette_mode_enabled_flag = 1;
        spsExt.palette_max_size = 64;
        spsExt.delta_palette_max_predictor_size = 32;
        spsExt.scc_extension_flag = 1;

        return MFX_ERR_NONE;
    });

    Push(BLK_SetPPSExt
        , [](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        MFX_CHECK(Glob::VideoParam::Get(strg).mfx.CodecProfile == MFX_PROFILE_HEVC_SCC, MFX_ERR_NONE);

        auto& ppsExt = PpsExt::GetOrConstruct(strg, SccPpsExt{});

        ppsExt = {};
        ppsExt.curr_pic_ref_enabled_flag = 1;
        ppsExt.scc_extension_flag = 1;

        return MFX_ERR_NONE;
    });
}

void SCC::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushPostRT Push)
{
    Push(BLK_PatchSliceHeader
        , [](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        MFX_CHECK(Glob::VideoParam::Get(global).mfx.CodecProfile == MFX_PROFILE_HEVC_SCC, MFX_ERR_NONE);
        auto& ssh = Base::Task::SSH::Get(s_task);

        ssh.num_ref_idx_active_override_flag = 1;

        if (ssh.type == 2)
        {
            // Change slice type from I to P and disable temporal MV prediction
            // to enable IBC
            ssh.type = 1;
            ssh.temporal_mvp_enabled_flag = 0;
        }
        else
        {
            ssh.num_ref_idx_l0_active_minus1++; //for cur pic ref
        }

        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
