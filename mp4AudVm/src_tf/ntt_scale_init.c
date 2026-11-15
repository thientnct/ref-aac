/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/*   Kazuaki Chikira and Takehiro Moriya (NTT) on 2000-08-11,                */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/


#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "mat_def_ntt.h"


void ntt_scale_init ( int        iscl, 
                      int        cb_lyr, 
                      int        ibps_scl, 
                      float      sampling_rate,
                      ntt_INDEX* ntt_index, 
                      ntt_INDEX* ntt_index_scl)
{
  /*--- Variables ---*/
  int i_ch, iscl0;
  int lsp_csize[2], lsp_cdim[2];
  static float ntt_ibps_scl[8];
  double *lsp_code_scl;
  double *lsp_fgcode_scl;
  static int prev_fw_code[8][ntt_FW_N_DIV_MAX*2];  
  static int prev_lsp_code[8][ntt_LSP_NIDX_MAX*2];  

  ntt_ibps_scl[iscl] = (float)ibps_scl/(float)ntt_index->numChannel; 
  
  ntt_index_scl->numChannel = ntt_index->numChannel;
  ntt_index_scl->block_size_samples = ntt_index->block_size_samples;
  
  /*--- Set other variables ---*/
  /*----------------------------------------------------------------------*/
  /* PROGRAM MODE                                                         */
  /*----------------------------------------------------------------------*/
    
  /* number of bits for gain quantization tool */
  lsp_csize[0] = ntt_NC0_SCL;
  lsp_csize[1] = ntt_NC1_SCL;
  lsp_cdim[0]= ntt_N_PR_SCL;
  lsp_cdim[1]= ntt_N_PR_SCL;

  /*----------------------------------------------------------------------*/
  /* LSP QUANTIZATION                                                     */
  /*----------------------------------------------------------------------*/
  
  /* codebook memory allocation and reading */
  if (iscl < cb_lyr){
    lsp_code_scl = 
      (double *)malloc((ntt_NC0_SCL+ntt_NC1_SCL)*ntt_N_PR_MAX
                       *sizeof(double));
    lsp_fgcode_scl = 
      (double *)malloc(ntt_N_MODE_SCL*ntt_MA_NP*ntt_N_PR_MAX
                       *sizeof(double));
    ntt_get_code(ntt_LSPCODEBOOK_SCL, ntt_LSP_NSTAGE,
                 lsp_csize, lsp_cdim,
                 (double (*)[ntt_N_PR_MAX])lsp_code_scl,
                 (double (*)[ntt_MA_NP][ntt_N_PR_MAX])lsp_fgcode_scl);
    ntt_index_scl->nttDataScl->lsp_code_scl = lsp_code_scl; 
    ntt_index_scl->nttDataScl->lsp_fgcode_scl = lsp_fgcode_scl; 
  }
  {
       int idiv;
       for(idiv=0; idiv<ntt_LSP_NIDX_MAX*ntt_index->numChannel; idiv++){
         prev_lsp_code[iscl][idiv] =0;
       }
       ntt_index_scl->nttDataScl->prev_lsp_code[iscl]=&(prev_lsp_code[iscl][0]);
       ntt_index_scl->nttDataScl->ma_np= 0;
  }

  /*----------------------------------------------------------------------*/
  /* FORWARD ENVELOPE QUANTIZATION                                        */
  /*----------------------------------------------------------------------*/
  
  {
       int idiv;
       for(idiv=0; idiv<ntt_FW_N_DIV*ntt_index->numChannel; idiv++){
         prev_fw_code[iscl][idiv] =0;
       }
       ntt_index_scl->nttDataScl->prev_fw_code[iscl]= &(prev_fw_code[iscl][0]);
       ntt_index_scl->nttDataScl->prev_w_type= -1;
  }

  /*--- Set variables for envelope coding ---*/
  /*----------------------------------------------------------------------*/
  /* BITRATE CONTROL                                                      */
  /*----------------------------------------------------------------------*/
  {
   float ftmp;
   /* int available_bits; */

    /* Number of bits per frame */
    ftmp =  (float)(ntt_index->block_size_samples*ntt_index->numChannel*
            ntt_ibps_scl[iscl]);
    ftmp /= (float)sampling_rate;
    ftmp += 0.5;
    ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl] = (int)ftmp;
#if BYTE_ALIGN
    ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl] 
     = (ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]/8)*8; /* BYTE-A */    
#endif
    ntt_ibps_scl[iscl] 
      = ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl] * sampling_rate
	/ntt_index->block_size_samples /ntt_index->numChannel; /* BYTE-A */    
if (tvq_debug_level>5)
fprintf(stderr, "UUUUUUUUU %5d %12.3f %5d bitrate for layer \n",
iscl, ntt_ibps_scl[iscl], ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]);
    /* number of available bits */
    /*available_bits = ntt_NBITS_FR_SCL[iscl];*/
  }
  /*----------------------------------------------------------------------*/
  /* READING CODEBOOK TABLES                                              */
  /*----------------------------------------------------------------------*/
  if (iscl < cb_lyr){
    int    cb_len_max;
    /*--- Interleaved vector quantization ---*/
    /* long frame */
    cb_len_max = ntt_CB_LEN_READ_SCL + ntt_CB_LEN_MGN;
    ntt_index_scl->nttDataScl->ntt_codev0_scl =
      (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
    ntt_index_scl->nttDataScl->ntt_codev1_scl =
      (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
    ntt_get_cdbk(ntt_CB_NAME_SCL0, ntt_index_scl->nttDataScl->ntt_codev0_scl,
                 ntt_CB_SIZE, ntt_CB_LEN_READ_SCL, cb_len_max);
    ntt_get_cdbk(ntt_CB_NAME_SCL1, ntt_index_scl->nttDataScl->ntt_codev1_scl,
                 ntt_CB_SIZE, ntt_CB_LEN_READ_SCL, cb_len_max);
    /* short frame */
    cb_len_max = ntt_CB_LEN_READ_S_SCL + ntt_CB_LEN_MGN;
    ntt_index_scl->nttDataScl->ntt_codev0s_scl =
      (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
    ntt_index_scl->nttDataScl->ntt_codev1s_scl =
      (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
    ntt_get_cdbk(ntt_CB_NAME_SCL0s, ntt_index_scl->nttDataScl->ntt_codev0s_scl,
                 ntt_CB_SIZE, ntt_CB_LEN_READ_S_SCL, cb_len_max);
    ntt_get_cdbk(ntt_CB_NAME_SCL1s, ntt_index_scl->nttDataScl->ntt_codev1s_scl,
                 ntt_CB_SIZE, ntt_CB_LEN_READ_S_SCL, cb_len_max);
    /*--- Bark-scale envelope ---*/
    /* long frame */
    ntt_index_scl->nttDataScl->ntt_fwcodev_scl =
      (double *)malloc(ntt_FW_CB_SIZE_SCL*ntt_FW_CB_LEN_SCL
                       *sizeof(double));
    ntt_get_cdbk(ntt_FW_CB_NAME_SCL, ntt_index_scl->nttDataScl->ntt_fwcodev_scl,
                 ntt_FW_CB_SIZE_SCL,
                 ntt_FW_CB_LEN_SCL, ntt_FW_CB_LEN_SCL);
  }
  /* band limit */
 { double qsample, bias, totalbps;
    double upperlimit;
    int upperlimit_i;
    int qsample_i;
    int bias_i;

     totalbps= ntt_index->nttDataBase->ntt_NBITS_FR
	     * sampling_rate/(float)ntt_index->block_size_samples
			/(float)ntt_index->numChannel;
     for(iscl0=0; iscl0<=iscl; iscl0++){
       totalbps += ntt_ibps_scl[iscl0]; 
     }
     upperlimit_i = (int)((totalbps * 100) / ntt_index->isampf);
     upperlimit_i = min( 100000, upperlimit_i);
     upperlimit_i *= 1024*16;
     upperlimit_i += 1562;
     upperlimit_i /= 3125;
     upperlimit = (double)(upperlimit_i)/16384.0/32.;
     qsample_i = ((int)ntt_ibps_scl[iscl] * 130) / ntt_index->isampf;
     qsample_i = min( 100000, qsample_i);
     qsample_i *= 1024*16;
     qsample_i += 1562;
     qsample_i /= 3125;

     bias_i = (upperlimit_i - qsample_i)/3;
      
      if( qsample_i < bias_i){
	  bias_i = (int)(upperlimit_i/4.);
	  qsample_i = bias_i;
      }
     qsample = (double)(qsample_i)/16384.0/32.;
     bias = (upperlimit_i - qsample_i)/3/16384.0/32.;

      if(bias <= 0.0) bias =0.0;
    for(i_ch=0; i_ch<ntt_index->numChannel; i_ch++){
         ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][0]=qsample;
         ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][0]=0.0;
         ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][1]=bias+qsample;
         ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][1]=bias;
         ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][2]=bias*2.+qsample;
         ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][2]=bias*2.;
         ntt_index_scl->nttDataScl->ac_top[iscl][i_ch][3]=bias*3.+qsample;
         ntt_index_scl->nttDataScl->ac_btm[iscl][i_ch][3]=bias*3.;
    }
  }
}
