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

 $Id: ct_sbrdec.c,v 1.1.2.10 2008/08/08 16:42:37 sps Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr decoder $Revision: 1.1.2.10 $
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <assert.h>

#include "ct_sbrdec.h"
#include "ct_envextr.h"
#include "ct_envcalc.h"
#include "ct_sbrdecoder.h"
#include "ct_freqsca.h"
#include "ct_hfgen.h"
#include "ct_polyphase.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif


#ifndef FALSE
#define FALSE              0
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static int FreqBandTable[MAX_NUM_CHANNELS][2][MAX_FREQ_COEFFS + 1];
static int NSfb[MAX_NUM_CHANNELS][2];
static int FreqBandTableNoise[MAX_NUM_CHANNELS][MAX_NOISE_COEFFS + 1];
static int NoNoiseBands[MAX_NUM_CHANNELS];                                 /* Number of noisebands */
static int V_k_master[MAX_NUM_CHANNELS][MAX_FREQ_COEFFS + 1];              /* Master BandTable which freqBandTable is derived from*/
static int Num_Master[MAX_NUM_CHANNELS];                                   /* Number of bands in V_k_master*/

static int Reset[MAX_NUM_CHANNELS];

static struct SBR_DEC
{
  int coreCodecFrameSize;
  int outSampleRate;

  int startIndexCodecQmf;
  int lowBandAddSamples;
  int noCols;
  int qmfBufLen;
  int bufWriteOffs;
  int bufReadOffs;

  int sbStopCodec[MAX_NUM_CHANNELS];
  int lowSubband[MAX_NUM_CHANNELS];
  int prevLowSubband[MAX_NUM_CHANNELS];
  int highSubband[MAX_NUM_CHANNELS];
  int noSubbands[MAX_NUM_CHANNELS];

#ifdef SBR_SCALABLE
  int prevMaxQmfSubbandAac[MAX_NUM_CHANNELS];
#endif

  float ***qmfBufferReal;
  float ***qmfBufferImag;
} sbrDec;

static void
resetSbrFreqBandTables(SBR_HEADER_DATA *headerData,
                       int samplingFreq,
                       int channel);




/*******************************************************************************
 Functionname:  sbr_dec
 *******************************************************************************
 
 Description:   sbr decoder core function 
 
 Arguments:     

 Return:        none

*******************************************************************************/
void
sbr_dec ( float* ftimeInPtr, 
          float* ftimeOutPtr,
          HANDLE_SBR_FRAME_DATA hFrameData,
          int applyProcessing,
          int channel
#ifdef SBR_SCALABLE
          ,int maxSfbFreqLine
#endif
          ,int bDownSampledSbr
#ifdef PARAMETRICSTEREO
          ,int sbrEnablePS,
          float* ftimeOutPtrPS,
          HANDLE_PS_DEC hParametricStereoDec
#endif
		  /* 20060107 */
          ,int	core_bandwidth
          ,int	bUsedBSAC
          )
{
  int i, k;
  int frameMove = sbrDec.lowBandAddSamples;

  float ***qmfBufferReal = sbrDec.qmfBufferReal;
  float ***qmfBufferImag = sbrDec.qmfBufferImag;
  
  static float lowBandBuffer[MAX_NUM_CHANNELS][2624];   /* 2048 + 576 */
  
  static float codecQmfBufferReal[MAX_NUM_CHANNELS][64][64];
  static float codecQmfBufferImag[MAX_NUM_CHANNELS][64][64];
  static float sbrQmfBufferReal[MAX_NUM_CHANNELS][64][64];
  static float sbrQmfBufferImag[MAX_NUM_CHANNELS][64][64];

  int *frameInfo = NULL;

  /* 20060107 */
  int	core_syn_ch_index =0;

#ifdef LOW_POWER_SBR
  float degreeAlias[MAX_NUM_CHANNELS][64];
#endif

#ifdef SBR_SCALABLE
  int maxQmfSubbandAac = (int) ( maxSfbFreqLine*32.0f /(sbrDec.coreCodecFrameSize));
#endif

  if(applyProcessing)
    frameInfo = hFrameData->frameInfo;

  /* 20060107 */
	if (bUsedBSAC==1)
	{
		  if (core_bandwidth !=0)
		  {
			  core_syn_ch_index = 64 * (core_bandwidth*2 -1) / (float)sbrDec.outSampleRate;
			  core_syn_ch_index = max(core_syn_ch_index-1,sbrDec.lowSubband[channel]);
		  }
	}
	else
		core_syn_ch_index = sbrDec.lowSubband[channel];

  /*
    Update buffers
  */
  memmove (lowBandBuffer[channel], lowBandBuffer[channel] + sbrDec.coreCodecFrameSize, 
           frameMove * sizeof (float));
  
  for (i = 0 ; i < sbrDec.coreCodecFrameSize ; i++) {
    lowBandBuffer[channel] [frameMove + i] = (float) ftimeInPtr [i] ;
  }



  /*
    low band codec signal subband filtering
  */
  if (applyProcessing) {
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      CalculateSbrAnaFilterbank( lowBandBuffer[channel] + sbrDec.startIndexCodecQmf + i * 32,
                                 codecQmfBufferReal[channel][sbrDec.bufWriteOffs + i],
                                 codecQmfBufferImag[channel][sbrDec.bufWriteOffs + i],
                                 channel,
#ifdef SBR_SCALABLE
                                 32
#else
                                 core_syn_ch_index  /* 20060107 */
#endif
#ifdef AAC_ELD
                                 ,hFrameData->ldsbr
#endif  
				);

    }
  }
  else {
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      CalculateSbrAnaFilterbank( lowBandBuffer[channel] + sbrDec.startIndexCodecQmf + i * 32,
                                 codecQmfBufferReal[channel][sbrDec.bufWriteOffs + i],
                                 codecQmfBufferImag[channel][sbrDec.bufWriteOffs + i],
                                 channel,
                                 32
#ifdef AAC_ELD
				,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif
				 );
    }
  }

  if (applyProcessing) {
    /*
     Inverse filtering of lowband + HF generation
    */
    generateHF ( codecQmfBufferReal[channel] + sbrDec.bufReadOffs,
                 codecQmfBufferImag[channel] + sbrDec.bufReadOffs,
                 sbrQmfBufferReal[channel] + sbrDec.bufReadOffs,
                 sbrQmfBufferImag[channel] + sbrDec.bufReadOffs,
                 hFrameData->sbr_invf_mode,
                 hFrameData->sbr_invf_mode_prev,
                 &FreqBandTableNoise[channel][1],
                 NoNoiseBands[channel],
                 sbrDec.lowSubband[channel],
                 V_k_master[channel],
                 Num_Master[channel],
                 sbrDec.outSampleRate,      
                 frameInfo,
#ifdef LOW_POWER_SBR
                 degreeAlias[channel],
#endif
                 channel
#ifdef AAC_ELD
                 ,hFrameData->numTimeSlots
                 ,hFrameData->ldsbr
#endif
                 );



#ifdef LOW_POWER_SBR
    for (i = 2*hFrameData->frameInfo[1]; i<2*hFrameData->frameInfo[hFrameData->frameInfo[0]+1]; i++) {
      memset (sbrQmfBufferReal[channel][sbrDec.bufReadOffs+i], 0, sbrDec.lowSubband[channel] * sizeof (float));
    }
#endif

   /*
      Adjust envelope of current frame.
    */
    calculateSbrEnvelope (hFrameData,
                          sbrQmfBufferReal[channel] + sbrDec.bufReadOffs, 
                          sbrQmfBufferImag[channel] + sbrDec.bufReadOffs,
                          FreqBandTable[channel],
                          NSfb[channel],
                          FreqBandTableNoise[channel],
                          NoNoiseBands[channel],
                          Reset[channel],
#ifdef LOW_POWER_SBR
                          degreeAlias[channel],
#endif
                          channel);

  }
  else {
    /* no sbr, set high band buffers to zero */
    for (i = 0; i < 64; i++) {
      memset (sbrQmfBufferReal[channel][i], 0, 64 * sizeof (float)); 
      memset (sbrQmfBufferImag[channel][i], 0, 64 * sizeof (float));
    }
  }



  /*
    Synthesis subband filtering.
  */
  for ( i = 0; i < sbrDec.noCols; i++ ) {
    int   xoverBand;

    if(applyProcessing){
#ifdef SBR_SCALABLE
      if ( i < 2*hFrameData->frameInfo[1] )
        xoverBand = max(sbrDec.prevLowSubband[channel],sbrDec.prevMaxQmfSubbandAac[channel]);
      else
        xoverBand = max(sbrDec.lowSubband[channel],maxQmfSubbandAac);
#else
      if ( i < 2*hFrameData->frameInfo[1] )
        xoverBand = sbrDec.prevLowSubband[channel];
      else
        xoverBand = sbrDec.lowSubband[channel];

	  /* 20060107 */
		if (bUsedBSAC)
		  xoverBand = core_syn_ch_index;

#endif
    }
    else{
      xoverBand = 32;
    }



    for ( k = 0; k < 64; k++ ) {
      qmfBufferReal[0][i][k] = 0.0;
      qmfBufferImag[0][i][k] = 0.0;
      if ( k < xoverBand ) {
        qmfBufferReal[0][i][k] += codecQmfBufferReal[channel][sbrDec.bufReadOffs+i][k];
        qmfBufferImag[0][i][k] += codecQmfBufferImag[channel][sbrDec.bufReadOffs+i][k];
      }

      if ( k >= xoverBand 
#ifdef LOW_POWER_SBR
           - 1
#endif
           ) 
      {
        qmfBufferReal[0][i][k] += sbrQmfBufferReal[channel][sbrDec.bufReadOffs+i][k];
        qmfBufferImag[0][i][k] += sbrQmfBufferImag[channel][sbrDec.bufReadOffs+i][k];
      }
    }
  }

#ifdef PARAMETRICSTEREO
  if (sbrEnablePS) {
    for ( i = 32; i < 32+6; i++ ) {
      for ( k = 0; k < 5; k++ ) {
        qmfBufferReal[0][i][k] = codecQmfBufferReal[channel][sbrDec.bufReadOffs+i][k];
        qmfBufferImag[0][i][k] = codecQmfBufferImag[channel][sbrDec.bufReadOffs+i][k];
      }
    }
  }


  /*
    Parametric stereo processing.
  */
  if (sbrEnablePS && applyProcessing) {
    tfApplyPsFrame(hParametricStereoDec,
                 qmfBufferReal[0],
                 qmfBufferImag[0],
                 qmfBufferReal[1],
                 qmfBufferImag[1],
                 sbrDec.highSubband[0]);
  }
  else {
    /* no PS, output L=R=M */
    for ( i = 0; i < sbrDec.noCols; i++ ) {
      for ( k = 0; k < 64; k++ ) {
        qmfBufferReal[1][i][k] = qmfBufferReal[0][i][k];
        qmfBufferImag[1][i][k] = qmfBufferImag[0][i][k];
      }
    }
  }
#endif



  /*
    Synthesis subband filtering.
  */
  for ( i = 0; i < sbrDec.noCols; i++ ) {
    if(bDownSampledSbr){
      CalculateSbrSynFilterbank(qmfBufferReal[0][i],
                                qmfBufferImag[0][i],
                                ftimeOutPtr + i * 32,
                                bDownSampledSbr,
                                channel
#ifdef AAC_ELD
				,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif			       			       
                                );
    }
    else{
      CalculateSbrSynFilterbank(qmfBufferReal[0][i],
                                qmfBufferImag[0][i],
                               ftimeOutPtr + i * 64,
                               bDownSampledSbr,
                               channel
#ifdef AAC_ELD
                               ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif			       
			       );

    }

#ifdef PARAMETRICSTEREO
    if (sbrEnablePS) {

      if(bDownSampledSbr){
        CalculateSbrSynFilterbank(qmfBufferReal[1][i],
                                  qmfBufferImag[1][i],
                                  ftimeOutPtrPS + i * 32,
                                  bDownSampledSbr,
                                  channel+1
#ifdef AAC_ELD
                                  ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif				  
				  );
      }
      else{
        CalculateSbrSynFilterbank(qmfBufferReal[1][i],
                                  qmfBufferImag[1][i],
                                  ftimeOutPtrPS + i * 64,
                                  bDownSampledSbr,
                                  channel+1
#ifdef AAC_ELD
                                  ,(hFrameData==NULL) ? 0 : hFrameData->ldsbr
#endif				  
				  );
      }
    }
#endif
  }

  /*
    Update Buffers
  */
  for (i = 0; i < sbrDec.bufWriteOffs; i++) {
    memmove (codecQmfBufferReal[channel][i],
             codecQmfBufferReal[channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (codecQmfBufferImag[channel][i],
             codecQmfBufferImag[channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (sbrQmfBufferReal[channel][i],
             sbrQmfBufferReal[channel][sbrDec.noCols + i],
             64 * sizeof (float));

    memmove (sbrQmfBufferImag[channel][i],
             sbrQmfBufferImag[channel][sbrDec.noCols + i],
             64 * sizeof (float));

  }

  Reset[channel] = 0;  
  
  if(applyProcessing){
    sbrDec.prevLowSubband[channel] = sbrDec.lowSubband[channel];

#ifdef SBR_SCALABLE
    sbrDec.prevMaxQmfSubbandAac[channel] = maxQmfSubbandAac;
#endif
  }
}


/*******************************************************************************
 Functionname:  initSbrDec
 *******************************************************************************
 
 Description:   initializes sbr decoder structure 
 
 Arguments:     
 
 Return:        none

*******************************************************************************/
void
initSbrDec ( int codecSampleRate,
             int bDownSampledSbr,
             int coreCodecFrameSize
#ifdef AAC_ELD
             ,int ldsbr
#endif
             )
{
  int outFrameSize,i,j;
  int upsampleFac = 2;
  int qmfBufLen;

  sbrDec.coreCodecFrameSize = coreCodecFrameSize;
  
  for(i=0;i<MAX_NUM_CHANNELS;i++){
    sbrDec.prevLowSubband[i] = (int) (upsampleFac * 32);  
    sbrDec.sbStopCodec[i] = (int) (upsampleFac * 32);
  }


  /* set sbr sampling frequency */
  sbrDec.outSampleRate = (int) (upsampleFac * codecSampleRate);
  outFrameSize = (int) (upsampleFac * coreCodecFrameSize);        

#ifdef AAC_ELD
  InitSbrAnaFilterbank (ldsbr);
#else
  InitSbrAnaFilterbank ();  
#endif
  InitSbrSynFilterbank (bDownSampledSbr
#ifdef AAC_ELD
			,ldsbr
#endif  
  			);

#ifdef AAC_ELD
  if(ldsbr==1 && upsampleFac==2) {
    qmfBufLen = 32;
  } else {
#endif
    qmfBufLen = 32+6;
#ifdef AAC_ELD
  }
#endif

  /* allocate qmf memory */
  sbrDec.qmfBufferReal  = (float ***) calloc (2, sizeof (float **));
  sbrDec.qmfBufferImag  = (float ***) calloc (2, sizeof (float **));
  for (i=0 ; i < 2 ; i++) {
    sbrDec.qmfBufferReal[i] = (float **) calloc (qmfBufLen, sizeof (float*));
    sbrDec.qmfBufferImag[i] = (float **) calloc (qmfBufLen, sizeof (float*));
    for (j=0 ; j < qmfBufLen ; j++) {
      sbrDec.qmfBufferReal[i][j] = (float *) calloc (64, sizeof (float));
      sbrDec.qmfBufferImag[i][j] = (float *) calloc (64, sizeof (float));
    }
  }
  
  /* Direct assignments */

  sbrDec.noCols             = outFrameSize / 64;
#ifdef AAC_ELD
  if(ldsbr==1) {
    sbrDec.bufWriteOffs       = 2;
  } else {
#endif
    sbrDec.bufWriteOffs       = 6+2;
#ifdef AAC_ELD
  }
#endif
  sbrDec.bufReadOffs        = 2;
  sbrDec.qmfBufLen          = sbrDec.noCols + sbrDec.bufWriteOffs;
  sbrDec.lowBandAddSamples  = 288;
  sbrDec.startIndexCodecQmf = 0;
}


/*******************************************************************************
 Functionname:  resetSbrDec
 *******************************************************************************
 
 Description:   resets sbr decoder structure 
 
 Arguments:     
 
 Return:        errorCode, noError if successful

 *******************************************************************************/
void
resetSbrDec ( HANDLE_SBR_FRAME_DATA hFrameData,
              float upsampleFac,
              int channel)
{ 
  Reset[channel] = 1;

  resetSbrFreqBandTables ( &(hFrameData->sbr_header),
                           sbrDec.outSampleRate,
                           channel); 

  sbrDec.sbStopCodec[channel] = sbrDec.lowSubband[channel];
  if (sbrDec.sbStopCodec[channel] > (int) (upsampleFac * 32) ) {
    sbrDec.sbStopCodec[channel] = (int) (upsampleFac * 32);
  }

  hFrameData->nSfb[LO] = NSfb[channel][LO];
  hFrameData->nSfb[HI] = NSfb[channel][HI];
  hFrameData->nNfb     = hFrameData->sbr_header.noNoiseBands;
  hFrameData->offset   = 2* hFrameData->nSfb[LO] - hFrameData->nSfb[HI];
}


/*******************************************************************************
 Functionname:  resetSbrFreqBandTables
 *******************************************************************************
 
 Description:   
 Arguments:     
 Return:        errorCode, noError if successful
 Revised:       

*******************************************************************************/
void
resetSbrFreqBandTables ( SBR_HEADER_DATA *headerData,
                         int samplingFreq,
                         int channel)
{

  int lsbM, lsb, usb;


  /*Calculate master frequency function */
  sbrdecFindStartAndStopBand(samplingFreq,
                             headerData->startFreq,
                             headerData->stopFreq,
                             &lsbM, &usb);

  sbrdecUpdateFreqScale(V_k_master[channel], &Num_Master[channel],
                          lsbM, usb, headerData->freqScale,
                          headerData->alterScale, 0);


  /*Derive Hiresolution from master frequency function*/
  sbrdecUpdateHiRes(FreqBandTable[channel][HI], &NSfb[channel][HI],
                    V_k_master[channel], Num_Master[channel],
                    headerData->xover_band );
  /*Derive  Loresolution from Hiresolution*/
  sbrdecUpdateLoRes(FreqBandTable[channel][LO], &NSfb[channel][LO],
                    FreqBandTable[channel][HI], NSfb[channel][HI]);
        
 
  lsb = FreqBandTable[channel][LOW_RES][0];
  usb = FreqBandTable[channel][LOW_RES][NSfb[channel][LOW_RES]];

  sbrDec.lowSubband[channel]  = lsb;
  sbrDec.highSubband[channel] = usb;
  sbrDec.noSubbands[channel]  = usb - lsb;

  /*Calculate number of noise bands*/
  if(headerData->noise_bands == 0)
  {
    NoNoiseBands[channel] = 1;
  }
  else /* Calculate nor of noise bands 1,2 or 3 bands/octave */
  {
    NoNoiseBands[channel] = (int) ( headerData->noise_bands * log((double) usb / lsb) / log(2.0) + 0.5 );
    
    if( NoNoiseBands[channel] == 0) {
      NoNoiseBands[channel] = 1;
    }
  }

  headerData->noNoiseBands = NoNoiseBands[channel];
  
  /* Get noise bands */
  sbrdecDownSampleLoRes(FreqBandTableNoise[channel], 
                        NoNoiseBands[channel],
                        FreqBandTable[channel][LO],
                        NSfb[channel][LO]);

}


