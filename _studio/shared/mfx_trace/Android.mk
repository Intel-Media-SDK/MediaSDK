LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_INCLUDES_INTERNAL_HW) \
    $(MFX_HOME)/api/mediasdk_structures

LOCAL_CFLAGS := \
    $(MFX_CFLAGS_INTERNAL_HW) \
    -Wall -Werror
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_HEADER_LIBRARIES := libmfx_headers

ifeq ($(MFX_ENABLE_ITT_TRACES),true)
  LOCAL_STATIC_LIBRARIES += libittnotify
  LOCAL_HEADER_LIBRARIES += llibittnotify_headers
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfx_trace_hw

include $(BUILD_STATIC_LIBRARY)
