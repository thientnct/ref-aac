
TARGET = mpeg12to4


ifneq "$(MAKELEVEL)" "0"

# converter stuff
CNV_SRC	= src
CNV_OBJ	= mpeg12to4.o

# mpeg12 stuff
COMLIBS	= -lmpeg12dec -lmpeg12fdm
COMLIBS_libmpeg12dec.a_OPTIONS =MPEG_2_5=1
maplibmpeg12fdm.a = "mpeg12dec"
COMLIBS_libmpeg12fdm.a_OPTIONS =MPEG_2_5=1

# mpeg4 stuff
COMLIBS += -lstreamfile
COMLIBS_libstreamfile.a_OPTIONS = "MPEG12=1"
maplibstreamfile.a = "mp4AudVm_Rewrite"
inclibstreamfile.a = ../mp4AudVm_Rewrite/src_mp4 ../mp4AudVm_Rewrite/src_frame
#MP4_SRC =            ../mp4AudVm_Rewrite/src_mp4 ../mp4AudVm_Rewrite/src_frame ../mp4AudVm_Rewrite/src_tf
#MP4_OBJ = streamfile.o streamfile_helper.o streamfile_mp4.o streamfile_fl4.o streamfile_latm.o streamfile_raw.o
#ISOMP4 = 1
DEFFLAGS += "-DMPEG12"


#LIBS = -lisomediafile

#MP4_SRC	= 
#MP4_OBJ	= streamfile.o streamfile_helper.o streamfile_diagnose.o streamfile_mp4.o streamfile



SRCDIRS = $(CNV_SRC) $(M12_SRC) $(MP4_SRC)
OBJS 	= $(CNV_OBJ) $(MP4_OBJ)

include ../general/makefile.all








else # MAKELEVEL = 0

.PHONY: all $(TARGET) clean

all: $(TARGET)

$(TARGET):
	@$(MAKE) DEFFLAGS=$(DEFFLAGS) binary

clean:
	@$(MAKE) clean

endif
