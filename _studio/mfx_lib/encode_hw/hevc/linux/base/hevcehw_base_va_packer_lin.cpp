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
#include "hevcehw_base_va_packer_lin.h"

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
#include "mfx_common_int.h"
#include "hevcehw_base_va_lin.h"
#include "mfx_session.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Base;

void VAPacker::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_HardcodeCaps
        , [this](const mfxVideoParam&, mfxVideoParam& par, StorageRW& strg) -> mfxStatus
    {
        auto&       caps     = Glob::EncodeCaps::Get(strg);
        eMFXHWType platform = Glob::VideoCore::Get(strg).GetHWType();
        auto        vaGUID   = DDI_VA::MapGUID(strg, Glob::GUID::Get(strg));

        HardcodeCapsCommon(caps, platform, par);

        bool bBase = (platform < MFX_HW_CNL);
        bool bLP   = (vaGUID.Entrypoint == VAEntrypointEncSliceLP);

        caps.LCUSizeSupported |=
              (32 >> 4) * ( bBase || (!bBase && !bLP))
            + (64 >> 4) * (!bBase);

        caps.BRCReset                   = 1; // no bitrate resolution control
        caps.BlockSize                  = 2;
        caps.MbQpDataSupport            = 1;
        caps.TUSupport                  = 73;
        caps.ParallelBRC                = bLP ? 0 : 1;

        caps.MaxEncodedBitDepth |= (!caps.BitDepth8Only);
        caps.YUV444ReconSupport |= (!caps.Color420Only && IsOn(par.mfx.LowPower));
        caps.YUV422ReconSupport &= (!caps.Color420Only && !IsOn(par.mfx.LowPower));

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

    sps.seq_fields.bits.low_delay_seq    = bs_sps.low_delay_mode;
    sps.seq_fields.bits.hierachical_flag = bs_sps.hierarchical_flag;

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

void InitPriority(
    const mfxU32& maxContextPriority,
    const mfxPriority& contextPriority,
    VAContextParameterUpdateBuffer& hevcPriorityBuffer)
{
    memset(&hevcPriorityBuffer, 0, sizeof(VAContextParameterUpdateBuffer));

    hevcPriorityBuffer.flags.bits.context_priority_update = 1;

    if(contextPriority == MFX_PRIORITY_LOW)
    {
        hevcPriorityBuffer.context_priority.bits.priority = 0;
    }
    else if (contextPriority == MFX_PRIORITY_HIGH)
    {
        hevcPriorityBuffer.context_priority.bits.priority = maxContextPriority;
    }
    else
    {
        hevcPriorityBuffer.context_priority.bits.priority = maxContextPriority/2;
    }
}

void AddVaMiscHRD(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    auto& hrd = AddVaMisc<VAEncMiscParameterHRD>(VAEncMiscParameterTypeHRD, buf);

    hrd.initial_buffer_fullness = InitialDelayInKB(par.mfx) * 8000;
    hrd.buffer_size             = BufferSizeInKB(par.mfx) * 8000;
}

void AddVaMiscRC(
    const Glob::VideoParam::TRef& par
    , const Glob::PPS::TRef& bs_pps
    , const Task::Common::TRef & task
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

    // TCBRC control
#if VA_CHECK_VERSION(1, 10, 0)
    rc.target_frame_size = task.TCBRCTargetFrameSize;
#endif

    if (IsOn(par.mfx.LowPower) && (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP))
    {
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
        rc.min_qp = task.m_minQP - 6*(CO3.TargetBitDepthLuma - 8);
        if ((rc.min_qp < 0) || (rc.min_qp > 51))
            rc.min_qp = 1;

        rc.max_qp = task.m_maxQP - 6*(CO3.TargetBitDepthLuma - 8);
        if ((rc.max_qp < 0) || (rc.max_qp > 51))
            rc.max_qp = 51;
    }

#ifdef MFX_ENABLE_QVBR
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
    if (CO3.WinBRCSize)
    {
        rc.rc_flags.bits.frame_tolerance_mode = 1; //sliding window
        rc.window_size = 1000;
        rc.bits_per_second = CO3.WinBRCMaxAvgKbps * 1000;
        rc.target_percentage = uint32_t(100.0 * (mfxF64)TargetKbps(par.mfx) / (mfxF64)CO3.WinBRCMaxAvgKbps);
    }

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

void AddVaMiscMaxSliceSize(
    const Glob::VideoParam::TRef& par
    , std::list<std::vector<mfxU8>>& buf)
{
    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
    auto& maxSliceSize_param = AddVaMisc<VAEncMiscParameterMaxSliceSize>(VAEncMiscParameterTypeMaxSliceSize, buf);

    maxSliceSize_param.max_slice_size = CO2.MaxSliceSize;
}

void CUQPMap::Init (mfxU32 picWidthInLumaSamples, mfxU32 picHeightInLumaSamples, mfxU32 blockSize)
{
    mfxU32 blkSz   = 8 << blockSize;
    m_width        = (picWidthInLumaSamples  + blkSz - 1) / blkSz;
    m_height       = (picHeightInLumaSamples + blkSz - 1) / blkSz;
    m_pitch        = mfx::align2_value(m_width, 64);
    m_h_aligned    = mfx::align2_value(m_height, 4);
    m_block_width  = blkSz;
    m_block_height = blkSz;
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

    for (mfxU32 i = 0; i < cuqpMap.m_h_aligned; i++)
    {
        for (mfxU32 j = 0; j < cuqpMap.m_pitch; j++)
        {
            mfxU32 y = std::min(i * drBlkH/inBlkSize, inputH - 1);
            mfxU32 x = std::min(j * drBlkW/inBlkSize, inputW - 1);

            cuqpMap.m_buffer[i * cuqpMap.m_pitch + j] = mbqp->QP[y * inputW + x];
        }
    }

    return true;
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
    pps.nal_unit_type                   = task.SliceNUT;

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

void VAPacker::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetCallChains
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        MFX_CHECK(!strg.Contains(CC::Key), MFX_ERR_NONE);

        const auto& par = Glob::VideoParam::Get(strg);
        auto& cc = CC::GetOrConstruct(strg);

        cc.InitSPS.Push([ &par](
            CallChains::TInitSPS::TExt
            , const StorageR& glob
            , VAEncSequenceParameterBufferHEVC& sps)
        {
            const auto& bs_sps = Glob::SPS::Get(glob);
            InitSPS(par, bs_sps, sps);
        });
        cc.UpdatePPS.Push([this](
            CallChains::TUpdatePPS::TExt
            , const StorageR& global
            , const StorageR& s_task
            , const VAEncSequenceParameterBufferHEVC& /*sps*/
            , VAEncPictureParameterBufferHEVC& pps)
        {
            UpdatePPS(Task::Common::Get(s_task), Task::SSH::Get(s_task), GetResources(RES_REF), pps);
        });

        return MFX_ERR_NONE;
    });
}

void VAPacker::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_Init
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& core = Glob::VideoCore::Get(strg);

        const auto& par     = Glob::VideoParam::Get(strg);
        const auto& bs_pps  = Glob::PPS::Get(strg);
        const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);

        auto& cc = CC::GetOrConstruct(strg);
        cc.InitSPS(strg, m_sps);

        InitPPS(par, bs_pps, m_pps);
        InitSSH(par, Glob::SliceInfo::Get(strg), m_slices);

        if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP &&
            par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ) {
            cc.AddPerSeqMiscData[VAEncMiscParameterTypeHRD].Push([this, &par](
                VAPacker::CallChains::TAddMiscData::TExt
                , const StorageR& strg
                , const StorageR& local
                , std::list<std::vector<mfxU8>>& data)
            {
                AddVaMiscHRD(par, m_vaPerSeqMiscData);
                return true;
            });
        }
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeParallelBRC].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscPBRC(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeFrameRate].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscFR(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeQualityLevel].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscTU(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeEncQuality].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscQualityParams(par, m_vaPerSeqMiscData);
            return true;
        });
        if (CO2.MaxSliceSize)
        {
	        cc.AddPerSeqMiscData[VAEncMiscParameterTypeMaxSliceSize].Push([this, &par](
		    VAPacker::CallChains::TAddMiscData::TExt
		    , const StorageR&
		    , const StorageR&
		    , std::list<std::vector<mfxU8>>& data)
	        {
		        AddVaMiscMaxSliceSize(par, m_vaPerSeqMiscData);
		        return true;
	        });
        }

        auto& vaInitPar = Tmp::DDI_InitParam::GetOrConstruct(local);
        auto& bsInfo    = Glob::AllocBS::Get(strg);

        SetMaxBs(bsInfo.GetInfo().Width * bsInfo.GetInfo().Height);

        for (auto& AddMisc : cc.AddPerSeqMiscData)
        {
            if (AddMisc.second(strg, local, m_vaPerSeqMiscData))
            {
                auto& misc = m_vaPerSeqMiscData.back();
                vaInitPar.push_back(PackVaMiscPar(misc));
            }
        }

        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);

        if (IsOn(CO3.EnableMBQP))
        {
            const mfxExtHEVCParam& HEVCPar = ExtBuffer::Get(par);
            const auto& caps = Glob::EncodeCaps::Get(strg);
            m_qpMap.Init(HEVCPar.PicWidthInLumaSamples, HEVCPar.PicHeightInLumaSamples, caps.BlockSize);
        }

        mfxStatus sts = Register(core, Glob::AllocRec::Get(strg).GetResponse(), RES_REF);
        MFX_CHECK_STS(sts);

        sts = Register(core, bsInfo.GetResponse(), RES_BS);
        MFX_CHECK_STS(sts)

        auto& resources = Glob::DDI_Resources::GetOrConstruct(strg);
        DDIExecParam xPar;

        xPar.Function = MFX_FOURCC_NV12;
        PackResources(xPar.Resource, RES_REF);
        resources.push_back(xPar);

        xPar.Function = MFX_FOURCC_P8;
        PackResources(xPar.Resource, RES_BS);
        resources.push_back(xPar);

        InitFeedback(bsInfo.GetResponse().NumFrameActual);

        GetFeedbackInterface(Glob::DDI_Feedback::GetOrConstruct(strg));

        Glob::DDI_SubmitParam::GetOrConstruct(strg);

        cc.ReadFeedback.Push([this](
                CallChains::TReadFeedback::TExt
                , const StorageR& /*global*/
                , StorageW& s_task
                , const VACodedBufferSegment& fb)
        {
            return ReadFeedback(fb, Task::Common::Get(s_task).BsDataLength);
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
	cc.AddPerPicMiscData[VAEncMiscParameterTypeRateControl].Push([this](
            CallChains::TAddMiscData::TExt
            , const StorageR& global
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            const auto& par    = Glob::VideoParam::Get(global);
            const auto& bs_pps = Glob::PPS::Get(global);
            auto& task = Task::Common::Get(s_task);

            AddVaMiscRC(par, bs_pps, task, data, !!(m_resetHintFlags & RF_BRC_RESET));
            return true;
        });
	cc.AddPerPicMiscData[VAEncMiscParameterTypeRIR].Push([](
                VAPacker::CallChains::TAddMiscData::TExt
                , const StorageR&
                , const StorageR& s_task
                , std::list<std::vector<mfxU8>>& data)
        {

	     auto& task = Task::Common::Get(s_task);

	     if (task.IRState.refrType)
	     {
                 auto& vaRIR = AddVaMisc<VAEncMiscParameterRIR>(VAEncMiscParameterTypeRIR, data);
                 vaRIR.rir_flags.value = task.IRState.refrType;
                 vaRIR.intra_insertion_location = task.IRState.IntraLocation;
                 vaRIR.intra_insert_size = task.IRState.IntraSize;
                 vaRIR.qp_delta_for_inserted_intra = mfxU8(task.IRState.IntRefQPDelta);
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
        const auto& bs_pps = Glob::PPS::Get(strg);
        const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);

        auto& cc = CC::GetOrConstruct(strg);
        cc.InitSPS(strg, m_sps);

        const auto& reset_hint = Glob::ResetHint::Get(strg);
        m_resetHintFlags = reset_hint.Flags;

        InitPPS(par, bs_pps, m_pps);
        InitSSH(par, Glob::SliceInfo::Get(strg), m_slices);

        m_vaPerSeqMiscData.clear();

        if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP &&
            par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ) {
            cc.AddPerSeqMiscData[VAEncMiscParameterTypeHRD].Push([this, &par](
                VAPacker::CallChains::TAddMiscData::TExt
                , const StorageR& strg
                , const StorageR& local
                , std::list<std::vector<mfxU8>>& data)
            {
                AddVaMiscHRD(par, m_vaPerSeqMiscData);
                return true;
            });
        }
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeParallelBRC].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscPBRC(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeFrameRate].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscFR(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeQualityLevel].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscTU(par, m_vaPerSeqMiscData);
            return true;
        });
        cc.AddPerSeqMiscData[VAEncMiscParameterTypeEncQuality].Push([this, &par](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& strg
            , const StorageR& local
            , std::list<std::vector<mfxU8>>& data)
        {
            AddVaMiscQualityParams(par, m_vaPerSeqMiscData);
            return true;
        });
        if (CO2.MaxSliceSize)
        {
	        cc.AddPerSeqMiscData[VAEncMiscParameterTypeMaxSliceSize].Push([this, &par](
		    VAPacker::CallChains::TAddMiscData::TExt
		    , const StorageR&
		    , const StorageR&
		    , std::list<std::vector<mfxU8>>& data)
	        {
		        AddVaMiscMaxSliceSize(par, m_vaPerSeqMiscData);
		        return true;
	        });
        }

        auto& vaInitPar = Tmp::DDI_InitParam::GetOrConstruct(local);
        vaInitPar.clear();

        for (auto& AddMisc : cc.AddPerSeqMiscData)
        {
            if (AddMisc.second(strg, local, m_vaPerSeqMiscData))
            {
                auto& misc = m_vaPerSeqMiscData.back();
                vaInitPar.push_back(PackVaMiscPar(misc));
            }
        }

        return MFX_ERR_NONE;
    });
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
            slice.num_ref_idx_l0_active_minus1 = task.NumRefActive[0]==0 ? 0 : task.NumRefActive[0] - 1;

            if (slice.slice_type == 0)
                slice.num_ref_idx_l1_active_minus1 = task.NumRefActive[1]==0 ? 0 : task.NumRefActive[1] - 1;
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

void VAPacker::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SubmitTask
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task      = Task::Common::Get(s_task);
        auto& core      = Glob::VideoCore::Get(global);
        const auto& priority_par = Glob::PriorityPar::Get(global);
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

        auto& cc = CC::Get(global);
        cc.UpdatePPS(global, s_task, m_sps, m_pps);
        UpdateSlices(task, sh, m_pps, m_slices);

        m_pps.coded_buf = GetResources(RES_BS).at(task.BS.Idx);

        auto& par = Glob::DDI_SubmitParam::Get(global);
        par.clear();
        
        par.push_back(PackVaBuffer(VAEncSequenceParameterBufferType, m_sps));
        par.push_back(PackVaBuffer(VAEncPictureParameterBufferType, m_pps));

        std::transform(m_slices.begin(), m_slices.end(), std::back_inserter(par)
            , [&](VAEncSliceParameterBufferHEVC& s) { return PackVaBuffer(VAEncSliceParameterBufferType, s); });

        if (cc.FillCUQPData(global, s_task, m_qpMap))
        {
            par.push_back(PackVaBuffer(
                VAEncQPBufferType
                , m_qpMap.m_buffer.data()
                , m_qpMap.m_pitch
                , m_qpMap.m_h_aligned));
        }

        m_vaPerPicMiscData.clear();
        m_vaPackedHeaders.clear();

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_AUD)
            , ph.AUD[mfx::clamp<mfxU8>(task.CodingType, 1, 3) - 1], par);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_VPS)
            , ph.VPS, par, VAEncPackedHeaderHEVC_VPS);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_SPS)
            , ph.SPS, par, VAEncPackedHeaderHEVC_SPS);

        AddPackedHeaderIf(!!(task.InsertHeaders & INSERT_PPS)
            , ph.PPS, par, VAEncPackedHeaderHEVC_PPS);

        AddPackedHeaderIf((!!(task.InsertHeaders & INSERT_SEI) || task.ctrl.NumPayload) && ph.PrefixSEI.BitLen
            , ph.PrefixSEI, par/*, VAEncPackedHeaderHEVC_SEI*/);

        std::for_each(
            ph.SSH.begin()
            , ph.SSH.begin() + std::min<size_t>(m_slices.size(), ph.SSH.size())
            , [&](PackedData& pd)
        {
            AddPackedHeaderIf(true, pd, par, VAEncPackedHeaderHEVC_Slice);
        });

        for (auto& AddMisc : cc.AddPerPicMiscData)
        {
            if (AddMisc.second(global, s_task, m_vaPerPicMiscData))
            {
                auto& misc = m_vaPerPicMiscData.back();
                par.push_back(PackVaBuffer(VAEncMiscParameterBufferType, misc.data(), (mfxU32)misc.size()));
            }
        }

        if(priority_par.m_MaxContextPriority)
        {
            mfxPriority contextPriority = core.GetSession()->m_priority;
            InitPriority(priority_par.m_MaxContextPriority, contextPriority, m_hevcPriorityBuf);
            par.push_back(PackVaBuffer(VAContextParameterUpdateBufferType , m_hevcPriorityBuf));
        }

        SetFeedback(task.StatusReportId, *(VASurfaceID*)task.HDLRaw.first, GetResources(RES_BS).at(task.BS.Idx));

        m_numSkipFrames  = 0;
        m_sizeSkipFrames = 0;

        return MFX_ERR_NONE;
    });
}

void VAPacker::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_QueryTask
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& fb = Glob::DDI_Feedback::Get(global);
        MFX_CHECK(!fb.bNotReady, MFX_TASK_BUSY);

        auto& task = Task::Common::Get(s_task);

        if (!(task.SkipCMD & SKIPCMD_NeedDriverCall))
            return MFX_ERR_NONE;

        auto pFB = fb.Get(task.StatusReportId);
        MFX_CHECK(pFB, MFX_TASK_BUSY);

        auto& rtErr = Glob::RTErr::Get(global);

        auto sts = CC::Get(global).ReadFeedback(global, s_task, *(const VACodedBufferSegment*)pFB);
        SetIf(rtErr, sts < MFX_ERR_NONE, sts);

        fb.Remove(task.StatusReportId);

        return sts;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
