// Copyright (c) 2019 Intel Corporation
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
#include "hevcehw_g9_va_packer_lin.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
#include "mfx_common_int.h"
#include "hevcehw_g9_va_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Gen9;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Gen9;

void VAPacker::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_HardcodeCaps
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        auto&       caps     = Glob::EncodeCaps::Get(strg);
        eMFXHWType  platform = Glob::VideoCore::Get(strg).GetHWType();
        auto        vaGUID   = DDI_VA::MapGUID(strg, Glob::GUID::Get(strg));

        HardcodeCapsCommon(caps, platform, par);

        bool bGen9 = (platform < MFX_HW_CNL);
        bool bLP   = (vaGUID.Entrypoint == VAEntrypointEncSliceLP);

        caps.LCUSizeSupported |=
              (32 >> 4) * ( bGen9 || (!bGen9 && !bLP))
            + (64 >> 4) * (!bGen9);

        caps.BRCReset                   = 1; // no bitrate resolution control
        caps.BlockSize                  = 2;
        caps.UserMaxFrameSizeSupport    = 1;
        caps.MbQpDataSupport            = 1;
        caps.TUSupport                  = 73;
        caps.SliceStructure             = 4;

        caps.MaxEncodedBitDepth |= (!caps.BitDepth8Only);
        caps.YUV444ReconSupport |= (!caps.Color420Only && IsOn(par.mfx.LowPower));
        caps.YUV422ReconSupport |= (!caps.Color420Only && IsOff(par.mfx.LowPower));

        return MFX_ERR_NONE;
    });

}

void InitSPS(
    const ExtBuffer::Param<mfxVideoParam>& par
    , const SPS& bs_sps
    , VAEncSequenceParameterBufferHEVC & sps)
{
    sps = {};

    sps.general_profile_idc = bs_sps.general.profile_idc;
    sps.general_level_idc   = bs_sps.general.level_idc;
    sps.general_tier_flag   = bs_sps.general.tier_flag;

    sps.intra_period        = par.mfx.GopPicSize;
    sps.intra_idr_period    = par.mfx.GopPicSize * par.mfx.IdrInterval;
    sps.ip_period           = par.mfx.GopRefDist;

    if (   par.mfx.RateControlMethod != MFX_RATECONTROL_CQP
        && par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ
        && par.mfx.RateControlMethod != MFX_RATECONTROL_LA_EXT)
    {
        sps.bits_per_second = TargetKbps(par.mfx) * 1000;
    }

    sps.pic_width_in_luma_samples  = (uint16_t)bs_sps.pic_width_in_luma_samples;
    sps.pic_height_in_luma_samples = (uint16_t)bs_sps.pic_height_in_luma_samples;

    sps.seq_fields.bits.chroma_format_idc                   = bs_sps.chroma_format_idc;
    sps.seq_fields.bits.separate_colour_plane_flag          = bs_sps.separate_colour_plane_flag;
    sps.seq_fields.bits.bit_depth_luma_minus8               = bs_sps.bit_depth_luma_minus8;
    sps.seq_fields.bits.bit_depth_chroma_minus8             = bs_sps.bit_depth_chroma_minus8;
    sps.seq_fields.bits.scaling_list_enabled_flag           = bs_sps.scaling_list_enabled_flag;
    sps.seq_fields.bits.strong_intra_smoothing_enabled_flag = bs_sps.strong_intra_smoothing_enabled_flag;
    sps.seq_fields.bits.amp_enabled_flag                    = bs_sps.amp_enabled_flag;
    sps.seq_fields.bits.sample_adaptive_offset_enabled_flag = bs_sps.sample_adaptive_offset_enabled_flag;
    sps.seq_fields.bits.pcm_enabled_flag                    = bs_sps.pcm_enabled_flag;
    sps.seq_fields.bits.pcm_loop_filter_disabled_flag       = 1;//bs_sps.pcm_loop_filter_disabled_flag;
    sps.seq_fields.bits.sps_temporal_mvp_enabled_flag       = bs_sps.temporal_mvp_enabled_flag;

    sps.log2_min_luma_coding_block_size_minus3 = (mfxU8)bs_sps.log2_min_luma_coding_block_size_minus3;

    sps.log2_diff_max_min_luma_coding_block_size   = (mfxU8)bs_sps.log2_diff_max_min_luma_coding_block_size;
    sps.log2_min_transform_block_size_minus2       = (mfxU8)bs_sps.log2_min_transform_block_size_minus2;
    sps.log2_diff_max_min_transform_block_size     = (mfxU8)bs_sps.log2_diff_max_min_transform_block_size;
    sps.max_transform_hierarchy_depth_inter        = (mfxU8)bs_sps.max_transform_hierarchy_depth_inter;
    sps.max_transform_hierarchy_depth_intra        = (mfxU8)bs_sps.max_transform_hierarchy_depth_intra;
    sps.pcm_sample_bit_depth_luma_minus1           = (mfxU8)bs_sps.pcm_sample_bit_depth_luma_minus1;
    sps.pcm_sample_bit_depth_chroma_minus1         = (mfxU8)bs_sps.pcm_sample_bit_depth_chroma_minus1;
    sps.log2_min_pcm_luma_coding_block_size_minus3 = (mfxU8)bs_sps.log2_min_pcm_luma_coding_block_size_minus3;
    sps.log2_max_pcm_luma_coding_block_size_minus3 = (mfxU8)(bs_sps.log2_min_pcm_luma_coding_block_size_minus3
        + bs_sps.log2_diff_max_min_pcm_luma_coding_block_size);

    sps.vui_parameters_present_flag                             = bs_sps.vui_parameters_present_flag;
    sps.vui_fields.bits.aspect_ratio_info_present_flag          = bs_sps.vui.aspect_ratio_info_present_flag;
    sps.vui_fields.bits.neutral_chroma_indication_flag          = bs_sps.vui.neutral_chroma_indication_flag;
    sps.vui_fields.bits.field_seq_flag                          = bs_sps.vui.field_seq_flag;
    sps.vui_fields.bits.vui_timing_info_present_flag            = bs_sps.vui.timing_info_present_flag;
    sps.vui_fields.bits.bitstream_restriction_flag              = bs_sps.vui.bitstream_restriction_flag;
    sps.vui_fields.bits.tiles_fixed_structure_flag              = bs_sps.vui.tiles_fixed_structure_flag;
    sps.vui_fields.bits.motion_vectors_over_pic_boundaries_flag = bs_sps.vui.motion_vectors_over_pic_boundaries_flag;
    sps.vui_fields.bits.restricted_ref_pic_lists_flag           = bs_sps.vui.restricted_ref_pic_lists_flag;
    sps.vui_fields.bits.log2_max_mv_length_horizontal           = bs_sps.vui.log2_max_mv_length_horizontal;
    sps.vui_fields.bits.log2_max_mv_length_vertical             = bs_sps.vui.log2_max_mv_length_vertical;


    sps.aspect_ratio_idc             = bs_sps.vui.aspect_ratio_idc;
    sps.sar_width                    = bs_sps.vui.sar_width;
    sps.sar_height                   = bs_sps.vui.sar_height;
    sps.vui_num_units_in_tick        = bs_sps.vui.num_units_in_tick;
    sps.vui_time_scale               = bs_sps.vui.time_scale;
    sps.min_spatial_segmentation_idc = bs_sps.vui.min_spatial_segmentation_idc;
    sps.max_bytes_per_pic_denom      = bs_sps.vui.max_bytes_per_pic_denom;
    sps.max_bits_per_min_cu_denom    = bs_sps.vui.max_bits_per_min_cu_denom;
}

void InitPPS(
    const ExtBuffer::Param<mfxVideoParam>& /*par*/
    , const PPS& bs_pps
    , VAEncPictureParameterBufferHEVC & pps)
{
    pps = {};

    std::for_each(
        std::begin(pps.reference_frames)
        , std::end(pps.reference_frames)
        , [](decltype(*std::begin(pps.reference_frames)) ref) { ref.picture_id = VA_INVALID_ID; });

    pps.last_picture            = 0;
    pps.pic_init_qp             = (mfxU8)(bs_pps.init_qp_minus26 + 26);
    pps.diff_cu_qp_delta_depth  = (mfxU8)bs_pps.diff_cu_qp_delta_depth;
    pps.pps_cb_qp_offset        = (mfxU8)bs_pps.cb_qp_offset;
    pps.pps_cr_qp_offset        = (mfxU8)bs_pps.cr_qp_offset;
    pps.num_tile_columns_minus1 = (mfxU8)bs_pps.num_tile_columns_minus1;
    pps.num_tile_rows_minus1    = (mfxU8)bs_pps.num_tile_rows_minus1;

    auto NumTileDdiToVa = [](mfxU16 n) { return (uint8_t)std::max<int>(0, n - 1); };

    std::transform(
        bs_pps.column_width
        , bs_pps.column_width + pps.num_tile_columns_minus1 + 1u
        , pps.column_width_minus1
        , NumTileDdiToVa);
    std::transform(
        bs_pps.row_height
        , bs_pps.row_height + pps.num_tile_rows_minus1 + 1u
        , pps.row_height_minus1
        , NumTileDdiToVa);

    pps.log2_parallel_merge_level_minus2     = (mfxU8)bs_pps.log2_parallel_merge_level_minus2;
    pps.ctu_max_bitsize_allowed              = 0;
    pps.num_ref_idx_l0_default_active_minus1 = (mfxU8)bs_pps.num_ref_idx_l0_default_active_minus1;
    pps.num_ref_idx_l1_default_active_minus1 = (mfxU8)bs_pps.num_ref_idx_l1_default_active_minus1;
    pps.slice_pic_parameter_set_id           = 0;

    pps.pic_fields.bits.dependent_slice_segments_enabled_flag      = bs_pps.dependent_slice_segments_enabled_flag;
    pps.pic_fields.bits.sign_data_hiding_enabled_flag              = bs_pps.sign_data_hiding_enabled_flag;
    pps.pic_fields.bits.constrained_intra_pred_flag                = bs_pps.constrained_intra_pred_flag;
    pps.pic_fields.bits.transform_skip_enabled_flag                = bs_pps.transform_skip_enabled_flag;
    pps.pic_fields.bits.cu_qp_delta_enabled_flag                   = bs_pps.cu_qp_delta_enabled_flag;
    pps.pic_fields.bits.weighted_pred_flag                         = bs_pps.weighted_pred_flag;
    pps.pic_fields.bits.weighted_bipred_flag                       = bs_pps.weighted_bipred_flag;
    pps.pic_fields.bits.transquant_bypass_enabled_flag             = bs_pps.transquant_bypass_enabled_flag;
    pps.pic_fields.bits.tiles_enabled_flag                         = bs_pps.tiles_enabled_flag;
    pps.pic_fields.bits.entropy_coding_sync_enabled_flag           = bs_pps.entropy_coding_sync_enabled_flag;
    pps.pic_fields.bits.loop_filter_across_tiles_enabled_flag      = bs_pps.loop_filter_across_tiles_enabled_flag;
    pps.pic_fields.bits.pps_loop_filter_across_slices_enabled_flag = bs_pps.loop_filter_across_slices_enabled_flag;
    pps.pic_fields.bits.scaling_list_data_present_flag             = bs_pps.scaling_list_data_present_flag;
    pps.pic_fields.bits.screen_content_flag                        = 0;
    pps.pic_fields.bits.enable_gpu_weighted_prediction             = 0;
    pps.pic_fields.bits.no_output_of_prior_pics_flag               = 0;
}

void InitSSH(
    const ExtBuffer::Param<mfxVideoParam>& /*par*/
    , Glob::SliceInfo::TRef sinfo
    , std::vector<VAEncSliceParameterBufferHEVC> & slices)
{
    slices.resize(sinfo.size());
    std::transform(sinfo.begin(), sinfo.end(), slices.begin()
        , [](const SliceInfo& si)
    {
        VAEncSliceParameterBufferHEVC s = {};
        s.slice_segment_address = si.SegmentAddress;
        s.num_ctu_in_slice = si.NumLCU;
        return s;
    });

    if (!slices.empty())
        slices.rbegin()->slice_fields.bits.last_slice_of_pic_flag = 1;
}

void AddVaMiscHRD(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    auto& hrd = AddVaMisc<VAEncMiscParameterHRD>(VAEncMiscParameterTypeHRD, buf);

    uint32_t bNeedBufParam =
        par.mfx.RateControlMethod != MFX_RATECONTROL_CQP
        && par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ;

    hrd.initial_buffer_fullness = bNeedBufParam * InitialDelayInKB(par.mfx) * 8000;
    hrd.buffer_size             = bNeedBufParam * BufferSizeInKB(par.mfx) * 8000;
}

void AddVaMiscRC(
    const Glob::VideoParam::TRef& par
    , const Glob::PPS::TRef& bs_pps
    , std::list<std::vector<mfxU8>>& buf
    , bool bResetBRC = false)
{
    auto& rc = AddVaMisc<VAEncMiscParameterRateControl>(VAEncMiscParameterTypeRateControl, buf);

    uint32_t bNeedRateParam =
            par.mfx.RateControlMethod != MFX_RATECONTROL_CQP
        && par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ
        && par.mfx.RateControlMethod != MFX_RATECONTROL_LA_EXT;

    rc.bits_per_second = bNeedRateParam * MaxKbps(par.mfx) * 1000;

    if (rc.bits_per_second)
        rc.target_percentage = uint32_t(100.0 * (mfxF64)TargetKbps(par.mfx) / (mfxF64)MaxKbps(par.mfx));

    rc.rc_flags.bits.reset = bNeedRateParam && bResetBRC;

    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);

#ifdef PARALLEL_BRC_support
    rc.rc_flags.bits.enable_parallel_brc =
        bNeedRateParam
        && (par.AsyncDepth > 1)
        && (par.mfx.GopRefDist > 1)
        && (par.mfx.GopRefDist <= 8)
        && (CO2.BRefType == MFX_B_REF_PYRAMID);
#endif //PARALLEL_BRC_support

    rc.ICQ_quality_factor = uint32_t((par.mfx.RateControlMethod == MFX_RATECONTROL_ICQ) * par.mfx.ICQQuality);
    rc.initial_qp = bs_pps.init_qp_minus26 + 26;

    //  MBBRC control
    // Control VA_RC_MB 0: default, 1: enable, 2: disable, other: reserved
    rc.rc_flags.bits.mb_rate_control = IsOn(CO2.MBBRC) + IsOff(CO2.MBBRC) * 2;

#ifdef MFX_ENABLE_QVBR
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);

    rc.quality_factor = uint32_t((par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR) * CO3.QVBRQuality);
#endif
}

void AddVaMiscPBRC(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
#ifdef PARALLEL_BRC_support
    auto& pbrc = AddVaMisc<VAEncMiscParameterParallelRateControl>(
        VAEncMiscParameterTypeParallelBRC, buf);
    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);

    if (CO2.BRefType != MFX_B_REF_PYRAMID)
    {
        pbrc.num_b_in_gop[0] = par.mfx.GopRefDist - 1;
        pbrc.num_b_in_gop[1] = 0;
        pbrc.num_b_in_gop[2] = 0;

        return;
    }

    if (par.mfx.GopRefDist <= 8)
    {
        static uint32_t B[9] = { 0,0,1,1,1,1,1,1,1 };
        static uint32_t B1[9] = { 0,0,0,1,2,2,2,2,2 };
        static uint32_t B2[9] = { 0,0,0,0,0,1,2,3,4 };

        mfxI32 numBpyr = par.mfx.GopPicSize / par.mfx.GopRefDist;
        mfxI32 lastBpyrW = par.mfx.GopPicSize%par.mfx.GopRefDist;

        pbrc.num_b_in_gop[0] = numBpyr * B[par.mfx.GopRefDist] + B[lastBpyrW];;
        pbrc.num_b_in_gop[1] = numBpyr * B1[par.mfx.GopRefDist] + B1[lastBpyrW];
        pbrc.num_b_in_gop[2] = numBpyr * B2[par.mfx.GopRefDist] + B2[lastBpyrW];

        return;
    }

    pbrc.num_b_in_gop[0] = 0;
    pbrc.num_b_in_gop[1] = 0;
    pbrc.num_b_in_gop[2] = 0;
#else
    std::ignore = par;
    std::ignore = buf;
#endif //PARALLEL_BRC_support
}

void AddVaMiscFR(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    auto& fr = AddVaMisc<VAEncMiscParameterFrameRate>(VAEncMiscParameterTypeFrameRate, buf);

    PackMfxFrameRate(par.mfx.FrameInfo.FrameRateExtN, par.mfx.FrameInfo.FrameRateExtD, fr.framerate);
}

void AddVaMiscTU(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    auto& tu = AddVaMisc<VAEncMiscParameterBufferQualityLevel>(VAEncMiscParameterTypeQualityLevel, buf);

    tu.quality_level = par.mfx.TargetUsage;
}

void AddVaMiscQualityParams(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
    auto& quality_param = AddVaMisc<VAEncMiscParameterEncQuality>(VAEncMiscParameterTypeEncQuality, buf);

    quality_param.PanicModeDisable = IsOff(CO3.BRCPanicMode);
}

void CUQPMap::Init (mfxU32 picWidthInLumaSamples, mfxU32 picHeightInLumaSamples)
{
    //16x32 only: driver limitation
    m_width        = (picWidthInLumaSamples  + 31) / 32;
    m_height       = (picHeightInLumaSamples + 31) / 32;
    m_pitch        = mfx::align2_value(m_width, 64);
    m_h_aligned    = mfx::align2_value(m_height, 4);
    m_block_width  = 32;
    m_block_height = 32;
    m_buffer.resize(m_pitch * m_h_aligned);
    std::fill(m_buffer.begin(), m_buffer.end(), mfxI8(0));
}

static bool FillCUQPDataVA(
    const Task::Common::TRef & task
    , const Glob::VideoParam::TRef &par
    , CUQPMap& cuqpMap)
{
    const mfxExtMBQP* mbqp = ExtBuffer::Get(task.ctrl);

    bool bInitialized =
        cuqpMap.m_width
        && cuqpMap.m_height
        && cuqpMap.m_block_width
        && cuqpMap.m_block_height;

    if (!bInitialized)
        return false;

    const mfxExtHEVCParam & HEVCParam = ExtBuffer::Get(par);

    mfxU32 drBlkW = cuqpMap.m_block_width;  // block size of driver
    mfxU32 drBlkH = cuqpMap.m_block_height;  // block size of driver
    mfxU16 inBlkSize = 16;                    //mbqp->BlockSize ? mbqp->BlockSize : 16;  //input block size

    mfxU32 inputW = CeilDiv(HEVCParam.PicWidthInLumaSamples, inBlkSize);
    mfxU32 inputH = CeilDiv(HEVCParam.PicHeightInLumaSamples, inBlkSize);

    bool bInvalid = !mbqp || !mbqp->QP || (mbqp->NumQPAlloc < inputW * inputH);
    if (bInvalid)
        return false;

    for (mfxU32 i = 0; i < cuqpMap.m_height; i++)
    {
        for (mfxU32 j = 0; j < cuqpMap.m_width; j++)
        {
            mfxU32 y = i * drBlkH / inBlkSize;
            mfxU32 x = j * drBlkW / inBlkSize;

            y = (y < inputH) ? y : inputH;
            x = (x < inputW) ? x : inputW;

            cuqpMap.m_buffer[i * cuqpMap.m_pitch + j] = mbqp->QP[y * inputW + x];
        }
    }

    return true;
}

void VAPacker::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_Init
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& core = Glob::VideoCore::Get(strg);

        const auto& par     = Glob::VideoParam::Get(strg);
        const auto& bs_sps  = Glob::SPS::Get(strg);
        const auto& bs_pps  = Glob::PPS::Get(strg);
        
        InitSPS(par, bs_sps, m_sps);
        InitPPS(par, bs_pps, m_pps);
        InitSSH(par, Glob::SliceInfo::Get(strg), m_slices);

        AddVaMiscHRD(par, m_vaPerSeqMiscData);
        AddVaMiscRC(par, bs_pps, m_vaPerSeqMiscData);
        AddVaMiscPBRC(par, m_vaPerSeqMiscData);
        AddVaMiscFR(par, m_vaPerSeqMiscData);
        AddVaMiscTU(par, m_vaPerSeqMiscData);
        AddVaMiscQualityParams(par, m_vaPerSeqMiscData);

        auto& vaInitPar = Tmp::DDI_InitParam::GetOrConstruct(local);

        std::for_each(m_vaPerSeqMiscData.begin(), m_vaPerSeqMiscData.end()
            , [&](decltype(*m_vaPerSeqMiscData.begin()) data)
        {
            DDIExecParam par = {};
            par.Function = VAEncMiscParameterBufferType;
            par.In.pData = data.data();
            par.In.Size  = (mfxU32)data.size();
            par.In.Num   = 1;

            vaInitPar.push_back(par);
        });

        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);

        if (IsOn(CO3.EnableMBQP))
        {
            const mfxExtHEVCParam& HEVCPar = ExtBuffer::Get(par);
            m_qpMap.Init(HEVCPar.PicWidthInLumaSamples, HEVCPar.PicHeightInLumaSamples);
        }

        auto MidToVA = [&core](mfxMemId mid)
        {
            VASurfaceID *pSurface = nullptr;
            mfxStatus sts = core.GetFrameHDL(mid, (mfxHDL*)&pSurface);
            ThrowIf(!!sts, sts);
            return *pSurface;
        };
        auto& recResponse = Glob::AllocRec::Get(strg).Response();
        auto& bsResponse = Glob::AllocBS::Get(strg).Response();

        m_rec.resize(recResponse.NumFrameActual, VA_INVALID_SURFACE);
        m_bs.resize(bsResponse.NumFrameActual, VA_INVALID_SURFACE);

        mfxStatus sts = Catch(
            MFX_ERR_NONE
            , std::transform<mfxMemId*, VASurfaceID*, std::function<VASurfaceID(mfxMemId)>>
            , recResponse.mids
            , recResponse.mids + recResponse.NumFrameActual
            , m_rec.data()
            , MidToVA);
        MFX_CHECK_STS(sts);

        sts = Catch(
            MFX_ERR_NONE
            , std::transform<mfxMemId*, VASurfaceID*, std::function<VASurfaceID(mfxMemId)>>
            , bsResponse.mids
            , bsResponse.mids + bsResponse.NumFrameActual
            , m_bs.data()
            , MidToVA);
        MFX_CHECK_STS(sts);

        auto& resources = Glob::DDI_Resources::GetOrConstruct(strg);
        DDIExecParam xPar = {};
        xPar.Function = MFX_FOURCC_NV12;
        xPar.Resource.pData = m_rec.data();
        xPar.Resource.Size = sizeof(VASurfaceID);
        xPar.Resource.Num = mfxU32(m_rec.size());
        resources.push_back(xPar);
        xPar.Function = MFX_FOURCC_P8;
        xPar.Resource.pData = m_bs.data();
        xPar.Resource.Size = sizeof(VABufferID);
        xPar.Resource.Num = mfxU32(m_bs.size());
        resources.push_back(xPar);

        m_feedback.resize(Glob::AllocBS::Get(strg).Response().NumFrameActual);

        Glob::DDI_SubmitParam::GetOrConstruct(strg);
        auto& fb = Glob::DDI_Feedback::GetOrConstruct(strg);
        fb.Out.pData = &m_feedbackTmp;
        fb.Out.Size = sizeof(VACodedBufferSegment);
        fb.Out.Num = 1;
        fb.Get = [this](mfxU32 id) -> void*
        {
            if (!FeedbackReady(id))
                return nullptr;
            return &m_feedback.at(FeedbackIDWrap(id));
        };
        fb.Update = [this](mfxU32 id)
        {
            MFX_CHECK(!FeedbackReady(id), MFX_ERR_UNDEFINED_BEHAVIOR);
            m_feedback[FeedbackIDWrap(id)] = m_feedbackTmp;
            m_feedbackTmp = {};
            FeedbackReady(id) = true;
            return MFX_ERR_NONE;
        };

        auto& cc = CC::GetOrConstruct(strg);
        cc.ReadFeedback.Push([](
                CallChains::TReadFeedback::TExt
                , const StorageR& global
                , StorageW& s_task
                , const VACodedBufferSegment& fb)
        {
            auto& bsInfo = Glob::AllocBS::Get(global).Info();

            MFX_CHECK(!(fb.status & VA_CODED_BUF_STATUS_BAD_BITSTREAM), MFX_ERR_GPU_HANG);
            MFX_CHECK(fb.buf && fb.size, MFX_ERR_DEVICE_FAILED);
            MFX_CHECK(mfxU32(bsInfo.Width * bsInfo.Height) >= fb.size, MFX_ERR_DEVICE_FAILED);

            Task::Common::Get(s_task).BsDataLength = fb.size;

            return MFX_ERR_NONE;
        });
        cc.FillCUQPData.Push([](
            CallChains::TFillCUQPData::TExt
            , const StorageR& global
            , const StorageR& s_task
            , CUQPMap& qpmap)
        {
            auto& task = Task::Common::Get(s_task);
            return task.bCUQPMap && FillCUQPDataVA(task, Glob::VideoParam::Get(global), qpmap);
        });
        cc.AddPerPicMiscData[VAEncMiscParameterTypeSkipFrame].Push([this](
            CallChains::TAddMiscData::TExt
            , const StorageR&
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            if (Task::Common::Get(s_task).SkipCMD & SKIPCMD_NeedNumSkipAdding)
            {
                auto& sf = AddVaMisc<VAEncMiscParameterSkipFrame>(VAEncMiscParameterTypeSkipFrame, data);
                sf.num_skip_frames  = uint8_t(m_numSkipFrames);
                sf.size_skip_frames = m_sizeSkipFrames;
                sf.skip_frame_flag  = !!m_numSkipFrames;

                return true;
            }

            return false;
        });

        return MFX_ERR_NONE;
    });
}

void VAPacker::ResetState(const FeatureBlocks& /*blocks*/, TPushRS Push)
{
    Push(BLK_Reset
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        const auto& par = Glob::VideoParam::Get(strg);
        const auto& bs_sps = Glob::SPS::Get(strg);
        const auto& bs_pps = Glob::PPS::Get(strg);

        InitSPS(par, bs_sps, m_sps);
        InitPPS(par, bs_pps, m_pps);
        InitSSH(par, Glob::SliceInfo::Get(strg), m_slices);

        m_vaPerSeqMiscData.clear();

        AddVaMiscHRD(par, m_vaPerSeqMiscData);
        AddVaMiscRC(par, bs_pps, m_vaPerSeqMiscData, !!(Glob::ResetHint::Get(strg).Flags & RF_BRC_RESET));
        AddVaMiscPBRC(par, m_vaPerSeqMiscData);
        AddVaMiscFR(par, m_vaPerSeqMiscData);
        AddVaMiscTU(par, m_vaPerSeqMiscData);
        AddVaMiscQualityParams(par, m_vaPerSeqMiscData);

        auto& vaInitPar = Tmp::DDI_InitParam::GetOrConstruct(local);
        vaInitPar.clear();

        std::for_each(m_vaPerSeqMiscData.begin(), m_vaPerSeqMiscData.end()
            , [&](decltype(*m_vaPerSeqMiscData.begin()) data)
        {
            DDIExecParam par = {};
            par.Function = VAEncMiscParameterBufferType;
            par.In.pData = data.data();
            par.In.Size = (mfxU32)data.size();
            par.In.Num = 1;

            vaInitPar.push_back(par);
        });

        return MFX_ERR_NONE;
    });
}

void UpdatePPS(
    const TaskCommonPar & task
    , const Slice & sh
    , const std::vector<VASurfaceID> & rec
    , VAEncPictureParameterBufferHEVC & pps)
{
    pps.pic_fields.bits.idr_pic_flag       = !!(task.FrameType & MFX_FRAMETYPE_IDR);
    pps.pic_fields.bits.coding_type        = task.CodingType;
    pps.pic_fields.bits.reference_pic_flag = !!(task.FrameType & MFX_FRAMETYPE_REF);

    pps.collocated_ref_pic_index = 0xff;
    if (sh.temporal_mvp_enabled_flag)
        pps.collocated_ref_pic_index = task.RefPicList[!sh.collocated_from_l0_flag][sh.collocated_ref_idx];

    pps.decoded_curr_pic.picture_id     = rec.at(task.Rec.Idx);
    pps.decoded_curr_pic.pic_order_cnt  = task.POC;
    pps.decoded_curr_pic.flags          = 0;

    auto pDpbBegin = task.DPB.Active;
    auto pDpbEnd   = task.DPB.Active + Size(task.DPB.Active);
    auto pDpbValidEnd = std::find_if(pDpbBegin, pDpbEnd
        , [](decltype(*pDpbBegin) ref) { return ref.Rec.Idx == IDX_INVALID; });

    std::transform(pDpbBegin, pDpbValidEnd, pps.reference_frames
        , [&](decltype(*pDpbBegin) ref)
    {
        VAPictureHEVC vaRef  = {};
        vaRef.picture_id     = rec.at(ref.Rec.Idx);
        vaRef.pic_order_cnt  = ref.POC;
        vaRef.flags          = VA_PICTURE_HEVC_LONG_TERM_REFERENCE * !!ref.isLTR;
        return vaRef;
    });

    VAPictureHEVC vaRefInvalid = {};
    vaRefInvalid.picture_id = VA_INVALID_SURFACE;
    vaRefInvalid.flags      = VA_PICTURE_HEVC_INVALID;

    std::fill(
        pps.reference_frames + (pDpbValidEnd - pDpbBegin)
        , pps.reference_frames + Size(pps.reference_frames)
        , vaRefInvalid);
}

void UpdateSlices(
    const TaskCommonPar & task
    , const Slice & sh
    , const VAEncPictureParameterBufferHEVC & pps
    , std::vector<VAEncSliceParameterBufferHEVC> & slices)
{
    std::for_each(slices.begin(), slices.end()
        , [&](VAEncSliceParameterBufferHEVC& slice)
    {
        slice.slice_type                 = sh.type;
        slice.slice_pic_parameter_set_id = pps.slice_pic_parameter_set_id;

        if (slice.slice_type != 2)
        {
            slice.num_ref_idx_l0_active_minus1 = task.NumRefActive[0] - 1;

            if (slice.slice_type == 0)
                slice.num_ref_idx_l1_active_minus1 = task.NumRefActive[1] - 1;
            else
                slice.num_ref_idx_l1_active_minus1 = 0;

        }
        else
            slice.num_ref_idx_l0_active_minus1 = slice.num_ref_idx_l1_active_minus1 = 0;


        VAPictureHEVC vaRefInvalid = {};
        vaRefInvalid.picture_id = VA_INVALID_SURFACE;
        vaRefInvalid.flags      = VA_PICTURE_HEVC_INVALID;

        auto GetRef = [&](mfxU8 idx)
        {
            if (idx < Size(pps.reference_frames))
                return pps.reference_frames[idx];
            return vaRefInvalid;
        };

        std::fill(slice.ref_pic_list0, slice.ref_pic_list0 + Size(slice.ref_pic_list0), vaRefInvalid);
        std::fill(slice.ref_pic_list1, slice.ref_pic_list1 + Size(slice.ref_pic_list1), vaRefInvalid);

        std::transform(
            task.RefPicList[0]
            , task.RefPicList[0] + task.NumRefActive[0]
            , slice.ref_pic_list0
            , GetRef);
        std::transform(
            task.RefPicList[1]
            , task.RefPicList[1] + task.NumRefActive[1]
            , slice.ref_pic_list1
            , GetRef);

        slice.max_num_merge_cand     = 5 - sh.five_minus_max_num_merge_cand;
        slice.slice_qp_delta         = sh.slice_qp_delta;
        slice.slice_cb_qp_offset     = sh.slice_cb_qp_offset;
        slice.slice_cr_qp_offset     = sh.slice_cr_qp_offset;
        slice.slice_beta_offset_div2 = sh.beta_offset_div2;
        slice.slice_tc_offset_div2   = sh.tc_offset_div2;

        slice.slice_fields.bits.dependent_slice_segment_flag          = sh.dependent_slice_segment_flag;
        slice.slice_fields.bits.slice_temporal_mvp_enabled_flag       = sh.temporal_mvp_enabled_flag;
        slice.slice_fields.bits.slice_sao_luma_flag                   = sh.sao_luma_flag;
        slice.slice_fields.bits.slice_sao_chroma_flag                 = sh.sao_chroma_flag;
        slice.slice_fields.bits.num_ref_idx_active_override_flag =
                slice.num_ref_idx_l0_active_minus1 != pps.num_ref_idx_l0_default_active_minus1 ||
                slice.num_ref_idx_l1_active_minus1 != pps.num_ref_idx_l1_default_active_minus1;
        slice.slice_fields.bits.mvd_l1_zero_flag                      = sh.mvd_l1_zero_flag;
        slice.slice_fields.bits.cabac_init_flag                       = sh.cabac_init_flag;
        slice.slice_fields.bits.slice_deblocking_filter_disabled_flag = sh.deblocking_filter_disabled_flag;
        slice.slice_fields.bits.collocated_from_l0_flag               = sh.collocated_from_l0_flag;
    });
}

bool AddPackedHeaderIf(
    bool cond
    , const PackedData pd
    , std::list<VAEncPackedHeaderParameterBuffer>& vaPhs
    , std::list<DDIExecParam>& par
    , uint32_t type = VAEncPackedHeaderRawData)
{
    if (!cond)
        return false;

    VAEncPackedHeaderParameterBuffer vaPh = {};
    vaPh.type                = type;
    vaPh.bit_length          = pd.BitLen;
    vaPh.has_emulation_bytes = pd.bHasEP;
    vaPhs.push_back(vaPh);

    DDIExecParam xPar;
    xPar.Function = VAEncPackedHeaderParameterBufferType;
    xPar.In.pData = &vaPhs.back();
    xPar.In.Size  = sizeof(VAEncPackedHeaderParameterBuffer);
    xPar.In.Num   = 1;
    par.push_back(xPar);

    xPar.Function = VAEncPackedHeaderDataBufferType;
    xPar.In.pData = pd.pData;
    xPar.In.Size  = pd.BitLen / 8;
    xPar.In.Num   = 1;
    par.push_back(xPar);

    return true;
}

void VAPacker::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SubmitTask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task      = Task::Common::Get(s_task);
        bool  bSkipCurr =
            !(task.SkipCMD & SKIPCMD_NeedDriverCall)
            && (task.SkipCMD & SKIPCMD_NeedCurrentFrameSkipping);

        m_numSkipFrames  += !!(task.SkipCMD & SKIPCMD_NeedNumSkipAdding) * task.ctrl.SkipFrame;
        m_numSkipFrames  += bSkipCurr;
        m_sizeSkipFrames += bSkipCurr * task.BsDataLength;

        MFX_CHECK(task.SkipCMD & SKIPCMD_NeedDriverCall, MFX_ERR_NONE);

        auto& sh = Task::SSH::Get(s_task);
        auto& ph = Glob::PackedHeaders::Get(global);

        if (task.RepackHeaders & INSERT_PPS)
        {
            InitPPS(Glob::VideoParam::Get(global), Glob::PPS::Get(global), m_pps);
        }

        UpdatePPS(task, sh, m_rec, m_pps);
        UpdateSlices(task, sh, m_pps, m_slices);

        MFX_CHECK(task.BS.Idx < m_bs.size(), MFX_ERR_UNKNOWN);
        m_pps.coded_buf = m_bs.at(task.BS.Idx);

        auto& cc  = CC::Get(global);
        auto& par = Glob::DDI_SubmitParam::Get(global);
        par.clear();

        auto AddBuffer = [&](VABufferType type, void* pData, mfxU32 size, mfxU32 num = 1)
        {
            DDIExecParam xPar = {};
            xPar.Function = type;
            xPar.In.pData = pData;
            xPar.In.Size  = size;
            xPar.In.Num   = num;
            par.push_back(xPar);
        };
        auto AddMiscBuffer = [&](std::vector<mfxU8>& misc)
        {
            AddBuffer(VAEncMiscParameterBufferType, misc.data(), (mfxU32)misc.size());
        };
        
        AddBuffer(VAEncSequenceParameterBufferType, &m_sps, sizeof(m_sps));
        AddBuffer(VAEncPictureParameterBufferType, &m_pps, sizeof(m_pps));

        std::for_each(m_slices.begin(), m_slices.end()
            , [&](VAEncSliceParameterBufferHEVC& s)
        {
            AddBuffer(VAEncSliceParameterBufferType, &s, sizeof(s));
        });

        if (cc.FillCUQPData(global, s_task, m_qpMap))
        {
            AddBuffer(
                VAEncQPBufferType
                , m_qpMap.m_buffer.data()
                , m_qpMap.m_pitch
                , m_qpMap.m_h_aligned);
        }

        m_vaPerPicMiscData.clear();
        m_vaPackedHeaders.clear();

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_AUD)
            , ph.AUD[mfx::clamp<mfxU8>(task.CodingType, 1, 3) - 1], m_vaPackedHeaders, par);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_VPS)
            , ph.VPS, m_vaPackedHeaders, par, VAEncPackedHeaderHEVC_VPS);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_SPS)
            , ph.SPS, m_vaPackedHeaders, par, VAEncPackedHeaderHEVC_SPS);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_PPS)
            , ph.PPS, m_vaPackedHeaders, par, VAEncPackedHeaderHEVC_PPS);

        AddPackedHeaderIf((!!(task.InsertHeaders & INSERT_SEI) || task.ctrl.NumPayload) && ph.PrefixSEI.BitLen
            , ph.PrefixSEI, m_vaPackedHeaders, par/*, VAEncPackedHeaderHEVC_SEI*/);

        std::for_each(
            ph.SSH.begin()
            , ph.SSH.begin() + std::min<size_t>(m_slices.size(), ph.SSH.size())
            , [&](PackedData& pd)
        {
            AddPackedHeaderIf(true, pd, m_vaPackedHeaders, par, VAEncPackedHeaderHEVC_Slice);
        });

        for (auto& AddMisc : cc.AddPerPicMiscData)
        {
            if (AddMisc.second(global, s_task, m_vaPerPicMiscData))
                AddMiscBuffer(m_vaPerPicMiscData.back());
        }

        m_numSkipFrames  = 0;
        m_sizeSkipFrames = 0;

        return MFX_ERR_NONE;
    });
}

void VAPacker::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_QueryTask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        MFX_CHECK(!Glob::DDI_Feedback::Get(global).bNotReady, MFX_TASK_BUSY);

        auto& task = Task::Common::Get(s_task);

        if (!(task.SkipCMD & SKIPCMD_NeedDriverCall))
            return MFX_ERR_NONE;

        MFX_CHECK(FeedbackReady(task.StatusReportId), MFX_TASK_BUSY);

        auto& fb    = m_feedback.at(FeedbackIDWrap(task.StatusReportId));
        auto& rtErr = Glob::RTErr::Get(global);

        auto sts = CC::Get(global).ReadFeedback(global, s_task, fb);
        SetIf(rtErr, sts < MFX_ERR_NONE, sts);

        FeedbackReady(task.StatusReportId) = false;

        return sts;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
