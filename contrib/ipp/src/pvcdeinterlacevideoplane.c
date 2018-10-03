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
//  Function(s) to deinterlace video image(s)
//
//
*/

#include "precomp.h"
#include "pvcown.h"
#include "ownvc.h"
#include "pvcmacro.h"
#include "pvccommonfunctions.h"

#if (_IPP >= _IPP_A6)

#ifndef imp_mfxiDeinterlaceFilterTriangle_8u_C1R
#define imp_mfxiDeinterlaceFilterTriangle_8u_C1R

IPPFUN(IppStatus, mfxiDeinterlaceFilterTriangle_8u_C1R, (const Ipp8u *pSrc,
                                                         Ipp32s srcStep,
                                                         Ipp8u *pDst,
                                                         Ipp32s dstStep,
                                                         IppiSize roiSize,
                                                         Ipp32u centerWeight,
                                                         Ipp32u layout))
{
    Ipp32u edgeWeight;

    /* check error(s) */
    IPP_BAD_PTR2_RET(pSrc, pDst);
    if ((3 > roiSize.height) || (1 > roiSize.width))
        return ippStsSizeErr;

    /* create pixel's weight(s) */
    if (256 < centerWeight)
        centerWeight = 256;
    edgeWeight = (256 - centerWeight) / 2;

    switch (layout)
    {
        /* process upper slice */
    case IPP_UPPER:
        if (16 > roiSize.width)
        {
            IppiSize size;
            Ipp32s x, y;

            /* skip upper row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);

            /* set new source line */
            pSrc += srcStep;
            pDst += dstStep;

            /* process all lines except last one */
            for (y = 1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };
        }
        else
            deinterlace_filter_triangle_upper_slice(pSrc,
                                                    srcStep,
                                                    pDst,
                                                    dstStep,
                                                    roiSize.width,
                                                    roiSize.height,
                                                    centerWeight);
        break;

        /* process center slice */
    case IPP_CENTER:
        if (16 > roiSize.width)
        {
            Ipp32s x, y;

            /* we need to process last line from previos slice */
            pSrc -= srcStep;
            pDst -= dstStep;

            /* process all lines except last one */
            for (y = -1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                }

                pSrc += srcStep;
                pDst += dstStep;
            }
        }
        else
            deinterlace_filter_triangle_middle_slice(pSrc,
                                                     srcStep,
                                                     pDst,
                                                     dstStep,
                                                     roiSize.width,
                                                     roiSize.height,
                                                     centerWeight);
        break;

        /* process lower slice */
    case IPP_LOWER:
        if (16 > roiSize.width)
        {
            IppiSize size;
            Ipp32s x, y;

            /* we need to process last line from previos slice */
            pSrc -= srcStep;
            pDst -= dstStep;

            /* process all lines except last one */
            for (y = -1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            }

            /* skip lower row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);
        }
        else
            deinterlace_filter_triangle_lower_slice(pSrc,
                                                    srcStep,
                                                    pDst,
                                                    dstStep,
                                                    roiSize.width,
                                                    roiSize.height,
                                                    centerWeight);
        break;

        /* image is not sliced */
    default:
        if (16 > roiSize.width)
        {
            IppiSize size;
            Ipp32s x, y;

            /* skip upper row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);

            /* set new source line */
            pSrc += srcStep;
            pDst += dstStep;

            /* process all lines except last one */
            for (y = 1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };

            /* skip lower row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);
        }
        else
            deinterlace_filter_triangle(pSrc,
                                        srcStep,
                                        pDst,
                                        dstStep,
                                        roiSize.width,
                                        roiSize.height,
                                        centerWeight);
        break;
    };

    return ippStsOk;

} /* IPPFUN(IppStatus, mfxiDeinterlaceFilterTriangle_8u_C1R, (const Ipp8u *pSrc, */

#endif /* imp_mfxiDeinterlaceFilterTriangle_8u_C1R */
#endif /* (_IPP >= _IPP_A6) */


#ifndef imp_mfxiDeinterlaceFilterTriangle_8u_C1R
#define imp_mfxiDeinterlaceFilterTriangle_8u_C1R

IPPFUN(IppStatus, mfxiDeinterlaceFilterTriangle_8u_C1R, (const Ipp8u *pSrc,
                                                         Ipp32s srcStep,
                                                         Ipp8u *pDst,
                                                         Ipp32s dstStep,
                                                         IppiSize roiSize,
                                                         Ipp32u centerWeight,
                                                         Ipp32u layout))
{
    Ipp32u edgeWeight;

    /* check error(s) */
    IPP_BAD_PTR2_RET(pSrc, pDst);
    if ((3 > roiSize.height) || (1 > roiSize.width))
        return ippStsSizeErr;

    /* create pixel's weight(s) */
    if (256 < centerWeight)
        centerWeight = 256;
    edgeWeight = (256 - centerWeight) / 2;

    switch (layout)
    {
        /* process upper slice */
    case IPP_UPPER:
        {
            IppiSize size;
            Ipp32s x, y;

            /* skip upper row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);

            /* set new source line */
            pSrc += srcStep;
            pDst += dstStep;

            /* process all lines except last one */
            for (y = 1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };
        };
        break;

        /* process center slice */
    case IPP_CENTER:
        {
            Ipp32s x, y;

            /* we need to process last line from previos slice */
            pSrc -= srcStep;
            pDst -= dstStep;

            /* process all lines except last one */
            for (y = -1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };
        };
        break;

        /* process lower slice */
    case IPP_LOWER:
        {
            IppiSize size;
            Ipp32s x, y;

            /* we need to process last line from previos slice */
            pSrc -= srcStep;
            pDst -= dstStep;

            /* process all lines except last one */
            for (y = -1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };

            /* skip lower row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);
        };
        break;

        /* image is not sliced */
    default:
        {
            IppiSize size;
            Ipp32s x, y;

            /* skip upper row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);

            /* set new source line */
            pSrc += srcStep;
            pDst += dstStep;

            /* process all lines except last one */
            for (y = 1;y < (signed) (roiSize.height - 1);y += 1)
            {
                for (x = 0;x < (signed) roiSize.width;x += 1)
                {
                    pDst[x] = (Ipp8u) (((pSrc[x - srcStep] + pSrc[x + srcStep]) * edgeWeight +
                                        pSrc[x] * centerWeight) / 256);
                };

                pSrc += srcStep;
                pDst += dstStep;
            };

            /* skip lower row */
            size.width = roiSize.width;
            size.height = 1;
            mfxiCopy_8u_C1R(pSrc, srcStep,
                            pDst, dstStep,
                            size);
        };
        break;
    };

    return ippStsOk;

} /* IPPFUN(IppStatus, mfxiDeinterlaceFilterTriangle_8u_C1R, (const Ipp8u *pSrc, */

#endif /* imp_mfxiDeinterlaceFilterTriangle_8u_C1R */
