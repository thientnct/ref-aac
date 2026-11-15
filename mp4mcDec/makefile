#-*-makefile-*-
#ifdef MPEG4V1
# Usage: gmake [<general options>] [USE_AFSP=0] [ENABLE_RTPLAY=1] [ISOMP4=0] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [SBR=0] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
#else
# Usage: gmake [<general options>] [USE_AFSP=0] [ENABLE_RTPLAY=1] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [SBR=0] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
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

ifneq "$(ISOMP4)" "0"
TOTAL_PATH_ADD += isomp4
endif
#endif          /* strip-to-mpeg2 needs this comment */

#ifdef CT_SBR  /* strip-to-mpeg2 needed this comment */
ifndef SBR
SBR=1
endif

ifneq "$(SBR)" "0"
TOTAL_PATH_ADD += sbr
endif

ifndef LOW_POWER_SBR
LOW_POWER_SBR=0
endif

ifneq "$(LOW_POWER_SBR)" "0"
TOTAL_PATH_ADD += lp
# PS is not permitted in the case of LP (sps@2008-02-28)
override PARAMETRICSTEREO=0
endif

ifndef PARAMETRICSTEREO
PARAMETRICSTEREO=1
endif

ifneq "$(PARAMETRICSTEREO)" "0"
TOTAL_PATH_ADD += ps
endif
#endif          /* strip-to-mpeg2 needed this comment */

ifndef USE_AFSP
USE_AFSP = 1
endif

ifneq "$(USE_AFSP)" "0"
TOTAL_PATH_ADD += AFsp
endif

ifndef ENABLE_RTPLAY
ENABLE_RTPLAY = 0
endif

ifneq "$(ENABLE_RTPLAY)" "0"
USE_AFSP = 0
TOTAL_PATH_ADD += RTPlay
endif

ifneq  "$(MAKELEVEL)" "0"

override ADDONDIR = $(TOTAL_PATH_ADD)
ifneq "$(findstring libisoaac.a,$(TARGET))" ""
ADDONOBJDIR = _aac
else
ifneq "$(findstring aacdec_mc,$(TARGET))" ""
ADDONOBJDIR = _aac
override MODULELIBS += -lisoaac
else
ifneq "$(findstring adif2mp4,$(TARGET))" ""
ADDONOBJDIR = _conv
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifneq "$(ISOMP4)" "0"
override MODULELIBS += -lisoaac_mp4
override DEFFLAGS += -DMPEG4V1
else
#endif          /* strip-to-mpeg2 needs this comment */
override MODULELIBS += -lisoaac
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
endif
else
ADDONOBJDIR = _mp4
override DEFFLAGS += -DMPEG4V1
ifneq "$(findstring mp4audec_mc,$(TARGET))" ""
override MODULELIBS += -lisoaac_mp4
endif
#endif          /* strip-to-mpeg2 needs this comment */
endif
endif
endif

endif # ifneq  "$(MAKELEVEL)" "0"

# -include ../general/makefile.all spread in file
include ../general/makefile.platform
include ../general/makefile.rules

ifeq "$(MAKELEVEL)" "0"

# Common
OBJS = bitinput.o config.o coupling.o diagnose.o drc.o \
       huffdec1.o huffdec2.o huffdec3.o huffinit.o hufftables.o \
       intensity.o intrins.o monopred.o stereo.o tns.o

ifeq "$(ENABLE_RTPLAY)" "0"
ifeq "$(USE_AFSP)" "0"
OBJS += aiff_support.o
endif
endif

# + DOLBY imdct
OBJS += adifdec.o block.o dolby_adapt.o transfo.o

# + SONY gain control
OBJS += gc_unpac.o gc_compensate.o gc_common.o \
        gc_pqf_common.o gc_ipqf.o gc.o gc_mdct_common.o gc_imdct.o


#ifdef CT_SBR  /* strip-to-mpeg2 needed this comment */
# + Coding Technologies SBR
ifeq "$(SBR)" "1"
OBJS += ct_envcalc.o ct_envdec.o ct_envextr.o ct_freqsca.o ct_hfgen.o \
        ct_sbrbitb.o ct_sbrcrc.o ct_sbrdec.o ct_sbrdecoder.o ct_polyphase.o
ifeq "$(PARAMETRICSTEREO)" "1"
OBJS += ct_psdec.o   # Parametric Stereo
endif
endif
#endif          /* strip-to-mpeg2 needed this comment */


OBJS_AACDEC    = decoder.o audioout.o
OBJS_LIB       = $(OBJS)
AAC_OBJS_ALL   = $(OBJS_LIB) $(OBJS_AACDEC)

OBJS_ADIF2MP4  = adif2mp4.o bitoutput.o
CONV_OBJS_ALL  = $(OBJS_ADIF2MP4)

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
# additional files for MPEG4 only
# + LTP + PNS
MP4_OBJS_LIB   = nok_lt_prediction.o pns.o
OBJS_MP4AUDEC  = $(OBJS_AACDEC)
MP4_OBJS_ALL   = $(OBJS_LIB) $(MP4_OBJS_LIB) $(OBJS_MP4AUDEC)
#endif          /* strip-to-mpeg2 needs this comment */

# -DBIGDECODER
#  Enabling this option builds an AAC Decoder that can handle up
#  to 48 channels of audio. This is required for stream L8, S15, S16.

override DEFFLAGS = -DNEW_ADTS -DWRITE_NEW_ADTS -DDRC -DBIGDECODER

#ifdef CT_SBR  /* strip-to-mpeg2 needed this comment */
ifeq "$(SBR)" "1"
override DEFFLAGS += -DCT_SBR
ifneq "$(LOW_POWER_SBR)" "0"
override DEFFLAGS += -DLOW_POWER_SBR
endif
ifneq "$(PARAMETRICSTEREO)" "0"
override DEFFLAGS += -DPARAMETRICSTEREO
endif
endif
#endif          /* strip-to-mpeg2 needed this comment */

ifneq "$(RESET_TEST)" "0" 
override DEFFLAGS += -DLAST_RESET_TEST
endif

ifneq "$(TORTURE_TEST)" "0" 
override DEFFLAGS += -DARITHMETIC_TORTURE_TEST
endif

ifeq "$(PNS_NONDETERMINISTIC_RANDOM)" "1" 
override DEFFLAGS += -DPNS_NONDETERMINISTIC_RANDOM
endif

ifneq "$(ENABLE_RTPLAY)" "0"
override DEFFLAGS += -DENABLE_RTPLAY
EXTERNLIBS  += -laudio
endif

ALL_BIN   = aacdec_mc adif2mp4
AAC_LIBS  = libisoaac.a
CONV_LIBS = $(AAC_LIBS)

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifeq "$(ISOMP4)" "1"
ALL_BIN  += mp4audec_mc
MP4_LIBS  = libisoaac_mp4.a
override CONV_LIBS = $(MP4_LIBS)
endif
#endif          /* strip-to-mpeg2 needs this comment */

SRCDIRS = src
OPTIONS = $(MFLAGS) SRCDIRS="$(SRCDIRS)" USE_AFSP=$(USE_AFSP) COMLIBS="$(COMLIBS)" EXTERNLIBS="$(EXTERNLIBS)" DEFFLAGS="$(DEFFLAGS)" LIBS="$(LIBS)" INCL="$(INCL)" INCLDEP=1

.PHONY: all allafsp $(ALL_BIN) $(MP4_LIBS) $(AAC_LIBS) clean cleanafsp

all: $(ALL_BIN)

allafsp:
	$(MAKE) USE_AFSP=0 all
	$(MAKE) USE_AFSP=1 all

aacdec_mc: $(AAC_LIBS)
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_AACDEC)" binary

adif2mp4: $(CONV_LIBS)
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_ADIF2MP4)" binary

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifeq "$(ISOMP4)" "1"

mp4audec_mc: $(MP4_LIBS)
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_MP4AUDEC)" binary

libisoaac_mp4.a:
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LIB) $(MP4_OBJS_LIB)" library

endif
#endif          /* strip-to-mpeg2 needs this comment */

libisoaac.a:
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LIB)" library

clean:
	@$(MAKE) TARGET="aacdec_mc libisoaac.a"       OBJS="$(AAC_OBJS_ALL)"  clean_mode
	@$(MAKE) TARGET="adif2mp4"                    OBJS="$(CONV_OBJS_ALL)" clean_mode
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
	@$(MAKE) TARGET="mp4audec_mc libisoaac_mp4.a" OBJS="$(MP4_OBJS_ALL)"  clean_mode
#endif          /* strip-to-mpeg2 needs this comment */

cleanafsp:
	@$(MAKE) USE_AFSP=0 TARGET="aacdec_mc libisoaac.a"       OBJS="$(AAC_OBJS_ALL)"  clean_mode
	@$(MAKE) USE_AFSP=0 TARGET="adif2mp4"                    OBJS="$(CONV_OBJS_ALL)" clean_mode
#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
	@$(MAKE) USE_AFSP=0 TARGET="mp4audec_mc libisoaac_mp4.a" OBJS="$(MP4_OBJS_ALL)"  clean_mode
	@$(MAKE) USE_AFSP=1 TARGET="mp4audec_mc libisoaac_mp4.a" OBJS="$(MP4_OBJS_ALL)"  clean_mode
#endif          /* strip-to-mpeg2 needs this comment */
	@$(MAKE) USE_AFSP=1 TARGET="adif2mp4"                    OBJS="$(CONV_OBJS_ALL)" clean_mode
	@$(MAKE) USE_AFSP=1 TARGET="aacdec_mc libisoaac.a"       OBJS="$(AAC_OBJS_ALL)"  clean_mode

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE) USE_AFSP=$(USE_AFSP) TARGET="$(TARGET)" OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"
