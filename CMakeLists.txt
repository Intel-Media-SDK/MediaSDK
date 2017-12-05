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

cmake_minimum_required( VERSION 2.6.2 )
project( mediasdk )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set( BUILDER_ROOT "${CMAKE_HOME_DIRECTORY}/builder" )

if( NOT DEFINED CMAKE_MFX_HOME )
  set( CMAKE_MFX_HOME "${CMAKE_HOME_DIRECTORY}/api" )
endif()

if( "$ENV{MFX_HOME}" STREQUAL "" )
  set( ENV{MFX_HOME} ${CMAKE_HOME_DIRECTORY} )
endif()

option( ENABLE_OPENCL "Build targets dependent on OpenCL?" ON )
if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  option( ENABLE_DRM "Build DRM versions of the targets?" ON )
  option( ENABLE_X11 "Build X11 versions of the targets?" ON )
  option( ENABLE_X11_DRI3 "Build X11 DRI3 versions of the targets?" OFF )
  option( ENABLE_WAYLAND "Build WAYLAND versions of the targets?" OFF )
endif()

include( ${BUILDER_ROOT}/FindOpenCL.cmake )
include( ${BUILDER_ROOT}/FindFunctions.cmake )
include( ${BUILDER_ROOT}/FindGlobals.cmake )
include( ${BUILDER_ROOT}/FindMFX.cmake )
include( ${BUILDER_ROOT}/FindPackages.cmake )
include( ${BUILDER_ROOT}/FindInternals.cmake )
include( ${BUILDER_ROOT}/FindTrace.cmake )

if( EXISTS "${BUILDER_ROOT}/ConfTargets.cmake" )
  include( ${BUILDER_ROOT}/ConfTargets.cmake )
else()
  message( STATUS "There is no ConfTargets.cmake" )
endif()

foreach( dir api contrib _studio samples )
  set( CMAKE_SOURCE_DIR "${CMAKE_HOME_DIRECTORY}/${dir}" )
  create_build()
endforeach()

create_plugins_cfg(${CMAKE_BINARY_DIR})
