include_directories (
  ${CMAKE_SOURCE_DIR}/samples/sample_common/include
  ${CMAKE_SOURCE_DIR}/samples/sample_misc/wayland/include
)

set( defs "${WARNING_FLAGS}" )
make_library( shortname universal static "nasafestring" )
set( defs "" )
