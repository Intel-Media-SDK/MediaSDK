set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs( )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/aenc/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/shared/include)

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/enctools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_enctools_brc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_enctools_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_enctools_aenc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_enctools_utils.cpp
    )
set( sources.plus "" )

set( defs "${API_FLAGS} ${WARNING_FLAGS}" )
make_library(enctools hw static )
set( defs "" )

if (MFX_ENABLE_AENC)
  find_library( AENC_LIBRARY libaenc.a PATHS ${CMAKE_HOME_DIRECTORY}/contrib/aenc/ )
  if(NOT ${AENC_LIBRARY} MATCHES NOTFOUND)
    message(STATUS "aenc library was found here ${AENC_LIBRARY}")

    add_library(aenc STATIC IMPORTED GLOBAL)
    set_target_properties(aenc PROPERTIES IMPORTED_LOCATION "${CMAKE_HOME_DIRECTORY}/contrib/aenc/libaenc.a")
    target_include_directories(aenc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  else()
    message(STATUS "aenc library was not found (optional)! mfxlib will be built without aenc")
    set(MFX_ENABLE_AENC OFF)
  endif()
endif()

if (MFX_ENABLE_AENC)
  target_link_libraries(enctools_hw aenc)
endif()
