#
# ========= User editable part begins here. =========
#

set ( MFX_COPYRIGHT "Copyright(c) 2018 Intel Corporation" )
set ( MFX_PRODUCT_NAME "Intel(R) Media SDK" )
#set ( MFX_PRODUCT_NAME "Intel(R) Media SDK for Embedded Linux" )

#set ( MFX_COPYRIGHT "Copyright(c) 2014-2018 Intel Corporation" )
#set ( MFX_PRODUCT_NAME "Intel(R) Media SDK - Audio for Linux*" )

# components subset

set ( MFX_USE_COPY_KERNEL_GENX OFF )

set ( MFX_USE_AUDIO_CODECS OFF )

set ( MFX_USE_SW_CODECS OFF )
set ( MFX_USE_HW_CODECS ON )

set ( MFX_USE_ENCODERS OFF )
set ( MFX_USE_DECODERS ON )

set ( MFX_USE_FEI OFF )
set ( MFX_USE_H264LA OFF )
set ( MFX_USE_VPP ON )
set ( MFX_USE_JPEG OFF )

list ( APPEND MFX_CODECS "h264" )
#list ( APPEND MFX_CODECS "h265" )
#list ( APPEND MFX_CODECS "mpeg2" )
#list ( APPEND MFX_CODECS "mjpeg" )
#list ( APPEND MFX_CODECS "vc1" )
#list ( APPEND MFX_CODECS "vp8" )
#list ( APPEND MFX_CODECS "vp9" )


set ( MFX_TRACE_DISABLE ON )

#
# ========= User editable part stop[s here. =========
#

cmake_policy(SET CMP0057 NEW)

if ( ${MFX_USE_COPY_KERNEL_GENX} )
  add_definitions( -DMFX_USE_COPY_KERNEL_GENX)
endif()

if ( ${MFX_USE_VPP} )
  add_definitions( -DMFX_ENABLE_VPP)
  add_definitions( -DMFX_ENABLE_DENOISE_VIDEO_VPP)
  add_definitions( -DMFX_ENABLE_VPP_COMPOSITION)
#  add_definitions( -DMFX_ENABLE_VPP_FRC)
#  add_definitions( -DMFX_ENABLE_VPP_ROTATION)
#  add_definitions( -DMFX_ENABLE_VPP_VIDEO_SIGNAL)
endif()

# sw encodes/decoders 
if ( ${MFX_USE_SW_CODECS} )
  if ( ${MFX_USE_DECODERS} )
    if ("h264" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE)
    endif()
    if ("h265" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_DECODE)
      add_definitions( -DAS_HEVCD_PLUGIN)
    endif()
    if ("mjpeg" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE)
    endif()
    if ("vc1" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE)
    endif()
    if ("mpeg2" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE)
    endif()
    if ("vp8" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE)
    endif()
    if ("vp9" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE)
      add_definitions( -DAS_VP9D_PLUGIN)
    endif()
  endif()

  if ( ${MFX_USE_ENCODERS} )
    if ("h264" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE)
    endif()
    if ("h265" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
      add_definitions( -DAS_HEVCE_PLUGIN)
    endif()
    if ("mjpeg" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE)
    endif()
    if ("mpeg2" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE)
    endif()
    if ("vp8" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE)
    endif()
    if ("vp9" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE)
      add_definitions( -DAS_VP9E_PLUGIN)
    endif()
  endif()

  add_definitions( -DMFX_ENABLE_USER_DECODE)
  add_definitions( -DMFX_ENABLE_USER_ENCODE)

endif()


# hw encode/decode 
if ( ${MFX_USE_HW_CODECS} )
  if ( ${MFX_USE_DECODERS} )
    if ("h264" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE_HW)
      add_definitions( -DUMC_ENABLE_H264_VIDEO_DECODER)
    endif()
    if ("h265" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE_HW)
      add_definitions( -DUMC_ENABLE_H265_VIDEO_DECODER)
      add_definitions( -DAS_HEVCD_PLUGIN -DMFX_VA)
    endif()
    if ("mjpeg" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE_HW)
      add_definitions( -DUMC_ENABLE_MJPEG_VIDEO_DECODER)
    endif()
    if ("vc1" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE_HW)
      add_definitions( -DUMC_ENABLE_VC1_VIDEO_DECODER)
    endif()
    if ("mpeg2" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE_HW)
      add_definitions( -DUMC_ENABLE_MPEG2_VIDEO_DECODER)
    endif()
    if ("vp8" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE_HW)
    endif()
    if ("vp9" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE_HW)
      add_definitions( -DAS_VP9D_PLUGIN -DMFX_VA)
    endif()
  endif()

  if ( ${MFX_USE_ENCODERS} )
    if ("h264" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE_HW)
      add_definitions( -DUMC_ENABLE_H264_VIDEO_ENCODER)
    endif()
    if ("h265" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE_HW)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
      add_definitions( -DAS_HEVCE_PLUGIN -DMFX_VA)
      add_definitions( -DUMC_ENABLE_H265_VIDEO_ENCODER)
    endif()
    if ("mjpeg" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE_HW)
      add_definitions( -DUMC_ENABLE_MJPEG_VIDEO_ENCODER)
    endif()
    if ("mpeg2" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE_HW)
      add_definitions( -DUMC_ENABLE_MPEG2_VIDEO_ENCODER)
    endif()
    if ("vp8" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE_HW)
    endif()
    if ("vp9" IN_LIST MFX_CODECS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE_HW)
      add_definitions( -DAS_VP9E_PLUGIN)
    endif()
  endif()

  if ( ${MFX_USE_FEI} )
    add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENCPAK)
    add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PREENC)
    add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENC)
    add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PAK)
    add_definitions( -DMFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)
  endif()

  if ( ${MFX_USE_H264LA} )
    add_definitions( -DMFX_ENABLE_LA_H264_VIDEO_HW)
    add_definitions( -DAS_H264LA_PLUGIN)
  endif()

endif()

# audio codecs
if ( ${MFX_USE_AUDIO_CODECS} )
  add_definitions( -DMFX_ENABLE_AAC_AUDIO_DECODE)
  add_definitions( -DMFX_ENABLE_MP3_AUDIO_DECODE)

  add_definitions( -DUMC_ENABLE_AAC_AUDIO_DECODER)
  add_definitions( -DUMC_ENABLE_MP3_AUDIO_DECODER)
  add_definitions( -DUMC_ENABLE_AAC_AUDIO_ENCODER)
endif()

if ( ${MFX_TRACE_DISABLE} )
  add_definitions( -DMFX_TRACE_DISABLE)
endif()

add_definitions( -DMFX_CODECS_CONFIGURED )

set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DMFX_PRODUCT_NAME='\"${MFX_PRODUCT_NAME}\"' ")
set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DMFX_COPYRIGHT='\"${MFX_COPYRIGHT}\"' ")

set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${MFX_CXX_FLAGS} )
string (REPLACE ";" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

