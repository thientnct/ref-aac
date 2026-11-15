/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation),
  Yasuhiro Toguri (Sony Corporation)

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


#include "adifdec.h"
#include "extension_type.h"
#include "block.h"
#include "config.h"
#include "coupling.h"
#include "dolby_adapt.h"
#include "diagnose.h"
#include "gc.h"
#include "huffdec.h"
#include "huffinit.h"
#include "intensity.h"
#include "intrins.h"
#include "monopred.h"
#include "bitinput.h"
#include "stereo.h"

#ifdef MPEG4V1
#include "nok_lt_prediction.h"
#include "pns.h"
#endif
#ifdef CT_SBR
#include "ct_sbrdecoder.h"
#endif
#ifdef DRC
#include "drc.h"
#endif


long		bno;
int		debug[256];


/* prediction */
static	PRED_STATUS	*sp_status[Chans];
#ifdef MPEG4V1
/* long term prediction */
static  NOK_LT_PRED_STATUS *nok_lt_status[Chans];
#endif

/* all storage is global */
Info*		winmap[NUM_WIN_SEQ];
static Float	*coef[Chans], *data[Chans], *state[Chans];
#ifdef CT_SBR
static Float	*tmpData[Chans];
#endif
byte		hasmask[Chans], *mask[Chans], *group[Chans],
  wnd[Chans], max_sfb[Chans],
  *cb_map[Chans], d_bytes[MAX_DBYTES];
Wnd_Shape	wnd_shape[Chans];
short		*factors[Chans];
TNS_frame_info	*tns[Chans];

static	int	last_rstgrp_num[Chans] = {0};

#if (CChans > 0)
Float		*cc_coef[CChans], *cc_gain[CChans][Chans][1<<LEN_NCC];
byte		cc_wnd[CChans] = {0, 0, 0, 0};
Wnd_Shape	cc_wnd_shape[CChans];
Wnd_Shape	wnd_shape_SSR[Chans];
#if (ICChans > 0)
Float		*cc_state[ICChans];
#endif
#endif
#ifdef CT_SBR
static SBRDECODER ct_sbrDecoder;
static SBRBITSTREAM ct_sbrBitStr;
int sbrPresentInFillElem;
static int ct_dontApplySBR = 0;
#endif

#ifdef DRC
static HANDLE_DRC drc;
#endif

#ifdef ARITHMETIC_TORTURE_TEST
extern int *hcw_touched[],*esc_touched;
extern Huffman *huff_book_list[];
extern int not_touched_coeffs, not_touched_scfs, not_touched_escapes;
#endif

static int first_cpe = 0;

static void
predinit(void)
{
  int i, ch;
  for (ch = 0; ch < Chans; ch++) {
    for (i = 0; i < LN2; i++) {
      init_pred_stat(&sp_status[ch][i],
                     PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
    }
  }
#ifdef MPEG4V1
  for (ch = 0; ch < Chans; ch++) {
    nok_init_lt_pred(nok_lt_status[ch]);
  }
#endif
}


void
decoder_init(void)
{
  int i, j, k;

  /* set defaults */
  default_config = 1;
  current_program = -1;
  prog_config.comments[0] = 0;
  prog_config.data.num_ele = 0;
  mc_info.profile = Main_Profile;
  mc_info.sampling_rate_idx = 3;
  mc_info.sampling_rate = sampling_rate[mc_info.sampling_rate_idx];
  
#ifdef CT_SBR
  mc_info.HE_AAC_level = 5;
  mc_info.bDownSampledSbr = 0;
  mc_info.upsamplingFactor = 1;
  mc_info.sbrPresentFlag = 2;
#ifdef PARAMETRICSTEREO
  mc_info.sbrEnablePS = 0;
#endif
#endif

  /* allocate memory */
  for(i=0; i<Chans; i++){
    coef[i] = (Float *)mal1(LN2*sizeof(*coef[0]));
    data[i] = (Float *)mal1(LN2*sizeof(*data[0]));
#ifdef CT_SBR    
    tmpData[i] = (Float *)mal1(LN*sizeof(*data[0]));
#endif
    state[i] = (Float *)mal1(LN*sizeof(*state[0]));	/* changed LN4 to LN 1/97 mfd */
    fltclr(state[i], LN);
    factors[i] = (short *)mal1(MAXBANDS*sizeof(*factors[0]));
    cb_map[i] = (byte *)mal1(MAXBANDS*sizeof(*cb_map[0]));
    group[i] = (byte *)mal1(NSHORT*sizeof(group[0]));
    lpflag[i] = (int *)mal1(MAXBANDS*sizeof(*lpflag[0]));
    prstflag[i] = (int *)mal1((LEN_PRED_RSTGRP+1)*sizeof(*prstflag[0]));
    tns[i] = (TNS_frame_info *)mal1(sizeof(*tns[0]));
    sp_status[i]  = (PRED_STATUS *)mal1(LN*sizeof(*sp_status[0]));
    wnd_shape[i].prev_bk = 0;
#ifdef MPEG4V1
    nok_lt_status[i]  = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_lt_status[0]));
#endif
  }
  for(i=0; i<Chans; i++) {
    mask[i] = (byte *)mal1(MAXBANDS*sizeof(mask[0]));
  }
#if (CChans > 0)
  for(i=0; i<CChans; i++) {
    cc_coef[i] = (Float *)mal1(LN2*sizeof(*cc_coef[0]));
    for(j=0; j<Chans; j++)
      for (k=0; k<(1<<LEN_NCC); k++)
        cc_gain[i][j][k] = (Float *)mal1(MAXBANDS*sizeof(*cc_gain[0][0][0]));
#if (ICChans > 0)
    if (i < ICChans) {
      cc_state[i] = (Float *)mal1(LN*sizeof(*state[0]));  /* changed LN4 to LN 1/97 mfd */
      fltclr(cc_state[i], LN);
      cc_wnd_shape[i].prev_bk = 0;
    }
#endif
  }
  init_cc();
#endif

  for(i = 0; i < Chans; i++){
    wnd_shape_SSR[i].prev_bk = 0;
    wnd_shape[i].prev_bk = 0;
    wnd_shape[i].this_bk = 0;
  }

  predinit();
#ifdef CT_SBR
  ct_sbrDecoder = NULL;
  ct_sbrBitStr.NrElements = 0;
  ct_sbrBitStr.NrElementsCore = 0;
#endif
#ifdef	DRC
  drc = drc_init();
  drc_set_params(drc, 0, 0, -1);
#endif
}

void decoder_free(void)
{
  unsigned short int i, j, k;
  for(i=0; i<Chans; i++) {
    free1(coef[i]);
    free1(data[i]);
#ifdef CT_SBR
    free1(tmpData[i]);
#endif
    free1(state[i]);
    free1(factors[i]);
    free1(cb_map[i]);
    free1(group[i]);
    free1(lpflag[i]);
    free1(prstflag[i]);
    free1(tns[i]);
    free1(sp_status[i]);
#ifdef MPEG4V1
    free1(nok_lt_status[i]);
#endif
  }
  for(i=0; i<Chans; i++) {
    free1(mask[i]);
  }
  for(i=0; i<CChans; i++) {
    free1(cc_coef[i]);
    for(j=0; j<Chans; j++) { 
      for (k=0; k<(1<<LEN_NCC); k++)
        free1(cc_gain[i][j][k]); 
    } 
    if (i < ICChans) {
      free1(cc_state[i]); 
    }
  }
#ifdef CT_SBR
  closeSBR(ct_sbrDecoder);
#endif
#ifdef DRC
  drc_free(drc);
#endif
  destroyTransfo();
  destroyGC();
  destroyCC();

#ifdef ARITHMETIC_TORTURE_TEST
  if (debug['t']) {
    PRINT(SE,"unused coefficients: %i\n",not_touched_coeffs);
    for (i=0; i<BOOKSCL-1; i++) {
      PRINT(SE, "huffman codebook %i:\n",i+1);
      for (j=0; j<book[i+1].n; j++) if (!hcw_touched[i][j])
        PRINT(SE,"  code %4lx (index %4i) not used\n",book[i+1].hcw[j].cw,book[i+1].hcw[j].index);
    }
    PRINT(SE,"unused scalefactors: %i\n",not_touched_scfs);
    PRINT(SE, "scalefactor codebook:\n");
    for (j=0; j<book[BOOKSCL].n; j++) if (!hcw_touched[BOOKSCL-1][j]) 
      PRINT(SE,"  code %4lx (index %4i) not used\n",book[BOOKSCL].hcw[j].cw,book[BOOKSCL].hcw[j].index);
    PRINT(SE,"unused escapes: %i\n",not_touched_escapes);
    for (j=16; j<8192; j++) if (!esc_touched[j]) /* scalefactors and escapes are limited to 8192 */
      PRINT(SE,"  escape sequence    %4i  not used\n",j);
  }
  for(i=0; i<NSPECBOOKS; i++) {
    free1(hcw_touched[i]);
  }
  free1(esc_touched);
#endif
}



void
decoder_setupParameters(int sampling_rate_idx)
{
  huffbookinit(sampling_rate_idx);

  winmap[0] = win_seq_info[ONLY_LONG_WINDOW];
  winmap[1] = win_seq_info[ONLY_LONG_WINDOW];
  winmap[2] = win_seq_info[EIGHT_SHORT_WINDOW];
  winmap[3] = win_seq_info[ONLY_LONG_WINDOW];

#ifdef ARITHMETIC_TORTURE_TEST
  if (!hcw_touched[0]) {
    int i,j;
    not_touched_coeffs=0;
    for(i=0; i<(NBOOKS-1); i++) {
      hcw_touched[i]=(int*)mal1(book[i+1].n*sizeof(int));
      huff_book_list[i]=book[i+1].hcw;
      for(j=0; j<book[i+1].n; j++) hcw_touched[i][j]=0;
      if (i!=BOOKSCL-1) not_touched_coeffs+=book[i+1].n;
      else not_touched_scfs=book[i+1].n;
    }
    not_touched_escapes=8192-15;
    esc_touched=(int*)mal1(8192*sizeof(int)); /* scalefactors and escapes are limited to 8192 */
    for (i=0; i<8192; i++) esc_touched[i]=0;
  }
#endif
}

void
decoder_selectProgram(int selectedProgram)
{
  current_program = selectedProgram;
}


#ifdef CT_SBR
void
decoder_setSBR_conformance(int dontApplySBR)
{
  ct_dontApplySBR = dontApplySBR;
}

/* SBRBITSTREAM* */
void*
decoder_getSBRbitstream()
{
  return &ct_sbrBitStr;
}


#endif

#ifdef DRC
void
decoder_setDrc(Float drc_high, Float drc_low, int drc_ref_level)
{
  drc_set_params(drc, drc_high, drc_low, drc_ref_level);
}
#endif



static int
getdata(int *tag, int *dt_cnt, byte *data_bytes)
{
  int i, align_flag, cnt;

  dse_present = 1;

  *tag = getbits(LEN_TAG);

  if (debug['x']) {
    for (i=0; i<prog_config.data.num_ele; i++) {
      if (*tag==prog_config.data.ele_tag[i]) { i=-1;break; }
    }
    if (i!=-1) PRINT(SE, "data element %d does not belong to current program\n", *tag);
  }

  set_log_level(ID_DSE, LEN_TAG, *tag);  /* required for CRC and ADIF2MP4 */

  align_flag = getbits(LEN_D_ALIGN);
  if ((cnt = getbits(LEN_D_CNT)) == (1<<LEN_D_CNT)-1)
    cnt +=  getbits(LEN_D_ESC);
  *dt_cnt = cnt;
  if (debug['x'])
    PRINT(SE, "data element %d has %d bytes\n", *tag, cnt);
  if (align_flag)
    byte_align();

  for (i=0; i<cnt; i++) {
    data_bytes[i] = getbits(LEN_BYTE);
    if (debug['X'])
      PRINT(SE, "%6d %2x\n", i, data_bytes[i]);
  }

  set_log_level(LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */

  return 0;
}


#ifdef CT_SBR

static void
sbrSetElementType(SBR_ELEMENT_ID Type, int chIndx)
{
  ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].ElementID=Type;
  ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].chIndx=chIndx;
  ct_sbrBitStr.NrElementsCore++;
}

static void ResetSbrBitstream(void)
{
  ct_sbrBitStr.NrElements=0;
  ct_sbrBitStr.NrElementsCore=0;  
}



static int
getSbrBitStream(int count, int Extention_Type)
{

  int byteCount = 1;

  if(count>0) {
    int i;

    if(count < MAXSBRBYTES && ct_sbrBitStr.NrElements < MAXNRELEMENTS) {

      ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].ExtensionType = Extention_Type;
      ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].Payload       = count;
      ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].Data[0]       = getbits(4);
      
      for (i = 1 ; i < count ; i++) {
        ct_sbrBitStr.sbrElement[ct_sbrBitStr.NrElements].Data[i] = getbits(8);
        byteCount++;
      }

      ct_sbrBitStr.NrElements +=1;
    }
    else {
      getbits(4); 
      for (i = 0 ; i < count-1 ; i++) {
        byteCount++;
        getbits(8);
      }

    }
  }

  return byteCount;
}

#endif



static int extension_payload ( int   cnt, 
                               byte* data ) 
{

/* TODO: In the MPEG-4 context 'data' could be used to store data
   transmitted in an EXT_DATA_ELEMENT.  Retrieving such data is not
   yet implemented. Thus, 'data' is not used yet. */

  int type, i;
    
  type = getbits(LEN_EXT_TYPE);
  if (debug['x'])
    PRINT(SE, "fill element extension %d\n", type);
    
  switch(type) {
#ifdef	DRC
  case EXT_DYNAMIC_RANGE:
    drc_present=1;
    return drc_parse(drc);
#endif


#ifdef CT_SBR
  case EXT_SBR_DATA:
  case EXT_SBR_DATA_CRC:
    return getSbrBitStream(cnt, type);
#endif

  case EXT_FILL_DATA:
    if (getbits(LEN_NIBBLE) != 0) PRINT(SE,"WARNING: fill_nibble not '0000'\n"); /* shall be '0000' */
    for (i=0; i<cnt-1; i++)
      if (getbits(LEN_BYTE) != 165 ) PRINT(SE,"WARNING: fill_byte not '10100101'\n");
    return cnt;

  case EXT_FILL:
  case EXT_DATA_ELEMENT:
  case EXT_SAC_DATA:
  default: 
    getbits(LEN_NIBBLE);
    for (i=0; i<cnt-1; i++)
      getbits(LEN_BYTE);
    return cnt;
  }
}

static void
getfill(byte *data)
{
  int cnt;

  set_log_level(ID_FIL, 0, 0);  /* required for CRC and ADIF2MP4 */

  if ((cnt = getbits(LEN_F_CNT)) == (1<<LEN_F_CNT)-1)
    cnt +=  getbits(LEN_F_ESC) - 1;
  if (debug['x'])
    PRINT(SE, "fill element has %d bytes\n", cnt);
  while (cnt > 0)
    cnt -= extension_payload(cnt, data);

  set_log_level(LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */
}








/* parse a block */
void
dec_parse()
{
  int ele_id, d_tag, d_cnt;
  MC_Info *mip = &mc_info;


  if(debug['n'])
    PRINT(SE, "\rblock %ld  ", bno);
  if(debug['N'])
    PRINT(SE, "block %ld\n", bno);

  byte_align();

  reset_mc_info(mip);
#ifdef	DRC	
  drc_reset(drc);
#endif
#ifdef CT_SBR

  sbrPresentInFillElem = 0;

  ResetSbrBitstream();
#endif

  if(bno==0)
    first_cpe = 0;

  while ((ele_id=getbits(LEN_SE_ID)) != ID_END) {
    /* get audio syntactic element */

    if(debug['v'])
      PRINT(SE, "\nele_id %d\n", ele_id);

    switch (ele_id) {
#ifdef CT_SBR
      int chIndx;
#endif

    case ID_SCE:		/* single channel */
    case ID_CPE:		/* channel pair */
    case ID_LFE:		/* low freq effects channel */
      if (huffdecode(ele_id, mip, wnd, wnd_shape,
                     cb_map, factors, 
                     group, hasmask, mask, max_sfb,
                     lpflag, prstflag
#ifdef MPEG4V1
                     , nok_lt_status
#endif
                     , tns, coef
#ifdef CT_SBR
                     , &chIndx
#endif
                     ,&first_cpe
                     ) < 0)
        myexit("huffdecode");
#ifdef CT_SBR
      if (ele_id == ID_SCE)
        sbrSetElementType(SBR_ID_SCE, chIndx);
      else if (ele_id == ID_CPE)
        sbrSetElementType(SBR_ID_CPE, chIndx);
#endif
      break;
#if (CChans > 0)
    case ID_CCE:		/* coupling channel */
      if (getcc(mip, cc_wnd, cc_wnd_shape, cc_coef, cc_gain) < 0)
        myexit("getcc");
      break;
#endif				
    case ID_DSE:		/* data element */
      if (getdata(&d_tag, &d_cnt, d_bytes) < 0)
        myexit("data channel");
      break;
    case ID_PCE:		/* program config element */
      set_log_level(ele_id, 0, 0);   /* required for CRC and ADIF2MP4 */
      get_prog_config(&prog_config);
      set_log_level(LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */
      break;
    case ID_FIL:		/* fill element */

      getfill(d_bytes);
#ifdef CT_SBR
      if(ct_sbrBitStr.NrElements){
        sbrPresentInFillElem = 1;
      }
#endif

      break;
    default:
      PRINT(SE, "Element not supported: %d\n", ele_id);
      break;
    }
  }

  byte_align();

  set_log_level(ID_END, 0, 0);  /* required for CRC and ADIF2MP4 */

#ifdef CT_SBR
  if (ct_sbrBitStr.NrElements) {
    /* for every core SCE or CPE there must be an SBR element, otherwise sths. wrong */
    if (ct_sbrBitStr.NrElements != ct_sbrBitStr.NrElementsCore)
      myexit("number of SBR elements does not match number of SCE/CPEs");
  }
#endif

  if(debug['v']) {
    PRINT(SE, "\nele_id %d\n", ele_id);
    print_mc_info(mip);
  }
  check_mc_info(mip, (bno==0 && default_config));

#ifdef MPEG4V1
  if ((ltp_present)||(noise_present)) stream_is_mpeg4 = 1;
#endif
  if(debug['V']) show_tools(&mc_info, wnd_shape, hasmask);

}



void
dec_decode(Float ***data_extern, int *block_length)
{
  int i, j, ch, wn;
  int left, right;
  Info *info;
  Ch_Info *cip;
  MC_Info *mip = &mc_info;

#ifdef	DRC	
  drc_map_channels(drc, mip);
#endif

#if (ICChans > 0)
  /* process independently switched coupling channels */
#ifdef	DRC
  /* apply dynamic range control */ 
  i=0;
  for (ch=0; ch<mip->ncch; ch++) {
#ifdef CT_SBR
    wn = mip->ch_info[ch].widx;
#endif
    if (mip->cc_ind[ch]) {
      drc_apply(drc, cc_coef[ch], Chans+i
#ifdef CT_SBR
        ,ct_sbrBitStr.NrElements
        ,wnd [wn]
#endif
        );
      i++;	/* index for drc_info[] */
    }
  }
#endif
  if(!bno)
    for (ch=0; ch<Chans; ch++)
      wnd_shape[ch].prev_bk = wnd_shape[ch].this_bk;


  /* transform */
  ind_coupling(mip, cc_wnd, cc_wnd_shape, cc_coef,
               cc_state);
#endif

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
        synt(info, group[wn], mask[wn], coef[right], coef[left]);
      }
    }
  }

  /* intensity stereo and prediction */
  for (ch=0; ch<Chans; ch++) {
    if (!(mip->ch_info[ch].present)) continue;
    wn = mip->ch_info[ch].widx;
    info = winmap[wnd[wn]];
#ifdef MPEG4V1
    pns( mip, info, ch,
         group[wn], cb_map[ch], factors[ch],
         lpflag[wn], coef );
#endif
    if(mc_info.profile == Main_Profile) {
      predict(info, mip->profile, lpflag[wn], sp_status[ch], coef[ch]);
    }
    intensity(mip, info, ch, 
              group[wn], cb_map[ch], factors[ch], 
              lpflag[wn], coef);
  }

	
  for (ch=0; ch<Chans; ch++) {
    if (!(mip->ch_info[ch].present)) continue;
    wn = mip->ch_info[ch].widx;
    info = winmap[wnd[wn]];

    if(mc_info.profile == Main_Profile) {
      /* predictor reset */
      left = ch;
      right = left;
      if ( (mip->ch_info[ch].cpe) &&
           (mip->ch_info[ch].common_window) ) {
        /* prstflag's shared by channel pair */
        right = mip->ch_info[ch].paired_ch;
      }
      predict_reset(info, prstflag[wn], sp_status, left, right, last_rstgrp_num, lpflag[wn]);
    }
#ifdef MPEG4V1
    else if(mc_info.profile == LTP_Profile)
      nok_lt_predict (info, wnd[wn], 
		      wnd_shape[wn].this_bk,
		      wnd_shape[ch].prev_bk,
                      nok_lt_status[ch]->sbk_prediction_used,
                      nok_lt_status[ch]->sfb_prediction_used,
                      nok_lt_status[ch], nok_lt_status[ch]->weight,
                      nok_lt_status[ch]->delay, coef[ch],
                      BLOCK_LEN_LONG, tns[ch]);
    
    /* PNS predictor reset */
    predict_pns_reset(info, sp_status[ch], cb_map[ch]);
#endif


#if (CChans > 0)
    /* if cc_domain indicates before TNS */
    coupling(info, mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

    /* tns */
    for (i=j=0; i<tns[ch]->n_subblocks; i++) {
      if (debug['T']) {
        PRINT(SE, "%ld %d %d\n", bno, ch, i);
        print_tns( &(tns[ch]->info[i]));
      }

      tns_decode_subblock(&coef[ch][j],
                          max_sfb[wn],
                          info->sbk_sfb_top[i],
                          info->islong,
                          &(tns[ch]->info[i]) );

      j += info->bins_per_sbk[i];
    }

#if (CChans > 0)
    /* if cc_domain indicated after TNS */
    coupling(info, mip, coef, cc_coef, cc_gain, ch, !CC_DOM, !CC_IND);
#endif
#ifdef	DRC
    /* apply dynamic range control to each audio channel */
    drc_apply(drc, coef[ch], ch
#ifdef CT_SBR
              ,(ct_sbrBitStr.NrElements != 0 || mc_info.sbrPresentFlag != 0)
              ,wnd [wn]
#endif
      );
#endif    

    /* inverse transform */
    wnd_shape_SSR[ch].this_bk = wnd_shape[wn].this_bk;
    wnd_shape[ch].prev_bk = wnd_shape[wn].this_bk;

    if(mc_info.profile == SSR_Profile){
      gain_control(coef[ch], &wnd_shape_SSR[ch], wnd[wn], ch, data[ch]);
    }
    else {
      freq2time_adapt (wnd [wn], &wnd_shape_SSR[ch], coef [ch], state [ch], data[ch]);
    }

    wnd_shape_SSR[ch].prev_bk  = wnd_shape_SSR[ch].this_bk; 
    /*    freq2time_adapt (wnd [wn], &wnd_shape [wn], coef [ch], state [ch], data[ch]);*/
 
#if (CChans > 0)
    /* independently switched coupling */
    coupling(info, mip, data, cc_coef, cc_gain, ch, CC_DOM, CC_IND);
#endif

#ifdef MPEG4V1
    if(mc_info.profile == LTP_Profile)
      nok_lt_update(nok_lt_status[ch], data[ch], state[ch], BLOCK_LEN_LONG);
#endif
  } /* end chan loop */

  
#ifdef CT_SBR
  if (ct_sbrBitStr.NrElements == 0 || mc_info.sbrPresentFlag == 0) {
#endif
    if (data_extern!=NULL) *data_extern = data;
    if (block_length!=NULL) *block_length = LN2;
#ifdef CT_SBR
  } else 
  {

    if (bno==0 ) {
      int sampleRate = sampling_rate[mc_info.sampling_rate_idx];
      int lfeCh = FChans + SChans + BChans;   /* BIGDECODER ? 48 : 7 */

      if(mc_info.sbrPresentFlag == 2){
        if (debug['n']) 
            PRINT(SE, "Implicit signalling of SBR detected.\n");
      }

      switch (mc_info.HE_AAC_level){
        case 2:
          if(sampleRate > 24000){
            myexit("AAC sample rate too high for this level");
          }
          if(mc_info.nch > 2){
            myexit("Too many channels for this level");
          }
        break;
        case 3:
          if(sampleRate > 24000){
            mip->bDownSampledSbr = 1;
          }
          if(mc_info.nch > 2){
            myexit("Too many channels for this level");
          }
        break;
        case 4:
          if(mc_info.nch > 2){
            if(sampleRate > 24000){
              myexit("AAC sample rate too high for this level");
            }
            if(mc_info.nch > 6 || (mc_info.nch == 6 && !mc_info.ch_info[lfeCh].present)){
              myexit("Too many channels for this level");
            }
          }
          else{
            if(sampleRate > 48000){
              myexit("AAC sample rate too high for this level");
            }
            if(sampleRate > 24000){
              mip->bDownSampledSbr = 1;
            }
          }
          break;
        case 5:
          if(sampleRate > 48000){
            myexit("AAC sample rate too high for this level");
          }
          if(mc_info.nch > 6 || (mc_info.nch == 6 && !mc_info.ch_info[lfeCh].present)){
            myexit("Too many channels for this level");
          }
          break;
        default:
           myexit("Unknown HE-AAC Profile level");
           break;   
      };


      if (!ct_dontApplySBR) {
        ct_sbrDecoder = openSBR(sampleRate, mip->bDownSampledSbr, NULL);

        if (ct_sbrDecoder==NULL) {
          myexit("openSBR failed!\n");
        }
      }
    }   
     
    
    mip->upsamplingFactor = 2 - mip->bDownSampledSbr;

  

    if (ct_dontApplySBR) {
      if (data_extern!=NULL) *data_extern = data;
      if (block_length!=NULL) *block_length = LN2;
    } else {
      if (applySBR(ct_sbrDecoder,
                  &ct_sbrBitStr,
                  data,
                  tmpData,
                  mip->bDownSampledSbr,
#ifdef PARAMETRICSTEREO
                  mip->sbrEnablePS,
#endif
                  mc_info.ch_info) != SBRDEC_OK)
        myexit("invalid sbr bitstream");


      if (data_extern!=NULL) *data_extern = tmpData;
      if (block_length!=NULL) *block_length = LN2;

      if (mip->upsamplingFactor == 2.0f) {
        if (block_length!=NULL) *block_length = LN;
      }
    }
  }
#endif
}


void
dec(Float ***data_extern, int *block_length)
{
  dec_parse();
  dec_decode(data_extern, block_length);
}


