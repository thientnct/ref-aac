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

 $Id: ct_polyphase.h,v 1.7 2003/01/29 13:11:55 boe Exp $

*******************************************************************************/
/*!
  \file
  \brief  Polyphase Filterbank $Revision: 1.7 $
*/

#ifndef __MPEG_BWE_FILTERBANK
#define __MPEG_BWE_FILTERBANK

/*
     The Twiddle structure is used to supply the initialization
   functions with information of the twiddles (or phase-shifts)
   distributed between the analysis and synthesis filter banks.
   
     As an example: the default filter bank, i.e. the filter bank
   initialized when the twiddle argument is NULL corresponds to a
   twiddle with fi = 0.5 and beta = 64 for a non-downsampled
   synthesis filterbank and fi = 0.25 and beta = 32 for a down-
   sampled synthesis filter bank.

     As an other example: the twiddle for a complex-valued filter 
   bank, with the real part being equal the cosine modulated bank
   (used in the LP mode) and the imaginary part being the sine
   modulated version of the same are fi = -32 for the non-downsampled
   synthesis bank and fi = -16 for the downsampled synthesis filter
   bank. In both cases beta equals 0.
*/

typedef struct _Twiddle
{
  double fi;
  double beta;
} Twiddle;

void
InitSbrAnaFilterbank(const Twiddle *twiddle,
                     int bDownSampleSBR); 

void
CalculateSbrAnaFilterbank( float * timeSig,
                           float * Sr,
                           float * Si,
                           int channel,
                           int maxBand);

void
InitSbrSynFilterbank(const Twiddle *twiddle,
                     int bDownSampleSBR); 

void
CalculateSbrSynFilterbank( float * Sr,
                           float * Si,
                           double * timeSig,
                           int bDownSampledSbr,
                           int channel);


#endif
