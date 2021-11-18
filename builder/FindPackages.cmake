# Copyright (c) 2017-2018 Intel Corporation
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

#
# Usage: check_variant( <variant> )
#  Terminates configuration process if specified build variant is not supported.
#
function( check_variant variant configured )
  if( NOT ${ARGV0} MATCHES none|sw|hw|drm|x11 )
    message( FATAL_ERROR "bug: unknown build variant (${ARGV0}) specified" )
  endif()

  set( configured 0 )

  if( ${ARGV0} MATCHES none )
    set( configured 1 )
  elseif( ${ARGV0} MATCHES sw )
    if( Linux )
      set( configured 0 )
    else()
      set( configured 1 )
    endif()
  elseif( ${ARGV0} MATCHES hw AND Linux AND PKG_LIBVA_FOUND )
    set( configured 1 )
  elseif( ${ARGV0} MATCHES hw AND Darwin )
    set( configured 1 )
  elseif( ${ARGV0} MATCHES drm AND PKG_LIBDRM_FOUND AND PKG_LIBVA_FOUND AND PKG_LIBVA_DRM_FOUND )
    set( configured 1 )
  elseif( ${ARGV0} MATCHES x11 AND PKG_X11_FOUND AND PKG_LIBVA_FOUND AND PKG_LIBVA_X11_FOUND )
    set( configured 1 )
  elseif( ${ARGV0} MATCHES universal AND PKG_X11_FOUND AND PKG_LIBVA_FOUND AND PKG_LIBVA_X11_FOUND AND PKG_LIBDRM_FOUND AND PKG_LIBVA_DRM_FOUND )
    set( configured 1 )
  endif()

  set( ${ARGV1} ${configured} PARENT_SCOPE )
endfunction()

#
# Usage: append_property( <target> <property_name> <property>)
#  Appends settings to the target property.
#
function( append_property target property_name property )
  get_target_property( property ${ARGV0} ${ARGV1} )
  if( property MATCHES NOTFOUND)
    set( property "" )
  endif()
  string( REPLACE ";" " " property "${ARGV2} ${property}" )
  set_target_properties( ${ARGV0} PROPERTIES ${ARGV1} "${property}" )
endfunction()

#
# Usage: configure_build_variant( <target> <variant> )
#   Sets compilation and link flags for the specified target according to the
# specified variant.
#
function( configure_build_variant target variant )
  if( Linux )
    configure_build_variant_linux( ${ARGV0} ${ARGV1} )
  elseif( Darwin )
    configure_build_variant_darwin( ${ARGV0} ${ARGV1} )
  endif()
endfunction()

function( configure_build_variant_linux target variant )
  if( NOT Linux )
    return() # should not occur; just in case
  endif()
  set( link_flags_list "-Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack -Wl,--no-as-needed -ldl")
  append_property( ${ARGV0} LINK_FLAGS "${link_flags_list} ${MFX_LDFLAGS} -fstack-protector" )
#  message( STATUS "Libva located at: ${PKG_LIBVA_LIBRARY_DIRS}" )

  if( ARGV1 MATCHES hw AND Linux )
    append_property( ${ARGV0} COMPILE_FLAGS "-DMFX_VA" )
    append_property( ${ARGV0} COMPILE_FLAGS "${PKG_LIBVA_CFLAGS}" )
    foreach(libpath ${PKG_LIBVA_LIBRARY_DIRS} )
     append_property( ${ARGV0} LINK_FLAGS "-L${libpath}" )
    endforeach()
    #append_property( ${ARGV0} LINK_FLAGS "${PKG_LIBVA_LDFLAGS_OTHER}" )

    target_link_libraries( ${ARGV0} va ${MDF_LIBS} )

  elseif( ARGV1 MATCHES universal )
    if(ENABLE_X11)
      append_property( ${ARGV0} COMPILE_FLAGS "-DLIBVA_X11_SUPPORT" )
    endif()
    append_property( ${ARGV0} COMPILE_FLAGS "-DLIBVA_DRM_SUPPORT" )
    append_property( ${ARGV0} COMPILE_FLAGS "-DMFX_VA -DLIBVA_SUPPORT" )
    append_property( ${ARGV0} COMPILE_FLAGS "${PKG_LIBDRM_CFLAGS} ${PKG_LIBVA_CFLAGS} ${PKG_LIBVA_DRM_CFLAGS} ${PKG_LIBVA_X11_CFLAGS} ${PKG_X11_CFLAGS}" )

  elseif( ARGV1 MATCHES drm )
    append_property( ${ARGV0} COMPILE_FLAGS "-DLIBVA_SUPPORT -DLIBVA_DRM_SUPPORT" )
    append_property( ${ARGV0} COMPILE_FLAGS "${PKG_LIBDRM_CFLAGS} ${PKG_LIBVA_CFLAGS} ${PKG_LIBVA_DRM_CFLAGS}" )
    foreach(libpath ${PKG_LIBDRM_LIBRARY_DIRS} ${PKG_LIBVA_LIBRARY_DIRS} ${PKG_LIBVA_DRM_LIBRARY_DIRS})
      append_property( ${ARGV0} LINK_FLAGS "-L${libpath}" )
    endforeach()
    #append_property( ${ARGV0} LINK_FLAGS "${PKG_LIBDRM_LDFLAGS_OTHER} ${PKG_LIBVA_LDFLAGS_OTHER} ${PKG_LIBVA_DRM_LDFLAGS_OTHER}" )

    target_link_libraries( ${ARGV0} va drm va-drm )

  elseif( ARGV1 MATCHES x11 )
    append_property( ${ARGV0} COMPILE_FLAGS "-DLIBVA_SUPPORT -DLIBVA_X11_SUPPORT" )
    append_property( ${ARGV0} COMPILE_FLAGS "${PKG_LIBVA_CFLAGS} ${PKG_LIBVA_X11_CFLAGS} ${PKG_X11_CFLAGS}" )
    foreach(libpath ${PKG_LIBVA_LIBRARY_DIRS} ${PKG_LIBVA_X11_LIBRARY_DIRS} ${PKG_X11_LIBRARY_DIRS} )
      append_property( ${ARGV0} LINK_FLAGS "-L${libpath}" )
    endforeach()
    #append_property( ${ARGV0} LINK_FLAGS "${PKG_LIBVA_LDFLAGS_OTHER} ${PKG_LIBVA_X11_LDFLAGS_OTHER} ${PKG_X11_LDFLAGS_OTHER}" )

    target_link_libraries( ${ARGV0} va-x11 va X11 )

  elseif( ARGV1 MATCHES none OR ARGV1 MATCHES sw)
    # Multiple 'none' and 'sw' variants include cm_rt_linux.h. And
    # cm_rt_linux.h includes LIBVA headers unconditionally. We need to tell the
    # compiler where to find the LIBVA headers, especially if LIBVA is installed
    # in a custom location.
    append_property( ${ARGV0} COMPILE_FLAGS "${PKG_LIBVA_CFLAGS}" )
    foreach(libpath ${PKG_LIBVA_LIBRARY_DIRS} )
     append_property( ${ARGV0} LINK_FLAGS "-L${libpath}" )
    endforeach()

  endif()
endfunction()

function(configure_target target cflags libdirs)
  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} ${ARGV1}" PARENT_SCOPE)
  foreach(libpath ${ARGV2})
    set(SCOPE_LINKFLAGS "${SCOPE_LINKFLAGS} -L${libpath}" PARENT_SCOPE)
  endforeach()
endfunction()

function(configure_libmfx_target target)
  # prefer local Dispatcher sources to preinstalled
  if (NOT TARGET mfx)
     configure_target(${ARGV0} "${PKG_MFX_CFLAGS}" "${PKG_MFX_LIBRARY_DIRS}")
  endif()

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS "${SCOPE_LINKFLAGS} -Wl,--default-symver" PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} mfx PARENT_SCOPE)
endfunction()

function(configure_itt_target target)
  configure_target(${ARGV0} "${ITT_CFLAGS}" "${ITT_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} ${ITT_LIBRARIES} PARENT_SCOPE)
endfunction()


function(configure_libdrm_target target)
  configure_target(${ARGV0} "${PKG_LIBDRM_CFLAGS}" "${PKG_LIBDRM_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} drm PARENT_SCOPE)
endfunction()

function(configure_libva_target target)
  configure_target(${ARGV0} "${PKG_LIBVA_CFLAGS}" "${PKG_LIBVA_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -DLIBVA_SUPPORT" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} va PARENT_SCOPE)
endfunction()

function(configure_libva_drm_target target)
  configure_target(${ARGV0} "${PKG_LIBVA_DRM_CFLAGS}" "${PKG_LIBVA_DRM_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -DLIBVA_DRM_SUPPORT" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} va-drm PARENT_SCOPE)
endfunction()

function(configure_wayland_target target)
  configure_target(${ARGV0} "${PKG_WAYLAND_CLIENT_CFLAGS}" "${PKG_WAYLAND_CLIENT_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -DLIBVA_SUPPORT -DLIBVA_WAYLAND_SUPPORT")
  if(WAYLAND_LINUX_DMABUF_XML_PATH AND WAYLAND_SCANNER_BINARY_PATH)
    set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -DWAYLAND_LINUX_DMABUF_SUPPORT")
  endif()
  set(SCOPE_CFLAGS "${SCOPE_CFLAGS}" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} drm_intel drm wayland-client PARENT_SCOPE)
endfunction()

function(configure_libva_x11_target target)
  configure_target(${ARGV0} "${PKG_LIBVA_X11_CFLAGS}" "${PKG_LIBVA_X11_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -DLIBVA_X11_SUPPORT" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} va-x11 PARENT_SCOPE)
endfunction()

function(configure_x11_target target)
  configure_target(${ARGV0} "${PKG_X11_CFLAGS}" "${PKG_X11_LIBRARY_DIRS}")
  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} X11 PARENT_SCOPE)
endfunction()

function(configure_universal_target target)
  set(LOCAL_CFLAGS "")

  if(ENABLE_V4L2)
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DENABLE_V4L2_SUPPORT" )
  endif()

  if(ENABLE_MONDELLO)
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DENABLE_MONDELLO_SUPPORT" )
  endif()

  if(ENABLE_PS)
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DENABLE_PS" )
  endif()

  if(ENABLE_FF)
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DENABLE_FF" )
  endif()

  if(PKG_LIBVA_FOUND)
    configure_target(${ARGV0} "${PKG_LIBVA_CFLAGS}" "${PKG_LIBVA_LIBRARY_DIRS}")
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DLIBVA_SUPPORT")
    if( ${PKG_LIBVA_VERSION} LESS 0.36 )
       set(LOCAL_CFLAGS "${LOCAL_CFLAGS}" )
    endif()
  endif()

  if(PKG_LIBVA_DRM_FOUND AND PKG_LIBDRM_FOUND)
    configure_target(${ARGV0} "${PKG_LIBDRM_CFLAGS}" "${PKG_LIBDRM_LIBRARY_DIRS}" "${PKG_LIBVA_DRM_CFLAGS}" "${PKG_LIBVA_DRM_LIBRARY_DIRS}")
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DLIBVA_DRM_SUPPORT")
  endif()

  if (PKG_LIBVA_X11_FOUND AND PKG_X11_FOUND)
    configure_target(${ARGV0} "${PKG_X11_CFLAGS}" "${PKG_X11_LIBRARY_DIRS}" "${PKG_LIBVA_X11_CFLAGS}" "${PKG_LIBVA_X11_LIBRARY_DIRS}")
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DLIBVA_X11_SUPPORT ")
    if ( PKG_XCB_FOUND )
        set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DX11_DRI3_SUPPORT")
    endif()
  endif()

  if (PKG_WAYLAND_CLIENT_FOUND)
    configure_target(${ARGV0} "${PKG_WAYLAND_CLIENT_CFLAGS}" "${PKG_WAYLAND_CLIENT_LIBRARY_DIRS}")
    set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DLIBVA_WAYLAND_SUPPORT")
    if(WAYLAND_LINUX_DMABUF_XML_PATH AND WAYLAND_SCANNER_BINARY_PATH)
      set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -DWAYLAND_LINUX_DMABUF_SUPPORT")
    endif()
  endif()

  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} ${LOCAL_CFLAGS}" PARENT_SCOPE)
endfunction()

function(configure_dl_target target)
  set(SCOPE_LIBS ${SCOPE_LIBS} dl PARENT_SCOPE)
endfunction()

function(configure_pthread_target target)
  set(SCOPE_LIBS ${SCOPE_LIBS} pthread PARENT_SCOPE)
endfunction()

function(configure_libavutil_target target)
  configure_target(${ARGV0} "${PKG_LIBAVUTIL_CFLAGS}" "${PKG_LIBAVUTIL_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} avutil PARENT_SCOPE)
endfunction()

function(configure_libavcodec_target target)
  configure_target(${ARGV0} "${PKG_LIBAVCODEC_CFLAGS}" "${PKG_LIBAVCODEC_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} avcodec PARENT_SCOPE)
endfunction()

function(configure_libavformat_target target)
  configure_target(${ARGV0} "${PKG_LIBAVFORMAT_CFLAGS}" "${PKG_LIBAVFORMAT_LIBRARY_DIRS}")

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS ${SCOPE_LINKFLAGS} PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} avformat PARENT_SCOPE)
endfunction()

function( configure_opencl_target target )
  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -I${OPENCL_INCLUDE}" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS "${SCOPE_LINKFLAGS} -L${OPENCL_LIBRARY_PATH}" PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} OpenCL PARENT_SCOPE)
endfunction()

function( configure_corevideo_target target )
  set(SCOPE_CFLAGS "${SCOPE_CFLAGS} -I/System/Library/Frameworks/CoreVideo.framework/Headers" PARENT_SCOPE)
  set(SCOPE_LINKFLAGS "${SCOPE_LINKFLAGS} -framework CoreVideo" PARENT_SCOPE)
endfunction()


function(configure_dependencies target dependencies variant)
  if (${ARGV2} MATCHES universal)
    configure_universal_target (${ARGV0})
  endif()

  foreach(dependency ${ARGV1})
    if(${dependency} STREQUAL libmfx)
      configure_libmfx_target(${ARGV0})
    elseif(${dependency} STREQUAL itt)
      if(ENABLE_ITT)
        configure_itt_target(${ARGV0})
      endif()
    elseif(${dependency} STREQUAL _enable_sw)
      # that's fake dependency to check --enable-sw=yes|no option
      if(NOT ENABLE_SW)
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL _force_skipping)
      # that's fake dependency to force skipping the target
      report_add_target(SKIPPING ${ARGV0})
      return()
    elseif(${dependency} STREQUAL _force_not_configured)
      # that's fake dependency to force marking the target as not configured
      report_add_target(NOT_CONFIGURED ${ARGV0})
      return()
    elseif(${dependency} STREQUAL libdrm)
      if(PKG_LIBDRM_FOUND)
        configure_libdrm_target(${ARGV0})
      else()
        report_add_target(SKIPPING ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL libva-drm)
      if(PKG_LIBVA_DRM_FOUND)
        configure_libva_drm_target(${ARGV0})
      else()
        report_add_target(SKIPPING ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL libva-x11)
      if(ENABLE_X11)
        if(PKG_LIBVA_X11_FOUND)
          configure_libva_x11_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL wayland-client)
      if(ENABLE_WAYLAND)
        if(PKG_WAYLAND_CLIENT_FOUND)
          configure_wayland_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL libva)
      if(PKG_LIBVA_FOUND)
        configure_libva_target(${ARGV0})
      else()
        report_add_target(SKIPPING ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL x11)
      if(ENABLE_X11)
        if(PKG_X11_FOUND)
          configure_x11_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL dl)
      configure_dl_target(${ARGV0})
    elseif(${dependency} STREQUAL pthread)
      configure_pthread_target(${ARGV0})
    elseif(${dependency} STREQUAL libavutil)
      if(ENABLE_FFMPEG)
        if(PKG_LIBAVUTIL_FOUND)
          configure_libavutil_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL libavcodec)
      if(ENABLE_FFMPEG)
        if(PKG_LIBAVCODEC_FOUND)
          configure_libavcodec_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL libavformat)
      if(ENABLE_FFMPEG)
        if(PKG_LIBAVFORMAT_FOUND)
          configure_libavformat_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    elseif(${dependency} STREQUAL opencl)
      if(ENABLE_OPENCL)
        if(OPENCL_FOUND)
          configure_opencl_target(${ARGV0})
        else()
          report_add_target(SKIPPING ${ARGV0})
          return()
        endif()
      else()
        report_add_target(NOT_CONFIGURED ${ARGV0})
        return()
      endif()
    else()
      message( FATAL_ERROR "Unknown dependency '${dependency}' for the target '${ARGV0}'" )
      return()
    endif()
  endforeach()

  set(SCOPE_CFLAGS ${SCOPE_CFLAGS} PARENT_SCOPE)
  set(SCOPE_LINKFLAGS "${SCOPE_LINKFLAGS} -Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack -fstack-protector" PARENT_SCOPE)
  set(SCOPE_LIBS ${SCOPE_LIBS} PARENT_SCOPE)
endfunction()

function( configure_build_variant_darwin target variant )
  if( NOT Darwin)
    return() # should not occur; just in case
  endif()

  if( ARGV1 MATCHES hw)
    list( APPEND darwin_includes
      /System/Library/Frameworks/CoreVideo.framework/Headers
      /System/Library/Frameworks/VideoToolbox.framework/Headers
      )
    list( APPEND darwin_frameworks
      CoreFoundation
      CoreMedia
      CoreVideo
      )

    append_property( ${ARGV0} COMPILE_FLAGS "-DMFX_VA" )

    foreach( item ${darwin_includes} )
      append_property( ${ARGV0} COMPILE_FLAGS "-I${item}" )
    endforeach()
    foreach( item ${darwin_frameworks} )
      append_property( ${ARGV0} LINK_FLAGS "-framework ${item}" )
    endforeach()
  endif()

endfunction()

if( Linux )
  find_package(PkgConfig REQUIRED)

  # required:
  pkg_check_modules(PKG_LIBVA     REQUIRED libva>=1.10.0)
  pkg_check_modules(PKG_LIBDRM    REQUIRED libdrm)
  pkg_check_modules(PKG_LIBVA_DRM REQUIRED libva-drm>=1.10.0)

  # optional:
  pkg_check_modules( PKG_MFX       libmfx>=1.28 )

  if ( ENABLE_X11 OR NOT DEFINED ENABLE_X11)
    if ( ENABLE_X11 )
      set(X11_REQUIRED REQUIRED)
    endif()

    pkg_check_modules( PKG_X11       ${X11_REQUIRED} x11 )
    pkg_check_modules( PKG_LIBVA_X11 ${X11_REQUIRED} libva-x11>=1.10.0 )

    if ( PKG_X11_FOUND AND PKG_LIBVA_X11_FOUND )
      set( ENABLE_X11 ON )
    else()
      set( ENABLE_X11 OFF )
      message( STATUS "x11 modules not found (optional). Samples will be built without x11 support" )
    endif()
  endif()

  if( ENABLE_X11_DRI3 )
    pkg_check_modules(PKG_XCB REQUIRED xcb xcb-dri3 x11-xcb xcb-present)
  endif()

  if( ENABLE_WAYLAND )
    pkg_check_modules(PKG_DRM_INTEL      REQUIRED libdrm_intel)
    pkg_check_modules(PKG_WAYLAND_CLIENT REQUIRED wayland-client)

    pkg_check_modules(PKG_WAYLAND_SCANNER "wayland-scanner>=1.15")
    pkg_check_modules(PKG_WAYLAND_PROTCOLS "wayland-protocols>=1.15")

    if ( PKG_WAYLAND_SCANNER_FOUND AND PKG_WAYLAND_PROTCOLS_FOUND )
      pkg_get_variable(WAYLAND_PROTOCOLS_PATH wayland-protocols pkgdatadir)
      if(WAYLAND_PROTOCOLS_PATH)
        find_file(
            WAYLAND_LINUX_DMABUF_XML_PATH linux-dmabuf-unstable-v1.xml
            PATHS ${WAYLAND_PROTOCOLS_PATH}/unstable/linux-dmabuf
            NO_DEFAULT_PATH)
      endif()

      pkg_get_variable(WAYLAND_SCANNER_BIN_PATH wayland-scanner bindir)
      pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)
      if ( WAYLAND_SCANNER_BIN_PATH AND WAYLAND_SCANNER_BIN )
        find_program(
            WAYLAND_SCANNER_BINARY_PATH ${WAYLAND_SCANNER_BIN}
            PATHS ${WAYLAND_SCANNER_BIN_PATH})
      endif()
    endif()
  endif()
endif()

