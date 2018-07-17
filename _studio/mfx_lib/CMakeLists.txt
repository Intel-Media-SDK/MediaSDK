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
    ${prefix}/mfx_critical_error_handler.cpp
    ${prefix}/mfx_vpx_dec_common.cpp
  )
endforeach()

if ("mpeg2" IN_LIST MFX_CODECS )
  foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
    list( APPEND sources
      ${prefix}/mfx_mpeg2_dec_common.cpp
    )
  endforeach()
endif()

set( defs "" )
make_library( mfx_common none static )
append_property( mfx_common COMPILE_FLAGS " -DMFX_RT" )

foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_ddi_enc_dump.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
  )
endforeach()

if ("mpeg2" IN_LIST MFX_CODECS )
  foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
    list( APPEND sources
      ${prefix}/mfx_mpeg2_enc_common_hw.cpp
      ${prefix}/mfx_mpeg2_encode_vaapi.cpp
      ${prefix}/mfx_mpeg2_encode_factory.cpp
    )
  endforeach()
endif()

set( defs "" )
make_library( mfx_common hw static )
append_property( mfx_common_hw COMPILE_FLAGS " -DMFX_RT" )

### libmfxhw
list( APPEND mdirs
  scheduler vpp 
  decode/h264 cmrt_cross_platform encode_hw/h264 
  decode/h265 decode/vp9 mctf_package/mctf
  )

if ("mpeg2" IN_LIST MFX_CODECS )
  list( APPEND mdirs
    decode/mpeg2 encde_hw/mpeg2
    )
endif()

if ("vc1" IN_LIST MFX_CODECS )
  list( APPEND mdirs
    decode/vc1
    )
endif()

if ("mjpeg" IN_LIST MFX_CODECS )
  list( APPEND mdirs
    decode/mjpeg encode_hw/mjpeg
    )
endif()

if ("vp8" IN_LIST MFX_CODECS )
  list( APPEND mdirs
    decode/vp8
    )
endif()

if ( ${MFX_USE_FEI} )
  include_directories( ${MSDK_LIB_ROOT}/fei/h264_common )
  include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
  include_directories( ${MSDK_LIB_ROOT}/fei/h264_pak )
  include_directories( ${MSDK_LIB_ROOT}/fei/h264_enc )
endif()

foreach( dir ${mdirs} )
  include_directories( ${MSDK_LIB_ROOT}/${dir}/include )
endforeach()

include_directories( ${MSDK_LIB_ROOT}/genx/copy_kernels/include )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
include_directories( ${MSDK_STUDIO_ROOT}/hevce_hw/h265/include )

list( APPEND cdirs
  brc h264_enc vp9_dec
  )

if ("mpeg2" IN_LIST MFX_CODECS )
  list( APPEND cdirs
    mpeg2_dec
    )
endif()

if ("vc1" IN_LIST MFX_CODECS )
  list( APPEND cdirs
    vc1_dec vc1_common 
    )
endif()

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

if ("mpeg2" IN_LIST MFX_CODECS )
  list( APPEND sources
    ${prefix}/mfx_mpeg2_dec_common.cpp
  )
endif()

if ("vc1" IN_LIST MFX_CODECS )
  list( APPEND sources
    ${prefix}/mfx_vc1_dec_common.cpp
  )
endif()


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

if ( ${MFX_USE_COPY_KERNEL_GENX} )
  foreach( prefix ${MSDK_LIB_ROOT}/genx/copy_kernels/src )
    list( APPEND sources
      ${prefix}/genx_cht_copy_isa.cpp
      ${prefix}/genx_skl_copy_isa.cpp
      ${prefix}/genx_icl_copy_isa.cpp
      ${prefix}/genx_icllp_copy_isa.cpp
      ${prefix}/genx_cnl_copy_isa.cpp
    )
  endforeach()
endif()

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
  umc
  vm
  vm_plus
  umc_va_hw
  cmrt_cross_platform_hw
  decode_hw
  mfx_common
  mfx_common_hw
  asc
  ${MCTF_LIB}
  vpp_hw
  ${ITT_LIBRARIES}
  pthread
  dl
)

if (("h264" IN_LIST MFX_CODECS))
  list( APPEND LIBS h264_dec_hw )
endif()

if (("h265" IN_LIST MFX_CODECS))
  list( APPEND LIBS umc_h265_hw )
endif()

if ( NOT ${MFX_TRACE_DISABLE} )
  list( APPEND LIBS
    mfx_trace
    mfx_trace_hw
  )
endif()

if ( ${MFX_USE_JPEG} )
  list( APPEND LIBS
    jpeg_common
    jpeg_dec_hw
  )
endif()

if ( ${MFX_USE_ENCODERS} )
  list( APPEND LIBS
    lbitrate_control
    h264_common
    lh264_la
    encode_hw
    h264_preenc
    h264_enc
    h264_pak
 )
endif()

if ( ${MFX_USE_COPY_KERNEL_GENX} )
  list( APPEND LIBS
    genx_h264_encode_embeded
 )
endif()

if ("mpeg2" IN_LIST MFX_CODECS )
  list( APPEND LIBS
    mpeg2_dec_hw
  )
endif()

if ("vc1" IN_LIST MFX_CODECS )
  list( APPEND LIBS
    vc1_common
    vc1_dec_hw
  )
endif()

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
