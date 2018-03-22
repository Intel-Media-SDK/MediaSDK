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
#ifndef _MOTIONESTIMATIONENGINE_H_
#define _MOTIONESTIMATIONENGINE_H_

#include "asc_structures.h"
#include "asc.h"

namespace ns_asc {

void MotionRangeDeliveryF(mfxI16 xLoc, mfxI16 yLoc, mfxI16 *limitXleft, mfxI16 *limitXright, mfxI16 *limitYup, mfxI16 *limitYdown, ASCImDetails dataIn);

/* 4x4 Block size Functions */
bool MVcalcSAD4x4(ASCMVector MV, pmfxU8 curY, pmfxU8 refY, ASCImDetails dataIn, mfxU32 *bestSAD, mfxI32 *distance);
/* 8x8 Block size Functions */
bool MVcalcSAD8x8(ASCMVector MV, pmfxU8 curY, pmfxU8 refY, ASCImDetails *dataIn, mfxU32 *bestSAD, mfxI32 *distance);
mfxU16 __cdecl ME_simple(ASCVidRead *videoIn, mfxI32 fPos, ASCImDetails *dataIn, ASCimageData *scale, ASCimageData *scaleRef, bool first, ASCVidData *limits, t_ME_SAD_8x8_Block_Search ME_SAD_8x8_Block_Search);
/* All Block sizes -general case- */
bool MVcalcSAD(ASCMVector MV, pmfxU8 curY, pmfxU8 refY, ASCImDetails dataIn, mfxI32 fPos,mfxI32 xLoc, mfxI32 yLoc, mfxU32 *bestSAD, mfxI32 *distance);
/* ------------------------------ */
void SearchLimitsCalc(mfxI32 xLoc, mfxI32 yLoc, mfxI32 *limitXleft, mfxI32 *limitXright, mfxI32 *limitYup, mfxI32 *limitYdown, ASCImDetails dataIn, mfxI32 range, ASCMVector mv, ASCVidData limits);
mfxF32 Dist(ASCMVector vector);
mfxI32 DistInt(ASCMVector vector);
void MVpropagationCheck(mfxI32 xLoc, mfxI32 yLoc, ASCImDetails dataIn, ASCMVector *propagatedMV);
};
#endif //_MOTIONESTIMATIONENGINE_H_