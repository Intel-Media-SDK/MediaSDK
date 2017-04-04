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

if(__ITT)
  if( Linux )
    if( __ARCH MATCHES intel64 )
      set( arch "64" )
    elseif()
      set( arch "32" )
    endif()

    find_path( ITT_INCLUDE ittnotify.h PATHS $ENV{ITT_PATH}/include )
    find_path( ITT_LIB libittnotify${arch}.a PATHS $ENV{ITT_PATH} )
    if(NOT ITT_INCLUDE MATCHES NOTFOUND AND NOT ITT_LIB MATCHES NOTFOUND)
      set( VTUNE_FOUND TRUE )
      message( STATUS "ITT_PATH is set to $ENV{ITT_PATH}" )

      include_directories( $ENV{ITT_PATH}/include )
      link_directories( $ENV{ITT_PATH}/ )

      append("-DMFX_TRACE_ENABLE_ITT" CMAKE_C_FLAGS)
      append("-DMFX_TRACE_ENABLE_ITT" CMAKE_CXX_FLAGS)

      set( ITT_LIBS "" )
      list( APPEND ITT_LIBS
        mfx_trace
        ittnotify${arch}
        dl
      )
    else()
      set( ITT_LIBS mfx_trace )
      set( VTUNE_FOUND FALSE )
    endif()
  endif()
endif()
