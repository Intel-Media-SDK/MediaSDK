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

if( Linux )
  set( os_arch "lin" )
elseif( Darwin )
  set( os_arch "darwin" )
endif()
if( __ARCH MATCHES ia32)
  set( os_arch "${os_arch}_ia32" )
else()
  set( os_arch "${os_arch}_x64" )
endif()

if( Linux OR Darwin )
  if(CMAKE_MFX_HOME)
    set( MFX_API_HOME ${CMAKE_MFX_HOME} )
  else()
    set( MFX_API_HOME $ENV{MFX_HOME} )
  endif()

  find_path( MFX_INCLUDE mfxdefs.h PATHS ${MFX_API_HOME}/include )
  find_library( MFX_LIBRARY libmfx.a PATHS ${MFX_API_HOME}/lib PATH_SUFFIXES ${os_arch} )

  include_directories( ${MFX_INCLUDE} )
  get_filename_component( MFX_LIBRARY_PATH ${MFX_LIBRARY} PATH )
  link_directories( ${MFX_LIBRARY_PATH} )

else()
  set( MFX_INCLUDE NOTFOUND )
  set( MFX_LIBRARY NOTFOUND )

endif()

if(NOT MFX_INCLUDE MATCHES NOTFOUND)
  set( MFX_FOUND TRUE )
  include_directories( ${MFX_INCLUDE} )
endif()

if(NOT DEFINED MFX_FOUND)
  message( FATAL_ERROR "Intel(R) Media SDK was not found (required)! Set/check MFX_HOME environment variable!")
else ()
  message( STATUS "Intel(R) Media SDK was found here $ENV{MFX_HOME}")
endif()

if(NOT MFX_LIBRARY MATCHES NOTFOUND)
  get_filename_component(MFX_LIBRARY_PATH ${MFX_LIBRARY} PATH )
  link_directories( ${MFX_LIBRARY_PATH} )
endif()

if( Linux )
  set(MFX_LDFLAGS "-Wl,--default-symver" )
endif()

