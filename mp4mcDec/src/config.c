/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  -

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
#include "adifdec.h"
#include "diagnose.h"
#include "huffinit.h"
#include "interface.h"
#include "bitinput.h"
#include "tns.h"

/* --- local data --- */

static int
pred_max_bands_tbl[(1<<LEN_SAMP_IDX)] = {
  33,     /* 96000 */
  33,     /* 88200 */
  38,     /* 64000 */
  40,     /* 48000 */
  40,     /* 44100 */
  40,     /* 32000 */
  41,     /* 24000 */
  41,     /* 22050 */
  37,     /* 16000 */
  37,     /* 12000 */
  37,     /* 11025 */
  34,     /* 8000  */
  34,     /* 7350  */
  0,
  0,
  0
};

static int
tns_max_bands_tbl[(1<<LEN_SAMP_IDX)][4] = {
    /* entry for each sampling rate	
     * 1    Main/LC long window
     * 2    Main/LC short window
     * 3    SSR long window
     * 4    SSR short window
     */
    {31,  9, 28,  7},	    /* 96000 */
    {31,  9, 28,  7},	    /* 88200 */
    {34, 10, 27,  7},	    /* 64000 */
    {40, 14, 26,  6},	    /* 48000 */
    {42, 14, 26,  6},	    /* 44100 */
    {51, 14, 26,  6},	    /* 32000 */
    {46, 14, 29,  7},	    /* 24000 */
    {46, 14, 29,  7},	    /* 22050 */
    {42, 14, 23,  8},	    /* 16000 */
    {42, 14, 23,  8},	    /* 12000 */
    {42, 14, 23,  8},	    /* 11025 */
    {39, 14, 19,  7},	    /* 8000  */
    {39, 14, 19,  7},       /* 7350  */
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};

static int sampling_boundarys[(1<<LEN_SAMP_IDX)] = {
  92017, 75132, 55426, 46009, 37566, 27713, 23004, 18783,
  13856, 11502, 9391, 0, 0, 0, 0, 0
};

/* --- global data --- */

ProgConfig	prog_config;
MC_Info		mc_info;

int		current_program;
int		default_config;
/* Number of channels for decoderBitBuffer-calculations */
int		channel_num;

int sampling_rate[(1<<LEN_SAMP_IDX)] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

/* --- functions --- */


/* enter program configuration into MC_Info structure
 *  only configures for channels specified in chandefs.h
 */
static int
enter_mc_info(MC_Info *mip, ProgConfig *pcp)
{  
    int i, j, id, tag, cw;
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
    }
    mip->sampling_rate = sampling_rate[mip->sampling_rate_idx];
    
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
	id = elp->ele_is_cpe[i]?ID_CPE:ID_SCE;
	tag = elp->ele_tag[i];
	if (enter_chn(id, tag, 'f', cw, mip) < 0)
	    return(-1);
    }

    /* side elements, left to right then front to back */
    elp = &pcp->side;
    for (i=0, j=elp->num_ele; i<j; i++) {
	id = elp->ele_is_cpe[i]?ID_CPE:ID_SCE;
	tag = elp->ele_tag[i];
	if (enter_chn(id, tag, 's', cw, mip) < 0)
	    return(-1);
    }

    /* back elements, outside to center */
    elp = &pcp->back;
    for (i=0, j=elp->num_ele; i<j; i++) {
	id = elp->ele_is_cpe[i]?ID_CPE:ID_SCE;
	tag = elp->ele_tag[i];
	if (enter_chn(id, tag, 'b', cw, mip) < 0)
	     return(-1);
    }
    
    /* lfe elements */
    elp = &pcp->lfe;
    for (i=0, j=elp->num_ele; i<j; i++) {
	id = elp->ele_is_cpe[i];
	tag = elp->ele_tag[i];
	if (enter_chn(id, tag, 'l', cw, mip) < 0)
	    return(-1);
    }
        
    /* coupling channel elements */
    elp = &pcp->coupling;
    for (i=0, j=elp->num_ele; i<j; i++) {
	mip->cc_tag[i] = elp->ele_tag[i];
	mip->cc_ind[i] = elp->ele_is_cpe[i];
    }
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



/*
 * get "closest" sampling frequency index for odd sampling rates
 * (ISO/IEC 14496-3 Table 4.55)
 */
int
getNextSampFreqIndex(int sampRate)
{
  int i = 0;
  if (sampRate < sampling_boundarys[(1<<LEN_SAMP_IDX)-1]) return -1;
  while (sampRate < sampling_boundarys[i]) { i++; }
  return i;
}

/*
 * profile dependent parameters
 */
int tns_max_bands(int islong)
{
    int i;

    /* construct second index */
    i = islong ? 0 : 1;
    i += (mc_info.profile ==  SSR_Profile) ? 2 : 0;
    
    return tns_max_bands_tbl[mc_info.sampling_rate_idx][i];
}

int
tns_max_order(int islong)
{
    if (islong) {
	switch (mc_info.profile) {
	case Main_Profile:
#ifdef MPEG4V1
	case LTP_Profile:
#endif
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

int
pred_max_bands(void)
{
    return pred_max_bands_tbl[mc_info.sampling_rate_idx];
}

#define LF	"%-25s %d\n"


/*
 * program configuration element
 */
static void
get_ele_list(EleList *p, int enable_cpe)
{  
    int i, j;
    for (i=0, j=p->num_ele; i<j; i++) {
	if (enable_cpe)
	    p->ele_is_cpe[i] = getbits(LEN_ELE_IS_CPE);
        else
            p->ele_is_cpe[i] = 0; /* sdb */
	p->ele_tag[i] = getbits(LEN_TAG);
    }
}

int
get_prog_config(ProgConfig *p)
{
    int i, j, tag;

    tag = getbits(LEN_TAG);

    p->profile = getbits(LEN_PROFILE);
    p->sampling_rate_idx = getbits(LEN_SAMP_IDX);
    p->front.num_ele = getbits(LEN_NUM_ELE);
    p->side.num_ele = getbits(LEN_NUM_ELE);
    p->back.num_ele = getbits(LEN_NUM_ELE);
    p->lfe.num_ele = getbits(LEN_NUM_LFE);
    p->data.num_ele = getbits(LEN_NUM_DAT);
    p->coupling.num_ele = getbits(LEN_NUM_CCE);
    if ((p->mono_mix.present = getbits(LEN_MIX_PRES)) == 1)
	p->mono_mix.ele_tag = getbits(LEN_TAG);
    if ((p->stereo_mix.present = getbits(LEN_MIX_PRES)) == 1)
	p->stereo_mix.ele_tag = getbits(LEN_TAG);
    if ((p->matrix_mix.present = getbits(LEN_MIX_PRES)) == 1) {
	p->matrix_mix.ele_tag = getbits(LEN_MMIX_IDX);
	p->matrix_mix.pseudo_enab = getbits(LEN_PSUR_ENAB);
    }
    get_ele_list(&p->front, 1);
    get_ele_list(&p->side, 1);
    get_ele_list(&p->back, 1);
    get_ele_list(&p->lfe, 0);
    get_ele_list(&p->data, 0);
    get_ele_list(&p->coupling, 1);
    
    byte_align();
    j = getbits(LEN_COMMENT_BYTES);
    for (i=0; i<j; i++)
	p->comments[i] = getbits(LEN_BYTE);
    p->comments[i] = 0;	/* null terminator for string */

    /* activate new program configuration if appropriate */
    if (current_program < 0)
	current_program = tag;	    /* select first program found */
    if (tag == current_program) {
	/* enter configuration into MC_Info structure */
	if (enter_mc_info(&mc_info, p) < 0)
	    return -1;
	/* inhibit default configuration */
	default_config = 0;
    }

    return tag;
}


static void print_ele(char *s, EleList *p)
{
    int i;
    PRINT(SE, "%s\n", s);
    if (p->num_ele) {
      if (!strstr(s,"coupling"))
	PRINT(SE, "%6s %6s %6s\n", "idx", "cpe", "tag");
      else
        PRINT(SE, "%6s %6s %6s\n", "idx", "ind", "tag");
    }
    for (i=0; i<p->num_ele; i++)
	PRINT(SE, "%6d %6d %6d\n", i, p->ele_is_cpe[i], p->ele_tag[i]);    
}


void
print_adif_header(ADIF_Header *p)
{
  PRINT(SE, "\nADIF header:\n");
  PRINT(SE, LF, "p->copy_id_present", p->copy_id_present);
  PRINT(SE, LF, "p->original_copy", p->original_copy);
  PRINT(SE, LF, "p->home", p->home);
  PRINT(SE, LF, "p->bitstream_type", p->bitstream_type);
  PRINT(SE, LF, "p->bitrate", (int) p->bitrate);
  PRINT(SE, LF, "buffer_fullness", (int)p->buffer_fullness );
}

void
print_adts_header(ADTS_Header *p)
{
  PRINT(SE, "\nADTS header:\n");
  PRINT(SE, LF, "p->id", (int)p->id );
  PRINT(SE, LF, "p->layer", (int)p->layer );
  PRINT(SE, LF, "p->profile", (int)p->profile );
  PRINT(SE, LF, "p->sampling_rate", (int)sampling_rate[p->sampling_freq_idx] );
  PRINT(SE, LF, "buffer_fullness", (int)(p->adts_buffer_fullness<<5) );
  PRINT(SE, LF, "ChannelConfiguration", (int)p->channel_config );
  PRINT(SE, LF, "p->protection_absent", (int)p->protection_abs );
  PRINT(SE, LF, "p->num_of_rdb", (int)p->num_of_rdb );
}

void
print_PCE(ProgConfig *p)
{
  PRINT(SE, LF, "p->profile", p->profile);
  PRINT(SE, LF, "p->sampling_rate_idx", p->sampling_rate_idx);
	
  print_ele("p->front", &p->front);
  print_ele("p->side", &p->side);
  print_ele("p->back", &p->back);
  print_ele("p->lfe", &p->lfe);
  print_ele("p->coupling", &p->coupling);
	
  PRINT(SE, LF, "p->data.num_ele", p->data.num_ele);
  PRINT(SE, LF, "p->mono_mix.present", p->mono_mix.present);
  PRINT(SE, LF, "p->stereo_mix.present", p->stereo_mix.present);
  PRINT(SE, LF, "p->matrix_mix.present", p->matrix_mix.present);
  PRINT(SE, "Comment: %s\n", p->comments);
}




/* translate prog config or default config 
 * into to multi-channel info structure
 *  returns index of channel in MC_Info
 */
int
enter_chn(int id, int tag, char position, int common_window, MC_Info *mip)
{
  int nch, cidx=0;
  Ch_Info *cip;

  nch = (id == ID_CPE) ? 2 : 1;

  switch (position) {
  /* use configuration already in MC_Info, but now common_window is valid */
  case 0:
	  cidx = ch_index(mip, id, tag);

	  if (nch == 1) {
	    mip->ch_info[cidx].widx = cidx; /* only one channel for window info*/
	  }
	  else {
	    if (common_window) {
	      mip->ch_info[cidx].widx = cidx; /* window info is left */
	      mip->ch_info[cidx+1].widx = cidx;
	    }
	    else {
	      mip->ch_info[cidx].widx = cidx; /* each has window info */
	      mip->ch_info[cidx+1].widx = cidx+1;
	    }
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
		    if (id == ID_CPE) {
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
		    if (id == ID_CPE) {
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
	  cidx = FChans + SChans + BChans + mip->nlch;
	  mip->nlch += nch;
	  break;
  default:
	myexit("enter_chn");
  }
  mip->nch += nch;

  if (id != ID_CPE) {
	  /* SCE */
	  cip = &mip->ch_info[cidx];
	  cip->present = 1;
	  cip->tag = tag;
	  cip->cpe = 0;
	  cip->common_window = common_window;
	  cip->widx = cidx;
	  /* mip->nch = cidx + 1; */
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
	  /* mip->nch = cidx + 2; */
  }
  
  return cidx;
}



static char
default_position(MC_Info *mip, int id, int* first_cpe)
{


    if (mip->nch < FChans) {
	if (id == ID_CPE)
	    /* here is the first CPE */
	    *first_cpe = 1;
	else if ( (bno==0) && !(*first_cpe) )
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
chn_config(int id, int tag, int common_window, MC_Info *mip, int* first_cpe)
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
	    if ((position = default_position(mip, id, first_cpe)) == 0) {
/*	    if ((position = default_position(mip, id)) == 0) { */
	  	  PRINT(SE,"Unanticipated channel\n");
		    myexit("default_position");
	    }
	    cidx = enter_chn(id, tag, position, common_window, mip);
	    break;
	  case ID_LFE:
	    cidx = enter_chn(id, tag, 'l', common_window, mip);
	    break;
	  }
  }
  else {	
    cidx = enter_chn(id, tag, 0, common_window, mip);
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

  /* always reset number of coupling channels per audio channel */
  for (i=0; i<Chans; i++)
	  mip->ch_info[i].ncch = 0;
	
  /* only reset for implicit configuration */
  if (default_config) {
	  /* reset channel counts */
	  mip->nch = 0;
	  mip->nfch = 0;
	  mip->nsch = 0;
	  mip->nbch = 0;
	  mip->nlch = 0;
	  mip->ncch = 0;
	  /* reset prior to first block scan only! */
	  if (bno==0)
	      mip->nfsce = 0;
	  for (i=0; i<Chans; i++) {
	      p = &mip->ch_info[i];
	      p->present = 0;
	      p->cpe = 0;
	      p->ch_is_left = 0;
	      p->paired_ch = 0;
	      p->is_present = 0;
	      p->widx = 0;
	      p->ncch = 0;
	  }
  }
}


void
check_mc_info(MC_Info *mip, int new_config)
{
    int i, err;
    Ch_Info *s, *p;
    static MC_Info save_mc_info;

    if (new_config) {
	/* enter valid configuration */
	for (i=0; i<Chans; i++) {
	    s = &save_mc_info.ch_info[i];
	    p = &mip->ch_info[i];
	    s->present = p->present;
	    s->cpe = p->cpe;
	    s->ch_is_left = p->ch_is_left;
	    s->paired_ch = p->paired_ch;
	}

        /*count channels for decoderBufferSize*/
        channel_num = mip->nch-mip->nlch;
        for (i = 0; i<mip->ncch; i++) {
          channel_num+= mip->cc_ind[i];
        }
    }
    else {
	/* check this block's configuration */
	err = 0;
	for (i=0; i<Chans; i++) {
	    s = &save_mc_info.ch_info[i];
	    p = &mip->ch_info[i];
	    if (s->present != p->present) err=1;
            if (!s->present) continue; /* sdb */
	    if (s->cpe != p->cpe) err=1;
	    if (s->ch_is_left != p->ch_is_left) err=1;
	    if (s->paired_ch != p->paired_ch) err=1;
	}
	if (err) {
	    PRINT(SE, "Channel configuration inconsistency\n");
	    for (i=0; i<Chans; i++) {
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

void
print_mc_info(MC_Info *mip){
    int i, j;
    Ch_Info *cip;
    PRINT(SE, "sampling_rate_idx: %i\n",mip->sampling_rate_idx); 
    PRINT(SE, "sampling_rate    : %i\n",mip->sampling_rate); 
    PRINT(SE, "profile: %i\n",mip->profile); 
    PRINT(SE, "Ch Info\n");
    PRINT(SE, "idx tag cpe wdx  cch dom ind\n");
    for (i=0; i<Chans; i++) {
	cip = &mip->ch_info[i];
	if (!cip->present)
	    continue;
	PRINT(SE, "%3d %3d %3d %3d ", i, cip->tag, cip->cpe, cip->widx);
	for (j=0; j<cip->ncch; j++) {
	    if (j>0)
		PRINT(SE, "\n%16s", "");
	    PRINT(SE, " %3d %3d %3d", cip->cch[j], cip->cc_dom[j], cip->cc_ind[j]);
	}
	PRINT(SE, "\n");
    }
}

/* given id and tag,
 *  returns channel index of SCE or left chn in CPE
 */
int
ch_index(MC_Info *mip, int id, int tag)
{
    int ch;
    int cpe = (id == ID_CPE)? 1 : 0;

    if(id == ID_LFE) {
      for (ch=FChans+SChans+BChans; ch < FChans+SChans+BChans+LChans; ch++) {

        if(!(mip->ch_info[ch].present))
          continue;

        if(mip->ch_info[ch].tag == tag) {
          return ch;
        }
      }
    }
    else
    {
      for (ch=0; ch<FChans+SChans+BChans; ch++) {

	if (!(mip->ch_info[ch].present))
          continue;
        
	if ( (mip->ch_info[ch].cpe == cpe) && 
             (mip->ch_info[ch].tag == tag) ){
          return ch;
	}
      }
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
