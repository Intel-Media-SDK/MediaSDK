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

find_path( GTEST_INCLUDE gtest/gtest.h PATHS "$ENV{GTEST_INCLUDE_PATH}" "/usr/local/include" )
find_library( GTEST_LIBRARY gtest PATHS "$ENV{GTEST_LIBRARY_PATH}" "/usr/local/lib" )
find_library( GTEST_MAIN_LIBRARY gtest_main PATHS "$ENV{GTEST_LIBRARY_PATH}" "/usr/local/lib" )

if(NOT GTEST_INCLUDE MATCHES NOTFOUND)
  if(NOT GTEST_LIBRARY MATCHES NOTFOUND AND
        NOT GTEST_MAIN_LIBRARY MATCHES NOTFOUND)
    set( GTEST_FOUND TRUE )

    get_filename_component(GTEST_LIB_DIR ${GTEST_LIBRARY} PATH)
    get_filename_component(GTEST_MAIN_LIB_DIR ${GTEST_MAIN_LIBRARY} PATH)

    include_directories( "${GTEST_INCLUDE}" )
    link_directories( "${GTEST_LIB_DIR}" "${GTEST_MAIN_LIB_DIR}" )
  endif()
endif()

if(NOT DEFINED GTEST_FOUND)
  message( STATUS "Google tests libraries and headers were not found! Build GTest and install to /usr/local." )
else ()
  message( STATUS "Google tests libraries were found in ${GTEST_LIB_DIR}" )
  message( STATUS "Google tests headers were found in ${GTEST_INCLUDE}" )
endif()

