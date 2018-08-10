// Copyright (c) 2017 Intel Corporation
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

#include "umc_mpeg2_dec_defs.h"

namespace UMC
{

int16_t reset_dc[4] = {128, 256, 512, 1024};

int16_t intra_dc_multi[4] = {3, 2, 1, 0};

int16_t q_scale[2][32] =
{
  {
    0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
   32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62
  },
  {
    0,  1,  2,  3,  4,  5,  6,  7,  8, 10, 12, 14, 16, 18, 20, 22,
   24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96,104,112
  }
};

sVideoFrameBuffer::sVideoFrameBuffer()
    : Y_comp_data (0)
    , U_comp_data (0)
    , V_comp_data (0)
    , user_data   (0)
    , frame_type  (NONE_PICTURE) // 1-I, 2-P, 3-B
    , frame_time  (0) 
    , is_original_frame_time(false)
    , duration    (0)
    , va_index    (0)
    , IsUserDataDecoded(false)
    , us_buf_size (0)
    , us_data_size(0)
    , prev_index  (0)
    , next_index  (0)
    , isCorrupted (false)
{}

sFrameBuffer::sFrameBuffer()
    : Y_comp_height      (0)
    , Y_comp_pitch       (0)
    , U_comp_pitch       (0)
    , V_comp_pitch       (0)
    , pic_size           (0)
    , common_curr_index  (0)
    , latest_prev        (0)
    , latest_next        (0)
    , retrieve           (0)
    , allocated_mb_width (0)
    , allocated_mb_height(0)
    , allocated_cformat  (YV12)
    , ret_array_curr     (0)
    , ret_index          (0)
    , ret_array_free     (0)
    , ret_array_len      (0)
{
    memset(curr_index,         0, sizeof(curr_index));   // 0 initially
    memset(field_buffer_index, 0, sizeof(field_buffer_index));
    memset(ret_array,          0, sizeof(ret_array));
}

}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
