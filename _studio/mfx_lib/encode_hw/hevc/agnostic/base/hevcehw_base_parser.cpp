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

#include "hevcehw_base_parser.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

BitstreamReader::BitstreamReader(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
    : m_bsStart(bs)
    , m_bsEnd(bs + size)
    , m_bs(bs)
    , m_bitStart(bitOffset & 7)
    , m_bitOffset(bitOffset & 7)
    , m_emulation(true)
{
}

BitstreamReader::~BitstreamReader()
{
}

void BitstreamReader::Reset(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
{
    if (bs)
    {
        m_bsStart = bs;
        m_bsEnd = bs + size;
        m_bs = bs;
        m_bitOffset = (bitOffset & 7);
        m_bitStart = (bitOffset & 7);
    }
    else
    {
        m_bs = m_bsStart;
        m_bitOffset = m_bitStart;
    }
}

mfxU32 BitstreamReader::GetBit()
{
    assert(m_bs < m_bsEnd);
    ThrowIf(m_bs >= m_bsEnd, EndOfBuffer());

    mfxU32 b = (*m_bs >> (7 - m_bitOffset)) & 1;

    bool bNewByte = (++m_bitOffset == 8);

    m_bs += bNewByte;
    m_bitOffset *= !bNewByte;

    bool bSkipEPByte =
        bNewByte
        && m_emulation
        && m_bs - m_bsStart >= 2
        && (m_bsEnd - m_bs) >= 1
        && m_bs[ 0] == 0x03
        && m_bs[-1] == 0x00
        && m_bs[-2] == 0x00
        && (m_bs[1] & 0xfc) == 0x00;

    m_bs += bSkipEPByte;

    return b;
}

mfxU32 BitstreamReader::GetBits(mfxU32 n)
{
    mfxU32 b = 0;

    while (n--)
        b = (b << 1) | GetBit();

    return b;
}

mfxU32 BitstreamReader::GetUE()
{
    mfxU32 lz = 0;

    while (!GetBit())
        lz++;

    return !lz ? 0 : ((1 << lz) | GetBits(lz)) - 1;
}

mfxI32 BitstreamReader::GetSE()
{
    mfxU32 ue = GetUE();
    return (ue & 1) ? (mfxI32)((ue + 1) >> 1) : -(mfxI32)((ue + 1) >> 1);
}

template<class TDst>
bool TryRead(TDst& dst, StorageR& strg, StorageR::TKey key)
{
    if (!strg.Contains(key))
        return false;
    dst = strg.Read<TDst>(key);
    return true;
}

void Parser::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_LoadSPSPPS
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        mfxExtCodingOptionSPSPPS* pSP = ExtBuffer::Get(par);

        MFX_CHECK(pSP, MFX_ERR_NONE);

        auto pSPS = make_storable<SPS>();
        auto pPPS = make_storable<PPS>();

        TryRead(m_needRextConstraints, strg, Glob::NeedRextConstraints::Key);
        TryRead(m_readSpsExt, strg, Glob::ReadSpsExt::Key);
        TryRead(m_readPpsExt, strg, Glob::ReadPpsExt::Key);

        auto sts = LoadSPSPPS(par, *pSPS, *pPPS);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        bool bErr = pSP->SPSBuffer && pSP->SPSBufSize && !strg.TryInsert(Glob::SPS::Key, std::move(pSPS));
        ThrowAssert(bErr, "failed to insert Glob::SPS");

        bErr = pSP->PPSBuffer && pSP->PPSBufSize && !strg.TryInsert(Glob::PPS::Key, std::move(pPPS));
        ThrowAssert(bErr, "failed to insert Glob::PPS");

        return sts;
    });
}

mfxStatus Parser::LoadSPSPPS(mfxVideoParam& par, SPS & sps, PPS & pps)
{
    mfxExtCodingOptionSPSPPS* pSP = ExtBuffer::Get(par);
    if (!pSP)
        return MFX_ERR_NONE;

    mfxStatus sts = MFX_ERR_NONE;

    if (pSP->SPSBuffer)
    {
        BitstreamReader bs(pSP->SPSBuffer, pSP->SPSBufSize);

        sts = ParseSPS(bs, sps);
        MFX_CHECK_STS(sts);

        sts = SpsToMFX(sps, par);
        MFX_CHECK_STS(sts);
    }

    if (pSP->PPSBuffer)
    {
        BitstreamReader bs(pSP->PPSBuffer, pSP->PPSBufSize);

        sts = ParsePPS(bs, pps);
        MFX_CHECK_STS(sts);

        sts = PpsToMFX(pps, par);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus Parser::ParseNALU(BitstreamReader& bs, NALU & nalu)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool emulation = bs.GetEmulation();

    bs.SetEmulation(false);

    try
    {
        mfxU32 start_code = bs.GetBits(24);
        mfxU32 n = 3;

        while ((start_code & 0x00FFFFFF) != 1)
        {
            start_code <<= 8;
            start_code |= bs.GetBits(8);
            n++;
        }

        if (bs.GetBit())
            return MFX_ERR_INVALID_VIDEO_PARAM;

        nalu.long_start_code = (n > 3) && !(start_code >> 24);
        nalu.nal_unit_type = bs.GetBits(6);
        nalu.nuh_layer_id = bs.GetBits(6);
        nalu.nuh_temporal_id_plus1 = bs.GetBits(3);
    }
    catch (std::exception &)
    {
        sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }

    bs.SetEmulation(emulation);

    return sts;
}

mfxStatus ParsePTL(
    BitstreamReader& bs
    , PTL& ptl
    , std::function<bool(const PTL&)> NeedRextConstraints)
{
    if (ptl.profile_present_flag)
    {
        ptl.profile_space               = bs.GetBits(2);
        ptl.tier_flag                   = bs.GetBit();
        ptl.profile_idc                 = bs.GetBits(5);
        ptl.profile_compatibility_flags = bs.GetBits(32);
        ptl.progressive_source_flag     = bs.GetBit();
        ptl.interlaced_source_flag      = bs.GetBit();
        ptl.non_packed_constraint_flag  = bs.GetBit();
        ptl.frame_only_constraint_flag  = bs.GetBit();

        bool bREXT =
            ((ptl.profile_idc >= 4) && (ptl.profile_idc <= 7))
            || (ptl.profile_compatibility_flags & 0xf0)
            || (NeedRextConstraints && NeedRextConstraints(ptl));
        
        ptl.constraint.max_12bit        = bs.CondBit(bREXT, 0);
        ptl.constraint.max_10bit        = bs.CondBit(bREXT, 0);
        ptl.constraint.max_8bit         = bs.CondBit(bREXT, 0);
        ptl.constraint.max_422chroma    = bs.CondBit(bREXT, 0);
        ptl.constraint.max_420chroma    = bs.CondBit(bREXT, 0);
        ptl.constraint.max_monochrome   = bs.CondBit(bREXT, 0);
        ptl.constraint.intra            = bs.CondBit(bREXT, 0);
        ptl.constraint.one_picture_only = bs.CondBit(bREXT, 0);
        ptl.constraint.lower_bit_rate   = bs.CondBit(bREXT, 0);

        //general_reserved_zero_34bits
        MFX_CHECK(!bs.GetBits(24), MFX_ERR_UNSUPPORTED);
        MFX_CHECK(!bs.GetBits(10), MFX_ERR_UNSUPPORTED);
        //general_reserved_zero_43bits
        MFX_CHECK(bREXT || !bs.GetBits(9), MFX_ERR_UNSUPPORTED);

        /*if (   ptl.profile_idc >= 1 && ptl.profile_idc <= 5
            || (ptl.profile_compatibility_flags & 0x3e))
        {
            ptl.inbld_flag = bs.GetBit();
        }
        else*/
        MFX_CHECK(!bs.GetBit(), MFX_ERR_UNSUPPORTED);//general_reserved_zero_bit
    }

    ptl.level_idc = (mfxU8)bs.CondBits(ptl.level_present_flag, 8, 0);

    return MFX_ERR_NONE;
}

bool DPocSort(mfxI16 l, mfxI16 r)
{
    if (l < 0 && r > 0) return true;
    if (r < 0 && l > 0) return false;
    if (l < 0) return (l > r);
    return (l < r);
}

void ParseSTRPS(
    BitstreamReader& bs
    , SPS & sps
    , mfxU32 idx)
{
    STRPS & strps = sps.strps[idx];

    strps.inter_ref_pic_set_prediction_flag = bs.CondBit(idx != 0, 0);

    if (strps.inter_ref_pic_set_prediction_flag)
    {
        auto& cur = strps;

        strps.delta_rps_sign = bs.GetBit();
        strps.abs_delta_rps_minus1 = (mfxU16)bs.GetUE();

        mfxU32 RefRpsIdx = idx - (strps.delta_idx_minus1 + 1);
        STRPS& ref = sps.strps[RefRpsIdx];
        mfxI16 NumDeltaPocs = 0;
        mfxI16 RefNumDeltaPocs = NumDeltaPocs;
        mfxI16 deltaRps = (1 - 2 * strps.delta_rps_sign) * (strps.abs_delta_rps_minus1 + 1);
        bool used_by_curr_pic_flag[std::extent<decltype(cur.pic)>::value] = {};
        bool use_delta_flag[std::extent<decltype(cur.pic)>::value] = {};

        for (mfxI16 i = 0; i <= RefNumDeltaPocs; ++i)
        {
            auto& curPic = cur.pic[NumDeltaPocs];
            used_by_curr_pic_flag[i] = !!bs.GetBit();

            use_delta_flag[i] = used_by_curr_pic_flag[i] || bs.GetBit();
            if (use_delta_flag[i])
            {
                curPic.used_by_curr_pic_sx_flag = used_by_curr_pic_flag[i];
                curPic.DeltaPocSX = (i < RefNumDeltaPocs) * ref.pic[i].DeltaPocSX + deltaRps;
                cur.num_negative_pics += (curPic.DeltaPocSX < 0);
                cur.num_positive_pics += (curPic.DeltaPocSX > 0);
                NumDeltaPocs++;
            }
        }

        std::sort(
            cur.pic
            , cur.pic + NumDeltaPocs
            , [](STRPS::Pic& l, STRPS::Pic& r)
        {
            return DPocSort(l.DeltaPocSX, r.DeltaPocSX);
        });

        for (mfxI16 i = 0; i <= RefNumDeltaPocs; ++i)
        {
            cur.pic[i].used_by_curr_pic_flag = used_by_curr_pic_flag[i];
            cur.pic[i].use_delta_flag = use_delta_flag[i];
        }
    }
    else
    {
        strps.num_negative_pics = bs.GetUE();
        strps.num_positive_pics = bs.GetUE();

        mfxI16 sign = -1;
        auto ParseIntraPic = [&](STRPS::Pic& pic)
        {
            pic.delta_poc_sx_minus1      = bs.GetUE();
            pic.used_by_curr_pic_sx_flag = bs.GetBit();
            pic.DeltaPocSX = mfxI16(sign * (pic.delta_poc_sx_minus1 + 1));
        };

        std::for_each(
            strps.pic
            , strps.pic + strps.num_negative_pics
            , ParseIntraPic);

        sign = 1;
        std::for_each(
            strps.pic + strps.num_negative_pics
            , strps.pic + strps.num_negative_pics + strps.num_positive_pics
            , ParseIntraPic);
    }
}

mfxStatus ParseHRD(
    BitstreamReader& bs
    , SPS & sps)
{
    HRDInfo & hrd = sps.vui.hrd;

    hrd.nal_hrd_parameters_present_flag = bs.GetBit();
    hrd.vcl_hrd_parameters_present_flag = bs.GetBit();

    bool bHrdParam =
        hrd.nal_hrd_parameters_present_flag
        || hrd.vcl_hrd_parameters_present_flag;

    hrd.sub_pic_hrd_params_present_flag              = bs.CondBit(bHrdParam, 0);
    hrd.tick_divisor_minus2                          = bs.CondBits(hrd.sub_pic_hrd_params_present_flag, 8, 0);
    hrd.du_cpb_removal_delay_increment_length_minus1 = bs.CondBits(hrd.sub_pic_hrd_params_present_flag, 5, 0);
    hrd.sub_pic_cpb_params_in_pic_timing_sei_flag    = bs.CondBit(hrd.sub_pic_hrd_params_present_flag, 0);
    hrd.dpb_output_delay_du_length_minus1            = bs.CondBits(hrd.sub_pic_hrd_params_present_flag, 5, 0);
    hrd.bit_rate_scale                               = bs.CondBits(bHrdParam, 4, 0);
    hrd.cpb_size_scale                               = bs.CondBits(bHrdParam, 4, 0);
    hrd.cpb_size_du_scale                            = bs.CondBits(hrd.sub_pic_hrd_params_present_flag, 4, 0);
    hrd.initial_cpb_removal_delay_length_minus1      = bs.CondBits(bHrdParam, 5, 0);
    hrd.au_cpb_removal_delay_length_minus1           = bs.CondBits(bHrdParam, 5, 0);
    hrd.dpb_output_delay_length_minus1               = bs.CondBits(bHrdParam, 5, 0);

    bool bErr = std::any_of(
        hrd.sl
        , hrd.sl + sps.max_sub_layers_minus1 + 1
        , [&](HRDInfo::SubLayer& sl)
    {
        sl.fixed_pic_rate_general_flag = bs.GetBit();
        sl.fixed_pic_rate_within_cvs_flag = !sl.fixed_pic_rate_general_flag && bs.GetBit();

        bool bFixedPicRate =
            sl.fixed_pic_rate_within_cvs_flag || sl.fixed_pic_rate_general_flag;

        sl.elemental_duration_in_tc_minus1  = bs.CondUE(bFixedPicRate, 0);
        sl.low_delay_hrd_flag               = bs.CondBit(!bFixedPicRate, 0);
        sl.cpb_cnt_minus1                   = bs.CondUE(!sl.low_delay_hrd_flag, 0);

        std::for_each(
            sl.cpb
            , sl.cpb + (sl.cpb_cnt_minus1 + 1) * hrd.nal_hrd_parameters_present_flag
            , [&](decltype(*sl.cpb) cpb)
        {
            cpb.bit_rate_value_minus1 = bs.GetUE();
            cpb.cpb_size_value_minus1 = bs.GetUE();

            cpb.cpb_size_du_value_minus1 = bs.CondUE(hrd.sub_pic_hrd_params_present_flag, 0);
            cpb.bit_rate_du_value_minus1 = bs.CondUE(hrd.sub_pic_hrd_params_present_flag, 0);

            cpb.cbr_flag = (mfxU8)bs.GetBit();
        });

        return !!hrd.vcl_hrd_parameters_present_flag;
    });

    MFX_CHECK(!bErr, MFX_ERR_UNSUPPORTED);

    return MFX_ERR_NONE;
}

mfxStatus ParseVUI(
    BitstreamReader& bs
    , SPS & sps)
{
    VUI & vui = sps.vui;

    vui.aspect_ratio_info_present_flag = bs.GetBit();

    vui.aspect_ratio_idc = (mfxU8)bs.CondBits(vui.aspect_ratio_info_present_flag, 8, 0);
    vui.sar_width  = (mfxU16)bs.CondBits(vui.aspect_ratio_idc == 255, 16, 0);
    vui.sar_height = (mfxU16)bs.CondBits(vui.aspect_ratio_idc == 255, 16, 0);

    vui.overscan_info_present_flag = bs.GetBit();
    vui.overscan_appropriate_flag = bs.CondBit(vui.overscan_info_present_flag, 0);

    vui.video_signal_type_present_flag  = bs.GetBit();
    vui.video_format                    = bs.CondBits(vui.video_signal_type_present_flag, 3, 0);
    vui.video_full_range_flag           = bs.CondBit(vui.video_signal_type_present_flag, 0);
    vui.colour_description_present_flag = bs.CondBit(vui.video_signal_type_present_flag, 0);
    vui.colour_primaries                = (mfxU8)bs.CondBits(vui.colour_description_present_flag, 8, 0);
    vui.transfer_characteristics        = (mfxU8)bs.CondBits(vui.colour_description_present_flag, 8, 0);
    vui.matrix_coeffs                   = (mfxU8)bs.CondBits(vui.colour_description_present_flag, 8, 0);

    vui.chroma_loc_info_present_flag        = bs.GetBit();
    vui.chroma_sample_loc_type_top_field    = bs.CondUE(vui.chroma_loc_info_present_flag, 0);
    vui.chroma_sample_loc_type_bottom_field = bs.CondUE(vui.chroma_loc_info_present_flag, 0);

    vui.neutral_chroma_indication_flag  = bs.GetBit();
    vui.field_seq_flag                  = bs.GetBit();
    vui.frame_field_info_present_flag   = bs.GetBit();

    vui.default_display_window_flag = bs.GetBit();
    vui.def_disp_win_left_offset    = bs.CondUE(vui.default_display_window_flag, 0);
    vui.def_disp_win_right_offset   = bs.CondUE(vui.default_display_window_flag, 0);
    vui.def_disp_win_top_offset     = bs.CondUE(vui.default_display_window_flag, 0);
    vui.def_disp_win_bottom_offset  = bs.CondUE(vui.default_display_window_flag, 0);

    vui.timing_info_present_flag = bs.GetBit();

    vui.num_units_in_tick               = bs.CondBits(vui.timing_info_present_flag, 32, 0);
    vui.time_scale                      = bs.CondBits(vui.timing_info_present_flag, 32, 0);
    vui.poc_proportional_to_timing_flag = bs.CondBit(vui.timing_info_present_flag, 0);
    vui.num_ticks_poc_diff_one_minus1   = bs.CondUE(vui.poc_proportional_to_timing_flag, 0);

    vui.hrd_parameters_present_flag = bs.GetBit();

    mfxStatus sts = MFX_ERR_NONE;
    bool bErr = vui.hrd_parameters_present_flag && Res2Bool(sts, ParseHRD(bs, sps));
    MFX_CHECK(!bErr, sts);

    vui.bitstream_restriction_flag              = bs.GetBit();
    vui.tiles_fixed_structure_flag              = bs.CondBit(vui.bitstream_restriction_flag, 0);
    vui.motion_vectors_over_pic_boundaries_flag = bs.CondBit(vui.bitstream_restriction_flag, 0);
    vui.restricted_ref_pic_lists_flag           = bs.CondBit(vui.bitstream_restriction_flag, 0);
    vui.min_spatial_segmentation_idc            = bs.CondUE(vui.bitstream_restriction_flag, 0);
    vui.max_bytes_per_pic_denom                 = bs.CondUE(vui.bitstream_restriction_flag, 0);
    vui.max_bits_per_min_cu_denom               = bs.CondUE(vui.bitstream_restriction_flag, 0);
    vui.log2_max_mv_length_horizontal           = bs.CondUE(vui.bitstream_restriction_flag, 0);
    vui.log2_max_mv_length_vertical             = bs.CondUE(vui.bitstream_restriction_flag, 0);

    return MFX_ERR_NONE;
}

mfxStatus Parser::ParseSPS(BitstreamReader& bs, SPS & sps)
{
    mfxStatus sts = MFX_ERR_NONE;
    NALU nalu = {};

    sps = SPS{};

    do
    {
        sts = ParseNALU(bs, nalu);
        MFX_CHECK_STS(sts);
    } while (nalu.nal_unit_type != SPS_NUT);

    try
    {
        sps.video_parameter_set_id = bs.GetBits(4);
        sps.max_sub_layers_minus1 = bs.GetBits(3);
        sps.temporal_id_nesting_flag = bs.GetBit();

        sps.general.profile_present_flag = 1;
        sps.general.level_present_flag = 1;

        sts = ParsePTL(bs, sps.general, m_needRextConstraints);
        MFX_CHECK_STS(sts);

        std::for_each(sps.sub_layer, sps.sub_layer + sps.max_sub_layers_minus1
            , [&](decltype(*sps.sub_layer) sl)
        {
            sl.profile_present_flag = bs.GetBit();
            sl.level_present_flag = bs.GetBit();
        });

        MFX_CHECK(!(sps.max_sub_layers_minus1 && bs.GetBits(2 * 8)) // reserved_zero_2bits[ i ]
            , MFX_ERR_INVALID_VIDEO_PARAM);

        auto ParsePTLWrap = [&](PTL& ptl) { return Res2Bool(sts, ParsePTL(bs, ptl, m_needRextConstraints)); };
        bool bErr = std::any_of(sps.sub_layer, sps.sub_layer + sps.max_sub_layers_minus1, ParsePTLWrap);
        MFX_CHECK(!bErr, sts);

        sps.seq_parameter_set_id = bs.GetUE();
        sps.chroma_format_idc = bs.GetUE();

        sps.separate_colour_plane_flag = (sps.chroma_format_idc == 3) && bs.GetBit();

        sps.pic_width_in_luma_samples = bs.GetUE();
        sps.pic_height_in_luma_samples = bs.GetUE();

        sps.conformance_window_flag = bs.GetBit();

        sps.conf_win_left_offset    = bs.CondUE(sps.conformance_window_flag, 0);
        sps.conf_win_right_offset   = bs.CondUE(sps.conformance_window_flag, 0);
        sps.conf_win_top_offset     = bs.CondUE(sps.conformance_window_flag, 0);
        sps.conf_win_bottom_offset  = bs.CondUE(sps.conformance_window_flag, 0);

        sps.bit_depth_luma_minus8 = bs.GetUE();
        sps.bit_depth_chroma_minus8 = bs.GetUE();
        sps.log2_max_pic_order_cnt_lsb_minus4 = bs.GetUE();

        sps.sub_layer_ordering_info_present_flag = bs.GetBit();

        std::for_each(
            sps.sub_layer + (sps.max_sub_layers_minus1 * sps.sub_layer_ordering_info_present_flag)
            , sps.sub_layer + sps.max_sub_layers_minus1 + 1
            , [&](decltype(*sps.sub_layer) sl)
        {
            sl.max_dec_pic_buffering_minus1 = bs.GetUE();
            sl.max_num_reorder_pics = bs.GetUE();
            sl.max_latency_increase_plus1 = bs.GetUE();
        });

        sps.log2_min_luma_coding_block_size_minus3 = bs.GetUE();
        sps.log2_diff_max_min_luma_coding_block_size = bs.GetUE();
        sps.log2_min_transform_block_size_minus2 = bs.GetUE();
        sps.log2_diff_max_min_transform_block_size = bs.GetUE();
        sps.max_transform_hierarchy_depth_inter = bs.GetUE();
        sps.max_transform_hierarchy_depth_intra = bs.GetUE();
        sps.scaling_list_enabled_flag = bs.GetBit();

        MFX_CHECK(!sps.scaling_list_enabled_flag, MFX_ERR_UNSUPPORTED);

        sps.amp_enabled_flag                    = bs.GetBit();
        sps.sample_adaptive_offset_enabled_flag = bs.GetBit();
        sps.pcm_enabled_flag                    = bs.GetBit();

        sps.pcm_sample_bit_depth_luma_minus1                = bs.CondBits(sps.pcm_enabled_flag, 4, 0);
        sps.pcm_sample_bit_depth_chroma_minus1              = bs.CondBits(sps.pcm_enabled_flag, 4, 0);
        sps.log2_min_pcm_luma_coding_block_size_minus3      = bs.CondUE(sps.pcm_enabled_flag, 0);
        sps.log2_diff_max_min_pcm_luma_coding_block_size    = bs.CondUE(sps.pcm_enabled_flag, 0);
        sps.pcm_loop_filter_disabled_flag                   = bs.CondBit(sps.pcm_enabled_flag, 0);

        sps.num_short_term_ref_pic_sets = (mfxU8)bs.GetUE();

        for (mfxU32 idx = 0; idx < sps.num_short_term_ref_pic_sets; idx++)
        {
            ParseSTRPS(bs, sps, idx);
        }

        sps.long_term_ref_pics_present_flag = bs.GetBit();
        sps.num_long_term_ref_pics_sps = bs.CondUE(sps.long_term_ref_pics_present_flag, 0);

        for (mfxU32 i = 0; i < sps.num_long_term_ref_pics_sps; i++)
        {
            sps.lt_ref_pic_poc_lsb_sps[i] = (mfxU16)bs.GetBits(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
            sps.used_by_curr_pic_lt_sps_flag[i] = (mfxU8)bs.GetBit();
        }

        sps.temporal_mvp_enabled_flag = bs.GetBit();
        sps.strong_intra_smoothing_enabled_flag = bs.GetBit();

        sps.vui_parameters_present_flag = bs.GetBit();

        bErr = sps.vui_parameters_present_flag && Res2Bool(sts, ParseVUI(bs, sps));
        MFX_CHECK(!bErr, sts);

        sps.extension_flag = bs.GetBit();
        MFX_CHECK(sps.extension_flag, sts);

        sps.range_extension_flag = bs.GetBit();
        sps.ExtensionFlags = mfxU8((sps.range_extension_flag << 7) + bs.GetBits(7));


        sps.transform_skip_rotation_enabled_flag    = bs.CondBit(sps.range_extension_flag, 0);
        sps.transform_skip_context_enabled_flag     = bs.CondBit(sps.range_extension_flag, 0);
        sps.implicit_rdpcm_enabled_flag             = bs.CondBit(sps.range_extension_flag, 0);
        sps.explicit_rdpcm_enabled_flag             = bs.CondBit(sps.range_extension_flag, 0);
        sps.extended_precision_processing_flag      = bs.CondBit(sps.range_extension_flag, 0);
        sps.intra_smoothing_disabled_flag           = bs.CondBit(sps.range_extension_flag, 0);
        sps.high_precision_offsets_enabled_flag     = bs.CondBit(sps.range_extension_flag, 0);
        sps.persistent_rice_adaptation_enabled_flag = bs.CondBit(sps.range_extension_flag, 0);
        sps.cabac_bypass_alignment_enabled_flag     = bs.CondBit(sps.range_extension_flag, 0);

        mfxU8 extFlags = (sps.ExtensionFlags << 1);
        MFX_CHECK(!(extFlags && !m_readSpsExt), MFX_ERR_UNSUPPORTED);
        MFX_CHECK(extFlags && m_readSpsExt, MFX_ERR_NONE);

        for (mfxU8 extId = 1; extFlags; extId++)
        {
            MFX_CHECK(!(extFlags & 0x80) || m_readSpsExt(sps, extId, bs), MFX_ERR_UNSUPPORTED);
            extFlags <<= 1;
        }
    }
    catch (std::exception &)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxStatus Parser::ParsePPS(BitstreamReader& bs, PPS & pps)
{
    NALU nalu = {};

    pps = PPS{};

    do
    {
        mfxStatus sts = ParseNALU(bs, nalu);
        MFX_CHECK_STS(sts);
    } while (nalu.nal_unit_type != PPS_NUT);

    try
    {
        pps.pic_parameter_set_id = bs.GetUE();
        pps.seq_parameter_set_id = bs.GetUE();
        pps.dependent_slice_segments_enabled_flag = bs.GetBit();
        pps.output_flag_present_flag = bs.GetBit();
        pps.num_extra_slice_header_bits = bs.GetBits(3);
        pps.sign_data_hiding_enabled_flag = bs.GetBit();
        pps.cabac_init_present_flag = bs.GetBit();
        pps.num_ref_idx_l0_default_active_minus1 = bs.GetUE();
        pps.num_ref_idx_l1_default_active_minus1 = bs.GetUE();
        pps.init_qp_minus26 = bs.GetSE();
        pps.constrained_intra_pred_flag = bs.GetBit();
        pps.transform_skip_enabled_flag = bs.GetBit();
        pps.cu_qp_delta_enabled_flag = bs.GetBit();

        pps.diff_cu_qp_delta_depth = bs.CondUE(pps.cu_qp_delta_enabled_flag, 0);

        pps.cb_qp_offset = bs.GetSE();
        pps.cr_qp_offset = bs.GetSE();
        pps.slice_chroma_qp_offsets_present_flag = bs.GetBit();
        pps.weighted_pred_flag = bs.GetBit();
        pps.weighted_bipred_flag = bs.GetBit();
        pps.transquant_bypass_enabled_flag = bs.GetBit();
        pps.tiles_enabled_flag = bs.GetBit();
        pps.entropy_coding_sync_enabled_flag = bs.GetBit();

        pps.num_tile_columns_minus1 = (mfxU16)bs.CondUE(pps.tiles_enabled_flag, 0);
        pps.num_tile_rows_minus1 = (mfxU16)bs.CondUE(pps.tiles_enabled_flag, 0);
        pps.uniform_spacing_flag = bs.CondBit(pps.tiles_enabled_flag, 1);

        std::generate_n(
            pps.column_width
            , pps.num_tile_columns_minus1 * !pps.uniform_spacing_flag
            , [&]() { return mfxU16(bs.GetUE() + 1); });

        std::generate_n(
            pps.row_height
            , pps.num_tile_rows_minus1 * !pps.uniform_spacing_flag
            , [&]() { return mfxU16(bs.GetUE() + 1); });

        pps.loop_filter_across_tiles_enabled_flag = bs.CondBit(pps.tiles_enabled_flag, 0);

        pps.loop_filter_across_slices_enabled_flag = bs.GetBit();
        pps.deblocking_filter_control_present_flag = bs.GetBit();

        pps.deblocking_filter_override_enabled_flag = bs.CondBit(pps.deblocking_filter_control_present_flag, 0);
        pps.deblocking_filter_disabled_flag = bs.CondBit(pps.deblocking_filter_control_present_flag, 0);

        bool bDblkOffsets = pps.deblocking_filter_control_present_flag && !pps.deblocking_filter_disabled_flag;
        pps.beta_offset_div2 = bs.CondSE(bDblkOffsets, 0);
        pps.tc_offset_div2 = bs.CondSE(bDblkOffsets, 0);

        pps.scaling_list_data_present_flag = bs.GetBit();
        MFX_CHECK(!pps.scaling_list_data_present_flag, MFX_ERR_UNSUPPORTED);

        pps.lists_modification_present_flag = bs.GetBit();
        pps.log2_parallel_merge_level_minus2 = (mfxU16)bs.GetUE();
        pps.slice_segment_header_extension_present_flag = bs.GetBit();

        pps.extension_flag = bs.GetBit();

        MFX_CHECK(pps.extension_flag, MFX_ERR_NONE);

        pps.range_extension_flag = bs.GetBit();
        pps.ExtensionFlags = mfxU8((pps.range_extension_flag << 7) + bs.GetBits(7));

        pps.log2_max_transform_skip_block_size_minus2 = bs.CondUE(
            pps.range_extension_flag && pps.transform_skip_enabled_flag, 0);

        pps.cross_component_prediction_enabled_flag = bs.CondBit(pps.range_extension_flag, 0);
        pps.chroma_qp_offset_list_enabled_flag = bs.CondBit(pps.range_extension_flag, 0);

        pps.diff_cu_chroma_qp_offset_depth = bs.CondUE(pps.chroma_qp_offset_list_enabled_flag, 0);
        pps.chroma_qp_offset_list_len_minus1 = bs.CondUE(pps.chroma_qp_offset_list_enabled_flag, 0);

        mfxU32 nChromaQpOffsets = (pps.chroma_qp_offset_list_len_minus1 + 1) * pps.chroma_qp_offset_list_enabled_flag;
        for (mfxU32 i = 0; i < nChromaQpOffsets; i++)
        {
            pps.cb_qp_offset_list[i] = (mfxI8)bs.GetSE();
            pps.cr_qp_offset_list[i] = (mfxI8)bs.GetSE();
        }

        pps.log2_sao_offset_scale_luma = bs.CondUE(pps.range_extension_flag, 0);
        pps.log2_sao_offset_scale_chroma = bs.CondUE(pps.range_extension_flag, 0);

        mfxU8 extFlags = (pps.ExtensionFlags << 1);
        MFX_CHECK(!extFlags || m_readPpsExt, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(extFlags && m_readPpsExt, MFX_ERR_NONE);

        for (mfxU8 extId = 1; extFlags; extId++)
        {
            bool bExtSts = !(extFlags & 0x80) || m_readPpsExt(pps, extId, bs);
            MFX_CHECK(bExtSts, MFX_ERR_UNSUPPORTED);
            extFlags <<= 1;
        }
    }
    catch (std::exception &)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

const mfxU8 AspectRatioByIdc[][2] =
{
    {  0,  0}, {  1,  1}, { 12, 11}, { 10, 11},
    { 16, 11}, { 40, 33}, { 24, 11}, { 20, 11},
    { 32, 11}, { 80, 33}, { 18, 11}, { 15, 11},
    { 64, 33}, {160, 99}, {  4,  3}, {  3,  2},
    {  2,  1},
};

mfxStatus Parser::SpsToMFX(const SPS& sps, mfxVideoParam& par)
{
    mfxExtHEVCParam* pHEVCParam = ExtBuffer::Get(par);
    mfxExtCodingOption* pCO = ExtBuffer::Get(par);
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
    mfxFrameInfo& fi = par.mfx.FrameInfo;
    mfxU16 SubWidthC[4] = { 1, 2, 2, 1 };
    mfxU16 SubHeightC[4] = { 1, 2, 1, 1 };
    mfxU16 cropUnitX = SubWidthC[sps.chroma_format_idc];
    mfxU16 cropUnitY = SubHeightC[sps.chroma_format_idc];

    par.mfx.CodecProfile = sps.general.profile_idc;
    par.mfx.CodecLevel = sps.general.level_idc / 3;

    par.mfx.CodecLevel |= (sps.general.tier_flag * MFX_TIER_HEVC_HIGH);

    if (pHEVCParam)
    {
        pHEVCParam->GeneralConstraintFlags = sps.general.rext_constraint_flags_0_31;
        pHEVCParam->GeneralConstraintFlags |= ((mfxU64)sps.general.rext_constraint_flags_32_42 << 32);

        pHEVCParam->PicWidthInLumaSamples = (mfxU16)sps.pic_width_in_luma_samples;
        pHEVCParam->PicHeightInLumaSamples = (mfxU16)sps.pic_height_in_luma_samples;
    }

    par.mfx.NumRefFrame = sps.sub_layer[0].max_dec_pic_buffering_minus1;

    par.mfx.FrameInfo.ChromaFormat = sps.chroma_format_idc;
    fi.BitDepthLuma = sps.bit_depth_luma_minus8 + 8;
    fi.BitDepthChroma = sps.bit_depth_chroma_minus8 + 8;

    if (pCO3)
    {
        pCO3->TargetChromaFormatPlus1 = 1 + sps.chroma_format_idc;
        pCO3->TargetBitDepthLuma = sps.bit_depth_luma_minus8 + 8;
        pCO3->TargetBitDepthChroma = sps.bit_depth_chroma_minus8 + 8;
    }

    fi.Width  = mfx::align2_value((mfxU16)sps.pic_width_in_luma_samples, HW_SURF_ALIGN_W);
    fi.Height = mfx::align2_value((mfxU16)sps.pic_height_in_luma_samples, HW_SURF_ALIGN_H);

    fi.CropX = 0;
    fi.CropY = 0;
    fi.CropW = fi.Width;
    fi.CropH = fi.Height;

    fi.CropX += cropUnitX * (mfxU16)sps.conf_win_left_offset;
    fi.CropW -= cropUnitX * (mfxU16)sps.conf_win_left_offset;
    fi.CropW -= cropUnitX * (mfxU16)sps.conf_win_right_offset;
    fi.CropY += cropUnitY * (mfxU16)sps.conf_win_top_offset;
    fi.CropH -= cropUnitY * (mfxU16)sps.conf_win_top_offset;
    fi.CropH -= cropUnitY * (mfxU16)sps.conf_win_bottom_offset;

    MFX_CHECK(sps.vui_parameters_present_flag, MFX_ERR_NONE);

    auto& vui = sps.vui;

    bool bArFromSarWH = vui.aspect_ratio_info_present_flag && vui.aspect_ratio_idc == 255;
    SetIf(fi.AspectRatioW, bArFromSarWH, vui.sar_width);
    SetIf(fi.AspectRatioH, bArFromSarWH, vui.sar_height);

    bool bArFromIdc =
        vui.aspect_ratio_info_present_flag
        && !bArFromSarWH
        && vui.aspect_ratio_idc < Size(AspectRatioByIdc);
    auto ArIdc = vui.aspect_ratio_idc * bArFromIdc;
    SetIf(fi.AspectRatioW, bArFromIdc, AspectRatioByIdc[ArIdc][0]);
    SetIf(fi.AspectRatioH, bArFromIdc, AspectRatioByIdc[ArIdc][1]);

    SetIf(fi.FrameRateExtN, vui.timing_info_present_flag, vui.time_scale);
    SetIf(fi.FrameRateExtD, vui.timing_info_present_flag, vui.num_units_in_tick);

    cropUnitX *= vui.default_display_window_flag;
    cropUnitY *= vui.default_display_window_flag;
    fi.CropX += cropUnitX * (mfxU16)vui.def_disp_win_left_offset;
    fi.CropW -= cropUnitX * (mfxU16)vui.def_disp_win_left_offset;
    fi.CropW -= cropUnitX * (mfxU16)vui.def_disp_win_right_offset;
    fi.CropY += cropUnitY * (mfxU16)vui.def_disp_win_top_offset;
    fi.CropH -= cropUnitY * (mfxU16)vui.def_disp_win_top_offset;
    fi.CropH -= cropUnitY * (mfxU16)vui.def_disp_win_bottom_offset;

    MFX_CHECK(sps.vui.hrd_parameters_present_flag, MFX_ERR_NONE);

    if (pCO)
        pCO->VuiNalHrdParameters = MFX_CODINGOPTION_ON;

    auto& hrd = sps.vui.hrd;
    auto& sl0 = hrd.sl[0];
    auto& cpb0 = sl0.cpb[0];

    MaxKbps(par.mfx) = ((cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale)) / 1000;
    BufferSizeInKB(par.mfx) = ((cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale)) / 8000;

    par.mfx.TargetKbps = par.mfx.MaxKbps;

    par.mfx.RateControlMethod =
        MFX_RATECONTROL_CBR * cpb0.cbr_flag
        + MFX_RATECONTROL_VBR * !cpb0.cbr_flag;

    return MFX_ERR_NONE;
}

mfxStatus Parser::PpsToMFX(const PPS& pps, mfxVideoParam& par)
{
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

    if (pCO3)
    {
        std::fill_n(pCO3->NumRefActiveP, 8, mfxU16(pps.num_ref_idx_l0_default_active_minus1 + 1));
        std::fill_n(pCO3->NumRefActiveBL1, 8, mfxU16(pps.num_ref_idx_l1_default_active_minus1 + 1));

        pCO3->TransformSkip = (pps.transform_skip_enabled_flag
            ? (mfxU16)MFX_CODINGOPTION_ON 
            : (mfxU16)MFX_CODINGOPTION_OFF);
    }

    mfxExtHEVCTiles* pTiles    = ExtBuffer::Get(par);
    bool             bSetTiles = pps.tiles_enabled_flag && pTiles;

    if (bSetTiles)
    {
        pTiles->NumTileColumns = pps.num_tile_columns_minus1 + 1;
        pTiles->NumTileRows    = pps.num_tile_rows_minus1 + 1;
    }

    return MFX_ERR_NONE;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
