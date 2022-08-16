// Copyright (c) 2017-2020 Intel Corporation
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

#include "umc_h264_va_packer.h"
#include "umc_h264_task_supplier.h"

#ifdef MFX_ENABLE_CPLIB
#include "mfx_cenc.h"
#endif

#include "umc_va_linux.h"
#include "umc_va_linux_protected.h"
#include "umc_va_video_processing.h"
#include "umc_va_fei.h"

#include "mfx_common_int.h"
#include "mfx_ext_buffers.h"

#include "mfxfei.h"
#include "mfx_trace.h"

namespace UMC
{

enum ChoppingStatus
{
    CHOPPING_NONE = 0,
    CHOPPING_SPLIT_SLICE_DATA = 1,
    CHOPPING_SPLIT_SLICES = 2,
    CHOPPING_SKIP_SLICE,
};

Packer::Packer(VideoAccelerator * va, TaskSupplier * supplier)
    : m_va(va)
    , m_supplier(supplier)
{

}

Packer::~Packer()
{
}

Status Packer::SyncTask(H264DecoderFrame* pFrame, void * error)
{
    return m_va->SyncTask(pFrame->m_index, error);
}

Status Packer::QueryTaskStatus(int32_t index, void * status, void * error)
{
    return m_va->QueryTaskStatus(index, status, error);
}

Status Packer::QueryStreamOut(H264DecoderFrame* /*pFrame*/)
{
    return UMC_OK;
}

Packer * Packer::CreatePacker(VideoAccelerator * va, TaskSupplier* supplier)
{
    Packer * packer = 0;
#ifdef MFX_ENABLE_CPLIB
    if (va->GetProtectedVA() && IS_PROTECTION_CENC(va->GetProtectedVA()->GetProtected()))
        packer = new PackerVA_CENC(va, supplier);
    else
#endif
        packer = new PackerVA(va, supplier);

    return packer;
}



/****************************************************************************************************/
// VA linux packer implementation
/****************************************************************************************************/
PackerVA::PackerVA(VideoAccelerator * va, TaskSupplier * supplier)
    : Packer(va, supplier)
{
    m_enableStreamOut = !!DynamicCast<FEIVideoAccelerator>(va);
}

Status PackerVA::GetStatusReport(void * /*pStatusReport*/, size_t /*size*/)
{
    return UMC_OK;
}

void PackerVA::FillFrame(VAPictureH264 * pic, const H264DecoderFrame *pFrame,
                         int32_t field, int32_t reference, int32_t defaultIndex)
{
    int32_t index = pFrame->m_index;

    if (index == -1)
        index = defaultIndex;

    pic->picture_id = m_va->GetSurfaceID(index);
    pic->frame_idx = pFrame->isLongTermRef() ? (uint16_t)pFrame->m_LongTermFrameIdx : (uint16_t)pFrame->m_FrameNum;

    int parityNum0 = pFrame->GetNumberByParity(0);
    if (parityNum0 >= 0 && parityNum0 < 2)
    {
        pic->TopFieldOrderCnt = pFrame->m_PicOrderCnt[parityNum0];
    }
    else
    {
        VM_ASSERT(0);
    }
    int parityNum1 = pFrame->GetNumberByParity(1);
    if (parityNum1 >= 0 && parityNum1 < 2)
    {
        pic->BottomFieldOrderCnt = pFrame->m_PicOrderCnt[parityNum1];
    }
    else
    {
        VM_ASSERT(0);
    }
    pic->flags = 0;

    if (pFrame->m_PictureStructureForDec == 0)
    {
        pic->flags |= field ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
    }

    if (reference == 1)
        pic->flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;

    if (reference == 2)
        pic->flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;

    if (pic->picture_id == VA_INVALID_ID)
    {
        pic->frame_idx = 0;
        pic->TopFieldOrderCnt = 0;
        pic->BottomFieldOrderCnt = 0;
        pic->flags = VA_PICTURE_H264_INVALID;
    }
}

int32_t PackerVA::FillRefFrame(VAPictureH264 * pic, const H264DecoderFrame *pFrame,
                            ReferenceFlags flags, bool isField, int32_t defaultIndex)
{
    int32_t index = pFrame->m_index;

    if (index == -1)
        index = defaultIndex;

    pic->picture_id = m_va->GetSurfaceID(index);
    pic->frame_idx = pFrame->isLongTermRef() ? (uint16_t)pFrame->m_LongTermFrameIdx : (uint16_t)pFrame->m_FrameNum;

    int parityNum0 = pFrame->GetNumberByParity(0);
    if (parityNum0 >= 0 && parityNum0 < 2)
    {
        pic->TopFieldOrderCnt = pFrame->m_PicOrderCnt[parityNum0];
    }
    else
    {
        VM_ASSERT(0);
    }
    int parityNum1 = pFrame->GetNumberByParity(1);
    if (parityNum1 >= 0 && parityNum1 < 2)
    {
        pic->BottomFieldOrderCnt = pFrame->m_PicOrderCnt[parityNum1];
    }
    else
    {
        VM_ASSERT(0);
    }

    pic->flags = 0;

    if (isField)
    {
        pic->flags |= flags.field ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
    }

    pic->flags |= flags.isShortReference ? VA_PICTURE_H264_SHORT_TERM_REFERENCE : VA_PICTURE_H264_LONG_TERM_REFERENCE;

    if (pic->picture_id == VA_INVALID_ID)
    {
        pic->frame_idx = 0;
        pic->TopFieldOrderCnt = 0;
        pic->BottomFieldOrderCnt = 0;
        pic->flags = VA_PICTURE_H264_INVALID;
    }

    return pic->picture_id;
}

void PackerVA::FillFrameAsInvalid(VAPictureH264 * pic)
{
    pic->picture_id = VA_INVALID_SURFACE;
    pic->frame_idx = 0;
    pic->TopFieldOrderCnt = 0;
    pic->BottomFieldOrderCnt = 0;
    pic->flags = VA_PICTURE_H264_INVALID;
}


void PackerVA::PackPicParams(H264DecoderFrameInfo * pSliceInfo, H264Slice * pSlice)
{
    const UMC_H264_DECODER::H264SliceHeader* pSliceHeader = pSlice->GetSliceHeader();
    const UMC_H264_DECODER::H264SeqParamSet* pSeqParamSet = pSlice->GetSeqParam();
    const UMC_H264_DECODER::H264PicParamSet* pPicParamSet = pSlice->GetPicParam();

    const H264DecoderFrame *pCurrentFrame = pSliceInfo->m_pFrame;

    UMCVACompBuffer *picParamBuf;
    VAPictureParameterBufferH264* pPicParams_H264 = (VAPictureParameterBufferH264*)m_va->GetCompBuffer(VAPictureParameterBufferType, &picParamBuf, sizeof(VAPictureParameterBufferH264));
    if (!pPicParams_H264)
        throw h264_exception(UMC_ERR_FAILED);

    memset(pPicParams_H264, 0, sizeof(VAPictureParameterBufferH264));

    int32_t reference = pCurrentFrame->isShortTermRef() ? 1 : (pCurrentFrame->isLongTermRef() ? 2 : 0);

    FillFrame(&(pPicParams_H264->CurrPic), pCurrentFrame, pSliceHeader->bottom_field_flag, reference, 0);

    pPicParams_H264->CurrPic.flags = 0;

    if (reference == 1)
        pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;

    if (reference == 2)
        pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;

    if (pSliceHeader->field_pic_flag)
    {
        if (pSliceHeader->bottom_field_flag)
        {
            pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_BOTTOM_FIELD;
            pPicParams_H264->CurrPic.TopFieldOrderCnt = 0;
        }
        else
        {
            pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_TOP_FIELD;
            pPicParams_H264->CurrPic.BottomFieldOrderCnt = 0;
        }
    }

    //packing
    pPicParams_H264->picture_width_in_mbs_minus1 = (unsigned short)(pSeqParamSet->frame_width_in_mbs - 1);
    pPicParams_H264->picture_height_in_mbs_minus1 = (unsigned short)(pSeqParamSet->frame_height_in_mbs - 1);

    pPicParams_H264->bit_depth_luma_minus8 = (unsigned char)(pSeqParamSet->bit_depth_luma - 8);
    pPicParams_H264->bit_depth_chroma_minus8 = (unsigned char)(pSeqParamSet->bit_depth_chroma - 8);

    pPicParams_H264->num_ref_frames = (unsigned char)pSeqParamSet->num_ref_frames;

    pPicParams_H264->seq_fields.bits.chroma_format_idc = pSeqParamSet->chroma_format_idc;
    pPicParams_H264->seq_fields.bits.residual_colour_transform_flag = pSeqParamSet->residual_colour_transform_flag;
    //pPicParams_H264->seq_fields.bits.gaps_in_frame_num_value_allowed_flag = ???
    pPicParams_H264->seq_fields.bits.frame_mbs_only_flag = pSeqParamSet->frame_mbs_only_flag;
    pPicParams_H264->seq_fields.bits.mb_adaptive_frame_field_flag = pSliceHeader->MbaffFrameFlag;
    pPicParams_H264->seq_fields.bits.direct_8x8_inference_flag = pSeqParamSet->direct_8x8_inference_flag;
    pPicParams_H264->seq_fields.bits.MinLumaBiPredSize8x8 = pSeqParamSet->level_idc > 30 ? 1 : 0;
    pPicParams_H264->seq_fields.bits.log2_max_frame_num_minus4 = (unsigned char)(pSeqParamSet->log2_max_frame_num - 4);
    pPicParams_H264->seq_fields.bits.pic_order_cnt_type = pSeqParamSet->pic_order_cnt_type;
    pPicParams_H264->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = (unsigned char)(pSeqParamSet->log2_max_pic_order_cnt_lsb - 4);
    pPicParams_H264->seq_fields.bits.delta_pic_order_always_zero_flag = pSeqParamSet->delta_pic_order_always_zero_flag;

    // pPicParams_H264->num_slice_groups_minus1 = (unsigned char)(pPicParamSet->num_slice_groups - 1);
    // pPicParams_H264->slice_group_map_type = (unsigned char)pPicParamSet->SliceGroupInfo.slice_group_map_type;
    pPicParams_H264->pic_init_qp_minus26 = (unsigned char)(pPicParamSet->pic_init_qp - 26);
    pPicParams_H264->pic_init_qs_minus26 = (unsigned char)(pPicParamSet->pic_init_qs - 26);
    pPicParams_H264->chroma_qp_index_offset = (unsigned char)pPicParamSet->chroma_qp_index_offset[0];
    pPicParams_H264->second_chroma_qp_index_offset = (unsigned char)pPicParamSet->chroma_qp_index_offset[1];

    pPicParams_H264->pic_fields.bits.entropy_coding_mode_flag = pPicParamSet->entropy_coding_mode;
    pPicParams_H264->pic_fields.bits.weighted_pred_flag = pPicParamSet->weighted_pred_flag;
    pPicParams_H264->pic_fields.bits.weighted_bipred_idc = pPicParamSet->weighted_bipred_idc;
    pPicParams_H264->pic_fields.bits.transform_8x8_mode_flag = pPicParamSet->transform_8x8_mode_flag;
    pPicParams_H264->pic_fields.bits.field_pic_flag = pSliceHeader->field_pic_flag;
    pPicParams_H264->pic_fields.bits.constrained_intra_pred_flag = pPicParamSet->constrained_intra_pred_flag;
    pPicParams_H264->pic_fields.bits.pic_order_present_flag = pPicParamSet->bottom_field_pic_order_in_frame_present_flag;
    pPicParams_H264->pic_fields.bits.deblocking_filter_control_present_flag = pPicParamSet->deblocking_filter_variables_present_flag;
    pPicParams_H264->pic_fields.bits.redundant_pic_cnt_present_flag = 0;//pPicParamSet->redundant_pic_cnt_present_flag;
    pPicParams_H264->pic_fields.bits.reference_pic_flag = pSliceHeader->nal_ref_idc != 0; //!!!

    pPicParams_H264->frame_num = (unsigned short)pSliceHeader->frame_num;

//    pPicParams_H264->num_ref_idx_l0_default_active_minus1 = (unsigned char)(pPicParamSet->num_ref_idx_l0_active-1);
//    pPicParams_H264->num_ref_idx_l1_default_active_minus1 = (unsigned char)(pPicParamSet->num_ref_idx_l1_active-1);

    //create reference picture list
    for (int32_t i = 0; i < 16; i++)
    {
        FillFrameAsInvalid(&(pPicParams_H264->ReferenceFrames[i]));
    }

    int32_t j = 0;

    int32_t viewCount = m_supplier->GetViewCount();

    for (int32_t i = 0; i < viewCount; i++)
    {
        ViewItem & view = m_supplier->GetViewByNumber(i);
        H264DBPList * pDPBList = view.GetDPBList(0);
        int32_t dpbSize = pDPBList->GetDPBSize();

        int32_t start = j;

        for (H264DecoderFrame * pFrm = pDPBList->head(); pFrm && (j < dpbSize + start); pFrm = pFrm->future())
        {
            if (j >= 16)
            {
                VM_ASSERT(false);
                throw h264_exception(UMC_ERR_FAILED);
            }
            VM_ASSERT(j < dpbSize + start);

            int32_t defaultIndex = 0;

            if ((0 == pCurrentFrame->m_index) && !pFrm->IsFrameExist())
            {
                defaultIndex = 1;
            }

            int32_t reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);
            if (!reference && pCurrentFrame != pFrm && (pFrm->isInterViewRef(0) || pFrm->isInterViewRef(1)) &&
                (pFrm->PicOrderCnt(0, 3) == pCurrentFrame->PicOrderCnt(0, 3)) && pFrm->m_viewId < pCurrentFrame->m_viewId)
            { // interview reference
                reference = 1;
            }

            if (!reference)
            {
                continue;
            }

            reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);
            int32_t field = pFrm->m_bottom_field_flag[0];
            FillFrame(&(pPicParams_H264->ReferenceFrames[j]), pFrm,
                field, reference, defaultIndex);

            reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);

            if ((pFrm == pCurrentFrame) && ((&pCurrentFrame->m_pSlicesInfo) != pSliceInfo))
            {
                FillFrame(&(pPicParams_H264->ReferenceFrames[j]), pFrm, 0, reference, defaultIndex);
            }

            j++;
        }
    }

    picParamBuf->SetDataSize(sizeof(VAPictureParameterBufferH264));
}

void PackerVA::PackPriorityParams()
{
    mfxPriority priority = m_va->m_ContextPriority;
    UMCVACompBuffer *GpuPriorityBuf;
    VAContextParameterUpdateBuffer* GpuPriorityBuf_H264Decode = (VAContextParameterUpdateBuffer *)m_va->GetCompBuffer(VAContextParameterUpdateBufferType, &GpuPriorityBuf, sizeof(VAContextParameterUpdateBuffer));
    if (!GpuPriorityBuf_H264Decode)
        throw h264_exception(UMC_ERR_FAILED);

    memset(GpuPriorityBuf_H264Decode, 0, sizeof(VAContextParameterUpdateBuffer));

    GpuPriorityBuf_H264Decode->flags.bits.context_priority_update = 1;
    if(priority == MFX_PRIORITY_LOW)
    {
        GpuPriorityBuf_H264Decode->context_priority.bits.priority = 0;
    }
    else if (priority == MFX_PRIORITY_HIGH)
    {
        GpuPriorityBuf_H264Decode->context_priority.bits.priority = m_va->m_MaxContextPriority;
    }
    else
    {
        GpuPriorityBuf_H264Decode->context_priority.bits.priority = m_va->m_MaxContextPriority/2;
    }

    GpuPriorityBuf->SetDataSize(sizeof(VAContextParameterUpdateBuffer));
}

//returns both NAL unit size (in bytes) and bit offset from start to actual slice data
inline
uint8_t* GetSliceStat(H264Slice* slice, uint32_t* size, uint32_t* offset)
{
    VM_ASSERT(slice);
    VM_ASSERT(size);

    H264HeadersBitstream* bs = slice->GetBitStream();
    VM_ASSERT(bs);

    uint8_t* base;   //ptr to first byte of start code
    bs->GetOrg(reinterpret_cast<uint32_t**>(&base), size);

    if (offset)
    {
        uint8_t* ptr;    //ptr to slice data
        uint32_t position;
        bs->GetState(reinterpret_cast<uint32_t**>(&ptr), &position);

        VM_ASSERT(base != ptr &&
                  !"slice header should be already parsed here"
        );

        //GetState returns internal offset (bits left) but we need consumed bits
        position = 31 - position;
        //bit from start code to slice data
        position += 8 * (ptr - base);

        *offset = position;
    }

    return base;
}

void PackerVA::CreateSliceParamBuffer(H264DecoderFrameInfo * pSliceInfo)
{
    int32_t count = pSliceInfo->GetSliceCount();

    UMCVACompBuffer *pSliceParamBuf;
    size_t sizeOfStruct = sizeof(VASliceParameterBufferH264);

    if (!m_va->IsLongSliceControl())
    {
        sizeOfStruct = sizeof(VASliceParameterBufferBase);
    }
    m_va->GetCompBuffer(VASliceParameterBufferType, &pSliceParamBuf, sizeOfStruct*(count));
    if (!pSliceParamBuf)
        throw h264_exception(UMC_ERR_FAILED);

    pSliceParamBuf->SetNumOfItem(count);
}

void PackerVA::CreateSliceDataBuffer(H264DecoderFrameInfo * pSliceInfo)
{
    int32_t count = pSliceInfo->GetSliceCount();

    uint32_t size = 0;
    for (int32_t i = 0; i < count; i++)
    {
        H264Slice* pSlice = pSliceInfo->GetSlice(i);
        uint32_t NalUnitSize;
        GetSliceStat(pSlice, &NalUnitSize, 0);

        size += NalUnitSize;
    }

    uint32_t const AlignedNalUnitSize = mfx::align2_value(size, 128);

    UMCVACompBuffer* compBuf;
    m_va->GetCompBuffer(VASliceDataBufferType, &compBuf, AlignedNalUnitSize);
    if (!compBuf)
        throw h264_exception(UMC_ERR_FAILED);

    memset((uint8_t*)compBuf->GetPtr() + size, 0, AlignedNalUnitSize - size);

    compBuf->SetDataSize(0);
}

int32_t PackerVA::PackSliceParams(H264Slice *pSlice, int32_t sliceNum, int32_t chopping, int32_t )
{
    int32_t partial_data = CHOPPING_NONE;
    H264DecoderFrame *pCurrentFrame = pSlice->GetCurrentFrame();
    const UMC_H264_DECODER::H264SliceHeader *pSliceHeader = pSlice->GetSliceHeader();

    VAPictureParameterBufferH264* pPicParams_H264 = (VAPictureParameterBufferH264*)m_va->GetCompBuffer(VAPictureParameterBufferType);
    if (!pPicParams_H264)
        throw h264_exception(UMC_ERR_FAILED);

    UMCVACompBuffer* compBuf;
    VASliceParameterBufferH264* pSlice_H264 = (VASliceParameterBufferH264*)m_va->GetCompBuffer(VASliceParameterBufferType, &compBuf);
    if (!pSlice_H264)
        throw h264_exception(UMC_ERR_FAILED);

    if (m_va->IsLongSliceControl())
    {
        pSlice_H264 += sliceNum;
        memset(pSlice_H264, 0, sizeof(VASliceParameterBufferH264));
    }
    else
    {
        pSlice_H264 = (VASliceParameterBufferH264*)((VASliceParameterBufferBase*)pSlice_H264 + sliceNum);
        memset(pSlice_H264, 0, sizeof(VASliceParameterBufferBase));
    }

    uint32_t NalUnitSize, SliceDataOffset;
    uint8_t* pNalUnit = GetSliceStat(pSlice, &NalUnitSize, &SliceDataOffset);
    if (SliceDataOffset >= NalUnitSize * 8)
        //no slice data, skipping
        return CHOPPING_SKIP_SLICE;

    H264HeadersBitstream* pBitstream = pSlice->GetBitStream();
    if (chopping == CHOPPING_SPLIT_SLICE_DATA)
    {
        NalUnitSize = pBitstream->BytesLeft();
        pNalUnit += pBitstream->BytesDecoded();
    }

    UMCVACompBuffer* CompBuf;
    uint8_t *pVAAPI_BitStreamBuffer = (uint8_t*)m_va->GetCompBuffer(VASliceDataBufferType, &CompBuf);
    if (!pVAAPI_BitStreamBuffer)
        throw h264_exception(UMC_ERR_FAILED);

    int32_t AlignedNalUnitSize = NalUnitSize;

    pSlice_H264->slice_data_flag = chopping == CHOPPING_NONE ? VA_SLICE_DATA_FLAG_ALL : VA_SLICE_DATA_FLAG_END;

    if (CompBuf->GetBufferSize() - CompBuf->GetDataSize() < AlignedNalUnitSize)
    {
        AlignedNalUnitSize = NalUnitSize = CompBuf->GetBufferSize() - CompBuf->GetDataSize();
        pBitstream->SetDecodedBytes(pBitstream->BytesDecoded() + NalUnitSize);
        pSlice_H264->slice_data_flag = chopping == CHOPPING_NONE ? VA_SLICE_DATA_FLAG_BEGIN : VA_SLICE_DATA_FLAG_MIDDLE;
        partial_data = CHOPPING_SPLIT_SLICE_DATA;
    }

    pSlice_H264->slice_data_size = NalUnitSize;

    pSlice_H264->slice_data_offset = CompBuf->GetDataSize();
    CompBuf->SetDataSize(pSlice_H264->slice_data_offset + AlignedNalUnitSize);

    VM_ASSERT (CompBuf->GetBufferSize() >= pSlice_H264->slice_data_offset + AlignedNalUnitSize);

    pVAAPI_BitStreamBuffer += pSlice_H264->slice_data_offset;

    std::copy(pNalUnit, pNalUnit + NalUnitSize, pVAAPI_BitStreamBuffer);
    memset(pVAAPI_BitStreamBuffer + NalUnitSize, 0, AlignedNalUnitSize - NalUnitSize);

    if (!m_va->IsLongSliceControl())
        return partial_data;

    pSlice_H264->slice_data_bit_offset = (unsigned short)SliceDataOffset;

    pSlice_H264->first_mb_in_slice = (unsigned short)(pSlice->GetSliceHeader()->first_mb_in_slice >> pSlice->GetSliceHeader()->MbaffFrameFlag);
    pSlice_H264->slice_type = (unsigned char)pSliceHeader->slice_type;
    pSlice_H264->direct_spatial_mv_pred_flag = (unsigned char)pSliceHeader->direct_spatial_mv_pred_flag;
    pSlice_H264->cabac_init_idc = (unsigned char)(pSliceHeader->cabac_init_idc);
    pSlice_H264->slice_qp_delta = (char)pSliceHeader->slice_qp_delta;
    pSlice_H264->disable_deblocking_filter_idc = (unsigned char)pSliceHeader->disable_deblocking_filter_idc;
    pSlice_H264->luma_log2_weight_denom = (unsigned char)pSliceHeader->luma_log2_weight_denom;
    pSlice_H264->chroma_log2_weight_denom = (unsigned char)pSliceHeader->chroma_log2_weight_denom;

    if (pSliceHeader->slice_type == INTRASLICE ||
        pSliceHeader->slice_type == S_INTRASLICE)
    {
        pSlice_H264->num_ref_idx_l0_active_minus1 = 0;
        pSlice_H264->num_ref_idx_l1_active_minus1 = 0;
    }
    else if (pSliceHeader->slice_type == PREDSLICE ||
        pSliceHeader->slice_type == S_PREDSLICE)
    {
        if (pSliceHeader->num_ref_idx_active_override_flag != 0)
        {
            pSlice_H264->num_ref_idx_l0_active_minus1 = (unsigned char)(pSliceHeader->num_ref_idx_l0_active-1);
        }
        else
        {
            pSlice_H264->num_ref_idx_l0_active_minus1 = (unsigned char)(pSlice->GetPicParam()->num_ref_idx_l0_active - 1);
        }
        pSlice_H264->num_ref_idx_l1_active_minus1 = 0;
    }
    else // B slice
    {
        if (pSliceHeader->num_ref_idx_active_override_flag != 0)
        {
            pSlice_H264->num_ref_idx_l0_active_minus1 = (unsigned char)(pSliceHeader->num_ref_idx_l0_active - 1);
            pSlice_H264->num_ref_idx_l1_active_minus1 = (unsigned char)(pSliceHeader->num_ref_idx_l1_active-1);
        }
        else
        {
            pSlice_H264->num_ref_idx_l0_active_minus1 = (unsigned char)(pSlice->GetPicParam()->num_ref_idx_l0_active - 1);
            pSlice_H264->num_ref_idx_l1_active_minus1 = (unsigned char)(pSlice->GetPicParam()->num_ref_idx_l1_active - 1);
        }
    }

    if (pSliceHeader->disable_deblocking_filter_idc != DEBLOCK_FILTER_OFF)
    {
        pSlice_H264->slice_alpha_c0_offset_div2 = (char)(pSliceHeader->slice_alpha_c0_offset / 2);
        pSlice_H264->slice_beta_offset_div2 = (char)(pSliceHeader->slice_beta_offset / 2);
    }

    if ((pPicParams_H264->pic_fields.bits.weighted_pred_flag &&
         ((PREDSLICE == pSliceHeader->slice_type) || (S_PREDSLICE == pSliceHeader->slice_type))) ||
         ((pPicParams_H264->pic_fields.bits.weighted_bipred_idc == 1) && (BPREDSLICE == pSliceHeader->slice_type)))
    {
        //Weights
        const UMC_H264_DECODER::PredWeightTable *pPredWeight[2];
        pPredWeight[0] = pSlice->GetPredWeigthTable(0);
        pPredWeight[1] = pSlice->GetPredWeigthTable(1);

        int32_t  i;
        for(i=0; i < 32; i++)
        {
            if (pPredWeight[0][i].luma_weight_flag)
            {
                pSlice_H264->luma_weight_l0[i] = pPredWeight[0][i].luma_weight;
                pSlice_H264->luma_offset_l0[i] = pPredWeight[0][i].luma_offset;
            }
            else
            {
                pSlice_H264->luma_weight_l0[i] = (uint8_t)pPredWeight[0][i].luma_weight;
                pSlice_H264->luma_offset_l0[i] = 0;
            }
            if (pPredWeight[1][i].luma_weight_flag)
            {
                pSlice_H264->luma_weight_l1[i] = pPredWeight[1][i].luma_weight;
                pSlice_H264->luma_offset_l1[i] = pPredWeight[1][i].luma_offset;
            }
            else
            {
                pSlice_H264->luma_weight_l1[i] = (uint8_t)pPredWeight[1][i].luma_weight;
                pSlice_H264->luma_offset_l1[i] = 0;
            }
            if (pPredWeight[0][i].chroma_weight_flag)
            {
                pSlice_H264->chroma_weight_l0[i][0] = pPredWeight[0][i].chroma_weight[0];
                pSlice_H264->chroma_offset_l0[i][0] = pPredWeight[0][i].chroma_offset[0];
                pSlice_H264->chroma_weight_l0[i][1] = pPredWeight[0][i].chroma_weight[1];
                pSlice_H264->chroma_offset_l0[i][1] = pPredWeight[0][i].chroma_offset[1];
            }
            else
            {
                pSlice_H264->chroma_weight_l0[i][0] = (uint8_t)pPredWeight[0][i].chroma_weight[0];
                pSlice_H264->chroma_offset_l0[i][0] = 0;
                pSlice_H264->chroma_weight_l0[i][1] = (uint8_t)pPredWeight[0][i].chroma_weight[1];
                pSlice_H264->chroma_offset_l0[i][1] = 0;
            }
            if (pPredWeight[1][i].chroma_weight_flag)
            {
                pSlice_H264->chroma_weight_l1[i][0] = pPredWeight[1][i].chroma_weight[0];
                pSlice_H264->chroma_offset_l1[i][0] = pPredWeight[1][i].chroma_offset[0];
                pSlice_H264->chroma_weight_l1[i][1] = pPredWeight[1][i].chroma_weight[1];
                pSlice_H264->chroma_offset_l1[i][1] = pPredWeight[1][i].chroma_offset[1];
            }
            else
            {
                pSlice_H264->chroma_weight_l1[i][0] = (uint8_t)pPredWeight[1][i].chroma_weight[0];
                pSlice_H264->chroma_offset_l1[i][0] = 0;
                pSlice_H264->chroma_weight_l1[i][1] = (uint8_t)pPredWeight[1][i].chroma_weight[1];
                pSlice_H264->chroma_offset_l1[i][1] = 0;
            }
        }
    }

    int32_t realSliceNum = pSlice->GetSliceNum();

    if (pCurrentFrame == nullptr)
        throw h264_exception(UMC_ERR_NULL_PTR);

    const H264DecoderRefPicList* pH264DecRefPicList0 = pCurrentFrame->GetRefPicList(realSliceNum, 0);
    const H264DecoderRefPicList* pH264DecRefPicList1 = pCurrentFrame->GetRefPicList(realSliceNum, 1);

    if (pH264DecRefPicList0 == nullptr || pH264DecRefPicList1 == nullptr)
        throw h264_exception(UMC_ERR_NULL_PTR);

    H264DecoderFrame **pRefPicList0 = pH264DecRefPicList0->m_RefPicList;
    H264DecoderFrame **pRefPicList1 = pH264DecRefPicList1->m_RefPicList;
    ReferenceFlags *pFields0 = pH264DecRefPicList0->m_Flags;
    ReferenceFlags *pFields1 = pH264DecRefPicList1->m_Flags;

    int32_t i;
    for(i = 0; i < 32; i++)
    {
        if (pRefPicList0[i] != NULL && i < pSliceHeader->num_ref_idx_l0_active)
        {
            int32_t defaultIndex = ((0 == pCurrentFrame->m_index) && !pRefPicList0[i]->IsFrameExist()) ? 1 : 0;

            FillRefFrame(&(pSlice_H264->RefPicList0[i]), pRefPicList0[i],
                pFields0[i], pSliceHeader->field_pic_flag, defaultIndex);

            if (pSlice_H264->RefPicList0[i].picture_id == pPicParams_H264->CurrPic.picture_id &&
                pRefPicList0[i]->IsFrameExist())
            {
                pSlice_H264->RefPicList0[i].BottomFieldOrderCnt = 0;
            }
        }
        else
        {
            FillFrameAsInvalid(&(pSlice_H264->RefPicList0[i]));
        }

        if (pRefPicList1[i] != NULL && i < pSliceHeader->num_ref_idx_l1_active)
        {
            int32_t defaultIndex = ((0 == pCurrentFrame->m_index) && !pRefPicList1[i]->IsFrameExist()) ? 1 : 0;

            FillRefFrame(&(pSlice_H264->RefPicList1[i]), pRefPicList1[i],
                pFields1[i], pSliceHeader->field_pic_flag, defaultIndex);

            if (pSlice_H264->RefPicList1[i].picture_id == pPicParams_H264->CurrPic.picture_id && pRefPicList1[i]->IsFrameExist())
            {
                pSlice_H264->RefPicList1[i].BottomFieldOrderCnt = 0;
            }
        }
        else
        {
            FillFrameAsInvalid(&(pSlice_H264->RefPicList1[i]));
        }
    }

    return partial_data;
}

void PackerVA::PackProcessingInfo(H264DecoderFrameInfo * sliceInfo)
{
    VideoProcessingVA *vpVA = m_va->GetVideoProcessingVA();
    if (!vpVA)
        throw h264_exception(UMC_ERR_FAILED);

    UMCVACompBuffer *pipelineVABuf;
    auto* pipelineBuf = reinterpret_cast<VAProcPipelineParameterBuffer *>(m_va->GetCompBuffer(VAProcPipelineParameterBufferType, &pipelineVABuf, sizeof(VAProcPipelineParameterBuffer)));
    if (!pipelineBuf)
        throw h264_exception(UMC_ERR_FAILED);
    pipelineVABuf->SetDataSize(sizeof(VAProcPipelineParameterBuffer));

    MFX_INTERNAL_CPY(pipelineBuf, &vpVA->m_pipelineParams, sizeof(VAProcPipelineParameterBuffer));

    pipelineBuf->surface = m_va->GetSurfaceID(sliceInfo->m_pFrame->m_index); // should filled in packer
    pipelineBuf->additional_outputs = (VASurfaceID*)vpVA->GetCurrentOutputSurface();
    // To keep output aligned, decode downsampling use this fixed combination of chroma sitting type
    pipelineBuf->input_color_properties.chroma_sample_location = VA_CHROMA_SITING_HORIZONTAL_LEFT | VA_CHROMA_SITING_VERTICAL_CENTER;
}

void PackerVA::PackQmatrix(const UMC_H264_DECODER::H264ScalingPicParams * scaling)
{
    UMCVACompBuffer *quantBuf;
    auto* pQmatrix_H264 = reinterpret_cast<VAIQMatrixBufferH264 *>(m_va->GetCompBuffer(VAIQMatrixBufferType, &quantBuf, sizeof(VAIQMatrixBufferH264)));
    if (!pQmatrix_H264)
        throw h264_exception(UMC_ERR_FAILED);
    quantBuf->SetDataSize(sizeof(VAIQMatrixBufferH264));

    int32_t j;

    for(j = 0; j < 6; ++j)
    {
        std::copy(std::begin(scaling->ScalingLists4x4[j].ScalingListCoeffs), std::end(scaling->ScalingLists4x4[j].ScalingListCoeffs), std::begin(pQmatrix_H264->ScalingList4x4[j]));
    }

    for(j = 0; j < 2; ++j)
    {
        std::copy(std::begin(scaling->ScalingLists8x8[j].ScalingListCoeffs), std::end(scaling->ScalingLists8x8[j].ScalingListCoeffs), std::begin(pQmatrix_H264->ScalingList8x8[j]));
    }
}

void PackerVA::BeginFrame(H264DecoderFrame* pFrame, int32_t field)
{
    FrameData* fd = pFrame->GetFrameData();
    VM_ASSERT(fd);

    FrameData::FrameAuxInfo* aux;

    if (!m_enableStreamOut)
        return;

    aux = fd->GetAuxInfo(MFX_EXTBUFF_FEI_DEC_STREAM_OUT);
    if (aux)
    {
        VM_ASSERT(aux->type == MFX_EXTBUFF_FEI_DEC_STREAM_OUT);

        mfxExtFeiDecStreamOut* so =
            reinterpret_cast<mfxExtFeiDecStreamOut*>(aux->ptr);
        if (!so)
            throw h264_exception(UMC_ERR_FAILED);

        uint32_t size = so->NumMBAlloc * sizeof(mfxFeiDecStreamOutMBCtrl);
        if (pFrame->GetAU(field)->IsField())
            size /= 2;

        VAStreamOutBuffer* buffer = NULL;
        m_va->GetCompBuffer(VADecodeStreamoutBufferType, reinterpret_cast<UMCVACompBuffer**>(&buffer), size, pFrame->m_index);
        if (buffer)
        {
            buffer->BindToField(field);
            buffer->RemapRefs(so->RemapRefIdx == MFX_CODINGOPTION_ON);
        }
    }
}

void PackerVA::EndFrame()
{
}

void PackerVA::PackAU(const H264DecoderFrame *pFrame, int32_t isTop)
{
    H264DecoderFrameInfo* sliceInfo =
        const_cast<H264DecoderFrameInfo *>(pFrame->GetAU(isTop));

    uint32_t const count_all = sliceInfo->GetSliceCount();
    if (!m_va || !count_all)
        return;

    uint32_t first_slice = 0;
    H264Slice* slice = sliceInfo->GetSlice(first_slice);

    NAL_Unit_Type const type = slice->GetSliceHeader()->nal_unit_type;
    UMC_H264_DECODER::H264ScalingPicParams const* scaling =
        &slice->GetPicParam()->scaling[type == NAL_UT_CODED_SLICE_EXTENSION ? 1 : 0];
    PackQmatrix(scaling);

    int32_t chopping = CHOPPING_NONE;

    for ( ; first_slice < count_all; )
    {
        PackPicParams(sliceInfo, slice);

        CreateSliceParamBuffer(sliceInfo);
        CreateSliceDataBuffer(sliceInfo);
        if(m_va->m_MaxContextPriority)
            PackPriorityParams();

        uint32_t n = 0, count = 0;
        for (; n < count_all; ++n)
        {
            // put slice header
            H264Slice *pSlice = sliceInfo->GetSlice(first_slice + n);
            chopping = PackSliceParams(pSlice, n, chopping, 0 /* ignored */);
            if (chopping != CHOPPING_SKIP_SLICE)
            {
                ++count;

                if (chopping != CHOPPING_NONE)
                    break;
            }
        }

        first_slice += n;

        UMCVACompBuffer *sliceParamBuf;
        m_va->GetCompBuffer(VASliceParameterBufferType, &sliceParamBuf);
        if (!sliceParamBuf)
            throw h264_exception(UMC_ERR_FAILED);

        sliceParamBuf->SetNumOfItem(count);

        if (m_va->GetVideoProcessingVA())
            PackProcessingInfo(sliceInfo);

        Status sts = m_va->Execute();
        if (sts != UMC_OK)
            throw h264_exception(sts);
    }
}

Status PackerVA::QueryStreamOut(H264DecoderFrame* pFrame)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "PackerVA::QueryStreamOut");
    if (!m_enableStreamOut)
        return UMC_OK;

    VM_ASSERT(dynamic_cast<FEIVideoAccelerator*>(m_va) &&
              "VA should be [FEIVideoAccelerator] if [streamout] is enabled");

    if (!pFrame)
        return UMC_ERR_FAILED;

    FrameData const* fd = pFrame->GetFrameData();
    VM_ASSERT(fd);

    FrameData::FrameAuxInfo const* aux = fd->GetAuxInfo(MFX_EXTBUFF_FEI_DEC_STREAM_OUT);
    if (!aux)
        return UMC_ERR_FAILED;

    VM_ASSERT(aux->type == MFX_EXTBUFF_FEI_DEC_STREAM_OUT);

    mfxExtFeiDecStreamOut* so = reinterpret_cast<mfxExtFeiDecStreamOut*>(aux->ptr);

    if (!so || !so->MB)
        return UMC_ERR_FAILED;

    VM_ASSERT(pFrame->GetTotalMBs() >= 0);
    uint32_t const count = pFrame->GetTotalMBs();

    if (so->NumMBAlloc < count)
        return UMC_ERR_FAILED;

    FEIVideoAccelerator* fei_va =
        static_cast<FEIVideoAccelerator*>(m_va);

    //top field
    int32_t const top = pFrame->GetNumberByParity(0);
    VAStreamOutBuffer* buffer = fei_va->QueryStreamOutBuffer(pFrame->m_index, top);
    if (!buffer || !buffer->GetPtr())
        return UMC_ERR_FAILED;

    mfxFeiDecStreamOutMBCtrl* src = reinterpret_cast<mfxFeiDecStreamOutMBCtrl *>(buffer->GetPtr());

    int32_t const offset1 =  count * top;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "std::copy");
        std::copy(src, src + count, so->MB + offset1);
    }

    fei_va->ReleaseBuffer(buffer);

    if (!pFrame->GetAU(top)->IsField())
        return UMC_OK;

    int32_t const bottom = pFrame->GetNumberByParity(1);
    buffer = fei_va->QueryStreamOutBuffer(pFrame->m_index, bottom);
    if (!buffer || !buffer->GetPtr())
        return UMC_ERR_FAILED;

    src = reinterpret_cast<mfxFeiDecStreamOutMBCtrl *>(buffer->GetPtr());

    int32_t const offset2 =  count * bottom;
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "std::copy");
        std::copy(src, src + count, so->MB + offset2);
    }
    fei_va->ReleaseBuffer(buffer);

    return UMC_OK;
}

#ifdef MFX_ENABLE_CPLIB

PackerVA_CENC::PackerVA_CENC(VideoAccelerator * va, TaskSupplier * supplier)
    : PackerVA(va, supplier)
{
}

void PackerVA_CENC::PackPicParams(H264DecoderFrameInfo * pSliceInfo, H264Slice * pSlice)
{
    const UMC_H264_DECODER::H264SliceHeader* pSliceHeader = pSlice->GetSliceHeader();
    const H264DecoderFrame *pCurrentFrame = pSliceInfo->m_pFrame;

    UMCVACompBuffer *picParamBuf;
    VAPictureParameterBufferH264* pPicParams_H264 = (VAPictureParameterBufferH264*)m_va->GetCompBuffer(VAPictureParameterBufferType, &picParamBuf, sizeof(VAPictureParameterBufferH264));
    if (!pPicParams_H264)
        throw h264_exception(UMC_ERR_FAILED);

    memset(pPicParams_H264, 0, sizeof(VAPictureParameterBufferH264));

    int32_t reference = pCurrentFrame->isShortTermRef() ? 1 : (pCurrentFrame->isLongTermRef() ? 2 : 0);

    FillFrame(&(pPicParams_H264->CurrPic), pCurrentFrame, pSliceHeader->bottom_field_flag, reference, 0);

    pPicParams_H264->CurrPic.flags = 0;

    if (reference == 1)
        pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;

    if (reference == 2)
        pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;

    if (pSliceHeader->field_pic_flag)
    {
        if (pSliceHeader->bottom_field_flag)
        {
            pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_BOTTOM_FIELD;
            pPicParams_H264->CurrPic.TopFieldOrderCnt = 0;
        }
        else
        {
            pPicParams_H264->CurrPic.flags |= VA_PICTURE_H264_TOP_FIELD;
            pPicParams_H264->CurrPic.BottomFieldOrderCnt = 0;
        }
    }

    //create reference picture list
    for (int32_t i = 0; i < 16; i++)
    {
        FillFrameAsInvalid(&(pPicParams_H264->ReferenceFrames[i]));
    }

    int32_t j = 0;

    int32_t viewCount = m_supplier->GetViewCount();

    for (int32_t i = 0; i < viewCount; i++)
    {
        ViewItem & view = m_supplier->GetViewByNumber(i);
        H264DBPList * pDPBList = view.GetDPBList(0);
        int32_t dpbSize = pDPBList->GetDPBSize();

        int32_t start = j;

        for (H264DecoderFrame * pFrm = pDPBList->head(); pFrm && (j < dpbSize + start); pFrm = pFrm->future())
        {
            if (j >= 16)
            {
                VM_ASSERT(false);
                throw h264_exception(UMC_ERR_FAILED);
            }
            VM_ASSERT(j < dpbSize + start);

            int32_t defaultIndex = 0;

            if ((0 == pCurrentFrame->m_index) && !pFrm->IsFrameExist())
            {
                defaultIndex = 1;
            }

            int32_t reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);
            if (!reference && pCurrentFrame != pFrm && (pFrm->isInterViewRef(0) || pFrm->isInterViewRef(1)) &&
                (pFrm->PicOrderCnt(0, 3) == pCurrentFrame->PicOrderCnt(0, 3)) && pFrm->m_viewId < pCurrentFrame->m_viewId)
            { // interview reference
                reference = 1;
            }

            if (!reference)
            {
                continue;
            }

            reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);
            int32_t field = pFrm->m_bottom_field_flag[0];
            FillFrame(&(pPicParams_H264->ReferenceFrames[j]), pFrm,
                field, reference, defaultIndex);

            reference = pFrm->isShortTermRef() ? 1 : (pFrm->isLongTermRef() ? 2 : 0);

            if ((pFrm == pCurrentFrame) && ((&pCurrentFrame->m_pSlicesInfo) != pSliceInfo))
            {
                FillFrame(&(pPicParams_H264->ReferenceFrames[j]), pFrm, 0, reference, defaultIndex);
            }

            j++;
        }
    }

    picParamBuf->SetDataSize(sizeof(VAPictureParameterBufferH264));

    mfxBitstream *bs = m_va->GetProtectedVA()->GetBitstream();
    if (!bs)
        throw h264_exception(UMC_ERR_FAILED);

    auto decryptParam = reinterpret_cast<mfxExtCencParam*>(GetExtendedBuffer(bs->ExtParam, bs->NumExtParam, MFX_EXTBUFF_CENC_PARAM));
    if (!decryptParam)
        throw h264_exception(UMC_ERR_FAILED);

    UMCVACompBuffer *pParamBuf;
    VACencStatusParameters* pCENCStatusParams = (VACencStatusParameters*)m_va->GetCompBuffer(VACencStatusParameterBufferType, &pParamBuf, sizeof(VACencStatusParameters));
    if (!pCENCStatusParams)
        throw h264_exception(UMC_ERR_FAILED);

    pCENCStatusParams->status_report_index_feedback = decryptParam->StatusReportIndex;

    pParamBuf->SetDataSize(sizeof(VACencStatusParameters));
}

void PackerVA_CENC::PackAU(const H264DecoderFrame *pFrame, int32_t isTop)
{
    H264DecoderFrameInfo* sliceInfo =
        const_cast<H264DecoderFrameInfo *>(pFrame->GetAU(isTop));

    uint32_t const count_all = sliceInfo->GetSliceCount();
    if (!m_va || !count_all)
        return;

    uint32_t first_slice = 0;
    H264Slice* slice = sliceInfo->GetSlice(first_slice);

    PackPicParams(sliceInfo, slice);

    Status sts = m_va->Execute();
    if (sts != UMC_OK)
        throw h264_exception(sts);
}

#endif

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE
