# Copyright (c) 2018-2020 Intel Corporation
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

mfx_include_dirs()

### ENCODE HW
list( APPEND Encoders vp9 h265 h264 mjpeg mpeg2 )
foreach( dir ${Encoders} )
  include_directories( ${MSDK_LIB_ROOT}/encode_hw/${dir}/include )
endforeach()

include_directories( ${MSDK_STUDIO_ROOT}/enctools/include )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/shared )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/agnostic )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/agnostic/base )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/agnostic/g12 )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/linux )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/linux/base )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc/linux/g12 )
include_directories( ${MSDK_LIB_ROOT}/shared/include)
include_directories( ${MSDK_LIB_ROOT}/cmrt_cross_platform/include )
include_directories( ${MSDK_LIB_ROOT}/mctf_package/mctf/include )
include_directories( ${MSDK_LIB_ROOT}/enc_hw/mpeg2/include )
include_directories( ${MSDK_LIB_ROOT}/pak/mpeg2/include )
include_directories( ${MSDK_LIB_ROOT}/genx/h264_encode/isa )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )
# umc codec
include_directories( ${MSDK_UMC_ROOT}/codec/brc/include )
include_directories( ${MSDK_UMC_ROOT}/codec/jpeg_common/include )
include_directories( ${MSDK_UMC_ROOT}/codec/h264_enc/include )

set( defs "" )
set( sources "" )
foreach( dir ${Encoders} )
  file( GLOB_RECURSE srcs "${dir}/src/*.c" "${dir}/src/*.cpp" )
  list( APPEND sources ${srcs})
endforeach()
foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_ddi_enc_dump.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/mfx_h264_fei_vaapi.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
    ${prefix}/mfx_mpeg2_enc_common_hw.cpp
    ${prefix}/mfx_mpeg2_encode_factory.cpp
    ${prefix}/mfx_mpeg2_encode_vaapi.cpp
    )
endforeach()

list( APPEND sources
    shared/ehw_resources_pool.cpp
    shared/ehw_task_manager.cpp
    shared/ehw_device_vaapi.cpp
    shared/ehw_utils_vaapi.cpp
    hevc/hevcehw_disp.cpp
    hevc/agnostic/hevcehw_base.cpp
    hevc/agnostic/base/hevcehw_base_impl.cpp
    hevc/agnostic/base/hevcehw_base_alloc.cpp
    hevc/agnostic/base/hevcehw_base_constraints.cpp
    hevc/agnostic/base/hevcehw_base_dirty_rect.cpp
    hevc/agnostic/base/hevcehw_base_dpb_report.cpp
    hevc/agnostic/base/hevcehw_base_encoded_frame_info.cpp
    hevc/agnostic/base/hevcehw_base_ext_brc.cpp
    hevc/agnostic/base/hevcehw_base_la_ext_brc.cpp
    hevc/agnostic/base/hevcehw_base_hdr_sei.cpp
    hevc/agnostic/base/hevcehw_base_hrd.cpp
    hevc/agnostic/base/hevcehw_base_interlace.cpp
    hevc/agnostic/base/hevcehw_base_legacy.cpp
    hevc/agnostic/base/hevcehw_base_legacy_defaults.cpp
    hevc/agnostic/base/hevcehw_base_max_frame_size.cpp
    hevc/agnostic/base/hevcehw_base_packer.cpp
    hevc/agnostic/base/hevcehw_base_recon_info.cpp
    hevc/agnostic/base/hevcehw_base_parser.cpp
    hevc/agnostic/base/hevcehw_base_roi.cpp
    hevc/agnostic/base/hevcehw_base_task.cpp
    hevc/agnostic/base/hevcehw_base_weighted_prediction.cpp
    hevc/agnostic/g12/hevcehw_g12_caps.cpp
    hevc/agnostic/g12/hevcehw_g12_rext.cpp
    hevc/agnostic/g12/hevcehw_g12_scc.cpp
    hevc/linux/base/hevcehw_base_dirty_rect_lin.cpp
    hevc/linux/base/hevcehw_base_fei_lin.cpp
    hevc/linux/base/hevcehw_base_interlace_lin.cpp
    hevc/linux/base/hevcehw_base_lin.cpp
    hevc/linux/base/hevcehw_base_roi_lin.cpp
    hevc/linux/base/hevcehw_base_va_lin.cpp
    hevc/linux/base/hevcehw_base_va_packer_lin.cpp
    hevc/linux/base/hevcehw_base_weighted_prediction_lin.cpp
    hevc/linux/base/hevcehw_base_max_frame_size_lin.cpp
    hevc/linux/g12/hevcehw_g12_lin.cpp
    hevc/linux/g12/hevcehw_g12_rext_lin.cpp
    hevc/linux/g12/hevcehw_g12_qp_modulation_lin.cpp
)

make_library( encode hw static )

### BRC
set( SRC_DIR "${MSDK_UMC_ROOT}/codec/brc/src" )
set( defs "" )
set( sources "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( bitrate_control none static )
