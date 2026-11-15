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



Source file: flex_mux.c


Required modules:
common.o		common module
bitstream.o             bitstream handling module

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

**********************************************************************/
#include <string.h>
#include <stdio.h>

#include "allHandles.h"

#include "obj_descr.h"           /* structs */
#include "lpc_common.h"          /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "flex_mux.h"

static int locMainDebugLevel=0;
static   HANDLE_BSBITBUFFER tmpBuf;
void initObjDescr( OBJECT_DESCRIPTOR *od,int mainDebugLevel)
{
  od->ODLength.length=32; 
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;

  tmpBuf =BsAllocBuffer(256*8);
  locMainDebugLevel=mainDebugLevel;
  
}

void presetObjDescr( OBJECT_DESCRIPTOR *od, int numLayers)
{

  od->ODLength.value=0; 
  od->ODescrId.value=1;
  od->streamCount.value=numLayers;
  od->extensionFlag.value=0;
  
}

void initTFspecConf ( TF_SPECIFIC_CONFIG *tfConf )
{
  
/*   tfConf->TFCodingType.length=2 ; */
  tfConf->frameLength.length=1 ;
  tfConf->dependsOnCoreCoder.length=1 ;
/*  tfConf->dependsOnCoreCoder.value=0 ; */
  tfConf->coreCoderDelay.length=14 ;
/*  tfConf->coreCoderDelay.value=0 ; */
  tfConf->extension.length=1 ;
  tfConf->extension.value=0 ;
  
#ifdef VERSION2
  tfConf->numOfSubFrame.length=5;
  tfConf->numOfSubFrame.value=1;
  tfConf->layer_length.length=11;
  tfConf->layer_length.value=0;

  tfConf->aacSectionDataResilienceFlag.length=1;
  tfConf->aacSectionDataResilienceFlag.value=0;
  tfConf->aacScalefactorDataResilienceFlag.length=1;
  tfConf->aacScalefactorDataResilienceFlag.value=0;
  tfConf->aacSpectralDataResilienceFlag.length=1;
  tfConf->aacSpectralDataResilienceFlag.value=0;

  tfConf->extension3.length=1 ;
  tfConf->extension3.value=0 ;
#endif
  return;
}


void initCelpSpecConf (CELP_SPECIFIC_CONFIG *celpConf
#ifdef VERSION2
                       ,
		       int audioObjectType
#endif /*VERSION2*/
                       )
{
  celpConf->excitationMode.length = 1;
  celpConf->sampleRateMode.length = 1;
  celpConf->fineRateControl.length = 1;

#ifdef VERSION2
  if (audioObjectType == ER_CELP)
    celpConf->silenceCompressionSW.length = 1;
  else if (audioObjectType ==   CELP)
    celpConf->silenceCompressionSW.length = 0;
  else
    CommonExit(1,"wrong object type value");
#endif

  /* RPE mode */
  celpConf->RPE_Configuration.length = 3;

  /* MPE mode */
  celpConf->MPE_Configuration.length = 5;
  celpConf->numEnhLayers.length = 2;
  celpConf->bandwidthScalabilityMode.length = 1;
#ifdef CORRIGENDUM1
  celpConf->isBaseLayer.length = 1;	/* 14496-3 COR1 */
  celpConf->isBWSLayer.length = 1;	/* 14496-3 COR1 */
  celpConf->CELP_BRS_id.length = 2;	/* 14496-3 COR1 */
  celpConf->BWS_Configuration.length = 2;
#endif
}

#ifndef CORRIGENDUM1
void initCelpEnhSpecConf (CELP_ENH_SPECIFIC_CONFIG *celpEnhConf)
{
  celpEnhConf->BWS_Configuration.length = 2;
}
#endif

#if 0 /* functionality is already in initCelpSpecConf() */
void initErCelpSpecConf (ER_CELP_SPECIFIC_CONFIG *erCelpConf)
{
  erCelpConf->excitationMode.length = 1;
  erCelpConf->sampleRateMode.length = 1;
  erCelpConf->fineRateControl.length = 1;

  erCelpConf->silenceCompressionSW.length = 1;

  /* RPE mode */
  erCelpConf->RPE_Configuration.length = 3;

  /* MPE mode */
  erCelpConf->MPE_Configuration.length = 5;
  erCelpConf->numEnhLayers.length = 2;
  erCelpConf->bandwidthScalabilityMode.length = 1;
}
#endif

/* AI 990616 */
void initHvxcSpecConf (HVXC_SPECIFIC_CONFIG *hvxcConf)
{
#ifdef CORRIGENDUM1
  hvxcConf->isBaseLayer.length = 1;	/* 14496-3 COR1 (AI 2000/10) */
#endif
  hvxcConf->HVXCvarMode.length = 1;
  hvxcConf->HVXCrateMode.length = 2;
  hvxcConf->extensionFlag.length = 1;
#ifdef VERSION2
#if 0
  hvxcConf->packMode.length = 1;
#endif
  hvxcConf->vrScalFlag.length = 1;    /* VR scalable mode (YM 990728) */
#endif
  return;
}

#ifdef VERSION2
/* HP20001009 */
void initParaSpecConf (PARA_SPECIFIC_CONFIG *paraConf)
{
#ifdef CORRIGENDUM1
  paraConf->isBaseLayer.length = 1;	/* 14496-3 COR1 */
#endif

  /* base layer */
  paraConf->PARAmode.length = 2;

  paraConf->HVXCvarMode.length = 1;
  paraConf->HVXCrateMode.length = 2;
  paraConf->extensionFlag.length = 1;
  paraConf->vrScalFlag.length = 1;

  paraConf->HILNquantMode.length = 1;
  paraConf->HILNmaxNumLine.length = 8;
  paraConf->HILNsampleRateCode.length = 4;
  paraConf->HILNframeLength.length = 12;
  paraConf->HILNcontMode.length = 2;

  paraConf->PARAextensionFlag.length = 1;

  /* enha/ext layer(s) */
  paraConf->HILNenhaLayer.length = 1;
  paraConf->HILNenhaQuantMode.length = 2;
  return;
}
#endif

void initESDescr( ES_DESCRIPTOR **es)
{

  *es = NULL;
  *es = (ES_DESCRIPTOR*) malloc(sizeof(ES_DESCRIPTOR));
  

  if (*es==NULL) CommonExit(-1,"no mem");

  memset (*es, 0, sizeof (ES_DESCRIPTOR)) ;

#ifdef OLD_DESCRIPTORS
  (*es)->ESNumber.length=5;
  (*es)->dependsOn_Es_number.length=5;
#else
  (*es)->ESNumber.length=16;
  (*es)->dependsOn_Es_number.length=16;
#endif
  (*es)->streamDependence.length=1;
  (*es)->URLFlag.length=1;
  (*es)->extensFlag.length=1;

  (*es)->DecConfigDescr.profileAndLevelIndication.length=8 ;
  (*es)->DecConfigDescr.streamType.length=6 ;
  (*es)->DecConfigDescr.upsteam.length=1 ;
  (*es)->DecConfigDescr.specificInfoFlag.length=1 ;
  (*es)->DecConfigDescr.bufferSizeDB.length=24 ;
  (*es)->DecConfigDescr.maxBitrate.length=32 ;
  (*es)->DecConfigDescr.avgBitrate.length=32 ;
  (*es)->DecConfigDescr.specificInfoLength.length=8 ;
  (*es)->DecConfigDescr.audioSpecificConfig.audioDecoderType.length=5 ;
  (*es)->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.length= 4;
  (*es)->DecConfigDescr.audioSpecificConfig.samplingFrequency.length= 24;
  (*es)->DecConfigDescr.audioSpecificConfig.channelConfiguration.length=4 ;
#ifndef EP_CONFING_MISSING
  (*es)->DecConfigDescr.audioSpecificConfig.epConfig.length=2 ;
#endif
  (*es)->ALConfigDescriptor.useAccessUnitStartFlag.length = 1;
  (*es)->ALConfigDescriptor.useAccessUnitEndFlag.length = 1;
  (*es)->ALConfigDescriptor.useRandomAccessPointFlag.length = 1;
  (*es)->ALConfigDescriptor.usePaddingFlag.length = 1;
  (*es)->ALConfigDescriptor.seqNumLength.length = 4;

}
void presetESDescr( ES_DESCRIPTOR *es,int layer)
{
  es->ESNumber.value=layer;

  /* if this is the first layer, there is no dependence */
  es->streamDependence.value=((layer==0)?0:1); 
  es->URLFlag.value=0;
  es->extensFlag.value=0;
  es->dependsOn_Es_number.value=(layer>0)?(layer-1):0;

  es->DecConfigDescr.profileAndLevelIndication.value=0;
  es->DecConfigDescr.streamType.value=5 ; /* audio stream */
  es->DecConfigDescr.upsteam.value=0;
  es->DecConfigDescr.specificInfoFlag.value=1 ;
  es->DecConfigDescr.bufferSizeDB.value=6144;
  es->DecConfigDescr.maxBitrate.value=0;
  es->DecConfigDescr.avgBitrate.value=0;
  es->DecConfigDescr.specificInfoLength.value=2; /* 16 bits if TFcoding */

  /* es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value= 0x6;*/  /*24 kHz */ /* cause this is patched afterward */
  es->DecConfigDescr.audioSpecificConfig.channelConfiguration.value=1 ;
#ifndef EP_CONFING_MISSING
  es->DecConfigDescr.audioSpecificConfig.epConfig.value=0;	/* epConfig=0 at default */
#endif
}


static void BsRWBitWrapper(HANDLE_BSBITSTREAM stream,		/* in: bit stream */
                      unsigned long *data,		/* out: bits read/write */
                      int numBit,		/* in: num bits to read */
                      int WriteFlag)
{
  if (WriteFlag){
    BsPutBit(stream,*data,numBit);
  } else {
    BsGetBit(stream,data,numBit);
  }
}


void    advanceTFspecConf (   HANDLE_BSBITSTREAM       bitStream,TF_SPECIFIC_CONFIG *tfConf,int audioObjectType, int WriteFlag) 
{
  
/*   BsRWBitWrapper(bitStream, &(tfConf->TFCodingType.value),   */
/*                  tfConf->TFCodingType.length,WriteFlag); */
  
  BsRWBitWrapper(bitStream, &(tfConf->frameLength.value),
                 tfConf->frameLength.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(tfConf->dependsOnCoreCoder.value),  
                 tfConf->dependsOnCoreCoder.length,WriteFlag);
  if (tfConf->dependsOnCoreCoder.value != 0) {
    BsRWBitWrapper(bitStream, &(tfConf->coreCoderDelay.value),  
                   tfConf->coreCoderDelay.length,WriteFlag);
  }

  BsRWBitWrapper(bitStream, &(tfConf->extension.value),  
                 tfConf->extension.length,WriteFlag);

  if (tfConf->extension.value==0) return;

  if (audioObjectType == 22) { /* ER_BSAC */
    BsRWBitWrapper(bitStream, &(tfConf->numOfSubFrame.value),  
                 tfConf->numOfSubFrame.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(tfConf->layer_length.value),  
                 tfConf->layer_length.length,WriteFlag);
  }

  /* ER AAC */
  if (audioObjectType==17 || audioObjectType==18  || audioObjectType==19 ||
      audioObjectType==20 || audioObjectType==21  || audioObjectType==23) {
    BsRWBitWrapper(bitStream, &(tfConf->aacSectionDataResilienceFlag.value),  
                 tfConf->aacSectionDataResilienceFlag.length,WriteFlag);
    BsRWBitWrapper(bitStream,&(tfConf->aacScalefactorDataResilienceFlag.value), 
                 tfConf->aacScalefactorDataResilienceFlag.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(tfConf->aacSpectralDataResilienceFlag.value),  
                 tfConf->aacSpectralDataResilienceFlag.length,WriteFlag);
  }
  
  BsRWBitWrapper(bitStream, &(tfConf->extension3.value),  
                 tfConf->extension3.length,WriteFlag);
  
}

void advanceCelpSpecConf ( HANDLE_BSBITSTREAM      bitStream,CELP_SPECIFIC_CONFIG *celpConf,int WriteFlag) 
{
#ifdef CORRIGENDUM1
  BsRWBitWrapper(bitStream, &(celpConf->isBaseLayer.value),  
                 celpConf->isBaseLayer.length,WriteFlag);
  if (celpConf->isBaseLayer.value == 1) { /* CELP Base Layer */
    BsRWBitWrapper(bitStream, &(celpConf->excitationMode.value),  
		   celpConf->excitationMode.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(celpConf->sampleRateMode.value),  
		   celpConf->sampleRateMode.length,WriteFlag);
  
    BsRWBitWrapper(bitStream, &(celpConf->fineRateControl.value),  
		   celpConf->fineRateControl.length,WriteFlag);

#ifdef VERSION2
    if (celpConf->silenceCompressionSW.length == 0)
      celpConf->silenceCompressionSW.value=0;
    else  
      BsRWBitWrapper(bitStream, &(celpConf->silenceCompressionSW.value),  
		     celpConf->silenceCompressionSW.length,WriteFlag);
#endif

    if (celpConf->excitationMode.value == RegularPulseExc)
    {
      BsRWBitWrapper(bitStream, &(celpConf->RPE_Configuration.value),  
		     celpConf->RPE_Configuration.length,WriteFlag);
    }
  
    if (celpConf->excitationMode.value == MultiPulseExc)
    {
      BsRWBitWrapper(bitStream, &(celpConf->MPE_Configuration.value),  
		     celpConf->MPE_Configuration.length,WriteFlag);

      BsRWBitWrapper(bitStream, &(celpConf->numEnhLayers.value),  
		     celpConf->numEnhLayers.length,WriteFlag);

      BsRWBitWrapper(bitStream, &(celpConf->bandwidthScalabilityMode.value),  
		     celpConf->bandwidthScalabilityMode.length,WriteFlag);
    }
  } else {
    BsRWBitWrapper(bitStream, &(celpConf->isBWSLayer.value),  
		   celpConf->isBWSLayer.length,WriteFlag);
    if (celpConf->isBWSLayer.value == 1) { /* CELP BWS Layer */
      BsRWBitWrapper(bitStream, &(celpConf->BWS_Configuration.value),  
		     celpConf->BWS_Configuration.length,WriteFlag);
    } else { /* CELP BRS Layer */
      BsRWBitWrapper(bitStream, &(celpConf->CELP_BRS_id.value),  
		     celpConf->CELP_BRS_id.length,WriteFlag);
    }
  }
#else
  BsRWBitWrapper(bitStream, &(celpConf->excitationMode.value),  
                 celpConf->excitationMode.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(celpConf->sampleRateMode.value),  
                 celpConf->sampleRateMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(celpConf->fineRateControl.value),  
                 celpConf->fineRateControl.length,WriteFlag);

#ifdef VERSION2
  if (celpConf->silenceCompressionSW.length == 0)
    celpConf->silenceCompressionSW.value=0;
  else  
    BsRWBitWrapper(bitStream, &(celpConf->silenceCompressionSW.value),  
                   celpConf->silenceCompressionSW.length,WriteFlag);
#endif

  if (celpConf->excitationMode.value == RegularPulseExc)
  {
    BsRWBitWrapper(bitStream, &(celpConf->RPE_Configuration.value),  
                   celpConf->RPE_Configuration.length,WriteFlag);
  }
  
  if (celpConf->excitationMode.value == MultiPulseExc)
  {
    BsRWBitWrapper(bitStream, &(celpConf->MPE_Configuration.value),  
                   celpConf->MPE_Configuration.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(celpConf->numEnhLayers.value),  
                   celpConf->numEnhLayers.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(celpConf->bandwidthScalabilityMode.value),  
                   celpConf->bandwidthScalabilityMode.length,WriteFlag);
  }
#endif
}

#ifndef CORRIGENDUM1
void advanceCelpEnhSpecConf ( HANDLE_BSBITSTREAM  bitStream,CELP_ENH_SPECIFIC_CONFIG *celpEnhConf,int WriteFlag) 
{
  BsRWBitWrapper(bitStream, &(celpEnhConf->BWS_Configuration.value),  
		 celpEnhConf->BWS_Configuration.length,WriteFlag);
}

#ifdef VERSION2
void advanceErCelpSpecConf ( HANDLE_BSBITSTREAM      bitStream,CELP_SPECIFIC_CONFIG *erCelpConf,int WriteFlag) 
{
  BsRWBitWrapper(bitStream, &(erCelpConf->excitationMode.value),  
                 erCelpConf->excitationMode.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(erCelpConf->sampleRateMode.value),  
                 erCelpConf->sampleRateMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(erCelpConf->fineRateControl.value),  
                 erCelpConf->fineRateControl.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(erCelpConf->silenceCompressionSW.value),  
                 erCelpConf->silenceCompressionSW.length,WriteFlag);
  
  if (erCelpConf->excitationMode.value == RegularPulseExc)
  {
    BsRWBitWrapper(bitStream, &(erCelpConf->RPE_Configuration.value),  
                   erCelpConf->RPE_Configuration.length,WriteFlag);
  }
  
  if (erCelpConf->excitationMode.value == MultiPulseExc)
  {
    BsRWBitWrapper(bitStream, &(erCelpConf->MPE_Configuration.value),  
                   erCelpConf->MPE_Configuration.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(erCelpConf->numEnhLayers.value),  
                   erCelpConf->numEnhLayers.length,WriteFlag);

    BsRWBitWrapper(bitStream, &(erCelpConf->bandwidthScalabilityMode.value),  
                   erCelpConf->bandwidthScalabilityMode.length,WriteFlag);
  }
}
#endif
#endif

/* AI 990616 */
void advanceHvxcSpecConf(
HANDLE_BSBITSTREAM  bitStream,
HVXC_SPECIFIC_CONFIG *hvxcConf,
int WriteFlag) 
{
#ifdef CORRIGENDUM1
  BsRWBitWrapper(bitStream, &(hvxcConf->isBaseLayer.value),  
                 hvxcConf->isBaseLayer.length,WriteFlag);

  if (hvxcConf->isBaseLayer.value) {
    BsRWBitWrapper(bitStream, &(hvxcConf->HVXCvarMode.value),  
		   hvxcConf->HVXCvarMode.length,WriteFlag);
  
    BsRWBitWrapper(bitStream, &(hvxcConf->HVXCrateMode.value),
		   hvxcConf->HVXCrateMode.length,WriteFlag);
  
    BsRWBitWrapper(bitStream, &(hvxcConf->extensionFlag.value),  
		   hvxcConf->extensionFlag.length,WriteFlag);
#ifdef VERSION2
    if (hvxcConf->extensionFlag.value != 0) {	/* for "EP" tool (AI 990616) */
      /* for VR scalable mode (YM 990728) */
      BsRWBitWrapper(bitStream, &(hvxcConf->vrScalFlag.value),
		     hvxcConf->vrScalFlag.length,WriteFlag);
    }
#endif
  }
#else
  BsRWBitWrapper(bitStream, &(hvxcConf->HVXCvarMode.value),  
                 hvxcConf->HVXCvarMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(hvxcConf->HVXCrateMode.value),
                 hvxcConf->HVXCrateMode.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(hvxcConf->extensionFlag.value),  
                 hvxcConf->extensionFlag.length,WriteFlag);
#ifdef VERSION2
  if (hvxcConf->extensionFlag.value != 0) {	/* for "EP" tool (AI 990616) */
#if 0
    BsRWBitWrapper(bitStream, &(hvxcConf->packMode.value),  
		   hvxcConf->packMode.length,WriteFlag);
#endif
    /* for VR scalable mode (YM 990728) */
    BsRWBitWrapper(bitStream, &(hvxcConf->vrScalFlag.value),
		   hvxcConf->vrScalFlag.length,WriteFlag);
  }
#endif
#endif
  return;
}

#ifdef VERSION2
/* HP20001009 */
void advanceParaSpecConf(
HANDLE_BSBITSTREAM  bitStream,
PARA_SPECIFIC_CONFIG *paraConf,
int WriteFlag) 
{
#ifdef CORRIGENDUM1
  BsRWBitWrapper(bitStream, &(paraConf->isBaseLayer.value),  
                 paraConf->isBaseLayer.length,WriteFlag);
  if (paraConf->isBaseLayer.value==0)
    advanceParaEnhSpecConf(bitStream,paraConf,WriteFlag);
  else
#endif
  {
  BsRWBitWrapper(bitStream, &(paraConf->PARAmode.value),  
                 paraConf->PARAmode.length,WriteFlag);
  if (paraConf->PARAmode.value != 1) {
    BsRWBitWrapper(bitStream, &(paraConf->HVXCvarMode.value),  
		   paraConf->HVXCvarMode.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->HVXCrateMode.value),
		   paraConf->HVXCrateMode.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->extensionFlag.value),  
		   paraConf->extensionFlag.length,WriteFlag);
    if (paraConf->extensionFlag.value != 0) {	/* for "EP" tool (AI 990616) */
      BsRWBitWrapper(bitStream, &(paraConf->vrScalFlag.value),
		     paraConf->vrScalFlag.length,WriteFlag);
    }
  }
  if (paraConf->PARAmode.value != 0) {
    BsRWBitWrapper(bitStream, &(paraConf->HILNquantMode.value),  
		   paraConf->HILNquantMode.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->HILNmaxNumLine.value),  
		   paraConf->HILNmaxNumLine.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->HILNsampleRateCode.value),  
		   paraConf->HILNsampleRateCode.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->HILNframeLength.value),  
		   paraConf->HILNframeLength.length,WriteFlag);
    BsRWBitWrapper(bitStream, &(paraConf->HILNcontMode.value),  
		   paraConf->HILNcontMode.length,WriteFlag);
  }
  BsRWBitWrapper(bitStream, &(paraConf->PARAextensionFlag.value),  
                 paraConf->PARAextensionFlag.length,WriteFlag);
  }
  return;
}

void advanceParaEnhSpecConf(
HANDLE_BSBITSTREAM  bitStream,
PARA_SPECIFIC_CONFIG *paraConf,
int WriteFlag) 
{
  BsRWBitWrapper(bitStream, &(paraConf->HILNenhaLayer.value),  
                 paraConf->HILNenhaLayer.length,WriteFlag);
  if (paraConf->HILNenhaLayer.value) {
    BsRWBitWrapper(bitStream, &(paraConf->HILNenhaQuantMode.value),  
		   paraConf->HILNenhaQuantMode.length,WriteFlag);
  }
  return;
}
#endif

void  advanceESDescr ( HANDLE_BSBITSTREAM  bitStream, 
                       ES_DESCRIPTOR *es, 
                       int WriteFlag) 
{
  
  BsRWBitWrapper(bitStream,&(es->ESNumber.value),es->ESNumber.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(es->streamDependence.value),es->streamDependence.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(es->URLFlag.value),  es->URLFlag.length,WriteFlag);

#ifdef OLD_DESCRIPTORS
  if (es->streamDependence.value != 0) {
    BsRWBitWrapper(bitStream, &(es->dependsOn_Es_number.value),  es->dependsOn_Es_number.length,WriteFlag);
  }
  BsRWBitWrapper(bitStream, &(es->extensFlag.value),  es->extensFlag.length,WriteFlag);
#else
  BsRWBitWrapper(bitStream, &(es->extensFlag.value),  es->extensFlag.length,WriteFlag);
  BsRWBitWrapper(bitStream, &(es->extensFlag.value),  5 /*priority*/,WriteFlag);
  if (es->streamDependence.value != 0)
    BsRWBitWrapper(bitStream, &(es->dependsOn_Es_number.value),  es->dependsOn_Es_number.length,WriteFlag);
#endif

  es->DecConfigDescr.profileAndLevelIndication.value=0x40;

  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.profileAndLevelIndication.value),  es->DecConfigDescr.profileAndLevelIndication.length,WriteFlag);
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.streamType.value) ,  es->DecConfigDescr.streamType.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.upsteam.value) ,  es->DecConfigDescr.upsteam.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.specificInfoFlag.value) ,  es->DecConfigDescr.specificInfoFlag.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.bufferSizeDB.value) ,  es->DecConfigDescr.bufferSizeDB.length,WriteFlag) ;

  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.maxBitrate.value) ,  es->DecConfigDescr.maxBitrate.length,WriteFlag) ;
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.avgBitrate.value) ,  es->DecConfigDescr.avgBitrate.length,WriteFlag) ;

#ifdef OLD_DESCRIPTORS
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.specificInfoLength.value) ,  es->DecConfigDescr.specificInfoLength.length,WriteFlag) ;
#endif
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value),  
                 es->DecConfigDescr.audioSpecificConfig.audioDecoderType.length,WriteFlag) ;
  
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value),  
                 es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.length,WriteFlag);
  if (es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
    BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.samplingFrequency.value),  
		   es->DecConfigDescr.audioSpecificConfig.samplingFrequency.length,WriteFlag);
  BsRWBitWrapper(bitStream, &(es->DecConfigDescr.audioSpecificConfig.channelConfiguration.value),  
                 es->DecConfigDescr.audioSpecificConfig.channelConfiguration.length,WriteFlag);


  switch (es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value)
    {
    case AAC_LTP :
    case AAC_SCAL :
    case TWIN_VQ :  
#ifdef VERSION2
    case ER_TWIN_VQ :  
    case ER_BSAC :  
#endif
      initTFspecConf ( &(es->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig));
      advanceTFspecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig), es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value, WriteFlag);
      /* added by NTT 20001121 */
#ifdef VERSION2
#ifndef EP_CONFIG_MISSING
      /* parse epConfig */
      if((es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_TWIN_VQ) ||
         (es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_BSAC))
      BsRWBitWrapper(bitStream, 
                     &(es->DecConfigDescr.audioSpecificConfig.epConfig.value),
		     es->DecConfigDescr.audioSpecificConfig.epConfig.length,
                     WriteFlag);
#endif
#endif
      break;
    case CELP :
#ifdef CORRIGENDUM1
      initCelpSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig)
#ifdef VERSION2
			,
			es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value
#endif /*VERSION2*/
			); 
      advanceCelpSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),WriteFlag);
#else
      if ( es->streamDependence.value == 0 ) { /*CELP BaseLayer*/
	initCelpSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig)
#ifdef VERSION2
                          ,
                          es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value
#endif /*VERSION2*/
                          ); 
	advanceCelpSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),WriteFlag);
      } else if ( es->DecConfigDescr.audioSpecificConfig.BWS_on==1 ) { 
        /*CELP BWS Enh.Layer*/
	initCelpEnhSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig)); 
	advanceCelpEnhSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig),WriteFlag);
      } /* No SpecConf for CELP BRS Enh.Layer */
#endif
      break;
#ifdef VERSION2
    case ER_CELP :
#ifdef CORRIGENDUM1
      initCelpSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),
			es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value); 
      advanceCelpSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),WriteFlag);
#else
      if ( es->streamDependence.value == 0 ) { /*CELP BaseLayer*/
	initCelpSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),
                          es->DecConfigDescr.audioSpecificConfig.audioDecoderType.value); 
	advanceErCelpSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig),WriteFlag);
      } else if ( es->DecConfigDescr.audioSpecificConfig.BWS_on==1 ) { 
        /*CELP BWS Enh.Layer*/
	initCelpEnhSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig)); 
	advanceCelpEnhSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig),WriteFlag);
      } /* No SpecConf for CELP BRS Enh.Layer */
#endif
#ifndef EP_CONFIG_MISSING
      /* parse epConfig */
      BsRWBitWrapper(bitStream, 
		     &(es->DecConfigDescr.audioSpecificConfig.epConfig.value),  
		     es->DecConfigDescr.audioSpecificConfig.epConfig.length,
		     WriteFlag);
#endif
      break;
#endif
    case HVXC:	/* AI 990616 */
#ifdef CORRIGENDUM1
      initHvxcSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig)); 
      advanceHvxcSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig),WriteFlag);
#else
      if ( es->streamDependence.value == 0 ) { /* HVXC BaseLayer(AI 991129) */
	initHvxcSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig)); 
	advanceHvxcSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig),WriteFlag);
      }
      else {	/* HVXC Enh.Layer(AI 991129) */
	/* No SpecConf */
      }
#endif
      break;
#ifdef VERSION2
    case ER_HVXC :
#ifdef CORRIGENDUM1
      initHvxcSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig)); 
      advanceHvxcSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig),WriteFlag);
#else
      if ( es->streamDependence.value == 0 ) { /* HVXC BaseLayer(AI 991129) */
	initHvxcSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig)); 
	advanceHvxcSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig),WriteFlag);
      }
      else {	/* HVXC Enh.Layer(AI 991129) */
	/* No SpecConf */
      }
#endif
#ifndef EP_CONFIG_MISSING
      /* parse epConfig */
      BsRWBitWrapper(bitStream, 
		     &(es->DecConfigDescr.audioSpecificConfig.epConfig.value),  
		     es->DecConfigDescr.audioSpecificConfig.epConfig.length,
		     WriteFlag);
#endif
      break;
#endif      

#ifdef VERSION2
    case ER_HILN :
    case ER_PARA :
      initParaSpecConf (&(es->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig));
#ifndef CORRIGENDUM1
      if ( es->streamDependence.value == 0 ) /* BaseLayer */
	advanceParaSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig),WriteFlag);
      else
	advanceParaEnhSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig),WriteFlag);
#else
      advanceParaSpecConf(bitStream,&(es->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig),WriteFlag);
#endif
#ifndef EP_CONFIG_MISSING
      /* parse epConfig */
      BsRWBitWrapper(bitStream, 
		     &(es->DecConfigDescr.audioSpecificConfig.epConfig.value),  
		     es->DecConfigDescr.audioSpecificConfig.epConfig.length,
		     WriteFlag);
#endif
      break;
#endif
    default :
      CommonExit(-1,"audioDecoderType not implemented");
      break;
    }

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useAccessUnitStartFlag.value),es->ALConfigDescriptor.useAccessUnitStartFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useAccessUnitEndFlag.value),es->ALConfigDescriptor.useAccessUnitEndFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.useRandomAccessPointFlag.value),es->ALConfigDescriptor.useRandomAccessPointFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.usePaddingFlag.value),es->ALConfigDescriptor.usePaddingFlag.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(es->ALConfigDescriptor.seqNumLength.value),es->ALConfigDescriptor.seqNumLength.length,WriteFlag);
}


void  advanceODescr (   HANDLE_BSBITSTREAM       bitStream, OBJECT_DESCRIPTOR *od,int WriteFlag) 
{
  od->ODLength.length=32; 
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;
  
/*   &(od->ODLength.value); is written later as soon as  length is known*/
  
  BsRWBitWrapper(bitStream, &(od->ODescrId.value),  od->ODescrId.length,WriteFlag);
  
  BsRWBitWrapper(bitStream, &(od->streamCount.value),  od->streamCount.length,WriteFlag);

  BsRWBitWrapper(bitStream, &(od->extensionFlag.value),  od->extensionFlag.length,WriteFlag);
}

void getAccessUnit( HANDLE_BSBITSTREAM       bitStream ,  HANDLE_BSBITBUFFER  AUBuffer,unsigned int *AUIndex, unsigned long *totalLength, ES_DESCRIPTOR *es)
{
  unsigned long  index,length,AUStartFlag,AUEndFlag,dummy;
  unsigned long  seq_number;
  
  /* read default AL-PDU header */
  BsGetBit(bitStream,&index,8);
  BsGetBit(bitStream,&length,8);
  *totalLength += length;

  if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
    BsGetBit(bitStream,&AUStartFlag,1);
  if (AUStartFlag!=1) CommonExit(-1,"error in getAccessUnit");

  if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
    BsGetBit(bitStream,&AUEndFlag,1);

  if (es->ALConfigDescriptor.seqNumLength.value > 0)
    BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
  else
    BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

  *AUIndex = index;
  
  if (AUStartFlag!=1) CommonExit(-1,"Error  AL-PDU header ");
  BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
  while (AUEndFlag!=1) {
    BsGetBit(bitStream,&index,8);

    if (*AUIndex != index) 
      CommonExit(-1,"FlexMux index error");

    BsGetBit(bitStream,&length,8);
    *totalLength += length;

    if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
      BsGetBit(bitStream,&AUStartFlag,1);
    if (AUStartFlag==1) CommonExit(-1,"error in getAccessUnit");

    if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
      BsGetBit(bitStream,&AUEndFlag,1);

    if (es->ALConfigDescriptor.seqNumLength.value > 0)
      BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
    else
      BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

    BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
  }  
}

int  nextAccessUnit( HANDLE_BSBITSTREAM       bitStream ,                      
                     unsigned int*     layer, 
                     FRAME_DATA*       frameData
                     )
{
  unsigned long index, dummy,AUStartFlag,AUEndFlag,AUIndex;
  unsigned long length;
  unsigned long  seq_number;
  ES_DESCRIPTOR*  es;
  HANDLE_BSBITBUFFER     AUBuffer  ;

  /* read default AL-PDU header */
  /* suppose trivial streammap table: index 0 = layer 0 ; index 1 = layer 1 etc.*/ 

  if ( BsEof( bitStream, 8 ) ) return 1;

  BsGetBit(bitStream,&index,8);
  *layer = index;
  AUIndex = index;
  BsGetBit(bitStream,&length,8);
  es = frameData->od->ESDescriptor[index];

  AUBuffer = frameData->layer[index].bitBuf  ;

  if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
    if (BsGetBit(bitStream,&AUStartFlag,1)== -1) {
      return -1;
    };
  if (AUStartFlag!=1) CommonExit(-1,"error in getAccessUnit");

  if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
    BsGetBit(bitStream,&AUEndFlag,1);

  if (es->ALConfigDescriptor.seqNumLength.value > 0)
    BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
  else
    BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */
  
  if (AUStartFlag!=1) CommonExit(-1,"Error  AL-PDU header ");
  if (AUBuffer!=0 ) {
    if (BsBufferFreeBits(AUBuffer) > (long)length*8 )  {
      BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
      frameData->layer[AUIndex].AULength[frameData->layer[AUIndex].NoAUInBuffer] = length*8;	/* SHPARK 2000/11/15 */
      frameData->layer[AUIndex].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */

#if 0 /* SHPARK 2000.11.15 */
      frameData->layer[AUIndex].AULength[0] = length*8;	/* AI 2000/01/20 */
#endif
    }  else {
      BsGetSkip(bitStream,length*8);
      CommonWarning ("flexmux input buffer overflow for layer %d ; skiping next AU",index);
    }
  } else {
    BsGetSkip(bitStream,length*8);
  }
  while (AUEndFlag!=1) {
    BsGetBit(bitStream,&index,8);

    if (AUIndex != index) 
      CommonExit(-1,"FlexMux index error");

    BsGetBit(bitStream,&length,8);

    if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
      BsGetBit(bitStream,&AUStartFlag,1);
    if (AUStartFlag==1) CommonExit(-1,"error in getAccessUnit");

    if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
      BsGetBit(bitStream,&AUEndFlag,1);

    if (es->ALConfigDescriptor.seqNumLength.value > 0)
      BsGetBit(bitStream,&seq_number,es->ALConfigDescriptor.seqNumLength.value);
    else
      BsGetBit(bitStream,&dummy,6);  /*6 padding bits (alm) */

    if (AUBuffer!=0 ) {
      if (BsBufferFreeBits(AUBuffer) > (long)length*8 )  {
        BsGetBufferAppend( bitStream, AUBuffer,1,length*8);    
      }  else {
        BsGetSkip(bitStream,length*8);
        CommonWarning ("flexmux input buffer overflow for layer %d ; skiping next AU",index);
      }
    } else {
      BsGetSkip(bitStream,length*8);
    }

  }  
  return 0;
}

void writeFlexMuxPDU(int index,HANDLE_BSBITSTREAM  bitStream , HANDLE_BSBITBUFFER  AUBuffer)
{
  unsigned long  align;
  unsigned long  length,AUStartFlag,AUEndFlag;
  int maxBytes=255;
  HANDLE_BSBITSTREAM AUStream;

  AUEndFlag=1;
  AUStartFlag=1;
    
  AUStream=    BsOpenBufferRead ( AUBuffer)	;

  AUStartFlag=1;
  while (BsBufferNumBit(AUBuffer) > maxBytes*8 ) {
    AUEndFlag=0;

    BsGetBuffer ( AUStream,tmpBuf,maxBytes*8);
    BsRemoveAllCompletelyReadBytesFromBuffer(AUStream,AUBuffer);
    BsPutBit(bitStream,index,8);
    BsPutBit(bitStream,maxBytes,8);
    BsPutBit(bitStream,AUStartFlag,1);
    BsPutBit(bitStream,AUEndFlag,1);
    BsPutBit(bitStream,0,6);/* padding bits */
    BsPutBuffer(bitStream,tmpBuf);

    AUStartFlag=0;
    
  } 
  BsCloseRemove ( AUStream,1)	;

  AUEndFlag=1;
  length= BsBufferNumBit(AUBuffer) / 8;
  align = 8 - BsBufferNumBit(AUBuffer) % 8;
  if (align == 8) align = 0;
  if (align != 0) {
    length+=1;
  }
  BsPutBit(bitStream,index,8);
  BsPutBit(bitStream,length,8);
  BsPutBit(bitStream,AUStartFlag,1);
  BsPutBit(bitStream,AUEndFlag,1);
  BsPutBit(bitStream,0,6);/* padding bits */
  BsPutBuffer(bitStream,AUBuffer);
  BsPutBit(bitStream,0,align);
  

  

  
}

void putAccessUnit(int index,HANDLE_BSBITSTREAM  bitStream , HANDLE_BSBITBUFFER  AUBuffer, ES_DESCRIPTOR *es)
{
  unsigned long  align;
  unsigned long  length,AUStartFlag,AUEndFlag;
  int maxBytes=255;
  HANDLE_BSBITSTREAM AUStream;

  AUEndFlag=1;
  AUStartFlag=1;
    
  AUStream=    BsOpenBufferRead ( AUBuffer)	;

  AUStartFlag=1;
  while ( BsBufferNumBit(AUBuffer) > maxBytes*8 ) {
    AUEndFlag=0;

    BsGetBuffer ( AUStream,tmpBuf,maxBytes*8);
    BsRemoveAllCompletelyReadBytesFromBuffer(AUStream,AUBuffer);
    BsPutBit(bitStream,index,8);
    BsPutBit(bitStream,maxBytes,8);
    if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
      BsPutBit(bitStream,AUStartFlag,1);
    if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
      BsPutBit(bitStream,AUEndFlag,1);
    if (es->ALConfigDescriptor.seqNumLength.value > 0)
      BsPutBit(bitStream,0,es->ALConfigDescriptor.seqNumLength.value);
    else
      BsPutBit(bitStream,0,6);  /*6 padding bits (alm) */

    BsPutBuffer(bitStream,tmpBuf);

    AUStartFlag=0;
    
  } 
  BsCloseRemove ( AUStream,1)	;

  AUEndFlag=1;
  length= BsBufferNumBit(AUBuffer) / 8;
  align = 8 - BsBufferNumBit(AUBuffer) % 8;
  if (align == 8) align = 0;
  if (align != 0) {
    length+=1;
  }
  BsPutBit(bitStream,index,8);
  BsPutBit(bitStream,length,8);
  if (es->ALConfigDescriptor.useAccessUnitStartFlag.value)
    BsPutBit(bitStream,AUStartFlag,1);
  if (es->ALConfigDescriptor.useAccessUnitEndFlag.value)
    BsPutBit(bitStream,AUEndFlag,1);
  if (es->ALConfigDescriptor.seqNumLength.value > 0)
    BsPutBit(bitStream,0,es->ALConfigDescriptor.seqNumLength.value);
  else
    BsPutBit(bitStream,0,6);  /*6 padding bits (alm) */
  BsPutBuffer(bitStream,AUBuffer);
  BsPutBit(bitStream,0,align);
}

void removeAU(HANDLE_BSBITSTREAM  stream, int decBits, FRAME_DATA*        fd,int layer)
{
  unsigned int i;
  int AUlen = 0;
  if (fd->layer[layer].NoAUInBuffer<1)
    CommonExit(-1,"no more  AUs !!!!!");

  AUlen = fd->layer[layer].AULength[0];
  
  if (AUlen-decBits>7)
    if (locMainDebugLevel>2)
      fprintf(stderr," %d Fill bits \n",AUlen-decBits);
  if (AUlen-decBits<0)
    CommonExit(-1,"more bits decoded than in AU !!!!!");

  BsGetSkip(stream,AUlen-decBits);
  for (i=0;i<fd->layer[layer].NoAUInBuffer-1;i++)
    fd->layer[layer].AULength[i]=fd->layer[layer].AULength[i+1];
  fd->layer[layer].NoAUInBuffer--;
  
}

