# Copyright (c) 2017-2020 Intel Corporation
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

if (ENABLE_TEXTLOG)
  append("-DMFX_TRACE_ENABLE_TEXTLOG" CMAKE_C_FLAGS)
  append("-DMFX_TRACE_ENABLE_TEXTLOG" CMAKE_CXX_FLAGS)
endif()

if (ENABLE_STAT)
  append("-DMFX_TRACE_ENABLE_STAT" CMAKE_C_FLAGS)
  append("-DMFX_TRACE_ENABLE_STAT" CMAKE_CXX_FLAGS)
endif()

option( MFX_ENABLE_KERNELS "Build with advanced media kernels support?" ON )
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  option( MFX_ENABLE_SW_FALLBACK "Enabled software fallback for codecs?" ON )
else ()
  # disable SW fallback on ia32 by default (no ipp p8 yet)
  option( MFX_ENABLE_SW_FALLBACK "Enabled software fallback for codecs?" OFF )
endif()


cmake_dependent_option(
  BUILD_KERNELS "Rebuild kernels (shaders)?" OFF
  "MFX_ENABLE_KERNELS" OFF)

if (BUILD_KERNELS)
  find_program(CMC cmc)
  if(NOT CMC)
    message(FATAL_ERROR "Failed to find cm compiler")
  endif()
  find_program(GENX_IR GenX_IR)
  if(NOT GENX_IR)
    message(FATAL_ERROR "Failed to find GenX_IR compiler (part of IGC)")
  endif()
endif()

if ( ${API_VERSION} VERSION_GREATER 1.25 )
  set ( MFX_1_26_OPTIONS_ALLOWED ON )
else()
  set ( MFX_1_26_OPTIONS_ALLOWED OFF )
endif()

if ( ${API_VERSION} VERSION_GREATER 1.33 )
  set ( MFX_1_34_OPTIONS_ALLOWED ON )
else()
  set ( MFX_1_34_OPTIONS_ALLOWED OFF )
endif()

#if ON, enables encoding tools (EncTools) for encoding quality improvement; experimental feature
option( MFX_ENABLE_ENCTOOLS "Enabled EncTools?" OFF)
#if ON, enables adaptive encoding tools, part of EncTools, provided as libaenc.a binary; experimental feature
option( MFX_ENABLE_AENC "Enabled AENC extension?" OFF)

option( MFX_ENABLE_USER_DECODE "Enabled user decode plugins?" ON)
option( MFX_ENABLE_USER_ENCODE "Enabled user encode plugins?" ON)
option( MFX_ENABLE_USER_ENC "Enabled user ENC plugins?" ON)
option( MFX_ENABLE_USER_VPP "Enabled user VPP plugins?" ON)

option( MFX_ENABLE_AV1_VIDEO_DECODE "Enabled AV1 decoder?" ${MFX_1_34_OPTIONS_ALLOWED})
option( MFX_ENABLE_VP8_VIDEO_DECODE "Enabled VP8 decoder?" ON)
option( MFX_ENABLE_VP9_VIDEO_DECODE "Enabled VP9 decoder?" ON)
option( MFX_ENABLE_H264_VIDEO_DECODE "Enabled AVC decoder?" ON)
option( MFX_ENABLE_H265_VIDEO_DECODE "Enabled HEVC decoder?" ON)
option( MFX_ENABLE_MPEG2_VIDEO_DECODE "Enabled MPEG2 decoder?" ON)
option( MFX_ENABLE_MPEG2_VIDEO_ENCODE "Enabled MPEG2 encoder?" ON)
option( MFX_ENABLE_MJPEG_VIDEO_DECODE "Enabled MJPEG decoder?" ON)
option( MFX_ENABLE_MJPEG_VIDEO_ENCODE "Enabled MJPEG encoder?" ON)
option( MFX_ENABLE_VC1_VIDEO_DECODE "Enabled VC1 decoder?" ON)

option( MFX_ENABLE_H264_VIDEO_ENCODE "Enable H.264 (AVC) encoder?" ON)
cmake_dependent_option(
  MFX_ENABLE_H264_VIDEO_FEI_ENCODE "Enable H.264 (AVC) FEI?" ON
  "MFX_ENABLE_H264_VIDEO_ENCODE" OFF)

option( MFX_ENABLE_H265_VIDEO_ENCODE "Enable H.265 (HEVC) encoder?" ON)
cmake_dependent_option(
  MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE "Enable H.265 (HEVC) FEI?" ON
  "MFX_ENABLE_H265_VIDEO_ENCODE" OFF)

option( MFX_ENABLE_VP9_VIDEO_ENCODE "Enable VP9 encoder?" ON)

option( MFX_ENABLE_ASC "Enable ASC support?"  ON )

cmake_dependent_option(
  MFX_ENABLE_MCTF "Build with MCTF support?"  ${MFX_1_26_OPTIONS_ALLOWED}
  "MFX_ENABLE_ASC;MFX_ENABLE_KERNELS" OFF)

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
