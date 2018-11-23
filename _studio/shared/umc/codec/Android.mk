LOCAL_PATH:= $(call my-dir)

# Setting subdirectories to march thru
MFX_LOCAL_DIRS := \
    brc \
    h264_enc \
    vc1_common \
    jpeg_common

MFX_LOCAL_DIRS_IMPL := \
    mpeg2_dec \
    h265_dec \
    h264_dec \
    vc1_dec \
    jpeg_dec \
    vp9_dec

MFX_LOCAL_SRC_FILES := \
  $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS), $(wildcard $(LOCAL_PATH)/$(dir)/src/*.cpp)))

MFX_LOCAL_SRC_FILES_IMPL := \
  $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS_IMPL), $(wildcard $(LOCAL_PATH)/$(dir)/src/*.cpp)))

MFX_LOCAL_INCLUDES := \
  $(foreach dir, $(MFX_LOCAL_DIRS) $(MFX_LOCAL_DIRS_IMPL), $(wildcard $(LOCAL_PATH)/$(dir)/include))

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LOCAL_SRC_FILES)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES) \
    $(MFX_INCLUDES_INTERNAL)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL) \
    -Wall -Werror
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_HEADER_LIBRARIES := libmfx_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libumc_codecs_merged

include $(BUILD_STATIC_LIBRARY)

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LOCAL_SRC_FILES_IMPL)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES) \
    $(MFX_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_HEADER_LIBRARIES := libmfx_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libumc_codecs_merged_hw

include $(BUILD_STATIC_LIBRARY)
