set( MFX_API_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

add_definitions(-DMFX_D3D11_ENABLED -D_UNICODE -DUNICODE)

list(APPEND sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_critical_section.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dispatcher_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_driver_store_loader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_dxva2_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_function_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_library_iterator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_load_dll.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_load_plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_plugin_hive.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mfx_win_reg_key.cpp
)

include_directories (
  ${MFX_API_FOLDER}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library (mfx SHARED ${sources})
set_target_properties( mfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER mfx )

install (TARGETS mfx
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install( DIRECTORY ${MFX_API_FOLDER}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mfx FILES_MATCHING PATTERN *.h )
