/**********************************************************************
MPEG-4 Audio VM
Encoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Header file: enc_tf.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
SE    Sebastien Etienne, Jean Bernard Rault CCETT <jbrault@ccett.fr>

Changes:
14-jun-96   HP    first version
18-jun-96   HP    added bit reservoir handling
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
09-aug-96   HP    added EncXxxInfo(), EncXxxFree()
15-aug-96   HP    changed EncXxxInit(), EncXxxFrame() interfaces to
                  enable multichannel signals / float fSample, bitRate
26-aug-96   HP    CVS
19-feb-97   HP    added include <stdio.h>
08-apr-97   BT    added EncG729Init() EncG729Frame()
22-apr-99   HP    created from enc.h
**********************************************************************/


#ifndef _enc_tf_h_
#define _enc_tf_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"

#include "obj_descr.h"
#include "nttNew.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */

char *EncTfInfo (
  FILE *helpStream);		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
/* EncTfInit() */
/* Init t/f-based encoder core. */

void EncTfInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int quantDebugLevel,          /* in: quantDebugLevel */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,
  TF_DATA*    tfData,
  ntt_DATA*   nttData,
  int mainDebugLevel
);


/* EncTfFrame() */
/* Encode one audio frame into one bit stream frame with */
/* t/f-based encoder core. */

void EncTfFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  long bitRate,
  long fSampleRate,
  ENC_FRAME_DATA* frameData,
  TF_DATA*    tfData,
  ntt_DATA* nttData
);


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree (void);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _enc_tf_h_ */

/* end of enc_tf.h */
