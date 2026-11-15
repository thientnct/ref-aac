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

 $Id: ct_envcalc.h,v 1.2 2002/11/15 10:05:20 ct1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  Envelope calculation prototypes $Revision: 1.2 $
*/
#ifndef __CT_ENVCALC_H
#define __CT_ENVCALC_H

#include "ct_envextr.h"


 
void calculateSbrEnvelope(SBR_FRAME_DATA *frameData,
                          float aBufR[][64],
                          float aBufI[][64],
                          int freqBandTable1[2][MAX_FREQ_COEFFS + 1],
                          const int *nSfb,
                          int freqBandTable2[MAX_NOISE_COEFFS + 1],
                          int nNBands,
                          int reset,
#ifdef LOW_POWER_SBR
                          float *degreeAlias,
#endif
                          int ch);
#endif
