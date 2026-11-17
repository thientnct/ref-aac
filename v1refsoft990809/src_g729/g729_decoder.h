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
Copyright (C) 1997. */

/* ITU-T G.729 function prototypes: */

void g729_decoder_init(void);

void g729_decoder
(
    BsBitStream *bs,  /* input bitstream */
    Word16      sig[], /* output signal */
    int         identical_mode        /* if 0: use some float optimizations; if 1: identical output to ITU-T code */
);
