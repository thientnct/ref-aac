#-*-makefile-*-

# ./makefile.cfg should have
# USE_AFSP=1
# ISOMP4=0

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

OBJS=mpeg4conf.o ssnrcd.o audiofile_io.o cepstrum_analysis.o pns.o transfo.o

SRCDIRS=src
OPTIONS=SRCDIRS="$(SRCDIRS)" INCLDEP=1

all: ssnrcd

ssnrcd:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS)" TARGET=$@ binary
	
mpeg4conf:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS)" TARGET=$@ binary

clean:
	@$(MAKE)            OBJS="$(OBJS)" TARGET=ssnrcd $@

endif
