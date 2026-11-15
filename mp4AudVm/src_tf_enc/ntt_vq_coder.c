/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Takeshi Mori (NTT) on 2000-11-21,                                       */
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

/* 25-aug-1997  NI  bug fixes */

#include <stdio.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

/*#include "tf_main.h"*/
#include "bitstream.h"
#include "nok_ltp_enc.h" 
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_encode.h"
#include "flex_mux.h"

void ntt_vq_coder(double *in_init[MAX_TIME_CHANNELS],
		  double *inSpec[MAX_TIME_CHANNELS],
		  double      *spectral_line_vector[MAX_TIME_CHANNELS],
		  double      external_pw[],
		  int         pw_select,
		  WINDOW_SEQUENCE         windowSequence,
		  ntt_INDEX   *index,
		  ntt_PARAM   *param_ntt,
		  int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
                  int         nr_of_sfb[MAX_TIME_CHANNELS],
		  int         available_bits,
		  double      lpc_spectrum[],
		  double      *reconstructed_spectrum[MAX_TIME_CHANNELS])
{
  static double sig_tmp[ntt_T_FR_MAX*3], *sig;
  static double bark_env[ntt_T_FR_MAX];
  static double spectrum[ntt_T_FR_MAX];
  static double pitch_sequence[ntt_T_FR_MAX];
  static double gain[ntt_T_SHRT_MAX];
  static double perceptual_weight[ntt_T_FR_MAX];
  static int InitFlag = 1;
  int    sb, ismp, i_ch, top;
  double spectrum_gain = 0;
  Info   *sfbInfo[4], *p_sfbInfo;
  int nsbk = 0;
  int nfr = 0;

  /*--- Initialization ---*/
  if (InitFlag){
    ntt_zerod(index->block_size_samples*index->numChannel*2, sig_tmp);
    sig = sig_tmp + index->block_size_samples * index->numChannel;
    for (i_ch=0; i_ch<index->numChannel; i_ch++){
      top = index->block_size_samples * i_ch;
      ntt_movdd(index->block_size_samples, in_init[i_ch], sig+top);
    }
  }
  
 {
  if(index->w_type == EIGHT_SHORT_SEQUENCE){
     p_sfbInfo = &eight_short_info;
     p_sfbInfo->nsbk = 8;
     p_sfbInfo->islong = 0;
     for(ismp=0; ismp<8; ismp++){
       p_sfbInfo->bins_per_sbk[ismp] = index->block_size_samples/8;
       p_sfbInfo->sfb_per_sbk[ismp]= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index->max_sfb[0] /*p_sfbInfo->sfb_per_sbk[ismp]*/; sb++){
	 top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = top;
       }
     }
  }
  else{
     p_sfbInfo = &only_long_info;
     p_sfbInfo->nsbk = 1;
     p_sfbInfo->islong = 1;
     p_sfbInfo->bins_per_bk = index->block_size_samples;
       p_sfbInfo->sfb_per_bk= nr_of_sfb[0];
       top =0;
       for(sb=0; sb<index->max_sfb[0] /*p_sfbInfo->sfb_per_sbk[0]*/; sb++){
         top = top +sfb_width_table[0][sb];
         p_sfbInfo->bk_sfb_top[sb] = top;
       }
  }
  sfbInfo[index->w_type] = p_sfbInfo;
  
}
   if(tvq_debug_level >5)
   fprintf(stderr, "UUUUUUU %5d %5d %5d available_bits ntt_NMTOOL_BITS\n",
    available_bits, ntt_NMTOOL_BITSperCH*index->numChannel+3, 
    ntt_NMTOOL_BITS_SperCH*index->numChannel+1);

  for (i_ch=0; i_ch<index->numChannel; i_ch++){
    top = index->block_size_samples * i_ch;
    ntt_movdd(index->block_size_samples, inSpec[i_ch], sig+index->block_size_samples*index->numChannel+top);
  }

  /*--- Pre process ---*/
  if (param_ntt->ntt_param_set_flag == 0){
    ntt_tf_pre_process(sig, index, param_ntt, 1, InitFlag);
  }
  index->w_type = windowSequence;

  /*--- Bit number control ---*/
  switch(windowSequence){
    /* long frame */
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    index->nttDataBase->ntt_VQTOOL_BITS 
      = available_bits - (ntt_NMTOOL_BITSperCH*index->numChannel+3);
    spectrum_gain = 1./sqrt((double)index->block_size_samples*2.);
    nsbk=1;
    nfr = index->block_size_samples;
    break;
    /* short frame */
  case EIGHT_SHORT_SEQUENCE:
    index->nttDataBase->ntt_VQTOOL_BITS_S 
      = available_bits - (ntt_NMTOOL_BITS_SperCH*index->numChannel+1);
    spectrum_gain = 1./sqrt((double)index->block_size_samples/8*2.);
    nsbk=8;
    nfr = index->block_size_samples/8;
    break;
  default:
    fprintf(stderr, "ntt_vq_coder(): Error. %d: No such window type.\n",
	    windowSequence);
    exit(1);
    break;
  }

  /*--- Spectrum line normalization
    (for gain alignment of NTT & MPEG4 VM MDCT ---*/
  for (i_ch=0; i_ch<index->numChannel; i_ch++){
    top = i_ch * index->block_size_samples;
    for (ismp=0; ismp<index->block_size_samples; ismp++){
      spectrum[ismp+top] = spectral_line_vector[i_ch][ismp] * spectrum_gain;
    }
  }
  
  if(index->numChannel == 2){
  int sb, win;
  int sfbw;
  double buf[ntt_N_FR_MAX];
   /* if(index->ms_mask ==1 ) */
    {
     for( win=0; win<nsbk; win++ ) {
     int sboffs   = win*nfr;

     for( sb=0; sb<index->max_sfb[0] /*nr_of_sfb[MONO_CHAN]*/; sb++ ) {
           sfbw = sfb_width_table[MONO_CHAN][sb];

       if( index->msMask[win][sb] != 0  ) {
         for (ismp=sboffs; ismp<sboffs+sfbw; ismp++){
           buf[ismp] =  (spectrum[ismp] - spectrum[ismp+index->block_size_samples])/2.;
           spectrum[ismp] =  (spectrum[ismp]+spectrum[ismp+index->block_size_samples])/2.;
           spectrum[ismp+index->block_size_samples]  = buf[ismp];
         }
       }
       sboffs += sfbw;
      }
     }
    }
/*
    else if(index->ms_mask ==2){
        for (ismp=0; ismp<index->block_size_samples; ismp++){
	       buf[ismp] =  (spectrum[ismp] - spectrum[ismp+index->block_size_samples])/2.;
	       spectrum[ismp] =(spectrum[ismp]+spectrum[ismp+index->block_size_samples])/2.;
	       spectrum[ismp+index->block_size_samples]  = buf[ismp];
        }
    }
*/
  }

  /*--- Process spectrum ---*/
  ntt_tf_proc_spectrum(spectrum, index,
		       lpc_spectrum, bark_env,
		       pitch_sequence, gain);
  switch(windowSequence){
    /* pitch_q flag long frame */
  case ONLY_LONG_SEQUENCE: case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    if( index->pitch_q == 1) 
	  index->nttDataBase->ntt_VQTOOL_BITS -=  ntt_PIT_TBITperCH*index->numChannel;
    break;
  case EIGHT_SHORT_SEQUENCE: /* T.Ishikawa 981012*/
	break;
  case NUM_WIN_SEQ:
	break;
  }
  if(index->bandlimit == 1){
	index->nttDataBase->ntt_VQTOOL_BITS   -= ntt_BWID_BITS*index->numChannel;
	index->nttDataBase->ntt_VQTOOL_BITS_S -= ntt_BWID_BITS*index->numChannel;
  }
      /*--- Perceptual model ---*/
  if (pw_select == NTT_PW_EXTERNAL &&
      (index->w_type == ONLY_LONG_SEQUENCE ||
       index->w_type == LONG_START_SEQUENCE ||
       index->w_type == LONG_STOP_SEQUENCE )){
    /* external perceptual weighting */
    ntt_movdd(index->block_size_samples*index->numChannel, external_pw, perceptual_weight);
  }
  else{
    /* internal perceptual weighting */
    ntt_tf_perceptual_model(lpc_spectrum, bark_env, gain, windowSequence,
			    spectrum, 
			    pitch_sequence, 
			    index->numChannel,
			    index->block_size_samples,
			    index->nttDataBase->bandUpper,
			    perceptual_weight);
  }

  /*--- Quantize spectrum ---*/
  ntt_tf_quantize_spectrum(spectrum, lpc_spectrum, bark_env,
			   pitch_sequence,
			   gain, perceptual_weight, index);
  ntt_vq_decoder(index,  reconstructed_spectrum, sfbInfo);
  { 
    int ii, jj;
    float dummy;
    for (ii=0; ii<index->numChannel; ii++){
      for(jj=0; jj<index->block_size_samples; jj++){
        dummy=(float)reconstructed_spectrum[ii][jj];
        reconstructed_spectrum[ii][jj]=dummy;
      }
    }
  }

  ntt_movdd(index->block_size_samples*index->numChannel*2, sig, sig-index->block_size_samples*index->numChannel);
  InitFlag = 0;

}

void ntt_headerenc(int iscl,
		   HANDLE_BSBITSTREAM  stream, 
		   ntt_INDEX* index, 
		   int *used_bits,
		   int *nr_of_sfb,
		   TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
		   NOK_LT_PRED_STATUS *nok_lt_status,
		   PRED_TYPE pred_type)
{
      int g,sfb,win;
      int gwin, num_groups;
      short group_len[8];
      QC_MOD_SELECT qc_select = NTT_VQ;

      if(index->numChannel > 1){
           BsPutBit(stream, (unsigned int)index->ms_mask, 2);
           *used_bits += 2;
      }
      if( index->numChannel==2 ){
       if (index->ms_mask==1){
          if (index->w_type== EIGHT_SHORT_SEQUENCE ){
	    if(iscl < 0){ /* only core */
	       BsPutBit( stream,index->group_code,7);      /* max_sfb */
               *used_bits += 7;
	    }
	    num_groups =
              TVQ_decode_grouping ( index->group_code, group_len );
          }
	  else {
            num_groups = 1;
	    group_len[0]=1;
	  }
         win=0;
          for (g=0;g<num_groups;g++){
           for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
             BsPutBit(stream,index->msMask[win][sfb],1);/*ms_mask*/
             *used_bits += 1;
           }
           for (gwin=1;gwin< group_len[g]; gwin++){
             for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
               index->msMask[win+gwin][sfb]=index->msMask[win][sfb];
                /* apply same ms mask to a ll subwindows of same group */
             }
           }
           /*win += (gwin -1);*/
           win += group_len[g];
         }
       }
     }
 if(iscl <0){
   WINDOW_SEQUENCE windowSequence;
   int i_ch;

   windowSequence = index->w_type;
   
   for(i_ch=0; i_ch < index->numChannel; i_ch++)
   {
     if(pred_type == NOK_LTP) /* LTP used. */
       *used_bits += nok_ltp_encode(stream, windowSequence, 
       /**TM nr_of_sfb[i_ch], */   index->max_sfb[0],
				    nok_lt_status + i_ch, NTT_VQ_SYS, 
				    NTT_TVQ, 1 /**TM i_ch***/);
     else
     {
       BsPutBit(stream, 0, 1); /* LTP not used. */
       *used_bits += 1;
     }

     /* TNS data */
      if(tnsInfo[MONO_CHAN]) {
	if (tnsInfo[MONO_CHAN]->tnsDataPresent) {     /* TNS active */
			int tns_bits,bits_tmp;
			bits_tmp=0;
                        tns_bits= write_tns_data( tnsInfo[MONO_CHAN], 
                                        windowSequence, stream, 0 );
        if(tvq_debug_level>5)
	  fprintf(stderr, "AAAAA TNS %5d %5d\n", 
	  tnsInfo[MONO_CHAN]->tnsDataPresent, tns_bits);
#if 1
			switch (index->w_type){
				case ONLY_LONG_SEQUENCE: 
				case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
				bits_tmp = index->nttDataBase->ntt_NBITS_FR
				   -(ntt_NMTOOL_BITSperCH*index->numChannel)-tns_bits-(*used_bits);
				break;
				case EIGHT_SHORT_SEQUENCE:
				bits_tmp = index->nttDataBase->ntt_NBITS_FR
				      -(ntt_NMTOOL_BITS_SperCH*index->numChannel)
				      -tns_bits-(*used_bits);
				break;
                                default:
	                        break;
			}

			if(bits_tmp<50) /* TNS not active T.Ishikawa 981214 */
				{
					BsPutBit(stream,0,1);
					*used_bits += 1;
					tnsInfo[MONO_CHAN]->tnsDataPresent=0;
					index->restore_flag_tns=1;
					if(tvq_debug_level>2)
					fprintf(stderr,"tnsDataNotPresent %5d ,bits_tmp %5d\n",tns_bits,bits_tmp);
				}
			else /* TNS active T.Ishikawa 981214 */
#endif
				{
					*used_bits+=write_tns_data( tnsInfo[MONO_CHAN], 
                                        windowSequence, stream, 1 );
					index->restore_flag_tns=0;
					if(tvq_debug_level>2)
					fprintf(stderr,"tnsDataPresent %5d bits_tmp %5d\n",tns_bits,bits_tmp);
				}
        } 
        else {                    /* switch it off */
         BsPutBit(stream,0,1);
         *used_bits += 1;
        }
      }
      else {                    /* switch it off */
        BsPutBit(stream,0,1);
        *used_bits += 1;
      }

	if(tvq_debug_level>2)
        fprintf(stderr, "TNS end %5d\n",  *used_bits); 
   }

 } /* only for core */

}



void ntt_select_ms( double      *spectral_line_vector[MAX_TIME_CHANNELS],
		    ntt_INDEX   *index,
		    int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
                    int         nr_of_sfb[MAX_TIME_CHANNELS],
		    int         iscl)
{
 int ismp;
 double bufm[1024];
 double bufs[1024];
 double lp, rp, ratio_lr, ratio_sm;
 int ii, jj;
 int sfb_offset[60];
	if(index->numChannel < 2) return;
    { int k, i;
	   k=0;
	   sfb_offset[0] = 0;
           for(i = 0; i < nr_of_sfb[MONO_CHAN]; i++ ) {
               sfb_offset[i] = k;
               k +=sfb_width_table[MONO_CHAN][i];
           }
    }
    if(index->ms_mask != 1){
        lp=rp=0.0;
        for (ismp=0; ismp<index->block_size_samples; ismp++){
	   lp += (spectral_line_vector[0][ismp]*spectral_line_vector[0][ismp]);
	   rp += (spectral_line_vector[1][ismp]*spectral_line_vector[1][ismp]);
        }
        ratio_lr = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
        for (ismp=0; ismp<index->block_size_samples; ismp++){
	       bufm[ismp] =  (spectral_line_vector[0][ismp]
			     +spectral_line_vector[1][ismp]);
	       bufs[ismp] =  (spectral_line_vector[0][ismp]
			     -spectral_line_vector[1][ismp]);
        }
        lp=rp=0.0;
        for (ismp=0; ismp<index->block_size_samples; ismp++){
	   lp += (bufm[ismp]*bufm[ismp]);
	   rp += (bufs[ismp]*bufs[ismp]);
        }
        ratio_sm = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
	if (ratio_lr > ratio_sm) index->ms_mask = 0;
        else                     index->ms_mask = 2;
            index->group_code =127; /*TEST*/
    }
    if(tvq_debug_level>2)
    fprintf(stderr, "MMMMMM %5d flag\n",  index->ms_mask);
if(index->ms_mask ==1){
  if(index->w_type==EIGHT_SHORT_SEQUENCE){
    int is, icounter;
    icounter =0;
    for(jj=index->last_max_sfb[iscl+1]; jj<index->max_sfb[iscl+1]; jj++){
        lp=rp=0.0;
        for(ii=0; ii<8; ii++){
	 is= index->block_size_samples/8*ii;
	 for (ismp=is+sfb_offset[jj]; ismp<is+sfb_offset[jj+1]; ismp++){
	   lp += (spectral_line_vector[0][ismp]*spectral_line_vector[0][ismp]);
	   rp += (spectral_line_vector[1][ismp]*spectral_line_vector[1][ismp]);
         }
	}
        ratio_lr = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
        for(ii=0; ii<8; ii++){
	 is= index->block_size_samples/8*ii;
         for (ismp=is+sfb_offset[jj]; ismp<is+sfb_offset[jj+1]; ismp++){
	       bufm[ismp] =  (spectral_line_vector[0][ismp]
			     +spectral_line_vector[1][ismp]);
	       bufs[ismp] =  (spectral_line_vector[0][ismp]
			     -spectral_line_vector[1][ismp]);
         }
        }
        lp=rp=0.0;
        for(ii=0; ii<8; ii++){
	 is= index->block_size_samples/8*ii;
         for (ismp=is+sfb_offset[jj]; ismp<is+sfb_offset[jj+1]; ismp++){
	   lp += (bufm[ismp]*bufm[ismp]);
	   rp += (bufs[ismp]*bufs[ismp]);
         }
        }
        ratio_sm = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
	if (ratio_lr > ratio_sm) index->msMask[0][jj] = 0;
	else  index->msMask[0][jj] = 1;
    if(index->msMask[0][jj] ==1) icounter++;
    }
    if(icounter < jj/4)index->ms_mask = 0; 
    if(icounter > jj-jj/4)index->ms_mask = 2; 
  }
  else{
    int icounter;
    icounter =0;
    if(tvq_debug_level >5)
     fprintf(stderr, "LLLLLLLL last_max_sfb,max_sfb,iscl,%5d %5d %5d\n",
     index->last_max_sfb[iscl+1], index->max_sfb[iscl+1], iscl);
    for(jj=index->last_max_sfb[iscl+1]; jj<index->max_sfb[iscl+1]; jj++){
        lp=rp=0.0;
        for (ismp=sfb_offset[jj]; ismp<sfb_offset[jj+1]; ismp++){
	   lp += (spectral_line_vector[0][ismp]*spectral_line_vector[0][ismp]);
	   rp += (spectral_line_vector[1][ismp]*spectral_line_vector[1][ismp]);
        }
        ratio_lr = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
        for (ismp=sfb_offset[jj]; ismp<sfb_offset[jj+1]; ismp++){
	       bufm[ismp] =  (spectral_line_vector[0][ismp]
			     +spectral_line_vector[1][ismp]);
	       bufs[ismp] =  (spectral_line_vector[0][ismp]
			     -spectral_line_vector[1][ismp]);
        }
        lp=rp=0.0;
        for (ismp=sfb_offset[jj]; ismp<sfb_offset[jj+1]; ismp++){
	   lp += (bufm[ismp]*bufm[ismp]);
	   rp += (bufs[ismp]*bufs[ismp]);
        }
        ratio_sm = ntt_max(lp/(rp+0.1), rp/(lp+0.1));
	if (ratio_lr > ratio_sm) index->msMask[0][jj] = 0;
	else  index->msMask[0][jj] = 1;
    if(tvq_debug_level > 4)
    fprintf(stderr, "MMMMMM %5d %5d %5d \n", jj, sfb_offset[jj], index->msMask[0][jj]);
    
    if(index->msMask[0][jj] ==1) icounter++;
    }
    if(icounter < jj/4)index->ms_mask = 0; 
    if(icounter > jj-jj/4)index->ms_mask = 2; 
    if(tvq_debug_level > 3)
    fprintf(stderr, "MMMMMM %5d %5d \n",  index->ms_mask, jj);
  }
}
	for(ii=0; ii<8; ii++){
	  if(index->ms_mask == 2){
	    for(jj=index->last_max_sfb[iscl+1]; jj<index->max_sfb[iscl+1]; jj++){
              index->msMask[ii][jj]=1;    /* force ms_mask */
	    }
	  }
	/* for index->ms_mask =1  no optimized encoder*/
	  else if(index->ms_mask == 1){
	    for(jj=index->last_max_sfb[iscl+1]; jj<index->max_sfb[iscl+1]; jj++){
              index->msMask[ii][jj]=(jj+ii)%2;    
	    }
	  }
	  else if(index->ms_mask == 0){
	    for(jj=index->last_max_sfb[iscl+1]; jj<index->max_sfb[iscl+1]; jj++){
              index->msMask[ii][jj]=0;   
	    }
	  }
	}
}

void ntt_tns_enc(double *spectral_line_vector[MAX_TIME_CHANNELS],
		 ntt_INDEX  *index, 
	         int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
	         WINDOW_SEQUENCE   windowSequence,
		 int         nr_of_sfb[MAX_TIME_CHANNELS],
		 TNS_INFO	 *tnsInfo[MAX_TIME_CHANNELS]
		)
{

  if(tnsInfo[MONO_CHAN] != NULL) {
    int i_ch;
    int sfb, k, sfb_offset[ 100 ];

    /* calc. sfb offset table */
    for (sfb=k=0; sfb<nr_of_sfb[MONO_CHAN]; sfb++) {
      sfb_offset[sfb] = k;
      k += sfb_width_table[MONO_CHAN][sfb];
    }
    sfb_offset[sfb] = k;

    for (i_ch=0; i_ch<index->numChannel; i_ch++) {
      TnsEncode(nr_of_sfb[MONO_CHAN],        /* Number of bands per w indow */
                nr_of_sfb[MONO_CHAN],        /* max_sfb */
                windowSequence,              /* block type */
                sfb_offset,                  /* Scalefactor band offs t table */
                spectral_line_vector[i_ch],  /* Spectral data array */
                tnsInfo[i_ch]);             
   } 
  }
}


int  tvqScaleableFlexMuxEncode(
  double      *DTimeSigBuf[MAX_TIME_CHANNELS],
  double      *DTimeSigLookAheadBuf[MAX_TIME_CHANNELS],
  double      *spectral_line_vector[MAX_TIME_CHANNELS],
  double      *nok_tmp_DTimeSigBuf[MAX_TIME_CHANNELS],
  double      *baselayer_spectral_line_vector[MAX_TIME_CHANNELS],
  double      *ntt_ms_buf[MAX_TIME_CHANNELS],
  WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS],
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  HANDLE_BSBITSTREAM fixed_stream,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  int         block_size_samples,
  int         sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
  NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS]
  )
{
          int     ismp, tomo_tmp;
          int     used_bits, ntt_available_bits;
	  int     debugLevel;
          double  lpc_spectrum[ntt_T_FR_MAX];
	  double *tvq_target_spectrum[MAX_TIME_CHANNELS];
	  double spectrum_orig[MAX_TIME_CHANNELS][ntt_T_FR_MAX]; /* T.Ishikawa 981215 */
          HANDLE_BSBITSTREAM mat_layerStream;
          HANDLE_BSBITBUFFER mat_dynpartBuf;
	  double baselayer_rec_spectrum[BLOCK_LEN_LONG];
          
	  int i_sup;
	  windowShape[MONO_CHAN]=WS_FHG; /* <---*/
	  mat_dynpartBuf = BsAllocBuffer(10000); /* Adhoc Size */
	  mat_layerStream = BsOpenBufferWrite(mat_dynpartBuf);
          used_bits=0;
          debugLevel=tvq_debug_level;
	  
          BsPutBit(mat_layerStream, windowSequence[MONO_CHAN], 2);
          if(debugLevel>5)
	   fprintf(stderr,"windowSequence %d\n",(int)windowSequence[MONO_CHAN]);
          /*BsPutBit(fixed_stream, windowSequence[MONO_CHAN], 2);*/ /* T.Ishikawa 980727 */
          index->w_type = windowSequence[MONO_CHAN];
          windowSequence[index->numChannel-1] = windowSequence[MONO_CHAN];
	  used_bits += 2;
          BsPutBit(mat_layerStream,  (unsigned long)windowShape[MONO_CHAN], 1);
          /*BsPutBit(fixed_stream,  window_shape, 1);*//* T.Ishikawa 980727 */
          used_bits += 1;
	  index->group_code =0x7f;
          index->last_max_sfb[0] =0;
	  index->restore_flag_tns=0;

	  ntt_maxsfb( index, sfb_width_table, nr_of_sfb, -1);

	  ntt_select_ms( spectral_line_vector,
		        index, sfb_width_table, nr_of_sfb, -1);

	  /* Backup original spectrum */
	  for(i_sup=0;i_sup<index->numChannel;i_sup++)
	    for(ismp=0;ismp<index->block_size_samples;ismp++)
	      spectrum_orig[i_sup][ismp]=spectral_line_vector[i_sup][ismp];
	  
	  if(index->restore_flag_tns==1) {
	     if(tvq_debug_level > 5)
		fprintf(stderr,"Restore spectrum.\n");
	     for(i_sup=0;i_sup<index->numChannel;i_sup++)
	     for(ismp=0;ismp<index->block_size_samples;ismp++)
	     spectral_line_vector[i_sup][ismp]=spectrum_orig[i_sup][ismp];
	  }		  
	  
	  tvq_target_spectrum[0] = spectral_line_vector[0];
	  tvq_target_spectrum[1] = spectral_line_vector[1];
	   if(debugLevel>1) 
	   fprintf(stderr,"ntt_tns_enc END pred_type = %2d %2d\n",
	   pred_type, NOK_LTP);
#if 1 /***TM9902*/
         if(pred_type == NOK_LTP)
           for(i_sup = 0; i_sup < index->numChannel; i_sup++)
           {
             tvq_target_spectrum[i_sup] = baselayer_spectral_line_vector[i_sup];

              for(ismp = 0; ismp < block_size_samples; ismp++){
	       baselayer_spectral_line_vector[i_sup][ismp] = 
	       spectral_line_vector[i_sup][ismp];
	      }
        if(tvq_debug_level >2){
	   fprintf(stderr, "HHHH %12.3f %12.3f \n",
           spectral_line_vector[i_sup][0], spectral_line_vector[i_sup][1]);
            fprintf(stderr, "HHHH %12.3f %12.3f \n",
           nok_tmp_DTimeSigBuf[i_sup][0], nok_tmp_DTimeSigBuf[i_sup][1]);
            fprintf(stderr, "HHHH %5d \n", windowSequence[i_sup]);
            fprintf(stderr, "HHHH %5d \n", windowShape[MONO_CHAN]);
            fprintf(stderr, "HHHH %5d \n", block_size_samples);
            fprintf(stderr, "HHHH %5d \n", sfb_offset[i_sup][0]);
            fprintf(stderr, "HHHH %5d \n", nr_of_sfb[i_sup]);
	 }
	   nok_ltp_enc(spectral_line_vector[i_sup],
                         nok_tmp_DTimeSigBuf[i_sup],
                         windowSequence[i_sup],
                         windowShape[MONO_CHAN], windowShape[MONO_CHAN],
                         block_size_samples,
                         block_size_samples/2,
                         block_size_samples/8,
                         &sfb_offset[i_sup][0], nr_of_sfb[i_sup],
                         &nok_lt_status[i_sup],
                         tnsInfo[i_sup],
                         NTT_VQ_SYS);
           }
#endif
	if(debugLevel>5) 
           fprintf(stderr, "MSMSMS %5d \n", index->ms_mask);
	ntt_headerenc( -1, mat_layerStream, index, &used_bits,
			 nr_of_sfb, tnsInfo, nok_lt_status, pred_type);
	if(debugLevel>2)  fprintf(stderr,"headerenc %d\n",used_bits);
	  ntt_available_bits = index->nttDataBase->ntt_NBITS_FR - used_bits;
	  
	  ntt_vq_coder(DTimeSigBuf,
                       DTimeSigLookAheadBuf,
                       spectral_line_vector,
                       lpc_spectrum /*ntt_external_pw[MONO_CHAN]*/,
                       NTT_PW_INTERNAL /*ntt_pw_select*/,
                       windowSequence[MONO_CHAN],
                       index,
                       param_ntt,
                       sfb_width_table,
		       nr_of_sfb,
		       ntt_available_bits,
                       lpc_spectrum,
                       reconstructed_spectrum);
	  
          /* Bit packing */

	if(tvq_debug_level>5){
	   fprintf(stderr," before pack%5ld\n",BsCurrentBit(mat_layerStream));
	   fprintf(stderr," before pack%5ld\n",BsCurrentBit(fixed_stream));
	}
	tomo_tmp=used_bits;
          used_bits += ntt_BitPack(index, mat_layerStream);
	  if (debugLevel>2) 
	  fprintf(stderr,"____ ntt_BaseLayer bit %d %d\n",used_bits-tomo_tmp,used_bits);
		  
          if(tvq_debug_level >5 )
		fprintf(stderr,"  after pack%5ld\n",BsCurrentBit(mat_layerStream));
		  BsClose(mat_layerStream);
		  tomo_tmp=BsCurrentBit(fixed_stream);
		  writeFlexMuxPDU( 0 , fixed_stream , mat_dynpartBuf);
	   if (debugLevel>2)
	   fprintf(stderr,"< Base %ld >\n",BsCurrentBit(fixed_stream)-tomo_tmp);

          /* 
	   * Restore the spectrum of the base layer and update LTP buffer. 
	   */
	   if(pred_type == NOK_LTP)
	   {
	     
	     for(i_sup = 0; i_sup < index->numChannel; i_sup++)
	     {   
	       for(ismp = 0; ismp < block_size_samples; ismp++)
		 baselayer_rec_spectrum[ismp] = 
		 reconstructed_spectrum[i_sup][ismp];
	       
	       /* Add the LTP contribution to the reconstructed spectrum. */
	       nok_ltp_reconstruct(baselayer_rec_spectrum, 
				   windowSequence[i_sup], 
				   &sfb_offset[i_sup][0], nr_of_sfb[i_sup],
				   block_size_samples/8,
				   &nok_lt_status[i_sup]);
	       
	       /* This is passed to the scaleable encoder. */
	       for(ismp = 0; ismp < block_size_samples; ismp++)
		 reconstructed_spectrum[i_sup][ismp] = 
		 baselayer_rec_spectrum[ismp];
#if 1	       
	       /* TNS synthesis filtering. */
	       if(tnsInfo[i_sup] != NULL && (index->restore_flag_tns==0))
		 TnsEncode2(nr_of_sfb[i_sup], nr_of_sfb[i_sup], 
			    windowSequence[i_sup], &sfb_offset[i_sup][0], 
			    baselayer_rec_spectrum, tnsInfo[i_sup] , 1);
#endif
	       /* Update the time buffer of LTP. */
	       nok_ltp_update(baselayer_rec_spectrum, nok_lt_status[i_sup].overlap_buffer, 
			      windowSequence[i_sup], windowShape[MONO_CHAN], 
			      windowShape[MONO_CHAN], block_size_samples, 
			      block_size_samples/2, block_size_samples/8, 8,
			      &nok_lt_status[i_sup]);
	     }
	  }
	  
	  /* Scalable encoder */
          {
	    int iscl;
	    int  iii, jjj;
	    for(iii=0; iii<8; iii++){
	       for(jjj=0; jjj<index->max_sfb[0]; jjj++){
	         index_scl->msMask[iii][jjj]=index->msMask[iii][jjj];
	       }
	    }
	    index_scl->max_sfb[0] = index->max_sfb[0];
            index_scl->w_type = index->w_type;
            index_scl->group_code = index->group_code;
	    index_scl->pf = index->pf;
            if(tvq_debug_level >5)
	      fprintf(stderr, "LLL %5d ntt_NSclLay \n", index->nttDataScl->ntt_NSclLay); 
	   for (iscl=0; iscl<index->nttDataScl->ntt_NSclLay; iscl++){
#if 1 /*new*/
             tvqScalEnc(
                 tvq_target_spectrum,
                 index,
                 index_scl,
                 param_ntt,
                 fixed_stream,
                 tnsInfo,
                 sfb_width_table,
                 nr_of_sfb,
                 reconstructed_spectrum,
		 pred_type,
		 iscl );

#endif
	      }
          }
          return(used_bits);	 
}

void tvqScalEnc(
  double      *tvq_target_spectrum[MAX_TIME_CHANNELS],
  ntt_INDEX   *index,
  ntt_INDEX   *index_scl,
  ntt_PARAM   *param_ntt,
  HANDLE_BSBITSTREAM fixed_stream,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
  PRED_TYPE   pred_type,
  int         iscl
)

{
          double  lpc_spectrum[ntt_T_FR_MAX]; /*dummy*/
          HANDLE_BSBITSTREAM mat_layerStream;
          HANDLE_BSBITBUFFER mat_dynpartBuf;
          NOK_LT_PRED_STATUS /* dummy for enhance */
	       nok_lt_status[MAX_TIME_CHANNELS];
         int ntt_available_bits;
	 int ismp, tomo_tmp, tomo_tmp2, used_bits;
	 int     debugLevel;
         double  *ntt_ms_buf[MAX_TIME_CHANNELS];
         double  buffer[MAX_TIME_CHANNELS][1024];
	 /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
         int vq_bits;
#endif

	   /*
           nok_lt_status[0]=NULL;
	   nok_lt_status[1]=NULL;
           */

	   debugLevel=tvq_debug_level;
	    index_scl->last_max_sfb[iscl+1] = index_scl->max_sfb[iscl];
				mat_dynpartBuf = BsAllocBuffer(10000); /* Adhoc Size */
				mat_layerStream = BsOpenBufferWrite(mat_dynpartBuf);

	      used_bits=0;
	      index_scl->ms_mask =0;
	      if(index->numChannel > 1){
	        for(ismp=0; ismp<index->block_size_samples; ismp++){
		  ntt_ms_buf[0] = &(buffer[0][0]);
		  ntt_ms_buf[1] = &(buffer[1][0]);
		  ntt_ms_buf[0][ismp] = 
		    tvq_target_spectrum[0][ismp] - reconstructed_spectrum[0][ismp];
		  
		  ntt_ms_buf[1][ismp] = 
		    tvq_target_spectrum[1][ismp] - reconstructed_spectrum[1][ismp];
		}
	      }
              ntt_maxsfb( index_scl, sfb_width_table, nr_of_sfb, iscl);
              ntt_select_ms( ntt_ms_buf,
		        index_scl, sfb_width_table, nr_of_sfb, iscl);
	      
	      ntt_headerenc( iscl, 
			     mat_layerStream, index_scl, &used_bits,
			     nr_of_sfb, tnsInfo, nok_lt_status, 
			     pred_type);
	      ntt_available_bits = 
		 index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]-used_bits;
              ntt_scale_vq_coder(tvq_target_spectrum, /*spectral_line_vector,*/
                                 lpc_spectrum,
                                 index,
                                 index_scl,
                                 param_ntt,
                                 sfb_width_table,
		                 nr_of_sfb,
                                 ntt_available_bits,
				 reconstructed_spectrum, iscl);

              /* Scalable bit packing */
				tomo_tmp=used_bits;
				tomo_tmp2=BsCurrentBit(mat_layerStream);
              used_bits += ntt_SclBitPack(index_scl, mat_layerStream , iscl);
		if(debugLevel>2)
		fprintf(stderr,"____ ntt_SclLayer bit %d %d [%d]\n",used_bits-tomo_tmp,used_bits,iscl);
		if(debugLevel>2)
		fprintf(stderr,"<<< mat_Stream[%d] %ld\n",iscl,BsCurrentBit(mat_layerStream)-tomo_tmp2);
				
				BsClose(mat_layerStream);
				tomo_tmp=BsCurrentBit(fixed_stream);
				/* added by NTT 20001121 */
#ifdef ESC_SUPPORT

                                if(index_scl->er_tvq==1){
				  if (index_scl->nttDataScl->epFlag==1){
				    /* epFlag == 1 */
				    if(index_scl->w_type==EIGHT_SHORT_SEQUENCE)
				      vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
				    else
				      vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
				    
				    ntt_writeFlexMuxPDU( iscl+1 , fixed_stream , vq_bits, mat_dynpartBuf, ((index->nttDataBase->epFlag==1)?2:1));
				  }else
				    writeFlexMuxPDU( iscl+1+((index->nttDataBase->epFlag)?1:0) , fixed_stream , mat_dynpartBuf);

                                }else{

                                writeFlexMuxPDU( iscl+1 , fixed_stream , mat_dynpartBuf);
                                }                                 

#else
                                writeFlexMuxPDU( iscl+1 , fixed_stream , mat_dynpartBuf);

#endif

}


void ntt_maxsfb(    ntt_INDEX   *index,
		    int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
                    int     nr_of_sfb[MAX_TIME_CHANNELS],
                    int     iscl
		    )
{
 int max_line, sb, sb_ind, k, i;
 int sfb_offset[MAX_SCFAC_BANDS];
 float ftmp;
        sb_ind=0;
        if(iscl <0){ /*base*/
           if(tvq_debug_level>5)
	      fprintf(stderr, "%12.2f %5d DDDD base \n",
	      index->nttDataBase->bandUpper, iscl);
	   ftmp = (float)(index->nttDataBase->bandUpper);
	   if (index->w_type==EIGHT_SHORT_SEQUENCE){
	      ftmp *= (float)(index->block_size_samples/8);
              max_line = (int)ftmp; 
	   }
	   else{
	      ftmp *= (float)(index->block_size_samples);
	      max_line = (int)ftmp;
           }
       }
       else{
           if(tvq_debug_level>5)
           fprintf(stderr, "%12.2f %5d DDDD scl\n",
	      index->nttDataScl->ac_top[iscl][0][3], iscl);
	   ftmp = (float)(index->nttDataScl->ac_top[iscl][0][3]);
	   if (index->w_type==EIGHT_SHORT_SEQUENCE){
	      ftmp *= (float)(index->block_size_samples/8);
              max_line = (int)ftmp; 
	   }
	   else{
	      ftmp *= (float)(index->block_size_samples);
	      max_line = (int)ftmp;
           }
       }   
	   sfb_offset[0] = 0;
           k=0;
           for(i = 0; i < nr_of_sfb[MONO_CHAN]; i++ ) {
               sfb_offset[i] = k;
               k +=sfb_width_table[MONO_CHAN][i];
           }
           sfb_offset[i] = k;
           for(sb=0; (sfb_offset[sb]<max_line); sb++){
             sb_ind = sb+1;
            }
            index->max_sfb[iscl+1] = sb_ind;
}

