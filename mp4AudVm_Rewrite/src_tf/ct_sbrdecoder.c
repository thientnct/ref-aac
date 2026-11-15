/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

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
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrdecoder.c,v 1.1.2.9 2007/08/22 13:53:55 sps Exp $

*******************************************************************************/
/*!
  \file
  \brief  SBR decoder frontend $Revision: 1.1.2.9 $
*/


#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_sbrdecoder.h"
#include "ct_envextr.h"
#include "ct_sbrdec.h"
#include "ct_envdec.h"
#include "ct_sbrcrc.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif
#include "tf_mainHandle.h"
#include "extension_type.h"

#ifdef AAC_ELD
#include "obj_descr.h" 
#endif


typedef struct
{
  int outFrameSize;

  SBR_SYNC_STATE syncState ;

  SBR_FRAME_DATA frameData;

} SBR_CHANNEL;

typedef struct sbrDecoderInstance
{
  SBR_CHANNEL SbrChannel[MAX_NUM_CHANNELS];
#ifdef PARAMETRICSTEREO
  HANDLE_PS_DEC hParametricStereoDec;
#endif

} sbrDecoderInstance ;


#ifdef AAC_ELD
static void initFrameData(SBR_CHANNEL *SbrChannel);
#endif

/*******************************************************************************
 Functionname: openSBR
 *******************************************************************************

 Description: 

 Arguments:   

 Return:      

*******************************************************************************/

SBRDECODER openSBR (int sampleRate,
                    int bDownSampledSbr,
                    int coreCodecFrameSize
#ifdef AAC_ELD
                    ,int ldsbr
                    ,UINT8 *sbrHeader
#endif
                    )
{
  int i ;

  HANDLE_ERROR_INFO err = NOERROR;

  SBRDECODER self ;
  SBR_CHANNEL *SbrChannel ;

  self = (SBRDECODER) calloc (1, sizeof (sbrDecoderInstance));
  if (!self) return NULL ;

  SbrChannel = &self->SbrChannel [0] ;

  /* Allocate memory and initialise the filterbanks, and set up the buffers
      so that the SBR-Tool can be used for at least up-sampling only.*/
  initSbrDec (sampleRate,
              bDownSampledSbr,
              coreCodecFrameSize
#ifdef AAC_ELD
              ,ldsbr
#endif
              );
#ifdef PARAMETRICSTEREO
  tfInitPsDec  (&self->hParametricStereoDec, 32 /* noCols */);
#endif


  for (i = 0; i < MAX_NUM_CHANNELS; i++) {
    memset (&(SbrChannel[i]), 0, sizeof (SBR_CHANNEL));
   
    /* Set the sync-state to UPSAMPLING, since given the initialisation above, that's all
       we can do for now.
    */
    SbrChannel[i].syncState    = UPSAMPLING;
    SbrChannel[i].outFrameSize = coreCodecFrameSize*(2-bDownSampledSbr);
#ifdef AAC_ELD
    SbrChannel[i].frameData.ldsbr = ldsbr;

    if(sbrHeader != NULL) {

      /* Reset sbr decoder in case of transmission of SBR header inside ASC */
      BIT_BUFFER bitBuf ;
      SBR_HEADER_STATUS headerStatus = HEADER_OK;

      initBitBuffer (&bitBuf, sbrHeader, MAX_SBR_HEADER_SIZE*sizeof(UINT8));
       
      headerStatus = sbrGetHeaderData (&(SbrChannel[i].frameData.sbr_header),
                                       &bitBuf,
                                       0, /* SbrId - Don't care */
                                       SbrChannel[i].syncState);

      if ( headerStatus == HEADER_RESET ){
        initFrameData(&(SbrChannel[i]));
        resetSbrDec (&(SbrChannel[i].frameData), 2.0f, i);
        /* thats all we need to start SBR */
        SbrChannel[i].syncState = SBR_ACTIVE;
      }
    }

    if ( ldsbr ) {
      /**ATTENTION: the rate value is only usable in the dual rate sbr modus!
       * If you want to use single rate you have to adjust the value correct.
       */
      SbrChannel[i].frameData.rate  = 1;
    } else {
#endif
      SbrChannel[i].frameData.rate  = 2;
#ifdef AAC_ELD
    }
#endif
  }

  return self ;
}


/*******************************************************************************
 Functionname: initFrameData
 *******************************************************************************

 Description: 

 Arguments:   

 Return:      

*******************************************************************************/
static void
initFrameData(SBR_CHANNEL *SbrChannel){

#ifdef SBR_SCALABLE
  SbrChannel->frameData.bStereoEnhancementLayerDecodablePrevE = 1;
  SbrChannel->frameData.bStereoEnhancementLayerDecodablePrevQ = 1;
#endif
  switch (SbrChannel->outFrameSize){
  case 2048:
  case 1024:
    SbrChannel->frameData.numTimeSlots = 16;
    break;
  case 1920:
  case 960:
    SbrChannel->frameData.numTimeSlots = 15;
    break;
  };
}



/*******************************************************************************
 Functionname:closeSBR
 *******************************************************************************

 Description: deletes SBR decoder

 Arguments:   

 Return:      none

*******************************************************************************/

void closeSBR (SBRDECODER self)
{
  if (self)
    free (self);
}




/*******************************************************************************
 Functionname:readSBR
 *******************************************************************************

 Description: 

 Arguments:   

 Return:      none

*******************************************************************************/
static SBR_ERROR readSBR (SBRDECODER self, SBRBITSTREAM *stream, int nChannels)
{
  HANDLE_ERROR_INFO err = NOERROR ;
  
  int element;
  int lr, sbrChannelIndex = 0;
  int CRCLen, SbrFrameOK = 1;
  int sbrCRCAlwaysOn = 0;
  int bs_header_flag = 0;

  SBR_HEADER_STATUS headerStatus = HEADER_OK;

  
  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;

  /* eval Bitstream */
  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements; element++)
  {
  
    BIT_BUFFER bitBuf ;

#ifdef SBR_SCALABLE
    BIT_BUFFER bitBufEnh, *pBitBufEnh;
    int bStereoLayer = 0;

    if(stream[1].NrElements>0) {
      initBitBuffer (&bitBufEnh, stream[1].sbrElement[element].Data,
                     stream[1].sbrElement[element].Payload * 8) ;
      pBitBufEnh = &bitBufEnh;
      bStereoLayer =1;
    }
    else{
      pBitBufEnh = &bitBuf;
      if(nChannels==2)
        bStereoLayer=1;
    }

#endif

    initBitBuffer (&bitBuf, stream->sbrElement[element].Data,
                   stream->sbrElement[element].Payload * 8) ;
  

    /* we have to skip a nibble because the first element of Data only
       contains a nibble of data ! */      
#ifdef AAC_ELD
     if (SbrChannel->frameData.ldsbr != 1)
#endif
       BufGetBits (&bitBuf, LEN_NIBBLE);

#ifdef SBR_SCALABLE
     if(&bitBuf != pBitBufEnh) {
       BufGetBits (pBitBufEnh, LEN_NIBBLE);
     }
#endif
      
    
    /* read control data */
   
    if (stream->sbrElement[element].ExtensionType == EXT_SBR_DATA_CRC || sbrCRCAlwaysOn) {
      CRCLen = 8*(stream->sbrElement[element].Payload-1)+4 - SI_SBR_CRC_BITS;
      SbrFrameOK = SbrCrcCheck (&bitBuf, CRCLen);
    }

    if (SbrFrameOK){
      /* 
      The sbr data seems ok, if the header flag is set we read the header and check
      if vital parameters have changed since the previous frame. If the syncState equals
      UPSAMPLING, the SBR Tool has not been initialised by SBR header data, and can only do
      upsampling. 
      */      

      bs_header_flag = BufGetBits (&bitBuf, 1); /* read Header flag */

      if (bs_header_flag) { 
      
        /* If syncState == SBR_ACTIVE, it means that we've had a SBR header before, and we
        will compare with the previous header to see if a reset is required. If the syncState
        equals UPSAMPLING this means that the SBR-Tool so far is only initialised to do upsampling
        and hence we need to do a reset, and initialise the system according to the present header.*/

        headerStatus = sbrGetHeaderData (&(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                                         &bitBuf,
                                         SBR_ID_SCE,
                                         SbrChannel[sbrChannelIndex].syncState);
      
      }

       switch (stream->sbrElement[element].ElementID){
          case SBR_ID_SCE :

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
            }
            
            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ){
              resetSbrDec ( &(SbrChannel[sbrChannelIndex].frameData),
                            2.0f,
                            sbrChannelIndex);
             
             /* At this point we have a header and the system has been reset,
                hence syncState from now on will be SBR_ACTIVE.*/
              SbrChannel[sbrChannelIndex].syncState = SBR_ACTIVE;
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
              sbrGetSingleChannelElement (&(SbrChannel[sbrChannelIndex].frameData),
                                          &bitBuf
#ifdef PARAMETRICSTEREO
                                          ,self->hParametricStereoDec
#endif
                                          );
            }
            break;

          case SBR_ID_CPE :

            if(bs_header_flag){
              memcpy (&(SbrChannel[sbrChannelIndex + 1].frameData.sbr_header),
                      &(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                      sizeof(SBR_HEADER_DATA) );
            }

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
              initFrameData(&SbrChannel[sbrChannelIndex + 1]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ) {
              for (lr = 0 ; lr < 2 ; lr++) {
                resetSbrDec ( &(SbrChannel[sbrChannelIndex+lr].frameData),
                              2.0f,
                              sbrChannelIndex+lr);
                /* At this point we have a header and the system has been reset,
                   hence syncState from now on will be SBR_ACTIVE.*/
                SbrChannel[sbrChannelIndex+lr].syncState = SBR_ACTIVE;
              }
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
#ifdef SBR_SCALABLE
              sbrGetChannelPairElement (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf, pBitBufEnh,bStereoLayer);
#else
              sbrGetChannelPairElement (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf);
#endif
            }

            sbrChannelIndex++;
            break;

          default:
            return SBRDEC_ILLEGAL_PLUS_ELE_ID;     
      }
    }

    sbrChannelIndex++;
  }

  return SBRDEC_OK ;
}


static SBR_ERROR readSBR_BSAC (SBRDECODER self, SBRBITSTREAM *stream, int nChannels)
{
  HANDLE_ERROR_INFO err = NOERROR ;
  
  int element;
  int lr, sbrChannelIndex = 0;
  int CRCLen, SbrFrameOK = 1;
  int sbrCRCAlwaysOn = 0;
  int bs_header_flag = 0;

  SBR_HEADER_STATUS headerStatus = HEADER_OK;

  
  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;

  /* eval Bitstream */
  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements; element++)
  {
  
    BIT_BUFFER bitBuf ;

#ifdef SBR_SCALABLE
    BIT_BUFFER bitBufEnh, *pBitBufEnh;
    int bStereoLayer = 0;

    if(stream[1].NrElements>0) {
      initBitBuffer (&bitBufEnh, stream[1].sbrElement[element].Data,
                     stream[1].sbrElement[element].Payload * 8) ;
      pBitBufEnh = &bitBufEnh;
      bStereoLayer =1;
    }
    else{
      pBitBufEnh = &bitBuf;
      if(nChannels==2)
        bStereoLayer=1;
    }

#endif

    initBitBuffer (&bitBuf, stream->sbrElement[element].Data,
                   stream->sbrElement[element].Payload * 8) ;
  
    /* we have to skip a nibble because the first element of Data only
       contains a nibble of data ! */
#ifdef SBR_SCALABLE
     if(&bitBuf != pBitBufEnh) {
       BufGetBits (pBitBufEnh, LEN_NIBBLE);
     }
#endif
      
    
    /* read control data */
   
    if (stream->sbrElement[element].ExtensionType == EXT_SBR_DATA_CRC || sbrCRCAlwaysOn) {
      CRCLen = 8*(stream->sbrElement[element].Payload-1)+4 - SI_SBR_CRC_BITS;
      SbrFrameOK = SbrCrcCheck (&bitBuf, CRCLen);
    }

    if (SbrFrameOK){
      /* 
      The sbr data seems ok, if the header flag is set we read the header and check
      if vital parameters have changed since the previous frame. If the syncState equals
      UPSAMPLING, the SBR Tool has not been initialised by SBR header data, and can only do
      upsampling. 
      */      

      bs_header_flag = BufGetBits (&bitBuf, 1); /* read Header flag */

      if (bs_header_flag) { 
      
        /* If syncState == SBR_ACTIVE, it means that we've had a SBR header before, and we
        will compare with the previous header to see if a reset is required. If the syncState
        equals UPSAMPLING this means that the SBR-Tool so far is only initialised to do upsampling
        and hence we need to do a reset, and initialise the system according to the present header.*/

        headerStatus = sbrGetHeaderData (&(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                                         &bitBuf,
                                         SBR_ID_SCE,
                                         SbrChannel[sbrChannelIndex].syncState);
      
      }

       switch (stream->sbrElement[element].ElementID){
          case SBR_ID_SCE :

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
            }
            
            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ){
              resetSbrDec ( &(SbrChannel[sbrChannelIndex].frameData),
                            2.0f,
                            sbrChannelIndex);
             
             /* At this point we have a header and the system has been reset,
                hence syncState from now on will be SBR_ACTIVE.*/
              SbrChannel[sbrChannelIndex].syncState = SBR_ACTIVE;
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
              sbrGetSingleChannelElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                          &bitBuf);
            }
            break;

          case SBR_ID_CPE :

            if(bs_header_flag){
              memcpy (&(SbrChannel[sbrChannelIndex + 1].frameData.sbr_header),
                      &(SbrChannel[sbrChannelIndex].frameData.sbr_header),
                      sizeof(SBR_HEADER_DATA) );
            }

            /* If header status equals reset and syncState equals UPSAMPLING it means that up until now
            SBR has at most been operated in upsampling mode. Now we will do the real thing, and hence
            we need to initialise start-up parameters in the frameData struct*/
            if ( headerStatus == HEADER_RESET && SbrChannel[sbrChannelIndex].syncState == UPSAMPLING){
              initFrameData(&SbrChannel[sbrChannelIndex]);
              initFrameData(&SbrChannel[sbrChannelIndex + 1]);
            }

            /* change of control data, reset decoder */
            if ( headerStatus == HEADER_RESET ) {
              for (lr = 0 ; lr < 2 ; lr++) {
                resetSbrDec ( &(SbrChannel[sbrChannelIndex+lr].frameData),
                              2.0f,
                              sbrChannelIndex+lr);
                /* At this point we have a header and the system has been reset,
                   hence syncState from now on will be SBR_ACTIVE.*/
                SbrChannel[sbrChannelIndex+lr].syncState = SBR_ACTIVE;
              }
            }

            if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE){
#ifdef SBR_SCALABLE
              sbrGetChannelPairElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf, pBitBufEnh,bStereoLayer);
#else
              sbrGetChannelPairElement_BSAC (&(SbrChannel[sbrChannelIndex].frameData),
                                        &(SbrChannel[sbrChannelIndex + 1].frameData), &bitBuf);
#endif
            }

            sbrChannelIndex++;
            break;

          default:
            return SBRDEC_ILLEGAL_PLUS_ELE_ID;     
      }
    }

    sbrChannelIndex++;
  }

  return SBRDEC_OK ;
}
/*******************************************************************************
 Functionname:applySbr
 *******************************************************************************

 Description: sbr decoder processing,
              set up SBR decoder phase 2 in case of different cotrol data

 Arguments:   

 Return:      errorCode, noError if successful

*******************************************************************************/
SBR_ERROR
applySBR (SBRDECODER self,
          SBRBITSTREAM * stream, 
          float** timeData,
          float** timeDataOut,
#ifdef SBR_SCALABLE
          int maxSfbFreqLine,
#endif
          int bDownSampledSbr,
#ifdef PARAMETRICSTEREO
          int sbrEnablePS,
#endif
          int numChannels
          /* 20060107 */
          ,int	core_bandwidth
          ,int bUsedBSAC
          )
{
  SBR_ERROR err = SBRDEC_OK ;

  int element, eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex;
  int processedChannels[MAX_TIME_CHANNELS];
  int i;

  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;
    
  memset(&processedChannels,0,MAX_TIME_CHANNELS*sizeof(int));

  /* We have already done a check (outside this function) to make sure that SBR data exists. 
     This is since this implementation of the implicit signalling does not assume that SBR 
     exists, but rather checks for its existence, and adjusts the output sample rate if 
     SBR data is found.

     However, if we had assumed that SBR data did exist, we would have to check now if it does
     i.e. stream->NrElements != 0, and if it doesn't we would have to use the SBR-Tool for upsampling,
     since we've already set up the output device at twice the sampling rate.
     */
  if (stream->NrElements)
  {
	  /* 20060107 */
	  if (bUsedBSAC==1)
			err = readSBR_BSAC (self, stream, numChannels) ;
	  else
			err = readSBR (self, stream, numChannels) ;
    if (err != SBRDEC_OK) return err ;
  }


  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements ; element++)
  {
    if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      eleChannels = (stream->sbrElement [element].ElementID == SBR_ID_CPE) ? 2 : 1 ;
#if 0
      channelIndex = stream->sbrElement [element].chIndx;
#else
      channelIndex = sbrChannelIndex;
#endif

      for(i=0; i<eleChannels; i++){
        decodeSbrData(SbrChannel[sbrChannelIndex + i].frameData.iEnvelope,
                      SbrChannel[sbrChannelIndex + i].frameData.sbrNoiseFloorLevel,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec1,
                      SbrChannel[sbrChannelIndex + i].frameData.domain_vec2,
                      SbrChannel[sbrChannelIndex + i].frameData.nSfb,
                      SbrChannel[sbrChannelIndex + i].frameData.frameInfo,
                      SbrChannel[sbrChannelIndex + i].frameData.nNfb,
                      SbrChannel[sbrChannelIndex + i].frameData.ampRes,
                      SbrChannel[sbrChannelIndex + i].frameData.nScaleFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.nNoiseFactors,
                      SbrChannel[sbrChannelIndex + i].frameData.offset,
                      SbrChannel[sbrChannelIndex + i].frameData.coupling,
                      sbrChannelIndex + i);
      }

      if(eleChannels == 2 && SbrChannel[sbrChannelIndex].frameData.coupling){
          sbr_envelope_unmapping (SbrChannel[sbrChannelIndex].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex+1].frameData.iEnvelope,
                                  SbrChannel[sbrChannelIndex].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex+1].frameData.sbrNoiseFloorLevel,
                                  SbrChannel[sbrChannelIndex].frameData.nScaleFactors,
                                  SbrChannel[sbrChannelIndex].frameData.nNoiseFactors,
                                  SbrChannel[sbrChannelIndex+1].frameData.ampRes);
              
      }

#ifdef PARAMETRICSTEREO
      if(sbrEnablePS /*eleChannels == 1 && SbrChannel[sbrChannelIndex].frameData.enablePS*/)
        tfDecodePs(self->hParametricStereoDec);
#endif

#ifdef SBR_SCALABLE
      eleChannels = numChannels;
#endif

      for(i=0; i<eleChannels; i++){
        sbr_dec (timeData[channelIndex + i],
                 timeDataOut[channelIndex + i],
                 &(SbrChannel[sbrChannelIndex + i].frameData),
                 (SbrChannel[sbrChannelIndex + i].syncState==SBR_ACTIVE),
                 sbrChannelIndex + i,
#ifdef SBR_SCALABLE
                 maxSfbFreqLine,
#endif
                 bDownSampledSbr
#ifdef PARAMETRICSTEREO
                 ,sbrEnablePS,
                 sbrEnablePS?timeDataOut[channelIndex + i + 1]:NULL,
                 self->hParametricStereoDec
#endif
				/* 20060107 */
                 ,core_bandwidth
                 ,bUsedBSAC
                 ) ;

        processedChannels[channelIndex + i] = 1;
      }

      sbrChannelIndex += eleChannels;
    }
  }



  /* 
  Do the LFE channel, if available, or channels were syncState != ACTIVE for which we do up-sampling only.
  */
  for (channelIndex = 0 ; channelIndex < numChannels ; channelIndex++)
  {
    if(!processedChannels[channelIndex]){
      sbr_dec (timeData[channelIndex],
               timeDataOut[channelIndex],
               NULL,  /* no SBR...*/
               0,
               sbrChannelIndex,
#ifdef SBR_SCALABLE
               maxSfbFreqLine,
#endif
               bDownSampledSbr
#ifdef PARAMETRICSTEREO
               ,sbrEnablePS,
               sbrEnablePS?timeDataOut[channelIndex + 1]:NULL,
               NULL
#endif
				/* 20060107 */
               ,core_bandwidth
               ,bUsedBSAC
               );

      sbrChannelIndex++;
      processedChannels[channelIndex] = 1;
    }
  }

  return err ;
}
