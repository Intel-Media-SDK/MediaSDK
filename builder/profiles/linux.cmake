set ( MFX_COPYRIGHT "Copyright(c) 2018 Intel Corporation" )
set ( MFX_PRODUCT_NAME "Intel(R) Media SDK" )

add_definitions( -DMFX_VA_LINUX )

set ( MFX_NO_COPY_KERNEL_GENX OFF )
set ( MFX_TRACE_DISABLE OFF )

set ( MFX_USER_ENCODE ON )
set ( MFX_USER_DECODE ON )

set ( MFX_USE_VPP ON )
set ( MFX_USE_JPEG ON )

set ( MFX_SW_DECODERS "" )
set ( MFX_SW_ENCODERS "" )
set ( MFX_HW_DECODERS "" )
set ( MFX_HW_ENCODERS "" )
set ( MFX_PLUGINS "" )

list ( APPEND MFX_PLUGINS "hevcd_plugin")
list ( APPEND MFX_PLUGINS "hevce_hw_plugin")
list ( APPEND MFX_PLUGINS "hevc_fei_plugin")
list ( APPEND MFX_PLUGINS "vp8d_plugin")
list ( APPEND MFX_PLUGINS "vp9d_plugin")

# MFX_USE_HW_ENCODERS must be turned on
set ( MFX_USE_FEI ON )
set ( MFX_USE_H264LA OFF )

list ( APPEND MFX_SW_DECODERS "h264" )
list ( APPEND MFX_SW_DECODERS "h265" )
list ( APPEND MFX_SW_DECODERS "vc1" )
list ( APPEND MFX_SW_DECODERS "mpeg2" )
list ( APPEND MFX_SW_DECODERS "mjpeg" )
#list ( APPEND MFX_SW_DECODERS "vp8" )
list ( APPEND MFX_SW_DECODERS "vp9" )

list ( APPEND MFX_SW_ENCODERS "h264" )
#list ( APPEND MFX_SW_ENCODERS "h265" )
#list ( APPEND MFX_SW_ENCODERS "vc1" )
list ( APPEND MFX_SW_ENCODERS "mpeg2" )
list ( APPEND MFX_SW_ENCODERS "mjpeg" )
#list ( APPEND MFX_SW_ENCODERS "vp8" )
#list ( APPEND MFX_SW_ENCODERS "vp9" )

#list ( APPEND MFX_HW_DECODERS "h264" )
#list ( APPEND MFX_HW_DECODERS "h265" )
list ( APPEND MFX_HW_DECODERS "vc1" )
list ( APPEND MFX_HW_DECODERS "mpeg2" )
#list ( APPEND MFX_HW_DECODERS "mjpeg" )
list ( APPEND MFX_HW_DECODERS "vp8" )
list ( APPEND MFX_HW_DECODERS "vp9" )

list ( APPEND MFX_HW_ENCODERS "h264" )
#list ( APPEND MFX_HW_ENCODERS "h265" )
#list ( APPEND MFX_HW_ENCODERS "vc1" )
list ( APPEND MFX_HW_ENCODERS "mpeg2" )
#list ( APPEND MFX_HW_ENCODERS "mvc" )
#list ( APPEND MFX_HW_ENCODERS "mjpeg" )
#list ( APPEND MFX_HW_ENCODERS "vp8" )
#list ( APPEND MFX_HW_ENCODERS "vp9" )

add_definitions( -DUMC_ENABLE_MJPEG_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_MPEG2_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_VC1_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_H264_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_H265_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_VP9_VIDEO_DECODER )

add_definitions( -DUMC_ENABLE_MJPEG_VIDEO_ENCODER )
add_definitions( -DUMC_ENABLE_MPEG2_VIDEO_ENCODER )
add_definitions( -DUMC_ENABLE_MVC_VIDEO_ENCODER )
add_definitions( -DUMC_ENABLE_H264_VIDEO_ENCODER )

add_definitions( -DUMC_ENABLE_UMC_SCENE_ANALYZER )

set ( MFX_USE_AUDIO_CODECS ON )

add_definitions( -DUMC_ENABLE_AAC_AUDIO_DECODER )
add_definitions( -DUMC_ENABLE_MP3_AUDIO_DECODER )
add_definitions( -DUMC_ENABLE_AAC_AUDIO_ENCODER )

