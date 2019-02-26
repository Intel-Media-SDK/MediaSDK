// Copyright (c) 2016-2019 Intel Corporation
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

#ifndef _MFX_WIN_CONFIG_H_
#define _MFX_WIN_CONFIG_H_

    #define MFX_ENABLE_KERNELS

    #if ((MFX_VERSION >= 1026) && (!AS_CAMERA_PLUGIN))
    #define MFX_ENABLE_MCTF
    #endif

    #define MFX_ENABLE_USER_DECODE
    #define MFX_ENABLE_USER_ENCODE
    #define MFX_ENABLE_USER_ENC
    #define MFX_ENABLE_USER_VPP

    #define MFX_ENABLE_MPEG2_VIDEO_DECODE
    #define MFX_ENABLE_MJPEG_VIDEO_DECODE
    #define MFX_ENABLE_H264_VIDEO_DECODE
    #if defined(MFX_VA)
        #define MFX_ENABLE_H265_VIDEO_DECODE
        #define MFX_ENABLE_VP8_VIDEO_DECODE_HW
        #define MFX_ENABLE_VP9_VIDEO_DECODE_HW
    #endif
    #define MFX_ENABLE_VC1_VIDEO_DECODE

    #define MFX_ENABLE_MPEG2_VIDEO_ENCODE
    #define MFX_ENABLE_MJPEG_VIDEO_ENCODE
    #define MFX_ENABLE_H264_VIDEO_ENCODE
    #define MFX_ENABLE_H264_VIDEO_ENCODE_HW
    #if defined(AS_HEVCD_PLUGIN) || defined(AS_HEVCE_PLUGIN) || defined(MFX_VA)
        #define MFX_ENABLE_H265_VIDEO_ENCODE
    #endif

#endif // _MFX_WIN_CONFIG_H_
