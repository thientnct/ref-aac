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



*******************************************************************************/
/*!
  \file
  \brief  SBR decoder frontend $Revision: 1.1.2.5 $
*/


#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "adifdec.h"
#include "block.h"
#include "config.h"
#include "common.h"

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_sbrdecoder.h"
#include "ct_sbrdecoderConfTool.h"
#include "ct_envextr.h"
#include "ct_sbrdec.h"
#include "ct_envdec.h"
#include "ct_sbrcrc.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif

#ifdef BSAC
#include "..\sbrConfToolBSAC\BSAC_parse.h"
#endif


typedef struct
{
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


int fileIndex = -1;
const float conCriteria[][2] = {
  {5.0f,   1.4f}, /* sbr_qmf */
  {90.0f,  2.0f}, /* sbr_e   */ 
  {51.0f,  1.5f}, /* sbr_gh  */
  {36.0f,  3.4f}, /* sbr_i   */ 
  {120.0f, 1.9f}, /* sbr_s   */
  {2.0f,   1.0f}  /* sbr_twi */
#ifdef PARAMETRICSTEREO
  ,
  {8.0f,   1.7f},
  {12.0f,  1.8f}
#ifndef NO_PS_CONF_DCOR2
  /* these conformance criteria have been proposed in 14496-4:2004/Amd.11:2006/DCOR 2
   * balloting for this hasn't closed at the time of this writing, so they are not official.
   * Hence they can be removed from the build by defining NO_PS_CONF_DCOR2 in this file and in decoderConfTool.c
   */
  ,{30.0f,  3.6f},
  {45.0f,   3.7f},
  {12.0f,   1.8f},
  {40.0f,   3.6f},
  {40.0f,   3.7f}
#endif
#endif
};


extern float drcFactorVector[Chans][3][64];


/*******************************************************************************
 Functionname:applySbr
 *******************************************************************************

 Description: sbr decoder processing,
              set up SBR decoder phase 2 in case of different cotrol data

 Arguments:   

 Return:      errorCode, noError if successful

*******************************************************************************/
SBR_ERROR
readAndDecodeSBRConfTool (SBRDECODER self,
                  SBRBITSTREAM * stream, 
                  int bDownSampledSbr,
                  Ch_Info ch_info[10],
                  int* headerAvailable)
{
  SBR_ERROR err = SBRDEC_OK ;

  int element, eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex;
  int processedChannels[Chans];
  int i;

  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;
    
  memset(&processedChannels,0,Chans*sizeof(int));

  
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
    err = readSBR (self, stream) ;
    if (err != SBRDEC_OK) return err ;
  }


  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements ; element++)
  {
    if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      *headerAvailable = 1;
      eleChannels = (stream->sbrElement [element].ElementID == SBR_ID_CPE) ? 2 : 1 ;
      channelIndex = stream->sbrElement [element].chIndx;
  
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
     
      sbrChannelIndex += eleChannels;
    }
  }

  return err ;
}

#ifdef BSAC
extern int core_bandwidth;
#endif


/*******************************************************************************
 Functionname:applySBRConfTool
 *******************************************************************************

 Description: sbr decoder processing,
              set up SBR decoder phase 2 in case of different cotrol data

 Arguments:   

 Return:      errorCode, noError if successful

*******************************************************************************/
SBR_ERROR
applySBRConfTool (SBRDECODER self,
                  SBRBITSTREAM * stream, 
                  double* timeData[Chans],
                  double* timeDataOut[Chans],
                  int bDownSampledSbr,
                  Ch_Info ch_info[10],
#ifdef PARAMETRICSTEREO
                  int sbrEnablePS,
#endif
                  int storeIndex)
{
  SBR_ERROR err = SBRDEC_OK ;

  int element, eleChannels=0;
  int  sbrChannelIndex = 0;
  int channelIndex;
  int processedChannels[Chans];
  int i;

  SBR_CHANNEL *SbrChannel = &self->SbrChannel [0] ;
    
  memset(&processedChannels,0,Chans*sizeof(int));

 
  sbrChannelIndex = 0;
  for (element = 0 ; element < stream->NrElements ; element++)
  {
    if (SbrChannel[sbrChannelIndex].syncState == SBR_ACTIVE)
    {
      eleChannels = (stream->sbrElement [element].ElementID == SBR_ID_CPE) ? 2 : 1 ;
      channelIndex = stream->sbrElement [element].chIndx;
 
#ifdef PARAMETRICSTEREO
      if(sbrEnablePS /*eleChannels == 1 && SbrChannel[sbrChannelIndex].frameData.enablePS*/)
        DecodePs(self->hParametricStereoDec);
#endif

      for(i=0; i<eleChannels; i++){
#ifndef BSAC
        sbr_dec (timeData[channelIndex + i] + storeIndex + 384 + 481,
#else
		sbr_dec_bsac (timeData[channelIndex + i] + storeIndex + 384 + 481,
#endif
                 timeDataOut[channelIndex + i],
                 &(SbrChannel[sbrChannelIndex + i].frameData),
                 (SbrChannel[sbrChannelIndex + i].syncState==SBR_ACTIVE),
                 sbrChannelIndex + i,
                 bDownSampledSbr,
                 drcFactorVector[channelIndex]
#ifdef PARAMETRICSTEREO
                 ,sbrEnablePS,
                 sbrEnablePS?timeDataOut[channelIndex + i + 1]:NULL,
                 self->hParametricStereoDec
#endif
#ifdef BSAC
				 ,core_bandwidth
#endif
                 );

        processedChannels[channelIndex + i] = 1;
      }


      sbrChannelIndex += eleChannels;
    }
  }


  /* 
  Do the LFE channel, if available, or channels were syncState != ACTIVE for which we do up-sampling only.
  */
  for (channelIndex = 0 ; channelIndex < Chans ; channelIndex++)
  {
    if(ch_info[channelIndex].present && !processedChannels[channelIndex]){
      sbr_dec (timeData[channelIndex] + storeIndex + 384 + 481,
               timeDataOut[channelIndex],
               NULL,  /* no SBR...*/
               0,
               sbrChannelIndex,
               bDownSampledSbr,
               drcFactorVector[channelIndex]
#ifdef PARAMETRICSTEREO
               ,sbrEnablePS,
               sbrEnablePS?timeDataOut[channelIndex + 1]:NULL,
               NULL
#endif
               );

      sbrChannelIndex++;
      processedChannels[channelIndex] = 1;
    }
  }



  for(i= 0; i < Chans; i++){
    memcpy(drcFactorVector[i][0], drcFactorVector[i][1],64*sizeof(float));
    memcpy(drcFactorVector[i][1], drcFactorVector[i][2],64*sizeof(float));
  }

  return err ;
}

#ifdef PARAMETRICSTEREO
  extern int doPsConformance;
#endif

void
calculateDiffandRms(double* timeData[Chans],
                    double* timeDataRef[Chans],
                    Ch_Info ch_info[10],
                    int frameSize)
{
  int channelIndex, i;
  static double maxDiff = 0;
  int maxPos = 0;
  static double totalDiff = 0;
  static int count = 0;
  double diff;
  extern char result[256];
  int tmp;

  for (channelIndex = 0 ; channelIndex < Chans ; channelIndex++){
    if(ch_info[channelIndex].present
#ifdef PARAMETRICSTEREO
       || (doPsConformance && channelIndex > 0 && ch_info[channelIndex - 1].present)
#endif
       ){
      for(i= 0; i < frameSize; i++){
        diff = fabs(timeDataRef[channelIndex][i ] - timeData[channelIndex][i]);
        totalDiff += pow(timeDataRef[channelIndex][i ] - timeData[channelIndex][i],2);

        count++;
        if(diff > maxDiff){
          maxPos = i;
          maxDiff = diff;
        }
      }
    }
  }

  if (fileIndex == 0 || fileIndex == 5) {
    tmp = sprintf(result, "%s  QMF-TEST: Max Diff: %.3f (threshold %.3f) RMS Diff: %.3f  (threshold %.3f)", GREP_STRING, maxDiff,conCriteria[fileIndex][0], sqrt(totalDiff/count), conCriteria[fileIndex][1]);
  }
  else if (fileIndex >=1 && fileIndex <=4) {
    tmp = sprintf(result, "%s  SBR-TEST: Max Diff: %.3f (threshold %.3f) RMS Diff: %.3f  (threshold %.3f)", GREP_STRING, maxDiff,conCriteria[fileIndex][0], sqrt(totalDiff/count), conCriteria[fileIndex][1]);
  }
#ifdef PARAMETRICSTEREO
  else if (fileIndex >=6 && fileIndex <=
#ifndef NO_PS_CONF_DCOR2
           12
#else
           7
#endif
           ) {
    tmp = sprintf(result, "%s  SBR+PS-TEST: Max Diff: %.3f (threshold %.3f) RMS Diff: %.3f  (threshold %.3f)", GREP_STRING, maxDiff,conCriteria[fileIndex][0], sqrt(totalDiff/count), conCriteria[fileIndex][1]);
  }
#endif
  else {
    tmp = sprintf(result, "%s  SBR-TEST: Max Diff: %.3f RMS Diff: %.3f", GREP_STRING, maxDiff, sqrt(totalDiff/count));
  }
  if (fileIndex != -1) {
    if (fileIndex != 5) {
      if (maxDiff < conCriteria[fileIndex][0] && sqrt(totalDiff/count) < conCriteria[fileIndex][1])
        tmp += sprintf(result + tmp, ", -> Conformance PASSED");
      else
        tmp += sprintf(result + tmp, ", -> Conformance FAILED");
    }
#if !defined LOW_POWER_SBR
    else {
      if (maxDiff < conCriteria[fileIndex][0] && sqrt(totalDiff/count) < conCriteria[fileIndex][1])
        tmp += sprintf(result + tmp, ", -> Twiddle estimation succeded");
      else
        tmp += sprintf(result + tmp, ", -> Twiddle estimation might not have succeeded; later conformance tests might fail for this reason.");
    }
#endif
  }
  memset(result + tmp, 0, sizeof(result) - tmp);
}



void
calculateDiffandRmsQmf(double* timeData[Chans],
                       double* timeDataRef[Chans],
                       Ch_Info ch_info[10],
                       int frameSize)
{
  int channelIndex, i;
  static double maxDiffQmf = 0;
  static double totalDiffQmf = 0;
  static int countQmf = 0;
  int tmp;

  extern char resultQmf[256];

  for (channelIndex = 0 ; channelIndex < Chans ; channelIndex++)
  {
    if(ch_info[channelIndex].present
#ifdef PARAMETRICSTEREO
       || (doPsConformance && channelIndex > 0 && ch_info[channelIndex - 1].present)
#endif
       ){
      for(i= 0; i < frameSize; i++){
        double diff = fabs(timeDataRef[channelIndex][i] - timeData[channelIndex][i]);
        totalDiffQmf += pow(timeDataRef[channelIndex][i] - timeData[channelIndex][i],2);

        countQmf++;
        if(diff > maxDiffQmf){
          maxDiffQmf = diff;
        }
      }
    }
  }
  
  if (fileIndex != -1) {
    tmp = sprintf(resultQmf, "%s  (Informative) QMF-TEST: Max Diff: %.3f (%.3f) RMS Diff: %.3f  (%.3f)", GREP_STRING, maxDiffQmf,conCriteria[fileIndex][0], sqrt(totalDiffQmf/countQmf), conCriteria[fileIndex][1]);
  }
  else {
    tmp = sprintf(resultQmf, "%s  (Informative) QMF-TEST: Max Diff: %.3f RMS Diff: %.3f", GREP_STRING, maxDiffQmf, sqrt(totalDiffQmf/countQmf));
  }

  memset(resultQmf + tmp, 0, sizeof(resultQmf) - tmp);

}

Twiddle
identifyQmfBank( const float *inData,
                 int noChannels,
                 int bDownSampledSbr,
                 int k0,              /* first QMF channel in analysis, zero-index based            */
                 int k1 )             /* last QMF channel in analysis (exclusive), zero-index based */

{
  Twiddle twiddle; 
  int n, k, nch, K, L;
  int F = 32768;        /* analysis length        */
  int M =    64;        /* QMF bank size          */

  double s0, s1, a, b;
  double Hr[64] = { 0 };
  double Hi[64] = { 0 };
  double Ph[64] = { 0 };

  if(bDownSampledSbr) {
    F /= 2;
    M /= 2;
    k1 = MIN(32, k1);
  }
  
  /* compute DFT with frequency offset */
  for(k = k0; k < k1; k++) {
    for(n = 0; n < F; n++) {
      Hr[k] += inData[n * noChannels] * cos(2 * PI / (2*M) * (k+0.5) * n);
      Hi[k] -= inData[n * noChannels] * sin(2 * PI / (2*M) * (k+0.5) * n);
    } 
  }

  /* compute unwrapped phase */
  for(k = k0; k < k1; k++) {
    
    Ph[k] = atan2( Hi[k], Hr[k] ) / PI;
    
    if(Ph[k] > Ph[k-1])
      while(Ph[k] - Ph[k-1] > 1)
        Ph[k] -= 2;
    else
      while(Ph[k-1] - Ph[k] > 1)
        Ph[k] += 2;
  }

  /* linear curve fitting */
  nch = k1 - k0;                                               /* number of channels in analysis */

  /* compute factors */
  K = ( k1 * (k1-1) * (2*k1-1) - k0 * (k0-1) * (2*k0-1) ) / 6; /* Sum(k^2) from k0 to k1-1. */
  L = ( k1 * (k1-1) - k0 * (k0-1) ) / 2;                       /* Sum(k)   from k0 to k1-1. */
  
  /* compute synthesis twiddle f(k) = a * k + b */
  s0 = s1 = 0;
  for(k = k0; k < k1; k++) {
    s0 += k * Ph[k];
    s1 += Ph[k];
  }

  a = ( nch * s0 - L * s1 ) / (K * nch - L * L);
  b = ( s0 - K * a ) / L;

  /* rewrite phase j*pi*(a*k+b) as j*pi/L*((k+0.5)*fi+beta). */ 
  twiddle.fi   = M * a;          
  twiddle.beta = M * b - M/2 * a;

  /* Round fi in 1/M steps */
  if(twiddle.fi >= 0)
    twiddle.fi = (int) (M * twiddle.fi   + 0.5);          
  else
    twiddle.fi = (int) (M * twiddle.fi   - 0.5);          

  twiddle.fi  *= (1.0 / M);          
  
  /* Round beta in steps of one */
  if(twiddle.beta >= 0)
    twiddle.beta  = (int) (twiddle.beta + 0.5);     
  else
    twiddle.beta  = (int) (twiddle.beta - 0.5);     

  /* align sign of beta with fi (optional) */
  if(twiddle.fi > 0) {
    while(twiddle.beta < 0)
      twiddle.beta += 2 * M;
  } else if(twiddle.fi < 0) {
    while(twiddle.beta > 0)
      twiddle.beta -= 2 * M;
  }

  return twiddle;
}

