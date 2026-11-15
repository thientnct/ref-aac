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

 $Id: ct_sbrdecoderConfTool.h,v 1.1.2.4 2004/01/19 16:00:48 kk Exp $

*******************************************************************************/
/*!
  \file
  \brief  SBR decoder frontend prototypes and definitions $Revision: 1.1.2.4 $
*/
#ifndef __SBRDECODERCONFTOOL_H
#define __SBRDECODERCONFTOOL_H

#include "ct_sbrdecoder.h"
#include "ct_polyphase.h"


#ifdef __cplusplus
extern "C"
{
#endif


  SBR_ERROR
  readAndDecodeSBRConfTool (SBRDECODER self,
                  SBRBITSTREAM * stream, 
                  int bDownSampledSbr,
                  Ch_Info ch_info[10],
                  int* headerAvailable);

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
                  int storeIndex);

  void
  calculateDiffandRms(double* timeDataRef[Chans],
                      double* timeData[Chans],
                      Ch_Info ch_info[10],
                      int frameSize);

  void
  calculateDiffandRmsQmf(double* timeDataRef[Chans],
                      double* timeData[Chans],
                      Ch_Info ch_info[10],
                      int frameSize);

  Twiddle
  identifyQmfBank( const float *inData,
                   int noChannels,
                   int bDownSampledSbr,
                   int k0,              
                   int k1 );             

#ifdef __cplusplus
}
#endif

#endif
