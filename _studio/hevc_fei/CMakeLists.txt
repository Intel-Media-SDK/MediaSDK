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

if(API_VERSION VERSION_LESS "1.27")
  message("Skipping HEVC FEI plugin due to API restriction")
else()

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

set(HEVC_FEI_Encoder_HW_GUID "5418a70666f94d5cb4f7b1caee86339b")

# Plugin version info
set (HEVC_FEI_HW_PRODUCT_NAME "Intel(R) Media Server Studio 2018 - HEVC FEI Encode")
set (HEVC_FEI_HW_DESCRIPTION "Intel(R) Media Server Studio 2018 - HEVC FEI Hardware Encode Plug-in")


if( NOT DEFINED ENV{MFX_HEVC_VERSION} )
  set( hevc_version 0.0.000.0000 )
else( )
  set( hevc_version $ENV{MFX_HEVC_VERSION} )
endif( )

if( Linux OR Darwin )
  execute_process(
    COMMAND echo
    COMMAND cut -f 1 -d.
    COMMAND date "+.%-y.%-m.%-d"
    OUTPUT_VARIABLE cur_date
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  string( SUBSTRING ${hevc_version} 0 1 ver )

  set(HEVCE_HW_version_defs  "-DMFX_PLUGIN_FILE_VERSION=\"\\\"${ver}${cur_date}\"\\\" -DMFX_PLUGIN_PRODUCT_VERSION=\"\\\"${hevc_version}\"\\\"")
  set(HEVCE_FEI_HW_version_defs  "-DMFX_PLUGIN_FILE_VERSION=\"\\\"${ver}${cur_date}\"\\\" -DMFX_PLUGIN_PRODUCT_VERSION=\"\\\"${hevc_version}\"\\\"")
endif( )

# =============================================================================

mfx_include_dirs( )

set(MFX_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories( ${MFX_ROOTDIR}/shared/include )
include_directories( ${MFX_ROOTDIR}/mfx_lib/encode_hw/h265/include )
include_directories( ${MFX_ROOTDIR}/mfx_lib/plugin/include )
include_directories( ${MFX_ROOTDIR}/mfx_lib/scheduler/include )
include_directories( ${MFX_ROOTDIR}/mfx_lib/cmrt_cross_platform/include )

# =============================================================================

set( sources "" )
set( sources.plus "" )

foreach( dir ${CMAKE_CURRENT_SOURCE_DIR}/../mfx_lib/encode_hw/h265/src )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()


set( defs "-DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${HEVCE_HW_PRODUCT_NAME}\"\\\" -DMFX_FILE_DESCRIPTION=\"\\\"${HEVCE_HW_DESCRIPTION}\"\\\" ${HEVCE_HW_version_defs}" )

set( defs " -DAS_HEVCE_PLUGIN ${defs}" )
make_library( mfx_hevce_lib hw static )

set( defs "" )

# =============================================================================

#shared lib
set( sources "" )
set( sources.plus "" )

set( USE_STRICT_NAME TRUE )
set(MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MFX_ROOTDIR}/mfx_lib/plugin/libmfxsw_plugin.map" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h265_fei/include )

foreach( dir ${CMAKE_CURRENT_SOURCE_DIR}/h265_fei/src )
  file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()

  set( LIBS "" )
  list( APPEND LIBS
    mfx_hevce_lib_hw
    mfx_common_hw
    umc
    vm
    vm_plus
    mfx_trace
    ${ITT_LIBRARIES}
    pthread
    dl
  )

set( defs "-DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${HEVC_FEI_HW_PRODUCT_NAME}\"\\\" -DMFX_FILE_DESCRIPTION=\"\\\"${HEVC_FEI_HW_DESCRIPTION}\"\\\" ${HEVCE_FEI_HW_version_defs}" )

gen_plugins_cfg("HEVC_Encoder_HW" ${HEVC_FEI_Encoder_HW_GUID} "mfx_hevc_fei_hw64" "02" "HEVC")

set( defs " -DAS_HEVCE_PLUGIN -DAS_HEVC_FEI_ENCODE_PLUGIN ${defs}" )
make_library( mfx_hevc_fei_hw64 hw shared )
msdk_install( mfx_hevc_fei_hw64 ${MFX_PLUGINS_DIR} )
set( defs "" )
endif( )
