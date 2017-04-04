include_directories (
  ${CMAKE_SOURCE_DIR}/sample_common/include
  ${CMAKE_SOURCE_DIR}/sample_misc/wayland/include
  ${CMAKE_SOURCE_DIR}/sample_multi_transcode/include
  ${CMAKE_SOURCE_DIR}/sample_plugins/vpp_plugin/include
  ${CMAKE_SOURCE_DIR}/sample_plugins/rotate_cpu/include
)

list( APPEND LIBS_VARIANT sample_common )
list( APPEND LIBS_NOVARIANT vpp_plugin )

set(DEPENDENCIES itt libmfx dl pthread)

make_executable( shortname universal "nosafestring" )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
