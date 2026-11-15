/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)

and edited by

Heiko Purnhagen (University of Hannover)

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

Copyright (c) 1998.




BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

**********************************************************************/

#ifndef _FLEX_MUX_H_INCLUDED
#define _FLEX_MUX_H_INCLUDED

#include "obj_descr.h"           /* typedef structs */


typedef enum {
  NULL_OBJECT	= 0,	/* NULL Object */
  AAC_MAIN 	= 1,	/* AAC Main Object */
  AAC_LC 	= 2,	/* AAC Low Complexity(LC) Object */
  AAC_SSR 	= 3,	/* AAC Scalable Sampling Rate(SSR) Object */
  AAC_LTP 	= 4,	/* AAC Long Term Predictor(LTP) Object */
  RSVD_5 	= 5,	/* (reserved) */
  AAC_SCAL 	= 6,	/* AAC Scalable Object */
  TWIN_VQ 	= 7,	/* TwinVQ Object */
  CELP 		= 8,	/* CELP Object */
  HVXC 		= 9,	/* HVXC Object */
  RSVD_10 	= 10,	/* (reserved) */
  RSVD_11 	= 11,	/* (reserved) */
  TTSI 		= 12,	/* TTSI Object(not supported) */
  MAIN_SYNTH 	= 13,	/* Main Synthetic Object(not supported) */
  WAV_TAB_SYNTH = 14,	/* Wavetable Synthesis Object(not supported) */
  GEN_MIDI 	= 15,	/* General MIDI Object(not supported) */
  ALG_SYNTH_AUD_FX = 16	/* Algorithmic Synthesis and Audio FX Object(not supported) */
  ,
  ER_AAC_LC	= 17,	/* Error Resilient(ER) AAC Low Complexity(LC) Object */
  RSVD_18 	= 18,	/* (reserved) */
  ER_AAC_LTP	= 19,	/* Error Resilient(ER) AAC Long Term Predictor(LTP) Object */
  ER_AAC_SCAL	= 20,	/* Error Resilient(ER) AAC Scalable Object */
  ER_TWIN_VQ	= 21,	/* Error Resilient(ER) TwinVQ Object */
  ER_BSAC	= 22,	/* Error Resilient(ER) BSAC Object */
  ER_AAC_LD	= 23,	/* Error Resilient(ER) AAC LD Object */
  ER_CELP	= 24,	/* Error Resilient(ER) CELP Object */
  ER_HVXC	= 25,	/* Error Resilient(ER) HVXC Object */
  ER_HILN	= 26,	/* Error Resilient(ER) HILN Object */
  ER_PARA	= 27,	/* Error Resilient(ER) Parametric Object */
  RSVD_28 	= 28,	/* (reserved) */
  RSVD_29 	= 29,	/* (reserved) */
  RSVD_30 	= 30,	/* (reserved) */
  RSVD_31 	= 31	/* (reserved) */
} AUDIO_OBJECT_TYPE_ID;

void advanceESDescr ( HANDLE_BSBITSTREAM       bitStream, 
                      ES_DESCRIPTOR*    es,
                      int               WriteFlag );
void initESDescr( ES_DESCRIPTOR **es);
void presetESDescr( ES_DESCRIPTOR *es, int numLayers);
void initObjDescr( OBJECT_DESCRIPTOR *od,int mainDebugLevel);
void presetObjDescr( OBJECT_DESCRIPTOR *od, int numLayers);

void getAccessUnit( HANDLE_BSBITSTREAM    bitStream,
                    HANDLE_BSBITBUFFER    AUBuffer,
                    unsigned int*  AUIndex,
                    unsigned long* totalLength,
                    ES_DESCRIPTOR  *es );

void  advanceODescr (   HANDLE_BSBITSTREAM       bitStream, OBJECT_DESCRIPTOR *od, int WriteFlag) ;
void writeFlexMuxPDU(int index,HANDLE_BSBITSTREAM  bitStream , HANDLE_BSBITBUFFER  AUBuffer);
int  nextAccessUnit( HANDLE_BSBITSTREAM       bitStream ,                      
                     unsigned int*     layer, 
                     FRAME_DATA*       frameData);

void putAccessUnit(int index,HANDLE_BSBITSTREAM  bitStream , HANDLE_BSBITBUFFER  AUBuffer, ES_DESCRIPTOR *es);
void    advanceTFspecConf (   HANDLE_BSBITSTREAM bitStream,TF_SPECIFIC_CONFIG *tfConf, int audioObjectType, int WriteFlag); 
void advanceCelpSpecConf ( HANDLE_BSBITSTREAM      bitStream,CELP_SPECIFIC_CONFIG *celpConf,int WriteFlag);

/* added by NTT 20001121 *//* moved from ntt_vq_coder.c 20011120 */
void ntt_writeFlexMuxPDU(
                         int index,
                         HANDLE_BSBITSTREAM bitStream,
                         int vq_bits,
                         HANDLE_BSBITBUFFER AUBuffer,
                         int idx_base
);

#ifndef CORRIGENDUM1
void advanceCelpEnhSpecConf ( HANDLE_BSBITSTREAM  bitStream,CELP_ENH_SPECIFIC_CONFIG *celpEnhConf,int WriteFlag);
#ifdef VERSION2
void advanceErCelpSpecConf ( HANDLE_BSBITSTREAM      bitStream,CELP_SPECIFIC_CONFIG *erCelpConf,int WriteFlag);
#endif
#endif
void advanceHvxcSpecConf(
HANDLE_BSBITSTREAM  bitStream,
HVXC_SPECIFIC_CONFIG *hvxcConf,
int WriteFlag);
#ifdef VERSION2
/* HP20001009 */
void advanceParaSpecConf(
HANDLE_BSBITSTREAM  bitStream,
PARA_SPECIFIC_CONFIG *paraConf,
int WriteFlag);
void advanceParaEnhSpecConf(
HANDLE_BSBITSTREAM  bitStream,
PARA_SPECIFIC_CONFIG *paraConf,
int WriteFlag);
#endif

void initTFspecConf ( TF_SPECIFIC_CONFIG *tfConf );
void initCelpSpecConf (CELP_SPECIFIC_CONFIG *celpConf
#ifdef VERSION2
                       ,
		       int audioObjectType
#endif /*VERSION2*/
                       );
#ifndef CORRIGENDUM1
void initCelpEnhSpecConf (CELP_ENH_SPECIFIC_CONFIG *celpEnhConf);
#endif
#if 0 /* funktionality is already included in initCelpSpecConf() */
void initErCelpSpecConf (ER_CELP_SPECIFIC_CONFIG *erCelpConf);
#endif
void initHvxcSpecConf (HVXC_SPECIFIC_CONFIG *hvxcConf);
#ifdef VERSION2
void initParaSpecConf (PARA_SPECIFIC_CONFIG *paraConf);
#endif

void removeAU(HANDLE_BSBITSTREAM  stream, int decBits, FRAME_DATA*        fd,int layer);

#endif
