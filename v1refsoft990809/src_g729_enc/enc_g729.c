/* $Id: enc_g729.c,v 1.7 1999/04/23 16:02:05 purnhage Exp $ */

/************************* MPEG  Audio Decoder **************************
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

/* CREATED BY :  SE  Sebastien Etienne -- Apr-97  */

/* 08-apr-97   SE   first version. */

/*******************************************************************************************
 *
 * Master module for G729 based codecs
 *
 ******************************************************************************************/
#include <stdio.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "common_m4a.h"

#include "enc_g729.h"
#include "typedef.h"		/* For Word16 definition */
#include "ld8k.h"		/* For L_FRAME definition */
#include "g729_coder.h"
#include "../src_tf/tf_main.h" /* need MONO_CHAN */

#define PROGVER "ITU-T G.729 Speech Coder     ANSI-C Source Code  Version 3.3    Last modified: December 26, 1995"


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */
char *EncG729Info (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER 
	   );
  }
  return  PROGVER;
}
/*****************************************************************************************
 ***
 *** Function: EncG729Init
 ***
 *** Purpose:  Initialize the G729-part and the macro blocks of the G729 part of the VM
 ***
 *** Description:
 *** 
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/
void EncG729Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: decoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: decoder delay (num samples) */
  BsBitBuffer *bitHeader)	/* out: header from bit stream */
{
  if (numChannel != 1)
    CommonExit(1,"EncG729Init: audio data has more the one channel (%d)",
	       numChannel);

  if (fSample != 8000.0)
    CommonExit(1,"EncG729Init: fsamlpe out of range (%d)",
	       fSample);

  if (bitRate != 8000.0)
    CommonExit(1,"EncG729Init: bitrate out of range (%d)",
	       bitRate);

   g729_coder_init();

  *frameNumSample = 160;
  *delayNumSample = 0;

  printf("EncG729Init: OK \n");
}

/*****************************************************************************************
 ***
 *** Function:    EncG729Frame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on G729 encoding 
 ***
 *** Description:
 *** 
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:  returns the number of used bits
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/

void EncG729Frame (
  float **sampleBuf,      /* in: audio frame samples                   */
                          /*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,    /* out: bit stream frame                     */
                          /*      or NULL during encoder start up      */
  int frameAvailNumBit,   /* in: total num bits available for          */
                          /*     this frame (incl. bit reservoir)      */
  int frameNumBit,        /* in: average num bits per frame            */
  int frameMaxNumBit,     /* in: max num bits per frame                */
  int numSample)	  /* in: number of sample red		       */
{
  BsBitStream *bs;
  extern Word16 *new_speech;

  int i;

  bs = BsOpenBufferWrite(bitBuf);


  for (i=0; i<L_FRAME; i++)
  {
     new_speech[i] = (short int)sampleBuf[MONO_CHAN][i];
  }
  g729_coder(bs,new_speech);

/*   printf("NUM %d \n",numSample); */

  if (numSample == frameNumBit)
  {
    for (i=0; i<L_FRAME; i++)
    {
       new_speech[i] =  (short int)sampleBuf[MONO_CHAN][i+L_FRAME];
    }
    g729_coder(bs,new_speech);
  }

  BsClose(bs);
}

