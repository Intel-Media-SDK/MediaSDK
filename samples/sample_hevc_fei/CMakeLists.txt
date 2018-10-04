find_path( FEI_INCLUDE mfxfeihevc.h PATHS ${MFX_INCLUDE} )

if(FEI_INCLUDE MATCHES NOTFOUND)
  set(skipping _force_skipping)
endif()

find_path( VA_HEVC_FEI va/va_fei_hevc.h PATHS ${PKG_LIBVA_INCLUDEDIR} )
if(VA_HEVC_FEI MATCHES NOTFOUND)
  set(skipping _force_skipping)
endif()

if(API_VERSION VERSION_LESS "1.27")
  set(skipping _force_skipping)
endif()

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

list( APPEND LIBS_VARIANT sample_common )

set(DEPENDENCIES ${skipping} itt libmfx dl pthread)
make_executable( shortname universal )

install( TARGETS ${target} RUNTIME DESTINATION ${MFX_SAMPLES_INSTALL_BIN_DIR} )
set( defs "" )
