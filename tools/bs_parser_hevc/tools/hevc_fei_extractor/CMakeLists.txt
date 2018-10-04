find_path( FEI_INCLUDE mfxfeihevc.h PATHS ${MFX_INCLUDE} )

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

list( APPEND LIBS bs_parser_hevc_static )

set( defs " -DMFX_VERSION_USE_LATEST " )
set(DEPENDENCIES pthread)

make_executable( shortname universal )

install( TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
set( defs "" )
