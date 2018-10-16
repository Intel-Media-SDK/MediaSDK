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

#include <stdio.h>
#include "umc_defs.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_dec_hw.h"

#include "vaapi_ext_interface.h"

#ifdef _MSVC_LANG
#pragma warning(disable: 4244)
#endif

using namespace UMC;


///////////////////////////////////////////////////////////////////////////
// PackVA class


bool PackVA::SetVideoAccelerator(VideoAccelerator * va)
{
    m_va = va;
    if (!m_va)
    {
        va_mode = UNKNOWN;
        return true;
    }
    if ((m_va->m_Profile & VA_CODEC) != VA_MPEG2)
    {
        return false;
    }
    va_mode = m_va->m_Profile;
    IsProtectedBS = false;


    return true;
}



PackVA::~PackVA(void)
{
    if(pSliceInfoBuffer)
    {
        free(pSliceInfoBuffer);
        pSliceInfoBuffer = NULL;
    }
}

Status PackVA::InitBuffers(int size_bs, int size_sl)
{
    UMCVACompBuffer* CompBuf;
    if(va_mode != VA_VLD_L)
        return UMC_ERR_UNSUPPORTED;

    try
    {
        if (NULL == (vpPictureParam = (VAPictureParameterBufferMPEG2*)m_va->GetCompBuffer(VAPictureParameterBufferType, &CompBuf, sizeof (VAPictureParameterBufferMPEG2))))
            return UMC_ERR_ALLOC;

        if (va_mode == VA_VLD_L)
        {
            if (NULL == (pQmatrixData = (VAIQMatrixBufferMPEG2*)m_va->GetCompBuffer(VAIQMatrixBufferType, &CompBuf, sizeof (VAIQMatrixBufferMPEG2))))
                return UMC_ERR_ALLOC;

            int32_t prev_slice_size_getting = slice_size_getting;
            slice_size_getting = sizeof (VASliceParameterBufferMPEG2) * size_sl;

            if(NULL == pSliceInfoBuffer)
                pSliceInfoBuffer = (VASliceParameterBufferMPEG2*)malloc(slice_size_getting);
            else if (prev_slice_size_getting < slice_size_getting)
            {
                free(pSliceInfoBuffer);
                pSliceInfoBuffer = (VASliceParameterBufferMPEG2*)malloc(slice_size_getting);
            }

            if( NULL == pSliceInfoBuffer)
                return UMC_ERR_ALLOC;

            memset(pSliceInfoBuffer, 0, slice_size_getting);

            if (NULL == (pBitsreamData = (uint8_t*)m_va->GetCompBuffer(VASliceDataBufferType, &CompBuf, size_bs)))
                return UMC_ERR_ALLOC;

            CompBuf->SetPVPState(NULL, 0);


            bs_size_getting = CompBuf->GetBufferSize();
            pSliceInfo = pSliceInfoBuffer;

        }
    }
    catch(...)
    {
        return UMC_ERR_NOT_ENOUGH_BUFFER;
    }
    return UMC_OK;
}

Status
PackVA::SaveVLDParameters(
    sSequenceHeader*    sequenceHeader,
    sPictureHeader*     PictureHeader,
    uint8_t*              bs_start_ptr,
    sFrameBuffer*       frame_buffer,
    int32_t              task_num,
    int32_t              source_mb_height)
{
    (void)source_mb_height;

    int i;
    int NumOfItem;

    if(va_mode == VA_NO)
    {
        return UMC_OK;
    }

    int32_t numSlices = (int32_t)(pSliceInfo - pSliceInfoBuffer);
    for (int i = 1; i < numSlices; i++)
    {
        VASliceParameterBufferMPEG2 & slice = pSliceInfoBuffer[i];
        VASliceParameterBufferMPEG2 & prevSlice = pSliceInfoBuffer[i - 1];
        if (slice.slice_vertical_position == prevSlice.slice_vertical_position && slice.slice_horizontal_position == prevSlice.slice_horizontal_position)
        {
            // remove slice duplication
            std::copy(&pSliceInfoBuffer[i], pSliceInfo, &pSliceInfoBuffer[i-1]);
            numSlices--;
            pSliceInfo--;
            i--;
        }
    }

    if(va_mode == VA_VLD_L)
    {
        int32_t size = 0;
        if(pSliceInfoBuffer < pSliceInfo)
            size = pSliceInfo[-1].slice_data_offset + pSliceInfo[-1].slice_data_size;
        MFX_INTERNAL_CPY((uint8_t*)pBitsreamData, bs_start_ptr, size);

        UMCVACompBuffer* CompBuf;
        m_va->GetCompBuffer(VASliceDataBufferType, &CompBuf);
        int32_t buffer_size = CompBuf->GetBufferSize();
        if(buffer_size > size)
            memset(pBitsreamData + size, 0, buffer_size - size);

        pQmatrixData->load_intra_quantiser_matrix = QmatrixData.load_intra_quantiser_matrix;
        pQmatrixData->load_non_intra_quantiser_matrix = QmatrixData.load_non_intra_quantiser_matrix;
        pQmatrixData->load_chroma_intra_quantiser_matrix = QmatrixData.load_chroma_intra_quantiser_matrix;
        pQmatrixData->load_chroma_non_intra_quantiser_matrix = QmatrixData.load_chroma_non_intra_quantiser_matrix;

        MFX_INTERNAL_CPY((uint8_t*)pQmatrixData->intra_quantiser_matrix, (uint8_t*)QmatrixData.intra_quantiser_matrix,
            sizeof(pQmatrixData->intra_quantiser_matrix));
        MFX_INTERNAL_CPY((uint8_t*)pQmatrixData->non_intra_quantiser_matrix, (uint8_t*)QmatrixData.non_intra_quantiser_matrix,
            sizeof(pQmatrixData->non_intra_quantiser_matrix));
        MFX_INTERNAL_CPY((uint8_t*)pQmatrixData->chroma_intra_quantiser_matrix, (uint8_t*)QmatrixData.chroma_intra_quantiser_matrix,
            sizeof(pQmatrixData->chroma_intra_quantiser_matrix));
        MFX_INTERNAL_CPY((uint8_t*)pQmatrixData->chroma_non_intra_quantiser_matrix, (uint8_t*)QmatrixData.chroma_non_intra_quantiser_matrix,
            sizeof(pQmatrixData->chroma_non_intra_quantiser_matrix));

        NumOfItem = (int)(pSliceInfo - pSliceInfoBuffer);

        VASliceParameterBufferMPEG2 *l_pSliceInfoBuffer = (VASliceParameterBufferMPEG2*)m_va->GetCompBuffer(
            VASliceParameterBufferType,
            &CompBuf,
            (int32_t) (sizeof (VASliceParameterBufferMPEG2))*NumOfItem);

        MFX_INTERNAL_CPY((uint8_t*)l_pSliceInfoBuffer, (uint8_t*)pSliceInfoBuffer,
            sizeof(VASliceParameterBufferMPEG2) * NumOfItem);

        m_va->GetCompBuffer(VASliceParameterBufferType,&CompBuf);
        CompBuf->SetNumOfItem(NumOfItem);
    }

    if (vpPictureParam)
    {
        int32_t pict_type = PictureHeader->picture_coding_type;
        VAPictureParameterBufferMPEG2 *pPictureParam = vpPictureParam;

        pPictureParam->horizontal_size = sequenceHeader->mb_width[task_num] * 16;
        pPictureParam->vertical_size = sequenceHeader->mb_height[task_num] * 16;

        //pPictureParam->forward_reference_picture = m_va->GetSurfaceID(prev_index);//prev_index;
        //pPictureParam->backward_reference_picture = m_va->GetSurfaceID(next_index);//next_index;
        int32_t f_ref_pic = VA_INVALID_SURFACE;

        if(pict_type == MPEG2_P_PICTURE)
        {
            if(m_va->GetSurfaceID(frame_buffer->frame_p_c_n[frame_buffer->frame_p_c_n[frame_buffer->curr_index[task_num]].prev_index].va_index) < 0)
            {
                f_ref_pic = m_va->GetSurfaceID(frame_buffer->frame_p_c_n[frame_buffer->curr_index[task_num]].va_index);
            }
            else
            {
                f_ref_pic = m_va->GetSurfaceID(frame_buffer->frame_p_c_n[frame_buffer->frame_p_c_n[frame_buffer->curr_index[task_num]].prev_index].va_index);
            }
        }
        else if(pict_type == MPEG2_B_PICTURE)
        {
            f_ref_pic = m_va->GetSurfaceID(frame_buffer->frame_p_c_n[frame_buffer->frame_p_c_n[frame_buffer->curr_index[task_num]].prev_index].va_index);
        }

        pPictureParam->forward_reference_picture = (pict_type == MPEG2_I_PICTURE)
            ? VA_INVALID_SURFACE
            : f_ref_pic;
        pPictureParam->backward_reference_picture = (pict_type == MPEG2_B_PICTURE)
            ? m_va->GetSurfaceID(frame_buffer->frame_p_c_n[frame_buffer->frame_p_c_n[frame_buffer->curr_index[task_num]].next_index].va_index)
            : VA_INVALID_SURFACE;

        // NOTE
        // It may occur that after Reset() B-frame is needed only in 1 reference frame for successful decoding,
        // however, we need to provide 2 surfaces.
        if (VA_INVALID_SURFACE == pPictureParam->forward_reference_picture)
            pPictureParam->forward_reference_picture = pPictureParam->backward_reference_picture;

        pPictureParam->picture_coding_type = PictureHeader->picture_coding_type;
        for(i=0, pPictureParam->f_code=0; i<4; i++)
        {
            pPictureParam->f_code |= (PictureHeader->f_code[i] << (12 - 4*i));
        }

        pPictureParam->picture_coding_extension.value = 0;
        pPictureParam->picture_coding_extension.bits.intra_dc_precision = PictureHeader->intra_dc_precision;
        pPictureParam->picture_coding_extension.bits.picture_structure = PictureHeader->picture_structure;
        pPictureParam->picture_coding_extension.bits.top_field_first = PictureHeader->top_field_first;
        pPictureParam->picture_coding_extension.bits.frame_pred_frame_dct = PictureHeader->frame_pred_frame_dct;
        pPictureParam->picture_coding_extension.bits.concealment_motion_vectors = PictureHeader->concealment_motion_vectors;
        pPictureParam->picture_coding_extension.bits.q_scale_type = PictureHeader->q_scale_type;
        pPictureParam->picture_coding_extension.bits.intra_vlc_format = PictureHeader->intra_vlc_format;
        pPictureParam->picture_coding_extension.bits.alternate_scan = PictureHeader->alternate_scan;
        pPictureParam->picture_coding_extension.bits.repeat_first_field = PictureHeader->repeat_first_field;
        pPictureParam->picture_coding_extension.bits.progressive_frame = PictureHeader->progressive_frame;
        pPictureParam->picture_coding_extension.bits.is_first_field = !((PictureHeader->picture_structure != FRAME_PICTURE)
                                            && (field_buffer_index == 1));
    }

    return UMC_OK;
}

Status
PackVA::SetBufferSize(
    int32_t          numMB,
    int             size_bs,
    int             size_sl)
{
    (void)size_bs;

        try
        {
            UMCVACompBuffer*  CompBuf  = NULL;

            m_va->GetCompBuffer(
                VAPictureParameterBufferType,
                &CompBuf,
                (int32_t)sizeof (VAPictureParameterBufferMPEG2));
            CompBuf->SetDataSize((int32_t) (sizeof (VAPictureParameterBufferMPEG2)));
            CompBuf->FirstMb = 0;
            CompBuf->NumOfMB = 0;

            if (va_mode == VA_VLD_W)
            {
                m_va->GetCompBuffer(
                    VAIQMatrixBufferType,
                    &CompBuf,
                    (int32_t)sizeof(VAIQMatrixBufferMPEG2));
                CompBuf->SetDataSize((int32_t)sizeof(VAIQMatrixBufferMPEG2));
                CompBuf->FirstMb = 0;
                CompBuf->NumOfMB = 0;

                m_va->GetCompBuffer(
                    VASliceParameterBufferType,
                    &CompBuf,
                    (int32_t)sizeof(VASliceParameterBufferMPEG2)*size_sl);
                CompBuf->SetDataSize((int32_t) ((uint8_t*)pSliceInfo - (uint8_t*)pSliceInfoBuffer));
                CompBuf->FirstMb = 0;
                CompBuf->NumOfMB = numMB;

                m_va->GetCompBuffer(
                    VASliceDataBufferType,
                    &CompBuf,
                    bs_size);
                CompBuf->SetDataSize(bs_size);
                CompBuf->FirstMb = 0;
                CompBuf->NumOfMB = numMB;
            }
        }
        catch (...)
        {
            return UMC_ERR_NOT_ENOUGH_BUFFER;
        }

    return UMC_OK;
}


#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
