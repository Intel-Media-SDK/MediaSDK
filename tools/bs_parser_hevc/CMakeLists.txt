include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file( GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )

set( defs " -DMFX_VERSION_USE_LATEST " )

make_library( bs_parser_hevc_static none static )
set( defs "" )
