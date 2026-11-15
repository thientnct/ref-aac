#-*-makefile-*-
#ifdef MPEG4V1
# Usage: gmake [<general options>] [AAC_PP=0] [SBR=0] [MPEG2=1] [PARAMETRICSTEREO=0]
#else
# USAGE: gmake [<general options>] [AAC_PP=0] [SBR=0] [PARAMETRICSTEREO=0]
#endif

include ../general/makefile.cfg
-include ../general/makefile.cfg-local
-include makefile.cfg
-include makefile.cfg-local

TOTAL_PATH_ADD =

#ifdef CT_SBR  /* strip-to-mpeg2 needed this comment */
ifndef SBR
SBR=1
endif

ifneq "$(SBR)" "0"
TOTAL_PATH_ADD += sbr
endif

ifndef PARAMETRICSTEREO
PARAMETRICSTEREO=1
endif

ifneq "$(PARAMETRICSTEREO)" "0"
TOTAL_PATH_ADD += ps
endif
#endif          /* strip-to-mpeg2 needed this comment */

ifneq  "$(MAKELEVEL)" "0"
override ADDONDIR = $(TOTAL_PATH_ADD)
endif

# -include ../general/makefile.all spread in file
include ../general/makefile.platform
include ../general/makefile.rules

ifeq "$(MAKELEVEL)" "0"

OBJS_AAC_QC  = aac_qc.o aac_se_enc.o aac_back_pred.o tns.o mc_enc.o ms.o is.o drc.o
OBJS_PSYCH   = psych.o
OBJS_AAC_ENC = util.o transfo.o imdct.o

# Some compile flag definitions:
# -DMPEG4V1 : include MPEG4 AAC extensions.
# -DSRS     : include gain control.
# -DPULSE   : include pulse coding in encoder.
override DEFFLAGS += -DSRS -DDOUBLE_WIN -DDOLBY_MDCT -DOLD_PRED_PARAMS -DPLOTMTV

ifndef AAC_PP
AAC_PP = 1
endif
ifeq "$(AAC_PP)" "1"
override DEFFLAGS += -DSON_AAC_PP
OBJS_SON_AACPP_ENC = son_gc_common.o son_gc_detecter.o son_gc_etc_enc.o son_pqf.o son_gc_modifier.o son_gc_pack.o son_pqf_common.o son_gc_detectreset.o
endif

#ifdef MPEG4V1  /* strip-to-mpeg2 needs this comment */
ifndef MPEG2
MPEG2=0
endif
ifeq "$(MPEG2)" "0"
override DEFFLAGS   += -DMPEG4V1
OBJS_LTP = nok_ltp_enc.o nok_pitch.o
endif
#endif          /* strip-to-mpeg2 needs this comment */
#ifdef CT_SBR   /* strip-to-mpeg2 needs this comment */
ifeq "$(SBR)" "1"
override DEFFLAGS += -DCT_SBR
OBJS_CT_SBR = ct_sbr.o ct_polyphase.o
ifeq "$(PARAMETRICSTEREO)" "1"
override DEFFLAGS += -DPARAMETRICSTEREO
OBJS_CT_SBR += ct_psenc.o
endif
endif
#endif          /* strip-to-mpeg2 needs this comment */

OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o $(OBJS_T2F_ENC) $(OBJS_AAC_QC) $(OBJS_LTP) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC) $(OBJS_MOT_QC) $(OBJS_CT_SBR)
LIBS_ENC_TF = $(LIBS_T2F_ENC) $(LIBS_VQ) $(LIBS_SON_AACPP_ENC)

OBJS_COM = common.o cmdline.o bitstream.o audio.o austream.o plotmtv_interf.o
OBJS_ENC = encoder.o $(OBJS_COM) $(OBJS_ENC_PAR) $(OBJS_ENC_LPC) $(OBJS_ENC_TF)
LIBS_ENC = $(LIBS_ENC_PAR) $(LIBS_ENC_LPC) $(LIBS_ENC_TF)

SRCDIRS = src
OPTIONS = $(MFLAGS) SRCDIRS="$(SRCDIRS)" LIBS="$(LIBS_ENC)" DEFFLAGS="$(DEFFLAGS)" INCLDEP=1

.PHONY: all enc clean

all: enc

enc:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS_ENC)" ENCODE=1 TARGET=$@ binary

clean:
	@$(MAKE)            OBJS="$(OBJS_ENC)" ENCODE=1 TARGET=enc $@

endif # ifeq "$(MAKELEVEL)" "0"
