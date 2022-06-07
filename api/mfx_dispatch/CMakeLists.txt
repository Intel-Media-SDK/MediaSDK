cmake_minimum_required( VERSION 3.6 FATAL_ERROR )
project( mfx )
include( GNUInstallDirs )

set( MFX_API_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../include )

# While equal to get_mfx_version in samples/builder, this function should remain separate to make this file self-sufficient 
function( get_api_version mfx_version_major mfx_version_minor )
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h major REGEX "#define MFX_VERSION_MAJOR" LIMIT_COUNT 1)
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h minor REGEX "#define MFX_VERSION_MINOR" LIMIT_COUNT 1)
  string(REPLACE "#define MFX_VERSION_MAJOR " "" major ${major})
  string(REPLACE "#define MFX_VERSION_MINOR " "" minor ${minor})
  set(${mfx_version_major} ${major} PARENT_SCOPE)
  set(${mfx_version_minor} ${minor} PARENT_SCOPE)
endfunction()

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__bin )

get_api_version(MFX_VERSION_MAJOR MFX_VERSION_MINOR)

set( PKG_CONFIG_FNAME "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/lib${PROJECT_NAME}.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake" ${PKG_CONFIG_FNAME} @ONLY)

install( FILES ${PKG_CONFIG_FNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )

# For backwards compatibility, create a relative symbolic link without the "lib"
# prefix to the .pc file.
set( PKG_CONFIG_LFNAME "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.pc" )
add_custom_target(pc_link_target ALL COMMAND ${CMAKE_COMMAND} -E copy ${PKG_CONFIG_FNAME} ${PKG_CONFIG_LFNAME})
install( FILES ${PKG_CONFIG_LFNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )

add_subdirectory(linux)
