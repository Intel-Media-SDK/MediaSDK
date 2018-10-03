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

#ifndef __PJQUANT_H__
#define __PJQUANT_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif



#define QUANT_BITS 15

#define IPPJ_QNT_OPT \
    ((_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7))


#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

ASMAPI(void,mfxownpj_QuantInv_8x8_16s_I,(
        Ipp16s* pSrcDst,
  const Ipp16u* pQTbl));

#endif

#if IPPJ_QNT_OPT || (_IPPXSC >= _IPPXSC_S2)

ASMAPI(void,mfxownpj_QuantInv_8x8_16s,(
        Ipp16s* pSrc,
        Ipp16s* pDst,
  const Ipp16u* pQTbl));

#endif

#if (_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPPXSC >= _IPPXSC_S2)

ASMAPI(void,mfxownpj_QuantFwd_8x8_Sfs15_16s,(
        Ipp16s*  pSrc,
        Ipp16s*  pDst,
  const Ipp16u*  pQTbl));

#endif

#endif /* __PJQUANT_H__ */
