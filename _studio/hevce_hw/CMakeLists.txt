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

set( MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs()

set( HEVC_Encoder_HW_GUID "6fadc791a0c2eb479ab6dcd5ea9da347" )

# Plugin version info
set( HEVCE_HW_PRODUCT_NAME "Intel(R) Media SDK" )
set( HEVCE_HW_DESCRIPTION "Intel(R) Media SDK HEVCe plugin" )

if( NOT DEFINED ENV{MFX_HEVC_VERSION} )
  set( hevc_version 0.0.000.0000 )
else()
  set( hevc_version $ENV{MFX_HEVC_VERSION} )
endif()

if( __ARCH MATCHES intel64 )
  set( plugin_name mfx_hevce_hw64 )
else()
  set( plugin_name mfx_hevce_hw32 )
endif()

if( __CONFIG MATCHES debug )
  set( plugin_name ${plugin_name}_d )
endif()

set_file_and_product_version( ${hevc_version} version_defs )

set( sources "" )
set( sources.plus "" )
set( LIBS "" )
set( defs "" )

include_directories( ${MSDK_STUDIO_ROOT}/hevce_hw/include )
include_directories( ${MSDK_STUDIO_ROOT}/hevce_hw/h265/include )
set( dir ${MSDK_STUDIO_ROOT}/hevce_hw/h265/src )
file( GLOB_RECURSE srcs "${dir}/*.c" "${dir}/*.cpp" )
list( APPEND sources ${srcs} )

list( APPEND LIBS
  mfx_common_hw
  umc_va_hw
  umc
  vm
  vm_plus
  ${ITT_LIBS}
  pthread
  dl
)

set( USE_STRICT_NAME TRUE )
set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_STUDIO_ROOT}/mfx_lib/plugin/libmfxsw_plugin.map" )
set( defs "${defs} -DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${HEVCE_HW_PRODUCT_NAME}\"\\\"" )
set( defs "${defs} -DMFX_FILE_DESCRIPTION=\"\\\"${HEVCE_HW_DESCRIPTION}\"\\\"" )
set( defs "${defs} ${WARNING_FLAGS} ${version_defs}" )
set( defs "-DAS_HEVCE_PLUGIN ${defs} " )
gen_plugins_cfg( "HEVC_Encoder_HW" ${HEVC_Encoder_HW_GUID} ${plugin_name} "02" "HEVC" )
make_library( ${plugin_name} hw shared )
install( TARGETS ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
