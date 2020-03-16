# Copyright (c) 2017-2020 Intel Corporation
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

  mfx_include_dirs( )

  set(HEVC_FEI_Encoder_HW_GUID "5418a70666f94d5cb4f7b1caee86339b")

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( plugin_name mfx_hevc_fei_hw64 )
  else()
    set( plugin_name mfx_hevc_fei_hw32 )
  endif()

  set_file_and_product_version( ${MEDIA_VERSION_STR} version_defs )

  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )
  set( defs "" )

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h265_fei/include )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../mfx_lib/shared/include )
  list( APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/h265_fei/src/export.cpp)

  set( USE_STRICT_NAME TRUE )
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_STUDIO_ROOT}/mfx_lib/plugin/libmfxsw_plugin.map" )
  set( defs "${defs} ${version_defs}" )
  gen_plugins_cfg("HEVC_Encoder_HW" ${HEVC_FEI_Encoder_HW_GUID} ${plugin_name} "02" "HEVC")
  make_library( ${plugin_name} hw shared )
  install( TARGETS ${plugin_name} LIBRARY DESTINATION ${MFX_PLUGINS_DIR} )
endif( )
