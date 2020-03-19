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

set( MFX_ROOT "${CMAKE_HOME_DIRECTORY}/_studio/shared" )

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${MFX_ROOT}/include
  ${MFX_ROOT}/umc/core/umc/include
)

set( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )
# Optimized for processors with Intel SSE4.2

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( ipp_defs "-D_Y8 -D_ARCH_EM64T" )
else ()
  set( ipp_defs "-D_P8 -D_ARCH_IA32" )
endif()
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ipp_defs}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ipp_defs}" )

set( sources "" )
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/asm_intel64/*.s" )
else()
  #file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/asm_ia32/*.s" )
endif()

list( APPEND sources ${srcs})
enable_language( C ASM )
set( CMAKE_ASM_SOURCE_FILE_EXTENSIONS s )
make_library( ipp none static )

