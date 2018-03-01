# Purpose:
#   Defines include paths, compilation flags, etc. to build Media SDK targets.
#
# Defined variables:
#   MFX_CFLAGS - common flags for all targets
#   MFX_C_INCLUDES - common include paths for all targets
#   MFX_LDFLAGS - common link flags for all targets

# =============================================================================
# Common definitions

MFX_CFLAGS := -DANDROID

# Android version preference:
MFX_ANDROID_VERSION:= MFX_O

# Passing Android-dependency information to the code
MFX_CFLAGS += \
  -DMFX_ANDROID_VERSION=$(MFX_ANDROID_VERSION) \
  -include $(MFX_HOME)/android/include/mfx_config.h

# Setting usual paths to include files
MFX_C_INCLUDES := \
  $(LOCAL_PATH)/include \
  $(MFX_HOME)/api/include \
  $(MFX_HOME)/android/include

# Setting usual link flags
MFX_LDFLAGS := \
  -z noexecstack \
  -z relro -z now

#  Security
MFX_CFLAGS += \
  -fstack-protector \
  -fPIE -fPIC -pie \
  -O2 -D_FORTIFY_SOURCE=2 \
  -Wformat -Wformat-security

# Setting vendor
LOCAL_MODULE_OWNER := intel

# Moving executables to proprietary location
LOCAL_PROPRIETARY_MODULE := true

# =============================================================================

# Android OS specifics
include $(MFX_HOME)/android/build/mfx_android_os.mk

# STL support definitions
include $(MFX_HOME)/android/build/mfx_stl.mk

# Definitions specific to Media SDK internal things
include $(MFX_HOME)/android/build/mfx_defs_internal.mk
