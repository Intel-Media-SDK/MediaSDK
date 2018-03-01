ifneq ($(BOARD_HAVE_MEDIASDK_SRC),true)
ifeq ($(BOARD_HAVE_MEDIASDK_OPEN_SOURCE),true)
  MFX_HOME:= $(call my-dir)

  # Recursively call sub-folder Android.mk
  include $(call all-subdir-makefiles)
endif
endif
