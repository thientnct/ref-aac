/* $Id: g723_decoder.c,v 1.3 1999/04/22 16:20:05 purnhage Exp $ */

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
#include "decod2.h"
#include "deccng2.h"
#include "util2.h"

FILE  *Fep  = NULL;     /* Frame erasures file pointer */
long  FrCnt = 0;
long  FlLen;

/* ITU-T G.723 decoder initialization function: */

void g723_decoder_init(void)
{
    /* Local variable: */

    /* Initializations: */

    /*
     * Init decoder
     */
    Init_Decod();

    /* Init Comfort Noise Functions */
    Init_Dec_Cng();
}

/* ITU-T G.723 decoder function: */

void g723_decoder
(
        BsBitStream *bitStream,
        Word16      sig[]
)
{
    /* Local variable: */

    FLOAT  DataBuff[Frame];
    char   Line[24];
    Word16 Crc;
    int    i;

    /* Bitstream to parameters decoding: */

    if (Line_Rd(Line, bitStream) == (-1)) {
        return;
    }
    if (Fep == NULL)
        Crc = (Word16) 0;
    else
        fread((char*)&Crc, sizeof(Word16), 1, Fep);
    Decod(DataBuff, Line, Crc);

    for (i=0; i<Frame; i++)
    {
        if (DataBuff[i] < (FLOAT)-32767.5)
            sig[i] = -32768L;
        else if (DataBuff[i] > (FLOAT)32766.5)
            sig[i] = 32767;
        else
        {
            if (DataBuff[i] < 0)
                sig[i] = (Word16) (DataBuff[i]-(FLOAT)0.5);
            else
                sig[i] = (Word16) (DataBuff[i]+(FLOAT)0.5);
        }
    }
    return;
}
