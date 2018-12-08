# Copyright (c) 2018-2019 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__lib )
set( CMAKE_BIN_DIR ${CMAKE_BINARY_DIR}/__bin )
set_property( GLOBAL PROPERTY PROP_PLUGINS_CFG "" )

# .....................................................
function( collect_oses )
  if( CMAKE_SYSTEM_NAME MATCHES Windows )
    set( Windows    true PARENT_SCOPE )
    set( NotLinux   true PARENT_SCOPE )
    set( NotDarwin  true PARENT_SCOPE )

  elseif( CMAKE_SYSTEM_NAME MATCHES Linux )
    set( Linux      true PARENT_SCOPE )
    set( NotDarwin  true PARENT_SCOPE )
    set( NotWindows true PARENT_SCOPE )

  elseif( CMAKE_SYSTEM_NAME MATCHES Darwin )
    set( Darwin     true PARENT_SCOPE )
    set( NotLinux   true PARENT_SCOPE )
    set( NotWindows true PARENT_SCOPE )

  endif()
endfunction()

# .....................................................
function( append what where )
  set(${ARGV1} "${ARGV0} ${${ARGV1}}" PARENT_SCOPE)
endfunction()

# .....................................................
function( get_source include sources)
  file( GLOB_RECURSE include "[^.]*.h" )
  file( GLOB_RECURSE sources "[^.]*.c" "[^.]*.cpp" )

  set( ${ARGV0} ${include} PARENT_SCOPE )
  set( ${ARGV1} ${sources} PARENT_SCOPE )
endfunction()

#
# Usage: get_target(target name none|<variant>)
#
function( get_target target name variant)
  if( ARGV1 MATCHES shortname )
    get_filename_component( tname ${CMAKE_CURRENT_SOURCE_DIR} NAME )
  else()
    set( tname ${ARGV1} )
  endif()
  if( ARGV2 MATCHES none OR ARGV2 MATCHES universal OR DEFINED USE_STRICT_NAME)
    set( target ${tname} )
  else()
   set( target ${tname}_${ARGV2} )
  endif()

  set( ${ARGV0} ${target} PARENT_SCOPE )
endfunction()

# .....................................................
function( get_folder folder )
  set( folder ${CMAKE_PROJECT_NAME} )
  set (${ARGV0} ${folder} PARENT_SCOPE)
endfunction()

function( get_mfx_version mfx_version_major mfx_version_minor )
  file(STRINGS ${MFX_API_HOME}/include/mfxdefs.h major REGEX "#define MFX_VERSION_MAJOR" LIMIT_COUNT 1)
  if(major STREQUAL "") # old style version
     file(STRINGS ${MFX_API_HOME}/include/mfxvideo.h major REGEX "#define MFX_VERSION_MAJOR")
  endif()
  file(STRINGS ${MFX_API_HOME}/include/mfxdefs.h minor REGEX "#define MFX_VERSION_MINOR" LIMIT_COUNT 1)
  if(minor STREQUAL "") # old style version
     file(STRINGS ${MFX_API_HOME}/include/mfxvideo.h minor REGEX "#define MFX_VERSION_MINOR")
  endif()
  string(REPLACE "#define MFX_VERSION_MAJOR " "" major ${major})
  string(REPLACE "#define MFX_VERSION_MINOR " "" minor ${minor})
  set(${mfx_version_major} ${major} PARENT_SCOPE)
  set(${mfx_version_minor} ${minor} PARENT_SCOPE)
endfunction()

function( gen_plugins_cfg plugin_id guid plugin_name type codecID )
  get_mfx_version(mfx_version_major mfx_version_minor)
  math(EXPR api_version "${mfx_version_major}*256 + ${mfx_version_minor}")

  get_property( PLUGINS_CFG GLOBAL PROPERTY PROP_PLUGINS_CFG )
  set(PLUGINS_CFG "${PLUGINS_CFG}[${plugin_id}_${guid}]\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}GUID = ${guid}\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}PluginVersion = 1\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}APIVersion = ${api_version}\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}Path = ${MFX_PLUGINS_DIR}/lib${plugin_name}.so\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}Type = ${type}\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}CodecID = ${codecID}\n")
  set(PLUGINS_CFG "${PLUGINS_CFG}Default = 0\n")
  set_property( GLOBAL PROPERTY PROP_PLUGINS_CFG ${PLUGINS_CFG} )

endfunction()

function( create_plugins_cfg directory )
  get_property( PLUGINS_CFG GLOBAL PROPERTY PROP_PLUGINS_CFG )
  file(WRITE ${directory}/plugins.cfg ${PLUGINS_CFG})

  install( FILES ${directory}/plugins.cfg DESTINATION ${MFX_PLUGINS_CONF_DIR} )

endfunction()

# Usage:
#  make_library(shortname|<name> none|<variant> static|shared)
#    - shortname|<name>: use folder name as library name or <name> specified by user
#    - <variant>|none: build library in specified variant (with drm or x11 or wayland support, etc),
#      universal - special variant which enables compilation flags required for all backends, but
#      moves dependency to runtime instead of linktime
#    or without variant if none was specified
#    - static|shared: build static or shared library
#
function( make_library name variant type )
  get_target( target ${ARGV0} ${ARGV1} )
  if( ${ARGV0} MATCHES shortname )
    get_folder( folder )
  else ()
    set( folder ${ARGV0} )
  endif()

  configure_dependencies(${target} "${DEPENDENCIES}" ${variant})
  if(SKIPPING MATCHES ${target} OR NOT_CONFIGURED MATCHES ${target})
    return()
  else()
    report_add_target(BUILDING ${target})
  endif()

  if( NOT sources )
   get_source( include sources )
  endif()

  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif()

  if( ARGV2 MATCHES static )
    add_library( ${target} STATIC ${include} ${sources} )
    append_property(${target} COMPILE_FLAGS "${SCOPE_CFLAGS}")

  elseif( ARGV2 MATCHES shared )
    add_library( ${target} SHARED ${include} ${sources} )

    if( Linux )
      target_link_libraries( ${target} "-Xlinker --start-group" )
    endif()

    foreach( lib ${LIBS_VARIANT} )
      if(ARGV1 MATCHES none OR ARGV1 MATCHES universal)
        add_dependencies( ${target} ${lib} )
        target_link_libraries( ${target} ${lib} )
      else()
        add_dependencies( ${target} ${lib}_${ARGV1} )
        target_link_libraries( ${target} ${lib}_${ARGV1} )
      endif()
    endforeach()

    foreach( lib ${LIBS_NOVARIANT} )
      add_dependencies( ${target} ${lib} )
      target_link_libraries( ${target} ${lib} )
    endforeach()

    append_property(${target} COMPILE_FLAGS "${CFLAGS} ${SCOPE_CFLAGS}")
    append_property(${target} LINK_FLAGS "${LDFLAGS} ${SCOPE_LINKFLAGS}")
    foreach(lib ${LIBS} ${SCOPE_LIBS})
      target_link_libraries( ${target} ${lib} )
    endforeach()

    if( Linux )
      target_link_libraries( ${target} "-Xlinker --end-group" )
    endif()

#    set_target_properties( ${target} PROPERTIES LINK_INTERFACE_LIBRARIES "" )
  endif()

  configure_build_variant( ${target} ${ARGV1} )

  if( defs )
    append_property( ${target} COMPILE_FLAGS ${defs} )
  endif()
  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )

  if( Linux )
    target_link_libraries( ${target} "-lgcc" )
  endif()

  set( target ${target} PARENT_SCOPE )
endfunction()

# Usage:
#  make_executable(name|<name> none|<variant>)
#    - name|<name>: use folder name as library name or <name> specified by user
#    - <variant>|none: build library in specified variant (with drm or x11 or wayland support, etc),
#      universal - special variant which enables compilation flags required for all backends, but
#      moves dependency to runtime instead of linktime
#
function( make_executable name variant )
  get_target( target ${ARGV0} ${ARGV1} )
  get_folder( folder )

  configure_dependencies(${target} "${DEPENDENCIES}" ${variant})
  if(SKIPPING MATCHES ${target} OR NOT_CONFIGURED MATCHES ${target})
    return()
  else()
    report_add_target(BUILDING ${target})
  endif()

  if( NOT sources )
    get_source( include sources )
  endif()

  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif()

  add_executable( ${target} ${include} ${sources} )

  if( defs )
    append_property( ${target} COMPILE_FLAGS ${defs} )
  endif()
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )

  if( Linux )
    target_link_libraries( ${target} "-Xlinker --start-group" )
  endif()

  foreach( lib ${LIBS_VARIANT} )
    if(ARGV1 MATCHES none OR ARGV1 MATCHES universal)
      add_dependencies( ${target} ${lib} )
      target_link_libraries( ${target} ${lib} )
    else()
      add_dependencies( ${target} ${lib}_${ARGV1} )
      target_link_libraries( ${target} ${lib}_${ARGV1} )
    endif()
  endforeach()

  foreach( lib ${LIBS_NOVARIANT} )
    add_dependencies( ${target} ${lib} )
    target_link_libraries( ${target} ${lib} )
  endforeach()

  if( ${NEED_DISPATCHER} )
    target_link_libraries( ${target} debug mfx_d )
    target_link_libraries( ${target} optimized mfx )
  endif()

  foreach( lib ${LIBS} ${SCOPE_LIBS} )
    target_link_libraries( ${target} ${lib} )
  endforeach()

  append_property(${target} COMPILE_FLAGS "${CFLAGS} ${SCOPE_CFLAGS}")
  append_property(${target} LINK_FLAGS "${LDFLAGS} ${SCOPE_LINKFLAGS}")

  configure_build_variant( ${target} ${ARGV1} )

  foreach( lib ${LIBS_SUFFIX} )
    target_link_libraries( ${target} ${lib} )
  endforeach()

  if( Linux )
    target_link_libraries( ${target} "-Xlinker --end-group -lgcc" )
  endif()

  set( target ${target} PARENT_SCOPE )
endfunction()

function( set_file_and_product_version input_version version_defs )
  if( Linux OR Darwin )
    execute_process(
      COMMAND echo
      COMMAND cut -f 1 -d.
      COMMAND date "+.%-y.%-m.%-d"
      OUTPUT_VARIABLE cur_date
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string( SUBSTRING ${input_version} 0 1 ver )

    set( version_defs " -DMFX_PLUGIN_FILE_VERSION=\"\\\"${ver}${cur_date}\"\\\"" )

    set( git_commit "" )
    git_describe( git_commit )

    set( version_defs " -DMFX_PLUGIN_FILE_VERSION=\"\\\"${ver}${cur_date}${git_commit}\"\\\"" )
    set( version_defs "${version_defs} -DMFX_PLUGIN_PRODUCT_VERSION=\"\\\"${input_version}\"\\\"" PARENT_SCOPE )
    
  endif()
endfunction()

function( git_describe git_commit )
  execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE git_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  if( NOT ${git_commit} MATCHES "^$" )
    set( git_commit "${git_commit}" PARENT_SCOPE )
  endif()
endfunction()

function(msdk_install target dir)
  if( Windows )
    install( TARGETS ${target} RUNTIME DESTINATION ${dir} )
  else()
    install( TARGETS ${target} LIBRARY DESTINATION ${dir} )
  endif()
endfunction()

