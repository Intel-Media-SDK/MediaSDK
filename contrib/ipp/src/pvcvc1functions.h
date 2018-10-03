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
//          Declaration of optimized function(s) for VC1
//
*/

#ifndef __PVC_VC1_FUNCTIONS_H
#define __PVC_VC1_FUNCTIONS_H

#include "pvcown.h"

/* declaration of optimized function(s) */


/* for ippiFilterDeblockingLuma/Chroma_VerEdge_MBAFF_H264_8u_C1IR functions */
#if ((_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7))
/* VC1 rangemapping function */
void mfxrangemapping_vc1_sse2(Ipp8u*,Ipp32s,Ipp8u*,Ipp32s,Ipp32s,Ipp32s,Ipp32s);

#endif /* ((_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)) */

#endif /* __PVC_VC1_FUNCTIONS_H */
