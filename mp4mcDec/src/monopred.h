/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Bernd Edler and Hendrik Fuchs, University of Hannover

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

#ifndef _monopred_h_
#define _monopred_h_

#include "adifdec.h"

#define MAX_PGRAD   2
#define MINVAR	    1
#define	Q_ZERO	    0x0000
#define	Q_ONE	    0x3F80
 
/* Predictor status information for mono predictor */
typedef struct {
  short  r[MAX_PGRAD];		/* contents of delay elements */
  short  kor[MAX_PGRAD];     	/* estimates of correlations */
  short  var[MAX_PGRAD];     	/* estimates of variances */
#ifdef LAST_RESET_TEST
  short  last_reset;
#endif
} PRED_STATUS;

typedef struct {
  float  r[MAX_PGRAD];		/* contents of delay elements */
  float  kor[MAX_PGRAD];     	/* estimates of correlations */
  float  var[MAX_PGRAD];     	/* estimates of variances */
#ifdef LAST_RESET_TEST
  long  last_reset;
#endif
} TMP_PRED_STATUS;


extern	int		*lpflag[Chans];
extern	int		*prstflag[Chans];


void		init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b);
void		predict(Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *coef);
void		predict_reset(Info* info, int *prstflag, PRED_STATUS **psp, int firstCh, int lastCh, int *last_rstgrp_num, int *lpflag);
void		reset_pred_state(PRED_STATUS *psp);

#endif /* _monopred_h_ */
