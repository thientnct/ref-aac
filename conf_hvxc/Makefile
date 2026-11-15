#-*-makefile-*-

# ./makefile.cfg should have
# USE_AFSP=1
# ISOMP4=0

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

OBJS_HVXC_FCOMP=hvxc_fcomp.o hvxcCommon.o

OBJS_HVXC_INI_PHASE=hvxc_ini_phase.o

OBJS_SEGSNR=segsnr.o

SRCDIRS=src

OPTIONS=SRCDIRS="$(SRCDIRS)" INCLDEP=1

TARGETS=hvxc_fcomp hvxc_ini_phase segsnr

.PHONY: all hvxc_fcomp hvxc_ini_phase segsnr

all: $(TARGETS)

hvxc_fcomp:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS_HVXC_FCOMP)" TARGET=$@ binary

hvxc_ini_phase:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS_HVXC_INI_PHASE)" TARGET=$@ binary

segsnr:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS_SEGSNR)" TARGET=$@ binary

clean:
	@$(MAKE)            OBJS="$(OBJS_HVXC_FCOMP)" TARGET=hvxc_fcomp $@
	@$(MAKE)            OBJS="$(OBJS_HVXC_INI_PHASE)" TARGET=hvxc_ini_phase $@
	@$(MAKE)            OBJS="$(OBJS_SEGSNR)" TARGET=segsnr $@

endif
