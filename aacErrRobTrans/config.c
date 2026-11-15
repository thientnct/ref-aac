/*
  "This software module was originally developed by 

  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS
  
  and edited by

  Thomas Schaefer (Fraunhofer Gesellschaft IIS)
  
  in the course of 
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
  Copyright(c)1996, 1997, 1998.
*/

#include "all.h"
#include "escHandling.h"

/* 
 * profile dependent parameters
 */
int
tns_max_bands(int islong)
{
    int i;

    if (low_delay) {
      i = frame_length_flag ? 0 : 1;
      return tns_max_bands_tbl_low_delay[mc_info.sampling_rate_idx][i];
    } else {
      /* construct second index */
      i = islong ? 0 : 1;
      i += (mc_info.profile ==  SSR_Profile) ? 2 : 0;
      return tns_max_bands_tbl[mc_info.sampling_rate_idx][i];
    }
}

int
tns_max_order(int islong)
{
    if (islong) {
	switch (mc_info.profile) {
	case Main_Profile:
	    return 20;
	case LC_Profile:
	case SSR_Profile:
	    return 12;
	}
    }
    else {
	/* short window */
	return 7;
    }
    return 0;
}

int pred_max_bands ( )
{   
    return pred_max_bands_tbl[mc_info.sampling_rate_idx];
}

/*
 * adif_header
 */
int get_adif_header ( )
{
    int i, n;
    int tag = 0;
    int select_status;
    ProgConfig tmp_config;
    ADIF_Header *p = &adif_header;
    unsigned long    adif_buf_full;
    
    /* adif header */
    for (i=0; i<LEN_ADIF_ID; i++) {
      p->adif_id[i] = getbits(LEN_BYTE); 
      /* WriteElemToBuffer(ADIF_ID, p->adif_id[i], LEN_BYTE); */
    }
    p->adif_id[i] = 0;	    /* null terminated string */
    /* test for id */
    if (strncmp(p->adif_id, "ADIF", 4) != 0)
	return -1;	    /* bad id */
	
    /* copyright string */
    getbits(LEN_COPYRT_PRES);
    /*WriteElemToBuffer(COPYRIGHT_ID_PRESENT, p->copy_id_present, LEN_COPYRT_PRES);	*/
    if ( p->copy_id_present == 1 ) {
      for (i=0; i<LEN_COPYRT_ID; i++) {
        p->copy_id[i] = getbits(LEN_BYTE); 
        /*WriteElemToBuffer(COPYRIGHT_ID, p->copy_id[i], LEN_BYTE);*/
      }
      p->copy_id[i] = 0;  /* null terminated string */
    }
    p->original_copy = getbits(LEN_ORIG);
    /*WriteElemToBuffer(ORIGINAL_COPY, p->original_copy, LEN_ORIG);*/
    p->home = getbits(LEN_HOME);
    /* WriteElemToBuffer(HOME,p->home,LEN_HOME); */
    p->bitstream_type = getbits(LEN_BS_TYPE);
    /* WriteElemToBuffer(BITSTREAM_TYPE, p->bitstream_type, LEN_BS_TYPE); */
    p->bitrate = getbits(LEN_BIT_RATE);
    /* WriteElemToBuffer(BITRATE, p->bitrate, LEN_BIT_RATE); */

    /* program config elements */ 
    select_status = -1; 
      
    n = getbits(LEN_NUM_PCE);
    /* WriteElemToBuffer(NUM_PROGRAM_CONFIG_ELEMENTS, n, LEN_NUM_PCE); */
    n = n + 1;
    for (i=0; i<n; i++) {
        adif_buf_full = getbits(LEN_ADIF_BF);
        /* WriteElemToBuffer(ADIF_BUFFER_FULLNESS, adif_buf_full, LEN_ADIF_BF); */
	tmp_config.buffer_fullness =
	    (p->bitstream_type == 0) ? adif_buf_full : 0;
	tag = get_prog_config(&tmp_config);
	if (current_program < 0)
	    current_program = tag;	    /* default is first prog */
	if (current_program == tag) {
	    memcpy(&prog_config, &tmp_config, sizeof(prog_config));
	    select_status = 1;
	}
    }
    if (debug['v']) {
	PRINT(SE, "\nADIF header:\n");
	PRINT(SE, "%s %d\n", "p->copy_id_present", p->copy_id_present);
	PRINT(SE, "%s %d\n", "p->original_copy", p->original_copy);
	PRINT(SE, "%s %d\n", "p->home", p->home);
	PRINT(SE, "%s %d\n", "p->bitstream_type", p->bitstream_type);
	PRINT(SE, "%s %ld\n", "p->bitrate", p->bitrate);
	PRINT(SE, "%s %d\n", "number of PCE", n);
	PRINT(SE, "Select status %d for PCE tag %d\n", select_status, tag);
	if (select_status) {
	    ProgConfig *p = &prog_config;
	    PRINT(SE, "%s %d\n", "p->profile", p->profile);
	    PRINT(SE, "%s %d\n", "p->sampling_rate_idx", p->sampling_rate_idx);
	    PRINT(SE, "%s %d\n", "p->front.num_ele", p->front.num_ele);
	    PRINT(SE, "%s %d\n", "p->side.num_ele", p->side.num_ele);
	    PRINT(SE, "%s %d\n", "p->back.num_ele", p->back.num_ele);
	    PRINT(SE, "%s %d\n", "p->lfe.num_ele", p->lfe.num_ele);
	    PRINT(SE, "%s %d\n", "p->data.num_ele", p->data.num_ele);
	    PRINT(SE, "%s %d\n", "p->coupling.num_ele", p->coupling.num_ele);
	    PRINT(SE, "%s %d\n", "p->mono_mix.present", p->mono_mix.present);
	    PRINT(SE, "%s %d\n", "p->stereo_mix.present", p->stereo_mix.present);
	    PRINT(SE, "%s %d\n", "p->matrix_mix.present", p->matrix_mix.present);
	    
	    PRINT(SE, "Comment: %s\n", prog_config.comments);
	}
    }

    return select_status;
}


/*
 * program configuration element
 */
static void
get_ele_list(EleList *p, int enable_cpe)
{  
    int i, j;
    for (i=0, j=p->num_ele; i<j; i++) {
      if (enable_cpe) {
	    p->ele_is_cpe[i] = getbits(LEN_ELE_IS_CPE);
        /*WriteElemToBuffer(FRONT_SIDE_BACK_ELEMENT_IS_CPE, p->ele_is_cpe[i], LEN_ELE_IS_CPE);*/
      }
      p->ele_tag[i] = getbits(LEN_TAG);
      /*WriteElemToBuffer(FRONT_SIDE_BACK_LFE_DATA_CC_ELEMENT_TAG_SELECT, p->ele_tag[i], LEN_TAG);*/
    }
}

int get_prog_config(ProgConfig *p)
{
    int i, j, tag;

    tag = getbits(LEN_TAG);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(ELEMENT_INSTANCE_TAG, tag, LEN_TAG);
#endif
    p->profile = getbits(LEN_PROFILE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(PROFILE, p->profile, LEN_PROFILE);
#endif
    p->sampling_rate_idx = getbits(LEN_SAMP_IDX);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(SAMPLING_FREQUENCY_INDEX, p->sampling_rate_idx, LEN_SAMP_IDX);
#endif
    p->front.num_ele = getbits(LEN_NUM_ELE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_FRONT_CHANNEL_ELEMENTS, p->front.num_ele, LEN_NUM_ELE);
#endif
    p->side.num_ele = getbits(LEN_NUM_ELE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_SIDE_CHANNEL_ELEMENTS, p->side.num_ele, LEN_NUM_ELE);
#endif
    p->back.num_ele = getbits(LEN_NUM_ELE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_BACK_CHANNEL_ELEMENTS,  p->back.num_ele, LEN_NUM_ELE);
#endif
    p->lfe.num_ele = getbits(LEN_NUM_LFE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_LFE_CHANNEL_ELEMENTS,  p->lfe.num_ele, LEN_NUM_LFE);
#endif
    p->data.num_ele = getbits(LEN_NUM_DAT);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_ASSOC_DATA_ELEMENTS, p->data.num_ele, LEN_NUM_DAT); 
#endif
    p->coupling.num_ele = getbits(LEN_NUM_CCE);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(NUM_VALID_CC_ELEMENTS, p->coupling.num_ele, LEN_NUM_CCE);
#endif
    p->mono_mix.present = getbits(LEN_MIX_PRES);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(MONO_MIXDOWN_PRESENT, p->mono_mix.present, LEN_MIX_PRES);
#endif
    if (p->mono_mix.present == 1) {
	p->mono_mix.ele_tag = getbits(LEN_TAG);
    /*WriteElemToBuffer(MONO_MIXDOWN_ELEMENT_NUMBER, p->mono_mix.ele_tag, LEN_TAG);*/
    }
    p->stereo_mix.present = getbits(LEN_MIX_PRES);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(MATRIX_MIXDOWN_IDX_PRESENT, p->stereo_mix.present, LEN_MIX_PRES);
#endif
    if (p->stereo_mix.present == 1) {
	p->stereo_mix.ele_tag = getbits(LEN_TAG);
    /*WriteElemToBuffer(STEREO_MIXDOWN_ELEMENT_NUMBER, p->stereo_mix.ele_tag, LEN_TAG);*/
    }
    p->matrix_mix.present = getbits(LEN_MIX_PRES);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(MATRIX_MIXDOWN_IDX_PRESENT, p->matrix_mix.present, LEN_MIX_PRES);
#endif
    if ( p->matrix_mix.present == 1 ) {
	p->matrix_mix.ele_tag = getbits(LEN_MMIX_IDX);
    /*WriteElemToBuffer(MATRIX_MIXDOWN_IDX, p->matrix_mix.ele_tag, LEN_MMIX_IDX);*/
	p->matrix_mix.pseudo_enab = getbits(LEN_PSUR_ENAB);
    /*WriteElemToBuffer(PSEUDO_SURROUND_ENABLE, p->matrix_mix.pseudo_enab, LEN_PSUR_ENAB);*/
    }
    get_ele_list(&p->front, 1);
    get_ele_list(&p->side, 1);
    get_ele_list(&p->back, 1);
    get_ele_list(&p->lfe, 0);
    get_ele_list(&p->data, 0);
    get_ele_list(&p->coupling, 1);
    
    byte_align();
    j = getbits(LEN_COMMENT_BYTES);
#ifdef ID_PCE_WRITE_SUPPORT
    WriteElemToBuffer(COMMENT_FIELD_BYTES,j,LEN_COMMENT_BYTES);
#endif
    for (i=0; i<j; i++) {
	p->comments[i] = getbits(LEN_BYTE);
    /*WriteElemToBuffer(COMMENT_FIELD_DATA, p->comments[i], LEN_BYTE);*/
    }
    p->comments[i] = 0;	/* null terminator for string */

    /* activate new program configuration if appropriate */
    if (current_program < 0)
	current_program = tag;	    /* always select new program */
    if (tag == current_program) {
	/* enter configuration into MC_Info structure */
	if (enter_mc_info(&mc_info, p) < 0)
	    return -1;
	/* inhibit default configuration */
	default_config = 0;
    }
   
    return tag;
}

/* enter program configuration into MC_Info structure
 *  only configures for channels specified in all.h
 */
int
enter_mc_info(MC_Info *mip, ProgConfig *pcp)
{  
    int i, j, cpe, tag, cidx, cw;
    EleList *elp;
    MIXdown *mxp;

    /* reset channel counts */
    mip->nch = 0;
    mip->nfch = 0;
    mip->nfsce = 0;
    mip->nsch = 0;
    mip->nbch = 0;
    mip->nlch = 0;
    mip->ncch = 0;

    /* profile and sampling rate
     *	re-configure if new sampling rate
     */    
    mip->profile = pcp->profile;
    if (mip->sampling_rate_idx != pcp->sampling_rate_idx) {
	mip->sampling_rate_idx = pcp->sampling_rate_idx;
	infoinit(&samp_rate_info[mip->sampling_rate_idx]);
    }
    
    cw = 0;			    /* changed later */
    
    /* front elements, center out */
    elp = &pcp->front;
    /* count number of leading SCE's */
    for (i=0, j=elp->num_ele; i<j; i++) {
	if (elp->ele_is_cpe[i])
	    break;
	mip->nfsce++;
    }
    for (i=0, j=elp->num_ele; i<j; i++) {
	cpe = elp->ele_is_cpe[i];
	tag = elp->ele_tag[i];
	if ((cidx = enter_chn(cpe, tag, 'f', cw, mip)) < 0)
	    return(-1);
    }
    
    /* side elements, left to right then front to back */
    elp = &pcp->side;
    for (i=0, j=elp->num_ele; i<j; i++) {
	cpe = elp->ele_is_cpe[i];
	tag = elp->ele_tag[i];
	if ((cidx = enter_chn(cpe, tag, 's', cw, mip)) < 0)
	    return(-1);
    }
	  
    /* back elements, outside to center */
    elp = &pcp->back;
    for (i=0, j=elp->num_ele; i<j; i++) {
	cpe = elp->ele_is_cpe[i];
	tag = elp->ele_tag[i];
	if ((cidx = enter_chn(cpe, tag, 'b', cw, mip)) < 0)
	     return(-1);
    }
    
    /* lfe elements */
    elp = &pcp->lfe;
    for (i=0, j=elp->num_ele; i<j; i++) {
	cpe = elp->ele_is_cpe[i];
	tag = elp->ele_tag[i];
	if ((cidx = enter_chn(cpe, tag, 'l', cw, mip)) < 0)
	    return(-1);
    }
        
    /* coupling channel elements */
    elp = &pcp->coupling;
    for (i=0, j=elp->num_ele; i<j; i++)
	mip->cch_tag[i] = elp->ele_tag[i];
    mip->ncch = j;
    
    /* mono mixdown elements */
    mxp = &pcp->mono_mix;
    if (mxp->present) {
	PRINT(SE,"Unanticipated mono mixdown channel\n");
	return(-1);
    }
    
    /* stereo mixdown elements */
    mxp = &pcp->stereo_mix;
    if (mxp->present) {
	PRINT(SE,"Unanticipated stereo mixdown channel\n");
	return(-1);
    }
    
    /* matrix mixdown elements */
    mxp = &pcp->matrix_mix;
    if (mxp->present) {
	PRINT(SE,"Unanticipated matrix mixdown channel\n");
	return(-1);
    }
    
    /* save to check future consistency */
    check_mc_info(&mc_info, 1);

    return 1;
}

/* translate prog config or default config 
 * into to multi-channel info structure
 *  returns index of channel in MC_Info
 */
int
enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip)
{
    int nch, cidx=0;
    Ch_Info *cip;

    nch = (cpe == 1) ? 2 : 1;
    
    switch (position) {
    /* use configuration already in MC_Info, but now common_window is valid */
    case 0:
	cidx = ch_index(mip, cpe, tag);
	if (common_window) {
	    mip->ch_info[cidx].widx = cidx;	/* window info is left */
	    mip->ch_info[cidx+1].widx = cidx;
	}
	else {
	    mip->ch_info[cidx].widx = cidx;	/* each has window info */
	    mip->ch_info[cidx+1].widx = cidx+1;
	}
	return cidx;
	
    /* build configuration */
    case 'f':
	if (mip->nfch + nch > FChans) {
	    PRINT(SE,"Unanticipated front channel\n");
	    return -1;
	}
	if (mip->nfch == 0) {
	    /* consider case of center channel */
	    if (FCenter) {
		if (cpe) {
		    /* has center speaker but center channel missing */
		    cidx = 0 + 1;
		    mip->nfch = 1 + nch;
		}
		else {
		    if (mip->nfsce & 1) {
			/* has center speaker and this is center channel */
			/* odd number of leading SCE's */
			cidx = 0;
			mip->nfch = nch;
		    }
		    else {
			/* has center speaker but center channel missing */
			/* even number of leading SCE's */
			/* (Note that in implicit congiguration
			 * channel to speaker mapping may be wrong 
			 * for first block while count of SCE's prior
			 * to first CPE is being make. However first block
			 * is not written so it doesn't matter.
			 * Second block will be correct.
			 */
			cidx = 0 + 1;
			mip->nfch = 1 + nch;
		    }
		}
	    }
	    else {
		if (cpe) {
		    /* no center speaker and center channel missing */
		    cidx = 0;
		    mip->nfch = nch;
		}
		else {
		    /* no center speaker so this is left channel */
		    cidx = 0;
		    mip->nfch = nch;
		}
	    }
	}
	else {
	    cidx = mip->nfch;
	    mip->nfch += nch;
	}
	break;
    case 's':
	if (mip->nsch + nch > SChans)  {
	    PRINT(SE,"Unanticipated side channel\n");
	    return -1;
	}
	cidx = FChans + mip->nsch;
	mip->nsch += nch;
	break;
    case 'b':
	if (mip->nbch + nch > BChans) {
	    PRINT(SE,"Unanticipated back channel\n");
	    return -1;
	}
	cidx = FChans + SChans + mip->nbch;
	mip->nbch += nch;
	break;
    case 'l':
	if (mip->nlch + nch > LChans) {
	    PRINT(SE,"Unanticipated LFE channel\n");
	    return -1;
	}
	cidx = FChans + SChans + BChans + mip->nfch;
	mip->nlch += nch;
	break;
    default:
	myexit("enter_chn");
    }
    mip->nch += nch;

    if (cpe == 0) {
	/* SCE */
	cip = &mip->ch_info[cidx];
	cip->present = 1;
	cip->tag = tag;
	cip->cpe = 0;
	cip->common_window = common_window;
	cip->widx = cidx;
	mip->nch = cidx + 1;
    }
    else {
        /* CPE */
	/* left */
	cip = &mip->ch_info[cidx];
	cip->present = 1;
	cip->tag = tag;
	cip->cpe = 1;
	cip->common_window = common_window;
	cip->ch_is_left = 1;
	cip->paired_ch = cidx+1;
	/* right */
	cip = &mip->ch_info[cidx+1];
	cip->present = 1;
	cip->tag = tag;
	cip->cpe = 1;
	cip->common_window = common_window;
	cip->ch_is_left = 0;
	cip->paired_ch = cidx;
	if (common_window) {
	    mip->ch_info[cidx].widx = cidx;	/* window info is left */
	    mip->ch_info[cidx+1].widx = cidx;
	}
	else {
	    mip->ch_info[cidx].widx = cidx;	/* each has window info */
	    mip->ch_info[cidx+1].widx = cidx+1;
	}
	mip->nch = cidx + 2;
    }
    
    return cidx;
}

static char
default_position(MC_Info *mip, int id)
{
    static int first_cpe = 0;

    if (mip->nch < FChans) {
	if (id == ID_CPE)
	    /* here is the first CPE */
	    first_cpe = 1;
	else if ( (bno==0) && !first_cpe )
	    /* count number of SCE prior to first CPE in first block */
	    mip->nfsce++;
	return('f');
    }
    else if  (mip->nch < FChans+SChans) {
	return('s');
    }
    else if  (mip->nch < FChans+SChans+BChans) {
	return('b');
    }
    
    return 0;
}

/* retrieve appropriate channel index for the program
 * and decoder configuration
 */
int
chn_config(int id, int tag, int common_window, MC_Info *mip)
{
    int cpe, cidx=0;
    char position;
    
    /* channel index to position mapping for 5.1 configuration is
     *  0	center
     *  1	left  front
     *  2	right front
     *  3	left surround
     *  4	right surround
     *  5	lfe
     */

    cpe = (id == ID_CPE) ? 1 : 0;
    
    if (default_config) {
	switch (id) {
	case ID_SCE:
	case ID_CPE:
	    if ((position = default_position(mip, id)) == 0) {
		PRINT(SE,"Unanticipated channel\n");
		myexit("default_position");
	    }
	    cidx = enter_chn(cpe, tag, position, common_window, mip);
	    break;
	case ID_LFE:
	    cidx = enter_chn(cpe, tag, 'l', common_window, mip);
	    break;
	}
    }
    else {	
	cidx = enter_chn(cpe, tag, 0, common_window, mip);
    }	

    return cidx;	    /* index of chn in mc_info */
}

/* 
 * check continuity of configuration from one
 * block to next
 */
void
reset_mc_info(MC_Info *mip)
{
    int i;
    Ch_Info *p;

    /* only reset for implicit configuration */
    if (default_config) {
	/* reset channel counts */
	mip->nch = 0;
	mip->nfch = 0;
	mip->nsch = 0;
	mip->nbch = 0;
	mip->nlch = 0;
	mip->ncch = 0;
	if (bno==0)
	    /* reset prior to first block scan only! */
	    mip->nfsce = 0;
	for (i=0; i<Chans; i++) {
	    p = &mip->ch_info[i];
	    p->present = 0;
	    p->cpe = 0;
	    p->ch_is_left = 0;
	    p->paired_ch = 0;
	    p->is_info.is_present = 0;
	    p->widx = 0;
	    p->ncch = 0;
	}
    }
}


void
check_mc_info(MC_Info *mip, int new_config)
{
    int i, nch, err;
    Ch_Info *s, *p;
    static MC_Info save_mc_info;

    nch = mip->nch;
    if (new_config) {
	/* enter valid configuration */
	for (i=0; i<nch; i++) {
	    s = &save_mc_info.ch_info[i];
	    p = &mip->ch_info[i];
	    s->present = p->present;
	    s->cpe = p->cpe;
	    s->ch_is_left = p->ch_is_left;
	    s->paired_ch = p->paired_ch;
	}
    }
    else {
	/* check this block's configuration */
	err = 0;
	for (i=0; i<nch; i++) {
	    s = &save_mc_info.ch_info[i];
	    p = &mip->ch_info[i];
	    if (s->present != p->present) err=1;
	    if (s->cpe != p->cpe) err=1;
	    if (s->ch_is_left != p->ch_is_left) err=1;
	    if (s->paired_ch != p->paired_ch) err=1;
	}
	if (err) {
	    PRINT(SE, "Channel configuration inconsistency\n");
	    for (i=0; i<nch; i++) {
		s = &save_mc_info.ch_info[i];
		p = &mip->ch_info[i];
		PRINT(SE, "Channel %d\n", i);
		PRINT(SE, "present    %d %d\n", s->present,  p->present);
		PRINT(SE, "cpe        %d %d\n", s->cpe,  p->cpe);
		PRINT(SE, "ch_is_left %d %d\n", s->ch_is_left,  p->ch_is_left);
		PRINT(SE, "paired_ch  %d %d\n", s->paired_ch,  p->paired_ch);
		PRINT(SE, "\n");
	    }
	    myexit("check_mc_info");
	}
    }
}


/* given cpe and tag,
 *  returns channel index of SCE or left chn in CPE
 */
int
ch_index(MC_Info *mip, int cpe, int tag)
{
    int ch;
    
    for (ch=0; ch<mip->nch; ch++) {
	if (!(mip->ch_info[ch].present))
	    continue;
	if ( (mip->ch_info[ch].cpe == cpe) && 
	    (mip->ch_info[ch].tag == tag) )
	    return ch;
    }
    
    /* no match, so channel is not in this program
     *	dummy up the ch_info structure so rest of chn will parse
     */
    if (XChans > 0) {
	ch = Chans - XChans;	/* left scratch channel */
	mip->ch_info[ch].cpe = cpe;
	mip->ch_info[ch].ch_is_left = 1;
	mip->ch_info[ch].widx = ch;
	if (cpe) {
	    mip->ch_info[ch].paired_ch = ch+1;
	    mip->ch_info[ch+1].ch_is_left = 0;
	    mip->ch_info[ch+1].paired_ch = ch;
	}
    }
    else {
	ch = -1;		/* error, no scratch space */
    }
    
    return ch;
}
