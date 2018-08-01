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

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}")
mfx_include_dirs( )

set( ASC_DIR "${MSDK_LIB_ROOT}/../shared/asc" )

include_directories( ${ASC_DIR}/include )
include_directories( ${ASC_DIR}/../../mfx_lib/cmrt_cross_platform/include )


set( defs "" )
set( sources "" )
set( sources.plus "" )

list( APPEND sources
    ${ASC_DIR}/src/asc.cpp
    ${ASC_DIR}/src/asc_avx2_impl.cpp
    ${ASC_DIR}/src/asc_c_impl.cpp
    ${ASC_DIR}/src/asc_sse4_impl.cpp
    ${ASC_DIR}/src/genx_scd_bdw_isa.cpp
    ${ASC_DIR}/src/genx_scd_bxt_isa.cpp
    ${ASC_DIR}/src/genx_scd_cnl_isa.cpp
    ${ASC_DIR}/src/genx_scd_skl_isa.cpp
    ${ASC_DIR}/src/iofunctions.cpp
    ${ASC_DIR}/src/motion_estimation_engine.cpp
    ${ASC_DIR}/src/tree.cpp
)

make_library(asc_gp none static )
set( defs "" )
