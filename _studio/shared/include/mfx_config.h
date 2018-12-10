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

#ifndef _MFX_CONFIG_H_
#define _MFX_CONFIG_H_

#include "mfxdefs.h"
#if !defined(ANDROID)
    // mfxconfig.h is auto-generated file containing mediasdk per-component
    // enable defines
    #include "mfxconfig.h"
#endif

#define CMAPIUPDATE
#define MFX_ENABLE_VPP_COMPOSITION
//#define MFX_ENABLE_VPP_FRC
#define MFX_ENABLE_VPP_ROTATION
#define MFX_ENABLE_VPP_VIDEO_SIGNAL

#ifdef MFX_VA
    #if defined(LINUX32) || defined(LINUX64)
        #include <va/va_version.h>
        #undef  MFX_VA_LINUX
        #define MFX_VA_LINUX
    #endif
#endif

#if !defined(LINUX_TARGET_PLATFORM) || defined(LINUX_TARGET_PLATFORM_BDW) || defined(LINUX_TARGET_PLATFORM_CFL) || defined(LINUX_TARGET_PLATFORM_BXT)
    #if !defined(ANDROID)
        // HW decoders are part of library
        #define MFX_ENABLE_H264_VIDEO_DECODE
        
        #if MFX_VERSION >= 1025
            #if !defined(AS_H264LA_PLUGIN)
                #define MFX_ENABLE_MFE
            #endif
        #endif

        // vpp
        #define MFX_ENABLE_DENOISE_VIDEO_VPP
        #define MFX_ENABLE_VPP

        #if defined(AS_H264LA_PLUGIN)
            #define MFX_ENABLE_LA_H264_VIDEO_HW
        #endif

    #else // #if !defined(ANDROID)
        #include "mfx_android_defs.h"
    #endif // #if !defined(ANDROID)

    #if defined(AS_H264LA_PLUGIN)
        #undef MFX_ENABLE_H264_VIDEO_FEI_ENCODE
        #undef MFX_ENABLE_VPP
    #endif

#else // LINUX_TARGET_PLATFORM
    #if defined(LINUX_TARGET_PLATFORM_BXTMIN) // PRE_SI_GEN == 9
        #include "mfx_common_linux_bxtmin.h"
    #else
        #error "Target platform should be specified!"
    #endif
#endif // LINUX_TARGET_PLATFORM

// Here follows per-codec feature enable options which as of now we don't
// want to expose on build system level since they are too detailed.
#if defined(MFX_ENABLE_H264_VIDEO_ENCODE)
    #define MFX_ENABLE_H264_VIDEO_ENCODE_HW
    #if MFX_VERSION >= 1023
        #define MFX_ENABLE_H264_REPARTITION_CHECK
    #endif
    #if MFX_VERSION >= 1027
        #define MFX_ENABLE_H264_ROUNDING_OFFSET
    #endif
    #if defined(MFX_ENABLE_H264_VIDEO_FEI_ENCODE)
        #define MFX_ENABLE_H264_VIDEO_FEI_ENCPAK
        #define MFX_ENABLE_H264_VIDEO_FEI_PREENC
        #define MFX_ENABLE_H264_VIDEO_FEI_ENC
        #define MFX_ENABLE_H264_VIDEO_FEI_PAK
    #endif
#endif

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)
    #define MFX_ENABLE_HEVCE_INTERLACE
    #define MFX_ENABLE_HEVCE_ROI
    //#define MFX_ENABLE_HEVCE_DIRTY_RECT
    //#define MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION
    //#define MFX_ENABLE_HEVCE_FADE_DETECTION
#endif

#if defined(MFX_ENABLE_VP9_VIDEO_ENCODE)
    #define MFX_ENABLE_VP9_VIDEO_ENCODE_HW
#endif

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE)
#define MFX_ENABLE_VP9_VIDEO_DECODE_HW
#endif

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE)
#define MFX_ENABLE_VP8_VIDEO_DECODE_HW
#endif

#if defined(MFX_ENABLE_ASC)
    #define MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP
#endif

#if (MFX_VERSION >= MFX_VERSION_NEXT) && defined(MFX_ENABLE_MCTF)
    #define MFX_ENABLE_MCTF_EXT // extended MCTF interface
#endif

#if MFX_VERSION >= 1028
    #define MFX_ENABLE_RGBP
    #define MFX_ENABLE_FOURCC_RGB565
#endif

// The line below HAS to be changed to MFX_VERSION specific version i.e. 1027
// after inclusion of respective features into official API
#if MFX_VERSION >= MFX_VERSION_NEXT
    #define MFX_ENABLE_VPP_RUNTIME_HSBC
#endif

#if defined(MFX_VA_LINUX)
    #if VA_CHECK_VERSION(1,3,0)
        #define MFX_ENABLE_QVBR
    #endif
#endif

#endif // _MFX_CONFIG_H_
