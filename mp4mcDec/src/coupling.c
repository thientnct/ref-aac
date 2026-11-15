/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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

#include "config.h"
#include "coupling.h"
#include "diagnose.h"
#include "dolby_adapt.h"
#include "huffdec_intern.h"
#include "huffinit.h"
#include "intrins.h"
#include "monopred.h"
#include "bitinput.h"
#include "tns.h"

#if (CChans > 0)

static	Float	    cc_gain_scale[4];
static	int	    *cc_lpflag[CChans];
static	int	    *cc_prstflag[CChans];
static	PRED_STATUS *cc_sp_status[CChans];
#ifdef MPEG4V1
static  Float *cc_data[CChans], *cc_state[CChans];
static	NOK_LT_PRED_STATUS *nok_cc_ltp_status[CChans];
#endif
static	Float	    *cc_prev_quant[CChans];
static	byte	    *cc_group[CChans];
static int	    target[Chans];
static byte	    cb_map[MAXBANDS];
static int last_cc_rstgrp_num[CChans] = {0};
static int cc_have_malloc = 0;

void
init_cc(void)
{
    int i, ch;
    cc_gain_scale[3] = 2.0;

    /* initialize gain scale */
    for (i=2; i>=0; i--)
	cc_gain_scale[i] = sqrt(cc_gain_scale[i+1]);

    /* coupling channel predictors */
    for (ch = 0; ch < CChans; ch++) {
#ifdef MPEG4V1
	if (!cc_have_malloc) {
	  cc_data[ch] = (Float *)mal1(LN2*sizeof(*cc_data[0]));
	  cc_state[ch] = (Float *)mal1(LN*sizeof(*cc_state[0]));
	}
	for (i = 0; i < LN2; i++)
	  cc_data[ch][i] = cc_state[ch][i] = 0.0f;
#endif
	if (!cc_have_malloc) {
	  cc_lpflag[ch] = (int*)mal1(MAXBANDS*sizeof(*cc_lpflag[0]));
	  cc_prstflag[ch] = (int*)mal1((LEN_PRED_RSTGRP+1)*sizeof(*cc_prstflag[0]));
	  cc_sp_status[ch] = (PRED_STATUS*)mal1(LN*sizeof(*cc_sp_status[0]));
	  cc_prev_quant[ch] = (Float*)mal1(LN2*sizeof(*cc_prev_quant[0]));
	  cc_group[ch] = (byte*)mal1(MAXBANDS*sizeof(*cc_group[0]));
	}
	for (i = 0; i < LN2; i++) {
	    init_pred_stat(&cc_sp_status[ch][i],
	        PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
	    cc_prev_quant[ch][i] = 0.;
	}
#ifdef MPEG4V1
    /* coupling channel long term prediction.  */
	if (!cc_have_malloc) nok_cc_ltp_status[ch]  = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_cc_ltp_status[0]));
	nok_init_lt_pred (nok_cc_ltp_status[ch]);
#endif
    }
    cc_have_malloc = 1;
}

void destroyCC(void)
{
  unsigned short int ch;
  for (ch = 0; ch < CChans; ch++) {
#ifdef MPEG4V1
    free1 (cc_data[ch]);
    free1 (cc_state[ch]);
    free1 (nok_cc_ltp_status[ch]);
#endif
    free1 (cc_lpflag[ch]);
    free1 (cc_prstflag[ch]);
    free1 (cc_sp_status[ch]);
    free1 (cc_prev_quant[ch]);
    free1 (cc_group[ch]);
  }
}

/*
 * get scale factors from simpler cb_map information
 */
static int cc_hufffac(Info *info, byte *group, byte *cb_map, 
#ifdef MPEG4V1
short global_gain,
#endif
short *factors)
{
    Hcb *hcb;
    Huffman *hcw;
    int i, b, bb, t, n, fac, is_pos;
#ifdef MPEG4V1
    int noise_pcm_flag = 1;
    int noise_nrg;
#endif

    /* clear array for the case of max_sfb == 0 */
    shortclr(factors, MAXBANDS);

    /* scale factors are dpcm relative to global gain
     * intensity positions are dpcm relative to zero
     */
    fac = 0;
    is_pos = 0;
#ifdef MPEG4V1
    noise_nrg = global_gain - NOISE_OFFSET;
#endif

    /* get scale factors */
    hcb = &book[BOOKSCL];
    hcw = hcb->hcw;
    bb = 0;
    if (debug['f'])
	PRINT(SE,"scale factors\n");
    for(b = 0; b < info->nsbk; ){
	n = info->sfb_per_sbk[b];
	b = *group++;
	for(i = 0; i < n; i++){
	    switch (cb_map[i]) {
	    case ZERO_HCB:	    /* zero book */
		factors[i] = fac;	/* no difference from band to band for zero length codes */
		break;
	    default:		    /* spectral books */
		/* decode scale factor */
		t = decode_huff_cw(hcw);
		if(t >= MAXFAC || t < 0)
		    return 0;
		fac += t - MIDFAC;    /* 1.5 dB */
		factors[i] = fac;
		break;
	    case BOOKSCL:	    /* invalid books */
#ifndef MPEG4V1
	    case RESERVED_HCB:
#endif
		return 0;
	    case INTENSITY_HCB:	    /* intensity books */
	    case INTENSITY_HCB2:
		/* decode intensity position */
		t = decode_huff_cw(hcw);
		is_pos += t - MIDFAC;
		factors[i] = is_pos;
		break;
#ifdef MPEG4V1
	    case NOISE_HCB:	    /* noise books */
                /* decode noise energy */
                if (noise_pcm_flag) {
                  noise_pcm_flag = 0;
                  t = getbits( NOISE_PCM_BITS ) - NOISE_PCM_OFFSET;
                }
                else
                  t = decode_huff_cw(hcw) - MIDFAC;
                noise_nrg += t;
                if (debug['f'])
                  PRINT(SE,"\n%3d %3d (noise, code %d)", i, noise_nrg, t);
                factors[i] = noise_nrg;
                break;
#endif


    	    }
	    if (debug['f'])
	       PRINT(SE,"%3d: %3d %3d\n", i, cb_map[i], factors[i]);
	}
	if (debug['f'])
	   PRINT(SE,"\n");

	/* expand short block grouping */
	if (!(info->islong)) {
	    for(bb++; bb < b; bb++) {
		for (i=0; i<n; i++) {
		    factors[i+n] = factors[i];
		}
		factors += n;
	    }
	}
	cb_map += n;
	factors += n;
    }
    return 1;
}

int
getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef,
Float *cc_gain[CChans][Chans][1<<LEN_NCC])
{
    int i, j, k, m, n, cpe, tag, cidx, ch, nele, nch;
    int shared[Chans];
    int cc_l, cc_r, cc_dom, cc_gain_ele_sign, ind_sw_cc, cgep, scl_idx,
    sign, fac;
    byte cc_max_sfb[CChans];
    short global_gain, factors[MAXBANDS];
    Float scale, cc_scale;
    TNS_frame_info tns;
    int target_multiple[Chans]; /* if coupling channel goes several times onto one channel,
                                   keep the gains separately */

    int dummy = 0;

    Info infodata;
    Info *info = &infodata;

    for (i=0;i<Chans;i++)
      target_multiple[i]=0;

    tag = getbits(LEN_TAG);

    if (default_config) {
	cidx = mip->ncch;
    }
    else {
	cidx = XCChans;	    /* default is scratch index */
	for (i=0; i<mip->ncch; i++) {
	    if (mip->cc_tag[i] == tag)
		cidx = i;
	}
    }
    
    if (cidx >= CChans) {
	PRINT(SE, "Unanticipated coupling channel\n");
	myexit("coupling");
    }

    if (cidx==XCChans) set_log_level(ID_NUL, LEN_TAG, tag);
    else set_log_level(ID_CCE, LEN_TAG, tag);  /* required for CRC and ADIF2MP4 */

    /*  get ind_sw_cce flag */
    ind_sw_cc = getbits(LEN_IND_SW_CCE);
    mip->cc_ind[cidx] = ind_sw_cc;

    if (debug['c'])
	PRINT(SE, "cch %d is tag %d, ind %d\n", cidx, tag, ind_sw_cc);
    
    /* coupled (target) elements */
    nele = getbits(LEN_NCC);
    nch = 0;
    for (i=0; i<nele+1; i++) {
	cpe = getbits(LEN_IS_CPE);
	tag = getbits(LEN_TAG);
	ch = ch_index(mip, cpe, tag);
	if (debug['c'])
	    PRINT(SE, "target ch %d tag %d cpe %d\n", ch, tag, cpe);
	
	if (!cpe) {
	    target[nch] = ch;
	    shared[nch++] = 0;
	}
	else {
	    cc_l = getbits(LEN_CC_LR);
	    cc_r = getbits(LEN_CC_LR);
	    if (debug['c'])
		PRINT(SE, "cc_l cc_r %d %d\n", cc_l, cc_r);
	    j = (cc_r<<1) | cc_l;  /* 0: l=0, r=0
                                      1: l=1, r=0
                                      2: l=0, r=1
                                      3: l=1, r=1 */
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
                target[nch] = mip->ch_info[ch].paired_ch;
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
    
    cc_dom = getbits(LEN_CC_DOM);
    cc_gain_ele_sign = getbits(LEN_CC_SGN);
    scl_idx = getbits(LEN_CCH_GES);

    /*
     * coupling channel bitstream
     * (equivalent to SCE)
     */
    fltclr(cc_coef[cidx], LN2);

    if (!getics(info, 0, &cc_wnd[cidx], &cc_wnd_shape[cidx].this_bk,
                cc_group[cidx], &cc_max_sfb[cidx], cc_lpflag[cidx], cc_prstflag[cidx],
                cb_map, cc_coef[cidx], &global_gain, factors,
#ifdef MPEG4V1
                nok_cc_ltp_status[cidx],
#endif
                &tns, dummy /*ch*/, dummy /*max_spectral_line*/
		))
	return -1;

    /* coupling for first target channel(s) is already at
     * correct scale
     */
    ch = shared[0] ? 2 : 1;
    for (j=0; j<ch; j++) {
	for (i=0; i<MAXBANDS; i++)
	    cc_gain[cidx][target[j]][target_multiple[target[j]]][i] = 1.0;
        target_multiple[target[j]]++;
    }

    /*
     * bitstreams for target channel scale factors
     */
    for (; ch<nch; ch++) {
	/* if needed, get common gain elment present */
	cgep = ind_sw_cc ? 1 : getbits(LEN_CCH_CGP);
	if (cgep) {
	    /* common gain */
	    int t;
	    Hcb *hcb;
	    Huffman *hcw;

	    /*  get just one scale factor */
	    hcb = &book[BOOKSCL];
	    hcw = hcb->hcw;
	    t = decode_huff_cw(hcw);
	    fac = t - MIDFAC;

	    /* recover stepsize */
	    scale = cc_gain_scale[scl_idx];
	    scale = pow(scale, -fac);
	    if (debug['c'])
		PRINT(SE, "target %d common gain element scale %f\n", ch, scale);

	    /* copy to gain array */
	    n = shared[ch] ? 2 : 1;
	    for (m=0; m<n; m++) {
		k=0;
		for (i=0; i<info->nsbk; i++)
		    for (j=0; j<info->sfb_per_sbk[i]; j++)
			cc_gain[cidx][target[ch]][target_multiple[target[ch]]][k++] = scale;
                target_multiple[target[ch]]++;
		if (n == 2)
		    ch++;
	    }
	}
	else {
	    /* must be dependently switched cce */
	    
	    /* get scale factors
	     * use sectioning of coupling channel
	     */
	    if (!cc_hufffac(info, cc_group[cidx], cb_map, 
#ifdef MPEG4V1
                            global_gain,
#endif
                            factors)) {
		if (debug['c'])
		    PRINT(SE, "error in unpacking coupling coordinates:  cc_hufffac\n");
		return 0;
	    }

	    /* recover sign and stepsize */
	    cc_scale = cc_gain_scale[scl_idx];
	    k=0;
	    for (i=0; i<info->nsbk; i++) {
                for (j=0; j<info->sfb_per_sbk[i]; j++) {
		    fac = factors[k];
		    if (cc_gain_ele_sign) {
			sign = 1-(fac & 1);
			fac >>= 1;
		    }
		    else {
			sign = 1;
		    }
		    scale = pow(cc_scale, -fac);
		    scale *= (sign==0) ? -1 : 1;
		    cc_gain[cidx][target[ch]][target_multiple[target[ch]]][k++] = scale;
                }
	    }
	    /* shared gain lists */
            target_multiple[target[ch]]++;
	    if ( shared[ch] ) {
		for (i=0; i<MAXBANDS; i++)
		    cc_gain[cidx][target[ch+1]][target_multiple[target[ch+1]]][i] = 
		        cc_gain[cidx][target[ch]][target_multiple[target[ch]]-1][i];
		ch++;
                target_multiple[target[ch]]++;
	    }
	}
    }

#ifdef CPLDEBUG
    if (debug['c']) {
	/* print gains for each target channel */
	PRINT(SE, "coupling channel idx %d\n", cidx);
	for (ch=0; ch<nch; ch++) {
	    PRINT(SE, "target channel idx %d\n", ch);
            for (n=0; n<target_multiple[target[ch]]; n++) {
                k=0;
                for (i=0; i<info->nsbk; i++) {
		    for (j=0; j<info->sfb_per_sbk[i]; j++)
		        PRINT(SE, "%3d %3d %3f\n", i, j, cc_gain[cidx][target[ch]][n][k++]);
                }
	    }
	}
    }
#endif

    for (ch=0; ch<Chans; ch++) {
      if (target_multiple[ch]>1) {
        PRINT(SE, "WARNING: coupling channel idx %d is mixed %i times onto channel %d!\n",cidx,target_multiple[ch],ch);
      }
    }

#ifdef MPEG4V1
    if(mc_info.profile == LTP_Profile)
      nok_lt_predict (info, cc_wnd[cidx], cc_wnd_shape[cidx].this_bk,
		      cc_wnd_shape[cidx].prev_bk,
		      nok_cc_ltp_status[cidx]->sbk_prediction_used,
		      nok_cc_ltp_status[cidx]->sfb_prediction_used,
		      nok_cc_ltp_status[cidx], nok_cc_ltp_status[cidx]->weight,
		      nok_cc_ltp_status[cidx]->delay, cc_coef[cidx],
		      BLOCK_LEN_LONG, &tns);
    else
#endif
    {
      /* process coupling channel the same as other channels, 
       * except that it can only be a SCE
       */
      predict(info, mip->profile,
	      cc_lpflag[cidx], cc_sp_status[cidx], cc_coef[cidx]);
      predict_reset(info, cc_prstflag[cidx], cc_sp_status, cidx, cidx, last_cc_rstgrp_num, cc_lpflag[cidx]);
    }

    for (i=j=0; i<tns.n_subblocks; i++) {
	tns_decode_subblock(&cc_coef[cidx][j],
	    cc_max_sfb[cidx], 
	    info->sbk_sfb_top[i],
	    info->islong,
	    &(tns.info[i]) );
	j += info->bins_per_sbk[i];
    }
    
    /* invert the cch to ch mapping */
    if(cidx != XCChans) {
        for (i=0;i<Chans;i++)
            target_multiple[i]=0;
	for (i=0; i<nch; i++) {
	    j = mip->ch_info[target[i]].ncch++;
	    mip->ch_info[target[i]].cch[j] = cidx;
            mip->ch_info[target[i]].cc_multi[j] = target_multiple[target[i]]++;
	    mip->ch_info[target[i]].cc_dom[j] = cc_dom;
	    mip->ch_info[target[i]].cc_ind[j] = ind_sw_cc;
	}
    }

    if (default_config)
	mip->ncch++;

    set_log_level(LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */
    
    return 1;
}


#if (ICChans > 0)
/* transform independently switched coupling channels into
 * time domain
 */
void
ind_coupling(MC_Info   *mip, 
             byte      *cc_wnd, 
             Wnd_Shape *cc_wnd_shape, 
             Float     *cc_coef[CChans],
             Float     *cc_state[ICChans])
{
    int cidx;

    for (cidx=0; cidx<mip->ncch; cidx++) {
	if (mip->cc_ind[cidx]) {
	    Float data[LN2];
	    freq2time_adapt(cc_wnd[cidx], &cc_wnd_shape[cidx],
		cc_coef[cidx], cc_state[cidx], data);
	    fltcpy(cc_coef[cidx], data, LN2);
#ifdef MPEG4V1
	    if(mc_info.profile == LTP_Profile)
	      nok_lt_update(nok_cc_ltp_status[cidx], cc_coef[cidx], cc_state[cidx], BLOCK_LEN_LONG);
#endif
	}
    }
}
#endif
	

static void mix_cc(Info *info, Float *coef, Float *cc_coef, Float *cc_gain, int ind)
{
    int nsbk, sbk, sfb, nsfb, k, top;
    Float scale;

    if (!ind) {    
	/* frequency-domain coupling */
	k = 0;
	nsbk = info->nsbk;
	for (sbk=0; sbk<nsbk; sbk++) {
	    nsfb = info->sfb_per_sbk[sbk];
	    for (sfb=0; sfb<nsfb; sfb++) {
                            top = info->bk_sfb_top[sbk*nsfb+sfb];
		scale = *cc_gain++;
/* if (cb_map[sfb] == 0) PRINT(SE, "ZERO_HCB %d %d %d\n", nsbk, sbk, sfb); */		
		if ((scale == 0) || (cb_map[sfb] == ZERO_HCB)) {
		    k = top;
		}
		else {
                    /* mix in coupling channel */
		    for (; k<top; k++) {
			coef[k] += cc_coef[k] * scale;
		    }
		}
	    }
	}
    }
    else {
	/* time-domain coupling (coef[] is actually data[]!) */
	scale = *cc_gain;
	for (k=0; k<LN2; k++) {
	    coef[k] += cc_coef[k] * scale;
	}    
    }
}
   

void
coupling(Info *info, MC_Info *mip, Float **coef, Float **cc_coef,
    Float *cc_gain[CChans][Chans][1<<LEN_NCC], int ch, int cc_dom, int cc_ind)
{
    int i, j, wn, cch, ind, k;    
    Ch_Info *cip = &mip->ch_info[ch];
    int mix;
       
    j=cip->ncch;
    for (i=0; i<j; i++) {
	/* Boolean logic to determine whether to mix coupling channel */
	mix = (cc_ind && cip->cc_ind[i]) || 
	   (!cc_ind && !cc_dom && !cip->cc_ind[i] && !cip->cc_dom[i]) ||
	   (!cc_ind && cc_dom && !cip->cc_ind[i] && cip->cc_dom[i]);

	if (mix) {
	    wn = cip->widx;
	    cch = cip->cch[i];
	    ind = cip->cc_ind[i];
            k = cip->cc_multi[i];
	    if (debug['c'])
		PRINT(SE, "mixing cch %d onto ch %d, mode %d %d (%d)\n",
		    cch, ch, cc_dom, cc_ind, k);
	    mix_cc(info, coef[ch], cc_coef[cch], cc_gain[cch][ch][k], ind);
	}
    }
}

#endif	/* (CChan > 0) */
