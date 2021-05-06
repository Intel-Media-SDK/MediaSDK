Name: @PROJECT_NAME@
Description: Intel(R) Media SDK
Version: @mfx_version_major@.@mfx_version_minor@

prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@

Libs: -L${libdir} -l@mfxlibname@
Cflags: -I${includedir} -I${includedir}/mfx
