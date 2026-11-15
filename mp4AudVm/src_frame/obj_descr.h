/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)
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

Copyright (c) 1998.




BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

**********************************************************************/

#ifndef _OBJ_DESCR_H_INCLUDED
#define _OBJ_DESCR_H_INCLUDED

#include "allHandles.h"

typedef unsigned char UINT8;
typedef unsigned long UINT32;


typedef struct  {
        UINT8  length ;/* in bits */
        UINT32 value;
} DESCR_ELE;



typedef struct {
 DESCR_ELE  TFCodingType ;
 DESCR_ELE  frameLength     ;
 DESCR_ELE  dependsOnCoreCoder;
 DESCR_ELE  coreCoderDelay;
 DESCR_ELE  extension;
 /* no channel config yet */
 DESCR_ELE  numOfSubFrame;
 DESCR_ELE  layer_length;
 DESCR_ELE  aacSectionDataResilienceFlag;
 DESCR_ELE  aacScalefactorDataResilienceFlag;
 DESCR_ELE  aacSpectralDataResilienceFlag;
 DESCR_ELE  extension3;
} TF_SPECIFIC_CONFIG;

typedef struct {
  DESCR_ELE  excitationMode ;
  DESCR_ELE  sampleRateMode ;
  DESCR_ELE  fineRateControl ;

  DESCR_ELE  RPE_Configuration ;

  DESCR_ELE  MPE_Configuration ;
  DESCR_ELE  numEnhLayers ;
  DESCR_ELE  bandwidthScalabilityMode ;
#ifdef VERSION2
  DESCR_ELE  silenceCompressionSW;
#endif

#ifdef CORRIGENDUM1
  DESCR_ELE  BWS_Configuration ;
  DESCR_ELE  isBaseLayer;	/* 14496-3 COR1 */
  DESCR_ELE  isBWSLayer;	/* 14496-3 COR1 */
  DESCR_ELE  CELP_BRS_id;	/* 14496-3 COR1 */
#endif

 /* no channel config yet */
} CELP_SPECIFIC_CONFIG;

#ifndef CORRIGENDUM1
typedef struct {
  DESCR_ELE  BWS_Configuration ;
} CELP_ENH_SPECIFIC_CONFIG;
#endif

#if 0 /* allready contained in CELP_SPECIFIC_CONFIG see above*/ 
typedef struct {
  DESCR_ELE  excitationMode ;
  DESCR_ELE  sampleRateMode ;
  DESCR_ELE  fineRateControl ;

  DESCR_ELE  silenceCompressionSW;

  DESCR_ELE  RPE_Configuration ;

  DESCR_ELE  MPE_Configuration ;
  DESCR_ELE  numEnhLayers ;
  DESCR_ELE  bandwidthScalabilityMode ;

 /* no channel config yet */
} ER_CELP_SPECIFIC_CONFIG;
#endif

#ifdef VERSION2
/* HP 20000330 */
typedef struct {
#ifdef CORRIGENDUM1
  DESCR_ELE  isBaseLayer;	/* 14496-3 COR1 HP20001023 */
#endif
  /* base layer */
  DESCR_ELE  PARAmode;

  DESCR_ELE  HVXCvarMode;
  DESCR_ELE  HVXCrateMode;
  DESCR_ELE  extensionFlag;
  DESCR_ELE  vrScalFlag;

  DESCR_ELE  HILNquantMode;
  DESCR_ELE  HILNmaxNumLine;
  DESCR_ELE  HILNsampleRateCode;
  DESCR_ELE  HILNframeLength;
  DESCR_ELE  HILNcontMode;

  DESCR_ELE  PARAextensionFlag;

  /* enha/ext layer(s) */
  DESCR_ELE  HILNenhaLayer;
  DESCR_ELE  HILNenhaQuantMode;
} PARA_SPECIFIC_CONFIG;
#endif

/* AI 990616 */
typedef struct {
#ifdef CORRIGENDUM1
  DESCR_ELE  isBaseLayer;	/* 14496-3 COR1 (AI 2000/10) */
#endif
  DESCR_ELE  HVXCvarMode;
  DESCR_ELE  HVXCrateMode;
  DESCR_ELE  extensionFlag;
#ifdef VERSION2
#if 0
  DESCR_ELE  packMode;
#endif
  DESCR_ELE  vrScalFlag;      /* VR scalable mode (YM 990728) */
#endif
} HVXC_SPECIFIC_CONFIG;



typedef struct {
  DESCR_ELE  audioDecoderType        ;
  DESCR_ELE  samplingFreqencyIndex; /* HP20001010: "u" missing ;-) */
  DESCR_ELE  samplingFrequency; /* HP20001010 */
  DESCR_ELE  channelConfiguration;
#ifndef CORRIGENDUM1
  int        BWS_on; /* CelpBandWidthScalablity on ? */
#endif

 union {
    TF_SPECIFIC_CONFIG TFSpecificConfig;
    CELP_SPECIFIC_CONFIG celpSpecificConfig;
#ifndef CORRIGENDUM1
    CELP_ENH_SPECIFIC_CONFIG celpEnhSpecificConfig;
#endif
#ifdef VERSION2
    PARA_SPECIFIC_CONFIG paraSpecificConfig;
#endif
    HVXC_SPECIFIC_CONFIG hvxcSpecificConfig;	/* AI 990616 */
#if 0 /* not needed , everthing is in CELP_SPECIFIC_CONFIG*/
    ER_CELP_SPECIFIC_CONFIG erCelpSpecificConfig;
#endif
 } specConf;

  DESCR_ELE  epConfig;
}  AUDIO_SPECIFIC_CONFIG;



typedef struct {
 DESCR_ELE  profileAndLevelIndication;
 DESCR_ELE  streamType      ;
 DESCR_ELE  upsteam;
 DESCR_ELE  specificInfoFlag;
 DESCR_ELE  bufferSizeDB;
 DESCR_ELE  maxBitrate;
 DESCR_ELE  avgBitrate;
#if 0
 DESCR_ELE  bitresFullness;
#endif
 DESCR_ELE  specificInfoLength;
 AUDIO_SPECIFIC_CONFIG audioSpecificConfig;
} DEC_CONF_DESCRIPTOR ;


typedef struct {
 DESCR_ELE useAccessUnitStartFlag;
 DESCR_ELE useAccessUnitEndFlag;
 DESCR_ELE useRandomAccessPointFlag;
 DESCR_ELE usePaddingFlag;
 DESCR_ELE seqNumLength;
  /* to be completed */
} AL_CONF_DESCRIPTOR ;

typedef struct {
 DESCR_ELE  ESNumber;
 DESCR_ELE  streamDependence;
 DESCR_ELE  URLFlag;
 DESCR_ELE  extensFlag;
 DESCR_ELE  dependsOn_Es_number;
 DEC_CONF_DESCRIPTOR DecConfigDescr;  
 AL_CONF_DESCRIPTOR ALConfigDescriptor;
} ES_DESCRIPTOR ;


typedef struct {
 DESCR_ELE  ODLength; 
 DESCR_ELE  ODescrId;
 DESCR_ELE  streamCount;
 DESCR_ELE  extensionFlag;
 ES_DESCRIPTOR *ESDescriptor[50]; 
} OBJECT_DESCRIPTOR ;

typedef struct {
  HANDLE_BSBITBUFFER      bitBuf;
  int               sampleRate;
  int               bitRate;
  unsigned int NoAUInBuffer;
  unsigned int *AULength;
} LAYER_DATA ;


typedef struct {
  OBJECT_DESCRIPTOR* od ;
  LAYER_DATA layer[50];
  unsigned int scalOutSelect;
} FRAME_DATA ;

typedef struct {
  OBJECT_DESCRIPTOR* od ;
  LAYER_DATA layer[50];
  int        opMode;
} ENC_FRAME_DATA ;


#endif
