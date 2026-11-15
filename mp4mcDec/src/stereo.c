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

#include "stereo.h"

void synt(Info *info, byte *group, byte *mask, Float *right, Float *left)
{
    Float vrr, vrl, *r, *l;
    short *band;
    int i, n, nn, b, bb, nband;

    /*mask is grouped */
    bb = 0;
    for(b = 0; b < info->nsbk; ){
	nband = info->sfb_per_sbk[b];
	band = info->sbk_sfb_top[b];

	b = *group++;		/*b = index of last sbk in group */
	for(; bb < b; bb++){	/* bb = sbk index */
	    n = 0;
	    for(i = 0; i < nband; i++){
		nn = band[i];	/* band is offset table, nn is last coef in band */
		if(mask[i]){
		    r = right + n;
		    l = left + n;
		    for(; n < nn; n++){	/* n is coef index */
			vrr = *r;
			vrl = *l;
			*l = vrr + vrl;
			*r = vrl - vrr;
			r++;
			l++;
		    }
		}
		n = nn;
	    }
	    right += info->bins_per_sbk[bb];
	    left += info->bins_per_sbk[bb];

	}
	mask += info->sfb_per_sbk[bb-1];
    }
}

/* Map mask to intensity stereo signalling */
void map_mask(Info *info, byte *group, byte *mask, byte *cb_map, byte hasmask)
{
    
    /* pre-condition: (hasmask == 1) || (hasmask == 2) */
    int sfb, b, nband;
    
    /* mask goes by group */
    for (b = 0; b < info->nsbk; ) {
        nband = info->sfb_per_sbk[b];
        
        for (sfb = 0; sfb<nband; sfb++){
            switch (cb_map[sfb])
            {
            case INTENSITY_HCB:
                if (mask[sfb])
                {
                    if (hasmask == 1) 
                    {
                        cb_map[sfb] = INTENSITY_HCB2;
                    }
                    mask[sfb] = 0;
                }
                break;
            case INTENSITY_HCB2:
                if (mask[sfb])
                {
                    if (hasmask == 1) 
                    {
                        cb_map[sfb] = INTENSITY_HCB;
                    }
                    mask[sfb] = 0;
                }
                break;
#ifdef MPEG4V1
            case NOISE_HCB:
                if (mask[sfb])
                {
                    cb_map[sfb] = NOISE_HCB + 100;
                    mask[sfb] = 0;
                }
                break;
#endif
            default: break;
            }
        }
        
        mask += info->sfb_per_sbk[b];
        cb_map += info->sfb_per_sbk[b];
        b = *group++;        /* b = index of last sbk in group */
    }
}
