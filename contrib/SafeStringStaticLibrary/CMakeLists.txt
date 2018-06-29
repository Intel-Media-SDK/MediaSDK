cmake_minimum_required( VERSION 2.6.2 FATAL_ERROR )
project( SafeString )

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__lib )

if( CMAKE_SYSTEM_NAME MATCHES Windows )
  add_definitions(-DMINGW_HAS_SECURE_API -DWIN32)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DWIN64)
  endif( )
else( )
  add_definitions(-DUNIX)

  if( CMAKE_SYSTEM_NAME MATCHES Linux )
    add_definitions(-D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64 -DLINUX -DLINUX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DLINUX64)
    endif( )
  endif( )

  if( CMAKE_SYSTEM_NAME MATCHES Darwin )
    add_definitions(-DOSX)
    add_definitions(-DOSX32)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      add_definitions(-DOSX64)
    endif( )
  endif( )

  set(no_warnings "-Wno-unknown-pragmas -Wno-unused")
  set(warnings "-Wall -Wformat -Wformat-security")
  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fPIC -std=c++11 ${warnings} ${no_warnings}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

  if (DEFINED CMAKE_FIND_ROOT_PATH)
    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " LINK_FLAGS)
  endif (DEFINED CMAKE_FIND_ROOT_PATH)
endif( )

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)


set( sources "" )
file( GLOB_RECURSE sources
  "safeclib/*.c"
)

function( append_property target property_name property )
  get_target_property( property ${ARGV0} ${ARGV1} )
  if( property MATCHES NOTFOUND)
    set( property "" )
  endif( )
  string( REPLACE ";" " " property "${ARGV2} ${property}" )
  set_target_properties( ${ARGV0} PROPERTIES ${ARGV1} "${property}" )
endfunction( )

function( make_static_library name )
  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif( )

  set( target ${ARGV0} )

  add_library( ${target} STATIC ${include} ${sources} )

  set( link_flags_list "-Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack")
  append_property( ${target} LINK_FLAGS "${link_flags_list} -fstack-protector" )

  if( defs )
    append_property( ${target} COMPILE_FLAGS ${defs} )
  endif( )

  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${target} )
endfunction( )

make_static_library( SafeString )
set( defs "" )
