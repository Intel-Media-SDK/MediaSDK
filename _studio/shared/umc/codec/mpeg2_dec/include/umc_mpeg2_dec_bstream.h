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

#pragma once

#include "umc_defs.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_defs.h"

//#ifdef __ICL
#define DECLALIGN(x)


#define GET_BYTE_PTR(video) \
  video##_curr_ptr + ((video##_bit_offset + 7) >> 3)

#define GET_END_PTR(video) \
  video##_end_ptr

#define GET_START_PTR(video) \
  video##_start_ptr

#define GET_OFFSET(video) \
  (GET_BYTE_PTR(video) - video##_start_ptr)

#define GET_BIT_OFFSET(video) \
  ((video##_curr_ptr - video##_start_ptr)*8 + video##_bit_offset)

#define SET_PTR(video, PTR) {           \
  video##_curr_ptr = (uint8_t*)(PTR);      \
  video##_bit_offset = 0;                \
}

#define GET_REMAINED_BYTES(video) \
  (GET_END_PTR(video) - GET_BYTE_PTR(video))

#define INIT_BITSTREAM(video, start_ptr, end_ptr) { \
  video##_start_ptr = (uint8_t*)(start_ptr); \
  video##_end_ptr = (uint8_t*)(end_ptr); \
  SET_PTR(video, video##_start_ptr); \
}

#define SHOW_BITS_32(video, CODE) { \
  uint32_t _code = \
  (video##_curr_ptr[0] << 24) | \
  (video##_curr_ptr[1] << 16) | \
  (video##_curr_ptr[2] << 8) | \
  (video##_curr_ptr[3]); \
  CODE = (_code << video##_bit_offset) | (video##_curr_ptr[4] >> (8 - video##_bit_offset)); \
}

#define SHOW_BITS_LONG(video, NUM_BITS, CODE) { \
  uint32_t _code2; \
  SHOW_BITS_32(video, _code2); \
  CODE = _code2 >> (32 - NUM_BITS); \
}

#define SHOW_HI9BITS(video, CODE) \
  CODE = (((video##_curr_ptr[0] << 24) | (video##_curr_ptr[1] << 16)) << video##_bit_offset);

#define EXPAND_17BITS(video, CODE) \
  CODE |= video##_curr_ptr[2] << (8 + video##_bit_offset);

#define SHOW_HI17BITS(video, CODE) \
  CODE = (((video##_curr_ptr[0] << 24) | (video##_curr_ptr[1] << 16) | (video##_curr_ptr[3]<<8)) << video##_bit_offset);

#define EXPAND_25BITS(video, CODE) \
  CODE |= video##_curr_ptr[3] << video##_bit_offset;

#define SHOW_BITS(video, NUM_BITS, CODE) { \
  uint32_t _code = \
  (video##_curr_ptr[0] << 24) | \
  (video##_curr_ptr[1] << 16) | \
  (video##_curr_ptr[2] << 8); \
  CODE = (_code << video##_bit_offset) >> (32 - NUM_BITS); \
}

#define SHOW_1BIT(video, CODE) { \
  CODE = (video##_curr_ptr[0] >> (7-video##_bit_offset)) & 1; \
}

#define IS_NEXTBIT1(video) \
  (video##_curr_ptr[0] & (0x80 >> video##_bit_offset))

#define SHOW_TO9BITS(video, NUM_BITS, CODE) { \
  uint32_t _code = \
  (video##_curr_ptr[0] << 8) | (video##_curr_ptr[1] ); \
  CODE = (_code >> (16 - NUM_BITS - video##_bit_offset)) & ((1<<NUM_BITS)-1); \
}

#define SKIP_TO_END(video) \
  video##_curr_ptr = video##_end_ptr; \
  video##_bit_offset = 0;


#define SKIP_BITS(video, NUM_BITS) \
  video##_bit_offset += NUM_BITS; \
  video##_curr_ptr += (video##_bit_offset >> 3); \
  video##_bit_offset &= 7;

#define GET_BITS(video, NUM_BITS, CODE) { \
  SHOW_BITS(video, NUM_BITS, CODE); \
  SKIP_BITS(video, NUM_BITS); \
}

#define GET_TO9BITS(video, NUM_BITS, CODE) { \
  SHOW_TO9BITS(video, NUM_BITS, CODE); \
  SKIP_BITS(video, NUM_BITS); \
}

#define GET_BITS_LONG(video, NUM_BITS, CODE) { \
  SHOW_BITS_LONG(video, NUM_BITS, CODE); \
  SKIP_BITS(video, NUM_BITS); \
}

#define SKIP_BITS_LONG(video, NUM_BITS) \
  SKIP_BITS(video, NUM_BITS)

#define SKIP_BITS_32(video) \
  SKIP_BITS(video, 32);

#define GET_BITS32(video, CODE) { \
  SHOW_BITS_32(video, CODE) \
  SKIP_BITS_32(video); \
}

#define GET_1BIT(video, CODE) {\
  SHOW_1BIT(video, CODE) \
  SKIP_BITS(video, 1) \
}

#define UNGET_BITS_32(video) \
  video##_curr_ptr -= 4;

/***************************************************************/

#define FIND_START_CODE(video, code) {                          \
  uint8_t *ptr = GET_BYTE_PTR(video);                             \
  uint8_t *end_ptr = GET_END_PTR(video) - 3;                      \
  while (ptr < end_ptr && (ptr[0] || ptr[1] || (1 != ptr[2]))) {\
    ptr++;                                                      \
  }                                                             \
  if (ptr < end_ptr) {                                          \
    code = 256 + ptr[3];                                        \
    SET_PTR(video, ptr);                                        \
  } else {                                                      \
    code = (uint32_t)UMC::UMC_ERR_NOT_ENOUGH_DATA;                \
  }                                                             \
  /*printf("code %x at %5d (%5d) %p\n",code,idx,end,video##_bitstream_current_data);*/ \
}

#define GET_START_CODE(video, code) { \
  FIND_START_CODE(video, code)        \
  if(code != (uint32_t)UMC_ERR_NOT_ENOUGH_DATA) { \
    SKIP_BITS_32(video);                    \
  }                                         \
}

/***************************************************************/

#define VLC_BAD        0x80
#define VLC_NEXTTABLE  0x40

#define DECODE_VLC(value, bitstream, pVLC) \
{ \
  uint32_t __code; \
  int32_t tbl_value; \
  int32_t bits_table0 = pVLC.bits_table0; \
  \
  SHOW_HI9BITS(bitstream, __code); \
  \
  tbl_value = pVLC.table0[__code >> (32 - bits_table0)]; \
  \
  if (tbl_value & (VLC_BAD | VLC_NEXTTABLE)) { \
    int32_t max_bits = pVLC.max_bits; \
    int32_t bits_table1 = pVLC.bits_table1; \
  \
    if (tbl_value & VLC_BAD) return UMC_ERR_INVALID_STREAM; \
    EXPAND_17BITS(bitstream, __code); \
    tbl_value = pVLC.table1[(__code >> (32 - max_bits)) & ((1 << bits_table1) - 1)]; \
  } \
  \
  value = tbl_value >> 8; \
  \
  tbl_value &= 0x3F; \
  SKIP_BITS(bitstream, tbl_value); \
}

#endif /* __UMC_MPEG2_DEC_BSTREAM_H */

/***************************************************************/


#define SCAN_MATRIX_TYPE   const uint8_t
#define QUANT_MATRIX_TYPE  uint8_t

struct DecodeIntraSpec_MPEG2 {
  QUANT_MATRIX_TYPE _quantMatrix[64];
  int16_t pDstBlock[64];
  int16_t idct[64];
  QUANT_MATRIX_TYPE *quantMatrix;
  SCAN_MATRIX_TYPE  *scanMatrix;
  int32_t intraVLCFormat;
  int32_t intraShiftDC;
};

struct DecodeInterSpec_MPEG2 {
  QUANT_MATRIX_TYPE _quantMatrix[64];
  int16_t pDstBlock[64];
  int16_t idct[64];
  QUANT_MATRIX_TYPE *quantMatrix;
  SCAN_MATRIX_TYPE  *scanMatrix;
  int32_t idxLastNonZero;
  int32_t align;
};
