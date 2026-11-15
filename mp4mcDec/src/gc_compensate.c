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

/* This file includes the functions for the gain compensater in the gain
control tool. */

#include "chandefs.h"
#include "gc.h"
#include "intrins.h"

#define	MAX_CHANNELS	Chans 

GAIN_CONTROL_DATA       g_info[MAX_CHANNELS];

/***************************************************************************
This function carries out the gain compensate and the overlap-add. The
following procedures are done in this function.
(a) The gain control function is calculated.
(b) The gain control function applies to IMDCT output samples as a another
    IMDCT window.
(c) The reconstructed time domain signal produces by overlap-add.
****************************************************************************/
void gc_compensate(
	double	input[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	int	band,
	double	*out[])
{
	int	i,j,k,a,b;
	static 	GAIN_CONTROL_DATA	g_info_prev[MAX_CHANNELS];
	static	int init_flag = 0;
	static	double	gcOverlapBuffer[MAX_CHANNELS][NBANDS][BLOCK_LEN_LONG/NBANDS*2];
	double	*a_gcwind;

	int	last_cur_block = 0;

	a_gcwind = (double *)calloc(block_size_samples/NBANDS*2,sizeof(double));

	if(init_flag == 0){
		for(i = 0; i < MAX_CHANNELS; i++){
			for(j = 0; j < NBANDS ; j++){
				for(k = 0; k < SHORT_WIN_IN_LONG; k++){
					g_info_prev[i].gInfo[j].gBlock[k].num_gain_data = 0;
                }
			}
		}
		init_flag = 1;
	}

	if(window_sequence == EIGHT_SHORT_SEQUENCE){
		for(k = 0; k < SHORT_WIN_IN_LONG; k++){
			/* Calculate Gain Control Function */
			gc_gainc_window(block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2,
                            g_info_prev[ch].gInfo[band].gBlock[0],
                            g_info[ch].gInfo[band],	
							a_gcwind, window_sequence, k); 
			/* Apply Gain Control Function */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2; j++){
				a = band*block_size_samples/NBANDS*2+
				k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+j;
				input[a] *= a_gcwind[j];
			}
			/* Overlapping */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS; j++){
				a = j+block_size_samples/NBANDS*7/16+
				block_size_samples/SHORT_WIN_IN_LONG/NBANDS*k;
				b = band*block_size_samples/NBANDS*2+
                    k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+j;
				gcOverlapBuffer[ch][band][a] 
										  += input[b];
			}
			/* The input data are set as the previous data for next frame */
			for( j = 0; j < block_size_samples/SHORT_WIN_IN_LONG/NBANDS; j++){
				a = j+block_size_samples/NBANDS*7/16+
				block_size_samples/SHORT_WIN_IN_LONG/NBANDS*(k+1);
				b = band*block_size_samples/NBANDS*2+
				k*block_size_samples/SHORT_WIN_IN_LONG/NBANDS*2+
				block_size_samples/SHORT_WIN_IN_LONG/NBANDS+j;

				gcOverlapBuffer[ch][band][a] = input[b];
			}
            g_info_prev[ch].gInfo[band].gBlock[0] = g_info[ch].gInfo[band].gBlock[k];
		}
		for(j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBuffer[ch][band][j];
		}
		for(j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBuffer[ch][band][j] =
					gcOverlapBuffer[ch][band][j+block_size_samples/NBANDS];
		}
	}
	else {
		if(window_sequence == ONLY_LONG_SEQUENCE) {
			last_cur_block = 0;
		}
		else if((window_sequence == LONG_START_SEQUENCE)||
				(window_sequence == LONG_STOP_SEQUENCE)) {
			last_cur_block = 1;
		}
		else {
			printf("Illegal window sequence type ! : %d \n",window_sequence);
			exit(1);
		}
		/* Calculate Gain Control Function */
		gc_gainc_window(block_size_samples/NBANDS*2,
                        g_info_prev[ch].gInfo[band].gBlock[0],
                        g_info[ch].gInfo[band],
						a_gcwind, window_sequence,0); 
		/* Apply Gain Control Function */
		for( j = 0; j < block_size_samples/NBANDS*2; j++){
			input[band*block_size_samples/NBANDS*2+j] *= a_gcwind[j];
		}
		/* Overlapping */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			out[band][j] = gcOverlapBuffer[ch][band][j]
			+input[band*block_size_samples/NBANDS*2+j];
		}
		/* The input data are set as the previous data for next frame */
		for( j = 0; j < block_size_samples/NBANDS; j++){
			gcOverlapBuffer[ch][band][j] =
					input[band*block_size_samples/NBANDS*2
							+block_size_samples/NBANDS+j];
		}
        g_info_prev[ch].gInfo[band].gBlock[0] = g_info[ch].gInfo[band].gBlock[last_cur_block];
	}

	free1(a_gcwind);

} /* gc_compensate() */

/********************************************************************
                      End of gc_compensate.c
********************************************************************/
