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

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"
#include "umc_mpeg2_slice.h"
#include "umc_mpeg2_frame.h"
#include "umc_mpeg2_va_packer.h"
#include "mfx_session.h"

namespace UMC_MPEG2_DECODER
{
    Packer * Packer::CreatePacker(UMC::VideoAccelerator * va)
    {
        return new PackerVA(va);
    }

    Packer::Packer(UMC::VideoAccelerator * va)
        : m_va(va)
    {}

    Packer::~Packer()
    {}


/****************************************************************************************************/
// Linux VAAPI packer implementation
/****************************************************************************************************/

    PackerVA::PackerVA(UMC::VideoAccelerator * va)
        : Packer(va)
    {}

    void PackerVA::BeginFrame() {}
    void PackerVA::EndFrame() {}

    // Pack picture
    void PackerVA::PackAU(MPEG2DecoderFrame const& frame, uint8_t fieldIndex)
    {
        int sliceCount = frame.GetAU(fieldIndex)->GetSliceCount();

        if (!sliceCount)
            return;

        const MPEG2DecoderFrameInfo & frameInfo = *frame.GetAU(fieldIndex);
        PackPicParams(frame, fieldIndex);

        PackQmatrix(frameInfo);

        CreateSliceDataBuffer(frameInfo);
        CreateSliceParamBuffer(frameInfo);

        uint32_t sliceNum = 0;
        for (int32_t n = 0; n < sliceCount; n++)
        {
            PackSliceParams(*frame.GetAU(fieldIndex)->GetSlice(n), sliceNum);
            sliceNum++;
        }

        if(m_va->m_MaxContextPriority)
            PackPriorityParams();

        auto s = m_va->Execute();
        if(s != UMC::UMC_OK)
            throw mpeg2_exception(s);
    }

    void PackerVA::PackPriorityParams()
    {
        mfxPriority priority = m_va->m_ContextPriority;
        UMC::UMCVACompBuffer *GpuPriorityBuf;
        VAContextParameterUpdateBuffer* GpuPriorityBuf_Mpeg2Decode = (VAContextParameterUpdateBuffer *)m_va->GetCompBuffer(VAContextParameterUpdateBufferType, &GpuPriorityBuf, sizeof(VAContextParameterUpdateBuffer));
        if (!GpuPriorityBuf_Mpeg2Decode)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        memset(GpuPriorityBuf_Mpeg2Decode, 0, sizeof(VAContextParameterUpdateBuffer));
        GpuPriorityBuf_Mpeg2Decode->flags.bits.context_priority_update = 1;

        if(priority == MFX_PRIORITY_LOW)
        {
            GpuPriorityBuf_Mpeg2Decode->context_priority.bits.priority = 0;
        }
        else if (priority == MFX_PRIORITY_HIGH)
        {
            GpuPriorityBuf_Mpeg2Decode->context_priority.bits.priority = m_va->m_MaxContextPriority;
        }
        else
        {
            GpuPriorityBuf_Mpeg2Decode->context_priority.bits.priority = m_va->m_MaxContextPriority/2;
        }

        GpuPriorityBuf->SetDataSize(sizeof(VAContextParameterUpdateBuffer));
    }


    // Pack picture parameters
    void PackerVA::PackPicParams(const MPEG2DecoderFrame & frame, uint8_t fieldIndex)
    {
        const MPEG2DecoderFrameInfo & frameInfo = *frame.GetAU(fieldIndex);
        const auto slice  = frameInfo.GetSlice(0);
        const auto seq    = slice->GetSeqHeader();
        const auto pic    = slice->GetPicHeader();
        const auto picExt = slice->GetPicExtHeader();

        UMC::UMCVACompBuffer *picParamBuf;
        auto picParam = (VAPictureParameterBufferMPEG2*)m_va->GetCompBuffer(VAPictureParameterBufferType, &picParamBuf, sizeof(VAPictureParameterBufferMPEG2)); // Allocate buffer
        if (!picParam)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        picParamBuf->SetDataSize(sizeof(VAPictureParameterBufferMPEG2));
        memset(picParam, 0, sizeof(VAPictureParameterBufferMPEG2));

        picParam->horizontal_size = seq.horizontal_size_value;
        picParam->vertical_size   = seq.vertical_size_value;

        const auto refPic0 = frameInfo.GetForwardRefPic();
        const auto refPic1 = frameInfo.GetBackwardRefPic();

        if (MPEG2_P_PICTURE == pic.picture_coding_type && refPic0)
        {
            picParam->forward_reference_picture  = (VASurfaceID)m_va->GetSurfaceID(refPic0->GetMemID());
            picParam->backward_reference_picture = VA_INVALID_SURFACE;
        }
        else if (MPEG2_B_PICTURE == pic.picture_coding_type && refPic0 && refPic1)
        {
            picParam->forward_reference_picture  = (VASurfaceID)m_va->GetSurfaceID(refPic0->GetMemID());
            picParam->backward_reference_picture = (VASurfaceID)m_va->GetSurfaceID(refPic1->GetMemID());
        }
        else
        {
            picParam->forward_reference_picture  = VA_INVALID_SURFACE;
            picParam->backward_reference_picture = VA_INVALID_SURFACE;
        }

        picParam->picture_coding_type = pic.picture_coding_type;
        for (uint8_t i = 0; i < 4; ++i)
        {
            picParam->f_code |= (picExt.f_code[i] << (12 - 4*i));
        }

        picParam->picture_coding_extension.bits.intra_dc_precision         = picExt.intra_dc_precision;
        picParam->picture_coding_extension.bits.picture_structure          = picExt.picture_structure;
        picParam->picture_coding_extension.bits.top_field_first            = picExt.top_field_first;
        picParam->picture_coding_extension.bits.frame_pred_frame_dct       = picExt.frame_pred_frame_dct;
        picParam->picture_coding_extension.bits.concealment_motion_vectors = picExt.concealment_motion_vectors;
        picParam->picture_coding_extension.bits.q_scale_type               = picExt.q_scale_type;
        picParam->picture_coding_extension.bits.intra_vlc_format           = picExt.intra_vlc_format;
        picParam->picture_coding_extension.bits.alternate_scan             = picExt.alternate_scan;
        picParam->picture_coding_extension.bits.repeat_first_field         = picExt.repeat_first_field;
        picParam->picture_coding_extension.bits.progressive_frame          = picExt.progressive_frame;
        picParam->picture_coding_extension.bits.is_first_field             = !( (picExt.picture_structure != FRM_PICTURE) && (fieldIndex == 1) ) ;
    }

    void PackerVA::CreateSliceParamBuffer(const MPEG2DecoderFrameInfo & info)
    {
        uint32_t count = info.GetSliceCount();

        UMC::UMCVACompBuffer *pSliceParamBuf;
        size_t sizeOfStruct = m_va->IsLongSliceControl() ? sizeof(VASliceParameterBufferMPEG2) : sizeof(VASliceParameterBufferBase);
        m_va->GetCompBuffer(VASliceParameterBufferType, &pSliceParamBuf, sizeOfStruct*(count)); // Allocate buffer
        if (!pSliceParamBuf)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        pSliceParamBuf->SetNumOfItem(count);
    }

    // Calculate size of slice data and create buffer
    void PackerVA::CreateSliceDataBuffer(const MPEG2DecoderFrameInfo & info)
    {
        uint32_t count = info.GetSliceCount();
        uint32_t size = 0;

        for (uint32_t i = 0; i < count; ++i)
        {
            MPEG2Slice* slice = info.GetSlice(i);

            uint8_t *nalUnit; //ptr to first byte of start code
            uint32_t nalUnitSize; // size of NAL unit in byte

            slice->GetBitStream().GetOrg(nalUnit, nalUnitSize);
            size += nalUnitSize + sizeof(start_code_prefix);
        }

        UMC::UMCVACompBuffer* compBuf;
        m_va->GetCompBuffer(VASliceDataBufferType, &compBuf, size); // Allocate buffer
        if (!compBuf)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        compBuf->SetDataSize(0);
    }

    // Pack slice parameters
    void PackerVA::PackSliceParams(const MPEG2Slice & slice, uint32_t sliceNum)
    {
        const auto sliceHeader = slice.GetSliceHeader();
        const auto pic         = slice.GetPicHeader();

        UMC::UMCVACompBuffer* compBuf;
        auto sliceParams = (VASliceParameterBufferMPEG2*)m_va->GetCompBuffer(VASliceParameterBufferType, &compBuf);
        if (!sliceParams)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        if (m_va->IsLongSliceControl())
        {
            sliceParams += sliceNum;
            memset(sliceParams, 0, sizeof(VASliceParameterBufferMPEG2));
        }
        else
        {
            sliceParams = (VASliceParameterBufferMPEG2*)((VASliceParameterBufferBase*)sliceParams + sliceNum);
            memset(sliceParams, 0, sizeof(VASliceParameterBufferBase));
        }

        uint8_t *  rawDataPtr = nullptr;
        uint32_t  rawDataSize = 0;

        slice.GetBitStream().GetOrg(rawDataPtr, rawDataSize);

        sliceParams->slice_data_size = rawDataSize + prefix_size;
        sliceParams->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

        auto sliceDataBuf = (uint8_t*)m_va->GetCompBuffer(VASliceDataBufferType, &compBuf);
        if (!sliceDataBuf)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        sliceParams->slice_data_offset = compBuf->GetDataSize();
        sliceDataBuf += sliceParams->slice_data_offset;
        std::copy(start_code_prefix, start_code_prefix + prefix_size, sliceDataBuf);
        std::copy(rawDataPtr, rawDataPtr + rawDataSize, sliceDataBuf + prefix_size);
        compBuf->SetDataSize(sliceParams->slice_data_offset + sliceParams->slice_data_size);

        if (!m_va->IsLongSliceControl())
            return;

        sliceParams->macroblock_offset         = sliceHeader.mbOffset + prefix_size * 8;
        sliceParams->slice_horizontal_position = sliceHeader.macroblockAddressIncrement;
        sliceParams->slice_vertical_position   = sliceHeader.slice_vertical_position - 1;
        sliceParams->quantiser_scale_code      = sliceHeader.quantiser_scale_code;
        sliceParams->intra_slice_flag          = (pic.picture_coding_type == MPEG2_I_PICTURE);

        return;
    }

    static const uint8_t default_intra_quantizer_matrix[64] =
    {
         8, 16, 16, 19, 16, 19, 22, 22,
        22, 22, 22, 22, 26, 24, 26, 27,
        27, 27, 26, 26, 26, 26, 27, 27,
        27, 29, 29, 29, 34, 34, 34, 29,
        29, 29, 27, 27, 29, 29, 32, 32,
        34, 34, 37, 38, 37, 35, 35, 34,
        35, 38, 38, 40, 40, 40, 48, 48,
        46, 46, 56, 56, 58, 69, 69, 83
    };

    static const uint8_t default_non_intra_quantizer_matrix[64] =
    {
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16
    };

    // Pack matrix parameters
    void PackerVA::PackQmatrix(const MPEG2DecoderFrameInfo & info)
    {
        UMC::UMCVACompBuffer *quantBuf;
        auto qmatrix = (VAIQMatrixBufferMPEG2*)m_va->GetCompBuffer(VAIQMatrixBufferType, &quantBuf, sizeof(VAIQMatrixBufferMPEG2)); // Allocate buffer
        if (!qmatrix)
            throw mpeg2_exception(UMC::UMC_ERR_FAILED);

        quantBuf->SetDataSize(sizeof(VAIQMatrixBufferMPEG2));

        const auto slice    = info.GetSlice(0);
        const auto seq      = slice->GetSeqHeader();
        const auto customQM = slice->GetQMatrix();

        // intra_quantizer_matrix
        qmatrix->load_intra_quantiser_matrix = 1;
        const uint8_t * intra_quantizer_matrix = customQM && customQM->load_intra_quantiser_matrix ?
                                                    customQM->intra_quantiser_matrix :
                                                    (seq.load_intra_quantiser_matrix ? seq.intra_quantiser_matrix : default_intra_quantizer_matrix);
        std::copy(intra_quantizer_matrix, intra_quantizer_matrix + 64, qmatrix->intra_quantiser_matrix);

        // chroma_intra_quantiser_matrix
        qmatrix->load_chroma_intra_quantiser_matrix = 1;
        const uint8_t * chroma_intra_quantiser_matrix = customQM && customQM->load_chroma_intra_quantiser_matrix ?
                                                            customQM->chroma_intra_quantiser_matrix :
                                                            (seq.load_intra_quantiser_matrix ? seq.intra_quantiser_matrix : default_intra_quantizer_matrix);
        std::copy(chroma_intra_quantiser_matrix, chroma_intra_quantiser_matrix + 64, qmatrix->chroma_intra_quantiser_matrix);

        // non_intra_quantiser_matrix
        qmatrix->load_non_intra_quantiser_matrix = 1;
        const uint8_t * non_intra_quantiser_matrix = customQM && customQM->load_non_intra_quantiser_matrix ?
                                                        customQM->non_intra_quantiser_matrix :
                                                        (seq.load_non_intra_quantiser_matrix ? seq.non_intra_quantiser_matrix : default_non_intra_quantizer_matrix);
        std::copy(non_intra_quantiser_matrix, non_intra_quantiser_matrix + 64, qmatrix->non_intra_quantiser_matrix);

        // chroma_non_intra_quantiser_matrix
        qmatrix->load_chroma_non_intra_quantiser_matrix = 1;
        const uint8_t * chroma_non_intra_quantiser_matrix = customQM && customQM->load_chroma_non_intra_quantiser_matrix ?
                                                                customQM->chroma_non_intra_quantiser_matrix :
                                                                (seq.load_non_intra_quantiser_matrix ? seq.non_intra_quantiser_matrix : default_non_intra_quantizer_matrix);
        std::copy(chroma_non_intra_quantiser_matrix, chroma_non_intra_quantiser_matrix + 64, qmatrix->chroma_non_intra_quantiser_matrix);
    }
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
