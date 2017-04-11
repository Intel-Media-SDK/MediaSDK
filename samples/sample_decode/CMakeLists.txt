include_directories (
  ${CMAKE_SOURCE_DIR}/samples/sample_common/include
  ${CMAKE_SOURCE_DIR}/samples/sample_misc/wayland/include
  ${CMAKE_SOURCE_DIR}/samples/sample_decode/include
)

list( APPEND LIBS_VARIANT sample_common )

set(DEPENDENCIES libmfx dl pthread)
make_executable( shortname universal "nosafestring" )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
