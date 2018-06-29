// Copyright (c) 2018 Intel Corporation
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

#include "hevc_cabac.h"

namespace BS_HEVC
{

const Bs8u CtxOffset[num_SE+1] = {
/*SAO_MERGE_LEFT_FLAG          */          0,
/*SAO_TYPE_IDX_LUMA            */          1,
/*SPLIT_CU_FLAG                */          2,
/*CU_TRANSQUANT_BYPASS_FLAG    */          5,
/*CU_SKIP_FLAG                 */          6,
/*PRED_MODE_FLAG               */          9,
/*PART_MODE                    */         10,
/*PREV_INTRA_LUMA_PRED_FLAG    */         14,
/*INTRA_CHROMA_PRED_MODE       */         15,
/*RQT_ROOT_CBF                 */         16,
/*MERGE_FLAG                   */         17,
/*MERGE_IDX                    */         18,
/*INTER_PRED_IDC               */         19,
/*REF_IDX_LX                   */         24,
/*MVP_LX_FLAG                  */         26,
/*SPLIT_TRANSFORM_FLAG         */         27,
/*CBF_LUMA                     */         30,
/*CBF_CX                       */         32,
/*ABS_MVD_GREATER0_FLAG        */         36 + 1,
/*ABS_MVD_GREATER1_FLAG        */         37 + 1,
/*CU_QP_DELTA_ABS              */         38 + 1,
/*TRANSFORM_SKIP_FLAG0         */         40 + 1,
/*TRANSFORM_SKIP_FLAG1         */         41 + 1,
/*LAST_SIG_COEFF_X_PREFIX      */         42 + 1,
/*LAST_SIG_COEFF_Y_PREFIX      */         60 + 1,
/*CODED_SUB_BLOCK_FLAG         */         78 + 1,
/*SIG_COEFF_FLAG               */         82 + 1,
/*COEFF_ABS_LEVEL_GREATER1_FLAG*/        124 + 1 + 2,
/*COEFF_ABS_LEVEL_GREATER2_FLAG*/        148 + 1 + 2,
/*CU_CHROMA_QP_OFFSET_FLAG     */        154 + 1 + 2,
/*CU_CHROMA_QP_OFFSET_IDX      */        155 + 1 + 2,
/*LOG2_RES_SCALE_ABS_PLUS1     */        156 + 1 + 2,
/*RES_SCALE_SIGN_FLAG          */        164 + 1 + 2,
/*EXPLICIT_RDPCM_FLAG          */        166 + 1 + 2,
/*EXPLICIT_RDPCM_DIR_FLAG      */        168 + 1 + 2,
/*PALETTE_MODE_FLAG                    */170 + 1 + 2,
/*TU_RESIDUAL_ACT_FLAG                 */171 + 1 + 2,
/*PALETTE_RUN_PREFIX                   */172 + 1 + 2,
/*COPY_ABOVE_PALETTE_INDICES_FLAG      */180 + 1 + 2,
/*COPY_ABOVE_INDICES_FOR_FINAL_RUN_FLAG*/180 + 1 + 2,
/*PALETTE_TRANSPOSE_FLAG               */181 + 1 + 2,
/*                             */ CtxTblSize
};

const Bs8u CtxInitTbl[3][CtxTblSize] =
{
    {
        12 + 141, 59 + 141, 88 + 51, 39 + 102, 99 + 58, 98 + 56, 140 + 14, 91 + 63, 36 + 118, 101 + 53,
        113 + 71, 145 + 9, 46 + 108, 80 + 74, 120 + 64, 40 + 23, 25 + 129, 128 + 26, 71 + 83, 141 + 13,
        29 + 125, 5 + 149, 118 + 36, 152 + 2, 17 + 137, 43 + 111, 85 + 69, 83 + 70, 114 + 24, 121 + 17,
        89 + 22, 111 + 30, 17 + 77, 9 + 129, 39 + 143, 33 + 121, 90 + 64, 140 + 14, 127 + 27, 126 + 28,
        88 + 66, 10 + 129, 61 + 78, 49 + 61, 30 + 80, 41 + 83, 61 + 64, 4 + 136, 116 + 37, 95 + 30,
        125 + 2, 6 + 134, 75 + 34, 31 + 80, 32 + 111, 67 + 60, 2 + 109, 62 + 17, 93 + 15, 57 + 66,
        31 + 32, 28 + 82, 99 + 11, 30 + 94, 1 + 124, 113 + 27, 26 + 127, 49 + 76, 108 + 19, 14 + 126,
        25 + 84, 67 + 44, 113 + 30, 41 + 86, 97 + 14, 23 + 56, 100 + 8, 103 + 20, 36 + 27, 9 + 82,
        109 + 62, 16 + 118, 92 + 49, 64 + 47, 70 + 41, 3 + 122, 85 + 25, 55 + 55, 74 + 20, 85 + 39,
        91 + 17, 55 + 69, 62 + 45, 58 + 67, 138 + 3, 76 + 103, 138 + 15, 73 + 52, 3 + 104, 14 + 111,
        129 + 12, 57 + 122, 11 + 142, 102 + 23, 21 + 86, 88 + 37, 15 + 126, 165 + 14, 148 + 5, 8 + 117,
        90 + 50, 32 + 107, 141 + 41, 122 + 60, 8 + 144, 63 + 73, 12 + 140, 20 + 116, 109 + 44, 119 + 17,
        86 + 53, 97 + 14, 107 + 29, 109 + 30, 105 + 6, 85 + 56, 106 + 5, 51 + 89, 10 + 82, 97 + 40,
        32 + 106, 137 + 3, 62 + 90, 22 + 116, 21 + 118, 73 + 80, 28 + 46, 116 + 33, 2 + 90, 32 + 107,
        57 + 50, 20 + 102, 28 + 124, 55 + 85, 66 + 113, 55 + 111, 51 + 131, 46 + 94, 79 + 148, 5 + 117,
        16 + 181, 79 + 59, 111 + 42, 54 + 82, 56 + 111, 120 + 32, 45 + 107, 5 + 149, 56 + 98, 128 + 26,
        125 + 29, 141 + 13, 142 + 12, 73 + 81, 62 + 92, 42 + 112, 125 + 29, 21 + 133, 98 + 56, 103 + 36,
        25 + 114, 34 + 105, 57 + 82, 76 + 78, 111 + 43, 27 + 127, 80 + 74, 135 + 19, 76 + 78, 121 + 33,
        4 + 150, 129 + 25, 141 + 13, 139 + 15, 38 + 116
    },
    {
        99 + 54, 181 + 4, 106 + 1, 3 + 136, 105 + 21, 133 + 21, 196 + 1, 92 + 93, 168 + 33, 5 + 144,
        143 + 11, 78 + 61, 46 + 108, 12 + 142, 110 + 44, 89 + 63, 5 + 74, 47 + 63, 74 + 48, 29 + 66,
        63 + 16, 21 + 42, 15 + 16, 15 + 16, 54 + 99, 89 + 64, 1 + 167, 102 + 22, 102 + 36, 11 + 83,
        65 + 88, 106 + 5, 94 + 55, 87 + 20, 81 + 86, 83 + 71, 132 + 22, 41 + 99, 10 + 188, 101 + 53,
        60 + 94, 95 + 44, 6 + 133, 34 + 91, 14 + 96, 13 + 81, 62 + 48, 59 + 36, 20 + 59, 124 + 1,
        93 + 18, 20 + 90, 36 + 42, 84 + 26, 33 + 78, 52 + 59, 82 + 13, 57 + 37, 31 + 77, 56 + 67,
        47 + 61, 124 + 1, 61 + 49, 27 + 67, 20 + 90, 0 + 95, 9 + 70, 51 + 74, 82 + 29, 103 + 7,
        74 + 4, 98 + 12, 30 + 81, 80 + 31, 92 + 3, 74 + 20, 91 + 17, 46 + 77, 28 + 80, 20 + 101,
        96 + 44, 17 + 44, 87 + 67, 5 + 150, 149 + 5, 138 + 1, 40 + 113, 115 + 24, 103 + 20, 45 + 78,
        59 + 4, 136 + 17, 7 + 159, 96 + 87, 124 + 16, 86 + 50, 5 + 148, 67 + 87, 71 + 95, 12 + 171,
        109 + 31, 92 + 44, 66 + 87, 80 + 74, 162 + 4, 102 + 81, 125 + 15, 98 + 38, 97 + 56, 115 + 39,
        35 + 135, 150 + 3, 44 + 79, 65 + 58, 55 + 52, 76 + 45, 71 + 36, 111 + 10, 105 + 62, 105 + 46,
        54 + 129, 75 + 65, 89 + 62, 143 + 40, 92 + 48, 105 + 35, 89 + 51, 17 + 137, 91 + 105, 89 + 107,
        45 + 122, 89 + 65, 118 + 34, 73 + 94, 116 + 66, 103 + 79, 27 + 107, 83 + 66, 87 + 49, 132 + 21,
        119 + 2, 89 + 47, 94 + 43, 40 + 129, 50 + 144, 86 + 80, 74 + 93, 96 + 58, 108 + 59, 56 + 81,
        133 + 49, 38 + 69, 22 + 145, 32 + 59, 16 + 106, 88 + 19, 96 + 71, 29 + 125, 151 + 3, 92 + 62,
        135 + 19, 58 + 96, 30 + 124, 117 + 37, 66 + 88, 50 + 104, 111 + 43, 27 + 127, 98 + 56, 100 + 39,
        111 + 28, 120 + 19, 50 + 89, 49 + 105, 59 + 95, 81 + 73, 137 + 17, 97 + 57, 24 + 130, 118 + 36,
        33 + 121, 132 + 22, 71 + 83, 47 + 107, 109 + 45
    },
    {
        61 + 92, 48 + 112, 89 + 18, 73 + 66, 12 + 114, 49 + 105, 129 + 68, 127 + 58, 81 + 120, 17 + 117,
        153 + 1, 133 + 6, 105 + 49, 27 + 127, 119 + 64, 64 + 88, 60 + 19, 34 + 120, 43 + 94, 35 + 60,
        55 + 24, 50 + 13, 19 + 12, 12 + 19, 73 + 80, 9 + 144, 18 + 150, 160 + 64, 39 + 128, 88 + 34,
        73 + 80, 98 + 13, 32 + 117, 34 + 58, 10 + 157, 25 + 129, 140 + 14, 100 + 69, 48 + 150, 68 + 86,
        50 + 104, 83 + 56, 38 + 101, 88 + 37, 53 + 57, 51 + 73, 30 + 80, 49 + 46, 36 + 58, 2 + 123,
        52 + 59, 90 + 21, 58 + 21, 124 + 1, 115 + 11, 108 + 3, 6 + 105, 36 + 43, 64 + 44, 16 + 107,
        80 + 13, 44 + 81, 59 + 51, 67 + 57, 29 + 81, 46 + 49, 10 + 84, 83 + 42, 88 + 23, 82 + 29,
        62 + 17, 6 + 119, 123 + 3, 10 + 101, 56 + 55, 44 + 35, 33 + 75, 73 + 50, 54 + 39, 79 + 42,
        135 + 5, 55 + 6, 83 + 71, 150 + 20, 54 + 100, 41 + 98, 111 + 42, 18 + 121, 39 + 84, 65 + 58,
        21 + 42, 31 + 93, 127 + 39, 114 + 69, 98 + 42, 33 + 103, 130 + 23, 121 + 33, 133 + 33, 104 + 79,
        131 + 9, 60 + 76, 131 + 22, 128 + 26, 135 + 31, 172 + 11, 5 + 135, 113 + 23, 102 + 51, 115 + 39,
        69 + 101, 0 + 153, 73 + 65, 23 + 115, 102 + 20, 17 + 104, 20 + 102, 60 + 61, 140 + 27, 7 + 144,
        27 + 156, 99 + 41, 122 + 29, 73 + 110, 85 + 55, 113 + 27, 93 + 47, 47 + 107, 165 + 31, 56 + 111,
        38 + 129, 87 + 67, 128 + 24, 9 + 158, 162 + 20, 136 + 46, 120 + 14, 62 + 87, 33 + 103, 6 + 147,
        0 + 121, 61 + 75, 9 + 113, 120 + 49, 169 + 39, 37 + 129, 22 + 145, 143 + 11, 146 + 6, 162 + 5,
        65 + 117, 17 + 90, 122 + 45, 80 + 11, 27 + 80, 51 + 56, 103 + 64, 21 + 133, 86 + 68, 26 + 128,
        105 + 49, 59 + 95, 115 + 39, 130 + 24, 150 + 4, 26 + 128, 99 + 55, 97 + 57, 44 + 110, 133 + 6,
        8 + 131, 133 + 6, 22 + 117, 116 + 38, 109 + 45, 16 + 138, 86 + 68, 53 + 101, 8 + 146, 72 + 82,
        15 + 139, 48 + 106, 6 + 148, 95 + 59, 2 + 152
    }
};

const Bs8s HEVC_CABAC::CtxIncTbl[num_SE_full][6] = {
/*SAO_MERGE_LEFT_FLAG          */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*SAO_TYPE_IDX_LUMA            */ {         0,   BYPASS,    ERROR,    ERROR,    ERROR,    ERROR },
/*SPLIT_CU_FLAG                */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_TRANSQUANT_BYPASS_FLAG    */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_SKIP_FLAG                 */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PRED_MODE_FLAG               */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PART_MODE                    */ {         0,        1, EXTERNAL,   BYPASS,    ERROR,    ERROR },
/*PREV_INTRA_LUMA_PRED_FLAG    */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*INTRA_CHROMA_PRED_MODE       */ {         0,   BYPASS,   BYPASS,    ERROR,    ERROR,    ERROR },
/*RQT_ROOT_CBF                 */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*MERGE_FLAG                   */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*MERGE_IDX                    */ {         0,   BYPASS,   BYPASS,   BYPASS,    ERROR,    ERROR },
/*INTER_PRED_IDC               */ {  EXTERNAL,        4,    ERROR,    ERROR,    ERROR,    ERROR },
/*REF_IDX_LX                   */ {         0,        1,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*MVP_LX_FLAG                  */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*SPLIT_TRANSFORM_FLAG         */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CBF_LUMA                     */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CBF_CX                       */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*ABS_MVD_GREATER0_FLAG        */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*ABS_MVD_GREATER1_FLAG        */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_QP_DELTA_ABS              */ {         0,        1,        1,        1,        1,   BYPASS },
/*TRANSFORM_SKIP_FLAG0         */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*TRANSFORM_SKIP_FLAG1         */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*LAST_SIG_COEFF_X_PREFIX      */ {  EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL },
/*LAST_SIG_COEFF_Y_PREFIX      */ {  EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL },
/*CODED_SUB_BLOCK_FLAG         */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*SIG_COEFF_FLAG               */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*COEFF_ABS_LEVEL_GREATER1_FLAG*/ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*COEFF_ABS_LEVEL_GREATER2_FLAG*/ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_QP_DELTA_ABS              */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_QP_DELTA_ABS              */ {         0,        0,        0,        0,        0,    ERROR },
/*LOG2_RES_SCALE_ABS_PLUS1     */ {  EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL,    ERROR,    ERROR },
/*RES_SCALE_SIGN_FLAG          */ {  EXTERNAL,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*EXPLICIT_RDPCM_FLAG          */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*EXPLICIT_RDPCM_DIR_FLAG      */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PALETTE_MODE_FLAG            */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*TU_RESIDUAL_ACT_FLAG         */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PALETTE_RUN_PREFIX           */ {  EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL, EXTERNAL },
/*COPY_ABOVE_PALETTE_INDICES_FL*/ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*COPY_ABOVE_INDICES_FOR_FINAL_*/ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PALETTE_TRANSPOSE_FLAG       */ {         0,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*END_OF_SLICE_SEGMENT_FLAG    */ { TERMINATE,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*END_OF_SUB_STREAM_ONE_BIT    */ { TERMINATE,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*SAO_OFFSET_ABS               */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*SAO_OFFSET_SIGN              */ {    BYPASS,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*SAO_BAND_POSITION            */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*SAO_EO_CLASS_LUMA            */ {    BYPASS,   BYPASS,   BYPASS,    ERROR,    ERROR,    ERROR },
/*MPM_IDX                      */ {    BYPASS,   BYPASS,    ERROR,    ERROR,    ERROR,    ERROR },
/*REM_INTRA_LUMA_PRED_MODE     */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*ABS_MVD_MINUS2               */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*MVD_SIGN_FLAG                */ {    BYPASS,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*CU_QP_DELTA_SIGN_FLAG        */ {    BYPASS,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*LAST_SIG_COEFF_X_SUFFIX      */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*LAST_SIG_COEFF_Y_SUFFIX      */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*COEFF_ABS_LEVEL_REMAINING    */ {    BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS,   BYPASS },
/*COEFF_SIGN_FLAG              */ {    BYPASS,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
/*PCM_FLAG                     */ { TERMINATE,    ERROR,    ERROR,    ERROR,    ERROR,    ERROR },
};

const Bs8u rangeTabLpsT[4][64] = {
    { 15 + 113, 100 + 28, 74 + 54, 6 + 117, 103 + 13, 57 + 54, 60 + 45, 24 + 76, 61 + 34, 81 + 9, 27 + 58, 1 + 80, 29 + 48, 29 + 44, 36 + 33, 62 + 4, 49 + 13, 52 + 7, 0 + 56, 24 + 29, 20 + 31, 4 + 44, 33 + 13, 2 + 41, 35 + 6, 14 + 25, 21 + 16, 20 + 15, 3 + 30, 24 + 8, 6 + 24, 1 + 28, 9 + 18, 10 + 16, 14 + 10, 8 + 15, 6 + 16, 8 + 13, 10 + 10, 7 + 12, 6 + 12, 16 + 1, 6 + 10, 10 + 5, 3 + 11, 2 + 12, 0 + 13, 3 + 9, 10 + 2, 7 + 4, 5 + 6, 7 + 3, 7 + 3, 8 + 1, 6 + 3, 0 + 8, 5 + 3, 2 + 5, 2 + 5, 4 + 3, 0 + 6, 4 + 2, 3 + 3, 0 + 2 },
    { 151 + 25, 125 + 42, 68 + 90, 50 + 100, 132 + 10, 77 + 58, 66 + 62, 94 + 28, 61 + 55, 42 + 68, 64 + 40, 7 + 92, 47 + 47, 52 + 37, 50 + 35, 72 + 8, 22 + 54, 13 + 59, 36 + 33, 31 + 34, 16 + 46, 57 + 2, 45 + 11, 39 + 14, 34 + 16, 30 + 18, 11 + 34, 7 + 36, 7 + 34, 25 + 14, 15 + 22, 11 + 24, 18 + 15, 18 + 13, 20 + 10, 23 + 5, 10 + 17, 4 + 22, 0 + 24, 20 + 3, 11 + 11, 6 + 15, 1 + 19, 4 + 15, 12 + 6, 6 + 11, 11 + 5, 5 + 10, 10 + 4, 2 + 12, 6 + 7, 4 + 8, 0 + 12, 9 + 2, 5 + 6, 1 + 9, 3 + 6, 3 + 6, 3 + 6, 4 + 4, 1 + 7, 6 + 1, 5 + 2, 0 + 2 },
    { 164 + 44, 117 + 80, 146 + 41, 92 + 86, 61 + 108, 136 + 24, 50 + 102, 111 + 33, 0 + 137, 105 + 25, 69 + 54, 75 + 42, 9 + 102, 68 + 37, 31 + 69, 65 + 30, 43 + 47, 62 + 24, 67 + 14, 34 + 43, 19 + 54, 22 + 47, 62 + 4, 46 + 17, 20 + 39, 9 + 47, 23 + 31, 40 + 11, 46 + 2, 8 + 38, 28 + 15, 2 + 39, 0 + 39, 4 + 33, 3 + 32, 31 + 2, 28 + 4, 8 + 22, 22 + 7, 25 + 2, 18 + 8, 23 + 2, 20 + 3, 20 + 2, 4 + 17, 8 + 12, 18 + 1, 8 + 10, 13 + 4, 13 + 3, 10 + 5, 5 + 10, 9 + 5, 2 + 11, 9 + 3, 1 + 11, 2 + 9, 8 + 3, 4 + 6, 4 + 6, 6 + 3, 4 + 5, 3 + 5, 1 + 1 },
    { 135 + 105, 102 + 125, 100 + 116, 145 + 60, 163 + 32, 157 + 28, 114 + 61, 24 + 142, 23 + 135, 63 + 87, 59 + 83, 99 + 36, 113 + 15, 25 + 97, 102 + 14, 60 + 50, 73 + 31, 79 + 20, 21 + 73, 0 + 89, 30 + 55, 32 + 48, 13 + 63, 65 + 7, 40 + 29, 20 + 45, 13 + 49, 51 + 8, 41 + 15, 33 + 20, 47 + 3, 1 + 47, 40 + 5, 25 + 18, 20 + 21, 38 + 1, 17 + 20, 2 + 33, 20 + 13, 14 + 17, 28 + 2, 11 + 17, 23 + 4, 23 + 2, 7 + 17, 13 + 10, 11 + 11, 7 + 14, 7 + 13, 7 + 12, 6 + 12, 1 + 16, 12 + 4, 7 + 8, 8 + 6, 0 + 14, 4 + 9, 6 + 6, 9 + 3, 5 + 6, 10 + 1, 5 + 5, 7 + 2, 0 + 2 },
};

const Bs8u transIdxLps[64] = {
    104 - 104, 62 - 62, 0 + 1, 1 + 1, 0 + 2, 0 + 4, 0 + 4, 2 + 3, 3 + 3, 5 + 2, 0 + 8, 3 + 6, 3 + 6, 0 + 11, 10 + 1, 8 + 4,
    3 + 10, 1 + 12, 4 + 11, 7 + 8, 1 + 15, 2 + 14, 1 + 17, 3 + 15, 10 + 9, 2 + 17, 3 + 18, 10 + 11, 2 + 20, 18 + 4, 10 + 13, 14 + 10,
    4 + 20, 3 + 22, 21 + 5, 15 + 11, 14 + 13, 18 + 9, 15 + 13, 10 + 19, 2 + 27, 6 + 24, 27 + 3, 18 + 12, 15 + 16, 9 + 23, 5 + 27, 9 + 24,
    16 + 17, 22 + 11, 14 + 20, 0 + 34, 5 + 30, 31 + 4, 19 + 16, 27 + 9, 13 + 23, 16 + 20, 23 + 14, 19 + 18, 23 + 14, 30 + 8, 0 + 38, 43 + 20
};

const Bs8u transIdxMps[64] = {
    0 + 1, 0 + 2, 0 + 3, 2 + 2, 2 + 3, 5 + 1, 2 + 5, 7 + 1, 0 + 9, 2 + 8, 9 + 2, 6 + 6, 8 + 5, 10 + 4, 12 + 3, 12 + 4,
    10 + 7, 9 + 9, 13 + 6, 13 + 7, 16 + 5, 15 + 7, 14 + 9, 1 + 23, 8 + 17, 10 + 16, 26 + 1, 16 + 12, 19 + 10, 7 + 23, 4 + 27, 27 + 5,
    6 + 27, 5 + 29, 16 + 19, 20 + 16, 34 + 3, 33 + 5, 1 + 38, 13 + 27, 13 + 28, 4 + 38, 20 + 23, 20 + 24, 15 + 30, 23 + 23, 17 + 30, 7 + 41,
    4 + 45, 4 + 46, 50 + 1, 3 + 49, 5 + 48, 27 + 27, 22 + 33, 35 + 21, 47 + 10, 31 + 27, 50 + 9, 5 + 55, 39 + 22, 0 + 62, 41 + 21, 6 + 57
};

};
