LOCAL_PATH:= $(call my-dir)

MFX_LOCAL_SRC_FILES := $(addprefix h265/src/, \
    export.cpp \
    mfx_h265_encode_hw.cpp \
    mfx_h265_encode_hw_brc.cpp \
    mfx_h265_encode_hw_bs.cpp \
    mfx_h265_encode_hw_ddi.cpp \
    mfx_h265_encode_hw_par.cpp \
    mfx_h265_encode_hw_utils.cpp \
    mfx_h265_encode_vaapi.cpp)

MFX_LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/h265/include \
    $(MFX_HOME)/_studio/shared/umc/codec/brc/include

MFX_LOCAL_STATIC_LIBRARIES := \
    libmfx_trace_hw \
    libmfx_lib_merged_hw \
    libumc_core_merged_hw \
    libsafec

MFX_LOCAL_LDFLAGS := \
    $(MFX_LDFLAGS) \
    -Wl,--version-script=$(MFX_HOME)/_studio/mfx_lib/plugin/libmfxsw_plugin.map

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LOCAL_SRC_FILES)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_C_INCLUDES) \
    $(MFX_C_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL) \
    -DMFX_ENABLE_H265_VIDEO_ENCODE
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)

LOCAL_STATIC_LIBRARIES := $(MFX_LOCAL_STATIC_LIBRARIES)

LOCAL_LDFLAGS := $(MFX_LOCAL_LDFLAGS)

LOCAL_SHARED_LIBRARIES := libva

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libmfx_hevce_hw32
LOCAL_MODULE_TARGET_ARCH := x86
LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LOCAL_SRC_FILES)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_C_INCLUDES) \
    $(MFX_C_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL) \
    -DMFX_ENABLE_H265_VIDEO_ENCODE
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_STATIC_LIBRARIES := $(MFX_LOCAL_STATIC_LIBRARIES)

LOCAL_LDFLAGS := $(MFX_LOCAL_LDFLAGS)

LOCAL_SHARED_LIBRARIES := libva

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libmfx_hevce_hw64
LOCAL_MODULE_TARGET_ARCH := x86_64
LOCAL_MULTILIB := 64

include $(BUILD_SHARED_LIBRARY)
