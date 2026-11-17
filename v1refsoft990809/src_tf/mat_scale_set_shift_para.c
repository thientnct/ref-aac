/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
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
#ifdef __cplusplus
extern "C" {
#endif

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
#include "ntt_scale_conf.h"
#include "mat_def_ntt.h"

/* For Enh-2 moving */
int mat_scale_set_shift_para2(int iscl, int *bark_table,
			  ntt_INDEX* ntt_index)
{
	int ii, i_ch;
	double band_l, band_h, band_ref;
   for(i_ch=0; i_ch<ntt_index->numChannel; i_ch++){
	band_l= 
	   ntt_index->nttDataScl->ac_btm[iscl][i_ch][mat_shift[iscl][i_ch]];
	band_h= 
	   ntt_index->nttDataScl->ac_top[iscl][i_ch][mat_shift[iscl][i_ch]];
	band_ref= 
	   ntt_index->nttDataScl->ac_btm[1][i_ch][mat_shift[1][i_ch]];
	if(iscl>=0) {
	 float dave, ave, ave0, ave1, iitmp, iitmp2, hltmp;
          /* Long */
                hltmp = (float)(band_h-band_l);
		ave0 = (float)ntt_index->block_size_samples*hltmp;
		ave =  ave0/(float)ntt_N_CRB_SCL;
                dave = ave/(float)ntt_N_CRB_SCL;
		ave1 = (float)band_l*(float)ntt_index->block_size_samples;
                bark_table [i_ch*ntt_N_CRB_SCL+ntt_N_CRB_SCL-1] =
		     (int)ave1+(int)ave0;
		for(ii=0;ii<ntt_N_CRB_SCL-1;ii++){
		   iitmp = (float)(ii+1);
		   iitmp2 = iitmp*iitmp;
		   iitmp *= ave;
		   iitmp *= 0.5;
		   iitmp2 *= dave;
		   iitmp2 *= 0.5;
		   iitmp2 += 0.5;
		   iitmp2 += iitmp;
		   bark_table[i_ch*ntt_N_CRB_SCL+ii] =
		     (int)ave1+ (int)iitmp2;
		}
	}
       }
	return(0);
}
#ifdef __cplusplus
}
#endif
