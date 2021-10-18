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

find_path( OPENCL_INCLUDE CL/opencl.h PATHS /usr/include /opt/intel/opencl/include)
find_library( OPENCL_LIBRARY libOpenCL.so PATHS /usr/lib /opt/intel/opencl/)

set( OCL_LIBS "" )

if ( NOT OPENCL_INCLUDE MATCHES NOTFOUND )
    if ( NOT OPENCL_LIBRARY MATCHES NOTFOUND )
        set ( OPENCL_FOUND TRUE )

        get_filename_component( OPENCL_LIBRARY_PATH ${OPENCL_LIBRARY} PATH )

        list( APPEND OPENCL_LIBS OpenCL )
    endif()
endif()

if ( NOT DEFINED OPENCL_FOUND )
  message( STATUS "Intel OpenCL SDK was not found (optional). The following will not be built: rotate_opencl plugin.")
else ()
  message( STATUS "Intel OpenCL SDK was found here: ${OPENCL_LIBRARY_PATH} and ${OPENCL_INCLUDE}" )
endif()
