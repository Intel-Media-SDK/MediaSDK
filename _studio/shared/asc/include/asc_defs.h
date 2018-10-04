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
#ifndef __ASC_DEFS__
#define __ASC_DEFS__


#undef  NULL
#define NULL              0
#define OUT_BLOCK         16  // output pixels computed per thread
#define MVBLK_SIZE        8
#define BLOCK_SIZE        4
#define BLOCK_SIZE_SHIFT  2
#define NumTSC            10
#define NumSC             10
#define FLOAT_MAX         2241178.0
#define FRAMEMUL          16
#define CHROMASUBSAMPLE   4
#define ASC_SMALL_WIDTH   128
#define ASC_SMALL_HEIGHT  64
#define MAXLTRHISTORY     120
#define ASC_SMALL_AREA    8192//13 bits
#define S_AREA_SHIFT      13
#define TSC_INT_SCALE     5
#define GAINDIFF_THR      20

/*--MACROS--*/
#define NMAX(a,b)         ((a>b)?a:b)
#define NMIN(a,b)         ((a<b)?a:b)
#define NABS(a)           (((a)<0)?(-(a)):(a))
#define NAVG(a,b)         ((a+b)/2)

#define Clamp(x)           ((x<0)?0:((x>255)?255:x))
#define RF_DECISION_LEVEL 10

#define TSCSTATBUFFER     3
#define ASCVIDEOSTATSBUF  2

#define SCD_BLOCK_PIXEL_WIDTH   32
#define SCD_BLOCK_HEIGHT        8

//#define ASC_DEBUG

#if defined(ASC_DEBUG)
    #define ASC_PRINTF(...)     printf(__VA_ARGS__)
    #define ASC_FPRINTF(...)	fprintf(__VA_ARGS__)
    #define ASC_FFLUSH(x)       fflush(x)
#else
    #define ASC_PRINTF(...)
    #define ASC_FPRINTF(...)
    #define ASC_FFLUSH(x)
#endif


#define SCD_CHECK_CM_ERR(STS, ERR) if ((STS) != CM_SUCCESS) { ASC_PRINTF("FAILED at file: %s, line: %d, cmerr: %d\n", __FILE__, __LINE__, STS); return ERR; }
#define SCD_CHECK_MFX_ERR(STS) if ((STS) != MFX_ERR_NONE) { ASC_PRINTF("FAILED at file: %s, line: %d, mfxerr: %d\n", __FILE__, __LINE__, STS); return STS; }

#define ASC_ALIGN_DECL(X) __attribute__ ((aligned(X)))


#if (defined (__GNUC__))
#if (defined( __x86_64__ ) || defined( __ppc64__ ))
#define ARCH64
#else
#define ARCH32
#endif
#endif

#endif //__ASC_DEFS__