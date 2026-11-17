/* $Id: dec_g723.c,v 1.7 1999/05/20 10:23:32 purnhage Exp $ */

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

/* 26-mar-97   SE   first version. */
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
#include "g723_decoder.h"
#include "../src_tf/tf_main.h" /* need MONO_CHAN */

#define PROGVER "ITU-T G.723.1 Floating Point Speech Coder ANSI C Source Code. Version 5.1F"

/* ITU-T G.723 variables: */

enum  Crate   WrkRate = Rate63;

Flag  UseHp = True;
Flag  UsePf = True;
Flag  UseVx = False;
Flag  UsePr = True;

/*****************************************************************************************
 ***
 *** Function: DecG723Info
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

char *DecG723Info (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "decoder parameter string format:\n"
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
 *** Function: DecG723Init
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

void DecG723Init ( int   numChannel,		/* in: num audio channels */
                   float fSample,		/* in: sampling frequancy [Hz] */
                   float bitRate,		/* in: bit rate [bit/sec] */
                   char* decPara,		/* in: decoder parameter string */
                   int*  frameNumSample,	/* out: num samples per frame */
                   int*  delayNumSample )	/* out: encoder delay (num samples) */
{
  if( strstr( decPara, "-v" ) ) {
            UseVx = True ;
  }

  if( strstr( decPara, "-Noh" ) ) {
            UseHp = False;
  }

  if( strstr( decPara, "-Nop" ) ) {
            UsePf = False;
  }

  if (numChannel != 1)
    CommonExit(1,"EncG723Init: audio data has more the one channel (%d)",
	       numChannel);

  if (fSample != 8000.0)
    CommonExit(1,"EncG723Init: fsampe out of range (%f)",
	       fSample);

  if ((bitRate != 5334) && (bitRate != 6400.0))
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

   g723_decoder_init();

  *frameNumSample = Frame;
  *delayNumSample = 0;
}

/*****************************************************************************************
 ***
 *** Function:    DecG723Frame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on G723 encoding 
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

void DecG723Frame (
		 BsBitBuffer *bitBuf,		/* in: bit stream frame */
		 float **sampleBuf,		/* out: frameNumSample audio samples */
		 int *usedNumBit         	/* out: num bits used for this frame */
		 )
{
  Word16 tab1[Frame];
  BsBitStream *bs;

  int i;
  int nb;

  nb = 0;

  bs = BsOpenBufferRead(bitBuf);

  g723_decoder(bs,tab1);
  {
    for (i=0; i< Frame; i++)
    {
      sampleBuf[MONO_CHAN][i] = (float)tab1[i];
    }
    nb = 1;
  }
  if ( WrkRate == Rate63)
    *usedNumBit = nb*192; /* 24*8 = 192*/
  else
    *usedNumBit = nb*160; /* 20*8 = 160*/
  
/*   g723_decoder(bs,tab1); */
/*   { */
/*     for (i=0; i< Frame; i++) */
/*       { */
/*         sampleBuf[MONO_CHAN][i+Frame] = (float)tab1[i]; */
/*       } */
/*     nb = 2; */
/*   } */
  
  /*   printf("NB :%d \n",nb); */
  
/*   if ( WrkRate == Rate63) */
/*     *usedNumBit += nb*192; / 24*8 = 192*/ 
/*   else */
/*     *usedNumBit += nb*160; / 20*8 = 160*/ 
  
  BsClose(bs);
}

