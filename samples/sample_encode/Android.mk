LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_INCLUDES) \
    $(MFX_INCLUDES_LIBVA) \
    $(MFX_HOME)/samples/sample_common/include \
    $(MFX_HOME)/samples/sample_plugins/rotate_cpu/include

LOCAL_CFLAGS := \
    $(MFX_CFLAGS) \
    $(MFX_CFLAGS_LIBVA)

LOCAL_STATIC_LIBRARIES := libsample_common libmfx
LOCAL_SHARED_LIBRARIES := libva libva-android

LOCAL_MULTILIB := both
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sample_encode
LOCAL_MODULE_STEM_32 := sample_encode32
LOCAL_MODULE_STEM_64 := sample_encode64

include $(BUILD_EXECUTABLE)
