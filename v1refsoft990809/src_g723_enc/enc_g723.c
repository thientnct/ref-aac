/* $Id: enc_g723.c,v 1.5 1999/04/23 16:02:05 purnhage Exp $ */

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

/*******************************************************************************************
 *
 * Master module for G723 based codecs
 *
 ******************************************************************************************/
#include <stdio.h>
#include <string.h>

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

#include "dec_g723.h"
#include "typedef2.h"
#include "cst2.h"		/* for frame definition */
#include "g723_coder.h"
#include "../src_tf/tf_main.h" /* need MONO_CHAN */
#include "enc_g723.h"
#define PROGVER "ITU-T G.723.1 Floating Point Speech Coder ANSI C Source Code. Version 5.1F"

/* ITU-T G.723 variables: */


#if 0
enum  Crate   WrkRate = Rate63;

Flag  UseHp = True;
Flag  UsePf = True;
Flag  UseVx = False;
Flag  UsePr = True;

#endif
extern enum  Crate   WrkRate ;
extern Flag  UseHp ;
extern Flag  UsePf;
extern Flag  UseVx;
extern Flag  UsePr;

float nb_trame;

/*****************************************************************************************
 ***
 *** Function: EncG723Info
 ***
 *** Purpose:  Information about the G723-part of the VM
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

char *EncG723Info (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "encoder parameter string format:\n"
	    "possible options:\n"
	    "-Noh : Highpassfilter disabled \n"
	    "-Nop : Postfilter disabled \n"
	    "-v : VAD/CNG enabled \n"
	    "\n");
  }
  return PROGVER;
}


/*****************************************************************************************
 ***
 *** Function: EncG723Init
 ***
 *** Purpose:  Initialize the G723-part and the macro blocks of the G723 part of the VM
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

void EncG723Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequancy [Hz] */
  float bitRate,		/* in: bit rate [bit/sec] */
  char *encPara,		/* in: coder parameter string */
  long encNumSample,		/* in : number of samples to be encoded */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader)	/* out: header from bit stream */
{
  if( strstr( encPara, "-v" ) ) {
            UseVx = True ;
  }

  if( strstr( encPara, "-Noh" ) ) {
            UseHp = False;
  }

  if( strstr( encPara, "-Nop" ) ) {
            UsePf = False;
  }

  if (numChannel != 1)
    CommonExit(1,"EncG723Init: audio data has more the one channel (%d)",
	       numChannel);

  if (fSample != 8000.0)
    CommonExit(1,"EncG723Init: fsampe out of range (%f)",
	       fSample);

  if ((bitRate != 5333) && (bitRate != 6400.0))
    CommonExit(1,"EncG723Init: bitrate out of range (%f), must be 5333 or 6400",
	       bitRate);

   if(UsePf == 0)
       printf("EncG723Init: Postfilter disabled\n");
   else
       printf("EncG723Init: Postfilter enabled\n");

   if (bitRate == 6400)
   {
       WrkRate = Rate63;
       printf("EncG723Init: Rate 6.3 kb/s\n");
   }
   else
   {
       WrkRate = Rate53;
       printf("EncG723Init: Rate 5.3 kb/s\n");
   }
   if (UseHp == 0)
       printf("EncG723Init: Highpassfilter disabled\n");
   else
       printf("EncG723Init: Highpassfilter enabled\n");
   if (UseVx == 0)
       printf("EncG723Init: VAD/CNG disabled\n");
   else
       printf("EncG723Init: VAD/CNG enabled\n");

   g723_coder_init();

  printf("\n %ld \n",encNumSample);

  nb_trame = encNumSample / (float)(2*Frame);

  *frameNumSample = 2*Frame;
  *delayNumSample = 0;
}

/*****************************************************************************************
 ***
 *** Function:    EncG723Frame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on G723 coding
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

void EncG723Frame (
  float **sampleBuf,      /* in: audio frame samples                   */
                          /*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,    /* out: bit stream frame                     */
                          /*      or NULL during encoder start up      */
  int frameAvailNumBit,   /* in: total num bits available for          */
                          /*     this frame (incl. bit reservoir)      */
  int frameNumBit,        /* in: average num bits per frame            */
  int frameMaxNumBit,     /* in: max num bits per frame                */
  int numSample)	  /* in: number of sample read		       */
{
  BsBitStream *bs;
  Word16 new_speech[Frame];
  static int ancien;

  int i;
  int one;
  int two;

  one = 1;
  two = 1;

  printf("nb_trame : %f \n",nb_trame);

  if (nb_trame < 0.5) one = 0;
  if (nb_trame < 1) two = 0;

  nb_trame--;

  bs = BsOpenBufferWrite(bitBuf);

  if (one)
  {
    for (i=0; i<Frame; i++)
    {
       new_speech[i] = sampleBuf[MONO_CHAN][i];
    }
    g723_coder(bs,new_speech);
    printf("  1 ");
  }

  if (two)
  {
    for (i=0; i<Frame; i++)
    {
       new_speech[i] = sampleBuf[MONO_CHAN][i+Frame];
    }
    g723_coder(bs,new_speech);
    printf("  2 ");
  }

  printf("  OK \n");

  BsClose(bs);
}

