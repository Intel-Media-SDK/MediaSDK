LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_C_INCLUDES_INTERNAL_HW) \
    $(MFX_HOME)/_studio/mfx_lib/cmrt_cross_platform/include

LOCAL_CFLAGS := $(MFX_CFLAGS_INTERNAL_HW)
LOCAL_CFLAGS_32 := $(MFX_CFLAGS_INTERNAL_32)
LOCAL_CFLAGS_64 := $(MFX_CFLAGS_INTERNAL_64)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libasc

include $(BUILD_STATIC_LIBRARY)