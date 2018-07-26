// Copyright (c) 2017-2018 Intel Corporation
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

#include "mfx_common.h"

#include "mfx_h265_encode_hw_ddi.h"
#if defined (MFX_VA_LINUX)
#include "mfx_h265_encode_vaapi.h"
#endif
#include "mfx_h265_encode_hw_ddi_trace.h"

namespace MfxHwH265Encode
{

GUID GetGUID(MfxVideoParam const & /* par */)
{
    GUID guid = DXVA2_Intel_Encode_HEVC_Main;

    return guid;
}

DriverEncoder* CreatePlatformH265Encoder(MFXCoreInterface* core, ENCODER_TYPE /*type*/)
{
    if (core)
    {
        mfxCoreParam par = {};

        if (core->GetCoreParam(&par))
            return 0;

        switch(par.Impl & 0xF00)
        {
#if defined (MFX_VA_LINUX)
        case MFX_IMPL_VIA_VAAPI:
            return new VAAPIEncoder;
#endif
        default:
            return 0;
        }
    }

    return 0;
}

// this function is aimed to workaround all CAPS reporting problems in mainline driver
mfxStatus HardcodeCaps(ENCODE_CAPS_HEVC& caps, MFXCoreInterface* core)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!caps.LCUSizeSupported)
        caps.LCUSizeSupported = 2;
    caps.BlockSize = 2; // 32x32
    (void)core;
    return sts;
}

mfxStatus QueryHwCaps(MFXCoreInterface* core, GUID guid, ENCODE_CAPS_HEVC & caps)
{
    std::unique_ptr<DriverEncoder> ddi;

    ddi.reset(CreatePlatformH265Encoder(core));
    MFX_CHECK(ddi.get(), MFX_ERR_UNSUPPORTED);

    mfxStatus sts = ddi.get()->CreateAuxilliaryDevice(core, guid, 1920, 1088);
    MFX_CHECK_STS(sts);

    sts = ddi.get()->QueryEncodeCaps(caps);
    MFX_CHECK_STS(sts);

    return sts;
}

mfxStatus CheckHeaders(
    MfxVideoParam const & par,
    ENCODE_CAPS_HEVC const & caps)
{
    MFX_CHECK_COND(
           par.m_sps.log2_min_luma_coding_block_size_minus3 == 0
        && par.m_sps.separate_colour_plane_flag == 0
        && par.m_sps.pcm_enabled_flag == 0);

    {
        MFX_CHECK_COND(par.m_sps.amp_enabled_flag == 0);
        MFX_CHECK_COND(par.m_sps.sample_adaptive_offset_enabled_flag == 0);
    }

    MFX_CHECK_COND(par.m_pps.tiles_enabled_flag == 0);

    MFX_CHECK_COND(par.m_sps.chroma_format_idc == 1);

    MFX_CHECK_COND(
      !(   par.m_sps.pic_width_in_luma_samples > caps.MaxPicWidth
        || par.m_sps.pic_height_in_luma_samples > caps.MaxPicHeight
        || (UINT)(((par.m_pps.num_tile_columns_minus1 + 1) * (par.m_pps.num_tile_rows_minus1 + 1)) > 1) > caps.TileSupport));
    MFX_CHECK_COND(
        !(  caps.BitDepth8Only == 1 // 8 bit only
            && (par.m_sps.bit_depth_luma_minus8 != 0 || par.m_sps.bit_depth_chroma_minus8 != 0))); // not 8 bit 

    MFX_CHECK_COND(
        !(  caps.BitDepth8Only == 0 // 10 or 8 bit only
            && (!(par.m_sps.bit_depth_luma_minus8 == 0 || par.m_sps.bit_depth_luma_minus8 == 2)
                || !(par.m_sps.bit_depth_chroma_minus8 == 0 || par.m_sps.bit_depth_chroma_minus8 == 2))));

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
mfxStatus FillCUQPDataDDI(Task& task, MfxVideoParam &par, MFXCoreInterface& core, mfxFrameInfo &CUQPFrameInfo)
{
    mfxStatus mfxSts = MFX_ERR_NONE;
    mfxCoreParam coreParams = {};

    if (core.GetCoreParam(&coreParams))
       return  MFX_ERR_UNSUPPORTED;

    if (!task.m_bCUQPMap || ((coreParams.Impl & 0xF00) == MFX_IMPL_VIA_VAAPI))
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
