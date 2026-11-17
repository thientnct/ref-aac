/*******************************************************************************
*                                                                              *
* Interface between the MPEG-4 Audio VM and the ITU-T G.729 8 kbit/s decoder   *
*                                                                              *
********************************************************************************

This software module was originally developed by
Luca Cellario (CSELT)
and edited by
Bernhard Grill (University of Erlangen)
Jean-Bernard Rault (CCETT)
Bodo Teichmann (FhG)
in the course of development of the MPEG-4 Audio (ISO/IEC 14496-3).
This software module is an implementation of a part of one or more MPEG-4 Audio
(ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio (ISO/IEC 14496-3).
ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC 14496-3) free license to this
software module or modifications thereof for use in hardware or software
products claiming conformance to the MPEG-4 Audio (ISO/IEC 14496-3).
Those intending to use this software module in hardware or software products are
advised that its use may infringe existing patents.
The original developer of this software module and his/her company, the
subsequent editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.
The original developers retain full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit third parties
from using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (C) 1997.

Some routines and include files called in this software are part of
the fixed point implementation of the ITU-T G.729 standard, version 3.3. */

#include <stdio.h>

/* MPEG-4 Audio VM includes: */

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"

/* ITU-T G.729 includes: */

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"
#include "common_m4a.h"
/* ITU-T G.729 variables: */

static Word16 prm[PRM_SIZE+1];          /* parameters */
static Word16 rec_sig[L_FRAME+M], *rec; /* reconstructed signal */
static Word16 coef[MP1*2], *coef_p;     /* coefficients for postfilter */
static Word16 pitch;                    /* pitch lag in first subframe */
static Word16 voicing;                  /* voicing from previous frame */
static Word16 voicing_sf;               /* voicing for subframe */
static Word16 bitsno[PRM_SIZE]={1+NC0_B,NC1_B*2,8,1,13,4,7,5,13,4,7}; /*
                                           number of bits for each parameter */

/* ITU-T G.729 decoder initialization function: */

void g729_decoder_init(void)
{
    /* Local variable: */

    int i; /* index */

    /* Initializations: */

    for (i=0; i<M; i++)
        rec_sig[i]=(Word16)0;
    rec=rec_sig+M;

    Init_Decod_ld8k();
    Init_Post_Filter();
    Init_Post_Process();

    voicing=60;
}

/* ITU-T G.729 decoder function: */

void g729_decoder
(
    BsBitStream *bs,  /* input bitstream */
    Word16      sig[], /* output signal */
    int         postProcMode        /* if 0: use some float optimizations; if 1: identical output to ITU-T code */
)
{
    /* Local variable: */

    int i; /* index */

    /* Bitstream to parameters decoding: */

    for (i=0; i<PRM_SIZE; i++)
        BsGetBitShort(bs,(unsigned short *)&prm[i+1],bitsno[i]);

    prm[0]=0;
    prm[4]=Check_Parity_Pitch(prm[3],prm[4]);

    /* Parameters to signal decoding: */

    Decod_ld8k(prm,voicing,rec,coef,&pitch);

    /* Signal postfiltering: */

    voicing=0;
    coef_p=coef;
    for(i=0; i<L_FRAME; i+=L_SUBFR)
    {
        Post(pitch,&rec[i],coef_p,&sig[i],&voicing_sf);
        if (voicing_sf!=0)
            voicing=voicing_sf;
        coef_p+=MP1;
    }
    Copy(&rec_sig[L_FRAME],&rec_sig[0],M);
    
    switch( postProcMode ) {
    case 0: /* orig. high-pass filter */
      Post_Process(sig,L_FRAME);
      break;
    case 1: /* no high-pass filter, this mode is used only for adding with high layer signal */
      for( i=0; i<L_FRAME; i++ ) {
        sig[i] <<= 1;
      }
      break;
    default:
      CommonExit(-1,"\nerror in g729: illegal identical mode\n");
  }
}
