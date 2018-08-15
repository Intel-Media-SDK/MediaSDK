# Copyright (c) 2017 Intel Corporation
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

message( STATUS "Global Configuration of Targets" )
if(__TARGET_PLATFORM)
    add_definitions( -DLINUX_TARGET_PLATFORM -DLINUX_TARGET_PLATFORM_${__TARGET_PLATFORM} )
endif()
#
set( T_ARCH "sse4.2" )
#if( __TARGET_PLATFORM STREQUAL "BDW" )
#    set( T_ARCH "avx2" )
#endif()
message( STATUS "Target Architecture to compile: ${T_ARCH}" )
#
if( __TARGET_PLATFORM STREQUAL "BXTMIN" )
    set(MFX_LIB_ONLY TRUE)
    message( STATUS "!!! BXTMIN target MFX_LIB_ONLY !!!" )
endif()

# HEVC plugins disabled by default
set( ENABLE_HEVC FALSE )
set( ENABLE_HEVC_FEI FALSE )

if( MFX_LIB_ONLY )
    set( ENABLE_HEVC_FEI FALSE )
    message("!!! NO HEVC PLUGINS - MFX LIB ONLY BUILD !!!")
else()
    if ((CMAKE_C_COMPILER MATCHES icc) OR ENABLE_HEVC_ON_GCC )
       set(ENABLE_HEVC TRUE)
       set(ENABLE_HEVC_FEI TRUE)
       message( STATUS "  Enabling HEVC plugins build!")
    endif()
endif()

append("-std=c++11" CMAKE_CXX_FLAGS)

# SW HEVC decoder & encoder require SSE4.2
  if (CMAKE_C_COMPILER MATCHES icc)
    append("-xSSE4.2 -static-intel" CMAKE_C_FLAGS)
  else()
    append("-m${T_ARCH}" CMAKE_C_FLAGS)
  endif()

  if (CMAKE_CXX_COMPILER MATCHES icpc)
    append("-xSSE4.2 -static-intel" CMAKE_CXX_FLAGS)
  else()
    append("-m${T_ARCH}" CMAKE_CXX_FLAGS)
  endif()

if (ENABLE_TEXTLOG)
  append("-DMFX_TRACE_ENABLE_TEXTLOG" CMAKE_C_FLAGS)
  append("-DMFX_TRACE_ENABLE_TEXTLOG" CMAKE_CXX_FLAGS)
endif()

if (ENABLE_STAT)
  append("-DMFX_TRACE_ENABLE_STAT" CMAKE_C_FLAGS)
  append("-DMFX_TRACE_ENABLE_STAT" CMAKE_CXX_FLAGS)
endif()

option( MFX_ENABLE_KERNELS "Build with advanced media kernels support?" ON )

option( MFX_ENABLE_USER_DECODE "Enabled user decode plugins?" ON)
option( MFX_ENABLE_USER_ENCODE "Enabled user encode plugins?" ON)
option( MFX_ENABLE_USER_ENC "Enabled user ENC plugins?" ON)
option( MFX_ENABLE_USER_VPP "Enabled user VPP plugins?" ON)

option( MFX_ENABLE_VP8_VIDEO_DECODE "Enabled VP8 decoder?" ON)
option( MFX_ENABLE_VP9_VIDEO_DECODE "Enabled VP9 decoder?" ON)
option( MFX_ENABLE_H265_VIDEO_DECODE "Enabled HEVC decoder?" ON)
option( MFX_ENABLE_MPEG2_VIDEO_DECODE "Enabled MPEG2 decoder?" ON)
option( MFX_ENABLE_MPEG2_VIDEO_ENCODE "Enabled MPEG2 encoder?" ON)
option( MFX_ENABLE_MJPEG_VIDEO_DECODE "Enabled MJPEG decoder?" ON)
option( MFX_ENABLE_MJPEG_VIDEO_ENCODE "Enabled MJPEG encoder?" ON)

option( MFX_ENABLE_H264_VIDEO_ENCODE "Enable H.264 (AVC) encoder?" ON)
cmake_dependent_option(
  MFX_ENABLE_H264_VIDEO_FEI_ENCODE "Enable H.264 (AVC) FEI?" ON
  "MFX_ENABLE_H264_VIDEO_ENCODE" OFF)

option( MFX_ENABLE_H265_VIDEO_ENCODE "Enable H.265 (HEVC) encoder?" ON)
cmake_dependent_option(
  MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE "Enable H.265 (HEVC) FEI?" ON
  "MFX_ENABLE_H265_VIDEO_ENCODE" OFF)

# Now we will include config file which may overwrite default values of the
# options and options which user provided in a command line.
# It is critically important to include config file _after_ definition of
# all options. Otherwise rewrite of options in a config file will not take
# effect!
if (DEFINED MFX_CONFIG_FILE)
    # Include user provided cmake config file of the format:
    # set( VARIABLE VALUE )
    include(${MFX_CONFIG_FILE})
endif()

configure_file(mfxconfig.h.in mfxconfig.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
