function (add_codecs_definitions)
    cmake_policy(SET CMP0057 NEW)

    if ( ${MFX_TRACE_DISABLE} )
        add_definitions( -DMFX_TRACE_DISABLE)
    endif()

    if ( ${MFX_NO_COPY_KERNEL_GENX} )
        add_definitions( -DMFX_NO_COPY_KERNEL_GENX)
    endif()

    if ( ${MFX_USE_VPP} )
      add_definitions( -DMFX_ENABLE_VPP)
      add_definitions( -DMFX_ENABLE_DENOISE_VIDEO_VPP)
      add_definitions( -DMFX_ENABLE_VPP_COMPOSITION)
      add_definitions( -DMFX_ENABLE_SCENE_CHANGE_DETECTION_VPP)
#      add_definitions( -DMFX_ENABLE_VPP_FRC)
      add_definitions( -DMFX_ENABLE_VPP_ROTATION)
      add_definitions( -DMFX_ENABLE_VPP_VIDEO_SIGNAL)
      add_definitions( -DMFX_ENABLE_VPP_RUNTIME_HSBC)
    endif()

    # Software decoders

    if ("h264" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE)
    endif()
    if ("h265" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_DECODE)
    endif()
    if ("mjpeg" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE)
    endif()
    if ("vc1" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE)
    endif()
    if ("mpeg2" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE)
    endif()
    if ("vp8" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE)
    endif()
    if ("vp9" IN_LIST MFX_SW_DECODERS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE)
    endif()

    # Software encoders

    if ("h264" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE)
    endif()
    if ("h265" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
    endif()
    if ("mjpeg" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE)
    endif()
    if ("mpeg2" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE)
    endif()
    if ("vp8" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE)
    endif()
    if ("vp9" IN_LIST MFX_SW_ENCODERS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE)
    endif()

    # Hardware decoders

    if ("h264" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE_HW)
    endif()
    if ("h265" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE_HW)
    endif()
    if ("mjpeg" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE_HW)
    endif()
    if ("vc1" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE_HW)
    endif()
    if ("mpeg2" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE_HW)
    endif()
    if ("vp8" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE_HW)
    endif()
    if ("vp9" IN_LIST MFX_HW_DECODERS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE_HW)
    endif()

    # Hardware encoders

    if ("h264" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE_HW)
    endif()
    if ("h265" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE_HW)
      add_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
      add_definitions( -DAS_HEVCE_PLUGIN -DMFX_VA)
      add_definitions( -DMFX_ENABLE_HEVCE_INTERLACE)
      add_definitions( -DMFX_ENABLE_HEVCE_ROI)
    endif()
    if ("mjpeg" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE_HW)
    endif()
    if ("mpeg2" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE_HW)
    endif()
    if ("vp8" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE_HW)
    endif()
    if ("vp9" IN_LIST MFX_HW_ENCODERS )
      add_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE_HW)
    endif()


    if ( ${MFX_USE_H264LA} )
      add_definitions( -DMFX_ENABLE_LA_H264_VIDEO_HW)
    else ()
      if ( ${MFX_USE_FEI} )
        add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENCPAK)
        add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PREENC)
        add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENC)
        add_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PAK)
      endif()
    endif()

    # audio codecs
    if ( ${MFX_USE_AUDIO_CODECS} )
      add_definitions( -DMFX_ENABLE_AAC_AUDIO_DECODE)
      add_definitions( -DMFX_ENABLE_MP3_AUDIO_DECODE)
    endif()

    if ( ${MFX_USER_ENCODE} )
        add_definitions( -DMFX_ENABLE_USER_ENCODE)
    endif()
    if ( ${MFX_USER_DECODE} )
        add_definitions( -DMFX_ENABLE_USER_DECODE)
    endif()

endfunction() # add_codecs_definitions()

function (remove_codecs_definitions)

        remove_definitions( -DMFX_ENABLE_VPP)
        remove_definitions( -DMFX_ENABLE_DENOISE_VIDEO_VPP)
        remove_definitions( -DMFX_ENABLE_VPP_COMPOSITION)
        remove_definitions( -DMFX_ENABLE_VPP_FRC)
        remove_definitions( -DMFX_ENABLE_VPP_ROTATION)
        remove_definitions( -DMFX_ENABLE_VPP_VIDEO_SIGNAL)

    # Software decoders

        remove_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_HEVC_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE)
        remove_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE)

    # Software encoders

        remove_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE)
        remove_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE)

    # Hardware decoders

        remove_definitions( -DMFX_ENABLE_H264_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_H265_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_MJPEG_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_VC1_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_MPEG2_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_VP8_VIDEO_DECODE_HW)
        remove_definitions( -DMFX_ENABLE_VP9_VIDEO_DECODE_HW)

    # Hardware encoders

        remove_definitions( -DMFX_ENABLE_H264_VIDEO_ENCODE_HW)
        remove_definitions( -DMFX_ENABLE_H265_VIDEO_ENCODE_HW)
        remove_definitions( -DMFX_ENABLE_HEVC_VIDEO_ENCODE)
        remove_definitions( -DAS_HEVCE_PLUGIN -DMFX_VA)
        remove_definitions( -DMFX_ENABLE_MJPEG_VIDEO_ENCODE_HW)
        remove_definitions( -DMFX_ENABLE_MPEG2_VIDEO_ENCODE_HW)
        remove_definitions( -DMFX_ENABLE_VP8_VIDEO_ENCODE_HW)
        remove_definitions( -DMFX_ENABLE_VP9_VIDEO_ENCODE_HW)

        remove_definitions( -DMFX_ENABLE_LA_H264_VIDEO_HW)

        remove_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENCPAK)
        remove_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PREENC)
        remove_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_ENC)
        remove_definitions( -DMFX_ENABLE_H264_VIDEO_FEI_PAK)

    # Audio codecs

        remove_definitions( -DMFX_ENABLE_AAC_AUDIO_DECODE)
        remove_definitions( -DMFX_ENABLE_MP3_AUDIO_DECODE)

        remove_definitions( -DMFX_ENABLE_USER_ENCODE)
        remove_definitions( -DMFX_ENABLE_USER_DECODE)

        remove_definitions( -DMFX_NO_COPY_KERNEL_GENX)
        remove_definitions( -DMFX_TRACE_DISABLE)

endfunction() # remove_codecs_definitions()
