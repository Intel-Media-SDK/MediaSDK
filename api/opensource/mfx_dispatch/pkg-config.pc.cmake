Name: @PROJECT_NAME@
Description: Intel(R) Media SDK Dispatcher
Version: @MFX_VERSION_MAJOR@.@MFX_VERSION_MINOR@

prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/@INSTALL_LIB_PATH@
includedir=@CMAKE_INSTALL_PREFIX@/include
Libs: -L${libdir} -lmfx
Cflags: -I${includedir}
