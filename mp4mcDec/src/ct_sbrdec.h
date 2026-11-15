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

 $Id: ct_sbrdec.h,v 1.4 2003/07/15 14:43:14 ct1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr decoder $Revision: 1.4 $
*/
#ifndef __CT_SBRDEC_H
#define __CT_SBRDEC_H

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_envcalc.h"

void sbr_dec ( double* ftimeInPtr,
               double* ftimeOutPtr,
               HANDLE_SBR_FRAME_DATA hFrameData,
               int applyProcessing,
               int channel,
               int bDownSampledSbr,
               float drcFactorVector[3][64]
#ifdef PARAMETRICSTEREO
               ,int sbrEnablePS,
               double* ftimeOutPtrPS,
               HANDLE_PS_DEC hParametricStereoDec
#endif
               );

void
initSbrDec ( int codecSampleRate,
             int bDownSampledSbr,
             const struct _Twiddle *twiddle);

void
resetSbrDec ( HANDLE_SBR_FRAME_DATA hFrameData,
              float upSampleFac,
              int channel);

  

int getStartBand (int ch);

int getStopBand  (int ch);

/*only for BSAC Extensions*/
void sbr_dec_bsac ( double* ftimeInPtr,
               double* ftimeOutPtr,
               HANDLE_SBR_FRAME_DATA hFrameData,
               int applyProcessing,
               int channel,
               int bDownSampledSbr,
               float drcFactorVector[3][64]
#ifdef PARAMETRICSTEREO
               ,int sbrEnablePS,
               double* ftimeOutPtrPS,
               HANDLE_PS_DEC hParametricStereoDec
#endif
			   , int core_bandwidth
               );

#endif
