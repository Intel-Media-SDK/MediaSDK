# Purpose:
#   Defines include paths, compilation flags, etc. to build Media SDK targets.
#
# Defined variables:
#   MFX_CFLAGS - common flags for all targets
#   MFX_INCLUDES - common include paths for all targets
#   MFX_INCLUDES_LIBVA - include paths to LibVA headers
#   MFX_LDFLAGS - common link flags for all targets

# =============================================================================
# Common definitions

MFX_CFLAGS := -DANDROID

# Android version preference:
ifneq ($(filter 9 9.% P ,$(PLATFORM_VERSION)),)
  MFX_ANDROID_VERSION:= MFX_P
endif
ifneq ($(filter 8.% O ,$(PLATFORM_VERSION)),)
  ifneq ($(filter 8.0.%,$(PLATFORM_VERSION)),)
    MFX_ANDROID_VERSION:= MFX_O
  else
    MFX_ANDROID_VERSION:= MFX_O_MR1
  endif
endif

# Passing Android-dependency information to the code
MFX_CFLAGS += \
  -DMFX_ANDROID_VERSION=$(MFX_ANDROID_VERSION) \
  -include mfx_android_config.h

# Setting version information for the binaries
ifeq ($(MFX_VERSION),)
  MFX_VERSION = "6.0.010"
endif

MFX_CFLAGS += \
  -DMFX_FILE_VERSION=\"`echo $(MFX_VERSION) | cut -f 1 -d.``date +.%-y.%-m.%-d`\" \
  -DMFX_PRODUCT_VERSION=\"$(MFX_VERSION)\"

#  Security
MFX_CFLAGS += \
  -fstack-protector \
  -fPIE -fPIC -pie \
  -O2 -D_FORTIFY_SOURCE=2 \
  -Wformat -Wformat-security \
  -fexceptions -frtti

ifeq ($(MFX_ENABLE_ITT_TRACES),)
  # Enabled ITT traces by default
  MFX_ENABLE_ITT_TRACES := true
endif

ifeq ($(MFX_ENABLE_ITT_TRACES),true)
  MFX_CFLAGS += -DMFX_TRACE_ENABLE_ITT
endif

# LibVA support.
MFX_CFLAGS_LIBVA := -DLIBVA_SUPPORT -DLIBVA_ANDROID_SUPPORT

# Setting usual paths to include files
MFX_INCLUDES := \
  $(LOCAL_PATH)/include \
  $(MFX_HOME)/api/include \
  $(MFX_HOME)/android/include

MFX_INCLUDES_LIBVA := $(TARGET_OUT_HEADERS)/libva

# Setting usual link flags
MFX_LDFLAGS := \
  -z noexecstack \
  -z relro -z now

# Setting vendor
LOCAL_MODULE_OWNER := intel

# Moving executables to proprietary location
LOCAL_PROPRIETARY_MODULE := true

# =============================================================================

# Definitions specific to Media SDK internal things
include $(MFX_HOME)/android/mfx_defs_internal.mk
