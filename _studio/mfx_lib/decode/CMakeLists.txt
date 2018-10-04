# Copyright (c) 2017-2018 Intel Corporation
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

list( APPEND Decoders h264 h265 mpeg2 vp8 vp9 mjpeg vc1 )
foreach( dir ${Decoders} )
  include_directories( ${MSDK_LIB_ROOT}/decode/${dir}/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/${dir}_dec/include )
endforeach()
include_directories( ${MSDK_LIB_ROOT}/vpp/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_dec/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_enc/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_common/include )
include_directories( ${MSDK_UMC_ROOT}/codec/vc1_common/include )
include_directories( ${MSDK_UMC_ROOT}/codec/color_space_converter/include )

set( sources.plus "" )

### H264 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/h264_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/h264/src/mfx_h264_dec_decode.cpp)
make_library( h264_dec hw static )

### H265 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/h265_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/h265/src/mfx_h265_dec_decode.cpp)
make_library( h265_dec hw static )

### JPEG Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/jpeg_common/src" )
set( sources "" )
set( defs "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( jpeg_common none static )

### JPEG Encoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/jpeg_enc/src" )
set( sources "" )
set( defs "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( jpeg_enc none static )

set( SRC_DIR "${MSDK_UMC_ROOT}/codec/jpeg_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg/src/mfx_mjpeg_dec_decode.cpp)
make_library( jpeg_dec hw static )

### MPEG2 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/mpeg2_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/mpeg2/src/mfx_mpeg2_decode.cpp)
make_library( mpeg2_dec hw static )

### VC1 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vc1_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/vc1/src/mfx_vc1_decode.cpp)
make_library( vc1_dec hw static )

set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vc1_common/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( vc1_common none static )

### VP8 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vp8_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources 
	${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_common.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_hw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_hw_vaapi.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_tables.cpp )
make_library( vp8_dec hw static )

### VP9 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vp9_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources 
	${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode_hw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode_utils.cpp )
make_library( vp9_dec hw static )

### color space converter
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/color_space_converter/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( color_space_converter hw static )
