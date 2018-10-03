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
//     Purpose : IPPI Color Space Conversion
//
*M*/

#include "precomp.h"
#include <emmintrin.h> // __m128 etc
#include <tmmintrin.h> // _mm_shuffle_epi8
#include "ownj.h"

#if ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 )

/*
     Ipp32s iRY  = (Ipp32s)(0.299f   * 65536.f/4. + 0.5f);
     Ipp32s iGY  = (Ipp32s)(0.587f   * 65536.f/4. + 0.5f);
     Ipp32s iBY  = (Ipp32s)(0.114f   * 65536.f/4. + 0.5f);
     Ipp32s iRu  = (Ipp32s)(0.16874f * 65536.f/4. + 0.5f);
     Ipp32s iGu  = (Ipp32s)(0.33126f * 65536.f/4. + 0.5f);
     Ipp32s iBu  = (Ipp32s)(0.5f     * 65536.f/4. + 0.5f);
     Ipp32s iGv  = (Ipp32s)(0.41869f * 65536.f/4. + 0.5f);
     Ipp32s iBv  = (Ipp32s)(0.08131f * 65536.f/4. + 0.5f);
*/
#define iRY  0x00001323
#define iGY  0x00002591
#define iBY  0x0000074c
#define iRu  0x00000acd
#define iGu  0x00001533
#define iBu  0x00002000
#define iGv  0x00001acc
#define iBv  0x00000534


/*
    Lib = V8 U8
    Caller = mfxiRGBToYCbCr_JPEG_8u_C4P3R
*/
extern void mfxownRGBToYCbCr_JPEG_8u_C4P3R(
                                       const Ipp8u* pBGR, int bgrStep, Ipp8u* pYCC[3], int yccStep, IppiSize roiSize)
{
    int h,w,ttt=0;
    int width = roiSize.width;
    int width8 = roiSize.width &  0x08;
    int width16= roiSize.width & ~0x0f;
    Ipp8u* pDstU = pYCC[1];
    Ipp8u* pDstV = pYCC[2];
    int height = roiSize.height;
    __m128i eZero = _mm_setzero_si128();
    __m128i kYr = _mm_set1_epi16( iRY );
    __m128i kYg = _mm_set1_epi16( iGY );
    __m128i kYb = _mm_set1_epi16( iBY );
    __m128i k16  = _mm_set1_epi32(0x00200020);
    __m128i k128 = _mm_set1_epi32(0x01010101<<4);
    __m128i kRu = _mm_set1_epi16( iRu );
    __m128i kGu = _mm_set1_epi16( iGu );
    __m128i kBu = _mm_set1_epi16( iBu );
    __m128i kGv = _mm_set1_epi16( iGv );
    __m128i kBv = _mm_set1_epi16( iBv );
    const __m128i sHf  = _mm_set_epi32( 0x0f0b0703,0x0e0a0602,0x0d090501,0x0c080400 );
    if(!( ((intptr_t)pBGR&0x0f)||((intptr_t)pYCC[0]&0x0f)||((intptr_t)pYCC[1]&0x0f)||((intptr_t)pYCC[2]&0x0f)||(bgrStep&0x0f)||(yccStep&0x0f) ) )
        ttt=1;

    for( h = 0; h < height; h ++ )
    {
        const Ipp8u* src;
        Ipp8u* dsty,*dstu,*dstv;
        src  = pBGR   + h * bgrStep;
        dsty = pYCC[0]+ h * yccStep;
        dstu = pDstU  + h * yccStep;
        dstv = pDstV  + h * yccStep;
        if(ttt)
        {
            for(w = 0; w < width16; w += 16 )
            {
                __m128i t0, t1, t2, t3, eR, eB, eG, eY, eY1, eY0, eU, eV;
                __m128i eR1, eB1, eG1;
                __m128i mB, mG, mR;
                t0 = _mm_load_si128(((__m128i*) src ));       /* |A|b3|g3|r3|A|b2|g2|r2|A|b1|g1|r1|A|b0|g0|r0| */
                t1 = _mm_load_si128(((__m128i*)(src + 16)));  /* |A|r7|g7|b7|A|b6|g6|r6|A|b5|g5|r5|A|b4|g4|r4| */
                t2 = _mm_load_si128(((__m128i*)(src + 32)));  /* |A|rb|gb|bb|A|ba|ga|ra|A|b9|g9|r9|A|b8|g8|r8| */
                t3 = _mm_load_si128(((__m128i*)(src + 48)));
                t0 = _mm_shuffle_epi8( t0, sHf );  /* |B|G|R(3-0)| */
                t1 = _mm_shuffle_epi8( t1, sHf );  /* |B|G|R(7-4)| */
                t2 = _mm_shuffle_epi8( t2, sHf );  /* |B|G|R(a-8)| */
                t3 = _mm_shuffle_epi8( t3, sHf );  /* |B|G|R(f-b)| */
                src += 64;
                eV = mR = _mm_unpacklo_epi32( t0, t1);  /* |G|G|R(7-4)|R(3-0)|           */
                mG = _mm_unpacklo_epi32( t2, t3);       /* |G|G|R(f-b)|R(a-8)|           */
                mR = _mm_unpacklo_epi64( mR, mG);       /* |R(f-b)|R(a-8)|R(7-4)|R(3-0)| */
                mG = _mm_unpackhi_epi64( eV, mG);       /* |G(f-b)|G(a-8)|G(7-4)|G(3-0)| */
                mB = _mm_unpackhi_epi32( t0, t1);       /* |A|A|B(7-4)|B(3-0)|           */
                t2 = _mm_unpackhi_epi32( t2, t3);       /* |A|A|B(f-b)|B(a-8)|           */
                mB = _mm_unpacklo_epi64( mB, t2);       /* |R(f-b)|R(a-8)|R(7-4)|R(3-0)| */
                eR = _mm_unpacklo_epi8( eZero, mR );
                eG = _mm_unpacklo_epi8( eZero, mG );
                eB = _mm_unpacklo_epi8( eZero, mB );
                /* calculation */
                eY = _mm_adds_epu16( _mm_mulhi_epu16( eR, kYr), _mm_mulhi_epu16( eG, kYg));
                eY = _mm_adds_epu16( eY, _mm_mulhi_epu16( eB, kYb));
                eY0= _mm_srli_epi16(_mm_adds_epu16( eY, k16 ),6);
                eR1 = _mm_unpackhi_epi8( eZero, mR );
                eG1 = _mm_unpackhi_epi8( eZero, mG );
                eB1 = _mm_unpackhi_epi8( eZero, mB );
                /* calculation */
                eY = _mm_adds_epu16( _mm_mulhi_epu16( eR1,kYr), _mm_mulhi_epu16( eG1,kYg));
                eY = _mm_adds_epu16( eY, _mm_mulhi_epu16( eB1,kYb));
                eY1= _mm_srli_epi16(_mm_adds_epu16( eY, k16 ), 6);
                eY0 = _mm_packus_epi16( eY0, eY1 );
                _mm_store_si128((__m128i*)dsty, eY0 );
                eR = _mm_srli_epi16( eR,  1 );
                eG = _mm_srli_epi16( eG,  1 );
                eB = _mm_srli_epi16( eB,  1 );
                eR1= _mm_srli_epi16( eR1, 1 );
                eG1= _mm_srli_epi16( eG1, 1 );
                eB1= _mm_srli_epi16( eB1, 1 );
                dsty += 16;
                /* Cb */
                eY = _mm_add_epi16( _mm_mulhi_epu16( eR, kRu), _mm_mulhi_epu16( eG,kGu));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eB, kBu), eY);
                eU = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
                eY = _mm_add_epi16( _mm_mulhi_epu16( eR1, kRu), _mm_mulhi_epu16( eG1,kGu));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eB1, kBu), eY);
                eY0 = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
                _mm_store_si128((__m128i*)dstu,_mm_packus_epi16( eU, eY0 ));
                dstu += 16;
                /* Cr */
                eY = _mm_add_epi16( _mm_mulhi_epu16( eG, kGv), _mm_mulhi_epu16( eB, kBv));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eR,kBu), eY);
                eV = _mm_srli_epi16(_mm_add_epi16( eY, k128), 5 );
                eY = _mm_add_epi16( _mm_mulhi_epu16( eG1,kGv), _mm_mulhi_epu16( eB1, kBv));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eR1,kBu), eY);
                eY0 = _mm_srli_epi16(_mm_add_epi16( eY, k128), 5 );
                _mm_store_si128((__m128i*)dstv,_mm_packus_epi16( eV, eY0 ));
                dstv += 16;
            }
        } else
        {
            for(w = 0; w < width16; w += 16 )
            {
                __m128i t0, t1, t2, t3, eR, eB, eG, eY, eY1, eY0, eU, eV;
                __m128i eR1, eB1, eG1;
                __m128i mB, mG, mR;
                t0 = _mm_loadu_si128(((__m128i*) src ));       /* |A|b3|g3|r3|A|b2|g2|r2|A|b1|g1|r1|A|b0|g0|r0| */
                t1 = _mm_loadu_si128(((__m128i*)(src + 16)));  /* |A|r7|g7|b7|A|b6|g6|r6|A|b5|g5|r5|A|b4|g4|r4| */
                t2 = _mm_loadu_si128(((__m128i*)(src + 32)));  /* |A|rb|gb|bb|A|ba|ga|ra|A|b9|g9|r9|A|b8|g8|r8| */
                t3 = _mm_loadu_si128(((__m128i*)(src + 48)));
                t0 = _mm_shuffle_epi8( t0, sHf );  /* |B|G|R(3-0)| */
                t1 = _mm_shuffle_epi8( t1, sHf );  /* |B|G|R(7-4)| */
                t2 = _mm_shuffle_epi8( t2, sHf );  /* |B|G|R(a-8)| */
                t3 = _mm_shuffle_epi8( t3, sHf );  /* |B|G|R(f-b)| */
                src += 64;
                eV = mR = _mm_unpacklo_epi32( t0, t1);  /* |G|G|R(7-4)|R(3-0)|           */
                mG = _mm_unpacklo_epi32( t2, t3);       /* |G|G|R(f-b)|R(a-8)|           */
                mR = _mm_unpacklo_epi64( mR, mG);       /* |R(f-b)|R(a-8)|R(7-4)|R(3-0)| */
                mG = _mm_unpackhi_epi64( eV, mG);       /* |G(f-b)|G(a-8)|G(7-4)|G(3-0)| */
                mB = _mm_unpackhi_epi32( t0, t1);       /* |A|A|B(7-4)|B(3-0)|           */
                t2 = _mm_unpackhi_epi32( t2, t3);       /* |A|A|B(f-b)|B(a-8)|           */
                mB = _mm_unpacklo_epi64( mB, t2);       /* |R(f-b)|R(a-8)|R(7-4)|R(3-0)| */
                eR = _mm_unpacklo_epi8( eZero, mR );
                eG = _mm_unpacklo_epi8( eZero, mG );
                eB = _mm_unpacklo_epi8( eZero, mB );
                /* calculation */
                eY = _mm_adds_epu16( _mm_mulhi_epu16( eR, kYr), _mm_mulhi_epu16( eG, kYg));
                eY = _mm_adds_epu16( eY, _mm_mulhi_epu16( eB, kYb));
                eY0= _mm_srli_epi16(_mm_adds_epu16( eY, k16 ),6);
                eR1 = _mm_unpackhi_epi8( eZero, mR );
                eG1 = _mm_unpackhi_epi8( eZero, mG );
                eB1 = _mm_unpackhi_epi8( eZero, mB );
                /* calculation */
                eY = _mm_adds_epu16( _mm_mulhi_epu16( eR1,kYr), _mm_mulhi_epu16( eG1,kYg));
                eY = _mm_adds_epu16( eY, _mm_mulhi_epu16( eB1,kYb));
                eY1= _mm_srli_epi16(_mm_adds_epu16( eY, k16 ), 6);
                eY0 = _mm_packus_epi16( eY0, eY1 );
                _mm_storeu_si128((__m128i*)dsty, eY0 );
                eR = _mm_srli_epi16( eR,  1 );
                eG = _mm_srli_epi16( eG,  1 );
                eB = _mm_srli_epi16( eB,  1 );
                eR1= _mm_srli_epi16( eR1, 1 );
                eG1= _mm_srli_epi16( eG1, 1 );
                eB1= _mm_srli_epi16( eB1, 1 );
                dsty += 16;
                /* Cb */
                eY = _mm_add_epi16( _mm_mulhi_epu16( eR, kRu), _mm_mulhi_epu16( eG,kGu));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eB, kBu), eY);
                eU = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
                eY = _mm_add_epi16( _mm_mulhi_epu16( eR1, kRu), _mm_mulhi_epu16( eG1,kGu));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eB1, kBu), eY);
                eY0 = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
                _mm_storeu_si128((__m128i*)dstu,_mm_packus_epi16( eU, eY0 ));
                dstu += 16;
                /* Cr */
                eY = _mm_add_epi16( _mm_mulhi_epu16( eG, kGv), _mm_mulhi_epu16( eB, kBv));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eR,kBu), eY);
                eV = _mm_srli_epi16(_mm_add_epi16( eY, k128), 5 );
                eY = _mm_add_epi16( _mm_mulhi_epu16( eG1,kGv), _mm_mulhi_epu16( eB1, kBv));
                eY = _mm_sub_epi16( _mm_mulhi_epu16( eR1,kBu), eY);
                eY0 = _mm_srli_epi16(_mm_add_epi16( eY, k128), 5 );
                _mm_storeu_si128((__m128i*)dstv,_mm_packus_epi16( eV, eY0 ));
                dstv += 16;
            }
        }
        if(width8)
        {
            __m128i t0, t1, eR, eB, eG, eY, eY0, eU, eV;
            __m128i mB, mG, mR;
            w+=8;
            t0  = _mm_unpacklo_epi64(_mm_loadl_epi64(((__m128i*) src      )),_mm_loadl_epi64(((__m128i*)(src + 8))));
            t1  = _mm_unpacklo_epi64(_mm_loadl_epi64(((__m128i*)(src + 16))),_mm_loadl_epi64(((__m128i*)(src +24))));
            src += 32;
            t0 = _mm_shuffle_epi8( t0, sHf );  /* |B|G|R(3-0)| */
            t1 = _mm_shuffle_epi8( t1, sHf );  /* |B|G|R(7-4)| */

            mR = _mm_unpacklo_epi32( t0, t1);  /* |G|G|R(7-4)|R(3-0)|           */
            mG = _mm_unpackhi_epi64( mR, mR);       /* |G(f-b)|G(a-8)|G(7-4)|G(3-0)| */
            mB = _mm_unpackhi_epi32( t0, t1);       /* |A|A|B(7-4)|B(3-0)|           */

            eR = _mm_unpacklo_epi8( eZero, mR );
            eG = _mm_unpacklo_epi8( eZero, mG );
            eB = _mm_unpacklo_epi8( eZero, mB );
            /* calculation */
            eY = _mm_adds_epu16( _mm_mulhi_epu16( eR,kYr), _mm_mulhi_epu16( eG,kYg));
            eY = _mm_adds_epu16( eY, _mm_mulhi_epu16( eB,kYb));
            eY0= _mm_srli_epi16(_mm_adds_epu16( eY, k16 ),6);
            eY0 = _mm_packus_epi16( eY0, eY0 );
            _mm_storel_epi64((__m128i*)dsty, eY0 );
            dsty += 8;
            eR = _mm_srli_epi16( eR,  1 );
            eG = _mm_srli_epi16( eG,  1 );
            eB = _mm_srli_epi16( eB,  1 );
            /* Cb */
            eY = _mm_add_epi16( _mm_mulhi_epu16( eR, kRu), _mm_mulhi_epu16( eG,kGu));
            eY = _mm_sub_epi16( _mm_mulhi_epu16( eB, kBu), eY);
            eU = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
            /* Cr */
            eY = _mm_adds_epu16( _mm_mulhi_epu16( eG,kGv), _mm_mulhi_epu16( eB,kBv));
            eY = _mm_sub_epi16( _mm_mulhi_epu16( eR,kBu), eY);
            eV = _mm_srli_epi16(_mm_adds_epi16( eY, k128), 5 );
            _mm_storel_epi64((__m128i*)dstu, _mm_packus_epi16( eU, eU ) );
            _mm_storel_epi64((__m128i*)dstv, _mm_packus_epi16( eV, eV ) );
            dstu += 8;
            dstv += 8;
        }
        for( ; w < width; w ++ )
        {
            int g,r,b;
            b = src[2];g = src[1];r = src[0];
            src += 4;
            *dsty++ = ( Ipp8u )(( iRY * r + iGY * g + iBY * b + 0x002000) >> 14 );
            *dstu++ = ( Ipp8u )((-iRu * r - iGu * g + iBu * b + 0x201000) >> 14 ); /* Cb */
            *dstv++ = ( Ipp8u )(( iBu * r - iGv * g - iBv * b + 0x201000) >> 14 ); /* Cr */
        }
    }
}




/* YCrCrToRGB */

#define kRCr  0x00002cdd
#define kGCr  0x000016da
#define kGCb  0x00000b03
#define kBCb  0x000038b4
#define kR    0x00000b37
#define kG    0x00000877
#define kB    0x00000e2d


/*
    Lib = V8 U8
    Caller = mfxiYCbCrToBGR_JPEG_8u_P3C4R
*/
extern void mfxownYCbCrToBGR_JPEG_8u_P3C4R(
                                       const Ipp8u* pYCC[3],int yccStep,Ipp8u* pBGR,int bgrStep,IppiSize roiSize, Ipp8u aval,int orger)
{
    int h,ttt=0;
    int width = roiSize.width;
    int width16 = width & ~0x0f;int width8  = width & 0x08;
    int last  = width & 0x07;
    const __m128i iRCr = _mm_set1_epi16( kRCr );
    const __m128i iGCr = _mm_set1_epi16( kGCr );
    const __m128i iGCb = _mm_set1_epi16( kGCb );
    const __m128i iBCb = _mm_set1_epi16( kBCb );
    const __m128i iR   = _mm_set1_epi16( kR );
    const __m128i iG   = _mm_set1_epi16( kG );
    const __m128i iB   = _mm_set1_epi16( kB );
    __m128i kOKR = _mm_set1_epi32( 0x00080008);
    __m128i eZero = _mm_setzero_si128();
    __m128i eAval = _mm_set1_epi8( aval);
    __m128i sHf  = _mm_set_epi32( 0x0f0b0703,0x0e0a0602,0x0d090501,0x0c080400 ); //RGB
    if(orger)
        sHf  = _mm_set_epi32( 0x0f03070b,0x0e02060a,0x0d010509,0x0c000408 ); //BGR
    if(!( ((intptr_t)pBGR&0x0f)||((intptr_t)pYCC[0]&0x0f)||((intptr_t)pYCC[1]&0x0f)||((intptr_t)pYCC[2]&0x0f)||(bgrStep&0x0f)||(yccStep&0x0f) ) )
        ttt=1;

    for(h = 0; h < roiSize.height; h++ )
    {
        int w=0;
        const Ipp8u* srcy = pYCC[0] + h * yccStep;
        const Ipp8u* srcu = pYCC[1] + h * yccStep;
        const Ipp8u* srcv = pYCC[2] + h * yccStep;
        Ipp8u* dst  = pBGR    + h * bgrStep;
        if(ttt)
        {
            for(w = 0; w < width16; w += 16 )
            {
                __m128i t0, t1, t2;
                __m128i eR0, eR1, eB0, eB1, eG0, eG1;
                __m128i eY0, eY1;
                __m128i tU, tV;
                __m128i eU, eV;
                t0  = _mm_load_si128((__m128i*)srcy); /* |y15 - y0| */
                srcy += 16;
                tU  = _mm_load_si128((__m128i*)srcu); /* |u15 - u0| */
                srcu += 16;
                tV  = _mm_load_si128((__m128i*)srcv); /* |v15 - v0| */
                srcv += 16;
                eY0 = _mm_slli_epi16(              _mm_unpacklo_epi8( t0, eZero),4); /* | y7 - y0| */
                eY1 = _mm_slli_epi16(              _mm_unpackhi_epi8( t0, eZero),4); /* |y15 - y8| */
                eU  = _mm_slli_epi16((_mm_unpacklo_epi8( tU, eZero)),7); /* | u7 - u0| */
                eV  = _mm_slli_epi16((_mm_unpacklo_epi8( tV, eZero)),7); /* | v7 - v0| */

                eR0 = _mm_mulhi_epi16(  eV, iRCr );
                eR0 = _mm_adds_epi16 ( eR0,  eY0 );
                eR0 = _mm_subs_epi16 ( eR0,  iR  );
                eR0 = _mm_adds_epi16 ( eR0, kOKR );
                eR0 = _mm_srai_epi16 ( eR0,    4 );

                tV  = _mm_slli_epi16((_mm_unpackhi_epi8( tV, eZero)),7); /* | v7 - v0| */
                eR1 = _mm_mulhi_epi16(  tV, iRCr );
                eR1 = _mm_adds_epi16 ( eR1,  eY1 );
                eR1 = _mm_subs_epi16 ( eR1,  iR  );
                eR1 = _mm_adds_epi16 ( eR1, kOKR );
                eR1 = _mm_srai_epi16 ( eR1,    4 );
                eR0 = _mm_packus_epi16(eR0,  eR1 );

                tU  = _mm_slli_epi16((_mm_unpackhi_epi8( tU, eZero)),7); /* | v7 - v0| */
                eB0 = _mm_mulhi_epi16(  eU, iBCb );
                eB0 = _mm_adds_epi16 ( eB0,  eY0 );
                eB0 = _mm_subs_epi16 ( eB0,  iB  );
                eB0 = _mm_adds_epi16 ( eB0, kOKR );
                eB0 = _mm_srai_epi16 ( eB0,    4 );
                eB1 = _mm_mulhi_epi16(  tU, iBCb );
                eB1 = _mm_adds_epi16 ( eB1,  eY1 );
                eB1 = _mm_subs_epi16 ( eB1,  iB  );
                eB1 = _mm_adds_epi16 ( eB1, kOKR );
                eB1 = _mm_srai_epi16 ( eB1,    4 );
                eB0 = _mm_packus_epi16(eB0,  eB1 );

                eG0 = _mm_mulhi_epi16(  eU, iGCb );
                eG0 = _mm_adds_epi16 ( eG0, _mm_mulhi_epi16( eV, iGCr ) );
                eY0 = _mm_adds_epi16 ( eY0,  iG  );
                eY0 = _mm_subs_epi16 ( eY0,  eG0 );
                eY0 = _mm_adds_epi16 ( eY0, kOKR );
                eY0 = _mm_srai_epi16 ( eY0,    4 );
                eG1 = _mm_mulhi_epi16(  tU, iGCb );
                eG1 = _mm_adds_epi16 ( eG1, _mm_mulhi_epi16( tV, iGCr ) );
                eY1 = _mm_adds_epi16 ( eY1,  iG  );
                eY1 = _mm_subs_epi16 ( eY1,  eG1 );
                eY1 = _mm_adds_epi16 ( eY1, kOKR );
                eY1 = _mm_srai_epi16 ( eY1,    4 );
                eY0 = _mm_packus_epi16(eY0,  eY1 );

                t0  = _mm_unpacklo_epi32( eR0, eY0  );   //|.G1|.R1|.G0|.R0|
                t1  = _mm_unpacklo_epi32( eB0, eAval);   //|.A |.B1|.A |.B0|

                t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_store_si128( (__m128i*)dst, t2);
                t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_store_si128( (__m128i*)( dst+16 ), t2);
                t0  = _mm_unpackhi_epi32( eR0, eY0  );   //|.G3|.R3|.G2|.R2|
                t1  = _mm_unpackhi_epi32( eB0, eAval);   //|.A |.B3|.A |.B2|
                t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_store_si128( (__m128i*)( dst+32 ), t2);
                t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_store_si128( (__m128i*)( dst+48 ), t2);
                dst += 64;
            }
        } else
        {
            for(w = 0; w < width16; w += 16 )
            {
                __m128i t0, t1, t2;
                __m128i eR0, eR1, eB0, eB1, eG0, eG1;
                __m128i eY0, eY1;
                __m128i tU, tV;
                __m128i eU, eV;
                t0  = _mm_loadu_si128((__m128i*)srcy); /* |y15 - y0| */
                srcy += 16;
                tU  = _mm_loadu_si128((__m128i*)srcu); /* |u15 - u0| */
                srcu += 16;
                tV  = _mm_loadu_si128((__m128i*)srcv); /* |v15 - v0| */
                srcv += 16;
                eY0 = _mm_slli_epi16(              _mm_unpacklo_epi8( t0, eZero),4); /* | y7 - y0| */
                eY1 = _mm_slli_epi16(              _mm_unpackhi_epi8( t0, eZero),4); /* |y15 - y8| */
                eU  = _mm_slli_epi16((_mm_unpacklo_epi8( tU, eZero)),7); /* | u7 - u0| */
                eV  = _mm_slli_epi16((_mm_unpacklo_epi8( tV, eZero)),7); /* | v7 - v0| */

                eR0 = _mm_mulhi_epi16(  eV, iRCr );
                eR0 = _mm_adds_epi16 ( eR0,  eY0 );
                eR0 = _mm_subs_epi16 ( eR0,  iR  );
                eR0 = _mm_adds_epi16 ( eR0, kOKR );
                eR0 = _mm_srai_epi16 ( eR0,    4 );

                tV  = _mm_slli_epi16((_mm_unpackhi_epi8( tV, eZero)),7); /* | v7 - v0| */
                eR1 = _mm_mulhi_epi16(  tV, iRCr );
                eR1 = _mm_adds_epi16 ( eR1,  eY1 );
                eR1 = _mm_subs_epi16 ( eR1,  iR  );
                eR1 = _mm_adds_epi16 ( eR1, kOKR );
                eR1 = _mm_srai_epi16 ( eR1,    4 );
                eR0 = _mm_packus_epi16(eR0,  eR1 );

                tU  = _mm_slli_epi16((_mm_unpackhi_epi8( tU, eZero)),7); /* | v7 - v0| */
                eB0 = _mm_mulhi_epi16(  eU, iBCb );
                eB0 = _mm_adds_epi16 ( eB0,  eY0 );
                eB0 = _mm_subs_epi16 ( eB0,  iB  );
                eB0 = _mm_adds_epi16 ( eB0, kOKR );
                eB0 = _mm_srai_epi16 ( eB0,    4 );
                eB1 = _mm_mulhi_epi16(  tU, iBCb );
                eB1 = _mm_adds_epi16 ( eB1,  eY1 );
                eB1 = _mm_subs_epi16 ( eB1,  iB  );
                eB1 = _mm_adds_epi16 ( eB1, kOKR );
                eB1 = _mm_srai_epi16 ( eB1,    4 );
                eB0 = _mm_packus_epi16(eB0,  eB1 );

                eG0 = _mm_mulhi_epi16(  eU, iGCb );
                eG0 = _mm_adds_epi16 ( eG0, _mm_mulhi_epi16( eV, iGCr ) );
                eY0 = _mm_adds_epi16 ( eY0,  iG  );
                eY0 = _mm_subs_epi16 ( eY0,  eG0 );
                eY0 = _mm_adds_epi16 ( eY0, kOKR );
                eY0 = _mm_srai_epi16 ( eY0,    4 );
                eG1 = _mm_mulhi_epi16(  tU, iGCb );
                eG1 = _mm_adds_epi16 ( eG1, _mm_mulhi_epi16( tV, iGCr ) );
                eY1 = _mm_adds_epi16 ( eY1,  iG  );
                eY1 = _mm_subs_epi16 ( eY1,  eG1 );
                eY1 = _mm_adds_epi16 ( eY1, kOKR );
                eY1 = _mm_srai_epi16 ( eY1,    4 );
                eY0 = _mm_packus_epi16(eY0,  eY1 );

                t0  = _mm_unpacklo_epi32( eR0, eY0  );   //|.G1|.R1|.G0|.R0|
                t1  = _mm_unpacklo_epi32( eB0, eAval);   //|.A |.B1|.A |.B0|

                t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_storeu_si128( (__m128i*)dst, t2);
                t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_storeu_si128( (__m128i*)( dst+16 ), t2);
                t0  = _mm_unpackhi_epi32( eR0, eY0  );   //|.G3|.R3|.G2|.R2|
                t1  = _mm_unpackhi_epi32( eB0, eAval);   //|.A |.B3|.A |.B2|
                t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_storeu_si128( (__m128i*)( dst+32 ), t2);
                t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
                _mm_storeu_si128( (__m128i*)( dst+48 ), t2);
                dst += 64;
            }
        }
        if( width8 )
        {
            __m128i t0, t1, t2;
            __m128i eR0, eB0, eG0;
            __m128i eY0;
            __m128i tU, tV;
            __m128i eU, eV;
            t0  = _mm_loadl_epi64((__m128i*)srcy); /* |y15 - y0| */
            srcy += 8;
            tU  = _mm_loadl_epi64((__m128i*)srcu); /* |u15 - u0| */
            srcu += 8;
            tV  = _mm_loadl_epi64((__m128i*)srcv); /* |v15 - v0| */
            srcv += 8;
            eY0 = _mm_slli_epi16(              _mm_unpacklo_epi8( t0, eZero),4); /* | y7 - y0| */
            eU  = _mm_slli_epi16((_mm_unpacklo_epi8( tU, eZero)),7); /* | u7 - u0| */
            eV  = _mm_slli_epi16((_mm_unpacklo_epi8( tV, eZero)),7); /* | v7 - v0| */

            eR0 = _mm_mulhi_epi16(  eV, iRCr );
            eR0 = _mm_adds_epi16 ( eR0,  eY0 );
            eR0 = _mm_subs_epi16 ( eR0,  iR  );
            eR0 = _mm_adds_epi16 ( eR0, kOKR );
            eR0 = _mm_srai_epi16 ( eR0,    4 );
            eR0 = _mm_packus_epi16(eR0,  eR0 );
            eB0 = _mm_mulhi_epi16(  eU, iBCb );
            eB0 = _mm_adds_epi16 ( eB0,  eY0 );
            eB0 = _mm_subs_epi16 ( eB0,  iB  );
            eB0 = _mm_adds_epi16 ( eB0, kOKR );
            eB0 = _mm_srai_epi16 ( eB0,    4 );
            eB0 = _mm_packus_epi16(eB0,  eB0 );
            eG0 = _mm_mulhi_epi16(  eU, iGCb );
            eG0 = _mm_adds_epi16 ( eG0, _mm_mulhi_epi16( eV, iGCr ) );
            eY0 = _mm_adds_epi16 ( eY0,  iG  );
            eY0 = _mm_subs_epi16 ( eY0,  eG0 );
            eY0 = _mm_adds_epi16 ( eY0, kOKR );
            eY0 = _mm_srai_epi16 ( eY0,    4 );
            eY0 = _mm_packus_epi16(eY0,  eY0 );

            t0  = _mm_unpacklo_epi32( eR0, eY0  );   //|.G1|.R1|.G0|.R0|
            t1  = _mm_unpacklo_epi32( eB0, eAval);   //|.A |.B1|.A |.B0|

            t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
            t2 = _mm_shuffle_epi8( t2, sHf );
            _mm_storeu_si128( (__m128i*)dst, t2);
            t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
            t2 = _mm_shuffle_epi8( t2, sHf );
            _mm_storeu_si128( (__m128i*)( dst+16 ), t2);
            dst += 32;
        }
        if(last)
        {
            __m128i t0, t1, t2;
            __m128i eR0, eB0, eG0;
            __m128i eY0;
            __m128i tU, tV;
            __m128i eU, eV;
            for(w = 0; w < last; w++)
            {
                ((Ipp8u*)(&t0))[w]= srcy[w];
                ((Ipp8u*)(&tU))[w]= srcu[w];
                ((Ipp8u*)(&tV))[w]= srcv[w];
            }
            eY0 = _mm_slli_epi16(              _mm_unpacklo_epi8( t0, eZero),4); /* | y7 - y0| */
            eU  = _mm_slli_epi16((_mm_unpacklo_epi8( tU, eZero)),7); /* | u7 - u0| */
            eV  = _mm_slli_epi16((_mm_unpacklo_epi8( tV, eZero)),7); /* | v7 - v0| */

            eR0 = _mm_mulhi_epi16(  eV, iRCr );
            eR0 = _mm_adds_epi16 ( eR0,  eY0 );
            eR0 = _mm_subs_epi16 ( eR0,  iR  );
            eR0 = _mm_adds_epi16 ( eR0, kOKR );
            eR0 = _mm_srai_epi16 ( eR0,    4 );
            eR0 = _mm_packus_epi16(eR0,  eR0 );
            eB0 = _mm_mulhi_epi16(  eU, iBCb );
            eB0 = _mm_adds_epi16 ( eB0,  eY0 );
            eB0 = _mm_subs_epi16 ( eB0,  iB  );
            eB0 = _mm_adds_epi16 ( eB0, kOKR );
            eB0 = _mm_srai_epi16 ( eB0,    4 );
            eB0 = _mm_packus_epi16(eB0,  eB0 );
            eG0 = _mm_mulhi_epi16(  eU, iGCb );
            eG0 = _mm_adds_epi16 ( eG0, _mm_mulhi_epi16( eV, iGCr ) );
            eY0 = _mm_adds_epi16 ( eY0,  iG  );
            eY0 = _mm_subs_epi16 ( eY0,  eG0 );
            eY0 = _mm_adds_epi16 ( eY0, kOKR );
            eY0 = _mm_srai_epi16 ( eY0,    4 );
            eY0 = _mm_packus_epi16(eY0,  eY0 );

            t0  = _mm_unpacklo_epi32( eR0, eY0  );   //|.G1|.R1|.G0|.R0|
            t1  = _mm_unpacklo_epi32( eB0, eAval);   //|.A |.B1|.A |.B0|

            t2 = _mm_unpacklo_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
            t2 = _mm_shuffle_epi8( t2, sHf );
            if(last & 0x04)
            {
                _mm_storeu_si128( (__m128i*)dst, t2);
                dst+=16;
                t2 = _mm_unpackhi_epi64( t0, t1 );       //|.A |.B0|.G0|.R0|
                t2 = _mm_shuffle_epi8( t2, sHf );
            }
            for(w = 0; w < ((last& 0x03)*4); w+=4)
            {
                dst[w+0] = ((Ipp8u*)(&t2))[w+0];
                dst[w+1] = ((Ipp8u*)(&t2))[w+1];
                dst[w+2] = ((Ipp8u*)(&t2))[w+2];
                dst[w+3] = ((Ipp8u*)(&t2))[w+3];
            }
        }
    }
}





#endif
