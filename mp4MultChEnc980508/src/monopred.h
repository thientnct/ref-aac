/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
Bernd Edler and Hendrik Fuchs, University of Hannover in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
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
