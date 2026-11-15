#-*-makefile-*-

# ./makefile.cfg should have
# USE_AFSP=1
# ISOMP4=0

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

OBJS_HILNCONF=hilnconf.o afsp_io.o

SRCDIRS=src

OPTIONS=SRCDIRS="$(SRCDIRS)" INCLDEP=1

TARGETS=hilnconf

.PHONY: all hilnconf

all: $(TARGETS)

hilnconf:
	@$(MAKE) $(OPTIONS) OBJS="$(OBJS_HILNCONF)" TARGET=$@ binary

clean:
	@$(MAKE)            OBJS="$(OBJS_HILNCONF)" TARGET=hilnconf $@

endif
