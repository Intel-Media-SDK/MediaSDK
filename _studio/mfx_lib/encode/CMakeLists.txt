if( MFX_ENABLE_SW_FALLBACK )
  set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

  mfx_include_dirs( )

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_common/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_enc/include )
  include_directories( ${MSDK_UMC_ROOT}/codec/color_space_converter/include )

  set( defs "" )
  set( sources "" )
  set( sources.plus "" )

  list( APPEND sources
      mjpeg/src/mfx_mjpeg_encode.cpp
      )
  make_library( encode none static )
  set( defs "" )

  ### JPEG Encoder
  set( sources "" )
  set( defs "" )
  list( APPEND sources
      ${MSDK_UMC_ROOT}/codec/jpeg_enc/src/enchtbl.cpp
      ${MSDK_UMC_ROOT}/codec/jpeg_enc/src/encqtbl.cpp
      ${MSDK_UMC_ROOT}/codec/jpeg_enc/src/jpegenc.cpp
      ${MSDK_UMC_ROOT}/codec/jpeg_enc/src/jpegencrst.cpp
      ${MSDK_UMC_ROOT}/codec/jpeg_enc/src/umc_mjpeg_video_encoder.cpp
      )
  make_library( encode sw static )
endif()
