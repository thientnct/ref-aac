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

/* This file includes the functions for IPQF in the gain control tool. */

#include "chandefs.h"
#include "gc.h"
#include "intrins.h"

#ifndef PI
#define	PI	(3.14159265358979)
#endif


extern  GAIN_CONTROL_DATA   g_info[Chans];

static	double	**app_pqfbuf0[Chans];
static	double	**app_pqfbuf1[Chans];
static	double	**pp_q0, **pp_q1, **pp_t0, **pp_t1;

void destroyGC()
{
  int i,j;
  for (i = 0; i < Chans; i++) {
    for (j = 0; j < NBANDS/2; j++) {
      if (app_pqfbuf0[i]) if (app_pqfbuf0[i][j]) free1(app_pqfbuf0[i][j]);
      if (app_pqfbuf1[i]) if (app_pqfbuf1[i][j]) free1(app_pqfbuf1[i][j]);
    }
    if (app_pqfbuf0[i]) free1(app_pqfbuf0[i]);
    if (app_pqfbuf1[i]) free1(app_pqfbuf1[i]);
  }
  for (i = 0; i < NBANDS; i++) {
    if (pp_q0) if (pp_q0[i]) free1(pp_q0[i]);
    if (pp_q1) if (pp_q1[i]) free1(pp_q1[i]);
    if (pp_t0) if (pp_t0[i]) free1(pp_t0[i]);
    if (pp_t1) if (pp_t1[i]) free1(pp_t1[i]);
  }
  if (pp_q0) free1(pp_q0);
  if (pp_q1) free1(pp_q1);
  if (pp_t0) free1(pp_t0);
  if (pp_t1) free1(pp_t1);
}

/*****************************************************************************
This function sets the coefficients for Efficient (2*kk*mm)-Tap PQF
Synthesis.

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Prototype FIR Length
	p_proto[2*kk*mm]:	Prototype Filter Coef.
	(*ppp_q0)[mm][mm]:	1st Coefficients
	(*ppp_q1)[mm][mm]:	1st Coefficients
	(*ppp_t0)[mm][kk]:	2nd Coefficients
	(*ppp_t1)[mm][kk]:	2nd Coefficients
*****************************************************************************/
static void gc_setcoef_eff_pqfsyn(
	int	mm,
	int	kk,
	double	*p_proto,
	double	***ppp_q0,
	double	***ppp_q1,
	double	***ppp_t0,
	double	***ppp_t1
	)
{
	int	i, k, n;
	double	w;

	/* Set 1st Mul&Acc Coef's */
	*ppp_q0 = (double **) calloc(mm, sizeof(double *));
	*ppp_q1 = (double **) calloc(mm, sizeof(double *));
	for (n = 0; n < mm; ++n) {
		(*ppp_q0)[n] = (double *) calloc(mm, sizeof(double));
		(*ppp_q1)[n] = (double *) calloc(mm, sizeof(double));
	}
	for (n = 0; n < mm; ++n) {
		for (i = 0; i < mm; ++i) {
			w = (2*i+1)*(2*n+1-mm)*PI/(4*mm);
			(*ppp_q0)[n][i] = 2.0 * cos((double) w);

			w = (2*i+1)*(2*(mm+n)+1-mm)*PI/(4*mm);
			(*ppp_q1)[n][i] = 2.0 * cos((double) w);
		}
	}

	/* Set 2nd Mul&Acc Coef's */
	*ppp_t0 = (double **) calloc(mm, sizeof(double *));
	*ppp_t1 = (double **) calloc(mm, sizeof(double *));
	for (n = 0; n < mm; ++n) {
		(*ppp_t0)[n] = (double *) calloc(kk, sizeof(double));
		(*ppp_t1)[n] = (double *) calloc(kk, sizeof(double));
	}
	for (n = 0; n < mm; ++n) {
		for (k = 0; k < kk; ++k) {
			(*ppp_t0)[n][k] = mm * p_proto[2*k    *mm + n];
			(*ppp_t1)[n][k] = mm * p_proto[(2*k+1)*mm + n];

			if (k%2 != 0) {
				(*ppp_t0)[n][k] = -(*ppp_t0)[n][k];
				(*ppp_t1)[n][k] = -(*ppp_t1)[n][k];
			}
		}
	}
} /* gc_setcoef_eff_pqfsyn() */

/*****************************************************************************
The (2*kk*mm)-tap efficient PQF synthesis function.

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Tap Number
	pp_q0[mm][mm]:		1st Coefficients
	pp_q1[mm][mm]:		1st Coefficients
	pp_t0[mm][kk]:		2nd Coefficients
	pp_t1[mm][kk]:		2nd Coefficients
	p_vin [mm]:		Newly Input Data
	p_uout[mm]:		Output Data
	pp_buf0[mm][kk]:	Data Buffer
	pp_buf1[mm][kk]:	Data Buffer
*****************************************************************************/
static void gc_eff_pqfsyn(
	int	mm,
	int	kk,
	double	**pp_q0,
	double	**pp_q1,
	double	**pp_t0,
	double	**pp_t1,
	double	*p_vin,
	double	*p_uout,
	double	**pp_buf0,
	double	**pp_buf1
	)
{
	int	i, n, k;
	double	acc;

	/* Shift Buffer Data and Append New Data */
	for (n = 0; n < mm/2; ++n) {
		for (k = 0; k < 2*kk-1; ++k) {
			pp_buf0[n][k] = pp_buf0[n][k+1];
			pp_buf1[n][k] = pp_buf1[n][k+1];
		}
	}

	/* 1st Mul&Acc Operations and Append the Results to the Buufers */
	for (n = 0; n < mm/2; ++n) {
		acc = 0.0;
		for (i = 0; i < mm; ++i) {
			acc += pp_q0[n][i]*p_vin[i];
		}
		pp_buf0[n][2*kk-1] = acc;

		acc = 0.0;
		for (i = 0; i < mm; ++i) {
			acc += pp_q1[n][i]*p_vin[i];
		}
		pp_buf1[n][2*kk-1] = acc;
	}
	
	/* 2nd Mul & Acc Operation */
	for (n = 0; n < mm/2; ++n) {
		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			acc += pp_t0[n][k]*pp_buf0[n][2*kk-1-2*k];
		}
		for (k = 0; k < kk; ++k) {
			acc += pp_t1[n][k]*pp_buf1[n][2*kk-2-2*k];
		}
		p_uout[n] = acc;

		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			acc += pp_t0[mm-1-n][k]*pp_buf0[n][2*kk-1-2*k];
		}
		for (k = 0; k < kk; ++k) {
			acc -= pp_t1[mm-1-n][k]*pp_buf1[n][2*kk-2-2*k];
		}
		p_uout[mm-1-n] = acc;
	}
} /* gc_eff_pqfsyn() */

/*****************************************************************************
This function is a main routine for IPQF. The time signal is reconstruced 
from the 4-band time signals.
*****************************************************************************/
void gc_ipqf_main(
	double	*ipqfInBufCh[NBANDS],
	int	block_size_samples,
	int	ch,
	double	output[] 
	)
{

	int max_band;

	static	int	initFlag = 0;
	static	double	a_pqfproto[NPQFTAPS];

	int	i, j;
	double	a_ipqfin[NBANDS];

	max_band = g_info[ch].max_band;

	if (initFlag == 0) {
		for (i = 0; i < Chans; i++) {
			app_pqfbuf0[i] = (double **)calloc(NBANDS/2,
						sizeof(double *));
			app_pqfbuf1[i] = (double **)calloc(NBANDS/2,
						sizeof(double *));
			for (j = 0; j < NBANDS/2; j++) {
				app_pqfbuf0[i][j] = (double *)calloc(
							NPQFTAPS/NBANDS,
							sizeof(double));
				app_pqfbuf1[i][j] = (double *)calloc(
							NPQFTAPS/NBANDS,
							sizeof(double));
			}
		}
		gc_set_protopqf(a_pqfproto);
		gc_setcoef_eff_pqfsyn(NBANDS, NPQFTAPS/(2*NBANDS), a_pqfproto,
				&pp_q0, &pp_q1, &pp_t0, &pp_t1);
		initFlag = 1;
	}

	for (i = 0; i < block_size_samples; i++) {
		output[i] = 0.0;
	}

	for (i = 0; i < block_size_samples / NBANDS; i++) {
		for(j = 0; j < NBANDS; j++){
			a_ipqfin[j] = ipqfInBufCh[j][i];
		}
		gc_eff_pqfsyn(NBANDS, NPQFTAPS/(2*NBANDS),
				pp_q0, pp_q1, pp_t0, pp_t1, a_ipqfin,
				output+i*NBANDS, app_pqfbuf0[ch], app_pqfbuf1[ch]);
	}
} /* gc_ipqf_main() */

/*******************************************************************
                         End of  gc_ipqf.c
*******************************************************************/


