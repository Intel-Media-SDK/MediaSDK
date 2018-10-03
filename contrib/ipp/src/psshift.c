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
//  Description:
//      Functions of shift (mfxs_ domain)
//
//      mfxsLShiftC_16u
//      mfxsLShiftC_16s
//
*/

#include "precomp.h"
#include "owns.h"

#define IPP_BAD_SHIFT1_RET() \
    IPP_BAD_PTR1_RET(pSrcDst) \
    IPP_BAD_SIZE_RET(len) \
    IPP_BADARG_RET(val < 0, ippStsShiftErr)
#define IPP_BAD_SHIFT2_RET() \
    IPP_BAD_PTR2_RET(pSrc, pDst) \
    IPP_BAD_SIZE_RET(len) \
    IPP_BADARG_RET(val < 0, ippStsShiftErr)


#define IPP_SHIFT_OPT \
    ((_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7) || (_IPPXSC >= _IPPXSC_S2))

#define _OPT_HSW \
    ((_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9))
    

#if IPP_SHIFT_OPT
extern void mfxownps_LShiftC_16u(const Ipp16u*, int, Ipp16u*, int);
#endif


/* ============================= LSHIFT ============================ */


IPPFUN(IppStatus, mfxsLShiftC_16u, (const Ipp16u* __RESTRICT pSrc, int val, Ipp16u* __RESTRICT pDst, int len))
{
    if (!val)
        return mfxsCopy_16s((const Ipp16s*)pSrc, (Ipp16s*)pDst, len);
    if (val > 15) {
        IPP_BAD_PTR1_RET(pSrc)
        return mfxsZero_16s((Ipp16s*)pDst, len);
    }

    IPP_BAD_SHIFT2_RET()

#if IPP_SHIFT_OPT && !_OPT_HSW
    mfxownps_LShiftC_16u(pSrc, val, pDst, len);
#else
    {
    int i;
    for (i = 0; i < len; i ++)
        pDst[i] = (Ipp16u)(pSrc[i] << val);
    }
#endif

    return ippStsNoErr;
}

IPPFUN(IppStatus, mfxsLShiftC_16s, (const Ipp16s* __RESTRICT pSrc, int val, Ipp16s* __RESTRICT pDst, int len))
{
    return mfxsLShiftC_16u((const Ipp16u*)pSrc, val, (Ipp16u*)pDst, len);
}

