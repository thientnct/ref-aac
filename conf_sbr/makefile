#-*-makefile-*-
#ifdef MPEG4V1
# Usage: gmake [<general options>] [USE_AFSP=0] [ISOMP4=0] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
#else
# Usage: gmake [<general options>] [USE_AFSP=0] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
#endif

include ../general/makefile.cfg
-include ../general/makefile.cfg-local
-include makefile.cfg
-include makefile.cfg-local

TOTAL_PATH_ADD =

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifndef ISOMP4
ISOMP4 = 1
endif

ifeq "$(ISOMP4)" "1"
TOTAL_PATH_ADD += isomp4
endif
#endif          /* strip-to-mpeg2 needs this comment */

TOTAL_PATH_ADD += sbr

ifndef LOW_POWER_SBR
LOW_POWER_SBR=0
endif

ifeq "$(LOW_POWER_SBR)" "1"
TOTAL_PATH_ADD += lp
# PS is not permitted in the case of LP (sps@2008-02-28)
override PARAMETRICSTEREO=0
endif

ifndef PARAMETRICSTEREO
PARAMETRICSTEREO=1
endif

ifeq "$(PARAMETRICSTEREO)" "1"
TOTAL_PATH_ADD += ps
endif

ifndef USE_AFSP
USE_AFSP = 1
endif

ifeq "$(USE_AFSP)" "1"
TOTAL_PATH_ADD += AFsp
endif

ifneq  "$(MAKELEVEL)" "0"

override ADDONDIR = $(TOTAL_PATH_ADD)
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifeq "$(ISOMP4)" "1"
ADDONOBJDIR = _mp4
override DEFFLAGS += -DMPEG4V1
else
#endif          /* strip-to-mpeg2 needs this comment */
ADDONOBJDIR = _aac
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
endif
#endif          /* strip-to-mpeg2 needs this comment */

endif # ifneq  "$(MAKELEVEL)" "0"

# -include ../general/makefile.all spread in file
include ../general/makefile.platform
include ../general/makefile.rules


SBR_CONF_OBJS = ct_sbrConfToolDownSampler.o \
                ct_sbrdecoderConfTool.o \
                decoderConfTool.o \
                audiofile_io.o

override DEFFLAGS += -DCT_SBR
ifeq "$(LOW_POWER_SBR)" "1"
override DEFFLAGS += -DLOW_POWER_SBR
endif
ifeq "$(PARAMETRICSTEREO)" "1"
override DEFFLAGS += -DPARAMETRICSTEREO
endif

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifeq "$(ISOMP4)" "1"
COMLIBS_isoaac = -lisoaac_mp4
else
#endif
COMLIBS_isoaac = -lisoaac
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
endif
#endif

SRCDIRS = src

OPTIONS = $(MFLAGS) SRCDIRS="$(SRCDIRS)" USE_AFSP=$(USE_AFSP) COMLIBS="$(COMLIBS)" EXTERNLIBS="$(EXTERNLIBS)" DEFFLAGS="$(DEFFLAGS)" LIBS="$(LIBS)" INCL="$(INCL)" INCLDEP=1

.PHONY: all allafsp sbr_conftool sbr_conftool_lp clean

all: sbr_conftool

sbr_conftool:
	@$(MAKE) $(OPTIONS) TARGET=$@ COMLIBS="$(COMLIBS_isoaac)" OBJS="$(SBR_CONF_OBJS)" binary

ifeq "$(MAKELEVEL)" "0"

clean:
	@$(MAKE) TARGET="sbr_conftool"      OBJS="$(SBR_CONF_OBJS)" clean_mode

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE) USE_AFSP=$(USE_AFSP) TARGET="$(TARGET)" OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"
