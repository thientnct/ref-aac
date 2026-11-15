/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



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



Header file: dec_tf.h

 

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


#ifndef _dec_tf_h_
#define _dec_tf_h_


#include <stdio.h>              /* typedef FILE */
#include "allHandles.h"

#include "lpc_common.h"         /* TDS */
#include "nttNew.h"             /* TDS */
#include "all.h"
#include "tns.h"
#include "huffdec2.h"
#ifdef VERSION2
#include "resilience.h"
#endif
#include "flex_mux.h"
#include "ntt_conf.h"
#include "dec_hvxc.h"






#ifdef __cplusplus
extern "C" {
#endif
#define MAX_AU 12 
#define MAX_BITBUF 256000

/* ---------- functions ---------- */
/* DecTfInfo() */
/* Get info about t/f-based decoder core. */

char *DecTfInfo (
  FILE *helpStream);		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */


/* DecTfInit() */
/* Init t/f-based decoder core. */

void DecTfInit ( int                 numChannel,       /* in: num audio channels */
                 float               fSample,          /* in: sampling frequancy [Hz] */
                 float               bitRate,          /* in: bit rate [bit/sec] */
                 char*               decPara,          /* in: decoder parameter string */
                 int                 profile,          /* in: bitstream profile as indicated in the ADIF header */
                 HANDLE_BSBITBUFFER         bitHeader,        /* in: header from bit stream */
                 int*                frameNumSample,   /* out: num samples per frame */
                 int*                delayNumSample,   /* out: encoder delay (num samples) */
                 char*               aacDebugStr,
                 HANDLE_DECODER_GENERAL hFault,
                 FRAME_DATA*         frameData,
                 TF_DATA*            tfData,
                 ntt_DATA*           nttData,
                 int                 mainDebugLevel
#ifdef VERSION2
                 ,int                epDebugLevel,
                 char*               infoFileName,
                 char*               audioFileName
#endif
);

void DecTfInitNew (
                   char*               decPara,          /* in: decoder parameter string */    
                   char*               aacDebugStr,
                   HANDLE_DECODER_GENERAL hFault,
                   FRAME_DATA*         frameData,
                   TF_DATA*            tfData,
                   LPC_DATA*           lpcData,
                   ntt_DATA*           nttData,
                   int                 mainDebugLevel,
                   int                 layer
#ifdef VERSION2
                   ,int                epDebugLevel,
                   char*               infoFileName,
                   char*               audioFileName
#endif
                 );


/* DecTfFrame() */
/* Decode one bit stream frame into one audio frame with */
/* t/f-based decoder core. */

void DecTfFrame ( HANDLE_BSBITBUFFER        bitBuf,           /* in: bit stream frame */
                  float**            sampleBuf,        /* out: frameNumSample audio samples */
                  int*               usedNumBit,       /* out: num bits used for this frame */
                  int                numChannels,
                  int                mainDebugLevel,
                  HANDLE_DECODER_GENERAL hFault,   /**TM9902*/
                  FRAME_DATA*        frameData,
                  TF_DATA*           tfData,
                  ntt_DATA*          nttData );

void DecTfFrameNew(int          numChannels, 
                   FRAME_DATA*  fd, /* config data , obj descr. etc. */
                   TF_DATA*     tfData, /* actual frame input/output data */
                   LPC_DATA*    lpcData,
                   ntt_DATA*    nttData,
                   HANDLE_DECODER_GENERAL hFault,/* additional data for error protection, huffman reordering and 
                                     error conceilment etc. */
                   QC_MOD_SELECT      qc_select  
                   );


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ( 
#ifdef VERSION2
                HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                HANDLE_RESILIENCE hResilience
#endif /*VERSION2*/
                );


void tvqAUDecode(
       int          numChannels,
       FRAME_DATA*  fd, /* config data , obj descr. etc. */
       TF_DATA*     tfData,
       HANDLE_DECODER_GENERAL hFault,
       QC_MOD_SELECT      qc_select,
       ntt_INDEX*   index,
       ntt_INDEX*   index_scl,
       HANDLE_BSBITSTREAM  mat_layerStream 
       );
void tvqAUDecode2( 
                  FRAME_DATA*  fd, /* config data , obj descr. etc. */
                  TF_DATA*     tfData,
                  HANDLE_DECODER_GENERAL hFault,
                  QC_MOD_SELECT      qc_select,
                  ntt_INDEX*   index,
                  ntt_INDEX*   index_scl,
                  int * nextLayer);

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_tf_h_ */

/* end of dec_tf.h */
