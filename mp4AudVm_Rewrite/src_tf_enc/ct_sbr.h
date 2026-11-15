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

 $Id: ct_sbr.h,v 1.1.2.2 2007/06/12 00:13:32 samsung1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr encoding functions $Revision: 1.1.2.2 $
*/

/* 20060107 */

#ifndef _CT_SBR_H
#define _CT_SBR_H

#undef PARAMETRICSTEREO /*temporally disabled for BSAC Extensions*/

#include "bitstream.h"
#include "ct_polyphase_enc.h"
#ifdef PARAMETRICSTEREO
#include "ct_psenc.h"
#endif

#define DELAY 4697
#define TIME_BUF_SIZE (640 + (32-1)*64 + DELAY)

#define MAX_ENVELOPES           5

#define MAX_FREQ_COEFFS         58

#define MAX_NOISE_ENVELOPES      2
#define MAX_NOISE_COEFFS         5

typedef struct {

  int sendHdrCnt;
  int sendHdrFrame;

  int startFreq;
  int stopFreq;
  int numBands;
  int *freqBandTable;
  int numNoiseBands;

  int bufOffset;
  float timeBuffer[TIME_BUF_SIZE];

  int envData[MAX_ENVELOPES][MAX_FREQ_COEFFS];
  int noiseData[MAX_NOISE_ENVELOPES][MAX_NOISE_COEFFS];

  int count;
  int esc_count;
  int fillBits;
  int totalBits;

  struct SBR_ENCODER_ANA_FILTERBANK sbrFbank;

#ifdef PARAMETRICSTEREO
  int psAvailable;
  PS_DATA psData;
#endif

} SBR_CHAN;

void SbrInit(int bitRate, int nChan, SBR_CHAN *sbrChan);

int SbrEncodeSingleChannel_BSAC(SBR_CHAN *sbrChan, 
                           float *samples
#ifdef PARAMETRICSTEREO
                           , float *samplesRight
#endif
                           );
/*
int SbrEncodeChannelPair(SBR_CHAN *sbrChanL, 
                         SBR_CHAN *sbrChanR, 
                         float *samplesL, 
                         float *samplesR);
*/
int SbrEncodeChannelPair_BSAC(SBR_CHAN *sbrChanL, 
                         SBR_CHAN *sbrChanR, 
                         float *samplesL, 
                         float *samplesR);


int WriteSbrSCE(SBR_CHAN *sbrChan,  
    			HANDLE_BSBITSTREAM fixedStream);  /* 20060107 */

int WriteSbrCPE(SBR_CHAN *sbrChanL,  
                SBR_CHAN *sbrChanR,
				HANDLE_BSBITSTREAM fixedStream);  /* 20060107 */

#endif
