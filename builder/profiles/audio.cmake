set ( MFX_COPYRIGHT "Copyright(c) 2014-2018 Intel Corporation" )
set ( MFX_PRODUCT_NAME "Intel(R) Media SDK - Audio for Linux*" )

set ( MFX_NO_COPY_KERNEL_GENX OFF )
set ( MFX_TRACE_DISABLE OFF )

set ( MFX_USER_ENCODE OFF )
set ( MFX_USER_DECODE OFF )

set ( MFX_USE_VPP OFF )
set ( MFX_USE_JPEG OFF )

set ( MFX_USE_FEI OFF )
set ( MFX_USE_H264LA OFF )

set ( MFX_USE_AUDIO_CODECS ON )

add_definitions( -DUMC_ENABLE_AAC_AUDIO_DECODER )
add_definitions( -DUMC_ENABLE_MP3_AUDIO_DECODER )
add_definitions( -DUMC_ENABLE_AAC_AUDIO_ENCODER )

