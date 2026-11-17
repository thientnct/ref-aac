/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
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
#include <string.h>
#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"

#if ntt_CB_INCLUDE

#include <string.h>
#include "codebook.h"

void ntt_get_cdbk(char *name,
	      double *codev_l,
	      int cb_size,
	      int cb_len,
	      int cb_len_mx)
{
    /* Variables */
    int		i_cb, i_cv, index, index2, idflg;
    double      *cbptr;

    idflg=0;
    if (strcmp(name, ntt_CB_NAME0) == 0){
	cbptr = cb_shape_0;
	idflg=1;
    }
    if (strcmp(name, ntt_CB_NAME1) == 0){
	cbptr = cb_shape_1;
	idflg=1;
    }
    if (strcmp(name, ntt_CB_NAME2) == 0){
	cbptr = cb_shape_2;
	idflg=1;
    }
    if (strcmp(name, ntt_CB_NAME3) == 0){
	cbptr = cb_shape_3;
	idflg=1;
    }
    if (strcmp(name, ntt_FW_CB_NAME) == 0){
	cbptr = cb_env;
	idflg=1;
    }
    if (strcmp(name, ntt_PIT_CB_NAME) == 0){
	cbptr = cb_pit;
	idflg=1;
    }

    if (! idflg){
      CommonExit ( 1, "%s: Codebook name error.\n", name);
    }

    /* Main operation */
    for ( i_cb=0; i_cb<cb_size; i_cb++ ){
	for ( i_cv=0; i_cv<cb_len; i_cv++ ){
	    index = i_cb*cb_len_mx  + i_cv;
	    index2 = i_cb*cb_len + i_cv;
	    codev_l[index] = cbptr[index2];
	}
    }
}

#else /* ntt_CB_INCLUDE */

/* ntt_get_cdbk(): get codebook from codebook file */

void ntt_get_cdbk(char *name,
	      double *codev_l,
	      int cb_size,
	      int cb_len,
	      int cb_len_mx)
{
    /* Variables */
    FILE	*fp;
    int		i_cb, i_cv, index;
    char ftmpname[100];
  
    /* open data file */
    if ( (fp=fopen(name,"r")) == NULL ){
      strcpy(ftmpname,"../"); 
      strcat(ftmpname,name);
      if ( (fp=fopen(ftmpname,"r")) == NULL ){
	CommonExit ( 1, "Error! Codebook file not found: %s\n", name );
      }
    }
    if(tvq_debug_level > 5)
       fprintf(stderr, "JJJJJ get_cdbk %5d %5d %5d \n",
              cb_size, cb_len, cb_len_mx);
    /* Main operation */
    for ( i_cb=0; i_cb<cb_size; i_cb++ ){
      ntt_zerod(cb_len_mx-cb_len, &codev_l[cb_len+i_cb*cb_len_mx]);
      for ( i_cv=0; i_cv<cb_len; i_cv++ ){
	index = i_cb*cb_len_mx  + i_cv;
	fscanf( fp, "%lf", &codev_l[index] );
      }
    }
    fclose (fp);
}


#endif /* ntt_CB_INCLUDE */
