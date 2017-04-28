if (NOT ENABLE_WAYLAND)
    return()
endif()
include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

list(APPEND LIBS sample_common)

set(DEPENDENCIES  libva libdrm wayland-client)

make_library(mfx_wayland none shared "nosafestring" )

install( TARGETS ${target} LIBRARY DESTINATION ${MFX_SAMPLES_INSTALL_LIB_DIR} )
