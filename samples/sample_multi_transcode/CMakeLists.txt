include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_misc/wayland/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_plugins/vpp_plugin/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_plugins/rotate_cpu/include
)

if (WAYLAND_LINUX_DMABUF_XML_PATH AND WAYLAND_SCANNER_BINARY_PATH)
  include_directories(
    ${CMAKE_BINARY_DIR}/samples/sample_misc/wayland
  )
endif()

list( APPEND LIBS_VARIANT sample_common )
list( APPEND LIBS_NOVARIANT vpp_plugin )

set(DEPENDENCIES itt libmfx dl pthread)

make_executable( shortname universal )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
