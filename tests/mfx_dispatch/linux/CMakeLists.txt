# Copyright (c) 2017-2018 Intel Corporation
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

# This test hooks dlopen and dlsym calls, i.e. it provides mock mediasdk
# library implementation inside. A key thing for the trick to work is
# avoiding non-default symbol versioning, i.e. -Wl,--default-symver which
# is propagated by mediasdk test system should be avoided. 

if( NOT DEFINED MFX_MODULES_DIR )
  set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
endif( )

add_definitions( -DMFX_MODULES_DIR="${MFX_MODULES_DIR}" )
add_definitions( -DMFX_PLUGINS_CONF_DIR="${MFX_PLUGINS_CONF_DIR}" )

add_executable(mfx_dispatch_test
  mfx_dispatch_test_main.cpp
  mfx_dispatch_test_cases_libs.cpp
  mfx_dispatch_test_cases_plugins.cpp
  mfx_dispatch_test_mocks.cpp
  mfx_dispatch_test_fixtures.cpp)

target_link_libraries( mfx_dispatch_test mfx gtest gmock pthread )

target_include_directories( mfx_dispatch_test PUBLIC ../../../api/mfx_dispatch/linux )

set_target_properties(mfx_dispatch_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE})

add_test(NAME run_mfx_dispatch_test
  COMMAND ./mfx_dispatch_test
  WORKING_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE})

set_property(TEST run_mfx_dispatch_test PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE}")

install(TARGETS mfx_dispatch_test RUNTIME DESTINATION ${MFX_APPS_DIR}/tests)

