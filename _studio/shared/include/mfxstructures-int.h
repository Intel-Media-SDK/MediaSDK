// Copyright (c) 2007-2020 Intel Corporation
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

#ifndef __MFXSTRUCTURES_INT_H__
#define __MFXSTRUCTURES_INT_H__
#include "mfxstructures.h"
#include "mfx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum eMFXPlatform
{
    MFX_PLATFORM_SOFTWARE      = 0,
    MFX_PLATFORM_HARDWARE      = 1,
};

enum eMFXVAType
{
    MFX_HW_NO       = 0,
    MFX_HW_D3D9     = 1,
    MFX_HW_D3D11    = 2,
    MFX_HW_VAAPI    = 4,// Linux VA-API
};

enum eMFXHWType
{
    MFX_HW_UNKNOWN   = 0,
    MFX_HW_SNB       = 0x300000,

    MFX_HW_IVB       = 0x400000,

    MFX_HW_HSW       = 0x500000,
    MFX_HW_HSW_ULT   = 0x500001,

    MFX_HW_VLV       = 0x600000,

    MFX_HW_BDW       = 0x700000,

    MFX_HW_CHT       = 0x800000,

    MFX_HW_SCL       = 0x900000,

    MFX_HW_APL       = 0x1000000,

    MFX_HW_KBL       = 0x1100000,
    MFX_HW_GLK       = MFX_HW_KBL + 1,
    MFX_HW_CFL       = MFX_HW_KBL + 2,

    MFX_HW_CNL       = 0x1200000,
    MFX_HW_ICL       = 0x1400000,
    MFX_HW_ICL_LP    = MFX_HW_ICL + 1,

    MFX_HW_JSL       = 0x1500001,
    MFX_HW_EHL       = 0x1500002,
    
    MFX_HW_TGL_LP    = 0x1600000,
    MFX_HW_RKL       = MFX_HW_TGL_LP + 2,
    MFX_HW_ADL_S     = MFX_HW_TGL_LP + 4,
    MFX_HW_DG1       = 0x1600003,


};

enum eMFXGTConfig
{
    MFX_GT_UNKNOWN = 0,
    MFX_GT1     = 1,
    MFX_GT2     = 2,
    MFX_GT3     = 3,
    MFX_GT4     = 4
};

// mfxU8 CodecProfile, CodecLevel
/*
Some components (samples, JPEG decoder) has used MFX_FOURCC_RGBP already.
So, for API 1.27 and below "MFX_FOURCC_RGBP" defined inside of msdk library
and samples.
Since next version of API (1.28) "MFX_FOURCC_RGBP" should officially
defined in API "mfxstructures.h".
*/
enum
{
    MFX_FOURCC_IMC3         = MFX_MAKEFOURCC('I','M','C','3'),
    MFX_FOURCC_YUV400       = MFX_MAKEFOURCC('4','0','0','P'),
    MFX_FOURCC_YUV411       = MFX_MAKEFOURCC('4','1','1','P'),
    MFX_FOURCC_YUV422H      = MFX_MAKEFOURCC('4','2','2','H'),
    MFX_FOURCC_YUV422V      = MFX_MAKEFOURCC('4','2','2','V'),
    MFX_FOURCC_YUV444       = MFX_MAKEFOURCC('4','4','4','P'),
#if (MFX_VERSION <= 1027)
    MFX_FOURCC_RGBP         = MFX_MAKEFOURCC('R','G','B','P'),
#else
#endif
};

#ifdef __cplusplus
};
#endif

#endif // __MFXSTRUCTURES_H
