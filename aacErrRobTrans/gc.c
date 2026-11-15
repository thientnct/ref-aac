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

/* This file includes a main function for the gain control tool. */

#include	"all.h"
#include	<stdio.h>
#include	<stdlib.h>	/* for malloc */
#include	<math.h>

#define	BLOCK_LEN_LONG	1024

extern   int ssr_decoder_band;

void	gc_imdct_main(float	*freqSignalCh,Wnd_Shape *wnd_shape,
					  int window_sequence,
					  double *timeSignalChWithGCandOverlapping, int ch);
void	gc_compensate(double input[], int block_size_samples,
						  int window_sequence, int	ch,
						  int band, double *out[]);
void	gc_ipqf_main(double	*bandSignalCh[NBANDS], int	block_size_samples,
						 int ch, double	timeSignalCh[]);

/**************************************************************************
This function is a main function for the gain_control tool. The input data
are the MDCT coefficients, an id of window sequence, an id of the channel
number and an id of the maximum band number. The sub routines for the IMDCT,
the gain compensater and the IPQF are called from this function. The output
data are the reconstructed time signal.
***************************************************************************/
void
gain_control(
	float	*freqSigCh,
	Wnd_Shape	*wnd_shape,
	int	window_sequenceCh,
	int	ch,
	float	*timeSigCh
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
	gc_imdct_main(freqSigCh, wnd_shape, window_sequenceCh, imdctOut, ch); 

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
		timeSigCh[i] = (float)ipqfout[i];
	}

	free(imdctOut);
	for (i = 0; i < NBANDS; i++) {
		free(bandSigCh[i]);
	}
	free(bandSigCh);

} /* gain_control() */

#endif
/*******************************************************************
                         End of gc.c
*******************************************************************/




