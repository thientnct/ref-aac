/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
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
void    ntt_dec_pitch(/* Input */
		  int    index_pit[],
		  int    index_pls[],
		  int    index_pgain[],
		  int    numChannel,
		  int    block_size_samples,
		  int    isampf,
		  /* Output */
		  double pit_seq[],
		  double pgain[],
		  double bandUpper,
		  double *pcode,
		  short  *pleave1)
{
    /*--- Variables ---*/
    int i_sup , top;
	    for (i_sup=0; i_sup< numChannel; i_sup++){
	        top = i_sup * block_size_samples;
	        ntt_dec_pgain(index_pgain[i_sup], &pgain[i_sup]);
	    }
#if 0
	    ntt_dec_pit_seq(index_pit, index_pls, pit_seq);
	}

	break;
    case EIGHT_SHORT_SEQUENCE:
	for (i_sup=0; i_sup<ntt_N_SUP; i_sup++){
	    top = i_sup * ntt_N_FR;
	    for (isf=0; isf<ntt_N_SHRT; isf++){
		subtop = isf * ntt_N_FR_S;
		idtop  = i_sup * ntt_N_SHRT + isf;
		pgain[idtop]=0.0;
		for(ismp=0; ismp<ntt_N_FR_S; ismp++)
		     pit_seq[top+subtop+ismp] =0.0;
	    }
<<<<<<< ntt_dec_pitch.c
	}
	break;
    }

=======
#endif
	    ntt_dec_pit_seq(index_pit, index_pls,  numChannel,
	       block_size_samples, isampf,
	       bandUpper, pcode, pleave1, pit_seq);
}
