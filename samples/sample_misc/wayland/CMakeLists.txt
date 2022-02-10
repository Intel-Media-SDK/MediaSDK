if (NOT ENABLE_WAYLAND)
    return()
endif()
include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

list(APPEND LIBS sample_common)

set(DEPENDENCIES  libva libdrm wayland-client)

make_library(mfx_wayland none shared )

if (WAYLAND_LINUX_DMABUF_XML_PATH AND WAYLAND_SCANNER_BINARY_PATH)
  execute_process(
	  COMMAND "${WAYLAND_SCANNER_BINARY_PATH}"
              "client-header" "${WAYLAND_LINUX_DMABUF_XML_PATH}"
              "samples/sample_misc/wayland/linux-dmabuf-unstable-v1.h"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      RESULT_VARIABLE WAYLAND_SCANNER_RESULT)
  if (WAYLAND_SCANNER_RESULT)
    message(FATAL_ERROR "Failed to generate linux-dmabuf-unstable-v1.h")
  endif()

  execute_process(
      COMMAND "${WAYLAND_SCANNER_BINARY_PATH}"
              "private-code" "${WAYLAND_LINUX_DMABUF_XML_PATH}"
              "samples/sample_misc/wayland/linux-dmabuf-unstable-v1.c"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      RESULT_VARIABLE WAYLAND_SCANNER_RESULT)
  if (WAYLAND_SCANNER_RESULT)
    message(FATAL_ERROR "Failed to generate linux-dmabuf-unstable-v1.c")
  endif()

  include_directories(
    ${CMAKE_BINARY_DIR}/samples/sample_misc/wayland
  )
  target_sources(
    ${target}
    PRIVATE ${CMAKE_BINARY_DIR}/samples/sample_misc/wayland/linux-dmabuf-unstable-v1.c)
endif()

install( TARGETS ${target} LIBRARY DESTINATION ${MFX_SAMPLES_INSTALL_LIB_DIR} )
