/**********************************************************************
MPEG-4 Audio VM
Encoder cores (parametric, LPC-based, t/f-based)



SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Heiko Purnhagen (University of Hannover)

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.



Header file: enc.h

$Id: enc.h,v 1.7 1998/07/30 15:51:13 lueck Exp $

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
  BsBitBuffer *bitHeader,   /* out: header for bit stream */
  int icplenabl,			/* in : couple enable */
  int isdisabl,			/* in : intensity stereo enable */
  int msdisabl,			/* in : ms stereo enable */
  int tnsdisabl,			/* in : tns enable */
  int pnsSfbStart
#ifdef CT_SBR
  , int sbrenable
#ifdef PARAMETRICSTEREO
  , int sbrpsenable
#endif
#endif
);	


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
#ifdef CT_SBR
  float **sampleBufOrig,
#endif
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
							/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
							/*	   this frame (incl. bit reservoir) */
  int frameNumBit,			/* in: average num bits per frame */
  int minBitsNoOverflow,    /* in: minimum number of bits needed to avoid padding */
  int frameMaxNumBit,		/* in: max num bits per frame */
  int lfePresent,
  double *icplmix,			/* independent coupling coefficients */
  float *cplSampleBuf,		/* coupling channel PCM audio */ 
  DRC_INTF *dvars_inpt		/* dynamic range control params */
);              /* in: lfe channel present */  

/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree ();


/* EncLpcFree() */
/* Free memory allocated by LPC-based encoder core. */

void EncLpcFree ();


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree (void);

/* EncTfGetProfile() */
/* Return AAC profile selected by EncTfInit() (enum AAC_PROFILE). */

int EncTfGetProfile (void);

/* ByteAlign(...), used to byte align bitstream */
int ByteAlign(BsBitStream* ptrBs);

void couple_e(double *cplmix, short nfchans, float *cpltc);
void SetCplMix(double *cplmix, int numChannel);

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _enc_h_ */

/* end of enc.h */
