# Purpose:
#   Defines include paths, compilation flags, etc. to build Media SDK
# internal targets (libraries, test applications, etc.).
#
# Defined variables:
#   MFX_CFLAGS_INTERNAL - all flags needed to build MFX targets
#   MFX_CFLAGS_INTERNAL_HW - all flags needed to build MFX HW targets
#   MFX_INCLUDES_INTERNAL - all include paths needed to build MFX targets
#   MFX_INCLUDES_INTERNAL_HW - all include paths needed to build MFX HW targets

MFX_CFLAGS_INTERNAL := $(MFX_CFLAGS)
MFX_CFLAGS_INTERNAL_HW := $(MFX_CFLAGS_INTERNAL) -DMFX_VA
MFX_CFLAGS_INTERNAL_32 := -DLINUX32
MFX_CFLAGS_INTERNAL_64 := -DLINUX32 -DLINUX64

MFX_INCLUDES_INTERNAL :=  \
    $(MFX_INCLUDES) \
    $(MFX_HOME)/_studio/shared/include \
    $(MFX_HOME)/_studio/shared/umc/core/umc/include \
    $(MFX_HOME)/_studio/shared/umc/core/vm/include \
    $(MFX_HOME)/_studio/shared/umc/core/vm_plus/include \
    $(MFX_HOME)/_studio/shared/umc/io/umc_va/include \
    $(MFX_HOME)/_studio/mfx_lib/shared/include

MFX_INCLUDES_INTERNAL_HW := \
    $(MFX_INCLUDES_INTERNAL) \
    $(MFX_INCLUDES_LIBVA)
