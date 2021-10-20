include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_misc/wayland/include
)

if(WAYLAND_LINUX_DMABUF_XML_PATH AND WAYLAND_SCANNER_BINARY_PATH)
  include_directories(
    ${CMAKE_BINARY_DIR}/samples/sample_misc/wayland
  )
endif()

make_library( shortname universal static )
set( defs "" )
