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

### ENCODE HW
list( APPEND Encoders h264 mjpeg mpeg2 )
foreach( dir ${Encoders} )
  include_directories( ${MSDK_LIB_ROOT}/encode_hw/${dir}/include )
endforeach()

include_directories( ${MSDK_LIB_ROOT}/cmrt_cross_platform/include )
include_directories( ${MSDK_LIB_ROOT}/enc_hw/mpeg2/include )
include_directories( ${MSDK_LIB_ROOT}/pak/mpeg2/include )
include_directories( ${MSDK_LIB_ROOT}/genx/h264_encode/include )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
# umc codec
include_directories( ${MSDK_UMC_ROOT}/codec/brc/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_common/include )
include_directories( ${MSDK_UMC_ROOT}/codec/h264_enc/include )

set( sources "" )
foreach( dir ${Encoders} )
  file( GLOB_RECURSE srcs "${dir}/src/*.c" "${dir}/src/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_ddi_enc_dump.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/mfx_h264_fei_vaapi.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
    ${prefix}/mfx_mpeg2_enc_common_hw.cpp
    ${prefix}/mfx_mpeg2_encode_factory.cpp
    ${prefix}/mfx_mpeg2_encode_vaapi.cpp
    )
endforeach()

set( defs "${WARNING_FLAGS}" )
make_library( encode hw static )
set( defs "" )

### BRC
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/brc/src" )
set( defs "${WARNING_FLAGS}" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( bitrate_control none static )

include_directories( ${MSDK_LIB_ROOT}/genx/h264_encode/include )
set( SRC_DIR "${MSDK_LIB_ROOT}/genx/h264_encode/src" )
set( defs "${WARNING_FLAGS}" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( genx_h264_encode_embeded none static )
set( defs "" )
