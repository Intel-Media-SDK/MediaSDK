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

#include "mfx_scd.h"
#include "libmfx_core_interface.h"

#include "mfx_utils.h"
#include "mfx_task.h"
#include <cassert>

#if defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)

#define OUT_BLOCK         16
#define BLOCK_SIZE        4
#define NumTSC            10
#define NumSC             10
#define FLOAT_MAX         2241178.0
#define FRAMEMUL          16
#define CHROMASUBSAMPLE   4
#define SMALL_WIDTH       112
#define SMALL_HEIGHT      64
#define LN2               0.6931471805599453
#define MEMALLOCERROR     1000
#define MEMALLOCERRORU8   1001
#define MEMALLOCERRORMV   1002

#define NMAX(a,b)         ((a>b)?a:b)
#define NMIN(a,b)         ((a<b)?a:b)
#define NABS(a)           (((a)<0)?(-(a)):(a))
#define NAVG(a,b)         ((a+b)/2)

#define Clamp(x)          ((x<0)?0:((x>255)?255:x))
#define TSCSTATBUFFER     3

#define EXTRANEIGHBORS
#define SAD_SEARCH_VSTEP  2  // 1=FS 2=FHS
#define DECISION_THR      6 //Total number of trees is 13, decision has to be bigger than 6 to say it is a scene change.

#define EXTRANEIGHBORS
#define SAD_SEARCH_VSTEP 2  // 1=FS 2=FHS
#define NUM_VIDEO_SAMPLE_LAYERS 3
#define SAMPLE_VIDEO_INPUT_INDEX 2

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))

static const mfxU16 tab_killmask[8][8] = {
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
};

static mfxI32 PDISTTbl2mod[NumTSC*NumSC] =
{
    2, 3, 4, 5, 5, 5, 5, 5, 5, 5,
    2, 2, 3, 4, 4, 4, 5, 5, 5, 5,
    1, 2, 2, 3, 3, 3, 4, 4, 5, 5,
    1, 1, 2, 2, 3, 3, 3, 4, 4, 5,
    1, 1, 2, 2, 2, 2, 2, 3, 3, 4,
    1, 1, 1, 2, 2, 2, 2, 3, 3, 3,
    1, 1, 1, 1, 2, 2, 2, 2, 3, 3,
    1, 1, 1, 1, 2, 2, 2, 2, 2, 3,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static mfxI32 PDISTTbl2[NumTSC*NumSC] =
{
    2, 3, 3, 4, 4, 5, 5, 5, 5, 5,
    2, 2, 3, 3, 4, 4, 5, 5, 5, 5,
    1, 2, 2, 3, 3, 3, 4, 4, 5, 5,
    1, 1, 2, 2, 3, 3, 3, 4, 4, 5,
    1, 1, 2, 2, 3, 3, 3, 3, 3, 4,
    1, 1, 1, 2, 2, 3, 3, 3, 3, 3,
    1, 1, 1, 1, 2, 2, 3, 3, 3, 3,
    1, 1, 1, 1, 2, 2, 2, 3, 3, 3,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static mfxI32 PDISTTbl3[NumTSC*NumSC] =
{
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    4, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
    3, 3, 4, 4, 4, 5, 5, 5, 5, 5,
    3, 3, 3, 4, 4, 4, 4, 5, 5, 5,
    2, 2, 2, 2, 3, 4, 4, 4, 5, 5,
    1, 2, 2, 2, 2, 3, 4, 4, 4, 5,
    1, 1, 2, 2, 2, 2, 3, 3, 4, 4

};
static mfxF32 lmt_sc2[NumSC] = { 4.0, 9.0, 15.0, 23.0, 32.0, 42.0, 53.0, 65.0, 78.0, FLOAT_MAX }; // lower limit of SFM(Rs,Cs) range for spatial classification
// 9 ranges of SC are: 0 0-4, 1 4-9, 2 9-15, 3 15-23, 4 23-32, 5 32-44, 6 42-53, 7 53-65, 8 65-78, 9 78->??
static mfxF32 lmt_tsc2[NumTSC] = { 0.75, 1.5, 2.25, 3.0, 4.0, 5.0, 6.0, 7.5, 9.25, FLOAT_MAX };   // lower limit of AFD
// 8 ranges of TSC (based on FD) are:0 0-0.75 1 0.75-1.5, 2 1.5-2.25. 3 2.25-3, 4 3-4, 5 4-5, 6 5-6, 7 6-7.5, 8 7.5-9.25, 9 9.25->??
static mfxF32 TH[4] = { -12.0, -4.0, 4.0, 12.0 };

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
#define SCD_CPU_DISP(OPT,func, ...)  (( CPU_AVX2 == OPT ) ? func ## _AVX2(__VA_ARGS__) : (( CPU_SSE4 == OPT ) ? func ## _SSE4(__VA_ARGS__) : func ## _C(__VA_ARGS__) ))
#else
#define SCD_CPU_DISP(OPT,func, ...)  (( CPU_SSE4 == OPT ) ? func ## _SSE4(__VA_ARGS__) : func ## _C(__VA_ARGS__) )
#endif

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
void ME_SAD_8x8_Block_Search_AVX2(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
           mfxU32 *bestSAD, int *bestX, int *bestY)
{
    __m256i
        s0 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0*pitch]), (__m128i *)&pSrc[1*pitch])),
        s1 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2*pitch]), (__m128i *)&pSrc[3*pitch])),
        s2 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4*pitch]), (__m128i *)&pSrc[5*pitch])),
        s3 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6*pitch]), (__m128i *)&pSrc[7*pitch]));

    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8 pr = pRef + (y * pitch) + x;
            __m256i
                r0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[0*pitch])),
                r1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[1*pitch])),
                r2 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[2*pitch])),
                r3 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[3*pitch])),
                r4 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[4*pitch])),
                r5 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[5*pitch])),
                r6 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[6*pitch])),
                r7 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[7*pitch]));
            r0 = _mm256_mpsadbw_epu8(r0, s0, 0x28);
            r1 = _mm256_mpsadbw_epu8(r1, s0, 0x3a);
            r2 = _mm256_mpsadbw_epu8(r2, s1, 0x28);
            r3 = _mm256_mpsadbw_epu8(r3, s1, 0x3a);
            r4 = _mm256_mpsadbw_epu8(r4, s2, 0x28);
            r5 = _mm256_mpsadbw_epu8(r5, s2, 0x3a);
            r6 = _mm256_mpsadbw_epu8(r6, s3, 0x28);
            r7 = _mm256_mpsadbw_epu8(r7, s3, 0x3a);
            r0 = _mm256_add_epi16(r0, r1);
            r2 = _mm256_add_epi16(r2, r3);
            r4 = _mm256_add_epi16(r4, r5);
            r6 = _mm256_add_epi16(r6, r7);
            r0 = _mm256_add_epi16(r0, r2);
            r4 = _mm256_add_epi16(r4, r6);
            r0 = _mm256_add_epi16(r0, r4);
            // horizontal sum
            __m128i
                t = _mm_add_epi16(_mm256_castsi256_si128(r0), _mm256_extractf128_si256(r0, 1));
            // kill out-of-bound values
            if (xrange - x < 8)
                t = _mm_or_si128(t, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            t = _mm_minpos_epu16(t);
            mfxU32
                SAD = _mm_extract_epi16(t, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(t, 1);
                *bestY = y;
            }
        }
    }
}
#endif

void ME_SAD_8x8_Block_Search_SSE4(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
           mfxU32 *bestSAD, int *bestX, int *bestY)
{
    __m128i
        s0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0*pitch]), (__m128i *)&pSrc[1*pitch]),
        s1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2*pitch]), (__m128i *)&pSrc[3*pitch]),
        s2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4*pitch]), (__m128i *)&pSrc[5*pitch]),
        s3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6*pitch]), (__m128i *)&pSrc[7*pitch]);
    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8
                pr = pRef + (y * pitch) + x;
            __m128i
                r0 = _mm_loadu_si128((__m128i *)&pr[0*pitch]),
                r1 = _mm_loadu_si128((__m128i *)&pr[1*pitch]),
                r2 = _mm_loadu_si128((__m128i *)&pr[2*pitch]),
                r3 = _mm_loadu_si128((__m128i *)&pr[3*pitch]),
                r4 = _mm_loadu_si128((__m128i *)&pr[4*pitch]),
                r5 = _mm_loadu_si128((__m128i *)&pr[5*pitch]),
                r6 = _mm_loadu_si128((__m128i *)&pr[6*pitch]),
                r7 = _mm_loadu_si128((__m128i *)&pr[7*pitch]);
            r0 = _mm_add_epi16(_mm_mpsadbw_epu8(r0, s0, 0), _mm_mpsadbw_epu8(r0, s0, 5));
            r1 = _mm_add_epi16(_mm_mpsadbw_epu8(r1, s0, 2), _mm_mpsadbw_epu8(r1, s0, 7));
            r2 = _mm_add_epi16(_mm_mpsadbw_epu8(r2, s1, 0), _mm_mpsadbw_epu8(r2, s1, 5));
            r3 = _mm_add_epi16(_mm_mpsadbw_epu8(r3, s1, 2), _mm_mpsadbw_epu8(r3, s1, 7));
            r4 = _mm_add_epi16(_mm_mpsadbw_epu8(r4, s2, 0), _mm_mpsadbw_epu8(r4, s2, 5));
            r5 = _mm_add_epi16(_mm_mpsadbw_epu8(r5, s2, 2), _mm_mpsadbw_epu8(r5, s2, 7));
            r6 = _mm_add_epi16(_mm_mpsadbw_epu8(r6, s3, 0), _mm_mpsadbw_epu8(r6, s3, 5));
            r7 = _mm_add_epi16(_mm_mpsadbw_epu8(r7, s3, 2), _mm_mpsadbw_epu8(r7, s3, 7));
            r0 = _mm_add_epi16(r0, r1);
            r2 = _mm_add_epi16(r2, r3);
            r4 = _mm_add_epi16(r4, r5);
            r6 = _mm_add_epi16(r6, r7);
            r0 = _mm_add_epi16(r0, r2);
            r4 = _mm_add_epi16(r4, r6);
            r0 = _mm_add_epi16(r0, r4);
            // kill out-of-bound values
            if (xrange - x < 8)
                r0 = _mm_or_si128(r0, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            r0 = _mm_minpos_epu16(r0);
            mfxU32
                SAD = _mm_extract_epi16(r0, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(r0, 1);
                *bestY = y;
            }
        }
    }
}

void ME_SAD_8x8_Block_Search_C(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
        mfxU32 *bestSAD, int *bestX, int *bestY) {
    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x++) {
            pmfxU8
                pr = pRef + (y * pitch) + x,
                ps = pSrc;
            mfxU32
                SAD = 0;
            for (int i = 0; i < 8; i++) {
                SAD += abs(pr[0] - ps[0]);
                SAD += abs(pr[1] - ps[1]);
                SAD += abs(pr[2] - ps[2]);
                SAD += abs(pr[3] - ps[3]);
                SAD += abs(pr[4] - ps[4]);
                SAD += abs(pr[5] - ps[5]);
                SAD += abs(pr[6] - ps[6]);
                SAD += abs(pr[7] - ps[7]);
                pr += pitch;
                ps += pitch;
            }
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x;
                *bestY = y;
            }
        }
    }
}

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))
#define _mm_movehl_epi64(a, b) _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)))

mfxU32 ME_SAD_8x8_Block(mfxU8 *pSrc, mfxU8 *pRef, mfxU32 srcPitch, mfxU32 refPitch)
{
    __m128i s0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0*srcPitch]), (__m128i *)&pSrc[1*srcPitch]);
    __m128i r0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[0*refPitch]), (__m128i *)&pRef[1*refPitch]);
    __m128i s1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2*srcPitch]), (__m128i *)&pSrc[3*srcPitch]);
    __m128i r1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[2*refPitch]), (__m128i *)&pRef[3*refPitch]);
    __m128i s2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4*srcPitch]), (__m128i *)&pSrc[5*srcPitch]);
    __m128i r2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[4*refPitch]), (__m128i *)&pRef[5*refPitch]);
    __m128i s3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6*srcPitch]), (__m128i *)&pSrc[7*srcPitch]);
    __m128i r3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[6*refPitch]), (__m128i *)&pRef[7*refPitch]);
    s0 = _mm_sad_epu8(s0, r0);
    s1 = _mm_sad_epu8(s1, r1);
    s2 = _mm_sad_epu8(s2, r2);
    s3 = _mm_sad_epu8(s3, r3);
    s0 = _mm_add_epi32(s0, s1);
    s2 = _mm_add_epi32(s2, s3);
    s0 = _mm_add_epi32(s0, s2);
    s2 = _mm_movehl_epi64(s2, s0);
    s0 = _mm_add_epi32(s0, s2);
    return _mm_cvtsi128_si32(s0);
}

BOOL MVcalcSAD8x8(MVector MV, pmfxU8 curY, pmfxU8 refY, ImDetails dataIn, mfxU32 *bestSAD, mfxI32 *distance) {
    mfxI32
        preDist = (MV.x * MV.x) + (MV.y * MV.y);
    pmfxU8
        fRef = refY + MV.x + (MV.y * dataIn.Extended_Width);
    mfxU32 SAD = 0;
    SAD = ME_SAD_8x8_Block(curY, fRef, dataIn.Extended_Width, dataIn.Extended_Width);
    if((SAD < *bestSAD) || ((SAD == *(bestSAD)) && *distance > preDist))
    {
        *distance = preDist;
        *(bestSAD) = SAD;
        return TRUE;
    }
    return FALSE;
}

void MotionRangeDeliveryF(mfxI32 xLoc, mfxI32 yLoc, mfxI32 *limitXleft, mfxI32 *limitXright, mfxI32 *limitYup, mfxI32 *limitYdown, ImDetails dataIn, VidData limits) {
    limits;
    mfxI32
        locX = 0,
        locY = 0;
    locY = yLoc / ((3 * (16 / dataIn.block_height)) / 2);
    locX = xLoc / ((3 * (16 / dataIn.block_width)) / 2);
    *limitXleft = NMAX(-16, -(xLoc * dataIn.block_width) - dataIn.horizontal_pad);
    *limitXright = NMIN(15, dataIn.Extended_Width - (((xLoc + 1) * dataIn.block_width) + dataIn.horizontal_pad));
    *limitYup = NMAX(-12, -(yLoc * dataIn.block_height) - dataIn.vertical_pad);
    *limitYdown = NMIN(11, dataIn.Extended_Height - (((yLoc + 1) * dataIn.block_width) + dataIn.vertical_pad));
}

mfxI32 DistInt(MVector vector) {
    return (vector.x*vector.x) + (vector.y*vector.y);;
}

void MVpropagationCheck(mfxI32 xLoc, mfxI32 yLoc, ImDetails dataIn, MVector *propagatedMV) {
    mfxI32
        left = (xLoc * dataIn.block_width) + dataIn.horizontal_pad,
        right = (dataIn.Extended_Width - left - dataIn.block_width),
        up = (yLoc * dataIn.block_height) + dataIn.vertical_pad,
        down = (dataIn.Extended_Height - up - dataIn.block_height);
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

void SearchLimitsCalc(mfxI32 xLoc, mfxI32 yLoc, mfxI32 *limitXleft, mfxI32 *limitXright, mfxI32 *limitYup, mfxI32 *limitYdown, ImDetails dataIn, mfxI32 range, MVector mv, VidData limits) {
    mfxI32
        locX = (xLoc * dataIn.block_width) + dataIn.horizontal_pad + mv.x,
        locY = (yLoc * dataIn.block_height) + dataIn.vertical_pad + mv.y;
    *limitXleft = NMAX(-locX,-range);
    *limitXright = NMIN(dataIn.Extended_Width - ((xLoc + 1) * dataIn.block_width) - dataIn.horizontal_pad - mv.x,range);
    *limitYup = NMAX(-locY,-range);
    *limitYdown = NMIN(dataIn.Extended_Height - ((yLoc + 1) * dataIn.block_height) - dataIn.vertical_pad - mv.y,range);
    if(limits.limitRange) {
        *limitXleft = NMAX(*limitXleft,-limits.maxXrange);
        *limitXright = NMIN(*limitXright,limits.maxXrange);
        *limitYup = NMAX(*limitYup,-limits.maxYrange);
        *limitYdown = NMIN(*limitYdown,limits.maxYrange);
    }
}

#define MAXSPEED 0
#define EXTRANEIGHBORS

mfxI32 __cdecl SceneChangeDetector::HME_Low8x8fast(VidRead *videoIn, mfxI32 fPos, ImDetails dataIn, imageData *scale, imageData *scaleRef, BOOL first, mfxU32 accuracy, VidData limits) {
    const MVector zero = {0,0};
    MVector
        lineMV[2],
        tMV,
        ttMV,
        *current,
        preMV = zero,
        Nmv;
    mfxU8
        *objFrame,
        *refFrame[4];
    mfxI32
        limitXleft = 0,
        limitXright = 0,
        limitYup = 0,
        limitYdown = 0,
        k,
        direction[2],
        acc,
        counter,
        lineSAD[2],
        xLoc = (fPos % dataIn.Width_in_blocks),
        yLoc = (fPos / dataIn.Width_in_blocks),
        distance = INT_MAX,
        plane = 0,
        bestPlane = 0,
        bestPlaneN = 0,
        zeroSAD = INT_MAX,
        offset = (yLoc * dataIn.Extended_Width * dataIn.block_height) + (xLoc * dataIn.block_width);
    mfxU32
        tl,
        *outSAD,
        bestSAD = INT_MAX;
    Rsad
        range;
    BOOL
        foundBetter = FALSE,
        truneighbor = FALSE;
    objFrame = scale->Image.Y + offset;
    refFrame[0] = scaleRef->Image.Y + offset;
    current = scale->pInteger;
    outSAD = scale->SAD;
    if (accuracy == 1)
        acc = 1;
    else
        acc = 4;
    if (first) {
        MVcalcSAD8x8(zero, objFrame, refFrame[0], dataIn, &bestSAD, &distance);
        zeroSAD = bestSAD;
        current[fPos] = zero;
        outSAD[fPos] = bestSAD;
        if (zeroSAD == 0)
            return zeroSAD;
        MotionRangeDeliveryF(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, limits);
    }
    for (k = 0; k<5; k++) {
        range.SAD = outSAD[fPos];
        range.BestMV = current[fPos];
        range.distance = INT_MAX;
        range.angle = 361;
        range.direction = 0;
    }
    direction[0] = 0;
    lineMV[0].x = 0;
    lineMV[0].y = 0;
    k = 0;
    {
        mfxU8
            *ps = objFrame,
            *pr = refFrame[plane] + (limitYup * dataIn.Extended_Width) + limitXleft;
        int xrange = limitXright - limitXleft + 1,
            yrange = limitYdown - limitYup + 1,
            bX = 0,
            bY = 0;
        mfxU32
            bSAD = INT_MAX;
        SCD_CPU_DISP(m_cpuOpt, ME_SAD_8x8_Block_Search, ps, pr, dataIn.Extended_Width, xrange, yrange, &bSAD, &bX, &bY);
        if (bSAD < range.SAD) {
            range.SAD = bSAD;
            range.BestMV.x = bX + limitXleft;
            range.BestMV.y = bY + limitYup;
            range.distance = DistInt(range.BestMV);
        }
    }
    outSAD[fPos] = range.SAD;
    current[fPos] = range.BestMV;

    /*ExtraNeighbors*/
#ifdef EXTRANEIGHBORS
    if (fPos > (dataIn.Width_in_blocks * 3)) {
        tl = range.SAD;
        Nmv.x = current[fPos - (dataIn.Width_in_blocks * 3)].x;
        Nmv.y = current[fPos - (dataIn.Width_in_blocks * 3)].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if ((fPos > (dataIn.Width_in_blocks * 2)) && (xLoc > 0)) {
        tl = range.SAD;
        Nmv.x = current[fPos - (dataIn.Width_in_blocks * 2) - 1].x;
        Nmv.y = current[fPos - (dataIn.Width_in_blocks * 2) - 1].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if (fPos > (dataIn.Width_in_blocks * 2)) {
        tl = range.SAD;
        Nmv.x = current[fPos - (dataIn.Width_in_blocks * 2)].x;
        Nmv.y = current[fPos - (dataIn.Width_in_blocks * 2)].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if ((fPos > (dataIn.Width_in_blocks * 2)) && (xLoc < dataIn.Width_in_blocks - 1)) {
        tl = range.SAD;
        Nmv.x = current[fPos - (dataIn.Width_in_blocks * 2) + 1].x;
        Nmv.y = current[fPos - (dataIn.Width_in_blocks * 2) + 1].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
#endif
    if ((fPos > dataIn.Width_in_blocks) && (xLoc > 0)) {
        tl = range.SAD;
        Nmv.x = current[fPos - dataIn.Width_in_blocks - 1].x;
        Nmv.y = current[fPos - dataIn.Width_in_blocks - 1].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if (fPos > dataIn.Width_in_blocks) {
        tl = range.SAD;
        Nmv.x = current[fPos - dataIn.Width_in_blocks].x;
        Nmv.y = current[fPos - dataIn.Width_in_blocks].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if ((fPos> dataIn.Width_in_blocks) && (xLoc < dataIn.Width_in_blocks - 1)) {
        tl = range.SAD;
        Nmv.x = current[fPos - dataIn.Width_in_blocks + 1].x;
        Nmv.y = current[fPos - dataIn.Width_in_blocks + 1].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if (xLoc > 0) {
        tl = range.SAD;
        distance = INT_MAX;
        Nmv.x = current[fPos - 1].x;
        Nmv.y = current[fPos - 1].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if (xLoc > 1) {
        tl = range.SAD;
        distance = INT_MAX;
        Nmv.x = current[fPos - 2].x;
        Nmv.y = current[fPos - 2].y;
        distance = DistInt(Nmv);
        MVpropagationCheck(xLoc, yLoc, dataIn, &Nmv);
        if (MVcalcSAD8x8(Nmv, objFrame, refFrame[plane], dataIn, &tl, &distance)) {
            range.BestMV = Nmv;
            range.SAD = tl;
            foundBetter = FALSE;
            truneighbor = TRUE;
            bestPlaneN = plane;
        }
    }
    if (truneighbor) {
        ttMV = range.BestMV;
        bestSAD = range.SAD;
        SearchLimitsCalc(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, 1, range.BestMV, limits);
        for (tMV.y = limitYup; tMV.y <= limitYdown; tMV.y++) {
            for (tMV.x = limitXleft; tMV.x <= limitXright; tMV.x++) {
                preMV.x = tMV.x + ttMV.x;
                preMV.y = tMV.y + ttMV.y;
                foundBetter = MVcalcSAD8x8(preMV, objFrame, refFrame[plane], dataIn, &bestSAD, &distance);
                if (foundBetter) {
                    range.BestMV = preMV;
                    range.SAD = bestSAD;
                    bestPlaneN = plane;
                    foundBetter = FALSE;
                }
            }
        }
        if (truneighbor) {
            outSAD[fPos] = range.SAD;
            current[fPos] = range.BestMV;
            truneighbor = FALSE;
            bestPlane = bestPlaneN;
        }
    }
    range.SAD = outSAD[fPos];
    /*Zero search*/
    if (!first) {
        SearchLimitsCalc(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, 16/*32*/, zero, limits);
        counter = 0;
        for (tMV.y = limitYup; tMV.y <= limitYdown; tMV.y += 2) {
            lineSAD[0] = INT_MAX;
            for (tMV.x = limitXleft + ((counter % 2) * 2); tMV.x <= limitXright; tMV.x += 4) {
                ttMV.x = tMV.x + zero.x;
                ttMV.y = tMV.y + zero.y;
                bestSAD = range.SAD;
                distance = range.distance;
                foundBetter = MVcalcSAD8x8(ttMV, objFrame, refFrame[0], dataIn, &bestSAD, &distance);
                if (foundBetter) {
                    range.BestMV = ttMV;
                    range.SAD = bestSAD;
                    range.distance = distance;
                    foundBetter = FALSE;
                    bestPlane = 0;
                }
            }
            counter++;
        }
    }
    ttMV = range.BestMV;
    SearchLimitsCalc(xLoc, yLoc, &limitXleft, &limitXright, &limitYup, &limitYdown, dataIn, 1, range.BestMV, limits);
    for (tMV.y = limitYup; tMV.y <= limitYdown; tMV.y++) {
        for (tMV.x = limitXleft; tMV.x <= limitXright; tMV.x++) {
            preMV.x = tMV.x + ttMV.x;
            preMV.y = tMV.y + ttMV.y;
            foundBetter = MVcalcSAD8x8(preMV, objFrame, refFrame[0], dataIn, &bestSAD, &distance);
            if (foundBetter) {
                range.BestMV = preMV;
                range.SAD = bestSAD;
                range.distance = distance;
                bestPlane = 0;
                foundBetter = FALSE;
            }
        }
    }
    outSAD[fPos] = range.SAD;
    current[fPos] = range.BestMV;
    videoIn->average += (range.BestMV.x * range.BestMV.x) + (range.BestMV.y * range.BestMV.y);
    return(zeroSAD);
}

void SceneChangeDetector::MotionAnalysis(VidData *dataIn, VidSample *videoIn, VidSample *videoRef, mfxF32 *TSC, mfxF32 *AFD, mfxF32 *MVdiffVal, Layers lyrIdx) {
    mfxI32
        correction = 0;
    mfxU32
        valNoM = 0,
        valb = 0;
    /*--Motion Estimation--*/
    *MVdiffVal = 0;
    mfxF32
        diff = videoRef->layer[0].avgval - videoIn->layer[0].avgval;
    imageData *refImageIn = &videoRef->layer[lyrIdx];

    if(diff >= 50 || diff <= -50) {
        for(mfxI32 i = 0; i < dataIn->layer[0]._cheight; i++) {
            for(mfxI32 j = 0; j < dataIn->layer[0]._cwidth; j++) {
                correction = (mfxI32)(videoRef->layer[lyrIdx].Image.Y[j + i*dataIn->layer[lyrIdx].Extended_Width]) - (mfxI32)(diff);
                if(correction > 255)
                    correction = 255;
                else if(correction < 0)
                    correction = 0;
                support->gainCorrection.Image.Y[j + i*dataIn->layer[lyrIdx].Extended_Width] = (mfxU8)correction;
            }
        }
        refImageIn = &support->gainCorrection;
    }

    for (int fPos = 0; fPos<dataIn->layer[lyrIdx].Blocks_in_Frame; fPos++) {
        valNoM += HME_Low8x8fast(support, fPos, dataIn->layer[lyrIdx], &(videoIn->layer[lyrIdx]), refImageIn, TRUE, 1, *dataIn);
        valb += videoIn->layer[lyrIdx].SAD[fPos];
        *MVdiffVal += (videoIn->layer[lyrIdx].pInteger[fPos].x - videoRef->layer[lyrIdx].pInteger[fPos].x) * (videoIn->layer[lyrIdx].pInteger[fPos].x - videoRef->layer[lyrIdx].pInteger[fPos].x);
        *MVdiffVal += (videoIn->layer[lyrIdx].pInteger[fPos].y - videoRef->layer[lyrIdx].pInteger[fPos].y) * (videoIn->layer[lyrIdx].pInteger[fPos].y - videoRef->layer[lyrIdx].pInteger[fPos].y);
    }
    *TSC = (mfxF32)valb / (mfxF32)dataIn->layer[lyrIdx].Pixels_in_Y_layer;
    *AFD = (mfxF32)valNoM / (mfxF32)dataIn->layer[lyrIdx].Pixels_in_Y_layer;
    *MVdiffVal = (mfxF32)((mfxF64)*MVdiffVal / (mfxF64)dataIn->layer[lyrIdx].Blocks_in_Frame);
    refImageIn = NULL;
}

int TableLookUp(mfxI32 limit, mfxF32 *table, mfxF32 comparisonValue) {
    for (int pos = 0; pos < limit; pos++) {
        if (comparisonValue < table[pos])
            return pos;
    }
    return limit;
}

// Load 0..3 floats to XMM register from memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
static inline __m128 LoadPartialXmm(float *pSrc, mfxI32 len)
{
    __m128 xmm = _mm_setzero_ps();
    if (len & 2)
    {
        xmm = _mm_loadh_pi(xmm, (__m64 *)pSrc);
        pSrc += 2;
    }
    if (len & 1)
    {
        xmm = _mm_move_ss(xmm, _mm_load_ss(pSrc));
    }
    return xmm;
}

// Store 0..3 floats from XMM register to memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
static inline void StorePartialXmm(float *pDst, __m128 xmm, mfxI32 len)
{
    if (len & 2)
    {
        _mm_storeh_pi((__m64 *)pDst, xmm);
        pDst += 2;
    }
    if (len & 1)
    {
        _mm_store_ss(pDst, xmm);
    }
}

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
// Load 0..7 floats to YMM register from memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
static inline __m256 LoadPartialYmm(float *pSrc, mfxI32 len)
{
    __m128 xlo = _mm_setzero_ps();
    __m128 xhi = _mm_setzero_ps();
    if (len & 4) {
        xhi = _mm_loadu_ps(pSrc);
        pSrc += 4;
    }
    if (len & 2) {
        xlo = _mm_loadh_pi(xlo, (__m64 *)pSrc);
        pSrc += 2;
    }
    if (len & 1) {
        xlo = _mm_move_ss(xlo, _mm_load_ss(pSrc));
    }
    return _mm256_insertf128_ps(_mm256_castps128_ps256(xlo), xhi, 1);
}

// Store 0..7 floats from YMM register to memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
static inline void StorePartialYmm(float *pDst, __m256 ymm, mfxI32 len)
{
    __m128 xlo = _mm256_castps256_ps128(ymm);
    __m128 xhi = _mm256_extractf128_ps(ymm, 1);
    if (len & 4) {
        _mm_storeu_ps(pDst, xhi);
        pDst += 4;
    }
    if (len & 2) {
        _mm_storeh_pi((__m64 *)pDst, xlo);
        pDst += 2;
    }
    if (len & 1) {
        _mm_store_ss(pDst, xlo);
    }
}
#endif

// Load 0..15 bytes to XMM register from memory
// NOTE: elements of XMM are permuted [ 8 4 2 - 1 ]
template <char init>
static inline __m128i LoadPartialXmm(unsigned char *pSrc, mfxI32 len)
{
    __m128i xmm = _mm_set1_epi8(init);
    if (len & 8) {
        xmm = _mm_loadh_epi64(xmm, (__m64 *)pSrc);
        pSrc += 8;
    }
    if (len & 4) {
        xmm = _mm_insert_epi32(xmm, *((int *)pSrc), 1);
        pSrc += 4;
    }
    if (len & 2) {
        xmm = _mm_insert_epi16(xmm, *((short *)pSrc), 1);
        pSrc += 2;
    }
    if (len & 1) {
        xmm = _mm_insert_epi8(xmm, *pSrc, 0);
    }
    return xmm;
}

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
// Load 0..31 bytes to YMM register from memory
// NOTE: elements of YMM are permuted [ 16 8 4 2 - 1 ]
template <char init>
static inline __m256i LoadPartialYmm(unsigned char *pSrc, mfxI32 len)
{
    __m128i xlo = _mm_set1_epi8(init);
    __m128i xhi = _mm_set1_epi8(init);
    if (len & 16) {
        xhi = _mm_loadu_si128((__m128i *)pSrc);
        pSrc += 16;
    }
    if (len & 8) {
        xlo = _mm_loadh_epi64(xlo, (__m64 *)pSrc);
        pSrc += 8;
    }
    if (len & 4) {
        xlo = _mm_insert_epi32(xlo, *((int *)pSrc), 1);
        pSrc += 4;
    }
    if (len & 2) {
        xlo = _mm_insert_epi16(xlo, *((short *)pSrc), 1);
        pSrc += 2;
    }
    if (len & 1) {
        xlo = _mm_insert_epi8(xlo, *pSrc, 0);
    }
    return _mm256_inserti128_si256(_mm256_castsi128_si256(xlo), xhi, 1);
}
#endif

/*
 *   RsCsCalc_diff versions
 */
#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
void RsCsCalc_diff_AVX2(pmfxF32 pRs0, pmfxF32 pCs0, pmfxF32 pRs1, pmfxF32 pCs1, int wblocks, int hblocks,
                        pmfxF32 pRsDiff, pmfxF32 pCsDiff)
{
    mfxI32 i, len = wblocks * hblocks;
    __m256d accRs = _mm256_setzero_pd();
    __m256d accCs = _mm256_setzero_pd();

    for (i = 0; i < len - 7; i += 8)
    {
        __m256 rs = _mm256_sub_ps(_mm256_loadu_ps(&pRs0[i]), _mm256_loadu_ps(&pRs1[i]));
        __m256 cs = _mm256_sub_ps(_mm256_loadu_ps(&pCs0[i]), _mm256_loadu_ps(&pCs1[i]));

        rs = _mm256_mul_ps(rs, rs);
        cs = _mm256_mul_ps(cs, cs);

        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_castps256_ps128(rs)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_castps256_ps128(cs)));
        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_extractf128_ps(rs, 1)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_extractf128_ps(cs, 1)));
    }

    if (i < len)
    {
        __m256 rs = _mm256_sub_ps(LoadPartialYmm(&pRs0[i], len & 0x7), LoadPartialYmm(&pRs1[i], len & 0x7));
        __m256 cs = _mm256_sub_ps(LoadPartialYmm(&pCs0[i], len & 0x7), LoadPartialYmm(&pCs1[i], len & 0x7));

        rs = _mm256_mul_ps(rs, rs);
        cs = _mm256_mul_ps(cs, cs);

        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_castps256_ps128(rs)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_castps256_ps128(cs)));
        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_extractf128_ps(rs, 1)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_extractf128_ps(cs, 1)));
    }

    // horizontal sum
    accRs = _mm256_hadd_pd(accRs, accCs);
    __m128d s = _mm_add_pd(_mm256_castpd256_pd128(accRs), _mm256_extractf128_pd(accRs, 1));

    __m128 t = _mm_cvtpd_ps(s);
    _mm_store_ss(pRsDiff, t);
    _mm_store_ss(pCsDiff, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0,0,0,1)));
}
#endif

void RsCsCalc_diff_SSE4(pmfxF32 pRs0, pmfxF32 pCs0, pmfxF32 pRs1, pmfxF32 pCs1, int wblocks, int hblocks,
                        pmfxF32 pRsDiff, pmfxF32 pCsDiff)
{
    mfxI32 i, len = wblocks * hblocks;
    __m128d accRs = _mm_setzero_pd();
    __m128d accCs = _mm_setzero_pd();

    for (i = 0; i < len - 3; i += 4)
    {
        __m128 rs = _mm_sub_ps(_mm_loadu_ps(&pRs0[i]), _mm_loadu_ps(&pRs1[i]));
        __m128 cs = _mm_sub_ps(_mm_loadu_ps(&pCs0[i]), _mm_loadu_ps(&pCs1[i]));

        rs = _mm_mul_ps(rs, rs);
        cs = _mm_mul_ps(cs, cs);

        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(rs));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(cs));
        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(_mm_movehl_ps(rs, rs)));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(_mm_movehl_ps(cs, cs)));
    }

    if (i < len)
    {
        __m128 rs = _mm_sub_ps(LoadPartialXmm(&pRs0[i], len & 0x3), LoadPartialXmm(&pRs1[i], len & 0x3));
        __m128 cs = _mm_sub_ps(LoadPartialXmm(&pCs0[i], len & 0x3), LoadPartialXmm(&pCs1[i], len & 0x3));

        rs = _mm_mul_ps(rs, rs);
        cs = _mm_mul_ps(cs, cs);

        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(rs));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(cs));
        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(_mm_movehl_ps(rs, rs)));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(_mm_movehl_ps(cs, cs)));
    }

    // horizontal sum
    accRs = _mm_hadd_pd(accRs, accCs);

    __m128 t = _mm_cvtpd_ps(accRs);
    _mm_store_ss(pRsDiff, t);
    _mm_store_ss(pCsDiff, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0,0,0,1)));
}

void RsCsCalc_diff_C(pmfxF32 pRs0, pmfxF32 pCs0, pmfxF32 pRs1, pmfxF32 pCs1, int wblocks, int hblocks,
                     pmfxF32 pRsDiff, pmfxF32 pCsDiff)
{
    mfxI32 len = wblocks * hblocks;
    double accRs = 0.0;
    double accCs = 0.0;

    for (mfxI32 i = 0; i < len; i++)
    {
        accRs += (pRs0[i] - pRs1[i]) * (pRs0[i] - pRs1[i]);
        accCs += (pCs0[i] - pCs1[i]) * (pCs0[i] - pCs1[i]);
    }
    *pRsDiff = (float)accRs;
    *pCsDiff = (float)accCs;
}

/*
 *  ImageDiffHistogram versions
 */
static const int HIST_THRESH_LO = 4;
static const int HIST_THRESH_HI = 12;

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
void ImageDiffHistogram_AVX2(pmfxU8 pSrc, pmfxU8 pRef, int pitch, int width, int height,
                             mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC)
{
    __m256i sDC = _mm256_setzero_si256();
    __m256i rDC = _mm256_setzero_si256();

    __m256i h0 = _mm256_setzero_si256();
    __m256i h1 = _mm256_setzero_si256();
    __m256i h2 = _mm256_setzero_si256();
    __m256i h3 = _mm256_setzero_si256();

    __m256i zero = _mm256_setzero_si256();

    for (mfxI32 i = 0; i < height; i++)
    {
        // process 32 pixels per iteration
        mfxI32 j;
        for (j = 0; j < width - 31; j += 32)
        {
            __m256i s = _mm256_loadu_si256((__m256i *)(&pSrc[j]));
            __m256i r = _mm256_loadu_si256((__m256i *)(&pRef[j]));

            sDC = _mm256_add_epi64(sDC, _mm256_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm256_add_epi64(rDC, _mm256_sad_epu8(r, zero));

            r = _mm256_sub_epi8(r, _mm256_set1_epi8(-128));   // convert to signed
            s = _mm256_sub_epi8(s, _mm256_set1_epi8(-128));

            __m256i dn = _mm256_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m256i dp = _mm256_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m256i m0 = _mm256_cmpgt_epi8(dn, _mm256_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m256i m1 = _mm256_cmpgt_epi8(dn, _mm256_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m256i m2 = _mm256_cmpgt_epi8(_mm256_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m256i m3 = _mm256_cmpgt_epi8(_mm256_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm256_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm256_sub_epi8(zero, m1);
            m2 = _mm256_sub_epi8(zero, m2);
            m3 = _mm256_sub_epi8(zero, m3);

            h0 = _mm256_add_epi32(h0, _mm256_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm256_add_epi32(h1, _mm256_sad_epu8(m1, zero));
            h2 = _mm256_add_epi32(h2, _mm256_sad_epu8(m2, zero));
            h3 = _mm256_add_epi32(h3, _mm256_sad_epu8(m3, zero));
        }

        // process remaining 1..31 pixels
        if (j < width)
        {
            __m256i s = LoadPartialYmm<0>(&pSrc[j], width & 0x1f);
            __m256i r = LoadPartialYmm<0>(&pRef[j], width & 0x1f);

            sDC = _mm256_add_epi64(sDC, _mm256_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm256_add_epi64(rDC, _mm256_sad_epu8(r, zero));

            s = LoadPartialYmm<-1>(&pSrc[j], width & 0x1f);   // ensure unused elements not counted

            r = _mm256_sub_epi8(r, _mm256_set1_epi8(-128));   // convert to signed
            s = _mm256_sub_epi8(s, _mm256_set1_epi8(-128));

            __m256i dn = _mm256_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m256i dp = _mm256_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m256i m0 = _mm256_cmpgt_epi8(dn, _mm256_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m256i m1 = _mm256_cmpgt_epi8(dn, _mm256_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m256i m2 = _mm256_cmpgt_epi8(_mm256_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m256i m3 = _mm256_cmpgt_epi8(_mm256_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm256_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm256_sub_epi8(zero, m1);
            m2 = _mm256_sub_epi8(zero, m2);
            m3 = _mm256_sub_epi8(zero, m3);

            h0 = _mm256_add_epi32(h0, _mm256_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm256_add_epi32(h1, _mm256_sad_epu8(m1, zero));
            h2 = _mm256_add_epi32(h2, _mm256_sad_epu8(m2, zero));
            h3 = _mm256_add_epi32(h3, _mm256_sad_epu8(m3, zero));
        }
        pSrc += pitch;
        pRef += pitch;
    }

    // finish horizontal sums
    __m128i tsDC = _mm_add_epi64(_mm256_castsi256_si128(sDC), _mm256_extractf128_si256(sDC, 1));
    __m128i trDC = _mm_add_epi64(_mm256_castsi256_si128(rDC), _mm256_extractf128_si256(rDC, 1));
    tsDC = _mm_add_epi64(tsDC, _mm_movehl_epi64(tsDC, tsDC));
    trDC = _mm_add_epi64(trDC, _mm_movehl_epi64(trDC, trDC));

    __m128i th0 = _mm_add_epi32(_mm256_castsi256_si128(h0), _mm256_extractf128_si256(h0, 1));
    __m128i th1 = _mm_add_epi32(_mm256_castsi256_si128(h1), _mm256_extractf128_si256(h1, 1));
    __m128i th2 = _mm_add_epi32(_mm256_castsi256_si128(h2), _mm256_extractf128_si256(h2, 1));
    __m128i th3 = _mm_add_epi32(_mm256_castsi256_si128(h3), _mm256_extractf128_si256(h3, 1));
    th0 = _mm_add_epi32(th0, _mm_movehl_epi64(th0, th0));
    th1 = _mm_add_epi32(th1, _mm_movehl_epi64(th1, th1));
    th2 = _mm_add_epi32(th2, _mm_movehl_epi64(th2, th2));
    th3 = _mm_add_epi32(th3, _mm_movehl_epi64(th3, th3));

    _mm_storel_epi64((__m128i *)pSrcDC, tsDC);
    _mm_storel_epi64((__m128i *)pRefDC, trDC);

    histogram[0] = _mm_cvtsi128_si32(th0);
    histogram[1] = _mm_cvtsi128_si32(th1);
    histogram[2] = _mm_cvtsi128_si32(th2);
    histogram[3] = _mm_cvtsi128_si32(th3);
    histogram[4] = width * height;

    // undo cumulative counts, by differencing
    histogram[4] -= histogram[3];
    histogram[3] -= histogram[2];
    histogram[2] -= histogram[1];
    histogram[1] -= histogram[0];
}
#endif

void ImageDiffHistogram_SSE4(pmfxU8 pSrc, pmfxU8 pRef, int pitch, int width, int height,
                             mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC)
{
    __m128i sDC = _mm_setzero_si128();
    __m128i rDC = _mm_setzero_si128();

    __m128i h0 = _mm_setzero_si128();
    __m128i h1 = _mm_setzero_si128();
    __m128i h2 = _mm_setzero_si128();
    __m128i h3 = _mm_setzero_si128();

    __m128i zero = _mm_setzero_si128();

    for (mfxI32 i = 0; i < height; i++)
    {
        // process 16 pixels per iteration
        mfxI32 j;
        for (j = 0; j < width - 15; j += 16)
        {
            __m128i s = _mm_loadu_si128((__m128i *)(&pSrc[j]));
            __m128i r = _mm_loadu_si128((__m128i *)(&pRef[j]));

            sDC = _mm_add_epi64(sDC, _mm_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm_add_epi64(rDC, _mm_sad_epu8(r, zero));

            r = _mm_sub_epi8(r, _mm_set1_epi8(-128));   // convert to signed
            s = _mm_sub_epi8(s, _mm_set1_epi8(-128));

            __m128i dn = _mm_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m128i dp = _mm_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m128i m0 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m128i m1 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m128i m2 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m128i m3 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm_sub_epi8(zero, m1);
            m2 = _mm_sub_epi8(zero, m2);
            m3 = _mm_sub_epi8(zero, m3);

            h0 = _mm_add_epi32(h0, _mm_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm_add_epi32(h1, _mm_sad_epu8(m1, zero));
            h2 = _mm_add_epi32(h2, _mm_sad_epu8(m2, zero));
            h3 = _mm_add_epi32(h3, _mm_sad_epu8(m3, zero));
        }

        // process remaining 1..15 pixels
        if (j < width)
        {
            __m128i s = LoadPartialXmm<0>(&pSrc[j], width & 0xf);
            __m128i r = LoadPartialXmm<0>(&pRef[j], width & 0xf);

            sDC = _mm_add_epi64(sDC, _mm_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm_add_epi64(rDC, _mm_sad_epu8(r, zero));

            s = LoadPartialXmm<-1>(&pSrc[j], width & 0xf);      // ensure unused elements not counted

            r = _mm_sub_epi8(r, _mm_set1_epi8(-128));   // convert to signed
            s = _mm_sub_epi8(s, _mm_set1_epi8(-128));

            __m128i dn = _mm_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m128i dp = _mm_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m128i m0 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m128i m1 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m128i m2 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m128i m3 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm_sub_epi8(zero, m1);
            m2 = _mm_sub_epi8(zero, m2);
            m3 = _mm_sub_epi8(zero, m3);

            h0 = _mm_add_epi32(h0, _mm_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm_add_epi32(h1, _mm_sad_epu8(m1, zero));
            h2 = _mm_add_epi32(h2, _mm_sad_epu8(m2, zero));
            h3 = _mm_add_epi32(h3, _mm_sad_epu8(m3, zero));
        }
        pSrc += pitch;
        pRef += pitch;
    }

    // finish horizontal sums
    sDC = _mm_add_epi64(sDC, _mm_movehl_epi64(sDC, sDC));
    rDC = _mm_add_epi64(rDC, _mm_movehl_epi64(rDC, rDC));

    h0 = _mm_add_epi32(h0, _mm_movehl_epi64(h0, h0));
    h1 = _mm_add_epi32(h1, _mm_movehl_epi64(h1, h1));
    h2 = _mm_add_epi32(h2, _mm_movehl_epi64(h2, h2));
    h3 = _mm_add_epi32(h3, _mm_movehl_epi64(h3, h3));

    _mm_storel_epi64((__m128i *)pSrcDC, sDC);
    _mm_storel_epi64((__m128i *)pRefDC, rDC);

    histogram[0] = _mm_cvtsi128_si32(h0);
    histogram[1] = _mm_cvtsi128_si32(h1);
    histogram[2] = _mm_cvtsi128_si32(h2);
    histogram[3] = _mm_cvtsi128_si32(h3);
    histogram[4] = width * height;

    // undo cumulative counts, by differencing
    histogram[4] -= histogram[3];
    histogram[3] -= histogram[2];
    histogram[2] -= histogram[1];
    histogram[1] -= histogram[0];
}

void ImageDiffHistogram_C(pmfxU8 pSrc, pmfxU8 pRef, int pitch, int width, int height,
                          mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC)
{
    mfxI64 srcDC = 0;
    mfxI64 refDC = 0;

    histogram[0] = 0;
    histogram[1] = 0;
    histogram[2] = 0;
    histogram[3] = 0;
    histogram[4] = 0;

    for (mfxI32 i = 0; i < height; i++)
    {
        for (mfxI32 j = 0; j < width; j++)
        {
            int s = pSrc[j];
            int r = pRef[j];
            int d = s - r;

            srcDC += s;
            refDC += r;

            if (d < -HIST_THRESH_HI)
                histogram[0]++;
            else if (d < -HIST_THRESH_LO)
                histogram[1]++;
            else if (d < HIST_THRESH_LO)
                histogram[2]++;
            else if (d < HIST_THRESH_HI)
                histogram[3]++;
            else
                histogram[4]++;
        }
        pSrc += pitch;
        pRef += pitch;
    }
    *pSrcDC = srcDC;
    *pRefDC = refDC;
}

/*
 * RsCsCalc_4x4 Versions
 */

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
void RsCsCalc_4x4_AVX2(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxF32 pRs, pmfxF32 pCs)
{
    for (mfxI32 i = 0; i < hblocks; i++)
    {
        // 4 horizontal blocks at a time
        mfxI32 j;
        for (j = 0; j < wblocks - 3; j += 4)
        {
            __m256i rs = _mm256_setzero_si256();
            __m256i cs = _mm256_setzero_si256();
            __m256i a0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pSrc[-srcPitch + 0]));

            for (mfxI32 k = 0; k < 4; k++)
            {
                __m256i b0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pSrc[-1]));
                __m256i c0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pSrc[0]));
                pSrc += srcPitch;

                // accRs += dRs * dRs
                a0 = _mm256_sub_epi16(c0, a0);
                a0 = _mm256_madd_epi16(a0, a0);
                rs = _mm256_add_epi32(rs, a0);

                // accCs += dCs * dCs
                b0 = _mm256_sub_epi16(c0, b0);
                b0 = _mm256_madd_epi16(b0, b0);
                cs = _mm256_add_epi32(cs, b0);

                // reuse next iteration
                a0 = c0;
            }

            // horizontal sum
            rs = _mm256_hadd_epi32(rs, cs);
            rs = _mm256_permute4x64_epi64(rs, _MM_SHUFFLE(3, 1, 2, 0));    // [ cs3 cs2 cs1 cs0 rs3 rs2 rs1 rs0 ]

            // scale by 1.0f/N and store
            __m256 t = _mm256_mul_ps(_mm256_cvtepi32_ps(rs), _mm256_set1_ps(1.0f / 16));
            _mm_storeu_ps(&pRs[i * wblocks + j], _mm256_extractf128_ps(t, 0));
            _mm_storeu_ps(&pCs[i * wblocks + j], _mm256_extractf128_ps(t, 1));

            pSrc -= 4 * srcPitch;
            pSrc += 16;
        }

        // remaining blocks
        for (; j < wblocks; j++)
        {
            mfxI32 accRs = 0;
            mfxI32 accCs = 0;

            for (mfxI32 k = 0; k < 4; k++)
            {
                for (mfxI32 l = 0; l < 4; l++)
                {
                    mfxI32 dRs = pSrc[l] - pSrc[l - srcPitch];
                    mfxI32 dCs = pSrc[l] - pSrc[l - 1];
                    accRs += dRs * dRs;
                    accCs += dCs * dCs;
                }
                pSrc += srcPitch;
            }
            pRs[i * wblocks + j] = accRs * (1.0f / 16);
            pCs[i * wblocks + j] = accCs * (1.0f / 16);

            pSrc -= 4 * srcPitch;
            pSrc += 4;
        }
        pSrc -= 4 * wblocks;
        pSrc += 4 * srcPitch;
    }
}
#endif

void RsCsCalc_4x4_SSE4(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxF32 pRs, pmfxF32 pCs)
{
    for (mfxI32 i = 0; i < hblocks; i++)
    {
        // 4 horizontal blocks at a time
        mfxI32 j;
        for (j = 0; j < wblocks - 3; j += 4)
        {
            __m128i rs = _mm_setzero_si128();
            __m128i cs = _mm_setzero_si128();
            __m128i a0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 0]));
            __m128i a1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 8]));

            for (mfxI32 k = 0; k < 4; k++)
            {
                __m128i b0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-1]));
                __m128i b1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[7]));
                __m128i c0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[0]));
                __m128i c1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[8]));
                pSrc += srcPitch;

                // accRs += dRs * dRs
                a0 = _mm_sub_epi16(c0, a0);
                a1 = _mm_sub_epi16(c1, a1);
                a0 = _mm_madd_epi16(a0, a0);
                a1 = _mm_madd_epi16(a1, a1);
                a0 = _mm_hadd_epi32(a0, a1);
                rs = _mm_add_epi32(rs, a0);

                // accCs += dCs * dCs
                b0 = _mm_sub_epi16(c0, b0);
                b1 = _mm_sub_epi16(c1, b1);
                b0 = _mm_madd_epi16(b0, b0);
                b1 = _mm_madd_epi16(b1, b1);
                b0 = _mm_hadd_epi32(b0, b1);
                cs = _mm_add_epi32(cs, b0);

                // reuse next iteration
                a0 = c0;
                a1 = c1;
            }

            // scale by 1.0f/N and store
            _mm_storeu_ps(&pRs[i * wblocks + j], _mm_mul_ps(_mm_cvtepi32_ps(rs), _mm_set1_ps(1.0f / 16)));
            _mm_storeu_ps(&pCs[i * wblocks + j], _mm_mul_ps(_mm_cvtepi32_ps(cs), _mm_set1_ps(1.0f / 16)));

            pSrc -= 4 * srcPitch;
            pSrc += 16;
        }

        // remaining blocks
        for (; j < wblocks; j++)
        {
            mfxI32 accRs = 0;
            mfxI32 accCs = 0;

            for (mfxI32 k = 0; k < 4; k++)
            {
                for (mfxI32 l = 0; l < 4; l++)
                {
                    mfxI32 dRs = pSrc[l] - pSrc[l - srcPitch];
                    mfxI32 dCs = pSrc[l] - pSrc[l - 1];
                    accRs += dRs * dRs;
                    accCs += dCs * dCs;
                }
                pSrc += srcPitch;
            }
            pRs[i * wblocks + j] = accRs * (1.0f / 16);
            pCs[i * wblocks + j] = accCs * (1.0f / 16);

            pSrc -= 4 * srcPitch;
            pSrc += 4;
        }
        pSrc -= 4 * wblocks;
        pSrc += 4 * srcPitch;
    }
}

void RsCsCalc_4x4_C(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxF32 pRs, pmfxF32 pCs)
{
    for (mfxI32 i = 0; i < hblocks; i++)
    {
        for (mfxI32 j = 0; j < wblocks; j++)
        {
            mfxI32 accRs = 0;
            mfxI32 accCs = 0;

            for (mfxI32 k = 0; k < 4; k++)
            {
                for (mfxI32 l = 0; l < 4; l++)
                {
                    mfxI32 dRs = pSrc[l] - pSrc[l - srcPitch];
                    mfxI32 dCs = pSrc[l] - pSrc[l - 1];
                    accRs += dRs * dRs;
                    accCs += dCs * dCs;
                }
                pSrc += srcPitch;
            }
            pRs[i * wblocks + j] = accRs * (1.0f / 16);
            pCs[i * wblocks + j] = accCs * (1.0f / 16);

            pSrc -= 4 * srcPitch;
            pSrc += 4;
        }
        pSrc -= 4 * wblocks;
        pSrc += 4 * srcPitch;
    }
}

/*
 * RsCsCalc_sqrt2 Versions
 */

#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
void RsCsCalc_sqrt2_AVX2(pmfxF32 pRs, pmfxF32 pCs, pmfxF32 pRsCs, pmfxF32 pRsFrame, pmfxF32 pCsFrame, int wblocks, int hblocks)
{
    mfxI32 i, len = wblocks * hblocks;
    __m256d accRs = _mm256_setzero_pd();
    __m256d accCs = _mm256_setzero_pd();
    __m256 zero = _mm256_setzero_ps();

    for (i = 0; i < len - 7; i += 8)
    {
        __m256 rs = _mm256_loadu_ps(&pRs[i]);
        __m256 cs = _mm256_loadu_ps(&pCs[i]);
        __m256 rc = _mm256_add_ps(rs, cs);

        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_castps256_ps128(rs)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_castps256_ps128(cs)));
        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_extractf128_ps(rs, 1)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_extractf128_ps(cs, 1)));

        // x = x * rsqrt(x), with rsqrt(0.0f) flushed to 0.0f
        rs = _mm256_mul_ps(rs, _mm256_andnot_ps(_mm256_cmp_ps(rs, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(rs)));
        cs = _mm256_mul_ps(cs, _mm256_andnot_ps(_mm256_cmp_ps(cs, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(cs)));
        rc = _mm256_mul_ps(rc, _mm256_andnot_ps(_mm256_cmp_ps(rc, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(rc)));

        _mm256_storeu_ps(&pRs[i], rs);
        _mm256_storeu_ps(&pCs[i], cs);
        _mm256_storeu_ps(&pRsCs[i], rc);
    }

    if (i < len)
    {
        __m256 rs = LoadPartialYmm(&pRs[i], len & 0x7);
        __m256 cs = LoadPartialYmm(&pCs[i], len & 0x7);
        __m256 rc = _mm256_add_ps(rs, cs);

        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_castps256_ps128(rs)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_castps256_ps128(cs)));
        accRs = _mm256_add_pd(accRs, _mm256_cvtps_pd(_mm256_extractf128_ps(rs, 1)));
        accCs = _mm256_add_pd(accCs, _mm256_cvtps_pd(_mm256_extractf128_ps(cs, 1)));

        // x = x * rsqrt(x), with rsqrt(0.0f) flushed to 0.0f
        rs = _mm256_mul_ps(rs, _mm256_andnot_ps(_mm256_cmp_ps(rs, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(rs)));
        cs = _mm256_mul_ps(cs, _mm256_andnot_ps(_mm256_cmp_ps(cs, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(cs)));
        rc = _mm256_mul_ps(rc, _mm256_andnot_ps(_mm256_cmp_ps(rc, zero, _CMP_EQ_OQ), _mm256_rsqrt_ps(rc)));

        StorePartialYmm(&pRs[i], rs, len & 0x7);
        StorePartialYmm(&pCs[i], cs, len & 0x7);
        StorePartialYmm(&pRsCs[i], rc, len & 0x7);
    }

    // horizontal sum
    accRs = _mm256_hadd_pd(accRs, accCs);
    __m128d s = _mm_add_pd(_mm256_castpd256_pd128(accRs), _mm256_extractf128_pd(accRs, 1));

    s = _mm_sqrt_pd(_mm_div_pd(s, _mm_set1_pd(hblocks * wblocks)));

    __m128 t = _mm_cvtpd_ps(s);
    _mm_store_ss(pRsFrame, t);
    _mm_store_ss(pCsFrame, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0,0,0,1)));
}
#endif

void RsCsCalc_sqrt2_SSE4(pmfxF32 pRs, pmfxF32 pCs, pmfxF32 pRsCs, pmfxF32 pRsFrame, pmfxF32 pCsFrame, int wblocks, int hblocks)
{
    mfxI32 i, len = wblocks * hblocks;
    __m128d accRs = _mm_setzero_pd();
    __m128d accCs = _mm_setzero_pd();
    __m128 zero = _mm_setzero_ps();

    for (i = 0; i < len - 3; i += 4)
    {
        __m128 rs = _mm_loadu_ps(&pRs[i]);
        __m128 cs = _mm_loadu_ps(&pCs[i]);
        __m128 rc = _mm_add_ps(rs, cs);

        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(rs));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(cs));
        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(_mm_movehl_ps(rs, rs)));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(_mm_movehl_ps(cs, cs)));

        // x = x * rsqrt(x), with rsqrt(0.0f) flushed to 0.0f
        rs = _mm_mul_ps(rs, _mm_andnot_ps(_mm_cmpeq_ps(rs, zero), _mm_rsqrt_ps(rs)));
        cs = _mm_mul_ps(cs, _mm_andnot_ps(_mm_cmpeq_ps(cs, zero), _mm_rsqrt_ps(cs)));
        rc = _mm_mul_ps(rc, _mm_andnot_ps(_mm_cmpeq_ps(rc, zero), _mm_rsqrt_ps(rc)));

        _mm_storeu_ps(&pRs[i], rs);
        _mm_storeu_ps(&pCs[i], cs);
        _mm_storeu_ps(&pRsCs[i], rc);
    }

    if (i < len)
    {
        __m128 rs = LoadPartialXmm(&pRs[i], len & 0x3);
        __m128 cs = LoadPartialXmm(&pCs[i], len & 0x3);
        __m128 rc = _mm_add_ps(rs, cs);

        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(rs));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(cs));
        accRs = _mm_add_pd(accRs, _mm_cvtps_pd(_mm_movehl_ps(rs, rs)));
        accCs = _mm_add_pd(accCs, _mm_cvtps_pd(_mm_movehl_ps(cs, cs)));

        // x = x * rsqrt(x), with rsqrt(0.0f) flushed to 0.0f
        rs = _mm_mul_ps(rs, _mm_andnot_ps(_mm_cmpeq_ps(rs, zero), _mm_rsqrt_ps(rs)));
        cs = _mm_mul_ps(cs, _mm_andnot_ps(_mm_cmpeq_ps(cs, zero), _mm_rsqrt_ps(cs)));
        rc = _mm_mul_ps(rc, _mm_andnot_ps(_mm_cmpeq_ps(rc, zero), _mm_rsqrt_ps(rc)));

        StorePartialXmm(&pRs[i], rs, len & 0x3);
        StorePartialXmm(&pCs[i], cs, len & 0x3);
        StorePartialXmm(&pRsCs[i], rc, len & 0x3);
    }

    // horizontal sum
    accRs = _mm_hadd_pd(accRs, accCs);

    accRs = _mm_sqrt_pd(_mm_div_pd(accRs, _mm_set1_pd(hblocks * wblocks)));

    __m128 t = _mm_cvtpd_ps(accRs);
    _mm_store_ss(pRsFrame, t);
    _mm_store_ss(pCsFrame, _mm_shuffle_ps(t, t, _MM_SHUFFLE(0,0,0,1)));
}

void RsCsCalc_sqrt2_C(pmfxF32 pRs, pmfxF32 pCs, pmfxF32 pRsCs, pmfxF32 pRsFrame, pmfxF32 pCsFrame, int wblocks, int hblocks)
{
    mfxI32 len = wblocks * hblocks;
    double accRs = 0.0;
    double accCs = 0.0;

    for (mfxI32 i = 0; i < len; i++)
    {
        float rs = pRs[i];
        float cs = pCs[i];
        accRs += rs;
        accCs += cs;
        pRs[i] = sqrt(rs);
        pCs[i] = sqrt(cs);
        pRsCs[i] = sqrt(rs + cs);
    }

    *pRsFrame = (float)sqrt(accRs / (hblocks * wblocks));
    *pCsFrame = (float)sqrt(accCs / (hblocks * wblocks));
}


void SceneChangeDetector::RsCsCalc(imageData *exBuffer, ImDetails vidCar) {
    pmfxU8
        ss = exBuffer->Image.Y;
    mfxI32
        hblocks = (mfxI32)vidCar._cheight / BLOCK_SIZE,
        wblocks = (mfxI32)vidCar._cwidth / BLOCK_SIZE;

    SCD_CPU_DISP(m_cpuOpt, RsCsCalc_4x4, ss, vidCar.Extended_Width, wblocks, hblocks, exBuffer->Rs, exBuffer->Cs);
    SCD_CPU_DISP(m_cpuOpt, RsCsCalc_sqrt2, exBuffer->Rs, exBuffer->Cs, exBuffer->RsCs, &exBuffer->RsVal, &exBuffer->CsVal, wblocks, hblocks);

    ss = exBuffer->Image.Y;
    mfxI32
        sumAll = 0;
    for(mfxI32 i = 0; i < vidCar._cheight; i++) {
        for(mfxI32 j = 0; j < vidCar._cwidth; j++)
            sumAll += ss[j];
        ss += vidCar.Extended_Width;
    }
    exBuffer->avgval = (mfxF32)((mfxF64)(sumAll)/vidCar._cwidth/vidCar._cheight);
}

/// Provide input surface to scene change detector
// input frame to SCD is subWidth x subHeight
mfxStatus SceneChangeDetector::MapFrame(mfxFrameSurface1 *frame)
{
    MFX_CHECK_NULL_PTR1(frame);

    CmSurface2D *surface;

    int res = 0;
    int streamParity = Get_stream_parity();
    int ouputWidth = subWidth;
    int subImage_height = gpustep_h;

    std::map<void *, CmSurface2D *>::iterator it;
    it = m_tableCmRelations.find(frame->Data.MemId);

    if (m_tableCmRelations.end() == it)
    {
        res = m_pCmDevice->CreateSurface2D(frame->Data.MemId, surface);
        if (res!=0)
        {
            return MFX_ERR_DEVICE_FAILED;
        }
        m_tableCmRelations.insert(std::pair<void *, CmSurface2D *>(frame->Data.MemId, surface));
    }
    else
    {
        surface = it->second;
    }

    // Get surface and Buffer index
    SurfaceIndex * pSurfaceIndex = NULL;
    SurfaceIndex * pOuputBufferIndex = NULL;
    res = surface->GetIndex(pSurfaceIndex);
    if (res!=0) return MFX_ERR_DEVICE_FAILED;
    res = m_pCmBufferOut->GetIndex(pOuputBufferIndex);
    if (res!=0) return MFX_ERR_DEVICE_FAILED;

    for (CmKernel* pCmKernel : { m_pCmKernelTopField, m_pCmKernelBotField, m_pCmKernelFrame })
    {
        if (!pCmKernel)
            continue;

        subImage_height = (m_pCmKernelFrame == pCmKernel) ? gpustep_h : gpustep_h / 2;

        res = pCmKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurfaceIndex);
        if (res!=0) return MFX_ERR_DEVICE_FAILED;
        res = pCmKernel->SetKernelArg(1, sizeof(SurfaceIndex), pOuputBufferIndex);
        if (res!=0) return MFX_ERR_DEVICE_FAILED;
        res = pCmKernel->SetKernelArg(2, sizeof(int), &ouputWidth);
        if (res!=0) return MFX_ERR_DEVICE_FAILED;
        res = pCmKernel->SetKernelArg(3, sizeof(int), &gpustep_w);
        if (res!=0) return MFX_ERR_DEVICE_FAILED;
        res = pCmKernel->SetKernelArg(4, sizeof(int), &subImage_height);
        if (res!=0) return MFX_ERR_DEVICE_FAILED;
    }

    return MFX_ERR_NONE;
}

void SceneChangeDetector::Setup_UFast_Environment() {

    static mfxU32 scaleVal = 8;

    m_dataIn->accuracy = 1;

    m_dataIn->layer[0].Original_Width  = SMALL_WIDTH;
    m_dataIn->layer[0].Original_Height = SMALL_HEIGHT;
    m_dataIn->layer[0]._cwidth  = SMALL_WIDTH;
    m_dataIn->layer[0]._cheight = SMALL_HEIGHT;

    m_dataIn->layer[0].block_width = 8;
    m_dataIn->layer[0].block_height = 8;
    m_dataIn->layer[0].vertical_pad = 16 / scaleVal;
    m_dataIn->layer[0].horizontal_pad = 16 / scaleVal;
    m_dataIn->layer[0].Extended_Height = m_dataIn->layer[0].vertical_pad + m_dataIn->layer[0]._cheight + m_dataIn->layer[0].vertical_pad;
    m_dataIn->layer[0].Extended_Width = m_dataIn->layer[0].horizontal_pad + m_dataIn->layer[0]._cwidth + m_dataIn->layer[0].horizontal_pad;
    m_dataIn->layer[0].pitch = m_dataIn->layer[0].Extended_Width;
    m_dataIn->layer[0].Total_non_corrected_pixels = m_dataIn->layer[0].Original_Height * m_dataIn->layer[0].Original_Width * 6 / CHROMASUBSAMPLE;
    m_dataIn->layer[0].Pixels_in_Y_layer = m_dataIn->layer[0]._cwidth * m_dataIn->layer[0]._cheight;
    m_dataIn->layer[0].Pixels_in_U_layer = m_dataIn->layer[0].Pixels_in_Y_layer / CHROMASUBSAMPLE;
    m_dataIn->layer[0].Pixels_in_V_layer = m_dataIn->layer[0].Pixels_in_Y_layer / CHROMASUBSAMPLE;
    m_dataIn->layer[0].Pixels_in_full_frame = m_dataIn->layer[0].Pixels_in_Y_layer + m_dataIn->layer[0].Pixels_in_U_layer + m_dataIn->layer[0].Pixels_in_V_layer;
    m_dataIn->layer[0].Pixels_in_block = m_dataIn->layer[0].block_height * m_dataIn->layer[0].block_width;
    m_dataIn->layer[0].Height_in_blocks = m_dataIn->layer[0]._cheight / m_dataIn->layer[0].block_height;
    m_dataIn->layer[0].Width_in_blocks = m_dataIn->layer[0]._cwidth / m_dataIn->layer[0].block_width;
    m_dataIn->layer[0].Blocks_in_Frame = (m_dataIn->layer[0]._cheight / m_dataIn->layer[0].block_height) * (m_dataIn->layer[0]._cwidth / m_dataIn->layer[0].block_width);
    m_dataIn->layer[0].sidesize = m_dataIn->layer[0]._cheight + (1 * m_dataIn->layer[0].vertical_pad);
    m_dataIn->layer[0].initial_point = (m_dataIn->layer[0].Extended_Width * m_dataIn->layer[0].vertical_pad) + m_dataIn->layer[0].horizontal_pad;
    m_dataIn->layer[0].endPoint = ((m_dataIn->layer[0]._cheight + m_dataIn->layer[0].vertical_pad) * m_dataIn->layer[0].Extended_Width) - m_dataIn->layer[0].horizontal_pad;
    m_dataIn->layer[0].MVspaceSize = (m_dataIn->layer[0]._cheight / m_dataIn->layer[0].block_height) * (m_dataIn->layer[0]._cwidth / m_dataIn->layer[0].block_width);
}

void SceneChangeDetector::ImDetails_Init(ImDetails *Rdata) {
    Rdata->block_height = 0;
    Rdata->block_width = 0;
    Rdata->horizontal_pad = 0;
    Rdata->vertical_pad = 0;
    Rdata->Original_Height = 0;
    Rdata->Original_Width = 0;
    Rdata->Pixels_in_Y_layer = 0;
    Rdata->Pixels_in_U_layer = 0;
    Rdata->Pixels_in_V_layer = 0;
    Rdata->Pixels_in_full_frame = 0;
    Rdata->Blocks_in_Frame = 0;
    Rdata->endPoint = 0;
    Rdata->Extended_Width = 0;
    Rdata->Extended_Height = 0;
    Rdata->Height_in_blocks = 0;
    Rdata->initial_point = 0;
    Rdata->MVspaceSize = 0;
    Rdata->pitch = 0;
    Rdata->Pixels_in_block = 0;
    Rdata->sidesize = 0;
    Rdata->Total_non_corrected_pixels = 0;
    Rdata->Width_in_blocks = 0;
    Rdata->_cheight = 0;
    Rdata->_cwidth = 0;
}

void SceneChangeDetector::Params_Init() {
    m_dataIn->accuracy  = 1;
    m_dataIn->processed_frames = 0;
    m_dataIn->total_number_of_frames = -1;
    m_dataIn->starting_frame = 0;
    m_dataIn->key_frame_frequency = INT_MAX;
    m_dataIn->limitRange = 0;
    m_dataIn->maxXrange = 32;
    m_dataIn->maxYrange = 32;
    m_dataIn->interlaceMode = 0;
    m_dataIn->StartingField = TopField;
    m_dataIn->currentField  = TopField;

    ImDetails_Init(&m_dataIn->layer[0]);
}


/// Detect new scene
/*!
 This function detects scene change in previous field.

 Decision tree is coded in SCDetectGPU().

 \param[in] Data
 input image
 \param[in] DataRef
 reference picture
 \param[in] imageInfo
 image info
 \param[in] current
 statistics for current buffer (Temporal Spatial Complexity)
 \param[in] reference
 statistics for reference buffer (Temporal Spatial Complexity)

 \param[out] current->Schg
 scene change detection for previous field
 If videoData[Current_Frame]->frame_number = N and current->Schg = 1,
 a new scene occured at Fild number N-1.
*/
void SceneChangeDetector::ShotDetect(imageData Data, imageData DataRef, ImDetails imageInfo, TSCstat *current, TSCstat *reference)
{
    pmfxU8
        ssFrame = Data.Image.Y,
        refFrame = DataRef.Image.Y;
    pmfxF32
        objRs = Data.Rs,
        objCs = Data.Cs,
        refRs = DataRef.Rs,
        refCs = DataRef.Cs;
    mfxF64
        histTOT = 0.0;

    current->RsCsDiff = 0.0;
    current->Schg = -1;
    current->Gchg = 0;

    SCD_CPU_DISP(m_cpuOpt, RsCsCalc_diff, objRs, objCs, refRs, refCs, 2*imageInfo.Width_in_blocks, 2*imageInfo.Height_in_blocks, &current->RsDiff, &current->CsDiff);
    SCD_CPU_DISP(m_cpuOpt, ImageDiffHistogram, ssFrame, refFrame, imageInfo.Extended_Width, imageInfo._cwidth, imageInfo._cheight, current->histogram, &current->ssDCint, &current->refDCint);

    if(reference->Schg)
        current->last_shot_distance = 1;
    else
        current->last_shot_distance++;

    current->RsDiff /= imageInfo.Blocks_in_Frame * (2 / (8 / imageInfo.block_height)) * (2 / (8 / imageInfo.block_width));
    current->CsDiff /= imageInfo.Blocks_in_Frame * (2 / (8 / imageInfo.block_height)) * (2 / (8 / imageInfo.block_width));
    current->RsCsDiff = (mfxF32)sqrt((current->RsDiff*current->RsDiff) + (current->CsDiff*current->CsDiff));
    histTOT = (mfxF32)(current->histogram[0] + current->histogram[1] + current->histogram[2] + current->histogram[3] + current->histogram[4]);
    current->ssDCval = current->ssDCint * (1.0 / imageInfo.Pixels_in_Y_layer);
    current->refDCval = current->refDCint * (1.0 / imageInfo.Pixels_in_Y_layer);
    current->gchDC = NABS(current->ssDCval - current->refDCval);
    current->posBalance = (mfxF32)(current->histogram[3] + current->histogram[4]);
    current->negBalance = (mfxF32)(current->histogram[0] + current->histogram[1]);
    current->posBalance -= current->negBalance;
    current->posBalance = NABS(current->posBalance);
    current->posBalance /= histTOT;
    current->negBalance /= histTOT;
    if ((current->RsCsDiff > 7.6) && (current->RsCsDiff < 10) && (current->gchDC > 2.9) && (current->posBalance > 0.18))
        current->Gchg = TRUE;
    if ((current->RsCsDiff >= 10) && (current->gchDC > 1.6) && (current->posBalance > 0.18))
        current->Gchg = TRUE;
    current->diffAFD = current->AFD - reference->AFD;
    current->diffTSC = current->TSC - reference->TSC;
    current->diffRsCsDiff = current->RsCsDiff - reference->RsCsDiff;
    current->diffMVdiffVal = current->MVdiffVal - reference->MVdiffVal;
    current->Schg = SCDetectGPU(current->diffMVdiffVal, current->RsCsDiff, current->MVdiffVal, current->Rs, current->AFD, current->CsDiff, current->diffTSC, current->TSC, current->gchDC, current->diffRsCsDiff, current->posBalance, current->SC, current->TSCindex, current->SCindex, current->Cs, current->diffAFD, current->negBalance, current->ssDCval, current->refDCval, current->RsDiff);
    reference->lastFrameInShot = (mfxU8)current->Schg;
}


void CorrectionForGoPSize(VidRead *support, mfxU32 PdIndex) {
    support->detectedSch = 0;
    if(support->logic[PdIndex]->Schg) {
        if(support->lastSCdetectionDistance % support->gopSize)
            support->pendingSch = 1;
        else {
            support->lastSCdetectionDistance = 0;
            support->pendingSch = 0;
            support->detectedSch = 1;
        }
    }
    else if(support->pendingSch) {
        if(!(support->lastSCdetectionDistance % support->gopSize)) {
            support->lastSCdetectionDistance = 0;
            support->pendingSch = 0;
            support->detectedSch = 1;
        }
    }
    support->lastSCdetectionDistance++;
}

BOOL SceneChangeDetector::CompareStats(mfxU8 current, mfxU8 reference, BOOL isInterlaced){
    if(current > 2 || reference > 2 || current == reference) {
        printf("SCD Error: Invalid stats comparison\n");
        exit(-666);
    }
    mfxU8 comparison = 0;
    comparison += ::abs(support->logic[current]->Rs - support->logic[reference]->Rs) < 0.075;
    comparison += ::abs(support->logic[current]->Cs - support->logic[reference]->Cs) < 0.075;
    comparison += ::abs(support->logic[current]->SC - support->logic[reference]->SC) < 0.075;
    comparison += (!isInterlaced && support->logic[current]->TSCindex == 0) || isInterlaced;
    if(comparison == 4)
        return Same;
    return Not_same;
}

BOOL SceneChangeDetector::FrameRepeatCheck(BOOL isInterlaced) {
    mfxU8 reference = previous_frame_data;
    if(isInterlaced)
        reference = previous_previous_frame_data;
    return(CompareStats(current_frame_data, reference, isInterlaced));
}

void SceneChangeDetector::processFrame() {
    support->logic[current_frame_data]->frameNum   = videoData[Current_Frame]->frame_number;
    support->logic[current_frame_data]->firstFrame = support->firstFrame;
    support->logic[current_frame_data]->avgVal     = videoData[Current_Frame]->layer[0].avgval;
    /*---------RsCs data--------*/
    support->logic[current_frame_data]->Rs = videoData[Current_Frame]->layer[0].RsVal;
    support->logic[current_frame_data]->Cs = videoData[Current_Frame]->layer[0].CsVal;
    support->logic[current_frame_data]->SC = (mfxF32)sqrt(( videoData[Current_Frame]->layer[0].RsVal * videoData[Current_Frame]->layer[0].RsVal) + (videoData[Current_Frame]->layer[0].CsVal * videoData[Current_Frame]->layer[0].CsVal));
    if (support->firstFrame) {
        support->logic[current_frame_data]->TSC                = 0;
        support->logic[current_frame_data]->AFD                = 0;
        support->logic[current_frame_data]->TSCindex           = 0;
        support->logic[current_frame_data]->SCindex            = 0;
        support->logic[current_frame_data]->Schg               = 0;
        support->logic[current_frame_data]->Gchg               = 0;
        support->logic[current_frame_data]->picType            = 0;
        support->logic[current_frame_data]->lastFrameInShot    = 0;
        support->logic[current_frame_data]->pdist              = 0;
        support->logic[current_frame_data]->MVdiffVal          = 0;
        support->logic[current_frame_data]->RsCsDiff           = 0;
        support->logic[current_frame_data]->last_shot_distance = 0;
        support->firstFrame = FALSE;
    }
    else {
        /*--------Motion data-------*/
        MotionAnalysis(m_dataIn, videoData[Current_Frame], videoData[Reference_Frame], &support->logic[current_frame_data]->TSC, &support->logic[current_frame_data]->AFD, &support->logic[current_frame_data]->MVdiffVal, (Layers)0);
        support->logic[current_frame_data]->TSCindex = TableLookUp(NumTSC, lmt_tsc2, support->logic[current_frame_data]->TSC);
        support->logic[current_frame_data]->SCindex = TableLookUp(NumSC, lmt_sc2, support->logic[current_frame_data]->SC);
        support->logic[current_frame_data]->pdist = support->PDistanceTable[(support->logic[current_frame_data]->TSCindex * NumSC) + support->logic[current_frame_data]->SCindex];
        /*------Shot Detection------*/
        ShotDetect(videoData[Current_Frame]->layer[0], videoData[Reference_Frame]->layer[0], m_dataIn->layer[0], support->logic[current_frame_data], support->logic[previous_frame_data]);
        support->logic[current_frame_data]->repeatedFrame = FrameRepeatCheck(m_dataIn->interlaceMode != progressive_frame);
    }
    m_dataIn->processed_frames++;
    CorrectionForGoPSize(support, current_frame_data);
}

void SceneChangeDetector::GeneralBufferRotation() {
    VidSample      *videoTransfer;
    videoTransfer = videoData[0];
    videoData[0]  = videoData[1];
    videoData[1]  = videoTransfer;
    TSCstat        *metaTransfer;
    metaTransfer  = support->logic[previous_previous_frame_data];
    support->logic[previous_previous_frame_data] = support->logic[previous_frame_data];
    support->logic[previous_frame_data]          = support->logic[current_frame_data];
    support->logic[current_frame_data]           = metaTransfer;
}

BOOL SceneChangeDetector::ProcessField()
{
    PutFrameInterlaced();
    return(dataReady);
}

mfxU32 SceneChangeDetector::Get_frame_number() {
    if(dataReady)
        return support->logic[previous_previous_frame_data]->frameNum;
    else
        return 0;
}

mfxU32 SceneChangeDetector::Get_frame_shot_Decision() {
    if(dataReady)
        return support->logic[previous_previous_frame_data]->Schg;
    else
        return 0;
}

mfxU32 SceneChangeDetector::Get_frame_last_in_scene() {
    if(dataReady)
        return support->logic[previous_previous_frame_data]->lastFrameInShot;
    else
        return 0;
}

BOOL SceneChangeDetector::Query_is_frame_repeated() {
    if(dataReady)
        return support->logic[previous_previous_frame_data]->repeatedFrame;
    else
        return 0;
}

BOOL SceneChangeDetector::Get_Last_frame_Data() {
    if(dataReady)
        GeneralBufferRotation();

    return(dataReady);
}

/// function return Progressive, TFF or BFF
mfxI32 SceneChangeDetector::Get_stream_parity()
{
    if (!m_dataIn->interlaceMode)
        return (progressive_frame);
    else if (m_dataIn->currentField == TopField)
        return (topfieldfirst_frame);
    else
        return (bottomfieldFirst_frame);
}

/// Get input data to scene change detector for one interlace field
/*!
 This function gets the image data for one field as an 64x112 picture to perform scene detection
 input data is contained in videoData[Current_Frame]->layer[0].Image.Y (stride is 116)

 \param parity [in]
 tells if it is first or second field
 \returns 1 is data is ready
 */
BOOL SceneChangeDetector::RunFrame(mfxU32 parity){
    videoData[Current_Frame]->frame_number = videoData[Reference_Frame]->frame_number + 1;
    GpuSubSampleASC_Image(_width, _height, _pitch, (Layers)0, parity);
    RsCsCalc(&videoData[Current_Frame]->layer[0], m_dataIn->layer[0]);
    processFrame();
    GeneralBufferRotation();
    return(!support->logic[previous_frame_data]->firstFrame);
}

void SceneChangeDetector::PutFrameProgressive() {
    dataReady = RunFrame(TopField);
}

void SceneChangeDetector::SetInterlaceMode(mfxU32 interlaceMode)
{
    m_dataIn->interlaceMode = interlaceMode;
    m_dataIn->StartingField = TopField;
    if(m_dataIn->interlaceMode != progressive_frame)
    {
        if(m_dataIn->interlaceMode == bottomfieldFirst_frame)
            m_dataIn->StartingField = BottomField;
    }
    m_dataIn->currentField = m_dataIn->StartingField;
}

void SceneChangeDetector::SetParityTFF()
{
    SetInterlaceMode(topfieldfirst_frame);
}

void SceneChangeDetector::SetParityBFF()
{
    SetInterlaceMode(bottomfieldFirst_frame);
}

void SceneChangeDetector::SetProgressiveOp()
{
    SetInterlaceMode(progressive_frame);
}

void SceneChangeDetector::PutFrameInterlaced()
{
    dataReady = RunFrame(m_dataIn->currentField);
    SetNextField();
}

mfxI32 logBase2aligned(mfxI32 number) {
    mfxI32 data = (mfxI32)(ceil((log((double)number) / LN2)));
    return (mfxI32)pow(2,(double)data);
}

void U8AllocandSet(pmfxU8 *ImageLayer, mfxI32 imageSize) {
#if ! defined(MFX_VA_LINUX)
    *ImageLayer = (pmfxU8) _aligned_malloc(imageSize, 16);
#else
    *ImageLayer = (pmfxU8) memalign(16, imageSize);
#endif
    if(*ImageLayer == NULL)
        exit(MEMALLOCERRORU8);
    else
        memset(*ImageLayer,0,imageSize);
}

void PdYuvImage_Alloc(YUV *pImage, mfxI32 dimVideoExtended) {
    mfxI32
        dimAligned;
    dimAligned = logBase2aligned(dimVideoExtended);
    U8AllocandSet(&pImage->data,dimAligned);
}

void PdMVector_Alloc(MVector **MV, mfxI32 mvArraysize){
    mfxI32
        MVspace = sizeof(MVector) * mvArraysize;
    *MV = new MVector[MVspace];//(MVector*) malloc(MVspace);
    if(*MV == NULL)
        exit(MEMALLOCERRORMV);
    else
        memset(*MV,0,MVspace);
}

void PdRsCs_Alloc(pmfxF32 *RCs, mfxI32 mvArraysize) {
    mfxI32
        MVspace = sizeof(mfxF32) * mvArraysize;
    *RCs = (pmfxF32) malloc(MVspace);
    if(*RCs == NULL)
        exit(MEMALLOCERROR);
    else
        memset(*RCs,0,MVspace);
}

void PdSAD_Alloc(pmfxU32 *SAD, mfxI32 mvArraysize) {
    mfxI32
        MVspace = sizeof(mfxI32) * mvArraysize;
    *SAD = (pmfxU32) malloc(MVspace);
    if(*SAD == NULL)
        exit(MEMALLOCERROR);
    else
        memset(*SAD,0,MVspace);
}

void Pdmem_AllocGeneral(imageData *Buffer, ImDetails Rval) {
    /*YUV frame mem allocation*/
    PdYuvImage_Alloc(&(Buffer->Image), Rval.Extended_Width * Rval.Extended_Height);
    Buffer->Image.Y = Buffer->Image.data + Rval.initial_point;

    /*Motion Vector array mem allocation*/
    PdMVector_Alloc(&(Buffer->pInteger),Rval.Blocks_in_Frame);

    /*RsCs array mem allocation*/
    PdRsCs_Alloc(&(Buffer->Cs),Rval.Pixels_in_Y_layer / 16);
    PdRsCs_Alloc(&(Buffer->Rs),Rval.Pixels_in_Y_layer / 16);
    PdRsCs_Alloc(&(Buffer->RsCs),Rval.Pixels_in_Y_layer / 16);

    /*SAD array mem allocation*/
    PdSAD_Alloc(&(Buffer->SAD),Rval.Blocks_in_Frame);
}


void SceneChangeDetector::VidSample_Alloc() {
    for(mfxI32 i = 0; i < 2; i++)
        Pdmem_AllocGeneral(&(videoData[i]->layer[0]),m_dataIn->layer[0]);
#if ! defined(MFX_VA_LINUX)
    videoData[2]->layer[0].Image.data = (mfxU8*)_aligned_malloc(m_dataIn->layer[0].Extended_Width * m_dataIn->layer[0].Extended_Height, 0x1000);
#else
    videoData[2]->layer[0].Image.data = (mfxU8*)memalign(0x1000, m_dataIn->layer[0].Extended_Width * m_dataIn->layer[0].Extended_Height);
#endif
    videoData[2]->layer[0].Image.Y = videoData[2]->layer[0].Image.data;
}

void Pdmem_disposeGeneral(imageData *Buffer) {
    free(Buffer->Cs);
    Buffer->Cs = NULL;
    free(Buffer->Rs);
    Buffer->Rs = NULL;
    free(Buffer->RsCs);
    Buffer->RsCs = NULL;
    //free(Buffer->pInteger);
    delete[] Buffer->pInteger;
    Buffer->pInteger = NULL;
    free(Buffer->SAD);
    Buffer->SAD = NULL;
    Buffer->Image.Y = NULL;
    Buffer->Image.U = NULL;
    Buffer->Image.V = NULL;
#if ! defined(MFX_VA_LINUX)
    _aligned_free(Buffer->Image.data);
#else
    free(Buffer->Image.data);
#endif
    Buffer->Image.data = NULL;

}

void SceneChangeDetector::VidSample_dispose() {
    // videoData[0] and videoData[1] have image data and statistics
    for(mfxI32 i = (SAMPLE_VIDEO_INPUT_INDEX - 1); i >= 0; i--) {
        if(videoData[i] != NULL) {
            Pdmem_disposeGeneral(videoData[i]->layer);
            delete videoData[i]->layer;
            delete (videoData[i]);
        }
    }
    // videoData[2] has data only
#if ! defined(MFX_VA_LINUX)
    _aligned_free(videoData[SAMPLE_VIDEO_INPUT_INDEX]->layer[0].Image.data);
#else
    free(videoData[SAMPLE_VIDEO_INPUT_INDEX]->layer[0].Image.data);
#endif
    videoData[SAMPLE_VIDEO_INPUT_INDEX]->layer[0].Image.data = NULL;
    delete videoData[SAMPLE_VIDEO_INPUT_INDEX]->layer;
    delete (videoData[SAMPLE_VIDEO_INPUT_INDEX]);
}

void SceneChangeDetector::VidRead_dispose() {
    if(support->logic != NULL) {
        for(mfxI32 i = 0; i < TSCSTATBUFFER; i++)
            delete support->logic[i];
        delete[] support->logic;
    }
    if(support->gainCorrection.Image.data != NULL)
        Pdmem_disposeGeneral(&support->gainCorrection);
}

void SceneChangeDetector::alloc() {
    VidSample_Alloc();
}

void SceneChangeDetector::IO_Setup() {
    alloc();
}

void TSCstat_Init(TSCstat **logic) {
    for(int i = 0; i < TSCSTATBUFFER; i++)
    {
        logic[i] = new TSCstat;
        logic[i]->AFD = 0.0;
        logic[i]->Cs = 0.0;
        logic[i]->CsDiff = 0.0;
        logic[i]->diffAFD = 0.0;
        logic[i]->diffMVdiffVal = 0.0;
        logic[i]->diffRsCsDiff = 0.0;
        logic[i]->diffTSC = 0.0;
        logic[i]->frameNum = 0;
        logic[i]->gchDC = 0.0;
        logic[i]->Gchg = 0;
        for(int j = 0; j < 5; j++)
            logic[i]->histogram[j] = 0;
        logic[i]->MVdiffVal = 0.0;
        logic[i]->negBalance = 0.0;
        logic[i]->pdist = 0;
        logic[i]->picType = 0;
        logic[i]->posBalance = 0.0;
        logic[i]->refDCint = 0;
        logic[i]->refDCval = 0.0;
        logic[i]->repeatedFrame = 0;
        logic[i]->Rs = 0.0;
        logic[i]->RsCsDiff = 0.0;
        logic[i]->RsDiff = 0.0;
        logic[i]->SC = 0.0;
        logic[i]->Schg = 0;
        logic[i]->SCindex = 0;
        logic[i]->scVal = 0;
        logic[i]->ssDCint = 0;
        logic[i]->ssDCval = 0;
        logic[i]->TSC = 0.0;
        logic[i]->TSCindex = 0;
        logic[i]->tscVal = 0;
        logic[i]->last_shot_distance = 0;
        logic[i]->lastFrameInShot = 0;
    }
}

void SceneChangeDetector::VidRead_Init() {
    support->average = 0.0;
    support->avgSAD = 0.0;
    support->gopSize = 1;
    support->pendingSch = 0;
    support->lastSCdetectionDistance = 0;
    support->detectedSch = 0;
    support->logic = new TSCstat *[TSCSTATBUFFER];
    TSCstat_Init(support->logic);
    support->PDistanceTable = PDISTTbl2;
    support->size = Small_Size;
    support->firstFrame = TRUE;
    support->gainCorrection.Image.data = NULL;
    support->gainCorrection.Image.Y = NULL;
    support->gainCorrection.Image.U = NULL;
    support->gainCorrection.Image.V = NULL;
    Pdmem_AllocGeneral(&support->gainCorrection,m_dataIn->layer[0]);
}

void SceneChangeDetector::nullifier(imageData *Buffer) {
    Buffer->pInteger = NULL;
    Buffer->Cs       = NULL;
    Buffer->Rs       = NULL;
    Buffer->RsCs     = NULL;
    Buffer->SAD      = NULL;
    Buffer->CsVal    = -1.0;
    Buffer->RsVal    = -1.0;
    Buffer->avgval   = 0.0;
}

void SceneChangeDetector::imageInit(YUV *buffer) {
    buffer->data = NULL;
    buffer->Y = NULL;
    buffer->U = NULL;
    buffer->V = NULL;
}

void SceneChangeDetector::VidSample_Init() {
    for(mfxI32 i = 0; i < 2; i++) {
        nullifier(&videoData[i]->layer[0]);
        imageInit(&videoData[i]->layer[0].Image);
        videoData[i]->frame_number = -1;
        videoData[i]->forward_reference = -1;
        videoData[i]->backward_reference = -1;
    }
}

// MODEL 01112017 with random forest approach

BOOL SCDetect1(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 RsCsDiff, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffMVdiffVal < 103.5) {
        if (TSC < 10.3) {
            if (refDCval < 20.6) {
                if (diffRsCsdiff < 4.85) {
                    return 0;
                }
                else if (diffRsCsdiff >= 4.85) {
                    if (diffAFD < 2.1) {
                        if (AFD < 0.87) {
                            if (SC < 11.88) {
                                return 1;
                            }
                            else if (SC >= 11.88) {
                                return 0;
                            }
                        }
                        else if (AFD >= 0.87) {
                            return 0;
                        }
                    }
                    else if (diffAFD >= 2.1) {
                        return 1;
                    }
                }
            }
            else if (refDCval >= 20.6) {
                if (diffMVdiffVal < 65.25) {
                    if (diffAFD < 9) {
                        return 0;
                    }
                    else if (diffAFD >= 9) {
                        if (MVDiff < 165.44) {
                            return 0;
                        }
                        else if (MVDiff >= 165.44) {
                            if (RsDiff < 25.74) {
                                return 1;
                            }
                            else if (RsDiff >= 25.74) {
                                if (diffRsCsdiff < 195.56) {
                                    return 0;
                                }
                                else if (diffRsCsdiff >= 195.56) {
                                    if (AFD < 28) {
                                        return 0;
                                    }
                                    else if (AFD >= 28) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 65.25) {
                    if (diffTSC < 1.73) {
                        if (RsCsDiff < 358.88) {
                            return 0;
                        }
                        else if (RsCsDiff >= 358.88) {
                            if (RsDiff < 302.23) {
                                return 1;
                            }
                            else if (RsDiff >= 302.23) {
                                return 0;
                            }
                        }
                    }
                    else if (diffTSC >= 1.73) {
                        if (ssDCval < 39.62) {
                            if (negBalance < 0.27) {
                                return 0;
                            }
                            else if (negBalance >= 0.27) {
                                if (CsDiff < 24.91) {
                                    return 0;
                                }
                                else if (CsDiff >= 24.91) {
                                    return 1;
                                }
                            }
                        }
                        else if (ssDCval >= 39.62) {
                            if (ssDCval < 172.15) {
                                if (ssDCval < 51.7) {
                                    if (CsDiff < 66.76) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 66.76) {
                                        if (SC < 18.81) {
                                            return 1;
                                        }
                                        else if (SC >= 18.81) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (ssDCval >= 51.7) {
                                    return 0;
                                }
                            }
                            else if (ssDCval >= 172.15) {
                                if (TSC < 6.16) {
                                    return 1;
                                }
                                else if (TSC >= 6.16) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (TSC >= 10.3) {
            if (diffAFD < 15.27) {
                if (TSC < 16.32) {
                    if (ssDCval < 49.47) {
                        if (negBalance < 0.67) {
                            if (refDCval < 38.61) {
                                return 0;
                            }
                            else if (refDCval >= 38.61) {
                                if (diffAFD < -19.64) {
                                    return 0;
                                }
                                else if (diffAFD >= -19.64) {
                                    return 1;
                                }
                            }
                        }
                        else if (negBalance >= 0.67) {
                            return 0;
                        }
                    }
                    else if (ssDCval >= 49.47) {
                        if (negBalance < 0.89) {
                            if (RsDiff < 233.37) {
                                return 0;
                            }
                            else if (RsDiff >= 233.37) {
                                if (RsDiff < 233.97) {
                                    return 1;
                                }
                                else if (RsDiff >= 233.97) {
                                    return 0;
                                }
                            }
                        }
                        else if (negBalance >= 0.89) {
                            if (posBalance < 0.83) {
                                return 1;
                            }
                            else if (posBalance >= 0.83) {
                                return 0;
                            }
                        }
                    }
                }
                else if (TSC >= 16.32) {
                    if (SC < 22.41) {
                        if (posBalance < 0.6) {
                            if (negBalance < 0.25) {
                                return 0;
                            }
                            else if (negBalance >= 0.25) {
                                if (diffRsCsdiff < 92.24) {
                                    return 1;
                                }
                                else if (diffRsCsdiff >= 92.24) {
                                    return 0;
                                }
                            }
                        }
                        else if (posBalance >= 0.6) {
                            if (ssDCval < 42.75) {
                                if (Rs < 8.68) {
                                    return 0;
                                }
                                else if (Rs >= 8.68) {
                                    return 1;
                                }
                            }
                            else if (ssDCval >= 42.75) {
                                return 0;
                            }
                        }
                    }
                    else if (SC >= 22.41) {
                        if (AFD < 62.25) {
                            if (Rs < 33.07) {
                                return 0;
                            }
                            else if (Rs >= 33.07) {
                                if (SC < 42.62) {
                                    if (TSC < 18.09) {
                                        return 0;
                                    }
                                    else if (TSC >= 18.09) {
                                        return 1;
                                    }
                                }
                                else if (SC >= 42.62) {
                                    return 0;
                                }
                            }
                        }
                        else if (AFD >= 62.25) {
                            if (MVDiff < 391.27) {
                                return 1;
                            }
                            else if (MVDiff >= 391.27) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (diffAFD >= 15.27) {
                if (negBalance < 0.24) {
                    if (diffRsCsdiff < 143.37) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= 143.37) {
                        if (CsDiff < 47.3) {
                            if (SC < 24.53) {
                                return 1;
                            }
                            else if (SC >= 24.53) {
                                return 0;
                            }
                        }
                        else if (CsDiff >= 47.3) {
                            if (diffRsCsdiff < 158.88) {
                                if (Rs < 15.67) {
                                    return 0;
                                }
                                else if (Rs >= 15.67) {
                                    return 1;
                                }
                            }
                            else if (diffRsCsdiff >= 158.88) {
                                if (MVDiff < 6.12) {
                                    return 1;
                                }
                                else if (MVDiff >= 6.12) {
                                    if (MVDiff < 304.92) {
                                        return 0;
                                    }
                                    else if (MVDiff >= 304.92) {
                                        if (Rs < 17.41) {
                                            return 1;
                                        }
                                        else if (Rs >= 17.41) {
                                            if (CsDiff < 615.52) {
                                                return 0;
                                            }
                                            else if (CsDiff >= 615.52) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (negBalance >= 0.24) {
                    if (MVDiff < 66.76) {
                        return 0;
                    }
                    else if (MVDiff >= 66.76) {
                        if (posBalance < 0.43) {
                            if (ssDCval < 73.04) {
                                if (CsDiff < 79.52) {
                                    if (Rs < 17.26) {
                                        return 1;
                                    }
                                    else if (Rs >= 17.26) {
                                        return 0;
                                    }
                                }
                                else if (CsDiff >= 79.52) {
                                    return 1;
                                }
                            }
                            else if (ssDCval >= 73.04) {
                                if (diffAFD < 20.71) {
                                    if (gchDC < 6.62) {
                                        if (MVDiff < 105.37) {
                                            if (SC < 35.95) {
                                                return 0;
                                            }
                                            else if (SC >= 35.95) {
                                                return 1;
                                            }
                                        }
                                        else if (MVDiff >= 105.37) {
                                            return 1;
                                        }
                                    }
                                    else if (gchDC >= 6.62) {
                                        return 0;
                                    }
                                }
                                else if (diffAFD >= 20.71) {
                                    if (refDCval < 85.88) {
                                        return 0;
                                    }
                                    else if (refDCval >= 85.88) {
                                        if (Cs < 18.84) {
                                            return 1;
                                        }
                                        else if (Cs >= 18.84) {
                                            if (TSC < 25.99) {
                                                if (gchDC < 3.21) {
                                                    if (Rs < 36.59) {
                                                        return 1;
                                                    }
                                                    else if (Rs >= 36.59) {
                                                        return 0;
                                                    }
                                                }
                                                else if (gchDC >= 3.21) {
                                                    return 0;
                                                }
                                            }
                                            else if (TSC >= 25.99) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (posBalance >= 0.43) {
                            if (gchDC < 106.58) {
                                if (gchDC < 26.08) {
                                    if (TSC < 15.39) {
                                        return 1;
                                    }
                                    else if (TSC >= 15.39) {
                                        return 0;
                                    }
                                }
                                else if (gchDC >= 26.08) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 106.58) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 103.5) {
        if (diffTSC < 4.17) {
            if (diffAFD < 6.91) {
                if (CsDiff < 140.41) {
                    if (ssDCval < 234.84) {
                        if (MVDiff < 186.58) {
                            if (MVDiff < 181.58) {
                                return 0;
                            }
                            else if (MVDiff >= 181.58) {
                                return 1;
                            }
                        }
                        else if (MVDiff >= 186.58) {
                            if (diffRsCsdiff < 88.48) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 88.48) {
                                if (Rs < 15.8) {
                                    return 1;
                                }
                                else if (Rs >= 15.8) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (ssDCval >= 234.84) {
                        if (Rs < 29.36) {
                            return 1;
                        }
                        else if (Rs >= 29.36) {
                            return 0;
                        }
                    }
                }
                else if (CsDiff >= 140.41) {
                    if (ssDCval < 95.86) {
                        if (ssDCval < 53.07) {
                            return 0;
                        }
                        else if (ssDCval >= 53.07) {
                            return 1;
                        }
                    }
                    else if (ssDCval >= 95.86) {
                        return 0;
                    }
                }
            }
            else if (diffAFD >= 6.91) {
                if (Rs < 11.15) {
                    if (MVDiff < 265.86) {
                        if (ssDCval < 45.3) {
                            return 1;
                        }
                        else if (ssDCval >= 45.3) {
                            if (AFD < 31.43) {
                                return 0;
                            }
                            else if (AFD >= 31.43) {
                                return 1;
                            }
                        }
                    }
                    else if (MVDiff >= 265.86) {
                        if (refDCval < 53.25) {
                            return 1;
                        }
                        else if (refDCval >= 53.25) {
                            return 0;
                        }
                    }
                }
                else if (Rs >= 11.15) {
                    if (CsDiff < 314.44) {
                        return 0;
                    }
                    else if (CsDiff >= 314.44) {
                        return 1;
                    }
                }
            }
        }
        else if (diffTSC >= 4.17) {
            if (diffRsCsdiff < 186.04) {
                if (posBalance < 0.52) {
                    if (refDCval < 88.57) {
                        if (MVDiff < 340.07) {
                            if (Cs < 17.54) {
                                if (diffTSC < 5.44) {
                                    if (TSC < 11.44) {
                                        if (diffRsCsdiff < 18.02) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 18.02) {
                                            return 1;
                                        }
                                    }
                                    else if (TSC >= 11.44) {
                                        return 0;
                                    }
                                }
                                else if (diffTSC >= 5.44) {
                                    if (MVDiff < 289.26) {
                                        return 1;
                                    }
                                    else if (MVDiff >= 289.26) {
                                        if (MVDiff < 289.97) {
                                            return 0;
                                        }
                                        else if (MVDiff >= 289.97) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                            else if (Cs >= 17.54) {
                                if (Cs < 18.44) {
                                    return 0;
                                }
                                else if (Cs >= 18.44) {
                                    return 1;
                                }
                            }
                        }
                        else if (MVDiff >= 340.07) {
                            if (diffRsCsdiff < 143.68) {
                                if (Cs < 24.44) {
                                    return 0;
                                }
                                else if (Cs >= 24.44) {
                                    return 1;
                                }
                            }
                            else if (diffRsCsdiff >= 143.68) {
                                return 1;
                            }
                        }
                    }
                    else if (refDCval >= 88.57) {
                        if (AFD < 35.16) {
                            if (MVDiff < 174.39) {
                                return 1;
                            }
                            else if (MVDiff >= 174.39) {
                                if (gchDC < 1.52) {
                                    return 1;
                                }
                                else if (gchDC >= 1.52) {
                                    return 0;
                                }
                            }
                        }
                        else if (AFD >= 35.16) {
                            if (diffRsCsdiff < 69.52) {
                                if (SC < 25.38) {
                                    return 0;
                                }
                                else if (SC >= 25.38) {
                                    if (MVDiff < 219.19) {
                                        return 0;
                                    }
                                    else if (MVDiff >= 219.19) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffRsCsdiff >= 69.52) {
                                if (diffTSC < 6.29) {
                                    if (Rs < 15.46) {
                                        return 1;
                                    }
                                    else if (Rs >= 15.46) {
                                        return 0;
                                    }
                                }
                                else if (diffTSC >= 6.29) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (posBalance >= 0.52) {
                    if (diffRsCsdiff < 38.65) {
                        if (posBalance < 0.68) {
                            if (MVDiff < 381.2) {
                                if (diffTSC < 4.98) {
                                    return 0;
                                }
                                else if (diffTSC >= 4.98) {
                                    return 1;
                                }
                            }
                            else if (MVDiff >= 381.2) {
                                return 0;
                            }
                        }
                        else if (posBalance >= 0.68) {
                            return 0;
                        }
                    }
                    else if (diffRsCsdiff >= 38.65) {
                        if (SC < 15.73) {
                            return 1;
                        }
                        else if (SC >= 15.73) {
                            if (refDCval < 66.66) {
                                if (refDCval < 34.18) {
                                    if (TSC < 12.37) {
                                        return 0;
                                    }
                                    else if (TSC >= 12.37) {
                                        return 1;
                                    }
                                }
                                else if (refDCval >= 34.18) {
                                    if (negBalance < 0.08) {
                                        return 0;
                                    }
                                    else if (negBalance >= 0.08) {
                                        if (SC < 19.01) {
                                            return 0;
                                        }
                                        else if (SC >= 19.01) {
                                            if (Rs < 21.2) {
                                                return 1;
                                            }
                                            else if (Rs >= 21.2) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (refDCval >= 66.66) {
                                if (SC < 28.98) {
                                    if (diffAFD < 26.24) {
                                        if (ssDCval < 65.9) {
                                            return 0;
                                        }
                                        else if (ssDCval >= 65.9) {
                                            if (refDCval < 73.23) {
                                                return 0;
                                            }
                                            else if (refDCval >= 73.23) {
                                                if (refDCval < 157.73) {
                                                    return 1;
                                                }
                                                else if (refDCval >= 157.73) {
                                                    return 0;
                                                }
                                            }
                                        }
                                    }
                                    else if (diffAFD >= 26.24) {
                                        if (MVDiff < 159.54) {
                                            return 0;
                                        }
                                        else if (MVDiff >= 159.54) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (SC >= 28.98) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 186.04) {
                if (negBalance < 0.08) {
                    if (diffRsCsdiff < 401.03) {
                        if (SC < 28.51) {
                            if (gchDC < 59.06) {
                                return 0;
                            }
                            else if (gchDC >= 59.06) {
                                return 1;
                            }
                        }
                        else if (SC >= 28.51) {
                            if (Rs < 30.21) {
                                return 0;
                            }
                            else if (Rs >= 30.21) {
                                return 1;
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 401.03) {
                        if (posBalance < 1) {
                            return 1;
                        }
                        else if (posBalance >= 1) {
                            if (CsDiff < 332.8) {
                                return 0;
                            }
                            else if (CsDiff >= 332.8) {
                                return 1;
                            }
                        }
                    }
                }
                else if (negBalance >= 0.08) {
                    if (MVDiff < 357.93) {
                        if (diffTSC < 8.49) {
                            if (diffMVdiffVal < 112) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 112) {
                                if (Rs < 29.76) {
                                    return 1;
                                }
                                else if (Rs >= 29.76) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffTSC >= 8.49) {
                            if (diffMVdiffVal < 121.75) {
                                if (SC < 40.68) {
                                    return 1;
                                }
                                else if (SC >= 40.68) {
                                    return 0;
                                }
                            }
                            else if (diffMVdiffVal >= 121.75) {
                                return 1;
                            }
                        }
                    }
                    else if (MVDiff >= 357.93) {
                        if (diffAFD < 18.54) {
                            return 0;
                        }
                        else if (diffAFD >= 18.54) {
                            if (CsDiff < 39.57) {
                                return 0;
                            }
                            else if (CsDiff >= 39.57) {
                                if (diffMVdiffVal < 294.13) {
                                    if (SC < 28.59) {
                                        return 1;
                                    }
                                    else if (SC >= 28.59) {
                                        return 0;
                                    }
                                }
                                else if (diffMVdiffVal >= 294.13) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect2(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffMVdiffVal < 100.17) {
        if (TSC < 11.87) {
            if (diffRsCsdiff < 36.58) {
                if (refDCval < 17.2) {
                    if (diffAFD < 0.61) {
                        return 0;
                    }
                    else if (diffAFD >= 0.61) {
                        if (AFD < 0.84) {
                            return 1;
                        }
                        else if (AFD >= 0.84) {
                            return 0;
                        }
                    }
                }
                else if (refDCval >= 17.2) {
                    return 0;
                }
            }
            else if (diffRsCsdiff >= 36.58) {
                if (SC < 21.2) {
                    if (ssDCval < 21.2) {
                        if (RsDiff < 57.39) {
                            return 0;
                        }
                        else if (RsDiff >= 57.39) {
                            return 1;
                        }
                    }
                    else if (ssDCval >= 21.2) {
                        if (diffRsCsdiff < 130.84) {
                            if (MVDiff < 181.08) {
                                if (diffMVdiffVal < 90.75) {
                                    return 0;
                                }
                                else if (diffMVdiffVal >= 90.75) {
                                    if (AFD < 13.47) {
                                        return 1;
                                    }
                                    else if (AFD >= 13.47) {
                                        return 0;
                                    }
                                }
                            }
                            else if (MVDiff >= 181.08) {
                                if (diffAFD < 13.08) {
                                    if (refDCval < 43.83) {
                                        if (negBalance < 0.29) {
                                            return 0;
                                        }
                                        else if (negBalance >= 0.29) {
                                            if (Cs < 7.07) {
                                                return 0;
                                            }
                                            else if (Cs >= 7.07) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (refDCval >= 43.83) {
                                        return 0;
                                    }
                                }
                                else if (diffAFD >= 13.08) {
                                    return 1;
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 130.84) {
                            if (RsDiff < 112.27) {
                                return 0;
                            }
                            else if (RsDiff >= 112.27) {
                                if (gchDC < 12.87) {
                                    if (diffMVdiffVal < 82.75) {
                                        if (ssDCval < 40.36) {
                                            return 1;
                                        }
                                        else if (ssDCval >= 40.36) {
                                            return 0;
                                        }
                                    }
                                    else if (diffMVdiffVal >= 82.75) {
                                        return 1;
                                    }
                                }
                                else if (gchDC >= 12.87) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (SC >= 21.2) {
                    if (diffAFD < 33.92) {
                        return 0;
                    }
                    else if (diffAFD >= 33.92) {
                        if (Cs < 13.93) {
                            return 1;
                        }
                        else if (Cs >= 13.93) {
                            return 0;
                        }
                    }
                }
            }
        }
        else if (TSC >= 11.87) {
            if (diffRsCsdiff < 147.94) {
                if (AFD < 63.73) {
                    if (Rs < 10.72) {
                        if (SC < 15.67) {
                            if (refDCval < 113.37) {
                                return 0;
                            }
                            else if (refDCval >= 113.37) {
                                return 1;
                            }
                        }
                        else if (SC >= 15.67) {
                            return 1;
                        }
                    }
                    else if (Rs >= 10.72) {
                        if (MVDiff < 366.85) {
                            return 0;
                        }
                        else if (MVDiff >= 366.85) {
                            if (MVDiff < 369.16) {
                                return 1;
                            }
                            else if (MVDiff >= 369.16) {
                                if (RsDiff < 63.6) {
                                    if (gchDC < 10.92) {
                                        return 1;
                                    }
                                    else if (gchDC >= 10.92) {
                                        return 0;
                                    }
                                }
                                else if (RsDiff >= 63.6) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
                else if (AFD >= 63.73) {
                    if (diffRsCsdiff < 0.89) {
                        return 1;
                    }
                    else if (diffRsCsdiff >= 0.89) {
                        return 0;
                    }
                }
            }
            else if (diffRsCsdiff >= 147.94) {
                if (negBalance < 0.28) {
                    if (CsDiff < 156.48) {
                        return 0;
                    }
                    else if (CsDiff >= 156.48) {
                        if (diffTSC < 26.74) {
                            if (CsDiff < 158.29) {
                                return 1;
                            }
                            else if (CsDiff >= 158.29) {
                                if (gchDC < 50.9) {
                                    return 0;
                                }
                                else if (gchDC >= 50.9) {
                                    if (Cs < 21.22) {
                                        if (RsCsDiff < 468.61) {
                                            if (SC < 26.79) {
                                                return 1;
                                            }
                                            else if (SC >= 26.79) {
                                                return 0;
                                            }
                                        }
                                        else if (RsCsDiff >= 468.61) {
                                            return 1;
                                        }
                                    }
                                    else if (Cs >= 21.22) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (diffTSC >= 26.74) {
                            return 1;
                        }
                    }
                }
                else if (negBalance >= 0.28) {
                    if (Cs < 25.04) {
                        if (TSC < 13.25) {
                            if (diffMVdiffVal < 78.27) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 78.27) {
                                return 1;
                            }
                        }
                        else if (TSC >= 13.25) {
                            if (RsCsDiff < 254.83) {
                                return 1;
                            }
                            else if (RsCsDiff >= 254.83) {
                                if (diffAFD < 25.2) {
                                    if (SC < 36.55) {
                                        if (RsDiff < 399.63) {
                                            if (CsDiff < 252.79) {
                                                if (RsDiff < 392.6) {
                                                    return 0;
                                                }
                                                else if (RsDiff >= 392.6) {
                                                    return 1;
                                                }
                                            }
                                            else if (CsDiff >= 252.79) {
                                                return 1;
                                            }
                                        }
                                        else if (RsDiff >= 399.63) {
                                            return 0;
                                        }
                                    }
                                    else if (SC >= 36.55) {
                                        return 1;
                                    }
                                }
                                else if (diffAFD >= 25.2) {
                                    if (SC < 43.79) {
                                        if (SC < 22.02) {
                                            return 0;
                                        }
                                        else if (SC >= 22.02) {
                                            if (RsCsDiff < 292.22) {
                                                if (Rs < 28.31) {
                                                    return 1;
                                                }
                                                else if (Rs >= 28.31) {
                                                    return 0;
                                                }
                                            }
                                            else if (RsCsDiff >= 292.22) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (SC >= 43.79) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (Cs >= 25.04) {
                        if (CsDiff < 113.57) {
                            return 1;
                        }
                        else if (CsDiff >= 113.57) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 100.17) {
        if (CsDiff < 97.18) {
            if (diffRsCsdiff < 24.55) {
                if (diffTSC < 2.66) {
                    if (ssDCval < 234.84) {
                        return 0;
                    }
                    else if (ssDCval >= 234.84) {
                        if (Rs < 29.36) {
                            return 1;
                        }
                        else if (Rs >= 29.36) {
                            return 0;
                        }
                    }
                }
                else if (diffTSC >= 2.66) {
                    if (SC < 11.11) {
                        if (posBalance < 0.56) {
                            return 1;
                        }
                        else if (posBalance >= 0.56) {
                            if (refDCval < 24.15) {
                                return 1;
                            }
                            else if (refDCval >= 24.15) {
                                return 0;
                            }
                        }
                    }
                    else if (SC >= 11.11) {
                        if (MVDiff < 147.33) {
                            if (Scindex < 2.5) {
                                return 1;
                            }
                            else if (Scindex >= 2.5) {
                                return 0;
                            }
                        }
                        else if (MVDiff >= 147.33) {
                            return 0;
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 24.55) {
                if (diffTSC < 3.8) {
                    if (ssDCval < 42.18) {
                        if (posBalance < 0.49) {
                            return 1;
                        }
                        else if (posBalance >= 0.49) {
                            return 0;
                        }
                    }
                    else if (ssDCval >= 42.18) {
                        return 0;
                    }
                }
                else if (diffTSC >= 3.8) {
                    if (negBalance < 0.09) {
                        if (MVDiff < 363.48) {
                            if (diffTSC < 23.27) {
                                return 0;
                            }
                            else if (diffTSC >= 23.27) {
                                if (SC < 21.03) {
                                    return 1;
                                }
                                else if (SC >= 21.03) {
                                    return 0;
                                }
                            }
                        }
                        else if (MVDiff >= 363.48) {
                            if (negBalance < 0.05) {
                                if (Cs < 15.03) {
                                    return 0;
                                }
                                else if (Cs >= 15.03) {
                                    return 1;
                                }
                            }
                            else if (negBalance >= 0.05) {
                                return 1;
                            }
                        }
                    }
                    else if (negBalance >= 0.09) {
                        if (posBalance < 0.25) {
                            return 1;
                        }
                        else if (posBalance >= 0.25) {
                            if (ssDCval < 45.14) {
                                return 1;
                            }
                            else if (ssDCval >= 45.14) {
                                if (diffTSC < 7.28) {
                                    if (Rs < 9.31) {
                                        return 1;
                                    }
                                    else if (Rs >= 9.31) {
                                        return 0;
                                    }
                                }
                                else if (diffTSC >= 7.28) {
                                    if (Cs < 15.67) {
                                        if (posBalance < 0.65) {
                                            if (Cs < 5.57) {
                                                return 0;
                                            }
                                            else if (Cs >= 5.57) {
                                                return 1;
                                            }
                                        }
                                        else if (posBalance >= 0.65) {
                                            if (RsDiff < 90.25) {
                                                if (ssDCval < 68.14) {
                                                    return 0;
                                                }
                                                else if (ssDCval >= 68.14) {
                                                    if (Cs < 11.12) {
                                                        return 1;
                                                    }
                                                    else if (Cs >= 11.12) {
                                                        return 0;
                                                    }
                                                }
                                            }
                                            else if (RsDiff >= 90.25) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (Cs >= 15.67) {
                                        if (AFD < 57.45) {
                                            return 0;
                                        }
                                        else if (AFD >= 57.45) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (CsDiff >= 97.18) {
            if (diffRsCsdiff < 140.24) {
                if (Cs < 14.07) {
                    if (diffTSC < -1.08) {
                        return 0;
                    }
                    else if (diffTSC >= -1.08) {
                        if (SC < 17.81) {
                            return 1;
                        }
                        else if (SC >= 17.81) {
                            if (Rs < 12.51) {
                                return 0;
                            }
                            else if (Rs >= 12.51) {
                                if (ssDCval < 53.87) {
                                    return 0;
                                }
                                else if (ssDCval >= 53.87) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (Cs >= 14.07) {
                    if (AFD < 64.15) {
                        if (AFD < 38.14) {
                            return 0;
                        }
                        else if (AFD >= 38.14) {
                            if (gchDC < 30.83) {
                                if (diffMVdiffVal < 161) {
                                    if (diffTSC < 1.29) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 1.29) {
                                        if (SC < 21.58) {
                                            return 0;
                                        }
                                        else if (SC >= 21.58) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (diffMVdiffVal >= 161) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 30.83) {
                                return 0;
                            }
                        }
                    }
                    else if (AFD >= 64.15) {
                        if (TSC < 31.21) {
                            if (Rs < 24.49) {
                                return 1;
                            }
                            else if (Rs >= 24.49) {
                                return 0;
                            }
                        }
                        else if (TSC >= 31.21) {
                            return 1;
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 140.24) {
                if (diffMVdiffVal < 122.8) {
                    if (MVDiff < 135.66) {
                        if (negBalance < 0.08) {
                            return 0;
                        }
                        else if (negBalance >= 0.08) {
                            if (CsDiff < 121.94) {
                                return 0;
                            }
                            else if (CsDiff >= 121.94) {
                                return 1;
                            }
                        }
                    }
                    else if (MVDiff >= 135.66) {
                        if (Rs < 18.33) {
                            if (CsDiff < 177.72) {
                                return 0;
                            }
                            else if (CsDiff >= 177.72) {
                                return 1;
                            }
                        }
                        else if (Rs >= 18.33) {
                            if (ssDCval < 69.92) {
                                if (Rs < 26.85) {
                                    return 1;
                                }
                                else if (Rs >= 26.85) {
                                    return 0;
                                }
                            }
                            else if (ssDCval >= 69.92) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 122.8) {
                    if (negBalance < 0.08) {
                        if (RsCsDiff < 472.38) {
                            if (diffRsCsdiff < 350.27) {
                                if (diffAFD < 31.34) {
                                    return 0;
                                }
                                else if (diffAFD >= 31.34) {
                                    if (CsDiff < 157.38) {
                                        if (MVDiff < 266.04) {
                                            return 0;
                                        }
                                        else if (MVDiff >= 266.04) {
                                            return 1;
                                        }
                                    }
                                    else if (CsDiff >= 157.38) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffRsCsdiff >= 350.27) {
                                return 1;
                            }
                        }
                        else if (RsCsDiff >= 472.38) {
                            if (SC < 43.52) {
                                if (SC < 38.59) {
                                    return 1;
                                }
                                else if (SC >= 38.59) {
                                    if (Rs < 31.84) {
                                        return 0;
                                    }
                                    else if (Rs >= 31.84) {
                                        return 1;
                                    }
                                }
                            }
                            else if (SC >= 43.52) {
                                return 0;
                            }
                        }
                    }
                    else if (negBalance >= 0.08) {
                        if (diffAFD < 24.7) {
                            if (ssDCval < 96.33) {
                                if (diffAFD < 24.48) {
                                    if (posBalance < 0.69) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.69) {
                                        if (SC < 21.77) {
                                            return 1;
                                        }
                                        else if (SC >= 21.77) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (diffAFD >= 24.48) {
                                    return 0;
                                }
                            }
                            else if (ssDCval >= 96.33) {
                                if (Cs < 16.95) {
                                    return 1;
                                }
                                else if (Cs >= 16.95) {
                                    if (diffTSC < 12) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 12) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (diffAFD >= 24.7) {
                            if (Cs < 20.95) {
                                return 1;
                            }
                            else if (Cs >= 20.95) {
                                if (SC < 29.35) {
                                    if (diffTSC < 15.77) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 15.77) {
                                        return 1;
                                    }
                                }
                                else if (SC >= 29.35) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect3(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffTSC < 5.51) {
        if (diffAFD < 7.5) {
            if (RsDiff < 82.95) {
                if (ssDCval < 17.64) {
                    if (diffTSC < 0.44) {
                        return 0;
                    }
                    else if (diffTSC >= 0.44) {
                        if (diffAFD < 0.84) {
                            return 1;
                        }
                        else if (diffAFD >= 0.84) {
                            if (MVDiff < 11.37) {
                                return 0;
                            }
                            else if (MVDiff >= 11.37) {
                                return 1;
                            }
                        }
                    }
                }
                else if (ssDCval >= 17.64) {
                    if (MVDiff < 440.86) {
                        if (posBalance < 0.48) {
                            return 0;
                        }
                        else if (posBalance >= 0.48) {
                            if (posBalance < 0.49) {
                                if (TSC < 6.87) {
                                    return 0;
                                }
                                else if (TSC >= 6.87) {
                                    return 1;
                                }
                            }
                            else if (posBalance >= 0.49) {
                                return 0;
                            }
                        }
                    }
                    else if (MVDiff >= 440.86) {
                        if (ssDCval < 217.28) {
                            if (MVDiff < 443.66) {
                                return 1;
                            }
                            else if (MVDiff >= 443.66) {
                                return 0;
                            }
                        }
                        else if (ssDCval >= 217.28) {
                            return 1;
                        }
                    }
                }
            }
            else if (RsDiff >= 82.95) {
                if (SC < 15.21) {
                    if (CsDiff < 78.51) {
                        return 0;
                    }
                    else if (CsDiff >= 78.51) {
                        if (diffRsCsdiff < 76.45) {
                            if (SC < 9.74) {
                                if (Rs < 6.62) {
                                    return 0;
                                }
                                else if (Rs >= 6.62) {
                                    return 1;
                                }
                            }
                            else if (SC >= 9.74) {
                                return 0;
                            }
                        }
                        else if (diffRsCsdiff >= 76.45) {
                            if (Rs < 8.27) {
                                if (RsDiff < 220.1) {
                                    return 1;
                                }
                                else if (RsDiff >= 220.1) {
                                    return 0;
                                }
                            }
                            else if (Rs >= 8.27) {
                                return 1;
                            }
                        }
                    }
                }
                else if (SC >= 15.21) {
                    if (TSC < 17.87) {
                        if (diffTSC < 2.14) {
                            return 0;
                        }
                        else if (diffTSC >= 2.14) {
                            if (SC < 20.25) {
                                if (gchDC < 3.65) {
                                    if (Cs < 11.2) {
                                        return 1;
                                    }
                                    else if (Cs >= 11.2) {
                                        if (Rs < 16) {
                                            return 0;
                                        }
                                        else if (Rs >= 16) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (gchDC >= 3.65) {
                                    return 0;
                                }
                            }
                            else if (SC >= 20.25) {
                                if (posBalance < 0.95) {
                                    return 0;
                                }
                                else if (posBalance >= 0.95) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (TSC >= 17.87) {
                        if (posBalance < 0.29) {
                            if (diffRsCsdiff < 39.15) {
                                if (Cs < 24.47) {
                                    if (CsDiff < 107.86) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 107.86) {
                                        return 1;
                                    }
                                }
                                else if (Cs >= 24.47) {
                                    return 0;
                                }
                            }
                            else if (diffRsCsdiff >= 39.15) {
                                return 0;
                            }
                        }
                        else if (posBalance >= 0.29) {
                            if (gchDC < 86.6) {
                                return 0;
                            }
                            else if (gchDC >= 86.6) {
                                if (Rs < 31.93) {
                                    return 0;
                                }
                                else if (Rs >= 31.93) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffAFD >= 7.5) {
            if (Rs < 14.67) {
                if (MVDiff < 127.91) {
                    if (ssDCval < 31.61) {
                        return 1;
                    }
                    else if (ssDCval >= 31.61) {
                        return 0;
                    }
                }
                else if (MVDiff >= 127.91) {
                    if (diffMVdiffVal < 120.65) {
                        if (diffRsCsdiff < 151.22) {
                            if (MVDiff < 267.71) {
                                if (posBalance < 0.31) {
                                    if (Rs < 11.64) {
                                        return 1;
                                    }
                                    else if (Rs >= 11.64) {
                                        return 0;
                                    }
                                }
                                else if (posBalance >= 0.31) {
                                    return 0;
                                }
                            }
                            else if (MVDiff >= 267.71) {
                                if (Rs < 10.44) {
                                    if (SC < 10.55) {
                                        return 0;
                                    }
                                    else if (SC >= 10.55) {
                                        return 1;
                                    }
                                }
                                else if (Rs >= 10.44) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 151.22) {
                            if (SC < 9.37) {
                                return 0;
                            }
                            else if (SC >= 9.37) {
                                return 1;
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 120.65) {
                        if (MVDiff < 346.7) {
                            if (diffRsCsdiff < 14.68) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 14.68) {
                                if (Scindex < 3.5) {
                                    if (diffRsCsdiff < 62.87) {
                                        if (TSC < 7) {
                                            return 1;
                                        }
                                        else if (TSC >= 7) {
                                            if (refDCval < 55.61) {
                                                return 0;
                                            }
                                            else if (refDCval >= 55.61) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (diffRsCsdiff >= 62.87) {
                                        return 1;
                                    }
                                }
                                else if (Scindex >= 3.5) {
                                    return 0;
                                }
                            }
                        }
                        else if (MVDiff >= 346.7) {
                            if (AFD < 19.68) {
                                return 1;
                            }
                            else if (AFD >= 19.68) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (Rs >= 14.67) {
                if (MVDiff < 182.01) {
                    return 0;
                }
                else if (MVDiff >= 182.01) {
                    if (posBalance < 0.02) {
                        if (Rs < 15.9) {
                            return 1;
                        }
                        else if (Rs >= 15.9) {
                            return 0;
                        }
                    }
                    else if (posBalance >= 0.02) {
                        if (MVDiff < 182.4) {
                            return 1;
                        }
                        else if (MVDiff >= 182.4) {
                            if (ssDCval < 61.82) {
                                if (RsCsDiff < 422.81) {
                                    return 0;
                                }
                                else if (RsCsDiff >= 422.81) {
                                    if (Rs < 30.47) {
                                        return 1;
                                    }
                                    else if (Rs >= 30.47) {
                                        return 0;
                                    }
                                }
                            }
                            else if (ssDCval >= 61.82) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffTSC >= 5.51) {
        if (diffMVdiffVal < 106.86) {
            if (MVDiff < 79.84) {
                if (Rs < 14.58) {
                    if (Rs < 14.44) {
                        return 0;
                    }
                    else if (Rs >= 14.44) {
                        return 1;
                    }
                }
                else if (Rs >= 14.58) {
                    return 0;
                }
            }
            else if (MVDiff >= 79.84) {
                if (posBalance < 0.42) {
                    if (diffAFD < 19.93) {
                        if (diffMVdiffVal < 63.49) {
                            if (CsDiff < 127.35) {
                                return 0;
                            }
                            else if (CsDiff >= 127.35) {
                                if (negBalance < 0.41) {
                                    return 0;
                                }
                                else if (negBalance >= 0.41) {
                                    if (refDCval < 84.72) {
                                        return 1;
                                    }
                                    else if (refDCval >= 84.72) {
                                        if (SC < 36.64) {
                                            return 0;
                                        }
                                        else if (SC >= 36.64) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                        else if (diffMVdiffVal >= 63.49) {
                            if (ssDCval < 81.21) {
                                if (diffTSC < 6.04) {
                                    return 0;
                                }
                                else if (diffTSC >= 6.04) {
                                    if (negBalance < 0.19) {
                                        return 0;
                                    }
                                    else if (negBalance >= 0.19) {
                                        return 1;
                                    }
                                }
                            }
                            else if (ssDCval >= 81.21) {
                                if (MVDiff < 84.31) {
                                    return 1;
                                }
                                else if (MVDiff >= 84.31) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 19.93) {
                        if (TSCindex < 8.5) {
                            return 0;
                        }
                        else if (TSCindex >= 8.5) {
                            if (TSC < 13.22) {
                                if (TSC < 9.75) {
                                    return 1;
                                }
                                else if (TSC >= 9.75) {
                                    return 0;
                                }
                            }
                            else if (TSC >= 13.22) {
                                if (negBalance < 0.25) {
                                    return 0;
                                }
                                else if (negBalance >= 0.25) {
                                    if (diffRsCsdiff < 472.69) {
                                        if (AFD < 52.37) {
                                            return 1;
                                        }
                                        else if (AFD >= 52.37) {
                                            if (TSC < 19.39) {
                                                return 0;
                                            }
                                            else if (TSC >= 19.39) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (diffRsCsdiff >= 472.69) {
                                        if (TSC < 24.11) {
                                            return 0;
                                        }
                                        else if (TSC >= 24.11) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (posBalance >= 0.42) {
                    if (CsDiff < 231.75) {
                        if (diffRsCsdiff < 454.35) {
                            if (diffAFD < 43.41) {
                                if (AFD < 10.52) {
                                    if (Rs < 18.64) {
                                        return 0;
                                    }
                                    else if (Rs >= 18.64) {
                                        return 1;
                                    }
                                }
                                else if (AFD >= 10.52) {
                                    if (diffAFD < 22.55) {
                                        return 0;
                                    }
                                    else if (diffAFD >= 22.55) {
                                        if (diffAFD < 22.74) {
                                            return 1;
                                        }
                                        else if (diffAFD >= 22.74) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (diffAFD >= 43.41) {
                                if (Rs < 18.92) {
                                    return 1;
                                }
                                else if (Rs >= 18.92) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 454.35) {
                            return 1;
                        }
                    }
                    else if (CsDiff >= 231.75) {
                        if (Rs < 19.83) {
                            if (posBalance < 0.64) {
                                return 0;
                            }
                            else if (posBalance >= 0.64) {
                                return 1;
                            }
                        }
                        else if (Rs >= 19.83) {
                            return 0;
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 106.86) {
            if (negBalance < 0.08) {
                if (diffMVdiffVal < 148.33) {
                    if (RsCsDiff < 358.23) {
                        return 0;
                    }
                    else if (RsCsDiff >= 358.23) {
                        if (negBalance < 0.01) {
                            return 0;
                        }
                        else if (negBalance >= 0.01) {
                            return 1;
                        }
                    }
                }
                else if (diffMVdiffVal >= 148.33) {
                    if (ssDCval < 115.11) {
                        if (ssDCval < 52.83) {
                            return 1;
                        }
                        else if (ssDCval >= 52.83) {
                            if (SC < 19.58) {
                                if (RsDiff < 26.27) {
                                    return 0;
                                }
                                else if (RsDiff >= 26.27) {
                                    return 1;
                                }
                            }
                            else if (SC >= 19.58) {
                                if (CsDiff < 334.21) {
                                    if (ssDCval < 103.33) {
                                        return 0;
                                    }
                                    else if (ssDCval >= 103.33) {
                                        if (Rs < 17.82) {
                                            return 0;
                                        }
                                        else if (Rs >= 17.82) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (CsDiff >= 334.21) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (ssDCval >= 115.11) {
                        return 1;
                    }
                }
            }
            else if (negBalance >= 0.08) {
                if (diffRsCsdiff < 94.81) {
                    if (ssDCval < 75.12) {
                        if (Rs < 12.08) {
                            if (diffRsCsdiff < 10.71) {
                                if (ssDCval < 55.39) {
                                    if (posBalance < 0.55) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.55) {
                                        return 0;
                                    }
                                }
                                else if (ssDCval >= 55.39) {
                                    return 0;
                                }
                            }
                            else if (diffRsCsdiff >= 10.71) {
                                if (negBalance < 0.89) {
                                    return 1;
                                }
                                else if (negBalance >= 0.89) {
                                    return 0;
                                }
                            }
                        }
                        else if (Rs >= 12.08) {
                            return 0;
                        }
                    }
                    else if (ssDCval >= 75.12) {
                        if (AFD < 63.98) {
                            if (refDCval < 164.59) {
                                if (diffTSC < 11.6) {
                                    if (RsCsDiff < 227.8) {
                                        if (ssDCval < 131.42) {
                                            return 0;
                                        }
                                        else if (ssDCval >= 131.42) {
                                            return 1;
                                        }
                                    }
                                    else if (RsCsDiff >= 227.8) {
                                        return 1;
                                    }
                                }
                                else if (diffTSC >= 11.6) {
                                    if (MVDiff < 166.33) {
                                        return 0;
                                    }
                                    else if (MVDiff >= 166.33) {
                                        return 1;
                                    }
                                }
                            }
                            else if (refDCval >= 164.59) {
                                return 0;
                            }
                        }
                        else if (AFD >= 63.98) {
                            return 1;
                        }
                    }
                }
                else if (diffRsCsdiff >= 94.81) {
                    if (refDCval < 164.02) {
                        if (CsDiff < 40.09) {
                            return 0;
                        }
                        else if (CsDiff >= 40.09) {
                            if (diffTSC < 10.89) {
                                if (RsCsDiff < 404.31) {
                                    if (ssDCval < 88.15) {
                                        return 1;
                                    }
                                    else if (ssDCval >= 88.15) {
                                        if (SC < 23.86) {
                                            return 1;
                                        }
                                        else if (SC >= 23.86) {
                                            if (ssDCval < 104.33) {
                                                return 0;
                                            }
                                            else if (ssDCval >= 104.33) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (RsCsDiff >= 404.31) {
                                    if (diffTSC < 10.85) {
                                        if (diffRsCsdiff < 242.28) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 242.28) {
                                            return 1;
                                        }
                                    }
                                    else if (diffTSC >= 10.85) {
                                        return 0;
                                    }
                                }
                            }
                            else if (diffTSC >= 10.89) {
                                if (RsCsDiff < 1265.16) {
                                    if (CsDiff < 58.87) {
                                        if (AFD < 47.72) {
                                            return 1;
                                        }
                                        else if (AFD >= 47.72) {
                                            return 0;
                                        }
                                    }
                                    else if (CsDiff >= 58.87) {
                                        return 1;
                                    }
                                }
                                else if (RsCsDiff >= 1265.16) {
                                    if (Rs < 32.55) {
                                        return 0;
                                    }
                                    else if (Rs >= 32.55) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                    else if (refDCval >= 164.02) {
                        if (diffRsCsdiff < 279.16) {
                            if (Cs < 24.86) {
                                return 0;
                            }
                            else if (Cs >= 24.86) {
                                return 1;
                            }
                        }
                        else if (diffRsCsdiff >= 279.16) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect4(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    Scindex;
    TSCindex;

    if (diffAFD < 11.31) {
        if (diffMVdiffVal < 91.23) {
            if (AFD < 52.46) {
                if (diffMVdiffVal < 4.69) {
                    if (ssDCval < 16.66) {
                        if (ssDCval < 16.64) {
                            return 0;
                        }
                        else if (ssDCval >= 16.64) {
                            return 1;
                        }
                    }
                    else if (ssDCval >= 16.66) {
                        if (TSC < 11.53) {
                            return 0;
                        }
                        else if (TSC >= 11.53) {
                            if (refDCval < 48.09) {
                                if (MVDiff < 314.91) {
                                    return 0;
                                }
                                else if (MVDiff >= 314.91) {
                                    return 1;
                                }
                            }
                            else if (refDCval >= 48.09) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 4.69) {
                    if (refDCval < 20.88) {
                        if (diffTSC < 0.43) {
                            return 0;
                        }
                        else if (diffTSC >= 0.43) {
                            if (Cs < 3.96) {
                                return 0;
                            }
                            else if (Cs >= 3.96) {
                                return 1;
                            }
                        }
                    }
                    else if (refDCval >= 20.88) {
                        if (TSC < 13.75) {
                            if (diffAFD < 9.86) {
                                return 0;
                            }
                            else if (diffAFD >= 9.86) {
                                if (ssDCval < 40.99) {
                                    return 1;
                                }
                                else if (ssDCval >= 40.99) {
                                    return 0;
                                }
                            }
                        }
                        else if (TSC >= 13.75) {
                            if (Cs < 14.13) {
                                if (diffAFD < 5.29) {
                                    if (posBalance < 0.11) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.11) {
                                        return 0;
                                    }
                                }
                                else if (diffAFD >= 5.29) {
                                    return 0;
                                }
                            }
                            else if (Cs >= 14.13) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (AFD >= 52.46) {
                if (CsDiff < 151.91) {
                    return 0;
                }
                else if (CsDiff >= 151.91) {
                    if (CsDiff < 381.14) {
                        if (TSC < 16.71) {
                            return 0;
                        }
                        else if (TSC >= 16.71) {
                            return 1;
                        }
                    }
                    else if (CsDiff >= 381.14) {
                        if (AFD < 66.94) {
                            return 0;
                        }
                        else if (AFD >= 66.94) {
                            return 1;
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 91.23) {
            if (diffRsCsdiff < 14.71) {
                if (TSC < 20.02) {
                    if (diffAFD < 10.2) {
                        return 0;
                    }
                    else if (diffAFD >= 10.2) {
                        if (Rs < 10.54) {
                            return 1;
                        }
                        else if (Rs >= 10.54) {
                            return 0;
                        }
                    }
                }
                else if (TSC >= 20.02) {
                    if (RsCsDiff < 425.59) {
                        if (negBalance < 0.62) {
                            return 1;
                        }
                        else if (negBalance >= 0.62) {
                            return 0;
                        }
                    }
                    else if (RsCsDiff >= 425.59) {
                        return 1;
                    }
                }
            }
            else if (diffRsCsdiff >= 14.71) {
                if (SC < 12.36) {
                    if (diffTSC < 2.43) {
                        if (posBalance < 0.38) {
                            if (Cs < 4.34) {
                                return 0;
                            }
                            else if (Cs >= 4.34) {
                                return 1;
                            }
                        }
                        else if (posBalance >= 0.38) {
                            return 0;
                        }
                    }
                    else if (diffTSC >= 2.43) {
                        return 1;
                    }
                }
                else if (SC >= 12.36) {
                    if (diffTSC < 3.61) {
                        if (Rs < 9.4) {
                            if (gchDC < 46.24) {
                                return 1;
                            }
                            else if (gchDC >= 46.24) {
                                return 0;
                            }
                        }
                        else if (Rs >= 9.4) {
                            if (AFD < 14.32) {
                                if (diffAFD < 1.47) {
                                    return 0;
                                }
                                else if (diffAFD >= 1.47) {
                                    if (Rs < 11.29) {
                                        return 0;
                                    }
                                    else if (Rs >= 11.29) {
                                        return 1;
                                    }
                                }
                            }
                            else if (AFD >= 14.32) {
                                return 0;
                            }
                        }
                    }
                    else if (diffTSC >= 3.61) {
                        if (refDCval < 132.72) {
                            if (CsDiff < 44.45) {
                                return 0;
                            }
                            else if (CsDiff >= 44.45) {
                                if (SC < 19.62) {
                                    return 1;
                                }
                                else if (SC >= 19.62) {
                                    if (CsDiff < 254.49) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 254.49) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (refDCval >= 132.72) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    else if (diffAFD >= 11.31) {
        if (diffMVdiffVal < 107.59) {
            if (AFD < 30.09) {
                if (diffMVdiffVal < 65.11) {
                    if (gchDC < 18.43) {
                        return 0;
                    }
                    else if (gchDC >= 18.43) {
                        if (gchDC < 18.45) {
                            return 1;
                        }
                        else if (gchDC >= 18.45) {
                            if (posBalance < 0.98) {
                                return 0;
                            }
                            else if (posBalance >= 0.98) {
                                return 1;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 65.11) {
                    if (Rs < 17.11) {
                        if (negBalance < 0.23) {
                            return 0;
                        }
                        else if (negBalance >= 0.23) {
                            if (refDCval < 66.35) {
                                return 1;
                            }
                            else if (refDCval >= 66.35) {
                                if (diffTSC < 6.57) {
                                    return 0;
                                }
                                else if (diffTSC >= 6.57) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (Rs >= 17.11) {
                        if (TSC < 12.08) {
                            return 0;
                        }
                        else if (TSC >= 12.08) {
                            if (posBalance < 0.31) {
                                if (AFD < 18.65) {
                                    return 0;
                                }
                                else if (AFD >= 18.65) {
                                    return 1;
                                }
                            }
                            else if (posBalance >= 0.31) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (AFD >= 30.09) {
                if (MVDiff < 103.42) {
                    return 0;
                }
                else if (MVDiff >= 103.42) {
                    if (negBalance < 0.26) {
                        if (CsDiff < 220.03) {
                            if (negBalance < 0.18) {
                                return 0;
                            }
                            else if (negBalance >= 0.18) {
                                if (diffTSC < 16.25) {
                                    return 0;
                                }
                                else if (diffTSC >= 16.25) {
                                    if (ssDCval < 116.44) {
                                        return 1;
                                    }
                                    else if (ssDCval >= 116.44) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (CsDiff >= 220.03) {
                            if (MVDiff < 280.08) {
                                return 0;
                            }
                            else if (MVDiff >= 280.08) {
                                if (AFD < 72.1) {
                                    return 1;
                                }
                                else if (AFD >= 72.1) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (negBalance >= 0.26) {
                        if (diffRsCsdiff < 158.73) {
                            if (SC < 27.54) {
                                if (diffAFD < 16.23) {
                                    if (diffTSC < 10) {
                                        if (Rs < 17.45) {
                                            if (gchDC < 23.26) {
                                                if (TSC < 17.27) {
                                                    return 0;
                                                }
                                                else if (TSC >= 17.27) {
                                                    return 1;
                                                }
                                            }
                                            else if (gchDC >= 23.26) {
                                                return 1;
                                            }
                                        }
                                        else if (Rs >= 17.45) {
                                            return 0;
                                        }
                                    }
                                    else if (diffTSC >= 10) {
                                        return 1;
                                    }
                                }
                                else if (diffAFD >= 16.23) {
                                    return 0;
                                }
                            }
                            else if (SC >= 27.54) {
                                return 0;
                            }
                        }
                        else if (diffRsCsdiff >= 158.73) {
                            if (RsDiff < 310.02) {
                                if (diffMVdiffVal < -56.92) {
                                    if (CsDiff < 123.5) {
                                        return 1;
                                    }
                                    else if (CsDiff >= 123.5) {
                                        return 0;
                                    }
                                }
                                else if (diffMVdiffVal >= -56.92) {
                                    if (TSC < 12.88) {
                                        if (SC < 22.12) {
                                            return 1;
                                        }
                                        else if (SC >= 22.12) {
                                            return 0;
                                        }
                                    }
                                    else if (TSC >= 12.88) {
                                        return 1;
                                    }
                                }
                            }
                            else if (RsDiff >= 310.02) {
                                if (diffAFD < 21.29) {
                                    if (diffRsCsdiff < 290.72) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 290.72) {
                                        if (Cs < 28.6) {
                                            return 1;
                                        }
                                        else if (Cs >= 28.6) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (diffAFD >= 21.29) {
                                    if (diffTSC < 9.82) {
                                        if (SC < 25.78) {
                                            return 1;
                                        }
                                        else if (SC >= 25.78) {
                                            return 0;
                                        }
                                    }
                                    else if (diffTSC >= 9.82) {
                                        if (TSC < 35.02) {
                                            return 1;
                                        }
                                        else if (TSC >= 35.02) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 107.59) {
            if (posBalance < 0.62) {
                if (diffTSC < 5.69) {
                    if (SC < 21.85) {
                        if (TSC < 9.91) {
                            return 1;
                        }
                        else if (TSC >= 9.91) {
                            if (diffRsCsdiff < 162.57) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 162.57) {
                                if (CsDiff < 91.65) {
                                    return 0;
                                }
                                else if (CsDiff >= 91.65) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (SC >= 21.85) {
                        return 0;
                    }
                }
                else if (diffTSC >= 5.69) {
                    if (diffRsCsdiff < 48.21) {
                        if (ssDCval < 72.23) {
                            if (Rs < 11.47) {
                                return 1;
                            }
                            else if (Rs >= 11.47) {
                                return 0;
                            }
                        }
                        else if (ssDCval >= 72.23) {
                            if (RsCsDiff < 241.19) {
                                return 0;
                            }
                            else if (RsCsDiff >= 241.19) {
                                return 1;
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 48.21) {
                        if (MVDiff < 289.52) {
                            if (diffTSC < 8.4) {
                                if (Rs < 15.65) {
                                    return 1;
                                }
                                else if (Rs >= 15.65) {
                                    if (refDCval < 106.86) {
                                        if (diffMVdiffVal < 144.61) {
                                            if (CsDiff < 279.93) {
                                                return 0;
                                            }
                                            else if (CsDiff >= 279.93) {
                                                return 1;
                                            }
                                        }
                                        else if (diffMVdiffVal >= 144.61) {
                                            return 1;
                                        }
                                    }
                                    else if (refDCval >= 106.86) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffTSC >= 8.4) {
                                return 1;
                            }
                        }
                        else if (MVDiff >= 289.52) {
                            if (RsCsDiff < 501.31) {
                                if (diffRsCsdiff < 76.24) {
                                    if (posBalance < 0.43) {
                                        return 0;
                                    }
                                    else if (posBalance >= 0.43) {
                                        return 1;
                                    }
                                }
                                else if (diffRsCsdiff >= 76.24) {
                                    return 1;
                                }
                            }
                            else if (RsCsDiff >= 501.31) {
                                if (diffMVdiffVal < 214.35) {
                                    return 0;
                                }
                                else if (diffMVdiffVal >= 214.35) {
                                    if (RsCsDiff < 534.82) {
                                        return 0;
                                    }
                                    else if (RsCsDiff >= 534.82) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (posBalance >= 0.62) {
                if (diffAFD < 28.93) {
                    if (RsCsDiff < 411.47) {
                        if (SC < 20.4) {
                            if (diffRsCsdiff < 124.18) {
                                if (Rs < 13.64) {
                                    if (diffAFD < 14.71) {
                                        return 0;
                                    }
                                    else if (diffAFD >= 14.71) {
                                        if (gchDC < 28.44) {
                                            return 1;
                                        }
                                        else if (gchDC >= 28.44) {
                                            if (CsDiff < 96.21) {
                                                return 0;
                                            }
                                            else if (CsDiff >= 96.21) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (Rs >= 13.64) {
                                    return 0;
                                }
                            }
                            else if (diffRsCsdiff >= 124.18) {
                                return 1;
                            }
                        }
                        else if (SC >= 20.4) {
                            if (ssDCval < 87.38) {
                                if (Rs < 20.63) {
                                    return 0;
                                }
                                else if (Rs >= 20.63) {
                                    return 1;
                                }
                            }
                            else if (ssDCval >= 87.38) {
                                return 0;
                            }
                        }
                    }
                    else if (RsCsDiff >= 411.47) {
                        return 1;
                    }
                }
                else if (diffAFD >= 28.93) {
                    if (CsDiff < 43.32) {
                        if (Rs < 13.19) {
                            return 1;
                        }
                        else if (Rs >= 13.19) {
                            return 0;
                        }
                    }
                    else if (CsDiff >= 43.32) {
                        if (refDCval < 51.79) {
                            if (CsDiff < 67.76) {
                                if (AFD < 41.24) {
                                    return 1;
                                }
                                else if (AFD >= 41.24) {
                                    return 0;
                                }
                            }
                            else if (CsDiff >= 67.76) {
                                if (SC < 38.59) {
                                    if (gchDC < 45.67) {
                                        if (AFD < 40.06) {
                                            return 1;
                                        }
                                        else if (AFD >= 40.06) {
                                            return 0;
                                        }
                                    }
                                    else if (gchDC >= 45.67) {
                                        if (diffRsCsdiff < 45.44) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 45.44) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (SC >= 38.59) {
                                    return 0;
                                }
                            }
                        }
                        else if (refDCval >= 51.79) {
                            if (negBalance < 0.02) {
                                return 0;
                            }
                            else if (negBalance >= 0.02) {
                                if (negBalance < 0.96) {
                                    return 1;
                                }
                                else if (negBalance >= 0.96) {
                                    if (TSC < 19.6) {
                                        return 0;
                                    }
                                    else if (TSC >= 19.6) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect5(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    Scindex;

    if (diffMVdiffVal < 100.02) {
        if (TSC < 11.66) {
            if (diffRsCsdiff < 132.55) {
                if (ssDCval < 17.19) {
                    if (diffAFD < 0.46) {
                        return 0;
                    }
                    else if (diffAFD >= 0.46) {
                        if (Rs < 4.87) {
                            return 0;
                        }
                        else if (Rs >= 4.87) {
                            return 1;
                        }
                    }
                }
                else if (ssDCval >= 17.19) {
                    if (diffMVdiffVal < 64.12) {
                        if (gchDC < 23.54) {
                            return 0;
                        }
                        else if (gchDC >= 23.54) {
                            if (diffTSC < 2.06) {
                                return 0;
                            }
                            else if (diffTSC >= 2.06) {
                                if (SC < 18.84) {
                                    return 1;
                                }
                                else if (SC >= 18.84) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 64.12) {
                        if (diffTSC < 1.74) {
                            return 0;
                        }
                        else if (diffTSC >= 1.74) {
                            if (SC < 19.71) {
                                if (diffAFD < 9.85) {
                                    return 0;
                                }
                                else if (diffAFD >= 9.85) {
                                    if (diffMVdiffVal < 75.27) {
                                        return 1;
                                    }
                                    else if (diffMVdiffVal >= 75.27) {
                                        if (MVDiff < 240.93) {
                                            return 0;
                                        }
                                        else if (MVDiff >= 240.93) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                            else if (SC >= 19.71) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 132.55) {
                if (Cs < 12.69) {
                    if (diffRsCsdiff < 234.64) {
                        if (diffMVdiffVal < 54.12) {
                            return 0;
                        }
                        else if (diffMVdiffVal >= 54.12) {
                            if (ssDCval < 50.91) {
                                if (Cs < 11.12) {
                                    return 1;
                                }
                                else if (Cs >= 11.12) {
                                    return 0;
                                }
                            }
                            else if (ssDCval >= 50.91) {
                                return 0;
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 234.64) {
                        return 1;
                    }
                }
                else if (Cs >= 12.69) {
                    if (diffMVdiffVal < 83.65) {
                        return 0;
                    }
                    else if (diffMVdiffVal >= 83.65) {
                        if (AFD < 17.87) {
                            if (gchDC < 0.62) {
                                return 0;
                            }
                            else if (gchDC >= 0.62) {
                                return 1;
                            }
                        }
                        else if (AFD >= 17.87) {
                            if (Cs < 14.87) {
                                return 1;
                            }
                            else if (Cs >= 14.87) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        else if (TSC >= 11.66) {
            if (diffRsCsdiff < 147.28) {
                if (CsDiff < 376.73) {
                    if (SC < 19.53) {
                        if (gchDC < 3.73) {
                            if (diffTSC < 0.21) {
                                return 0;
                            }
                            else if (diffTSC >= 0.21) {
                                return 1;
                            }
                        }
                        else if (gchDC >= 3.73) {
                            if (ssDCval < 39.27) {
                                if (ssDCval < 34.35) {
                                    return 0;
                                }
                                else if (ssDCval >= 34.35) {
                                    return 1;
                                }
                            }
                            else if (ssDCval >= 39.27) {
                                if (refDCval < 115.67) {
                                    return 0;
                                }
                                else if (refDCval >= 115.67) {
                                    if (Cs < 9) {
                                        return 1;
                                    }
                                    else if (Cs >= 9) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (SC >= 19.53) {
                        if (MVDiff < 437.76) {
                            return 0;
                        }
                        else if (MVDiff >= 437.76) {
                            if (AFD < 28.78) {
                                return 1;
                            }
                            else if (AFD >= 28.78) {
                                return 0;
                            }
                        }
                    }
                }
                else if (CsDiff >= 376.73) {
                    if (AFD < 58.7) {
                        return 0;
                    }
                    else if (AFD >= 58.7) {
                        if (diffRsCsdiff < 0.89) {
                            return 1;
                        }
                        else if (diffRsCsdiff >= 0.89) {
                            return 0;
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 147.28) {
                if (diffAFD < 17.68) {
                    if (SC < 26.4) {
                        if (Cs < 14.03) {
                            if (diffTSC < 6.98) {
                                if (RsCsDiff < 577.86) {
                                    return 1;
                                }
                                else if (RsCsDiff >= 577.86) {
                                    return 0;
                                }
                            }
                            else if (diffTSC >= 6.98) {
                                return 0;
                            }
                        }
                        else if (Cs >= 14.03) {
                            return 1;
                        }
                    }
                    else if (SC >= 26.4) {
                        return 0;
                    }
                }
                else if (diffAFD >= 17.68) {
                    if (posBalance < 0.43) {
                        if (SC < 37.97) {
                            if (gchDC < 16.5) {
                                if (MVDiff < 103.77) {
                                    if (SC < 31.93) {
                                        return 0;
                                    }
                                    else if (SC >= 31.93) {
                                        return 1;
                                    }
                                }
                                else if (MVDiff >= 103.77) {
                                    return 1;
                                }
                            }
                            else if (gchDC >= 16.5) {
                                if (AFD < 57.82) {
                                    if (diffRsCsdiff < 279.42) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 279.42) {
                                        if (Cs < 20.44) {
                                            return 1;
                                        }
                                        else if (Cs >= 20.44) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (AFD >= 57.82) {
                                    return 1;
                                }
                            }
                        }
                        else if (SC >= 37.97) {
                            if (MVDiff < 198.44) {
                                if (RsCsDiff < 183.93) {
                                    return 1;
                                }
                                else if (RsCsDiff >= 183.93) {
                                    return 0;
                                }
                            }
                            else if (MVDiff >= 198.44) {
                                return 1;
                            }
                        }
                    }
                    else if (posBalance >= 0.43) {
                        if (negBalance < 0.12) {
                            return 0;
                        }
                        else if (negBalance >= 0.12) {
                            if (AFD < 60.3) {
                                if (posBalance < 0.71) {
                                    return 1;
                                }
                                else if (posBalance >= 0.71) {
                                    if (Rs < 19.85) {
                                        return 1;
                                    }
                                    else if (Rs >= 19.85) {
                                        return 0;
                                    }
                                }
                            }
                            else if (AFD >= 60.3) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 100.02) {
        if (diffTSC < 4.16) {
            if (diffAFD < 8.19) {
                if (CsDiff < 184.26) {
                    if (ssDCval < 234.84) {
                        if (refDCval < 52.97) {
                            if (CsDiff < 85.88) {
                                if (diffAFD < 7.55) {
                                    return 0;
                                }
                                else if (diffAFD >= 7.55) {
                                    return 1;
                                }
                            }
                            else if (CsDiff >= 85.88) {
                                if (TSC < 4.38) {
                                    return 0;
                                }
                                else if (TSC >= 4.38) {
                                    return 1;
                                }
                            }
                        }
                        else if (refDCval >= 52.97) {
                            return 0;
                        }
                    }
                    else if (ssDCval >= 234.84) {
                        if (Rs < 29.36) {
                            return 1;
                        }
                        else if (Rs >= 29.36) {
                            return 0;
                        }
                    }
                }
                else if (CsDiff >= 184.26) {
                    if (AFD < 38.4) {
                        if (AFD < 14.43) {
                            return 1;
                        }
                        else if (AFD >= 14.43) {
                            return 0;
                        }
                    }
                    else if (AFD >= 38.4) {
                        if (refDCval < 138.43) {
                            if (RsDiff < 397.71) {
                                return 1;
                            }
                            else if (RsDiff >= 397.71) {
                                return 0;
                            }
                        }
                        else if (refDCval >= 138.43) {
                            return 0;
                        }
                    }
                }
            }
            else if (diffAFD >= 8.19) {
                if (Rs < 16.42) {
                    if (TSCindex < 6.5) {
                        return 1;
                    }
                    else if (TSCindex >= 6.5) {
                        if (diffRsCsdiff < 92.85) {
                            if (negBalance < 0.54) {
                                if (RsDiff < 73.87) {
                                    return 1;
                                }
                                else if (RsDiff >= 73.87) {
                                    return 0;
                                }
                            }
                            else if (negBalance >= 0.54) {
                                return 0;
                            }
                        }
                        else if (diffRsCsdiff >= 92.85) {
                            if (MVDiff < 350.48) {
                                return 1;
                            }
                            else if (MVDiff >= 350.48) {
                                return 0;
                            }
                        }
                    }
                }
                else if (Rs >= 16.42) {
                    return 0;
                }
            }
        }
        else if (diffTSC >= 4.16) {
            if (diffRsCsdiff < 52.43) {
                if (SC < 12.77) {
                    if (MVDiff < 255.06) {
                        return 1;
                    }
                    else if (MVDiff >= 255.06) {
                        if (SC < 10.9) {
                            return 0;
                        }
                        else if (SC >= 10.9) {
                            if (diffRsCsdiff < 23.57) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 23.57) {
                                return 1;
                            }
                        }
                    }
                }
                else if (SC >= 12.77) {
                    if (AFD < 63.98) {
                        if (CsDiff < 168.75) {
                            return 0;
                        }
                        else if (CsDiff >= 168.75) {
                            if (MVDiff < 169.88) {
                                return 1;
                            }
                            else if (MVDiff >= 169.88) {
                                return 0;
                            }
                        }
                    }
                    else if (AFD >= 63.98) {
                        if (RsCsDiff < 333.15) {
                            return 0;
                        }
                        else if (RsCsDiff >= 333.15) {
                            return 1;
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 52.43) {
                if (gchDC < 26.93) {
                    if (diffAFD < 25.35) {
                        if (ssDCval < 52.66) {
                            if (posBalance < 0) {
                                if (Cs < 11.85) {
                                    return 1;
                                }
                                else if (Cs >= 11.85) {
                                    return 0;
                                }
                            }
                            else if (posBalance >= 0) {
                                return 1;
                            }
                        }
                        else if (ssDCval >= 52.66) {
                            if (CsDiff < 41.52) {
                                return 0;
                            }
                            else if (CsDiff >= 41.52) {
                                if (diffTSC < 7.33) {
                                    if (SC < 20.3) {
                                        if (RsCsDiff < 80.53) {
                                            return 0;
                                        }
                                        else if (RsCsDiff >= 80.53) {
                                            return 1;
                                        }
                                    }
                                    else if (SC >= 20.3) {
                                        if (diffRsCsdiff < 212.39) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 212.39) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (diffTSC >= 7.33) {
                                    if (MVDiff < 122.67) {
                                        return 0;
                                    }
                                    else if (MVDiff >= 122.67) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 25.35) {
                        if (CsDiff < 1110.67) {
                            if (AFD < 31.9) {
                                if (Cs < 20.73) {
                                    return 1;
                                }
                                else if (Cs >= 20.73) {
                                    if (TSC < 12.07) {
                                        return 0;
                                    }
                                    else if (TSC >= 12.07) {
                                        return 1;
                                    }
                                }
                            }
                            else if (AFD >= 31.9) {
                                return 1;
                            }
                        }
                        else if (CsDiff >= 1110.67) {
                            if (TSC < 31.62) {
                                return 1;
                            }
                            else if (TSC >= 31.62) {
                                return 0;
                            }
                        }
                    }
                }
                else if (gchDC >= 26.93) {
                    if (negBalance < 0.03) {
                        if (diffRsCsdiff < 345.95) {
                            if (SC < 17.15) {
                                return 1;
                            }
                            else if (SC >= 17.15) {
                                if (AFD < 15.78) {
                                    return 1;
                                }
                                else if (AFD >= 15.78) {
                                    if (RsDiff < 66.14) {
                                        return 1;
                                    }
                                    else if (RsDiff >= 66.14) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 345.95) {
                            if (SC < 35.65) {
                                if (TSC < 15.26) {
                                    return 0;
                                }
                                else if (TSC >= 15.26) {
                                    return 1;
                                }
                            }
                            else if (SC >= 35.65) {
                                return 0;
                            }
                        }
                    }
                    else if (negBalance >= 0.03) {
                        if (diffMVdiffVal < 128.43) {
                            if (diffAFD < 42.77) {
                                if (posBalance < 0.41) {
                                    if (Cs < 18.82) {
                                        return 0;
                                    }
                                    else if (Cs >= 18.82) {
                                        return 1;
                                    }
                                }
                                else if (posBalance >= 0.41) {
                                    return 0;
                                }
                            }
                            else if (diffAFD >= 42.77) {
                                return 1;
                            }
                        }
                        else if (diffMVdiffVal >= 128.43) {
                            if (refDCval < 171.19) {
                                if (diffRsCsdiff < 183.61) {
                                    if (negBalance < 0.72) {
                                        if (diffMVdiffVal < 156.27) {
                                            if (Cs < 7.14) {
                                                return 1;
                                            }
                                            else if (Cs >= 7.14) {
                                                return 0;
                                            }
                                        }
                                        else if (diffMVdiffVal >= 156.27) {
                                            if (negBalance < 0.71) {
                                                if (TSC < 36.98) {
                                                    return 1;
                                                }
                                                else if (TSC >= 36.98) {
                                                    if (Cs < 10.67) {
                                                        return 1;
                                                    }
                                                    else if (Cs >= 10.67) {
                                                        return 0;
                                                    }
                                                }
                                            }
                                            else if (negBalance >= 0.71) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (negBalance >= 0.72) {
                                        return 1;
                                    }
                                }
                                else if (diffRsCsdiff >= 183.61) {
                                    if (diffAFD < 30.69) {
                                        if (refDCval < 60.17) {
                                            if (refDCval < 50.64) {
                                                return 1;
                                            }
                                            else if (refDCval >= 50.64) {
                                                return 0;
                                            }
                                        }
                                        else if (refDCval >= 60.17) {
                                            return 1;
                                        }
                                    }
                                    else if (diffAFD >= 30.69) {
                                        return 1;
                                    }
                                }
                            }
                            else if (refDCval >= 171.19) {
                                if (diffAFD < 26.48) {
                                    if (RsDiff < 425.45) {
                                        return 0;
                                    }
                                    else if (RsDiff >= 425.45) {
                                        return 1;
                                    }
                                }
                                else if (diffAFD >= 26.48) {
                                    if (Rs < 19.94) {
                                        return 0;
                                    }
                                    else if (Rs >= 19.94) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect6(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    Scindex;
    RsCsDiff;

    if (diffRsCsdiff < 88.44) {
        if (diffMVdiffVal < 118) {
            if (TSC < 9.34) {
                if (ssDCval < 17.33) {
                    if (diffTSC < 0.38) {
                        return 0;
                    }
                    else if (diffTSC >= 0.38) {
                        if (CsDiff < 1.48) {
                            return 0;
                        }
                        else if (CsDiff >= 1.48) {
                            return 1;
                        }
                    }
                }
                else if (ssDCval >= 17.33) {
                    if (gchDC < 3.06) {
                        return 0;
                    }
                    else if (gchDC >= 3.06) {
                        if (diffAFD < 9) {
                            return 0;
                        }
                        else if (diffAFD >= 9) {
                            if (SC < 17.38) {
                                if (negBalance < 0.5) {
                                    if (Cs < 12.32) {
                                        return 0;
                                    }
                                    else if (Cs >= 12.32) {
                                        if (Rs < 10.03) {
                                            return 0;
                                        }
                                        else if (Rs >= 10.03) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (negBalance >= 0.5) {
                                    if (posBalance < 0.65) {
                                        if (Rs < 11.41) {
                                            return 1;
                                        }
                                        else if (Rs >= 11.41) {
                                            return 0;
                                        }
                                    }
                                    else if (posBalance >= 0.65) {
                                        return 0;
                                    }
                                }
                            }
                            else if (SC >= 17.38) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (TSC >= 9.34) {
                if (MVDiff < 269.71) {
                    if (diffTSC < 7.77) {
                        return 0;
                    }
                    else if (diffTSC >= 7.77) {
                        if (AFD < 11.19) {
                            return 1;
                        }
                        else if (AFD >= 11.19) {
                            if (posBalance < 0.09) {
                                if (Rs < 14.6) {
                                    return 1;
                                }
                                else if (Rs >= 14.6) {
                                    return 0;
                                }
                            }
                            else if (posBalance >= 0.09) {
                                return 0;
                            }
                        }
                    }
                }
                else if (MVDiff >= 269.71) {
                    if (posBalance < 0.44) {
                        if (ssDCval < 55.51) {
                            return 1;
                        }
                        else if (ssDCval >= 55.51) {
                            if (CsDiff < 362.48) {
                                if (refDCval < 49.52) {
                                    return 1;
                                }
                                else if (refDCval >= 49.52) {
                                    if (Rs < 33.51) {
                                        if (TSC < 16.31) {
                                            return 0;
                                        }
                                        else if (TSC >= 16.31) {
                                            if (Cs < 13.63) {
                                                return 1;
                                            }
                                            else if (Cs >= 13.63) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (Rs >= 33.51) {
                                        return 1;
                                    }
                                }
                            }
                            else if (CsDiff >= 362.48) {
                                if (Cs < 23.86) {
                                    return 0;
                                }
                                else if (Cs >= 23.86) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (posBalance >= 0.44) {
                        return 0;
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 118) {
            if (diffAFD < 7.63) {
                if (CsDiff < 185.2) {
                    if (ssDCval < 233.99) {
                        if (diffRsCsdiff < 29.12) {
                            return 0;
                        }
                        else if (diffRsCsdiff >= 29.12) {
                            if (RsDiff < 27.84) {
                                return 1;
                            }
                            else if (RsDiff >= 27.84) {
                                return 0;
                            }
                        }
                    }
                    else if (ssDCval >= 233.99) {
                        if (Rs < 29.36) {
                            return 1;
                        }
                        else if (Rs >= 29.36) {
                            return 0;
                        }
                    }
                }
                else if (CsDiff >= 185.2) {
                    if (refDCval < 138.43) {
                        if (negBalance < 0.73) {
                            return 1;
                        }
                        else if (negBalance >= 0.73) {
                            return 0;
                        }
                    }
                    else if (refDCval >= 138.43) {
                        if (RsDiff < 524.34) {
                            return 0;
                        }
                        else if (RsDiff >= 524.34) {
                            return 1;
                        }
                    }
                }
            }
            else if (diffAFD >= 7.63) {
                if (Rs < 12.33) {
                    if (gchDC < 13.74) {
                        if (diffRsCsdiff < 8.36) {
                            return 0;
                        }
                        else if (diffRsCsdiff >= 8.36) {
                            if (TSC < 7.18) {
                                return 1;
                            }
                            else if (TSC >= 7.18) {
                                if (Cs < 5.06) {
                                    return 0;
                                }
                                else if (Cs >= 5.06) {
                                    if (CsDiff < 34.46) {
                                        if (Cs < 7.42) {
                                            return 1;
                                        }
                                        else if (Cs >= 7.42) {
                                            return 0;
                                        }
                                    }
                                    else if (CsDiff >= 34.46) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                    else if (gchDC >= 13.74) {
                        if (diffAFD < 13.68) {
                            return 0;
                        }
                        else if (diffAFD >= 13.68) {
                            if (posBalance < 0.89) {
                                if (AFD < 34.95) {
                                    return 1;
                                }
                                else if (AFD >= 34.95) {
                                    if (Rs < 10.01) {
                                        return 0;
                                    }
                                    else if (Rs >= 10.01) {
                                        return 1;
                                    }
                                }
                            }
                            else if (posBalance >= 0.89) {
                                return 0;
                            }
                        }
                    }
                }
                else if (Rs >= 12.33) {
                    if (diffMVdiffVal < 118.65) {
                        return 1;
                    }
                    else if (diffMVdiffVal >= 118.65) {
                        if (diffAFD < 24.44) {
                            return 0;
                        }
                        else if (diffAFD >= 24.44) {
                            if (negBalance < 0.32) {
                                return 0;
                            }
                            else if (negBalance >= 0.32) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffRsCsdiff >= 88.44) {
        if (diffMVdiffVal < 107.46) {
            if (TSCindex < 8.5) {
                if (ssDCval < 43.89) {
                    if (AFD < 8.11) {
                        if (AFD < 3.59) {
                            return 0;
                        }
                        else if (AFD >= 3.59) {
                            return 1;
                        }
                    }
                    else if (AFD >= 8.11) {
                        if (Cs < 5.71) {
                            if (AFD < 20.43) {
                                return 0;
                            }
                            else if (AFD >= 20.43) {
                                return 1;
                            }
                        }
                        else if (Cs >= 5.71) {
                            return 0;
                        }
                    }
                }
                else if (ssDCval >= 43.89) {
                    if (MVDiff < 138.27) {
                        return 0;
                    }
                    else if (MVDiff >= 138.27) {
                        if (TSC < 5.39) {
                            if (Rs < 27.58) {
                                if (Rs < 24.04) {
                                    return 0;
                                }
                                else if (Rs >= 24.04) {
                                    return 1;
                                }
                            }
                            else if (Rs >= 27.58) {
                                return 0;
                            }
                        }
                        else if (TSC >= 5.39) {
                            return 0;
                        }
                    }
                }
            }
            else if (TSCindex >= 8.5) {
                if (MVDiff < 103.41) {
                    return 0;
                }
                else if (MVDiff >= 103.41) {
                    if (diffRsCsdiff < 199.89) {
                        if (Rs < 18.44) {
                            if (gchDC < 13.63) {
                                if (refDCval < 83.77) {
                                    if (Rs < 17.03) {
                                        return 1;
                                    }
                                    else if (Rs >= 17.03) {
                                        if (posBalance < 0.25) {
                                            return 0;
                                        }
                                        else if (posBalance >= 0.25) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (refDCval >= 83.77) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 13.63) {
                                if (ssDCval < 109.21) {
                                    if (ssDCval < 43.02) {
                                        if (AFD < 33.13) {
                                            return 0;
                                        }
                                        else if (AFD >= 33.13) {
                                            return 1;
                                        }
                                    }
                                    else if (ssDCval >= 43.02) {
                                        return 0;
                                    }
                                }
                                else if (ssDCval >= 109.21) {
                                    return 1;
                                }
                            }
                        }
                        else if (Rs >= 18.44) {
                            if (gchDC < 0.6) {
                                if (Rs < 21.39) {
                                    return 1;
                                }
                                else if (Rs >= 21.39) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 0.6) {
                                if (diffMVdiffVal < -8.65) {
                                    if (negBalance < 0.29) {
                                        return 0;
                                    }
                                    else if (negBalance >= 0.29) {
                                        if (diffRsCsdiff < 149.83) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 149.83) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (diffMVdiffVal >= -8.65) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 199.89) {
                        if (Rs < 21.39) {
                            if (gchDC < 33.37) {
                                if (AFD < 30.62) {
                                    if (Rs < 17.57) {
                                        if (Rs < 12.5) {
                                            return 0;
                                        }
                                        else if (Rs >= 12.5) {
                                            return 1;
                                        }
                                    }
                                    else if (Rs >= 17.57) {
                                        return 0;
                                    }
                                }
                                else if (AFD >= 30.62) {
                                    if (refDCval < 42.15) {
                                        return 0;
                                    }
                                    else if (refDCval >= 42.15) {
                                        return 1;
                                    }
                                }
                            }
                            else if (gchDC >= 33.37) {
                                if (SC < 24.68) {
                                    return 0;
                                }
                                else if (SC >= 24.68) {
                                    if (diffTSC < 24.19) {
                                        if (diffMVdiffVal < 77.81) {
                                            if (SC < 28.33) {
                                                return 1;
                                            }
                                            else if (SC >= 28.33) {
                                                return 0;
                                            }
                                        }
                                        else if (diffMVdiffVal >= 77.81) {
                                            return 1;
                                        }
                                    }
                                    else if (diffTSC >= 24.19) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (Rs >= 21.39) {
                            if (CsDiff < 166.06) {
                                return 0;
                            }
                            else if (CsDiff >= 166.06) {
                                if (diffAFD < 53.06) {
                                    if (diffTSC < 12.39) {
                                        if (MVDiff < 436.2) {
                                            return 0;
                                        }
                                        else if (MVDiff >= 436.2) {
                                            if (Rs < 24.76) {
                                                return 0;
                                            }
                                            else if (Rs >= 24.76) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (diffTSC >= 12.39) {
                                        if (posBalance < 0.68) {
                                            if (AFD < 53.83) {
                                                return 1;
                                            }
                                            else if (AFD >= 53.83) {
                                                if (refDCval < 76.64) {
                                                    return 1;
                                                }
                                                else if (refDCval >= 76.64) {
                                                    if (Cs < 18.89) {
                                                        return 1;
                                                    }
                                                    else if (Cs >= 18.89) {
                                                        return 0;
                                                    }
                                                }
                                            }
                                        }
                                        else if (posBalance >= 0.68) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (diffAFD >= 53.06) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 107.46) {
            if (negBalance < 0.08) {
                if (diffAFD < 42.26) {
                    if (CsDiff < 253.49) {
                        if (gchDC < 19.87) {
                            return 1;
                        }
                        else if (gchDC >= 19.87) {
                            if (gchDC < 59.91) {
                                return 0;
                            }
                            else if (gchDC >= 59.91) {
                                if (Rs < 20.8) {
                                    return 1;
                                }
                                else if (Rs >= 20.8) {
                                    if (Rs < 33.82) {
                                        return 0;
                                    }
                                    else if (Rs >= 33.82) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                    else if (CsDiff >= 253.49) {
                        if (diffMVdiffVal < 203.19) {
                            return 1;
                        }
                        else if (diffMVdiffVal >= 203.19) {
                            return 0;
                        }
                    }
                }
                else if (diffAFD >= 42.26) {
                    if (SC < 42.96) {
                        if (negBalance < 0.05) {
                            if (RsDiff < 76.55) {
                                if (Rs < 14.25) {
                                    return 1;
                                }
                                else if (Rs >= 14.25) {
                                    return 0;
                                }
                            }
                            else if (RsDiff >= 76.55) {
                                return 1;
                            }
                        }
                        else if (negBalance >= 0.05) {
                            return 0;
                        }
                    }
                    else if (SC >= 42.96) {
                        return 0;
                    }
                }
            }
            else if (negBalance >= 0.08) {
                if (diffMVdiffVal < 121.79) {
                    if (Rs < 19.91) {
                        if (MVDiff < 270.08) {
                            return 1;
                        }
                        else if (MVDiff >= 270.08) {
                            if (diffRsCsdiff < 114.83) {
                                return 1;
                            }
                            else if (diffRsCsdiff >= 114.83) {
                                return 0;
                            }
                        }
                    }
                    else if (Rs >= 19.91) {
                        if (diffTSC < 10.89) {
                            return 0;
                        }
                        else if (diffTSC >= 10.89) {
                            if (Cs < 26.61) {
                                return 1;
                            }
                            else if (Cs >= 26.61) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 121.79) {
                    if (MVDiff < 313.35) {
                        if (diffTSC < 12.18) {
                            if (SC < 24.2) {
                                return 1;
                            }
                            else if (SC >= 24.2) {
                                if (diffAFD < 32.02) {
                                    if (refDCval < 81.28) {
                                        return 1;
                                    }
                                    else if (refDCval >= 81.28) {
                                        if (diffTSC < 6.44) {
                                            return 0;
                                        }
                                        else if (diffTSC >= 6.44) {
                                            if (diffAFD < 31.64) {
                                                if (RsDiff < 147.63) {
                                                    if (AFD < 26.47) {
                                                        return 0;
                                                    }
                                                    else if (AFD >= 26.47) {
                                                        return 1;
                                                    }
                                                }
                                                else if (RsDiff >= 147.63) {
                                                    return 1;
                                                }
                                            }
                                            else if (diffAFD >= 31.64) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                                else if (diffAFD >= 32.02) {
                                    return 1;
                                }
                            }
                        }
                        else if (diffTSC >= 12.18) {
                            if (posBalance < 0.92) {
                                return 1;
                            }
                            else if (posBalance >= 0.92) {
                                return 0;
                            }
                        }
                    }
                    else if (MVDiff >= 313.35) {
                        if (diffMVdiffVal < 247.27) {
                            if (diffAFD < 26.39) {
                                if (MVDiff < 337.48) {
                                    if (MVDiff < 322.74) {
                                        if (Rs < 8) {
                                            return 1;
                                        }
                                        else if (Rs >= 8) {
                                            return 0;
                                        }
                                    }
                                    else if (MVDiff >= 322.74) {
                                        return 1;
                                    }
                                }
                                else if (MVDiff >= 337.48) {
                                    if (diffRsCsdiff < 376.74) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 376.74) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffAFD >= 26.39) {
                                if (Rs < 25.78) {
                                    return 1;
                                }
                                else if (Rs >= 25.78) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffMVdiffVal >= 247.27) {
                            if (CsDiff < 59.26) {
                                if (AFD < 39.6) {
                                    return 1;
                                }
                                else if (AFD >= 39.6) {
                                    return 0;
                                }
                            }
                            else if (CsDiff >= 59.26) {
                                if (diffMVdiffVal < 276.74) {
                                    if (Cs < 20.71) {
                                        return 1;
                                    }
                                    else if (Cs >= 20.71) {
                                        return 0;
                                    }
                                }
                                else if (diffMVdiffVal >= 276.74) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect7(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    TSCindex;

    if (diffMVdiffVal < 103.5) {
        if (AFD < 26.4) {
            if (diffMVdiffVal < 54.84) {
                if (MVDiff < 212.2) {
                    if (ssDCval < 19.6) {
                        if (diffAFD < 0.52) {
                            return 0;
                        }
                        else if (diffAFD >= 0.52) {
                            if (Rs < 6.76) {
                                return 0;
                            }
                            else if (Rs >= 6.76) {
                                if (negBalance < 0.01) {
                                    return 0;
                                }
                                else if (negBalance >= 0.01) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (ssDCval >= 19.6) {
                        return 0;
                    }
                }
                else if (MVDiff >= 212.2) {
                    if (diffTSC < 4.08) {
                        if (TSC < 12.36) {
                            return 0;
                        }
                        else if (TSC >= 12.36) {
                            if (Rs < 9.11) {
                                return 1;
                            }
                            else if (Rs >= 9.11) {
                                return 0;
                            }
                        }
                    }
                    else if (diffTSC >= 4.08) {
                        if (negBalance < 0.42) {
                            return 0;
                        }
                        else if (negBalance >= 0.42) {
                            if (MVDiff < 223.2) {
                                return 1;
                            }
                            else if (MVDiff >= 223.2) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (diffMVdiffVal >= 54.84) {
                if (refDCval < 21.15) {
                    if (SC < 6.23) {
                        return 0;
                    }
                    else if (SC >= 6.23) {
                        return 1;
                    }
                }
                else if (refDCval >= 21.15) {
                    if (diffAFD < 7.28) {
                        return 0;
                    }
                    else if (diffAFD >= 7.28) {
                        if (ssDCval < 39.49) {
                            if (Cs < 12.04) {
                                if (gchDC < 13.91) {
                                    return 1;
                                }
                                else if (gchDC >= 13.91) {
                                    return 0;
                                }
                            }
                            else if (Cs >= 12.04) {
                                return 0;
                            }
                        }
                        else if (ssDCval >= 39.49) {
                            if (posBalance < 0.38) {
                                if (refDCval < 55.86) {
                                    if (negBalance < 0.23) {
                                        return 0;
                                    }
                                    else if (negBalance >= 0.23) {
                                        return 1;
                                    }
                                }
                                else if (refDCval >= 55.86) {
                                    if (diffMVdiffVal < 80.01) {
                                        return 0;
                                    }
                                    else if (diffMVdiffVal >= 80.01) {
                                        if (MVDiff < 85) {
                                            return 1;
                                        }
                                        else if (MVDiff >= 85) {
                                            if (AFD < 10.69) {
                                                return 1;
                                            }
                                            else if (AFD >= 10.69) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (posBalance >= 0.38) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        else if (AFD >= 26.4) {
            if (diffAFD < 12.59) {
                if (TSC < 17.88) {
                    if (refDCval < 74.48) {
                        if (MVDiff < 187.18) {
                            return 0;
                        }
                        else if (MVDiff >= 187.18) {
                            if (posBalance < 0.15) {
                                if (posBalance < 0.15) {
                                    if (diffAFD < 2.86) {
                                        return 0;
                                    }
                                    else if (diffAFD >= 2.86) {
                                        if (refDCval < 69.53) {
                                            if (Rs < 9.64) {
                                                return 1;
                                            }
                                            else if (Rs >= 9.64) {
                                                return 0;
                                            }
                                        }
                                        else if (refDCval >= 69.53) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (posBalance >= 0.15) {
                                    return 1;
                                }
                            }
                            else if (posBalance >= 0.15) {
                                return 0;
                            }
                        }
                    }
                    else if (refDCval >= 74.48) {
                        return 0;
                    }
                }
                else if (TSC >= 17.88) {
                    if (posBalance < 0.29) {
                        if (RsDiff < 253.41) {
                            if (Cs < 16.3) {
                                return 1;
                            }
                            else if (Cs >= 16.3) {
                                return 0;
                            }
                        }
                        else if (RsDiff >= 253.41) {
                            if (MVDiff < 334.48) {
                                return 0;
                            }
                            else if (MVDiff >= 334.48) {
                                if (AFD < 65.68) {
                                    return 1;
                                }
                                else if (AFD >= 65.68) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (posBalance >= 0.29) {
                        if (SC < 15.13) {
                            return 1;
                        }
                        else if (SC >= 15.13) {
                            if (AFD < 75.11) {
                                return 0;
                            }
                            else if (AFD >= 75.11) {
                                return 1;
                            }
                        }
                    }
                }
            }
            else if (diffAFD >= 12.59) {
                if (diffRsCsdiff < 151.24) {
                    if (Rs < 15.62) {
                        if (gchDC < 16.27) {
                            if (MVDiff < 223.23) {
                                return 0;
                            }
                            else if (MVDiff >= 223.23) {
                                return 1;
                            }
                        }
                        else if (gchDC >= 16.27) {
                            return 0;
                        }
                    }
                    else if (Rs >= 15.62) {
                        if (diffMVdiffVal < 97.28) {
                            if (diffRsCsdiff < 144.88) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 144.88) {
                                if (CsDiff < 65.07) {
                                    return 1;
                                }
                                else if (CsDiff >= 65.07) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffMVdiffVal >= 97.28) {
                            if (SC < 29.19) {
                                return 1;
                            }
                            else if (SC >= 29.19) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffRsCsdiff >= 151.24) {
                    if (Rs < 19.63) {
                        if (RsDiff < 139.98) {
                            return 0;
                        }
                        else if (RsDiff >= 139.98) {
                            if (posBalance < 0.74) {
                                if (AFD < 29.59) {
                                    if (AFD < 27.31) {
                                        return 1;
                                    }
                                    else if (AFD >= 27.31) {
                                        return 0;
                                    }
                                }
                                else if (AFD >= 29.59) {
                                    if (diffMVdiffVal < -76.19) {
                                        return 0;
                                    }
                                    else if (diffMVdiffVal >= -76.19) {
                                        if (diffMVdiffVal < 30.32) {
                                            return 1;
                                        }
                                        else if (diffMVdiffVal >= 30.32) {
                                            if (MVDiff < 155.1) {
                                                return 0;
                                            }
                                            else if (MVDiff >= 155.1) {
                                                if (diffAFD < 21.12) {
                                                    return 0;
                                                }
                                                else if (diffAFD >= 21.12) {
                                                    return 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if (posBalance >= 0.74) {
                                if (MVDiff < 317.95) {
                                    return 0;
                                }
                                else if (MVDiff >= 317.95) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (Rs >= 19.63) {
                        if (AFD < 39.19) {
                            if (diffRsCsdiff < 167.21) {
                                if (Rs < 24.91) {
                                    return 0;
                                }
                                else if (Rs >= 24.91) {
                                    return 1;
                                }
                            }
                            else if (diffRsCsdiff >= 167.21) {
                                return 0;
                            }
                        }
                        else if (AFD >= 39.19) {
                            if (negBalance < 0.46) {
                                if (RsCsDiff < 483.49) {
                                    return 0;
                                }
                                else if (RsCsDiff >= 483.49) {
                                    if (posBalance < 0.23) {
                                        if (SC < 32.04) {
                                            return 1;
                                        }
                                        else if (SC >= 32.04) {
                                            return 0;
                                        }
                                    }
                                    else if (posBalance >= 0.23) {
                                        if (diffMVdiffVal < 70.59) {
                                            if (Rs < 30.7) {
                                                return 1;
                                            }
                                            else if (Rs >= 30.7) {
                                                if (Rs < 35.98) {
                                                    return 0;
                                                }
                                                else if (Rs >= 35.98) {
                                                    return 1;
                                                }
                                            }
                                        }
                                        else if (diffMVdiffVal >= 70.59) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (negBalance >= 0.46) {
                                if (Cs < 24.44) {
                                    if (CsDiff < 135.31) {
                                        if (Rs < 19.97) {
                                            return 1;
                                        }
                                        else if (Rs >= 19.97) {
                                            return 0;
                                        }
                                    }
                                    else if (CsDiff >= 135.31) {
                                        return 1;
                                    }
                                }
                                else if (Cs >= 24.44) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 103.5) {
        if (diffTSC < 3.57) {
            if (diffRsCsdiff < 42.31) {
                if (diffMVdiffVal < 399.71) {
                    if (RsCsDiff < 250.22) {
                        return 0;
                    }
                    else if (RsCsDiff >= 250.22) {
                        if (RsDiff < 234.19) {
                            if (MVDiff < 358.14) {
                                return 1;
                            }
                            else if (MVDiff >= 358.14) {
                                return 0;
                            }
                        }
                        else if (RsDiff >= 234.19) {
                            if (diffRsCsdiff < 6.54) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 6.54) {
                                return 1;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 399.71) {
                    if (Rs < 7.12) {
                        return 1;
                    }
                    else if (Rs >= 7.12) {
                        if (AFD < 21.89) {
                            return 0;
                        }
                        else if (AFD >= 21.89) {
                            return 1;
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 42.31) {
                if (ssDCval < 52.02) {
                    if (Cs < 15.91) {
                        if (diffTSC < 3.42) {
                            return 1;
                        }
                        else if (diffTSC >= 3.42) {
                            return 0;
                        }
                    }
                    else if (Cs >= 15.91) {
                        return 0;
                    }
                }
                else if (ssDCval >= 52.02) {
                    if (CsDiff < 174.21) {
                        return 0;
                    }
                    else if (CsDiff >= 174.21) {
                        if (RsDiff < 114.23) {
                            return 1;
                        }
                        else if (RsDiff >= 114.23) {
                            if (negBalance < 0.59) {
                                if (TSC < 5.32) {
                                    return 1;
                                }
                                else if (TSC >= 5.32) {
                                    return 0;
                                }
                            }
                            else if (negBalance >= 0.59) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        else if (diffTSC >= 3.57) {
            if (posBalance < 0.61) {
                if (MVDiff < 267.41) {
                    if (diffMVdiffVal < 117.95) {
                        if (diffRsCsdiff < 126.8) {
                            if (Rs < 8.31) {
                                return 1;
                            }
                            else if (Rs >= 8.31) {
                                return 0;
                            }
                        }
                        else if (diffRsCsdiff >= 126.8) {
                            if (diffMVdiffVal < 106.68) {
                                if (refDCval < 88.36) {
                                    if (TSC < 19.53) {
                                        return 1;
                                    }
                                    else if (TSC >= 19.53) {
                                        return 0;
                                    }
                                }
                                else if (refDCval >= 88.36) {
                                    return 0;
                                }
                            }
                            else if (diffMVdiffVal >= 106.68) {
                                return 1;
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 117.95) {
                        if (TSC < 12.11) {
                            if (SC < 25.89) {
                                if (MVDiff < 222.85) {
                                    return 1;
                                }
                                else if (MVDiff >= 222.85) {
                                    if (posBalance < 0.07) {
                                        if (Rs < 10.15) {
                                            return 0;
                                        }
                                        else if (Rs >= 10.15) {
                                            return 1;
                                        }
                                    }
                                    else if (posBalance >= 0.07) {
                                        return 1;
                                    }
                                }
                            }
                            else if (SC >= 25.89) {
                                if (AFD < 30.43) {
                                    if (SC < 28.37) {
                                        return 0;
                                    }
                                    else if (SC >= 28.37) {
                                        return 1;
                                    }
                                }
                                else if (AFD >= 30.43) {
                                    return 0;
                                }
                            }
                        }
                        else if (TSC >= 12.11) {
                            if (RsCsDiff < 1258.25) {
                                if (negBalance < 0.19) {
                                    if (diffTSC < 12.29) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 12.29) {
                                        return 1;
                                    }
                                }
                                else if (negBalance >= 0.19) {
                                    return 1;
                                }
                            }
                            else if (RsCsDiff >= 1258.25) {
                                if (RsDiff < 638.08) {
                                    return 0;
                                }
                                else if (RsDiff >= 638.08) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (MVDiff >= 267.41) {
                    if (diffAFD < 14.29) {
                        if (ssDCval < 56.89) {
                            if (posBalance < 0.45) {
                                return 1;
                            }
                            else if (posBalance >= 0.45) {
                                return 0;
                            }
                        }
                        else if (ssDCval >= 56.89) {
                            if (RsCsDiff < 318) {
                                if (refDCval < 58.27) {
                                    if (Rs < 15.02) {
                                        return 0;
                                    }
                                    else if (Rs >= 15.02) {
                                        return 1;
                                    }
                                }
                                else if (refDCval >= 58.27) {
                                    return 0;
                                }
                            }
                            else if (RsCsDiff >= 318) {
                                if (TSC < 16.08) {
                                    return 0;
                                }
                                else if (TSC >= 16.08) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 14.29) {
                        if (diffTSC < 9.91) {
                            if (AFD < 45.12) {
                                return 1;
                            }
                            else if (AFD >= 45.12) {
                                if (gchDC < 28.58) {
                                    if (CsDiff < 113.67) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 113.67) {
                                        return 1;
                                    }
                                }
                                else if (gchDC >= 28.58) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffTSC >= 9.91) {
                            if (Cs < 6.35) {
                                return 0;
                            }
                            else if (Cs >= 6.35) {
                                if (AFD < 87.95) {
                                    return 1;
                                }
                                else if (AFD >= 87.95) {
                                    if (Rs < 17.86) {
                                        return 0;
                                    }
                                    else if (Rs >= 17.86) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (posBalance >= 0.61) {
                if (diffMVdiffVal < 148.69) {
                    if (refDCval < 88.22) {
                        if (diffAFD < 47.53) {
                            return 0;
                        }
                        else if (diffAFD >= 47.53) {
                            return 1;
                        }
                    }
                    else if (refDCval >= 88.22) {
                        if (Cs < 16.44) {
                            if (refDCval < 130.53) {
                                return 1;
                            }
                            else if (refDCval >= 130.53) {
                                return 0;
                            }
                        }
                        else if (Cs >= 16.44) {
                            if (AFD < 57.22) {
                                return 0;
                            }
                            else if (AFD >= 57.22) {
                                return 1;
                            }
                        }
                    }
                }
                else if (diffMVdiffVal >= 148.69) {
                    if (MVDiff < 286.38) {
                        if (negBalance < 0.02) {
                            if (refDCval < 43.46) {
                                if (gchDC < 81.7) {
                                    if (AFD < 34.87) {
                                        return 0;
                                    }
                                    else if (AFD >= 34.87) {
                                        return 1;
                                    }
                                }
                                else if (gchDC >= 81.7) {
                                    return 1;
                                }
                            }
                            else if (refDCval >= 43.46) {
                                return 0;
                            }
                        }
                        else if (negBalance >= 0.02) {
                            if (diffAFD < 23.79) {
                                if (Scindex < 2.5) {
                                    return 1;
                                }
                                else if (Scindex >= 2.5) {
                                    if (RsCsDiff < 205.82) {
                                        return 0;
                                    }
                                    else if (RsCsDiff >= 205.82) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffAFD >= 23.79) {
                                if (CsDiff < 67.76) {
                                    if (SC < 22.22) {
                                        return 1;
                                    }
                                    else if (SC >= 22.22) {
                                        if (Rs < 20.07) {
                                            return 0;
                                        }
                                        else if (Rs >= 20.07) {
                                            if (Cs < 9.13) {
                                                return 0;
                                            }
                                            else if (Cs >= 9.13) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (CsDiff >= 67.76) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (MVDiff >= 286.38) {
                        if (diffRsCsdiff < 45.7) {
                            return 0;
                        }
                        else if (diffRsCsdiff >= 45.7) {
                            if (negBalance < 0.05) {
                                if (diffMVdiffVal < 237.22) {
                                    return 0;
                                }
                                else if (diffMVdiffVal >= 237.22) {
                                    if (ssDCval < 72.69) {
                                        return 0;
                                    }
                                    else if (ssDCval >= 72.69) {
                                        if (Rs < 34.89) {
                                            return 1;
                                        }
                                        else if (Rs >= 34.89) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (negBalance >= 0.05) {
                                if (diffAFD < 20.1) {
                                    return 0;
                                }
                                else if (diffAFD >= 20.1) {
                                    if (AFD < 41.26) {
                                        return 1;
                                    }
                                    else if (AFD >= 41.26) {
                                        if (Cs < 19.96) {
                                            if (Cs < 11.53) {
                                                return 0;
                                            }
                                            else if (Cs >= 11.53) {
                                                return 1;
                                            }
                                        }
                                        else if (Cs >= 19.96) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect8(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffMVdiffVal < 103.57) {
        if (TSC < 9.44) {
            if (ssDCval < 21.03) {
                if (CsDiff < 22.87) {
                    if (RsCsDiff < 4.56) {
                        return 0;
                    }
                    else if (RsCsDiff >= 4.56) {
                        if (diffTSC < 0.38) {
                            return 0;
                        }
                        else if (diffTSC >= 0.38) {
                            if (Rs < 5.12) {
                                return 0;
                            }
                            else if (Rs >= 5.12) {
                                return 1;
                            }
                        }
                    }
                }
                else if (CsDiff >= 22.87) {
                    if (diffTSC < 0.48) {
                        return 0;
                    }
                    else if (diffTSC >= 0.48) {
                        if (refDCval < 16.92) {
                            if (Cs < 8.27) {
                                return 1;
                            }
                            else if (Cs >= 8.27) {
                                return 0;
                            }
                        }
                        else if (refDCval >= 16.92) {
                            return 1;
                        }
                    }
                }
            }
            else if (ssDCval >= 21.03) {
                if (diffMVdiffVal < 65.25) {
                    if (gchDC < 22.02) {
                        return 0;
                    }
                    else if (gchDC >= 22.02) {
                        if (diffAFD < 23.69) {
                            return 0;
                        }
                        else if (diffAFD >= 23.69) {
                            return 1;
                        }
                    }
                }
                else if (diffMVdiffVal >= 65.25) {
                    if (diffAFD < 7.33) {
                        return 0;
                    }
                    else if (diffAFD >= 7.33) {
                        if (Rs < 12.16) {
                            if (diffAFD < 9.34) {
                                return 0;
                            }
                            else if (diffAFD >= 9.34) {
                                if (posBalance < 0.31) {
                                    return 1;
                                }
                                else if (posBalance >= 0.31) {
                                    if (Rs < 5.73) {
                                        return 1;
                                    }
                                    else if (Rs >= 5.73) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (Rs >= 12.16) {
                            if (refDCval < 170.82) {
                                return 0;
                            }
                            else if (refDCval >= 170.82) {
                                if (AFD < 15.96) {
                                    return 1;
                                }
                                else if (AFD >= 15.96) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (TSC >= 9.44) {
            if (diffAFD < 12.58) {
                if (TSC < 18.67) {
                    if (MVDiff < 196.4) {
                        return 0;
                    }
                    else if (MVDiff >= 196.4) {
                        if (refDCval < 48.32) {
                            if (CsDiff < 70.19) {
                                return 0;
                            }
                            else if (CsDiff >= 70.19) {
                                if (ssDCval < 48.31) {
                                    return 1;
                                }
                                else if (ssDCval >= 48.31) {
                                    return 0;
                                }
                            }
                        }
                        else if (refDCval >= 48.32) {
                            if (AFD < 11.89) {
                                return 1;
                            }
                            else if (AFD >= 11.89) {
                                if (posBalance < 0.15) {
                                    if (diffTSC < 2.66) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 2.66) {
                                        if (diffTSC < 3.06) {
                                            return 1;
                                        }
                                        else if (diffTSC >= 3.06) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (posBalance >= 0.15) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
                else if (TSC >= 18.67) {
                    if (AFD < 52.46) {
                        if (ssDCval < 44.59) {
                            return 1;
                        }
                        else if (ssDCval >= 44.59) {
                            if (gchDC < 9.47) {
                                if (Cs < 22.22) {
                                    return 1;
                                }
                                else if (Cs >= 22.22) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 9.47) {
                                return 0;
                            }
                        }
                    }
                    else if (AFD >= 52.46) {
                        if (TSC < 20.52) {
                            return 1;
                        }
                        else if (TSC >= 20.52) {
                            if (RsDiff < 390.44) {
                                return 0;
                            }
                            else if (RsDiff >= 390.44) {
                                if (Cs < 24.5) {
                                    return 0;
                                }
                                else if (Cs >= 24.5) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
            else if (diffAFD >= 12.58) {
                if (negBalance < 0.28) {
                    if (RsDiff < 415.95) {
                        if (SC < 17.29) {
                            if (SC < 16.9) {
                                return 0;
                            }
                            else if (SC >= 16.9) {
                                return 1;
                            }
                        }
                        else if (SC >= 17.29) {
                            return 0;
                        }
                    }
                    else if (RsDiff >= 415.95) {
                        if (diffTSC < 16.87) {
                            return 0;
                        }
                        else if (diffTSC >= 16.87) {
                            if (TSC < 41.11) {
                                if (diffAFD < 37.07) {
                                    if (gchDC < 148.4) {
                                        return 1;
                                    }
                                    else if (gchDC >= 148.4) {
                                        return 0;
                                    }
                                }
                                else if (diffAFD >= 37.07) {
                                    if (TSC < 37.28) {
                                        return 0;
                                    }
                                    else if (TSC >= 37.28) {
                                        return 1;
                                    }
                                }
                            }
                            else if (TSC >= 41.11) {
                                return 0;
                            }
                        }
                    }
                }
                else if (negBalance >= 0.28) {
                    if (MVDiff < 66.76) {
                        return 0;
                    }
                    else if (MVDiff >= 66.76) {
                        if (posBalance < 0.45) {
                            if (Cs < 12.73) {
                                return 1;
                            }
                            else if (Cs >= 12.73) {
                                if (diffRsCsdiff < 144.45) {
                                    if (diffAFD < 15.7) {
                                        if (diffTSC < 9.62) {
                                            return 0;
                                        }
                                        else if (diffTSC >= 9.62) {
                                            return 1;
                                        }
                                    }
                                    else if (diffAFD >= 15.7) {
                                        return 0;
                                    }
                                }
                                else if (diffRsCsdiff >= 144.45) {
                                    if (TSC < 11.33) {
                                        return 0;
                                    }
                                    else if (TSC >= 11.33) {
                                        if (RsCsDiff < 457.01) {
                                            if (RsDiff < 101.92) {
                                                return 0;
                                            }
                                            else if (RsDiff >= 101.92) {
                                                if (CsDiff < 79.54) {
                                                    if (Rs < 19.38) {
                                                        return 1;
                                                    }
                                                    else if (Rs >= 19.38) {
                                                        return 0;
                                                    }
                                                }
                                                else if (CsDiff >= 79.54) {
                                                    return 1;
                                                }
                                            }
                                        }
                                        else if (RsCsDiff >= 457.01) {
                                            if (Scindex < 4.5) {
                                                return 1;
                                            }
                                            else if (Scindex >= 4.5) {
                                                if (diffTSC < 20.22) {
                                                    return 0;
                                                }
                                                else if (diffTSC >= 20.22) {
                                                    return 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (posBalance >= 0.45) {
                            if (Rs < 19.85) {
                                if (diffRsCsdiff < 226.83) {
                                    if (refDCval < 165.84) {
                                        if (refDCval < 74.15) {
                                            if (Cs < 7.79) {
                                                return 0;
                                            }
                                            else if (Cs >= 7.79) {
                                                return 1;
                                            }
                                        }
                                        else if (refDCval >= 74.15) {
                                            return 0;
                                        }
                                    }
                                    else if (refDCval >= 165.84) {
                                        return 1;
                                    }
                                }
                                else if (diffRsCsdiff >= 226.83) {
                                    if (SC < 12.19) {
                                        return 0;
                                    }
                                    else if (SC >= 12.19) {
                                        return 1;
                                    }
                                }
                            }
                            else if (Rs >= 19.85) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 103.57) {
        if (diffAFD < 9.02) {
            if (diffRsCsdiff < 10.88) {
                if (CsDiff < 139.64) {
                    return 0;
                }
                else if (CsDiff >= 139.64) {
                    if (ssDCval < 120.84) {
                        if (RsDiff < 191.59) {
                            return 0;
                        }
                        else if (RsDiff >= 191.59) {
                            if (negBalance < 0.69) {
                                return 1;
                            }
                            else if (negBalance >= 0.69) {
                                if (Rs < 21.55) {
                                    return 0;
                                }
                                else if (Rs >= 21.55) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (ssDCval >= 120.84) {
                        return 0;
                    }
                }
            }
            else if (diffRsCsdiff >= 10.88) {
                if (Cs < 5.75) {
                    return 1;
                }
                else if (Cs >= 5.75) {
                    if (CsDiff < 222.13) {
                        if (MVDiff < 232.71) {
                            if (posBalance < 0.53) {
                                if (gchDC < 9.85) {
                                    return 1;
                                }
                                else if (gchDC >= 9.85) {
                                    return 0;
                                }
                            }
                            else if (posBalance >= 0.53) {
                                return 0;
                            }
                        }
                        else if (MVDiff >= 232.71) {
                            if (TSC < 5.07) {
                                if (TSC < 4.73) {
                                    return 0;
                                }
                                else if (TSC >= 4.73) {
                                    return 1;
                                }
                            }
                            else if (TSC >= 5.07) {
                                return 0;
                            }
                        }
                    }
                    else if (CsDiff >= 222.13) {
                        if (posBalance < 0.82) {
                            if (posBalance < 0.19) {
                                return 0;
                            }
                            else if (posBalance >= 0.19) {
                                return 1;
                            }
                        }
                        else if (posBalance >= 0.82) {
                            return 0;
                        }
                    }
                }
            }
        }
        else if (diffAFD >= 9.02) {
            if (negBalance < 0.08) {
                if (RsCsDiff < 348) {
                    if (refDCval < 31.5) {
                        return 1;
                    }
                    else if (refDCval >= 31.5) {
                        if (diffMVdiffVal < 155.4) {
                            if (MVDiff < 108.29) {
                                return 1;
                            }
                            else if (MVDiff >= 108.29) {
                                return 0;
                            }
                        }
                        else if (diffMVdiffVal >= 155.4) {
                            if (negBalance < 0.03) {
                                if (gchDC < 69.76) {
                                    return 0;
                                }
                                else if (gchDC >= 69.76) {
                                    return 1;
                                }
                            }
                            else if (negBalance >= 0.03) {
                                if (diffTSC < 12.58) {
                                    return 0;
                                }
                                else if (diffTSC >= 12.58) {
                                    if (SC < 25.2) {
                                        return 1;
                                    }
                                    else if (SC >= 25.2) {
                                        if (Cs < 19.7) {
                                            return 0;
                                        }
                                        else if (Cs >= 19.7) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (RsCsDiff >= 348) {
                    if (RsDiff < 333.84) {
                        return 1;
                    }
                    else if (RsDiff >= 333.84) {
                        if (negBalance < 0.01) {
                            if (Cs < 20.88) {
                                if (AFD < 29.41) {
                                    return 1;
                                }
                                else if (AFD >= 29.41) {
                                    return 0;
                                }
                            }
                            else if (Cs >= 20.88) {
                                return 0;
                            }
                        }
                        else if (negBalance >= 0.01) {
                            if (RsDiff < 415.02) {
                                if (Cs < 20.14) {
                                    return 0;
                                }
                                else if (Cs >= 20.14) {
                                    return 1;
                                }
                            }
                            else if (RsDiff >= 415.02) {
                                return 1;
                            }
                        }
                    }
                }
            }
            else if (negBalance >= 0.08) {
                if (diffRsCsdiff < 189.64) {
                    if (Rs < 18.59) {
                        if (diffTSC < 3.22) {
                            if (TSCindex < 6.5) {
                                return 1;
                            }
                            else if (TSCindex >= 6.5) {
                                if (refDCval < 55.45) {
                                    if (AFD < 17.04) {
                                        return 0;
                                    }
                                    else if (AFD >= 17.04) {
                                        return 1;
                                    }
                                }
                                else if (refDCval >= 55.45) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffTSC >= 3.22) {
                            if (AFD < 51.67) {
                                if (CsDiff < 51.59) {
                                    if (TSC < 7.12) {
                                        return 1;
                                    }
                                    else if (TSC >= 7.12) {
                                        if (diffAFD < 19.81) {
                                            if (TSCindex < 8.5) {
                                                if (gchDC < 15.45) {
                                                    return 1;
                                                }
                                                else if (gchDC >= 15.45) {
                                                    return 0;
                                                }
                                            }
                                            else if (TSCindex >= 8.5) {
                                                if (refDCval < 36.53) {
                                                    return 1;
                                                }
                                                else if (refDCval >= 36.53) {
                                                    return 0;
                                                }
                                            }
                                        }
                                        else if (diffAFD >= 19.81) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (CsDiff >= 51.59) {
                                    if (diffTSC < 7.13) {
                                        if (Cs < 12.47) {
                                            return 1;
                                        }
                                        else if (Cs >= 12.47) {
                                            if (negBalance < 0.45) {
                                                return 1;
                                            }
                                            else if (negBalance >= 0.45) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (diffTSC >= 7.13) {
                                        return 1;
                                    }
                                }
                            }
                            else if (AFD >= 51.67) {
                                if (diffTSC < 15.2) {
                                    return 0;
                                }
                                else if (diffTSC >= 15.2) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (Rs >= 18.59) {
                        if (diffMVdiffVal < 150.37) {
                            if (diffTSC < 9.95) {
                                return 0;
                            }
                            else if (diffTSC >= 9.95) {
                                if (CsDiff < 147.33) {
                                    return 1;
                                }
                                else if (CsDiff >= 147.33) {
                                    if (Rs < 24.25) {
                                        return 0;
                                    }
                                    else if (Rs >= 24.25) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (diffMVdiffVal >= 150.37) {
                            if (diffAFD < 27.15) {
                                if (gchDC < 5.56) {
                                    return 1;
                                }
                                else if (gchDC >= 5.56) {
                                    if (RsDiff < 567.71) {
                                        if (negBalance < 0.8) {
                                            return 0;
                                        }
                                        else if (negBalance >= 0.8) {
                                            if (CsDiff < 99.31) {
                                                return 0;
                                            }
                                            else if (CsDiff >= 99.31) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (RsDiff >= 567.71) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffAFD >= 27.15) {
                                return 1;
                            }
                        }
                    }
                }
                else if (diffRsCsdiff >= 189.64) {
                    if (diffTSC < 3.74) {
                        if (SC < 18.9) {
                            return 1;
                        }
                        else if (SC >= 18.9) {
                            return 0;
                        }
                    }
                    else if (diffTSC >= 3.74) {
                        if (refDCval < 210.58) {
                            if (diffMVdiffVal < 122.68) {
                                if (posBalance < 0.35) {
                                    if (Cs < 26.8) {
                                        if (diffRsCsdiff < 208.85) {
                                            if (CsDiff < 136.12) {
                                                return 0;
                                            }
                                            else if (CsDiff >= 136.12) {
                                                return 1;
                                            }
                                        }
                                        else if (diffRsCsdiff >= 208.85) {
                                            return 1;
                                        }
                                    }
                                    else if (Cs >= 26.8) {
                                        return 0;
                                    }
                                }
                                else if (posBalance >= 0.35) {
                                    return 0;
                                }
                            }
                            else if (diffMVdiffVal >= 122.68) {
                                if (Rs < 41.72) {
                                    if (ssDCval < 113.09) {
                                        return 1;
                                    }
                                    else if (ssDCval >= 113.09) {
                                        if (diffAFD < 34.44) {
                                            if (AFD < 40.29) {
                                                return 1;
                                            }
                                            else if (AFD >= 40.29) {
                                                return 0;
                                            }
                                        }
                                        else if (diffAFD >= 34.44) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (Rs >= 41.72) {
                                    return 0;
                                }
                            }
                        }
                        else if (refDCval >= 210.58) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetect9(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffMVdiffVal < 102.66) {
        if (TSC < 11.38) {
            if (diffMVdiffVal < 43.4) {
                if (diffRsCsdiff < 27.19) {
                    return 0;
                }
                else if (diffRsCsdiff >= 27.19) {
                    if (Scindex < 2.5) {
                        if (SC < 10.77) {
                            if (diffTSC < 0.32) {
                                return 0;
                            }
                            else if (diffTSC >= 0.32) {
                                if (Rs < 6.29) {
                                    if (Cs < 6.15) {
                                        return 1;
                                    }
                                    else if (Cs >= 6.15) {
                                        return 0;
                                    }
                                }
                                else if (Rs >= 6.29) {
                                    return 1;
                                }
                            }
                        }
                        else if (SC >= 10.77) {
                            if (refDCval < 17.68) {
                                if (diffAFD < 1.93) {
                                    return 0;
                                }
                                else if (diffAFD >= 1.93) {
                                    return 1;
                                }
                            }
                            else if (refDCval >= 17.68) {
                                if (RsCsDiff < 247.31) {
                                    return 0;
                                }
                                else if (RsCsDiff >= 247.31) {
                                    if (Rs < 10.66) {
                                        return 1;
                                    }
                                    else if (Rs >= 10.66) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (Scindex >= 2.5) {
                        if (Rs < 11.32) {
                            if (Rs < 11.29) {
                                return 0;
                            }
                            else if (Rs >= 11.29) {
                                return 1;
                            }
                        }
                        else if (Rs >= 11.32) {
                            return 0;
                        }
                    }
                }
            }
            else if (diffMVdiffVal >= 43.4) {
                if (refDCval < 21.85) {
                    if (Cs < 3.72) {
                        return 0;
                    }
                    else if (Cs >= 3.72) {
                        return 1;
                    }
                }
                else if (refDCval >= 21.85) {
                    if (diffRsCsdiff < 37.98) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= 37.98) {
                        if (SC < 15.43) {
                            if (negBalance < 0.29) {
                                return 0;
                            }
                            else if (negBalance >= 0.29) {
                                if (diffAFD < 11.51) {
                                    return 0;
                                }
                                else if (diffAFD >= 11.51) {
                                    if (refDCval < 56.69) {
                                        return 1;
                                    }
                                    else if (refDCval >= 56.69) {
                                        if (Cs < 7.79) {
                                            return 0;
                                        }
                                        else if (Cs >= 7.79) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                        else if (SC >= 15.43) {
                            if (diffMVdiffVal < 83.72) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 83.72) {
                                if (diffMVdiffVal < 84.4) {
                                    return 1;
                                }
                                else if (diffMVdiffVal >= 84.4) {
                                    if (diffAFD < 12) {
                                        if (diffRsCsdiff < 129.6) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 129.6) {
                                            if (Rs < 27.94) {
                                                return 1;
                                            }
                                            else if (Rs >= 27.94) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (diffAFD >= 12) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (TSC >= 11.38) {
            if (diffRsCsdiff < 146.43) {
                if (SC < 19.73) {
                    if (gchDC < 10.71) {
                        if (CsDiff < 119.32) {
                            if (diffTSC < 4.39) {
                                if (Rs < 16.69) {
                                    return 0;
                                }
                                else if (Rs >= 16.69) {
                                    return 1;
                                }
                            }
                            else if (diffTSC >= 4.39) {
                                if (TSC < 11.82) {
                                    return 1;
                                }
                                else if (TSC >= 11.82) {
                                    if (AFD < 26.22) {
                                        return 0;
                                    }
                                    else if (AFD >= 26.22) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (CsDiff >= 119.32) {
                            return 1;
                        }
                    }
                    else if (gchDC >= 10.71) {
                        if (AFD < 52.88) {
                            if (posBalance < 0.22) {
                                return 1;
                            }
                            else if (posBalance >= 0.22) {
                                return 0;
                            }
                        }
                        else if (AFD >= 52.88) {
                            if (Rs < 11.4) {
                                return 1;
                            }
                            else if (Rs >= 11.4) {
                                return 0;
                            }
                        }
                    }
                }
                else if (SC >= 19.73) {
                    if (MVDiff < 322.08) {
                        if (CsDiff < 241.25) {
                            return 0;
                        }
                        else if (CsDiff >= 241.25) {
                            if (diffMVdiffVal < 97.58) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 97.58) {
                                return 1;
                            }
                        }
                    }
                    else if (MVDiff >= 322.08) {
                        if (CsDiff < 355.18) {
                            if (RsDiff < 63.6) {
                                return 1;
                            }
                            else if (RsDiff >= 63.6) {
                                if (gchDC < 16.72) {
                                    if (ssDCval < 67.67) {
                                        return 1;
                                    }
                                    else if (ssDCval >= 67.67) {
                                        if (ssDCval < 134.53) {
                                            return 0;
                                        }
                                        else if (ssDCval >= 134.53) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (gchDC >= 16.72) {
                                    return 0;
                                }
                            }
                        }
                        else if (CsDiff >= 355.18) {
                            if (MVDiff < 390.61) {
                                return 1;
                            }
                            else if (MVDiff >= 390.61) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 146.43) {
                if (negBalance < 0.27) {
                    if (MVDiff < 2.41) {
                        return 1;
                    }
                    else if (MVDiff >= 2.41) {
                        if (negBalance < 0.14) {
                            return 0;
                        }
                        else if (negBalance >= 0.14) {
                            if (TSC < 24.8) {
                                if (RsCsDiff < 263.17) {
                                    if (TSC < 13.59) {
                                        return 0;
                                    }
                                    else if (TSC >= 13.59) {
                                        return 1;
                                    }
                                }
                                else if (RsCsDiff >= 263.17) {
                                    return 0;
                                }
                            }
                            else if (TSC >= 24.8) {
                                if (TSC < 41.11) {
                                    if (posBalance < 0.39) {
                                        return 0;
                                    }
                                    else if (posBalance >= 0.39) {
                                        if (diffAFD < 34.17) {
                                            return 0;
                                        }
                                        else if (diffAFD >= 34.17) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (TSC >= 41.11) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
                else if (negBalance >= 0.27) {
                    if (SC < 28.86) {
                        if (RsDiff < 278.38) {
                            if (refDCval < 92.72) {
                                return 1;
                            }
                            else if (refDCval >= 92.72) {
                                if (SC < 24.64) {
                                    return 0;
                                }
                                else if (SC >= 24.64) {
                                    return 1;
                                }
                            }
                        }
                        else if (RsDiff >= 278.38) {
                            if (diffMVdiffVal < 45.73) {
                                if (gchDC < 7.99) {
                                    return 1;
                                }
                                else if (gchDC >= 7.99) {
                                    return 0;
                                }
                            }
                            else if (diffMVdiffVal >= 45.73) {
                                if (posBalance < 0.97) {
                                    return 1;
                                }
                                else if (posBalance >= 0.97) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (SC >= 28.86) {
                        if (diffTSC < 8.03) {
                            return 0;
                        }
                        else if (diffTSC >= 8.03) {
                            if (RsCsDiff < 506.67) {
                                if (RsCsDiff < 198.43) {
                                    return 1;
                                }
                                else if (RsCsDiff >= 198.43) {
                                    if (AFD < 58.85) {
                                        if (ssDCval < 75.65) {
                                            return 1;
                                        }
                                        else if (ssDCval >= 75.65) {
                                            return 0;
                                        }
                                    }
                                    else if (AFD >= 58.85) {
                                        return 1;
                                    }
                                }
                            }
                            else if (RsCsDiff >= 506.67) {
                                if (CsDiff < 737.77) {
                                    if (RsDiff < 404) {
                                        if (refDCval < 101.39) {
                                            return 1;
                                        }
                                        else if (refDCval >= 101.39) {
                                            return 0;
                                        }
                                    }
                                    else if (RsDiff >= 404) {
                                        return 1;
                                    }
                                }
                                else if (CsDiff >= 737.77) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 102.66) {
        if (diffAFD < 9.46) {
            if (diffTSC < 0.7) {
                if (ssDCval < 234.84) {
                    if (diffRsCsdiff < 85.29) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= 85.29) {
                        if (Rs < 11.7) {
                            return 1;
                        }
                        else if (Rs >= 11.7) {
                            return 0;
                        }
                    }
                }
                else if (ssDCval >= 234.84) {
                    return 1;
                }
            }
            else if (diffTSC >= 0.7) {
                if (RsCsDiff < 538.96) {
                    if (diffRsCsdiff < 10.51) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= 10.51) {
                        if (Rs < 11.98) {
                            if (gchDC < 7.39) {
                                return 1;
                            }
                            else if (gchDC >= 7.39) {
                                if (TSCindex < 8.5) {
                                    return 0;
                                }
                                else if (TSCindex >= 8.5) {
                                    return 1;
                                }
                            }
                        }
                        else if (Rs >= 11.98) {
                            if (diffAFD < -1.34) {
                                return 1;
                            }
                            else if (diffAFD >= -1.34) {
                                return 0;
                            }
                        }
                    }
                }
                else if (RsCsDiff >= 538.96) {
                    return 1;
                }
            }
        }
        else if (diffAFD >= 9.46) {
            if (negBalance < 0.1) {
                if (gchDC < 70.87) {
                    if (Cs < 7.3) {
                        return 1;
                    }
                    else if (Cs >= 7.3) {
                        if (CsDiff < 292.28) {
                            if (diffMVdiffVal < 170.44) {
                                if (MVDiff < 108.29) {
                                    return 1;
                                }
                                else if (MVDiff >= 108.29) {
                                    return 0;
                                }
                            }
                            else if (diffMVdiffVal >= 170.44) {
                                if (diffTSC < 12.58) {
                                    return 0;
                                }
                                else if (diffTSC >= 12.58) {
                                    if (SC < 19.27) {
                                        return 1;
                                    }
                                    else if (SC >= 19.27) {
                                        if (negBalance < 0.03) {
                                            return 0;
                                        }
                                        else if (negBalance >= 0.03) {
                                            if (AFD < 39.93) {
                                                return 0;
                                            }
                                            else if (AFD >= 39.93) {
                                                if (AFD < 58.79) {
                                                    return 1;
                                                }
                                                else if (AFD >= 58.79) {
                                                    return 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (CsDiff >= 292.28) {
                            return 1;
                        }
                    }
                }
                else if (gchDC >= 70.87) {
                    if (negBalance < 0.01) {
                        if (SC < 32.28) {
                            if (Cs < 19.85) {
                                return 1;
                            }
                            else if (Cs >= 19.85) {
                                return 0;
                            }
                        }
                        else if (SC >= 32.28) {
                            return 0;
                        }
                    }
                    else if (negBalance >= 0.01) {
                        if (diffMVdiffVal < 146.69) {
                            if (SC < 26.09) {
                                return 1;
                            }
                            else if (SC >= 26.09) {
                                return 0;
                            }
                        }
                        else if (diffMVdiffVal >= 146.69) {
                            return 1;
                        }
                    }
                }
            }
            else if (negBalance >= 0.1) {
                if (diffTSC < 11.05) {
                    if (Rs < 18.49) {
                        if (diffRsCsdiff < 45.97) {
                            if (gchDC < 9.73) {
                                if (Rs < 10.36) {
                                    return 1;
                                }
                                else if (Rs >= 10.36) {
                                    return 0;
                                }
                            }
                            else if (gchDC >= 9.73) {
                                if (TSC < 6.44) {
                                    return 1;
                                }
                                else if (TSC >= 6.44) {
                                    if (CsDiff < 70.66) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 70.66) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 45.97) {
                            if (TSC < 16.47) {
                                if (diffRsCsdiff < 141.42) {
                                    if (SC < 15.94) {
                                        if (diffRsCsdiff < 80.46) {
                                            if (diffAFD < 12.59) {
                                                if (AFD < 18.41) {
                                                    return 1;
                                                }
                                                else if (AFD >= 18.41) {
                                                    return 0;
                                                }
                                            }
                                            else if (diffAFD >= 12.59) {
                                                return 1;
                                            }
                                        }
                                        else if (diffRsCsdiff >= 80.46) {
                                            return 1;
                                        }
                                    }
                                    else if (SC >= 15.94) {
                                        if (diffTSC < 6.75) {
                                            return 0;
                                        }
                                        else if (diffTSC >= 6.75) {
                                            if (posBalance < 0.64) {
                                                return 1;
                                            }
                                            else if (posBalance >= 0.64) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                                else if (diffRsCsdiff >= 141.42) {
                                    return 1;
                                }
                            }
                            else if (TSC >= 16.47) {
                                if (MVDiff < 289.42) {
                                    return 1;
                                }
                                else if (MVDiff >= 289.42) {
                                    if (SC < 15.97) {
                                        return 1;
                                    }
                                    else if (SC >= 15.97) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (Rs >= 18.49) {
                        if (MVDiff < 196.8) {
                            if (gchDC < 5.64) {
                                return 1;
                            }
                            else if (gchDC >= 5.64) {
                                if (AFD < 39.48) {
                                    if (MVDiff < 188.26) {
                                        if (TSC < 14.58) {
                                            return 0;
                                        }
                                        else if (TSC >= 14.58) {
                                            return 1;
                                        }
                                    }
                                    else if (MVDiff >= 188.26) {
                                        return 1;
                                    }
                                }
                                else if (AFD >= 39.48) {
                                    return 1;
                                }
                            }
                        }
                        else if (MVDiff >= 196.8) {
                            if (Rs < 19.38) {
                                if (RsCsDiff < 271.77) {
                                    return 0;
                                }
                                else if (RsCsDiff >= 271.77) {
                                    if (CsDiff < 276.83) {
                                        return 1;
                                    }
                                    else if (CsDiff >= 276.83) {
                                        return 0;
                                    }
                                }
                            }
                            else if (Rs >= 19.38) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffTSC >= 11.05) {
                    if (MVDiff < 385.65) {
                        if (posBalance < 0.91) {
                            if (Rs < 26.34) {
                                return 1;
                            }
                            else if (Rs >= 26.34) {
                                if (Rs < 26.38) {
                                    return 0;
                                }
                                else if (Rs >= 26.38) {
                                    return 1;
                                }
                            }
                        }
                        else if (posBalance >= 0.91) {
                            if (CsDiff < 137.13) {
                                return 0;
                            }
                            else if (CsDiff >= 137.13) {
                                return 1;
                            }
                        }
                    }
                    else if (MVDiff >= 385.65) {
                        if (refDCval < 163.23) {
                            if (CsDiff < 60.65) {
                                return 0;
                            }
                            else if (CsDiff >= 60.65) {
                                return 1;
                            }
                        }
                        else if (refDCval >= 163.23) {
                            if (Cs < 17.27) {
                                return 1;
                            }
                            else if (Cs >= 17.27) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetectA(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    Scindex;
    TSCindex;

    if (diffTSC < 5.49) {
        if (diffMVdiffVal < 69.42) {
            if (ssDCval < 20.53) {
                if (diffTSC < 0.42) {
                    return 0;
                }
                else if (diffTSC >= 0.42) {
                    if (Rs < 6.72) {
                        if (gchDC < 0.76) {
                            return 1;
                        }
                        else if (gchDC >= 0.76) {
                            return 0;
                        }
                    }
                    else if (Rs >= 6.72) {
                        return 1;
                    }
                }
            }
            else if (ssDCval >= 20.53) {
                if (MVDiff < 271.76) {
                    if (MVDiff < 157.56) {
                        return 0;
                    }
                    else if (MVDiff >= 157.56) {
                        if (diffRsCsdiff < 167.38) {
                            if (diffAFD < 9) {
                                return 0;
                            }
                            else if (diffAFD >= 9) {
                                if (Rs < 6.52) {
                                    return 1;
                                }
                                else if (Rs >= 6.52) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 167.38) {
                            if (RsCsDiff < 261.82) {
                                if (AFD < 24.68) {
                                    return 0;
                                }
                                else if (AFD >= 24.68) {
                                    return 1;
                                }
                            }
                            else if (RsCsDiff >= 261.82) {
                                return 0;
                            }
                        }
                    }
                }
                else if (MVDiff >= 271.76) {
                    if (RsDiff < 390.43) {
                        if (RsCsDiff < 123.79) {
                            if (TSC < 16.24) {
                                return 0;
                            }
                            else if (TSC >= 16.24) {
                                return 1;
                            }
                        }
                        else if (RsCsDiff >= 123.79) {
                            if (refDCval < 43.63) {
                                if (AFD < 18.62) {
                                    return 0;
                                }
                                else if (AFD >= 18.62) {
                                    return 1;
                                }
                            }
                            else if (refDCval >= 43.63) {
                                if (diffRsCsdiff < -25.59) {
                                    return 0;
                                }
                                else if (diffRsCsdiff >= -25.59) {
                                    if (Rs < 17.02) {
                                        if (diffRsCsdiff < 36.48) {
                                            if (posBalance < 0.56) {
                                                if (RsCsDiff < 176.28) {
                                                    if (Rs < 10.98) {
                                                        return 1;
                                                    }
                                                    else if (Rs >= 10.98) {
                                                        return 0;
                                                    }
                                                }
                                                else if (RsCsDiff >= 176.28) {
                                                    return 1;
                                                }
                                            }
                                            else if (posBalance >= 0.56) {
                                                return 0;
                                            }
                                        }
                                        else if (diffRsCsdiff >= 36.48) {
                                            return 0;
                                        }
                                    }
                                    else if (Rs >= 17.02) {
                                        if (TSC < 17.83) {
                                            return 0;
                                        }
                                        else if (TSC >= 17.83) {
                                            if (Rs < 32.8) {
                                                return 0;
                                            }
                                            else if (Rs >= 32.8) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (RsDiff >= 390.43) {
                        if (CsDiff < 224.37) {
                            return 0;
                        }
                        else if (CsDiff >= 224.37) {
                            if (RsCsDiff < 551.56) {
                                return 1;
                            }
                            else if (RsCsDiff >= 551.56) {
                                if (RsDiff < 649.82) {
                                    return 0;
                                }
                                else if (RsDiff >= 649.82) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 69.42) {
            if (Rs < 11.1) {
                if (diffRsCsdiff < 20.16) {
                    return 0;
                }
                else if (diffRsCsdiff >= 20.16) {
                    if (CsDiff < 160.02) {
                        if (TSC < 6.97) {
                            if (SC < 14.11) {
                                if (diffMVdiffVal < 132.58) {
                                    if (posBalance < 0.67) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.67) {
                                        return 0;
                                    }
                                }
                                else if (diffMVdiffVal >= 132.58) {
                                    return 1;
                                }
                            }
                            else if (SC >= 14.11) {
                                return 0;
                            }
                        }
                        else if (TSC >= 6.97) {
                            if (negBalance < 0.46) {
                                if (refDCval < 39.67) {
                                    return 0;
                                }
                                else if (refDCval >= 39.67) {
                                    if (RsCsDiff < 83.68) {
                                        return 0;
                                    }
                                    else if (RsCsDiff >= 83.68) {
                                        return 1;
                                    }
                                }
                            }
                            else if (negBalance >= 0.46) {
                                if (RsDiff < 223.77) {
                                    if (SC < 9.35) {
                                        return 1;
                                    }
                                    else if (SC >= 9.35) {
                                        return 0;
                                    }
                                }
                                else if (RsDiff >= 223.77) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (CsDiff >= 160.02) {
                        return 1;
                    }
                }
            }
            else if (Rs >= 11.1) {
                if (RsCsDiff < 216.85) {
                    if (ssDCval < 233.94) {
                        if (diffRsCsdiff < 133.15) {
                            return 0;
                        }
                        else if (diffRsCsdiff >= 133.15) {
                            if (SC < 20.13) {
                                return 1;
                            }
                            else if (SC >= 20.13) {
                                return 0;
                            }
                        }
                    }
                    else if (ssDCval >= 233.94) {
                        return 1;
                    }
                }
                else if (RsCsDiff >= 216.85) {
                    if (CsDiff < 140.41) {
                        return 0;
                    }
                    else if (CsDiff >= 140.41) {
                        if (diffRsCsdiff < 251.34) {
                            if (AFD < 15.37) {
                                return 1;
                            }
                            else if (AFD >= 15.37) {
                                if (Rs < 15.41) {
                                    if (Rs < 13.04) {
                                        return 0;
                                    }
                                    else if (Rs >= 13.04) {
                                        if (TSC < 8.41) {
                                            return 0;
                                        }
                                        else if (TSC >= 8.41) {
                                            if (AFD < 62.91) {
                                                return 1;
                                            }
                                            else if (AFD >= 62.91) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                                else if (Rs >= 15.41) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffRsCsdiff >= 251.34) {
                            if (Cs < 17.15) {
                                return 1;
                            }
                            else if (Cs >= 17.15) {
                                if (Rs < 24.32) {
                                    return 0;
                                }
                                else if (Rs >= 24.32) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffTSC >= 5.49) {
        if (AFD < 28.86) {
            if (SC < 24.54) {
                if (posBalance < 0.42) {
                    if (MVDiff < 112.79) {
                        if (diffTSC < 8.37) {
                            return 0;
                        }
                        else if (diffTSC >= 8.37) {
                            if (SC < 21.55) {
                                return 1;
                            }
                            else if (SC >= 21.55) {
                                return 0;
                            }
                        }
                    }
                    else if (MVDiff >= 112.79) {
                        if (diffAFD < 10.35) {
                            if (Rs < 10.64) {
                                if (Rs < 7.71) {
                                    return 0;
                                }
                                else if (Rs >= 7.71) {
                                    return 1;
                                }
                            }
                            else if (Rs >= 10.64) {
                                return 0;
                            }
                        }
                        else if (diffAFD >= 10.35) {
                            if (negBalance < 0.2) {
                                if (Rs < 10.62) {
                                    return 1;
                                }
                                else if (Rs >= 10.62) {
                                    return 0;
                                }
                            }
                            else if (negBalance >= 0.2) {
                                if (ssDCval < 99.82) {
                                    if (diffMVdiffVal < 57.53) {
                                        if (AFD < 23.18) {
                                            return 1;
                                        }
                                        else if (AFD >= 23.18) {
                                            return 0;
                                        }
                                    }
                                    else if (diffMVdiffVal >= 57.53) {
                                        return 1;
                                    }
                                }
                                else if (ssDCval >= 99.82) {
                                    if (Rs < 19.26) {
                                        return 1;
                                    }
                                    else if (Rs >= 19.26) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (posBalance >= 0.42) {
                    if (diffMVdiffVal < 142.87) {
                        return 0;
                    }
                    else if (diffMVdiffVal >= 142.87) {
                        if (MVDiff < 295.38) {
                            if (negBalance < 0.07) {
                                if (refDCval < 39.55) {
                                    return 1;
                                }
                                else if (refDCval >= 39.55) {
                                    return 0;
                                }
                            }
                            else if (negBalance >= 0.07) {
                                return 1;
                            }
                        }
                        else if (MVDiff >= 295.38) {
                            if (AFD < 23.31) {
                                return 0;
                            }
                            else if (AFD >= 23.31) {
                                if (Cs < 16.14) {
                                    return 1;
                                }
                                else if (Cs >= 16.14) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            else if (SC >= 24.54) {
                if (diffMVdiffVal < 78.55) {
                    return 0;
                }
                else if (diffMVdiffVal >= 78.55) {
                    if (gchDC < 11.75) {
                        if (negBalance < 0.39) {
                            if (gchDC < 7.92) {
                                return 0;
                            }
                            else if (gchDC >= 7.92) {
                                return 1;
                            }
                        }
                        else if (negBalance >= 0.39) {
                            if (diffTSC < 6.49) {
                                return 0;
                            }
                            else if (diffTSC >= 6.49) {
                                return 1;
                            }
                        }
                    }
                    else if (gchDC >= 11.75) {
                        if (diffTSC < 15.74) {
                            return 0;
                        }
                        else if (diffTSC >= 15.74) {
                            return 1;
                        }
                    }
                }
            }
        }
        else if (AFD >= 28.86) {
            if (diffMVdiffVal < 122.8) {
                if (negBalance < 0.25) {
                    if (MVDiff < 199.92) {
                        return 0;
                    }
                    else if (MVDiff >= 199.92) {
                        if (diffRsCsdiff < 143.31) {
                            return 0;
                        }
                        else if (diffRsCsdiff >= 143.31) {
                            if (AFD < 29.34) {
                                return 1;
                            }
                            else if (AFD >= 29.34) {
                                if (diffTSC < 27.65) {
                                    if (Rs < 18.47) {
                                        if (diffAFD < 29.69) {
                                            return 1;
                                        }
                                        else if (diffAFD >= 29.69) {
                                            if (Cs < 13.31) {
                                                return 1;
                                            }
                                            else if (Cs >= 13.31) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (Rs >= 18.47) {
                                        if (diffRsCsdiff < 454.35) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 454.35) {
                                            if (Rs < 23.39) {
                                                return 0;
                                            }
                                            else if (Rs >= 23.39) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (diffTSC >= 27.65) {
                                    if (MVDiff < 331.01) {
                                        return 1;
                                    }
                                    else if (MVDiff >= 331.01) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (negBalance >= 0.25) {
                    if (refDCval < 86) {
                        if (RsDiff < 142.75) {
                            return 0;
                        }
                        else if (RsDiff >= 142.75) {
                            if (diffTSC < 9.53) {
                                if (diffMVdiffVal < 8.85) {
                                    return 1;
                                }
                                else if (diffMVdiffVal >= 8.85) {
                                    if (CsDiff < 335.44) {
                                        if (diffMVdiffVal < 109.63) {
                                            return 0;
                                        }
                                        else if (diffMVdiffVal >= 109.63) {
                                            return 1;
                                        }
                                    }
                                    else if (CsDiff >= 335.44) {
                                        return 1;
                                    }
                                }
                            }
                            else if (diffTSC >= 9.53) {
                                return 1;
                            }
                        }
                    }
                    else if (refDCval >= 86) {
                        if (RsCsDiff < 493.84) {
                            if (diffRsCsdiff < 161.99) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 161.99) {
                                if (TSC < 14.68) {
                                    return 0;
                                }
                                else if (TSC >= 14.68) {
                                    if (diffAFD < 22.55) {
                                        if (diffRsCsdiff < 283.1) {
                                            return 0;
                                        }
                                        else if (diffRsCsdiff >= 283.1) {
                                            return 1;
                                        }
                                    }
                                    else if (diffAFD >= 22.55) {
                                        if (SC < 35.16) {
                                            return 1;
                                        }
                                        else if (SC >= 35.16) {
                                            if (diffRsCsdiff < 229.15) {
                                                return 1;
                                            }
                                            else if (diffRsCsdiff >= 229.15) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (RsCsDiff >= 493.84) {
                            if (AFD < 51.45) {
                                if (Rs < 20.13) {
                                    return 1;
                                }
                                else if (Rs >= 20.13) {
                                    if (MVDiff < 220.98) {
                                        if (RsCsDiff < 498.52) {
                                            return 1;
                                        }
                                        else if (RsCsDiff >= 498.52) {
                                            return 0;
                                        }
                                    }
                                    else if (MVDiff >= 220.98) {
                                        if (RsDiff < 477.59) {
                                            return 0;
                                        }
                                        else if (RsDiff >= 477.59) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                            else if (AFD >= 51.45) {
                                return 1;
                            }
                        }
                    }
                }
            }
            else if (diffMVdiffVal >= 122.8) {
                if (negBalance < 0.04) {
                    if (diffAFD < 42.26) {
                        if (posBalance < 0.9) {
                            return 0;
                        }
                        else if (posBalance >= 0.9) {
                            if (diffMVdiffVal < 173.32) {
                                if (refDCval < 62.75) {
                                    return 0;
                                }
                                else if (refDCval >= 62.75) {
                                    return 1;
                                }
                            }
                            else if (diffMVdiffVal >= 173.32) {
                                if (Rs < 22.55) {
                                    return 1;
                                }
                                else if (Rs >= 22.55) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 42.26) {
                        if (SC < 42.96) {
                            return 1;
                        }
                        else if (SC >= 42.96) {
                            return 0;
                        }
                    }
                }
                else if (negBalance >= 0.04) {
                    if (diffAFD < 20.57) {
                        if (diffRsCsdiff < 181.76) {
                            if (RsCsDiff < 356.42) {
                                if (ssDCval < 91.39) {
                                    if (diffRsCsdiff < 76.53) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 76.53) {
                                        return 1;
                                    }
                                }
                                else if (ssDCval >= 91.39) {
                                    return 0;
                                }
                            }
                            else if (RsCsDiff >= 356.42) {
                                return 1;
                            }
                        }
                        else if (diffRsCsdiff >= 181.76) {
                            return 1;
                        }
                    }
                    else if (diffAFD >= 20.57) {
                        if (diffRsCsdiff < 50.91) {
                            if (Rs < 19.22) {
                                return 0;
                            }
                            else if (Rs >= 19.22) {
                                return 1;
                            }
                        }
                        else if (diffRsCsdiff >= 50.91) {
                            if (TSC < 10.91) {
                                if (Rs < 18.52) {
                                    return 1;
                                }
                                else if (Rs >= 18.52) {
                                    if (RsDiff < 166.38) {
                                        return 1;
                                    }
                                    else if (RsDiff >= 166.38) {
                                        return 0;
                                    }
                                }
                            }
                            else if (TSC >= 10.91) {
                                if (diffTSC < 47.33) {
                                    if (CsDiff < 28.48) {
                                        return 0;
                                    }
                                    else if (CsDiff >= 28.48) {
                                        if (MVDiff < 446) {
                                            return 1;
                                        }
                                        else if (MVDiff >= 446) {
                                            if (MVDiff < 470.25) {
                                                return 0;
                                            }
                                            else if (MVDiff >= 470.25) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (diffTSC >= 47.33) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetectB(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    TSCindex;

    if (diffAFD < 13.19) {
        if (MVDiff < 120.46) {
            if (ssDCval < 21.08) {
                if (diffTSC < 0.47) {
                    return 0;
                }
                else if (diffTSC >= 0.47) {
                    if (Rs < 4.87) {
                        return 0;
                    }
                    else if (Rs >= 4.87) {
                        return 1;
                    }
                }
            }
            else if (ssDCval >= 21.08) {
                if (diffTSC < 5.82) {
                    return 0;
                }
                else if (diffTSC >= 5.82) {
                    if (diffTSC < 5.83) {
                        return 1;
                    }
                    else if (diffTSC >= 5.83) {
                        return 0;
                    }
                }
            }
        }
        else if (MVDiff >= 120.46) {
            if (diffRsCsdiff < 23.87) {
                if (RsCsDiff < 444.81) {
                    if (RsCsDiff < 239.19) {
                        if (MVDiff < 306.68) {
                            if (diffAFD < 9.04) {
                                return 0;
                            }
                            else if (diffAFD >= 9.04) {
                                if (SC < 8.97) {
                                    return 1;
                                }
                                else if (SC >= 8.97) {
                                    return 0;
                                }
                            }
                        }
                        else if (MVDiff >= 306.68) {
                            if (ssDCval < 233.99) {
                                if (TSC < 10.96) {
                                    return 0;
                                }
                                else if (TSC >= 10.96) {
                                    if (refDCval < 69.96) {
                                        if (refDCval < 63.21) {
                                            if (Rs < 8.21) {
                                                return 1;
                                            }
                                            else if (Rs >= 8.21) {
                                                return 0;
                                            }
                                        }
                                        else if (refDCval >= 63.21) {
                                            return 1;
                                        }
                                    }
                                    else if (refDCval >= 69.96) {
                                        return 0;
                                    }
                                }
                            }
                            else if (ssDCval >= 233.99) {
                                return 1;
                            }
                        }
                    }
                    else if (RsCsDiff >= 239.19) {
                        if (diffTSC < -3.35) {
                            return 0;
                        }
                        else if (diffTSC >= -3.35) {
                            if (RsCsDiff < 240.03) {
                                return 1;
                            }
                            else if (RsCsDiff >= 240.03) {
                                if (diffMVdiffVal < 101.89) {
                                    if (Rs < 33.6) {
                                        return 0;
                                    }
                                    else if (Rs >= 33.6) {
                                        if (Cs < 23.21) {
                                            return 1;
                                        }
                                        else if (Cs >= 23.21) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (diffMVdiffVal >= 101.89) {
                                    if (negBalance < 0.56) {
                                        if (RsDiff < 188.84) {
                                            return 0;
                                        }
                                        else if (RsDiff >= 188.84) {
                                            return 1;
                                        }
                                    }
                                    else if (negBalance >= 0.56) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (RsCsDiff >= 444.81) {
                    if (AFD < 50.45) {
                        return 0;
                    }
                    else if (AFD >= 50.45) {
                        if (refDCval < 109.57) {
                            return 1;
                        }
                        else if (refDCval >= 109.57) {
                            if (Rs < 27.94) {
                                return 1;
                            }
                            else if (Rs >= 27.94) {
                                return 0;
                            }
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 23.87) {
                if (diffMVdiffVal < 102.16) {
                    if (diffRsCsdiff < 317.75) {
                        if (Rs < 11.98) {
                            if (ssDCval < 45.11) {
                                if (MVDiff < 243.31) {
                                    return 0;
                                }
                                else if (MVDiff >= 243.31) {
                                    if (diffTSC < -0.36) {
                                        if (SC < 10.52) {
                                            return 0;
                                        }
                                        else if (SC >= 10.52) {
                                            if (Cs < 8.04) {
                                                return 1;
                                            }
                                            else if (Cs >= 8.04) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (diffTSC >= -0.36) {
                                        return 1;
                                    }
                                }
                            }
                            else if (ssDCval >= 45.11) {
                                return 0;
                            }
                        }
                        else if (Rs >= 11.98) {
                            if (RsCsDiff < 38.59) {
                                return 1;
                            }
                            else if (RsCsDiff >= 38.59) {
                                if (MVDiff < 120.81) {
                                    return 1;
                                }
                                else if (MVDiff >= 120.81) {
                                    if (posBalance < 0.02) {
                                        if (TSC < 17.1) {
                                            return 0;
                                        }
                                        else if (TSC >= 17.1) {
                                            if (Cs < 20.47) {
                                                return 1;
                                            }
                                            else if (Cs >= 20.47) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (posBalance >= 0.02) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 317.75) {
                        if (diffTSC < 3.24) {
                            return 0;
                        }
                        else if (diffTSC >= 3.24) {
                            return 1;
                        }
                    }
                }
                else if (diffMVdiffVal >= 102.16) {
                    if (Rs < 10.18) {
                        if (negBalance < 0.05) {
                            return 0;
                        }
                        else if (negBalance >= 0.05) {
                            if (posBalance < 0.4) {
                                if (MVDiff < 222.29) {
                                    return 1;
                                }
                                else if (MVDiff >= 222.29) {
                                    if (SC < 9.06) {
                                        return 0;
                                    }
                                    else if (SC >= 9.06) {
                                        return 1;
                                    }
                                }
                            }
                            else if (posBalance >= 0.4) {
                                if (diffTSC < 3.93) {
                                    if (Rs < 8.43) {
                                        return 0;
                                    }
                                    else if (Rs >= 8.43) {
                                        return 1;
                                    }
                                }
                                else if (diffTSC >= 3.93) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (Rs >= 10.18) {
                        if (CsDiff < 88.15) {
                            if (RsCsDiff < 42.51) {
                                return 1;
                            }
                            else if (RsCsDiff >= 42.51) {
                                return 0;
                            }
                        }
                        else if (CsDiff >= 88.15) {
                            if (diffRsCsdiff < 315.25) {
                                if (refDCval < 58.32) {
                                    if (MVDiff < 339.01) {
                                        if (Rs < 20.59) {
                                            return 1;
                                        }
                                        else if (Rs >= 20.59) {
                                            return 0;
                                        }
                                    }
                                    else if (MVDiff >= 339.01) {
                                        return 0;
                                    }
                                }
                                else if (refDCval >= 58.32) {
                                    if (TSC < 4.18) {
                                        return 1;
                                    }
                                    else if (TSC >= 4.18) {
                                        if (Rs < 13.15) {
                                            if (Rs < 12.3) {
                                                return 0;
                                            }
                                            else if (Rs >= 12.3) {
                                                return 1;
                                            }
                                        }
                                        else if (Rs >= 13.15) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (diffRsCsdiff >= 315.25) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffAFD >= 13.19) {
        if (diffMVdiffVal < 107.46) {
            if (MVDiff < 83.21) {
                if (refDCval < 64.06) {
                    if (diffTSC < 10.18) {
                        return 0;
                    }
                    else if (diffTSC >= 10.18) {
                        if (AFD < 23.37) {
                            if (Rs < 13.23) {
                                return 0;
                            }
                            else if (Rs >= 13.23) {
                                return 1;
                            }
                        }
                        else if (AFD >= 23.37) {
                            return 0;
                        }
                    }
                }
                else if (refDCval >= 64.06) {
                    return 0;
                }
            }
            else if (MVDiff >= 83.21) {
                if (diffRsCsdiff < 139.49) {
                    if (ssDCval < 51.79) {
                        if (posBalance < 0.34) {
                            if (gchDC < 1.77) {
                                return 0;
                            }
                            else if (gchDC >= 1.77) {
                                return 1;
                            }
                        }
                        else if (posBalance >= 0.34) {
                            return 0;
                        }
                    }
                    else if (ssDCval >= 51.79) {
                        if (MVDiff < 84.67) {
                            if (CsDiff < 45.26) {
                                return 1;
                            }
                            else if (CsDiff >= 45.26) {
                                return 0;
                            }
                        }
                        else if (MVDiff >= 84.67) {
                            return 0;
                        }
                    }
                }
                else if (diffRsCsdiff >= 139.49) {
                    if (posBalance < 0.42) {
                        if (ssDCval < 71) {
                            if (MVDiff < 197.45) {
                                if (refDCval < 47.37) {
                                    return 0;
                                }
                                else if (refDCval >= 47.37) {
                                    if (diffMVdiffVal < 64.44) {
                                        if (SC < 20.36) {
                                            if (CsDiff < 151.53) {
                                                return 1;
                                            }
                                            else if (CsDiff >= 151.53) {
                                                return 0;
                                            }
                                        }
                                        else if (SC >= 20.36) {
                                            return 0;
                                        }
                                    }
                                    else if (diffMVdiffVal >= 64.44) {
                                        return 1;
                                    }
                                }
                            }
                            else if (MVDiff >= 197.45) {
                                return 1;
                            }
                        }
                        else if (ssDCval >= 71) {
                            if (diffTSC < 10.15) {
                                if (ssDCval < 82.82) {
                                    if (posBalance < 0.15) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.15) {
                                        if (SC < 23.38) {
                                            return 1;
                                        }
                                        else if (SC >= 23.38) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (ssDCval >= 82.82) {
                                    return 0;
                                }
                            }
                            else if (diffTSC >= 10.15) {
                                if (CsDiff < 376.41) {
                                    if (refDCval < 90.37) {
                                        if (AFD < 34.92) {
                                            return 0;
                                        }
                                        else if (AFD >= 34.92) {
                                            if (AFD < 46.97) {
                                                return 1;
                                            }
                                            else if (AFD >= 46.97) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (refDCval >= 90.37) {
                                        return 1;
                                    }
                                }
                                else if (CsDiff >= 376.41) {
                                    if (ssDCval < 160.63) {
                                        return 0;
                                    }
                                    else if (ssDCval >= 160.63) {
                                        if (Rs < 34.37) {
                                            return 0;
                                        }
                                        else if (Rs >= 34.37) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (posBalance >= 0.42) {
                        if (negBalance < 0.13) {
                            return 0;
                        }
                        else if (negBalance >= 0.13) {
                            if (Cs < 9.16) {
                                return 1;
                            }
                            else if (Cs >= 9.16) {
                                if (MVDiff < 311.33) {
                                    if (diffMVdiffVal < -5.96) {
                                        if (Cs < 14.83) {
                                            return 1;
                                        }
                                        else if (Cs >= 14.83) {
                                            return 0;
                                        }
                                    }
                                    else if (diffMVdiffVal >= -5.96) {
                                        return 0;
                                    }
                                }
                                else if (MVDiff >= 311.33) {
                                    if (MVDiff < 385.62) {
                                        return 1;
                                    }
                                    else if (MVDiff >= 385.62) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffMVdiffVal >= 107.46) {
            if (MVDiff < 264.33) {
                if (negBalance < 0.08) {
                    if (diffMVdiffVal < 148.33) {
                        if (RsCsDiff < 358.23) {
                            return 0;
                        }
                        else if (RsCsDiff >= 358.23) {
                            if (Cs < 17.5) {
                                return 1;
                            }
                            else if (Cs >= 17.5) {
                                if (Cs < 20.87) {
                                    return 0;
                                }
                                else if (Cs >= 20.87) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 148.33) {
                        if (ssDCval < 214.81) {
                            if (TSC < 18.81) {
                                return 1;
                            }
                            else if (TSC >= 18.81) {
                                if (negBalance < 0.02) {
                                    if (ssDCval < 175.2) {
                                        return 0;
                                    }
                                    else if (ssDCval >= 175.2) {
                                        return 1;
                                    }
                                }
                                else if (negBalance >= 0.02) {
                                    if (diffTSC < 22.08) {
                                        if (diffAFD < 36.04) {
                                            return 0;
                                        }
                                        else if (diffAFD >= 36.04) {
                                            return 1;
                                        }
                                    }
                                    else if (diffTSC >= 22.08) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (ssDCval >= 214.81) {
                            return 0;
                        }
                    }
                }
                else if (negBalance >= 0.08) {
                    if (diffMVdiffVal < 158.2) {
                        if (diffTSC < 7.24) {
                            if (refDCval < 68.4) {
                                if (posBalance < 0.33) {
                                    return 1;
                                }
                                else if (posBalance >= 0.33) {
                                    if (diffAFD < 18.28) {
                                        return 0;
                                    }
                                    else if (diffAFD >= 18.28) {
                                        return 1;
                                    }
                                }
                            }
                            else if (refDCval >= 68.4) {
                                if (Cs < 9.43) {
                                    return 1;
                                }
                                else if (Cs >= 9.43) {
                                    if (diffRsCsdiff < 226.77) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 226.77) {
                                        if (Rs < 19.25) {
                                            return 1;
                                        }
                                        else if (Rs >= 19.25) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                        else if (diffTSC >= 7.24) {
                            if (Rs < 41.7) {
                                if (refDCval < 84.79) {
                                    if (RsDiff < 23.49) {
                                        return 0;
                                    }
                                    else if (RsDiff >= 23.49) {
                                        return 1;
                                    }
                                }
                                else if (refDCval >= 84.79) {
                                    if (MVDiff < 163.5) {
                                        return 1;
                                    }
                                    else if (MVDiff >= 163.5) {
                                        if (refDCval < 93.17) {
                                            return 0;
                                        }
                                        else if (refDCval >= 93.17) {
                                            if (MVDiff < 249.84) {
                                                return 1;
                                            }
                                            else if (MVDiff >= 249.84) {
                                                return 0;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (Rs >= 41.7) {
                                return 0;
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 158.2) {
                        if (AFD < 26.41) {
                            if (Scindex < 4.5) {
                                return 1;
                            }
                            else if (Scindex >= 4.5) {
                                return 0;
                            }
                        }
                        else if (AFD >= 26.41) {
                            return 1;
                        }
                    }
                }
            }
            else if (MVDiff >= 264.33) {
                if (CsDiff < 65.16) {
                    if (diffMVdiffVal < 217.69) {
                        return 0;
                    }
                    else if (diffMVdiffVal >= 217.69) {
                        if (Cs < 8.88) {
                            return 1;
                        }
                        else if (Cs >= 8.88) {
                            return 0;
                        }
                    }
                }
                else if (CsDiff >= 65.16) {
                    if (diffAFD < 34.91) {
                        if (negBalance < 0.05) {
                            return 0;
                        }
                        else if (negBalance >= 0.05) {
                            if (AFD < 38.2) {
                                if (refDCval < 154.4) {
                                    return 1;
                                }
                                else if (refDCval >= 154.4) {
                                    if (Cs < 21.51) {
                                        return 1;
                                    }
                                    else if (Cs >= 21.51) {
                                        return 0;
                                    }
                                }
                            }
                            else if (AFD >= 38.2) {
                                if (MVDiff < 345.7) {
                                    if (RsCsDiff < 221.31) {
                                        return 0;
                                    }
                                    else if (RsCsDiff >= 221.31) {
                                        if (RsCsDiff < 487.24) {
                                            return 1;
                                        }
                                        else if (RsCsDiff >= 487.24) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (MVDiff >= 345.7) {
                                    if (diffMVdiffVal < 365.88) {
                                        return 0;
                                    }
                                    else if (diffMVdiffVal >= 365.88) {
                                        if (MVDiff < 725.11) {
                                            return 1;
                                        }
                                        else if (MVDiff >= 725.11) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 34.91) {
                        if (Rs < 38.02) {
                            if (diffRsCsdiff < 20.32) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 20.32) {
                                return 1;
                            }
                        }
                        else if (Rs >= 38.02) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetectC(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    if (diffMVdiffVal < 100.45) {
        if (TSC < 10.3) {
            if (diffRsCsdiff < 31.4) {
                if (refDCval < 16.89) {
                    if (diffRsCsdiff < 4.54) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= 4.54) {
                        if (Cs < 6.68) {
                            return 0;
                        }
                        else if (Cs >= 6.68) {
                            if (Cs < 8.98) {
                                return 1;
                            }
                            else if (Cs >= 8.98) {
                                return 0;
                            }
                        }
                    }
                }
                else if (refDCval >= 16.89) {
                    return 0;
                }
            }
            else if (diffRsCsdiff >= 31.4) {
                if (SC < 17.38) {
                    if (diffMVdiffVal < 76.09) {
                        if (refDCval < 20.86) {
                            if (Cs < 10.82) {
                                return 1;
                            }
                            else if (Cs >= 10.82) {
                                return 0;
                            }
                        }
                        else if (refDCval >= 20.86) {
                            if (diffAFD < 13.04) {
                                if (SC < 10.77) {
                                    if (SC < 10.72) {
                                        return 0;
                                    }
                                    else if (SC >= 10.72) {
                                        return 1;
                                    }
                                }
                                else if (SC >= 10.77) {
                                    return 0;
                                }
                            }
                            else if (diffAFD >= 13.04) {
                                if (Rs < 8.2) {
                                    return 1;
                                }
                                else if (Rs >= 8.2) {
                                    if (refDCval < 40.18) {
                                        return 1;
                                    }
                                    else if (refDCval >= 40.18) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                    else if (diffMVdiffVal >= 76.09) {
                        if (TSC < 7.2) {
                            return 1;
                        }
                        else if (TSC >= 7.2) {
                            if (diffAFD < 12.47) {
                                return 0;
                            }
                            else if (diffAFD >= 12.47) {
                                return 1;
                            }
                        }
                    }
                }
                else if (SC >= 17.38) {
                    if (refDCval < 25.04) {
                        return 1;
                    }
                    else if (refDCval >= 25.04) {
                        if (diffMVdiffVal < 92.2) {
                            if (MVDiff < 211.88) {
                                return 0;
                            }
                            else if (MVDiff >= 211.88) {
                                if (diffTSC < 5.38) {
                                    return 0;
                                }
                                else if (diffTSC >= 5.38) {
                                    if (Cs < 9.02) {
                                        return 0;
                                    }
                                    else if (Cs >= 9.02) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (diffMVdiffVal >= 92.2) {
                            if (SC < 33.57) {
                                return 0;
                            }
                            else if (SC >= 33.57) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        else if (TSC >= 10.3) {
            if (diffRsCsdiff < 146.43) {
                if (AFD < 63.7) {
                    if (diffAFD < 11.52) {
                        if (MVDiff < 271.87) {
                            return 0;
                        }
                        else if (MVDiff >= 271.87) {
                            if (refDCval < 49.49) {
                                if (SC < 16.66) {
                                    return 1;
                                }
                                else if (SC >= 16.66) {
                                    return 0;
                                }
                            }
                            else if (refDCval >= 49.49) {
                                if (MVDiff < 272.77) {
                                    return 1;
                                }
                                else if (MVDiff >= 272.77) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffAFD >= 11.52) {
                        if (negBalance < 0.35) {
                            return 0;
                        }
                        else if (negBalance >= 0.35) {
                            if (SC < 27.7) {
                                if (gchDC < 8.62) {
                                    return 1;
                                }
                                else if (gchDC >= 8.62) {
                                    if (TSC < 11.53) {
                                        if (Cs < 10.18) {
                                            return 0;
                                        }
                                        else if (Cs >= 10.18) {
                                            return 1;
                                        }
                                    }
                                    else if (TSC >= 11.53) {
                                        return 0;
                                    }
                                }
                            }
                            else if (SC >= 27.7) {
                                return 0;
                            }
                        }
                    }
                }
                else if (AFD >= 63.7) {
                    if (AFD < 66.1) {
                        return 1;
                    }
                    else if (AFD >= 66.1) {
                        return 0;
                    }
                }
            }
            else if (diffRsCsdiff >= 146.43) {
                if (MVDiff < 188.98) {
                    if (Rs < 13.8) {
                        return 1;
                    }
                    else if (Rs >= 13.8) {
                        if (RsCsDiff < 177.12) {
                            return 1;
                        }
                        else if (RsCsDiff >= 177.12) {
                            if (MVDiff < 6.12) {
                                return 1;
                            }
                            else if (MVDiff >= 6.12) {
                                if (SC < 21.41) {
                                    if (Rs < 16.35) {
                                        return 1;
                                    }
                                    else if (Rs >= 16.35) {
                                        return 0;
                                    }
                                }
                                else if (SC >= 21.41) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
                else if (MVDiff >= 188.98) {
                    if (diffTSC < 4.99) {
                        if (RsDiff < 160.72) {
                            if (Cs < 22.47) {
                                return 1;
                            }
                            else if (Cs >= 22.47) {
                                return 0;
                            }
                        }
                        else if (RsDiff >= 160.72) {
                            return 0;
                        }
                    }
                    else if (diffTSC >= 4.99) {
                        if (negBalance < 0.1) {
                            return 0;
                        }
                        else if (negBalance >= 0.1) {
                            if (diffAFD < 20.46) {
                                if (ssDCval < 130.95) {
                                    if (AFD < 36.54) {
                                        if (ssDCval < 74.09) {
                                            return 0;
                                        }
                                        else if (ssDCval >= 74.09) {
                                            if (Rs < 23.36) {
                                                return 1;
                                            }
                                            else if (Rs >= 23.36) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (AFD >= 36.54) {
                                        if (posBalance < 0.03) {
                                            return 0;
                                        }
                                        else if (posBalance >= 0.03) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (ssDCval >= 130.95) {
                                    return 0;
                                }
                            }
                            else if (diffAFD >= 20.46) {
                                if (gchDC < 46.32) {
                                    if (CsDiff < 226.23) {
                                        return 1;
                                    }
                                    else if (CsDiff >= 226.23) {
                                        if (RsDiff < 169.82) {
                                            return 0;
                                        }
                                        else if (RsDiff >= 169.82) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (gchDC >= 46.32) {
                                    if (TSC < 41.32) {
                                        if (Rs < 24.48) {
                                            if (AFD < 54.34) {
                                                return 1;
                                            }
                                            else if (AFD >= 54.34) {
                                                return 0;
                                            }
                                        }
                                        else if (Rs >= 24.48) {
                                            return 1;
                                        }
                                    }
                                    else if (TSC >= 41.32) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 100.45) {
        if (diffTSC < 3.56) {
            if (diffTSC < 2.75) {
                if (CsDiff < 171.69) {
                    if (MVDiff < 186.58) {
                        if (Rs < 13.35) {
                            if (diffMVdiffVal < 125.69) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 125.69) {
                                return 1;
                            }
                        }
                        else if (Rs >= 13.35) {
                            return 0;
                        }
                    }
                    else if (MVDiff >= 186.58) {
                        if (ssDCval < 233.94) {
                            return 0;
                        }
                        else if (ssDCval >= 233.94) {
                            if (SC < 36.81) {
                                return 1;
                            }
                            else if (SC >= 36.81) {
                                return 0;
                            }
                        }
                    }
                }
                else if (CsDiff >= 171.69) {
                    if (Rs < 11.31) {
                        return 1;
                    }
                    else if (Rs >= 11.31) {
                        if (MVDiff < 237.94) {
                            if (RsDiff < 163.33) {
                                return 0;
                            }
                            else if (RsDiff >= 163.33) {
                                if (diffAFD < 21.09) {
                                    return 1;
                                }
                                else if (diffAFD >= 21.09) {
                                    return 0;
                                }
                            }
                        }
                        else if (MVDiff >= 237.94) {
                            return 0;
                        }
                    }
                }
            }
            else if (diffTSC >= 2.75) {
                if (ssDCval < 42.5) {
                    if (RsDiff < 38.72) {
                        return 0;
                    }
                    else if (RsDiff >= 38.72) {
                        return 1;
                    }
                }
                else if (ssDCval >= 42.5) {
                    if (RsCsDiff < 301.45) {
                        return 0;
                    }
                    else if (RsCsDiff >= 301.45) {
                        if (TSCindex < 8.5) {
                            return 0;
                        }
                        else if (TSCindex >= 8.5) {
                            return 1;
                        }
                    }
                }
            }
        }
        else if (diffTSC >= 3.56) {
            if (posBalance < 0.61) {
                if (diffRsCsdiff < 79.53) {
                    if (SC < 12.77) {
                        return 1;
                    }
                    else if (SC >= 12.77) {
                        if (negBalance < 0.6) {
                            if (diffRsCsdiff < 61.06) {
                                if (MVDiff < 169.6) {
                                    return 1;
                                }
                                else if (MVDiff >= 169.6) {
                                    return 0;
                                }
                            }
                            else if (diffRsCsdiff >= 61.06) {
                                if (diffMVdiffVal < 118.47) {
                                    return 0;
                                }
                                else if (diffMVdiffVal >= 118.47) {
                                    if (MVDiff < 275.27) {
                                        return 1;
                                    }
                                    else if (MVDiff >= 275.27) {
                                        if (TSC < 16.5) {
                                            return 1;
                                        }
                                        else if (TSC >= 16.5) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                        else if (negBalance >= 0.6) {
                            if (AFD < 66.43) {
                                return 1;
                            }
                            else if (AFD >= 66.43) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffRsCsdiff >= 79.53) {
                    if (diffTSC < 7.01) {
                        if (Scindex < 3.5) {
                            if (MVDiff < 406.09) {
                                return 1;
                            }
                            else if (MVDiff >= 406.09) {
                                return 0;
                            }
                        }
                        else if (Scindex >= 3.5) {
                            if (diffMVdiffVal < 128.23) {
                                return 0;
                            }
                            else if (diffMVdiffVal >= 128.23) {
                                if (TSC < 20.74) {
                                    return 1;
                                }
                                else if (TSC >= 20.74) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (diffTSC >= 7.01) {
                        if (refDCval < 149.66) {
                            if (diffMVdiffVal < 103.82) {
                                if (Rs < 20.4) {
                                    return 0;
                                }
                                else if (Rs >= 20.4) {
                                    return 1;
                                }
                            }
                            else if (diffMVdiffVal >= 103.82) {
                                if (CsDiff < 302.84) {
                                    return 1;
                                }
                                else if (CsDiff >= 302.84) {
                                    if (diffRsCsdiff < 344.74) {
                                        if (posBalance < 0.46) {
                                            return 1;
                                        }
                                        else if (posBalance >= 0.46) {
                                            return 0;
                                        }
                                    }
                                    else if (diffRsCsdiff >= 344.74) {
                                        if (TSC < 11.12) {
                                            if (Rs < 10.81) {
                                                return 1;
                                            }
                                            else if (Rs >= 10.81) {
                                                return 0;
                                            }
                                        }
                                        else if (TSC >= 11.12) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                        else if (refDCval >= 149.66) {
                            if (diffAFD < 16.01) {
                                return 0;
                            }
                            else if (diffAFD >= 16.01) {
                                if (Rs < 41.72) {
                                    if (refDCval < 215.55) {
                                        return 1;
                                    }
                                    else if (refDCval >= 215.55) {
                                        return 0;
                                    }
                                }
                                else if (Rs >= 41.72) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            else if (posBalance >= 0.61) {
                if (diffMVdiffVal < 148.69) {
                    if (negBalance < 0.76) {
                        if (refDCval < 86.85) {
                            if (AFD < 42.89) {
                                return 0;
                            }
                            else if (AFD >= 42.89) {
                                if (refDCval < 45.39) {
                                    return 1;
                                }
                                else if (refDCval >= 45.39) {
                                    return 0;
                                }
                            }
                        }
                        else if (refDCval >= 86.85) {
                            if (Rs < 20.6) {
                                return 1;
                            }
                            else if (Rs >= 20.6) {
                                if (refDCval < 93.71) {
                                    return 1;
                                }
                                else if (refDCval >= 93.71) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (negBalance >= 0.76) {
                        if (diffTSC < 14.35) {
                            if (diffRsCsdiff < 51.27) {
                                return 0;
                            }
                            else if (diffRsCsdiff >= 51.27) {
                                if (Cs < 14.79) {
                                    return 1;
                                }
                                else if (Cs >= 14.79) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffTSC >= 14.35) {
                            return 1;
                        }
                    }
                }
                else if (diffMVdiffVal >= 148.69) {
                    if (refDCval < 161.22) {
                        if (negBalance < 0.07) {
                            if (RsCsDiff < 405.85) {
                                if (RsDiff < 94.63) {
                                    if (gchDC < 23.05) {
                                        if (SC < 8.83) {
                                            return 1;
                                        }
                                        else if (SC >= 8.83) {
                                            return 0;
                                        }
                                    }
                                    else if (gchDC >= 23.05) {
                                        return 1;
                                    }
                                }
                                else if (RsDiff >= 94.63) {
                                    if (gchDC < 60.48) {
                                        if (refDCval < 71.97) {
                                            return 0;
                                        }
                                        else if (refDCval >= 71.97) {
                                            if (SC < 28.91) {
                                                return 1;
                                            }
                                            else if (SC >= 28.91) {
                                                return 0;
                                            }
                                        }
                                    }
                                    else if (gchDC >= 60.48) {
                                        return 1;
                                    }
                                }
                            }
                            else if (RsCsDiff >= 405.85) {
                                if (ssDCval < 108.64) {
                                    if (AFD < 34.87) {
                                        return 0;
                                    }
                                    else if (AFD >= 34.87) {
                                        return 1;
                                    }
                                }
                                else if (ssDCval >= 108.64) {
                                    return 1;
                                }
                            }
                        }
                        else if (negBalance >= 0.07) {
                            if (RsCsDiff < 98.5) {
                                if (posBalance < 0.78) {
                                    return 1;
                                }
                                else if (posBalance >= 0.78) {
                                    return 0;
                                }
                            }
                            else if (RsCsDiff >= 98.5) {
                                if (diffRsCsdiff < -20.22) {
                                    return 0;
                                }
                                else if (diffRsCsdiff >= -20.22) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (refDCval >= 161.22) {
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}
BOOL SCDetectD(mfxF64 Cs, mfxF64 refDCval, mfxF64 MVDiff, mfxF64 Rs, mfxF64 AFD, mfxF64 CsDiff, mfxF64 diffTSC, mfxF64 TSC, mfxF64 diffRsCsdiff, mfxF64 posBalance, mfxF64 gchDC, mfxF64 TSCindex, mfxF64 RsCsDiff, mfxF64 Scindex, mfxF64 SC, mfxF64 RsDiff, mfxF64 diffAFD, mfxF64 negBalance, mfxF64 ssDCval, mfxF64 diffMVdiffVal) {
    Scindex;
    TSCindex;

    if (diffMVdiffVal < 103.57) {
        if (diffAFD < 9) {
            if (AFD < 48.81) {
                if (ssDCval < 21.03) {
                    if (diffAFD < 0.49) {
                        return 0;
                    }
                    else if (diffAFD >= 0.49) {
                        if (Rs < 5.89) {
                            return 0;
                        }
                        else if (Rs >= 5.89) {
                            if (posBalance < 0.02) {
                                return 1;
                            }
                            else if (posBalance >= 0.02) {
                                if (diffAFD < 2.35) {
                                    return 0;
                                }
                                else if (diffAFD >= 2.35) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (ssDCval >= 21.03) {
                    if (RsDiff < 128.81) {
                        if (MVDiff < 440.6) {
                            return 0;
                        }
                        else if (MVDiff >= 440.6) {
                            if (TSC < 15.9) {
                                return 0;
                            }
                            else if (TSC >= 15.9) {
                                if (AFD < 34.53) {
                                    return 1;
                                }
                                else if (AFD >= 34.53) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (RsDiff >= 128.81) {
                        if (Rs < 17) {
                            if (RsDiff < 129) {
                                return 1;
                            }
                            else if (RsDiff >= 129) {
                                if (Rs < 16.99) {
                                    if (diffTSC < 4.21) {
                                        return 0;
                                    }
                                    else if (diffTSC >= 4.21) {
                                        if (Rs < 15.31) {
                                            return 1;
                                        }
                                        else if (Rs >= 15.31) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (Rs >= 16.99) {
                                    return 1;
                                }
                            }
                        }
                        else if (Rs >= 17) {
                            if (refDCval < 173.47) {
                                return 0;
                            }
                            else if (refDCval >= 173.47) {
                                if (refDCval < 173.6) {
                                    return 1;
                                }
                                else if (refDCval >= 173.6) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            else if (AFD >= 48.81) {
                if (TSC < 18.85) {
                    return 0;
                }
                else if (TSC >= 18.85) {
                    if (diffRsCsdiff < -1.12) {
                        return 0;
                    }
                    else if (diffRsCsdiff >= -1.12) {
                        if (AFD < 65.68) {
                            if (CsDiff < 384.05) {
                                return 1;
                            }
                            else if (CsDiff >= 384.05) {
                                return 0;
                            }
                        }
                        else if (AFD >= 65.68) {
                            return 0;
                        }
                    }
                }
            }
        }
        else if (diffAFD >= 9) {
            if (MVDiff < 113.21) {
                if (diffTSC < 10.17) {
                    if (Rs < 11.83) {
                        if (ssDCval < 40.05) {
                            if (Rs < 11.26) {
                                return 0;
                            }
                            else if (Rs >= 11.26) {
                                return 1;
                            }
                        }
                        else if (ssDCval >= 40.05) {
                            return 0;
                        }
                    }
                    else if (Rs >= 11.83) {
                        return 0;
                    }
                }
                else if (diffTSC >= 10.17) {
                    if (AFD < 25.68) {
                        if (RsDiff < 103.31) {
                            if (Cs < 12.63) {
                                if (SC < 16.95) {
                                    return 0;
                                }
                                else if (SC >= 16.95) {
                                    return 1;
                                }
                            }
                            else if (Cs >= 12.63) {
                                return 0;
                            }
                        }
                        else if (RsDiff >= 103.31) {
                            if (RsCsDiff < 190.88) {
                                return 1;
                            }
                            else if (RsCsDiff >= 190.88) {
                                if (MVDiff < 20.44) {
                                    return 1;
                                }
                                else if (MVDiff >= 20.44) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (AFD >= 25.68) {
                        return 0;
                    }
                }
            }
            else if (MVDiff >= 113.21) {
                if (negBalance < 0.21) {
                    if (SC < 40.72) {
                        if (gchDC < 56.54) {
                            return 0;
                        }
                        else if (gchDC >= 56.54) {
                            if (negBalance < 0.09) {
                                return 0;
                            }
                            else if (negBalance >= 0.09) {
                                if (Cs < 16.08) {
                                    return 0;
                                }
                                else if (Cs >= 16.08) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (SC >= 40.72) {
                        return 1;
                    }
                }
                else if (negBalance >= 0.21) {
                    if (diffRsCsdiff < 129.91) {
                        if (ssDCval < 40.03) {
                            if (posBalance < 0.65) {
                                return 1;
                            }
                            else if (posBalance >= 0.65) {
                                return 0;
                            }
                        }
                        else if (ssDCval >= 40.03) {
                            if (refDCval < 47.91) {
                                if (SC < 20.95) {
                                    return 1;
                                }
                                else if (SC >= 20.95) {
                                    return 0;
                                }
                            }
                            else if (refDCval >= 47.91) {
                                if (AFD < 10.85) {
                                    return 1;
                                }
                                else if (AFD >= 10.85) {
                                    if (diffRsCsdiff < 88.5) {
                                        return 0;
                                    }
                                    else if (diffRsCsdiff >= 88.5) {
                                        if (Rs < 17.59) {
                                            if (Rs < 15.57) {
                                                return 0;
                                            }
                                            else if (Rs >= 15.57) {
                                                return 1;
                                            }
                                        }
                                        else if (Rs >= 17.59) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 129.91) {
                        if (diffAFD < 20.57) {
                            if (ssDCval < 61.82) {
                                if (RsCsDiff < 215.79) {
                                    return 1;
                                }
                                else if (RsCsDiff >= 215.79) {
                                    if (ssDCval < 58.39) {
                                        if (Rs < 10.06) {
                                            if (Cs < 6.65) {
                                                return 1;
                                            }
                                            else if (Cs >= 6.65) {
                                                return 0;
                                            }
                                        }
                                        else if (Rs >= 10.06) {
                                            return 0;
                                        }
                                    }
                                    else if (ssDCval >= 58.39) {
                                        return 1;
                                    }
                                }
                            }
                            else if (ssDCval >= 61.82) {
                                if (diffMVdiffVal < 6.14) {
                                    if (ssDCval < 86.25) {
                                        return 1;
                                    }
                                    else if (ssDCval >= 86.25) {
                                        return 0;
                                    }
                                }
                                else if (diffMVdiffVal >= 6.14) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffAFD >= 20.57) {
                            if (diffTSC < 10.66) {
                                if (CsDiff < 156.77) {
                                    if (CsDiff < 104.16) {
                                        if (AFD < 34.44) {
                                            return 0;
                                        }
                                        else if (AFD >= 34.44) {
                                            return 1;
                                        }
                                    }
                                    else if (CsDiff >= 104.16) {
                                        return 1;
                                    }
                                }
                                else if (CsDiff >= 156.77) {
                                    if (RsDiff < 174.99) {
                                        return 1;
                                    }
                                    else if (RsDiff >= 174.99) {
                                        return 0;
                                    }
                                }
                            }
                            else if (diffTSC >= 10.66) {
                                if (posBalance < 0.37) {
                                    if (diffMVdiffVal < -61.11) {
                                        if (Cs < 18.96) {
                                            return 1;
                                        }
                                        else if (Cs >= 18.96) {
                                            return 0;
                                        }
                                    }
                                    else if (diffMVdiffVal >= -61.11) {
                                        return 1;
                                    }
                                }
                                else if (posBalance >= 0.37) {
                                    if (MVDiff < 194.75) {
                                        return 0;
                                    }
                                    else if (MVDiff >= 194.75) {
                                        if (MVDiff < 400.8) {
                                            return 1;
                                        }
                                        else if (MVDiff >= 400.8) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal >= 103.57) {
        if (diffTSC < 3.58) {
            if (diffRsCsdiff < 29.12) {
                if (Rs < 4.34) {
                    if (RsDiff < 6.91) {
                        return 0;
                    }
                    else if (RsDiff >= 6.91) {
                        return 1;
                    }
                }
                else if (Rs >= 4.34) {
                    if (RsDiff < 226.68) {
                        return 0;
                    }
                    else if (RsDiff >= 226.68) {
                        if (RsDiff < 234.19) {
                            return 1;
                        }
                        else if (RsDiff >= 234.19) {
                            if (Rs < 29.35) {
                                return 0;
                            }
                            else if (Rs >= 29.35) {
                                if (SC < 35.63) {
                                    return 0;
                                }
                                else if (SC >= 35.63) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
            else if (diffRsCsdiff >= 29.12) {
                if (refDCval < 48.67) {
                    return 1;
                }
                else if (refDCval >= 48.67) {
                    if (CsDiff < 161.23) {
                        if (RsCsDiff < 41.63) {
                            return 1;
                        }
                        else if (RsCsDiff >= 41.63) {
                            return 0;
                        }
                    }
                    else if (CsDiff >= 161.23) {
                        if (SC < 18.75) {
                            return 1;
                        }
                        else if (SC >= 18.75) {
                            if (RsDiff < 206.78) {
                                return 0;
                            }
                            else if (RsDiff >= 206.78) {
                                if (SC < 31.16) {
                                    if (Rs < 20.57) {
                                        if (AFD < 31.77) {
                                            return 1;
                                        }
                                        else if (AFD >= 31.77) {
                                            return 0;
                                        }
                                    }
                                    else if (Rs >= 20.57) {
                                        return 1;
                                    }
                                }
                                else if (SC >= 31.16) {
                                    if (AFD < 27.8) {
                                        return 1;
                                    }
                                    else if (AFD >= 27.8) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (diffTSC >= 3.58) {
            if (posBalance < 0.56) {
                if (diffTSC < 11.36) {
                    if (MVDiff < 264.04) {
                        if (MVDiff < 220.17) {
                            if (refDCval < 81.46) {
                                return 1;
                            }
                            else if (refDCval >= 81.46) {
                                if (diffMVdiffVal < 128.77) {
                                    if (Cs < 24.36) {
                                        return 0;
                                    }
                                    else if (Cs >= 24.36) {
                                        return 1;
                                    }
                                }
                                else if (diffMVdiffVal >= 128.77) {
                                    if (negBalance < 0.45) {
                                        return 1;
                                    }
                                    else if (negBalance >= 0.45) {
                                        if (RsDiff < 267.41) {
                                            if (negBalance < 0.46) {
                                                return 0;
                                            }
                                            else if (negBalance >= 0.46) {
                                                return 1;
                                            }
                                        }
                                        else if (RsDiff >= 267.41) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                        }
                        else if (MVDiff >= 220.17) {
                            if (Cs < 16.03) {
                                if (diffMVdiffVal < 131.63) {
                                    if (CsDiff < 63.67) {
                                        if (Rs < 5.28) {
                                            return 1;
                                        }
                                        else if (Rs >= 5.28) {
                                            return 0;
                                        }
                                    }
                                    else if (CsDiff >= 63.67) {
                                        return 1;
                                    }
                                }
                                else if (diffMVdiffVal >= 131.63) {
                                    return 1;
                                }
                            }
                            else if (Cs >= 16.03) {
                                return 0;
                            }
                        }
                    }
                    else if (MVDiff >= 264.04) {
                        if (CsDiff < 67.81) {
                            if (Rs < 8.81) {
                                if (SC < 9.9) {
                                    return 0;
                                }
                                else if (SC >= 9.9) {
                                    return 1;
                                }
                            }
                            else if (Rs >= 8.81) {
                                return 0;
                            }
                        }
                        else if (CsDiff >= 67.81) {
                            if (TSC < 15.92) {
                                if (diffRsCsdiff < 32.39) {
                                    return 0;
                                }
                                else if (diffRsCsdiff >= 32.39) {
                                    return 1;
                                }
                            }
                            else if (TSC >= 15.92) {
                                if (diffMVdiffVal < 177.6) {
                                    return 0;
                                }
                                else if (diffMVdiffVal >= 177.6) {
                                    if (RsCsDiff < 255.4) {
                                        return 0;
                                    }
                                    else if (RsCsDiff >= 255.4) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (diffTSC >= 11.36) {
                    if (diffRsCsdiff < 59.3) {
                        if (Rs < 11.78) {
                            return 1;
                        }
                        else if (Rs >= 11.78) {
                            if (Rs < 19.68) {
                                return 0;
                            }
                            else if (Rs >= 19.68) {
                                return 1;
                            }
                        }
                    }
                    else if (diffRsCsdiff >= 59.3) {
                        if (ssDCval < 184.08) {
                            if (negBalance < 0.1) {
                                return 0;
                            }
                            else if (negBalance >= 0.1) {
                                if (SC < 40.79) {
                                    return 1;
                                }
                                else if (SC >= 40.79) {
                                    if (gchDC < 5.15) {
                                        if (posBalance < 0.09) {
                                            return 0;
                                        }
                                        else if (posBalance >= 0.09) {
                                            return 1;
                                        }
                                    }
                                    else if (gchDC >= 5.15) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (ssDCval >= 184.08) {
                            if (Rs < 27.31) {
                                return 0;
                            }
                            else if (Rs >= 27.31) {
                                return 1;
                            }
                        }
                    }
                }
            }
            else if (posBalance >= 0.56) {
                if (diffMVdiffVal < 145.85) {
                    if (negBalance < 0.16) {
                        if (TSC < 39.07) {
                            if (Rs < 33.56) {
                                if (MVDiff < 108.29) {
                                    return 1;
                                }
                                else if (MVDiff >= 108.29) {
                                    return 0;
                                }
                            }
                            else if (Rs >= 33.56) {
                                return 1;
                            }
                        }
                        else if (TSC >= 39.07) {
                            if (Rs < 25.6) {
                                return 1;
                            }
                            else if (Rs >= 25.6) {
                                return 0;
                            }
                        }
                    }
                    else if (negBalance >= 0.16) {
                        if (diffAFD < 24.78) {
                            if (diffTSC < 8.47) {
                                return 0;
                            }
                            else if (diffTSC >= 8.47) {
                                if (diffRsCsdiff < 34.71) {
                                    return 0;
                                }
                                else if (diffRsCsdiff >= 34.71) {
                                    if (SC < 32.19) {
                                        return 1;
                                    }
                                    else if (SC >= 32.19) {
                                        return 0;
                                    }
                                }
                            }
                        }
                        else if (diffAFD >= 24.78) {
                            return 1;
                        }
                    }
                }
                else if (diffMVdiffVal >= 145.85) {
                    if (MVDiff < 319.07) {
                        if (diffRsCsdiff < 50.44) {
                            if (AFD < 34.93) {
                                if (Cs < 9.69) {
                                    return 1;
                                }
                                else if (Cs >= 9.69) {
                                    return 0;
                                }
                            }
                            else if (AFD >= 34.93) {
                                return 1;
                            }
                        }
                        else if (diffRsCsdiff >= 50.44) {
                            if (diffMVdiffVal < 166.64) {
                                if (diffRsCsdiff < 278.95) {
                                    if (Cs < 16.71) {
                                        if (diffTSC < 18.26) {
                                            return 1;
                                        }
                                        else if (diffTSC >= 18.26) {
                                            if (SC < 27.36) {
                                                return 0;
                                            }
                                            else if (SC >= 27.36) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (Cs >= 16.71) {
                                        return 0;
                                    }
                                }
                                else if (diffRsCsdiff >= 278.95) {
                                    if (posBalance < 0.94) {
                                        return 1;
                                    }
                                    else if (posBalance >= 0.94) {
                                        if (Cs < 21.61) {
                                            return 1;
                                        }
                                        else if (Cs >= 21.61) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (diffMVdiffVal >= 166.64) {
                                if (SC < 32.97) {
                                    if (negBalance < 0.08) {
                                        if (MVDiff < 241.5) {
                                            return 1;
                                        }
                                        else if (MVDiff >= 241.5) {
                                            if (ssDCval < 92.33) {
                                                if (AFD < 32.63) {
                                                    return 1;
                                                }
                                                else if (AFD >= 32.63) {
                                                    return 0;
                                                }
                                            }
                                            else if (ssDCval >= 92.33) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (negBalance >= 0.08) {
                                        return 1;
                                    }
                                }
                                else if (SC >= 32.97) {
                                    if (diffAFD < 35.26) {
                                        return 0;
                                    }
                                    else if (diffAFD >= 35.26) {
                                        if (negBalance < 0.01) {
                                            return 0;
                                        }
                                        else if (negBalance >= 0.01) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (MVDiff >= 319.07) {
                        if (diffAFD < 25.04) {
                            if (Cs < 8.2) {
                                if (RsDiff < 30.45) {
                                    return 0;
                                }
                                else if (RsDiff >= 30.45) {
                                    return 1;
                                }
                            }
                            else if (Cs >= 8.2) {
                                return 0;
                            }
                        }
                        else if (diffAFD >= 25.04) {
                            if (CsDiff < 61.28) {
                                if (Cs < 10.01) {
                                    return 1;
                                }
                                else if (Cs >= 10.01) {
                                    return 0;
                                }
                            }
                            else if (CsDiff >= 61.28) {
                                if (Cs < 19.27) {
                                    return 1;
                                }
                                else if (Cs >= 19.27) {
                                    if (AFD < 50.52) {
                                        return 0;
                                    }
                                    else if (AFD >= 50.52) {
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

BOOL SCDetectUF1(mfxF64 diffMVdiffVal, mfxF64 RsCsDiff,   mfxF64 MVDiff,   mfxF64 Rs,       mfxF64 AFD,
                 mfxF64 CsDiff,        mfxF64 diffTSC,    mfxF64 TSC,      mfxF64 gchDC,    mfxF64 diffRsCsdiff,
                 mfxF64 posBalance,    mfxF64 SC,         mfxF64 TSCindex, mfxF64 Scindex,  mfxF64 Cs,
                 mfxF64 diffAFD,       mfxF64 negBalance, mfxF64 ssDCval,  mfxF64 refDCval, mfxF64 RsDiff) {
    RsCsDiff;

    if (diffMVdiffVal <= 103.527) {
        if (diffTSC <= 6.134) {
            if (gchDC <= 15.269) {
                if (ssDCval <= 21.023) {
                    if (diffTSC <= 0.417) {
                        return 0;
                    }
                    else if (diffTSC > 0.417) {
                        if (Rs <= 5.462) {
                            return 0;
                        }
                        else if (Rs > 5.462) {
                            if (diffMVdiffVal <= -0.205) {
                                return 0;
                            }
                            else if (diffMVdiffVal > -0.205) {
                                return 1;
                            }
                        }
                    }
                }
                else if (ssDCval > 21.023) {
                    if (diffMVdiffVal <= 67.134) {
                        if (TSC <= 11.516) {
                            return 0;
                        }
                        else if (TSC > 11.516) {
                            if (Scindex <= 3) {
                                if (posBalance <= 0.179) {
                                    if (Scindex <= 2) {
                                        return 1;
                                    }
                                    else if (Scindex > 2) {
                                        return 0;
                                    }
                                }
                                else if (posBalance > 0.179) {
                                    return 0;
                                }
                            }
                            else if (Scindex > 3) {
                                return 0;
                            }
                        }
                    }
                    else if (diffMVdiffVal > 67.134) {
                        if (diffRsCsdiff <= 63.996) {
                            return 0;
                        }
                        else if (diffRsCsdiff > 63.996) {
                            if (SC <= 15.946) {
                                return 1;
                            }
                            else if (SC > 15.946) {
                                if (AFD <= 12.302) {
                                    if (gchDC <= 4.697) {
                                        return 1;
                                    }
                                    else if (gchDC > 4.697) {
                                        return 0;
                                    }
                                }
                                else if (AFD > 12.302) {
                                    if (Scindex <= 3) {
                                        if (TSC <= 15.497) {
                                            return 0;
                                        }
                                        else if (TSC > 15.497) {
                                            return 1;
                                        }
                                    }
                                    else if (Scindex > 3) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (gchDC > 15.269) {
                if (CsDiff <= 362.222) {
                    if (diffAFD <= 12.919) {
                        if (Scindex <= 3) {
                            if (TSC <= 16.481) {
                                return 0;
                            }
                            else if (TSC > 16.481) {
                                if (diffTSC <= -0.743) {
                                    return 0;
                                }
                                else if (diffTSC > -0.743) {
                                    if (Rs <= 8.677) {
                                        return 0;
                                    }
                                    else if (Rs > 8.677) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (Scindex > 3) {
                            return 0;
                        }
                    }
                    else if (diffAFD > 12.919) {
                        if (ssDCval <= 60.792) {
                            if (RsDiff <= 200.838) {
                                if (ssDCval <= 40.225) {
                                    if (Rs <= 9.944) {
                                        return 1;
                                    }
                                    else if (Rs > 9.944) {
                                        return 0;
                                    }
                                }
                                else if (ssDCval > 40.225) {
                                    return 0;
                                }
                            }
                            else if (RsDiff > 200.838) {
                                return 1;
                            }
                        }
                        else if (ssDCval > 60.792) {
                            return 0;
                        }
                    }
                }
                else if (CsDiff > 362.222) {
                    if (MVDiff <= 315.473) {
                        return 0;
                    }
                    else if (MVDiff > 315.473) {
                        if (diffRsCsdiff <= 0.847) {
                            return 1;
                        }
                        else if (diffRsCsdiff > 0.847) {
                            if (Scindex <= 4) {
                                return 1;
                            }
                            else if (Scindex > 4) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
        else if (diffTSC > 6.134) {
            if (AFD <= 32.282) {
                if (diffMVdiffVal <= 55.545) {
                    if (MVDiff <= 203.17) {
                        return 0;
                    }
                    else if (MVDiff > 203.17) {
                        if (gchDC <= 10.399) {
                            if (TSCindex <= 8) {
                                return 0;
                            }
                            else if (TSCindex > 8) {
                                return 1;
                            }
                        }
                        else if (gchDC > 10.399) {
                            if (diffAFD <= 21.16) {
                                return 0;
                            }
                            else if (diffAFD > 21.16) {
                                if (Scindex <= 3) {
                                    return 0;
                                }
                                else if (Scindex > 3) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else if (diffMVdiffVal > 55.545) {
                    if (gchDC <= 12.985) {
                        if (Scindex <= 3) {
                            if (negBalance <= 0.192) {
                                return 0;
                            }
                            else if (negBalance > 0.192) {
                                if (TSCindex <= 8) {
                                    if (Scindex <= 2) {
                                        return 1;
                                    }
                                    else if (Scindex > 2) {
                                        return 0;
                                    }
                                }
                                else if (TSCindex > 8) {
                                    return 1;
                                }
                            }
                        }
                        else if (Scindex > 3) {
                            if (diffTSC <= 10.055) {
                                return 0;
                            }
                            else if (diffTSC > 10.055) {
                                if (CsDiff <= 205.071) {
                                    return 1;
                                }
                                else if (CsDiff > 205.071) {
                                    return 0;
                                }
                            }
                        }
                    }
                    else if (gchDC > 12.985) {
                        return 0;
                    }
                }
            }
            else if (AFD > 32.282) {
                if (posBalance <= 0.424) {
                    if (MVDiff <= 103.598) {
                        return 0;
                    }
                    else if (MVDiff > 103.598) {
                        if (diffTSC <= 9.236) {
                            if (Cs <= 12.935) {
                                return 1;
                            }
                            else if (Cs > 12.935) {
                                if (posBalance <= 0.055) {
                                    if (Rs <= 22.969) {
                                        return 1;
                                    }
                                    else if (Rs > 22.969) {
                                        return 0;
                                    }
                                }
                                else if (posBalance > 0.055) {
                                    return 0;
                                }
                            }
                        }
                        else if (diffTSC > 9.236) {
                            if (negBalance <= 0.298) {
                                if (diffTSC <= 19.853) {
                                    if (AFD <= 62.919) {
                                        return 0;
                                    }
                                    else if (AFD > 62.919) {
                                        return 1;
                                    }
                                }
                                else if (diffTSC > 19.853) {
                                    return 1;
                                }
                            }
                            else if (negBalance > 0.298) {
                                if (Scindex <= 4) {
                                    return 1;
                                }
                                else if (Scindex > 4) {
                                    if (ssDCval <= 145.23) {
                                        return 1;
                                    }
                                    else if (ssDCval > 145.23) {
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (posBalance > 0.424) {
                    if (diffRsCsdiff <= 150.412) {
                        return 0;
                    }
                    else if (diffRsCsdiff > 150.412) {
                        if (negBalance <= 0.095) {
                            return 0;
                        }
                        else if (negBalance > 0.095) {
                            if (posBalance <= 0.535) {
                                return 0;
                            }
                            else if (posBalance > 0.535) {
                                if (TSC <= 37.057) {
                                    if (MVDiff <= 197.295) {
                                        return 0;
                                    }
                                    else if (MVDiff > 197.295) {
                                        if (gchDC <= 52.606) {
                                            return 1;
                                        }
                                        else if (gchDC > 52.606) {
                                            if (CsDiff <= 227.3) {
                                                return 0;
                                            }
                                            else if (CsDiff > 227.3) {
                                                if (Rs <= 20.007) {
                                                    return 1;
                                                }
                                                else if (Rs > 20.007) {
                                                    return 0;
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (TSC > 37.057) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (diffMVdiffVal > 103.527) {
        if (diffAFD <= 9.276) {
            if (CsDiff <= 269.852) {
                if (diffAFD <= 7.65) {
                    if (diffRsCsdiff <= 76.772) {
                        return 0;
                    }
                    else if (diffRsCsdiff > 76.772) {
                        if (Scindex <= 2) {
                            return 1;
                        }
                        else if (Scindex > 2) {
                            if (TSCindex <= 8) {
                                if (TSCindex <= 5) {
                                    return 1;
                                }
                                else if (TSCindex > 5) {
                                    if (Rs <= 13.454) {
                                        return 1;
                                    }
                                    else if (Rs > 13.454) {
                                        return 0;
                                    }
                                }
                            }
                            else if (TSCindex > 8) {
                                return 0;
                            }
                        }
                    }
                }
                else if (diffAFD > 7.65) {
                    if (gchDC <= 7.051) {
                        if (Scindex <= 3) {
                            return 1;
                        }
                        else if (Scindex > 3) {
                            return 0;
                        }
                    }
                    else if (gchDC > 7.051) {
                        if (Scindex <= 2) {
                            if (Rs <= 9.829) {
                                return 0;
                            }
                            else if (Rs > 9.829) {
                                return 1;
                            }
                        }
                        else if (Scindex > 2) {
                            return 0;
                        }
                    }
                }
            }
            else if (CsDiff > 269.852) {
                if (ssDCval <= 131.184) {
                    if (diffTSC <= 1.106) {
                        if (Rs <= 11.307) {
                            return 1;
                        }
                        else if (Rs > 11.307) {
                            return 0;
                        }
                    }
                    else if (diffTSC > 1.106) {
                        return 1;
                    }
                }
                else if (ssDCval > 131.184) {
                    if (AFD <= 26.448) {
                        return 1;
                    }
                    else if (AFD > 26.448) {
                        return 0;
                    }
                }
            }
        }
        else if (diffAFD > 9.276) {
            if (negBalance <= 0.081) {
                if (diffAFD <= 42.238) {
                    if (diffMVdiffVal <= 155.375) {
                        if (Scindex <= 5) {
                            return 0;
                        }
                        else if (Scindex > 5) {
                            if (Rs <= 31.406) {
                                return 0;
                            }
                            else if (Rs > 31.406) {
                                return 1;
                            }
                        }
                    }
                    else if (diffMVdiffVal > 155.375) {
                        if (AFD <= 41.648) {
                            if (MVDiff <= 255.295) {
                                if (negBalance <= 0.01) {
                                    if (negBalance <= 0.003) {
                                        if (Rs <= 27.073) {
                                            return 1;
                                        }
                                        else if (Rs > 27.073) {
                                            return 0;
                                        }
                                    }
                                    else if (negBalance > 0.003) {
                                        return 0;
                                    }
                                }
                                else if (negBalance > 0.01) {
                                    return 1;
                                }
                            }
                            else if (MVDiff > 255.295) {
                                if (negBalance <= 0.05) {
                                    if (Scindex <= 2) {
                                        return 1;
                                    }
                                    else if (Scindex > 2) {
                                        return 0;
                                    }
                                }
                                else if (negBalance > 0.05) {
                                    if (Rs <= 9.735) {
                                        return 0;
                                    }
                                    else if (Rs > 9.735) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        else if (AFD > 41.648) {
                            return 0;
                        }
                    }
                }
                else if (diffAFD > 42.238) {
                    if (Scindex <= 5) {
                        if (posBalance <= 0.696) {
                            return 0;
                        }
                        else if (posBalance > 0.696) {
                            if (diffRsCsdiff <= 105.615) {
                                if (Cs <= 13.764) {
                                    return 1;
                                }
                                else if (Cs > 13.764) {
                                    return 0;
                                }
                            }
                            else if (diffRsCsdiff > 105.615) {
                                return 1;
                            }
                        }
                    }
                    else if (Scindex > 5) {
                        return 0;
                    }
                }
            }
            else if (negBalance > 0.081) {
                if (MVDiff <= 338.661) {
                    if (diffMVdiffVal <= 131.661) {
                        if (posBalance <= 0.493) {
                            if (MVDiff <= 149) {
                                return 1;
                            }
                            else if (MVDiff > 149) {
                                if (diffTSC <= 7.316) {
                                    if (SC <= 19.096) {
                                        if (TSCindex <= 8) {
                                            if (Scindex <= 1) {
                                                if (Rs <= 5.277) {
                                                    return 1;
                                                }
                                                else if (Rs > 5.277) {
                                                    return 0;
                                                }
                                            }
                                            else if (Scindex > 1) {
                                                return 1;
                                            }
                                        }
                                        else if (TSCindex > 8) {
                                            if (AFD <= 29.122) {
                                                return 0;
                                            }
                                            else if (AFD > 29.122) {
                                                return 1;
                                            }
                                        }
                                    }
                                    else if (SC > 19.096) {
                                        if (Scindex <= 3) {
                                            if (TSC <= 13.309) {
                                                return 0;
                                            }
                                            else if (TSC > 13.309) {
                                                return 1;
                                            }
                                        }
                                        else if (Scindex > 3) {
                                            return 0;
                                        }
                                    }
                                }
                                else if (diffTSC > 7.316) {
                                    if (ssDCval <= 87.892) {
                                        return 1;
                                    }
                                    else if (ssDCval > 87.892) {
                                        if (negBalance <= 0.335) {
                                            return 0;
                                        }
                                        else if (negBalance > 0.335) {
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                        else if (posBalance > 0.493) {
                            if (gchDC <= 40.948) {
                                if (diffAFD <= 27.875) {
                                    return 0;
                                }
                                else if (diffAFD > 27.875) {
                                    return 1;
                                }
                            }
                            else if (gchDC > 40.948) {
                                return 1;
                            }
                        }
                    }
                    else if (diffMVdiffVal > 131.661) {
                        if (diffRsCsdiff <= 63.107) {
                            if (Cs <= 9.592) {
                                if (MVDiff <= 266.625) {
                                    if (posBalance <= 0.723) {
                                        return 1;
                                    }
                                    else if (posBalance > 0.723) {
                                        if (MVDiff <= 240.116) {
                                            return 0;
                                        }
                                        else if (MVDiff > 240.116) {
                                            return 1;
                                        }
                                    }
                                }
                                else if (MVDiff > 266.625) {
                                    if (AFD <= 18.206) {
                                        return 1;
                                    }
                                    else if (AFD > 18.206) {
                                        if (refDCval <= 36.898) {
                                            return 1;
                                        }
                                        else if (refDCval > 36.898) {
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else if (Cs > 9.592) {
                                return 0;
                            }
                        }
                        else if (diffRsCsdiff > 63.107) {
                            if (diffTSC <= 12.36) {
                                if (SC <= 28.064) {
                                    if (gchDC <= 21.289) {
                                        return 1;
                                    }
                                    else if (gchDC > 21.289) {
                                        if (Cs <= 12.089) {
                                            return 1;
                                        }
                                        else if (Cs > 12.089) {
                                            if (diffRsCsdiff <= 183.864) {
                                                if (diffTSC <= 10.04) {
                                                    return 0;
                                                }
                                                else if (diffTSC > 10.04) {
                                                    return 1;
                                                }
                                            }
                                            else if (diffRsCsdiff > 183.864) {
                                                return 1;
                                            }
                                        }
                                    }
                                }
                                else if (SC > 28.064) {
                                    if (diffTSC <= 6.093) {
                                        return 0;
                                    }
                                    else if (diffTSC > 6.093) {
                                        if (posBalance <= 0.472) {
                                            if (TSC <= 11.958) {
                                                if (Cs <= 20.736) {
                                                    return 1;
                                                }
                                                else if (Cs > 20.736) {
                                                    return 0;
                                                }
                                            }
                                            else if (TSC > 11.958) {
                                                return 1;
                                            }
                                        }
                                        else if (posBalance > 0.472) {
                                            if (AFD <= 28.563) {
                                                return 0;
                                            }
                                            else if (AFD > 28.563) {
                                                if (Rs <= 25.565) {
                                                    return 1;
                                                }
                                                else if (Rs > 25.565) {
                                                    return 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if (diffTSC > 12.36) {
                                return 1;
                            }
                        }
                    }
                }
                else if (MVDiff > 338.661) {
                    if (diffAFD <= 23.042) {
                        if (ssDCval <= 47.476) {
                            if (Rs <= 10.657) {
                                return 1;
                            }
                            else if (Rs > 10.657) {
                                return 0;
                            }
                        }
                        else if (ssDCval > 47.476) {
                            if (CsDiff <= 297.735) {
                                return 0;
                            }
                            else if (CsDiff > 297.735) {
                                if (Rs <= 21.059) {
                                    return 0;
                                }
                                else if (Rs > 21.059) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else if (diffAFD > 23.042) {
                        if (diffMVdiffVal <= 273.75) {
                            if (MVDiff <= 445.848) {
                                if (ssDCval <= 102.26) {
                                    return 1;
                                }
                                else if (ssDCval > 102.26) {
                                    if (MVDiff <= 360.295) {
                                        if (Rs <= 25.332) {
                                            return 1;
                                        }
                                        else if (Rs > 25.332) {
                                            return 0;
                                        }
                                    }
                                    else if (MVDiff > 360.295) {
                                        return 0;
                                    }
                                }
                            }
                            else if (MVDiff > 445.848) {
                                return 0;
                            }
                        }
                        else if (diffMVdiffVal > 273.75) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

///Implement decision tree to detect new scene.
/*!
 Decision tree for scene change detection.
 last model update: 01/12/2017 from Sebastian Possos using random forest approach.
 11/11 tests pass with this new model.

 dependancy: SCDetect1() ... SCDetectD(), DESICION_THR = 6.

 Scene texture parametters:
 \param[in] RsCsDiff
 Row sum, Column sum Difference allows to detect change in scene texture.  This is close to the similarity measure.
 \param[in] Rs
 Row sum characterize horizontal texture
 \param[in] Cs
 Column sum characterize vertical texture

 \param[in] diffMVdiffVal
 acceleration of motion vector
 \param[in] MVDiff
 motion vector difference allows to detect change in motion vectors

 \param[in] refDCval
 image mean for reference frame
 \param[in] gchDC
 gain change to help detect fade in and fade out.

 scene behavior and its history:
 \param[in] AFD
 Absolute Frame difference
 \param[in] TSC
 Temporal Spatial Complexity
 \param[in] diffTSC
 difference in motion compensated frame difference
 ...

 \returns 1 when scene change has been detected otherwise returns 0
 */

#define DESICION_THR 6
BOOL SceneChangeDetector::SCDetectGPU(mfxF64 diffMVdiffVal, mfxF64 RsCsDiff,   mfxF64 MVDiff,   mfxF64 Rs,       mfxF64 AFD,
                                      mfxF64 CsDiff,        mfxF64 diffTSC,    mfxF64 TSC,      mfxF64 gchDC,    mfxF64 diffRsCsdiff,
                                      mfxF64 posBalance,    mfxF64 SC,         mfxF64 TSCindex, mfxF64 Scindex,  mfxF64 Cs,
                                      mfxF64 diffAFD,       mfxF64 negBalance, mfxF64 ssDCval,  mfxF64 refDCval, mfxF64 RsDiff){
    mfxI32 result = 0;

    result += SCDetect1(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, RsCsDiff, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect2(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect3(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect4(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect5(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect6(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect7(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect8(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetect9(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetectA(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetectB(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetectC(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);
    result += SCDetectD(Cs, refDCval, MVDiff, Rs, AFD, CsDiff, diffTSC, TSC, diffRsCsdiff, posBalance, gchDC, TSCindex, RsCsDiff, Scindex, SC, RsDiff, diffAFD, negBalance, ssDCval, diffMVdiffVal);

    return(result > DESICION_THR);
}
// End MODEL 01122017

void SceneChangeDetector::SetUltraFastDetection() {
    support->size = Small_Size;
}

mfxStatus SceneChangeDetector::SetWidth(mfxI32 Width) {
    if(Width < SMALL_WIDTH) {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    _width = Width;

    return MFX_ERR_NONE;
}

mfxStatus SceneChangeDetector::SetHeight(mfxI32 Height) {
    if(Height < SMALL_HEIGHT) {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    _height = Height;
    return MFX_ERR_NONE;
}

mfxStatus SceneChangeDetector::SetPitch(mfxI32 Pitch) {
    if(_width < SMALL_WIDTH) {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if(Pitch < _width) {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    _pitch = Pitch;
    return MFX_ERR_NONE;
}

void SceneChangeDetector::SetNextField() {
    if(m_dataIn->interlaceMode != progressive_frame)
        m_dataIn->currentField = !m_dataIn->currentField;
}

mfxStatus SceneChangeDetector::SetDimensions(mfxI32 Width, mfxI32 Height, mfxI32 Pitch) {
    MFX_CHECK_STS(SetWidth(Width));
    MFX_CHECK_STS(SetHeight(Height));
    MFX_CHECK_STS(SetPitch(Pitch));
    return MFX_ERR_NONE;
}

/// Load kernel in CM device
/// Initialize input image with input data
mfxStatus SceneChangeDetector::SubSampleImage(mfxU32 srcWidth, mfxU32 srcHeight, const unsigned char * pData)
{
    int res = 0;

    m_gpuwidth  = srcWidth;
    m_gpuheight = srcHeight;

    gpustep_w = srcWidth / subWidth;
    gpustep_h = srcHeight / subHeight;

    mfxI32 streamParity = Get_stream_parity();

    // Load kernels
    if (NULL == m_pCmProgram)
    {
        if(m_platform == PLATFORM_INTEL_HSW)
         res = m_pCmDevice->LoadProgram((void *)asc_genx_hsw, sizeof(asc_genx_hsw), m_pCmProgram);
        else if (m_platform == PLATFORM_INTEL_SKL)
         res = m_pCmDevice->LoadProgram((void *)asc_genx_skl, sizeof(asc_genx_skl), m_pCmProgram);
     else
         res = m_pCmDevice->LoadProgram((void *)asc_genx_bdw, sizeof(asc_genx_bdw), m_pCmProgram);
     if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
    }

    if (NULL == m_pCmKernelTopField || NULL == m_pCmKernelBotField || NULL == m_pCmKernelFrame)
    {
          // select the fastest kernels for the given input surface size
          if (gpustep_w == 6 && gpustep_h == 7) {
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_480p), m_pCmKernelFrame);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_480t), m_pCmKernelTopField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_480b), m_pCmKernelBotField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
          }
          else if (gpustep_w == 11 && gpustep_h == 11) {
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_720p), m_pCmKernelFrame);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_720t), m_pCmKernelTopField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_720b), m_pCmKernelBotField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
          }
          else if (gpustep_w == 17 && gpustep_h == 16) {
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_1080p), m_pCmKernelFrame);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_1080t), m_pCmKernelTopField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_1080b), m_pCmKernelBotField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
          }
          else if (gpustep_w == 34 && gpustep_h == 33) {
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_2160p), m_pCmKernelFrame);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_2160t), m_pCmKernelTopField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_2160b), m_pCmKernelBotField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
          }
          else if (gpustep_w <= 32) {
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_var_p), m_pCmKernelFrame);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_var_t), m_pCmKernelTopField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
                  res = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SubSample_var_b), m_pCmKernelBotField);
                  if (res != 0) return MFX_ERR_DEVICE_FAILED;
          }
          else {
             return MFX_ERR_UNSUPPORTED;
          }
    } // (NULL == m_pCmKernel)

    if (NULL == m_pCmQueue)
    {
        res = m_pCmDevice->CreateQueue(m_pCmQueue);
        if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
    }

    res = m_pCmDevice->CreateBufferUP(subWidth * subHeight, (void *)pData, m_pCmBufferOut);
    if(res != 0 || m_pCmBufferOut == NULL) {
        return MFX_ERR_DEVICE_FAILED;
    }

    SurfaceIndex * pOuputBufferIndex = NULL;
    res = m_pCmBufferOut->GetIndex(pOuputBufferIndex);
    if (res!=0) {
        return MFX_ERR_DEVICE_FAILED;
    }

    // create 2D Threadspace to handle kernels
    assert((subWidth % OUT_BLOCK) == 0);
    UINT threadsWidth = subWidth / OUT_BLOCK;
    UINT threadsHeight = subHeight;

    for (CmKernel* pCmKernel : { m_pCmKernelTopField, m_pCmKernelBotField, m_pCmKernelFrame })
    {
        if (pCmKernel)
        {
            res = pCmKernel->SetThreadCount(threadsWidth * threadsHeight);
            if(res != 0 ) return MFX_ERR_DEVICE_FAILED;
        }
    }

    res = m_pCmDevice->CreateThreadSpace(threadsWidth, threadsHeight, m_pCmThreadSpace);
    if(res != 0 ) return MFX_ERR_DEVICE_FAILED;

    res = m_pCmThreadSpace->SelectThreadDependencyPattern(CM_NONE_DEPENDENCY);
    if(res != 0 ) return MFX_ERR_DEVICE_FAILED;

    return MFX_ERR_NONE;
}

void SceneChangeDetector::ReadOutputImage()
{
    mfxU8
        *pDst = videoData[Current_Frame]->layer[0].Image.Y,
        *pSrc = videoData[2]->layer[0].Image.Y;
    const mfxU32
        w = m_dataIn->layer[0].Original_Width,
        h = m_dataIn->layer[0].Original_Height,
        pitch = m_dataIn->layer[0].pitch;
    h;
    for( mfxI32 i = 0; i < m_dataIn->layer[0].Original_Height; i++) {
        memcpy_s(pDst, w, pSrc, w);
        pDst += pitch;
        pSrc += w;
    }
}

void SceneChangeDetector::GPUProcess()
{
    CmTask * task = NULL;
    CmEvent * e = NULL;
    int res;
    mfxStatus status = MFX_ERR_NONE;
    CmKernel* pCmKernel =
          m_dataIn->interlaceMode
        ? (   (m_dataIn->currentField == TopField)
            ? m_pCmKernelTopField
            : m_pCmKernelBotField)
        : m_pCmKernelFrame;

    res = m_pCmDevice->CreateTask(task);
    if(res != 0 ) status = MFX_ERR_DEVICE_FAILED;

    res = task->AddKernel(pCmKernel);
    if(res != 0 ) status = MFX_ERR_DEVICE_FAILED;

    res = m_pCmQueue->Enqueue(task, e, m_pCmThreadSpace);
     // wait result here!!!
    INT sts;

    if (CM_SUCCESS == res && e)
    {
        sts = e->WaitForTaskFinished();
        if(sts == CM_EXCEED_MAX_TIMEOUT)
            status = MFX_ERR_GPU_HANG;
    } else {
        status = MFX_ERR_DEVICE_FAILED;
    }

    if(e) m_pCmQueue->DestroyEvent(e);
    m_pCmDevice->DestroyTask(task);
}

/// Initialize Scene Change Detector Class
/*
  \param[in] core a Video core
  \param[in] Width
  \param[in] Height
  \param[in] Pitch
  \param[in] interlaceMode
  \param[in] _mfxDeviceType notify that Handle is for D3D9, D3D11 or VAAPI device
  \param[in] _mfxDeviceHdl handle to the device
 */
mfxStatus SceneChangeDetector::Init(mfxI32 Width, mfxI32 Height, mfxI32 Pitch, mfxU32 interlaceMode, CmDevice* pCmDevice)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bInited)
        return MFX_ERR_NONE;

    m_dataIn = NULL;
    support = NULL;
    videoData = NULL;

    m_dataIn = new VidData;
    MFX_CHECK_NULL_PTR1(m_dataIn);
    m_dataIn->layer = NULL;
    m_dataIn->layer = new ImDetails;
    MFX_CHECK_NULL_PTR1(m_dataIn->layer);

    if (m_pCmDevice == NULL)
        m_pCmDevice = pCmDevice;
    MFX_CHECK_NULL_PTR1(m_pCmDevice);

    size_t capValueSize = sizeof(m_platform);
    INT cmSts = m_pCmDevice->GetCaps(CAP_GPU_PLATFORM, capValueSize, &m_platform);
    if (CM_SUCCESS != cmSts) return MFX_ERR_DEVICE_FAILED;

    Params_Init();

    // [1] GetPlatformType()
    m_cpuOpt = CPU_NONE;

    if (__builtin_cpu_supports("sse4.2"))
    {
        m_cpuOpt = CPU_SSE4;
    }
#ifdef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP_AVX2
    if (__builtin_cpu_supports("avx2"))
    {
        m_cpuOpt = CPU_AVX2;
    }
#endif

    GPUProc = true; // UseGPU;
    m_dataIn->interlaceMode = ( interlaceMode & MFX_PICSTRUCT_PROGRESSIVE ) ? progressive_frame :
                              ( interlaceMode & MFX_PICSTRUCT_FIELD_TFF )   ? topfieldfirst_frame : bottomfieldFirst_frame;

    videoData = new VidSample *[NUM_VIDEO_SAMPLE_LAYERS];
    MFX_CHECK_NULL_PTR1(videoData);
    for(mfxI32 i = 0; i < NUM_VIDEO_SAMPLE_LAYERS; i++)
    {
        videoData[i] = new VidSample;
        MFX_CHECK_NULL_PTR1(videoData[i]);
        videoData[i]->layer = new imageData;
        MFX_CHECK_NULL_PTR1(videoData[i]->layer);
    }
    VidSample_Init();
    Setup_UFast_Environment();
    IO_Setup();

    support = new VidRead;
    MFX_CHECK_NULL_PTR1(support);
    VidRead_Init();

    SetDimensions(Width, Height, Pitch);
    if(GPUProc)
        SubSampleImage((mfxU32) Width, (mfxU32) Height, videoData[2]->layer[0].Image.Y);

    SetUltraFastDetection();

    SetInterlaceMode(m_dataIn->interlaceMode);
    dataReady = FALSE;

    m_bInited = true;
    return sts;
}


mfxStatus SceneChangeDetector::SetGoPSize(mfxU32 GoPSize)
{
    if(GoPSize > Double_HEVC_Gop)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    else if(GoPSize == Forbidden_GoP)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    support->gopSize = GoPSize;
    support->pendingSch = 0;

    return MFX_ERR_NONE;
}

void SceneChangeDetector::ResetGoPSize()
{
    SetGoPSize(Immediate_GoP);
}

mfxStatus SceneChangeDetector::Close()
{
    if ( ! m_bInited )
        return MFX_ERR_NONE;

    if(videoData != NULL)
    {
        VidSample_dispose();
        delete[] videoData;
        videoData = NULL;
    }

    if(support != NULL)
    {
        VidRead_dispose();
        delete support;
        support = NULL;
    }

    if(m_dataIn != NULL)
    {
        delete m_dataIn->layer;
        delete m_dataIn;
        m_dataIn = NULL;
    }

    std::map<void *, CmSurface2D *>::iterator itSrc;


    if (m_pCmDevice) {
        for (itSrc = m_tableCmRelations.begin() ; itSrc != m_tableCmRelations.end(); itSrc++)
        {
            CmSurface2D *temp = itSrc->second;
            m_pCmDevice->DestroySurface(temp);
        }
        m_tableCmRelations.clear();

        m_pCmDevice->DestroyThreadSpace(m_pCmThreadSpace);
        m_pCmDevice->DestroyBufferUP(m_pCmBufferOut);
        m_pCmDevice->DestroyKernel(m_pCmKernelTopField);
        m_pCmDevice->DestroyKernel(m_pCmKernelBotField);
        m_pCmDevice->DestroyKernel(m_pCmKernelFrame);
        m_pCmDevice->DestroyProgram(m_pCmProgram);

        m_pCmKernelTopField = NULL;
        m_pCmKernelBotField = NULL;
        m_pCmKernelFrame = NULL;
        m_pCmThreadSpace = NULL;
        m_pCmProgram = NULL;
        m_pCmQueue = NULL;
        m_pCmBufferOut = NULL;
        m_pCmDevice = NULL;
    }



    m_bInited = false;

    return MFX_ERR_NONE;
}

void SceneChangeDetector::GpuSubSampleASC_Image(mfxI32 srcWidth, mfxI32 srcHeight, mfxI32 inputPitch, Layers dstIdx, mfxU32 parity)
{
    parity;  inputPitch; srcHeight; srcWidth; dstIdx;

    GPUProcess();
    ReadOutputImage();
}

#endif // defined(MFX_VA) && defined(MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
