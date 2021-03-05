add_subdirectory(tools/configure)

set (TRACER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(
  "$ENV{MFX_HOME}/include"
  "${TRACER_DIR}"
  )

set(headers
  "${TRACER_DIR}/config/config.h"
  "${TRACER_DIR}/dumps/dump.h"
  "${TRACER_DIR}/loggers/ilog.h"
  "${TRACER_DIR}/loggers/log.h"
  "${TRACER_DIR}/loggers/log_console.h"
  "${TRACER_DIR}/loggers/log_etw_events.h"
  "${TRACER_DIR}/loggers/log_file.h"
  "${TRACER_DIR}/loggers/log_syslog.h"
  "${TRACER_DIR}/loggers/timer.h"
  "${TRACER_DIR}/loggers/thread_info.h"
  "${TRACER_DIR}/tracer/tracer.h"
  "${TRACER_DIR}/tracer/functions_table.h"
  "${TRACER_DIR}/tracer/bits/mfxfunctions.h"
  "${TRACER_DIR}/wrappers/mfx_structures.h"
  )

set(sources
  "${TRACER_DIR}/config/config.cpp"
  "${TRACER_DIR}/dumps/dump.cpp"
  "${TRACER_DIR}/dumps/dump_mfxbrc.cpp"
  "${TRACER_DIR}/dumps/dump_mfxcommon.cpp"
  "${TRACER_DIR}/dumps/dump_mfxdefs.cpp"
  "${TRACER_DIR}/dumps/dump_mfxenc.cpp"
  "${TRACER_DIR}/dumps/dump_mfxplugin.cpp"
  "${TRACER_DIR}/dumps/dump_mfxsession.cpp"
  "${TRACER_DIR}/dumps/dump_mfxstructures.cpp"
  "${TRACER_DIR}/dumps/dump_mfxvideo.cpp"
  "${TRACER_DIR}/dumps/dump_mfxfei.cpp"
  "${TRACER_DIR}/dumps/dump_mfxla.cpp"
  "${TRACER_DIR}/dumps/dump_mfxvp8.cpp"
  "${TRACER_DIR}/loggers/log.cpp"
  "${TRACER_DIR}/loggers/log_console.cpp"
  "${TRACER_DIR}/loggers/log_etw_events.cpp"
  "${TRACER_DIR}/loggers/log_file.cpp"
  "${TRACER_DIR}/loggers/log_syslog.cpp"
  "${TRACER_DIR}/tracer/tracer.cpp"
  "${TRACER_DIR}/tracer/tracer_linux.cpp"
  "${TRACER_DIR}/tracer/tracer_windows.cpp"
  "${TRACER_DIR}/wrappers/mfx_core.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_core.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_decode.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_enc.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_encode.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_user.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_vpp.cpp"
  "${TRACER_DIR}/wrappers/mfx_video_fei.cpp"
  )

if( NOT DEFINED MFX_MODULES_DIR )
  set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
endif( )
add_definitions( -DMFX_MODULES_DIR="${MFX_MODULES_DIR}" )

make_library(mfx-tracer none shared)

set_target_properties( mfx-tracer PROPERTIES LINK_FLAGS
  "${LINK_FLAGS} -Wl,--version-script=${CMAKE_HOME_DIRECTORY}/api/mfx_dispatch/linux/libmfx.map" )

get_mfx_version(mfx_version_major mfx_version_minor)
set_target_properties(mfx-tracer PROPERTIES   VERSION ${mfx_version_major}.${mfx_version_minor})
set_target_properties(mfx-tracer PROPERTIES SOVERSION ${mfx_version_major})

target_link_libraries( mfx-tracer ${CMAKE_DL_LIBS})
target_compile_options(mfx-tracer PRIVATE -Wno-deprecated-declarations)

install(TARGETS mfx-tracer LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(defs "")
