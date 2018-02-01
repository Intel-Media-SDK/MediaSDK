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

#ifndef _MFX_COMMON_LINUX_BDW_H_
#define _MFX_COMMON_LINUX_BDW_H_



// h264
#define MFX_ENABLE_H264_VIDEO_DECODE

    #define MFX_ENABLE_H265_VIDEO_DECODE
    #define MFX_ENABLE_VP8_VIDEO_DECODE_HW
    //#define MFX_ENABLE_VP9_VIDEO_DECODE_HW

#if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN)
    #define MFX_ENABLE_H265_VIDEO_DECODE
#endif

#define MFX_ENABLE_H264_VIDEO_ENCODE
#if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN)
    #define MFX_ENABLE_H265_VIDEO_ENCODE
#endif

#if defined(AS_HEVC_FEI_ENCODE_PLUGIN) && MFX_VERSION >= 1024
    #define MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE
#endif

#if MFX_VERSION >= 1023
    #define MFX_ENABLE_H264_REPARTITION_CHECK
#endif

    // Unsupported on Linux:

#if (MFX_VERSION < MFX_VERSION_NEXT)
#endif

#if (MFX_VERSION >= 1025)
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
#define MFX_ENABLE_SCENE_CHANGE_DETECTION_VPP

#define MFX_ENABLE_H264_VIDEO_ENCODE_HW
#define MFX_ENABLE_MPEG2_VIDEO_ENCODE_HW
//#define MFX_ENABLE_H264_VIDEO_ENC_HW
#if defined(AS_H264LA_PLUGIN)
#define MFX_ENABLE_LA_H264_VIDEO_HW
#endif

// H265 FEI plugin

// undefine unsupported features DirtyRect and MoveRect on Linux

//H265 feature

// user plugin for decoder, encoder, and vpp
#define MFX_ENABLE_USER_DECODE
#define MFX_ENABLE_USER_ENCODE
#define MFX_ENABLE_USER_ENC
#define MFX_ENABLE_USER_VPP

// aac
#define MFX_ENABLE_AAC_AUDIO_DECODE

//mp3
#define MFX_ENABLE_MP3_AUDIO_DECODE
// linux support



#if defined(AS_H264LA_PLUGIN)
    #undef MFX_ENABLE_MJPEG_VIDEO_DECODE
    #undef MFX_ENABLE_MJPEG_VIDEO_ENCODE
    #undef MFX_ENABLE_H264_VIDEO_FEI_ENCPAK
    #undef MFX_ENABLE_H264_VIDEO_FEI_PREENC
    #undef MFX_ENABLE_H264_VIDEO_FEI_ENC
    #undef MFX_ENABLE_H264_VIDEO_FEI_PAK
    #undef MFX_ENABLE_VPP
#endif

#if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN) || defined(AS_VP8D_PLUGIN) || defined(AS_VP9D_PLUGIN) || (defined(AS_HEVC_FEI_ENCODE_PLUGIN) && MFX_VERSION >= 1024)
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
#if defined(AS_HEVCD_PLUGIN)
    #define MFX_ENABLE_H265_VIDEO_DECODE
#endif
#if defined(AS_HEVCE_PLUGIN)
    #define MFX_ENABLE_H265_VIDEO_ENCODE
        #define MFX_ENABLE_CM
#endif
#if defined(AS_HEVC_FEI_ENCODE_PLUGIN) && MFX_VERSION >= 1026
    #define MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE
#endif
#if defined(AS_VP8DHW_PLUGIN)
    #define MFX_ENABLE_VP8_VIDEO_DECODE_HW
#endif

#if defined(AS_VP8D_PLUGIN)
#define MFX_ENABLE_VP8_VIDEO_DECODE_HW
#endif

#if defined(AS_VP9D_PLUGIN)
//#define MFX_ENABLE_VP9_VIDEO_DECODE
#define MFX_ENABLE_VP9_VIDEO_DECODE_HW
#endif


#endif //_MFX_COMMON_LINUX_BDW_H_
