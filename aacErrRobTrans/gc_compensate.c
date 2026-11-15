/*
  "This software module was originally developed by
  
  Yoshiaki Oikawa (Sony Corporation) and
  Mitsuyuki Hatanaka (Sony Corporation) 

  and edited by
  
  Yoshiaki Oikawa (Sony Corporation) and
  Mitsuyuki Hatanaka (Sony Corporation)

  in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
  Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
  software module is an implementation of a part of one or more MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
  MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
  gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards free license to this software module or modifications
  thereof for use in hardware or software products claiming conformance
  to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards. Those intending to use this software module in hardware or
  software products are advised that this use may infringe existing
  patents. The original developer of this software module and his/her
  company, the subsequent editors and their companies, and ISO/IEC have
  no liability for use of this software module or modifications thereof
  in an implementation. Copyright is not released for non MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
  original developer retains full right to use the code for his/her own
  purpose, assign or donate the code to a third party and to inhibit
  third party from using the code for non MPEG-2 AAC/MPEG-4
  System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
  notice must be included in all copies or derivative works."
  Copyright (C) 1996.
*/

#ifdef	SSR

/* This file includes the functions for the gain compensater in the gain
control tool. */

#include	<stdlib.h>
#include	"gc.h"

#define	BLOCK_LEN_LONG	1024
#define	MAX_CHANNELS	5

GAIN_CONTROL_DATA       g_info[MAX_CHANNELS];
void gc_gainc_window(int ,GAIN_INF, GAIN_BLOCK, double *, int, int);

extern   int ssr_decoder_band;

/***************************************************************************
This function carries out the gain compensate and the overlap-add. The
following procedures are done in this function.
(a) The gain control function is calculated.
(b) The gain control function applies to IMDCT output samples as a another
    IMDCT window.
(c) The reconstructed time domain signal produces by overlap-add.
****************************************************************************/
void
gc_compensate(
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

	int	last_cur_block;

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

	free(a_gcwind);

} /* gc_compensate() */

#endif
/********************************************************************
                      End of gc_compensate.c
********************************************************************/
