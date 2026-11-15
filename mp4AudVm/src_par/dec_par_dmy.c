/**********************************************************************
MPEG-4 Audio VM
Decoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

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



Source file: dec_par_dmy.c

 

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "lpc_common.h"
#include "obj_descr.h"		/* AI 990616 */
#include "dec_par.h"		/* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "parametric decoder core dummy"


/* ---------- functions ---------- */


/* DecParInfo() */
/* Get info about parametric decoder core. */

char *DecParInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


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
  )
{
  CommonExit(1,"DecHvxcInitNew: dummy");

  return(NULL);
}


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
#ifdef VERSION2         /* for "EP" tool (YM 990616) */
  , int epFlag,                 /* in: error protection flag */
  int epDebugLevel,             /* in: debug level */
  char* infoFileName,           /* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData,       /* out: ep-information */
  int enhaFlag,			/* in: enhaFlag or -1 for decPara */
  int numExtLayer		/* in: numExtLayer or -1 for decPara */
#endif
)
{
  CommonExit(1,"DecParInit: dummy");
}


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
#ifdef VERSION2         /* for "EP" tool */
  , int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
#endif
)
{
  CommonExit(1,"DecParFrame: dummy");
}


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree (void)
{
  CommonExit(1,"DecParFree: dummy");
}


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
#ifdef VERSION2         /* for "EP" tool */
  , int epFlag,                 /* in: error protection flag */
  int epDebugLevel,             /* in: debug level */
  char* infoFileName,           /* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData       /* out: ep-information */
#endif
)
{
  CommonExit(1,"DecParInit: dummy");
  return ( 0 );
}


/* DecHvxcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core. */

void DecHvxcFrame (
  HvxcDecStatus *hvxcD,		/* in: HVXC decoder status handle */
  HANDLE_BSBITBUFFER bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor(AI 990127) */
  float pitchFact		/* in: pitch change factor(AI 990127) */
#ifdef VERSION2         /* for "EP" tool */
  , int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
#endif
)
{
  CommonExit(1,"DecHvxcFrame: dummy");
}

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
  )
{
  CommonExit(1,"DecHvxcFrameNew: dummy");
}


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
  )
{
  fprintf(stderr,"DecParInitNew: dummy\n");
}		

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
  )
{
  fprintf(stderr,"DecParFrameNew: dummy\n");
}
#endif


/* DecHvxcFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree(
HvxcDecStatus *hvxcD		/* in: HVXC decoder status handle */
) 
{
  CommonExit(1,"DecHvxcFree: dummy");
}


/* end of dec_par_dmy.c */

