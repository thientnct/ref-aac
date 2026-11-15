/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by ...

 $Id: decoder_sls.c,v 1.1.2.1 2007/01/17 21:27:54 haertlml Exp $
 SLS Decoder module
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include "allHandles.h"
#include "tf_mainHandle.h"
#include "block.h"
#include "buffer.h"
#include "coupling.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */

#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif

#include "aac.h"
#include "dec_tf.h"
#include "sls_int.h"
#include "lit_ll_dec.h"


#include "dolby_def.h"
#include "tf_main.h"
#include "common_m4a.h"
#include "nok_lt_prediction.h"
#include "port.h"
#include "buffers.h"
#include "bitstream.h"

#include "resilience.h"
#include "concealment.h"

#include "allVariables.h"
#include "obj_descr.h"

/* son_AACpp */
#include "sony_local.h"

#ifdef DRC
#include "drc.h"
#endif


/*
---     Global variables from aac-decoder      ---
*/

int debug [256] ;
static int* last_rstgrp_num = NULL;

/* -------------------------------------- */
/*       module-private functions         */
/* -------------------------------------- */

#if 0
static void predinit (HANDLE_AACDECODER hDec)
{
  int i, ch;

  if (hDec->pred_type == MONOPRED)
  {
    for (ch = 0; ch < Chans; ch++)
    {
      for (i = 0; i < LN2; i++)
      {
        init_pred_stat(&hDec->sp_status[ch][i],
                       PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
      }
    }
  }
}
#endif


#if 0

/* -------------------------------------- */
/*           Init SLS-Decoder             */
/* -------------------------------------- */
/* todo */
HANDLE_SLSDECODER SLSDecodeInit (int samplingRate,
                                 char *aacDebugStr, 
                                 int  block_size_samples,
                                 Info *** sfbInfo, 
                                 int  predictor_type,
                                 int  profile,
                                 const char *decPara)
{
  int i,band;

  
  HANDLE_AACDECODER hDec = (HANDLE_AACDECODER) calloc (1,  sizeof(T_AACDECODER));
  if (!hDec) return hDec ;

  hDec->blockSize= block_size_samples;
  hDec->pred_type = (PRED_TYPE)predictor_type;
  hDec->mip = &mc_info;
  hDec->short_win_in_long = 8;


#ifdef CT_SBR

#ifndef SBR_SCALABLE
  hDec->ct_sbrBitStr.NrElements     =  0;
  hDec->ct_sbrBitStr.NrElementsCore =  0;
#else
  hDec->ct_sbrBitStr[0].NrElements     =  0;
  hDec->ct_sbrBitStr[0].NrElementsCore =  0;
  hDec->ct_sbrBitStr[1].NrElements     =  0;
  hDec->ct_sbrBitStr[1].NrElementsCore =  0;
#endif
#endif



  for(i=0; i<Chans; i++)
  {
    hDec->coef[i] = (double *)mal1(LN2*sizeof(*hDec->coef[0]));
    hDec->data[i] = (double *)mal1(LN2*sizeof(*hDec->data[0]));
    hDec->factors[i] = (short *)mal1(MAXBANDS*sizeof(*hDec->factors[0]));
    hDec->state[i] = (double *)mal1(LN*sizeof(*hDec->state[0]));	/* changed LN4 to LN 1/97 mfd */
    fltclr(hDec->state[i], LN); 
    hDec->cb_map[i] = (byte *)mal1(MAXBANDS*sizeof(*hDec->cb_map[0]));
    hDec->group[i] = (byte *)mal1(NSHORT*sizeof(*hDec->group[0]));
    hDec->lpflag[i] = (int *)mal1(MAXBANDS*sizeof(*hDec->lpflag[0]));
    hDec->prstflag[i] = (int *)mal1((LEN_PRED_RSTGRP+1)*sizeof(*hDec->prstflag[0]));
    hDec->tns[i] = (TNS_frame_info *)mal1(sizeof(*hDec->tns[0]));
    hDec->wnd_shape[i].prev_bk = WS_FHG;
    hDec->wnd_shape_SSR[i].prev_bk= WS_FHG;

    if (predictor_type == MONOPRED)
    {
      hDec->sp_status[i] = (PRED_STATUS *)mal1(LN2*sizeof(*hDec->sp_status[0]));
    }
  }

  for(i=0; i<Winds; i++)
  {
    hDec->mask[i] = (byte *)mal1(MAXBANDS*sizeof(hDec->mask[0]));
  }

  /* coupling channels */

#if (CChans > 0)
  for(i=0; i<CChans; i++)
    {
      int j,k;
      hDec->cc_coef[i]= (double *)mal1(LN2*sizeof(*hDec->cc_coef[0]));
      for(j=0; j<Chans; j++)
	      hDec->cc_gain[i][j] = (double *)mal1(MAXBANDS*sizeof(*hDec->cc_gain[0][0]));
#if (ICChans > 0)
      if (i < ICChans)
        {
          hDec->cc_state[i] = (double *)mal1(LN*sizeof(*hDec->cc_state[0]));
          fltclr(hDec->cc_state[i], LN);
          hDec->cc_wnd_shape[i].prev_bk = 0;
        }
#endif
    }
#endif
  
  /*
      SSR Profile
  */
  for (i=0; i<Chans; i++){ 
    hDec->spectral_line_vector_for_gc[i] = (double *)calloc(block_size_samples,sizeof(*hDec->spectral_line_vector_for_gc[0]));
    hDec->imdctBufForGC[i] = (double *)calloc(block_size_samples*2, sizeof(*hDec->imdctBufForGC[0]));
    hDec->DBandSigBufForOverlapping[i] = (double **)calloc(NBANDS, sizeof(double *));
    hDec->DBandSigBuf[i] = (double **)calloc(NBANDS, sizeof(double *));
    hDec->gainInfo[i] = (GAINC **)calloc(NBANDS, sizeof(GAINC *));
    for (band = 0; band < NBANDS; band++) {
      hDec->DBandSigBufForOverlapping[i][band] =
        (double *)calloc(block_size_samples/NBANDS*2, sizeof(*hDec->DBandSigBufForOverlapping[i][0]));
      hDec->DBandSigBuf[i][band] =
        (double *)calloc(block_size_samples/NBANDS, sizeof(*hDec->DBandSigBuf[i][0]));
      hDec->gainInfo[i][band] = (GAINC *)calloc(hDec->short_win_in_long, sizeof(GAINC));
    }
  } 
  

  /* set defaults */

  current_program = -1;

  /* multi-channel info is invalid so far */

  mc_info.mcInfoCorrectFlag = 0;
  mc_info.profile = profile ;
  mc_info.sampling_rate_idx = Fs_48;

  /* reset all debug options */

  for (i=0;i<256;i++)
    debug[i]=0;

  if (aacDebugStr)
  {
    for (i=0;i<(int)strlen(aacDebugStr);i++){
      debug[(int)aacDebugStr[i]]=1;
      fprintf(stderr,"   !!! AAC debug option: %c recognized.\n", aacDebugStr[i]);
    }
  }
  
  /* Set sampling rate */

  for (i=0; i<(1<<LEN_SAMP_IDX); i++) {
    if (sampling_boundaries[i] <= samplingRate)
      break;
  }
  
  if (i == (1<<LEN_SAMP_IDX)) 
    CommonExit(1,"Unsupported sampling frequency %d", samplingRate);
  
  prog_config.sampling_rate_idx = mc_info.sampling_rate_idx = i;

#if (CChans > 0)
  init_cc(hDec->lpflag[CChans]); 
#endif
  
  for(i = 0; i < Chans; i++)
    {
      hDec->wnd_shape_SSR[i].prev_bk = 0;
      hDec->wnd_shape[i].prev_bk = 0;
      hDec->wnd_shape[i].this_bk = 0;
    }
  

  huffbookinit(block_size_samples, mc_info.sampling_rate_idx);
  predinit(hDec);

  winmap[0] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[1] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[2] = win_seq_info[EIGHT_SHORT_SEQUENCE];
  winmap[3] = win_seq_info[ONLY_LONG_SEQUENCE];

  *sfbInfo = winmap;

#ifdef DRC
  hDec->drc = drc_init();
  {
    double p1=0.0, p2=0.0;
    int p3=-1;
    const char *tmp = strstr(decPara, "drc ");
    if (tmp != NULL) {
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of "drc" */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if (*tmp!=0) p1 = atof(tmp);
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of first parameter */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if (*tmp!=0) p2 = atof(tmp);
      while ((*tmp!=' ')&&(*tmp!=0)) tmp++; /* go to end of second parameter */
      while ((*tmp==' ')&&(*tmp!=0)) tmp++; /* go to next token */
      if ((*tmp!=0)&&(*tmp!='-')) p3 = atoi(tmp);
    }
    drc_set_params(hDec->drc, p1, p2, p3);
  }
#endif

  return hDec ;
}

/* ------------------------------------------ */
/*  Release data allocated by SLSDecoderInit  */
/* ------------------------------------------ */

/* todo */
void SLSDecodeFree (HANDLE_SLSDECODER hDec)
{
  int i ;

  for(i=0; i<Chans; i++)
  {
    int band;
    free (hDec->state[i]);
    free (hDec->coef[i]) ;
    free (hDec->data[i]) ;
    free (hDec->factors[i]) ;
    free (hDec->cb_map[i]) ;
    free (hDec->group[i]) ;
    free (hDec->lpflag[i]) ;
    free (hDec->prstflag[i]) ;
    free (hDec->tns[i]) ;

    for (band = 0; band < NBANDS; band++) {
      free(hDec->DBandSigBufForOverlapping[i][band]);
      free(hDec->DBandSigBuf[i][band]);
      free(hDec->gainInfo[i][band]);
    }

    free(hDec->spectral_line_vector_for_gc[i]);
    free(hDec->imdctBufForGC[i]);
    free(hDec->DBandSigBufForOverlapping[i]);
    free(hDec->DBandSigBuf[i]);
    free(hDec->gainInfo[i]);


    if (hDec->pred_type == MONOPRED)
    {
      free (hDec->sp_status[i]) ;
    }
  }

  if (NULL != last_rstgrp_num) {
    free (last_rstgrp_num);
  }
  for(i=0; i<Winds; i++)
    free (hDec->mask[i]) ;

#if (CChans > 0)
  for(i=0; i<CChans; i++)
  {
    int j,k;
    free (hDec->cc_coef[i]) ;
    for(j=0; j<Chans; j++)
	    free (hDec->cc_gain[i][j]) ;
#if (ICChans > 0)
    if (i < ICChans)
    {
      free (hDec->cc_state[i]) ;
    }
#endif
  }
#endif

#ifdef DRC
  drc_free (hDec->drc);
#endif

  free (hDec) ;
}
#endif


#if 1
/* -------------------------------------- */
/*           DECODE 1 SLS frame           */
/* -------------------------------------- */
 
int SLSDecodeFrame ( HANDLE_AACDECODER        hDec,
                     TF_DATA*                 tfData,
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     /* Info**                   sfbInfo,  */
                     int                      msMask[8][60],
                     FRAME_DATA*              fd ,
                     HANDLE_DECODER_GENERAL   hFault,
                     int                      layer
                     )
{
  MC_Info *mip = hDec->mip ;
  Ch_Info *cip ;

  byte hasmask [Winds] ;

  int           left, right, ele_id, wn, ch ;
  int i;
  int           chCnt;
  int           outCh;
  int           sfb;
  Info*         sfbInfoP;
  Info          sfbInfoNScale;
  unsigned char nrOfSignificantElements = 0; /* SCE (mono) and CPE (stereo) only */
  HANDLE_BSBITSTREAM  layer_stream = NULL;

  HANDLE_RESILIENCE         hResilience      = hFault[layer].hResilience;
  HANDLE_ESC_INSTANCE_DATA  hEscInstanceData = hFault[layer].hEscInstanceData;
  HANDLE_BUFFER             hVm              = hFault[layer].hVm;


  static const unsigned long samplingRate [] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
  };
  
#if 0
  double pcm_scaling[4] = {1.0, 1.0/16, 1.0/256, 1.0/65536}; /* 16,20,24,32 bit PCM */   /* bug */
#else
  double pcm_scaling[4] = {256.0, 1.0, 1.0/16, 1.0/256, }; /* 8,16,20,24 bit PCM */   /* standard */
#endif
  
  short block_type_lossless_only = ONLY_LONG_SEQUENCE;
  int msStereoIntMDCT = 1;
  int coredisable = 0;


  int samplFreqIdx = mip->sampling_rate_idx;
  /*(fd==NULL) ? -1 : (int)fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value*/
  

  if(debug['n']) {
    /*     fprintf(stderr, "\rblock %ld", bno); */
    fprintf(stderr,"\n-------\nBlock: %ld\n", bno);
  }

  
  if (fd->layer[layer].NoAUInBuffer<=0) 
    CommonExit(-1,"AAZ layer error: NoAUInBuffer<=0"); ;
  
  if ( layer_stream == NULL ) {
    layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 
    setHuffdec2BitBuffer ( layer_stream );
  }	

  reset_mc_info(mip);

  /***** LIT_ll_read() *****/
  {
    for (ch=0;ch<numChannels;ch++)
      {
        tfData->ll_info[ch]->stream_len = GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm );
        tfData->ll_info[ch]->stream_len += GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm )<<8;
        for (i = 0; i<tfData->ll_info[ch]->stream_len;i++)
          tfData->ll_info[ch]->stream[i] = GetBits( 8, SLS_BITS, hResilience, hEscInstanceData, hVm );
      }
  }

  /**************************/
  
  removeAU( layer_stream,
            BsCurrentBit((HANDLE_BSBITSTREAM)layer_stream),
            fd,
            layer);
  
  BsCloseRemove ( layer_stream,1 );
  layer_stream = NULL;

  /* xxxxxxxxx */
  
  check_mc_info ( mip, 
                  0,
                  hEscInstanceData,
                  hResilience
                  );

  /* xxxxxxxxx */


  /********* LLE Info extracted *******/

  for (ch=0;ch<numChannels;ch++)
    {  
      LIT_ll_Dec(tfData->ll_info[ch],
                 tfData->ll_quantInfo[ch],
                 msMask[0],                  /* ms mask of layer 0 ? */
                 1 /*hasmask[wn]*/,
                 /* cb_map, */
                 tfData->ll_coef[ch],
                 &tfData->aacDecoder->mip->ch_info[ch+1],/* cip, *//*->ch_is_left,*/
                 samplingRate[tfData->aacDecoder->mip->sampling_rate_idx],
                 tfData->/* aacDecoder->mip-> */osf, 
                 tfData->aacDecoder->mip->type_PCM,
                 &block_type_lossless_only,
                 &msStereoIntMDCT,
                 !coredisable,
                 ch,
                 layer -1);   /* temp hack: was lastAacLay, now: only one sls layer supported */
    }

#if 1
  for (ch=0; ch<numChannels; ch++) {
    Ch_Info *cip = &tfData->aacDecoder->mip->ch_info[ch];
    int wn, left, right;
    if (!(cip->present)) continue;
    wn = cip->widx;  
    if (cip->cpe && cip->common_window && msStereoIntMDCT) {
      /* StereoIntMDCT */
      if (cip->ch_is_left) {
        left = ch - 1;                  /*temp hack*/
        right = cip->paired_ch - 1;		/*temp hack*/
#if 0 
        /* IntTNS */
        {
          {
            int i;
            /* Int M/S */
            /* do inverse M/S to provide L/R spectrum for TNS */
            for (i=0; i<mip->osf*1024; i++) {
              InverseIntMidSideINT32(&ll_coef[left][i],
                                     &ll_coef[right][i]);
            }
          }
          {
            int i,j;
            
            for (i=j=0; i<tns[left]->n_subblocks; i++) {
              if (debug['T']) {
                PRINT(SE, "%ld %d %d\n", bno, left, i);
                print_tns( &(tns[left]->info[i]));
                PRINT(SE, "%ld %d %d\n", bno, right, i);
                print_tns( &(tns[right]->info[i]));
              }    
              int_tns_decode_subblock(&ll_coef[left][j],
                                      max_sfb[wn],
                                      info->sbk_sfb_top[i],
                                      info->islong,
                                      &(tns[left]->info[i]) );  
              int_tns_decode_subblock(&ll_coef[right][j],
                                      max_sfb[wn],
                                      info->sbk_sfb_top[i],
                                      info->islong,
                                      &(tns[right]->info[i]) );  
              j += info->bins_per_sbk[i];
            }
          } 
          {
            int i;
            /* Int M/S */
            /* do forward M/S to revert to M/S spectrum after TNS */
            for (i=0; i<mip->osf*1024; i++) {
              IntMidSideINT32(&ll_coef[left][i],
                              &ll_coef[right][i]);
            }
          }
        }
#endif /* IntTNS */
        
        if (block_type_lossless_only != 0) {
          InvStereoIntMDCT(tfData->ll_coef[left],
                           tfData->ll_coef[right],
                           tfData->mdct_buf[left],
                           tfData->mdct_buf[right],
                           tfData->ll_timesig[left],
                           tfData->ll_timesig[right],
                           block_type_lossless_only,
                           tfData->prev_windowShape[wn], /* wnd_shape[wn].this_bk, */
                           tfData/* ->aacDecoder->mip */->osf
                           );
        } else {
          
          InvStereoIntMDCT((int*)tfData->ll_coef[left],
                           (int*)tfData->ll_coef[right],
                           (int*)tfData->mdct_buf[left],
                           (int*)tfData->mdct_buf[right],
                           (int*)tfData->ll_timesig[left],
                           (int*)tfData->ll_timesig[right],
                           (byte)tfData->windowShape[wn], /* wnd[wn], */
                           (byte)tfData->prev_windowShape[wn], /* wnd_shape[wn].this_bk, */
                           tfData->/* aacDecoder->mip-> */osf 
                           );
        }
      }
    }
  }
  
  for (ch=0; ch<numChannels; ch++) {
    int ii;
    
    if (!(&tfData->aacDecoder->mip->ch_info[ch+1].present)) continue;
    
    for (ii = 0;ii<tfData->/* aacDecoder->mip-> */osf*1024;ii++)
      tfData->time_sample_vector[ch][ii] = tfData->ll_timesig[ch][ii] * pcm_scaling[tfData->aacDecoder->mip->type_PCM];
  }
  
#endif






  /* Copy the coeff and pass them back to VM */
  
  outCh=0;
    
  for (ch=0; ch<Chans; ch++)
    {
      int i,j;
      
      if ((mip->ch_info[ch].present))
        {
          if ((outCh+1)> numChannels){
            CommonExit(-1,"wrong number of channels in command line");
          }
          for (i=0;i<hDec->blockSize;i++)
            {
              spectral_line_vector[outCh][i] = hDec->coef[ch][i];
	    }
#ifdef I2R_LOSSLESS
          if (outCh!=ch)
            memcpy(hDec->sls_quant_channel_temp[outCh], hDec->sls_quant_channel_temp[ch],1024*sizeof(int));
#endif

          outCh++;
        }
    }

  /* not necessary, we (=sls) should be the highest layer (??) */
#if 0
  /* pass scalefactors to calling function */
  if ((bitStreamType == SCALABLE ) || ( bitStreamType == SCALABLE_ER )) {
    if( pFactors!=NULL ) {
      if( numChannels == 1 ) {
        memcpy( pFactors[0], hDec->factors[0], sizeof(*hDec->factors[0]) * MAXBANDS);
        }
      else {
        memcpy( pFactors[0], hDec->factors[1], sizeof(*hDec->factors[1]) * MAXBANDS);
        memcpy( pFactors[1], hDec->factors[2], sizeof(*hDec->factors[2]) * MAXBANDS);
       }
    }
  }
#endif
  
  bno++;
  
  return GetReadBitCnt ( hVm );
  
}
#endif

#if 0
void decodeBlockType ( int coded_types, 
                       int granules, 
                       int gran_block_type[], 
                       int act_gran )
{
  static int ty_seq[4][2]={{0,1},{2,3},{2,3},{0,1}};
  register int tmp;
  register int i;
  
  if( act_gran < 0 ) {
    gran_block_type[0] = (coded_types >> (granules - 1)) & 0x03;
  } else if( act_gran == 0 ) {
    gran_block_type[0] = coded_types & 0x03;
  }
  for( i=1; i<granules; i++ ) {
    if( act_gran < 0 ) {
      tmp = ((coded_types >> (granules - 1 - i)) & 0x01);
      gran_block_type[i] = ty_seq[gran_block_type[i-1]][tmp];
    } else if( act_gran == i ) {
      tmp = coded_types  & 0x01;
      gran_block_type[i] = ty_seq[gran_block_type[i-1]][tmp];
    }
  }
}
#endif


static void set_mip_preset(MC_Info *mip, int channels)
{
  int ch;

  mip->nch = channels;
  mip->profile = 1;
  /* mono */
  if (mip->nch == 1) {
    mip->ch_info[0].present = 1;
  }
  /* stereo */
  if (mip->nch == 2) {
    for (ch=1;ch<=2;ch++){
      mip->ch_info[ch].present = 1;
      mip->ch_info[ch].cpe = 1;
      mip->ch_info[ch].common_window = 1;
      mip->ch_info[ch].widx = 1;
      if (ch == 1){
        mip->ch_info[ch].ch_is_left = 1;
        mip->ch_info[ch].paired_ch = 2;
      }
      else {
        mip->ch_info[ch].ch_is_left = 0;
        mip->ch_info[ch].paired_ch = 1;
      }
    }
  }
}


void slsAUDecode ( int numChannels,
                   FRAME_DATA* frameData,
                   TF_DATA* tfData,
                   HANDLE_DECODER_GENERAL hFault,
                   enum AAC_BIT_STREAM_TYPE bitStreamType)
{
  int i_ch, decoded_bits = 0;
  short* pFactors[MAX_TIME_CHANNELS]; 
  int i;
    
  HANDLE_BSBITSTREAM fixed_stream;
  HANDLE_AACDECODER  hDec = tfData->aacDecoder; 
  /* son_AACpp */
  HANDLE_BSBITSTREAM  gc_stream[MAX_TIME_CHANNELS];
  HANDLE_BSBITSTREAM  gc_WRstream[MAX_TIME_CHANNELS];
  HANDLE_BSBITBUFFER  gcBitBuf[MAX_TIME_CHANNELS];     /* bit buffer for gain_control_data() */
  
  int short_win_in_long = 8;
  
  byte max_sfb[Winds];
  byte dummy[MAX_TIME_CHANNELS][MAXBANDS];
  QC_MOD_SELECT qc_select;

  switch (frameData->scalOutObjectType) {
#if 0
  case ER_AAC_LD:
#ifdef AAC_ELD
  case ER_AAC_ELD:
#endif
    qc_select = AAC_LD;
    break;
#endif
  default:
    qc_select = AAC_QC;
    break;
  }


  for (i=0; i<MAX_TIME_CHANNELS; i++) {
    pFactors[i] = (short *)mal1(MAXBANDS*sizeof(*pFactors[0]));
  }
  
  for ( i_ch = 0 ; i_ch < numChannels ; i_ch++ ) {
    if ( ! hFault->hConcealment || ! ConcealmentAvailable ( 0, hFault->hConcealment ) ) {
      tfData->windowSequence[i_ch] = ONLY_LONG_SEQUENCE;
    }
  }

  fixed_stream = BsOpenBufferRead (frameData->layer[0].bitBuf);


  /* buffers for SSR  */
#if 0
  for (i_ch = 0 ; i_ch < numChannels ; i_ch++)
  {
    gcBitBuf[i_ch] = BsAllocBuffer(4096);
    gc_WRstream[i_ch] = BsOpenBufferWrite(gcBitBuf[i_ch]);
    gc_stream[i_ch] = BsOpenBufferRead(gcBitBuf[i_ch]);
  } /* for i_ch...*/
#endif

  /* run dummy aac core to fill spectrum */

#if 0
  decoded_bits += AACDecodeFrame ( hDec,
                                   fixed_stream,
                                   gc_WRstream,
                                   tfData->spectral_line_vector[0], 
                                   tfData->windowSequence, 
                                   tfData->windowShape,
                                   bitStreamType,
                                   max_sfb,
                                   numChannels,
                                   0,/*commonWindow*/
                                   tfData->sfbInfo,   
                                   dummy,/*sfbCbMap*/
                                   pFactors, /*pFactors */
                                   hFault,
                                   qc_select,
                                   tfData->nok_lt_status,
                                   frameData, 
                                   0,  
                                   0,  
                                   0
                                   );
#else
  /* lossless only: fill channel structure */
  set_mip_preset(hDec->mip, numChannels);
#endif
  
  removeAU(fixed_stream,decoded_bits,frameData,0);
  BsCloseRemove(fixed_stream,1);

#if 0  /* for ssr */
  for (i_ch=0; i_ch<numChannels; i_ch++){
    BsCloseRemove(gc_stream[i_ch],1);
    BsCloseRemove(gc_WRstream[i_ch],1);
    BsFreeBuffer(gcBitBuf[i_ch]);
  }/* for(i_ch...*/
#endif
}


#if 0
/* Module Functions for TNSDATA */


TNS_frame_info GetTnsData( void *paacDec, int index )
{
  return *(( (T_AACDECODER *) paacDec )->tns)[index];
}

int GetTnsDataPresent( void *paacDec, int index )
{
  return ( (T_AACDECODER *) paacDec )->tns_data_present[index];
}

void SetTnsDataPresent( void *paacDec, int index, int value )
{
  ( (T_AACDECODER *) paacDec )->tns_data_present[index] = value;
}

int GetTnsExtPresent( void *paacDec, int index )
{
  return ( (T_AACDECODER *) paacDec )->tns_ext_present[index];
}

void SetTnsExtPresent( void *paacDec, int index, int value )
{
 ( (T_AACDECODER *) paacDec )-> tns_ext_present[index] = value;
}


/* Module Functions for BLOCKSIZE */


int GetBlockSize( void *paacDec )
{
  return ( (T_AACDECODER *) paacDec )->blockSize;
}
#endif
