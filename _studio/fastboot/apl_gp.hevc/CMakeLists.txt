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

remove_codecs_definitions()
include( "${BUILDER_ROOT}/profiles/apl_gp.hevc.cmake" )
add_codecs_definitions()

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs()

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

### libmfxhw
list( APPEND mdirs
  scheduler vpp cmrt_cross_platform mctf_package/mctf
  decode/h265
  )

#include_directories( ${MSDK_LIB_ROOT}/fei/h264_common )
#include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
#include_directories( ${MSDK_LIB_ROOT}/fei/h264_pak )
#include_directories( ${MSDK_LIB_ROOT}/fei/h264_enc )
foreach( dir ${mdirs} )
  include_directories( ${MSDK_LIB_ROOT}/${dir}/include )
endforeach()

include_directories( ${MSDK_LIB_ROOT}/genx/copy_kernels/include )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
include_directories( ${MSDK_STUDIO_ROOT}/hevce_hw/h265/include )

list( APPEND cdirs
  brc 
#  h264_enc vp9_dec
#  mpeg2_dec vc1_dec vc1_common
  )

foreach( dir ${cdirs} )
  include_directories( ${MSDK_UMC_ROOT}/codec/${dir}/include )
endforeach()

# =============================================================================

set( sources "" )
set( sources.plus "" )
file( GLOB_RECURSE srcs "${MSDK_LIB_ROOT}/scheduler/src/*.cpp" )
list( APPEND sources ${srcs})
# =============================================================================

foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/libmfxsw.cpp
    ${prefix}/libmfxsw_async.cpp
    ${prefix}/libmfxsw_decode.cpp
    ${prefix}/libmfxsw_enc.cpp
    ${prefix}/libmfxsw_encode.cpp
    ${prefix}/libmfxsw_pak.cpp
    ${prefix}/libmfxsw_plugin.cpp
    ${prefix}/libmfxsw_query.cpp
    ${prefix}/libmfxsw_session.cpp
    ${prefix}/libmfxsw_vpp.cpp
    ${prefix}/mfx_session.cpp
    ${prefix}/mfx_user_plugin.cpp
    #=================
    ${prefix}/mfx_brc_common.cpp
    #=================
  )
endforeach()
foreach( prefix ${MSDK_STUDIO_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/cm_mem_copy.cpp
    ${prefix}/mfx_vpp_vaapi.cpp
    ${prefix}/libmfx_allocator.cpp
    ${prefix}/libmfx_allocator_vaapi.cpp
    ${prefix}/libmfx_core.cpp
    ${prefix}/libmfx_core_factory.cpp
    ${prefix}/libmfx_core_vaapi.cpp
    ${prefix}/mfx_umc_alloc_wrapper.cpp
    ${prefix}/mfx_static_assert_structs.cpp
    ${prefix}/mfx_mfe_adapter.cpp
  )
endforeach()

foreach( prefix ${MSDK_LIB_ROOT}/genx/field_copy/src )
  list( APPEND sources
    ${prefix}/genx_fcopy_gen8_isa.cpp
    ${prefix}/genx_fcopy_gen9_isa.cpp
  )
endforeach()

set( USE_STRICT_NAME TRUE )
set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/libmfx.map" )

set( mfxlibname mfxhw64.hevc )
set(MCTF_LIB "mctf_hw")

set( LIBS "" )
list( APPEND LIBS
  bitrate_control
  umc
  vm
  vm_plus
  umc_va_hw
  cmrt_cross_platform_hw
  encode_stub_hw
  mfx_common_gp
  mfx_common_gp_hw
  asc_gp
  ${MCTF_LIB}
  vpp_gp_hw
#  h264_common
#  umc_h265_hw
  ${ITT_LIBRARIES}
  pthread
  dl
  mfx_trace
  mfx_trace_hw
  h265_dec_gp_hw
#  h264_dec_gp_hw
#  vp9_dec_gp_hw
)

make_library( ${mfxlibname} hw shared )

get_mfx_version(mfx_version_major mfx_version_minor)
set (mfxlib_filename "lib${mfxlibname}-p.so.${mfx_version_major}.${mfx_version_minor}")

#set_target_properties( ${mfxlibname} PROPERTIES
#    LIBRARY_OUTPUT_NAME ${mfxlib_filename}
#    SUFFIX "" )

get_target_property(bindir ${mfxlibname} RUNTIME_OUTPUT_DIRECTORY) 

install( FILES ${bindir}/lib${mfxlibname}.so DESTINATION ${MFX_MODULES_DIR} RENAME ${mfxlib_filename} )
install( CODE "execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${MFX_MODULES_DIR}/${mfxlib_filename}
  ${MFX_MODULES_DIR}/lib${mfxlibname}.so )"
)
