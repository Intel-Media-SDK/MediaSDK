Name: @PROJECT_NAME@
Description: Intel(R) Media SDK Dispatcher
Version: @MFX_VERSION_MAJOR@.@MFX_VERSION_MINOR@

prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/lib/lin_x64
includedir=@CMAKE_INSTALL_PREFIX@/include
Libs: -L${libdir} -lmfx
Cflags: -I${includedir}
