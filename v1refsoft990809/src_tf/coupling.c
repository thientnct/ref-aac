/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
and edited by
Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation)
in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */


#if (CChans > 0)

static	Float	    cc_gain_scale[4];
static	int	    *cc_lpflag[CChans];
static	int	    *cc_prstflag[CChans];
static	PRED_STATUS *cc_sp_status[CChans];
static	Float	    *cc_prev_quant[CChans];
static	byte	    *cc_group[CChans];
static	int	    nok_use_monopred;
/* long term prediction */
static	NOK_LT_PRED_STATUS *nok_cc_lt_status[Chans];
static  NOK_LTP_DATA	*nok_cc_ltp[Chans];

void
init_cc(int use_monopred)
{
  int i, ch;
  cc_gain_scale[3] = 2.0;

  /* predictor selection */
  if (use_monopred)
    nok_use_monopred = 1;
  else
    nok_use_monopred = 0;
    
  /* initialize gain scale */
  for (i=2; i>=0; i--)
    cc_gain_scale[i] = sqrt(cc_gain_scale[i+1]);

  /* coupling channel predictors */
  for (ch = 0; ch < CChans; ch++) {
    cc_lpflag[ch] = (int*)mal1(MAXBANDS*sizeof(*cc_lpflag[0]));
    cc_prstflag[ch] = (int*)mal1((LEN_PRED_RSTGRP+1)*sizeof(*cc_prstflag[0]));
    if (nok_use_monopred) {
      cc_sp_status[ch] = (PRED_STATUS*)mal1(LN*sizeof(*cc_sp_status[0]));
    } else {
      nok_cc_sp_status[ch]  = (NOK_PRED_STATUS *)mal1(sizeof(*nok_sp_status[0]));
    }
    nok_cc_lt_status[ch]  = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_cc_lt_status[0]));
    nok_cc_ltp[ch]  = (NOK_LTP_DATA *)mal1(sizeof(*nok_cc_ltp[0]));
    cc_prev_quant[ch] = (Float*)mal1(LN2*sizeof(*cc_prev_quant[0]));
    cc_group[ch] = (byte*)mal1(MAXBANDS*sizeof(*cc_group[0]));
    if (nok_use_monopred) {
      for (i = 0; i < LN2; i++) {
        init_pred_stat(&cc_sp_status[ch][i],
                       PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
        cc_prev_quant[ch][i] = 0.;
      } 
    } else {
      for (i = 0; i < LN2; i++) {
        nok_init_pred_stat(&nok_cc_sp_status[ch][i]);
        cc_prev_quant[ch][i] = 0.;
      }
    }
    nok_init_lt_pred (nok_cc_lt_status[ch]);
  }
}

int getcc ( MC_Info*       mip, 
            byte*          cc_wnd, 
            Wnd_Shape*     cc_wnd_shape, 
            Float**        cc_coef,
            Float*         cc_gain[CChans][Chans],
            HANDLE_BUFFER  hVm,
            HANDLE_BUFFER  hHcrSpecData,
            HANDLE_HCR     hHcrInfo,
            HANDLE_EP_INFO hEpInfo )
{
  int i, j, k, m, n, cpe, tag, cidx, ch, nele, nch;
  int target[Chans], shared[Chans];
  int cc_l, cc_r, cc_dom, cc_gain_ele_sign, ind_sw_cc, cgep, scl_idx,
    sign, nsect, fac;
  byte sect[2*MAXBANDS+1], cc_max_sfb[CChans];
  short global_gain, factors[MAXBANDS];
  Float scale;
  Info *info;
  TNS_frame_info tns;

#ifdef  SRS
    int dummy = 0;
#endif

  tag = GetBits(ELEMENT_INSTANCE_TAG,  
                LEN_TAG,
                hResilience,
                hVm, 
                hEpInfo );

  if (default_config) {
    cidx = mip->ncch;
  }
  else {
    cidx = XCChans;	    /* default is scratch index */
    for (i=0; i<mip->ncch; i++) {
      if (mip->cch_tag[i] == tag)
        cidx = i;
    }
  }
    
  if (cidx >= CChans) {
    printf( "Unanticipated coupling channel\n");
    myexit("coupling");
  }

  /*  get ind_sw_cce flag */
  ind_sw_cc = mip->GetBits(IND_SW_CCE_FLAG,  
                           LEN_IND_SW_CCEG,
                           hResilience,
                           hVm, 
                           hEpInfo );
    
  /* coupled (target) elements */
  nele = GetBits(NUM_COUPLED_ELEMENTS,  
                 LEN_NCCG,
                 hResilience,
                 hVm, 
                 hEpInfo );
  nch = 0;
  for (i=0; i<nele; i++) {
    cpe = GetBits(CC_TARGET_IS_CPE,  
                  LEN_IS_CPEG,
                  hResilience,
                  hVm, 
                  hEpInfo );
    tag = GetBits(CC_TARGET_TAG_SELECT,  
                  LEN_TAGG,
                  hResilience,
                  hVm, 
                  hEpInfo );
    ch = ch_index(mip, cpe, tag);
	
    if (!cpe) {
      target[nch] = ch;
      shared[nch++] = 0;
    }
    else {
      cc_l = GetBits(CC_L,  
                     LEN_CC_LRG,
                     hResilience,
                     hVm, 
                     hEpInfo );
      cc_r = GetBits(CC_R,  
                     LEN_CC_LRG,
                     hResilience,
                     hVm, 
                     hEpInfo );
      j = (cc_l<<1) | cc_r;
      switch(j) {
      case 0:	    /* shared gain list */
        target[nch] = ch;
        target[nch+1] = mip->ch_info[ch].paired_ch;
        shared[nch] = 1;
        shared[nch+1] = 1;
        nch += 2;
        break;
      case 1:	    /* left channel gain list */
        target[nch] = ch;
        shared[nch] = 0;
        nch += 1;
        break;
      case 2:	    /* right channel gain list */
        target[nch+1] = mip->ch_info[ch].paired_ch;
        shared[nch] = 0;
        nch += 1;
        break;
      case 3:	    /* two gain lists */
        target[nch] = ch;
        target[nch+1] = mip->ch_info[ch].paired_ch;
        shared[nch] = 0;
        shared[nch+1] = 0;
        nch += 2;
        break;
      }
    }
  }
    
  /*     cc_com = GetBits(53,  LEN_CC_DOM) != CC_AFTER_TNS); */
  cc_com = GetBits(CC_DOMAIN,  
                   LEN_CC_DOMG,
                   hResilience,
                   hVm );
  cc_gain_ele_sign = GetBits(GAIN_ELEMENT_SIGN,  
                             LEN_CC_SGNG,
                             hResilience,
                             hVm );
  scl_idx = GetBits(GAIN_ELEMENT_SCALE,  
                    LEN_CCH_GESG,
                    hResilience,
                    hVm );

  /*
   * coupling channel bitstream
   * (equivalent to SCE)
   */
  fltclr(cc_coef[cidx], LN2);
  memcpy(&info, winmap[cc_wnd[cidx]], sizeof(Info));
  if (!getics ( info, 
                0, 
                &cc_wnd[cidx], 
                &cc_wnd_shape[cidx].this_bk,
                cc_group[cidx], 
                cc_max_sfb[cidx], 
                cc_lpflag[cidx], 
                cc_prstflag[cidx],
                &nsect, 
                sect, 
                cc_coef[cidx], 
                &global_gain, 
                factors, 
                nok_cc_ltp[cidx],
                &tns,
#ifdef  SRS
                dummy,
#endif
                bitStreamType,
                hResilience,
                hVm,
                hHcrSpecData,
                hHcrInfo,
                hEpInfo) )
    return -1;
    
  /* coupling for first target channel(s) is already at
   * correct scale
   */
  ch = shared[0] ? 2 : 1;
  for (j=0; j<ch; j++)
    for (i=0; i<MAXBANDS; i++)
      cc_gain[cidx][target[j]][i] = 1.0;

  /*
   * bitstreams for target channel scale factors
   */
  for (; ch<nch; ) {
    /* if needed, get common gain elment present */
    cgep = ind_sw_cc ? 1 : GetBits(COMMON_GAIN_ELEMENT_PRESENT,  
                                   LEN_CCH_CGPG,
                                   hResilience,
                                   hVm, 
                                   hEpInfo );
    if (cgep) {
      /* common gain */
      int t;
      Hcb *hcb;
      Huffman *hcw;

      /*  get ind_sw_cce flag */
      ind_sw_cc = mip->GetBits(IND_SW_CCE_FLAG,  
                               LEN_IND_SW_CCEG,
                               hResilience,
                               hVm, 
                               hEpInfo );

      /*  get just one scale factor */
      hcb = &book[BOOKSCL];
      hcw = hcb->hcw;
      fac = 0;    /* dpcm relative to 0 */
      t = decode_huff_cw(hcw, 
                         hResilience,
                         hVm, 
                         0, 
                         hEpInfo);
      fac += t - MIDFAC;    /* 1.5 dB */

      /* recover stepsize */
      scale = cc_gain_scale[scl_idx];
      scale = pow(scale, fac);

      /* copy to gain array */
      n = shared[ch] ? 2 : 1;
      for (m=0; m<n; m++) {
        k=0;
        for (i=0; i<info->nsbk; i++)
          for (j=0; j<info->sfb_per_sbk[i]; j++)
            cc_gain[cidx][target[ch]][k++] = scale;
        ch++;
      }
    }
    else {
      /* must be dependently switched cce */
      ind_sw_cc = 0;
	    
      /* get scale factors
       * use sectioning of coupling channel
       */
      hufffac ( info, 
                cc_group[cidx], 
                nsect, 
                sect, 
                global_gain, 
                factors,
                hVm,
                hResilience, 
                hEpInfo );

      /* recover sign and stepsize */
      scale = cc_gain_scale[scl_idx];
      k=0;
      for (i=0; i<info->nsbk; i++) {
        for (j=0; j<info->sfb_per_sbk[i]; j++) {
          fac = factors[k];
          if (cc_gain_ele_sign) {
            sign = fac & 1;
            fac >>= 1;
          }
          else {
            sign = 1;
          }
          scale = pow(scale, fac);
          scale *= (sign==0) ? 1 : -1;
          cc_gain[cidx][target[ch]][k++] = scale;
        }
      }
      /* shared gain lists */
      if ( shared[ch] ) {
        for (i=0; i<MAXBANDS; i++)
          cc_gain[cidx][target[ch+1]][k] = 
            cc_gain[cidx][target[ch]][k];
        ch++;
      }
    }
  }

  /* process coupling channel the same as other channels, 
   * except that it can only be a SCE
   */
  if (nok_use_monopred) {
    predict(info, mip->profile,
            cc_lpflag[cidx], cc_sp_status[cidx],
            cc_prev_quant[cidx], cc_coef[cidx]);
    predict_reset(info, cc_prstflag[cidx], cc_sp_status, cc_prev_quant, 
                  cidx, cidx);
  }
  else {
    nok_predict(info, mip->profile,
                cc_lpflag[cidx], nok_cc_sp_status[cidx],
                cc_prev_quant[cidx], cc_coef[cidx]);
    nok_predict_reset(info, cc_prstflag[cidx], nok_cc_sp_status,
                      cidx, cidx);
  }

  /* Long term prediction.  */
  nok_lt_predict ( cc_wnd_shape[cidx].this_bk,
                   nok_cc_ltp[cidx]->sbk_prediction_used,
                   nok_cc_ltp[cidx]->sfb_prediction_used,
                   nok_cc_ltp[cidx]->weight,
                   nok_cc_ltp[cidx]->delay, cc_coef[cidx],
                   BLOCK_LEN_LONG, BLOCK_LEN_MEDIUM, BLOCK_LEN_SHORT );

  for (i=j=0; i<tns.n_subblocks; i++) {
    tns_decode_subblock( cc_coef[cidx] + j,
                         info->sfb_per_sbk[i],
                         info->sbk_sfb_top[i],
                         &(tns.info[i]) );
    j += info->bins_per_sbk[i];
  }

  /* invert the cch to ch mapping */
  for (i=0; i<nch; i++) {
    mip->ch_info[target[i]].ncch = cidx+1;
    mip->ch_info[target[i]].cch[cidx] = cidx;
    mip->ch_info[target[i]].cc_dom = cc_dom;
    mip->ch_info[target[i]].cc_ind[cidx] = ind_sw_cc;
  }

  if (default_config)
    mip->ncch++;
	
  return 1;
}

#if (ICChans > 0)
/* transform independently switched coupling channels into
 * time domain
 */
void
ind_coupling(MC_Info *mip, byte *wnd, Wnd_Shape *wnd_shape,
             byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef, Float **cc_state)
{
  int i, j, cidx, dep, ch, widx;
  for (cidx=0; cidx<mip->ncch; cidx++)
    if (mip->cc_ind[cidx]) {
      Float data[LN2];
      freq2time_adapt(cc_wnd[wn], &cc_wnd_shape[wn], cc_coef[cidx],
                      cc_state[cidx], data);
      fltcpy(cc_coef[cidx], data, LN2);
    }
}

#endif
	

static void
mix_cc(Float *coef, Float *cc_coef, Float *cc_gain, int ind)
{
  int nsbk, sbk, sfb, nsfb, k, top;
  Float scale;
  Info info = winmap[win];

  if (!ind) {    
    /* frequency-domain coupling */
    k = 0;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) {
      nsfb = info->sfb_per_sbk[sbk];
      for (sfb=0; sfb<nsfb; sfb++) {
        top = info->sbk_sfb_top[sbk][sfb];
        scale = *cc_gain++;
        if (scale == 0) {
          /* no coupling */
          k = top;
        }
        else {
          /* mix in coupling channel */
          while (k<top) {
            coef[k] += cc_coef[k] * scale;
            k++;
          }
        }
      }
    }
  }
  else {
    /* time-domain coupling (coef[] is actually data[]!) */
    scale = *cc_gain;
    for (k=0; k<LN2; k++) 
      coef[k] += data[k] * scale;
  }	
}
   
void
coupling ( Info *info, MC_Info *mip, Float **coef, Float **cc_coef, Float *cc_gain[CChans][Chans], int ch, int cc_dom, int cc_ind )
{
  int i, j, wn, cch, ind;    
  Ch_Info *cip = &mip->ch_info[ch];
       
  j=cip->ncch;
  for (i=0; i<j; i++) {
    if (!cc_ind && (cc_dom != cip->cc_dom[i]) )
      continue;
    wn = cip->widx;
    cch = cip->cch[i];
    ind = cip->cc_ind[i];
    if (debug['c'])
      printf( "mixing cch %d onto ch %d, mode %d %d\n",
            cch, ch, cc_dom, dep);
    mix_cc(coef[ch], cc_coef[cch], cc_gain[cch][ch], ind);
  }
}

#endif	/* (CChans > 0) */
