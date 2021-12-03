
ifdef CONFIG_AUDIO_HELIX
CFLAGS += -DARM -DCONFIG_AUDIO_HELIX
COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS:=src
./src/subband.o ./src/scalfact.o ./src/dqchan.o ./src/huffman.o: CFLAGS += -Wno-unused-but-set-variable
endif
