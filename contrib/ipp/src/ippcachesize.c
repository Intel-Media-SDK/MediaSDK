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

#include <stdlib.h>

#ifndef __OWNCORE_H__
#include "owncore.h"
#endif

#ifndef __CPUDEF_H__
#include "cpudef.h"
#endif

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       mfxGetMaxCacheSizeB
//
//  Purpose:  Definition maximal from the sizes of L2 or L3 in bytes
//
//  Note:
//    1). Intel(R) processors are supported only.
//    2). IA64 and XSC are unsupported.
//    3). For unsupported processors the result is "0",
//        and the return status is "ippStsNotSupportedCpu".
//    4). For supported processors the result is "0",
//        and the return status is "ippStsUnknownCacheSize".
//        if sizes of the cache is unknown.
*/
#if !defined( _ARCH_IA64 ) && !defined( _ARCH_XSC )
static IppStatus ownStatus = ippStsNoOperation;
static int cacheSize = 0;
static int tableCache[112] = {
0xec,25165824,/* 24, 64, L3, from doc cpuid for Nehalem */
0xeb,18874368,/* 24, 64, L3, from doc cpuid for Nehalem */
0x4d,16777216,/* 16, 64, L3 */
0xea,12582912,/* 24, 64, L3, from doc cpuid for Nehalem */
0x4c,12582912,/* 12, 64, L3 */
0xe4, 8388608,/* 16, 64, L3, from doc cpuid for Nehalem */
0xde, 8388608,/* 12, 64, L3, from doc cpuid for Nehalem */
0x4b, 8388608,/* 16, 64, L3 */
0x47, 8388608,/*  8, 64, L3 */
0x4e, 6291456,/* 24, 64, L3 */
0x4a, 6291456,/* 12, 64, L3 */
0xe3, 4194304,/* 16, 64, L3, from doc cpuid for Nehalem */
0xdd, 4194304,/* 12, 64, L3, from doc cpuid for Nehalem */
0xd8, 4194304,/*  8, 64, L3, from doc cpuid for Nehalem */
0x49, 4194304,/* 16, 64, L3 */
0x29, 4194304,/*  8, 64, L3 */
0x46, 4194304,/*  4, 64, L3 */
0x48, 3145728,/* 12, 64, L2 */
0xe2, 2097152,/* 16, 64, L3, from doc cpuid for Nehalem */
0xdc, 2097152,/* 12, 64, L3, from doc cpuid for Nehalem */
0xd7, 2097152,/*  8, 64, L3, from doc cpuid for Nehalem */
0xd2, 2097152,/*  4, 64, L3, from doc cpuid for Nehalem */
0x25, 2097152,/*  8, 64, L3 */
0x7d, 2097152,/*  8, 64, L2 */
0x85, 2097152,/*  8, 32, L2 */
0x45, 2097152,/*  4, 32, L2 */
0xd6, 1048576,/*  8, 64, L3, from doc cpuid for Nehalem */
0xd1, 1048576,/*  4, 64, L3, from doc cpuid for Nehalem */
0x23, 1048576,/*  8, 64, L3 */
0x87, 1048576,/*  8, 64, L2 */
0x7c, 1048576,/*  8, 64, L2 */
0x78, 1048576,/*  4, 64, L2 */
0x84, 1048576,/*  8, 32, L2 */
0x44, 1048576,/*  4, 32, L2 */
0xd0,  524288,/*  4, 64, L3, from doc cpuid for Nehalem */
0x22,  524288,/*  4, 64, L3 */
0x7b,  524288,/*  8, 64, L2 */
0x80,  524288,/*  8, 64, L2 */
0x86,  524288,/*  4, 64, L2 */
0x3e,  524288,/*  4, 64, L2 */
0x7f,  524288,/*  2, 64, L2 */
0x83,  524288,/*  8, 32, L2 */
0x43,  524288,/*  4, 32, L2 */
0x3d,  393216,/*  6, 64, L2 */
0x21,  262144,/*  8, 64, L2, from doc cpuid for Nehalem */
0x7a,  262144,/*  8, 64, L2 */
0x82,  262144,/*  8, 32, L2 */
0x3c,  262144,/*  4, 64, L2 */
0x3f,  262144,/*  4, 64, L2 */
0x42,  262144,/*  4, 32, L2 */
0x3a,  196608,/*  6, 64, L2 */
0x79,  131072,/*  8, 64, L2 */
0x39,  131072,/*  4, 64, L2 */
0x3b,  131072,/*  2, 64, L2 */
0x41,  131072,/*  4, 32, L2 */
0x00,       0
};
extern int mfxownGetCacheSize( int* tableCache );
#endif
/***********************************************************/
IPPFUN( IppStatus, mfxGetMaxCacheSizeB, ( int* pSizeByte ) )
{
  IPP_BAD_PTR1_RET( pSizeByte )

#if defined( _ARCH_IA64 )
  *pSizeByte = 0;
  return ippStsNotSupportedCpu;
#elif defined( _ARCH_XSC )
  *pSizeByte = 0;
  return ippStsNotSupportedCpu;
#elif defined( _ARCH_LRB ) || defined( _ARCH_LRB2 )
  {
  int buf[4];
  if( ownStatus == ippStsNoOperation ) {
    mfxownGetReg( buf, 0x80000000, 0 );
    if( buf[0] < 0x80000006 ) {
      ownStatus = ippStsUnknownCacheSize;
      cacheSize = 0;
    } else {
      mfxownGetReg( buf, 0x80000006, 0 );
      cacheSize = (buf[2] >> 16)*1024;
      ownStatus = ippStsNoErr;
    }
  } 
  *pSizeByte = cacheSize;
  return ownStatus;
  }
#else
  {
  int maxNumberCpuid = 0;
  int buf[4], n, tmpMaxSise = 0;
  if( ownStatus == ippStsNoOperation ) {
    if( !(mfxhas_cpuid()) ) {
      ownStatus = ippStsNotSupportedCpu;
    } else if( ( maxNumberCpuid = mfxmax_cpuid_input() ) < 2 ) {
      ownStatus = ippStsNotSupportedCpu;
    } else if( !(mfxis_GenuineIntel()) ) {
      ownStatus = ippStsNotSupportedCpu;
    } else {
      if( maxNumberCpuid >= 4 ) {
      /*if( 0 ) {*/
        for( n = 0; n < 32; n++ ) {
          mfxownGetReg( buf, 4, n );
          if( !(buf[0]&0x1f) ) break;
          if( (buf[0]&0x1f) != 2 ) { /* 2 = Instraction cache */
            buf[2] += 1;
            buf[2] *= (buf[1]&0xfff)+1;
            buf[2] *= ((buf[1]>>12)&0x3ff)+1;
            buf[2] *= ((buf[1]>>22)&0x3ff)+1;
            if( buf[2] > tmpMaxSise ) tmpMaxSise = buf[2];
          }
        }
        if( tmpMaxSise ) {
          ownStatus = ippStsNoErr;
          cacheSize = tmpMaxSise;
        } else {
          ownStatus = ippStsUnknownCacheSize;
          cacheSize = 0;
        }
      } else { 
        cacheSize = mfxownGetCacheSize( tableCache );
        if( cacheSize < 0 ) {
          ownStatus = ippStsUnknownCacheSize;
          cacheSize = 0;
        } else {
          ownStatus = ippStsNoErr;
        }
      }
    }
  }
  *pSizeByte = cacheSize;
  return ownStatus;
  }
#endif
}
