LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := \
    $(addprefix safeclib/, $(notdir $(wildcard $(LOCAL_PATH)/safeclib/*.c)))

LOCAL_C_INCLUDES := \
    $(MFX_C_INCLUDES)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsafec

include $(BUILD_STATIC_LIBRARY)
