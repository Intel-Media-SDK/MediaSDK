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
#ifndef __MFXFEIHEVC_H__
#define __MFXFEIHEVC_H__
#include "mfxcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if (MFX_VERSION >= 1027)

MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer Header;

    mfxU16  SearchPath;
    mfxU16  LenSP;
    mfxU16  RefWidth;
    mfxU16  RefHeight;
    mfxU16  SearchWindow;

    mfxU16  NumMvPredictors[2]; /* 0 for L0 and 1 for L1 */
    mfxU16  MultiPred[2];       /* 0 for L0 and 1 for L1 */

    mfxU16  SubPelMode;
    mfxU16  AdaptiveSearch;
    mfxU16  MVPredictor;

    mfxU16  PerCuQp;
    mfxU16  PerCtuInput;
    mfxU16  ForceCtuSplit;
    mfxU16  NumFramePartitions;
    mfxU16  FastIntraMode;

    mfxU16  reserved0[107];
} mfxExtFeiHevcEncFrameCtrl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    struct {
        mfxU8   RefL0 : 4;
        mfxU8   RefL1 : 4;
    } RefIdx[4]; /* index is predictor number */

    mfxU32     BlockSize : 2;
    mfxU32     reserved0 : 30;

    mfxI16Pair MV[4][2]; /* first index is predictor number, second is 0 for L0 and 1 for L1 */
} mfxFeiHevcEncMVPredictors;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved0[54];

    mfxFeiHevcEncMVPredictors *Data;
} mfxExtFeiHevcEncMVPredictors;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer  Header;
    mfxU32        VaBufferID;
    mfxU32        Pitch;
    mfxU32        Height;
    mfxU16        reserved[6];

    mfxU8    *Data;
} mfxExtFeiHevcEncQP;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU32  ForceToIntra    : 1;
    mfxU32  ForceToInter    : 1;
    mfxU32  reserved0       : 30;

    mfxU32  reserved1[3];
} mfxFeiHevcEncCtuCtrl;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer Header;
    mfxU32  VaBufferID;
    mfxU32  Pitch;
    mfxU32  Height;
    mfxU16  reserved0[54];

    mfxFeiHevcEncCtuCtrl *Data;
} mfxExtFeiHevcEncCtuCtrl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer    Header;
    mfxU32      MaxFrameSize; /* in bytes */
    mfxU32      NumPasses;    /* up to 8 */
    mfxU16      reserved[8];
    mfxU8       DeltaQP[8];   /* list of delta QPs, only positive values */
} mfxExtFeiHevcRepackCtrl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct {
    mfxExtBuffer    Header;
    mfxU32          NumPasses;
    mfxU16          reserved[58];
} mfxExtFeiHevcRepackStat;
MFX_PACK_END()


enum {
    MFX_EXTBUFF_HEVCFEI_ENC_CTRL       = MFX_MAKEFOURCC('F','H','C','T'),
    MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED    = MFX_MAKEFOURCC('F','H','P','D'),
    MFX_EXTBUFF_HEVCFEI_ENC_QP         = MFX_MAKEFOURCC('F','H','Q','P'),
    MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL   = MFX_MAKEFOURCC('F','H','E','C'),
    MFX_EXTBUFF_HEVCFEI_REPACK_CTRL    = MFX_MAKEFOURCC('F','H','R','P'),
    MFX_EXTBUFF_HEVCFEI_REPACK_STAT    = MFX_MAKEFOURCC('F','H','R','S'),
};

#endif // MFX_VERSION

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif // __MFXFEIHEVC_H__
