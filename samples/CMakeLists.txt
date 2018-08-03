cmake_minimum_required( VERSION 2.6.2 FATAL_ERROR )
project( mediasdk-samples )

option( CMAKE_MFX_HOME "Where is Media SDK?" OFF )
option( CMAKE_VTUNE_HOME "Where is VTune?" OFF )

option( ENABLE_ITT "Build targets with ITT instrumentation support (requires VTune)?" OFF )

option( ENABLE_SW "Build SW versions of the targets?" OFF )

if( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
option( ENABLE_DRM "Build DRM versions of the targets?" ON )
option( ENABLE_X11 "Build X11 versions of the targets?" ON )
option( ENABLE_X11_DRI3 "Build X11 DRI3 versions of the targets?" OFF )
option( ENABLE_WAYLAND "Build WAYLAND versions of the targets?" OFF )
option( ENABLE_PS "Enable PS(internal)?" OFF )
option( ENABLE_FF "Enable FF(internal)?" OFF )
endif()

option( ENABLE_FFMPEG "Build targets dependent on FFmpeg?" ON )
option( ENABLE_OPENCL "Build targets dependent on OpenCL?" ON )

set( BUILDER_ROOT ${CMAKE_HOME_DIRECTORY}/../builder )
include( ../builder/FindFunctions.cmake )
include( ../builder/FindGlobals.cmake )
include( ../builder/FindMFX.cmake )
include( ../builder/FindPackages.cmake )
include( ../builder/FindITT.cmake )

set( NOT_CONFIGURED "" CACHE INTERNAL "" )
set( SKIPPING "" CACHE INTERNAL "" )
set( BUILDING "" CACHE INTERNAL "" )

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

append("-std=c++11 ${API_FLAGS}" CMAKE_CXX_FLAGS)

create_build( )

report_targets("The following targets were NOT configured:" "${NOT_CONFIGURED}")
report_targets("The following targets will be ${BoldRed}SKIPPED${EndColor}:" "${SKIPPING}")
report_targets("The following targets will be ${BoldGreen}BUILT${EndColor}:" "${BUILDING}")
