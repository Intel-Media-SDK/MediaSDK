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

if( Linux )
  set( os_arch "lin" )
elseif( Darwin )
  set( os_arch "darwin" )
elseif( Windows )
  set( os_arch "win" )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( os_arch "${os_arch}_x64" )
else()
  set( os_arch "${os_arch}_ia32" )
endif()

if( CMAKE_MFX_HOME )
  set( MFX_API_HOME ${CMAKE_MFX_HOME} )
else()
  set( MFX_API_HOME ${MFX_HOME} )
endif()

find_path( MFX_INCLUDE mfxdefs.h PATHS ${MFX_API_HOME}/include NO_CMAKE_FIND_ROOT_PATH )

if( NOT MFX_INCLUDE MATCHES NOTFOUND )
  include_directories( ${MFX_API_HOME}/mediasdk_structures )
  include_directories( ${MFX_INCLUDE} )
endif()

if( NOT MFX_INCLUDE MATCHES NOTFOUND )
  set( MFX_FOUND TRUE )
  include_directories( ${MFX_INCLUDE} )
endif()

if( NOT DEFINED MFX_FOUND )
  message( FATAL_ERROR "Intel(R) Media SDK was not found (required)! Set/check MFX_HOME environment variable!")
else()
  message( STATUS "Intel(R) Media SDK was found here ${MFX_HOME}")
endif()

# Potential source of confusion here. MFX_VERSION should contain API version i.e. 1025 for API 1.25, 
# Product version stored in MEDIA_VERSION_STR
if( NOT DEFINED API OR API STREQUAL "master")
  set( API_FLAGS "")
  set( API_USE_LATEST FALSE )
  get_mfx_version(major_vers minor_vers)
else( )
  if( API STREQUAL "latest" )
    # This would enable all latest non-production features     
    set( API_FLAGS -DMFX_VERSION_USE_LATEST )
    set( API_USE_LATEST TRUE )
    get_mfx_version(major_vers minor_vers)
  else()
    set( VERSION_REGEX "[0-9]+\\.[0-9]+" )

    # Breaks up a string in the form maj.min into two parts and stores
    # them in major, minor.  version should be a value, not a
    # variable, while major and minor should be variables.
    macro( split_api_version version major minor )
      if(${version} MATCHES ${VERSION_REGEX})
        string(REGEX REPLACE "^([0-9]+)\\.[0-9]+" "\\1" ${major} "${version}")
        string(REGEX REPLACE "^[0-9]+\\.([0-9]+)" "\\1" ${minor} "${version}")
      else(${version} MATCHES ${VERSION_REGEX})
        message("macro( split_api_version ${version} ${major} ${minor} ")
        message(FATAL_ERROR "Problem parsing API version string.")
      endif(${version} MATCHES ${VERSION_REGEX})
    endmacro( split_api_version )

    split_api_version(${API} major_vers minor_vers)
      # Compute a version number
    math(EXPR version_number "${major_vers} * 1000 + ${minor_vers}" )
    set(API_FLAGS -DMFX_VERSION=${version_number})
  endif()  
endif()

set( API_VERSION "${major_vers}.${minor_vers}")
if (NOT API_FLAGS STREQUAL "")
    add_definitions(${API_FLAGS})
endif()

message(STATUS "Enabling API ${major_vers}.${minor_vers} feature set with flags ${API_FLAGS}")

if( Linux )
  set( MFX_LDFLAGS "-Wl,--default-symver" )
endif()
