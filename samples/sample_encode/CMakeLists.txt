include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_plugins/rotate_cpu/include
)

list( APPEND LIBS_VARIANT sample_common )

set(DEPENDENCIES libmfx dl pthread)
make_executable( shortname universal "nosafestring" )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
