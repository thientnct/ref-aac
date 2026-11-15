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

 $Id: ct_envextr.h,v 1.5 2003/07/15 17:16:32 kaehleof Exp $

*******************************************************************************/
/*!
  \file
  \brief  Envelope extraction prototypes $Revision: 1.5 $
*/

#ifndef __CT_ENVEXTR_H
#define __CT_ENVEXTR_H

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_sbrdecoder.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif

typedef enum
{
  HEADER_OK,
  HEADER_RESET
}
SBR_HEADER_STATUS;


typedef enum
{
  INVF_OFF,
  INVF_LOW_LEVEL,
  INVF_MID_LEVEL,
  INVF_HIGH_LEVEL,

  INVF_NO_OVERRIDE
}
INVF_MODE;


typedef enum
{
  COUPLING_OFF,
  COUPLING_LEVEL,
  COUPLING_BAL
}
COUPLING_MODE;


typedef struct
{
  SBR_HEADER_STATUS status;      /*!< the current status of the header     */

  /* Changes in these variables indicates an error */
  int crcEnable;
  int ampResolution;

  /* Changes in these variables causes a reset of the decoder */
  int startFreq;
  int stopFreq;
  int xover_band;
  int freqScale;
  int alterScale;
  int noise_bands;               /*!< noise bands per octave, read from bitstream */

  /* Helper variable*/
  int noNoiseBands;              /*!< actual number of noise bands to read from the bitstream */

  int limiterBands;
  int limiterGains;
  int interpolFreq;
  int smoothingLength;
}
SBR_HEADER_DATA;

typedef SBR_HEADER_DATA *HANDLE_SBR_HEADER_DATA;



typedef struct
{
  int nScaleFactors;            /* total number of scalefactors in frame */
  int nNoiseFactors;
  int crcCheckSum;
  int frameClass;
  int frameInfo[LENGTH_FRAME_INFO];
  int nSfb[2];
  int nNfb;
  int offset;
  int ampRes;
  int nNoiseFloorEnvelopes;
  int p;
  int prevEnvIsShort;

  SBR_HEADER_DATA sbr_header;


  /* dynamic control signals */
  int domain_vec1[MAX_ENVELOPES];       
  int domain_vec2[MAX_ENVELOPES];       


  INVF_MODE sbr_invf_mode[MAX_NUM_NOISE_VALUES];
  INVF_MODE sbr_invf_mode_prev[MAX_NUM_NOISE_VALUES];

  COUPLING_MODE coupling;               /*!< 3 possibilities: off, level, pan */

  /* envelope data */
  float iEnvelope[MAX_NUM_ENVELOPE_VALUES];
  float sbrNoiseFloorLevel[MAX_NUM_NOISE_VALUES];
  int addHarmonics[MAX_NUM_ENVELOPE_VALUES];

  float sfb_nrg_prev[MAX_FREQ_COEFFS];  
  float prevNoiseLevel[MAX_NUM_NOISE_VALUES];

}
SBR_FRAME_DATA;

typedef SBR_FRAME_DATA *HANDLE_SBR_FRAME_DATA;



void sbrGetSingleChannelElement (HANDLE_SBR_FRAME_DATA hFrameData,
                                 HANDLE_BIT_BUFFER hBitBuf
#ifdef PARAMETRICSTEREO
                                 ,HANDLE_PS_DEC hParametricStereoDec
#endif
                                 );

void sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                              HANDLE_SBR_FRAME_DATA hFrameDataRight,
                              HANDLE_BIT_BUFFER hBitBuf);

SBR_HEADER_STATUS
sbrGetHeaderData (SBR_HEADER_DATA *h_sbr_header,
                  HANDLE_BIT_BUFFER hBitBuf,
                  SBR_ELEMENT_ID id_sbr,
                  SBR_SYNC_STATE syncState);




#endif
