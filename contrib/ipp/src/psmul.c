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

/*M*
//
//  Purpose: Functions of operation of multiplication
//
//  Contents:
//      mfxsMulC_16s_I
//      mfxsMul_16s_I
//
*M*/

#include "precomp.h"

#if !defined(__OWNS_H__)

  #include "owns.h"

#endif

#ifdef _IPP_C99
#include <complex.h>
#endif

#include "ps_anarith.h"


IPPFUN(IppStatus, mfxsMulC_16s_I, (Ipp16s val, Ipp16s* pSrcDst, int len))
{

  IPP_BAD_PTR1_RET(pSrcDst);
  IPP_BAD_SIZE_RET(len);

  if( val == 1 ) return ippStsNoErr;
  if( val == 0 ) return mfxsZero_16s(pSrcDst, len);

#if (_IPPLRB > _IPPLRB_PX)

  mfxownsMulC_16s_I_Sfs( val, pSrcDst, len, 0 );

#else

#if (_IPP64 >= _IPP64_I7)

  if( (IPP_INT_PTR(pSrcDst) & 1) == 0 ) {
    mfxownsMulC_16s(pSrcDst, val, pSrcDst, len);
  } /* if */
  else {

#endif

#if ( (_IPP == _IPP_PX) && (_IPP32E == _IPP32E_PX) && (_IPP64 >= _IPP64_PX) )

  { /* block */
    Ipp32s tmp;
    int    i;

    for( i = 0; i < len; i++ ) {
      tmp        = (Ipp32s)pSrcDst[i] * (Ipp32s)val;
      tmp        = IPP_MIN(tmp, IPP_MAX_16S);
      pSrcDst[i] = (Ipp16s)IPP_MAX(tmp, IPP_MIN_16S);
    } /* for */
  } /* block */

#else

  mfxownsMulC_16s_I(val, pSrcDst, len);

#endif

  ENDIF_I7

#endif

  return ippStsNoErr;
} /* mfxsMulC_16s_I */

IPPFUN(IppStatus, mfxsMul_16s_I, (const Ipp16s* __RESTRICT pSrc, Ipp16s* __RESTRICT pSrcDst, int len))
{

  IPP_BAD_PTR2_RET(pSrc, pSrcDst);
  IPP_BAD_SIZE_RET(len);

#if (_IPP64 >= _IPP64_I7)

  if( ((IPP_INT_PTR(pSrc) | IPP_INT_PTR(pSrcDst)) & 1) == 0 ) {
    mfxownsMul_16s(pSrcDst, pSrc, pSrcDst, len);
  } /* if */
  else {

#endif

#if ( (_IPP == _IPP_PX) && (_IPP32E == _IPP32E_PX) && (_IPP64 >= _IPP64_PX) && (_IPPLRB == _IPPLRB_PX) )

  { /* block */
    Ipp32s tmp;
    int i;

    for( i = 0; i < len; i++ ) {
      tmp        = (Ipp32s)pSrc[i] * (Ipp32s)pSrcDst[i];
      tmp        = IPP_MIN(tmp, IPP_MAX_16S);
      pSrcDst[i] = (Ipp16s)IPP_MAX(tmp, IPP_MIN_16S);
    } /* for */
  } /* block */

#else

  mfxownsMul_16s_I(pSrc, pSrcDst, len);

#endif

  ENDIF_I7

  return ippStsNoErr;
} /* mfxsMul_16s_I */

