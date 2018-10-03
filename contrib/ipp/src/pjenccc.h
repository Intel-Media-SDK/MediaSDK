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
//
//  Purpose:
//    Color conversions
//    (Forward transform)
//
//  Contents:
//
*/

#ifndef __PJENCCC_H__
#define __PJENCCC_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif

/* ---------------------- check input parameters --------------------------- */

#define IPP_BAD_ENC_CC_C3C1_RET() \
    IPP_BAD_PTR1_RET(pSrc) \
    IPP_BAD_PTR1_RET(pDst) \
    IPP_BAD_STEP_RET(SrcStep) \
    IPP_BAD_STEP_RET(DstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)

#define IPP_BAD_ENC_CC_P3P3_RET() \
    IPP_BAD_PTR2_RET(pSrc, pDst) \
    IPP_BAD_PTR3_RET(pSrc[0], pSrc[1], pSrc[2]) \
    IPP_BAD_PTR3_RET(pDst[0], pDst[1], pDst[2]) \
    IPP_BAD_STEP_RET(SrcStep) \
    IPP_BAD_STEP_RET(DstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)

#define IPP_BAD_ENC_CC_C3P3_RET() \
    IPP_BAD_PTR2_RET(pSrc, pDst) \
    IPP_BAD_PTR3_RET(pDst[0], pDst[1], pDst[2]) \
    IPP_BAD_STEP_RET(SrcStep) \
    IPP_BAD_STEP_RET(DstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)

#define IPP_BAD_ENC_CC_C4P4_RET() \
    IPP_BAD_PTR2_RET(pSrc, pDst) \
    IPP_BAD_PTR4_RET(pDst[0], pDst[1], pDst[2], pDst[3]) \
    IPP_BAD_STEP_RET(SrcStep) \
    IPP_BAD_STEP_RET(DstStep) \
    IPP_BAD_SIZE_RET(roiSize.width) \
    IPP_BAD_SIZE_RET(roiSize.height)



#endif /* __PJENCCC_H__ */
