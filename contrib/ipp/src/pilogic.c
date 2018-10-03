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
//  Description:
//      Logical functions (152)(mfxi domain)
//
//      mfxiAndC_8u/16u/32s_C1R/C3R/C4R/AC4R/C1IR/C3IR/C4IR/AC4IR
//
*/

#include "precomp.h"
#include "owni.h"


#define IPP_BAD_LOGICV_RET() \
    IPP_BAD_PTR3_RET(pSrc, value, pDst) \
    IPP_BAD_STEP_RET(srcStep) \
    IPP_BAD_STEP_RET(dstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)
#define IPP_BAD_LOGIC2_RET() \
    IPP_BAD_PTR2_RET(pSrc, pDst) \
    IPP_BAD_STEP_RET(srcStep) \
    IPP_BAD_STEP_RET(dstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)
#define IPP_BAD_LOGIC3_RET() \
    IPP_BAD_PTR3_RET(pSrc1, pSrc2, pDst) \
    IPP_BAD_STEP_RET(src1Step) \
    IPP_BAD_STEP_RET(src2Step) \
    IPP_BAD_STEP_RET(dstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)


#define IPP_LOGIC_OPT \
    ((_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7))
#define IPP_LOGIC_OPT1 \
    ((_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7))
/*
#define IPP_LOGIC_OPT_HSW ((_IPP>=_IPP_H9)||(_IPP32E >= _IPP32E_L9))
*/

#if IPP_LOGIC_OPT
extern void mfxownpi_AndC_8u_C1R(Ipp8u, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_AndC_8u_C3R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_AndC_8u_C4R(const Ipp8u[4], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_AndC_8u_AC4R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_And_8u_C1R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_And_8u_AC4R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_AndC_16u_C1R(Ipp16u, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_AndC_16u_C3R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_AndC_16u_C4R(const Ipp16u[4], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_AndC_16u_AC4R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_And_16u_C1R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_And_16u_AC4R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_AndC_32s_C1R(Ipp32s, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_AndC_32s_C3R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
//extern void mfxownpi_AndC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_AndC_32s_AC4R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_And_32s_C1R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_And_32s_AC4R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_OrC_8u_C1R(Ipp8u, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_OrC_8u_C3R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_OrC_8u_C4R(const Ipp8u[4], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_OrC_8u_AC4R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_Or_8u_C1R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_Or_8u_AC4R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_OrC_16u_C1R(Ipp16u, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_OrC_16u_C3R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_OrC_16u_C4R(const Ipp16u[4], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_OrC_16u_AC4R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_Or_16u_C1R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_Or_16u_AC4R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_OrC_32s_C1R(Ipp32s, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_OrC_32s_C3R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
//extern void mfxownpi_OrC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_OrC_32s_AC4R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_Or_32s_C1R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_Or_32s_AC4R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_XorC_8u_C1R(Ipp8u, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_XorC_8u_C3R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_XorC_8u_C4R(const Ipp8u[4], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_XorC_8u_AC4R(const Ipp8u[3], const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_Xor_8u_C1R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_Xor_8u_AC4R(const Ipp8u*, int, const Ipp8u*, int, Ipp8u*, int, int, int);
extern void mfxownpi_XorC_16u_C1R(Ipp16u, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_XorC_16u_C3R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_XorC_16u_C4R(const Ipp16u[4], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_XorC_16u_AC4R(const Ipp16u[3], const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_Xor_16u_C1R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_Xor_16u_AC4R(const Ipp16u*, int, const Ipp16u*, int, Ipp16u*, int, int, int);
extern void mfxownpi_XorC_32s_C1R(Ipp32s, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_XorC_32s_C3R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
//extern void mfxownpi_XorC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_XorC_32s_AC4R(const Ipp32s[3], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_Xor_32s_C1R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_Xor_32s_AC4R(const Ipp32s*, int, const Ipp32s*, int, Ipp32s*, int, int, int);
#endif
#if IPP_LOGIC_OPT1
extern void mfxownpi_AndC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_OrC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
extern void mfxownpi_XorC_32s_C4R(const Ipp32s[4], const Ipp32s*, int, Ipp32s*, int, int, int);
#endif

/* ================================== AND ================================== */


IPPFUN(IppStatus, mfxiAndC_16u_C1R, (const Ipp16u* pSrc, int srcStep, Ipp16u value,
                                     Ipp16u* pDst, int dstStep, IppiSize roiSize))
{
    IPP_BAD_LOGIC2_RET();

#if !IPP_LOGIC_OPT || IPP_LOGIC_OPT_HSW
    {
    int     i, j;
/*
    Ipp16u *src, *dst;
*/

    for (j = 0; j < roiSize.height; j ++) {
#if IPP_LOGIC_OPT_HSW
#pragma unroll(2)
#endif
        for (i = 0; i < roiSize.width; i ++)
            pDst[i] = value & pSrc[i];
        pSrc = (Ipp16u*)((Ipp8u*)pSrc + srcStep);
        pDst = (Ipp16u*)((Ipp8u*)pDst + dstStep);

    }
    }
#else
    mfxownpi_AndC_16u_C1R(value, pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height);
#endif

    return ippStsNoErr;
}


IPPFUN(IppStatus, mfxiAndC_16u_C1IR, (Ipp16u value, Ipp16u* pSrcDst, int srcDstStep, IppiSize roiSize))
{
    return mfxiAndC_16u_C1R(pSrcDst, srcDstStep, value, pSrcDst, srcDstStep, roiSize);
}


