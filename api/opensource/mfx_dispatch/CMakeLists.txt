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

cmake_minimum_required( VERSION 2.8.5 FATAL_ERROR )
project( mfx )

set( MFX_API_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

# While equal to get_mfx_version in samples/builder, this function should remain separate to make this file self-sufficient 
function( get_api_version mfx_version_major mfx_version_minor )
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h major REGEX "#define MFX_VERSION_MAJOR" LIMIT_COUNT 1)
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h minor REGEX "#define MFX_VERSION_MINOR" LIMIT_COUNT 1)
  string(REPLACE "#define MFX_VERSION_MAJOR " "" major ${major})
  string(REPLACE "#define MFX_VERSION_MINOR " "" minor ${minor})
  set(${mfx_version_major} ${major} PARENT_SCOPE)
  set(${mfx_version_minor} ${minor} PARENT_SCOPE)
endfunction()

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__lib )

if( CMAKE_SYSTEM_NAME MATCHES Windows )
  add_definitions(-DMINGW_HAS_SECURE_API -DWIN32)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DWIN64)
  endif( )
else( )

  # If user did not override CMAKE_INSTALL_PREFIX, then set the default prefix
  # to /opt/intel/mediasdk instead of cmake's default
  if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
    set( CMAKE_INSTALL_PREFIX /opt/intel/mediasdk CACHE PATH "Install Path Prefix" FORCE )
  endif( )
  message( STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}" )

  include( GNUInstallDirs )

  if( NOT DEFINED MFX_PLUGINS_DIR )
    set( MFX_PLUGINS_DIR ${CMAKE_INSTALL_PREFIX}/plugins )
  endif( )
  add_definitions( -DMFX_PLUGINS_DIR="${MFX_PLUGINS_DIR}" )
  message( STATUS "MFX_PLUGINS_DIR=${MFX_PLUGINS_DIR}" )

  if( NOT DEFINED MFX_MODULES_DIR )
    set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
  endif( )
  add_definitions( -DMFX_MODULES_DIR="${MFX_MODULES_DIR}" )
  message( STATUS "MFX_MODULES_DIR=${MFX_MODULES_DIR}" )

  add_definitions(-DUNIX)

  if( CMAKE_SYSTEM_NAME MATCHES Linux )
    add_definitions(-D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64 -DLINUX -DLINUX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DLINUX64)
    endif( )
  endif( )

  if( CMAKE_SYSTEM_NAME MATCHES Darwin )
    add_definitions(-DOSX)
    add_definitions(-DOSX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DOSX64)
    endif( )
  endif( )

  set(no_warnings "-Wno-unknown-pragmas -Wno-unused")
  set(warnings "-Wall -Wformat -Wformat-security")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fPIC -std=c++11 ${warnings} ${no_warnings}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

  if (DEFINED CMAKE_FIND_ROOT_PATH)
    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " LINK_FLAGS)
  endif (DEFINED CMAKE_FIND_ROOT_PATH)
endif( )

include_directories (
  ${MFX_API_FOLDER}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

list(APPEND sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_critical_section.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_critical_section_linux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_function_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_library_iterator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_library_iterator_linux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_load_dll.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_load_dll_linux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_win_reg_key.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dxva2_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_plugin_hive.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_plugin_hive_linux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_plugin_cfg_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_load_plugin.cpp
  )

function( append_property target property_name property )
  get_target_property( property ${ARGV0} ${ARGV1} )
  if( property MATCHES NOTFOUND)
    set( property "" )
  endif( )
  string( REPLACE ";" " " property "${ARGV2} ${property}" )
  set_target_properties( ${ARGV0} PROPERTIES ${ARGV1} "${property}" )
endfunction( )

function( make_static_library name )
  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif( )

  set( target ${ARGV0} )

  add_library( ${target} STATIC ${include} ${sources} )

  set( link_flags_list "-Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack")
  append_property( ${target} LINK_FLAGS "${link_flags_list} -fstack-protector" )

  if( defs )
    append_property( ${target} COMPILE_FLAGS ${defs} )
  endif( )

  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${target} )
endfunction( )

set( defs "" )
make_static_library( mfx )

set( defs "-DMFX_DISPATCHER_EXPOSED_PREFIX" )
make_static_library( dispatch_shared )

list(APPEND sources.plus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dispatcher_log.cpp
  )

set( defs "-DMFX_DISPATCHER_LOG -DDXVA2DEVICE_LOG" )
make_static_library( dispatch_trace )

get_api_version(MFX_VERSION_MAJOR MFX_VERSION_MINOR)
set( PKG_CONFIG_FNAME "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/lib${PROJECT_NAME}.pc")
set( PKG_CONFIG_FNAME_SHARED "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/lib${PROJECT_NAME}-shared.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake" ${PKG_CONFIG_FNAME} @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pkg-config-shared.pc.cmake" ${PKG_CONFIG_FNAME_SHARED} @ONLY)

install( TARGETS mfx ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install( TARGETS dispatch_shared ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install( FILES ${PKG_CONFIG_FNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
install( FILES ${PKG_CONFIG_FNAME_SHARED} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
install( DIRECTORY ${MFX_API_FOLDER}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN *.h )

# For backwards compatibility, create a relative symbolic link without the "lib"
# prefix to the .pc file.
set( PKG_CONFIG_LFNAME "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.pc" )
add_custom_target(pc_link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink lib${PROJECT_NAME}.pc ${PKG_CONFIG_LFNAME})
install( FILES ${PKG_CONFIG_LFNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
