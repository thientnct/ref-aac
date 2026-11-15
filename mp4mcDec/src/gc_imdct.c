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

/* This file includes the functions for IMDCT which is used for the 
gain control tool. */

#include "block.h"
#include "dolby_win_ssr.h"
#include "gc.h"
#include "intrins.h"

#define	GC_LFFT	256.0
#define	GC_SFFT	32.0

#ifndef PI
#define		PI		(3.14159265359)
#endif
#define		npow2(x)	(1L << (x))			/* 2^x */

#define		DFT		1
#define		IDFT		(-1)

#define		TRUE		1
#define		FALSE		0

#define		LFFT		1024
#define		LFFT_SHORT	(LFFT/SHORT_WIN_IN_LONG)
#define		FLAT		((LFFT/NBANDS-LFFT/NBANDS/SHORT_WIN_IN_LONG)/2)


/*****************************************************************************
This function carries out the inverse fast modified DCT.
		Input:		2^(ln-1) spectra
		Output:		2^(ln)   samples
*****************************************************************************/
static void gc_ifmdct(
	double	*p_y,			/* input array */
	double	*p_x,			/* output array */
	int	ln)			/* (2^(ln-1)) spectra (2**ln) points */
{
	int	i, k, nh;
	double	*p_u0, *p_u1;
	double	a0, a1, a2, a3, c1, s1, c5, s5, w;
	COMPLEX	*p_z0, *p_z1;
	COMPLEX	z0, z1;

	nh  = npow2(ln-1);
	p_u0 = (double *)  calloc(nh,   sizeof(double));
	p_u1 = (double *)  calloc(nh,   sizeof(double));
	p_z0 = (COMPLEX *) calloc(nh/2, sizeof(COMPLEX));
	p_z1 = (COMPLEX *) calloc(nh/2, sizeof(COMPLEX));

	/* set U(k) {p_u1[nh]} */
	for (k = 0; k < nh/2; ++k) {
		p_u1[k] =  p_y[2*k];
	}
	for (k = nh/2; k < nh; ++k) {
		p_u1[k] = -p_y[2*nh-1-2*k];
	}

	/* set Z(l) {p_z1} */
	for (i = 0; i < nh/2; i++) {
		z0 = gc_cexp(1.0, ((-2.0*PI)*i/nh));
		z1 = gc_cgen(p_u1[2*i], p_u1[2*i+1]);
		p_z1[i] = gc_cmul(z0, z1);
	}

	/* set z(n) {p_z0} */
	gc_fft(p_z1, p_z0, ln-2, DFT);

	/* set u(k) {p_u0} */
	for (i = 0; i < nh/2; i++) {
		w = 2.0*PI*(2.0*i+1)/(8*nh);
		c1 = cos(w);
		s1 = sin(w);
		c5 = cos(5.0*w);
		s5 = sin(5.0*w);
		a0 = ( c1 - s5) / 2.0;
		a1 = ( c1 + s5) / 2.0;
		a2 = ( s1 + c5) / 2.0;
		a3 = (-s1 + c5) / 2.0;

		p_u0[i]		= a0 * p_z0[i].re + a1 * p_z0[nh/2-1-i].re
				+ a2 * p_z0[i].im + a3 * p_z0[nh/2-1-i].im;
		p_u0[nh-1-i]	= a2 * p_z0[i].re - a3 * p_z0[nh/2-1-i].re
				- a0 * p_z0[i].im + a1 * p_z0[nh/2-1-i].im;
	}

	/* set y(n) {p_x} */
	for (i = 0; i < nh/2; ++i) {
		p_x[i] =  p_u0[i+nh/2];
	}
	for (i = nh/2; i < nh*3/2; ++i) {
		p_x[i] = -p_u0[nh*3/2-1-i];
	}
	for (i = nh*3/2; i < nh*2; ++i) {
		p_x[i] = -p_u0[i-nh*3/2];
	}

	free1(p_u0);
	free1(p_u1);
	free1(p_z0);
	free1(p_z1);

} /* gc_ifmdct() */

/*****************************************************************************
This function carries out the IMDCT and applies the IMDCT window.
		Input:	(2^(ln-1)) spectra
		Output:	(2^ln)     samples
*****************************************************************************/
static void gc_imdct(
	double	*p_y,			/* input array */
	double	*p_x,			/* output array */
	double	*p_w,			/* window function */
	int	ln)			/* (2^ln) points */
{
	int	i, nf;
	double	*p_buf0, *p_buf1;

	nf = npow2(ln);
	p_buf0 = (double *) calloc(nf,   sizeof(double));
	p_buf1 = (double *) calloc(nf/2, sizeof(double));

	for (i = 0; i < nf/2; ++i) {
		p_buf1[i] = p_y[i];
	}

	/* IMDCT */
	gc_ifmdct(p_buf1, p_buf0, ln);

	/* Apply MDCT window */
	for (i = 0; i < nf; i++) {
		p_x[i] = p_buf0[i] * p_w[i];
	}

	free1(p_buf0);
	free1(p_buf1);

} /* gc_imdct() */

/*************************************************************************
This function calculates an IMDCT window and calls the first IMDCT function.
The shape of an IMDCT window is selected from the two window types which
are the sine window and the Kaiser-Bessel derived window.
**************************************************************************/
static void gc_imdct_sub(
	double	in_data[],
	int	window_sequence,
        int wnd_shape,
	int previous_wnd_shape,
	int	band,
	double	out_data[]
	)
{
	double 	imdct_in[LFFT/NBANDS];
	double	imdct_out[LFFT/NBANDS*2];
	int i, j, k;
	int	lnl_imdct;

	double	window_imdct[LFFT/NBANDS*2];
	double	window_imdct_1st[LFFT_SHORT/NBANDS*2];
	double	window_imdct_2nd_7th[LFFT_SHORT/NBANDS*2];
	double	long_wind[LFFT/NBANDS];
	double	short_wind[LFFT_SHORT/NBANDS];
	double	phase;

	/* MDCT window initialize */

	/* for a long sine window */
	phase = PI /(2.0*(double)(LFFT/NBANDS));
	for (i = 0; i < LFFT/NBANDS; i++) {
		long_wind[i]  = sin( phase * ((double)i+ 0.5) );
	}
	/* for a short sine window */
	phase = PI /(2.0*(double)(LFFT_SHORT/NBANDS));
	for (i = 0; i < LFFT_SHORT/NBANDS; i++) {
		short_wind[i] = sin( phase * ((double)i+ 0.5) );
	}

	/* Set a IMDCT window function for this frame */
	switch(window_sequence){
	case ONLY_LONG_SEQUENCE:
		for(i = 0; i < LFFT/NBANDS; i++){
			if( previous_wnd_shape == WS_FHG ){
				window_imdct[i] = long_wind[i]; 
			}
			else if(previous_wnd_shape == WS_DOLBY){
				window_imdct[i] = dol_long_ssr[i];
			}
			if( wnd_shape == WS_FHG ){
				window_imdct[LFFT/NBANDS*2-1-i] = long_wind[i]; 
			}
			else if(wnd_shape == WS_DOLBY){
				window_imdct[LFFT/NBANDS*2-1-i] = dol_long_ssr[i];
			}
			else{
				printf("Window Shape Data Error !  %d \n",wnd_shape);
				exit(1);
			}
		}
		break;
	case EIGHT_SHORT_SEQUENCE:
		for(i = 0; i < LFFT_SHORT/NBANDS; i++){
			if( previous_wnd_shape == WS_FHG ){
				window_imdct_1st[i] = short_wind[i]; 
			}
			else if(previous_wnd_shape == WS_DOLBY){
				window_imdct_1st[i] = dol_short_ssr[i];
			}
			if( wnd_shape == WS_FHG ){
				window_imdct_1st[LFFT_SHORT/NBANDS*2-1-i] = short_wind[i]; 
				window_imdct_2nd_7th[i] = short_wind[i]; 
				window_imdct_2nd_7th[LFFT_SHORT/NBANDS*2-1-i] = short_wind[i]; 
			}
			else if(wnd_shape == WS_DOLBY){
				window_imdct_1st[LFFT_SHORT/NBANDS*2-1-i] = dol_short_ssr[i];
				window_imdct_2nd_7th[i] = dol_short_ssr[i];
				window_imdct_2nd_7th[LFFT_SHORT/NBANDS*2-1-i] = dol_short_ssr[i];
			}
		}
		break;
	case LONG_START_SEQUENCE:
		if(previous_wnd_shape == WS_FHG){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i] = long_wind[i];
			}
		}
		else if(previous_wnd_shape == WS_DOLBY){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i] = dol_long_ssr[i];
			}
		}
		for(i = 0; i < FLAT; i++){
			window_imdct[i+LFFT/NBANDS] = 1.0;
		}

		if(wnd_shape == WS_FHG){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT/NBANDS] =
				short_wind[LFFT_SHORT/NBANDS-1-i];
			}
		}
		else if(wnd_shape == WS_DOLBY){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT/NBANDS] =
				dol_short_ssr[LFFT_SHORT/NBANDS-1-i];
			}
		}


		for(i = 0; i < FLAT; i++){
			window_imdct[i+FLAT+LFFT/NBANDS+LFFT_SHORT/NBANDS] = 0.0;
		}
		break;
	case LONG_STOP_SEQUENCE:
		for(i = 0; i < FLAT; i++){
			window_imdct[i] = 0.0;
		}

		if( previous_wnd_shape == WS_FHG ){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT] = short_wind[i]; 
			}
		}
		else if(previous_wnd_shape == WS_DOLBY){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT] = dol_short_ssr[i];
			}

		}
		else{
			printf("previous_wnd_shape type error !! :%d \n",previous_wnd_shape);
		}

		for(i = 0; i < FLAT; i++){
			window_imdct[i+FLAT+LFFT_SHORT/NBANDS] = 1.0;
		}
		if(wnd_shape == WS_FHG){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT_SHORT/NBANDS+FLAT] =
				long_wind[LFFT/NBANDS-1-i]; 
			}
		}
		else if(wnd_shape == WS_DOLBY){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT_SHORT/NBANDS+FLAT] =
				dol_long_ssr[LFFT/NBANDS-1-i];
			}
		}
		break;
	  default:
		fprintf(stderr, "imdct_main: ERROR! Illegal Window Type \n");
		exit(1);
		break;
	}

	/* IMDCT */
	if(window_sequence == EIGHT_SHORT_SEQUENCE){
		for(j = 0; j < SHORT_WIN_IN_LONG; j++){
			for(k = 0; k < LFFT_SHORT/NBANDS; k++){
				imdct_in[k] = in_data[band*LFFT/NBANDS+j*LFFT_SHORT/NBANDS+k];
			}
			if(j == 0){
				for(k = 0; k < LFFT_SHORT/NBANDS*2; k++){
					window_imdct[k] = window_imdct_1st[k];
				}
			}
			else{
				for(k = 0; k < LFFT_SHORT/NBANDS*2; k++){
					window_imdct[k] = window_imdct_2nd_7th[k];
				}
			}
			lnl_imdct = gc_nlog(2, LFFT_SHORT/NBANDS) + 1;
			gc_imdct(imdct_in, imdct_out, window_imdct, lnl_imdct);
			for(k = 0; k < LFFT_SHORT/NBANDS*2; k++){
				out_data[band*LFFT/NBANDS*2+j*LFFT_SHORT/NBANDS*2+k] 
				= imdct_out[k]/GC_SFFT;
			}
		}
	}
	else{
		for(j = 0; j < LFFT/NBANDS; j++){
			imdct_in[j] = in_data[band*LFFT/NBANDS+j];
		}
		lnl_imdct = gc_nlog(2, LFFT/NBANDS) + 1;
		gc_imdct(imdct_in, imdct_out, window_imdct, lnl_imdct);
		for(j = 0; j < LFFT/NBANDS*2; j++){
			out_data[band*LFFT/NBANDS*2+j] = imdct_out[j]/GC_LFFT;
		}
	}
} /* gc_imdct_sub() */

/*****************************************************************************
This function is a main routine for IMDCT. Before IMDCT, the input MDCT 
coefficents are resorted into the 4-band MDCT coefficents for IPQF. The
spectral order is changed to reverse order in the even IPQF band.

		I:	(LFFT) IMDCT coefficients
		I:	id of window type
		I: id of channel number

		O:	(LFFT*2 samples) reconstructed time signal
*****************************************************************************/
void gc_imdct_main(
	Float	*coef,
	Wnd_Shape *wnd_shape,
	int	window_sequence,
	double	*timeSig
	)
{
	int	i;
	double	*imdct_in;
	int band, short_block;
	int window_shape,previous_window_shape;

	imdct_in = (double *)calloc(NBANDS*LFFT/NBANDS,sizeof(double));

	/* Spectral are resorted and changed the reverse order in the even 
    IPQF band. */

	if(window_sequence == EIGHT_SHORT_SEQUENCE){
		for(band = 0; band < ssr_decoder_band; band++){
			for(short_block = 0; short_block < SHORT_WIN_IN_LONG;
					short_block++){
				for(i = 0 ; i < LFFT_SHORT/NBANDS; i++){
					if(band % 2 == 0) /* even band */
					imdct_in[LFFT/NBANDS*band+LFFT_SHORT/NBANDS*short_block+i] = 
					(double)coef[LFFT_SHORT*short_block 
									 + LFFT_SHORT/NBANDS*band + i];
					else
					imdct_in[LFFT/NBANDS*band+LFFT_SHORT/NBANDS*short_block+i] = 
					(double)coef[LFFT_SHORT*short_block + 
							   LFFT_SHORT/NBANDS*band + LFFT_SHORT/NBANDS-1-i];

				}
			}
		}
	}
	else{
		for(band = 0; band < ssr_decoder_band; band++){
			for(i = 0; i < LFFT/NBANDS; i++){
				if(band % 2 == 0) /* even band */
				imdct_in[LFFT/NBANDS*band+i]
				= coef[LFFT/NBANDS*band+i];
				else
				imdct_in[LFFT/NBANDS*band+i]
				= coef[LFFT/NBANDS*band+LFFT/NBANDS-1-i]; 
			}
		}
	}

	window_shape = wnd_shape->this_bk;
	previous_window_shape = wnd_shape->prev_bk;
	
	/* IMDCT is carried out for each IPQF band. */
	for (band = 0; band < ssr_decoder_band; band++) {
		gc_imdct_sub(imdct_in, window_sequence, 
					 window_shape,previous_window_shape,
					 band, timeSig);
	}

		free1(imdct_in);

} /* gc_imdct_main() */

/*******************************************************************
                      End of gc_imdct.c
*******************************************************************/

