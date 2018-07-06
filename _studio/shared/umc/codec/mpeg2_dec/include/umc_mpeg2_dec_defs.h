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

#pragma once
#include "umc_defs.h"
#if defined (UMC_ENABLE_MPEG2_VIDEO_DECODER)
#include "umc_structures.h"
#include "umc_memory_allocator.h"
#include "umc_mpeg2_dec_bstream.h"

#include <vector>

#define DPB_SIZE 10

namespace UMC
{

//start/end codes
#define PICTURE_START_CODE       0x00000100
#define USER_DATA_START_CODE     0x000001B2
#define SEQUENCE_HEADER_CODE     0x000001B3
#define SEQUENCE_ERROR_CODE      0x000001B4
#define EXTENSION_START_CODE     0x000001B5
#define SEQUENCE_END_CODE        0x000001B7
#define GROUP_START_CODE         0x000001B8

#define SEQUENCE_EXTENSION_ID                  0x00000001
#define SEQUENCE_DISPLAY_EXTENSION_ID          0x00000002
#define QUANT_MATRIX_EXTENSION_ID              0x00000003
#define COPYRIGHT_EXTENSION_ID                 0x00000004
#define SEQUENCE_SCALABLE_EXTENSION_ID         0x00000005
#define PICTURE_DISPLAY_EXTENSION_ID           0x00000007
#define PICTURE_CODING_EXTENSION_ID            0x00000008
#define PICTURE_SPARTIAL_SCALABLE_EXTENSION_ID 0x00000009
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID 0x0000000a

#define DATA_PARTITIONING        0x00000000
#define SPARTIAL_SCALABILITY     0x00000001
#define SNR_SCALABILITY          0x00000002
#define TEMPORAL_SCALABILITY     0x00000003



enum MPEG2FrameType
{
    MPEG2_I_PICTURE               = 1,
    MPEG2_P_PICTURE               = 2,
    MPEG2_B_PICTURE               = 3
};

#define FRAME_PICTURE            3
#define TOP_FIELD                1
#define BOTTOM_FIELD             2
///////////////////////

extern int16_t q_scale[2][32];
extern int16_t reset_dc[4];
extern int16_t intra_dc_multi[4];


struct sVideoFrameBuffer
{
    typedef std::vector< std::pair<uint8_t *,size_t> > UserDataVector;

    sVideoFrameBuffer();

    uint8_t*           Y_comp_data;
    uint8_t*           U_comp_data;
    uint8_t*           V_comp_data;
    uint8_t*           user_data;
    FrameType        frame_type; // 1-I, 2-P, 3-B
    double           frame_time;
    bool             is_original_frame_time;
    double           duration;
    int32_t           va_index;
    bool             IsUserDataDecoded;
    int32_t           us_buf_size;
    int32_t           us_data_size;
    int32_t           prev_index;//index for frame_buffer for prev reference;
    int32_t           next_index;//index for frame_buffer for next reference;
    bool             isCorrupted;
};

struct sGOPTimeCode
{
    uint16_t           gop_seconds;
    uint16_t           gop_minutes;
    uint16_t           gop_hours;
    uint16_t           gop_picture;   // starting picture in gop_second
    uint16_t           gop_drop_frame_flag;
};

struct sSequenceHeader
{
    int32_t           mb_width[2*DPB_SIZE]; //the number of macroblocks in the row of the picture
    int32_t           mb_height[2*DPB_SIZE];//the number of macroblocks in the column of the picture
    int32_t           numMB[2*DPB_SIZE];    //the number of macroblocks in the picture

//sequence extension
    uint32_t           profile;
    uint32_t           level;
    uint32_t           extension_start_code_ID[2*DPB_SIZE];
    uint32_t           scalable_mode[2*DPB_SIZE];
    uint32_t           progressive_sequence;

    int32_t           frame_rate_extension_d;
    int32_t           frame_rate_extension_n;
    uint32_t           frame_rate_code;
    uint32_t           aspect_ratio_code;
    uint16_t           aspect_ratio_w;
    uint16_t           aspect_ratio_h;
    uint32_t           chroma_format;
    uint32_t           width;
    uint32_t           height;
    double           delta_frame_time;
    double           stream_time;
    int32_t           stream_time_temporal_reference; // for current stream_time
    int32_t           first_p_occure;
    int32_t           first_i_occure;
    int32_t           b_curr_number;
    int32_t           is_decoded;

    // GOP info.
    int32_t           closed_gop;    // no ref to previous GOP
    int32_t           broken_link;   // ref to absent prev GOP
    // GOP time code
    sGOPTimeCode     time_code;

    int32_t           frame_count;   // number of decoded pictures from last sequence header
};

struct sSHSavedPars
{
    int32_t mb_width;
    int32_t mb_height;
    int32_t numMB;
    uint32_t extension_start_code_ID;
    uint32_t scalable_mode;
};

// for prediction (forward and backward) and current frame;
struct sFrameBuffer
{
    sFrameBuffer();

    sVideoFrameBuffer     frame_p_c_n[DPB_SIZE*2];    // previous, current and next frames
    uint32_t                Y_comp_height;
    uint32_t                Y_comp_pitch;
    uint32_t                U_comp_pitch;
    uint32_t                V_comp_pitch;
    uint32_t                pic_size;
    int32_t                curr_index[2*DPB_SIZE]; // 0 initially
    int32_t                common_curr_index;// 0 initially
    int32_t                latest_prev; // 0 initially
    int32_t                latest_next; // 0 initially
    int32_t                retrieve;   // index of retrieved frame; -1 initially
    int32_t                field_buffer_index[2*DPB_SIZE];
    int32_t                allocated_mb_width;
    int32_t                allocated_mb_height;
    ColorFormat           allocated_cformat;
    int32_t                ret_array[2*DPB_SIZE];
    int32_t                ret_array_curr;
    int32_t                ret_index;
    int32_t                ret_array_free;
    int32_t                ret_array_len;
};


struct sPictureHeader
{
    MPEG2FrameType   picture_coding_type;
    uint32_t           d_picture;

    int32_t           full_pel_forward_vector;
    int32_t           full_pel_backward_vector;

    //extensions
    int32_t           f_code[4];
    int32_t           r_size[4];
    int32_t           low_in_range[4];
    int32_t           high_in_range[4];
    int32_t           range[4];
    uint32_t           picture_structure;
    uint32_t           intra_dc_precision;
    uint32_t           top_field_first;
    uint32_t           frame_pred_frame_dct;
    uint32_t           concealment_motion_vectors;
    uint32_t           q_scale_type;
    uint32_t           repeat_first_field;
    uint32_t           progressive_frame;
    int32_t           temporal_reference;

    int32_t           curr_reset_dc;
    int32_t           intra_vlc_format;
    int32_t           alternate_scan;
    int32_t           curr_intra_dc_multi;
    int32_t           max_slice_vert_pos;

    sGOPTimeCode     time_code;
    bool             first_in_sequence;
};

struct  VideoContext
{
//Slice
    int32_t       slice_vertical_position;
    uint32_t       m_bNewSlice;//l
    int32_t       cur_q_scale;

    int32_t       mb_row;
    int32_t       mb_col;

//Macroblock
    uint32_t       macroblock_motion_forward;
    uint32_t       macroblock_motion_backward;
    int32_t       prediction_type;

DECLALIGN(16) int16_t PMV[8];
DECLALIGN(16) int16_t vector[8];

    int16_t       dct_dc_past[3]; // y,u,v

    int32_t       mb_address_increment;//l
    int32_t       row_l, col_l, row_c, col_c;
    int32_t       offset_l, offset_c;

    uint8_t        *blkCurrYUV[3];

//Block
DECLALIGN(16)
    DecodeIntraSpec_MPEG2 decodeIntraSpec;
DECLALIGN(8)
    DecodeInterSpec_MPEG2 decodeInterSpec;
DECLALIGN(8)
    DecodeIntraSpec_MPEG2 decodeIntraSpecChroma;
DECLALIGN(8)
    DecodeInterSpec_MPEG2 decodeInterSpecChroma;

//Bitstream
    uint8_t*       bs_curr_ptr;
    int32_t       bs_bit_offset;
    uint8_t*       bs_start_ptr;
    uint8_t*       bs_end_ptr;
    uint8_t*       bs_sequence_header_start;

    int32_t Y_comp_pitch;
    int32_t U_comp_pitch;
    int32_t V_comp_pitch;
    int32_t Y_comp_height;
    uint32_t pic_size;

    int32_t blkOffsets[3][8];
    int32_t blkPitches[3][2];

    int32_t clip_info_width;
    int32_t clip_info_height;

    VideoStreamType stream_type;
    ColorFormat color_format;

};

struct Mpeg2Bitstream
{
    uint8_t* bs_curr_ptr;
    int32_t bs_bit_offset;
    uint8_t* bs_start_ptr;
    uint8_t* bs_end_ptr;
};

enum { FCodeFwdX = 0, FCodeFwdY = 1, FCodeBwdX = 2, FCodeBwdY = 3 };

} // namespace UMC

#ifdef _MSVC_LANG
#pragma warning(default: 4324)
#endif

extern int16_t zero_memory[64*8];

/*******************************************************/

#define RESET_PMV(array) {                                      \
  for(unsigned int nn=0; nn<sizeof(array)/sizeof(int32_t); nn++) \
    ((int32_t*)array)[nn] = 0;                                   \
}

#endif // UMC_ENABLE_MPEG2_VIDEO_DECODER
