#
# Component Makefile
#
# This Makefile should, at the very least, just include $(SDK_PATH)/make/component.mk. By default,
# this will take the sources in this directory, compile them and link them into
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the SDK documents if you need to do this.
#

ifdef CONFIG_DSP_CHIP_IS_MICROSEMI
COMPONENT_ADD_INCLUDEDIRS := .  Codec/test Codec Board DSP/zl38063

COMPONENT_SRCDIRS :=  .  Codec/test Codec Board DSP/zl38063
endif
ifdef CONFIG_DSP_CHIP_IS_IM501
COMPONENT_ADD_INCLUDEDIRS := .  Codec/test Codec Board DSP/im501

COMPONENT_SRCDIRS :=  .  Codec/test Codec Board DSP/im501
endif
ifdef CONFIG_DSP_CHIP_IS_NONE
COMPONENT_ADD_INCLUDEDIRS := .  Codec/test Codec Board

COMPONENT_SRCDIRS :=  .  Codec/test Codec Board
endif