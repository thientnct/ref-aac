/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Takehiro Moriya (NTT) on 1997-08-01,                                    */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Takehiro Moriya (NTT) on 1997-10-14,                                    */
/* in the course of development of the                                       */
/* MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 NBC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 25-aug-97   NI   bugfixes */

#include <math.h>
#include <stdio.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void     ntt_extend_pitch(
	      /* Input */
	      int     index_pit[], 
	      double  reconst[], 
	      int    numChannel,
	      int    block_size_samples,
	      int    isampf,
	      double  bandUpper,
	      /* Output */
	      double  pit_seq[])
{
    /*--- Variables ---*/
    int	   i_smp;
    float dtmp, pitch;
    float tmpnp0, tmpnp1;
    int npcount, iscount, top, ptop;
    int i_sup, ii,jj;
    float fcmin, fcmax, fctmp0, fctmp1;
    float bandwidth;
    fctmp1 = (float)block_size_samples/(float)isampf;
    fctmp0 = 1./(float)log(2.);
    fcmin = (float)log(fctmp1*0.2)*fctmp0;
    fcmax = (float)log(fctmp1*2.4)*fctmp0;
    bandwidth =4.0;
    if(isampf==8) bandwidth=1.5;
    if(isampf>=11) bandwidth=2.;
    if(isampf>=22) bandwidth=4.;
    if(bandwidth < (float)(1./bandUpper)) bandwidth = (float)(1./bandUpper);
    for(i_sup=0; i_sup< numChannel; i_sup++){
        top = i_sup * block_size_samples;
	ptop = i_sup * ntt_N_FR_P;
	dtmp = (float)index_pit[i_sup] / (float) ntt_BASF_STEP;
	fctmp0 =  fcmax-fcmin;
	dtmp = dtmp * fctmp0;
	dtmp = dtmp  + fcmin;
	pitch = (float)pow(2., dtmp);
	tmpnp0 = pitch*bandwidth;
	tmpnp1 = tmpnp0/(float)block_size_samples;
	tmpnp0 = (float)ntt_N_FR_P*tmpnp1;
	npcount = (int)tmpnp0;
	iscount=0;
        for (jj=0; jj<block_size_samples; jj++){
		    pit_seq[jj+top] = 0.0;
        }
        for (jj=0; jj<npcount/2; jj++){
		    pit_seq[jj+top] = reconst[jj+ptop];
		    iscount ++;
        }
        for (ii=0; ii<(ntt_N_FR_P )&& (iscount<ntt_N_FR_P); ii++){
	    tmpnp0 = pitch*(float)(ii+1);
	    tmpnp0 += 0.5;
	    i_smp = (int)(tmpnp0);
	    if(i_smp+(npcount-1)/2+1>=block_size_samples) continue;
            for (jj=-npcount/2; jj<(npcount-1)/2+1; jj++){
	       pit_seq[i_smp+jj+top] = reconst[iscount+ptop];
               iscount ++;
	       if(iscount >= ntt_N_FR_P) break;
	    }
        }
    }

}
