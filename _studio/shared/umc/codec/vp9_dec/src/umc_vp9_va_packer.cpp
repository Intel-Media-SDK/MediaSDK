// Copyright (c) 2020 Intel Corporation
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

#include "umc_vp9_dec_defs.h"

#ifdef MFX_ENABLE_VP9_VIDEO_DECODE

#include "umc_vp9_va_packer.h"
#include "umc_vp9_bitstream.h"
#include "umc_vp9_frame.h"
#include "umc_vp9_utils.h"
#include "mfx_session.h"

using namespace UMC;

namespace UMC_VP9_DECODER
{

Packer * Packer::CreatePacker(UMC::VideoAccelerator * va)
{
    Packer * packer = 0;

    packer = new PackerVA(va);

    return packer;
}

Packer::Packer(UMC::VideoAccelerator * va)
    : m_va(va)
{}

Packer::~Packer()
{}


/****************************************************************************************************/
// VA linux packer implementation
/****************************************************************************************************/
PackerVA::PackerVA(VideoAccelerator * va)
    : Packer(va)
{}

Status PackerVA::GetStatusReport(void * , size_t )
{
    return UMC_OK;
}

void PackerVA::BeginFrame()
{ /* nothing to do here*/ }

void PackerVA::EndFrame()
{ /* nothing to do here*/ }

void PackerVA::PackAU(VP9Bitstream* bs, VP9DecoderFrame const* info)
{
    if (!bs || !info)
        throw vp9_exception(MFX_ERR_NULL_PTR);

    UMC::UMCVACompBuffer* pCompBuf = nullptr;
    VADecPictureParameterBufferVP9 *picParam =
        (VADecPictureParameterBufferVP9*)m_va->GetCompBuffer(VAPictureParameterBufferType, &pCompBuf, sizeof(VADecPictureParameterBufferVP9));

    if (!picParam)
        throw vp9_exception(MFX_ERR_MEMORY_ALLOC);

    memset(picParam, 0, sizeof(VADecPictureParameterBufferVP9));
    PackPicParams(picParam, info);

    pCompBuf = nullptr;
    VASliceParameterBufferVP9 *sliceParam =
        (VASliceParameterBufferVP9*)m_va->GetCompBuffer(VASliceParameterBufferType, &pCompBuf, sizeof(VASliceParameterBufferVP9));
    if (!sliceParam)
        throw vp9_exception(MFX_ERR_MEMORY_ALLOC);

    memset(sliceParam, 0, sizeof(VASliceParameterBufferVP9));
    PackSliceParams(sliceParam, info);

    uint8_t* data;
    uint32_t length;
    bs->GetOrg(&data, &length);
    uint32_t const offset = bs->BytesDecoded();
    length -= offset;

    pCompBuf = nullptr;
    mfxU8 *bistreamData = (mfxU8*)m_va->GetCompBuffer(VASliceDataBufferType, &pCompBuf, length);
    if (!bistreamData)
        throw vp9_exception(MFX_ERR_MEMORY_ALLOC);

    std::copy(data + offset, data + offset + length, bistreamData);
    pCompBuf->SetDataSize(length);

    if(m_va->m_MaxContextPriority)
        PackPriorityParams();
}

void PackerVA::PackPriorityParams()
{
    mfxPriority priority = m_va->m_ContextPriority;
    UMCVACompBuffer *GpuPriorityBuf;
    VAContextParameterUpdateBuffer* GpuPriorityBuf_Vp9Decode = (VAContextParameterUpdateBuffer *)m_va->GetCompBuffer(VAContextParameterUpdateBufferType, &GpuPriorityBuf, sizeof(VAContextParameterUpdateBuffer));
    if (!GpuPriorityBuf_Vp9Decode)
        throw vp9_exception(MFX_ERR_MEMORY_ALLOC);

    memset(GpuPriorityBuf_Vp9Decode, 0, sizeof(VAContextParameterUpdateBuffer));
    GpuPriorityBuf_Vp9Decode->flags.bits.context_priority_update = 1;

    if(priority == MFX_PRIORITY_LOW)
    {
        GpuPriorityBuf_Vp9Decode->context_priority.bits.priority = 0;
    }
    else if (priority == MFX_PRIORITY_HIGH)
    {
        GpuPriorityBuf_Vp9Decode->context_priority.bits.priority = m_va->m_MaxContextPriority;
    }
    else
    {
    GpuPriorityBuf_Vp9Decode->context_priority.bits.priority = m_va->m_MaxContextPriority/2;
    }

    GpuPriorityBuf->SetDataSize(sizeof(VAContextParameterUpdateBuffer));
}

void PackerVA::PackPicParams(VADecPictureParameterBufferVP9* picParam, VP9DecoderFrame const* info)
{
    VM_ASSERT(picParam);
    VM_ASSERT(info);

    picParam->frame_width = info->width;
    picParam->frame_height = info->height;

    if (KEY_FRAME == info->frameType)
        memset(picParam->reference_frames, VA_INVALID_SURFACE, sizeof(picParam->reference_frames));
    else
    {
        for (mfxU8 ref = 0; ref < NUM_REF_FRAMES; ++ref)
        {
            if (info->ref_frame_map[ref] != VP9_INVALID_REF_FRAME)
            {
                picParam->reference_frames[ref] = m_va->GetSurfaceID(info->ref_frame_map[ref]);
            }
            else
            {
                picParam->reference_frames[ref] = VA_INVALID_SURFACE;
            }
        }
    }

    picParam->pic_fields.bits.subsampling_x = info->subsamplingX;
    picParam->pic_fields.bits.subsampling_y = info->subsamplingY;
    picParam->pic_fields.bits.frame_type = info->frameType;
    picParam->pic_fields.bits.show_frame = info->showFrame;
    picParam->pic_fields.bits.error_resilient_mode = info->errorResilientMode;
    picParam->pic_fields.bits.intra_only = info->intraOnly;
    picParam->pic_fields.bits.allow_high_precision_mv = info->allowHighPrecisionMv;
    picParam->pic_fields.bits.mcomp_filter_type = info->interpFilter;
    picParam->pic_fields.bits.frame_parallel_decoding_mode = info->frameParallelDecodingMode;
    picParam->pic_fields.bits.reset_frame_context = info->resetFrameContext;
    picParam->pic_fields.bits.refresh_frame_context = info->refreshFrameContext;
    picParam->pic_fields.bits.frame_context_idx = info->frameContextIdx;
    picParam->pic_fields.bits.segmentation_enabled = info->segmentation.enabled;
    picParam->pic_fields.bits.segmentation_temporal_update = info->segmentation.temporalUpdate;
    picParam->pic_fields.bits.segmentation_update_map = info->segmentation.updateMap;

    {
        picParam->pic_fields.bits.last_ref_frame = info->activeRefIdx[0];
        picParam->pic_fields.bits.last_ref_frame_sign_bias = info->refFrameSignBias[LAST_FRAME];
        picParam->pic_fields.bits.golden_ref_frame = info->activeRefIdx[1];
        picParam->pic_fields.bits.golden_ref_frame_sign_bias = info->refFrameSignBias[GOLDEN_FRAME];
        picParam->pic_fields.bits.alt_ref_frame = info->activeRefIdx[2];
        picParam->pic_fields.bits.alt_ref_frame_sign_bias = info->refFrameSignBias[ALTREF_FRAME];
    }
    picParam->pic_fields.bits.lossless_flag = info->lossless;

    picParam->filter_level = info->lf.filterLevel;
    picParam->sharpness_level = info->lf.sharpnessLevel;
    picParam->log2_tile_rows = info->log2TileRows;
    picParam->log2_tile_columns = info->log2TileColumns;
    picParam->frame_header_length_in_bytes = info->frameHeaderLength;
    picParam->first_partition_size = info->firstPartitionSize;

    for (mfxU8 i = 0; i < VP9_NUM_OF_SEGMENT_TREE_PROBS; ++i)
        picParam->mb_segment_tree_probs[i] = info->segmentation.treeProbs[i];

    for (mfxU8 i = 0; i < VP9_NUM_OF_PREDICTION_PROBS; ++i)
        picParam->segment_pred_probs[i] = info->segmentation.predProbs[i];

    picParam->profile = info->profile;
    picParam->bit_depth = info->bit_depth;
}

void PackerVA::PackSliceParams(VASliceParameterBufferVP9* sliceParam, VP9DecoderFrame const* info)
{
    VM_ASSERT(sliceParam);
    VM_ASSERT(info);

    sliceParam->slice_data_size = info->frameDataSize;
    sliceParam->slice_data_offset = 0;
    sliceParam->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

    for (mfxU8 segmentId = 0; segmentId < VP9_MAX_NUM_OF_SEGMENTS; ++segmentId)
    {
        sliceParam->seg_param[segmentId].segment_flags.fields.segment_reference_enabled = !!(info->segmentation.featureMask[segmentId] & (1 << SEG_LVL_REF_FRAME));

        sliceParam->seg_param[segmentId].segment_flags.fields.segment_reference =
            GetSegData(info->segmentation, segmentId, SEG_LVL_REF_FRAME);

        sliceParam->seg_param[segmentId].segment_flags.fields.segment_reference_skipped = !!(info->segmentation.featureMask[segmentId] & (1 << SEG_LVL_SKIP));

        for (mfxU8 ref = INTRA_FRAME; ref < MAX_REF_FRAMES; ++ref)
            for (mfxU8 mode = 0; mode < MAX_MODE_LF_DELTAS; ++mode)
                sliceParam->seg_param[segmentId].filter_level[ref][mode] = info->lf_info.level[segmentId][ref][mode];

        const mfxI32 qIndex = GetQIndex(info->segmentation, segmentId, info->baseQIndex);
        if (qIndex < 0)
            throw vp9_exception(MFX_ERR_UNDEFINED_BEHAVIOR);

        sliceParam->seg_param[segmentId].luma_ac_quant_scale = info->yDequant[qIndex][1];
        sliceParam->seg_param[segmentId].luma_dc_quant_scale = info->yDequant[qIndex][0];
        sliceParam->seg_param[segmentId].chroma_ac_quant_scale = info->uvDequant[qIndex][1];
        sliceParam->seg_param[segmentId].chroma_dc_quant_scale = info->uvDequant[qIndex][0];
    }
}


} // namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_VP9_VIDEO_DECODE
