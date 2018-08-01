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

list( APPEND Decoders h264 h265 vp9 )
foreach( dir ${Decoders} )
  include_directories( ${MSDK_LIB_ROOT}/decode/${dir}/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/${dir}_dec/include )
endforeach()
include_directories( ${MSDK_LIB_ROOT}/vpp/include )

set( sources.plus "" )
set( DECODE_SRC_DIR "${MSDK_LIB_ROOT}/decode/" )

### H264 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/h264_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${DECODE_SRC_DIR}/h264/src/mfx_h264_dec_decode.cpp)
make_library( h264_dec_gp hw static )

### H265 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/h265_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources ${DECODE_SRC_DIR}/h265/src/mfx_h265_dec_decode.cpp)
make_library( h265_dec_gp hw static )

### VP9 Decoder
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/vp9_dec/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND sources 
	${DECODE_SRC_DIR}/vp9/src/mfx_vp9_dec_decode.cpp
	${DECODE_SRC_DIR}/vp9/src/mfx_vp9_dec_decode_hw.cpp
	${DECODE_SRC_DIR}/vp9/src/mfx_vp9_dec_decode_utils.cpp )
make_library( vp9_dec_gp hw static )

