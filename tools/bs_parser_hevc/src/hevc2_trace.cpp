// Copyright (c) 2018-2019 Intel Corporation
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

#include "hevc2_trace.h"
#include "hevc2_struct.h"
#include <stdio.h>

namespace BS_HEVC2
{

const char NaluTraceMap[64][16] =
{
    "TRAIL_N", "TRAIL_R", "TSA_N", "TSA_R",
    "STSA_N", "STSA_R", "RADL_N", "RADL_R",
    "RASL_N", "RASL_R", "RSV_VCL_N10", "RSV_VCL_R11",
    "RSV_VCL_N12", "RSV_VCL_R13", "RSV_VCL_N14", "RSV_VCL_R15",
    "BLA_W_LP", "BLA_W_RADL", "BLA_N_LP", "IDR_W_RADL",
    "IDR_N_LP", "CRA_NUT", "RSV_IRAP_VCL22", "RSV_IRAP_VCL23",
    "RSV_VCL24", "RSV_VCL25", "RSV_VCL26", "RSV_VCL27",
    "RSV_VCL28", "RSV_VCL29", "RSV_VCL30", "RSV_VCL31",
    "VPS_NUT", "SPS_NUT", "PPS_NUT", "AUD_NUT",
    "EOS_NUT", "EOB_NUT", "FD_NUT", "PREFIX_SEI_NUT",
    "SUFFIX_SEI_NUT", "RSV_NVCL41", "RSV_NVCL42", "RSV_NVCL43",
    "RSV_NVCL44", "RSV_NVCL45", "RSV_NVCL46", "RSV_NVCL47",
    "UNSPEC48", "UNSPEC49", "UNSPEC50", "UNSPEC51",
    "UNSPEC52", "UNSPEC53", "UNSPEC54", "UNSPEC55",
    "UNSPEC56", "UNSPEC57", "UNSPEC58", "UNSPEC59",
    "UNSPEC60", "UNSPEC61", "UNSPEC62", "UNSPEC63"
};

const char PicTypeTraceMap[3][4] =
{
    "I", "PI", "BPI"
};

const char ProfileTraceMap::map[11][12] =
{
    "UNKNOWN", "MAIN", "MAIN_10", "MAIN_SP",
    "REXT", "REXT_HT", "MAIN_SV", "MAIN_SC",
    "MAIN_3D", "SCC", "REXT_SC"
};

const char* ProfileTraceMap::operator[] (unsigned int i)
{
    return map[(i < (sizeof(map) / sizeof(map[0]))) ? i : 0];
}

const char ChromaFormatTraceMap[4][12] =
{
    "CHROMA_400", "CHROMA_420", "CHROMA_422", "CHROMA_444"
};

const char ARIdcTraceMap::map[19][16] =
{
    "Unspecified",
    "1:1", "12:11", "10:11", "16:11",
    "40:33", "24:11", "20:11", "32:11",
    "80:33", "18:11", "15:11", "64:33",
    "160:99", "4:3", "3:2", "2:1",
    "Reserved",
    "Extended_SAR"
};

const char* ARIdcTraceMap::operator[] (unsigned int i)
{
    if (i < 17)
        return map[i];
    if (i == Extended_SAR)
        return map[18];
    return map[17];
}

const char VideoFormatTraceMap[6][12] =
{
    "Component", "PAL", "NTSC", "SECAM", "MAC", "Unspecified"
};

const char* SEITypeTraceMap::operator[] (unsigned int i)
{
    switch (i)
    {
    case SEI_BUFFERING_PERIOD                    : return "SEI_BUFFERING_PERIOD";
    case SEI_PICTURE_TIMING                      : return "SEI_PICTURE_TIMING";
    case SEI_PAN_SCAN_RECT                       : return "SEI_PAN_SCAN_RECT";
    case SEI_FILLER_PAYLOAD                      : return "SEI_FILLER_PAYLOAD";
    case SEI_USER_DATA_REGISTERED_ITU_T_T35      : return "SEI_USER_DATA_REGISTERED_ITU_T_T35";
    case SEI_USER_DATA_UNREGISTERED              : return "SEI_USER_DATA_UNREGISTERED";
    case SEI_RECOVERY_POINT                      : return "SEI_RECOVERY_POINT";
    case SEI_SCENE_INFO                          : return "SEI_SCENE_INFO";
    case SEI_FULL_FRAME_SNAPSHOT                 : return "SEI_FULL_FRAME_SNAPSHOT";
    case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START: return "SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START";
    case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END  : return "SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END";
    case SEI_FILM_GRAIN_CHARACTERISTICS          : return "SEI_FILM_GRAIN_CHARACTERISTICS";
    case SEI_POST_FILTER_HINT                    : return "SEI_POST_FILTER_HINT";
    case SEI_TONE_MAPPING_INFO                   : return "SEI_TONE_MAPPING_INFO";
    case SEI_FRAME_PACKING                       : return "SEI_FRAME_PACKING";
    case SEI_DISPLAY_ORIENTATION                 : return "SEI_DISPLAY_ORIENTATION";
    case SEI_SOP_DESCRIPTION                     : return "SEI_SOP_DESCRIPTION";
    case SEI_ACTIVE_PARAMETER_SETS               : return "SEI_ACTIVE_PARAMETER_SETS";
    case SEI_DECODING_UNIT_INFO                  : return "SEI_DECODING_UNIT_INFO";
    case SEI_TEMPORAL_LEVEL0_INDEX               : return "SEI_TEMPORAL_LEVEL0_INDEX";
    case SEI_DECODED_PICTURE_HASH                : return "SEI_DECODED_PICTURE_HASH";
    case SEI_SCALABLE_NESTING                    : return "SEI_SCALABLE_NESTING";
    case SEI_REGION_REFRESH_INFO                 : return "SEI_REGION_REFRESH_INFO";
    case SEI_NO_DISPLAY                          : return "SEI_NO_DISPLAY";
    case SEI_TIME_CODE                           : return "SEI_TIME_CODE";
    case SEI_MASTERING_DISPLAY_COLOUR_VOLUME     : return "SEI_MASTERING_DISPLAY_COLOUR_VOLUME";
    case SEI_SEGM_RECT_FRAME_PACKING             : return "SEI_SEGM_RECT_FRAME_PACKING";
    case SEI_TEMP_MOTION_CONSTRAINED_TILE_SETS   : return "SEI_TEMP_MOTION_CONSTRAINED_TILE_SETS";
    case SEI_CHROMA_RESAMPLING_FILTER_HINT       : return "SEI_CHROMA_RESAMPLING_FILTER_HINT";
    case SEI_KNEE_FUNCTION_INFO                  : return "SEI_KNEE_FUNCTION_INFO";
    case SEI_COLOUR_REMAPPING_INFO               : return "SEI_COLOUR_REMAPPING_INFO";
    default                                  : return "Unknown";
    }
}

const char PicStructTraceMap[13][12] =
{
    "FRAME", "TOP", "BOT", "TOP_BOT",
    "BOT_TOP", "TOP_BOT_TOP", "BOT_TOP_BOT", "FRAME_x2",
    "FRAME_x3", "TOP_PREVBOT", "BOT_PREVTOP", "TOP_NEXTBOT",
    "BOT_NEXTTOP"
};

const char ScanTypeTraceMap[4][12] =
{
    "INTERLACED", "PROGRESSIVE", "UNKNOWN", "RESERVED"
};

const char SliceTypeTraceMap[4][2] =
{
    "B", "P", "I", "?"
};

const char* RPLTraceMap::operator[] (const RefPic& r)
{
#ifdef __GNUC__
    sprintf
#else
    sprintf_s
#endif
        (m_buf, "POC: %4d LT: %d Used: %d Lost: %d", r.POC, r.long_term, r.used, r.lost);
    return m_buf;
}

const char PredModeTraceMap[3][6] =
{
    "INTER", "INTRA", "SKIP"
};

const char PartModeTraceMap[8][12] =
{
    "PART_2Nx2N",
    "PART_2NxN",
    "PART_Nx2N",
    "PART_NxN",
    "PART_2NxnU",
    "PART_2NxnD",
    "PART_nLx2N",
    "PART_nRx2N"
};

const char IntraPredModeTraceMap[35][9] =
{
    "Planar", "DC",
    "225\xF8", "230.625\xF8", "236.25\xF8", "241.875\xF8", "247.5\xF8", "253.125\xF8", "258.75\xF8", "264.375\xF8",
    "270\xF8", "275.625\xF8", "281.25\xF8", "286.875\xF8", "292.5\xF8", "298.125\xF8", "303.75\xF8", "309.375\xF8",
    "315\xF8", "320.625\xF8", "326.25\xF8", "331.875\xF8", "337.5\xF8", "343.125\xF8", "348.75\xF8", "354.375\xF8",
      "0\xF8",   "5.625\xF8",  "11.25\xF8", " 16.875\xF8",  "22.5\xF8",  "28.125\xF8",  "33.75\xF8",  "39.375\xF8",
     "45\xF8"
};

const char SAOTypeTraceMap[3][5] =
{
    "N/A", "Band", "Edge"
};

const char EOClassTraceMap[4][5] =
{
    "0\xF8", "90\xF8", "135\xF8", "45\xF8"
};


};
