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

mfx_include_dirs()

set( CURRENT_SRC_ROOT "${MSDK_STUDIO_ROOT}/shared/umc" )
include_directories( ${CURRENT_SRC_ROOT}/../include )

### UMC core umc
set( sources "" )
file( GLOB_RECURSE srcs "${CURRENT_SRC_ROOT}/core/umc/src/*.c" "${CURRENT_SRC_ROOT}/core/umc/src/*.cpp" )
list( APPEND sources ${srcs})

set( defs "${WARNING_FLAGS}" )
make_library( umc none static )
set( defs "" )
### UMC core umc

### UMC vm
set( sources "" )
file( GLOB_RECURSE srcs "${CURRENT_SRC_ROOT}/core/vm/src/*.c" "${CURRENT_SRC_ROOT}/core/vm/src/*.cpp" )
list( APPEND sources ${srcs})

set( defs "${WARNING_FLAGS}" )
make_library( vm none static )
set( defs "" )
### UMC vm

### UMC vm plus
set( sources "" )
file( GLOB_RECURSE srcs "${CURRENT_SRC_ROOT}/core/vm_plus/src/*.c" "${CURRENT_SRC_ROOT}/core/vm_plus/src/*.cpp" )
list( APPEND sources ${srcs} )

set( defs "${WARNING_FLAGS}" )
make_library( vm_plus none static )
set( defs "" )
### UMC vm plus

### UMC VA
set( sources "" )
file( GLOB_RECURSE srcs "${CURRENT_SRC_ROOT}/io/umc_va/src/*.c" "${CURRENT_SRC_ROOT}/io/umc_va/src/*.cpp" )
list( APPEND sources ${srcs} )

set( defs "${WARNING_FLAGS}" )
make_library( umc_va hw static )
set( defs "" )
### UMC VA
