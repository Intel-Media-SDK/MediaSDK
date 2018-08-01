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

list ( APPEND MFX_SW_DECODERS "h265" )

list ( APPEND MFX_HW_DECODERS "h265" )

add_definitions( -DUMC_ENABLE_H265_VIDEO_DECODER )

set ( MFX_USE_AUDIO_CODECS OFF )
