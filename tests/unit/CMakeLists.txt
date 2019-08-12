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

if (BUILD_DISPATCHER)
  add_subdirectory(suites/mfx_dispatch/linux)
endif()

# If some test suite wants to run during build-time
# it have to register itself via 'set_property', e.g.
## set_property(GLOBAL APPEND PROPERTY UNIT_TEST_SUITES "my_test_suite_target_name")
get_property(UNIT_TEST_SUITES GLOBAL PROPERTY UNIT_TEST_SUITES)

add_custom_target( test_suites
  ALL
  DEPENDS ${UNIT_TEST_SUITES}
)

add_custom_command(TARGET test_suites
  COMMENT "Run tests"
  POST_BUILD
  COMMAND ctest ARGS --output-on-failure
)
