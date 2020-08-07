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
include_directories( ${MSDK_LIB_ROOT}/cmrt_cross_platform/include )
include_directories( ${MSDK_LIB_ROOT}/mctf_package/mctf/include )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/h264/include )
include_directories( ${MSDK_UMC_ROOT}/codec/brc/include )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_common )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
include_directories( ${MSDK_STUDIO_ROOT}/enctools/include )

# =============================================================================
set( sources "" )
set( sources.plus "" )
foreach( dir ${MSDK_LIB_ROOT}/fei/h264_la )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()

set( defs "-DMFX_VA -DAS_H264LA_PLUGIN" )
make_library( h264_la none static )

set( defs "" )

# =============================================================================
set( sources "" )
set( sources.plus "" )
foreach( dir ${MSDK_LIB_ROOT}/fei/h264_common )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
set( defs "-DMFX_VA" )
make_library( h264_common none static )

set( defs "" )

# =============================================================================
set( sources "" )
set( sources.plus "" )
foreach( dir ${MSDK_LIB_ROOT}/fei/h264_enc )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
set( defs "-DMFX_VA" )
make_library( h264_enc none static )

set( defs "" )

# =============================================================================
set( sources "" )
set( sources.plus "" )
include_directories ( ${MSDK_LIB_ROOT}/fei/h264_preenc )
foreach( dir ${MSDK_LIB_ROOT}/fei/h264_preenc )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
set( defs "-DMFX_VA" )
make_library( h264_preenc none static )

set( defs "" )

# =============================================================================
set( sources "" )
set( sources.plus "" )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_pak )
foreach( dir ${MSDK_LIB_ROOT}/fei/h264_pak )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
set( defs "-DMFX_VA" )
make_library( h264_pak none static )

set( defs "" )
