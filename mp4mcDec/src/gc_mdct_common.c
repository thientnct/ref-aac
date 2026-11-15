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

/* This file contains the encoder and decoder common functions for the 
MDCT and IMDCT for the gain control tool. */

#include "gc.h"
#include "intrins.h"

#ifndef PI
#define		PI		(3.14159265358979323846)
#endif
#define		npow2(x)	(1L << (x))			/* 2^x */

#define		DFT		1
#define		IDFT		(-1)

#define		TRUE		1
#define		FALSE		0

/*****************************************************************************
This function calculates the value log[m] of n.
*****************************************************************************/
int
gc_nlog(int m, int n)
{
	int	ln, nn;

	for (nn = 1, ln = 0; nn < n; ++ln, nn *= m);
	return(ln);
}

/*****************************************************************************
This function produces a complex value from the real part value and 
imaginary part value.
*****************************************************************************/
COMPLEX gc_cgen (double x, double y)
{
	COMPLEX	c;

	c.re = x;
	c.im = y;

	return(c);
}

/*****************************************************************************
This function produces a complex value from the amplitude value and
phase value.
*****************************************************************************/
COMPLEX gc_cexp (double r, double a)
{
	COMPLEX	c;

	c.re = r * ((double) cos((double) a));
	c.im = r * ((double) sin((double) a));

	return(c);
}

/*****************************************************************************
The multiplication for the complex values.
*****************************************************************************/
COMPLEX gc_cmul (COMPLEX c0, COMPLEX c1)
{
	COMPLEX	c2;

	c2.re = c0.re * c1.re - c0.im * c1.im;
	c2.im = c0.re * c1.im + c0.im * c1.re;
	return(c2);
}

/*****************************************************************************
The function for the bit-order reverse of FFT
*****************************************************************************/
static int gc_bit_reverse (
	int	n,
	int	bitpat0)
{
	int	bp0, bp1;
	int	bitpat1;

	bp0 = 1 << (n-1);
	bp1 = 1;
	bitpat1 = 0;
	do {
		if ((bitpat0 & bp0) != 0) {
			bitpat1 |= bp1;
		}
		bp0 = bp0 >> 1;
		bp1 = bp1 << 1;
	} while (bp0 != 0);
	return (bitpat1);
}

/*****************************************************************************
The function for the FFT processing of n (=2**ln) samples
*****************************************************************************/
static void gc_fft_proc(
	double	*p_x, double *p_y,		/* array of re. & im. parts */
	int	ln)						/* n = 2**ln samples */
{
	int	np;
	int	i, j, k, l;
	int	nblocks, nsize, nhsize;
	int	nsjk0, nsjk1;
	int	*p_mark;
	double	*p_wx, *p_wy;
	double	x,y;
	double	ax, ay;

	np = npow2(ln);

	/* Set p_wx[]+im*p_wy[] = exp (-im*2*PI*i/np).  */
	p_wx = (double *) calloc(np, sizeof(double));
	p_wy = (double *) calloc(np, sizeof(double));
	for (i = 0; i < np/2; ++i) {
		p_wx[i] = (double) cos(2*PI*i/np);
		p_wy[i] = (double) -sin(2*PI*i/np);
	}

	/* Reallocation of Array */
	p_mark = (int *) calloc(np, sizeof(int));
	for (i = 0; i < np; ++i) {
		p_mark[i] = FALSE;
	}
	for (i = 0; i < np; ++i) {
		if (p_mark[i] == FALSE) {
			j = gc_bit_reverse (ln, i);
			x = p_x[j];
			y = p_y[j];
			p_x[j] = p_x[i];
			p_y[j] = p_y[i];
			p_x[i] = x;
			p_y[i] = y;
			p_mark[j] = TRUE;
		}
	}

	/* FFT Calculation */
	nblocks = np/2;
	nsize = 2;
	for (i = 0; i < ln; ++i) {
		nhsize = nsize/2;
		nsjk0 = 0;
		nsjk1 = nhsize;
		for (j = 0; j < nblocks; ++j) {
			l = 0;
			for (k = 0; k < nhsize; ++k) {
				ax = p_x[nsjk1]*p_wx[l] - p_y[nsjk1]*p_wy[l];
				ay = p_x[nsjk1]*p_wy[l] + p_y[nsjk1]*p_wx[l];
				p_x[nsjk1] = p_x[nsjk0] - ax;
				p_y[nsjk1] = p_y[nsjk0] - ay;
				p_x[nsjk0] += ax;
				p_y[nsjk0] += ay;
				l += nblocks;
				++nsjk0;
				++nsjk1;
			}
			nsjk0 += nhsize;
			nsjk1 += nhsize;
		}
		nblocks = nblocks/2;
		nsize = nsize*2;
	}

	free1((char *) p_wx);
	free1((char *) p_wy);
	free1((char *) p_mark);

} /* gc_fft_proc() */

/*****************************************************************************
The function for the FFT of n (=2**l) samples
*****************************************************************************/
void gc_fft(
	COMPLEX	*p_c0,			/* input array */
	COMPLEX	*p_c1,			/* output array */
	int	ln,			/* (2**ln) points */
	int	idir)			/* DFT/IDFT */
{
	int	n;
	int	i;
	double	*p_x, *p_y;

	n = npow2(ln);
	p_x = (double *) calloc(n, sizeof(double));
	p_y = (double *) calloc(n, sizeof(double));
	for (i = 0; i < n; ++i) {
		p_x[i] = p_c0[i].re;
		p_y[i] = p_c0[i].im;
		switch (idir) {
		case DFT:
			break;
		case IDFT:
			p_y[i] = -p_y[i];
			break;
		default:
			perror("fft: should be DFT or IDFT.");
			exit(1);
		}
	}
	gc_fft_proc(p_x, p_y, ln);
	for (i = 0; i < n; ++i) {
		p_c1[i].re = p_x[i];
		p_c1[i].im = p_y[i];
		if (idir == IDFT) {
			p_c1[i].re = p_c1[i].re/n;
			p_c1[i].im = -p_c1[i].im/n;
		}
	}
	free1((char *)p_x);
	free1((char *)p_y);

} /* gc_fft() */

/**********************************************************************
                        End of gc_mdct_common.c
**********************************************************************/
