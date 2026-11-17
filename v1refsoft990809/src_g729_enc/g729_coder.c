/*******************************************************************************
*                                                                              *
* Interface between the MPEG-4 Audio VM and the ITU-T G.729 8 kbit/s coder     *
*                                                                              *
********************************************************************************

This software module was originally developed by
Sebastien Etienne (CCETT)
and edited by
Jean-Bernard Rault (CCETT)
Luca Cellario (CSELT)
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
#include "tab_ld8k.h"

/* ITU-T G.729 variables: */

static Word16 prm[PRM_SIZE];    /* parameters */
static Word16 rec_sig[L_FRAME]; /* reconstructed signal */

/* ITU-T G.729 coder initialization function: */

void g729_coder_init(void)
{
    /* Local variable: */

    int i; /* index */

    /* Initializations: */

    Init_Pre_Process();
    Init_Coder_ld8k();

    for (i=0; i<PRM_SIZE; i++)
        prm[i]=(Word16)0;
}

/* ITU-T G.729 coder function: */

void g729_coder
(
    BsBitStream *bs,  /* output bitstream */
    Word16      sig[] /* input signal */
)
{
    /* Local variable: */

    int i; /* index */

    /* Signal preprocessing: */

    Pre_Process(sig,L_FRAME);

    /* Signal to parameters coding: */

    Coder_ld8k(prm,rec_sig);

    /* Parameters to bitstream coding: */

    for (i=0; i<PRM_SIZE; i++)
    {
        BsPutBit(bs,prm[i],bitsno[i]);
/*        printf("[%d] : %d %d \n",i,prm[i],bitsno[i]); */
    }
}
