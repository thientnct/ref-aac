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

 $Id: ct_polyphase.h,v 1.1.2.4 2007/06/20 15:37:49 snl Exp $

*******************************************************************************/
/*!
  \file
  \brief  Polyphase Filterbank $Revision: 1.1.2.4 $
*/

#ifndef __MPEG_BWE_FILTERBANK
#define __MPEG_BWE_FILTERBANK

void
#ifdef AAC_ELD
InitSbrAnaFilterbank(int ldsbr);
#else
InitSbrAnaFilterbank(void);
#endif


void
CalculateSbrAnaFilterbank( float * timeSig,
                           float * Sr,
                           float * Si,
                           int channel,
                           int maxBand
#ifdef AAC_ELD
			   ,int ldsbr
#endif			   
			   );

void
InitSbrSynFilterbank(int bDownSampleSBR
#ifdef AAC_ELD
		    ,int ldsbr
#endif			
			); 

void
CalculateSbrSynFilterbank( float * Sr,
                           float * Si,
                           float * timeSig,
                           int bDownSampledSbr,
                           int channel
#ifdef AAC_ELD
			   ,int ldsbr
#endif			   
			   );

#endif
