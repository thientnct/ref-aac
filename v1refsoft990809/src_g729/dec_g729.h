/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

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



Header file: dec_g729.h

$Id: dec_g729.h,v 1.2 1999/05/20 10:23:32 purnhage Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
18-jun-96   HP    first version
24-jun-96   HP    fixed comment
15-aug-96   HP    added DecXxxInfo(), DecXxxFree()
                  changed DecXxxInit(), DecXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
19-feb-97   HP    added include <stdio.h>
04-apr-97   BT    added DecG729Init() DecG729Frame()
09-apr-99   HP    added DecHvxcXxx() from dec_hvxc.h
22-apr-99   HP    created from dec.h
**********************************************************************/


#ifndef _dec_g729_h_
#define _dec_g729_h_


#include <stdio.h>              /* typedef FILE */
#include "bitstreamHandle.h"	/* bit stream handle */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* DecG729Info() */
/* Get info about G729 decoder core. */

char *DecG729Info (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecG729Init() */
/* Init G729-based decoder core. */

void DecG729Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample);		/* out: decoder delay (num samples) */


/* DecG729Frame() */
/* Decode one bit stream frame into one audio frame with */
/* G729-based decoder core. */

void DecG729Frame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int postProcMode);            /* in : 1 :normal postprocessing: 0 omit postprocessing for use in scaleable mode */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_g729_h_ */

/* end of dec_g729.h */
