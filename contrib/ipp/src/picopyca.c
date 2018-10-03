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

/*
//  Purpose:    Copying image into second image.
//              Filling image by value.
//
//  Contents:
//      (V8,U8) mfxiCopy_8u_C3C1R               mfxiCopy_8u_C1C3R (V8,U8)
//         (M7) mfxiCopy_8u_C4C1R (W7)          mfxiCopy_8u_C1C4R (W7))  (M7)
//              mfxiCopy_8u_C3CR                mfxiCopy_8u_C4CR
//      (V8,U8) mfxiCopy_8u_AC4C3R              mfxiCopy_8u_C3AC4R (V8,U8)
//   (V8,U8,M7) mfxiCopy_8u_C1R (M6,A6,W7,I7)   mfxiCopy_8u_C1MR (W7)    (M7)
//   (V8,U8,M7) mfxiCopy_8u_C3R (M6,A6,W7,I7)   mfxiCopy_8u_C3MR (A6,W7) (M7)
//   (V8,U8,M7) mfxiCopy_8u_C4R (M6,A6,W7,I7)   mfxiCopy_8u_C4MR
//         (M7) mfxiCopy_8u_AC4R (W7)           mfxiCopy_8u_AC4MR
//         (M7) mfxiSet_8u_C1R (M6,A6,W7)       mfxiSet_8u_C1MR (W7)     (M7)
//              mfxiSet_8u_C3CR                 mfxiSet_8u_C4CR
//         (M7) mfxiSet_8u_C3R (M6,A6,W7)       mfxiSet_8u_C3MR (A6,W7)  (M7)
//         (M7) mfxiSet_8u_C4R (M6,A6,W7)       mfxiSet_8u_C4MR
//              mfxiSet_8u_AC4R                 mfxiSet_8u_AC4MR
//
//              mfxiCopy_16s_C3C1R              mfxiCopy_16s_C1C3R
//              mfxiCopy_16s_C4C1R              mfxiCopy_16s_C1C4R
//              mfxiCopy_16s_C3CR               mfxiCopy_16s_C4CR
//      (V8,U8) mfxiCopy_16s_AC4C3R             mfxiCopy_16s_C3AC4R (V8,U8)
//   (V8,U8,M7) mfxiCopy_16s_C1R (M6,A6,W7,I7)  mfxiCopy_16s_C1MR (W7)    (M7)
//   (V8,U8,M7) mfxiCopy_16s_C3R (M6,A6,W7,I7)  mfxiCopy_16s_C3MR (A6,W7) (M7)
//   (V8,U8,M7) mfxiCopy_16s_C4R (M6,A6,W7,I7)  mfxiCopy_16s_C4MR
//         (M7) mfxiCopy_16s_AC4R (W7)          mfxiCopy_16s_AC4MR
//         (M7) mfxiSet_16s_C1R (M6,A6,W7)      mfxiSet_16s_C1MR (W7)     (M7)
//              mfxiSet_16s_C3CR                mfxiSet_16s_C4CR
//         (M7) mfxiSet_16s_C3R (M6,A6,W7)      mfxiSet_16s_C3MR (A6,W7)  (M7)
//         (M7) mfxiSet_16s_C4R (M6,A6,W7)      mfxiSet_16s_C4MR
//              mfxiSet_16s_AC4R                mfxiSet_16s_AC4MR
//
//              mfxiCopy_32f_C3C1R              mfxiCopy_32f_C1C3R
//              mfxiCopy_32f_C4C1R              mfxiCopy_32f_C1C4R
//              mfxiCopy_32f_C3CR               mfxiCopy_32f_C4CR
//              mfxiCopy_32f_AC4C3R             mfxiCopy_32f_C3AC4R
//   (V8,U8,M7) mfxiCopy_32f_C1R (M6,A6,W7,I7)  mfxiCopy_32f_C1MR (W7)    (M7)
//   (V8,U8,M7) mfxiCopy_32f_C3R (M6,A6,W7,I7)  mfxiCopy_32f_C3MR (A6,W7) (M7)
//   (V8,U8,M7) mfxiCopy_32f_C4R (M6,A6,W7,I7)  mfxiCopy_32f_C4MR
//              mfxiCopy_32f_AC4R               mfxiCopy_32f_AC4MR
//         (M7) mfxiSet_32f_C1R (M6,A6,W7)      mfxiSet_32f_C1MR (W7)     (M7)
//              mfxiSet_32f_C3CR                mfxiSet_32f_C4CR
//         (M7) mfxiSet_32f_C3R (M6,A6,W7)      mfxiSet_32f_C3MR (A6,W7)  (M7)
//         (M7) mfxiSet_32f_C4R (M6,A6,W7)      mfxiSet_32f_C4MR
//              mfxiSet_32f_AC4R                mfxiSet_32f_AC4MR
//
//         (M7) mfxiCopy_8u_C3P3R (A6,W7)       mfxiCopy_8u_P3C3R (W7)    (M7)(V8,U8)
//         (M7) mfxiCopy_8u_C4P4R (A6,W7)       mfxiCopy_8u_P4C4R (W7)    (M7)
//         (M7) mfxiCopy_16s_C3P3R (A6,W7)      mfxiCopy_16s_P3C3R (A6,W7)(M7)
//         (M7) mfxiCopy_16s_C4P4R (A6,W7)      mfxiCopy_16s_P4C4R (W7)   (M7)
//         (M7) mfxiCopy_32f_C3P3R (W7)         mfxiCopy_32f_P3C3R (W7)   (M7)
//         (M7) mfxiCopy_32f_C4P4R (W7)         mfxiCopy_32f_P4C4R (W7)   (M7)
//
*/
#include "precomp.h"
#include "owni.h"
#include "ippcore.h"

#if (_IPPLRB >= _IPPLRB_B1)
#if  defined(_REF_LIB)
extern void lmmintrin_init();
#endif
extern void mfxowniCopy8u_B1( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep, int width, int height );

extern void mfxowniSet_8u_C1R_B1( const Ipp8u* value, Ipp8u* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_8u_C3R_B1( const Ipp8u* value, Ipp8u* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_8u_C4R_B1( const Ipp8u* value, Ipp8u* pDst,  int dstStep, int  width, int height );

extern void mfxowniSet_16s_C1R_B1( const Ipp16s* value, Ipp16s* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_16s_C3R_B1( const Ipp16s* value, Ipp16s* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_16s_C4R_B1( const Ipp16s* value, Ipp16s* pDst,  int dstStep, int  width, int height );

extern void mfxowniSet_32s_C1R_B1( const Ipp32s* value, Ipp32s* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_32s_C3R_B1( const Ipp32s* value, Ipp32s* pDst,  int dstStep, int  width, int height );
extern void mfxowniSet_32s_C4R_B1( const Ipp32s* value, Ipp32s* pDst,  int dstStep, int  width, int height );

extern void mfxowniCopy_8u_P3C3R_B1( const Ipp8u* const pSrc[3], int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_C3P3R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* const pDst[3], int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_P4C4R_B1( const Ipp8u* const pSrc[4], int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_C4P4R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* const pDst[4], int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_32s_P3C3R_B1( const Ipp32s* const pSrc[3], int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_P3C3R_B1( const Ipp16s* const pSrc[3], int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_P4C4R_B1( const Ipp32s* const pSrc[4], int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_P4C4R_B1( const Ipp16s* const pSrc[4], int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C3P3R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* const pDst[3], int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_C3P3R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* const pDst[3], int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C4P4R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* const pDst[4], int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_C4P4R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* const pDst[4], int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_8u_C4C1R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_C3C1R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_C1C4R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_8u_C1C3R_B1( const Ipp8u* pSrc, int srcStep,
                                  Ipp8u* pDst, int dstStep,
                                  IppiSize roiSize );
extern void mfxowniCopy_16s_C4C1R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_C3C1R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_C1C4R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_C1C3R_B1( const Ipp16s* pSrc, int srcStep,
                                   Ipp16s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C4C1R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C3C1R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C1C4R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_32s_C1C3R_B1( const Ipp32s* pSrc, int srcStep,
                                   Ipp32s* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_8u_AC4C3R_B1( const Ipp8u* pSrc, int srcStep,
                                   Ipp8u* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_8u_C3AC4R_B1( const Ipp8u* pSrc, int srcStep,
                                   Ipp8u* pDst, int dstStep,
                                   IppiSize roiSize );
extern void mfxowniCopy_16s_AC4C3R_B1( const Ipp16s* pSrc, int srcStep,
                                    Ipp16s* pDst, int dstStep,
                                    IppiSize roiSize );
extern void mfxowniCopy_16s_C3AC4R_B1( const Ipp16s* pSrc, int srcStep,
                                    Ipp16s* pDst, int dstStep,
                                    IppiSize roiSize );
extern void mfxowniCopy_32s_AC4C3R_B1( const Ipp32s* pSrc, int srcStep,
                                    Ipp32s* pDst, int dstStep,
                                    IppiSize roiSize );
extern void mfxowniCopy_32s_C3AC4R_B1( const Ipp32s* pSrc, int srcStep,
                                    Ipp32s* pDst, int dstStep,
                                    IppiSize roiSize );
#endif

#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )
#include "cpudef.h"
#include "ippcore.h"
#endif

#if !( (_IPP>=_IPP_M6) || (_IPP32E>=_IPP32E_M7) || (_IPP64>=_IPP64_I7) )
#define _CCODE
#endif

#if ( _IPPLP32 >= _IPPLP32_S8 ) || ( _IPPLP64 >= _IPPLP64_N8 ) || ( _IPP <= _IPP_W7 )
#define BOUND_NT  524288
#define BOUND_NT_  8388608 /* Atom: Temporarily for Copy_16s C3P3R,P3C3R,C4P4R. 7.05.08 */
#else
#define BOUND_NT  1048576
#endif

#if  ( _IPP == _IPP_A6 ) || ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 ) || ( _IPP32E == _IPP32E_M7 )
#define CACHE_SIZE 2097152
#endif

#if ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 )
extern void mfxowniCopy_8u_C3AC4R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
                                        int width, int height );
extern void mfxowniCopy_16s_C3AC4R( const Ipp16s* pSrc, int srcStep, Ipp16s* pDst, int dstStep,
                                          int width, int height );
extern void mfxowniCopy_8u_AC4C3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
                                        int width, int height );
extern void mfxowniCopy_16s_AC4C3R( const Ipp16s* pSrc, int srcStep, Ipp16s* pDst, int dstStep,
                                          int width, int height );
extern void mfxowniCopy_8u_C1C3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
                                       int width, int height );
extern void mfxowniCopy_8u_C3C1R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
                                       int width, int height );
extern void mfxowniCopy_8u_P3C3R(const  Ipp8u* const pSrc[3], int srcStep,
                                           Ipp8u* pDst, int dstStep,
                                         IppiSize roiSize );
extern void mfxowniCopy_8u_P4C4R(const  Ipp8u* const pSrc[4], int srcStep,
                                           Ipp8u* pDst, int dstStep,
                                         IppiSize roiSize );
extern void mfxowniCopy_16s_P3C3R(const  Ipp16s* const pSrc[3], int srcStep,
                                            Ipp16s* pDst, int dstStep,
                                           IppiSize roiSize );
extern void mfxowniCopy_16s_P4C4R(const  Ipp16s* const pSrc[4], int srcStep,
                                            Ipp16s* pDst, int dstStep,
                                           IppiSize roiSize );
#endif

#if defined (_I7)
void mfxowniCopy_8u_I7( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int len );
void mfxowniCopy_8u_I7_NT( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int len );
#endif

#if defined (_A6) || (( _IPP >= _IPP_W7 ) && ( _IPP < _IPP_P8 ))
extern void mfxowniCopy8u_C3P3_88_A6
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_84_A6
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_48_A6
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_A6( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C4P4_48_A6
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[4], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C4P4_44_A6
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[4], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C4P4_A6( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy16s_C3P3_A6( const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy16s_C3P3_A6_NT( const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy16s_C4P4_A6( const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy16s_C4P4_A6_NT( const Ipp16s* pSrc, Ipp16s* pDst, int len );
#endif

#if defined (_A6) || ( _IPP >= _IPP_W7 )
extern void mfxowniCopy16s_P3C3_A6
            (const  Ipp16s* const pSrc[4], int srcStep, Ipp16s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
#endif

#if ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 )
extern void mfxowniCopy_8u_C1_W7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, int nonTemporal );
#endif

#if ( _IPP >= _IPP_W7 )
extern void mfxowniCopy8u_P3C3_W7
            ( const  Ipp8u* const pSrc[3], int srcStep, Ipp8u* pDst, int dstStep,
                     int width, int height, int nonTemporal );
extern void mfxowniCopy8u_P4C4_W7
            (const  Ipp8u* const pSrc[4], int srcStep, Ipp8u* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy16s_P4C4_W7
            (const  Ipp16s* const pSrc[4], int srcStep, Ipp16s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy32s_C3P3_W7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C3P3_W7_NT( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C4P4_W7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C4P4_W7_NT( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_P4C4_W7
            (const  Ipp32s* const pSrc[4], int srcStep, Ipp32s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy16s_AC4_W7(const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy8u_AC4_W7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C4C1_W7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C1C4_W7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
#if (_IPP < _IPP_H9 )
extern void mfxowniSet_8u_C1_W7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_8u_C3_W7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_8u_C4_W7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C1_W7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C3_W7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C4_W7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_32f_C3_W7( Ipp32f* buf, Ipp32f* pDst, int len, int nonTemporal );
extern void mfxowniCopy32s_P3C3_W7
            (const  Ipp32s* const pSrc[4], int srcStep, Ipp32s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
#endif
extern void mfxowniSet_32f_C4_W7( Ipp32f* buf, Ipp32f* pDst, int len, int nonTemporal );
extern void mfxowniCopy_8u_C3M_W7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_8u_C1M_W7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_8u_C1M_W7
            ( Ipp8u* buf, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_8u_C3M_W7
            ( Ipp8u* buf, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_16s_C1M_W7
            ( const Ipp16s* pSrc, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_16s_C3M_W7
            ( const Ipp16s* pSrc, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_16s_C1M_W7
            ( Ipp16s* buf, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_16s_C3M_W7
            ( Ipp16s* buf, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_32f_C1M_W7
            ( const Ipp32f* pSrc, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_32f_C3M_W7
            ( const Ipp32f* pSrc, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_32f_C1M_W7
            ( Ipp32f* value, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_32f_C3M_W7
            ( Ipp32f* buf, Ipp32f* pDst, int len, const Ipp8u* mask );
#endif

#if defined (_A6)
extern void mfxowniCopy_8u_A6( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy_8u_C3M_A6
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_16s_C3M_A6
            ( const Ipp16s* pSrc, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_32f_C3M_A6
            ( const Ipp32f* pSrc, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_8u_A6( Ipp8u val, Ipp8u* pDst, int len );
extern void mfxowniSet_8u_C3_A6( const Ipp8u value[3], Ipp8u* pDst, int len );
extern void mfxowniSet_8u_C3M_A6( const Ipp8u value[3], Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_16s_A6( Ipp16s val, Ipp16s* dst, int len );
extern void mfxowniSet_16s_C3_A6( const  Ipp16s value[3], Ipp16s* dst, int len );
extern void mfxowniSet_16s_C3M_A6( const Ipp16s value[3], Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_32f_A6( Ipp32f val, Ipp32f* dst, int len );
extern void mfxowniSet_32f_C3_A6( const  Ipp32f value[3], Ipp32f* dst, int len );
extern void mfxowniSet_32f_C3M_A6( const Ipp32f value[3], Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_64s_A6( Ipp64s val, Ipp64s* dst, int len );
extern void mfxowniSet_64sc_A6( Ipp64sc val, Ipp64sc* dst, int len );
#endif

#if defined (_M6)
extern void mfxowniCopy_8u_M6( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniSet_8u_M6( Ipp8u val, Ipp8u* pDst, int len );
extern void mfxowniSet_8u_C3_M6( const Ipp8u value[3], Ipp8u* pDst, int len );
extern void mfxowniSet_16s_M6( Ipp16s val, Ipp16s* dst, int len );
extern void mfxowniSet_16s_C3_M6( const Ipp16s value[3], Ipp16s* dst, int len );
extern void mfxowniSet_32f_M6( Ipp32f val, Ipp32f* dst, int len );
extern void mfxowniSet_32f_C3_M6( const Ipp32f value[3], Ipp32f* dst, int len );
extern void mfxowniSet_64s_M6( Ipp64s val, Ipp64s* dst, int len );
extern void mfxowniSet_64sc_M6( Ipp64sc val, Ipp64sc* dst, int len );
#endif

#if (_IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 )
extern void mfxowniCopy8uas( const Ipp8u* pSrc, int srcStep,Ipp8u* pDst, int dstStep,
                                  int width, int height );
extern void mfxowniCopy16sC1Mas( const Ipp16s* pSrc, int srcStep,Ipp16s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniCopy16sC3Mas( const Ipp16s* pSrc, int srcStep,Ipp16s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniCopy16sC4Mas( const Ipp16s* pSrc, int srcStep,Ipp16s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet16sC1Mas( Ipp16s value, Ipp16s* pDst,   int dstStep,
                             int  width,  int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet16sC3Mas( const Ipp16s value[3], Ipp16s* pDst, int dstStep,
                             int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet16sC4Mas( const Ipp16s value[4], Ipp16s* pDst, int dstStep,
                             int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniCopy32sC1Mas( const Ipp32s* pSrc, int srcStep,Ipp32s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniCopy32sC3Mas( const Ipp32s* pSrc, int srcStep,Ipp32s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniCopy32sC4Mas( const Ipp32s* pSrc, int srcStep,Ipp32s* pDst, int dstStep,
                              int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet32sC1Mas( Ipp32s value, Ipp32s* pDst,   int dstStep,
                             int  width,  int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet32sC3Mas( const Ipp32s value[3], Ipp32s* pDst, int dstStep,
                             int width, int height, const Ipp8u* mask, int maskStep );
extern void mfxowniSet32sC4Mas( const Ipp32s value[4], Ipp32s* pDst, int dstStep,
                             int width, int height, const Ipp8u* mask, int maskStep );
#endif

#if ( _IPP32E == _IPP32E_M7 )
extern void mfxowniCopy_8u_C1_M7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniCopy_16s_C1M_M7
            ( const Ipp16s* pSrc, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_16s_C3M_M7
            ( const Ipp16s* pSrc, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_32f_C1M_M7
            ( const Ipp32f* pSrc, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_32f_C3M_M7
            ( const Ipp32f* pSrc, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_16s_C1M_M7
            ( Ipp16s* buf, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_16s_C3M_M7
            ( Ipp16s* buf, Ipp16s* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_32f_C1M_M7
            ( Ipp32f* value, Ipp32f* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_32f_C3M_M7
            ( Ipp32f* buf, Ipp32f* pDst, int len, const Ipp8u* mask );
#endif

#if ( _IPP32E >= _IPP32E_M7 )
extern void mfxowniCopy8u_AC4_M7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy16s_AC4_M7(const Ipp16s* pSrc, Ipp16s* pDst, int len );
#if (_IPP32E < _IPP32E_L9)
extern void mfxowniSet_8u_C1_M7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_8u_C3_M7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_8u_C4_M7( Ipp8u* buf, Ipp8u* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C1_M7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C3_M7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_16s_C4_M7( Ipp16s* buf, Ipp16s* pDst, int len, int nonTemporal );
extern void mfxowniSet_32f_C3_M7( Ipp32f* buf, Ipp32f* pDst, int len, int nonTemporal );
extern void mfxowniCopy32s_P3C3_M7
            (const  Ipp32s* const pSrc[4], int srcStep, Ipp32s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
#endif
extern void mfxowniSet_32f_C4_M7( Ipp32f* buf, Ipp32f* pDst, int len, int nonTemporal );
extern void mfxowniCopy_8u_C1M_M7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy_8u_C3M_M7
            ( const Ipp8u* pSrc, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_8u_C1M_M7
            ( Ipp8u* buf, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniSet_8u_C3M_M7
            ( Ipp8u* buf, Ipp8u* pDst, int len, const Ipp8u* mask );
extern void mfxowniCopy8u_C4C1_M7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C1C4_M7(const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy32s_C3P3_M7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C3P3_NT_M7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C4P4_M7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy32s_C4P4_NT_M7( const Ipp32s* pSrc, Ipp32s* pDst, int len );
extern void mfxowniCopy8u_P3C3_M7
            ( const  Ipp8u* const pSrc[3], int srcStep, Ipp8u* pDst, int dstStep,
                     int width, int height, int nonTemporal );
extern void mfxowniCopy8u_P4C4_M7
            (const  Ipp8u* const pSrc[4], int srcStep, Ipp8u* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy16s_P3C3_M7
            (const  Ipp16s* const pSrc[4], int srcStep, Ipp16s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy16s_P4C4_M7
            (const  Ipp16s* const pSrc[4], int srcStep, Ipp16s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy32s_P4C4_M7
            (const  Ipp32s* const pSrc[4], int srcStep, Ipp32s* pDst, int dstStep,
                    int width, int height, int nonTemporal );
extern void mfxowniCopy16s_C3P3_NT_M7( const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy16s_C4P4_NT_M7( const Ipp16s* pSrc, Ipp16s* pDst, int len );
#endif

#if (( _IPP32E >= _IPP32E_M7 ) && ( _IPP32E < _IPP32E_Y8 ))
extern void mfxowniCopy8u_C3P3_88_M7
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_84_M7
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_48_M7
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[3], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C3P3_M7( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C4P4_M7( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern void mfxowniCopy8u_C4P4_48_M7
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[4], int dstStep,
       int width, int height );
extern void mfxowniCopy8u_C4P4_44_M7
     ( const Ipp8u* pSrc,  int srcStep, Ipp8u* const pDst[4], int dstStep,
       int width, int height );
extern void mfxowniCopy16s_C3P3_M7( const Ipp16s* pSrc, Ipp16s* pDst, int len );
extern void mfxowniCopy16s_C4P4_M7( const Ipp16s* pSrc, Ipp16s* pDst, int len );
#endif

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 )
extern void mfxowniCopy_8u_C3P3R_cn( const Ipp8u* pSrc, int srcStep,
                                Ipp8u* const pDst[3], int dstStep,
                                IppiSize roiSize );
extern void mfxowniCopy_8u_C4P4R_cn( const Ipp8u* pSrc, int srcStep,
                                Ipp8u* const pDst[4], int dstStep,
                                IppiSize roiSize );
extern void mfxowniCopy_16s_C3P3R_cn( const Ipp16s* pSrc, int srcStep,
                            Ipp16s* const pDst[3], int dstStep,
                                IppiSize roiSize );
extern void mfxowniCopy_16s_C4P4R_cn( const Ipp16s* pSrc, int srcStep,
                            Ipp16s* const pDst[4], int dstStep,
                                IppiSize roiSize );
#endif

#if ( _IPP32E >= _IPP32E_E9 )
void mfxowniCopy8u_E9( const Ipp8u* pSrc, int srcStep, Ipp8u *pDst, int dstStep, int width, int height );
void mfxowniCopy8u_cn( const Ipp8u* pSrc, int srcStep, Ipp8u *pDst, int dstStep, int width, int height );
#endif

#if (_IPP >= _IPP_H9 ) || (_IPP32E >= _IPP32E_L9)
void mfxowniSet_8u_C1R(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_8u_C3R(Ipp8u *value, Ipp8u* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_8u_C4R(Ipp8u *value, Ipp8u* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_16s_C1R(Ipp16s value, Ipp16s* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_16s_C3R(Ipp16s *value, Ipp16s* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_16s_C4R(Ipp16s *value, Ipp16s* pDst, int dstStep, IppiSize roiSize);
void mfxowniSet_32s_C3R(Ipp32s *value, Ipp32s* pDst, int dstStep, IppiSize roiSize);
void mfxowniCopy_32s_AC4C3R(const Ipp32s* pSrc, int srcStep, Ipp32s* pDst, int dstStep, int width, int height);
#endif


/*******************************************************************/
#if (_IPP_ARCH != _IPP_ARCH_XSC)
IPPFUN ( IppStatus, mfxiCopy_8u_C1R,
                    ( const Ipp8u* pSrc, int srcStep,
                    Ipp8u* pDst, int dstStep, IppiSize roiSize )) {

#if !( _IPP >= _IPP_V8 ) && !( _IPP32E >= _IPP32E_U8 ) && !(_IPPLRB >= _IPPLRB_B1)
    const Ipp8u *src = pSrc;
    Ipp8u *dst = pDst;
    int width = roiSize.width, height = roiSize.height;
    int h;
#if defined (_CCODE)
    int n;
#endif
#if ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 ) || defined (_I7) || ( _IPP32E == _IPP32E_M7 )
    int nonTemporal = 0;
#endif
#endif

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy8u_B1( pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height );
#elif ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 )
  #if ( _IPP32E >= _IPP32E_E9 )
    if( roiSize.width < 1024 ) {
      mfxowniCopy8u_E9( pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height );
    } else {
      mfxowniCopy8u_cn( pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height );
    }  
  #else
    mfxowniCopy8uas( pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height );
  #endif
#else

#if ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 ) || ( _IPP32E == _IPP32E_M7 )
    if( width*height > CACHE_SIZE/2 ) nonTemporal = 1;
#endif
#if defined (_I7)
    if( width > 16278 ) nonTemporal = 1;
#endif

    #if !defined(_CCODE)
    if( (srcStep == dstStep) && (srcStep == width) ) {
       width *= height;
       height = 1;
    }
    #endif

    for( h = 0; h < height; h++ ) {
        #if ( _IPP32E == _IPP32E_M7 )
        mfxowniCopy_8u_C1_M7( src, dst, width, nonTemporal );
        #elif ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 )
        mfxowniCopy_8u_C1_W7( src, dst, width, nonTemporal );
        #elif   defined (_A6)
        mfxowniCopy_8u_A6( src, dst, width );
        #elif   defined (_M6)
        mfxowniCopy_8u_M6( src, dst, width );
        #elif   defined (_I7)
        if(!nonTemporal) {
            mfxowniCopy_8u_I7( src, srcStep, dst, width );
        } else {
            mfxowniCopy_8u_I7_NT( src, srcStep, dst, width );
        }
        #else
        for( n = 0; n < width; n++ ) {
            dst[n] = src[n];
        }
        #endif
        src += srcStep, dst += dstStep;
    }

#endif /*#if ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 ) || (_IPPLRB >= _IPPLRB_B1)*/

    return ippStsNoErr;
}
#endif /* _IPP_ARCH != _IPP_ARCH_XSC */


/*******************************************************************/
IPPFUN ( IppStatus, mfxiCopy_16s_C1R,
                    ( const Ipp16s* pSrc, int srcStep,
                    Ipp16s* pDst, int dstStep, IppiSize roiSize )) {

#if !( _IPP >= _IPP_V8 ) && !( _IPP32E >= _IPP32E_U8 ) && !(_IPPLRB >= _IPPLRB_B1)
    const Ipp16s *src = pSrc;
    Ipp16s *dst = pDst;
    int height = roiSize.height;
    int h;
#if defined (_CCODE)
    int n, width = roiSize.width;
#else
    int width = roiSize.width*2;
#endif
#if ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 ) || defined (_I7) || ( _IPP32E == _IPP32E_M7 )
    int nonTemporal = 0;
#endif
#endif

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy8u_B1( (Ipp8u*)pSrc, srcStep, (Ipp8u*)pDst, dstStep, roiSize.width*2, roiSize.height );
#elif ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 )
  #if ( _IPP32E >= _IPP32E_E9 )
    if( roiSize.width*2 < 1024 ) {
      mfxowniCopy8u_E9( (Ipp8u*)pSrc, srcStep, (Ipp8u*)pDst, dstStep, roiSize.width*2, roiSize.height );
    } else {
      mfxowniCopy8u_cn( (Ipp8u*)pSrc, srcStep, (Ipp8u*)pDst, dstStep, roiSize.width*2, roiSize.height );
    }  
  #else
    mfxowniCopy8uas( (Ipp8u*)pSrc, srcStep, (Ipp8u*)pDst, dstStep, roiSize.width*2, roiSize.height );
  #endif
#else

#if ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 ) || ( _IPP32E == _IPP32E_M7 )
    if( width*height > CACHE_SIZE/2 ) nonTemporal = 1;
#endif
#if defined (_I7)
    if( width > 16278 ) nonTemporal = 1;
#endif

    #if !defined(_CCODE)
    if( (srcStep == dstStep) && (srcStep == width) ) {
       width *= height;
       height = 1;
    }
    #endif

    for( h = 0; h < height; h++ ) {
        #if ( _IPP32E == _IPP32E_M7 )
        mfxowniCopy_8u_C1_M7( (Ipp8u*)src, (Ipp8u*)dst, width, nonTemporal );
        #elif ( _IPP == _IPP_W7 ) || ( _IPP == _IPP_T7 )
        mfxowniCopy_8u_C1_W7( (Ipp8u*)src, (Ipp8u*)dst, width, nonTemporal );
        #elif   defined (_A6)
        mfxowniCopy_8u_A6( (Ipp8u*)src, (Ipp8u*)dst, width );
        #elif   defined (_M6)
        mfxowniCopy_8u_M6( (Ipp8u*)src, (Ipp8u*)dst, width );
        #elif   defined (_I7)
        if(!nonTemporal) {
            mfxowniCopy_8u_I7( (Ipp8u*)src, srcStep, (Ipp8u*)dst, width );
        } else {
            mfxowniCopy_8u_I7_NT( (Ipp8u*)src, srcStep, (Ipp8u*)dst, width );
        }
        #else
        for( n = 0; n < width; n++ ) {
            dst[n] = src[n];
        }
        #endif
        src = (Ipp16s*)((Ipp8u*)src + srcStep);
        dst = (Ipp16s*)((Ipp8u*)dst + dstStep);
    }

#endif /*#if ( _IPP >= _IPP_V8 ) || ( _IPP32E >= _IPP32E_U8 ) || (_IPPLRB >= _IPPLRB_B1)*/

    return ippStsNoErr;
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiCopy_8u_C3P3R,
                    ( const Ipp8u* pSrc, int srcStep,
                      Ipp8u* const pDst[3], int dstStep,
                      IppiSize roiSize )) {

#if !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 )
    const Ipp8u *src = pSrc;
    Ipp8u *dst1, *dst2, *dst3;
    int height = roiSize.height;
    int width = roiSize.width;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int wPL, wPX;
#else
    int align;
#endif
#endif /* !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 ) */

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2] )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1)
  #if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy_8u_C3P3R_B1( pSrc, srcStep, pDst, dstStep, roiSize );
  #else
    mfxowniCopy_8u_C3P3R_cn( pSrc, srcStep, pDst, dstStep, roiSize );
  #endif
#else
    dst1 = pDst[0], dst2 = pDst[1], dst3 = pDst[2];
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    align =
        (int)(((IPP_INT_PTR(src) & 0x7) << 4) |
        ((IPP_INT_PTR(dst1) | IPP_INT_PTR(dst2) | IPP_INT_PTR(dst3)) & 0x7));
    if( (dstStep*3 == srcStep) && (dstStep == width) &&
        ((srcStep*height < 160000) | !(align & 0x33)) ) {
        width *= height;
        height = 1;
    }
#else
    if( (dstStep*3 == srcStep) && (dstStep == width) && (srcStep*height < 160000) ) {
        width *= height;
        height = 1;
    }
#endif

#if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    switch ( align ) {
        case 0x00:
            mfxowniCopy8u_C3P3_88_M7( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x04:
            mfxowniCopy8u_C3P3_84_M7( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x40:
            mfxowniCopy8u_C3P3_48_M7( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        default:
#endif
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
    switch ( align ) {
        case 0x00:
            mfxowniCopy8u_C3P3_88_A6( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x04:
            mfxowniCopy8u_C3P3_84_A6( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x40:
            mfxowniCopy8u_C3P3_48_A6( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        default:
#endif
            for( h = 0; h < height; h++ ) {
                #if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
                mfxowniCopy8u_C3P3_M7( src, dst1, width );
                mfxowniCopy8u_C3P3_M7( src+1, dst2, width );
                mfxowniCopy8u_C3P3_M7( src+2, dst3, width );
                #elif defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
                mfxowniCopy8u_C3P3_A6( src, dst1, width );
                mfxowniCopy8u_C3P3_A6( src+1, dst2, width );
                mfxowniCopy8u_C3P3_A6( src+2, dst3, width );
                #else
                for( wPL=0,wPX=0; wPL < width; wPL++, wPX+=3 ) {
                    dst1[wPL] = src[wPX];
                }
                for( wPL=0,wPX=1; wPL < width; wPL++, wPX+=3 ) {
                dst2[wPL] = src[wPX];
                }
                for( wPL=0,wPX=2; wPL < width; wPL++, wPX+=3 ) {
                    dst3[wPL] = src[wPX];
                }
                #endif
                src += srcStep,
                dst1 += dstStep;
                dst2 += dstStep;
                dst3 += dstStep;
            }
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    }
#endif
#endif /* ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1) */
    return ippStsNoErr;
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiCopy_8u_C4P4R,
                    ( const Ipp8u* pSrc, int srcStep,
                      Ipp8u* const pDst[4], int dstStep,
                      IppiSize roiSize )) {

#if !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 )
    const Ipp8u *src = pSrc;
    Ipp8u *dst1, *dst2, *dst3, *dst4;
    int height = roiSize.height;
    int width = roiSize.width;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int wPL, wPX;
#else
    int align;
#endif
#endif /* !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 ) */

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_PTR4_RET( pDst[0], pDst[1], pDst[2], pDst[3])
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1)
  #if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy_8u_C4P4R_B1( pSrc, srcStep, pDst, dstStep, roiSize );
  #else
    mfxowniCopy_8u_C4P4R_cn( pSrc, srcStep, pDst, dstStep, roiSize );
  #endif
#else
    dst1 = pDst[0], dst2 = pDst[1], dst3 = pDst[2], dst4 = pDst[3];

#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    align =
        (int)(((IPP_INT_PTR(src) & 0x7) << 4) |
        ((IPP_INT_PTR(dst1) | IPP_INT_PTR(dst2) |
          IPP_INT_PTR(dst3) | IPP_INT_PTR(dst4)) & 0x7));
    if( (dstStep*4 == srcStep) && (dstStep == width) &&
        ((srcStep*height < 160000) | !(align & 0x33)) ) {
        width *= height;
        height = 1;
    }
#else
    if( (dstStep*4 == srcStep) && (dstStep == width) && (srcStep*height < 160000) ) {
        width *= height;
        height = 1;
    }
#endif

#if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    switch ( align ) {
        case 0x00:
        case 0x40:
            mfxowniCopy8u_C4P4_48_M7( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x04:
        case 0x44:
            mfxowniCopy8u_C4P4_44_M7( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        default:
#endif
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
    switch ( align ) {
        case 0x00:
        case 0x40:
            mfxowniCopy8u_C4P4_48_A6( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        case 0x04:
        case 0x44:
            mfxowniCopy8u_C4P4_44_A6( pSrc, srcStep, pDst, dstStep, width, height );
            break;
        default:
#endif
            for( h = 0; h < height; h++ ) {
                #if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
                mfxowniCopy8u_C4P4_M7( src, dst1, width );
                mfxowniCopy8u_C4P4_M7( src+1, dst2, width );
                mfxowniCopy8u_C4P4_M7( src+2, dst3, width );
                mfxowniCopy8u_C4P4_M7( src+3, dst4, width );
                #elif defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
                mfxowniCopy8u_C4P4_A6( src, dst1, width );
                mfxowniCopy8u_C4P4_A6( src+1, dst2, width );
                mfxowniCopy8u_C4P4_A6( src+2, dst3, width );
                mfxowniCopy8u_C4P4_A6( src+3, dst4, width );
                #else
                for( wPL=0,wPX=0; wPL < width; wPL++, wPX+=4 ) {
                    dst1[wPL] = src[wPX];
                }
                for( wPL=0,wPX=1; wPL < width; wPL++, wPX+=4 ) {
                    dst2[wPL] = src[wPX];
                }
                for( wPL=0,wPX=2; wPL < width; wPL++, wPX+=4 ) {
                    dst3[wPL] = src[wPX];
                }
                for( wPL=0,wPX=3; wPL < width; wPL++, wPX+=4 ) {
                    dst4[wPL] = src[wPX];
                }
                #endif
                src += srcStep,
                dst1 += dstStep;
                dst2 += dstStep;
                dst3 += dstStep;
                dst4 += dstStep;
            }
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    }
#endif
#endif /* _( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1) */
    return ippStsNoErr;
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiCopy_16s_C3P3R,
                    ( const Ipp16s* pSrc, int srcStep,
                      Ipp16s* const pDst[3], int dstStep,
                      IppiSize roiSize )) {

#if !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 )
    const Ipp16s *src = pSrc;
    Ipp16s *dst1, *dst2, *dst3;
    int height = roiSize.height;
    int width = roiSize.width;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int wPL, wPX;
#endif
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    IppStatus ownStatus = ippStsNoErr;
    int nonTemporal = 0, size = 0, cache = 0;
#endif
#endif /* !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 ) */

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_PTR3_RET( pDst[0], pDst[1], pDst[2] )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1)
  #if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy_16s_C3P3R_B1( pSrc, srcStep, pDst, dstStep, roiSize );
  #else
    mfxowniCopy_16s_C3P3R_cn( pSrc, srcStep, pDst, dstStep, roiSize );
  #endif
#else
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    size = width*height*12;
    #if (_IPPLP32 >=_IPPLP32_S8)||(_IPPLP64 >=_IPPLP64_N8)
    if( size > BOUND_NT_ ) {
    #else
    if( size > BOUND_NT ) {
    #endif
      ownStatus = ippGetMaxCacheSizeB( &cache );
      if( ownStatus == ippStsNoErr  ) {
        if( size >= cache ) nonTemporal = 1;
      }
    }
#endif

    dst1 = pDst[0], dst2 = pDst[1], dst3 = pDst[2];

#if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    if( !nonTemporal ) {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C3P3_M7( src, dst1, width );
            mfxowniCopy16s_C3P3_M7( src+1, dst2, width );
            mfxowniCopy16s_C3P3_M7( src+2, dst3, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
    } else {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C3P3_NT_M7( src, dst1, width );
            mfxowniCopy16s_C3P3_NT_M7( src+1, dst2, width );
            mfxowniCopy16s_C3P3_NT_M7( src+2, dst3, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
    }
#elif defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
    if( !nonTemporal ) {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C3P3_A6( src, dst1, width );
            mfxowniCopy16s_C3P3_A6( src+1, dst2, width );
            mfxowniCopy16s_C3P3_A6( src+2, dst3, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
    } else {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C3P3_A6_NT( src, dst1, width );
            mfxowniCopy16s_C3P3_A6_NT( src+1, dst2, width );
            mfxowniCopy16s_C3P3_A6_NT( src+2, dst3, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
    }
#else
    for( h = 0; h < height; h++ ) {
        for( wPL=0,wPX=0; wPL < width; wPL++, wPX+=3 ) {
            dst1[wPL] = src[wPX];
        }
        for( wPL=0,wPX=1; wPL < width; wPL++, wPX+=3 ) {
            dst2[wPL] = src[wPX];
        }
        for( wPL=0,wPX=2; wPL < width; wPL++, wPX+=3 ) {
            dst3[wPL] = src[wPX];
        }
        src = (Ipp16s*)((Ipp8u*)src + srcStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
    }
#endif
#endif /* ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1) */
    return ippStsNoErr;
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiCopy_16s_C4P4R,
                    ( const Ipp16s* pSrc, int srcStep,
                      Ipp16s* const pDst[4], int dstStep,
                      IppiSize roiSize )) {

#if !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 )
    const Ipp16s *src = pSrc;
    Ipp16s *dst1, *dst2, *dst3, *dst4;
    int height = roiSize.height;
    int width = roiSize.width;
    int h;
#if defined (_CCODE)|| defined (_M6) || defined (_I7)
    int wPL, wPX;
#endif
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    IppStatus ownStatus = ippStsNoErr;
    int nonTemporal = 0, size = 0, cache = 0;
#endif
#endif /* !(_IPPLRB >= _IPPLRB_B1) && !( _IPP >= _IPP_P8 ) && !( _IPP32E >= _IPP32E_Y8 ) */

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_PTR4_RET( pDst[0], pDst[1], pDst[2], pDst[3])
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1)
  #if (_IPPLRB >= _IPPLRB_B1)
    mfxowniCopy_16s_C4P4R_B1( pSrc, srcStep, pDst, dstStep, roiSize );
  #else
    mfxowniCopy_16s_C4P4R_cn( pSrc, srcStep, pDst, dstStep, roiSize );
  #endif
#else
#if defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8)) || ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    size = width*height*16;
    #if (_IPPLP32 >=_IPPLP32_S8)||(_IPPLP64 >=_IPPLP64_N8)
    if( size > BOUND_NT_ ) {
    #else
    if( size > BOUND_NT ) {
    #endif
      ownStatus = ippGetMaxCacheSizeB( &cache );
      if( ownStatus == ippStsNoErr  ) {
        if( size >= cache ) nonTemporal = 1;
      }
    }
#endif

    dst1 = pDst[0], dst2 = pDst[1], dst3 = pDst[2], dst4 = pDst[3];

#if ((_IPP32E >= _IPP32E_M7)&&(_IPP32E < _IPP32E_Y8))
    if( !nonTemporal ) {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C4P4_M7( src, dst1, width );
            mfxowniCopy16s_C4P4_M7( src+1, dst2, width );
            mfxowniCopy16s_C4P4_M7( src+2, dst3, width );
            mfxowniCopy16s_C4P4_M7( src+3, dst4, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
            dst4 = (Ipp16s*)((Ipp8u*)dst4 + dstStep);
        }
    } else {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C4P4_NT_M7( src, dst1, width );
            mfxowniCopy16s_C4P4_NT_M7( src+1, dst2, width );
            mfxowniCopy16s_C4P4_NT_M7( src+2, dst3, width );
            mfxowniCopy16s_C4P4_NT_M7( src+3, dst4, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
            dst4 = (Ipp16s*)((Ipp8u*)dst4 + dstStep);
        }
    }
#elif defined (_A6) || ((_IPP >= _IPP_W7)&&(_IPP < _IPP_P8))
    if( !nonTemporal ) {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C4P4_A6( src, dst1, width );
            mfxowniCopy16s_C4P4_A6( src+1, dst2, width );
            mfxowniCopy16s_C4P4_A6( src+2, dst3, width );
            mfxowniCopy16s_C4P4_A6( src+3, dst4, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
            dst4 = (Ipp16s*)((Ipp8u*)dst4 + dstStep);
        }
    } else {
        for( h = 0; h < height; h++ ) {
            mfxowniCopy16s_C4P4_A6_NT( src, dst1, width );
            mfxowniCopy16s_C4P4_A6_NT( src+1, dst2, width );
            mfxowniCopy16s_C4P4_A6_NT( src+2, dst3, width );
            mfxowniCopy16s_C4P4_A6_NT( src+3, dst4, width );
            src = (Ipp16s*)((Ipp8u*)src + srcStep);
            dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
            dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
            dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
            dst4 = (Ipp16s*)((Ipp8u*)dst4 + dstStep);
        }
    }
#else
    for( h = 0; h < height; h++ ) {
        for( wPL=0,wPX=0; wPL < width; wPL++, wPX+=4 ) {
            dst1[wPL] = src[wPX];
        }
        for( wPL=0,wPX=1; wPL < width; wPL++, wPX+=4 ) {
            dst2[wPL] = src[wPX];
        }
        for( wPL=0,wPX=2; wPL < width; wPL++, wPX+=4 ) {
            dst3[wPL] = src[wPX];
        }
        for( wPL=0,wPX=3; wPL < width; wPL++, wPX+=4 ) {
            dst4[wPL] = src[wPX];
        }
        src = (Ipp16s*)((Ipp8u*)src + srcStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        dst4 = (Ipp16s*)((Ipp8u*)dst4 + dstStep);
    }
#endif
#endif /* ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 ) || (_IPPLRB >= _IPPLRB_B1) */
    return ippStsNoErr;
}
