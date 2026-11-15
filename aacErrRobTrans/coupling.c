/*
  "This software module was originally developed by 
  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
  development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
  Copyright(c)1996.
*/

#include "all.h"
#if (CChan > 0)

static	Float	    cc_gain_scale[4];
static	int	    *cc_lpflag[CChans];
static	int	    *cc_prstflag[CChans];
static	PRED_STATUS *cc_sp_status[CChans];
static	Float	    *cc_prev_quant[CChans];
static	byte	    *cc_group[CChans];

void init_cc ( )
{
    int i, ch;
    cc_gain_scale[3] = 2.0;

    /* initialize gain scale */
    for (i=2; i>=0; i--)
	cc_gain_scale[i] = sqrt(cc_gain_scale[i+1]);

    /* coupling channel predictors */
    for (ch = 0; ch < CChans; ch++) {
	cc_lpflag[ch] = (int*)mal1(MAXBANDS*sizeof(*cc_lpflag[0]));
	cc_prstflag[ch] = (int*)mal1((LEN_PRED_RSTGRP+1)*sizeof(*cc_prstflag[0]));
	cc_sp_status[ch] = (PRED_STATUS*)mal1(LN*sizeof(*cc_sp_status[0]));
	cc_prev_quant[ch] = (Float*)mal1(LN2*sizeof(*cc_prev_quant[0]));
	cc_group[ch] = (byte*)mal1(MAXBANDS*sizeof(*cc_group[0]));
	for (i = 0; i < LN2; i++) {
	    init_pred_stat(&cc_sp_status[ch][i],
	        PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
	    cc_prev_quant[ch][i] = 0.;
	}
    }
}

int getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef,
          Float *cc_gain[CChans][Chans])
{
  int i, j, k, m, n, cpe, tag, cidx, ch, nele, nch;
  int target[Chans], shared[Chans];
  int cc_l, cc_r, cc_dom, cc_gain_ele_sign, ind_sw_cc, scl_idx,
    sign, nsect, fac;
  byte sect[2*MAXBANDS+1], cc_max_sfb[CChans];
  unsigned short global_gain, factors[MAXBANDS];
  unsigned long com_gain_elem_pres;
  Float scale;
  Info *info;
  TNS_frame_info tns;
  
  tag = getbits(LEN_TAG);
#ifdef ID_CCE_WRITE_SUPPORT
  WriteElemToBuffer(ELEMENT_INSTANCE_TAG, tag, LEN_TAG);
#endif
  
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
    PRINT(SE, "Unanticipated coupling channel\n");
    myexit("coupling");
  }
  
  /* coupled (target) elements */
  nele = getbits(LEN_NCC);
#ifdef ID_CCE_WRITE_SUPPORT
  WriteElemToBuffer(NUM_COUPLED_ELEMENTS, nele, LEN_NCC);
#endif
  nch = 0;
  for (i=0; i<nele; i++) {
    cpe = getbits(LEN_IS_CPE);
    WriteElemToBuffer(CC_TARGET_IS_CPE, cpe, LEN_IS_CPE);
    tag = getbits(LEN_TAG);
    WriteElemToBuffer(CC_TARGET_TAG_SELECT, tag, LEN_TAG);
    ch = ch_index(mip, cpe, tag);
    
    if (!cpe) {
      target[nch] = ch;
      shared[nch++] = 0;
    }
    else {
      cc_l = getbits(LEN_CC_LR);
#ifdef ID_CCE_WRITE_SUPPORT
      WriteElemToBuffer(CC_L, cc_l, LEN_CC_LR);
#endif
      cc_r = getbits(LEN_CC_LR);
#ifdef ID_CCE_WRITE_SUPPORT
      WriteElemToBuffer(CC_R, cc_r, LEN_CC_LR);
#endif
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
  
  cc_com = getbits(LEN_CC_DOM) != CC_AFTER_TNS;
#ifdef ID_CCE_WRITE_SUPPORT
  WriteElemToBuffer(CC_DOMAIN, cc_com, LEN_CC_DOM); 
#endif      
  cc_gain_ele_sign = getbits(LEN_CC_SGN);
#ifdef ID_CCE_WRITE_SUPPORT
  WriteElemToBuffer(GAIN_ELEMENT_SIGN, cc_gain_ele_sign, LEN_CC_SGN);
#endif      
  scl_idx = getbits(LEN_CCH_GES);
#ifdef ID_CCE_WRITE_SUPPORT
  WriteElemToBuffer(GAIN_ELEMENT_SCALE, scl_idx, LEN_CCH_GES);
#endif      
  
  /*
   * coupling channel bitstream
   * (equivalent to SCE)
   */
  fltclr(cc_coef[cidx], LN2);
  memcpy(&info, winmap[cc_wnd[cidx]], sizeof(Info));
  if (!getics(info, 0, &cc_wnd[cidx], &cc_wnd_shape[cidx].this_bk,
              cc_group[cidx], cc_max_sfb[cidx], cc_lpflag[cidx], cc_prstflag[cidx],
              &nsect, sect, cc_coef[cidx], &global_gain, factors, &tns))
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
    com_gain_elem_pres = getbits(LEN_CCH_CGP);
#ifdef ID_CCE_WRITE_SUPPORT
    WriteElemToBuffer(COMMON_GAIN_ELEMENT_PRESENT, com_gain_elem_pres, LEN_CCH_CGP);
#endif      
    if (com_gain_elem_pres) {
      /* common gain */
      int t;
      Hcb *hcb;
      Huffman *hcw;
      
      /*  get ind_sw_cce flag */
      ind_sw_cc = mip->getbits(LEN_IND_SW_CCE);
#ifdef ID_CCE_WRITE_SUPPORT
      WriteElemToBuffer(IND_SW_CCE_FLAG, ind_sw_cc, LEN_IND_SW_CCE);
#endif      
      /*  get just one scale factor */
      hcb = &book[BOOKSCL];
      hcw = hcb->hcw;
      fac = 0;    /* dpcm relative to 0 */
      t = decode_huff_cw(hcw);
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
      hufffac(info, cc_group[cidx], nsect, sect, global_gain, factors);
      
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
  predict(info, mip->profile,
          cc_lpflag[cidx], cc_sp_status[cidx],
          cc_prev_quant[cidx], cc_coef[cidx]);
  predict_reset(info, cc_prstflag[cidx], cc_sp_status, cc_prev_quant, 
                cidx, cidx);
  
  for (i=j=0; i<tns.n_subblocks; i++) {
    tns_decode_subblock( cc_coef[cidx] + j,
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

#if (ICChan > 0)
/* transform independently switched coupling channels into
 * time domain
 */
void
ind_coupling(MC_Info *mip, byte *wnd, Wnd_Shape *wnd_shape,
    byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float *cc_coef, Float *cc_state)
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
coupling(MC_Info *mip, Float **coef, Float **cc_coef, Float **cc_gain,  
    int ch, int cc_dom, int cc_ind)
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
	    PRINT(SE, "mixing cch %d onto ch %d, mode %d %d\n",
		cch, ch, cc_dom, dep);
	mix_cc(coef[ch], cc_coef[cch], cc_gain[cch][ch], ind);
    }
}

#endif	/* (CChan > 0) */
