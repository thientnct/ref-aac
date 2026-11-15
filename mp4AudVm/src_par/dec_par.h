/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Akira Inoue (Sony Corporation) and
Yuji Maeda (Sony Corporation)

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



Header file: dec_par.h

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


#ifndef _dec_par_h_
#define _dec_par_h_


#include <stdio.h>              /* typedef FILE */

#include "allHandles.h"

#include "dec_hvxc.h"


/* ---------- structures ---------- */

/* para decoder status struct   HP 20000330 */

typedef struct ParaDecStatusStruct ParaDecStatus;

struct ParaDecStatusStruct
{
  /* HP 20000330 */
  float **sampleBuf;
  int frameNumSample;

  int delayNumSample;
  int frameMaxNumSample;

  /* configuration variables */
  int dummy;
};


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* DecHvxcInfo() */
/* Get info about HVXC decoder core. */

char *DecHvxcInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */

/* DecHvxcInitNew() */
/* Init HVXC decoder core for system interface(AI 990616) */

HvxcDecStatus *DecHvxcInitNew (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer			/* in: number of layer */
#ifdef VERSION2		/* for "EP" tool (YM 990616) */
  , int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
#endif
);		

/* DecHvxcInit() */
/* Init HVXC decoder core. */

HvxcDecStatus *DecHvxcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  HANDLE_BSBITBUFFER bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample		/* out: decoder delay (num samples) */
#ifdef VERSION2		/* for "EP" tool (YM 990616) */
  , int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
#endif
  );

/* DecHvxcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core. */

void DecHvxcFrame (
  HvxcDecStatus *HDS,		/* in: HVXC decoder status handle */
  HANDLE_BSBITBUFFER bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor(AI 990209) */
  float pitchFact		/* in: pitch change factor(AI 990209) */
#ifdef VERSION2		/* for "EP" tool (YM 990616) */
  , int epFlag,			/* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
#endif
  );

/* DecHvxcFrameNew() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core - system interface(AI 990616) */

void DecHvxcFrameNew (
  FRAME_DATA*  fD,
  HvxcDecStatus* hvxcD,		/* in: HVXC decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  float speedFact,		/* in: speed change factor(AI 990127) */
  float pitchFact		/* in: pitch change factor(AI 990127) */
#ifdef VERSION2         /* for "EP" tool (YM 990616) */
  , int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
#endif
  );

/* DecHvxcFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree (
  HvxcDecStatus *HDS		/* in: HVXC decoder status handle */
);


/* DecParInfo() */
/* Get info about parametric decoder core. */

char *DecParInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecParInit() */
/* Init parametric decoder core. */

void DecParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  HANDLE_BSBITBUFFER bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample		/* out: decoder delay (num samples) */
#ifdef VERSION2		/* for "EP" tool (YM 990616) */
  , int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData, 	/* out: ep-information */
  int enhaFlag,			/* in: enhaFlag or -1 for decPara */
  int numExtLayer		/* in: numExtLayer or -1 for decPara */
#endif
  );

/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core. */

void DecParFrame (
  HANDLE_BSBITBUFFER bitBuf,	/* in: bit stream frame */
  HANDLE_BSBITSTREAM *stream,   /* in: NULL or bit stream(s) */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor(AI 990209) */
  float pitchFact		/* in: pitch change factor(AI 990209) */
#ifdef VERSION2		/* for "EP" tool (YM 990616) */
  , int epFlag,			/* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
#endif
  );


#ifdef VERSION2
/* DecParInitNew() */
/* Init parametric decoder core for system interface   HP 20000330 */

ParaDecStatus *DecParInitNew (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
);		

/* DecParFrameNew() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core - system interface   HP 20000330 */

void DecParFrameNew (
  FRAME_DATA*  fD,
  ParaDecStatus* paraD,		/* in: para decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  float speedFact,		/* in: speed change factor(AI 990127) */
  float pitchFact,		/* in: pitch change factor(AI 990127) */
  int epFlag,                   /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  );
#endif


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree (void);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_par_h_ */

/* end of dec_par.h */



