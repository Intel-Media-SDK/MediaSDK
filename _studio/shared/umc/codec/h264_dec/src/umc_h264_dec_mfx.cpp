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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)
#include "umc_h264_dec_mfx.h"
#include "mfxmvc.h"
#include "mfx_enc_common.h"

namespace UMC
{

Status FillVideoParamExtension(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * seqEx, mfxVideoParam *par)
{
    if (!seqEx || !par)
        return UMC_ERR_NULL_PTR;

    mfxExtMVCSeqDesc * points = (mfxExtMVCSeqDesc*)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);

    if (!points)
        return UMC_ERR_NULL_PTR;

    // calculate size
    if ((uint32_t)seqEx->num_level_values_signalled_minus1 + 1 != seqEx->levelInfo.size())
    {
        return UMC_ERR_FAILED;
    }

    points->NumView = (mfxU16)(seqEx->num_views_minus1 + 1);
    size_t numberOfPoints = 0;
    size_t numberOfTargets = 0;

    for (const auto & levelSignal : seqEx->levelInfo)
    {
        if ((uint32_t)levelSignal.num_applicable_ops_minus1 + 1 != levelSignal.opsInfo.size())
        {
            return UMC_ERR_FAILED;
        }

        for (const auto & ops : levelSignal.opsInfo)
        {
            if ((uint32_t)ops.applicable_op_num_target_views_minus1 + 1 != ops.applicable_op_target_view_id.size())
            {
                return UMC_ERR_FAILED;
            }

            numberOfPoints++;
            numberOfTargets += ops.applicable_op_target_view_id.size();
        }
    }

    points->NumOP = (mfxU32)numberOfPoints;
    points->NumViewId = (mfxU32)numberOfTargets;

    if (points->NumOP > points->NumOPAlloc || points->NumView > points->NumViewAlloc ||
        points->NumViewId > points->NumViewIdAlloc)
    {
        return UMC_ERR_NOT_ENOUGH_BUFFER;
    }

    size_t view_number = 0;
    for (const auto & viewInfo : seqEx->viewInfo)
    {
        mfxMVCViewDependency * dependency = &points->View[view_number];
        dependency->ViewId = (mfxU16)viewInfo.view_id;

        dependency->NumAnchorRefsL0 = viewInfo.num_anchor_refs_lx[0];
        dependency->NumAnchorRefsL1 = viewInfo.num_anchor_refs_lx[1];

        for (uint32_t k = 0; k < 16; k++)
        {
            dependency->AnchorRefL0[k] = viewInfo.anchor_refs_lx[0][k];
            dependency->AnchorRefL1[k] = viewInfo.anchor_refs_lx[1][k];
        }

        dependency->NumNonAnchorRefsL0 = viewInfo.num_non_anchor_refs_lx[0];
        dependency->NumNonAnchorRefsL1 = viewInfo.num_non_anchor_refs_lx[1];

        for (uint32_t k = 0; k < 16; k++)
        {
            dependency->NonAnchorRefL0[k] = viewInfo.non_anchor_refs_lx[0][k];
            dependency->NonAnchorRefL1[k] = viewInfo.non_anchor_refs_lx[1][k];
        }
        view_number++;
    }

    mfxU16 * targetViews = points->ViewId;
    mfxMVCOperationPoint * operationPoints = &points->OP[0];

    for (const auto & levelSignal : seqEx->levelInfo)
    {
        for (const auto & ops : levelSignal.opsInfo)
        {
            operationPoints->TemporalId = ops.applicable_op_temporal_id;
            operationPoints->LevelIdc = levelSignal.level_idc;
            operationPoints->NumViews = ops.applicable_op_num_views_minus1 + 1;
            operationPoints->NumTargetViews = ops.applicable_op_num_target_views_minus1 + 1;
            operationPoints->TargetViewId = targetViews;

            std::copy(ops.applicable_op_target_view_id.begin(), ops.applicable_op_target_view_id.end(), operationPoints->TargetViewId);

            operationPoints++;
            targetViews += ops.applicable_op_target_view_id.size();
        }
    }

    return UMC_OK;
}

Status FillVideoParam(const UMC_H264_DECODER::H264SeqParamSet * seq, mfxVideoParam *par, bool full)
{
    par->mfx.CodecId = MFX_CODEC_AVC;

    if (seq->bit_depth_luma > 8 || seq->bit_depth_chroma > 8)
    {
        par->mfx.FrameInfo.FourCC = seq->chroma_format_idc == 2 ? MFX_FOURCC_P210 : MFX_FOURCC_P010;
    }
    else
    {
        par->mfx.FrameInfo.FourCC = seq->chroma_format_idc == 2 ? MFX_FOURCC_NV16 : MFX_FOURCC_NV12;
    }

    par->mfx.FrameInfo.Width = (mfxU16) (seq->frame_width_in_mbs * 16);
    par->mfx.FrameInfo.Height = (mfxU16) (seq->frame_height_in_mbs * 16);

    par->mfx.FrameInfo.BitDepthLuma = seq->bit_depth_luma;
    par->mfx.FrameInfo.BitDepthChroma = seq->bit_depth_chroma;

    //if (seq->frame_cropping_flag)
    {
        par->mfx.FrameInfo.CropX = (mfxU16)(SubWidthC[seq->chroma_format_idc] * seq->frame_cropping_rect_left_offset);
        par->mfx.FrameInfo.CropY = (mfxU16)(SubHeightC[seq->chroma_format_idc] * seq->frame_cropping_rect_top_offset * (2 - seq->frame_mbs_only_flag));
        par->mfx.FrameInfo.CropH = (mfxU16)(seq->frame_height_in_mbs * 16 -
            SubHeightC[seq->chroma_format_idc]*(2 - seq->frame_mbs_only_flag) *
            (seq->frame_cropping_rect_top_offset + seq->frame_cropping_rect_bottom_offset));

        par->mfx.FrameInfo.CropW = (mfxU16)(seq->frame_width_in_mbs * 16 - SubWidthC[seq->chroma_format_idc] *
            (seq->frame_cropping_rect_left_offset + seq->frame_cropping_rect_right_offset));
    }

    par->mfx.FrameInfo.PicStruct = (mfxU8)(seq->frame_mbs_only_flag ? MFX_PICSTRUCT_PROGRESSIVE : MFX_PICSTRUCT_UNKNOWN);

    if (seq->chroma_format_idc == 2)
    {
        par->mfx.FrameInfo.ChromaFormat = (mfxU16)MFX_CHROMAFORMAT_YUV422;
    }
    else
        par->mfx.FrameInfo.ChromaFormat = (mfxU16)(seq->chroma_format_idc ? MFX_CHROMAFORMAT_YUV420 : MFX_CHROMAFORMAT_YUV400);

    if (seq->vui.aspect_ratio_info_present_flag || full)
    {
        par->mfx.FrameInfo.AspectRatioW = seq->vui.sar_width;
        par->mfx.FrameInfo.AspectRatioH = seq->vui.sar_height;
    }
    else
    {
        par->mfx.FrameInfo.AspectRatioW = 0;
        par->mfx.FrameInfo.AspectRatioH = 0;
    }

    if (seq->vui.timing_info_present_flag || full)
    {
        par->mfx.FrameInfo.FrameRateExtD = seq->vui.num_units_in_tick * 2;
        par->mfx.FrameInfo.FrameRateExtN = seq->vui.time_scale;
    }
    else
    {
        par->mfx.FrameInfo.FrameRateExtD = 0;
        par->mfx.FrameInfo.FrameRateExtN = 0;
    }

    par->mfx.CodecProfile = seq->profile_idc;
    par->mfx.CodecLevel = seq->level_idc;
    par->mfx.MaxDecFrameBuffering =  seq->vui.bitstream_restriction_flag ? seq->vui.max_dec_frame_buffering : 0;

    // video signal section
    mfxExtVideoSignalInfo * videoSignal = (mfxExtVideoSignalInfo *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (videoSignal)
    {
        videoSignal->VideoFormat = seq->vui.video_format;
        videoSignal->VideoFullRange = seq->vui.video_full_range_flag;
        videoSignal->ColourDescriptionPresent = seq->vui.colour_description_present_flag;
        videoSignal->ColourPrimaries = seq->vui.colour_primaries;
        videoSignal->TransferCharacteristics = seq->vui.transfer_characteristics;
        videoSignal->MatrixCoefficients = seq->vui.matrix_coefficients;
    }

    return UMC_OK;
}

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE
