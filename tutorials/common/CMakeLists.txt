set( sources
    ${CMAKE_CURRENT_SOURCE_DIR}/cmd_options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils_linux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common_vaapi.cpp )

make_library( tutorials_common none static )
