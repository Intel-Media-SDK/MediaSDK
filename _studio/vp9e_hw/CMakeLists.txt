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

set( MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs()

set( VP9_Encoder_HW_GUID "ce44ef6f1a6d2246b412bb38d6e45182" )

# Plugin version info
set( VP9E_HW_PRODUCT_NAME "Intel(R) Media SDK" )
set( VP9E_HW_DESCRIPTION "Intel(R) Media SDK VP9 Hardware Encode plugin - Compatibility Stub" )

if( NOT DEFINED ENV{MFX_VP9_VERSION} )
  set( vp9_version 0.0.000.0000 )
else()
  set( vp9_version $ENV{MFX_VP9_VERSION} )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( plugin_name mfx_vp9e_hw64 )
else()
  set( plugin_name mfx_vp9e_hw32 )
endif()

set_file_and_product_version( ${vp9_version} version_defs )

set( sources "" )
set( sources.plus "" )
set( LIBS "" )
set( defs "" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/vp9/include )
list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_encode_plugin.cpp )

set( USE_STRICT_NAME TRUE )
set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_STUDIO_ROOT}/mfx_lib/plugin/libmfxsw_plugin.map" )
set( defs "${defs} -DMFX_PLUGIN_PRODUCT_NAME=\"\\\"${VP9E_HW_PRODUCT_NAME}\"\\\"" )
set( defs "${defs} -DMFX_FILE_DESCRIPTION=\"\\\"${VP9E_HW_DESCRIPTION}\"\\\"" )
set( defs "${defs} ${version_defs}" )
gen_plugins_cfg( "VP9_Encoder_HW" ${VP9_Encoder_HW_GUID} ${plugin_name} "02" "VP9" )
make_library( ${plugin_name} hw shared )
install( TARGETS ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
