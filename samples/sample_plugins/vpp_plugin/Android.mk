LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_HOME)/android/mfx_defs.mk

LOCAL_SRC_FILES := $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_INCLUDES) \
    $(MFX_HOME)/samples/sample_common/include \
    $(MFX_HOME)/samples/sample_plugins/vpp_plugins/include

LOCAL_CFLAGS := \
    $(MFX_CFLAGS) \
    $(MFX_CFLAGS_LIBVA)

LOCAL_HEADER_LIBRARIES := libmfx_headers
LOCAL_STATIC_LIBRARIES := libsample_common

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsample_vpp_plugin

include $(BUILD_STATIC_LIBRARY)
