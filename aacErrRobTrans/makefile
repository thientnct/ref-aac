#-*-makefile-*-
# Usage: gmake [<general options>] [STATISTICS=1] [SCF_CONCEAL=1]

include ../general/makefile.all

ifeq "$(MAKELEVEL)" "0"

# Common
OBJS  = aiff_support.o config.o coupling.o decdata.o decoder.o \
	huffdec1.o huffdec2.o huffdec3.o huffinit.o hufftables.o \
	intensity.o intrins.o monopred.o portio.o stereo.o tns.o \
	reorderspec.o bitfct.o scf_resil.o resorg.o escHandling.o nulltio.o

# DOLBY imdct
OBJS += block.o dolby_adapt.o util.o weave.o transfo.o

# Statistics
ifeq "$(STATISTICS)" "1"
OBJS += statistics.o
else
OBJS += statistics_dummy.o
endif

# RVLC scalefactor coding
OBJS += rvlcScf.o escapeScf.o

# RVLC scalefactor concealment
ifeq "$(SCF_CONCEAL)" "1"
OBJS += rvlcConceal.o
else
OBJS += rvlcConceal_dummy.o
endif

SRCDIRS  += .:src_fhg
override DEFFLAGS += -DDOUBLE_WIN -DDOLBY_MDCT -D__USE_FIXED_PROTOTYPES__ # -DDEBUG_TESTVEC
OPTIONS  += $(MFLAGS) SRCDIRS="$(SRCDIRS)" DEFFLAGS="$(DEFFLAGS)" INCLDEP=1

.PHONY: all aactrans clean

all: aactrans

aactrans:
	@echo "***** making $@"
	@$(MAKE) $(OPTIONS) TARGET=$@       OBJS="$(OBJS)" binary

clean:
	@$(MAKE)            TARGET=aactrans OBJS="$(OBJS)" $@

endif
