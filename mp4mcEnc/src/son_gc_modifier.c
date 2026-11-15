/*******************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation) and Mitsuyuki Hatanaka (Sony Corporation)

and edited by
  Takashi Koike (Sony Corporation)

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

Copyright (C) 1996.
*******************************************************************/

#include	<stdlib.h>
#include	"sony_local.h"

static void son_gc_modifier_sub(
	double	input[],
	GAINC	*g_infoCh[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	int	band,
	double	output[]
	)
{
	/* This parameter will be removed outside */
	static	GAINC	g_info_prev[MAX_TIME_CHANNELS][NBANDS];

	int	i, j;
	double	*work_buf;
	double	*a_gcwind;

	work_buf = input;
	a_gcwind = (double *)calloc(block_size_samples/NBANDS*2,
					sizeof(double));

	switch(window_sequence){
	case ONLY_LONG_SEQUENCE:
		son_gainc_window(block_size_samples/NBANDS*2,
				g_info_prev[ch][band],
				g_infoCh[band][0],
				g_infoCh[band][0],
				a_gcwind, window_sequence);
		for (j = 0; j < block_size_samples/NBANDS*2; j++) {
			output[j] = work_buf[j]/a_gcwind[j];
		}
		g_info_prev[ch][band] = g_infoCh[band][0];
		break;
	case EIGHT_SHORT_SEQUENCE:
		for (i = 0; i < SHORT_WIN_IN_LONG; i++) {
			son_gainc_window(
				block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2,
					g_info_prev[ch][band],
					g_infoCh[band][i],
					g_infoCh[band][i],
					a_gcwind, window_sequence);
			for (j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2; j++) {
				output[j+block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2*i] = work_buf[j+block_size_samples/NBANDS*7/16+i*block_size_samples/SHORT_WIN_IN_LONG/NBANDS]/a_gcwind[j];
			}
			g_info_prev[ch][band] = g_infoCh[band][i];
		}
		break;
	case LONG_START_SEQUENCE:
		son_gainc_window(block_size_samples/NBANDS*2,
				g_info_prev[ch][band],
				g_infoCh[band][0],
				g_infoCh[band][1],
				a_gcwind, window_sequence);
		for (j = 0; j < block_size_samples/NBANDS*2; j++) {
			output[j] = work_buf[j]/a_gcwind[j];
		}
		g_info_prev[ch][band] = g_infoCh[band][1];
		break;
	case LONG_STOP_SEQUENCE:
		son_gainc_window(block_size_samples/NBANDS*2,
				g_info_prev[ch][band],
				g_infoCh[band][0],
				g_infoCh[band][1],
				a_gcwind, window_sequence);
		for (j = 0; j < block_size_samples/NBANDS*2; j++) {
			output[j] = work_buf[j]/a_gcwind[j];
		}
		g_info_prev[ch][band] = g_infoCh[band][1];
		break;
	default:
		fprintf(stderr, "gc_modifier: ERROR! Illegal Window Type \n");
		exit(0);
		break;
	}
	free(a_gcwind);
}

void	son_gc_modifier(
	double	*bandSigChForGCAnalysis[],
	GAINC	*g_infoCh[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	double	*bandSigChWithGC[]
	)
{
	int	i;

	for (i = 0; i < NBANDS; i++) {
		son_gc_modifier_sub(bandSigChForGCAnalysis[i], g_infoCh,
				block_size_samples,
				window_sequence, ch, i, bandSigChWithGC[i]);
	}
}
