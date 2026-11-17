/* $Id: g723_coder.c,v 1.3 1999/04/22 16:20:05 purnhage Exp $ */

/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) in the course of 
 development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY : 
SE    Sebastien Etienne, Jean Bernard Rault, CCETT <jbrault@ccett.fr> */

/* 05-may-97   SE   first version. */
/* 13-may-97   SE   integration in the VM. */

/*****************************************************************************/

/* ITU-T G.723 includes: */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "buffersHandle.h"       /* handler, defines, enums */

#include "typedef2.h"
#include "cst2.h"
#include "bitstream.h"
#include "coder2.h"
#include "vad2.h"
#include "codcng2.h"
#include "util2.h"
extern Flag UseVx;

/* ITU-T G.723 decoder initialization function: */

void g723_coder_init(void)
{
    /* Local variable: */

    /* Initializations: */

    /*
     * Init coder and the decoder
     */
    Init_Coder();

    /* Init Comfort Noise Functions */
    if (UseVx) {
        Init_Vad();
        Init_Cod_Cng();
    }
}

/* ITU-T G.723 coder function: */

void g723_coder
(
        BsBitStream *bitStream,
        Word16      sig[]
)
{
    /* Local variable: */

    char   Line[24];
    int    i;
    FLOAT  DataBuff[Frame];

    for (i=0; i<Frame; i++)
    {
        DataBuff[i] = sig[i];
    }

    Coder(DataBuff, Line);
    Line_Wr(Line, bitStream);

    return;
}
