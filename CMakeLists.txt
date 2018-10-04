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

cmake_minimum_required( VERSION 2.8.5 )
project( mediasdk )

include(CMakeDependentOption)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to `release` build if nothing was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to release")
  set(CMAKE_BUILD_TYPE release)
endif()

set( BUILDER_ROOT "${CMAKE_HOME_DIRECTORY}/builder" )

if( NOT DEFINED CMAKE_MFX_HOME )
  set( CMAKE_MFX_HOME "${CMAKE_HOME_DIRECTORY}/api" )
endif()

if( "$ENV{MFX_HOME}" STREQUAL "" )
  set( ENV{MFX_HOME} ${CMAKE_HOME_DIRECTORY} )
endif()

option( ENABLE_OPENCL "Build targets dependent on OpenCL?" ON )
option( ENABLE_ITT "Build targets with ITT instrumentation support (requires VTune)?" OFF )

# -DENABLE_ALL will enable all the dependencies and features unless user did not
# explicitly switched some of them OFF, i.e. configuring in the following way
# is possible:
#   cmake -DENABLE_ALL=ON -DENABLE_TEXTLOG=OFF
# and it will configure all targets except samples.
#
# TODO: As of now ENABLE_ALL is not fully implemented, it will not affect all
#   of the ENABLE_* options. Those options it don't affect are placed above
#   ENABLE_ALL definition and require some rework and/or pending CI adoption.
#
option( ENABLE_ALL "Enable all dependencies and features?" OFF )

if( CMAKE_SYSTEM_NAME MATCHES Linux )
  option( ENABLE_X11_DRI3 "Build X11 DRI3 versions of the targets?" ${ENABLE_ALL} )
  option( ENABLE_WAYLAND "Build WAYLAND versions of the targets?" ${ENABLE_ALL} )
endif()

option( ENABLE_TEXTLOG "Enable textlog tracing?" "${ENABLE_ALL}")
option( ENABLE_STAT "Enable stat tracing?" "${ENABLE_ALL}")

# -DBUILD_ALL will enable all the build targets unless user did not explicitly
# switched some targets OFF, i.e. configuring in the following way is possible:
#   cmake -DBUILD_ALL=ON -DBUILD_SAMPLES=OFF
# and it will configure all targets except samples.
option( BUILD_ALL "Build all the targets?" OFF )

option( BUILD_RUNTIME "Build mediasdk runtime (library, plugins, etc.)?" ON )
option( BUILD_DISPATCHER "Build dispatcher?" ON )
cmake_dependent_option(BUILD_SAMPLES "Build samples?" ON "BUILD_DISPATCHER" OFF )
# Tools depend on samples (sample_common) and can't be built without it. The
# following BUILD_TOOLS option declaration assures that.
cmake_dependent_option(BUILD_TOOLS "Build tools?" "${BUILD_ALL}" "BUILD_SAMPLES" OFF)

include( ${BUILDER_ROOT}/FindOpenCL.cmake )
include( ${BUILDER_ROOT}/FindFunctions.cmake )
include( ${BUILDER_ROOT}/FindGlobals.cmake )
include( ${BUILDER_ROOT}/FindMFX.cmake )
include( ${BUILDER_ROOT}/FindPackages.cmake )
include( ${BUILDER_ROOT}/FindInternals.cmake )
include( ${BUILDER_ROOT}/FindITT.cmake )
include( ${BUILDER_ROOT}/FindGTest.cmake )
include( ${BUILDER_ROOT}/ConfTargets.cmake )

if (BUILD_TOOLS)
  add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools)
endif()

if (BUILD_SAMPLES)
  add_subdirectory(${CMAKE_HOME_DIRECTORY}/samples)
endif()

if (BUILD_RUNTIME)
  add_subdirectory(${CMAKE_HOME_DIRECTORY}/_studio)
endif()

if (BUILD_DISPATCHER)
  add_subdirectory(api/mfx_dispatch/linux)
endif()

create_plugins_cfg(${CMAKE_BINARY_DIR})

message("")
message("********************** Media SDK Configuration Summary **********************")
message("MediaSDK:")
message("  API (request to use this version)       : ${API}")
message("  API_VERSION                             : ${API_VERSION}")
message("  API_FLAGS                               : ${API_FLAGS}")
message("  MFX_CONFIG_FILE                         : ${MFX_CONFIG_FILE}")
message("  See detailed per-component mediasdk configuration in the auto-generated file:")
message("    ${CMAKE_CURRENT_BINARY_DIR}/mfxconfig.h")
message("Cmake:")
message("  CMAKE_VERSION                           : ${CMAKE_VERSION}")
message("  CMAKE_GENERATOR                         : ${CMAKE_GENERATOR}")
message("  CMAKE_BUILD_TOOL                        : ${CMAKE_BUILD_TOOL}")
message("Target:")
message("  CMAKE_SYSTEM_NAME                       : ${CMAKE_SYSTEM_NAME}")
message("  CMAKE_SYSTEM_VERSION                    : ${CMAKE_SYSTEM_VERSION}")
message("  CMAKE_SYSTEM_PROCESSOR                  : ${CMAKE_SYSTEM_PROCESSOR}")
message("General:")
message("  CMAKE_BUILD_TYPE                        : ${CMAKE_BUILD_TYPE}")
message("  CMAKE_TOOLCHAIN_FILE                    : ${CMAKE_TOOLCHAIN_FILE}")
message("  CMAKE_C_COMPILER                        : ${CMAKE_C_COMPILER}")
message("  CMAKE_CXX_COMPILER                      : ${CMAKE_CXX_COMPILER}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message("  Building architecture                   : 64-bit")
else()
  message("  Building architecture                   : 32-bit")
endif()
message("Flags:")
message("  CMAKE_C_FLAGS                           : ${CMAKE_C_FLAGS}")
message("  CMAKE_CXX_FLAGS                         : ${CMAKE_CXX_FLAGS}")
message("Release flags:")
message("  CMAKE_C_FLAGS_RELEASE                   : ${CMAKE_C_FLAGS_RELEASE}")
message("  CMAKE_CXX_FLAGS_RELEASE                 : ${CMAKE_CXX_FLAGS_RELEASE}")
message("Debug flags:")
message("  CMAKE_C_FLAGS_DEBUG                     : ${CMAKE_C_FLAGS_DEBUG}")
message("  CMAKE_CXX_FLAGS_DEBUG                   : ${CMAKE_CXX_FLAGS_DEBUG}")
message("Install:")
message("  CMAKE_INSTALL_PREFIX                    : ${CMAKE_INSTALL_PREFIX}")
if (BUILD_TOOLS)
  message("  CMAKE_INSTALL_FULL_BINDIR               : ${CMAKE_INSTALL_FULL_BINDIR}")
endif()
message("  CMAKE_INSTALL_FULL_INCLUDEDIR           : ${CMAKE_INSTALL_FULL_INCLUDEDIR}")
message("  CMAKE_INSTALL_FULL_LIBDIR               : ${CMAKE_INSTALL_FULL_LIBDIR}")
message("  CMAKE_INSTALL_FULL_DATADIR              : ${CMAKE_INSTALL_FULL_DATADIR}")
message("Enable:")
message("  ENABLE_OPENCL                           : ${ENABLE_OPENCL}")
if( CMAKE_SYSTEM_NAME MATCHES Linux )
  message("  ENABLE_X11_DRI3                         : ${ENABLE_X11_DRI3}")
  message("  ENABLE_WAYLAND                          : ${ENABLE_WAYLAND}")
endif()
message("  ENABLE_ITT                              : ${ENABLE_ITT}")
message("  ENABLE_TEXTLOG                          : ${ENABLE_TEXTLOG}")
message("  ENABLE_STAT                             : ${ENABLE_STAT}")
message("Build:")
message("  BUILD_RUNTIME                           : ${BUILD_RUNTIME}")
message("  BUILD_DISPATCHER                        : ${BUILD_DISPATCHER}")
message("  BUILD_SAMPLES                           : ${BUILD_SAMPLES}")
message("  BUILD_TOOLS                             : ${BUILD_TOOLS}")
message("*****************************************************************************")
