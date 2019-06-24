// Copyright (c) 2017-2019 Intel Corporation
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

#include "libmfx_core_interface.h"

#include "mfx_h265_encode_hw_ddi.h"
#if defined (MFX_VA_LINUX)
#include "mfx_h265_encode_vaapi.h"
#endif
#include "mfx_h265_encode_hw_ddi_trace.h"

namespace MfxHwH265Encode
{

GUID GetGUID(MfxVideoParam const & par)
{
    GUID guid = DXVA2_Intel_Encode_HEVC_Main;

    mfxU16 bdId = 0, cfId = 0;

#if (MFX_VERSION >= 1027)
    if (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10 || par.m_ext.CO3.TargetBitDepthLuma == 10)
        bdId = 1;

    cfId = mfx::clamp<mfxU16>(par.m_ext.CO3.TargetChromaFormatPlus1 - 1, MFX_CHROMAFORMAT_YUV420, MFX_CHROMAFORMAT_YUV444) - MFX_CHROMAFORMAT_YUV420;

    if (par.m_platform && par.m_platform < MFX_HW_ICL)
        cfId = 0; // platforms below ICL do not support Main422/Main444 profile, using Main instead.
#else
    if (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10 || par.mfx.FrameInfo.BitDepthLuma == 10 || par.mfx.FrameInfo.FourCC == MFX_FOURCC_P010)
        bdId = 1;

     cfId = 0;
#endif
    if (par.m_platform && par.m_platform < MFX_HW_KBL)
        bdId = 0;

    mfxU16 cFamily = IsOn(par.mfx.LowPower);


    guid = GuidTable[cFamily][bdId] [cfId];
    DDITracer::TraceGUID(guid, stdout);
    return guid;
}

DriverEncoder* CreatePlatformH265Encoder(VideoCORE* core, ENCODER_TYPE type)
{
    (void)type;

    if (core)
    {
        switch(core->GetVAType())
        {
#if defined (MFX_VA_LINUX)
        case MFX_HW_VAAPI:
            return new VAAPIEncoder;
#endif
        default:
            return nullptr;
        }
    }

    return nullptr;
}

// this function is aimed to workaround all CAPS reporting problems in mainline driver
mfxStatus HardcodeCaps(MFX_ENCODE_CAPS_HEVC& caps, VideoCORE* core)
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(core);
    if (!caps.ddi_caps.BitDepth8Only && !caps.ddi_caps.MaxEncodedBitDepth)
        caps.ddi_caps.MaxEncodedBitDepth = 1;
    if (!caps.ddi_caps.Color420Only && !(caps.ddi_caps.YUV444ReconSupport || caps.ddi_caps.YUV422ReconSupport))
        caps.ddi_caps.YUV444ReconSupport = 1;
    if (!caps.ddi_caps.Color420Only && !(caps.ddi_caps.YUV422ReconSupport))   // VPG: caps are not correct now
        caps.ddi_caps.YUV422ReconSupport = 1;
#if (MFX_VERSION >= 1025)
    eMFXHWType platform = core->GetHWType();

    if (platform < MFX_HW_CNL)
    {   // not set until CNL now
        caps.ddi_caps.LCUSizeSupported = 0b10;   // 32x32 lcu is only supported
        caps.ddi_caps.BlockSize = 0b10; // 32x32
    }

    if (platform < MFX_HW_ICL)
        caps.ddi_caps.NegativeQPSupport = 0;
    else
        caps.ddi_caps.NegativeQPSupport = 1; // driver should set it for Gen11+ VME only

#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
    if (platform >= MFX_HW_ICL)
    {
        if (caps.ddi_caps.NoWeightedPred)
            caps.ddi_caps.NoWeightedPred = 0;

        caps.ddi_caps.LumaWeightedPred = 1;
        caps.ddi_caps.ChromaWeightedPred = 1;

        if (!caps.ddi_caps.MaxNum_WeightedPredL0)
            caps.ddi_caps.MaxNum_WeightedPredL0 = 3;
        if (!caps.ddi_caps.MaxNum_WeightedPredL1)
            caps.ddi_caps.MaxNum_WeightedPredL1 = 3;

        //caps.SliceLevelWeightedPred;
    }
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
#else
    if (!caps.ddi_caps.LCUSizeSupported)
        caps.ddi_caps.LCUSizeSupported = 2;
    caps.ddi_caps.BlockSize = 2; // 32x32
    (void)core;
#endif

    return sts;
}

mfxStatus QueryHwCaps(VideoCORE* core, GUID guid, MFX_ENCODE_CAPS_HEVC & caps, MfxVideoParam const & par)
{
    std::unique_ptr<DriverEncoder> ddi;

    MFX_CHECK_NULL_PTR1(core);
    ddi.reset(CreatePlatformH265Encoder(core));
    MFX_CHECK(ddi.get(), MFX_ERR_UNSUPPORTED);

    mfxStatus sts = ddi.get()->CreateAuxilliaryDevice(core, guid, par);
    MFX_CHECK_STS(sts);

    sts = ddi.get()->QueryEncodeCaps(caps);
    MFX_CHECK_STS(sts);

    return sts;
}

mfxStatus CheckHeaders(
    MfxVideoParam const & par,
    MFX_ENCODE_CAPS_HEVC const & caps)
{
    MFX_CHECK_COND(
           par.m_sps.log2_min_luma_coding_block_size_minus3 == 0
        && par.m_sps.separate_colour_plane_flag == 0
        && par.m_sps.pcm_enabled_flag == 0);

#if (MFX_VERSION >= 1025)
    if (par.m_platform >= MFX_HW_CNL)
    {
        MFX_CHECK_COND(par.m_sps.amp_enabled_flag == 1);
    }
    else
#endif
    {
        MFX_CHECK_COND(par.m_sps.amp_enabled_flag == 0);
        MFX_CHECK_COND(par.m_sps.sample_adaptive_offset_enabled_flag == 0);
    }

#if (MFX_VERSION < 1027)
    MFX_CHECK_COND(par.m_pps.tiles_enabled_flag == 0);
#endif

#if (MFX_VERSION >= 1027)
    MFX_CHECK_COND(
      !(   (!caps.ddi_caps.YUV444ReconSupport && (par.m_sps.chroma_format_idc == 3))
        || (!caps.ddi_caps.YUV422ReconSupport && (par.m_sps.chroma_format_idc == 2))
        || (caps.ddi_caps.Color420Only && (par.m_sps.chroma_format_idc != 1))));

    MFX_CHECK_COND(caps.ddi_caps.NumScalablePipesMinus1 == 0 || par.m_pps.num_tile_columns_minus1 <= caps.ddi_caps.NumScalablePipesMinus1);

    if (par.m_pps.tiles_enabled_flag)
    {
        MFX_CHECK_COND(par.m_pps.loop_filter_across_tiles_enabled_flag);
    }
#else
    MFX_CHECK_COND(par.m_sps.chroma_format_idc == 1);
#endif

    MFX_CHECK_COND(
      !(   par.m_sps.pic_width_in_luma_samples > caps.ddi_caps.MaxPicWidth
        || par.m_sps.pic_height_in_luma_samples > caps.ddi_caps.MaxPicHeight
        || (UINT)(((par.m_pps.num_tile_columns_minus1 + 1) * (par.m_pps.num_tile_rows_minus1 + 1)) > 1) > caps.ddi_caps.TileSupport));

    MFX_CHECK_COND(
      !(   (caps.ddi_caps.MaxEncodedBitDepth == 0 || caps.ddi_caps.BitDepth8Only)
        && (par.m_sps.bit_depth_luma_minus8 != 0 || par.m_sps.bit_depth_chroma_minus8 != 0)));

    MFX_CHECK_COND(
      !(   (caps.ddi_caps.MaxEncodedBitDepth == 2 || caps.ddi_caps.MaxEncodedBitDepth == 1 || !caps.ddi_caps.BitDepth8Only)
        && ( !(par.m_sps.bit_depth_luma_minus8 == 0
            || par.m_sps.bit_depth_luma_minus8 == 2
            || par.m_sps.bit_depth_luma_minus8 == 4)
          || !(par.m_sps.bit_depth_chroma_minus8 == 0
            || par.m_sps.bit_depth_chroma_minus8 == 2
            || par.m_sps.bit_depth_chroma_minus8 == 4))));

    MFX_CHECK_COND(
      !(   caps.ddi_caps.MaxEncodedBitDepth == 2
        && ( !(par.m_sps.bit_depth_luma_minus8 == 0
            || par.m_sps.bit_depth_luma_minus8 == 2
            || par.m_sps.bit_depth_luma_minus8 == 4)
          || !(par.m_sps.bit_depth_chroma_minus8 == 0
            || par.m_sps.bit_depth_chroma_minus8 == 2
            || par.m_sps.bit_depth_chroma_minus8 == 4))));

    MFX_CHECK_COND(
      !(   caps.ddi_caps.MaxEncodedBitDepth == 3
        && ( !(par.m_sps.bit_depth_luma_minus8 == 0
            || par.m_sps.bit_depth_luma_minus8 == 2
            || par.m_sps.bit_depth_luma_minus8 == 4
            || par.m_sps.bit_depth_luma_minus8 == 8)
          || !(par.m_sps.bit_depth_chroma_minus8 == 0
            || par.m_sps.bit_depth_chroma_minus8 == 2
            || par.m_sps.bit_depth_chroma_minus8 == 4
            || par.m_sps.bit_depth_chroma_minus8 == 8))));

    return MFX_ERR_NONE;
}

mfxU16 CodingTypeToSliceType(mfxU16 ct)
{
    switch (ct)
    {
     case CODING_TYPE_I : return 2;
     case CODING_TYPE_P : return 1;
     case CODING_TYPE_B :
     case CODING_TYPE_B1:
     case CODING_TYPE_B2: return 0;
    default: assert(!"invalid coding type"); return 0xFFFF;
    }
}


DDIHeaderPacker::DDIHeaderPacker()
{
}

DDIHeaderPacker::~DDIHeaderPacker()
{
}

void DDIHeaderPacker::Reset(MfxVideoParam const & par)
{
    m_buf.resize(6 + par.mfx.NumSlice);
    m_cur = m_buf.begin();
    m_packer.Reset(par);
}

void DDIHeaderPacker::ResetPPS(MfxVideoParam const & par)
{
    assert(!m_buf.empty());

    m_packer.ResetPPS(par);
}

void DDIHeaderPacker::NewHeader()
{
    assert(m_buf.size());

    if (++m_cur == m_buf.end())
        m_cur = m_buf.begin();

    Zero(*m_cur);
}

ENCODE_PACKEDHEADER_DATA* DDIHeaderPacker::PackHeader(Task const & task, mfxU32 nut)
{
    NewHeader();

    switch(nut)
    {
    case VPS_NUT:
        m_packer.GetVPS(m_cur->pData, m_cur->DataLength);
        break;
    case SPS_NUT:
        m_packer.GetSPS(m_cur->pData, m_cur->DataLength);
        break;
    case PPS_NUT:
        m_packer.GetPPS(m_cur->pData, m_cur->DataLength);
        break;
    case AUD_NUT:
        {
            mfxU32 frameType = task.m_frameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B);

            if (frameType == MFX_FRAMETYPE_I)
                m_packer.GetAUD(m_cur->pData, m_cur->DataLength, 0);
            else if (frameType == MFX_FRAMETYPE_P)
                m_packer.GetAUD(m_cur->pData, m_cur->DataLength, 1);
            else
                m_packer.GetAUD(m_cur->pData, m_cur->DataLength, 2);
        }
        break;
    case PREFIX_SEI_NUT:
        m_packer.GetPrefixSEI(task, m_cur->pData, m_cur->DataLength);
        break;
    case SUFFIX_SEI_NUT:
        m_packer.GetSuffixSEI(task, m_cur->pData, m_cur->DataLength);
        break;
    default:
        return 0;
    }
    m_cur->BufferSize = m_cur->DataLength;
    m_cur->SkipEmulationByteCount = 4;

    return &*m_cur;
}

ENCODE_PACKEDHEADER_DATA* DDIHeaderPacker::PackSliceHeader(
    Task const & task,
    mfxU32 id,
    mfxU32* qpd_offset,
    bool dyn_slice_size,
    mfxU32* sao_offset,
    mfxU16* ssh_start_len,
    mfxU32* ssh_offset,
    mfxU32* pwt_offset,
    mfxU32* pwt_length)
{
    bool is1stNALU = (id == 0 && task.m_insertHeaders == 0);
    NewHeader();

    m_packer.GetSSH(task, id, m_cur->pData, m_cur->DataLength, qpd_offset, dyn_slice_size, sao_offset, ssh_start_len, ssh_offset, pwt_offset, pwt_length);
    m_cur->BufferSize = CeilDiv(m_cur->DataLength, 8);
    m_cur->SkipEmulationByteCount = 3 + is1stNALU;

    return &*m_cur;
}

ENCODE_PACKEDHEADER_DATA* DDIHeaderPacker::PackSkippedSlice(Task const & task, mfxU32 id, mfxU32* qpd_offset)
{
    bool is1stNALU = (id == 0 && task.m_insertHeaders == 0);
    NewHeader();

    m_packer.GetSkipSlice(task, id, m_cur->pData, m_cur->DataLength, qpd_offset);
    m_cur->BufferSize = m_cur->DataLength;
    m_cur->SkipEmulationByteCount = 3 + is1stNALU;
    m_cur->DataLength *= 8;

    return &*m_cur;
}
mfxStatus FillCUQPDataDDI(Task& task, MfxVideoParam &par, VideoCORE& core, mfxFrameInfo &CUQPFrameInfo)
{
    mfxStatus mfxSts = MFX_ERR_NONE;
    if (!task.m_bCUQPMap || (core.GetVAType() == MFX_HW_VAAPI))
        return MFX_ERR_NONE;

    mfxExtMBQP *mbqp = ExtBuffer::Get(task.m_ctrl);
#ifdef MFX_ENABLE_HEVCE_ROI
    mfxExtEncoderROI* roi = ExtBuffer::Get(task.m_ctrl);
#endif

    MFX_CHECK(CUQPFrameInfo.Width && CUQPFrameInfo.Height, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(CUQPFrameInfo.AspectRatioW && CUQPFrameInfo.AspectRatioH, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxU32 drBlkW  = CUQPFrameInfo.AspectRatioW;  // block size of driver
    mfxU32 drBlkH  = CUQPFrameInfo.AspectRatioH;  // block size of driver
    mfxU16 inBlkSize = 16;                            //mbqp->BlockSize ? mbqp->BlockSize : 16;  //input block size

    mfxU32 pitch_MBQP = (par.mfx.FrameInfo.Width  + inBlkSize - 1)/ inBlkSize;

    if (mbqp && mbqp->NumQPAlloc)
    {
        if ((mbqp->NumQPAlloc *  inBlkSize *  inBlkSize) <
            (drBlkW  *  drBlkH  *  CUQPFrameInfo.Width  *  CUQPFrameInfo.Height))
        {
            task.m_bCUQPMap = false;
            return  MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }

        FrameLocker lock(&core, task.m_midCUQp);
        MFX_CHECK(lock.Y, MFX_ERR_LOCK_MEMORY);

        for (mfxU32 i = 0; i < CUQPFrameInfo.Height; i++)
            for (mfxU32 j = 0; j < CUQPFrameInfo.Width; j++)
                    lock.Y[i * lock.Pitch + j] = mbqp->QP[i*drBlkH/inBlkSize * pitch_MBQP + j*drBlkW/inBlkSize];

    }
#ifdef MFX_ENABLE_HEVCE_ROI
    else if (roi)
    {
        FrameLocker lock(&core, task.m_midCUQp);
        MFX_CHECK(lock.Y, MFX_ERR_LOCK_MEMORY);
        for (mfxU32 i = 0; i < CUQPFrameInfo.Height; i++)
        {
            for (mfxU32 j = 0; j < CUQPFrameInfo.Width; j++)
            {
                mfxU8 qp = (mfxU8)task.m_qpY;
                mfxI32 diff = 0;
                for (mfxU32 n = 0; n < roi->NumROI; n++)
                {
                    mfxU32 x = i*drBlkW;
                    mfxU32 y = i*drBlkH;
                    if (x >= roi->ROI[n].Left  &&  x < roi->ROI[n].Right  && y >= roi->ROI[n].Top && y < roi->ROI[n].Bottom)
                    {
                        diff = (task.m_bPriorityToDQPpar? (-1) : 1) * roi->ROI[n].Priority;
                        break;
                    }

                }
                lock.Y[i * lock.Pitch + j] = (mfxU8)(qp + diff);
            }
        }
    }
#endif
    return mfxSts;

}


}; // namespace MfxHwH265Encode
#endif
