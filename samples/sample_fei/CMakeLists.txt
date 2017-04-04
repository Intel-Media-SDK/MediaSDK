find_path( FEI_INCLUDE mfxfei.h PATHS ${MFX_INCLUDE} )

if(FEI_INCLUDE MATCHES NOTFOUND)
  set(skipping _force_skipping)
endif()

include_directories (
  ${CMAKE_SOURCE_DIR}/sample_common/include
  ${CMAKE_SOURCE_DIR}/sample_fei/include
)

list( APPEND LIBS_VARIANT sample_common )

set(DEPENDENCIES ${skipping} itt libmfx dl pthread)
make_executable( shortname universal "nosafestring" )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
