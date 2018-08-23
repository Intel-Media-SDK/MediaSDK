# Copyright (c) 2012-2018 Intel Corporation
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

set(defs "${API_FLAGS} ${WARNING_FLAGS}")
make_library(mfx-tracer none shared)

set_target_properties( mfx-tracer PROPERTIES LINK_FLAGS
  "${LINK_FLAGS} -Wl,--version-script=${TRACER_DIR}/libmfx.map" )

target_link_libraries( mfx-tracer dl rt )

install( TARGETS mfx-tracer LIBRARY DESTINATION ${MFX_MODULES_DIR} )

set(defs "")
