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

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs()

cmake_policy(SET CMP0057 NEW)

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

list( APPEND Decoders h264 h265 mpeg2 vp8 vp9 mjpeg vc1 )

if ("mpeg2" IN_LIST MFX_CODECS )
  list( APPEND Decoders mpeg2)
endif()

if ("mjpeg" IN_LIST MFX_CODECS )
  list( APPEND Decoders mjpeg)
endif()

if ("vc1" IN_LIST MFX_CODECS )
  list( APPEND Decoders vc1)
endif()

if ("h264" IN_LIST MFX_CODECS )
  list( APPEND Decoders h264)
endif()

if ("h265" IN_LIST MFX_CODECS )
  list( APPEND Decoders h265)
endif()

if ("vp8" IN_LIST MFX_CODECS )
  list( APPEND Decoders vp8)
endif()

if ("vp9" IN_LIST MFX_CODECS )
  list( APPEND Decoders vp9)
endif()

foreach( dir ${Decoders} )
  include_directories( ${MSDK_LIB_ROOT}/decode/${dir}/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/${dir}_dec/include )
endforeach()
include_directories( ${MSDK_LIB_ROOT}/vpp/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_dec/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_common/include )
include_directories( ${MSDK_UMC_ROOT}/codec/vc1_common/include )

set( defs "" )
set( sources "" )
set( sources.plus "" )
foreach( dir ${vdirs} )
  file( GLOB_RECURSE srcs "${dir}/src/*.c" "${dir}/src/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()

make_library( decode hw static )

### H264 Decoder
if ("h264" IN_LIST MFX_CODECS )
  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/h264_dec/src" )
  set( defs "" )
  set( sources "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( h264_dec hw static )
endif()

### JPEG Decoder
if ( ${MFX_USE_JPEG} )
  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/jpeg_common/src" )
  set( sources "" )
  set( defs "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( jpeg_common none static )

  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/jpeg_dec/src" )
  set( defs "" )
  set( sources "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( jpeg_dec hw static )
endif()

### MPEG2 Decoder
if ("mpeg2" IN_LIST MFX_CODECS )
  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/mpeg2_dec/src" )
  set( defs "" )
  set( sources "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( mpeg2_dec hw static )
endif()

### VC1 Decoder
if ("vc1" IN_LIST MFX_CODECS )
  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vc1_dec/src" )
  set( defs "" )
  set( sources "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( vc1_dec hw static )

  set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vc1_common/src" )
  set( defs "" )
  set( sources "" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( vc1_common none static )
endif()

