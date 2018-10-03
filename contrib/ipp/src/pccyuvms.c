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

#if !( (_IPP > _IPP_A6) || (_IPP32E>=_IPP32E_M7)/*|| (_IPPLRB >= _IPPLRB_B1)*/)

#if  defined(_REF_LIB)
extern void lmmintrin_init();
#endif

#define CLIP(x) ((x < 0) ? 0 : ((x > 255) ? 255 : x))


static
void mfxownYCbCr420ToYCbCr422_8u_P3R(
  const Ipp8u*   pSrc[3],
        int      srcStep[3],
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize,
        int      orderUV)
{
  int w;
  int h;

  const Ipp8u* pSrcU = pSrc[1];
  const Ipp8u* pSrcV = pSrc[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  int srcStepU = srcStep[1];
  int srcStepV = srcStep[2];

  if( orderUV )
  {
    pSrcU = pSrc[2];
    pSrcV = pSrc[1];
    srcStepU = srcStep[2];
    srcStepV = srcStep[1];
  }

  /* Y plane */
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dst;

    src = pSrc[0] + h * srcStep[0];
    dst = pDst[0] + h * dstStep[0];

    for( w = 0; w < width; w ++ )
    {
      *dst++ = *src++;
    }
  }

  height /= 2;width /= 2;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcu;
    const Ipp8u* srcv;
    Ipp8u* dstu;
    Ipp8u* dstv;

    srcu = pSrcU + h * srcStepU;
    srcv = pSrcV + h * srcStepV;

    dstu = pDst[1] + h * 2 * dstStep[1];
    dstv = pDst[2] + h * 2 * dstStep[2];

    for( w = 0; w < width; w ++ )
    {
      *(dstu+dstStep[1]) = *srcu;
      *dstu++            = *srcu++;
      *(dstv+dstStep[2]) = *srcv;
      *dstv++            = *srcv++;
    }
  }

  return;
} /* mfxownYCbCr420ToYCbCr422_8u_P3R() */




static
void mfxownYCrCb420ToYCbCr422_8u_P3R(
  const Ipp8u*   pSrc[3],
        int      srcStep[3],
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize)
{
  int w;
  int h;

  int width  = roiSize.width;
  int height = roiSize.height;

  /* Y plane */
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dst;

    src = pSrc[0] + h * srcStep[0];
    dst = pDst[0] + h * dstStep[0];

    for( w = 0; w < width; w ++ )
    {
      *dst++ = *src++;
    }
  }

  height /= 2;width  /= 2;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcu;
    const Ipp8u* srcv;

    Ipp8u* dstu;
    Ipp8u* dstv;

    srcu = pSrc[2] + h * srcStep[2]; // Cb or U
    srcv = pSrc[1] + h * srcStep[1]; // Cr or V

    dstu = pDst[1] + h * 2 * dstStep[1];
    dstv = pDst[2] + h * 2 * dstStep[2];

    for( w = 0; w < width; w ++ )
    {
      *(dstu+dstStep[1]) = *srcu;
      *dstu++            = *srcu++;
      *(dstv+dstStep[2]) = *srcv;
      *dstv++            = *srcv++;
    }
  }

  return;
} /* mfxownYCrCb420ToYCbCr422_8u_P3R() */


#if !(_IPPLRB >= _IPPLRB_B1)

static
void mfxownYCbCr422ToYCbCr420_8u_P3R(
  const Ipp8u*   pSrc[3],
        int      srcStep[3],
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize)
{
  int w;
  int h;

  int width  = roiSize.width;
  int height = roiSize.height;

  /* Y plane */
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dst;

    src = pSrc[0] + h * srcStep[0];
    dst = pDst[0] + h * dstStep[0];

    for( w = 0; w < width; w ++ )
    {
      *dst++ = *src++;
    }
  }

  width  /= 2;
  height /= 2;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcu;
    const Ipp8u* srcv;

    Ipp8u* dstu;
    Ipp8u* dstv;

    srcu = pSrc[1] + h * 2 * srcStep[1];
    srcv = pSrc[2] + h * 2 * srcStep[2];

    dstu = pDst[1] + h * dstStep[1];
    dstv = pDst[2] + h * dstStep[2];

    for( w = 0; w < width; w ++ )
    {
      *dstu++ = *srcu++;
      *dstv++ = *srcv++;
    }
  }

  return;
} /* mfxownYCbCr422ToYCbCr420_8u_P3R() */





static
void mfxownYCbCr422ToYCbCr422_8u_C2P3R(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize,
        int      orderUV)
{
  int w;
  int h;

  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];

  int dstStepU = dstStep[1];
  int dstStepV = dstStep[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  if( orderUV )
  {
    pDstU = pDst[2];
    pDstV = pDst[1];

    dstStepU = dstStep[2];
    dstStepV = dstStep[1];
  }

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty;
    Ipp8u* dstu;
    Ipp8u* dstv;

    src = pSrc + h * srcStep;

    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + h * dstStepU;
    dstv = pDstV + h * dstStepV;

    for( w = 0; w < width; w += 2 )
    {
      *dsty++ = *src++;
      *dstu++ = *src++;
      *dsty++ = *src++;
      *dstv++ = *src++;
    }
  }

  return;
} /* mfxownYCbCr422ToYCbCr422_8u_C2P3R() */


static
void mfxownYCbCr422ToYCbCr420_8u_C2P3R(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize,
        int      orderUV)
{
  int w;
  int h;

  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];

  int dstStepU = dstStep[1];
  int dstStepV = dstStep[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  if( orderUV )
  {
    pDstU = pDst[2];
    pDstV = pDst[1];

    dstStepU = dstStep[2];
    dstStepV = dstStep[1];
  }

  for( h = 0; h < height; h += 2 )
  {
    const Ipp8u* src;
    Ipp8u* dsty;
    Ipp8u* dstu;
    Ipp8u* dstv;

    src  = pSrc + h * srcStep;

    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + (h>>1) * dstStepU;
    dstv = pDstV + (h>>1) * dstStepV;

    for( w = 0; w < width; w += 2 )
    {
      dsty[0] = src[0];
      dsty[0 + dstStep[0]] = src[0 + srcStep];
      dsty[1] = src[2];
      dsty[1 + dstStep[0]] = src[2 + srcStep];

      *dstu++  = src[1];
      *dstv++  = src[3];

      dsty += 2;
      src += 4;
    }
  }

  return;
} /* mfxownYCbCr422ToYCbCr420_8u_C2P3R() */


static
void mfxownYCbCr422ToYCbCr411_8u_C2P3R(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize,
        int      orderUV)
{
  int w;
  int h;

  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];

  int dstStepU = dstStep[1];
  int dstStepV = dstStep[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  if( orderUV )
  {
    pDstU = pDst[2];
    pDstV = pDst[1];

    dstStepU = dstStep[2];
    dstStepV = dstStep[1];
  }

  for( h = 0; h < height; h++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty;
    Ipp8u* dstu;
    Ipp8u* dstv;

    src  = pSrc   + h * srcStep;

    dsty = pDst[0]+ h * dstStep[0];
    dstu = pDstU  + h * dstStepU;
    dstv = pDstV  + h * dstStepV;

    for( w = 0; w < width; w += 4 )
    {
      *dsty++ = src[0];
      *dsty++ = src[2];
      *dsty++ = src[4];
      *dsty++ = src[6];
      *dstu++ = src[1];
      *dstv++ = src[3];

       src += 8;
    }
  }

  return;
} /* mfxownYCbCr422ToYCbCr411_8u_C2P3R() */


static
void mfxownYCbCr420ToYCbCr420_8u_P2P3R(
  const Ipp8u*   pSrcY,
        int      srcYStep,
  const Ipp8u*   pSrcUV,
        int      srcUVStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize)
{
  int w;
  int h;

  int width  = roiSize.width;
  int height = roiSize.height;

  /* Y plane */
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dst;

    dst = pDst[0]+ h * dstStep[0];
    src = pSrcY  + h * srcYStep;

    for( w = 0; w < width; w ++ )
    {
      *dst++ = *src++;
    }
  }

  width  /= 2;
  height /= 2;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcuv;
    Ipp8u* dstu;
    Ipp8u* dstv;

    srcuv = pSrcUV + h *  srcUVStep;
    dstu = pDst[1] + h *  dstStep[1];
    dstv = pDst[2] + h *  dstStep[2];

    for( w = 0; w < width; w ++ )
    {
      *dstu++    = srcuv[0];
      *dstv++    = srcuv[1];

      srcuv += 2;
    }
  } /* for h */

  return;
} /* mfxownYCbCr420ToYCbCr420_8u_P2P3R() */



static
void mfxownCbYCr422ToYCbCr422_8u_C2P3R(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize)
{
  int w;
  int h;

  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];

  int dstStepU = dstStep[1];
  int dstStepV = dstStep[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty;
    Ipp8u* dstu;
    Ipp8u* dstv;

    src = pSrc + h * srcStep;

    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + h * dstStepU;
    dstv = pDstV + h * dstStepV;

    for( w = 0; w < width; w += 2 )
    {
      *dstu++ = *src++;
      *dsty++ = *src++;
      *dstv++ = *src++;
      *dsty++ = *src++;
    }
  }

  return;
} /* mfxownCbYCr422ToYCbCr422_8u_C2P3R() */


static
void mfxownCbYCr422ToYCbCr411_8u_C2P3R(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize)
{
  int w;
  int h;

  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];

  int dstStepU = dstStep[1];
  int dstStepV = dstStep[2];

  int width  = roiSize.width;
  int height = roiSize.height;

  for( h = 0; h < height; h++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty;
    Ipp8u* dstu;
    Ipp8u* dstv;

    src  = pSrc   + h * srcStep;

    dsty = pDst[0]+ h * dstStep[0];
    dstu = pDstU  + h * dstStepU;
    dstv = pDstV  + h * dstStepV;

    for( w = 0; w < width; w += 4 )
    {
      *dsty++ = src[1];
      *dsty++ = src[3];
      *dsty++ = src[5];
      *dsty++ = src[7];
      *dstu++ = src[0];
      *dstv++ = src[2];

       src += 8;
    }
  }

  return;
} /* mfxownCbYCr422ToYCbCr411_8u_C2P3R() */




#else

void mfxownYCbCr422ToYCbCr420_8u_P3R(const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize);
extern void mfxownCbYCr422ToYCbCr422_8u_C2P3R(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize);
extern void mfxownCbYCr422ToYCbCr411_8u_C2P3R(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize);
void mfxownYCbCr422ToYCbCr420_8u_C2P3R(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize, int orderUV);
void mfxownYCbCr422ToYCbCr411_8u_C2P3R(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize,int orderUV);
void mfxownYCbCr420ToYCbCr420_8u_P2P3R(const Ipp8u* pSrcY,int srcYStep,const Ipp8u* pSrcUV, int srcUVStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize);
void mfxownYCbCr422ToYCbCr422_8u_C2P3R(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize, int orderUV);

#endif


#else

extern void mfxownYCbCr422ToYCbCr420_8u_P3R(const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize );
extern void mfxownYCbCr420ToYCbCr422_8u_P3R(const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize, int );
extern void mfxownYCbCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize,int  );
extern void mfxalYCbCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize,int  );
extern void mfxownYCbCr422ToYCbCr420_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize, int orderUV );
extern void mfxownYCbCr420ToYCbCr420_8u_P2P3R(const Ipp8u* pSrcY,int srcYStep,const Ipp8u* pSrcUV,
        int srcUVStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize );

extern void mfxownCbYCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize  );
#endif




/* ///////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr422ToYCbCr420_8u_P3R,  mfxiYCbCr420ToYCbCr422_8u_P3R
//  Return:
//    ippStsNoErr       Ok
//    ippStsNullPtrErr  One or more pointers are NULL
//    ippStsSizeErr     if roiSize.width < 2 || roiSize.height < 2
//
//  Arguments:
//    pSrc       array of pointers to the components of the source image
//    srcStep    array of step values for every component
//    pDst       array of pointers to the components of the destination image
//    dstStep    array of steps values for every component
//    roiSize    region of interest to be processed, in pixels
//  Notes:
//    roiSize.width and roiSize.height  should be multiple 2.
*/

/* P422ToI420(IYUV) */
IPPFUN(IppStatus, mfxiYCbCr422ToYCbCr420_8u_P3R,(
  const Ipp8u*   pSrc[3],
        int      srcStep[3],
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR2_RET( pSrc, pDst );
  IPP_BAD_PTR3_RET( pSrc[0], pSrc[1], pSrc[2]);
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);

  roiSize.width  &= ~1;
  roiSize.height &= ~1;

  mfxownYCbCr422ToYCbCr420_8u_P3R( pSrc, srcStep, pDst, dstStep, roiSize);

  return ippStsNoErr;
} /* mfxiYCbCr422ToYCbCr420_8u_P3R() */


/* IYUVToP422 */
IPPFUN(IppStatus, mfxiYCbCr420ToYCbCr422_8u_P3R,(
  const Ipp8u*   pSrc[3],
        int      srcStep[3],
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR2_RET( pSrc, pDst );
  IPP_BAD_PTR3_RET( pSrc[0], pSrc[1], pSrc[2]);
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);

  roiSize.width  &= ~1;
  roiSize.height &= ~1;

  mfxownYCbCr420ToYCbCr422_8u_P3R( pSrc, srcStep, pDst, dstStep, roiSize, 0 );

  return ippStsNoErr;
} /* mfxiYCbCr420ToYCbCr422_8u_P3R() */



/* ///////////////////////////////////////////////////////////////////////////
//  Name: mfxiYCbCr422ToYCbCr422_8u_C2P3R mfxiYCrCb422ToYCbCr422_8u_C2P3R
//
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2
//
//  Arguments:
//    pSrc        array of pointers to the components of the source image
//    srcStep     array of step values for every component
//    pDst        array of pointers to the components of the destination image
//    dstStep     array of steps values for every component
//     roiSize    region of interest to be processed, in pixels
//  Notes:
//    roiSize.width            should be multiple 2.
*/

/* YUY2ToP422 */
IPPFUN(IppStatus, mfxiYCbCr422_8u_C2P3R,(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR3_RET( pSrc, pDst, dstStep );
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 1), ippStsSizeErr);

  roiSize.width  &= ~1;

#if ( (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7) )
  if(
     !(
      ( ( IPP_UINT_PTR(pDst[0]) | IPP_UINT_PTR(pSrc) | dstStep[0] | srcStep ) & 0x0f ) ||
           ( roiSize.width & 0x0f )
       )
    )
  {
    mfxalYCbCr422ToYCbCr422_8u_C2P3R( pSrc, srcStep, pDst, dstStep, roiSize, 0);
  }
  else
  {
#endif
    mfxownYCbCr422ToYCbCr422_8u_C2P3R( pSrc, srcStep, pDst, dstStep, roiSize, 0);
#if ( (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7) )
  }
#endif

  return ippStsNoErr;
} /* mfxiYCbCr422_8u_C2P3R() */



IPPFUN(IppStatus, mfxiCbYCr422ToYCbCr422_8u_C2P3R,(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR2_RET( pSrc, pDst );
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BAD_SIZE_RET(roiSize.height);
  roiSize.width  &= ~1;

  mfxownCbYCr422ToYCbCr422_8u_C2P3R( pSrc, srcStep, pDst, dstStep, roiSize);

  return ippStsNoErr;
} /* mfxiCbYCr422ToYCbCr422_8u_C2P3R() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name: mfxiYCbCr422ToYCbCr420_8u_C2P3R
//
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2 || roiSize.height < 2
//
//  Arguments:
//    pSrc       array of pointers to the components of the source image
//    srcStep    array of step values for every component
//    pDst       array of pointers to the components of the destination image
//    dstStep    array of steps values for every component
//     roiSize   region of interest to be processed, in pixels
//  Notes:
//    roiSize.width and roiSize.height  should be multiple 2.
*/

/* YUY2ToI420 */
IPPFUN(IppStatus, mfxiYCbCr422ToYCbCr420_8u_C2P3R,(
  const Ipp8u*   pSrc,
        int      srcStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR2_RET( pSrc, pDst );
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);

  roiSize.width  &= ~1;
  roiSize.height &= ~1;

  mfxownYCbCr422ToYCbCr420_8u_C2P3R( pSrc, srcStep, pDst, dstStep, roiSize, 0);

  return ippStsNoErr;
} /* mfxiYCbCr422ToYCbCr420_8u_C2P3R() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr420ToYCbCr422_Filter_8u_P2P3R
//  Purpose:    Converts a IYUV image to the P422 image.
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2 || roiSize.height < 8
//
//  Arguments:
//    pSrcY        pointer to the source Y plane
//    srcYStep     step  for the source Y plane
//    pSrcUV       pointer to the source UV plane
//    srcUVStep    step  for the source UV plane
//    pDst         array of pointers to the components of the destination image
//    dstStep      array of steps values for every component
//    roiSize      region of interest to be processed, in pixels
//  Notes:
//    roiSize.width  should be multiple 2.
//    roiSize.height should be multiple 8.
//    We use here Catmull-Rom interpolation.
*/


/* NV12ToP422, formerly mfxiYCbCr420ToYCbCr420_8u_P2P3R */
IPPFUN(IppStatus, mfxiYCbCr420_8u_P2P3R,(
  const Ipp8u*   pSrcY,
        int      srcYStep,
  const Ipp8u*   pSrcUV,
        int      srcUVStep,
        Ipp8u*   pDst[3],
        int      dstStep[3],
        IppiSize roiSize))
{
  IPP_BAD_PTR1_RET( pDst );
  IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2]);
  IPP_BAD_PTR2_RET( pSrcY, pSrcUV );
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);

  roiSize.width  &= ~1;
  roiSize.height &= ~1;

  mfxownYCbCr420ToYCbCr420_8u_P2P3R( pSrcY, srcYStep, pSrcUV, srcUVStep, pDst, dstStep, roiSize );

  return ippStsNoErr;
} /* mfxiYCbCr420_8u_P2P3R() */
