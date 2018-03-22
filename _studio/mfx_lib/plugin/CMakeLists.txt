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

set(HEVC_DECODER_HW_GUID   "33a61c0b4c27454ca8d85dde757c6f8e")
set(H264LA_ENCODER_GUID    "588f1185d47b42968dea377bb5d0dcb4")
set(VP8_DECODER_HW_GUID    "f622394d8d87452f878c51f2fc9b4131")

# =============================================================================

mfx_include_dirs()

include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
include_directories( ${MSDK_LIB_ROOT}/plugin/include )
include_directories( ${MSDK_LIB_ROOT}/scheduler/include )
include_directories( ${MSDK_LIB_ROOT}/genx/copy_kernels/include )

list( APPEND umc_dirs
  brc
  mpeg2_dec h264_dec h264_enc
  h265_dec
  )

foreach( dir ${umc_dirs} )
  include_directories( ${MSDK_STUDIO_ROOT}/shared/umc/codec/${dir}/include )
endforeach()

list( APPEND vdecs h264 mpeg2 vp8)
foreach( dec ${vdecs} )
  list( APPEND vdirs_impl decode/${dec} )
endforeach()

list( APPEND vencs h264 mpeg2 )
foreach( enc ${vencs} )
  list( APPEND vdirs_hw encode_hw/${enc} )
endforeach()

list( APPEND vdirs_impl vpp cmrt_cross_platform genx/h264_encode)
foreach( dir  ${vdirs} ${vdirs_impl} ${vdirs_hw} )
  include_directories( ${MSDK_LIB_ROOT}/${dir}/include )
endforeach()

# =============================================================================
# Plugins section
# =============================================================================

set( prefix ${MSDK_LIB_ROOT}/shared/src )
list( APPEND plugin_common_sources
  ${prefix}/mfx_common_int.cpp
  ${prefix}/mfx_common_decode_int.cpp
  ${prefix}/libmfxsw.cpp
  ${prefix}/libmfxsw_async.cpp
  ${prefix}/libmfxsw_query.cpp
  ${prefix}/libmfxsw_session.cpp
  ${prefix}/mfx_session.cpp
  ${prefix}/mfx_user_plugin.cpp
  ${prefix}/mfx_enc_common.cpp
  ${prefix}/mfx_critical_error_handler.cpp
)

set( prefix ${MSDK_STUDIO_ROOT}/shared/src )
list( APPEND plugin_common_sources
  ${prefix}/cm_mem_copy.cpp
  ${prefix}/libmfx_allocator.cpp
  ${prefix}/libmfx_allocator_vaapi.cpp
  ${prefix}/libmfx_core.cpp
  ${prefix}/libmfx_core_factory.cpp
  ${prefix}/libmfx_core_vaapi.cpp
  ${prefix}/mfx_umc_alloc_wrapper.cpp
  ${MSDK_LIB_ROOT}/cmrt_cross_platform/src/cmrt_cross_platform.cpp
)

set( prefix ${MSDK_LIB_ROOT}/scheduler/src )
list( APPEND plugin_common_sources
  ${prefix}/mfx_scheduler_core.cpp
  ${prefix}/mfx_scheduler_core_iunknown.cpp
  ${prefix}/mfx_scheduler_core_ischeduler.cpp
  ${prefix}/mfx_scheduler_core_task.cpp
  ${prefix}/mfx_scheduler_core_task_management.cpp
  ${prefix}/mfx_scheduler_core_thread.cpp
)

set( prefix ${MSDK_LIB_ROOT}/genx/copy_kernels/src )
list( APPEND plugin_common_sources
  ${prefix}/genx_cht_copy_isa.cpp
  ${prefix}/genx_skl_copy_isa.cpp
)

###############################################################################
## HEVC Decoder                                                              ##
###############################################################################
if( DEFINED HEVC_DECODER_HW_GUID )
  set( version_defs "" )
  set( defs "" )
  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )

  include_directories( ${MSDK_LIB_ROOT}/decode/h265/include )
  include_directories( ${MSDK_STUDIO_ROOT}/shared/umc/codec/h265_dec/include )

  ## umc_h265
  set( SRC_DIR "${MSDK_STUDIO_ROOT}/shared/umc/codec/h265_dec/src" )
  set( defs "-DAS_HEVCD_PLUGIN ${WARNING_FLAGS}" )
  file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
  list( APPEND sources ${srcs})
  make_library( umc_h265 hw static )

  ## cleanup
  set( version_defs "" )
  set( defs "" )
  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )

  ## plugin info: name, description and version
  set( HEVC_PRODUCT_NAME "Intel(R) Media SDK" )
  set( hw_HEVC_DECODER_DESCRIPTION "Intel(R) Media SDK HEVCd plugin" )

  if( NOT DEFINED ENV{MFX_HEVC_VERSION} )
    set( hevc_version 0.0.000.0000 )
  else()
    set( hevc_version $ENV{MFX_HEVC_VERSION} )
  endif()

  if( __ARCH MATCHES intel64 )
    set( plugin_name mfx_hevcd_hw64 )
  else()
    set( plugin_name mfx_hevcd_hw32 )
  endif()

  if( __CONFIG MATCHES debug )
    set( plugin_name ${plugin_name}_d )
  endif()

  set_file_and_product_version( ${hevc_version} version_defs )

  list( APPEND sources ${plugin_common_sources} )
  list( APPEND sources
    ${MSDK_LIB_ROOT}/shared/src/libmfxsw_decode.cpp
    ${MSDK_LIB_ROOT}/decode/h265/src/mfx_h265_dec_decode.cpp
    ${MSDK_LIB_ROOT}/plugin/src/mfx_hevc_dec_plugin.cpp
  )
  list( APPEND LIBS
    umc_h265_hw
    umc_va_hw
    umc
    vm
    vm_plus
    ${ITT_LIBS}
    pthread
    dl
  )

  ## defs
  set( USE_STRICT_NAME TRUE )
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/plugin/libmfxsw_plugin.map" )
  set( defs "${defs} -DMFX_VA -DVA_HEVC_DECODER -DAS_HEVCD_PLUGIN" )
  set( defs "${defs} -DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${HEVC_PRODUCT_NAME}\"\\\"" )
  set( defs "${defs} -DMFX_FILE_DESCRIPTION=\"\\\"${hw_HEVC_DECODER_DESCRIPTION}\"\\\"" )
  set( defs "${defs} ${version_defs} ${WARNING_FLAGS}" )
  gen_plugins_cfg("HEVC_Decoder_HW" ${HEVC_DECODER_HW_GUID} ${plugin_name} "01" "HEVC" "not_eval")
  make_library( ${plugin_name} hw shared)
  install( TARGETS ${plugin_name} ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
endif()

###############################################################################
## 1toN Lookahead (H264 LA)                                                  ##
###############################################################################

if( DEFINED H264LA_ENCODER_GUID )
  ## cleanup
  set( version_defs "" )
  set( defs "" )
  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )

  ## plugin info: name, description and version
  set( H264LA_PRODUCT_NAME "Intel(R) Media SDK" )
  set( hw_H264LA_ENCODER_DESCRIPTION "Intel(R) Media SDK Advanced AVCe plugin" )

  if( NOT DEFINED ENV{MFX_H264LA_VERSION} )
    set( h264la_version 0.0.000.0000 )
  else()
    set( h264la_version $ENV{MFX_H264LA_VERSION} )
  endif()

  if( __ARCH MATCHES intel64 )
    set( plugin_name mfx_h264la_hw64 )
  else()
    set( plugin_name mfx_h264la_hw32 )
  endif()

  if( __CONFIG MATCHES debug )
    set( plugin_name ${plugin_name}_d )
  endif()

  set_file_and_product_version( ${h264la_version} version_defs )

  ## include, source and libs
  list( APPEND sources ${plugin_common_sources} )

  set( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_enc_common.cpp
    ${prefix}/mfx_brc_common.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/libmfxsw_enc.cpp
  )

  set( prefix ${MSDK_LIB_ROOT}/encode_hw/h264/src )
  list( APPEND sources
    ${prefix}/mfx_h264_encode_cm.cpp
    ${prefix}/mfx_h264_encode_hw_utils.cpp
    ${prefix}/mfx_h264_encode_hw_utils_new.cpp
    ${prefix}/mfx_h264_encode_hw.cpp
  )

  set( prefix ${MSDK_LIB_ROOT}/genx/h264_encode/src )
  list( APPEND sources
    ${prefix}/genx_hsw_simple_me_isa.cpp
    ${prefix}/genx_bdw_simple_me_isa.cpp
    ${prefix}/genx_skl_simple_me_isa.cpp
    ${prefix}/genx_hsw_simple_me_proto.cpp
    ${prefix}/genx_hsw_histogram_isa.cpp
    ${prefix}/genx_skl_histogram_isa.cpp
  )
  list( APPEND sources
    ${MSDK_STUDIO_ROOT}/shared/umc/codec/brc/src/umc_video_brc.cpp
    ${MSDK_STUDIO_ROOT}/shared/umc/codec/brc/src/umc_h264_brc.cpp
    ${MSDK_STUDIO_ROOT}/shared/umc/codec/brc/src/umc_brc.cpp
  )

  list( APPEND sources ${MSDK_LIB_ROOT}/cmrt_cross_platform/src/cmrt_cross_platform.cpp )
  list( APPEND sources ${MSDK_LIB_ROOT}/fei/h264_la/mfx_h264_la.cpp )

  set( prefix ${MSDK_LIB_ROOT}/plugin/src )
  list( APPEND sources.plus
    ${prefix}/mfx_h264la_plugin.cpp
  )

  list( APPEND LIBS
    h264_la
    umc_va_hw
    umc
    vm
    vm_plus
    asc
    ${ITT_LIBS}
    pthread
    dl
  )

  ## defs
  set( USE_STRICT_NAME TRUE )
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/plugin/libmfxsw_plugin.map" )
  set( defs "${defs} -DAS_H264LA_PLUGIN -DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${H264LA_PRODUCT_NAME}\"\\\"" )
  set( defs "${defs} -DMFX_FILE_DESCRIPTION=\"\\\"${hw_H264LA_ENCODER_DESCRIPTION}\"\\\"" )
  set( defs "${defs} ${version_defs} ${WARNING_FLAGS}" )
  gen_plugins_cfg("H264la_Encoder" ${H264LA_ENCODER_GUID} ${plugin_name} "04" "AVC")
  make_library( ${plugin_name} hw shared)
  install( TARGETS ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
endif()

###############################################################################
## VP8                                                                       ##
###############################################################################
if( DEFINED VP8_DECODER_HW_GUID )
  ## cleanup
  set( version_defs "" )
  set( defs "" )
  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )

  ## plugin info: name, description and version
  set( VP8_PRODUCT_NAME "Intel(R) Media Server Studio 2017 - SDK VP8 Pack" )
  set( hw_VP8_DECODER_DESCRIPTION "Intel(R) Media Server Studio 2017 - SDK VP8 Hardware Decode Plug-in" )

  if( NOT DEFINED ENV{MFX_VP8_VERSION} )
    set( vp8_version 0.0.000.0000 )
  else()
    set( vp8_version $ENV{MFX_VP8_VERSION} )
  endif()

  if( __ARCH MATCHES intel64 )
    set( plugin_name mfx_vp8d_hw64 )
  else()
    set( plugin_name mfx_vp8d_hw32 )
  endif()
  if( __CONFIG MATCHES debug )
    set( plugin_name ${plugin_name}_d )
  endif()

  set_file_and_product_version( ${vp8_version} version_defs )

  ## include, source and libs
  include_directories( ${MSDK_LIB_ROOT}/scheduler/include )

  list( APPEND sources ${plugin_common_sources} )
  list( APPEND sources ${MSDK_LIB_ROOT}/shared/src/libmfxsw_decode.cpp )
  list( APPEND sources ${MSDK_LIB_ROOT}/shared/src/mfx_vpx_dec_common.cpp )

  set( prefix ${MSDK_LIB_ROOT}/plugin/src )
  list( APPEND sources.plus
    ${prefix}/mfx_vp8_dec_plugin.cpp
  )
  set( prefix ${MSDK_LIB_ROOT}/decode/vp8/src )
  list( APPEND sources
    ${prefix}/mfx_vp8_dec_decode_hw.cpp
    ${prefix}/mfx_vp8_dec_decode_common.cpp
    ${prefix}/mfx_vp8_dec_decode_hw_vaapi.cpp
    ${prefix}/mfx_vp8_dec_decode_tables.cpp
  )
  set( prefix ${MSDK_STUDIO_ROOT}/shared/umc/io/umc_va/src )
  list( APPEND sources
    ${prefix}/umc_va_linux.cpp
  )

  list( APPEND LIBS
    umc
    vm
    vm_plus
    va
    umc_va_hw
    ${ITT_LIBS}
    pthread
    dl
  )

  ## defs
  set( USE_STRICT_NAME TRUE )
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/plugin/libmfxsw_plugin.map" )
  set( defs " -DAS_VP8D_PLUGIN -DUMC_ENABLE_VP8_VIDEO_DECODER -DMFX_VA"
            " -DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${VP8_PRODUCT_NAME}\"\\\""
            " -DMFX_FILE_DESCRIPTION=\"\\\"${hw_VP8_DECODER_DESCRIPTION}\"\\\""
            " ${WARNING_FLAGS} ${version_defs}")
  gen_plugins_cfg( "VP8_Decoder_HW" ${VP8_DECODER_HW_GUID} ${plugin_name} "01" "VP8" )
  make_library( ${plugin_name} hw shared)
  install( TARGETS ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
endif()
