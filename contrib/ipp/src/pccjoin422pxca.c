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

#include "precomp.h"
#include "owncc.h"

#if  defined(_REF_LIB)
extern void lmmintrin_init();
#endif

#if _IPPLRB >= _IPPLRB_B1
void lrbAlYCbCr422_8u_P3C2R( const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize  roiSize);
void lrbYCbCr422_8u_P3C2R  ( const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize  roiSize);
#endif

#if defined _A6

  extern void Join422_8u_A6 (const Ipp8u* src[3], int step[3],
                                Ipp8u* dst, int dstep, int width, int height);
  extern void Split422_8u_A6(const Ipp8u* src, int step,
                                Ipp8u* dst[3], int dstep[3], int width, int height);

#endif

#if _IPP > _IPP_A6 /*defined _W7*/

  extern void Join422_8u_W7 (const Ipp8u* src[3], int step[3],
                                Ipp8u* dst, int dstep, int width, int height);
  extern void Split422_8u_W7(const Ipp8u* src, int step,
                                Ipp8u* dst[3], int dstep[3], int width, int height);

#endif

#if defined _I7

  extern void Join422_8u_I7 (Ipp8u*, Ipp8u*, Ipp8u*, Ipp8u*, int*, int, int);
  extern void Split422_8u_I7(const Ipp8u*, Ipp8u*, Ipp8u*, Ipp8u*, int*, int, int);

#endif


/*
  formerly mfxiJoin422_8u_P3C2R
  For input we has :
  plain #1
  y0y1y2y3y4y5     G   roiSize in this place
  plain #2
  cb0cb1cb2        B
  plain #3
  cr0cr1cr2        R

  for output we'l should to receive
  y0 cb0 y1 cr0 y2 cb1 y3 cr1 y4 cb2 y5 cr2 */


IPPFUN(IppStatus,mfxiYCbCr422_8u_P3C2R,( const Ipp8u* pSrc[3], int srcStep[3],
                                 Ipp8u* pDst, int dstStep, IppiSize  roiSize))
{

#if defined _I7
    int Inp_Arr[4];
#endif

  Ipp8u* pSrc_Y0 ;
  Ipp8u* pSrc_cB ;
  Ipp8u* pSrc_cR ;
  int Step_Y     ;
  int Step_cB    ;
  int Step_cR    ;

   /* test pointers */
  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pSrc[0]); IPP_BAD_PTR1_RET(pSrc[1]); IPP_BAD_PTR1_RET(pSrc[2]);
  IPP_BADARG_RET((roiSize.width < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height<=0), ippStsSizeErr);

  pSrc_Y0 = (Ipp8u*)pSrc[0];
  pSrc_cB = (Ipp8u*)pSrc[1];
  pSrc_cR = (Ipp8u*)pSrc[2];
  Step_Y  = srcStep[0];
  Step_cB = srcStep[1];
  Step_cR = srcStep[2];
   /* test ROI size */
#if  defined(_REF_LIB)
    lmmintrin_init();
#endif

#if _IPPLRB >= _IPPLRB_B1
    if(
        !(
          (srcStep[0] & 0x0f) || (srcStep[1] & 0x0f) || (srcStep[2] & 0x07) ||
          (dstStep & 0x0f) || (IPP_UINT_PTR(pDst) & 0x0f) ||
          (IPP_UINT_PTR(pSrc[0]) & 0x0f) || (IPP_UINT_PTR(pSrc[1]) & 0x07) ||
          (IPP_UINT_PTR(pSrc[2]) & 0x07)
        )
      )
        lrbAlYCbCr422_8u_P3C2R( pSrc, srcStep, pDst, dstStep, roiSize);
    else
        lrbYCbCr422_8u_P3C2R( pSrc, srcStep, pDst, dstStep, roiSize);
    //return ippStsNoErr;
#elif !defined _I7
  roiSize.width &= -2;

//#if !defined _I7
# if _IPP < _IPP_A6

  roiSize.width *=  2;
  int i;
  for(i = 0; i < roiSize.height; i++) {
      int j;
      int k = 0;
      int m = 0;
    for(j = 0; j < roiSize.width; j += 4)
    {
      pDst[j + 0] = pSrc_Y0[m    ];
      pDst[j + 1] = pSrc_cB[k    ];
      pDst[j + 2] = pSrc_Y0[m + 1];
      pDst[j + 3] = pSrc_cR[k    ];
      m += 2;
      k++;
    }
    pSrc_Y0 += Step_Y;
    pSrc_cB += Step_cB;
    pSrc_cR += Step_cR;
    pDst    += dstStep;
  }

# elif _IPP > _IPP_A6

     Join422_8u_W7(pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height);

# else

     Join422_8u_A6(pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height);

# endif
#else
  Inp_Arr[0] = Step_Y;
  Inp_Arr[1] = Step_cB;
  Inp_Arr[2] = Step_cR;
  Inp_Arr[3] = dstStep;

  Join422_8u_I7(pSrc_Y0, pSrc_cB, pSrc_cR, pDst, Inp_Arr, roiSize.width, roiSize.height);

#endif

  return ippStsNoErr;
} /* mfxiJoin422_8u_P3C2R() */

