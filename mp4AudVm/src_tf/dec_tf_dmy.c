/**********************************************************************
MPEG-4 Audio VM
Decoder core (t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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



Source file: dec_tf_dmy.c

 

Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "lpc_common.h"          /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"             /* common module */
#include "cmdline.h"            /* command line module */
#include "bitstream.h"          /* bit stream module */

#include "dec_tf.h"                /* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "t/f-based decoder core dummy"


/* ---------- functions ---------- */


/* DecTfInfo() */
/* Get info about t/f-based decoder core. */

char *DecTfInfo (
  FILE *helpStream)             /* in: print decPara help text to helpStream */
                                /*     if helpStream not NULL */
                                /* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


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
)
{
  CommonExit(1,"DecTfInit: dummy");
}

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
                 )
{
  CommonExit(1,"DecTfInitNew: dummy");
}

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
                  ntt_DATA*          nttData )
{
  CommonExit(1,"DecTfFrame: dummy");
}

void DecTfFrameNew(
                   int          numChannels, 
                   FRAME_DATA*  fd, /* config data ,one bitstream buffer for each layer, obj descr. etc. */
                   TF_DATA*     tfData,
                   LPC_DATA*    lpcData,
                   ntt_DATA*    nttData,
                   HANDLE_DECODER_GENERAL hFault,
                   QC_MOD_SELECT      qc_select  
)
{
  CommonExit(1,"DecTfFrameNew: dummy");
}


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ( 
#ifdef VERSION2
                HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
                HANDLE_RESILIENCE hResilience 
#endif
                )
{
  CommonExit(1,"DecTfFree: dummy");
}


/* HP 20010130 */
int get_prog_config_from_bs ( ProgConfig*       p,
                              int               block_size_samples,
#ifdef VERSION2
                              HANDLE_RESILIENCE hResilience, 
                              HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                              HANDLE_BSBITSTREAM bs )
{
  CommonExit(1,"get_prog_config_from_bs: dummy");
}

unsigned long get_channel_config_from_progConfig (ProgConfig* p)
{
  CommonExit(1,"get_channel_config_from_progConfig: dummy");
}

/* end of dec_tf_dmy.c */

