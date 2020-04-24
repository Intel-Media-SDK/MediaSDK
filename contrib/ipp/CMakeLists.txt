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

### ipp_sse4
set( sources "" )
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  file( GLOB_RECURSE asm_srcs "${SRC_DIR}/asm_intel64/*.s" )
  list( APPEND sources
    ${SRC_DIR}/pccyuvms.c
    ${SRC_DIR}/pjdecdct0w7cn.c
    ${SRC_DIR}/pjdechuffp.c
    ${SRC_DIR}/pjencccps.c
    ${SRC_DIR}/pjenchuffls.c
    ${SRC_DIR}/psmul.c
    ${SRC_DIR}/owncpufeatures.c
    ${SRC_DIR}/pccyuvmsw7.c
    ${SRC_DIR}/pjdecdct1.c
    ${SRC_DIR}/pjdecpred.c
    ${SRC_DIR}/pjencccpsy8.c
    ${SRC_DIR}/pjenchuffp.c
    ${SRC_DIR}/psmulm6a6w7cn.c
    ${SRC_DIR}/ippcachesize.c
    ${SRC_DIR}/piconvert02ca.c
    ${SRC_DIR}/pjdecdct1w7cn.c
    ${SRC_DIR}/pjdecqnt.c
    ${SRC_DIR}/pjenccctbl.c
    ${SRC_DIR}/pjencqnt.c
    ${SRC_DIR}/pjzigzag.c
    ${SRC_DIR}/psshift.c
    ${SRC_DIR}/piconvertca.c
    ${SRC_DIR}/pjdecdct32f.c
    ${SRC_DIR}/pjdecss0.c
    ${SRC_DIR}/pjencss0.c
    ${SRC_DIR}/ippmalloc.c
    ${SRC_DIR}/picopyca.c
    ${SRC_DIR}/pjdecdct32fw7cn.c
    ${SRC_DIR}/pjencdct32f.c
    ${SRC_DIR}/pjencss.c
    ${SRC_DIR}/pvcdeinterlacevideoplane.c
    ${SRC_DIR}/picopycpp8y8cn.c
    ${SRC_DIR}/pjdecdctcn.c
    ${SRC_DIR}/pjenccc0.c
    ${SRC_DIR}/pjencdct32fw7cn.c
    ${SRC_DIR}/pccjoin422pxca.c
    ${SRC_DIR}/pilogic.c
    ${SRC_DIR}/pjdechuff.c
    ${SRC_DIR}/pjenccc1.c
    ${SRC_DIR}/pjencdct.c
    ${SRC_DIR}/pjhufftbl.c
    ${SRC_DIR}/pscopyca.c
    ${SRC_DIR}/pccycbcr44deint.c
    ${SRC_DIR}/pizigzag.c
    ${SRC_DIR}/pjenccc.c
    ${SRC_DIR}/pjenchuff.c
    ${SRC_DIR}/pccycbcr44deintw7cn.c
    ${SRC_DIR}/pjdecdct0.c
    ${SRC_DIR}/pjdechuffls.c
    ${SRC_DIR}/psdiv.c
    ${SRC_DIR}/pvcvc1rangemap.c
    ${asm_srcs}
  )
else()
  #file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/asm_ia32/*.s" )
endif()

add_library(ipp_sse4 OBJECT ${sources})
target_compile_options(ipp_sse4 PRIVATE -msse4.2)
configure_build_variant(ipp_sse4 none)

### ipp
set( sources "" )
list( APPEND sources
  ${SRC_DIR}/ippinit.c
  $<TARGET_OBJECTS:ipp_sse4>
)

enable_language( C ASM )
set( CMAKE_ASM_SOURCE_FILE_EXTENSIONS s )
make_library( ipp none static )
