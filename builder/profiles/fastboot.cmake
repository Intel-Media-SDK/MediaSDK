# Copyright (c) 2018 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( MFX_LIBNAME "mfxhw64-fastboot" )
else()
  set( MFX_LIBNAME "mfxhw32-fastboot" )
endif()

set( BUILD_RUNTIME ON )
set( BUILD_DISPATCHER OFF )
set( BUILD_SAMPLES OFF )
set( BUILD_TUTORIALS OFF )
set( BUILD_TOOLS OFF )

set( MFX_ENABLE_KERNELS OFF )
set( MFX_ENABLE_SW_FALLBACK OFF )
set( MFX_ENABLE_MCTF OFF )
set( MFX_ENABLE_ASC OFF )

set( MFX_ENABLE_USER_DECODE OFF )
set( MFX_ENABLE_USER_ENCODE OFF )
set( MFX_ENABLE_USER_ENC OFF )
set( MFX_ENABLE_USER_VPP OFF )

set( MFX_ENABLE_AV1_VIDEO_DECODE OFF )
set( MFX_ENABLE_H264_VIDEO_ENCODE OFF )
set( MFX_ENABLE_H264_VIDEO_FEI_ENCODE OFF )
set( MFX_ENABLE_H265_VIDEO_ENCODE OFF )
set( MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE OFF )
set( MFX_ENABLE_VP9_VIDEO_ENCODE OFF )
set( MFX_ENABLE_VP8_VIDEO_DECODE OFF)
set( MFX_ENABLE_VP9_VIDEO_DECODE ON)
set( MFX_ENABLE_H264_VIDEO_DECODE ON)
set( MFX_ENABLE_H265_VIDEO_DECODE ON)
set ( MFX_ENABLE_MPEG2_VIDEO_DECODE ON)
set ( MFX_ENABLE_MPEG2_VIDEO_ENCODE OFF)
set ( MFX_ENABLE_MJPEG_VIDEO_DECODE OFF)
set ( MFX_ENABLE_MJPEG_VIDEO_ENCODE OFF)
set ( MFX_ENABLE_VC1_VIDEO_DECODE OFF)
