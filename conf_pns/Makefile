#-*-makefile-*-

# ./makefile.cfg should have
# USE_AFSP=1
# ISOMP4=0

ifndef AFSP_INCLUDE_PATH
  AFSP_INCLUDE_PATH = /home/audio-archiv/soft/${SYSTEM_NAME}/include #<afsp include path>
endif

ifndef AFSP_LIBRARY_PATH
  AFSP_LIBRARY_PATH = /home/audio-archiv/soft/${SYSTEM_NAME}/lib   #<afsp lib path>
endif

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

OBJS=PnsConformanceMain.o transfo.o CPnsConformance.o

SRCDIRS=src $(AFSP_INCLUDE_PATH)
OPTIONS=SRCDIRS="$(SRCDIRS)" INCLDEP=1

all: pnsConformance
pnsConformance:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS)" TARGET=$@ binary

clean:
	@$(MAKE)            OBJS="$(OBJS)" TARGET=pnsConformance $@

endif
