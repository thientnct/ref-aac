/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

/* This file includes a main function for the gain control tool. */

#include "block.h"
#include "gc.h"
#include "intrins.h"

int     ssr_decoder_band = 4;


/**************************************************************************
This function is a main function for the gain_control tool. The input data
are the MDCT coefficients, an id of window sequence, an id of the channel
number and an id of the maximum band number. The sub routines for the IMDCT,
the gain compensater and the IPQF are called from this function. The output
data are the reconstructed time signal.
***************************************************************************/
void gain_control(
	Float	*freqSigCh,
	Wnd_Shape	*wnd_shape,
	int	window_sequenceCh,
	int	ch,
	Float	*timeSigCh
	)
{
	int	i,j;
	double	*imdctOut;
	double	**bandSigCh;
	int	block_size_samples = BLOCK_LEN_LONG;
	double	ipqfout[BLOCK_LEN_LONG];

	imdctOut = (double *)calloc(ssr_decoder_band*BLOCK_LEN_LONG/NBANDS*2,
										 sizeof(double));
	bandSigCh = (double **)calloc(NBANDS,sizeof(double *));
	for (i = 0; i < NBANDS; i++) {
		bandSigCh[i] = (double *)calloc(BLOCK_LEN_LONG/NBANDS,sizeof(double));
	}

	for(i = 0; i < NBANDS; i++){
		for(j = 0; j < BLOCK_LEN_LONG/NBANDS; j++){
			bandSigCh[i][j] = 0.0;
		}
	}

	/* IMDCT */
	gc_imdct_main(freqSigCh, wnd_shape, window_sequenceCh, imdctOut); 

	/* Gain Compensater */
	for (i = 0; i < ssr_decoder_band; i++) {
		gc_compensate(imdctOut,
						  block_size_samples,
						  window_sequenceCh, ch, i,
						  bandSigCh);
	}

	/* IPQF */
	gc_ipqf_main(bandSigCh, block_size_samples, ch, ipqfout);
	
	for(i = 0; i < BLOCK_LEN_LONG; i++){
          timeSigCh[i] = /*(Float)*/ipqfout[i];
	}

	free1(imdctOut);
	for (i = 0; i < NBANDS; i++) {
		free1(bandSigCh[i]);
	}
	free1(bandSigCh);

} /* gain_control() */

/*******************************************************************
                         End of gc.c
*******************************************************************/




