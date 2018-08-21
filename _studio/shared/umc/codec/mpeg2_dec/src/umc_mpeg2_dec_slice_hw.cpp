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
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include <vector>
#include "umc_mpeg2_dec_hw.h"
#include "umc_mpeg2_dec_defs.h"
#include "umc_mpeg2_dec_bstream.h"
#include "mfx_trace.h"

#ifdef _MSVC_LANG
#pragma warning(disable: 4244)
#endif

using namespace UMC;

//extern uint8_t default_intra_quantizer_matrix[64];
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

struct VLCEntry
{
    int8_t value;
    int8_t length;
};

static const VLCEntry MBAddrIncrTabB1_1[16] = {
    {-1, 0}, {-1, 0}, {7, 5}, {6, 5}, {5, 4}, {5, 4}, {4, 4}, {4, 4}, {3, 3}, {3, 3}, {3, 3}, {3, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}};

static const VLCEntry MBAddrIncrTabB1_2[104] = {
    {33, 11}, {32, 11}, {31, 11}, {30, 11}, {29, 11}, {28, 11}, {27, 11}, {26, 11}, {25, 11}, {24, 11}, {23, 11}, {22, 11}, {21, 10}, {21, 10}, {20, 10}, {20, 10}, {19, 10}, {19, 10}, {18, 10}, {18, 10}, {17, 10}, {17, 10}, {16, 10}, {16, 10}, {15, 8}, {15, 8}, {15, 8}, {15, 8}, {15, 8}, {15, 8}, {15, 8}, {15, 8}, {14, 8}, {14, 8}, {14, 8}, {14, 8}, {14, 8}, {14, 8}, {14, 8}, {14, 8}, {13, 8}, {13, 8}, {13, 8}, {13, 8}, {13, 8}, {13, 8}, {13, 8}, {13, 8}, {12, 8}, {12, 8}, {12, 8}, {12, 8}, {12, 8}, {12, 8}, {12, 8}, {12, 8}, {11, 8}, {11, 8}, {11, 8}, {11, 8}, {11, 8}, {11, 8}, {11, 8}, {11, 8}, {10, 8}, {10, 8}, {10, 8}, {10, 8}, {10, 8}, {10, 8}, {10, 8}, {10, 8}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {9, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}, {8, 7}};

int32_t DecoderMBInc(VideoContext  *video)
{
    int32_t macroblock_address_increment = 0;
    int32_t cc;

    for (;;)
    {
        SHOW_BITS(video->bs, 11, cc)
        if (cc >= 24)
            break;

        if (cc != 15) {
            if (cc != 8) // if macroblock_escape
            {
                return 1;
            }

            macroblock_address_increment += 33;
        }

        GET_BITS(video->bs, 11, cc);
    }

    if (cc >= 1024) {
        GET_BITS(video->bs, 1, cc);
        return macroblock_address_increment + 1;
    }

    int32_t cc1;

    if (cc >= 128) {
        cc >>= 6;
        GET_BITS(video->bs, MBAddrIncrTabB1_1[cc].length, cc1);
        return macroblock_address_increment + MBAddrIncrTabB1_1[cc].value;
    }

    cc -= 24;
    GET_BITS(video->bs, MBAddrIncrTabB1_2[cc].length, cc1);
    return macroblock_address_increment + MBAddrIncrTabB1_2[cc].value;
}


Status MPEG2VideoDecoderHW::Init(BaseCodecParams *pInit)
{
    VideoDecoderParams *init = DynamicCast<VideoDecoderParams, BaseCodecParams>(pInit);
    if(!init)
        return UMC_ERR_INIT;

    if(!pack_w.SetVideoAccelerator(init->pVideoAccelerator))
        return UMC_ERR_UNSUPPORTED;

    return MPEG2VideoDecoderBase::Init(init);
}

Status MPEG2VideoDecoderHW::DecodeSliceHeader(VideoContext *video, int task_num)
{
    uint32_t extra_bit_slice;
    uint32_t code;
    int32_t bytes_remain = 0;
    int32_t bit_pos = 0;
    bool isCorrupted = false;

    if (!video)
    {
        return UMC_ERR_NULL_PTR;
    }

    FIND_START_CODE(video->bs, code)
    uint32_t start_code = code;

    if(code == (uint32_t)UMC_ERR_NOT_ENOUGH_DATA)
    {
        if (VA_VLD_W == pack_w.va_mode && false == pack_w.IsProtectedBS)
        {
            // update latest slice information
            uint8_t *start_ptr = GET_BYTE_PTR(video->bs);
            uint8_t *end_ptr = GET_END_PTR(video->bs);
            uint8_t *ptr = start_ptr;
            uint32_t count = 0;

            uint32_t zeroes = 0;
            //calculating the size of slice data, if 4 consecutive 0s are found, it indicates the end of the slice no further check is needed
            while (ptr < end_ptr)
            {
                if (*ptr++) zeroes = 0;
                else ++zeroes;
                ++count;
                if (zeroes == 4) break;
             }
            // Adjust the count by reducing the number of 0s found, either 4 consecutive 0s or 0-3 0s existing at the end of the data.
            // Based on the standard, they should be stuffing bytes.
            count -= zeroes;

            // update slice info structures (+ extra 10 bytes, this is @to do@)
            if(pack_w.pSliceInfoBuffer < pack_w.pSliceInfo)
                pack_w.pSliceInfo[-1].slice_data_size = count + 10;
            //fprintf(otl, "slice_data_size %x in DecodeSliceHeader(%d)\n", pack_w.pSliceInfo[-1].slice_data_size, __LINE__);
        }

        SKIP_TO_END(video->bs);

        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    if(pack_w.va_mode == VA_VLD_W) {
        // start of slice code
        bytes_remain = (int32_t)GET_REMAINED_BYTES(video->bs);
        bit_pos = (int32_t)GET_BIT_OFFSET(video->bs);
        if(pack_l.va_mode == VA_VLD_L) {
            if(pack_l.pSliceInfo > pack_l.pSliceInfoBuffer)
            {
                pack_l.pSliceInfo[-1].slice_data_size -= bytes_remain;

                pack_l.bs_size = pack_l.pSliceInfo[-1].slice_data_offset
                               + pack_l.pSliceInfo[-1].slice_data_size;    //AO: added by me
            }
            else
            {
                pack_l.pSliceStart = GET_START_PTR(Video[task_num][0]->bs)+bit_pos/8;
                pack_l.bs_size = 0;
            }
        }
    }

    if(code == PICTURE_START_CODE ||
       code > 0x1AF)
    {
      return UMC_ERR_NOT_ENOUGH_DATA;
    }
    // if (video) redundant checking
    {
        SKIP_BITS_32(video->bs);
    }

    video->slice_vertical_position = (code & 0xff);

    if(video->clip_info_height > 2800)
    {
        if(video->slice_vertical_position > 0x80)
          return UMC_ERR_INVALID_STREAM;
        GET_TO9BITS(video->bs, 3, code)
        video->slice_vertical_position += code << 7;
    }
    if( video->slice_vertical_position > PictureHeader[task_num].max_slice_vert_pos)
    {
        video->slice_vertical_position = PictureHeader[task_num].max_slice_vert_pos;

        if(pack_l.pSliceInfo > pack_l.pSliceInfoBuffer)
            pack_l.pSliceInfo[-1].slice_data_size += bytes_remain;

        isCorrupted = true;
        return UMC_WRN_INVALID_STREAM;
    }

    if((sequenceHeader.extension_start_code_ID[task_num] == SEQUENCE_SCALABLE_EXTENSION_ID) &&
        (sequenceHeader.scalable_mode[task_num] == DATA_PARTITIONING))
    {
        GET_TO9BITS(video->bs, 7, code)
        return UMC_ERR_UNSUPPORTED;
    }

    GET_TO9BITS(video->bs, 5, video->cur_q_scale)
    if(video->cur_q_scale == 0)
    {
        isCorrupted = true;
        //return UMC_ERR_INVALID_STREAM;
    }

    GET_1BIT(video->bs,extra_bit_slice)
    while(extra_bit_slice)
    {
        GET_TO9BITS(video->bs, 9, code)
        extra_bit_slice = code & 1;
    }


    if(pack_l.va_mode == VA_VLD_L)
    {
        if (GET_REMAINED_BYTES(video->bs) <= 0)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        uint32_t slice_idx = pack_w.pSliceInfo - pack_w.pSliceInfoBuffer;
        uint32_t num_allocated_slices = pack_w.slice_size_getting/sizeof(VASliceParameterBufferMPEG2);

        if (slice_idx < num_allocated_slices)
        {
            pack_l.pSliceInfo->macroblock_offset = GET_BIT_OFFSET(video->bs) - bit_pos;
            pack_l.pSliceInfo->slice_data_offset = GET_START_PTR(Video[task_num][0]->bs)+bit_pos/8-pack_l.pSliceStart;
            pack_l.pSliceInfo->quantiser_scale_code = video->cur_q_scale;
            pack_l.pSliceInfo->slice_data_size = bytes_remain;
            pack_l.pSliceInfo->intra_slice_flag = PictureHeader[task_num].picture_coding_type == MPEG2_I_PICTURE;

            if (video->stream_type != MPEG1_VIDEO)
            {
                pack_l.pSliceInfo->slice_vertical_position = start_code - 0x00000101; //SLICE_MIN_START_CODE 0x00000101

                uint32_t macroblock_address_increment=1;


                if (IS_NEXTBIT1(video->bs))
                {
                    SKIP_BITS(video->bs, 1)
                }
                else
                {
                    macroblock_address_increment = DecoderMBInc(video);
                }
                macroblock_address_increment--;
                pack_w.pSliceInfo->slice_horizontal_position = macroblock_address_increment;
            }
            else
                pack_l.pSliceInfo->slice_vertical_position = video->slice_vertical_position-1;

            pack_l.pSliceInfo->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
        }
        else
            return UMC_ERR_FAILED;
    }

    if (video->stream_type != MPEG1_VIDEO)
    {
      video->cur_q_scale = q_scale[PictureHeader[task_num].q_scale_type][video->cur_q_scale];
    }

    RESET_PMV(video->PMV)

    video->macroblock_motion_forward  = 0;
    video->macroblock_motion_backward = 0;

    video->m_bNewSlice = true;

    if (true != frame_buffer.frame_p_c_n[frame_buffer.curr_index[task_num]].isCorrupted && true == isCorrupted)
    {
        frame_buffer.frame_p_c_n[frame_buffer.curr_index[task_num]].isCorrupted = true;
    }

    return UMC_OK;
}

Status MPEG2VideoDecoderHW::DecodeSlice(VideoContext  *video, int task_num)
{
    (void)video;
    (void)task_num;

    pack_w.pSliceInfo++;
    return UMC_OK;
}

Status MPEG2VideoDecoderHW::SaveDecoderState()
{
    frameBuffer_backup_previous = frameBuffer_backup;
    frameBuffer_backup = frame_buffer;
    b_curr_number_backup = sequenceHeader.b_curr_number;
    first_i_occure_backup = sequenceHeader.first_i_occure;
    frame_count_backup = sequenceHeader.frame_count;
    return UMC_OK;
}

Status MPEG2VideoDecoderHW::RestoreDecoderState()
{
    frame_buffer = frameBuffer_backup;
    frameBuffer_backup = frameBuffer_backup_previous;
    sequenceHeader.b_curr_number = b_curr_number_backup;
    sequenceHeader.first_i_occure = first_i_occure_backup;
    sequenceHeader.frame_count = frame_count_backup;
    return UMC_OK;
}

Status MPEG2VideoDecoderHW::RestoreDecoderStateAndRemoveLastField()
{
    frame_buffer = frameBuffer_backup_previous;
    sequenceHeader.b_curr_number = b_curr_number_backup;
    sequenceHeader.first_i_occure = first_i_occure_backup;
    sequenceHeader.frame_count = frame_count_backup;
    return UMC_OK;
}

Status MPEG2VideoDecoderHW::PostProcessFrame(int display_index, int task_num)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MPEG2VideoDecoderBase::PostProcessFrame");
    Status umcRes = UMC_OK;

      if(pack_w.va_mode != VA_NO)
      {
          if(pack_w.pSliceInfoBuffer < pack_w.pSliceInfo && pack_w.va_mode == VA_VLD_W)
          {
             // printf("save data at the end of frame\n");

            pack_w.bs_size = pack_w.pSliceInfo[-1].slice_data_offset
                           + pack_w.pSliceInfo[-1].slice_data_size;
            if (pack_w.bs_size >= pack_w.bs_size_getting)
            {
                VASliceParameterBufferMPEG2  s_info;
                bool slice_split = false;
                int32_t sz = 0, sz_align = 0;
                memcpy_s(&s_info, sizeof(VASliceParameterBufferMPEG2), &pack_w.pSliceInfo[-1], sizeof(VASliceParameterBufferMPEG2));
                pack_w.pSliceInfo--;
                int size_bs = GET_END_PTR(Video[task_num][0]->bs) - GET_START_PTR(Video[task_num][0]->bs)+4;   // ao: I copied from BeginVAFrame. Is it ok? or == bs_size
                int size_sl = m_ClipInfo.clip_info.width*m_ClipInfo.clip_info.height/256;      // ao: I copied from BeginVAFrame. Is it ok?
                if(pack_w.pSliceInfoBuffer < pack_w.pSliceInfo)
                {
                  //  printf("Save previous slices\n");
mm:
                    int32_t numMB = (PictureHeader[task_num].picture_structure == FRAME_PICTURE)
                        ? sequenceHeader.numMB[task_num]
                        : sequenceHeader.numMB[task_num]/2;

                    if (pack_w.SetBufferSize(
                        numMB,
                        size_bs,
                        size_sl)!= UMC_OK)
                    {
                      return UMC_ERR_NOT_ENOUGH_BUFFER;
                    }

                    umcRes = pack_w.SaveVLDParameters(
                        &sequenceHeader,
                        &PictureHeader[task_num],
                        pack_w.pSliceStart,
                        &frame_buffer,task_num,
                        ((m_ClipInfo.clip_info.height + 15) >> 4));
                    if (UMC_OK != umcRes)
                        return umcRes;

                    umcRes = pack_w.m_va->Execute();
                    if (UMC_OK != umcRes)
                        return umcRes;

                    m_executed = true;

                    pack_w.add_to_slice_start = 0;

                    if(pack_w.IsProtectedBS)
                    {
                        if(!pack_w.is_bs_aligned_16)
                        {
                            pack_w.add_to_slice_start = sz - (sz_align - 16);
                        }
                    }
                }//if(pack_w.pSliceInfoBuffer < pack_w.pSliceInfo)


                pack_w.InitBuffers(size_bs, size_sl);

                memcpy_s(&pack_w.pSliceInfo[0], sizeof(s_info), &s_info, sizeof(s_info));

                pack_w.pSliceStart += pack_w.pSliceInfo[0].slice_data_offset;
                pack_w.pSliceInfo[0].slice_data_offset = 0;

                if(!pack_w.IsProtectedBS)
                {
                    pack_w.bs_size = pack_w.pSliceInfo[0].slice_data_size;
                }
                else
                {
                    // AO: add PAVP technologies
                }

                if(pack_w.bs_size >= pack_w.bs_size_getting)
                {
                    slice_split = true;
                    pack_w.pSliceInfo->slice_data_size = pack_w.bs_size_getting - 1;
                    pack_w.pSliceInfo->slice_data_flag = VA_SLICE_DATA_FLAG_BEGIN;
                    pack_w.bs_size -= (pack_w.bs_size_getting - 1);
                    s_info.slice_data_offset = pack_w.bs_size_getting - 1;
                    s_info.slice_data_size -= pack_w.pSliceInfo->slice_data_size;
                    pack_w.pSliceInfo++;

                    goto mm;
                }
                else
                {
                    if(slice_split)
                        pack_w.pSliceInfo->slice_data_flag = VA_SLICE_DATA_FLAG_END;
                }

                pack_w.pSliceInfo++;
            }
            //  printf("Save last slice\n");

            umcRes = pack_w.SaveVLDParameters(
                &sequenceHeader,
                &PictureHeader[task_num],
                pack_w.pSliceStart,
                &frame_buffer,
                task_num);
            if (UMC_OK != umcRes)
            {
                return umcRes;
            }

            int32_t numMB = (PictureHeader[task_num].picture_structure == FRAME_PICTURE)
                ? sequenceHeader.numMB[task_num]
                : sequenceHeader.numMB[task_num]/2;

              if(pack_w.SetBufferSize(numMB)!= UMC_OK)
                  return UMC_ERR_NOT_ENOUGH_BUFFER;

              umcRes = pack_w.m_va->Execute();
              if (UMC_OK != umcRes)
                  return UMC_ERR_DEVICE_FAILED;

              m_executed = true;
          }
          umcRes = pack_w.m_va->EndFrame();
          if (UMC_OK != umcRes || !m_executed)
              return UMC_ERR_DEVICE_FAILED;

          m_executed = false;

          if(display_index < 0)
          {
              return UMC_ERR_NOT_ENOUGH_DATA;
          }

          umcRes = PostProcessUserData(display_index);
          //pack_w.m_va->DisplayFrame(frame_buffer.frame_p_c_n[frame_buffer.retrieve    ].va_index,out_data);

          return UMC_OK;
      }//if(pack_w.va_mode != VA_NO)

    if (PictureHeader[task_num].picture_structure != FRAME_PICTURE &&
          frame_buffer.field_buffer_index[task_num] == 0)
    {
        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    if (display_index < 0)
    {
        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    umcRes = PostProcessUserData(display_index);

   return umcRes;
}

void MPEG2VideoDecoderHW::quant_matrix_extension(int task_num)
{
    int32_t i;
    uint32_t code;
    VideoContext* video = Video[task_num][0];
    int32_t load_intra_quantizer_matrix, load_non_intra_quantizer_matrix, load_chroma_intra_quantizer_matrix, load_chroma_non_intra_quantizer_matrix;
    uint8_t q_matrix[4][64] = {};

    GET_TO9BITS(video->bs, 4 ,code)
    GET_1BIT(video->bs,load_intra_quantizer_matrix)
    if(load_intra_quantizer_matrix)
    {
        for(i= 0; i < 64; i++) {
            GET_BITS(video->bs, 8, code);
            q_matrix[0][i] = (uint8_t)code;
        }
    }

    GET_1BIT(video->bs,load_non_intra_quantizer_matrix)
    if(load_non_intra_quantizer_matrix)
    {
        for(i= 0; i < 64; i++) {
            GET_BITS(video->bs, 8, code);
            q_matrix[1][i] = (uint8_t)code;
        }
    }

    GET_1BIT(video->bs,load_chroma_intra_quantizer_matrix);
    if(load_chroma_intra_quantizer_matrix && m_ClipInfo.color_format != YUV420)
    {
        for(i= 0; i < 64; i++) {
            GET_TO9BITS(video->bs, 8, code);
            q_matrix[2][i] = (uint8_t)code;
        }
    }

    GET_1BIT(video->bs,load_chroma_non_intra_quantizer_matrix);
    if(load_chroma_non_intra_quantizer_matrix && m_ClipInfo.color_format != YUV420)
    {
        for(i= 0; i < 64; i++) {
            GET_TO9BITS(video->bs, 8, code);
            q_matrix[2][i] = (uint8_t)code;
        }
    }

    if(pack_l.va_mode == VA_VLD_L) {
      if(load_intra_quantizer_matrix)
      {
          pack_l.QmatrixData.load_intra_quantiser_matrix              = load_intra_quantizer_matrix;
          for(i=0; i<64; i++)
              pack_l.QmatrixData.intra_quantiser_matrix[i] = q_matrix[0][i];
      }
      if(load_non_intra_quantizer_matrix)
      {
          pack_l.QmatrixData.load_non_intra_quantiser_matrix          = load_non_intra_quantizer_matrix;
          for(i=0; i<64; i++)
              pack_l.QmatrixData.non_intra_quantiser_matrix[i] = q_matrix[1][i];
      }
      if(load_chroma_intra_quantizer_matrix)
      {
          pack_l.QmatrixData.load_chroma_intra_quantiser_matrix       = load_chroma_intra_quantizer_matrix;
          for(i=0; i<64; i++)
              pack_l.QmatrixData.chroma_intra_quantiser_matrix[i] = q_matrix[2][i];
      }
      if(load_chroma_non_intra_quantizer_matrix)
      {
          pack_l.QmatrixData.load_chroma_non_intra_quantiser_matrix   = load_chroma_non_intra_quantizer_matrix;
          for(i=0; i<64; i++)
              pack_l.QmatrixData.chroma_non_intra_quantiser_matrix[i] = q_matrix[3][i];
      }
    }//if(va_mode == VA_VLD_L)

} //void quant_matrix_extension()

Status MPEG2VideoDecoderHW::ProcessRestFrame(int task_num)
{
    if (pack_w.m_va && bNeedNewFrame)
    {
        Status umcRes = BeginVAFrame(task_num);
        if (UMC_OK != umcRes)
            return umcRes;
        bNeedNewFrame = false;
    }

    Video[task_num][0]->stream_type = m_ClipInfo.stream_type;
    Video[task_num][0]->color_format = m_ClipInfo.color_format;
    Video[task_num][0]->clip_info_width = m_ClipInfo.clip_info.width;
    Video[task_num][0]->clip_info_height = m_ClipInfo.clip_info.height;

    return MPEG2VideoDecoderBase::ProcessRestFrame(task_num);
}


Status MPEG2VideoDecoderHW::BeginVAFrame(int task_num)
{
    int curr_index;
    PackVA *pack = &pack_w;
    int size_bs = GET_END_PTR(Video[task_num][0]->bs) - GET_START_PTR(Video[task_num][0]->bs)+4;
    int size_sl = m_ClipInfo.clip_info.width*m_ClipInfo.clip_info.height/256;

    if (pack->va_mode == VA_NO)
    {
        return UMC_OK;
    }

    {
        curr_index = frame_buffer.curr_index[task_num];
    }

    pack->m_SliceNum = 0;
    frame_buffer.frame_p_c_n[curr_index].va_index = pack->va_index;

    pack->field_buffer_index = frame_buffer.field_buffer_index[task_num];

    return pack->InitBuffers(size_bs, size_sl); //was "size" only
}

Status MPEG2VideoDecoderHW::UpdateFrameBuffer(int , uint8_t* iqm, uint8_t*niqm)
{

    if(pack_l.va_mode == VA_VLD_L)
    {
      pack_l.QmatrixData.load_intra_quantiser_matrix              = 1;
      pack_l.QmatrixData.load_non_intra_quantiser_matrix          = 1;
      pack_l.QmatrixData.load_chroma_intra_quantiser_matrix       = 1;
      pack_l.QmatrixData.load_chroma_non_intra_quantiser_matrix   = 1;
      if(iqm)
      {
        for(int i=0; i<64; i++)
        {
          pack_l.QmatrixData.intra_quantiser_matrix[i] = iqm[i];
          pack_l.QmatrixData.chroma_intra_quantiser_matrix[i] = iqm[i];
        }
      }
      else
      {
        for(int i=0; i<64; i++)
        {
          pack_l.QmatrixData.intra_quantiser_matrix[i] = default_intra_quantizer_matrix[i];
          pack_l.QmatrixData.chroma_intra_quantiser_matrix[i] = default_intra_quantizer_matrix[i];
        }
      }
      if(niqm)
      {
        for(int i=0; i<64; i++)
        {
            pack_l.QmatrixData.non_intra_quantiser_matrix[i] = niqm[i];
            pack_l.QmatrixData.chroma_non_intra_quantiser_matrix[i] = niqm[i];
        }
      }
      else
      {
        for(int i=0; i<64; i++)
        {
          pack_l.QmatrixData.non_intra_quantiser_matrix[i] = 16;
          pack_l.QmatrixData.chroma_non_intra_quantiser_matrix[i] = 16;
        }
      }
    }

    return UMC_OK;
}


#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
