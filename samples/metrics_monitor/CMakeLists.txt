# metrics_monitor library

if( PKG_LIBDRM_FOUND AND ("${PKG_LIBDRM_VERSION}" VERSION_GREATER 2.4.91))

    include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include/)

    set( sources "" )

    file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
    list( APPEND sources ${srcs})

    set(DEPENDENCIES libdrm )

    make_library( cttmetrics none shared )
    make_library( cttmetrics_static none static )

    install( TARGETS cttmetrics LIBRARY DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )

    # metrics_monitor sample

    set( sources "" )

    file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/sample/*.cpp" )
    list( APPEND sources ${srcs})
    list( APPEND LIBS cttmetrics)

    make_executable( metrics_monitor none )

    install( TARGETS metrics_monitor RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )

    # test_monitor

    pkg_check_modules(PKG_PCIACCESS pciaccess)

    if( PKG_PCIACCESS_FOUND AND
        BUILD_TESTS)

        file( GLOB_RECURSE test_srcs "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/test/*.c" )

        add_executable( test_monitor ${test_srcs})
        target_include_directories( test_monitor PRIVATE ./include ${PKG_LIBDRM_INCLUDE_DIRS})
        target_link_libraries( test_monitor pthread rt gtest cttmetrics ${PKG_LIBDRM_LIBRARIES})

        set_target_properties(test_monitor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE})

    else()

        if( NOT PKG_PCIACCESS_FOUND )
            message( STATUS "pciaccess was not found (optional), the following will not be built: test_monitor." )
        endif()

    endif()

else()
    message( STATUS "libdrm was not found, the following will not be built: metrics_monitor." )
endif()
