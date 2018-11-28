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

if(MFX_ENABLE_KERNELS)
  mfx_include_dirs( )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/asc/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/copy_kernels/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/field_copy/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h264_encode/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/mctf/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../cmrt_cross_platform/include )

  set( defs "" )
  set( sources "" )
  set( sources.plus "" )

  set( MSDK_GENX_ROOT ${MSDK_LIB_ROOT}/genx )

  if( MFX_ENABLE_ASC )
    set( prefix ${MSDK_GENX_ROOT}/asc/isa )
    list( APPEND sources
        ${prefix}/genx_scd_bdw_isa.cpp
        ${prefix}/genx_scd_cnl_isa.cpp
        ${prefix}/genx_scd_icl_isa.cpp
        ${prefix}/genx_scd_icllp_isa.cpp
        ${prefix}/genx_scd_skl_isa.cpp
    )
  endif()

  set( prefix ${MSDK_GENX_ROOT}/copy_kernels/isa )
  list( APPEND sources
    ${prefix}/genx_cht_copy_isa.cpp
    ${prefix}/genx_skl_copy_isa.cpp
    ${prefix}/genx_icl_copy_isa.cpp
    ${prefix}/genx_icllp_copy_isa.cpp
    ${prefix}/genx_cnl_copy_isa.cpp
  )

  set( prefix ${MSDK_GENX_ROOT}/field_copy/isa )
  list( APPEND sources
    ${prefix}/genx_fcopy_gen8_isa.cpp
    ${prefix}/genx_fcopy_gen9_isa.cpp
    ${prefix}/genx_fcopy_gen10_isa.cpp
    ${prefix}/genx_fcopy_gen11_isa.cpp
    ${prefix}/genx_fcopy_gen11lp_isa.cpp
  )

  set( prefix ${MSDK_GENX_ROOT}/h264_encode/isa )
  list( APPEND sources
    ${prefix}/genx_bdw_simple_me_isa.cpp
    ${prefix}/genx_cnl_histogram_isa.cpp
    ${prefix}/genx_cnl_simple_me_isa.cpp
    ${prefix}/genx_hsw_simple_me_isa.cpp
    ${prefix}/genx_hsw_simple_me_proto.cpp
    ${prefix}/genx_icl_histogram_isa.cpp
    ${prefix}/genx_icl_simple_me_isa.cpp
    ${prefix}/genx_icllp_histogram_isa.cpp
    ${prefix}/genx_icllp_simple_me_isa.cpp
    ${prefix}/genx_skl_histogram_isa.cpp
    ${prefix}/genx_skl_simple_me_isa.cpp
  )

  if( MFX_ENABLE_MCTF )
    set( prefix ${MSDK_GENX_ROOT}/mctf/isa )
    list( APPEND sources
      ${prefix}/genx_mc_gen8_isa.cpp
      ${prefix}/genx_mc_gen9_isa.cpp
      ${prefix}/genx_mc_gen11_isa.cpp
      ${prefix}/genx_mc_gen11lp_isa.cpp
      ${prefix}/genx_me_gen8_isa.cpp
      ${prefix}/genx_me_gen9_isa.cpp
      ${prefix}/genx_me_gen11_isa.cpp
      ${prefix}/genx_me_gen11lp_isa.cpp
      ${prefix}/genx_sd_gen8_isa.cpp
      ${prefix}/genx_sd_gen9_isa.cpp
      ${prefix}/genx_sd_gen11_isa.cpp
      ${prefix}/genx_sd_gen11lp_isa.cpp
    )
  endif()

  make_library(genx none static )
  set( defs "" )
endif()
