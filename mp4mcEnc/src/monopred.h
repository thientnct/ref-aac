/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Bernd Edler and Hendrik Fuchs (University of Hannover)

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

#define MAX_PGRAD   2
#define MINVAR	    1

#ifndef FLT_MIN
#define FLT_MIN	    1.17549435E-38F
#endif


/* Predictor status information for mono predictor */
typedef struct {
    int		grad;		       	/* predictor order */
    float	alpha;		       	/* adaptation time constant */
    float	a;		       	/* attenuation factor in R-Branch */
    float	b;		       	/* attenuation factor in cross branches */
    float	r[MAX_PGRAD+1];		/* contents of delay elements */
    float	kor[MAX_PGRAD+1];     	/* estimates of correlations */
    float	var[MAX_PGRAD+1];     	/* estimates of variances */
    float	k[MAX_PGRAD+1];		/* predictor coefficients */
} PRED_STATUS;

/* prototypes are now in port.h */

#endif /* _monopred_h_ */
