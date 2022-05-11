#
#Component Makefile
#

COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive
COMPONENT_EMBED_TXTFILES += gs-16b-1c-44100hz.mp3
