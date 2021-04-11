include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common)
list( APPEND LIBS_VARIANT tutorials_common )
set(DEPENDENCIES libva libva-drm libmfx dl pthread)
make_executable( shortname universal )
