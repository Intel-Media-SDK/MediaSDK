// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __MFX_ANDROID_DEFS_H__
#define __MFX_ANDROID_DEFS_H__

  #define MFX_ENABLE_VC1_VIDEO_DECODE

  #define MFX_ENABLE_MPEG2_VIDEO_DECODE

  #define MFX_ENABLE_H265_VIDEO_DECODE

  #define MFX_ENABLE_H265_VIDEO_ENCODE

  #define MFX_ENABLE_H264_VIDEO_DECODE

  #define MFX_ENABLE_H264_VIDEO_ENCODE

  #define MFX_ENABLE_MJPEG_VIDEO_DECODE

  #define MFX_ENABLE_MJPEG_VIDEO_ENCODE

  #define MFX_ENABLE_VP8_VIDEO_DECODE

  #define MFX_ENABLE_VP9_VIDEO_DECODE

  #define MFX_ENABLE_VP9_VIDEO_ENCODE

  #define MFX_ENABLE_VPP

  #define MFX_ENABLE_USER_ENCODE

#if MFX_ANDROID_VERSION >= MFX_P
  #define MFX_ENABLE_KERNELS
#endif

#endif // #ifndef __MFX_ANDROID_DEFS_H__
