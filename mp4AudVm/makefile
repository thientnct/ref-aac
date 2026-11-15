#-*-makefile-*-
#------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile (requires GNU make)
#
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
# PL    Pierre Lauber, FhG/IIS <lau@iis.fhg.de>
# SE	Sebastien Etienne, CCETT Rennes
# OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>
#
# Changes:
# 11-jun-96   BG    basic concept
# 13-jun-96   HP    first version
# 14-jun-96   HP    test stuff added
# 19-jun-96   HP    indiline.a added
# 20-jun-96   HP    modifications proposed by BG
# 28-jun-96   HP    fixed COPTS bug
# 04-jul-96   HP    restructured makefile, fixed CFLAGS/LIBS bug
# 04-jul-96   HP    included "pre-linked object file directory" by BG
# 05-jul-96   HP    fixed override CFLAGS/LIBS
# 20-aug-96   BG    OS2 modifications
# 26-aug-96   HP    removed PLOBJS, adapted to vm directory structure
# 26-aug-96   HP    CVS
# 13-sep-96   HP    removed -O option for Linux g++
# 25-oct-96   HP    adapted makefile options / added makefile.cfg
# 01-nov-96   HP    added "depend" target
# 03-dec-96   HP    added comment re. include dependency files
#                   added library dependency rules
# 21-jan-97   HP    added audio i/o module
# 31-jan-97   HP    seperated encoder and  -g directories
# 05-feb-97   HP    added COMLIBS
# 12-feb-97   HP    make CFLAGS and LDFLAGS values passed on and evaluated
# 14-feb-97   HP    included PICOLA speed control (by Panasonic)
# 11-mar-97   HP    included PICOLA source code
# 21-mar-97   BT    fixed CC and IRIX things, platform autodetect
# 26-mar-97   CCETT included G729  -g source code
# 20-may-97   HP    clean up
# 22-may-97   HP    added DEBUGPLOT (substitutes PLOTMTV)
# 03-jun-97   HP    disabled DEBUGPLOT as default
# 07-nov-97   HP    merged with FhG mods
# 08-apr-98   HP    added OSF platform
# 09-apr-98   HP/CF added rm before ar when making libs
# 13-oct-98   HP    improved platform detection
# 02-dec-98   HP/nn merged version-dependent directory stuff by FhG
# 11-jan-99   HP    linux_gcc
# 20-jan-99   HP    cleaned up platform dependency stuff and CFLAGS defaults
# 21-jan-99   HP    audio.c & austream.c
# 22-jan-99   HP    USE_AFSP
# 23-apr-99   HP    updated, some files renamed for IM1-compability
# 28-apr-99   HP    using AFSP_INCLUDE_PATH / AFSP_LIBRARY_PATH ...
# 21-mar-00   HP    new default for ISOMP4_PATH
# 27-oct-00   HP    changed CORRIGENDUM1 = 1 to CORRIGENDUM = 1
# 23-jan-01   PL    clean up of refSoft makefiles
# 19-jun-01   OK    removed G723/G729 dummy files
# 2002-11-25  HP    removed mp4audec from list of default targets
# 2003-03-26  OK    removed file format converters from list of default targets
# 22-jul-03   AR/RJ add extension 2 parametric coding
# 2003-08-27  HP    trying to disable SSC by default
#                   ARGH ... SSC=0 was not tested and hence does not work
# 14-apr-05   AR    Fixed SSC=0 problem
#----------------------------------------------------------------------

ifndef ISOMP4
ISOMP4 = 1
endif
ifndef USE_AFSP
USE_AFSP = 1
endif

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

ifndef SSC
SSC = 1
endif
ifeq "$(SSC)" "1"
DEFFLAGS += -DEXT2PAR
endif

ifndef VERSION
VERSION = 2
endif
ifeq "$(VERSION)" "2"
override DEFFLAGS += -DVERSION2
endif

# CELP Error Concealment
ifeq "$(CELP_EC)" "1"
override DEFFLAGS += -DVERSION2_EC
endif

ifneq "$(NO_AL_CONFIG)" "1"
override DEFFLAGS += -DAL_CONFIG
endif

# 14496-3 COR1
ifndef CORRIGENDUM
CORRIGENDUM = 1
endif
ifeq "$(CORRIGENDUM)" "1"
override DEFFLAGS += -DCORRIGENDUM1
endif

# error sensitivity category(ESC) support
ifndef ESC_SUPPORT
ESC_SUPPORT = 1
endif
ifeq "$(ESC_SUPPORT)" "1"
override DEFFLAGS += -DESC_SUPPORT
endif

# HP 991119
ifeq "$(MOUSECHANGE)" "1"
override DEFFLAGS += -DMOUSECHANGE
LIBPATH  += -L/usr/X11R6/lib
SYSLIBS  += -lX11
ifeq "$(OSTYPE)" "solaris"
INCL    += -I/sol/X11R6/Global/include
LIBPATH += -L/sol/X11R6/Global/lib
SYSLIBS += -lsocket
endif
endif

OBJS_ENC_PAR   = enc_par_dmy.o
OBJS_ENC_LPC   = enc_lpc_dmy.o
OBJS_ENC_TF    = enc_tf_dmy.o
OBJS_ENC_SSC   = 
OBJS_DEC_PAR   = dec_par_dmy.o
OBJS_DEC_LPC   = dec_lpc_dmy.o
OBJS_DEC_TF    = dec_tf_dmy.o tf_tables.o 
OBJS_DEC_SSC   = 

ifndef PAR
PAR = 1
endif
ifeq "$(PAR)" "1"
include makefile.par
endif

ifndef LPC
LPC = 1
endif
ifeq "$(LPC)" "1"
include makefile.lpc
endif

ifndef TF
TF = 1
endif
ifeq "$(TF)" "1"
include makefile.tf
endif

ifeq "$(SSC)" "1"
include makefile.ssc
endif

OBJS_DEC_PICOLA = 
LIBS_DEC_PICOLA = 
ifndef PICOLA
PICOLA = 1
endif
ifeq "$(PICOLA)" "1"
OBJS_DEC_PICOLA = pan_picola.o
else
OBJS_DEC_PICOLA = picola_dmy.o
endif

OBJS_CORE = common_m4a.o flex_mux.o bitstream.o 

ifeq "$(DEBUGPLOT)" "1"
OBJS_CORE += plotmtv_interf.o writevector.o
else
OBJS_CORE += plotmtv_interf_dummy.o
endif

ifeq "$(VERSION)" "2"
    ifeq "$(STATISTICS)" "1"
        OBJS_CORE += statistics.o
    else
        OBJS_CORE += statistics_dummy.o
    endif
    OBJS_CORE += resilience.o
endif

OBJS_COM = $(OBJS_CORE) cmdline.o audio.o austream.o fir_filt.o adif.o
OBJS_ENC = mp4auenc.o $(OBJS_COM)\
	$(OBJS_ENC_PAR) $(OBJS_ENC_LPC) $(OBJS_ENC_TF) $(OBJS_ENC_SSC)
OBJS_DEC = mp4audec.o $(OBJS_COM)\
	$(OBJS_DEC_PAR) $(OBJS_DEC_LPC) $(OBJS_DEC_TF) $(OBJS_ENC_SSC)\
	$(OBJS_DEC_PICOLA) mod_buf.o
OBJS_LATM = latm_cnvrt.o latm_modules.o cmdline.o $(OBJS_CORE)
OBJS_FL4_TO_MP4 = fl4_to_mp4.o cmdline.o $(OBJS_CORE)
OBJS_MP4_TO_FL4 = mp4_to_fl4.o cmdline.o $(OBJS_CORE)

ifeq "$(ISOMP4)"  "1"
OBJS_DEC += mp4ifc.o
endif

DUMMY_SOURCES = plotmtv_interf_dummy.c concealment_dummy.c statistics_dummy.c statistics_aac_dummy.c rvlcScfConceal_dummy.c

LIBS_ENC = $(LIBS_ENC_PAR) $(LIBS_ENC_LPC) $(LIBS_ENC_TF) $(LIBS_ENC_SSC) 
LIBS_DEC = $(LIBS_DEC_PAR) $(LIBS_DEC_LPC) $(LIBS_DEC_TF) $(LIBS_DEC_SSC) $(LIBS_DEC_PICOLA)
LIBS_LATM = $(LIBS_DEC_PAR) $(LIBS_DEC_LPC) $(LIBS_DEC_TF) $(LIBS_DEC_SSC) $(LIBS_DEC_PICOLA)

ADDONDIR = v$(VERSION)
ifneq "$(CORRIGENDUM)" "0"
ADDONDIR := $(ADDONDIR)_c$(CORRIGENDUM)
endif

#TARGETS = mp4audec mp4auenc
# HP20021125   mp4audec removed from list of default targets
#              use mp4audec built from mp4AudVm_Rewrite branch instead!!!
# OK20030226   latm_cnvrt fl4_to_mp4 and mp4_to_fl4 removed from list of
#              default targets. use the versions in the rewrite branch instead!!!
TARGETS = mp4auenc
#ifneq "$(VERSION)" "1"
#TARGETS += latm_cnvrt
#ifeq "$(ISOMP4)"  "1"
#TARGETS += fl4_to_mp4 mp4_to_fl4
#endif
#endif

# ifeq "$(ENCODE)" "1"
SRCDIRS := src_frame_enc:src_par_enc:src_lpc_enc:src_tf_enc:include_enc:\
           src_ssc_enc:src_ssc_enc/analysis_support:src_ssc_enc/quantisation_support:\
           src_ssc_enc/audio_read:src_ssc_enc/sbunch:src_ssc_enc/sinusoid_analysis:\
           src_ssc_enc/filter:src_ssc_enc/sinusoid_cont_phase:src_ssc_enc/stereo_analysis:\
           src_ssc_enc/formatter:src_ssc_enc/sinusoid_psycho:src_ssc_enc/stereo_quantisation:\
           src_ssc_enc/include:src_ssc_enc/sinusoid_quantisation:src_ssc_enc/transient_analysis:\
           src_ssc_enc/noise_analysis:src_ssc_enc/sinusoid_remove:src_ssc_enc/transient_detection:\
           src_ssc_enc/noise_quantisation:src_ssc_enc/sinusoid_synthesis:src_ssc_enc/transient_quantisation:\
           src_ssc_enc/noise_temp_env:src_ssc_enc/sinusoid_tracking:src_ssc_enc/transient_synthesis

LIBPATH += -L$(PLTDIR)/lib_enc
# endif
SRCDIRS := $(SRCDIRS):src_frame:src_par:src_lpc:src_tf:src_tf_fhg:src_latm:src_mp4:include
OPTIONS= $(MFLAGS) SRCDIRS="$(SRCDIRS)" ADDONDIR="$(ADDONDIR)" DEFFLAGS="$(DEFFLAGS)" INCL="$(INCL)"\
         LIBPATH="$(LIBPATH)" SYSLIBS="$(SYSLIBS)" INCLDEP=1

.PHONY: all mp4auenc mp4audec latm_cnvrt fl4_to_mp4 mp4_to_fl4 clean dummies

all: $(TARGETS)

mp4auenc :
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_ENC)" LIBS="$(LIBS_ENC)" ENCODE=1 binary

mp4audec : dummies
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_DEC)"        LIBS="$(LIBS_DEC)"   binary

latm_cnvrt : 
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LATM)"       LIBS="$(LIBS_LATM)"  binary

fl4_to_mp4 : 
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_FL4_TO_MP4)" LIBS="$(LIBS_LATM)"  binary

mp4_to_fl4 : 
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_MP4_TO_FL4)" LIBS="$(LIBS_LATM)"  binary

clean :
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=mp4auenc   OBJS="$(OBJS_ENC)" ENCODE=1  $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=mp4audec   OBJS="$(OBJS_DEC)"           $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=latm_cnvrt OBJS="$(OBJS_LATM)"          $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=fl4_to_mp4 OBJS="$(OBJS_FL4_TO_MP4)"    $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=mp4_to_fl4 OBJS="$(OBJS_MP4_TO_FL4)"    $@

dummies :
	@$(MAKE) SRCDIRS="$(SRCDIRS)" TARGET=mp4audec DUMMY_SOURCES="$(DUMMY_SOURCES)" $@

endif	# ifeq "$(MAKELEVEL)" "0"
