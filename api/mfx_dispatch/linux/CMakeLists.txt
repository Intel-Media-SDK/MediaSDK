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

set( MFX_API_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

# If user did not override CMAKE_INSTALL_PREFIX, then set the default prefix
# to /opt/intel/mediasdk instead of cmake's default
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set( CMAKE_INSTALL_PREFIX /opt/intel/mediasdk CACHE PATH "Install Path Prefix" FORCE )
endif( )
message( STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}" )

if( NOT DEFINED MFX_PLUGINS_CONF_DIR )
  set( MFX_PLUGINS_CONF_DIR ${CMAKE_INSTALL_FULL_DATADIR}/mfx )
endif( )
add_definitions( -DMFX_PLUGINS_CONF_DIR="${MFX_PLUGINS_CONF_DIR}" )
message( STATUS "MFX_PLUGINS_CONF_DIR=${MFX_PLUGINS_CONF_DIR}" )

if( NOT DEFINED MFX_MODULES_DIR )
  set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
endif( )
add_definitions( -DMFX_MODULES_DIR="${MFX_MODULES_DIR}" )
message( STATUS "MFX_MODULES_DIR=${MFX_MODULES_DIR}" )

add_definitions(-DUNIX)
add_definitions(-DMFX_DEPRECATED_OFF)

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

list(APPEND sources
  ${CMAKE_CURRENT_SOURCE_DIR}/mfxloader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mfxparser.cpp
)

include_directories (
  ${MFX_API_FOLDER}
)

add_library(mfx SHARED ${sources})
target_link_libraries(mfx dl)

set_target_properties( mfx PROPERTIES LINK_FLAGS
  "-Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libmfx.map -fstack-protector")
set_target_properties( mfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER mfx )
set_target_properties( mfx PROPERTIES   VERSION ${MFX_VERSION_MAJOR}.${MFX_VERSION_MINOR})
set_target_properties( mfx PROPERTIES SOVERSION ${MFX_VERSION_MAJOR})

install(TARGETS mfx LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install( DIRECTORY ${MFX_API_FOLDER}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mfx FILES_MATCHING PATTERN *.h )
