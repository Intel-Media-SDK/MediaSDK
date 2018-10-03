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
//    Downsampling functions
//
*/

#ifndef __PJENCSS_H__
#define __PJENCSS_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


/* ---------------------- check input parameters --------------------------- */

#define IPP_BAD_ENC_SS_C1C1_RET() \
    IPP_BAD_PTR1_RET(pSrc) \
    IPP_BAD_STEP_RET(srcStep) \
    IPP_BAD_SIZE_RET(srcSize.width) \
    IPP_BAD_SIZE_RET(srcSize.height) \
    IPP_BAD_PTR1_RET(pDst) \
    IPP_BAD_STEP_RET(dstStep) \
    IPP_BAD_SIZE_RET(dstSize.width) \
    IPP_BAD_SIZE_RET(dstSize.height)


/* --------------------- external functions declarations ------------------ */

#define IPPJ_ENCSS_OPT \
    ((_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7) || (_IPPLRB>=_IPPLRB_B1))


#if IPPJ_ENCSS_OPT || (_IPPXSC >= _IPPXSC_S2)

ASMAPI(void,mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1,(
  const Ipp8u*,
        int,
        Ipp8u*));

ASMAPI(void,mfxownpj_SampleDownRowH2V2_Box_JPEG_8u_C1,(
  const Ipp8u*,
  const Ipp8u*,
        int,
        Ipp8u*));

#if IPPJ_ENCSS_OPT

ASMAPI(void,mfxownpj_SampleDownH2V1_JPEG_8u_C1,(
  const Ipp8u*,
        Ipp8u*,
        int));

ASMAPI(void,mfxownpj_SampleDownH2V2_JPEG_8u_C1,(
  const Ipp8u*,
        int,
        Ipp8u*,
        int));

#endif
#endif

#if (_IPP >= _IPP_W7)

ASMAPI(void,mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1_w7,(
  const Ipp8u*,
        int,
        Ipp8u*));

#endif


#endif /* __PJENCSS_H__ */
