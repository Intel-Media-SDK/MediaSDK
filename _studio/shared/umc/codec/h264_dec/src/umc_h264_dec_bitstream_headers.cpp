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

#include "umc_h264_dec.h"
#include "umc_h264_bitstream_headers.h"
#include "umc_h264_headers.h"
#include <limits.h>

#define SCLFLAT16     0
#define SCLDEFAULT    1
#define SCLREDEFINED  2

#define PeakNextBits(current_data, bp, nbits, data) \
{ \
    uint32_t x; \
 \
    VM_ASSERT((nbits) > 0 && (nbits) <= 32); \
    VM_ASSERT(nbits >= 0 && nbits <= 31); \
 \
    int32_t offset = bp - (nbits); \
 \
    if (offset >= 0) \
    { \
        x = current_data[0] >> (offset + 1); \
    } \
    else \
    { \
        offset += 32; \
 \
        x = current_data[1] >> (offset); \
        x >>= 1; \
        x += current_data[0] << (31 - offset); \
    } \
 \
    VM_ASSERT(offset >= 0 && offset <= 31); \
 \
    (data) = x & bits_data[nbits]; \
}

using namespace UMC_H264_DECODER;

namespace UMC
{
static const int32_t pre_norm_adjust_index4x4[16] =
{// 0 1 2 3
    0,2,0,2,//0
    2,1,2,1,//1
    0,2,0,2,//2
    2,1,2,1 //3
};

static const int32_t pre_norm_adjust4x4[6][3] =
{
    {10,16,13},
    {11,18,14},
    {13,20,16},
    {14,23,18},
    {16,25,20},
    {18,29,23}
};

static const int32_t pre_norm_adjust8x8[6][6] =
{
    {20, 18, 32, 19, 25, 24},
    {22, 19, 35, 21, 28, 26},
    {26, 23, 42, 24, 33, 31},
    {28, 25, 45, 26, 35, 33},
    {32, 28, 51, 30, 40, 38},
    {36, 32, 58, 34, 46, 43}
};

static const int32_t pre_norm_adjust_index8x8[64] =
{// 0 1 2 3 4 5 6 7
    0,3,4,3,0,3,4,3,//0
    3,1,5,1,3,1,5,1,//1
    4,5,2,5,4,5,2,5,//2
    3,1,5,1,3,1,5,1,//3
    0,3,4,3,0,3,4,3,//4
    3,1,5,1,3,1,5,1,//5
    4,5,2,5,4,5,2,5,//6
    3,1,5,1,3,1,5,1 //7
};

const
int32_t mp_scan4x4[2][16] =
{
    {
        0,  1,  4,  8,
        5,  2,  3,  6,
        9,  12, 13, 10,
        7,  11, 14, 15
    },
    {
        0,  4,  1,  8,
        12, 5,  9,  13,
        2,  6,  10, 14,
        3,  7,  11, 15
    }
};

const int32_t hp_scan8x8[2][64] =
{
    //8x8 zigzag scan
    {
        0, 1, 8,16, 9, 2, 3,10,
        17,24,32,25,18,11, 4, 5,
        12,19,26,33,40,48,41,34,
        27,20,13, 6, 7,14,21,28,
        35,42,49,56,57,50,43,36,
        29,22,15,23,30,37,44,51,
        58,59,52,45,38,31,39,46,
        53,60,61,54,47,55,62,63
    },
//8x8 field scan
    {
        0, 8,16, 1, 9,24,32,17,
        2,25,40,48,56,33,10, 3,
        18,41,49,57,26,11, 4,19,
        34,42,50,58,27,12, 5,20,
        35,43,51,59,28,13, 6,21,
        36,44,52,60,29,14,22,37,
        45,53,61,30, 7,15,38,46,
        54,62,23,31,39,47,55,63
    }
};

const uint32_t bits_data[] =
{
    (((uint32_t)0x01 << (0)) - 1),
    (((uint32_t)0x01 << (1)) - 1),
    (((uint32_t)0x01 << (2)) - 1),
    (((uint32_t)0x01 << (3)) - 1),
    (((uint32_t)0x01 << (4)) - 1),
    (((uint32_t)0x01 << (5)) - 1),
    (((uint32_t)0x01 << (6)) - 1),
    (((uint32_t)0x01 << (7)) - 1),
    (((uint32_t)0x01 << (8)) - 1),
    (((uint32_t)0x01 << (9)) - 1),
    (((uint32_t)0x01 << (10)) - 1),
    (((uint32_t)0x01 << (11)) - 1),
    (((uint32_t)0x01 << (12)) - 1),
    (((uint32_t)0x01 << (13)) - 1),
    (((uint32_t)0x01 << (14)) - 1),
    (((uint32_t)0x01 << (15)) - 1),
    (((uint32_t)0x01 << (16)) - 1),
    (((uint32_t)0x01 << (17)) - 1),
    (((uint32_t)0x01 << (18)) - 1),
    (((uint32_t)0x01 << (19)) - 1),
    (((uint32_t)0x01 << (20)) - 1),
    (((uint32_t)0x01 << (21)) - 1),
    (((uint32_t)0x01 << (22)) - 1),
    (((uint32_t)0x01 << (23)) - 1),
    (((uint32_t)0x01 << (24)) - 1),
    (((uint32_t)0x01 << (25)) - 1),
    (((uint32_t)0x01 << (26)) - 1),
    (((uint32_t)0x01 << (27)) - 1),
    (((uint32_t)0x01 << (28)) - 1),
    (((uint32_t)0x01 << (29)) - 1),
    (((uint32_t)0x01 << (30)) - 1),
    (((uint32_t)0x01 << (31)) - 1),
    ((uint32_t)0xFFFFFFFF),
};

const uint16_t SAspectRatio[17][2] =
{
    { 0,  0}, { 1,  1}, {12, 11}, {10, 11}, {16, 11}, {40, 33}, { 24, 11},
    {20, 11}, {32, 11}, {80, 33}, {18, 11}, {15, 11}, {64, 33}, {160, 99},
    {4,   3}, {3,   2}, {2,   1}
};

const uint32_t SubWidthC[4]  = { 1, 2, 2, 1 };
const uint32_t SubHeightC[4] = { 1, 2, 1, 1 };

const uint8_t default_intra_scaling_list4x4[16]=
{
     6, 13, 20, 28, 13, 20, 28, 32, 20, 28, 32, 37, 28, 32, 37, 42
};
const uint8_t default_inter_scaling_list4x4[16]=
{
    10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34
};

const uint8_t default_intra_scaling_list8x8[64]=
{
     6, 10, 13, 16, 18, 23, 25, 27, 10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31, 16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36, 23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40, 27, 29, 31, 33, 36, 38, 40, 42
};
const uint8_t default_inter_scaling_list8x8[64]=
{
     9, 13, 15, 17, 19, 21, 22, 24, 13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27, 17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30, 21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33, 24, 25, 27, 28, 30, 32, 33, 35
};

H264BaseBitstream::H264BaseBitstream()
{
    Reset(0, 0);
}

H264BaseBitstream::H264BaseBitstream(uint8_t * const pb, const uint32_t maxsize)
{
    Reset(pb, maxsize);
}

H264BaseBitstream::~H264BaseBitstream()
{
}

void H264BaseBitstream::Reset(uint8_t * const pb, const uint32_t maxsize)
{
    m_pbs       = (uint32_t*)pb;
    m_pbsBase   = (uint32_t*)pb;
    m_bitOffset = 31;
    m_maxBsSize    = maxsize;
    m_tailBsSize = 0;

} // void Reset(uint8_t * const pb, const uint32_t maxsize)

void H264BaseBitstream::Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize)
{
    m_pbs       = (uint32_t*)pb;
    m_pbsBase   = (uint32_t*)pb;
    m_bitOffset = offset;
    m_maxBsSize = maxsize;
    m_tailBsSize = 0;

} // void Reset(uint8_t * const pb, int32_t offset, const uint32_t maxsize)


bool H264BaseBitstream::More_RBSP_Data()
{
    int32_t code, tmp;
    uint32_t* ptr_state = m_pbs;
    int32_t  bit_state = m_bitOffset;

    VM_ASSERT(m_bitOffset >= 0 && m_bitOffset <= 31);

    int32_t remaining_bytes = (int32_t)BytesLeft();

    if (remaining_bytes <= 0)
        return false;

    // get top bit, it can be "rbsp stop" bit
    CheckBitsLeft(1);
    h264GetBits(m_pbs, m_bitOffset, 1, code);

    // get remain bits, which is less then byte
    tmp = (m_bitOffset + 1) % 8;

    if(tmp)
    {
        CheckBitsLeft(tmp);
        h264GetBits(m_pbs, m_bitOffset, tmp, code);
        if ((code << (8 - tmp)) & 0x7f)    // most sig bit could be rbsp stop bit
        {
            m_pbs = ptr_state;
            m_bitOffset = bit_state;
            // there are more data
            return true;
        }
    }

    remaining_bytes = (int32_t)BytesLeft();

    // run through remain bytes
    while (0 < remaining_bytes)
    {
        h264GetBits(m_pbs, m_bitOffset, 8, code);

        if (code)
        {
            m_pbs = ptr_state;
            m_bitOffset = bit_state;
            // there are more data
            return true;
        }

        remaining_bytes -= 1;
    }

    return false;
}

void H264BaseBitstream::GetOrg(uint32_t **pbs, uint32_t *size)
{
    *pbs  = m_pbsBase;
    *size = m_maxBsSize;
}

void H264BaseBitstream::GetState(uint32_t** pbs,uint32_t* bitOffset)
{
    *pbs       = m_pbs;
    *bitOffset = m_bitOffset;
}

void H264BaseBitstream::SetState(uint32_t* pbs,uint32_t bitOffset)
{
    m_pbs = pbs;
    m_bitOffset = bitOffset;
}

void H264BaseBitstream::SetDecodedBytes(size_t nBytes)
{
    m_pbs = m_pbsBase + (nBytes / 4);
    m_bitOffset = 31 - ((int32_t) ((nBytes % sizeof(uint32_t)) * 8));
}

void H264BaseBitstream::SetTailBsSize(const uint32_t nBytes)
{
    m_tailBsSize = nBytes;
}

H264HeadersBitstream::H264HeadersBitstream()
    : H264BaseBitstream()
{
}

H264HeadersBitstream::H264HeadersBitstream(uint8_t * const pb, const uint32_t maxsize)
    : H264BaseBitstream(pb, maxsize)
{
}

inline bool CheckLevel(uint8_t level_idc, bool ignore_level_constrain = false)
{
#if (MFX_VERSION < 1035)
    std::ignore = ignore_level_constrain;
#endif

    switch(level_idc)
    {
    case H264VideoDecoderParams::H264_LEVEL_1:
    case H264VideoDecoderParams::H264_LEVEL_11:
    case H264VideoDecoderParams::H264_LEVEL_12:
    case H264VideoDecoderParams::H264_LEVEL_13:

    case H264VideoDecoderParams::H264_LEVEL_2:
    case H264VideoDecoderParams::H264_LEVEL_21:
    case H264VideoDecoderParams::H264_LEVEL_22:

    case H264VideoDecoderParams::H264_LEVEL_3:
    case H264VideoDecoderParams::H264_LEVEL_31:
    case H264VideoDecoderParams::H264_LEVEL_32:

    case H264VideoDecoderParams::H264_LEVEL_4:
    case H264VideoDecoderParams::H264_LEVEL_41:
    case H264VideoDecoderParams::H264_LEVEL_42:

    case H264VideoDecoderParams::H264_LEVEL_5:
    case H264VideoDecoderParams::H264_LEVEL_51:
    case H264VideoDecoderParams::H264_LEVEL_52:

    case H264VideoDecoderParams::H264_LEVEL_9:
        return true;

#if (MFX_VERSION >= 1035)
    case H264VideoDecoderParams::H264_LEVEL_6:
    case H264VideoDecoderParams::H264_LEVEL_61:
    case H264VideoDecoderParams::H264_LEVEL_62:
        return ignore_level_constrain;
#endif

    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
//  H264Bitstream::GetSequenceParamSet()
//    Read sequence parameter set data from bitstream.
// ---------------------------------------------------------------------------
Status H264HeadersBitstream::GetSequenceParamSet(H264SeqParamSet *sps, bool ignore_level_constrain)
{
    // Not all members of the seq param set structure are contained in all
    // seq param sets. So start by init all to zero.
    Status ps = UMC_OK;
    sps->Reset();

    // profile
    // TBD: add rejection of unsupported profile
    sps->profile_idc = (uint8_t)GetBits(8);

    switch (sps->profile_idc)
    {
    case H264VideoDecoderParams::H264_PROFILE_BASELINE:
    case H264VideoDecoderParams::H264_PROFILE_MAIN:
    case H264VideoDecoderParams::H264_PROFILE_SCALABLE_BASELINE:
    case H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_EXTENDED:
    case H264VideoDecoderParams::H264_PROFILE_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_HIGH10:
    case H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_HIGH422:
    case H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH:
    case H264VideoDecoderParams::H264_PROFILE_HIGH444:
    case H264VideoDecoderParams::H264_PROFILE_ADVANCED444_INTRA:
    case H264VideoDecoderParams::H264_PROFILE_ADVANCED444:
    case H264VideoDecoderParams::H264_PROFILE_HIGH444_PRED:
    case H264VideoDecoderParams::H264_PROFILE_CAVLC444_INTRA:
        break;
    default:
        return UMC_ERR_INVALID_STREAM;
    }

    sps->constraint_set0_flag = Get1Bit();
    sps->constraint_set1_flag = Get1Bit();
    sps->constraint_set2_flag = Get1Bit();
    sps->constraint_set3_flag = Get1Bit();
    sps->constraint_set4_flag = Get1Bit();
    sps->constraint_set5_flag = Get1Bit();

    // skip 2 zero bits
    GetBits(2);

    sps->level_idc = (uint8_t)GetBits(8);

    if (sps->level_idc == H264VideoDecoderParams::H264_LEVEL_UNKNOWN)
        sps->level_idc = H264VideoDecoderParams::H264_LEVEL_52;

    MFX_CHECK(CheckLevel(sps->level_idc, ignore_level_constrain), UMC_ERR_INVALID_STREAM);

    if (sps->level_idc == H264VideoDecoderParams::H264_LEVEL_9 &&
        sps->profile_idc != H264VideoDecoderParams::H264_PROFILE_BASELINE &&
        sps->profile_idc != H264VideoDecoderParams::H264_PROFILE_MAIN &&
        sps->profile_idc != H264VideoDecoderParams::H264_PROFILE_EXTENDED)
    {
        sps->level_idc = H264VideoDecoderParams::H264_LEVEL_1b;
    }

    // id
    int32_t sps_id = GetVLCElement(false);
    if (sps_id > (int32_t)(MAX_NUM_SEQ_PARAM_SETS - 1))
    {
        return UMC_ERR_INVALID_STREAM;
    }

    sps->seq_parameter_set_id = (uint8_t)sps_id;

    // see 7.3.2.1.1 "Sequence parameter set data syntax"
    // chapter of H264 standard for full list of profiles with chrominance
    if ((H264VideoDecoderParams::H264_PROFILE_SCALABLE_BASELINE == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_HIGH == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_HIGH10 == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_HIGH422 == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_MULTIVIEW_HIGH == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_STEREO_HIGH == sps->profile_idc) ||
        /* what're these profiles ??? */
        (H264VideoDecoderParams::H264_PROFILE_HIGH444 == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_HIGH444_PRED == sps->profile_idc) ||
        (H264VideoDecoderParams::H264_PROFILE_CAVLC444_INTRA == sps->profile_idc))
    {
        uint32_t chroma_format_idc = GetVLCElement(false);

        if (chroma_format_idc > 3)
            return UMC_ERR_INVALID_STREAM;

        sps->chroma_format_idc = (uint8_t)chroma_format_idc;
        if (sps->chroma_format_idc==3)
        {
            sps->residual_colour_transform_flag = Get1Bit();
        }

        uint32_t bit_depth_luma = GetVLCElement(false) + 8;
        uint32_t bit_depth_chroma = GetVLCElement(false) + 8;

        if (bit_depth_luma > 16 || bit_depth_chroma > 16)
            return UMC_ERR_INVALID_STREAM;

        sps->bit_depth_luma = (uint8_t)bit_depth_luma;
        sps->bit_depth_chroma = (uint8_t)bit_depth_chroma;

        if (!chroma_format_idc)
            sps->bit_depth_chroma = sps->bit_depth_luma;

        VM_ASSERT(!sps->residual_colour_transform_flag);
        if (sps->residual_colour_transform_flag == 1)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        sps->qpprime_y_zero_transform_bypass_flag = Get1Bit();
        sps->seq_scaling_matrix_present_flag = Get1Bit();
        if(sps->seq_scaling_matrix_present_flag)
        {
            // 0
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[0],(uint8_t*)default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[0]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[0],(uint8_t*) default_intra_scaling_list4x4);
                sps->type_of_scaling_list_used[0] = SCLDEFAULT;
            }
            // 1
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[1],(uint8_t*) default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[1]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[1],(uint8_t*) sps->ScalingLists4x4[0].ScalingListCoeffs);
                sps->type_of_scaling_list_used[1] = SCLDEFAULT;
            }
            // 2
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[2],(uint8_t*) default_intra_scaling_list4x4,&sps->type_of_scaling_list_used[2]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[2],(uint8_t*) sps->ScalingLists4x4[1].ScalingListCoeffs);
                sps->type_of_scaling_list_used[2] = SCLDEFAULT;
            }
            // 3
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[3],(uint8_t*)default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[3]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[3],(uint8_t*) default_inter_scaling_list4x4);
                sps->type_of_scaling_list_used[3] = SCLDEFAULT;
            }
            // 4
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[4],(uint8_t*) default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[4]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[4],(uint8_t*) sps->ScalingLists4x4[3].ScalingListCoeffs);
                sps->type_of_scaling_list_used[4] = SCLDEFAULT;
            }
            // 5
            if(Get1Bit())
            {
                GetScalingList4x4(&sps->ScalingLists4x4[5],(uint8_t*) default_inter_scaling_list4x4,&sps->type_of_scaling_list_used[5]);
            }
            else
            {
                FillScalingList4x4(&sps->ScalingLists4x4[5],(uint8_t*) sps->ScalingLists4x4[4].ScalingListCoeffs);
                sps->type_of_scaling_list_used[5] = SCLDEFAULT;
            }

            // 0
            if(Get1Bit())
            {
                GetScalingList8x8(&sps->ScalingLists8x8[0],(uint8_t*)default_intra_scaling_list8x8,&sps->type_of_scaling_list_used[6]);
            }
            else
            {
                FillScalingList8x8(&sps->ScalingLists8x8[0],(uint8_t*) default_intra_scaling_list8x8);
                sps->type_of_scaling_list_used[6] = SCLDEFAULT;
            }
            // 1
            if(Get1Bit())
            {
                GetScalingList8x8(&sps->ScalingLists8x8[1],(uint8_t*) default_inter_scaling_list8x8,&sps->type_of_scaling_list_used[7]);
            }
            else
            {
                FillScalingList8x8(&sps->ScalingLists8x8[1],(uint8_t*) default_inter_scaling_list8x8);
                sps->type_of_scaling_list_used[7] = SCLDEFAULT;
            }

        }
        else
        {
            int32_t i;

            for (i = 0; i < 6; i += 1)
            {
                FillFlatScalingList4x4(&sps->ScalingLists4x4[i]);
            }
            for (i = 0; i < 2; i += 1)
            {
                FillFlatScalingList8x8(&sps->ScalingLists8x8[i]);
            }
        }
    }
    else
    {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma = 8;
        sps->bit_depth_chroma = 8;

        SetDefaultScalingLists(sps);
    }

    // log2 max frame num (bitstream contains value - 4)
    uint32_t log2_max_frame_num = GetVLCElement(false) + 4;
    sps->log2_max_frame_num = (uint8_t)log2_max_frame_num;

    if (log2_max_frame_num > 16 || log2_max_frame_num < 4)
        return UMC_ERR_INVALID_STREAM;

    // pic order cnt type (0..2)
    uint32_t pic_order_cnt_type = GetVLCElement(false);
    sps->pic_order_cnt_type = (uint8_t)pic_order_cnt_type;
    if (pic_order_cnt_type > 2)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    if (sps->pic_order_cnt_type == 0)
    {
        // log2 max pic order count lsb (bitstream contains value - 4)
        uint32_t log2_max_pic_order_cnt_lsb = GetVLCElement(false) + 4;
        sps->log2_max_pic_order_cnt_lsb = (uint8_t)log2_max_pic_order_cnt_lsb;

        if (log2_max_pic_order_cnt_lsb > 16 || log2_max_pic_order_cnt_lsb < 4)
            return UMC_ERR_INVALID_STREAM;

        sps->MaxPicOrderCntLsb = (1 << sps->log2_max_pic_order_cnt_lsb);
    }
    else if (sps->pic_order_cnt_type == 1)
    {
        sps->delta_pic_order_always_zero_flag = Get1Bit();
        sps->offset_for_non_ref_pic = GetVLCElement(true);
        sps->offset_for_top_to_bottom_field = GetVLCElement(true);
        sps->num_ref_frames_in_pic_order_cnt_cycle = GetVLCElement(false);

        if (sps->num_ref_frames_in_pic_order_cnt_cycle > 255)
            return UMC_ERR_INVALID_STREAM;

        // get offsets
        for (uint32_t i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            sps->poffset_for_ref_frame[i] = GetVLCElement(true);
        }
    }    // pic order count type 1

    // num ref frames
    sps->num_ref_frames = GetVLCElement(false);
    if (sps->num_ref_frames > 16)
        return UMC_ERR_INVALID_STREAM;

    sps->gaps_in_frame_num_value_allowed_flag = Get1Bit();

    // picture width in MBs (bitstream contains value - 1)
    sps->frame_width_in_mbs = GetVLCElement(false) + 1;

    // picture height in MBs (bitstream contains value - 1)
    sps->frame_height_in_mbs = GetVLCElement(false) + 1;

    if (!(sps->frame_width_in_mbs * 16 < USHRT_MAX) ||
        !(sps->frame_height_in_mbs * 16 < USHRT_MAX))
        return UMC_ERR_INVALID_STREAM;

    sps->frame_mbs_only_flag = Get1Bit();
    sps->frame_height_in_mbs = (2-sps->frame_mbs_only_flag)*sps->frame_height_in_mbs;
    if (sps->frame_mbs_only_flag == 0)
    {
        sps->mb_adaptive_frame_field_flag = Get1Bit();
    }
    sps->direct_8x8_inference_flag = Get1Bit();
    if (sps->frame_mbs_only_flag==0)
    {
        sps->direct_8x8_inference_flag = 1;
    }
    sps->frame_cropping_flag = Get1Bit();

    if (sps->frame_cropping_flag)
    {
        sps->frame_cropping_rect_left_offset      = GetVLCElement(false);
        sps->frame_cropping_rect_right_offset     = GetVLCElement(false);
        sps->frame_cropping_rect_top_offset       = GetVLCElement(false);
        sps->frame_cropping_rect_bottom_offset    = GetVLCElement(false);

        // check cropping parameters
        int32_t cropX = SubWidthC[sps->chroma_format_idc] * sps->frame_cropping_rect_left_offset;
        int32_t cropY = SubHeightC[sps->chroma_format_idc] * sps->frame_cropping_rect_top_offset * (2 - sps->frame_mbs_only_flag);
        int32_t cropH = sps->frame_height_in_mbs * 16 -
            SubHeightC[sps->chroma_format_idc]*(2 - sps->frame_mbs_only_flag) *
            (sps->frame_cropping_rect_top_offset + sps->frame_cropping_rect_bottom_offset);

        int32_t cropW = sps->frame_width_in_mbs * 16 - SubWidthC[sps->chroma_format_idc] *
            (sps->frame_cropping_rect_left_offset + sps->frame_cropping_rect_right_offset);

        if (cropX < 0 || cropY < 0 || cropW < 0 || cropH < 0)
            return UMC_ERR_INVALID_STREAM;

        if (cropX > (int32_t)sps->frame_width_in_mbs * 16)
            return UMC_ERR_INVALID_STREAM;

        if (cropY > (int32_t)sps->frame_height_in_mbs * 16)
            return UMC_ERR_INVALID_STREAM;

        if (cropX + cropW > (int32_t)sps->frame_width_in_mbs * 16)
            return UMC_ERR_INVALID_STREAM;

        if (cropY + cropH > (int32_t)sps->frame_height_in_mbs * 16)
            return UMC_ERR_INVALID_STREAM;

    } // don't need else because we zeroid structure

    CheckBSLeft();

    sps->vui_parameters_present_flag = Get1Bit();
    if (sps->vui_parameters_present_flag)
    {
        H264VUI vui;
        Status vui_ps = GetVUIParam(sps, &vui);

        if (vui_ps == UMC_OK && IsBSLeft())
        {
            sps->vui = vui;
        }
    }

    return ps;
}    // GetSequenceParamSet

Status H264HeadersBitstream::GetVUIParam(H264SeqParamSet *sps, H264VUI *vui)
{
    Status ps = UMC_OK;

    vui->Reset();

    vui->aspect_ratio_info_present_flag = Get1Bit();

    vui->sar_width = 1; // default values
    vui->sar_height = 1;

    if (vui->aspect_ratio_info_present_flag)
    {
        vui->aspect_ratio_idc = (uint8_t) GetBits(8);
        if (vui->aspect_ratio_idc  ==  255) {
            vui->sar_width = (uint16_t) GetBits(16);
            vui->sar_height = (uint16_t) GetBits(16);
        }
        else
        {
            if (!vui->aspect_ratio_idc || vui->aspect_ratio_idc >= sizeof(SAspectRatio)/sizeof(SAspectRatio[0]))
            {
                vui->aspect_ratio_info_present_flag = 0;
            }
            else
            {
                vui->sar_width = SAspectRatio[vui->aspect_ratio_idc][0];
                vui->sar_height = SAspectRatio[vui->aspect_ratio_idc][1];
            }
        }
    }

    vui->overscan_info_present_flag = Get1Bit();
    if (vui->overscan_info_present_flag)
        vui->overscan_appropriate_flag = Get1Bit();

    vui->video_signal_type_present_flag = Get1Bit();
    if( vui->video_signal_type_present_flag ) {
        vui->video_format = (uint8_t) GetBits(3);
        vui->video_full_range_flag = Get1Bit();
        vui->colour_description_present_flag = Get1Bit();
        if( vui->colour_description_present_flag ) {
            vui->colour_primaries = (uint8_t) GetBits(8);
            vui->transfer_characteristics = (uint8_t) GetBits(8);
            vui->matrix_coefficients = (uint8_t) GetBits(8);
        }
    }
    vui->chroma_loc_info_present_flag = Get1Bit();
    if( vui->chroma_loc_info_present_flag ) {
        vui->chroma_sample_loc_type_top_field = (uint8_t) GetVLCElement(false);
        vui->chroma_sample_loc_type_bottom_field = (uint8_t) GetVLCElement(false);
    }
    vui->timing_info_present_flag = Get1Bit();

    if (vui->timing_info_present_flag)
    {
        vui->num_units_in_tick = GetBits(32);
        vui->time_scale = GetBits(32);
        vui->fixed_frame_rate_flag = Get1Bit();

        if (!vui->num_units_in_tick || !vui->time_scale)
            vui->timing_info_present_flag = 0;
    }

    vui->nal_hrd_parameters_present_flag = Get1Bit();
    if( vui->nal_hrd_parameters_present_flag )
        ps=GetHRDParam(sps, vui);
    vui->vcl_hrd_parameters_present_flag = Get1Bit();
    if( vui->vcl_hrd_parameters_present_flag )
        ps=GetHRDParam(sps, vui);
    if( vui->nal_hrd_parameters_present_flag  ||  vui->vcl_hrd_parameters_present_flag )
        vui->low_delay_hrd_flag = Get1Bit();
    vui->pic_struct_present_flag  = Get1Bit();
    vui->bitstream_restriction_flag = Get1Bit();
    if( vui->bitstream_restriction_flag ) {
        vui->motion_vectors_over_pic_boundaries_flag = Get1Bit();
        vui->max_bytes_per_pic_denom = (uint8_t) GetVLCElement(false);
        vui->max_bits_per_mb_denom = (uint8_t) GetVLCElement(false);
        vui->log2_max_mv_length_horizontal = (uint8_t) GetVLCElement(false);
        vui->log2_max_mv_length_vertical = (uint8_t) GetVLCElement(false);
        vui->num_reorder_frames = (uint8_t) GetVLCElement(false);

        int32_t value = GetVLCElement(false);
        if (value < (int32_t)sps->num_ref_frames || value < 0)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        vui->max_dec_frame_buffering = (uint8_t)value;
        if (vui->max_dec_frame_buffering > 16)
        {
            return UMC_ERR_INVALID_STREAM;
        }
    }

    return ps;
}

Status H264HeadersBitstream::GetHRDParam(H264SeqParamSet *, H264VUI *vui)
{
    uint32_t cpb_cnt = (uint32_t) (GetVLCElement(false) + 1);

    if (cpb_cnt >= 32)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    vui->cpb_cnt = (uint8_t)cpb_cnt;

    vui->bit_rate_scale = (uint8_t) GetBits(4);
    vui->cpb_size_scale = (uint8_t) GetBits(4);
    for( int32_t idx= 0; idx < vui->cpb_cnt; idx++ ) {
        vui->bit_rate_value[ idx ] = (uint32_t) (GetVLCElement(false)+1);
        vui->cpb_size_value[ idx ] = (uint32_t) ((GetVLCElement(false)+1));
        vui->cbr_flag[ idx ] = Get1Bit();
    }
    vui->initial_cpb_removal_delay_length = (uint8_t)(GetBits(5)+1);
    vui->cpb_removal_delay_length = (uint8_t)(GetBits(5)+1);
    vui->dpb_output_delay_length = (uint8_t) (GetBits(5)+1);
    vui->time_offset_length = (uint8_t) GetBits(5);
    return UMC_OK;
}

// ---------------------------------------------------------------------------
//    Read sequence parameter set extension data from bitstream.
// ---------------------------------------------------------------------------
Status H264HeadersBitstream::GetSequenceParamSetExtension(H264SeqParamSetExtension *sps_ex)
{
    sps_ex->Reset();

    uint32_t seq_parameter_set_id = GetVLCElement(false);
    sps_ex->seq_parameter_set_id = (uint8_t)seq_parameter_set_id;
    if (seq_parameter_set_id > MAX_NUM_SEQ_PARAM_SETS-1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    uint32_t aux_format_idc = GetVLCElement(false);
    sps_ex->aux_format_idc = (uint8_t)aux_format_idc;
    if (aux_format_idc > 3)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    if (sps_ex->aux_format_idc != 1 && sps_ex->aux_format_idc != 2)
        sps_ex->aux_format_idc = 0;

    if (sps_ex->aux_format_idc)
    {
        uint32_t bit_depth_aux = GetVLCElement(false) + 8;
        sps_ex->bit_depth_aux = (uint8_t)bit_depth_aux;
        if (bit_depth_aux > 12)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        sps_ex->alpha_incr_flag = Get1Bit();
        sps_ex->alpha_opaque_value = (uint8_t)GetBits(sps_ex->bit_depth_aux + 1);
        sps_ex->alpha_transparent_value = (uint8_t)GetBits(sps_ex->bit_depth_aux + 1);
    }

    sps_ex->additional_extension_flag = Get1Bit();

    CheckBSLeft();
    return UMC_OK;
}    // GetSequenceParamSetExtension

template <class num_t, class items_t> static
Status DecodeViewReferenceInfo(num_t &numItems, items_t *pItems, H264HeadersBitstream &bitStream)
{
    uint32_t j;

    // decode number of items
    numItems = (num_t) bitStream.GetVLCElement(false);
    if (H264_MAX_NUM_VIEW_REF <= numItems)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    // decode items
    for (j = 0; j < numItems; j += 1)
    {
        pItems[j] = (items_t) bitStream.GetVLCElement(false);
        if (H264_MAX_NUM_VIEW <= pItems[j])
        {
            return UMC_ERR_INVALID_STREAM;
        }
    }

    return UMC_OK;

} // Status DecodeViewReferenceInfo(num_t &numItems, items_t *pItems, H264HeadersBitstream &bitStream)

Status H264HeadersBitstream::GetSequenceParamSetSvcExt(H264SeqParamSetSVCExtension *pSPSSvcExt)
{
    pSPSSvcExt->Reset();

    if ((pSPSSvcExt->profile_idc != H264VideoDecoderParams::H264_PROFILE_SCALABLE_BASELINE) &&
        (pSPSSvcExt->profile_idc != H264VideoDecoderParams::H264_PROFILE_SCALABLE_HIGH))
        return UMC_OK;

    pSPSSvcExt->inter_layer_deblocking_filter_control_present_flag = Get1Bit();
    pSPSSvcExt->extended_spatial_scalability = (uint8_t)GetBits(2);

    if (pSPSSvcExt->chroma_format_idc == 1 || pSPSSvcExt->chroma_format_idc == 2)
    {
        pSPSSvcExt->chroma_phase_x = Get1Bit() - 1;
        pSPSSvcExt->seq_ref_layer_chroma_phase_x = pSPSSvcExt->chroma_phase_x;
    }

    if (pSPSSvcExt->chroma_format_idc == 1)
    {
        pSPSSvcExt->chroma_phase_y = (int8_t)(GetBits(2) - 1);
        pSPSSvcExt->seq_ref_layer_chroma_phase_y = pSPSSvcExt->chroma_phase_y;
    }

    if (pSPSSvcExt->extended_spatial_scalability == 1)
    {
        if (pSPSSvcExt->chroma_format_idc > 0)
        {
            pSPSSvcExt->seq_ref_layer_chroma_phase_x = Get1Bit() - 1;
            pSPSSvcExt->seq_ref_layer_chroma_phase_y = (int8_t)(GetBits(2) - 1);
        }

        pSPSSvcExt->seq_scaled_ref_layer_left_offset = GetVLCElement(true);
        pSPSSvcExt->seq_scaled_ref_layer_top_offset = GetVLCElement(true);
        pSPSSvcExt->seq_scaled_ref_layer_right_offset = GetVLCElement(true);
        pSPSSvcExt->seq_scaled_ref_layer_bottom_offset = GetVLCElement(true);
    }

    pSPSSvcExt->seq_tcoeff_level_prediction_flag = Get1Bit();

    if (pSPSSvcExt->seq_tcoeff_level_prediction_flag)
    {
        pSPSSvcExt->adaptive_tcoeff_level_prediction_flag = Get1Bit();
    }

    pSPSSvcExt->slice_header_restriction_flag = Get1Bit();

    CheckBSLeft();

    /* Reference has SVC VUI extension here, but there is no mention about it in standard */
    uint32_t svc_vui_parameters_present_flag = Get1Bit();
    if (svc_vui_parameters_present_flag)
    {
        GetSequenceParamSetSvcVuiExt(pSPSSvcExt); // ignore return status
    }

    Get1Bit(); // additional_extension2_flag
    return UMC_OK;
}

struct VUI_Entry
{
    uint32_t vui_ext_dependency_id;
    uint32_t vui_ext_quality_id;
    uint32_t vui_ext_temporal_id;
    uint32_t vui_ext_timing_info_present_flag;
    uint32_t vui_ext_num_units_in_tick;
    uint32_t vui_ext_time_scale;
    uint32_t vui_ext_fixed_frame_rate_flag;

    uint32_t vui_ext_nal_hrd_parameters_present_flag;
    uint32_t vui_ext_vcl_hrd_parameters_present_flag;

    uint32_t vui_ext_low_delay_hrd_flag;
    uint32_t vui_ext_pic_struct_present_flag;
};

struct SVC_VUI
{
    uint32_t vui_ext_num_entries;
    VUI_Entry vui_entry[1024];
};

Status H264HeadersBitstream::GetSequenceParamSetSvcVuiExt(H264SeqParamSetSVCExtension *pSPSSvcExt)
{
    SVC_VUI vui;
    vui.vui_ext_num_entries = GetVLCElement(false) + 1;
    if(vui.vui_ext_num_entries > H264_MAX_VUI_EXT_NUM)
        return UMC_ERR_INVALID_STREAM;
    for(uint32_t i = 0; i < vui.vui_ext_num_entries; i++)
    {
        vui.vui_entry[i].vui_ext_dependency_id      = GetBits(3);
        vui.vui_entry[i].vui_ext_quality_id         = GetBits(4);
        vui.vui_entry[i].vui_ext_temporal_id        = GetBits(3);

        vui.vui_entry[i].vui_ext_timing_info_present_flag = Get1Bit();

        if (vui.vui_entry[i].vui_ext_timing_info_present_flag)
        {
            vui.vui_entry[i].vui_ext_num_units_in_tick  = GetBits(32);
            vui.vui_entry[i].vui_ext_time_scale         = GetBits(32);
            vui.vui_entry[i].vui_ext_fixed_frame_rate_flag  = Get1Bit();
        }

        vui.vui_entry[i].vui_ext_nal_hrd_parameters_present_flag = Get1Bit();
        if (vui.vui_entry[i].vui_ext_nal_hrd_parameters_present_flag)
        {
            Status sts = GetHRDParam(pSPSSvcExt, &pSPSSvcExt->vui);
            if (sts != UMC_OK)
                return sts;
        }

        vui.vui_entry[i].vui_ext_vcl_hrd_parameters_present_flag = Get1Bit();
        if (vui.vui_entry[i].vui_ext_vcl_hrd_parameters_present_flag)
        {
            Status sts = GetHRDParam(pSPSSvcExt, &pSPSSvcExt->vui);
            if (sts != UMC_OK)
                return sts;
        }

        if (vui.vui_entry[i].vui_ext_nal_hrd_parameters_present_flag || vui.vui_entry[i].vui_ext_vcl_hrd_parameters_present_flag)
            vui.vui_entry[i].vui_ext_low_delay_hrd_flag = Get1Bit();

        vui.vui_entry[i].vui_ext_pic_struct_present_flag = Get1Bit();
    }

    return UMC_OK;
}

Status H264HeadersBitstream::GetSequenceParamSetMvcExt(H264SeqParamSetMVCExtension *pSPSMvcExt)
{
    pSPSMvcExt->Reset();

    // decode the number of available views
    pSPSMvcExt->num_views_minus1 = GetVLCElement(false);
    if (H264_MAX_NUM_VIEW <= pSPSMvcExt->num_views_minus1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    // allocate the views' info structs
    pSPSMvcExt->viewInfo.resize(pSPSMvcExt->num_views_minus1 + 1, H264ViewRefInfo());

    // parse view IDs
    for (uint32_t i = 0; i <= pSPSMvcExt->num_views_minus1; i += 1)
    {
        H264ViewRefInfo &viewRefInfo = pSPSMvcExt->viewInfo[i];

        // get the view ID
        viewRefInfo.view_id = GetVLCElement(false);
        if (H264_MAX_NUM_VIEW <= viewRefInfo.view_id)
        {
            return UMC_ERR_INVALID_STREAM;
        }
    }

    // parse anchor refs
    for (uint32_t i = 1; i <= pSPSMvcExt->num_views_minus1; i += 1)
    {
        H264ViewRefInfo &viewRefInfo = pSPSMvcExt->viewInfo[i];
        uint32_t listNum;

        for (listNum = LIST_0; listNum <= LIST_1; listNum += 1)
        {
            // decode LX anchor refs info
            Status umcRes = DecodeViewReferenceInfo(viewRefInfo.num_anchor_refs_lx[listNum],
                                             viewRefInfo.anchor_refs_lx[listNum],
                                             *this);
            if (UMC_OK != umcRes)
            {
                return umcRes;
            }
        }
    }

    // parse non-anchor refs
    for (uint32_t i = 1; i <= pSPSMvcExt->num_views_minus1; i += 1)
    {
        H264ViewRefInfo &viewRefInfo = pSPSMvcExt->viewInfo[i];
        uint32_t listNum;

        for (listNum = LIST_0; listNum <= LIST_1; listNum += 1)
        {

            // decode L0 non-anchor refs info
            Status umcRes = DecodeViewReferenceInfo(viewRefInfo.num_non_anchor_refs_lx[listNum],
                                             viewRefInfo.non_anchor_refs_lx[listNum],
                                             *this);
            if (UMC_OK != umcRes)
            {
                return umcRes;
            }
        }
    }

    // decode the number of applicable signal points
    pSPSMvcExt->num_level_values_signalled_minus1 = GetVLCElement(false);
    if (H264_MAX_NUM_OPS <= pSPSMvcExt->num_level_values_signalled_minus1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    // allocate the level info structure
    pSPSMvcExt->levelInfo.resize(pSPSMvcExt->num_level_values_signalled_minus1 + 1, H264LevelValueSignaled());

    // decode all ops
    for (uint32_t i = 0; i <= pSPSMvcExt->num_level_values_signalled_minus1; i += 1)
    {
        H264LevelValueSignaled &levelInfo = pSPSMvcExt->levelInfo[i];
        uint32_t j;

        // decode the level's profile idc
        levelInfo.level_idc = (uint8_t) GetBits(8);
        MFX_CHECK(CheckLevel(levelInfo.level_idc), UMC_ERR_INVALID_STREAM);

        // decode the number of operation points
        levelInfo.num_applicable_ops_minus1 = (uint16_t) GetVLCElement(false);
        if (H264_MAX_NUM_VIEW <= levelInfo.num_applicable_ops_minus1)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        // allocate the operation points structures
        levelInfo.opsInfo.resize(levelInfo.num_applicable_ops_minus1 + 1, H264ApplicableOp());

        // decode operation points
        for (j = 0; j <= levelInfo.num_applicable_ops_minus1; j += 1)
        {
            H264ApplicableOp &op = levelInfo.opsInfo[j];
            uint32_t k;

            // decode the temporal ID of the op
            op.applicable_op_temporal_id = (uint8_t) GetBits(3);

            // decode the number of target views
            op.applicable_op_num_target_views_minus1 = (uint16_t) GetVLCElement(false);
            if (H264_MAX_NUM_VIEW <= op.applicable_op_num_target_views_minus1)
            {
                return UMC_ERR_INVALID_STREAM;
            }

            // allocate the target view ID array
            op.applicable_op_target_view_id.resize(op.applicable_op_num_target_views_minus1 + 1, 0);

            // read target view IDs
            for (k = 0; k <= op.applicable_op_num_target_views_minus1; k += 1)
            {
                op.applicable_op_target_view_id[k] = (uint16_t) GetVLCElement(false);
                if (H264_MAX_NUM_VIEW <= op.applicable_op_target_view_id[k])
                {
                    return UMC_ERR_INVALID_STREAM;
                }
            }

            // decode applicable number of views
            op.applicable_op_num_views_minus1 = (uint16_t) GetVLCElement(false);
            if (H264_MAX_NUM_VIEW <= op.applicable_op_num_views_minus1)
            {
                return UMC_ERR_INVALID_STREAM;
            }
        }
    }

    CheckBSLeft();
    return UMC_OK;

} // Status H264HeadersBitstream::GetSequenceParamSetMvcExt(H264SeqParamSetMVCExtension *pSPSMvcExt)

Status H264HeadersBitstream::GetPictureParamSetPart1(H264PicParamSet *pps)
{
    // Not all members of the pic param set structure are contained in all
    // pic param sets. So start by init all to zero.
    pps->Reset();

    // id
    uint32_t pic_parameter_set_id = GetVLCElement(false);
    pps->pic_parameter_set_id = (uint16_t)pic_parameter_set_id;
    if (pic_parameter_set_id > MAX_NUM_PIC_PARAM_SETS-1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    // seq param set referred to by this pic param set
    uint32_t seq_parameter_set_id = GetVLCElement(false);
    pps->seq_parameter_set_id = (uint8_t)seq_parameter_set_id;
    if (seq_parameter_set_id > MAX_NUM_SEQ_PARAM_SETS-1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    CheckBSLeft();
    return UMC_OK;
}    // GetPictureParamSetPart1

// Number of bits required to code slice group ID, index is num_slice_groups - 2
static const uint8_t SGIdBits[7] = {1,2,2,3,3,3,3};

// ---------------------------------------------------------------------------
//    Read picture parameter set data from bitstream.
// ---------------------------------------------------------------------------
Status H264HeadersBitstream::GetPictureParamSetPart2(H264PicParamSet  *pps, H264SeqParamSet const* sps)
{
    if (!sps)
        return UMC_ERR_NULL_PTR;

    pps->entropy_coding_mode = Get1Bit();

    pps->bottom_field_pic_order_in_frame_present_flag = Get1Bit();

    // number of slice groups, bitstream has value - 1
    pps->num_slice_groups = GetVLCElement(false) + 1;
    if (pps->num_slice_groups != 1)
    {
        if (pps->num_slice_groups > MAX_NUM_SLICE_GROUPS)
        {
            return UMC_ERR_INVALID_STREAM;
        }

        uint32_t slice_group_map_type = GetVLCElement(false);
        pps->SliceGroupInfo.slice_group_map_type = (uint8_t)slice_group_map_type;

        if (slice_group_map_type > 6)
            return UMC_ERR_INVALID_STREAM;

        uint32_t const PicSizeInMapUnits = sps->frame_width_in_mbs * sps->frame_height_in_mbs;
        // Get additional, map type dependent slice group data
        switch (pps->SliceGroupInfo.slice_group_map_type)
        {
        case 0:
            for (uint32_t slice_group = 0; slice_group < pps->num_slice_groups; slice_group++)
            {
                // run length, bitstream has value - 1
                pps->SliceGroupInfo.run_length[slice_group] = GetVLCElement(false) + 1;
                if (pps->SliceGroupInfo.run_length[slice_group] > PicSizeInMapUnits)
                    return UMC_ERR_INVALID_STREAM;
            }
            break;

        case 1:
            // no additional info
            break;

        case 2:
            for (uint32_t slice_group = 0; slice_group < (uint32_t)(pps->num_slice_groups-1); slice_group++)
            {
                pps->SliceGroupInfo.t1.top_left[slice_group] = GetVLCElement(false);
                if (pps->SliceGroupInfo.t1.top_left[slice_group] >= PicSizeInMapUnits)
                    return UMC_ERR_INVALID_STREAM;

                pps->SliceGroupInfo.t1.bottom_right[slice_group] = GetVLCElement(false);
                if (pps->SliceGroupInfo.t1.bottom_right[slice_group] >= PicSizeInMapUnits)
                    return UMC_ERR_INVALID_STREAM;

                // check for legal values
                if (pps->SliceGroupInfo.t1.top_left[slice_group] >
                    pps->SliceGroupInfo.t1.bottom_right[slice_group])
                {
                    return UMC_ERR_INVALID_STREAM;
                }
            }
            break;

        case 3:
        case 4:
        case 5:
            // For map types 3..5, number of slice groups must be 2
            if (pps->num_slice_groups != 2)
            {
                return UMC_ERR_INVALID_STREAM;
            }
            pps->SliceGroupInfo.t2.slice_group_change_direction_flag = Get1Bit();
            pps->SliceGroupInfo.t2.slice_group_change_rate = GetVLCElement(false) + 1;
            if (pps->SliceGroupInfo.t2.slice_group_change_rate > PicSizeInMapUnits)
                return UMC_ERR_INVALID_STREAM;
            break;

        case 6:
            // mapping of slice group to map unit (macroblock if not fields) is
            // per map unit, read from bitstream
            {
                // number of map units, bitstream has value - 1
                pps->SliceGroupInfo.t3.pic_size_in_map_units = GetVLCElement(false) + 1;
                if (pps->SliceGroupInfo.t3.pic_size_in_map_units != PicSizeInMapUnits)
                    return UMC_ERR_INVALID_STREAM;

                uint32_t const len = std::max(1u, pps->SliceGroupInfo.t3.pic_size_in_map_units);

                pps->SliceGroupInfo.pSliceGroupIDMap.resize(len);

                // num_bits is Ceil(log2(num_groups)) - number of bits used to code each slice group id
                uint32_t const num_bits = SGIdBits[pps->num_slice_groups - 2];

                for (uint32_t map_unit = 0; map_unit < pps->SliceGroupInfo.t3.pic_size_in_map_units; map_unit++)
                {
                    uint8_t const slice_group_id = (uint8_t)GetBits(num_bits);
                    if (slice_group_id >  pps->num_slice_groups - 1)
                        return UMC_ERR_INVALID_STREAM;

                    pps->SliceGroupInfo.pSliceGroupIDMap[map_unit] = slice_group_id;
                }
            }
            break;

        default:
            return UMC_ERR_INVALID_STREAM;

        }    // switch
    }    // slice group info

    // number of list 0 ref pics used to decode picture, bitstream has value - 1
    pps->num_ref_idx_l0_active = GetVLCElement(false) + 1;

    // number of list 1 ref pics used to decode picture, bitstream has value - 1
    pps->num_ref_idx_l1_active = GetVLCElement(false) + 1;

    if (pps->num_ref_idx_l1_active > MAX_NUM_REF_FRAMES || pps->num_ref_idx_l0_active > MAX_NUM_REF_FRAMES)
        return UMC_ERR_INVALID_STREAM;

    // weighted pediction
    pps->weighted_pred_flag = Get1Bit();
    pps->weighted_bipred_idc = (uint8_t)GetBits(2);

    // default slice QP, bitstream has value - 26
    int32_t pic_init_qp = GetVLCElement(true) + 26;
    pps->pic_init_qp = (int8_t)pic_init_qp;
    if (pic_init_qp > QP_MAX)
    {
        //return UMC_ERR_INVALID_STREAM;
    }

    // default SP/SI slice QP, bitstream has value - 26
    pps->pic_init_qs = (uint8_t)(GetVLCElement(true) + 26);
    if (pps->pic_init_qs > QP_MAX)
    {
        //return UMC_ERR_INVALID_STREAM;
    }

    pps->chroma_qp_index_offset[0] = (int8_t)GetVLCElement(true);
    if ((pps->chroma_qp_index_offset[0] < -12) || (pps->chroma_qp_index_offset[0] > 12))
    {
        //return UMC_ERR_INVALID_STREAM;
    }

    pps->deblocking_filter_variables_present_flag = Get1Bit();
    pps->constrained_intra_pred_flag = Get1Bit();
    pps->redundant_pic_cnt_present_flag = Get1Bit();

    if (More_RBSP_Data())
    {
        pps->transform_8x8_mode_flag = Get1Bit();
        pps->pic_scaling_matrix_present_flag = Get1Bit();

        H264ScalingPicParams * scaling = &pps->scaling[0];

        if (pps->pic_scaling_matrix_present_flag)
        {
            for (int32_t i = 0; i < 3; i++)
            {
                pps->pic_scaling_list_present_flag[i] = Get1Bit();
                if (pps->pic_scaling_list_present_flag[i])
                {
                    GetScalingList4x4(&scaling->ScalingLists4x4[i], (uint8_t*)default_intra_scaling_list4x4, &pps->type_of_scaling_list_used[i]);
                }
                else
                {
                    pps->type_of_scaling_list_used[i] = SCLDEFAULT;
                }
            }

            for (int32_t i = 3; i < 6; i++)
            {
                pps->pic_scaling_list_present_flag[i] = Get1Bit();
                if (pps->pic_scaling_list_present_flag[i])
                {
                    GetScalingList4x4(&scaling->ScalingLists4x4[i], (uint8_t*)default_inter_scaling_list4x4, &pps->type_of_scaling_list_used[i]);
                }
                else
                {
                    pps->type_of_scaling_list_used[i] = SCLDEFAULT;
                }
            }


            if (pps->transform_8x8_mode_flag)
            {
                pps->pic_scaling_list_present_flag[6] = Get1Bit();
                if (pps->pic_scaling_list_present_flag[6])
                {
                    GetScalingList8x8(&scaling->ScalingLists8x8[0], (uint8_t*)default_intra_scaling_list8x8, &pps->type_of_scaling_list_used[6]);
                }
                else
                {
                    pps->type_of_scaling_list_used[6] = SCLDEFAULT;
                }

                pps->pic_scaling_list_present_flag[7] = Get1Bit();
                if (pps->pic_scaling_list_present_flag[7])
                {
                    GetScalingList8x8(&scaling->ScalingLists8x8[1], (uint8_t*)default_inter_scaling_list8x8, &pps->type_of_scaling_list_used[7]);
                }
                else
                {
                    pps->type_of_scaling_list_used[7] = SCLDEFAULT;
                }
            }

            MFX_INTERNAL_CPY(&pps->scaling[1], &pps->scaling[0], sizeof(H264ScalingPicParams));
        }

        pps->chroma_qp_index_offset[1] = (int8_t)GetVLCElement(true);
    }
    else
    {
        pps->chroma_qp_index_offset[1] = pps->chroma_qp_index_offset[0];
    }

    CheckBSLeft();
    return UMC_OK;
}    // GetPictureParamSet

Status InitializePictureParamSet(H264PicParamSet *pps, const H264SeqParamSet *sps, bool isExtension)
{
    if (!pps || !sps || pps->initialized[isExtension])
        return UMC_OK;

    if (pps->num_slice_groups != 1)
    {
        uint32_t PicSizeInMapUnits = sps->frame_width_in_mbs * sps->frame_height_in_mbs; // for range checks

        // Get additional, map type dependent slice group data
        switch (pps->SliceGroupInfo.slice_group_map_type)
        {
        case 0:
            for (uint32_t slice_group=0; slice_group<pps->num_slice_groups; slice_group++)
            {
                if (pps->SliceGroupInfo.run_length[slice_group] > PicSizeInMapUnits)
                {
                    return UMC_ERR_INVALID_STREAM;
                }
            }
            break;
        case 1:
            // no additional info
            break;
        case 2:
            for (uint32_t slice_group=0; slice_group<(uint32_t)(pps->num_slice_groups-1); slice_group++)
            {
                if (pps->SliceGroupInfo.t1.bottom_right[slice_group] >= PicSizeInMapUnits)
                {
                    return UMC_ERR_INVALID_STREAM;
                }
                if ((pps->SliceGroupInfo.t1.top_left[slice_group] %
                    sps->frame_width_in_mbs) >
                    (pps->SliceGroupInfo.t1.bottom_right[slice_group] %
                    sps->frame_width_in_mbs))
                {
                    return UMC_ERR_INVALID_STREAM;
                }
            }
            break;
        case 3:
        case 4:
        case 5:
            if (pps->SliceGroupInfo.t2.slice_group_change_rate > PicSizeInMapUnits)
            {
                return UMC_ERR_INVALID_STREAM;
            }
            break;
        case 6:
            // mapping of slice group to map unit (macroblock if not fields) is
            // per map unit, read from bitstream
            {
                if (pps->SliceGroupInfo.t3.pic_size_in_map_units != PicSizeInMapUnits)
                {
                    return UMC_ERR_INVALID_STREAM;
                }
                for (uint32_t map_unit = 0;
                     map_unit < pps->SliceGroupInfo.t3.pic_size_in_map_units;
                     map_unit++)
                {
                    if (pps->SliceGroupInfo.pSliceGroupIDMap[map_unit] >
                        pps->num_slice_groups - 1)
                    {
                        return UMC_ERR_INVALID_STREAM;
                    }
                }
            }
            break;
        default:
            return UMC_ERR_INVALID_STREAM;

        }    // switch
    }    // slice group info

    H264ScalingPicParams * scaling = &pps->scaling[isExtension];
    if (pps->pic_scaling_matrix_present_flag)
    {
        uint8_t *default_scaling = (uint8_t*)default_intra_scaling_list4x4;
        for (int32_t i = 0; i < 6; i += 3)
        {
            if (!pps->pic_scaling_list_present_flag[i])
            {
                if (sps->seq_scaling_matrix_present_flag) {
                    FillScalingList4x4(&scaling->ScalingLists4x4[i], (uint8_t*)sps->ScalingLists4x4[i].ScalingListCoeffs);
                }
                else
                {
                    FillScalingList4x4(&scaling->ScalingLists4x4[i], (uint8_t*)default_scaling);
                }
            }

            if (!pps->pic_scaling_list_present_flag[i+1])
            {
                FillScalingList4x4(&scaling->ScalingLists4x4[i+1], (uint8_t*)scaling->ScalingLists4x4[i].ScalingListCoeffs);
            }

            if (!pps->pic_scaling_list_present_flag[i+2])
            {
                FillScalingList4x4(&scaling->ScalingLists4x4[i+2], (uint8_t*)scaling->ScalingLists4x4[i+1].ScalingListCoeffs);
            }

            default_scaling = (uint8_t*)default_inter_scaling_list4x4;
        }

        if (pps->transform_8x8_mode_flag)
        {
            if (sps->seq_scaling_matrix_present_flag) {
                for (int32_t i = 6; i < 8; i++)
                {
                    if (!pps->pic_scaling_list_present_flag[i])
                    {
                        FillScalingList8x8(&scaling->ScalingLists8x8[i-6], (uint8_t*)sps->ScalingLists8x8[i-6].ScalingListCoeffs);
                    }
                }
            }
            else
            {
                if (!pps->pic_scaling_list_present_flag[6])
                {
                    FillScalingList8x8(&scaling->ScalingLists8x8[0], (uint8_t*)default_intra_scaling_list8x8);
                }

                if (!pps->pic_scaling_list_present_flag[7])
                {
                    FillScalingList8x8(&scaling->ScalingLists8x8[1], (uint8_t*)default_inter_scaling_list8x8);
                }
            }
        }
    }
    else
    {
        for (int32_t i = 0; i < 6; i++)
        {
            FillScalingList4x4(&scaling->ScalingLists4x4[i],(uint8_t *)sps->ScalingLists4x4[i].ScalingListCoeffs);
            pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
        }

        if (pps->transform_8x8_mode_flag)
        {
            for (int32_t i = 0; i < 2; i++)
            {
                FillScalingList8x8(&scaling->ScalingLists8x8[i],(uint8_t *)sps->ScalingLists8x8[i].ScalingListCoeffs);
                pps->type_of_scaling_list_used[i] = sps->type_of_scaling_list_used[i];
            }
        }
    }

    // calculate level scale matrices

    //start DC first
    //to do: reduce th anumber of matrices (in fact 1 is enough)
    // now process other 4x4 matrices
    for (int32_t i = 0; i < 6; i++)
    {
        for (int32_t j = 0; j < 88; j++)
            for (int32_t k = 0; k < 16; k++)
            {
                uint32_t level_scale = scaling->ScalingLists4x4[i].ScalingListCoeffs[k]*pre_norm_adjust4x4[j%6][pre_norm_adjust_index4x4[k]];
                scaling->m_LevelScale4x4[i].LevelScaleCoeffs[j][k] = (int16_t) level_scale;
            }
    }

    // process remaining 8x8  matrices
    for (int32_t i = 0; i < 2; i++)
    {
        for (int32_t j = 0; j < 88; j++)
            for (int32_t k = 0; k < 64; k++)
            {
                uint32_t level_scale = scaling->ScalingLists8x8[i].ScalingListCoeffs[k]*pre_norm_adjust8x8[j%6][pre_norm_adjust_index8x8[k]];
                scaling->m_LevelScale8x8[i].LevelScaleCoeffs[j][k] = (int16_t) level_scale;
            }
    }

    pps->initialized[isExtension] = 1;
    return UMC_OK;
}

Status H264HeadersBitstream::GetNalUnitPrefix(H264NalExtension *pExt, uint32_t NALRef_idc)
{
    Status ps = UMC_OK;

    ps = GetNalUnitExtension(pExt);

    if (ps != UMC_OK || !pExt->svc_extension_flag)
        return ps;

    if (pExt->svc.dependency_id || pExt->svc.quality_id) // shall be equals zero for prefix NAL units
        return UMC_ERR_INVALID_STREAM;

    pExt->svc.adaptiveMarkingInfo.num_entries = 0;

    if (NALRef_idc != 0)
    {
        pExt->svc.store_ref_base_pic_flag = Get1Bit();

        if ((pExt->svc.use_ref_base_pic_flag || pExt->svc.store_ref_base_pic_flag) && !pExt->svc.idr_flag)
        {
            ps = DecRefBasePicMarking(&pExt->svc.adaptiveMarkingInfo, pExt->svc.adaptive_ref_base_pic_marking_mode_flag);
            if (ps != UMC_OK)
                return ps;
        }

        /*int32_t additional_prefix_nal_unit_extension_flag = (int32_t) Get1Bit();

        if (additional_prefix_nal_unit_extension_flag == 1)
        {
            additional_prefix_nal_unit_extension_flag = Get1Bit();
        }*/
    }

    CheckBSLeft();
    return ps;
} // Status H264HeadersBitstream::GetNalUnitPrefix(void)

Status H264HeadersBitstream::GetNalUnitExtension(H264NalExtension *pExt)
{
    pExt->Reset();
    pExt->extension_present = 1;

    // decode the type of the extension
    pExt->svc_extension_flag = (uint8_t) GetBits(1);

    // decode SVC extension
    if (pExt->svc_extension_flag)
    {
        pExt->svc.idr_flag = Get1Bit();
        pExt->svc.priority_id = (uint8_t) GetBits(6);
        pExt->svc.no_inter_layer_pred_flag = Get1Bit();
        pExt->svc.dependency_id = (uint8_t) GetBits(3);
        pExt->svc.quality_id = (uint8_t) GetBits(4);
        pExt->svc.temporal_id = (uint8_t) GetBits(3);
        pExt->svc.use_ref_base_pic_flag = Get1Bit();
        pExt->svc.discardable_flag = Get1Bit();
        pExt->svc.output_flag = Get1Bit();
        GetBits(2);
    }
    // decode MVC extension
    else
    {
        pExt->mvc.non_idr_flag = Get1Bit();
        pExt->mvc.priority_id = (uint16_t) GetBits(6);
        pExt->mvc.view_id = (uint16_t) GetBits(10);
        pExt->mvc.temporal_id = (uint8_t) GetBits(3);
        pExt->mvc.anchor_pic_flag = Get1Bit();
        pExt->mvc.inter_view_flag = Get1Bit();
        GetBits(1);
    }

    CheckBSLeft();
    return UMC_OK;

} // Status H264HeadersBitstream::GetNalUnitExtension(H264NalExtension *pExt)

// ---------------------------------------------------------------------------
//    Read H.264 first part of slice header
//
//  Reading the rest of the header requires info in the picture and sequence
//  parameter sets referred to by this slice header.
//
//    Do not print debug messages when IsSearch is true. In that case the function
//    is being used to find the next compressed frame, errors may occur and should
//    not be reported.
//
// ---------------------------------------------------------------------------
Status H264HeadersBitstream::GetSliceHeaderPart1(H264SliceHeader *hdr)
{
    uint32_t val;

    // decode NAL extension
    if (NAL_UT_CODED_SLICE_EXTENSION == hdr->nal_unit_type)
    {
        GetNalUnitExtension(&hdr->nal_ext);

        // set the IDR flag
        if (hdr->nal_ext.svc_extension_flag)
        {
            hdr->IdrPicFlag = hdr->nal_ext.svc.idr_flag;
        }
        else
        {
            hdr->IdrPicFlag = hdr->nal_ext.mvc.non_idr_flag ^ 1;
        }
    }
    else
    {
        if (hdr->nal_ext.extension_present)
        {
            if (hdr->nal_ext.svc_extension_flag)
            {
                //hdr->IdrPicFlag = hdr->nal_ext.svc.idr_flag;
            }
            else
            {
                //hdr->IdrPicFlag = hdr->nal_ext.mvc.non_idr_flag ^ 1;
            }
        }
        else
        {
            //hdr->IdrPicFlag = (NAL_UT_IDR_SLICE == hdr->nal_unit_type) ? 1 : 0;
            hdr->nal_ext.mvc.anchor_pic_flag = (uint8_t) hdr->IdrPicFlag ? 1 : 0;
            hdr->nal_ext.mvc.inter_view_flag = (uint8_t) 1;
        }

        hdr->IdrPicFlag = (NAL_UT_IDR_SLICE == hdr->nal_unit_type) ? 1 : 0;
    }

    hdr->first_mb_in_slice = GetVLCElement(false);
    if (0 > hdr->first_mb_in_slice) // upper bound is checked in H264Slice
        return UMC_ERR_INVALID_STREAM;

    // slice type
    val = GetVLCElement(false);
    if (val > S_INTRASLICE)
    {
        if (val > S_INTRASLICE + S_INTRASLICE + 1)
        {
            return UMC_ERR_INVALID_STREAM;
        }
        else
        {
            // Slice type is specifying type of not only this but all remaining
            // slices in the picture. Since slice type is always present, this bit
            // of info is not used in our implementation. Adjust (just shift range)
            // and return type without this extra info.
            val -= (S_INTRASLICE + 1);
        }
    }

    if (val > INTRASLICE) // all other doesn't support
        return UMC_ERR_INVALID_STREAM;

    hdr->slice_type = (EnumSliceCodType)val;
    if (NAL_UT_IDR_SLICE == hdr->nal_unit_type && hdr->slice_type != INTRASLICE)
        return UMC_ERR_INVALID_STREAM;

    uint32_t pic_parameter_set_id = GetVLCElement(false);
    hdr->pic_parameter_set_id = (uint16_t)pic_parameter_set_id;
    if (pic_parameter_set_id > MAX_NUM_PIC_PARAM_SETS - 1)
    {
        return UMC_ERR_INVALID_STREAM;
    }

    CheckBSLeft();
    return UMC_OK;
} // Status GetSliceHeaderPart1(H264SliceHeader *pSliceHeader)

Status H264HeadersBitstream::GetSliceHeaderPart2(H264SliceHeader *hdr,
                                                 const H264PicParamSet *pps,
                                                 const H264SeqParamSet *sps)
{
    hdr->frame_num = GetBits(sps->log2_max_frame_num);

    hdr->bottom_field_flag = 0;
    if (sps->frame_mbs_only_flag == 0)
    {
        hdr->field_pic_flag = Get1Bit();
        hdr->MbaffFrameFlag = !hdr->field_pic_flag && sps->mb_adaptive_frame_field_flag;
        if (hdr->field_pic_flag != 0)
        {
            hdr->bottom_field_flag = Get1Bit();
        }
    }

    if (hdr->MbaffFrameFlag)
    {
        uint32_t const first_mb_in_slice
            = hdr->first_mb_in_slice;

        if (first_mb_in_slice * 2 > INT_MAX)
            return UMC_ERR_INVALID_STREAM;

        // correct frst_mb_in_slice in order to handle MBAFF
        hdr->first_mb_in_slice *= 2;
    }

    if (hdr->IdrPicFlag)
    {
        int32_t pic_id = hdr->idr_pic_id = GetVLCElement(false);
        if (pic_id < 0 || pic_id > 65535)
            return UMC_ERR_INVALID_STREAM;
    }

    if (sps->pic_order_cnt_type == 0)
    {
        hdr->pic_order_cnt_lsb = GetBits(sps->log2_max_pic_order_cnt_lsb);
        if (pps->bottom_field_pic_order_in_frame_present_flag && (!hdr->field_pic_flag))
            hdr->delta_pic_order_cnt_bottom = GetVLCElement(true);
    }

    if ((sps->pic_order_cnt_type == 1) && (sps->delta_pic_order_always_zero_flag == 0))
    {
        hdr->delta_pic_order_cnt[0] = GetVLCElement(true);
        if (pps->bottom_field_pic_order_in_frame_present_flag && (!hdr->field_pic_flag))
            hdr->delta_pic_order_cnt[1] = GetVLCElement(true);
    }

    if (pps->redundant_pic_cnt_present_flag)
    {
        hdr->hw_wa_redundant_elimination_bits[0] = (uint32_t)BitsDecoded();
        // redundant pic count
        hdr->redundant_pic_cnt = GetVLCElement(false);
        if (hdr->redundant_pic_cnt > 127)
            return UMC_ERR_INVALID_STREAM;

        hdr->hw_wa_redundant_elimination_bits[1] = (uint32_t)BitsDecoded();
    }

    CheckBSLeft();
    return UMC_OK;
}

Status H264HeadersBitstream::DecRefBasePicMarking(AdaptiveMarkingInfo *pAdaptiveMarkingInfo,
                                    uint8_t &adaptive_ref_pic_marking_mode_flag)
{
    uint32_t memory_management_control_operation;
    uint32_t num_entries = 0;

    adaptive_ref_pic_marking_mode_flag = Get1Bit();

    while (adaptive_ref_pic_marking_mode_flag != 0) {
        memory_management_control_operation = (uint8_t)GetVLCElement(false);
        if (memory_management_control_operation == 0)
            break;

        if (memory_management_control_operation > 6)
            return UMC_ERR_INVALID_STREAM;

        pAdaptiveMarkingInfo->mmco[num_entries] =
            (uint8_t)memory_management_control_operation;

        if (memory_management_control_operation != 5)
            pAdaptiveMarkingInfo->value[num_entries*2] = GetVLCElement(false);

        // Only mmco 3 requires 2 values
        if (memory_management_control_operation == 3)
            pAdaptiveMarkingInfo->value[num_entries*2+1] = GetVLCElement(false);

        num_entries++;
        if (num_entries >= MAX_NUM_REF_FRAMES) {
            return UMC_ERR_INVALID_STREAM;
        }
    }    // while
    pAdaptiveMarkingInfo->num_entries = num_entries;
    return UMC_OK;
}


Status H264HeadersBitstream::GetPredWeightTable(
    H264SliceHeader *hdr,        // slice header read goes here
    const H264SeqParamSet *sps,
    PredWeightTable *pPredWeight_L0, // L0 weight table goes here
    PredWeightTable *pPredWeight_L1) // L1 weight table goes here
{
    uint32_t luma_log2_weight_denom = GetVLCElement(false);
    hdr->luma_log2_weight_denom = (uint8_t)luma_log2_weight_denom;
    if (luma_log2_weight_denom > 7)
        return UMC_ERR_INVALID_STREAM;

    if (sps->chroma_format_idc != 0)
    {
        uint32_t chroma_log2_weight_denom = GetVLCElement(false);
        hdr->chroma_log2_weight_denom = (uint8_t)chroma_log2_weight_denom;
        if (chroma_log2_weight_denom > 7)
            return UMC_ERR_INVALID_STREAM;
    }

    for (int32_t refindex = 0; refindex < hdr->num_ref_idx_l0_active; refindex++)
    {
        pPredWeight_L0[refindex].luma_weight_flag = Get1Bit();
        if (pPredWeight_L0[refindex].luma_weight_flag)
        {
            pPredWeight_L0[refindex].luma_weight = (int8_t)GetVLCElement(true);
            pPredWeight_L0[refindex].luma_offset = (int8_t)GetVLCElement(true);
            pPredWeight_L0[refindex].luma_offset <<= (sps->bit_depth_luma - 8);
        }
        else
        {
            pPredWeight_L0[refindex].luma_weight = (int8_t)(1 << hdr->luma_log2_weight_denom);
            pPredWeight_L0[refindex].luma_offset = 0;
        }

        pPredWeight_L0[refindex].chroma_weight_flag = 0;
        if (sps->chroma_format_idc != 0)
        {
            pPredWeight_L0[refindex].chroma_weight_flag = Get1Bit();
        }

        if (pPredWeight_L0[refindex].chroma_weight_flag)
        {
            pPredWeight_L0[refindex].chroma_weight[0] = (int8_t)GetVLCElement(true);
            pPredWeight_L0[refindex].chroma_offset[0] = (int8_t)GetVLCElement(true);
            pPredWeight_L0[refindex].chroma_weight[1] = (int8_t)GetVLCElement(true);
            pPredWeight_L0[refindex].chroma_offset[1] = (int8_t)GetVLCElement(true);

            pPredWeight_L0[refindex].chroma_offset[0] <<= (sps->bit_depth_chroma - 8);
            pPredWeight_L0[refindex].chroma_offset[1] <<= (sps->bit_depth_chroma - 8);
        }
        else
        {
            pPredWeight_L0[refindex].chroma_weight[0] = (int8_t)(1 << hdr->chroma_log2_weight_denom);
            pPredWeight_L0[refindex].chroma_weight[1] = (int8_t)(1 << hdr->chroma_log2_weight_denom);
            pPredWeight_L0[refindex].chroma_offset[0] = 0;
            pPredWeight_L0[refindex].chroma_offset[1] = 0;
        }
    }

    if (BPREDSLICE == hdr->slice_type)
    {
        for (int32_t refindex = 0; refindex < hdr->num_ref_idx_l1_active; refindex++)
        {
            pPredWeight_L1[refindex].luma_weight_flag = Get1Bit();
            if (pPredWeight_L1[refindex].luma_weight_flag)
            {
                pPredWeight_L1[refindex].luma_weight = (int8_t)GetVLCElement(true);
                pPredWeight_L1[refindex].luma_offset = (int8_t)GetVLCElement(true);
                pPredWeight_L1[refindex].luma_offset <<= (sps->bit_depth_luma - 8);
            }
            else
            {
                pPredWeight_L1[refindex].luma_weight = (int8_t)(1 << hdr->luma_log2_weight_denom);
                pPredWeight_L1[refindex].luma_offset = 0;
            }

            pPredWeight_L1[refindex].chroma_weight_flag = 0;
            if (sps->chroma_format_idc != 0)
                pPredWeight_L1[refindex].chroma_weight_flag = Get1Bit();

            if (pPredWeight_L1[refindex].chroma_weight_flag)
            {
                pPredWeight_L1[refindex].chroma_weight[0] = (int8_t)GetVLCElement(true);
                pPredWeight_L1[refindex].chroma_offset[0] = (int8_t)GetVLCElement(true);
                pPredWeight_L1[refindex].chroma_weight[1] = (int8_t)GetVLCElement(true);
                pPredWeight_L1[refindex].chroma_offset[1] = (int8_t)GetVLCElement(true);

                pPredWeight_L1[refindex].chroma_offset[0] <<= (sps->bit_depth_chroma - 8);
                pPredWeight_L1[refindex].chroma_offset[1] <<= (sps->bit_depth_chroma - 8);
            }
            else
            {
                pPredWeight_L1[refindex].chroma_weight[0] = (int8_t)(1 << hdr->chroma_log2_weight_denom);
                pPredWeight_L1[refindex].chroma_weight[1] = (int8_t)(1 << hdr->chroma_log2_weight_denom);
                pPredWeight_L1[refindex].chroma_offset[0] = 0;
                pPredWeight_L1[refindex].chroma_offset[1] = 0;
            }
        }
    }    // B slice

    return UMC_OK;
}

Status H264HeadersBitstream::GetSliceHeaderPart3(
    H264SliceHeader *hdr,        // slice header read goes here
    PredWeightTable *pPredWeight_L0, // L0 weight table goes here
    PredWeightTable *pPredWeight_L1, // L1 weight table goes here
    RefPicListReorderInfo *pReorderInfo_L0,
    RefPicListReorderInfo *pReorderInfo_L1,
    AdaptiveMarkingInfo *pAdaptiveMarkingInfo,
    AdaptiveMarkingInfo *pBaseAdaptiveMarkingInfo,
    const H264PicParamSet *pps,
    const H264SeqParamSet *sps,
    const H264SeqParamSetSVCExtension *spsSvcExt)
{
    uint8_t ref_pic_list_reordering_flag_l0 = 0;
    uint8_t ref_pic_list_reordering_flag_l1 = 0;
    Status ps = UMC_OK;

    pReorderInfo_L0->num_entries = 0;
    pReorderInfo_L1->num_entries = 0;
    hdr->num_ref_idx_l0_active = pps->num_ref_idx_l0_active;
    hdr->num_ref_idx_l1_active = pps->num_ref_idx_l1_active;

    hdr->direct_spatial_mv_pred_flag = 1;

    if (!hdr->nal_ext.svc_extension_flag || hdr->nal_ext.svc.quality_id == 0)
    {
        if (BPREDSLICE == hdr->slice_type)
        {
            // direct mode prediction method
            hdr->direct_spatial_mv_pred_flag = Get1Bit();
        }

        if (PREDSLICE == hdr->slice_type ||
            S_PREDSLICE == hdr->slice_type ||
            BPREDSLICE == hdr->slice_type)
        {
            hdr->num_ref_idx_active_override_flag = Get1Bit();
            if (hdr->num_ref_idx_active_override_flag != 0)
            // ref idx active l0 and l1
            {
                hdr->num_ref_idx_l0_active = GetVLCElement(false) + 1;
                if (BPREDSLICE == hdr->slice_type)
                    hdr->num_ref_idx_l1_active = GetVLCElement(false) + 1;
            }
        }    // ref idx override

        if (hdr->num_ref_idx_l0_active < 0 || hdr->num_ref_idx_l0_active > (int32_t)MAX_NUM_REF_FRAMES ||
            hdr->num_ref_idx_l1_active < 0 || hdr->num_ref_idx_l1_active > (int32_t)MAX_NUM_REF_FRAMES)
            return UMC_ERR_INVALID_STREAM;

        if (hdr->slice_type != INTRASLICE && hdr->slice_type != S_INTRASLICE)
        {
            uint32_t reordering_of_pic_nums_idc;
            uint32_t reorder_idx;

            // Reference picture list reordering
            ref_pic_list_reordering_flag_l0 = Get1Bit();
            if (ref_pic_list_reordering_flag_l0)
            {
                reorder_idx = 0;
                reordering_of_pic_nums_idc = 0;

                // Get reorder idc,pic_num pairs until idc==3
                for (;;)
                {
                    reordering_of_pic_nums_idc = (uint8_t)GetVLCElement(false);
                    if (reordering_of_pic_nums_idc > 5)
                      return UMC_ERR_INVALID_STREAM;

                    if (reordering_of_pic_nums_idc == 3)
                        break;

                    if (reorder_idx >= MAX_NUM_REF_FRAMES)
                    {
                        return UMC_ERR_INVALID_STREAM;
                    }

                    pReorderInfo_L0->reordering_of_pic_nums_idc[reorder_idx] =
                                                (uint8_t)reordering_of_pic_nums_idc;
                    pReorderInfo_L0->reorder_value[reorder_idx]  =
                                                        GetVLCElement(false);
                  if (reordering_of_pic_nums_idc != 2)
                        // abs_diff_pic_num is coded minus 1
                        pReorderInfo_L0->reorder_value[reorder_idx]++;
                    reorder_idx++;
                }    // while

                pReorderInfo_L0->num_entries = reorder_idx;
            }    // L0 reordering info
            else
                pReorderInfo_L0->num_entries = 0;

            if (BPREDSLICE == hdr->slice_type)
            {
                ref_pic_list_reordering_flag_l1 = Get1Bit();
                if (ref_pic_list_reordering_flag_l1)
                {
                    // Get reorder idc,pic_num pairs until idc==3
                    reorder_idx = 0;
                    reordering_of_pic_nums_idc = 0;
                    for (;;)
                    {
                        reordering_of_pic_nums_idc = GetVLCElement(false);
                        if (reordering_of_pic_nums_idc > 5)
                            return UMC_ERR_INVALID_STREAM;

                        if (reordering_of_pic_nums_idc == 3)
                            break;

                        if (reorder_idx >= MAX_NUM_REF_FRAMES)
                        {
                            return UMC_ERR_INVALID_STREAM;
                        }

                        pReorderInfo_L1->reordering_of_pic_nums_idc[reorder_idx] =
                                                    (uint8_t)reordering_of_pic_nums_idc;
                        pReorderInfo_L1->reorder_value[reorder_idx]  =
                                                            GetVLCElement(false);
                        if (reordering_of_pic_nums_idc != 2)
                            // abs_diff_pic_num is coded minus 1
                            pReorderInfo_L1->reorder_value[reorder_idx]++;
                        reorder_idx++;
                    }    // while
                    pReorderInfo_L1->num_entries = reorder_idx;
                }    // L1 reordering info
                else
                    pReorderInfo_L1->num_entries = 0;
            }    // B slice
        }    // reordering info

        hdr->luma_log2_weight_denom = 0;
        hdr->chroma_log2_weight_denom = 0;

        // prediction weight table
        if ( (pps->weighted_pred_flag &&
              ((PREDSLICE == hdr->slice_type) || (S_PREDSLICE == hdr->slice_type))) ||
             ((pps->weighted_bipred_idc == 1) && (BPREDSLICE == hdr->slice_type)))
        {
            ps = GetPredWeightTable(hdr, sps, pPredWeight_L0, pPredWeight_L1);
            if (ps != UMC_OK)
                return ps;
        }

        // dec_ref_pic_marking
        pAdaptiveMarkingInfo->num_entries = 0;
        pBaseAdaptiveMarkingInfo->num_entries = 0;

        if (hdr->nal_ref_idc)
        {
            ps = DecRefPicMarking(hdr, pAdaptiveMarkingInfo);
            if (ps != UMC_OK)
            {
                return ps;
            }

            if (hdr->nal_unit_type == NAL_UT_CODED_SLICE_EXTENSION && hdr->nal_ext.svc_extension_flag &&
                spsSvcExt && !spsSvcExt->slice_header_restriction_flag)
            {
                hdr->nal_ext.svc.store_ref_base_pic_flag = Get1Bit();
                if ((hdr->nal_ext.svc.use_ref_base_pic_flag ||
                    hdr->nal_ext.svc.store_ref_base_pic_flag) &&
                    (!hdr->nal_ext.svc.idr_flag))
                {
                    ps = DecRefBasePicMarking(pBaseAdaptiveMarkingInfo, hdr->nal_ext.svc.adaptive_ref_base_pic_marking_mode_flag);
                    if (ps != UMC_OK)
                        return ps;
                }
            }
        }    // def_ref_pic_marking
    }

    if (pps->entropy_coding_mode == 1  &&    // CABAC
        (hdr->slice_type != INTRASLICE && hdr->slice_type != S_INTRASLICE))
        hdr->cabac_init_idc = GetVLCElement(false);
    else
        hdr->cabac_init_idc = 0;

    if (hdr->cabac_init_idc > 2)
        return UMC_ERR_INVALID_STREAM;

    hdr->slice_qp_delta = GetVLCElement(true);

    if (S_PREDSLICE == hdr->slice_type ||
        S_INTRASLICE == hdr->slice_type)
    {
        if (S_PREDSLICE == hdr->slice_type)
            hdr->sp_for_switch_flag = Get1Bit();
        hdr->slice_qs_delta = GetVLCElement(true);
    }

    if (pps->deblocking_filter_variables_present_flag)
    {
        // deblock filter flag and offsets
        hdr->disable_deblocking_filter_idc = GetVLCElement(false);
        if (hdr->disable_deblocking_filter_idc > DEBLOCK_FILTER_ON_NO_SLICE_EDGES)
            return UMC_ERR_INVALID_STREAM;

        if (hdr->disable_deblocking_filter_idc != DEBLOCK_FILTER_OFF)
        {
            hdr->slice_alpha_c0_offset = GetVLCElement(true)<<1;
            hdr->slice_beta_offset = GetVLCElement(true)<<1;

            if (hdr->slice_alpha_c0_offset < -12 || hdr->slice_alpha_c0_offset > 12)
            {
                return UMC_ERR_INVALID_STREAM;
            }

            if (hdr->slice_beta_offset < -12 || hdr->slice_beta_offset > 12)
            {
                return UMC_ERR_INVALID_STREAM;
            }
        }
        else
        {
            // set filter offsets to max values to disable filter
            hdr->slice_alpha_c0_offset = (int8_t)(0 - QP_MAX);
            hdr->slice_beta_offset = (int8_t)(0 - QP_MAX);
        }
    }

    if (pps->num_slice_groups > 1 &&
        pps->SliceGroupInfo.slice_group_map_type >= 3 &&
        pps->SliceGroupInfo.slice_group_map_type <= 5)
    {
        uint32_t num_bits;    // number of bits used to code slice_group_change_cycle
        uint32_t val;
        uint32_t pic_size_in_map_units;
        uint32_t max_slice_group_change_cycle=0;

        // num_bits is Ceil(log2(picsizeinmapunits/slicegroupchangerate + 1))
        pic_size_in_map_units = sps->frame_width_in_mbs * sps->frame_height_in_mbs;
            // TBD: change above to support fields

        max_slice_group_change_cycle = pic_size_in_map_units /
                        pps->SliceGroupInfo.t2.slice_group_change_rate;
        if (pic_size_in_map_units %
                        pps->SliceGroupInfo.t2.slice_group_change_rate)
            max_slice_group_change_cycle++;

        val = max_slice_group_change_cycle;// + 1;
        num_bits = 0;
        while (val)
        {
            num_bits++;
            val >>= 1;
        }
        hdr->slice_group_change_cycle = GetBits(num_bits);
        if (hdr->slice_group_change_cycle > max_slice_group_change_cycle)
        {
            //return UMC_ERR_INVALID_STREAM; don't see any reasons for that
        }
    }

    CheckBSLeft();
    return UMC_OK;
} // GetSliceHeaderPart3()

Status H264HeadersBitstream::DecRefPicMarking(H264SliceHeader *hdr,        // slice header read goes here
                    AdaptiveMarkingInfo *pAdaptiveMarkingInfo)
{
    if (hdr->IdrPicFlag)
    {
        hdr->no_output_of_prior_pics_flag = Get1Bit();
        hdr->long_term_reference_flag = Get1Bit();
    }
    else
    {
        return DecRefBasePicMarking(pAdaptiveMarkingInfo, hdr->adaptive_ref_pic_marking_mode_flag);
    }

    return UMC_OK;
}

Status H264HeadersBitstream::GetSliceHeaderPart4(H264SliceHeader *hdr,
                                          const H264SeqParamSetSVCExtension *)
{
    hdr->scan_idx_start = 0;
    hdr->scan_idx_end = 15;
    return UMC_OK;
}// GetSliceHeaderPart4()

void H264HeadersBitstream::GetScalingList4x4(H264ScalingList4x4 *scl, uint8_t *def, uint8_t *scl_type)
{
    uint32_t lastScale = 8;
    uint32_t nextScale = 8;
    bool DefaultMatrix = false;
    int32_t j;

    for (j = 0; j < 16; j++ )
    {
        if (nextScale != 0)
        {
            int32_t delta_scale  = GetVLCElement(true);
            if (delta_scale < -128 || delta_scale > 127)
                throw h264_exception(UMC_ERR_INVALID_STREAM);
            nextScale = ( lastScale + delta_scale + 256 ) & 0xff;
            DefaultMatrix = ( j == 0 && nextScale == 0 );
        }
        scl->ScalingListCoeffs[ mp_scan4x4[0][j] ] = ( nextScale == 0 ) ? (uint8_t)lastScale : (uint8_t)nextScale;
        lastScale = scl->ScalingListCoeffs[ mp_scan4x4[0][j] ];
    }
    if (!DefaultMatrix)
    {
        *scl_type=SCLREDEFINED;
        return;
    }
    *scl_type= SCLDEFAULT;
    FillScalingList4x4(scl,def);
    return;
}

void H264HeadersBitstream::GetScalingList8x8(H264ScalingList8x8 *scl, uint8_t *def, uint8_t *scl_type)
{
    uint32_t lastScale = 8;
    uint32_t nextScale = 8;
    bool DefaultMatrix=false;
    int32_t j;

    for (j = 0; j < 64; j++ )
    {
        if (nextScale != 0)
        {
            int32_t delta_scale  = GetVLCElement(true);
            if (delta_scale < -128 || delta_scale > 127)
                throw h264_exception(UMC_ERR_INVALID_STREAM);
            nextScale = ( lastScale + delta_scale + 256 ) & 0xff;
            DefaultMatrix = ( j == 0 && nextScale == 0 );
        }
        scl->ScalingListCoeffs[ hp_scan8x8[0][j] ] = ( nextScale == 0 ) ? (uint8_t)lastScale : (uint8_t)nextScale;
        lastScale = scl->ScalingListCoeffs[ hp_scan8x8[0][j] ];
    }
    if (!DefaultMatrix)
    {
        *scl_type=SCLREDEFINED;
        return;
    }
    *scl_type= SCLDEFAULT;
    FillScalingList8x8(scl,def);
    return;

}

Status H264HeadersBitstream::GetNALUnitType(NAL_Unit_Type &nal_unit_type, uint32_t &nal_ref_idc)
{
    uint32_t code;

    CheckBitsLeft(8);
    h264GetBits(m_pbs, m_bitOffset, 8, code);

    nal_ref_idc = (uint32_t) ((code & NAL_STORAGE_IDC_BITS)>>5);
    nal_unit_type = (NAL_Unit_Type) (code & NAL_UNITTYPE_BITS);
    return UMC_OK;

}

const H264SeqParamSet *GetSeqParams(const Headers & headers, int32_t seq_parameter_set_id)
{
    if (seq_parameter_set_id == -1)
        return 0;

    const H264SeqParamSet *csps = headers.m_SeqParams.GetHeader(seq_parameter_set_id);
    if (csps)
        return csps;

    csps = headers.m_SeqParamsMvcExt.GetHeader(seq_parameter_set_id);
    if (csps)
        return csps;

    csps = headers.m_SeqParamsSvcExt.GetHeader(seq_parameter_set_id);
    if (csps)
        return csps;

    return 0;
}

int32_t H264HeadersBitstream::ParseSEI(const Headers & headers, H264SEIPayLoad *spl)
{
    int32_t current_sps = headers.m_SeqParams.GetCurrentID();
    return sei_message(headers, current_sps, spl);
}

int32_t H264HeadersBitstream::sei_message(const Headers & headers, int32_t current_sps, H264SEIPayLoad *spl)
{
    uint32_t code;
    int32_t payloadType = 0;

    CheckBitsLeft(8);
    PeakNextBits(m_pbs, m_bitOffset, 8, code);
    while (code  ==  0xFF)
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        CheckBitsLeft(16);
        h264GetBits(m_pbs, m_bitOffset, 8, code);
        payloadType += 255;
        PeakNextBits(m_pbs, m_bitOffset, 8, code);
    }

    int32_t last_payload_type_byte;    //uint32_t integer using 8 bits
    CheckBitsLeft(8);
    h264GetBits(m_pbs, m_bitOffset, 8, last_payload_type_byte);

    payloadType += last_payload_type_byte;

    int32_t payloadSize = 0;

    CheckBitsLeft(8);
    PeakNextBits(m_pbs, m_bitOffset, 8, code);
    while( code  ==  0xFF )
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        CheckBitsLeft(16);
        h264GetBits(m_pbs, m_bitOffset, 8, code);
        payloadSize += 255;
        PeakNextBits(m_pbs, m_bitOffset, 8, code);
    }

    int32_t last_payload_size_byte;    //uint32_t integer using 8 bits

    CheckBitsLeft(8);
    h264GetBits(m_pbs, m_bitOffset, 8, last_payload_size_byte);
    payloadSize += last_payload_size_byte;
    spl->Reset();
    spl->payLoadSize = payloadSize;

    if (payloadType < 0 || payloadType > SEI_RESERVED)
        payloadType = SEI_RESERVED;

    spl->payLoadType = (SEI_TYPE)payloadType;

    if (spl->payLoadSize > BytesLeft())
    {
        throw h264_exception(UMC_ERR_INVALID_STREAM);
    }

    uint32_t * pbs;
    uint32_t bitOffsetU;
    int32_t bitOffset;

    GetState(&pbs, &bitOffsetU);
    bitOffset = bitOffsetU;

    CheckBSLeft(spl->payLoadSize);

    spl->isValid = 1;
    int32_t ret = sei_payload(headers, current_sps, spl);

    for (uint32_t i = 0; i < spl->payLoadSize; i++)
    {
        SkipNBits(pbs, bitOffset, 8);
    }

    SetState(pbs, bitOffset);
    return ret;
}

int32_t H264HeadersBitstream::sei_payload(const Headers & headers, int32_t current_sps,H264SEIPayLoad *spl)
{
    uint32_t payloadType =spl->payLoadType;
    switch( payloadType)
    {
    case SEI_BUFFERING_PERIOD_TYPE:
        buffering_period(headers, current_sps,spl);
        break;
    case SEI_PIC_TIMING_TYPE:
        pic_timing(headers,current_sps,spl);
        break;
    case SEI_DEC_REF_PIC_MARKING_TYPE:
        dec_ref_pic_marking_repetition(headers,current_sps,spl);
        break;

    case SEI_USER_DATA_REGISTERED_TYPE:
        user_data_registered_itu_t_t35(spl);
        break;
    case SEI_RECOVERY_POINT_TYPE:
        recovery_point(spl);
        break;

    case SEI_SCALABILITY_INFO:
        scalability_info(spl);
        break;
    case SEI_SCALABLE_NESTING:
    case SEI_USER_DATA_UNREGISTERED_TYPE:
    case SEI_PAN_SCAN_RECT_TYPE:
    case SEI_FILLER_TYPE:
    case SEI_SPARE_PIC_TYPE:
    case SEI_SCENE_INFO_TYPE:
    case SEI_SUB_SEQ_INFO_TYPE:
    case SEI_SUB_SEQ_LAYER_TYPE:
    case SEI_SUB_SEQ_TYPE:
    case SEI_FULL_FRAME_FREEZE_TYPE:
    case SEI_FULL_FRAME_FREEZE_RELEASE_TYPE:
    case SEI_FULL_FRAME_SNAPSHOT_TYPE:
    case SEI_PROGRESSIVE_REF_SEGMENT_START_TYPE:
    case SEI_PROGRESSIVE_REF_SEGMENT_END_TYPE:
    case SEI_MOTION_CONSTRAINED_SG_SET_TYPE:
    default:
        unparsed_sei_message(spl);
        break;
    }

    return current_sps;
}

int32_t H264HeadersBitstream::buffering_period(const Headers & headers, int32_t , H264SEIPayLoad *spl)
{
    int32_t seq_parameter_set_id = (uint8_t) GetVLCElement(false);
    const H264SeqParamSet *csps = GetSeqParams(headers, seq_parameter_set_id);
    H264SEIPayLoad::SEIMessages::BufferingPeriod &bps = spl->SEI_messages.buffering_period;

    // touch unreferenced parameters
    if (!csps)
    {
        throw h264_exception(UMC_ERR_INVALID_STREAM);
    }

    if (csps->vui.nal_hrd_parameters_present_flag)
    {
        if (csps->vui.cpb_cnt >= 32)
            throw h264_exception(UMC_ERR_INVALID_STREAM);

        for(int32_t i=0; i < csps->vui.cpb_cnt; i++)
        {
            bps.initial_cpb_removal_delay[0][i] = GetBits(csps->vui.initial_cpb_removal_delay_length);
            bps.initial_cpb_removal_delay_offset[0][i] = GetBits(csps->vui.initial_cpb_removal_delay_length);
        }
    }

    if (csps->vui.vcl_hrd_parameters_present_flag)
    {
        if (csps->vui.cpb_cnt >= 32)
            throw h264_exception(UMC_ERR_INVALID_STREAM);

        for(int32_t i=0; i < csps->vui.cpb_cnt; i++)
        {
            bps.initial_cpb_removal_delay[1][i] = GetBits(csps->vui.cpb_removal_delay_length);
            bps.initial_cpb_removal_delay_offset[1][i] = GetBits(csps->vui.cpb_removal_delay_length);
        }
    }

    AlignPointerRight();
    return seq_parameter_set_id;
}

int32_t H264HeadersBitstream::pic_timing(const Headers & headers, int32_t current_sps, H264SEIPayLoad *spl)
{
    if (current_sps == -1)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    const uint8_t NumClockTS[]={1,1,1,2,2,3,3,2,3};
    const H264SeqParamSet *csps = GetSeqParams(headers, current_sps);
    H264SEIPayLoad::SEIMessages::PicTiming &pts = spl->SEI_messages.pic_timing;

    if (!csps)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    if (csps->vui.nal_hrd_parameters_present_flag || csps->vui.vcl_hrd_parameters_present_flag)
    {
        pts.cbp_removal_delay = GetBits(csps->vui.cpb_removal_delay_length);
        pts.dpb_output_delay = GetBits(csps->vui.dpb_output_delay_length);
    }
    else
    {
        pts.cbp_removal_delay = INVALID_DPB_OUTPUT_DELAY;
        pts.dpb_output_delay = INVALID_DPB_OUTPUT_DELAY;
    }

    if (csps->vui.pic_struct_present_flag)
    {
        uint8_t picStruct = (uint8_t)GetBits(4);

        if (picStruct > 8)
            return UMC_ERR_INVALID_STREAM;

        pts.pic_struct = (DisplayPictureStruct)picStruct;

        for (int32_t i = 0; i < NumClockTS[pts.pic_struct]; i++)
        {
            pts.clock_timestamp_flag[i] = (uint8_t)Get1Bit();
            if (pts.clock_timestamp_flag[i])
            {
                pts.clock_timestamps[i].ct_type = (uint8_t)GetBits(2);
                pts.clock_timestamps[i].nunit_field_based_flag = (uint8_t)Get1Bit();
                pts.clock_timestamps[i].counting_type = (uint8_t)GetBits(5);
                pts.clock_timestamps[i].full_timestamp_flag = (uint8_t)Get1Bit();
                pts.clock_timestamps[i].discontinuity_flag = (uint8_t)Get1Bit();
                pts.clock_timestamps[i].cnt_dropped_flag = (uint8_t)Get1Bit();
                pts.clock_timestamps[i].n_frames = (uint8_t)GetBits(8);

                if (pts.clock_timestamps[i].full_timestamp_flag)
                {
                    pts.clock_timestamps[i].seconds_value = (uint8_t)GetBits(6);
                    pts.clock_timestamps[i].minutes_value = (uint8_t)GetBits(6);
                    pts.clock_timestamps[i].hours_value = (uint8_t)GetBits(5);
                }
                else
                {
                    if (Get1Bit())
                    {
                        pts.clock_timestamps[i].seconds_value = (uint8_t)GetBits(6);
                        if (Get1Bit())
                        {
                            pts.clock_timestamps[i].minutes_value = (uint8_t)GetBits(6);
                            if (Get1Bit())
                            {
                                pts.clock_timestamps[i].hours_value = (uint8_t)GetBits(5);
                            }
                        }
                    }
                }

                if (csps->vui.time_offset_length > 0)
                    pts.clock_timestamps[i].time_offset = (uint8_t)GetBits(csps->vui.time_offset_length);
            }
        }
    }

    AlignPointerRight();
    return current_sps;
}

void H264HeadersBitstream::user_data_registered_itu_t_t35(H264SEIPayLoad *spl)
{
    H264SEIPayLoad::SEIMessages::UserDataRegistered * user_data = &(spl->SEI_messages.user_data_registered);
    uint32_t code;
    h264GetBits(m_pbs, m_bitOffset, 8, code);
    user_data->itu_t_t35_country_code = (uint8_t)code;

    uint32_t i = 1;

    user_data->itu_t_t35_country_code_extension_byte = 0;
    if (user_data->itu_t_t35_country_code == 0xff)
    {
        h264GetBits(m_pbs, m_bitOffset, 8, code);
        user_data->itu_t_t35_country_code_extension_byte = (uint8_t)code;
        i++;
    }

    spl->user_data.resize(spl->payLoadSize + 1);

    for(int32_t k = 0; i < spl->payLoadSize; i++, k++)
    {
        h264GetBits(m_pbs, m_bitOffset, 8, code);
        spl->user_data[k] = (uint8_t) code;
    }
}

void H264HeadersBitstream::recovery_point(H264SEIPayLoad *spl)
{
    H264SEIPayLoad::SEIMessages::RecoveryPoint * recPoint = &(spl->SEI_messages.recovery_point);

    recPoint->recovery_frame_cnt = (uint8_t)GetVLCElement(false);

    recPoint->exact_match_flag = (uint8_t)Get1Bit();
    recPoint->broken_link_flag = (uint8_t)Get1Bit();
    recPoint->changing_slice_group_idc = (uint8_t)GetBits(2);

    if (recPoint->changing_slice_group_idc > 2)
    {
        spl->isValid = 0;
    }
}

int32_t H264HeadersBitstream::dec_ref_pic_marking_repetition(const Headers & headers, int32_t current_sps, H264SEIPayLoad *spl)
{
    if (current_sps == -1)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    const H264SeqParamSet *csps = GetSeqParams(headers, current_sps);
    if (!csps)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    spl->SEI_messages.dec_ref_pic_marking_repetition.original_idr_flag = (uint8_t)Get1Bit();
    spl->SEI_messages.dec_ref_pic_marking_repetition.original_frame_num = (uint8_t)GetVLCElement(false);

    if (!csps->frame_mbs_only_flag)
    {
        spl->SEI_messages.dec_ref_pic_marking_repetition.original_field_pic_flag = (uint8_t)Get1Bit();

        if (spl->SEI_messages.dec_ref_pic_marking_repetition.original_field_pic_flag)
        {
            spl->SEI_messages.dec_ref_pic_marking_repetition.original_bottom_field_flag = (uint8_t)Get1Bit();
        }
    }

    H264SliceHeader hdr;
    memset(&hdr, 0, sizeof(H264SliceHeader));

    hdr.IdrPicFlag = spl->SEI_messages.dec_ref_pic_marking_repetition.original_idr_flag;

    Status sts = DecRefPicMarking(&hdr, &spl->SEI_messages.dec_ref_pic_marking_repetition.adaptiveMarkingInfo);
    if (sts != UMC_OK)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    spl->SEI_messages.dec_ref_pic_marking_repetition.long_term_reference_flag = hdr.long_term_reference_flag;
    return current_sps;
}

void H264HeadersBitstream::unparsed_sei_message(H264SEIPayLoad *spl)
{
    for(uint32_t i = 0; i < spl->payLoadSize; i++)
        SkipNBits(m_pbs, m_bitOffset, 8)
    AlignPointerRight();
}

#ifdef _MSVC_LANG
#pragma warning(disable : 4189)
#endif
void H264HeadersBitstream::scalability_info(H264SEIPayLoad *spl)
{
    spl->SEI_messages.scalability_info.temporal_id_nesting_flag = (uint8_t)Get1Bit();
    spl->SEI_messages.scalability_info.priority_layer_info_present_flag = (uint8_t)Get1Bit();
    spl->SEI_messages.scalability_info.priority_id_setting_flag = (uint8_t)Get1Bit();

    spl->SEI_messages.scalability_info.num_layers = GetVLCElement(false) + 1;

    if (spl->SEI_messages.scalability_info.num_layers > 1024)
        throw h264_exception(UMC_ERR_INVALID_STREAM);

    spl->user_data.resize(sizeof(scalability_layer_info) * spl->SEI_messages.scalability_info.num_layers);
    scalability_layer_info * layers = (scalability_layer_info *) (&spl->user_data[0]);

    for (uint32_t i = 0; i < spl->SEI_messages.scalability_info.num_layers; i++)
    {
        layers[i].layer_id = GetVLCElement(false);
        layers[i].priority_id = (uint8_t)GetBits(6);
        layers[i].discardable_flag = (uint8_t)Get1Bit();
        layers[i].dependency_id = (uint8_t)GetBits(3);
        layers[i].quality_id = (uint8_t)GetBits(4);
        layers[i].temporal_id = (uint8_t)GetBits(3);

        uint8_t   sub_pic_layer_flag = (uint8_t)Get1Bit();  // Need to check
        uint8_t   sub_region_layer_flag = (uint8_t)Get1Bit(); // Need to check

        //if (sub_pic_layer_flag && !sub_region_layer_flag)
          //  throw h264_exception(UMC_ERR_INVALID_STREAM);

        uint8_t   iroi_division_info_present_flag = (uint8_t)Get1Bit(); // Need to check

        //if (sub_pic_layer_flag && iroi_division_info_present_flag)
          //  throw h264_exception(UMC_ERR_INVALID_STREAM);

        uint8_t   profile_level_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   bitrate_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   frm_rate_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   frm_size_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   layer_dependency_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   parameter_sets_info_present_flag = (uint8_t)Get1Bit();
        uint8_t   bitstream_restriction_info_present_flag = (uint8_t)Get1Bit();
        /*uint8_t   exact_inter_layer_pred_flag = (uint8_t)*/Get1Bit();

        if (sub_pic_layer_flag || iroi_division_info_present_flag)
        {
            /*uint8_t   exact_sample_value_match_flag = (uint8_t)*/Get1Bit();
        }

        uint8_t   layer_conversion_flag = (uint8_t)Get1Bit();
        /*uint8_t   layer_output_flag = (uint8_t)*/Get1Bit();

        if (profile_level_info_present_flag)
        {
            /*uint32_t   layer_profile_level_idc = (uint8_t)*/GetBits(24);
        }

        if (bitrate_info_present_flag)
        {
            /*uint32_t  avg_bitrate = */GetBits(16);
            /*uint32_t  max_bitrate_layer = */GetBits(16);
            /*uint32_t  max_bitrate_layer_representation = */GetBits(16);
            /*uint32_t  max_bitrate_calc_window = */GetBits(16);
        }

        if (frm_rate_info_present_flag)
        {
            layers[i].constant_frm_rate_idc = GetBits(2);
            layers[i].avg_frm_rate = GetBits(16);
        }

        if (frm_size_info_present_flag || iroi_division_info_present_flag)
        {
            layers[i].frm_width_in_mbs = GetVLCElement(false) + 1;
            layers[i].frm_height_in_mbs = GetVLCElement(false) + 1;
        }

        if (sub_region_layer_flag)
        {
            /*uint32_t base_region_layer_id = */GetVLCElement(false);
            uint8_t dynamic_rect_flag = (uint8_t)Get1Bit();
            if (!dynamic_rect_flag)
            {
                /*uint32_t horizontal_offset = */GetBits(16);
                /*uint32_t vertical_offset = */GetBits(16);
                /*uint32_t region_width = */GetBits(16);
                /*uint32_t region_height = */GetBits(16);
            }
        }

        if(sub_pic_layer_flag)
        {
            /*uint32_t roi_id = */GetVLCElement(false);
        }

        if (iroi_division_info_present_flag)
        {
            uint8_t iroi_grid_flag = (uint8_t)Get1Bit();
            if (iroi_grid_flag)
            {
                /*uint32_t grid_width_in_mbs_minus1 = */GetVLCElement(false);
                /*uint32_t grid_height_in_mbs_minus1 = */GetVLCElement(false);
            } else {
                int32_t num_rois_minus1 = GetVLCElement(false);
                int32_t FrmSizeInMbs = layers[i].frm_height_in_mbs * layers[i].frm_width_in_mbs;

                if (num_rois_minus1 < 0 || num_rois_minus1 > FrmSizeInMbs)
                    throw h264_exception(UMC_ERR_INVALID_STREAM);
                for (int32_t j = 0; j <= num_rois_minus1; j++)
                {
                    /*uint32_t first_mb_in_roi = */GetVLCElement(false);
                    /*uint32_t roi_width_in_mbs_minus1 = */GetVLCElement(false);
                    /*uint32_t roi_height_in_mbs_minus1 = */GetVLCElement(false);
                }
            }
        }

        if (layer_dependency_info_present_flag)
        {
            layers[i].num_directly_dependent_layers = GetVLCElement(false);

            if (layers[i].num_directly_dependent_layers > 255)
                throw h264_exception(UMC_ERR_INVALID_STREAM);

            for(uint32_t j = 0; j < layers[i].num_directly_dependent_layers; j++)
            {
                layers[i].directly_dependent_layer_id_delta_minus1[j] = GetVLCElement(false);
            }
        }
        else
        {
            layers[i].layer_dependency_info_src_layer_id_delta = GetVLCElement(false);
        }

        if (parameter_sets_info_present_flag)
        {
            int32_t num_seq_parameter_set_minus1 = GetVLCElement(false);
            if (num_seq_parameter_set_minus1 < 0 || num_seq_parameter_set_minus1 > 32)
                throw h264_exception(UMC_ERR_INVALID_STREAM);

            for(int32_t j = 0; j <= num_seq_parameter_set_minus1; j++ )
                /*uint32_t seq_parameter_set_id_delta = */GetVLCElement(false);

            int32_t num_subset_seq_parameter_set_minus1 = GetVLCElement(false);
            if (num_subset_seq_parameter_set_minus1 < 0 || num_subset_seq_parameter_set_minus1 > 32)
                throw h264_exception(UMC_ERR_INVALID_STREAM);

            for(int32_t j = 0; j <= num_subset_seq_parameter_set_minus1; j++ )
                /*uint32_t subset_seq_parameter_set_id_delta = */GetVLCElement(false);

            int32_t num_pic_parameter_set_minus1 = GetVLCElement(false);
            if (num_pic_parameter_set_minus1 < 0 || num_pic_parameter_set_minus1 > 255)
                throw h264_exception(UMC_ERR_INVALID_STREAM);

            for(int32_t j = 0; j <= num_pic_parameter_set_minus1; j++)
                /*uint32_t pic_parameter_set_id_delta = */GetVLCElement(false);
        }
        else
        {
            /*uint32_t parameter_sets_info_src_layer_id_delta = */GetVLCElement(false);
        }

        if (bitstream_restriction_info_present_flag)
        {
            /*uint8_t motion_vectors_over_pic_boundaries_flag = (uint8_t)*/Get1Bit();
            /*uint32_t max_bytes_per_pic_denom = */GetVLCElement(false);
            /*uint32_t max_bits_per_mb_denom = */GetVLCElement(false);
            /*uint32_t log2_max_mv_length_horizontal = */GetVLCElement(false);
            /*uint32_t log2_max_mv_length_vertical = */GetVLCElement(false);
            /*uint32_t num_reorder_frames = */GetVLCElement(false);
            /*uint32_t max_dec_frame_buffering = */GetVLCElement(false);
        }

        if (layer_conversion_flag)
        {
            /*uint32_t conversion_type_idc = */GetVLCElement(false);
            for(uint32_t j = 0; j < 2; j++)
            {
                uint8_t rewriting_info_flag = (uint8_t)Get1Bit();
                if (rewriting_info_flag)
                {
                    /*uint32_t rewriting_profile_level_idc = */GetBits(24);
                    /*uint32_t rewriting_avg_bitrate = */GetBits(16);
                    /*uint32_t rewriting_max_bitrate = */GetBits(16);
                }
            }
        }
    } // for 0..num_layers

    if (spl->SEI_messages.scalability_info.priority_layer_info_present_flag)
    {
        uint32_t pr_num_dId_minus1 = GetVLCElement(false);
        for(uint32_t i = 0; i <= pr_num_dId_minus1; i++)
        {
            /*uint32_t pr_dependency_id = */GetBits(3);
            int32_t pr_num_minus1 = GetVLCElement(false);

            if (pr_num_minus1 < 0 || pr_num_minus1 > 63)
                throw h264_exception(UMC_ERR_INVALID_STREAM);

            for(int32_t j = 0; j <= pr_num_minus1; j++)
            {
                /*uint32_t pr_id = */GetVLCElement(false);
                /*uint32_t pr_profile_level_idc = */GetBits(24);
                /*uint32_t pr_avg_bitrate = */GetBits(16);
                /*uint32_t pr_max_bitrate = */GetBits(16);
            }
        }
    }

    if (spl->SEI_messages.scalability_info.priority_id_setting_flag)
    {
        std::vector<char> priority_id_setting_uri; // it is string
        uint32_t PriorityIdSettingUriIdx = 0;
        do
        {
            priority_id_setting_uri.push_back(1);
            priority_id_setting_uri[PriorityIdSettingUriIdx] = (uint8_t)GetBits(8);
        } while (priority_id_setting_uri[PriorityIdSettingUriIdx++] != 0);
    }
}

#ifdef _MSVC_LANG
#pragma warning(default : 4189)
#endif
void SetDefaultScalingLists(H264SeqParamSet * sps)
{
    int32_t i;

    for (i = 0; i < 6; i += 1)
    {
        FillFlatScalingList4x4(&sps->ScalingLists4x4[i]);
    }
    for (i = 0; i < 2; i += 1)
    {
        FillFlatScalingList8x8(&sps->ScalingLists8x8[i]);
    }
}

} // namespace UMC


#endif // MFX_ENABLE_H264_VIDEO_DECODE
