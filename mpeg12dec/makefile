
include ../general/makefile.all
# this will set USE_AFSP and MPEG_2_5 from makefile.cfg

ifeq "$(MAKELEVEL)" "0"

OBJS_LIB  = decode.o huffman.o ieeefloat.o portableio.o tables.o decLayer123.o pcmBuffer.o bitBuffer.o frameData.o debug.o
OBJS_FDM  = frameDeMux.o frameData.o bitBuffer.o debug.o common.o
OBJS_BIN  = mpeg12dec.o fileWriter.o
OBJS_ALL  = $(OBJS_BIN) $(OBJS_LIB) $(OBJS_FDM)

TARGET_LIB     = libmpeg12dec.a
TARGET_FDM     = libmpeg12fdm.a
TARGET_BIN     = mpeg12dec
TARGET_ALL     = $(TARGET_BIN)
TARGET_CLEAN   = $(TARGET_BIN) $(TARGET_LIB)

SRCDIRS   = src
INCL      = -Iinclude
COMLIBS  += -lmpeg12dec -lmpeg12fdm

ifneq "$(USE_AFSP)" "0"
BIN_ADDONDIR += "afsp"
# done by general:
#DEFFLAGS     += -DUSELIB_AFsp
#LIB_PATH     += -L$(AFSP_LIBRARY_PATH)
#LIBS_BIN     += -ltsp
endif

ifneq "$(MPEG_2_5)" "0"
BIN_ADDONDIR	+= "mpeg25"
LIB_ADDONDIR    += "mpeg25"
FDM_ADDONDIR    += "mpeg25"
DEFFLAGS        += -DMPEG_2_5
endif

OPTIONS     = SRCDIRS="$(SRCDIRS)" LIBS="$(LIBS)" INCL="$(INCL)" DEFFLAGS="$(DEFFLAGS)" MPEG_2_5="$(MPEG_2_5)" USE_AFSP="$(USE_AFSP)"
OPTIONS_BIN = $(OPTIONS) ADDONDIR="$(BIN_ADDONDIR)" COMLIBS="$(COMLIBS)" \
                         COMLIBS_libmpeg12dec.a_OPTIONS="MPEG_2_5=$(MPEG_2_5)" \
                         COMLIBS_libmpeg12fdm.a_OPTIONS="MPEG_2_5=$(MPEG_2_5)" \
                         maplibmpeg12fdm.a="mpeg12dec"
OPTIONS_LIB = $(OPTIONS) ADDONDIR="$(LIB_ADDONDIR)"
OPTIONS_FDM = $(OPTIONS) ADDONDIR="$(FDM_ADDONDIR)"
.PHONY: all $(TARGET_ALL) clean

all: $(TARGET_ALL)

$(TARGET_BIN):
	@$(MAKE) TARGET=$@ $(OPTIONS_BIN) OBJS="$(OBJS_BIN)" binary

$(TARGET_FDM):
	@$(MAKE) TARGET=$@ $(OPTIONS_FDM) OBJS="$(OBJS_FDM)" library

$(TARGET_LIB):
	@$(MAKE) TARGET=$@ $(OPTIONS_LIB) OBJS="$(OBJS_LIB)" library

docs: src doc/Doxyfile
	@cd doc && doxygen


# end of old targets

clean:
	@$(MAKE) TARGET="$(TARGET_BIN)" ADDONDIR="$(ADDONDIR_BIN)" OBJS="$(OBJS_BIN)" clean_mode
	@$(MAKE) TARGET="$(TARGET_FDM)" ADDONDIR="$(ADDONDIR_FDM)" OBJS="$(OBJS_FDM)" clean_mode
	@$(MAKE) TARGET="$(TARGET_LIB)" ADDONDIR="$(ADDONDIR_LIB)" OBJS="$(OBJS_LIB)" clean_mode
	@cd doc && rm -rf html

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE) TARGET="$(TARGET)" ADDONDIR="$(ADDONDIR)" OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"

