/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



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



Header file: dec.h

$Id: dec.h,v 1.1.1.2 1998-05-07 17:21:21+03 suonio Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
18-jun-96   HP    first version
24-jun-96   HP    fixed comment
15-aug-96   HP    added DecXxxInfo(), DecXxxFree()
                  changed DecXxxInit(), DecXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
19-feb-97   HP    added include <stdio.h>
**********************************************************************/


#ifndef _dec_h_
#define _dec_h_

#include <stdio.h>              /* typedef FILE */

#include "bitstream.h"	/* bit stream module */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* DecParInfo() */
/* Get info about parametric decoder core. */

char *DecParInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecLpcInfo() */
/* Get info about LPC-based decoder core. */

char *DecLpcInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecTfInfo() */
/* Get info about t/f-based decoder core. */

char *DecTfInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecParInit() */
/* Init parametric decoder core. */

void DecParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  float speedFact,		/* in: speed change factor */
  float pitchFact,		/* in: pitch change factor */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample);		/* out: decoder delay (num samples) */


/* DecLpcInit() */
/* Init LPC-based decoder core. */

void DecLpcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample);		/* out: decoder delay (num samples) */


/* DecTfInit() */
/* Init t/f-based decoder core. */

void DecTfInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: decoder delay (num samples) */
  char *aacDebugStr
  );


/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core. */

void DecParFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample);	/* out: num samples in sampleBuf[][] */

/* DecLpcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* LPC-based decoder core. */

void DecLpcFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit);		/* out: num bits used for this frame */


/* DecTfFrame() */
/* Decode one bit stream frame into one audio frame with */
/* t/f-based decoder core. */

void DecTfFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit);		/* out: num bits used for this frame */


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree ();


/* DecLpcFree() */
/* Free memory allocated by LPC-based decoder core. */

void DecLpcFree ();


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ();


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_h_ */

/* end of dec.h */
