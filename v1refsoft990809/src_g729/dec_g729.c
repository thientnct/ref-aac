/* $Id: dec_g729.c,v 1.7 1999/05/20 10:23:32 purnhage Exp $ */

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

/* CREATED BY :  SE  Sebastien Etienne -- Mar-97  */

/* 26-mar-97   SE   first version. */

/*******************************************************************************************
 *
 * Master module for G729 based codecs
 *
 ******************************************************************************************/

#include <stdio.h>

/* MPEG-4 Audio VM includes: */

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "lpc_common.h"          /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "common_m4a.h"

#include "dec_g729.h"
#include "typedef.h"		/* For Word16 definition */
#include "ld8k.h"		/* For L_FRAME definition */
#include "g729_decoder.h"
#include "../src_tf/tf_main.h" /* need MONO_CHAN */

#define PROGVER "ITU-T G.729 Speech Decoder     ANSI-C Source Code  Version 3.3    Last modified: December 26, 1995"

char *DecG729Info (
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
 *** Function: DecG729Init
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

void DecG729Init ( int   numChannel,		/* in: num audio channels */
                   float fSample,		/* in: sampling frequancy [Hz] */
                   float bitRate,		/* in: bit rate [bit/sec] */
                   int*  frameNumSample,	/* out: num samples per frame */
                   int*  delayNumSample )	/* out: encoder delay (num samples) */
{
  if (numChannel != 1)
    CommonExit(1,"DecG729Init: audio data has more the one channel (%d)",
	       numChannel);

  if (fSample != 8000.0)
    CommonWarning("DecG729Init: fsampe out of range (%d)",
	       fSample);
  fSample = 8000.0;
  
  if (bitRate != 8000.0)
    CommonExit(1,"DecG729Init: bitrate out of range (%d)",
	       bitRate);

  g729_decoder_init();

  *frameNumSample = 160;
  *delayNumSample = 0;
}

/*****************************************************************************************
 ***
 *** Function:    DecG729Frame
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

void DecG729Frame (
		 BsBitBuffer *bitBuf,		/* in: bit stream frame */
		 float **sampleBuf,		/* out: frameNumSample audio samples */
		 int *usedNumBit,         	/* out: num bits used for this frame */
		 int postProcMode
		 )
{
  Word16 tab1[L_FRAME];
  Word16 tab2[L_FRAME];
  BsBitStream *bs;

  int i;

  bs = BsOpenBufferRead(bitBuf);

  g729_decoder(bs,tab1,postProcMode);
  
  for (i=0; i< L_FRAME; i++)
  {
     sampleBuf[MONO_CHAN][i] = (float)tab1[i];
  }

  g729_decoder(bs,tab2,postProcMode);
  
  for (i=0; i< L_FRAME; i++)
  {
     sampleBuf[MONO_CHAN][i+L_FRAME] = (float)tab2[i];
  }

  *usedNumBit = 160;
  BsClose(bs);
}

