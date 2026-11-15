/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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

#include "config.h"
#include "intensity.h"

/*
 * if (chan==RIGHT) { 
 *  do IS decoding for this channel (scale left ch. values with
 *  factor(SFr-SFl) )
 *  reset all lpflags for which IS is on
 *  pass decoded IS values to predict
 * }
 */
void intensity( MC_Info *mip, Info *info, int ch,
    byte *group, byte *cb_map, short *factors, 
                int *lpflag, Float **coef )
{
    Ch_Info *cip = &mip->ch_info[ch];
    Float   *left, *right, *r, *l, scale;
    int     cb, sign_sfb, sfb, n, nn, b, bb, nband;
    short   *band;
    
    if (!(cip->cpe && !cip->ch_is_left))
	return;
	
    right = coef[ ch ];
    left  = coef[ cip->paired_ch ];
    
    /* Intensity goes by group */
    bb = 0;
    for (b = 0; b < info->nsbk; ) {
	nband = info->sfb_per_sbk[b];
	band = info->sbk_sfb_top[b];

	b = *group++;		/* b = index of last sbk in group */
	for (; bb < b; bb++) {	/* bb = sbk index */
	    n = 0;
	    for (sfb = 0; sfb < nband; sfb++){
		nn = band[sfb];	/* band is offset table, nn is last coef in band */
		cb = cb_map[sfb];
		if (cb == INTENSITY_HCB  ||  cb == INTENSITY_HCB2) {
		    /* found intensity code book */ 
		    /* disable prediction (only important for long blocks) */
		    lpflag[1+sfb] = 0;
		    
	            sign_sfb = (cb == INTENSITY_HCB) ? 1 : -1;
	            scale = sign_sfb * pow( 0.5, 0.25*(factors[sfb]) );

		    /* reconstruct right intensity values */
		    if (debug['i'])
			PRINT(SE,"applying IS coding of %f on ch %d at sfb %3d %d\n",
			    scale, ch, sfb, factors[sfb]);
	            
		    r = right + n;
		    l = left + n;
		    for (; n < nn; n++) {	/* n is coef index */
			*r++ = *l++ * scale;
		    }
		}
		n = nn;
	    }
	    right += info->bins_per_sbk[bb];
	    left += info->bins_per_sbk[bb];
	    factors += nband;
	}
	cb_map += info->sfb_per_sbk[bb-1];
    }
}
