set ( MFX_COPYRIGHT "Copyright(c) 2018 Intel Corporation" )
set ( MFX_PRODUCT_NAME "Intel(R) Media SDK for Embedded Linux" )

set ( MFX_NO_COPY_KERNEL_GENX ON )
set ( MFX_TRACE_DISABLE ON )

set ( MFX_USE_VPP ON )

set ( MFX_USE_JPEG OFF )

set ( MFX_USER_ENCODE OFF )
set ( MFX_USER_DECODE OFF )

# MFX_USE_HW_ENCODERS must be turned on
set ( MFX_USE_FEI OFF )
set ( MFX_USE_H264LA OFF )

set ( MFX_SW_DECODERS "" )
set ( MFX_SW_ENCODERS "" )
set ( MFX_HW_DECODERS "" )
set ( MFX_HW_ENCODERS "" )
set ( MFX_PLUGINS "" )

list ( APPEND MFX_SW_DECODERS "h264" )
list ( APPEND MFX_SW_DECODERS "vp9"  )

list ( APPEND MFX_HW_DECODERS "h264" )
list ( APPEND MFX_HW_DECODERS "vp9" )

add_definitions( -DUMC_ENABLE_H264_VIDEO_DECODER )
add_definitions( -DUMC_ENABLE_VP9_VIDEO_DECODER )
#add_definitions( -DUMC_ENABLE_H264_VIDEO_ENCODER )

set ( MFX_USE_AUDIO_CODECS OFF )
