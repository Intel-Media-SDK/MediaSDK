# Copyright (c) 2017-2018 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs( )

set(UMC_CODECS ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/umc/codec/)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h264/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h265/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/mpeg2/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/vc1/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/vp8/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/vp9/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/av1/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../vpp/include )

include_directories( ${UMC_CODECS}/h265_dec/include )
include_directories( ${UMC_CODECS}/h264_dec/include )
include_directories( ${UMC_CODECS}/mpeg2_dec/include )
include_directories( ${UMC_CODECS}/vp9_dec/include )
include_directories( ${UMC_CODECS}/vc1_dec/include )
include_directories( ${UMC_CODECS}/av1_dec/include )
include_directories( ${UMC_CODECS}/jpeg_dec/include )
include_directories( ${UMC_CODECS}/vc1_common/include )
include_directories( ${UMC_CODECS}/color_space_converter/include )
include_directories( ${UMC_CODECS}/jpeg_common/include )

set(sources "")

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/h264/src/mfx_h264_dec_decode.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_au_splitter.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_dec_bitstream_headers.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_dec_debug.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_dec_defs_yuv.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_dec_mfx.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_dec_slice_decoder_decode_pic.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_frame.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_frame_list.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_heap.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_mfx_supplier.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_nal_spl.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_segment_decoder_dxva.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_slice_decoding.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_task_broker.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_task_supplier.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_va_packer.cpp
    ${UMC_CODECS}/h264_dec/src/umc_h264_va_supplier.cpp
    )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/h265/src/mfx_h265_dec_decode.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_au_splitter.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_bitstream_headers.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_debug.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_frame.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_frame_info.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_frame_list.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_heap.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_mfx_supplier.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_nal_spl.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_scaling_list.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_segment_decoder_dxva.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_sei.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_slice_decoding.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_tables.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_task_broker.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_task_supplier.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_va_packer.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_va_packer_vaapi.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_va_supplier.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_yuv.cpp
    ${UMC_CODECS}/h265_dec/src/umc_h265_mfx_utils.cpp
    )
list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg/src/mfx_mjpeg_dec_decode.cpp
    ${UMC_CODECS}/jpeg_common/src/bitstreamin.cpp
    ${UMC_CODECS}/jpeg_common/src/bitstreamout.cpp
    ${UMC_CODECS}/jpeg_common/src/colorcomp.cpp
    ${UMC_CODECS}/jpeg_common/src/jpegbase.cpp
    ${UMC_CODECS}/jpeg_common/src/membuffin.cpp
    ${UMC_CODECS}/jpeg_common/src/membuffout.cpp
    ${UMC_CODECS}/jpeg_dec/src/dechtbl.cpp
    ${UMC_CODECS}/jpeg_dec/src/decqtbl.cpp
    ${UMC_CODECS}/jpeg_dec/src/jpegdec.cpp
    ${UMC_CODECS}/jpeg_dec/src/jpegdec_base.cpp
    ${UMC_CODECS}/jpeg_dec/src/mfx_mjpeg_task.cpp
    ${UMC_CODECS}/jpeg_dec/src/umc_jpeg_frame_constructor.cpp
    ${UMC_CODECS}/jpeg_dec/src/umc_mjpeg_mfx_decode.cpp
    ${UMC_CODECS}/jpeg_dec/src/umc_mjpeg_mfx_decode_base.cpp
    ${UMC_CODECS}/jpeg_dec/src/umc_mjpeg_mfx_decode_hw.cpp
    )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/mpeg2/src/mfx_mpeg2_decode.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_bitstream.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_decoder.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_decoder_va.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_frame.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_slice.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_splitter.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_utils.cpp
    ${UMC_CODECS}/mpeg2_dec/src/umc_mpeg2_va_packer.cpp
    )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/vc1/src/mfx_vc1_decode.cpp
    ${UMC_CODECS}/vc1_common/src/umc_vc1_common.cpp
    ${UMC_CODECS}/vc1_common/src/umc_vc1_common_tables.cpp
    ${UMC_CODECS}/vc1_common/src/umc_vc1_common_tables_adv.cpp
    ${UMC_CODECS}/vc1_common/src/umc_vc1_spl_frame_constr.cpp
    ${UMC_CODECS}/vc1_common/src/umc_vc1_spl_tbl.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_huffman.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_bitplane.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_bpic.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_bpic_adv.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_frame_descr.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_frame_va.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_ipic.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_ipic_adv.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_pic_com.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_pic_com_adv.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_ppic.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_ppic_adv.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_seq.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_skipping.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_task_store.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_dec_vopdq.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_video_decoder.cpp
    ${UMC_CODECS}/vc1_dec/src/umc_vc1_video_decoder_hw.cpp
    )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_hw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_hw_vaapi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vp8/src/mfx_vp8_dec_decode_tables.cpp
    )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode_hw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vp9/src/mfx_vp9_dec_decode_utils.cpp
    ${UMC_CODECS}/vp9_dec/src/umc_vp9_bitstream.cpp
    ${UMC_CODECS}/vp9_dec/src/umc_vp9_utils.cpp
    ${UMC_CODECS}/vp9_dec/src/umc_vp9_va_packer.cpp
    )

if( MFX_ENABLE_SW_FALLBACK )
    list(APPEND sources
        ${UMC_CODECS}/color_space_converter/src/umc_video_processing.cpp
        ${UMC_CODECS}/color_space_converter/src/umc_color_space_conversion.cpp
        ${UMC_CODECS}/color_space_converter/src/umc_deinterlacing.cpp
        )
endif()

set( sources.plus "" )

set( defs "${API_FLAGS} ${WARNING_FLAGS}" )

list(APPEND sources
    ${CMAKE_CURRENT_SOURCE_DIR}/av1/src/mfx_av1_dec_decode.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_bitstream.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_decoder.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_decoder_va.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_frame.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_utils.cpp
    ${UMC_CODECS}/av1_dec/src/umc_av1_va_packer_vaapi.cpp
    )

make_library( decode hw static )
set( defs "" )

