include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

make_library( shortname none static "nosafestring" )

install( TARGETS ${target} ARCHIVE DESTINATION ${MFX_SAMPLES_INSTALL_LIB_DIR} )
