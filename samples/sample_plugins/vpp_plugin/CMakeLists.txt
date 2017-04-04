include_directories (
  ${CMAKE_SOURCE_DIR}/sample_common/include
  ${CMAKE_SOURCE_DIR}/sample_plugins/vpp_plugin/include
)

make_library( shortname none static "nosafestring" )

install( TARGETS ${target} ARCHIVE DESTINATION ${MFX_SAMPLES_INSTALL_LIB_DIR} )
