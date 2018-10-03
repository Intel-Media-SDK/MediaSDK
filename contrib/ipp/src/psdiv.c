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

/* /////////////////////////////// "psdiv.c" /////////////////////////////////
//
//  Purpose: Div of Intel(R) Performance Primitives Libraries
//           for Signal Processing  (ipps)
//
*/

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       mfxsDiv
//
//  Purpose:    divide every element of the source vector by the scalar value
//              or by corresponding element of the second source vector
//  Arguments:
//    val               the divisor value
//    pSrc              pointer to the divisor source vector
//    pSrc1             pointer to the divisor source vector
//    pSrc2             pointer to the dividend source vector
//    pDst              pointer to the destination vector
//    pSrcDst           pointer to the source/destination vector
//    len               vector's length, number of items
//    scaleFactor       scale factor parameter value
//  Return:
//    ippStsNullPtrErr     pointer(s) to the data vector is NULL
//    ippStsSizeErr        length of the vector is less or equal zero
//    ippStsDivByZeroErr   the scalar divisor value is zero
//    ippStsDivByZero      Warning status if an element of divisor vector is
//                      zero. If the dividend is zero then result is
//                      NaN, if the dividend is not zero then result
//                      is Infinity with correspondent sign. The
//                      execution is not aborted. For the integer operation
//                      zero instead of NaN and the corresponding bound
//                      values instead of Infinity
//    ippStsNoErr          otherwise
//  Note:
//    DivC(v,X,Y)  :    Y[n] = X[n] / v
//    DivC(v,X)    :    X[n] = X[n] / v
//    Div(X,Y)     :    Y[n] = Y[n] / X[n]
//    Div(X,Y,Z)   :    Z[n] = Y[n] / X[n]
*/


#include "precomp.h"
#include "owns.h"

#if !defined(_CCODE) && !defined(_A6)
int mfxownippsDivCRev_16u(const Ipp16u* pSrc,Ipp16u val,
                            Ipp16u* pDst,int len);
#endif



IPPFUN(IppStatus, mfxsDivCRev_16u, (const Ipp16u* __RESTRICT pSrc, Ipp16u val,
       Ipp16u* __RESTRICT pDst, int len))
{
    IppStatus   result = ippStsNoErr;

    IPP_BAD_PTR2_RET(pSrc,pDst)
    IPP_BAD_SIZE_RET(len)

#if ( defined(_CCODE) && !(_IPPLRB >= _IPPLRB_B1) ) || defined(_A6) || (_IPP64>=_IPP64_I7)
#if (_IPP64>=_IPP64_I7)
    if ( !( ( IPP_UINT_PTR(pSrc) & 1 ) ||
            ( IPP_UINT_PTR(pDst) & 1 ) ) && val ) {
        if ( mfxownippsDivCRev_16u(pSrc, val, pDst, len) )
            result = ippStsDivByZero;
    } else
#endif

    {
        int     i;

        if ( val ) {
            Ipp32u  a = val << 16;
            Ipp32u  b, r, t;

            for ( i = 0; i < len; i++ ) {
                b = (Ipp32u)pSrc[i];
                if ( b ) {
                    r = a / b;
                    t = ( r & 0x10000 ) >> 16;
                    t += 0x7fff + t;
                    r += t;
                    r >>= 16;
                    pDst[i] = (Ipp16u)r;
                } else {
                    result = ippStsDivByZero;
                    pDst[i] = IPP_MAX_16U;
                }
            }
        } else {
            for ( i = 0; i < len; i++ ) {
                if ( pSrc[i] == 0 ) {
                    result = ippStsDivByZero;
                    break;
                }
            }
            mfxsZero_16s( (Ipp16s *)pDst, len );
        }
    }
#else
    if ( val ) {
#if !defined(_OWN_MT)
        if ( mfxownippsDivCRev_16u( pSrc, val, pDst, len ) )
            result = ippStsDivByZero;
#else
        if ( len < 2048 ) {
            if ( mfxownippsDivCRev_16u( pSrc, val, pDst, len ) )
                result = ippStsDivByZero;
         } else {
            if ( mfxownippsDivCRev_16u_omp( pSrc, val, pDst, len ) )
                result = ippStsDivByZero;
         }
#endif
    } else {
        int i;

        for ( i = 0; i < len; i++ ) {
            if ( pSrc[i] == 0 ) {
                result = ippStsDivByZero;
                break;
            }
        }
        mfxsZero_16s( (Ipp16s *)pDst, len );
    }
#endif
    return result;
}


/*===========================================================================*/
IPPFUN(IppStatus, mfxsDivCRev_16u_I, (Ipp16u val, Ipp16u* pSrcDst, int len))
{
    IPP_BAD_PTR1_RET(pSrcDst)
    IPP_BAD_SIZE_RET(len)
    return mfxsDivCRev_16u( pSrcDst, val, pSrcDst, len );
}

