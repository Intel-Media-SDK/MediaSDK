LOCAL_PATH:= $(call my-dir)

include $(MFX_HOME)/mdp_msdk-lib/android/mfx_env.mk

include $(CLEAR_VARS)
include $(MFX_HOME)/mdp_msdk-lib/android/mfx_defs.mk

LOCAL_SRC_FILES := \
    $(addprefix safeclib/, $(notdir $(wildcard $(LOCAL_PATH)/safeclib/*.c)))

LOCAL_C_INCLUDES := \
    $(MFX_C_INCLUDES)

LOCAL_CFLAGS := \
    $(MFX_CFLAGS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsafec

include $(BUILD_STATIC_LIBRARY)
