LOCAL_PATH:= $(call my-dir)

MFX_LOCAL_DIRS_HW := \
    umc_va

MFX_LOCAL_SRC_FILES_HW := \
  $(patsubst $(LOCAL_PATH)/%, %, $(foreach dir, $(MFX_LOCAL_DIRS_HW), $(wildcard $(LOCAL_PATH)/$(dir)/src/*.cpp)))

MFX_LOCAL_INCLUDES_HW := \
  $(foreach dir, $(MFX_LOCAL_DIRS_HW), $(wildcard $(LOCAL_PATH)/$(dir)/include))

# =============================================================================

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(MFX_LOCAL_SRC_FILES_HW)

LOCAL_C_INCLUDES := \
    $(MFX_LOCAL_INCLUDES_HW) \
    $(MFX_INCLUDES_INTERNAL_HW)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_HEADER_LIBRARIES := libmfx_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libumc_io_merged_hw

include $(BUILD_STATIC_LIBRARY)
