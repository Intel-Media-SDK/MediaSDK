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

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

### CM
include_directories( ${MSDK_LIB_ROOT}/cmrt_cross_platform/include )
set( SRC_DIR "${MSDK_LIB_ROOT}/cmrt_cross_platform/src" )
set( defs "" )
set( sources "" )
set( sources.plus "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( cmrt_cross_platform hw static )
set( defs "" )

### MFX common
set( sources "" )
set( sources.plus "" )
foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_brc_common.cpp
    ${prefix}/mfx_common_decode_int.cpp
    ${prefix}/mfx_common_int.cpp
    ${prefix}/mfx_enc_common.cpp
    ${prefix}/mfx_mpeg2_dec_common.cpp
    ${prefix}/mfx_critical_error_handler.cpp
    ${prefix}/mfx_vpx_dec_common.cpp
  )
endforeach()

set( defs "" )
make_library( mfx_common none static )
append_property( mfx_common COMPILE_FLAGS " -DMFX_RT" )

foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_ddi_enc_dump.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
    ${prefix}/mfx_mpeg2_enc_common_hw.cpp
    ${prefix}/mfx_mpeg2_encode_vaapi.cpp
    ${prefix}/mfx_mpeg2_encode_factory.cpp
  )
endforeach()

set( defs "" )
make_library( mfx_common hw static )
append_property( mfx_common_hw COMPILE_FLAGS " -DMFX_RT" )

### libmfxhw
list( APPEND mdirs
  scheduler vpp decode/vc1
  decode/mjpeg decode/h264 decode/mpeg2 cmrt_cross_platform encode_hw/h264 encode_hw/mpeg2 encode_hw/mjpeg
  decode/h265 decode/vp8 decode/vp9 mctf_package/mctf
  )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_common )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_pak )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_enc )
foreach( dir ${mdirs} )
  include_directories( ${MSDK_LIB_ROOT}/${dir}/include )
endforeach()

include_directories( ${MSDK_LIB_ROOT}/genx/copy_kernels/include )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
include_directories( ${MSDK_STUDIO_ROOT}/hevce_hw/h265/include )

list( APPEND cdirs
  brc h264_enc mpeg2_dec vc1_dec vp9_dec vc1_common
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
    ${prefix}/mfx_mpeg2_dec_common.cpp
    ${prefix}/mfx_vc1_dec_common.cpp
    ${prefix}/mfx_vpx_dec_common.cpp
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
foreach( prefix ${MSDK_LIB_ROOT}/genx/copy_kernels/src )
  list( APPEND sources
    ${prefix}/genx_cht_copy_isa.cpp
    ${prefix}/genx_skl_copy_isa.cpp
    ${prefix}/genx_icl_copy_isa.cpp
    ${prefix}/genx_icllp_copy_isa.cpp
    ${prefix}/genx_cnl_copy_isa.cpp
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

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( mfxlibname mfxhw64 )
else()
  set( mfxlibname mfxhw32 )
endif()

if( API_VERSION VERSION_GREATER 1.25 )
  set(MCTF_LIB "mctf_hw")
else()
  set(MCTF_LIB "")
endif()

set( LIBS "" )
list( APPEND LIBS
  mfx_trace_hw
  bitrate_control
  h264_dec_hw
  h265_dec_hw
  jpeg_common
  jpeg_dec_hw
  mpeg2_dec_hw
  vc1_common
  vc1_dec_hw
  vp8_dec_hw
  vp9_dec_hw
  umc
  vm
  vm_plus
  umc_va_hw
  cmrt_cross_platform_hw
  encode_hw
  h264_la
  genx_h264_encode_embeded
  mfx_common
  mfx_common_hw
  asc
  ${MCTF_LIB}
  vpp_hw
  h264_common
  h264_preenc
  h264_enc
  h264_pak
  mfx_trace
  ${ITT_LIBRARIES}
  pthread
  dl
)

make_library( ${mfxlibname} hw shared )
make_library( mfxhw_static hw static )

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
