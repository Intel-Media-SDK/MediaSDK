LOCAL_PATH:= $(MFX_HOME)/_studio

# =============================================================================

MFX_LOCAL_DECODERS := h265 h264 mpeg2 vc1 mjpeg vp8 vp9
MFX_LOCAL_ENCODERS := h265 h264 mjpeg vp9

# Setting subdirectories to march thru
MFX_LOCAL_DIRS := \
    scheduler \
    fei

MFX_LOCAL_DIRS_IMPL := \
    $(addprefix decode/, $(MFX_LOCAL_DECODERS)) \
    vpp

MFX_LOCAL_DIRS_HW := \
    $(addprefix encode_hw/, $(MFX_LOCAL_ENCODERS)) \
    mctf_package/mctf \
    cmrt_cross_platform

MFX_LOCAL_SRC_FILES := \
    $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/src/*.cpp)))

MFX_LOCAL_SRC_FILES_IMPL := \
    $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS_IMPL), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/src/*.cpp)))

MFX_LOCAL_SRC_FILES_HW := \
    $(MFX_LOCAL_SRC_FILES_IMPL) \
    $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS_HW), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/src/*.cpp)))

MFX_LOCAL_SRC_FILES_HW += $(addprefix mfx_lib/genx/h264_encode/isa/, \
    genx_simple_me_gen8_isa.cpp \
    genx_simple_me_gen9_isa.cpp \
    genx_simple_me_gen11_isa.cpp \
    genx_simple_me_gen11lp_isa.cpp \
    genx_simple_me_gen12lp_isa.cpp \
    genx_histogram_gen9_isa.cpp \
    genx_histogram_gen11_isa.cpp \
    genx_histogram_gen11lp_isa.cpp \
    genx_histogram_gen12lp_isa.cpp)

MFX_LOCAL_SRC_FILES_HW += \
    mfx_lib/encode_hw/hevc/hevcehw_disp.cpp \
    mfx_lib/encode_hw/hevc/agnostic/hevcehw_base.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_impl.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_alloc.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_constraints.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_dirty_rect.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_dpb_report.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_encoded_frame_info.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_ext_brc.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_hdr_sei.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_hrd.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_interlace.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_legacy.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_legacy_defaults.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_max_frame_size.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_packer.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_parser.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_roi.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_task.cpp \
    mfx_lib/encode_hw/hevc/agnostic/base/hevcehw_base_weighted_prediction.cpp \
    mfx_lib/encode_hw/hevc/agnostic/g12/hevcehw_g12_caps.cpp \
    mfx_lib/encode_hw/hevc/agnostic/g12/hevcehw_g12_rext.cpp \
    mfx_lib/encode_hw/hevc/agnostic/g12/hevcehw_g12_scc.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_interlace_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_fei_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_dirty_rect_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_max_frame_size_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_roi_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_va_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_va_packer_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/base/hevcehw_base_weighted_prediction_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/g12/hevcehw_g12_lin.cpp \
    mfx_lib/encode_hw/hevc/linux/g12/hevcehw_g12_rext_lin.cpp \
    mfx_lib/encode_hw/shared/ehw_resources_pool.cpp \
    mfx_lib/encode_hw/shared/ehw_task_manager.cpp \
    mfx_lib/encode_hw/shared/ehw_device_vaapi.cpp \
    mfx_lib/encode_hw/shared/ehw_utils_vaapi.cpp

MFX_LOCAL_INCLUDES := \
    $(foreach dir, $(MFX_LOCAL_DIRS), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/include))

MFX_LOCAL_INCLUDES_IMPL := \
    $(MFX_LOCAL_INCLUDES) \
    $(foreach dir, $(MFX_LOCAL_DIRS_IMPL), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/include))

MFX_LOCAL_INCLUDES_HW := \
    $(MFX_LOCAL_INCLUDES_IMPL) \
    $(foreach dir, $(MFX_LOCAL_DIRS_HW), $(wildcard $(LOCAL_PATH)/mfx_lib/$(dir)/include)) \
    $(MFX_HOME)/_studio/mfx_lib/genx/h264_encode/isa \
    $(MFX_HOME)/_studio/mfx_lib/genx/field_copy/isa \
    $(MFX_HOME)/_studio/mfx_lib/genx/copy_kernels/isa \
    $(MFX_HOME)/_studio/mfx_lib/genx/mctf/isa \
    $(MFX_HOME)/_studio/mfx_lib/genx/asc/isa \
    $(MFX_HOME)/_studio/shared/asc/include \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/agnostic \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/agnostic/base \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/agnostic/g12 \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/linux \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/linux/base \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/hevc/linux/g12 \
    $(MFX_HOME)/_studio/mfx_lib/encode_hw/shared

MFX_LOCAL_STATIC_LIBRARIES_HW := \
    libmfx_lib_merged_hw \
    libumc_codecs_merged_hw \
    libumc_codecs_merged \
    libumc_io_merged_hw \
    libumc_core_merged_hw \
    libmfx_trace_hw \
    libasc

MFX_LOCAL_LDFLAGS_HW := \
    $(MFX_LDFLAGS) \
    -Wl,--version-script=$(LOCAL_PATH)/mfx_lib/libmfxhw.map

# =============================================================================

UMC_DIRS := \
    h264_enc \
    brc

UMC_DIRS_IMPL := \
    h265_dec h264_dec mpeg2_dec vc1_dec jpeg_dec vp9_dec \
    vc1_common jpeg_common

UMC_LOCAL_INCLUDES := \
    $(foreach dir, $(UMC_DIRS), $(wildcard $(MFX_HOME)/_studio/shared/umc/codec/$(dir)/include))

UMC_LOCAL_INCLUDES_IMPL := \
    $(UMC_LOCAL_INCLUDES) \
    $(foreach dir, $(UMC_DIRS_IMPL), $(wildcard $(MFX_HOME)/_studio/shared/umc/codec/$(dir)/include))

UMC_LOCAL_INCLUDES_HW := \
    $(UMC_LOCAL_INCLUDES_IMPL)

# =============================================================================

MFX_SHARED_FILES_IMPL := $(addprefix mfx_lib/shared/src/, \
    mfx_brc_common.cpp \
    mfx_common_int.cpp \
    mfx_enc_common.cpp \
    mfx_vc1_dec_common.cpp \
    mfx_vpx_dec_common.cpp \
    mfx_common_decode_int.cpp)

MFX_SHARED_FILES_HW := \
    $(MFX_SHARED_FILES_IMPL)

MFX_SHARED_FILES_HW += $(addprefix mfx_lib/shared/src/, \
    mfx_h264_enc_common_hw.cpp \
    mfx_h264_encode_vaapi.cpp \
    mfx_h264_encode_factory.cpp)

MFX_SHARED_FILES_HW += $(addprefix mfx_lib/genx/asc/isa/, \
    genx_scd_gen8_isa.cpp \
    genx_scd_gen9_isa.cpp \
    genx_scd_gen11_isa.cpp \
    genx_scd_gen11lp_isa.cpp \
    genx_scd_gen12lp_isa.cpp)

MFX_SHARED_FILES_HW += $(addprefix mfx_lib/genx/copy_kernels/isa/, \
    genx_copy_kernel_gen8_isa.cpp \
    genx_copy_kernel_gen9_isa.cpp \
    genx_copy_kernel_gen11_isa.cpp \
    genx_copy_kernel_gen11lp_isa.cpp \
    genx_copy_kernel_gen12lp_isa.cpp)

MFX_SHARED_FILES_HW += $(addprefix mfx_lib/genx/field_copy/isa/, \
    genx_fcopy_gen8_isa.cpp \
    genx_fcopy_gen9_isa.cpp \
    genx_fcopy_gen11_isa.cpp \
    genx_fcopy_gen11lp_isa.cpp \
    genx_fcopy_gen12lp_isa.cpp)

MFX_SHARED_FILES_HW += $(addprefix mfx_lib/genx/mctf/isa/, \
    genx_me_gen8_isa.cpp \
    genx_me_gen9_isa.cpp \
    genx_me_gen11_isa.cpp \
    genx_me_gen11lp_isa.cpp \
    genx_me_gen12lp_isa.cpp \
    genx_mc_gen8_isa.cpp \
    genx_mc_gen9_isa.cpp \
    genx_mc_gen11_isa.cpp \
    genx_mc_gen11lp_isa.cpp \
    genx_mc_gen12lp_isa.cpp \
    genx_sd_gen8_isa.cpp \
    genx_sd_gen9_isa.cpp \
    genx_sd_gen11_isa.cpp \
    genx_sd_gen11lp_isa.cpp \
    genx_sd_gen12lp_isa.cpp)

MFX_LIB_SHARED_FILES_1 := $(addprefix mfx_lib/shared/src/, \
    libmfxsw.cpp \
    libmfxsw_async.cpp \
    libmfxsw_decode.cpp \
    libmfxsw_enc.cpp \
    libmfxsw_encode.cpp \
    libmfxsw_pak.cpp \
    libmfxsw_plugin.cpp \
    libmfxsw_query.cpp \
    libmfxsw_session.cpp \
    libmfxsw_vpp.cpp \
    mfx_session.cpp \
    mfx_user_plugin.cpp \
    mfx_critical_error_handler.cpp)

MFX_LIB_SHARED_FILES_2 := $(addprefix shared/src/, \
    cm_mem_copy.cpp \
    fast_copy.cpp \
    fast_copy_c_impl.cpp \
    fast_copy_sse4_impl.cpp \
    mfx_vpp_vaapi.cpp \
    libmfx_allocator.cpp \
    libmfx_allocator_vaapi.cpp \
    libmfx_core.cpp \
    libmfx_core_hw.cpp \
    libmfx_core_factory.cpp \
    libmfx_core_vaapi.cpp \
    mfx_umc_alloc_wrapper.cpp \
    mfx_umc_mjpeg_vpp.cpp)

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := \
    $(MFX_LOCAL_SRC_FILES) \
    $(MFX_LOCAL_SRC_FILES_HW) \
    $(MFX_SHARED_FILES_HW)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES_HW) \
    $(UMC_LOCAL_INCLUDES_HW) \
    $(MFX_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror -Wno-unused-parameter
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_HEADER_LIBRARIES := libmfx_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfx_lib_merged_hw

include $(BUILD_STATIC_LIBRARY)

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LIB_SHARED_FILES_1) $(MFX_LIB_SHARED_FILES_2)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES_HW) \
    $(UMC_LOCAL_INCLUDES_HW) \
    $(MFX_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror -Wno-unused-parameter
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)

LOCAL_LDFLAGS := $(MFX_LOCAL_LDFLAGS_HW)

LOCAL_HEADER_LIBRARIES := libmfx_headers
LOCAL_WHOLE_STATIC_LIBRARIES := $(MFX_LOCAL_STATIC_LIBRARIES_HW)
LOCAL_SHARED_LIBRARIES := libva

ifeq ($(MFX_ENABLE_ITT_TRACES),true)
    LOCAL_WHOLE_STATIC_LIBRARIES += libittnotify
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfxhw32
LOCAL_MODULE_TARGET_ARCH := x86
LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LIB_SHARED_FILES_1) $(MFX_LIB_SHARED_FILES_2)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES_HW) \
    $(UMC_LOCAL_INCLUDES_HW) \
    $(MFX_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror -Wno-unused-parameter
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_LDFLAGS := $(MFX_LOCAL_LDFLAGS_HW)

LOCAL_HEADER_LIBRARIES := libmfx_headers
LOCAL_WHOLE_STATIC_LIBRARIES := $(MFX_LOCAL_STATIC_LIBRARIES_HW)
LOCAL_SHARED_LIBRARIES := libva

ifeq ($(MFX_ENABLE_ITT_TRACES),true)
    LOCAL_WHOLE_STATIC_LIBRARIES += libittnotify
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfxhw64
LOCAL_MODULE_TARGET_ARCH := x86_64
LOCAL_MULTILIB := 64

include $(BUILD_SHARED_LIBRARY)
