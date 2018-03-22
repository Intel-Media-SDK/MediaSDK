/*//////////////////////////////////////////////////////////////////////////////
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
*/
#include "asc_c_impl.h"
#include <algorithm>

using std::min;
using std::max;

void ME_SAD_8x8_Block_Search_C(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU16 *bestSAD, int *bestX, int *bestY) {
    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x += SAD_SEARCH_VSTEP) {/*x++) {*/
            pmfxU8
                pr = pRef + (y * pitch) + x,
                ps = pSrc;
            mfxU16
                SAD = 0;
            for (int i = 0; i < 8; i++) {
                SAD += (mfxU16)abs(pr[0] - ps[0]);
                SAD += (mfxU16)abs(pr[1] - ps[1]);
                SAD += (mfxU16)abs(pr[2] - ps[2]);
                SAD += (mfxU16)abs(pr[3] - ps[3]);
                SAD += (mfxU16)abs(pr[4] - ps[4]);
                SAD += (mfxU16)abs(pr[5] - ps[5]);
                SAD += (mfxU16)abs(pr[6] - ps[6]);
                SAD += (mfxU16)abs(pr[7] - ps[7]);
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

void ME_SAD_8x8_Block_FSearch_C(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU32 *bestSAD, int *bestX, int *bestY) {
    for (int y = 0; y < yrange; y++) {
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

void RsCsCalc_4x4_C(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxU16 pRs, pmfxU16 pCs)
{
    pSrc += (4 * srcPitch) + 4;
    for (mfxI16 i = 0; i < hblocks - 2; i++)
    {
        for (mfxI16 j = 0; j < wblocks - 2; j++)
        {
            mfxU16 accRs = 0;
            mfxU16 accCs = 0;

            for (mfxI32 k = 0; k < 4; k++)
            {
                for (mfxI32 l = 0; l < 4; l++)
                {
                    mfxU16 dRs = (mfxU16)abs(pSrc[l] - pSrc[l - srcPitch]) >> 2;
                    mfxU16 dCs = (mfxU16)abs(pSrc[l] - pSrc[l - 1]) >> 2;
                    accRs += (mfxU16)(dRs * dRs);
                    accCs += (mfxU16)(dCs * dCs);
                }
                pSrc += srcPitch;
            }
            pRs[i * wblocks + j] = accRs;
            pCs[i * wblocks + j] = accCs;

            pSrc -= 4 * srcPitch;
            pSrc += 4;
        }
        pSrc -= 4 * (wblocks - 2);
        pSrc += 4 * srcPitch;
    }
}

void RsCsCalc_bound_C(pmfxU16 pRs, pmfxU16 pCs, pmfxU16 pRsCs, pmfxU32 pRsFrame, pmfxU32 pCsFrame, int wblocks, int hblocks)
{
    mfxI32 len = wblocks * hblocks;
    mfxU16 accRs = 0;
    mfxU16 accCs = 0;

    //ASC_PRINTF("\n");
    for (mfxI32 i = 0; i < len; i++)
    {
        //    ASC_PRINTF("%i\t", pRs[i]);
        accRs += pRs[i] >> 7;
        accCs += pCs[i] >> 7;
        pRsCs[i] = (pRs[i] + pCs[i]) >> 1;
    }
    //ASC_PRINTF("\n");

    *pRsFrame = accRs;
    *pCsFrame = accCs;
}

void RsCsCalc_diff_C(pmfxU16 pRs0, pmfxU16 pCs0, pmfxU16 pRs1, pmfxU16 pCs1, int wblocks, int hblocks,
    pmfxU32 pRsDiff, pmfxU32 pCsDiff)
{
    mfxU32 len = wblocks * hblocks;
    mfxU16 accRs = 0;
    mfxU16 accCs = 0;

    for (mfxU32 i = 0; i < len; i++)
    {
        accRs += (mfxU16)abs((pRs0[i] >> 5) - (pRs1[i] >> 5));
        accCs += (mfxU16)abs((pCs0[i] >> 5) - (pCs1[i] >> 5));
    }
    *pRsDiff = accRs;
    *pCsDiff = accCs;
}


void ImageDiffHistogram_C(pmfxU8 pSrc, pmfxU8 pRef, mfxU32 pitch, mfxU32 width, mfxU32 height, mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC) {
    mfxI64 srcDC = 0;
    mfxI64 refDC = 0;

    histogram[0] = 0;
    histogram[1] = 0;
    histogram[2] = 0;
    histogram[3] = 0;
    histogram[4] = 0;

    for (mfxU32 i = 0; i < height; i++)
    {
        for (mfxU32 j = 0; j < width; j++)
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

void GainOffset_C(pmfxU8 *pSrc, pmfxU8 *pDst, mfxU16 width, mfxU16 height, mfxU16 pitch, mfxI16 gainDiff) {
    pmfxU8
        ss = *pSrc,
        dd = *pDst;
    for (mfxU16 i = 0; i < height; i++) {
        for (mfxU16 j = 0; j < width; j++) {
            mfxI16
                val = ss[j + i * pitch] - gainDiff;
            dd[j + i * pitch] = (mfxU8)min(max(val, mfxI16(0)), mfxI16(255));
        }
    }

    *pSrc = *pDst;
}

mfxStatus Calc_RaCa_pic_C(mfxU8 *pPicY, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs) {
    mfxI32 i, j;
    mfxI32 Rs, Cs;

    Rs = Cs = 0;
    for (i = 4; i < (height - 4); i += 4) {
        for (j = 4; j < (width - 4); j += 4) {
            calc_RACA_4x4_C(pPicY + i * pitch + j, pitch, &Rs, &Cs);
        }
    }

    mfxI32 w4 = (width - 8) >> 2;
    mfxI32 h4 = (height - 8) >> 2;
    mfxF64 d1 = 1.0 / (mfxF64)(w4*h4);
    mfxF64 drs = (mfxF64)Rs * d1;
    mfxF64 dcs = (mfxF64)Cs * d1;

    RsCs = sqrt(drs * drs + dcs * dcs);
    return MFX_ERR_NONE;
}

mfxI16 AvgLumaCalc_C(pmfxU32 pAvgLineVal, int len) {
    mfxU32
        acc = 0;
    mfxI16
        avgVal = 0;
    for (int i = 0; i < len; i++)
        acc += pAvgLineVal[i];
    avgVal = (mfxI16)(acc >> 9);
    return avgVal;
}