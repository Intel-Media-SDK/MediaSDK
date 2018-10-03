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
//      Optimized function(s) definitions
//
*/

#ifndef __PVC_COMMON_FUNCTIONS_H
#define __PVC_COMMON_FUNCTIONS_H

#include "ownvc.h"

#if (_IPP >= _IPP_A6)

#undef deinterlace_filter_triangle_upper_slice
#define deinterlace_filter_triangle_upper_slice deinterlace_filter_triangle_upper_slice_mmp
void deinterlace_filter_triangle_upper_slice_mmp(const Ipp8u *pSrc,
                                                 SIZE_T srcStep,
                                                 Ipp8u *pDst,
                                                 SIZE_T dstStep,
                                                 Ipp32u nWidth,
                                                 Ipp32u nHeight,
                                                 Ipp32u centerWeight);

#undef deinterlace_filter_triangle_middle_slice
#define deinterlace_filter_triangle_middle_slice deinterlace_filter_triangle_middle_slice_mmp
void deinterlace_filter_triangle_middle_slice_mmp(const Ipp8u *pSrc,
                                                  SIZE_T srcStep,
                                                  Ipp8u *pDst,
                                                  SIZE_T dstStep,
                                                  Ipp32u nWidth,
                                                  Ipp32u nHeight,
                                                  Ipp32u centerWeight);

#undef deinterlace_filter_triangle_lower_slice
#define deinterlace_filter_triangle_lower_slice deinterlace_filter_triangle_lower_slice_mmp
void deinterlace_filter_triangle_lower_slice_mmp(const Ipp8u *pSrc,
                                                 SIZE_T srcStep,
                                                 Ipp8u *pDst,
                                                 SIZE_T dstStep,
                                                 Ipp32u nWidth,
                                                 Ipp32u nHeight,
                                                 Ipp32u centerWeight);

#undef deinterlace_filter_triangle
#define deinterlace_filter_triangle deinterlace_filter_triangle_mmp
void deinterlace_filter_triangle_mmp(const Ipp8u *pSrc,
                                     SIZE_T srcStep,
                                     Ipp8u *pDst,
                                     SIZE_T dstStep,
                                     Ipp32u nWidth,
                                     Ipp32u nHeight,
                                     Ipp32u centerWeight);

#endif /* (_IPP >= _IPP_A6) */


#if (_IPP >= _IPP_W7)

#undef deinterlace_filter_triangle_upper_slice
#define deinterlace_filter_triangle_upper_slice deinterlace_filter_triangle_upper_slice_sse2
void deinterlace_filter_triangle_upper_slice_sse2(const Ipp8u *pSrc,
                                                  SIZE_T srcStep,
                                                  Ipp8u *pDst,
                                                  SIZE_T dstStep,
                                                  Ipp32u nWidth,
                                                  Ipp32u nHeight,
                                                  Ipp32u centerWeight);

#undef deinterlace_filter_triangle_middle_slice
#define deinterlace_filter_triangle_middle_slice deinterlace_filter_triangle_middle_slice_sse2
void deinterlace_filter_triangle_middle_slice_sse2(const Ipp8u *pSrc,
                                                   SIZE_T srcStep,
                                                   Ipp8u *pDst,
                                                   SIZE_T dstStep,
                                                   Ipp32u nWidth,
                                                   Ipp32u nHeight,
                                                   Ipp32u centerWeight);

#undef deinterlace_filter_triangle_lower_slice
#define deinterlace_filter_triangle_lower_slice deinterlace_filter_triangle_lower_slice_sse2
void deinterlace_filter_triangle_lower_slice_sse2(const Ipp8u *pSrc,
                                                  SIZE_T srcStep,
                                                  Ipp8u *pDst,
                                                  SIZE_T dstStep,
                                                  Ipp32u nWidth,
                                                  Ipp32u nHeight,
                                                  Ipp32u centerWeight);

#undef deinterlace_filter_triangle
#define deinterlace_filter_triangle deinterlace_filter_triangle_sse2
void deinterlace_filter_triangle_sse2(const Ipp8u *pSrc,
                                      SIZE_T srcStep,
                                      Ipp8u *pDst,
                                      SIZE_T dstStep,
                                      Ipp32u nWidth,
                                      Ipp32u nHeight,
                                      Ipp32u centerWeight);

#endif /* (_IPP >= _IPP_W7) */


#if (_IPP >= _IPP_T7)

#undef deinterlace_filter_triangle_upper_slice
#define deinterlace_filter_triangle_upper_slice deinterlace_filter_triangle_upper_slice_sse3
void deinterlace_filter_triangle_upper_slice_sse3(const Ipp8u *pSrc,
                                                  SIZE_T srcStep,
                                                  Ipp8u *pDst,
                                                  SIZE_T dstStep,
                                                  Ipp32u nWidth,
                                                  Ipp32u nHeight,
                                                  Ipp32u centerWeight);

#undef deinterlace_filter_triangle_middle_slice
#define deinterlace_filter_triangle_middle_slice deinterlace_filter_triangle_middle_slice_sse3
void deinterlace_filter_triangle_middle_slice_sse3(const Ipp8u *pSrc,
                                                   SIZE_T srcStep,
                                                   Ipp8u *pDst,
                                                   SIZE_T dstStep,
                                                   Ipp32u nWidth,
                                                   Ipp32u nHeight,
                                                   Ipp32u centerWeight);

#undef deinterlace_filter_triangle_lower_slice
#define deinterlace_filter_triangle_lower_slice deinterlace_filter_triangle_lower_slice_sse3
void deinterlace_filter_triangle_lower_slice_sse3(const Ipp8u *pSrc,
                                                  SIZE_T srcStep,
                                                  Ipp8u *pDst,
                                                  SIZE_T dstStep,
                                                  Ipp32u nWidth,
                                                  Ipp32u nHeight,
                                                  Ipp32u centerWeight);

#undef deinterlace_filter_triangle
#define deinterlace_filter_triangle deinterlace_filter_triangle_sse3
void deinterlace_filter_triangle_sse3(const Ipp8u *pSrc,
                                      SIZE_T srcStep,
                                      Ipp8u *pDst,
                                      SIZE_T dstStep,
                                      Ipp32u nWidth,
                                      Ipp32u nHeight,
                                      Ipp32u centerWeight);

#endif /* (_IPP >= _IPP_T7) */

#endif /* __PVC_COMMON_FUNCTIONS_H */
