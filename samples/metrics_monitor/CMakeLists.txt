# metrics_monitor library

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

set( sources "" )

file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
list( APPEND sources ${srcs})

make_library( cttmetrics none shared )
make_library( cttmetrics_static none static )

# metrics_monitor sample

set( sources "" )

file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/sample/*.cpp" )
list( APPEND sources ${srcs})
list( APPEND LIBS cttmetrics)

make_executable( metrics_monitor none )

# test_monitor

pkg_check_modules(PKG_LIBDRM    libdrm)
pkg_check_modules(PKG_PCIACCESS pciaccess)

if( PKG_LIBDRM_FOUND AND
    PKG_PCIACCESS_FOUND AND
    DEFINED GTEST_FOUND)

    set( sources "" )

    list ( APPEND incldirs
        ${GTEST_INCLUDE}/gtest
        ${GTEST_INCLUDE}/gtest/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/test )

    foreach( dir ${incldirs} )
      include_directories( ${dir} )
    endforeach( )

    file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp" )
    list( APPEND sources ${srcs})
    list( APPEND LIBS
      pthread
      rt
      gtest )

    set(DEPENDENCIES libdrm)

    make_executable( test_monitor none )

else()

    if( NOT PKG_PCIACCESS_FOUND )
        message( STATUS "pciaccess was not found (optional), the following will not be built: test_monitor." )
    endif()

    if( NOT PKG_LIBDRM_FOUND )
        message( STATUS "libdrm was not found (optional), the following will not be built: test_monitor." )
    endif()

    if( NOT DEFINED GTEST_FOUND )
        message( STATUS "Google tests was not found (optional), the following will not be built: test_monitor." )
    endif()

endif()
