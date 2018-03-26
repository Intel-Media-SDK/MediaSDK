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

mfx_include_dirs( )
# =============================================================================
# msdk_gmock:
#
include_directories (
  ${MSDK_SAMPLES_ROOT}/sample_common/include/vm
  ${MSDK_SAMPLES_ROOT}/sample_common/include
  ${CMAKE_HOME_DIRECTORY}/mdp_msdk-val-tools/bs_parser/include
  ${CMAKE_HOME_DIRECTORY}/mdp_msdk-api/mediasdk_structures
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/gtest/include
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/include
  ${CMAKE_HOME_DIRECTORY}/mdp_msdk-contrib/yaml-cpp/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/gmock
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${MSDK_TSUITE_ROOT}/mfx_serializer/include
)

list(APPEND sources
   ${MSDK_SAMPLES_ROOT}/sample_common/src/avc_bitstream.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/avc_nal_spl.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/avc_spl.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/base_allocator.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_device.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_allocator.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_utils.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_utils_drm.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_utils_x11.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vaapi_utils_android.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/sample_utils.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/sysmem_allocator.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/atomic.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/shared_object.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/thread.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/time_linux.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/atomic_linux.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/shared_object_linux.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/thread_linux.cpp
   ${MSDK_SAMPLES_ROOT}/sample_common/src/vm/time.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/test_sample_allocator.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_alloc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_bitstream.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_common.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_decoder.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_encoder.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_plugin.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_session.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_streams.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_struct.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_surface.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_trace.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_vpp.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_preenc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_enc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_pak.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_transcoder.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/ts_encpak.cpp
)

file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/test_suites/*.cpp" )
list( APPEND sources ${srcs})

# enable fei specific code
find_path( VA_FEI va/va_fei.h PATHS ${PKG_LIBVA_INCLUDEDIR} )
if(VA_FEI MATCHES NOTFOUND)
  file( GLOB_RECURSE hevc_fei_src "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/test_suites/hevc_fei*.cpp")
  file( GLOB_RECURSE hevc_fei_src "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/test_suites/hevce_fei*.cpp")
  list( REMOVE_ITEM sources ${hevc_fei_src})
else()
  include_directories(${MSDK_SAMPLES_ROOT}/sample_hevc_fei/include)
  list(APPEND sources ${MSDK_SAMPLES_ROOT}/sample_hevc_fei/src/fei_buffer_allocator.cpp)
endif()

#if(NOT API_USE_LATEST)
#  file( GLOB_RECURSE vp9_src "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock/test_suites/vp9*.cpp")
#  list( REMOVE_ITEM sources ${vp9_src})
#endif()


add_definitions( -D_VARIADIC_MAX=10 )
add_definitions( -DENABLE_PS=1 )

set( LIBS "" )
set( LIBS_SUFFIX "" )

list( APPEND LIBS
  dispatch_trace
  dl
  pthread
#  mfx_serializer
)
list( APPEND LIBS_SUFFIX gmock )

set( defs "-DMFX_VERSION_USE_LATEST ${WARNING_FLAGS}" )
make_executable( msdk_gmock universal )
set( defs "" )

# =============================================================================
# msdk_ts application
set (sources "")

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_HOME_DIRECTORY}/mdp_msdk-val-tools/bs_parser/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../_studio/shared/umc/codec/demuxer/include
  ${MSDK_SAMPLES_ROOT}/sample_common/include
)

list(APPEND sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/msdk_ts.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_blocks.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_tools.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_trace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_verif.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_libva_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_sample_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/debug_output.cpp
)

add_definitions( -DUSE_SAMPLE_ALLOCATOR=1 )

set( LIBS "" )
list( APPEND LIBS
  dispatch_trace
  umc
  vm
  vm_plus
  dl
  sample_common
  pthread
)

set( defs "-DMFX_VERSION_USE_LATEST ${WARNING_FLAGS}" )
make_executable( msdk_ts universal )
set( defs "" )

# =============================================================================
# msdk_ts_dll library

set( sources "" )

#find_package ( Perl )
#find_package ( PerlLibs )
set( PERL_EXECUTABLE "perl" )

execute_process ( COMMAND ${PERL_EXECUTABLE}
                  -MConfig -e "print \$Config{version}"
                  OUTPUT_VARIABLE PERL_OUTPUT
                  RESULT_VARIABLE PERL_RETURN_VALUE )
if ( NOT PERL_RETURN_VALUE )
  set ( PERL_VERSION ${PERL_OUTPUT} )
endif ( NOT PERL_RETURN_VALUE )

#if ( PERL_INCLUDE_PATH MATCHES .*-NOTFOUND )
  execute_process ( COMMAND ${PERL_EXECUTABLE}
                    -MConfig -e "print \$Config{archlibexp}"
                    OUTPUT_VARIABLE PERL_OUTPUT
                    RESULT_VARIABLE PERL_RETURN_VALUE )
  if ( NOT PERL_RETURN_VALUE )
    set ( PERL_INCLUDE_PATH ${PERL_OUTPUT}/CORE )
  endif ( NOT PERL_RETURN_VALUE )
#endif ( PERL_INCLUDE_PATH MATCHES .*-NOTFOUND )

#if ( PERL_LIBRARY MATCHES .*-NOTFOUND )
  execute_process ( COMMAND ${PERL_EXECUTABLE}
                    -MConfig -e "print \$Config{libperl}"
                    OUTPUT_VARIABLE PERL_OUTPUT
                    RESULT_VARIABLE PERL_RETURN_VALUE )
  if ( NOT PERL_RETURN_VALUE )
    #set ( PERL_LIBRARY /usr/lib/${PERL_OUTPUT} )
    find_file( PERL_LIBRARY ${PERL_OUTPUT}
          PATHS "/usr/lib/" ${PERL_INCLUDE_PATH} )
  endif ( NOT PERL_RETURN_VALUE )
#endif ( PERL_LIBRARY MATCHES .*-NOTFOUND )

message ( STATUS "  PERL_VERSION:           " ${PERL_VERSION} )
message ( STATUS "  PERL_EXECUTABLE:        " ${PERL_EXECUTABLE} )
message ( STATUS "  PERL_INCLUDE_PATH:      " ${PERL_INCLUDE_PATH} )
message ( STATUS "  PERL_LIBRARY:           " ${PERL_LIBRARY} )

include_directories (
  ${PERL_INCLUDE_PATH}
)

add_definitions( -DUSE_SAMPLE_ALLOCATOR=1 )

list(APPEND sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dll_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/msdk_ts.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_blocks.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_tools.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_trace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_verif.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_libva_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/msdk_ts_wrap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_sample_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/debug_output.cpp
)

set( LIBS "" )
list( APPEND LIBS
  dispatch_trace
  umc
  vm
  vm_plus
  dl
  pthread
  sample_common
  ${PERL_LIBRARY}
)

make_library( msdk_ts_dll universal shared )
