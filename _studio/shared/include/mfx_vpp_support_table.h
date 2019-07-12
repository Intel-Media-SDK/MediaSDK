// Copyright (c) 2019 Intel Corporation
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

#if defined (MFX_ENABLE_VPP)

#ifndef __MFX_VPP_SUPPORT_TABLE_H
#define __MFX_VPP_SUPPORT_TABLE_H
#include "mfxstructures.h"
#include <unordered_map>

// order in enumerator-lists below matters,
// enums are used to properly access VPPSupportTable
enum PlatformGen
{
    GEN6,
    GEN9,
    GEN11,
    NumGens
};

enum VPPFilter
{
    COLOR_CONVERSION,
    SCALING,
    BLENDING,
    DENOISE,
    DEINTERLACING,
    PROCAMP,
    DETAIL,
    MIRRORING,
    ROTATION,
    NumFilters
};

enum VPPSupportTableFourCC
{
    NV12,
    YV12,
    UYVY,
    YUY2,
    AYUV,
    RGB565,
    RGB4,
    RGBP,
    P010,
    Y210,
    Y410,
    A2RGB10,
    P016,
    Y216,
    Y416,
    NumFourCCFormats
};

const std::unordered_map<mfxU32, VPPSupportTableFourCC> MFXFourCC2VPPSupportTableFourCC
{
    {MFX_FOURCC_NV12, NV12},
    {MFX_FOURCC_YV12, YV12},
#if defined(MFX_VA_LINUX)
    {MFX_FOURCC_UYVY, UYVY},
#endif
    {MFX_FOURCC_YUY2, YUY2},
    {MFX_FOURCC_AYUV, AYUV},
#if defined (MFX_ENABLE_FOURCC_RGB565)
    {MFX_FOURCC_RGB565, RGB565},
#endif
    {MFX_FOURCC_RGB4, RGB4},
    {MFX_FOURCC_BGR4, RGB4 }, // the same constraints with MFX_FOURCC_RGB4
#ifdef MFX_ENABLE_RGBP
    {MFX_FOURCC_RGBP, RGBP},
#endif
    {MFX_FOURCC_P010, P010},
#if (MFX_VERSION >= 1027)
    {MFX_FOURCC_Y210, Y210},
    {MFX_FOURCC_Y410, Y410},
#endif
    {MFX_FOURCC_A2RGB10, A2RGB10},
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    {MFX_FOURCC_P016, P016},
    {MFX_FOURCC_Y216, Y216},
    {MFX_FOURCC_Y416, Y416}
#endif
};

const std::unordered_map<mfxU32, VPPFilter> VPPExtBuf2VPPSupportTableFilter
{
    {MFX_EXTBUFF_VPP_DENOISE, DENOISE},
    {MFX_EXTBUFF_VPP_DETAIL, DETAIL},
    {MFX_EXTBUFF_VPP_PROCAMP, PROCAMP},
    {MFX_EXTBUFF_VPP_ROTATION, ROTATION},
    {MFX_EXTBUFF_VPP_SCALING, SCALING},
    {MFX_EXTBUFF_VPP_MIRRORING, MIRRORING},
    {MFX_EXTBUFF_VPP_DEINTERLACING, DEINTERLACING},
    {MFX_EXTBUFF_VPP_COMPOSITE, BLENDING},
#if (MFX_VERSION >= 1025)
    {MFX_EXTBUFF_VPP_COLOR_CONVERSION, COLOR_CONVERSION},
#endif
};

constexpr mfxStatus Y = MFX_ERR_NONE, N = MFX_ERR_UNSUPPORTED;

const mfxStatus VPPSupportTable[NumGens][NumFilters][NumFourCCFormats][NumFourCCFormats] =
{
    {//gen6
        {//Color Format Conversion
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Scaling
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Blending
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Denoise
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Deinterlacing
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Procamp
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Detail
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Mirror
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Rotation
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
    },
    {//gen9
        {//Color Format Conversion
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Scaling
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Blending
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Denoise
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Deinterlacing
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Procamp
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    Y,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Detail
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Mirror
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Rotation
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    N,      Y,    Y,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   N,    N,    N,    N,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        }
    },
    {//gen11
        {//Color Format Conversion
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Scaling
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Blending
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Denoise
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Deinterlacing
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   AYUV*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    N,    N,    Y,    N,      N,    N,    N,    Y,    N,    N,       N,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Procamp
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /*   YV12*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   UYVY*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   YUY2*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /*   AYUV*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /* RGB565*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGB4*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   RGBP*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P010*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /*   Y210*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /*   Y410*/{   Y,    N,    N,    Y,    Y,      N,    Y,    N,    Y,    Y,    Y,       N,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Detail
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    N,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Mirror
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
        {//Rotation
            //   in\out NV12  YV12  UYVY  YUY2  AYUV  RGB565  RGB4  RGBP  P010  Y210  Y410  A2RGB10  P016  Y216  Y416
            /*   NV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YV12*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   UYVY*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   YUY2*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   AYUV*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /* RGB565*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGB4*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   RGBP*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   P010*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y210*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*   Y410*/{   Y,    Y,    N,    Y,    Y,      Y,    Y,    N,    Y,    Y,    Y,       Y,    N,    N,    N },
            /*A2RGB10*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   P016*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y216*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
            /*   Y416*/{   N,    N,    N,    N,    N,      N,    N,    N,    N,    N,    N,       N,    N,    N,    N },
        },
    },
};

#endif // __MFX_VPP_SUPPORT_TABLE_H
#endif // MFX_ENABLE_VPP
/* EOF */