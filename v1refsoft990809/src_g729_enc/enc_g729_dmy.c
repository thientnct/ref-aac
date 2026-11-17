/**********************************************************************
MPEG-4 Audio VM
Coder core (G729-based)



This software module was originally developed by

Sebastien Etienne (University of Hannover)

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

Copyright (c) 1996.



Source file: cod_g729_dmy.c

$Id: enc_g729_dmy.c,v 1.7 1999/04/23 16:02:05 purnhage Exp $

Required modules:
common.o		common module
bitstream.o		bits stream module

Authors:
SE    Sebastien Etienne, CCETT Rennes

Changes:
08-apr-97   SE    g729 core
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "common_m4a.h"		/* common module */
#include "bitstream.h"		/* bit stream module */

#include "enc_g729.h"		/* decoder cores */


/* ---------- declarations ---------- */



/* ---------- functions ---------- */

/* EncG729Init() */
/* Init G729-based encoder core. */

/* EncTfInfo() */
/* Get info about t/f-based encoder core. */
char *EncG729Info (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	   "g729 encoder dummy module , g729 encoder not compiled in\n");
  }
  return "g729 encoder dummy module";
}

void EncG729Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: decoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: decoder delay (num samples) */
  BsBitBuffer *bitHeader)	/* out: header from bit stream */
{
  CommonExit(1,"EncG729Init: dummy module , g729 encoder not compiled in");
}


/* EncG729Frame() */
/* Encode one bit stream frame into one audio frame with */
/* G729-based decoder core. */

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
  CommonExit(1,"EncG729Frame: dummy module , g729 encoder not compiled in");
}




/* end of enc_g729_dmy.c */

