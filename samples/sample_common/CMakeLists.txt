include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_misc/wayland/include
)

file( GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
list( APPEND sources ${srcs})

find_path( VA_FEI va/va_fei.h PATHS ${PKG_LIBVA_INCLUDEDIR} )
if(VA_FEI MATCHES NOTFOUND)
  list( REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/vaapi_buffer_allocator.cpp)
endif()


set( defs "${API_FLAGS} ${WARNING_FLAGS}" )
make_library( shortname universal static "nosafestring" )
set( defs "" )
