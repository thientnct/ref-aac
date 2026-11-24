/**********************************************************************
MPEG-4 Audio VM
Encoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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



Header file: enc.h

$Id: enc.h,v 1.1.1.2 1998-05-07 17:21:23+03 suonio Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
RG    Ralf Geiger, FhG/IIS

Changes:
14-jun-96   HP    first version
18-jun-96   HP    added bit reservoir handling
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
09-aug-96   HP    added EncXxxInfo(), EncXxxFree()
15-aug-96   HP    changed EncXxxInit(), EncXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
19-feb-97   HP    added include <stdio.h>
07-apr-98   RG    added argument lfePresent in EncTfFrame()
**********************************************************************/


#ifndef _enc_h_
#define _enc_h_

#include <stdio.h>              /* typedef FILE */

#include "bitstream.h"		/* bit stream module */

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* EncParInfo() */
/* Get info about parametric encoder core. */

char *EncParInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* EncLpcInfo() */
/* Get info about LPC-based encoder core. */

char *EncLpcInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */

char *EncTfInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* EncParInit() */
/* Init parametric encoder core. */

void EncParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader);	/* out: header for bit stream */


/* EncLpcInit() */
/* Init LPC-based encoder core. */

void EncLpcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader);	/* out: header for bit stream */


/* EncTfInit() */
/* Init t/f-based encoder core. */

void EncTfInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int quantDebugLevel,          /* in: quantDebugLevel */
  int bitReservInit,            /* in: initial bits in reservoir */
  int varBitRate,               /* in: 1 if variable bit rate, 0 if constant */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader);	/* out: header for bit stream */


/* EncParFrame() */
/* Encode one audio frame into one bit stream frame with */
/* parametric encoder core. */

void EncParFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit);		/* in: max num bits per frame */


/* EncLpcFrame() */
/* Encode one audio frame into one bit stream frame with */
/* LPC-based encoder core. */

void EncLpcFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit);		/* in: max num bits per frame */


/* EncTfFrame() */
/* Encode one audio frame into one bit stream frame with */
/* t/f-based encoder core. */

void EncTfFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int minBitsNoOverflow,        /* in: minimum number of bits needed to avoid padding */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int lfePresent);              /* in: lfe channel present */  

/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree ();


/* EncLpcFree() */
/* Free memory allocated by LPC-based encoder core. */

void EncLpcFree ();


/* EncTfGetProfile() */
/* Return AAC profile selected by EncTfInit() (enum AAC_PROFILE). */

int EncTfGetProfile (void);

/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree ();

/* ByteAlign(...), used to byte align bitstream */
int ByteAlign(BsBitStream* ptrBs);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _enc_h_ */

/* end of enc.h */
