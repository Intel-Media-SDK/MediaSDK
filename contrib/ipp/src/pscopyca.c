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
//  Purpose:    Copying a vector into second vector.
//              Filling a vector by 0.
//              Filling a vector by value.
//
//  Contents:   mfxsCopy_8u
//              mfxsCopy_16s
//              mfxsZero_16s
//              ************
//              mfxownsCopy_8u
//              mfxownsZero_8u
//              mfxownsSet_8u
//              mfxownsSet_16u
//              mfxownsSet_32s
//              mfxownsSet_64s
//              mfxownsSet_64sc
//              mfxownsPrefetchCopy_8u_A6
//
//
*M*/

#include "precomp.h"
#include "owns.h"
#include "pscopy.h"

#if ( _IPP32E >= _IPP32E_E9 )
/* #if ( _IPP32E <  _IPP32E_L9 ) */
#if 1
#define mfxownsCopy_8u_AVX mfxownsCopy_8u_E9
#endif
extern void mfxownsCopy_8u_repE9( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern Ipp8u* mfxownsZero_8u_E9( Ipp8u* dst, int len );
extern Ipp8u* mfxownsSet_8u_E9(  Ipp8u val, Ipp8u* dst, int len );
extern Ipp16u* mfxownsSet_16u_E9(  Ipp16u val, Ipp16u* dst, int len );
extern Ipp32s* mfxownsSet_32s_E9(  Ipp32s val, Ipp32s* dst, int len );
#define BOUND_NT_COPY  1048576
#define BOUND_NT_SET   2097152
#endif

#if ( _IPP >= _IPP_G9 )
/* #if ( _IPP <  _IPP_H9 ) */
#if 1
#define mfxownsCopy_8u_AVX mfxownsCopy_8u_G9
#endif
extern void mfxownsCopy_8u_repG9( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern Ipp8u* mfxownsZero_8u_G9( Ipp8u* dst, int len );
extern Ipp8u* mfxownsSet_8u_G9(  Ipp8u val, Ipp8u* dst, int len );
extern Ipp16u* mfxownsSet_16u_G9(  Ipp16u val, Ipp16u* dst, int len );
extern Ipp32s* mfxownsSet_32s_G9(  Ipp32s val, Ipp32s* dst, int len );
#define BOUND_NT_COPY  1048576
#define BOUND_NT_SET   2097152
#endif

#if !( (_IPP>=_IPP_M6) || (_IPP32E>=_IPP32E_M7) || (_IPP64>=_IPP64_I7) )
#define _CCODE
#endif

#if(( _IPP32E >= _IPP32E_M7 )||( _IPP >= _IPP_W7 )||(_IPPLRB >= _IPPLRB_B1))
  #define SET_OPT
#endif
#if ( _IPP32E >= _IPP32E_M7 )
  extern Ipp64sc* mfxownsSet_64sc_M7( Ipp64sc* pVal, Ipp64sc* pDst, int len );
  extern Ipp64sc* mfxownsSet_64sc( Ipp64sc val, Ipp64sc* pDst, int len ){
    return mfxownsSet_64sc_M7( &val, pDst, len );
  }
#endif

#if ( _IPP32E >= _IPP32E_M7 ) && ( _IPP32E < _IPP32E_E9 )
  extern Ipp8u* mfxownsSet_8u_M7( Ipp8u* pVal, Ipp8u* pDst, int len );
  extern Ipp16u* mfxownsSet_16u_M7( Ipp16u* pVal, Ipp16u* pDst, int len );
  extern Ipp32s* mfxownsSet_32s_M7( Ipp32s* pVal, Ipp32s* pDst, int len );

  extern Ipp32s* mfxownsSet_32s( Ipp32s val, Ipp32s* pDst, int len ){
    return mfxownsSet_32s_M7( &val, pDst, len );
  }
#endif

#if ( _IPP32E >= _IPP32E_M7 ) && ( _IPP32E < _IPP32E_L9 )
  extern Ipp64s* mfxownsSet_64s_M7( Ipp64s* pVal, Ipp64s* pDst, int len );
  extern Ipp64s* mfxownsSet_64s( Ipp64s val, Ipp64s* pDst, int len ){
    return mfxownsSet_64s_M7( &val, pDst, len );
  }
#endif

#if (_IPP >= _IPP_G9) || (_IPP32E >= _IPP32E_E9)
extern Ipp8u* mfxownsCopy_8u_AVX( const Ipp8u* pSrc, Ipp8u* pDst, int len );
#endif


#if _IPP_ARCH != _IPP_ARCH_XSC
/*****************************************************************/
IPPFUN ( IppStatus, mfxsCopy_8u,
                    ( const Ipp8u* pSrc, Ipp8u* pDst, int len )) {

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( len )

#ifdef _A6
    if( len < 8192 ) {
        mfxownsCopy_8u( pSrc, pDst, len );
        return ippStsNoErr;
    }
    if(len < 16384) {
        mfxownsCopy_8u_A6( pSrc, pDst, len );
        return ippStsNoErr;
    }
    mfxownsPrefetchCopy_8u_A6( pSrc, pDst, len );
#else
  #if ( _IPP32E >= _IPP32E_E9 )
    if ( len < 32768 ) {
        mfxownsCopy_8u_AVX( pSrc, pDst, len );
    } else {
        mfxownsCopy_8u_repE9( pSrc, pDst, len );
    }
/*
old code by A.Solntsev
    if( len <= BOUND_NT_COPY ) {
      mfxownsCopy_8u_E9( pSrc, pDst, len );
    } else {
      mfxownsCopy_8u( pSrc, pDst, len );
    }
*/
  #elif ( _IPP >= _IPP_G9 )
    if ( len < 32768 ) {
        mfxownsCopy_8u_AVX( pSrc, pDst, len );
    } else {
        mfxownsCopy_8u_repG9( pSrc, pDst, len );
    }
/*
old code by A.Solntsev
    if( len <= BOUND_NT_COPY ) {
      mfxownsCopy_8u_G9( pSrc, pDst, len );
    } else {
      mfxownsCopy_8u( pSrc, pDst, len );
    }
*/
  #else
    mfxownsCopy_8u( pSrc, pDst, len );
  #endif
#endif

    return ippStsNoErr;
}

/********************************************************************/
IPPFUN ( IppStatus, mfxsCopy_16s,
                    ( const Ipp16s* pSrc, Ipp16s* pDst, int len )) {
    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( len )

#ifdef _A6
    if( len < 4096 ) {
        mfxownsCopy_8u( (Ipp8u*)pSrc, (Ipp8u*)pDst, len*2 );
        return ippStsNoErr;
    }
    if(len < 8192) {
        mfxownsCopy_8u_A6( (Ipp8u*)pSrc, (Ipp8u*)pDst, len*2 );
        return ippStsNoErr;
    }
    mfxownsPrefetchCopy_8u_A6( (Ipp8u*)pSrc, (Ipp8u*)pDst, len*2 );
#else
  #if ( _IPP >= _IPP_G9 ) || ( _IPP32E >= _IPP32E_E9 )
    for (; len > 0x3fffffff; len -= 0x3ffffff0) {
        mfxsCopy_8u((const Ipp8u*)pSrc, (Ipp8u*)pDst, 0x7fffffe0);
        pSrc += 0x3ffffff0;
        pDst += 0x3ffffff0;
    }
    if ( len )
        mfxsCopy_8u((const Ipp8u*)pSrc, (Ipp8u*) pDst, len+len);
  #else
    mfxownsCopy_8u( (Ipp8u*)pSrc, (Ipp8u*)pDst, len*2 );
  #endif
#endif
    return ippStsNoErr;

}

/*****************************************************************/
IPPFUN ( IppStatus, mfxsZero_8u, ( Ipp8u* pDst, int len )) {

    IPP_BAD_PTR1_RET( pDst )
    IPP_BAD_SIZE_RET( len )

#if defined (_A6)
    if( len < 20480 ) {
        mfxownsZero_8u( pDst, len );
    } else {
        mfxownsZero_8u_A6( pDst, len );
    }
#else
  #if ( _IPP32E >= _IPP32E_E9 )
    if( len <= BOUND_NT_SET ) {
      mfxownsZero_8u_E9( pDst, len );
    } else {
      mfxownsZero_8u( pDst, len );
    }
  #elif ( _IPP >= _IPP_G9 )
    if( len <= BOUND_NT_SET ) {
      mfxownsZero_8u_G9( pDst, len );
    } else {
      mfxownsZero_8u( pDst, len );
    }
  #else
    mfxownsZero_8u( pDst, len );
  #endif
#endif

    return ippStsNoErr;
}

/*****************************************************************/
IPPFUN ( IppStatus, mfxsZero_16s, ( Ipp16s* pDst, int len )) {

    IPP_BAD_PTR1_RET( pDst )
    IPP_BAD_SIZE_RET( len )

#if defined (_A6)
    if( len < 10240 ) {
        mfxownsZero_8u( (Ipp8u*)pDst, len*2 );
    } else {
        mfxownsZero_8u_A6( (Ipp8u*)pDst, len*2 );
    }
#else
  #if ( _IPP32E >= _IPP32E_E9 )
    if( len*2 <= BOUND_NT_SET ) {
      mfxownsZero_8u_E9( (Ipp8u*)pDst, len*2 );
    } else {
      mfxownsZero_8u( (Ipp8u*)pDst, len*2 );
    }
  #elif ( _IPP >= _IPP_G9 )
    if( len*2 <= BOUND_NT_SET ) {
      mfxownsZero_8u_G9( (Ipp8u*)pDst, len*2 );
    } else {
      mfxownsZero_8u( (Ipp8u*)pDst, len*2 );
    }
  #else
    mfxownsZero_8u( (Ipp8u*)pDst, len*2 );
  #endif
#endif

    return ippStsNoErr;
}


#endif // _IPP_ARCH != _IPP_ARCH_XSC





#if defined (_A6)
/********************************* Return: pDst. *********************************/
extern Ipp8u* mfxownsPrefetchCopy_8u_A6( const Ipp8u *pSrc, Ipp8u *pDst, int len ) {

    int length = len, length_, n;
    const Ipp8u *src1 = pSrc;
    Ipp8u *dst1 = pDst;

    if( length <= 0 ) return pDst;

    while( IPP_INT_PTR( dst1 ) & 0x7 ) {
        *dst1 = *src1;
        src1++, dst1++, length--;
       if( !length ) return pDst;
    }

    if( length < 20480 ) {
        mfxownsCopy_8u_A6( src1, dst1, length );
        return pDst;
    }

    length_ = (4096 - (IPP_INT_PTR( src1 ) & 0xff8)) & 0xfff;
    if( length_ ) {
        mfxownsCopy_8u_A6(src1, dst1, length_);
        src1 += length_, dst1 += length_;
        length -= length_;
    }

    length -= (length_ =  length & 0xfff);
    length /= 4096;
    for(n=0; n < length; n++) {
        mfxownsPrefetchnta_A6(src1, 4096);
        mfxownsCopy_8u_A6(src1, dst1, 4096);
        src1 += 4096;
        dst1 += 4096;
    }

    if( length_ ) {
        mfxownsCopy_8u_A6(src1, dst1, length_);
    }

    return pDst;
}
#endif
