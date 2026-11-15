/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 18-apr-97   NI   generalized the module */

#include <stdio.h>
#include <string.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"
#include "ntt_relsp.h"


#if 1
/* see : ntt_get_cdbk.c */
#else 

void ntt_get_code(char *fname,
		  int nstage,
		  int csize[],
		  int cdim[],
		  double code[][ntt_N_PR_MAX],
		  double fgcode[][ntt_MA_NP][ntt_N_PR_MAX])
{
  FILE    *fp;
  int     i, j, k, off_code;
  double   code_buf[ntt_NC0_MAX+ntt_NC1_MAX][ntt_N_PR_MAX];
  int i_mode, i_ma;
  char ftmpname[100];

  if( ( fp = fopen( fname, "r" ) ) == NULL ){
    strcpy(ftmpname,"../"); 
    strcat(ftmpname,fname);
    if( ( fp = fopen( ftmpname, "r" ) ) == NULL ){      
      CommonExit ( 1, "ERROR!! Can't open codebook file: %s\n",
                   fname);
    }
  }

  off_code = 0;
  for( i=0; i<nstage; i++ ) {
    for( j=0; j<csize[i]; j++ ) {
      for( k=0; k<cdim[i]; k++ ){
	if( 1 != fscanf( fp, "%le", &(code_buf[off_code+j][k]) ) ){
	  CommonExit ( 1, " ERROR!! Can't read file in ntt_get_code\n" );
	}
	code[off_code+j][k] = code_buf[off_code+j][k];
      }
    }
    off_code += csize[i];
  }

  for(i_mode=0; i_mode<ntt_N_MODE; i_mode++){
    for(i_ma=0; i_ma<ntt_MA_NP; i_ma++){
      for(k=0; k<ntt_N_PR; k++){
	if (fscanf( fp, "%le", &(fgcode[i_mode][i_ma][k])) == EOF){
	  CommonExit ( 1,
		  "ntt_get_code(): %s: EOF befor reading finished.\n",
		  fname);
	}
      }
    }
  }
  fclose( fp );
}
#endif
