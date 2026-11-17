#----------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile (requires GNU make)
#
# $Id: makefile,v 1.6 1998-05-07 21:30:28+03 suonio Exp $
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
# CL    Chuck Lueck, TI <lueck@ti.com>
# YT    Yasuhiro Toguri (Sony Corporation)
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
# 31-jan-97   HP    seperated encoder and decoder directories
# 05-feb-97   HP    added COMLIBS
# 12-feb-97   HP    make CFLAGS and LDFLAGS values passed on and evaluated
# 14-feb-97   HP    included PICOLA speed control (by Panasonic)
# 11-mar-97   HP    included PICOLA source code
# 21-mar-97   BT    fixed CC and IRIX things, platform autodetect
# 07-jun-97   CL    Removed makefile.lpc, makefile.par, and related
#                   source files.  Makefile now builds for AAC only.
# 01-jul-97   YT    enable SSR profile with encoder source codes.
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# Notes:
#
# This makefile provides automatic handling of different system
# platforms. Therefore it is required to set the environment
# variable SYSTEM_NAME according to the actual platform.
# Currently, these platforms are supported:
#   SunOS
#   IRIX
#   Linux
#   Solaris
#
# If CFLAGS or LDFLAGS are changed for a subsequent invocation of make,
# object files and binaries are NOT remade automatically with the new
# options! Use "make clean" first ...
#
#----------------------------------------------------------------------

# Enable Sony AAC pre-processing
AAC_PP=""

# BG: OS2 fix
ifeq "$(SYSTEM_NAME)" "OS2"
SHELL = /bin/sh
export SHELL
endif

ifeq "$(SYSTEM_NAME)" ""
ifeq "$(OSTYPE)" "solaris"
SYSTEM_NAME = Solaris
endif
endif

ifeq "$(SYSTEM_NAME)" ""
ifeq "$(OSTYPE)" "sunos4"
SYSTEM_NAME = SunOS
endif
endif

ifeq "$(SYSTEM_NAME)" ""
ifeq "$(OSTYPE)" "iris4d"
SYSTEM_NAME = IRIX
endif
endif

ifeq "$(SYSTEM_NAME)" ""
ifeq "$(OSTYPE)" "linux"
SYSTEM_NAME = Linux
endif
endif


ifeq "$(SYSTEM_NAME)" ""
include error\ :\ SYSTEM_NAME\ environment\ variable_not\ defined\ \ \ \ \ \ \ \!!
include Options:\ SunOS\ IRIX\ Linux\ OS2\ Solaris\ \ \ \ \ \ \ \!!
endif


ifeq "$(MAKELEVEL)" "0"

# compiler options
# NOTE: If CFLAGS or LDFLAGS are modified, object files are NOT remade
#       automatically !!!
#       Delete the object file manually to force recompile with the
#       new CFLAGS compiler options.

CFLAGS += -g -DPLOTMTV 
#for more info on plotmtv see src_frame/plotmtv_interf.c


#OBJS for SON_AACPP
# HP 970314
LIBS_SON_AACPP_ENC =
ifdef AAC_PP
OBJS_SON_AACPP_ENC = son_gc_common.o son_gc_detecter.o son_gc_etc_enc.o son_pqf.o son_gc_modifier.o son_gc_pack.o son_pqf_common.o son_gc_detectreset.o
#LIBS_SON_AACPP_ENC = -lpqf_son_NBCpp -lgain_son_NBCpp
LIBS_SON_AACPP_ENC = 
endif

OBJS_AAC_QC  = aac_qc.o aac_se_enc.o aac_back_pred.o tns.o mc_enc.o ms.o is.o nok_ltp_enc.o nok_pitch.o

CFLAGS +=  -DSRS -DDOUBLE_WIN -DDOLBY_MDCT -DOLD_PRED_PARAMS -DMPEG4V1
# if AAC_PP defined, add SON_AAC_PP flag	
ifdef AAC_PP
CFLAGS += -DSON_AAC_PP
endif

OBJS_PSYCH = psych.o

OBJS_AAC_ENC = util.o transfo.o imdct.o

# HP 970314
OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o  $(OBJS_T2F_ENC)  $(OBJS_AAC_QC) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC) $(OBJS_MOT_QC) 
LIBS_ENC_TF = $(LIBS_T2F_ENC) $(LIBS_VQ) $(LIBS_SON_AACPP_ENC)


#include makefile.tf


#############################################################################
#
# don't edit after this line !!!
#
# (unless you know what you are doing)
#


# object files

OBJS_COM = common.o cmdline.o bitstream.o audio.o plotmtv_interf.o
#plotmtv_interf.c is just a C interface to the freeware program plotmtv, 
#see src_frame/plotmtv_interf.c for further information
OBJS_ENC = encoder.o $(OBJS_COM) $(OBJS_ENC_PAR) $(OBJS_ENC_LPC) $(OBJS_ENC_TF)

# libraries
# -lm is always included (see makelevel 1)   HP 961203
LIBS_COM = -ltsp
LIBS_ENC = $(LIBS_ENC_PAR) $(LIBS_ENC_LPC) $(LIBS_ENC_TF)


# rules

default : enc

all : enc 

enc : 
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)"\
	LIBS="$(LIBS_ENC)" COMLIBS="$(LIBS_COM)"\
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"\
	INCLDEP=1 ENCODE=1 TARGET=enc binary

depend :	
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)" ENCODE=1 TARGET=enc depend

clean :	
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)" ENCODE=1 TARGET=enc clean

makedirs : FORCE
	$(MAKE) $(MFLAGS) OBJS="" OBJ_DIR_SUFFIX="_enc" TARGET=enc makedirs

FORCE:

endif    #MAKELEVEL == 0


#############################################################################


ifeq "$(MAKELEVEL)" "1"


# -lm is always included   HP 961203
SYSLIBS = -lm
SED = sed 's§$*.o§$(OBJDIR)/$*.o $(OBJDIR)/$*.d§g'
# automatic platform detection
ifeq "$(SYSTEM_NAME)" "SunOS"
override CFLAGS += -ansi -pedantic
PLTDIR = sunos
CC = g++
LINK = g++
#CC = gcc
#LINK = gcc
SED = sed 's@$*.o@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'
else
ifeq "$(SYSTEM_NAME)" "IRIX"
PLTDIR = sgi

#gnu c++:
override CFLAGS += -ansi -pedantic
CC = g++
LINK = g++
SED = sed 's@$*.o@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'

#sgi c++:
#override CFLAGS += -ansi 
#CC = CC
#LINK = CC
#SED = sed 's@a.out@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'

else
ifeq "$(SYSTEM_NAME)" "Linux"
override CFLAGS += -ansi -pedantic
PLTDIR = linux
CC = g++
LINK = g++
override DEPFLAGS =  -M
SED = sed 's@$*.o@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'
else
ifeq "$(SYSTEM_NAME)" "OS2"
override CFLAGS += -ansi -pedantic
PLTDIR = os2
CC = gcc
LINK = gcc -g
SED = sed 's§$*.o§$(OBJDIR)/$*.o $(OBJDIR)/$*.d§g'
EXE_SUFFIX = .exe
else
ifeq "$(SYSTEM_NAME)" "Solaris"
  ifeq "$(MAKE_CC)" "native"	
    DCC = cc 
    CC = cc 
    LINK = cc
    SOLCC = native
    PLTDIR = sol_native
    override DEPFLAGS =  -xM1
    override LDFLAGS +=  -g -xs -Xc -z i_full  -DHAS_ULONG
    ifeq "$(OPTIM_LEV)" "3"
      PLTDIR = sol_native3
      override CFLAGS +=  -g -xs -Xc  -xO3 -v -DHAS_ULONG 
    else
      override CFLAGS += -g -xs -Xc -v -DHAS_ULONG
    endif
  else
    ifeq "$(MAKE_CC)" "gnu"
      ifeq "$(BOUNDS)" "1"

        DCC = gcc 
        CC = gcc
        PLTDIR = sol_gcc_bounds
        LINK = gcc -fbounds-checking
        override CFLAGS += -g  -pedantic -Wall -fbounds-checking -DHAS_ULONG
      else
        DCC = gcc
        CC = gcc
        LINK = gcc 
        PLTDIR = sol_gcc
        override CFLAGS += -g  -pedantic -Wall  -DHAS_ULONG
      endif
    else
      #override CFLAGS +=  -ansi -pedantic-errors -DHAS_ULONG -Wall  -Wno-unused
      #override CFLAGS += -ansi -pedantic-errors -fbounds-checking -DHAS_ULONG -Wall
      PLTDIR = sol
      DCC = g++
      CC = g++
      LINK = g++
      override DEPFLAGS =  -M
    endif
  endif
else
PLTDIR = unkown
endif
endif
endif
endif
endif


# set paths and directories

#SRCPATH = src_frame: src_par: src_lpc: src_tf
#SRCPATHENC = src_frame_enc: src_par_enc: src_lpc_enc: src_tf_enc
SRCPATH = src
SRCPATHENC = src
BINDIR = $(PLTDIR)
OBJDIR = $(PLTDIR)/obj
LIBDIR = $(PLTDIR)/lib
LIBDIRENC = $(PLTDIR)/lib_enc


# enable encoder directories (make encoder)
INCL =  -Iinclude  -Isrc
LIBPATH = -L$(LIBDIR) -L$(LIBDIRENC)
vpath %.c
vpath %.c $(SRCPATH): $(SRCPATHENC)


# add directory and extension to filenames

LIB_SUFFIX = .a
LNK_SUFFIX = .lnk

DIROBJS = $(addprefix $(OBJDIR)/,$(OBJS))
DIRDEPS = $(DIROBJS:%.o=%.d)

ifeq "$(ENCODE)" "1"
DIRLIBS = $(addsuffix .a,$(subst -l,$(LIBDIRENC)/lib,$(LIBS)))
else
DIRLIBS = $(addsuffix .a,$(subst -l,$(LIBDIR)/lib,$(LIBS)))
endif
DIRLIBS += $(addsuffix .a,$(subst -l,$(LIBDIR)/lib,$(COMLIBS)))

ifeq "$(suffix $(TARGET))" "$(LIB_SUFFIX)"
ifeq "$(ENCODE)" "1"
DIRTARGET = $(LIBDIRENC)/$(TARGET)
else
DIRTARGET = $(LIBDIR)/$(TARGET)
endif
else
DIRTARGET = $(BINDIR)/$(TARGET)
endif


# compose list of object/library files for linker
# force linking if list of object/library files changed

LNK = $(DIROBJS) $(LIBPATH) $(LIBS) $(COMLIBS)
-include $(DIRTARGET)$(LNK_SUFFIX)

ifneq "$(strip $(OLDLNK))" "$(strip $(LNK))"
FORCE_LINKING = force
endif


# rules

force :
	@echo "*****" force linking

binary : $(DIRTARGET)$(EXE_SUFFIX)
	@echo "*****" made binary $<

library : $(DIRTARGET)
	@echo "*****" made library $<

depend : $(DIRDEPS)
	@echo "*****" made dependencies for $(DIRTARGET)

makedirs :
	@echo "making directories"
	-mkdir -p $(OBJDIR)
	-mkdir -p $(LIBDIR)

ifeq "$(suffix $(TARGET))" "$(LIB_SUFFIX)"
# compose library
$(DIRTARGET) : $(DIROBJS)
	@echo "*****" composing library $@ ...
	ar rv $(DIRTARGET) $^
	ranlib $(DIRTARGET)
else
# link
$(DIRTARGET)$(EXE_SUFFIX) : $(DIROBJS) $(DIRLIBS) $(FORCE_LINKING)
	@echo "*****" linking $@ ...
	$(LINK) $(LDFLAGS) $(DIROBJS) $(LIBPATH)\
	$(LIBS) $(COMLIBS) $(SYSLIBS) -o $@
	@echo "OLDLNK = " $(LNK) > $(DIRTARGET)$(LNK_SUFFIX)
endif

# compile
$(DIROBJS) : $(OBJDIR)/%.o : %.c
	@echo "*****" compiling $< to $@ ...
	$(CC)  -c $(CFLAGS) $(INCL) $< -o $@

# generate dependencies
$(DIRDEPS) : $(OBJDIR)/%.d : %.c
	@echo "*****" making dependencies for $< to $@...
	$(CC)  $(DEPFLAGS)  $(INCL) $< |	$(SED) > $@
#sed script is different for gnu g++ and sgi CC

# include dependency files *.d
# NOTE: If an included *.d file does not exist, this file becomes a target
#       that is to be made if a corresponding rule exists!
#       (here: "generate dependencies" rule)
ifeq "$(INCLDEP)" "1"
-include $(DIRDEPS)
endif

clean : 
	@echo "*****" cleaning ...
	-rm $(DIRDEPS)
	-rm $(DIROBJS)
ifeq "$(suffix $(TARGET))" "$(LIB_SUFFIX)"
	-rm $(DIRTARGET)
else
	-rm $(DIRTARGET)$(EXE_SUFFIX)
	-rm $(DIRTARGET)$(LNK_SUFFIX)
endif


endif	#(MAKELEVEL) == 1


#############################################################################

# end of makefile

