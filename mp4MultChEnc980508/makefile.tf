#-*-makefile-*- do not remove, it's for emacs!
#----------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile.tf
#
# $Id: makefile.tf,v 1.1.1.2 1998-05-07 17:19:30+03 suonio Exp $
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
# TK    Takashi Koike, Sony Corporation <koike@av.crl.sony.co.jp>
# TK    Yasuhiro Toguri, Sony Corporation <toguri@av.crl.sony.co.jp>
# CL    Chuck Lueck, TI <lueck@ti.com>
#
# Changes:
# 20-aug-96   BG      updated version
# 26-aug-96   HP      changed to enc_tf.c and dec_tf.c, removed PLOBJS
# 26-aug-96   HP      CVS
# 28-aug-96   NI      added objects for the NTT's VQ coder
# 25-oct-96   HP      adapted makefile options
# 13-nov-96   BG/HP   possible default: TF_QC = AAC
# 06-dec-96   NI      changed objects for the NTT's VQ encoder
# 09-feb-97   TK      integrated AAC compliant pre-/post- processing modules
# 14-mar-97   HP      merged FhG AAC decoder code
#                     temporary patch to keep encoder "alive" (see HP 970314)
# 07-jun-97   CL      Modified makefile to build for AAC encoder only.
# 01-july-97  YT      enable SSR profile with encoder source codes.
#----------------------------------------------------------------------


# makefile.par options:
# TF_T2F=UER   UER T2F code
# TF_QC=UER    UER QC code
#      =AAC    AAC QC code

# defaults:
ifndef TF_T2F
TF_T2F = UER
endif
ifndef TF_QC
TF_QC = AAC
endif

#OBJS for SON_AACPP
# HP 970314
LIBS_SON_AACPP_ENC =
ifdef AAC_PP
#echo SONY AACPP LINKED..
#LIBS_SON_AACPP_ENC = -lpqf_son_NBCpp -lgain_son_NBCpp
LIBS_SON_AACPP_ENC = 
endif
LIBS_SON_AACPP_DEC =
OBJS_SON_AACPP_ENC = son_gc_common.o son_gc_detecter.o son_gc_etc_enc.o son_pqf.o son_gc_modifier.o son_gc_pack.o son_pqf_common.o 

OBJS_SON_AACPP_DEC = son_gc_unpack.o son_gc_common.o son_gc_compensate.o son_ipqf.o son_pqf_common.o son_gc_etc.o 

# disable TF dummy
OBJS_ENC_TF =
OBJS_DEC_TF =

# HP 970314
#OBJS_MOT_QC  = bstream.o hq.o huffman.o ninject.o q.o sf.o

OBJS_AAC_QC  = aac_qc.o

CFLAGS +=  -DSRS -DDOUBLE_WIN -DDOLBY_MDCT -DOLD_PRED_PARAMS 

OBJS_PSYCH = psych.o

OBJS_AAC_ENC = util.o transfo.o imdct.o

# the next 4 definitions are relevant to the master makefile
#OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o  $(OBJS_T2F_ENC)  $(OBJS_AAC_QC) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC)
# HP 970314
OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o  $(OBJS_T2F_ENC)  $(OBJS_AAC_QC) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC) $(OBJS_MOT_QC) 
LIBS_ENC_TF = $(LIBS_T2F_ENC) $(LIBS_VQ) $(LIBS_SON_AACPP_ENC)

# end of makefile.tf












