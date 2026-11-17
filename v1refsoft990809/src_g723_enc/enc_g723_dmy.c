/**********************************************************************
MPEG-4 Audio VM
Coder core (G723-based)



This software module was originally developed by

Sebastien Etienne (CCETT)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.



Source file: dec_g723_dmy.c

$Id: enc_g723_dmy.c,v 1.6 1999/04/23 16:02:05 purnhage Exp $

Required modules:
common.o		common module
bitstream.o		bits stream module

Authors:
SE    Sebastien Etienne, Jean Bernard Rault, CCETT <jbrault@ccett.fr>

Changes:
05-may-97   SE    g723 core
13-may-97   SE   integration in the VM.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "common_m4a.h"		/* common module */
#include "bitstream.h"		/* bit stream module */

/* HP 971110 */
#include "enc_g723.h"		/* encoder cores */

#define PROGVER "ITU-T G.723.1 Floating Point Speech Coder ANSI C Source Code. Version 5.1F"

/* ---------- declarations ---------- */



/* ---------- functions ---------- */

/* EncG723Info() */
/* Get info about G723-based encoder core. */
char *EncG723Info (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,
	   "g723 encoder dummy module , g723 encoder not compiled in\n");
  return "g723 encoder dummy module";
}

/* EncG723Init() */
/* Init G723-based coder core. */

void EncG723Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: coder parameter string */
  long encNumSample,		/* in : number of samples to be encoded */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: decoder delay (num samples) */
  BsBitBuffer *bitHeader)	/* out: header from bit stream */
{
  CommonExit(1,"EncG723Init: dummy");
}


/* EncG723Frame() */
/* Code one bit stream frame into one audio frame with */
/* G723-based decoder core. */

void EncG723Frame (
  float **sampleBuf,      /* in: audio frame samples                   */
                          /*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,    /* out: bit stream frame                     */
                          /*      or NULL during encoder start up      */
  int frameAvailNumBit,   /* in: total num bits available for          */
                          /*     this frame (incl. bit reservoir)      */
  int frameNumBit,        /* in: average num bits per frame            */
  int frameMaxNumBit,    /* in: max num bits per frame                */
  int numSample)	  /* in: number of sample red		       */
{
  CommonExit(1,"EncG723Frame: dummy");
}




/* end of enc_g723_dmy.c */

