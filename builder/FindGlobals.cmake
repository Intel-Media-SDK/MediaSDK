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

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set( CMAKE_VERBOSE_MAKEFILE             TRUE )

# the following options should disable rpath in both build and install cases
set( CMAKE_INSTALL_RPATH "" )
set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
set( CMAKE_SKIP_BUILD_RPATH TRUE )

collect_oses()

if( Linux OR Darwin )
  # If user did not override CMAKE_INSTALL_PREFIX, then set the default prefix
  # to /opt/intel/mediasdk instead of cmake's default
  if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
    set( CMAKE_INSTALL_PREFIX /opt/intel/mediasdk CACHE PATH "Install Path Prefix" FORCE )
  endif()

  include( GNUInstallDirs )

  add_definitions(-DUNIX)

  if( Linux )
    add_definitions(-D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64)

    add_definitions(-DLINUX)
    add_definitions(-DLINUX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DLINUX64)
    endif()
  endif()

  if( Darwin )
    add_definitions(-DOSX)
    add_definitions(-DOSX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DOSX64)
    endif()
  endif()

  # Potential source of confusion here. Environment $MFX_VERSION translates to product name (strings libmfxhw64.so | grep mediasdk),
  # but macro definition MFX_VERSION should contain API version i.e. 1025 for API 1.25
  if( NOT DEFINED ENV{MFX_VERSION} )
    set( version 0.0.000.0000 )
  else()
    set( version $ENV{MFX_VERSION} )
  endif()

  if( Linux OR Darwin )
    execute_process(
      COMMAND echo
      COMMAND cut -f 1 -d.
      COMMAND date "+.%-y.%-m.%-d"
      OUTPUT_VARIABLE cur_date
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string( SUBSTRING ${version} 0 1 ver )

    set( git_commit "" )
    git_describe( git_commit )

    add_definitions( -DMFX_FILE_VERSION=\"${ver}${cur_date}${git_commit}\")
    add_definitions( -DMFX_PRODUCT_VERSION=\"${version}\" )
    add_definitions( -DMSDK_BUILD=\"$ENV{BUILD_NUMBER}\")
  endif()

  if (CMAKE_C_COMPILER MATCHES icc)
    set(no_warnings "-Wno-deprecated -Wno-unknown-pragmas -Wno-unused")
  else()
    set(no_warnings "-Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-unused")
  endif()

  set(warnings "-Wall -Wformat -Wformat-security -Wnon-virtual-dtor")

  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -pipe -fPIC ${warnings} ${no_warnings}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fPIC ${warnings} ${no_warnings}")
  append("-fPIE -pie" CMAKE_EXEC_LINKER_FLAGS)

  # CACHE + FORCE should be used only here to make sure that this parameters applied globally
  # End user is responsible to adjust configuration parameters further if needed. Here
  # we se only minimal parameters which are really required for the proper configuration build.
  set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG"   CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}"          CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}"        CACHE STRING "" FORCE)

  if ( Darwin )
    if (CMAKE_C_COMPILER MATCHES clang)
       set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -v -std=c++11 -stdlib=libc++")
    endif()
  endif()

  if (DEFINED CMAKE_FIND_ROOT_PATH)
#    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " CMAKE_C_FLAGS)
#    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " CMAKE_CXX_FLAGS)
    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " LINK_FLAGS)
  endif (DEFINED CMAKE_FIND_ROOT_PATH)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    append("-m64 -g" CMAKE_C_FLAGS)
    append("-m64 -g" CMAKE_CXX_FLAGS)
    append("-m64 -g" LINK_FLAGS)
  else ()
    append("-m32 -g" CMAKE_C_FLAGS)
    append("-m32 -g" CMAKE_CXX_FLAGS)
    append("-m32 -g" LINK_FLAGS)
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(/usr/lib64)
  else()
    link_directories(/usr/lib)
  endif()
elseif( Windows )
  if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
    set( CMAKE_INSTALL_PREFIX "C:/Program Files/mediasdk/" CACHE PATH "Install Path Prefix" FORCE )
  endif()
endif( )

set( MFX_SAMPLES_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/samples )
set( MFX_SAMPLES_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/samples )

if( NOT DEFINED MFX_PLUGINS_DIR )
  set( MFX_PLUGINS_DIR ${CMAKE_INSTALL_PREFIX}/plugins )
endif( )

if( NOT DEFINED MFX_MODULES_DIR )
  set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
endif( )

message( STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}" )

# Some font definitions: colors, bold text, etc.
if(NOT Windows)
  string(ASCII 27 Esc)
  set(EndColor   "${Esc}[m")
  set(BoldColor  "${Esc}[1m")
  set(Red        "${Esc}[31m")
  set(BoldRed    "${Esc}[1;31m")
  set(Green      "${Esc}[32m")
  set(BoldGreen  "${Esc}[1;32m")
endif()

# Usage: report_targets( "Description for the following targets:" [targets] )
# Note: targets list is optional
function(report_targets description )
  message("")
  message("${ARGV0}")
  foreach(target ${ARGV1})
    message("  ${target}")
  endforeach()
  message("")
endfunction()

# Permits to accumulate strings in some variable for the delayed output
function(report_add_target var target)
  set(${ARGV0} ${${ARGV0}} ${ARGV1} CACHE INTERNAL "" FORCE)
endfunction()
