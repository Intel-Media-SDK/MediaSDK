include_directories (
  ${CMAKE_SOURCE_DIR}/samples/sample_common/include
  ${CMAKE_SOURCE_DIR}/samples/sample_plugins/vpp_plugin/include
)

make_library( shortname none static "nosafestring" )

install( TARGETS ${target} ARCHIVE DESTINATION ${MFX_SAMPLES_INSTALL_LIB_DIR} )
