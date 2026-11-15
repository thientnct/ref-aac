/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 

and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996, 1997, 1998.
 *                                                                           *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "tf_mainHandle.h"
#include "block.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "aac.h"

#include "lpc_common.h"

#include "dec_tf.h"
#include "dolby_def.h"
#include "tf_main.h"
#include "common_m4a.h"
#include "nok_lt_prediction.h"
#include "port.h"
#include "buffers.h"
#include "bitstream.h"
#ifdef VERSION2
#include "resilience.h"
#include "concealment.h"
#endif

#include "allVariables.h"
#include "obj_descr.h"

/* merged HP 971104 */

int    debug[256];
int    adif_header_present;
char   *aacDebugOptions=NULL;

/* prediction */
static  PRED_STATUS     *sp_status[Chans];
static  PRED_TYPE       pred_type;


void init( int     block_size_samples,
           Info*** sfbInfo)
{
#if (CChans > 0)
  init_cc();
#endif
  huffbookinit( block_size_samples);
  predinit();

  winmap[0] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[1] = win_seq_info[ONLY_LONG_SEQUENCE];
  winmap[2] = win_seq_info[EIGHT_SHORT_SEQUENCE];
  winmap[3] = win_seq_info[ONLY_LONG_SEQUENCE];
  *sfbInfo = winmap;
}

void
predinit(void)
{
  int i, ch;
  if (pred_type == MONOPRED)
    for (ch = 0; ch < Chans; ch++) {
      for (i = 0; i < LN2; i++) {
        init_pred_stat(&sp_status[ch][i],
                       PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
      }
    }
}


static int getdata ( int*              tag, 
                     int*              dt_cnt, 
                     byte*             data_bytes,
#ifdef VERSION2
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA    hEscInstanceData,
#endif /*VERSION2*/
                     HANDLE_BUFFER     hVm )
{
  int i, align_flag, cnt;

  *tag = GetBits ( LEN_TAG,
#ifdef VERSION2
                   ELEMENT_INSTANCE_TAG, 
                   hResilience,
                   hEscInstanceData,
#endif /*VERSION2*/
                   hVm );

  align_flag = GetBits ( LEN_D_ALIGN,
#ifdef VERSION2
                         MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                         hResilience,
                         hEscInstanceData,
#endif /*VERSION2*/
                   hVm );

  if ((cnt = GetBits ( LEN_D_CNT,
#ifdef VERSION2
                       MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                       hResilience,
                       hEscInstanceData,
#endif /*VERSION2*/
                       hVm)) == (1<<LEN_D_CNT)-1)

    cnt +=  GetBits ( LEN_D_ESC,
#ifdef VERSION2
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
#endif /*VERSION2*/
                      hVm);

  *dt_cnt = cnt;
  if (debug['x'])
    fprintf(stderr, "data element %d has %d bytes\n", *tag, cnt);
  if (align_flag)
    byte_align();

  for (i=0; i<cnt; i++){
    data_bytes[i] = (unsigned char) GetBits ( LEN_BYTE,
#ifdef VERSION2
                                              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                                              hResilience,
                                              hEscInstanceData,
#endif /*VERSION2*/
                                              hVm);
    if (debug['X'])
      fprintf(stderr, "%6d %2x\n", i, data_bytes[i]);
  }
  return 0;
}

static void getfill ( 
#ifdef VERSION2
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
#endif /*VERSION2*/
                     HANDLE_BUFFER hVm )
{
  int i, cnt;

  if ((cnt = GetBits (LEN_F_CNT,
#ifdef VERSION2
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
#endif /*VERSION2*/
                      hVm )) == (1<<LEN_F_CNT)-1)

    cnt +=  GetBits ( LEN_F_ESC,
#ifdef VERSION2
                      MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                      hResilience,
                      hEscInstanceData,
#endif /*VERSION2*/
                      hVm ) - 1;
  if (debug['x'])
    fprintf(stderr, "fill element has %d bytes\n", cnt);
  for (i=0; i<cnt; i++)
    GetBits ( LEN_BYTE,
#ifdef VERSION2
              MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
              hResilience,
              hEscInstanceData,
#endif /*VERSION2*/
              hVm );
}

/*
void
main(int argc, char *argv[])
{}
---     Global variables from aac-decoder      ---
*/
static Float*          coef[Chans]; 
static Float*          data[Chans];
static Float*          state[Chans];
static byte            hasmask[Winds];
static byte*           mask[Winds];
static byte*           group[Winds];
static WINDOW_SEQUENCE     wnd[Winds];
static byte*           cb_map[Chans];
static byte            d_bytes[Avjframe];
short*                 factors[Chans];
int*                   lpflag[Chans];
int*                   prstflag[Chans];
static TNS_frame_info* tns[Chans];
static Wnd_Shape       wnd_shape[Winds];
#if (CChans > 0)
static Float*          cc_coef[CChans];
static Float*          cc_gain[CChans][Chans];
static byte            cc_wnd[CChans];
static Wnd_Shape       cc_wnd_shape[CChans];
#endif
#if (ICChans > 0)
Float*                 cc_state[ICChans];
#endif
static int             i, j,  ch, wn, ele_id, d_tag, d_cnt;
static int             left, right;
static Info*           info;
MC_Info                mc_info;
static MC_Info*        mip       = &mc_info;
Ch_Info*               cip;
/* -------------------------------------- */
/* -------------------------------------- */
/*           Init AAC-Decoder             */
/* -------------------------------------- */
/* -------------------------------------- */
static int blockSize;

void aac_decode_init(int  sampling_rate_decoded,
                     char *aacDebugStr, 
                     int  block_size_samples,
                     Info *** sfbInfo, 
                     int  predictor_type)
{
  blockSize= block_size_samples;
  aacDebugOptions = aacDebugStr;
  pred_type = (PRED_TYPE)predictor_type;
  for(i=0; i<Chans; i++){
    coef[i] = (Float *)mal1(LN2*sizeof(*coef[0]));
    data[i] = (Float *)mal1(LN2*sizeof(*data[0]));
    state[i] = (Float *)mal1(LN*sizeof(*state[0]));     /*      changed LN4 to LN 1/97 mfd      */
    for (j = 0; j < LN; j++) 
      (state [i]) [j] = 0.;
    fltclr(state[i], LN);
    factors[i] = (short *)mal1(MAXBANDS*sizeof(*factors[0]));
    cb_map[i] = (byte *)mal1(MAXBANDS*sizeof(*cb_map[0]));
    group[i] = (byte *)mal1(NSHORT*sizeof(group[0]));
    lpflag[i] = (int *)mal1(MAXBANDS*sizeof(*lpflag[0]));
    prstflag[i] = (int *)mal1((LEN_PRED_RSTGRP+1)*sizeof(*prstflag[0]));
    tns[i] = (TNS_frame_info *)mal1(sizeof(*tns[0]));
    wnd_shape[i].prev_bk = WS_FHG;
    fltclr(state[i], LN4);
    if (pred_type == MONOPRED)
      sp_status[i]  = (PRED_STATUS *)mal1(LN2*sizeof(*sp_status[0]));
  }
  for(i=0; i<Winds; i++) {
    mask[i] = (byte *)mal1(MAXBANDS*sizeof(mask[0]));
  }
#if (CChans > 0)
  for(i=0; i<CChans; i++){
    cc_coef[i] = (Float *)mal1(LN2*sizeof(*cc_coef[0]));
    for(j=0; j<Chans; j++)
      cc_gain[i][j] = (Float *)mal1(MAXBANDS*sizeof(*cc_gain[0][0]));
#if (ICChans > 0)
    if (i < ICChans) {
      cc_state[i] = (Float *)mal1(LN*sizeof(*state[0]));
      fltclr(cc_state[i], LN);
      cc_wnd_shape[i].prev_bk = 0;
    }
#endif
  }
#endif
  
  /* set defaults */
  adif_header_present = 0;
  current_program = -1;
  /* default_config = 1; initialized in mp4ifc.c; don't discard changes made reading decoderSpecificInfo */
  /* mc_info.profile = Main_Profile;  this is specified by DecPara()*/
  mc_info.sampling_rate_idx = Fs_48;

  /* multi-channel info is invalid so far */
  mc_info.mcInfoCorrectFlag = 0;

  /* Set AAC-Debugging options ,
     also update sampling rate for aac if specified */
  set_aac_options(sampling_rate_decoded);

  /* Init hufftables, ... */
  init( block_size_samples,sfbInfo); 
}


/* ---------------------------------------------- */
/* ---------------------------------------------- */
/* -----  Set Options, samplingrate for AAC ----- */
/* ---------------------------------------------- */
/* ---------------------------------------------- */
void
set_aac_options(int aacSamplingRate)
{
  unsigned int i; 
  int *p;
  
  /* --- reset all debug options --- */
  for (i=0;i<256;i++)
    debug[i]=0;
  if (aacDebugOptions!=NULL){
    for (i=0;i<strlen(aacDebugOptions);i++){
      debug[(int)aacDebugOptions[i]]=1;
      fprintf(stderr,"   !!! AAC debug option: %c recognized.\n", aacDebugOptions[i]);
    }
  }
  
  /* Set sampling rate */
  /* aacSamplingRate = samplingRate; */
  p = &prog_config.sampling_rate_idx;
  for (i=0; i<(1<<LEN_SAMP_IDX); i++) {
    if (aacSamplingRate == samp_rate_info[i].samp_rate)
      break;
  }
  
  if (i == (1<<LEN_SAMP_IDX)) 
    CommonExit(1,"Unsupported sampling frequency %d", aacSamplingRate);
  
  *p = mc_info.sampling_rate_idx = i;
}


/* -------------------------------------- */
/* -------------------------------------- */
/*           DECODE 1 frame               */
/* -------------------------------------- */
/* -------------------------------------- */
 
int AacDecodeFrame ( HANDLE_BSBITSTREAM       fixed_stream,
                     HANDLE_BSBITSTREAM       gc_stream[MAX_TIME_CHANNELS],
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     enum AAC_BIT_STREAM_TYPE bitStreamType, 
                     byte                     max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     Info**                   sfbInfo, 
                     byte                     sfbCbMap[MAX_TIME_CHANNELS][MAXBANDS],
                     short*                   pFactors[MAX_TIME_CHANNELS],
                     HANDLE_DECODER_GENERAL   hFault,
                     QC_MOD_SELECT            qc_select,
		     NOK_LT_PRED_STATUS**     nok_lt_status,
                     FRAME_DATA*              fd,
                     int                      layer)
{
  int           chCnt;
  int           outCh;
  int           sfb;
  Info*         sfbInfoP;
  Info          sfbInfoNScale;
  unsigned char nrOfSignificantElements = 0; /* SCE (mono) and CPE (stereo) only */
  int samplFreqIdx = (fd==NULL) ? -1 : (int)fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.\
        samplingFreqencyIndex.value;
  HANDLE_BUFFER            hVm              = hFault->hVm;
#ifdef VERSION2
  HANDLE_RESILIENCE        hResilience      = hFault->hResilience;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData = hFault->hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment     = hFault->hConcealment;
#endif /*VERSION2*/

  /* pass scalefactors to calling function */
  if ( bitStreamType == SCALABLE ) {
    if( pFactors!=NULL ) {
      if( numChannels == 1 ) {
        pFactors[0]=factors[0];
      }
      else {
        pFactors[0]=factors[1];
        pFactors[1]=factors[2];
      }
    }
  }

  if(debug['n']) {
    /*     fprintf(stderr, "\rblock %ld", bno); */
    printf("\n-------\nBlock: %ld\n", bno);
  }
  
  /* Set the current bitStream for aac-decoder */
  setHuffdec2BitBuffer(fixed_stream);

  reset_mc_info(mip);

  if ( bitStreamType != SCALABLE ) {
#ifdef VERSION2
    if ( hResilience && GetEPFlag ( hResilience ) ) {
      if ( numChannels == 2 ) {
        ele_id = ID_CPE;
      }
      else {
        ele_id = ID_SCE;
      }
    }
    else
#endif /*VERSION2*/
      {
        ele_id = GetBits ( LEN_SE_ID,
#ifdef VERSION2
                           MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                           hResilience,
                           hEscInstanceData,
#endif /*VERSION2*/
                           hVm );
      }
    if ( ele_id == ID_SCE || ele_id == ID_CPE ) {
      nrOfSignificantElements++;
    }
    sfbInfoP = &sfbInfoNScale;
  }
  else {
    sfbInfoP = sfbInfo[windowSequence[MONO_CHAN]];
    if (numChannels == 2) {
      ele_id = ID_CPE;
      if ( commonWindow ) {
        wnd[mip->ch_info[0].widx] = windowSequence[MONO_CHAN] ;
      } else {
        CommonExit(-1,"\n 2 channels but no common window: not supported");
      }
    }
    else { 
      ele_id=ID_SCE;
      wnd[mip->ch_info[0].widx]=windowSequence[MONO_CHAN];
    }
  }
  if(debug['v'])
    fprintf(stderr, "\nele_id %d\n", ele_id);

#ifdef VERSION2
  BsSetSynElemId (ele_id) ;
#endif  
  while ( ele_id != ID_END ) {
    /* get audio syntactic element */
#ifdef VERSION2
    /* only SCE and FIL are currently allowed, if the bitstream might be disturbed */
    if ( ( GetEPFlag( hResilience ) ) && ( BsGetReadErrorForDataElementFlagEP ( COMMON_WINDOW , hEscInstanceData ) 
                                           || ! ( ( ele_id == ID_SCE ) 
                                                  || ( ele_id == ID_CPE )
                                                  || ( ele_id == ID_FIL ) ) 
                                           || nrOfSignificantElements > 1 ) ) {
      break;
    }
#endif /*VERSION2*/

    switch (ele_id) {
    case ID_SCE:                /* single channel */
    case ID_CPE:                /* channel pair */
    case ID_LFE:                /* low freq effects channel */
      if ( huffdecode ( ele_id, 
                        mip, 
                        wnd, 
                        wnd_shape,
                        cb_map, 
                        factors, 
                        group, 
                        hasmask,
                        mask, 
                        max_sfb, 
                        pred_type, 
                        lpflag, 
                        prstflag, 
                        nok_lt_status, 
                        tns, 
                        gc_stream, 
                        coef,
                        bitStreamType,
                        commonWindow,
                        sfbInfoP,
#ifdef VERSION2
                        qc_select,
#endif /*VERSION2*/
                        hFault ) < 0 )
        CommonExit(1,"huffdecode (decoder.c)");

      if ((bitStreamType == SCALABLE)){
        for (ch=0; ch<Chans; ch++ ) {
          if (!(mip->ch_info[ch].present)) 
            continue;
          /* copy codebooks of ALL sfbs and groups*/
          for (sfb=0; sfb<MAXBANDS; sfb++) {
            if ( numChannels == 2 ) {
              sfbCbMap[ch-1][sfb] = cb_map[ch][sfb];
            }
            if ( numChannels == 1 ) {
              sfbCbMap[0][sfb] = cb_map[ch][sfb];
            }
          }
        }
      }
      break;
#if (CChans > 0)
    case ID_CCE:                /* coupling channel */
      if ( getcc ( mip, 
                   cc_wnd, 
                   cc_wnd_shape, 
                   cc_coef, 
                   cc_gain,
                   hVm,
                   hHcrSpecData,
                   hHcrInfo, hEscInstanceData ) < 0)
        CommonExit(1,"getcc");
      break;
#endif                          
    case ID_DSE:                /* data element */
      if ( getdata ( &d_tag, 
                     &d_cnt, 
                     d_bytes,
#ifdef VERSION2
                     hResilience, 
                     hEscInstanceData,
#endif /*VERSION2*/
                     hVm ) < 0)
        CommonExit(1,"data channel");
      break;
    case ID_PCE:                /* program config element */
      get_prog_config ( &prog_config,
                        blockSize,
#ifdef VERSION2
                        hResilience, 
                        hEscInstanceData,
#endif /*VERSION2*/
                        hVm );
      break;
    case ID_FIL:                /* fill element */
      getfill ( 
#ifdef VERSION2
               hResilience, 
                hEscInstanceData,
#endif /*VERSION2*/
                hVm );
      break;
    default:
      fprintf(stderr, "Element not supported: %d\n", ele_id);
      break;
    }
    if (bitStreamType != SCALABLE) {
#ifdef VERSION2
      if ( hResilience && GetEPFlag ( hResilience ) ) {
        ele_id = ID_END;
      }
      else
#endif /*VERSION2*/
        {
          ele_id = GetBits ( LEN_SE_ID,
#ifdef VERSION2
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience,
                             hEscInstanceData,
#endif /*VERSION2*/
                             hVm );
        }
      if ( ele_id == ID_SCE || ele_id == ID_CPE ) {
        nrOfSignificantElements++;
      }
    }
    else {
      ele_id=ID_END;
    }
    if(debug['v']) {
      fprintf(stderr, "\nele_id %d\n", ele_id);
    }
  }
  check_mc_info ( mip, 
                  ( ! mc_info.mcInfoCorrectFlag && default_config ) || ( bitStreamType == SCALABLE )
#ifdef VERSION2
                  ,
                  hEscInstanceData,
                  hResilience
#endif /*VERSION2*/
                  ); 

#if (ICChans > 0)
  /* transform independently switched coupling channels */
  ind_coupling(mip, wnd, wnd_shape, cc_wnd, cc_wnd_shape, cc_coef ,cc_state);
#endif  

#ifdef VERSION2
  ConcealmentCheckClassBufferFullnessEP(GetReadBitCnt(hVm),
                                        hResilience,
                                        hEscInstanceData,
                                        hConcealment);
  if (ConcealmentGetEPprematurely(hConcealment))
    hEscInstanceData = ConcealmentGetEPprematurely(hConcealment);
#endif /*VERSION2*/

  if ((bitStreamType != SCALABLE)) {
    /* 
     * set window sequence and shape 
     * parameters of the framework variables
     */

    int chanCnt = 0;
    for (ch=0; ch<Chans; ch++) {
      cip = &mip->ch_info[ch];
      if (cip->present) {  
        windowSequence[chanCnt] = wnd[cip->widx];
        window_shape[chanCnt]   = wnd_shape[cip->widx].this_bk;
        chanCnt++;
      }
      if (chanCnt >= MAX_TIME_CHANNELS) {
        break;
      }
    }

    /* m/s stereo */
    for (ch=0; ch<Chans; ch++) {
      cip = &mip->ch_info[ch];
      if ((cip->present) && (cip->cpe) && (cip->ch_is_left)) {  
        wn = cip->widx;
        if(hasmask[wn]) {
          left = ch;
          right = cip->paired_ch;
          info = winmap[wnd[wn]];
          map_mask(info, group[wn], mask[wn], cb_map[right], hasmask[wn]);
#ifdef VERSION2
          ConcealmentMiddleSideStereo(info,
                                      group[wn],
                                      mask[wn],
                                      coef,
                                      left,
                                      right,
                                      hConcealment);

#endif /*VERSION2*/
          synt(info, group[wn], mask[wn], coef[right], coef[left]);
        }
      }
    }
  }

  /* Common part for aac_raw and aac_scaleable */
  chCnt = 0;
  for (ch=0; ch<Chans; ch++) {
#ifdef VERSION2
    BsSetChannel (ch);
    if (! ( hEscInstanceData && ( BsGetErrorForDataElementFlagEP ( SECT_CB, hEscInstanceData ) ) ) ) 
#endif
      {

      if (!(mip->ch_info[ch].present)) continue;

      if (bitStreamType != SCALABLE)
        wn = mip->ch_info[ch].widx;
      else
        wn = 0;

      info = winmap[wnd[wn]];

#ifdef PNS

      /* pns for scaleable object type is decoded in tfScaleableAUDecode() */
      if ((bitStreamType != SCALABLE)) {
      /* calc PNS spectrum */ 
      pns( mip, 
           info, 
           ch, 
           group[wn], 
           cb_map[ch], 
           factors[ch],
           lpflag[wn],
           coef );
      }
#endif /*PNS*/
      chCnt++;
    }
#ifdef VERSION2
    else {
      if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
        printf( "AacDecodeFrame: SI disturbed\n" );
        printf( "AacDecodeFrame: --> pns disabled (channel %d)\n", ch );
      }
    }
#endif
  }
  
  if ((bitStreamType != SCALABLE)) {
    /* intensity stereo and prediction */
    chCnt = 0;
    for (ch=0; ch<Chans; ch++) {
#ifdef VERSION2
      ConcealmentSetChannel(ch, hConcealment);
#endif /*VERSION2*/
      if (!(mip->ch_info[ch].present)) continue;
      wn = mip->ch_info[ch].widx;
      info = winmap[wnd[wn]];

      intensity(mip, info, ch, 
                group[wn], cb_map[ch], factors[ch], 
                lpflag[wn], coef);

      if (pred_type == MONOPRED) {
        if (mip->profile == Main_Profile) {
          predict(info, 
                  lpflag[wn], 
                  sp_status[ch], 
#ifdef VERSION2
                  hConcealment,
#endif /*VERSION2*/
                  coef[ch]);
        }
        else {
          if (*lpflag[wn] != 0) {
#ifdef VERSION2
            if ( ! ( hResilience && GetEPFlag ( hResilience ) ) ) {
#endif /*VERSION2*/
              CommonExit(1,"AacDecodeFrame: prediction not allowed in this profile!");
#ifdef VERSION2
            }
            else {
              if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
                printf("AacDecodeFrame: prediction not allowed in this profile\n");
              }
            }
#endif /*VERSION2*/
          }
        }
      }
      chCnt++;
    }
  
    chCnt = 0;
    for (ch=0; ch<Chans; ch++) {
#ifdef VERSION2
      BsSetChannel(ch);
#endif
      if (!(mip->ch_info[ch].present)) continue;

      wn = mip->ch_info[ch].widx;

      info = winmap[wnd[wn]];
    
      /* predictor reset */
      left = ch;
      right = left;
      if ( (mip->ch_info[ch].cpe) &&
           (mip->ch_info[ch].common_window) )
        /* prstflag's shared by channel pair */
        right = mip->ch_info[ch].paired_ch;
      if (pred_type == MONOPRED)
        predict_reset(info, prstflag[wn], sp_status, 
                      left, right);
      else if (pred_type == NOK_LTP) /* Long term prediction.  */
        nok_lt_predict(info, wnd[wn], 
                       wnd_shape[wn].this_bk,
                       wnd_shape[ch].prev_bk,
                       nok_lt_status[chCnt]->sbk_prediction_used,
                       nok_lt_status[chCnt]->sfb_prediction_used,
                       nok_lt_status[chCnt], nok_lt_status[chCnt]->weight, 
                       nok_lt_status[chCnt]->delay, coef[ch],
#ifdef VERSION2
                       (qc_select == AAC_LD) ? blockSize : BLOCK_LEN_LONG,
		       
                       (qc_select == AAC_LD) ? 
                       blockSize * BLOCK_LEN_MEDIUM / BLOCK_LEN_LONG :
                       BLOCK_LEN_MEDIUM,
		       
                       (qc_select == AAC_LD) ? 
                       blockSize * BLOCK_LEN_SHORT / BLOCK_LEN_LONG : 
                       BLOCK_LEN_SHORT,
		       
#else
                       BLOCK_LEN_LONG, BLOCK_LEN_MEDIUM, BLOCK_LEN_SHORT,
#endif
                       tns[ch], qc_select);
      
      chCnt++;
      
      /* Save window shape for next frame (used by LTP tool)*/
      wnd_shape[ch].prev_bk = wnd_shape[wn].this_bk;

#ifdef PNS
      /* PNS predictor reset, works only for MONOPRED ! */
      if(pred_type==MONOPRED)
        predict_pns_reset(info, sp_status[ch], cb_map[ch]);
#endif /*PNS*/

#if (CChans > 0)
      /* if cc_domain indicates before TNS */
      void              coupling(Info *info, MC_Info *mip, Float **coef, Float **cc_coef, Float **cc_gain, int ch, int cc_dom, int cc_dep);
      coupling(mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif
    
      /* tns */
#ifdef VERSION2
      if (! ( hEscInstanceData && ( BsGetErrorForDataElementFlagEP ( N_FILT       , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( COEF_RES     , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( LENGTH       , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( ORDER        , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( DIRECTION    , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( COEF_COMPRESS, hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( COEF         , hEscInstanceData ) ||
                           BsGetErrorForDataElementFlagEP ( GLOBAL_GAIN  , hEscInstanceData ) ) ) ) {
#endif /*VERSION2*/
        if (bitStreamType != SCALABLE) {
          for (i=j=0; i<tns[ch]->n_subblocks; i++) {
            if (debug['T']) {
              fprintf(stderr, "%ld %d %d\n", bno, ch, i);
              print_tns( &(tns[ch]->info[i]));
            }
#ifdef VERSION2
            if (tns[ch]->info[i].n_filt)
              ConcealmentTemporalNoiseShaping(coef,
                                              info,
                                              (short) i,
                                              (short) ch,
                                              hConcealment);
#endif /*VERSION2*/
            tns_decode_subblock(&coef[ch][j],
                                max_sfb[wn],
                                info->sbk_sfb_top[i],
                                info->islong,
                                &(tns[ch]->info[i]),
                                 qc_select,samplFreqIdx);      
            
            j += info->bins_per_sbk[i];
          }
        }
#ifdef VERSION2
      }
      else {
        if ( !hEscInstanceData || BsGetEpDebugLevel ( hEscInstanceData ) >= 2 ) {
          printf( "AacDecodeFrame: tns data disturbed\n" );
          printf( "AacDecodeFrame: --> tns disabled (channel %d)\n", ch );
        }
      }
#endif /*VERSION2*/
          
#if (CChans > 0)
      /* if cc_domain indicated after TNS */
      coupling(mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

#if (CChans > 0)
      /* independently switched coupling */#
        /* coupling(info, mip, coef, cc_coef, cc_gain, ch, CC_DOM, CC_IND);  */
#pragma warning " coupling is currently not supported in VM "
#endif
        }
  }
#ifdef VERSION2

  /* Concealment save and restore coefficient functionality */
  ConcealmentMainEntry(mip,
                       wnd,
                       wnd_shape,
                       windowSequence,
                       window_shape,
                       lpflag,
                       group,
                       hasmask,
                       mask,
                       coef,
                       hResilience,
                       hConcealment);

#endif /*VERSION2*/

  /* Copy the coeff and pass them back to VM */
  outCh=0;
  for (ch=0; ch<Chans; ch++) {
    if ((mip->ch_info[ch].present)){
      if ((outCh+1)> numChannels){
        CommonExit(-1,"wrong number of channels in command line");
      }
      for (i=0;i<blockSize;i++){
        spectral_line_vector[outCh][i] = (double)coef[ch][i];
      }
      outCh++;
    }
  }

  bno++;
  return GetReadBitCnt ( hVm );
}

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
