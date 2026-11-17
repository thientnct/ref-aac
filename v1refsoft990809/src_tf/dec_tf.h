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



Header file: dec_tf.h

$Id: dec_tf.h,v 1.3 1999/07/22 18:15:21 purnhage Exp $

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
#include "bitstreamHandle.h"	/* bit stream handle */

#include "lpc_common.h"         /* TDS */
#include "nttNew.h"             /* TDS */
#include "all.h"
#include "tns.h"
#include "huffdec2.h"
#include "reorderspecHandle.h"
#include "concealmentHandle.h"
#include "flex_mux.h"
#include "buffersHandle.h"
#include "ntt_conf.h"
#include "dec_hvxc.h"


/* ---------- functions ---------- */


  typedef struct {    
    HANDLE_BUFFER      hVm;
    HANDLE_BUFFER      hHcrSpecData;
    HANDLE_RESILIENCE  hResilience;      /* in: handler for error resilience info */
    HANDLE_HCR         hHcrInfo;    
    HANDLE_EP_INFO     hEpInfo;
    HANDLE_CONCEALMENT hConcealment;        
  } FAULT_TOLERANT_DATA;


  typedef FAULT_TOLERANT_DATA* HANDLE_FAULT_TOLERANT;


#ifdef __cplusplus
extern "C" {
#endif


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
                 BsBitBuffer*        bitHeader,        /* in: header from bit stream */
                 int*                frameNumSample,   /* out: num samples per frame */
                 int*                delayNumSample,   /* out: encoder delay (num samples) */
                 char*               aacDebugStr,
                 HANDLE_BUFFER*      hVm,
                 HANDLE_BUFFER*      hHcrSpecData,
                 FRAME_DATA*         frameData,
                 TF_DATA*            tfData,
                 ntt_DATA*           nttData,
                 int                 mainDebugLevel
);

void DecTfInitNew (
                   char*               decPara,          /* in: decoder parameter string */    
                   char*               aacDebugStr,
                   HANDLE_BUFFER*      hVm,
                   HANDLE_BUFFER*      hHcrSpecData,
                   FRAME_DATA*         frameData,
                   TF_DATA*            tfData,
                   LPC_DATA*           lpcData,
                   ntt_DATA*           nttData,
                   int                 mainDebugLevel,
                   int                 layer
                 );


/* DecTfFrame() */
/* Decode one bit stream frame into one audio frame with */
/* t/f-based decoder core. */

void DecTfFrame ( BsBitBuffer*       bitBuf,           /* in: bit stream frame */
                  float**            sampleBuf,        /* out: frameNumSample audio samples */
                  int*               usedNumBit,       /* out: num bits used for this frame */
                  int                numChannels,
                  int                mainDebugLevel,
                  HANDLE_FAULT_TOLERANT hFault,   /**TM9902*/
		  HANDLE_RESILIENCE  hResilience,      /* in: handler for error resilience info */
                  HANDLE_BUFFER      hVm,
                  HANDLE_BUFFER      hHcrSpecData,
                  HANDLE_HCR         hHcrInfo ,
                  FRAME_DATA*        frameData,
                  TF_DATA*           tfData,
                  ntt_DATA*          nttData,
                  HANDLE_EP_INFO     hEpInfo,
                  HANDLE_CONCEALMENT hConcealment);

void DecTfFrameNew(int          numChannels, 
                   FRAME_DATA*  fd, /* config data , obj descr. etc. */
                   TF_DATA*     tfData, /* actual frame input/output data */
                   LPC_DATA*    lpcData,
                   ntt_DATA*    nttData,
                   HANDLE_FAULT_TOLERANT hFault,/* additional data for error protection, huffman reordering and 
                                     error conceilment etc. */
                   QC_MOD_SELECT      qc_select  
                   );


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */


void DecTfFree ( HANDLE_EP_INFO    hEpInfo,
                 HANDLE_RESILIENCE hResilience );


void tvqAUDecode(
       int          numChannels,
       FRAME_DATA*  fd, /* config data , obj descr. etc. */
       TF_DATA*     tfData,
       HANDLE_FAULT_TOLERANT hFault,
       QC_MOD_SELECT      qc_select,
       ntt_INDEX*   index,
       ntt_INDEX*   index_scl,
       BsBitStream* mat_layerStream 
       );
void tvqAUDecode2( 
                  FRAME_DATA*  fd, /* config data , obj descr. etc. */
                  TF_DATA*     tfData,
                  HANDLE_FAULT_TOLERANT hFault,
                  QC_MOD_SELECT      qc_select,
                  ntt_INDEX*   index,
                  ntt_INDEX*   index_scl,
                  int * nextLayer);

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _dec_tf_h_ */

/* end of dec_tf.h */
