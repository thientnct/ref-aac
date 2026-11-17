#------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile (requires GNU make)
#
# $Id: makefile,v 1.60 1999/07/26 13:17:56 purnhage Exp $
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
# SE	Sebastien Etienne, CCETT Rennes
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
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# Notes:
#
# This makefile provides automatic handling of different system
# platforms. Therefore it is required to set the environment
# variable SYSTEM_NAME according to the actual platform.
# Currently, these platforms are supported:
#   IRIX
#   Linux
#   Solaris
#   ... and most other UNIX platforms ...
#
# If CFLAGS or LDFLAGS are changed for a subsequent invocation of make,
# object files and binaries are NOT remade automatically with the new
# options! Use "make clean" first ...
#
# To make mp4auenc and mp4audec, type:
#   make all
#
# To allow make running e.g. 4 jobs at once, type:
#   make MFLAGS=-j4
#
# On most platforms, the default compiler is g++. To use the platform's
# native compiler (e.g. cc) instead, type:
#   make MAKE_CC=native
# and to use gcc, type:
#   make MAKE_CC=gnu
#
#----------------------------------------------------------------------


# BG: OS2 fix
ifeq "$(SYSTEM_NAME)" "OS2"
SHELL = /bin/sh
export SHELL
endif

# HP 981013
ifeq "$(SYSTEM_NAME)" ""
ifeq "$(OSTYPE)" ""
SYSTEM_NAME = $(shell uname -s)
else
ifeq "$(OSTYPE)" "linux"
SYSTEM_NAME = Linux
endif
ifeq "$(OSTYPE)" "linux-gnu"
SYSTEM_NAME = Linux
endif
ifeq "$(OSTYPE)" "solaris"
SYSTEM_NAME = Solaris
endif
ifeq "$(OSTYPE)" "iris4d"
SYSTEM_NAME = IRIX
endif
ifeq "$(OSTYPE)" "sunos4"
SYSTEM_NAME = SunOS
endif
ifeq "$(OSTYPE)" "osf1"
SYSTEM_NAME = OSF
endif
ifeq "$(OSTYPE)" "cygwin32"
SYSTEM_NAME = cdk
endif
endif
endif


ifeq "$(SYSTEM_NAME)" ""
include error:\ SYSTEM_NAME\ environment\ variable\ not\ defined\ !!!
include Options:\ SunOS\ IRIX\ Linux\ OS2\ Solaris\ OSF\ cdk\ !!!
endif


ifeq "$(MAKELEVEL)" "0"


# COMPILE TIME OPTIONS
# 
# Several options are available to enable/disable VM modules at
# compile time. These options can be specified in the make command line
# (e.g. make PAR=0) or in the makefile.cfg file. 

# makefile options:
# PAR=1		parametric core enabled
#    =0		parametric core disabled
# LPC=1		LPC-based core enabled
#    =0		LPC-based core disabled
# TF=1		t/f-based core enabled
#   =0		t/f-based core disabled
# G729=1	g729-based core enabled
#     =0	g729-based core disabled
# G723=1	g723-based core enabled
#     =0	g723-based core disabled
# PICOLA=1	PICOLA speed control enabled
#       =0	PICOLA speed control disabled
# DEBUGPLOT=1	DEBUGPLOT enabled (only used for debugging)
#          =0	DEBUGPLOT disabled
# USE_AFSP=1    enable AFsp
#         =0    disable AFsp (only 16 bit .au files supported)

-include makefile.cfg


# defaults:
ifndef PAR
PAR = 1
endif
ifndef LPC
LPC = 1
endif
ifndef TF
TF = 1
endif
ifndef G729
G729 = 0
endif
ifndef G723
G723 = 0
endif
ifndef PICOLA
PICOLA = 1
endif
ifndef DEBUGPLOT
DEBUGPLOT = 0
endif
ifndef USE_AFSP
USE_AFSP = 0
endif
ifndef ISOMP4
ISOMP4 = 0
endif


# compiler options
# NOTE: If CFLAGS or LDFLAGS are modified, object files are NOT remade
#       automatically !!!
#       Delete the object file manually to force recompile with the
#       new CFLAGS compiler options.

#CFLAGS += -Wall
#CFLAGS += -g
#CFLAGS += -O2
#CFLAGS += -pg
#LDFLAGS += -pg
#LDFLAGS += -g -fbounds-checking
#LDFLAGS += -g

# The default CFLAGS set below should work fine with gcc/g++.
# If desired, you can override them when invoking make, e.g.:
#   make CFLAGS=""
CFLAGS += -ansi -pedantic -Wall -W 
#CFLAGS += -ansi -pedantic -Wall -Wno-unused -W

# If desired, you can override DEFFLAGS when invoking make, e.g.:
#   make DEFFLAGS=""
#DEFFLAGS += -DHAS_ULONG

# BT 990215: separate -W is necessary for gcc v 2.8.1
#CFLAGS += -ansi -pedantic -Wall 


ifndef VERSION
VERSION = 1
endif

ifeq "$(VERSION)" "2"
override DEFFLAGS += -DVERSION2
endif

ifneq "$(NO_AL_CONFIG)" "1"
override DEFFLAGS += -DAL_CONFIG
endif

ifeq "$(DEBUGPLOT)" "1"
override DEFFLAGS += -DDEBUGPLOT -DFHG_DEBUGPLOT
override CFLAGS += -g
override LDFLAGS += -g
endif
# for more info on plotmtv see src_frame/plotmtv_interf.c
# see also src_frame/writevector.c

# type "make USE=AFSP=1" to enable AFsp
ifeq "$(USE_AFSP)" "1"
override DEFFLAGS += -DUSE_AFSP
endif

# ISO MP4 file format library
ifeq "$(ISOMP4)" "1"
# ISOMP4_PATH should point to your mp4lib directory tree
ISOMP4_PATH = mp4lib
override DEFFLAGS +=-DMP4FF
override MFLAGS += ISOMP4_PATH=$(ISOMP4_PATH)
endif

# if AFSP_XXX_PATH are not set, AFsp-V3R2 as in ./include and
# ./<platform>/lib is used
ifdef AFSP_INCLUDE_PATH
override MFLAGS += AFSP_INCLUDE_PATH=$(AFSP_INCLUDE_PATH)
endif
ifdef AFSP_LIBRARY_PATH
override MFLAGS += AFSP_LIBRARY_PATH=$(AFSP_LIBRARY_PATH)
endif

ifdef VERBOSE
override MFLAGS += VERBOSE=$(VERBOSE)
endif

# BG: separate sub-makefiles for the three codec families:
#       makefile.par
#       makefile.lpc
#       makefile.tf
#       makefile.g729

OBJS_ENC_PAR = enc_par_dmy.o
OBJS_ENC_LPC = enc_lpc_dmy.o
OBJS_ENC_TF  = enc_tf_dmy.o
OBJS_ENC_G729 = enc_g729_dmy.o
OBJS_ENC_G723 = enc_g723_dmy.o
OBJS_DEC_PAR = dec_par_dmy.o
OBJS_DEC_LPC = dec_lpc_dmy.o
#OBJS_DEC_TF  = dec_tf_dmy.o
# I hate to do this, but I'm forced :-(   HP 990120
# Please, Heiko, tell me why it is/was necessary to include concealment_dummy.o in OBJS_DEC_TF. lauber 990421
OBJS_DEC_TF  = dec_tf_dmy.o tf_tables.o
OBJS_DEC_G729  = dec_g729_dmy.o
OBJS_DEC_G723  = dec_g723_dmy.o

OBJS_ALL_G729 = $(OBJS_DEC_G729) $(OBJS_ENC_G729)
OBJS_ALL_G723 = $(OBJS_DEC_G723) $(OBJS_ENC_G723)

LIBS_ENC_PAR = 
LIBS_DEC_PAR = 
LIBS_ENC_LPC = 
LIBS_DEC_LPC = 
LIBS_ENC_TF = 
LIBS_DEC_TF = 
LIBS_ENC_G729 = 
LIBS_DEC_G729 = 
LIBS_ENC_G723 = 
LIBS_DEC_G723 = 

ifeq "$(PAR)" "1"
include makefile.par
endif
ifeq "$(LPC)" "1"
include makefile.lpc
endif
ifeq "$(TF)" "1"
include makefile.tf
endif
ifeq "$(G729)" "1"
include makefile.g729
endif
ifeq "$(G723)" "1"
include makefile.g723
endif

OBJS_DEC_PICOLA = 
LIBS_DEC_PICOLA = 
ifeq "$(PICOLA)" "1"
OBJS_DEC_PICOLA = pan_picola.o
else
OBJS_DEC_PICOLA = picola_dmy.o
endif



#############################################################################
#
# don't edit after this line !!!
#
# (unless you know what you are doing)
#


# object files

OBJS_COM = common_m4a.o cmdline.o bitstream.o audio.o austream.o\
	fir_filt.o flex_mux.o adif.o

ifeq "$(DEBUGPLOT)" "1"
OBJS_COM += plotmtv_interf.o writevector.o
else
OBJS_COM += plotmtv_interf_dummy.o
endif
# plotmtv_interf.c is just a C interface to the freeware program plotmtv, 
# see src_frame/plotmtv_interf.c for further information
# see also src_frame/writevector.c

ifndef STATISTICS
STATISTICS = 0
endif
ifeq "$(STATISTICS)" "1"
OBJS_COM += statistics.o
else
OBJS_COM += statistics_dummy.o
endif

OBJS_ENC = mp4auenc.o $(OBJS_COM)\
	$(OBJS_ENC_PAR) $(OBJS_ENC_LPC) $(OBJS_ENC_TF)\
	$(OBJS_ALL_G729) $(OBJS_ALL_G723)
OBJS_DEC = mp4audec.o $(OBJS_COM)\
	$(OBJS_DEC_PAR) $(OBJS_DEC_LPC) $(OBJS_DEC_TF)\
	$(OBJS_DEC_PICOLA) $(OBJS_DEC_G729) $(OBJS_DEC_G723) mod_buf.o

ifeq "$(ISOMP4)"  "1"
OBJS_DEC += mp4ifc.o
endif

ifeq "$(VERSION)" "2"
OBJS_DEC += resilience.o 
endif


# libraries

# -lm is always included (see makelevel 1)   HP 961203
LIBS_COM = 
ifeq "$(USE_AFSP)" "1"
LIBS_COM += -ltsp
endif
ifeq "$(ISOMP4)"  "1"
LIBS_COM += -lisomp4
endif

LIBS_ENC = $(LIBS_ENC_PAR) $(LIBS_ENC_LPC) $(LIBS_ENC_TF) $(LIB_ENC_G729)\
	$(LIB_ENC_G723)
LIBS_DEC = $(LIBS_DEC_PAR) $(LIBS_DEC_LPC) $(LIBS_DEC_TF) $(LIBS_DEC_PICOLA)\
	$(LIB_DEC_G729)	$(LIB_DEC_G723)


# rules

default : all

all : mp4auenc mp4audec

mp4auenc :
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)"\
	LIBS="$(LIBS_ENC)" COMLIBS="$(LIBS_COM)"\
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"\
	DEFFLAGS="$(DEFFLAGS)" VERSION="$(VERSION)"\
	INCLDEP=1 ENCODE=1 TARGET=mp4auenc binary

mp4audec : 
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_DEC)"\
	LIBS="$(LIBS_DEC)" COMLIBS="$(LIBS_COM)"\
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"\
	DEFFLAGS="$(DEFFLAGS)" VERSION="$(VERSION)"\
	INCLDEP=1 TARGET=mp4audec binary


depend :
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)" VERSION="$(VERSION)"\
	ENCODE=1 TARGET=mp4auenc depend
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_DEC)" VERSION="$(VERSION)"\
	TARGET=mp4audec depend

clean :
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_ENC)" VERSION="$(VERSION)"\
	ENCODE=1 TARGET=mp4auenc clean
	$(MAKE) $(MFLAGS) OBJS="$(OBJS_DEC)" VERSION="$(VERSION)"\
	TARGET=mp4audec clean

makedirs :
	$(MAKE) $(MFLAGS) OBJS="" VERSION="$(VERSION)"\
	TARGET=mp4auenc makedirs
	$(MAKE) $(MFLAGS) OBJS="" VERSION="$(VERSION)"\
	TARGET=mp4audec makedirs



endif    #MAKELEVEL == 0


#############################################################################


ifeq "$(MAKELEVEL)" "1"

# HP 981202  VERSION now passed down to MAKELEVEL 1
ifeq "$(VERSION)" "2"
VERDIR = v2
else
VERDIR = v1
endif

# -lm is always included   HP 961203
SYSLIBS = -lm

# defaults for "make depend" as used by most platforms
DEPFLAGS = -M
# SED = sed 's§$*.o§$(OBJDIR)/$*.o $(OBJDIR)/$*.d§g'
SED = sed 's@$*.o@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'

# automatic platform detection
##############################
ifeq "$(SYSTEM_NAME)" "SunOS"
  PLTDIR       = sunos
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  #override CFLAGS += -ansi -pedantic-errors
  DCC = g++
  CC = g++
  LINK = g++
else
##############################
ifeq "$(SYSTEM_NAME)" "HPUX"
  PLTDIR       = hpux
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  #override CFLAGS += -ansi -pedantic-errors
  #override CFLAGS += -ansi -g -pedantic -Wall
  #DCC = g++
  #CC = g++
  #LINK = g++
  #DCC = gcc
  #CC  = gcc
  #LINK = gcc
  ifneq "$(PURIFY)" "1"
    override CFLAGS += -Aa +O2
    DCC = gcc
    CC = cc
    LINK = cc
    LANG = C
  else
    override CFLAGS += -Aa -g
    DCC = gcc
    CC = cc
    LINK = purify cc
    LANG = C
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "IRIX"
  PLTDIR       = sgi
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  ifeq "$(MAKE_CC)" "CC"
    ## sgi c:
    #override CFLAGS += -ansi -pedantic -DHAS_ULONG
    #DCC = cc
    #CC = cc
    #LINK = cc
    ## sgi c++:
    PLTVERSYSDIR = $(PLTVERDIR)_cc
    override CFLAGS += -g -ansi -pedantic
    override LDFLAGS += -g
    DCC = CC
    CC = CC
    LINK = CC
#    SED = sed 's§a.out§$(OBJDIR)/$*.o $(OBJDIR)/$*.d§g'
    SED = sed 's@a.out@$(OBJDIR)/$*.o $(OBJDIR)/$*.d@g'
  else
    ifeq "$(MAKE_CC)" "gnu"
      PLTVERSYSDIR = $(PLTVERDIR)_gcc
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_gcc3
        override CFLAGS += -O3
      else
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = gcc
      CC = gcc
      LINK = gcc
    else 
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_3
        override CFLAGS += -O3
      else
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = g++
      CC = g++
      LINK = g++
    endif
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "IRIX64"
  PLTDIR       = sgi64
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  ifeq "$(MAKE_CC)" "native"
    PLTVERSYSDIR = $(PLTVERDIR)_native
    override CFLAGS += -ansi -pedantic -fullwarn -woff 1174,1209,1552,1521
    CXX = cc -n32
    LINK = cc -n32
    ifeq "$(OPTIM_LEV)" "0"
      DCC = $(CXX) -g
      CC = $(CXX) -g
      #LINK =  $(LINK) -g
    endif # OPTIM_LEV == 0
    ifeq "$(OPTIM_LEV)" "1"
      PLTVERSYSDIR = $(PLTVERDIR)_native1
      DCC =  $(CXX) -O -g1
      CC =  $(CXX) -O -g1
      #LINK =  $(LINK) -g
    endif # OPTIM_LEV == 1
    ifeq "$(OPTIM_LEV)" "2"
      PLTVERSYSDIR = $(PLTVERDIR)_native2
      DCC =  $(CXX) -O2 -g2
      CC =  $(CXX) -O2 -g2
      #LINK =  $(LINK) -g
    endif # OPTIM_LEV == 2
    ifeq "$(OPTIM_LEV)" "3"
      PLTVERSYSDIR = $(PLTVERDIR)_native3
      DCC =  $(CXX) -O3 -g3
      CC =  $(CXX) -O3 -g3
      #LINK =  $(LINK) -g
    endif # OPTIM_LEV == 3
    ifeq "$(OPTIM_LEV)" "4"
      PLTVERSYSDIR = $(PLTVERDIR)_native4
      DCC =  $(CXX) -O4 -g4
      CC =  $(CXX) -O4 -g4
      #LINK =  $(LINK) -g
    endif # OPTIM_LEV == 4
  else
    ifeq "$(MAKE_CC)" "cc"
      PLTVERSYSDIR = $(PLTVERDIR)_cc
      override CFLAGS += -g -ansi -pedantic 
      override LDFLAGS += -g
      DCC = cc 
      CC = cc
      LINK = cc
    else
      ifeq "$(MAKE_CC)" "gnu"
        PLTVERSYSDIR = $(PLTVERDIR)_gcc
        ifeq "$(OPTIM_LEV)" "3"
          PLTVERSYSDIR = $(PLTVERDIR)_gcc3
          override CFLAGS += -O3
        else
          override CFLAGS += -g
          override LDFLAGS += -g
        endif
        DCC = gcc
        CC = gcc
        LINK = gcc
      else 
        ifeq "$(OPTIM_LEV)" "3"
          PLTVERSYSDIR = $(PLTVERDIR)_g++3
          override CFLAGS += -O3
        else
          PLTVERSYSDIR = $(PLTVERDIR)_g++
          override CFLAGS += -g
          override LDFLAGS += -g
        endif
        DCC = g++
        CC = g++
        LINK = g++
      endif
    endif
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "Linux"
  PLTDIR       = linux
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  ifeq "$(BOUNDS)" "1"
    PLTVERSYSDIR = $(PLTVERDIR)_bounds
    GCC_HOME = $(HOME)/tools/gcc-2.7.2
    override CFLAGS +=-B$(GCC_HOME)/ -g -fbounds-checking -ansi -pedantic -Wall #-fno-builtin -Wno-unused
    DCC = $(GCC_HOME)/xgcc
    CC = $(GCC_HOME)/xgcc
    LINK = $(GCC_HOME)/xgcc -B$(GCC_HOME)/ -g -fbounds-checking #-fno-builtin
  else
    #override CFLAGS += -ansi -pedantic -Wmissing-prototypes -Wall  -Wno-unused
    override CFLAGS += -ansi -pedantic -Wall -Wno-unused -Wmissing-prototypes 
    ifeq "$(MAKE_CC)" "gnu"
      PLTVERSYSDIR = $(PLTVERDIR)_gcc
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_gcc3
        override CFLAGS += -O3
      else
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = gcc
      CC = gcc
      LINK = gcc
    else 
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_g++3
        override CFLAGS += -O3
      else
        PLTVERSYSDIR = $(PLTVERDIR)_g++
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = g++
      CC = g++
      LINK = g++
    endif
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "Linux6"
  PLTDIR       = linux6
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  ifeq "$(BOUNDS)" "1"
    PLTVERSYSDIR = $(PLTVERDIR)_bounds
    GCC_HOME = $(HOME)/tools/gcc-2.7.2
    override CFLAGS +=-B$(GCC_HOME)/ -g -fbounds-checking -ansi -pedantic -Wall #-fno-builtin -Wno-unused
    DCC = $(GCC_HOME)/xgcc
    CC = $(GCC_HOME)/xgcc
    LINK = $(GCC_HOME)/xgcc -B$(GCC_HOME)/ -g -fbounds-checking #-fno-builtin
  else
    #override CFLAGS += -ansi -pedantic -Wmissing-prototypes -Wall  -Wno-unused
    override CFLAGS += -ansi -pedantic -Wall -Wno-unused -Wmissing-prototypes 
    ifeq "$(MAKE_CC)" "gnu"
      PLTVERSYSDIR = $(PLTVERDIR)_gcc
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_gcc3
        override CFLAGS += -O3
      else
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = gcc
      CC = gcc
      LINK = gcc
    else 
      ifeq "$(OPTIM_LEV)" "3"
        PLTVERSYSDIR = $(PLTVERDIR)_g++3
        override CFLAGS += -O3
      else
        PLTVERSYSDIR = $(PLTVERDIR)_g++
        override CFLAGS += -g
        override LDFLAGS += -g
      endif
      DCC = g++
      CC = g++
      LINK = g++
    endif
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "OSF"
  #override CFLAGS += -ansi -pedantic -Wall
  PLTDIR       = osf
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  #DCC = g++
  #CC = g++
  #LINK = g++
  DCC  = cc
  CC   = cc
  LINK = cc
else
##############################
ifeq "$(SYSTEM_NAME)" "OS2"
  #override CFLAGS += -ansi -pedantic-errors
  override CFLAGS += -ansi -pedantic -Wall
  PLTDIR       = os2
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  DCC = gcc
  CC = gcc
  LINK = gcc -g
  EXE_SUFFIX = .exe
else
##############################
ifeq "$(SYSTEM_NAME)" "Solaris"
  PLTDIR       = sol
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  ifeq "$(MAKE_CC)" "native"	
    DCC = cc 
    CC = cc 
    LINK = cc
    SOLCC = native
    PLTVERSYSDIR = $(PLTVERDIR)_native
    override DEPFLAGS = -xM1 
    override LDFLAGS  = -g -xs -Xc -z i_full
    #override DEFFLAGS += -DHAS_ULONG
    ifeq "$(OPTIM_LEV)" "3"
      PLTVERSYSDIR = $(PLTVERDIR)_native3
      override CFLAGS  =  -g -xs -Xc -xO3 -v
    else
      override CFLAGS  =  -g -xs -Xc  -v
    endif
  else
    #override DEFFLAGS += -DHAS_ULONG
    ifeq "$(BOUNDS)" "1"
      PLTVERSYSDIR = $(PLTVERDIR)_bounds
      DCC = gcc 
      CC = gcc
      LINK = gcc
      CFLAGS += -ansi -pedantic -Wall
      override CFLAGS += -g -fbounds-checking
      override LDFLAGS += -g -fbounds-checking
    else
      ifeq "$(MAKE_CC)" "gnu"
        PLTVERSYSDIR = $(PLTVERDIR)_gcc
        ifeq "$(OPTIM_LEV)" "3"
          PLTVERSYSDIR = $(PLTVERDIR)_gcc3
          override CFLAGS += -O3
        else
          override CFLAGS += -g
          override LDFLAGS += -g
        endif
        DCC = gcc
        CC = gcc
        LINK = gcc
      else 
        ifeq "$(OPTIM_LEV)" "3"
          PLTVERSYSDIR = $(PLTVERDIR)_g++3
          override CFLAGS += -O3
        else
          PLTVERSYSDIR = $(PLTVERDIR)_g++
          override CFLAGS += -g 
          override LDFLAGS += -g
        endif
        DCC = g++
        CC = g++
        LINK = g++
      endif
    endif
  endif
else
##############################
ifeq "$(SYSTEM_NAME)" "cdk"
  #override CFLAGS += -ansi -pedantic-errors
  PLTDIR       = cdk
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  DCC = gcc
  CC = gcc
  LINK = gcc
  EXE_SUFFIX = .exe
else
##############################
  PLTDIR       = unknown
  PLTVERDIR    = $(PLTDIR)_$(VERDIR)
  PLTVERSYSDIR = $(PLTVERDIR)
  DCC = $(CC)
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif


# set paths and directories

SRCPATH = src_frame: src_par: src_lpc: src_tf: src_g729: src_g723
SRCPATHENC = src_frame_enc: src_par_enc: src_lpc_enc: src_tf_enc:\
	src_g729_enc: src_g723_enc

BINDIR = $(PLTVERSYSDIR)
OBJDIR = $(PLTVERSYSDIR)/obj
LIBDIR = $(PLTDIR)/lib
LIBDIRENC = $(PLTDIR)/lib_enc
PLTDIRTAG = $(PLTDIR)/_$(SYSTEM_NAME)_ $(PLTVERSYSDIR)/_$(SYSTEM_NAME)_

INCL =
LIBPATH =

# AFSP paths
ifdef AFSP_INCLUDE_PATH
INCL += -I$(AFSP_INCLUDE_PATH)
endif
ifdef AFSP_LIBRARY_PATH
LIBPATH += -L$(AFSP_LIBRARY_PATH)
endif

ifdef ISOMP4_PATH
ISOMP4_INCLUDE_PATH = $(ISOMP4_PATH)/src
ISOMP4_PLATFORM_PATH = $(ISOMP4_PATH)/$(PLTDIR)
ISOMP4_LIBRARY_PATH = $(ISOMP4_PLATFORM_PATH)/libisomp4
INCL += -I$(ISOMP4_PLATFORM_PATH) -I$(ISOMP4_INCLUDE_PATH) 
LIBPATH += -L$(ISOMP4_LIBRARY_PATH)
endif

ifeq "$(ENCODE)" "1"

# enable encoder directories (make encoder)
INCL += -Isrc_frame -Iinclude -Isrc_frame_enc -Iinclude_enc\
	-Isrc_par -Isrc_lpc -Isrc_tf -Isrc_g729 -Isrc_g723\
	-Isrc_par_enc -Isrc_lpc_enc -Isrc_tf_enc -Isrc_g729_enc -Isrc_g723_enc
LIBPATH += -L$(LIBDIR) -L$(LIBDIRENC)
vpath %.c
vpath %.c $(SRCPATH): $(SRCPATHENC)

else

# disable encoder directories (make decoder -g)
INCL += -Isrc_frame -Iinclude -Isrc_par -Isrc_lpc -Isrc_tf\
	-Isrc_g729 -Isrc_g723
LIBPATH += -L$(LIBDIR)
vpath %.c
vpath %.c $(SRCPATH)

endif


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
# HP 990428   libtsp.a (the only entry in COMLIBS) can't be automatically
# generated by this makefile anyway - so there is no "dependency" required ...
#DIRLIBS += $(addsuffix .a,$(subst -l,$(LIBDIR)/lib,$(COMLIBS)))

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

makedirs : $(PLTDIRTAG)

binary : $(PLTDIRTAG)
binary : $(DIRTARGET)$(EXE_SUFFIX)
	@echo "*****" made binary $<

library : $(PLTDIRTAG)
library : $(DIRTARGET)
	@echo "*****" made library $<

depend : $(PLTDIRTAG)
depend : $(DIRDEPS)
	@echo "*****" made dependencies for $(DIRTARGET)

ifeq "$(suffix $(TARGET))" "$(LIB_SUFFIX)"
# compose library
$(DIRTARGET) : $(PLTDIRTAG)
$(DIRTARGET) : $(DIROBJS)
	@echo "*****" composing library $@ ...
	-rm $(DIRTARGET)
#	ar rv $(DIRTARGET) $^
	ar rv $(DIRTARGET) $(DIROBJS)
	ranlib $(DIRTARGET)
else
# link
$(DIRTARGET)$(EXE_SUFFIX) : $(PLTDIRTAG)
$(DIRTARGET)$(EXE_SUFFIX) : $(DIROBJS) $(DIRLIBS) $(FORCE_LINKING)
	@echo "*****" linking $@ ...
	$(LINK) $(LDFLAGS) $(DIROBJS) $(LIBPATH)\
	$(LIBS) $(COMLIBS) $(SYSLIBS) -o $@
	@echo "OLDLNK = " $(LNK) > $(DIRTARGET)$(LNK_SUFFIX)
endif

ifdef FHG_MAKEDUMMYFILE
# ... doesn't work outside of FhG   HP 990520
# generate dummy file
%_dummy.c: %.c
	@echo "*****" making dummy file for $< ...
	gawk -f makedummyfile $< > $@
endif

# compile
$(DIROBJS) : $(PLTDIRTAG)
$(DIROBJS) : $(OBJDIR)/%.o : %.c
	@echo "*****" compiling $< ...
ifeq "$(VERBOSE)" "1"
	$(CC) -c $(DEFFLAGS) $(CFLAGS) $(INCL) $< -o $@
else
	@$(CC) -c $(DEFFLAGS) $(CFLAGS) $(INCL) $< -o $@
endif

# generate dependencies
$(DIRDEPS) : $(PLTDIRTAG)
$(DIRDEPS) : $(OBJDIR)/%.d : %.c 
	@echo "*****" making dependencies for $< ...
#	lint -c $(DEFFLAGS) $(CFLAGS) $(INCL) $< 
ifeq "$(VERBOSE)" "1"
	$(DCC) $(DEFFLAGS) $(DEPFLAGS) $(INCL) $< | $(SED) > $@
else
	@$(DCC) $(DEFFLAGS) $(DEPFLAGS) $(INCL) $< | $(SED) > $@
endif

# include dependency files *.d
# NOTE: If an included *.d file does not exist, this file becomes a target
#       that is to be made if a corresponding rule exists!
#       (here: "generate dependencies" rule)
ifeq "$(INCLDEP)" "1"
-include $(DIRDEPS)
endif

$(PLTDIRTAG) :
	@echo "*****" making directories
	-mkdir -p $(OBJDIR)
	-mkdir -p $(LIBDIR)
	touch $(PLTDIRTAG)

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

