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

#define CMAPIUPDATE
#define MFX_ENABLE_VPP_COMPOSITION
//#define MFX_ENABLE_VPP_FRC
#define MFX_ENABLE_VPP_ROTATION
#define MFX_ENABLE_VPP_VIDEO_SIGNAL




    #if defined(LINUX32) || defined(LINUX64)
        #undef  MFX_VA_LINUX
        #define MFX_VA_LINUX


    #endif // #if defined(LINUX32) || defined(LINUX64)

#if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN)
    #if defined(HEVCE_EVALUATION)
        #define MFX_MAX_ENCODE_FRAMES 1000
    #endif
    #if defined(HEVCD_EVALUATION)
        #define MFX_MAX_DECODE_FRAMES 1000
    #endif
#endif

#if !defined(LINUX_TARGET_PLATFORM) || defined(LINUX_TARGET_PLATFORM_BDW) || defined(LINUX_TARGET_PLATFORM_CFL) || defined(LINUX_TARGET_PLATFORM_BXT)
    #if !defined(ANDROID)
        // HW decoders are part of library
        #define MFX_ENABLE_H264_VIDEO_DECODE
        #define MFX_ENABLE_H265_VIDEO_DECODE
        #define MFX_ENABLE_VP8_VIDEO_DECODE_HW
        #define MFX_ENABLE_VP9_VIDEO_DECODE_HW
        
        //h264e
        #define MFX_ENABLE_H264_VIDEO_ENCODE

        //h265e
        #if defined(AS_HEVCE_PLUGIN) || defined(MFX_VA)
            #define MFX_ENABLE_H265_VIDEO_ENCODE
        #endif
        
        #if MFX_VERSION >= 1023 && !defined(LINUX_TARGET_PLATFORM_BXT)
            #define MFX_ENABLE_H264_REPARTITION_CHECK
        #endif

        #if MFX_VERSION >= 1025
            #if !defined(AS_H264LA_PLUGIN)
                #define MFX_ENABLE_MFE
            #endif
        #endif

        //#define MFX_ENABLE_H264_VIDEO_PAK
        //#define MFX_ENABLE_H264_VIDEO_ENC
        #if defined(LINUX64)
            #define MFX_ENABLE_H264_VIDEO_FEI_ENCPAK
            #define MFX_ENABLE_H264_VIDEO_FEI_PREENC
            #define MFX_ENABLE_H264_VIDEO_FEI_ENC
            #define MFX_ENABLE_H264_VIDEO_FEI_PAK
        #endif

        // mpeg2
        #define MFX_ENABLE_MPEG2_VIDEO_DECODE
        #define MFX_ENABLE_MPEG2_VIDEO_ENCODE

        //// vc1
        #define MFX_ENABLE_VC1_VIDEO_DECODE

        // mjpeg
        #define MFX_ENABLE_MJPEG_VIDEO_DECODE
        #define MFX_ENABLE_MJPEG_VIDEO_ENCODE

        // vpp
        #define MFX_ENABLE_DENOISE_VIDEO_VPP
        #define MFX_ENABLE_VPP
        #if !defined(LINUX_TARGET_PLATFORM_BXT)
            #define MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP
        #endif

        #define MFX_ENABLE_H264_VIDEO_ENCODE_HW
        #define MFX_ENABLE_MPEG2_VIDEO_ENCODE_HW
        //#define MFX_ENABLE_H264_VIDEO_ENC_HW
        #if defined(AS_H264LA_PLUGIN)
            #define MFX_ENABLE_LA_H264_VIDEO_HW
        #endif

        // H265 FEI plugin

        // user plugin for decoder, encoder, and vpp
        #define MFX_ENABLE_USER_DECODE
        #define MFX_ENABLE_USER_ENCODE
        #define MFX_ENABLE_USER_ENC
        #define MFX_ENABLE_USER_VPP

        // aac
        #define MFX_ENABLE_AAC_AUDIO_DECODE

        //mp3
        #define MFX_ENABLE_MP3_AUDIO_DECODE

        // av1

    #else // #if !defined(ANDROID)
        #include "mfx_android_defs.h"
    #endif // #if !defined(ANDROID)

    #if defined(AS_H264LA_PLUGIN)
        #undef MFX_ENABLE_MJPEG_VIDEO_DECODE
        #undef MFX_ENABLE_MJPEG_VIDEO_ENCODE
        #undef MFX_ENABLE_H264_VIDEO_FEI_ENCPAK
        #undef MFX_ENABLE_H264_VIDEO_FEI_PREENC
        #undef MFX_ENABLE_H264_VIDEO_FEI_ENC
        #undef MFX_ENABLE_H264_VIDEO_FEI_PAK
        #undef MFX_ENABLE_VPP
    #endif

    #if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN) || defined(AS_VP8D_PLUGIN) || defined(AS_VP9D_PLUGIN) || (defined(AS_HEVC_FEI_ENCODE_PLUGIN) && MFX_VERSION >= MFX_VERSION_NEXT)
        #undef MFX_ENABLE_H265_VIDEO_DECODE
        #undef MFX_ENABLE_H265_VIDEO_ENCODE
        #undef MFX_ENABLE_H264_VIDEO_DECODE
        #undef MFX_ENABLE_H264_VIDEO_ENCODE
        #undef MFX_ENABLE_MPEG2_VIDEO_DECODE
        #undef MFX_ENABLE_MPEG2_VIDEO_ENCODE
        #undef MFX_ENABLE_VC1_VIDEO_DECODE
        #undef MFX_ENABLE_MJPEG_VIDEO_DECODE
        #undef MFX_ENABLE_MJPEG_VIDEO_ENCODE
        #undef MFX_ENABLE_DENOISE_VIDEO_VPP
        #undef MFX_ENABLE_VPP
        #undef MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP
        #undef MFX_ENABLE_H264_VIDEO_ENCODE_HW
        #undef MFX_ENABLE_MPEG2_VIDEO_ENCODE_HW
        #undef MFX_ENABLE_USER_DECODE
        #undef MFX_ENABLE_USER_ENCODE
        #undef MFX_ENABLE_AAC_AUDIO_DECODE
        #undef MFX_ENABLE_MP3_AUDIO_DECODE
        #undef MFX_ENABLE_H264_VIDEO_FEI_ENCPAK
        #undef MFX_ENABLE_H264_VIDEO_FEI_PREENC
        #undef MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE
        #undef MFX_ENABLE_VP8_VIDEO_DECODE_HW
    #endif // #if defined(AS_HEVCD_PLUGIN)

    #if defined(AS_HEVCE_PLUGIN)
        #define MFX_ENABLE_H265_VIDEO_ENCODE
            #define MFX_ENABLE_CM
    #endif
    #if defined(AS_HEVC_FEI_ENCODE_PLUGIN) && MFX_VERSION >= 1027 && !defined(LINUX_TARGET_PLATFORM_BXT)
        #define MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE
    #endif
    
#else // LINUX_TARGET_PLATFORM
    #if defined(LINUX_TARGET_PLATFORM_BXTMIN) // PRE_SI_GEN == 9
        #include "mfx_common_linux_bxtmin.h"
    #else
        #error "Target platform should be specified!"
    #endif
#endif // LINUX_TARGET_PLATFORM

#define MFX_ENABLE_HEVCE_INTERLACE
#define MFX_ENABLE_HEVCE_ROI

#if MFX_VERSION >= 1026
    #define MFX_ENABLE_MCTF
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    #define MFX_ENABLE_MCTF_EXT // extended MCTF interface
#endif
#endif

// The line below HAS to be changed to MFX_VERSION specific version i.e. 1027
// after inclusion of respective features into official API
#if MFX_VERSION >= MFX_VERSION_NEXT
    #define MFX_ENABLE_VPP_RUNTIME_HSBC
    #define MFX_ENABLE_RGBP
	#define MFX_ENABLE_FOURCC_RGB565
#endif

#endif // _MFX_CONFIG_H_
