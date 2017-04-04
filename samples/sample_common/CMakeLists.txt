include_directories (
  ${CMAKE_SOURCE_DIR}/sample_common/include
  ${CMAKE_SOURCE_DIR}/sample_misc/wayland/include
)

set( defs "${WARNING_FLAGS}" )
make_library( shortname universal static "nasafestring" )
set( defs "" )
