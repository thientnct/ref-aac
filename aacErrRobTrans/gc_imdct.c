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

/* This file includes the functions for IMDCT which is used for the 
gain control tool. */

#include	"all.h"
#include	"dolby_win_ssr.h"

#include	<math.h>
#include	<stdlib.h>

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

typedef	struct	{
	double	re;		/* Real part */
	double	im;		/* Imaginary part */
}	COMPLEX; /* Complex number */

typedef enum {
	WS_SIN, WS_DOL, N_WINDOW_SHAPES
	} Window_shape;

extern int ssr_decoder_band;

void gc_fft(COMPLEX	*p_c0, COMPLEX	*p_c1, int	ln, int	idir);
int gc_nlog(int m, int n);
COMPLEX gc_cgen(double x, double y);
COMPLEX gc_cexp(double r, double a);
COMPLEX gc_cmul(COMPLEX c0, COMPLEX c1);

/*****************************************************************************
This function carries out the inverse fast modified DCT.
		Input:		2^(ln-1) spectra
		Output:		2^(ln)   samples
*****************************************************************************/
void
gc_ifmdct(
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

	free(p_u0);
	free(p_u1);
	free(p_z0);
	free(p_z1);

} /* gc_ifmdct() */

/*****************************************************************************
This function carries out the IMDCT and applies the IMDCT window.
		Input:	(2^(ln-1)) spectra
		Output:	(2^ln)     samples
*****************************************************************************/
void
gc_imdct(
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

	free(p_buf0);
	free(p_buf1);

} /* gc_imdct() */

/*************************************************************************
This function calculates an IMDCT window and calls the first IMDCT function.
The shape of an IMDCT window is selected from the two window types which
are the sine window and the Kaiser-Bessel derived window.
**************************************************************************/
void
gc_imdct_sub(
	double	in_data[],
	int	window_sequence,
    int wnd_shape,
	int previous_wnd_shape,
	int	band,
	double	out_data[],
    int ch
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
			if( previous_wnd_shape == WS_SIN ){
				window_imdct[i] = long_wind[i]; 
			}
			else if(previous_wnd_shape == WS_DOL){
				window_imdct[i] = dol_long_ssr[i];
			}
			if( wnd_shape == WS_SIN ){
				window_imdct[LFFT/NBANDS*2-1-i] = long_wind[i]; 
			}
			else if(wnd_shape == WS_DOL){
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
			if( previous_wnd_shape == WS_SIN ){
				window_imdct_1st[i] = short_wind[i]; 
			}
			else if(previous_wnd_shape == WS_DOL){
				window_imdct_1st[i] = dol_short_ssr[i];
			}
			if( wnd_shape == WS_SIN ){
				window_imdct_1st[LFFT_SHORT/NBANDS*2-1-i] = short_wind[i]; 
				window_imdct_2nd_7th[i] = short_wind[i]; 
				window_imdct_2nd_7th[LFFT_SHORT/NBANDS*2-1-i] = short_wind[i]; 
			}
			else if(wnd_shape == WS_DOL){
				window_imdct_1st[LFFT_SHORT/NBANDS*2-1-i] = dol_short_ssr[i];
				window_imdct_2nd_7th[i] = dol_short_ssr[i];
				window_imdct_2nd_7th[LFFT_SHORT/NBANDS*2-1-i] = dol_short_ssr[i];
			}
		}
		break;
	case LONG_START_SEQUENCE:
		if(previous_wnd_shape == WS_SIN){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i] = long_wind[i];
			}
		}
		else if(previous_wnd_shape == WS_DOL){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i] = dol_long_ssr[i];
			}
		}
		for(i = 0; i < FLAT; i++){
			window_imdct[i+LFFT/NBANDS] = 1.0;
		}

		if(wnd_shape == WS_SIN){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT/NBANDS] =
				short_wind[LFFT_SHORT/NBANDS-1-i];
			}
		}
		else if(wnd_shape == WS_DOL){
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

		if( previous_wnd_shape == WS_SIN ){
			for(i = 0; i < LFFT_SHORT/NBANDS; i++){
				window_imdct[i+FLAT] = short_wind[i]; 
			}
		}
		else if(previous_wnd_shape == WS_DOL){
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
		if(wnd_shape == WS_SIN){
			for(i = 0; i < LFFT/NBANDS; i++){
				window_imdct[i+FLAT+LFFT_SHORT/NBANDS+FLAT] =
				long_wind[LFFT/NBANDS-1-i]; 
			}
		}
		else if(wnd_shape == WS_DOL){
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
void
gc_imdct_main(
	float	*coef,
	Wnd_Shape *wnd_shape,
	int	window_sequence,
	double	*timeSig,
    int	ch
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
					 band, timeSig, ch);
	}

		free(imdct_in);

} /* gc_imdct_main() */

#endif
/*******************************************************************
                      End of gc_imdct.c
*******************************************************************/

