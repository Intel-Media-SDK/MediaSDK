// Copyright (c) 2018-2020 Intel Corporation
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
#include "asc_defs.h"
#include "../include/motion_estimation_engine.h"
#include "../include/asc_cpu_dispatcher.h"
#include "../include/asc_structures.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <limits.h>


namespace ns_asc{

const ASCMVector zero = {0,0};

void MotionRangeDeliveryF(mfxI16 xLoc, mfxI16 yLoc, mfxI16 *limitXleft, mfxI16 *limitXright, mfxI16 *limitYup, mfxI16 *limitYdown, ASCImDetails dataIn) {
    *limitXleft     = (mfxI16)NMAX(-8, -(xLoc * dataIn.block_width) - dataIn.horizontal_pad);
    *limitXright    = (mfxI16)NMIN(7, dataIn.Extended_Width - (((xLoc + 1) * dataIn.block_width) + dataIn.horizontal_pad));
    *limitYup       = (mfxI16)NMAX(-8, -(yLoc * dataIn.block_height) - dataIn.vertical_pad);
    *limitYdown     = (mfxI16)NMIN(7, dataIn.Extended_Height - (((yLoc + 1) * dataIn.block_width) + dataIn.vertical_pad));
}

bool MVcalcSAD8x8(ASCMVector MV, pmfxU8 curY, pmfxU8 refY, ASCImDetails *dataIn, mfxU16 *bestSAD, mfxI32 *distance, t_ME_SAD_8x8_Block ME_SAD_8x8_opt) {
    mfxI32
        preDist = (MV.x * MV.x) + (MV.y * MV.y),
        _fPos = MV.x + (MV.y * dataIn->Extended_Width);
    pmfxU8
        fRef = &refY[_fPos];
    mfxU16
        SAD = ME_SAD_8x8_opt(curY, fRef, dataIn->Extended_Width, dataIn->Extended_Width);
    if((SAD < *bestSAD) || ((SAD == *(bestSAD)) && *distance > preDist)) {
        *distance = preDist;
        *(bestSAD) = SAD;
        return true;
    }
    return false;
}

void MVcalcVar8x8(ASCMVector MV, pmfxU8 curY, pmfxU8 refY, mfxI16 curAvg, mfxI16 refAvg, mfxI32 &var, mfxI32 &jtvar, mfxI32 &jtMCvar, ASCImDetails *dataIn, t_ME_VAR_8x8_Block ME_VAR_8x8_opt) {
    mfxI32
        _fPos = MV.x + (MV.y * dataIn->Extended_Width);
    pmfxU8
        fRef = &refY[_fPos];

    ME_VAR_8x8_opt(curY, refY, fRef, curAvg, refAvg, dataIn->Extended_Width, dataIn->Extended_Width, var, jtvar, jtMCvar);
}

void SearchLimitsCalcSqr(mfxI16 xLoc, mfxI16 yLoc, mfxI16 *limitXleft, mfxI16 *limitXright, mfxI16 *limitYup, mfxI16 *limitYdown, ASCImDetails *dataIn, mfxI32 range, ASCMVector mv, ASCVidData *limits) {
    mfxI16
        locX = (mfxI16)((xLoc * dataIn->block_width) + dataIn->horizontal_pad + mv.x),
        locY = (mfxI16)((yLoc * dataIn->block_height) + dataIn->vertical_pad + mv.y);
    *limitXleft     = (mfxI16)NMAX(-locX,-range);
    *limitXright    = (mfxI16)NMIN(dataIn->Extended_Width - ((xLoc + 1) * dataIn->block_width) - dataIn->horizontal_pad - mv.x,range - 1);
    *limitYup       = (mfxI16)NMAX(-locY,-range);
    *limitYdown     = (mfxI16)NMIN(dataIn->Extended_Height - ((yLoc + 1) * dataIn->block_height) - dataIn->vertical_pad - mv.y,range - 1);
    if(limits->limitRange) {
        *limitXleft     = (mfxI16)NMAX(*limitXleft,-limits->maxXrange);
        *limitXright    = (mfxI16)NMIN(*limitXright,limits->maxXrange);
        *limitYup       = (mfxI16)NMAX(*limitYup,-limits->maxYrange);
        *limitYdown     = (mfxI16)NMIN(*limitYdown,limits->maxYrange);
    }
}

void SearchLimitsCalc(mfxI16 xLoc, mfxI16 yLoc, mfxI16 *limitXleft, mfxI16 *limitXright, mfxI16 *limitYup, mfxI16 *limitYdown, ASCImDetails *dataIn, mfxI32 range, ASCMVector mv, ASCVidData *limits) {
    mfxI16
        locX = (mfxI16)((xLoc * dataIn->block_width) + dataIn->horizontal_pad + mv.x),
        locY = (mfxI16)((yLoc * dataIn->block_height) + dataIn->vertical_pad + mv.y);
    *limitXleft     = (mfxI16)NMAX(-locX,-range);
    *limitXright    = (mfxI16)NMIN(dataIn->Extended_Width - ((xLoc + 1) * dataIn->block_width) - dataIn->horizontal_pad - mv.x,range);
    *limitYup       = (mfxI16)NMAX(-locY,-range);
    *limitYdown     = (mfxI16)NMIN(dataIn->Extended_Height - ((yLoc + 1) * dataIn->block_height) - dataIn->vertical_pad - mv.y,range);
    if(limits->limitRange) {
        *limitXleft     = (mfxI16)NMAX(*limitXleft,-limits->maxXrange);
        *limitXright    = (mfxI16)NMIN(*limitXright,limits->maxXrange);
        *limitYup       = (mfxI16)NMAX(*limitYup,-limits->maxYrange);
        *limitYdown     = (mfxI16)NMIN(*limitYdown,limits->maxYrange);
    }
}

mfxF32 Dist(ASCMVector vector) {
    return (mfxF32)sqrt((double)(vector.x*vector.x) + (double)(vector.y*vector.y));
}

mfxI32 DistInt(ASCMVector vector) {
    return (vector.x*vector.x) + (vector.y*vector.y);
}

void MVpropagationCheck(mfxI32 xLoc, mfxI32 yLoc, ASCImDetails dataIn, ASCMVector *propagatedMV) {
    mfxI16
        left    = (mfxI16)((xLoc * dataIn.block_width) + dataIn.horizontal_pad),
        right   = (mfxI16)(dataIn.Extended_Width - left - dataIn.block_width),
        up      = (mfxI16)((yLoc * dataIn.block_height) + dataIn.vertical_pad),
        down    = (mfxI16)(dataIn.Extended_Height - up - dataIn.block_height);
    if(propagatedMV->x < 0) {
        if(left + propagatedMV->x < 0)
            propagatedMV->x = -left;
    }
    else {
        if(right - propagatedMV->x < 0)
            propagatedMV->x = right;
    }

    if(propagatedMV->y < 0) {
        if(up + propagatedMV->y < 0)
            propagatedMV->y = -up;
    }
    else {
        if(down - propagatedMV->y < 0)
            propagatedMV->y = down;
    }
}

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))

#define SAD_SEARCH_VSTEP 2  // 1=FS 2=FHS

mfxU16 __cdecl ME_simple(
    ASCVidRead *videoIn,
    mfxI32                    fPos,
    ASCImDetails             *dataIn,
    ASCimageData             *scale,
    ASCimageData             *scaleRef,
    bool /*first*/,
    ASCVidData               *limits,
    t_ME_SAD_8x8_Block_Search ME_SAD_8x8_Block_Search,
    t_ME_SAD_8x8_Block        ME_SAD_8x8_opt,
    t_ME_VAR_8x8_Block        ME_VAR_8x8_opt
    )
{
    ASCMVector
        tMV,
        ttMV,
        *current,
        predMV = zero,
        Nmv    = zero;
    mfxU8
        *objFrame = NULL,
        *refFrame = NULL;
    mfxI16
        limitXleft  = 0,
        limitXright = 0,
        limitYup    = 0,
        limitYdown  = 0,
        xLoc = ((mfxI16)fPos % (mfxI16)dataIn->Width_in_blocks),
        yLoc = ((mfxI16)fPos / (mfxI16)dataIn->Width_in_blocks);
    mfxI32
        distance     = 0,
        mainDistance = 0,
        offset       = (yLoc * (mfxI16)dataIn->Extended_Width * (mfxI16)dataIn->block_height) + (xLoc * (mfxI16)dataIn->block_width);
    mfxU16
        *outSAD,
        zeroSAD = USHRT_MAX,
        bestSAD = USHRT_MAX;
    mfxU8
        neighbor_count = 0;
    bool
        foundBetter = false;

    objFrame = &scale->Image.Y[offset];
    refFrame = &scaleRef->Image.Y[offset];

    current = scale->pInteger;
    outSAD  = scale->SAD;

    outSAD[fPos] = USHRT_MAX;

    MVcalcSAD8x8(zero, objFrame, refFrame, dataIn, &bestSAD, &distance, ME_SAD_8x8_opt);
    current[fPos] = zero;
    outSAD[fPos]  = bestSAD;
    zeroSAD       = bestSAD;
    mainDistance  = distance;
    if (bestSAD == 0)
        return bestSAD;

    if ((fPos > (mfxI32)dataIn->Width_in_blocks) && (xLoc > 0)) { //Top Left
        neighbor_count++;
        Nmv.x += current[fPos - dataIn->Width_in_blocks - 1].x;
        Nmv.y += current[fPos - dataIn->Width_in_blocks - 1].y;
    }
    if (fPos > (mfxI32)dataIn->Width_in_blocks) { // Top
        neighbor_count++;
        Nmv.x += current[fPos - dataIn->Width_in_blocks].x;
        Nmv.y += current[fPos - dataIn->Width_in_blocks].y;
    }
    if (xLoc > 0) {//Left
        neighbor_count++;
        Nmv.x += current[fPos - 1].x;
        Nmv.y += current[fPos - 1].y;
    }
    if (neighbor_count) {
        Nmv.x /= neighbor_count;
        Nmv.y /= neighbor_count;
        if ((Nmv.x + ((xLoc + 1) * MVBLK_SIZE)) > ASC_SMALL_WIDTH)
            Nmv.x -= (Nmv.x + ((xLoc + 1) * MVBLK_SIZE)) - ASC_SMALL_WIDTH;
        else if (((xLoc * MVBLK_SIZE) + Nmv.x) < 0)
            Nmv.x -= ((xLoc * MVBLK_SIZE) + Nmv.x);

        if ((Nmv.y + ((yLoc + 1) * MVBLK_SIZE)) > ASC_SMALL_HEIGHT)
            Nmv.y -= (Nmv.y + ((yLoc + 1) * MVBLK_SIZE)) - ASC_SMALL_HEIGHT;
        else if (((yLoc * MVBLK_SIZE) + Nmv.y) < 0)
            Nmv.y -= ((yLoc * MVBLK_SIZE) + Nmv.y);

        distance = mainDistance;
        if (Nmv.x != zero.x || Nmv.y != zero.y) {
            foundBetter = MVcalcSAD8x8(Nmv, objFrame, refFrame, dataIn, &bestSAD, &distance, ME_SAD_8x8_opt);
            if (foundBetter) {
                current[fPos] = Nmv;
                outSAD[fPos] = bestSAD;
                mainDistance = distance;
            }
        }
    }

    //Search around the best predictor (zero or Neighbor)
    SearchLimitsCalcSqr(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, 8, current[fPos], limits);//Checks limits for +-8
    ttMV     = current[fPos];
    bestSAD  = outSAD[fPos];
    distance = mainDistance;

    {//Search area in steps of 2 for x and y
        mfxI32 _fPos = (limitYup + ttMV.y) * dataIn->Extended_Width + limitXleft + ttMV.x;
        mfxU8
            *ps = objFrame,
            *pr = &refFrame[_fPos];
        int xrange = limitXright - limitXleft/* + 1*/,
            yrange = limitYdown - limitYup/* + 1*/,
            bX     = 0,
            bY     = 0;
        ME_SAD_8x8_Block_Search(ps, pr, dataIn->Extended_Width, xrange, yrange, &bestSAD, &bX, &bY);
        if (bestSAD < outSAD[fPos]) {
            outSAD[fPos]    = bestSAD;
            current[fPos].x = (mfxI16)bX + limitXleft + ttMV.x;
            current[fPos].y = (mfxI16)bY + limitYup + ttMV.y;
            mainDistance    = DistInt(current[fPos]);
        }
    }
    //Final refinement +-1 search
    ttMV     = current[fPos];
    bestSAD  = outSAD[fPos];
    distance = mainDistance;
    SearchLimitsCalc(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, 1, ttMV, limits);
    for (tMV.y = limitYup; tMV.y <= limitYdown; tMV.y++) {
        for (tMV.x = limitXleft; tMV.x <= limitXright; tMV.x++) {
            if (tMV.x != 0 || tMV.y != 0) {// don't search on center position
                predMV.x = tMV.x + ttMV.x;
                predMV.y = tMV.y + ttMV.y;
                foundBetter = MVcalcSAD8x8(predMV, objFrame, refFrame, dataIn, &bestSAD, &distance, ME_SAD_8x8_opt);
                if (foundBetter) {
                    current[fPos] = predMV;
                    outSAD[fPos]  = bestSAD;
                    mainDistance  = distance;
                    foundBetter = false;
                }
            }
        }
    }
    videoIn->average += (current[fPos].x * current[fPos].x) + (current[fPos].y * current[fPos].y);
    MVcalcVar8x8(current[fPos], objFrame, refFrame, scale->avgval, scaleRef->avgval, scale->var, scale->jtvar, scale->mcjtvar, dataIn, ME_VAR_8x8_opt);
    return(zeroSAD);
}
};
