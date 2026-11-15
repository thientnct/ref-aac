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

#include "adifdec.h"
#include "diagnose.h"
#include "gc_unpac.h"
#include "huffdec_intern.h"
#include "huffinit.h"
#include "intrins.h"
#include "bitinput.h"
#include "huffdec.h"

typedef int DINT_DATATYPE ;


static void
deinterleave(DINT_DATATYPE inptr[], DINT_DATATYPE outptr[], short ngroups,
             short nsubgroups[], int ncells[], short cellsize[])
{
  int i, j, k, l;
  DINT_DATATYPE *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
  short cell_inc, subgroup_inc;

  start_subgroup_ptr = outptr;

  for (i = 0; i < ngroups; i++)
    {
      cell_inc = 0;
      start_inptr = inptr;

      /* Compute the increment size for the subgroup pointer */

      subgroup_inc = 0;
      for (j = 0; j < ncells[i]; j++) {
        subgroup_inc += cellsize[j];
      }

      /* Perform the deinterleaving across all subgroups in a group */

      for (j = 0; j < ncells[i]; j++) {
        subgroup_ptr = start_subgroup_ptr;

        for (k = 0; k < nsubgroups[i]; k++) {
          outptr = subgroup_ptr + cell_inc;
          for (l = 0; l < cellsize[j]; l++) {
            *outptr++ = *inptr++;
          }
          subgroup_ptr += subgroup_inc;
        }
        cell_inc += cellsize[j];
      }
      start_subgroup_ptr += (inptr - start_inptr);
    }
}

static void 
calc_gsfb_table(Info *info, byte *group)
{
  int group_offset;
  int group_idx;
  int offset;
  short * group_offset_p;
  int sfb,len;
  /* first calc the group length*/
  if (info->islong){
    return;
  } else {
    group_offset = 0;
    group_idx =0;
    do  {
      info->group_len[group_idx]=group[group_idx]-group_offset;
      group_offset=group[group_idx];
      group_idx++;
    } while (group_offset<8);
    info->num_groups=group_idx;
    group_offset_p = info->bk_sfb_top;
    offset=0;
    for (group_idx=0;group_idx<info->num_groups;group_idx++){
      len = info->group_len[group_idx];
      for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++){
        offset += info->sfb_width_128[sfb] * len;
        *group_offset_p++ = offset;
#if 0
        PRINT(SE,"%d %d: %d\n",  group_idx, sfb, offset);
#endif	  
      }
    }
  }
}

#ifdef FRAMELENGTH_FLAG
#define MAX_TABLE_VALUES (128)
extern int copy_to_output;
extern int frameLengthFlag;
#endif

/*
 * read the codebook and boundaries
 */
static int
huffcb(byte *sect, int *sectbits, int tot_sfb, int sfb_per_sbk, byte max_sfb)
{
  int tmp, nsect, n, base, bits, len;

#ifdef FRAMELENGTH_FLAG
  int c = 0;
  int table[MAX_TABLE_VALUES][2] = {{0},{0}};
#endif

  if (debug['s']) {
    PRINT(SE,"total sfb %d\n", tot_sfb);
    PRINT(SE,"sect, top, cb\n");
  }
  bits = sectbits[0];
  len = (1 << bits) - 1;
  nsect = 0;

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag == 1 ) {
        copy_to_output = 0;
    }
#endif

  for(base = 0; base < tot_sfb && nsect < tot_sfb; ) {

    tmp = getbits(LEN_CB);
    *sect++ = tmp;

#ifdef FRAMELENGTH_FLAG
    table[c  ][0] = LEN_CB;
    table[c++][1] = tmp;
#endif

    n = getbits(bits);
#ifdef FRAMELENGTH_FLAG
    table[c  ][0] = bits;
    table[c++][1] = n;
#endif

    while(n == len && base < tot_sfb){
      base += len;
      n = getbits(bits);
#ifdef FRAMELENGTH_FLAG
    table[c  ][0] = bits;
    table[c++][1] = n;
#endif
    }
    base += n;


    *sect++ = base;
    nsect++;
    if (debug['s'])
      PRINT(SE," %6d %6d %6d \n", nsect, sect[-1], sect[-2]);

    /* insert a zero section for regions above max_sfb for each group */
    if ((sect[-1] % sfb_per_sbk) == max_sfb) {
      base += (sfb_per_sbk - max_sfb);
      *sect++ = 0;
      *sect++ = base;
      nsect++;
      if (debug['s'])
        PRINT(SE,"(%6d %6d %6d)\n", nsect, sect[-1], sect[-2]);
    }
  }

#ifdef FRAMELENGTH_FLAG
  if ( frameLengthFlag == 1 ) {

    int decValue = max_sfb - maxSfb960FrameLen[mc_info.sampling_rate_idx];
    c = MAX_TABLE_VALUES - 1; /* start from end */

    /* decreasing sections */
    while ( table[--c][0] == 0 ); /* get last section */   

    if ( table[c][0] == 5 && decValue > 0 ) {
      if ( table[c][1] >= decValue ) {
        table[c][1] -= decValue;
        if ( table[c][1] == 0 ) { 
          table[c][0] = 0;
          table[c-1][0] = 0; table[c-1][1] = 0;
        }
      }
      else { /* review this ! */
        int diff = decValue - table[c][1];
        table[c][0]   = 0; table[c][1]   = 0;
        table[c-1][0] = 0; table[c-1][1] = 0;
        if ( table[c-2][1] >= diff ) {
          table[c-2][1] -= diff;
          if ( table[c-2][1] == 0 ) {
            table[c-2][0] = 0;
            table[c-3][0] = 0; table[c-3][1] = 0; 
          }
        }
      }
    }
    c = 0;
    while ( table[c][0] != 0 ) {
      writebits(table[c][0], table[c][1]); /* write out decreased sections */
      c++;
    }
    copy_to_output = 1;
  }
#endif

  if(base != tot_sfb || nsect > tot_sfb)
    return 0;
  return nsect;
}

/* 
 * get scale factors
 */
static int
hufffac(Info *info, byte *group, int nsect, byte *sect,
        short global_gain, short *factors)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
#ifdef MPEG4V1
  int noise_pcm_flag = 1;
  int noise_nrg;
#endif

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for(i = 0; i < nsect; i++){
    top = sect[1];		/* top of section in sfb */
    t = sect[0];		/* codebook for this section */
    sect += 2;
    for(; sfb < top; sfb++) {
      fac_trans[sfb] = t;
    }
  }

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
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

#ifdef FRAMELENGTH_FLAG
      if ( frameLengthFlag == 1 ) {
        if ( info->nsbk == 1 ) {
          if ( i >= maxSfb960FrameLen[mc_info.sampling_rate_idx] ) {
            copy_to_output = 0;
          }
        }
      }
#endif

      switch (fac_trans[i]) {
      case ZERO_HCB:	    /* zero book */
        break;
      default:		    /* spectral books */
        /* decode scale factor */
        t = decode_huff_cw(hcw);
        fac += t - MIDFAC;    /* 1.5 dB */
        if(fac >= 2*maxfac || fac < 0)
          return 0;
        factors[i] = fac;
        break;
      case BOOKSCL:	    /* invalid books */
        return 0;
#ifndef MPEG4V1
      case RESERVED_HCB:
        myexit("The MPEG-4 tool PNS is not supported!\n");
        return 0;
#endif
      case INTENSITY_HCB:	    /* intensity books */
      case INTENSITY_HCB2:
        /* decode intensity position */
        is_present=1;
        t = decode_huff_cw(hcw);
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
#ifdef MPEG4V1
      case NOISE_HCB:	    /* noise books */
        /* decode noise energy */
        noise_present=1;
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
        PRINT(SE,"%3d: %3d %3d\n", i, fac_trans[i], factors[i]);
    }

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag == 1 ) {
      copy_to_output = 1;
    }
#endif
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
    fac_trans += n;
    factors += n;
  }
  return 1;
}


/*
 * PULSE noiseless coding
 */
struct Pulse_Info
{
  int pulse_data_present;
  int number_pulse;
  int pulse_start_sfb;
  int pulse_position[NUM_PULSE_LINES];
  int pulse_offset[NUM_PULSE_LINES];
  int pulse_amp[NUM_PULSE_LINES];
};
static struct Pulse_Info pulse_info;

static void
get_pulse_nc(struct Pulse_Info *pulse_info)
{
  int i;
  pulse_info->number_pulse = getbits(LEN_PULSE_NPULSE);
  pulse_info->pulse_start_sfb = getbits(LEN_PULSE_ST_SFB);
  for(i=0; i<pulse_info->number_pulse+1; i++) {
    pulse_info->pulse_offset[i] = getbits(LEN_PULSE_POFF);
    pulse_info->pulse_amp[i] = getbits(LEN_PULSE_PAMP);
    if (debug['P'])
      PRINT(SE,"Pulse: %d %d %d %d\n", pulse_info->pulse_start_sfb,
            i, pulse_info->pulse_offset[i], pulse_info->pulse_amp[i]);
  }
}


static void
pulse_nc(int *coef, struct Pulse_Info *pulse_info)
{
  int i, k;
    
  if (pulse_info->pulse_start_sfb>0){
    k = win_seq_info[ONLY_LONG_WINDOW]->sbk_sfb_top[0][pulse_info->pulse_start_sfb-1];
  }else{
    k = 0;
  }
    
  for(i=0; i<=pulse_info->number_pulse; i++) {
    k += pulse_info->pulse_offset[i];
    if (coef[k]>0) coef[k] += pulse_info->pulse_amp[i];
    else coef[k] -= pulse_info->pulse_amp[i];
  }
}


static int
huffspec(Info *info, int nsect, byte *sect, short *factors, Float *coef, int max_spectral_line)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, j, k, table, step, temp, stop, bottom, top;
  short *bands, *bandp;
  int quant[LN2], *qp;	    /* probably could be short */

  int idx,kstart;
  int* quantDebug;
  int tmp_spec[LN2];
  int max_spectral_line_OK = 1;

  intclr(quant, LN2);

  bands = info->bk_sfb_top;
  bottom = 0;
  k = 0;
  bandp = bands;
  for(i = nsect; i; i--) {
    table = sect[0];
    top = sect[1];
    sect += 2;
    quantDebug=quant;
    if( (table == 0) ||
#ifdef MPEG4V1
        (table == NOISE_HCB) ||
#endif
        (table == INTENSITY_HCB) ||
        (table == INTENSITY_HCB2) ) {
      bandp = bands+top;
      k = bandp[-1];
      bottom = top;
      continue;
    }
    if(table < BY4BOOKS+1) {
      step = 4;
    }
    else {
      step = 2;
    }
    hcb = &book[table];
    hcw = hcb->hcw;
    qp = quant+k;
    kstart=k;

    for(j=bottom; j<top; j++) {

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag == 1 ) {
      if ( info->nsbk == 1 ) {
        if ( j >= maxSfb960FrameLen[mc_info.sampling_rate_idx] ) {
          copy_to_output = 0;
        }
      }
    }
#endif

      stop = *bandp++;

      while(k < stop) {

#ifdef FRAMELENGTH_FLAG
        if ( frameLengthFlag == 1 ) {
          if ( info->nsbk == 1 ) {  /* long block */
            if ( k >= 960 ) {
              copy_to_output = 0;
            }
          }
#if 0
          else { /* short blocks */
            /* review this part ! */
          }
#endif        
        }
#endif
        temp = decode_huff_cw(hcw);
        unpack_idx(qp, temp, hcb);

        if (!hcb->signed_cb)
          get_sign_bits(qp, step);
        if(table == ESCBOOK){
          qp[0] = getescape(qp[0]);
          qp[1] = getescape(qp[1]);
        }
        qp += step;
        k += step;
        if ((max_spectral_line>0)&&(k>=max_spectral_line)) { /* check for the 12 lines allowed for LFEs */
          for (idx=max_spectral_line;idx<k;idx++) if (quant[idx]!=0) max_spectral_line_OK=0;
        }
      }
      if(debug['q']){
        PRINT(SE,"\nsect %d %d\n", table, kstart);
        for (idx=kstart ;idx<k;idx++) {
          PRINT(SE,"%5i",quantDebug[idx]);
        }
        PRINT(SE,"\n");
      }
    }
    bottom = top;
  }
  if (max_spectral_line_OK==0) PRINT(SE,"WARNING: more non-zero spectral lines than expected (maximum of %i lines)!\n", max_spectral_line);

#ifdef FRAMELENGTH_FLAG
  if ( frameLengthFlag == 1 ) {
    copy_to_output = 1;
  }
#endif

  /* PULSE noisless coding reconstruction */
  if ( (info->islong) && (pulse_info.pulse_data_present) )
    pulse_nc(quant, &pulse_info);

  if (!info->islong) {
    deinterleave (quant,tmp_spec,
                  info->num_groups,   
                  info->group_len,
                  info->sfb_per_sbk,
                  info->sfb_width_128);
    memcpy(quant,tmp_spec,sizeof(tmp_spec));
  }

  /* inverse quantization */
  for (i=0; i<info->bins_per_bk; i++) {
    coef[i] = esc_iquant(quant[i]);
  }

  /* rescaling */
  {
    int sbk, nsbk, sfb, nsfb, fac, top;
    Float *fp, scale;

    i = 0;
    fp = coef;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) {
      nsfb = info->sfb_per_sbk[sbk];
      k=0;
      for (sfb=0; sfb<nsfb; sfb++) {
        top = info->sbk_sfb_top[sbk][sfb];
        fac = factors[i++]-SF_OFFSET;

        if (fac >= 0 && fac < TEXP) {
          scale = exptable[fac];
        }
        else {
          if (fac == -SF_OFFSET) {
            scale = 0;
          }
          else {
            scale = pow( 2.0,  0.25*fac );
          }
        }
        for ( ; k<top; k++) {
          if(debug['l'])
            PRINT(SE, "%6d %12.2f %12.2f\n", k, *fp, (*fp)*scale);
          *fp++ *= scale;
        }
      }
    }
  }
  return 1;
}


void
getgroup(Info *info, byte *group)
{
  int i, j, first_short;

  if( debug['g'] ) PRINT(SE,"Grouping: 0");
  first_short=1;
  for (i = 0; i < info->nsbk; i++) {
    if (info->bins_per_sbk[i] > SN2) {
      /* non-short windows are always their own group */
      *group++ = i+1;
    }
    else {
      /* only short-window sequences are grouped! */
      if (first_short) {
        /* first short window is always a new group */
        first_short=0;
      }
      else {
        if((j = getbits(1)) == 0) {
          *group++ = i;
        }
        if( debug['g'] ) PRINT(SE,"%1d", j);
      }
    }
  }
  *group = i;
  if( debug['g'] ) PRINT(SE,"\n");
}

int
getics(Info *info, int common_window, byte *win, byte *wshape, 
       byte *group, byte *max_sfb, int *lpflag, int *prstflag, 
       byte *cb_map, Float *coef, short *global_gain, 
       short *factors,
#ifdef MPEG4V1
       NOK_LT_PRED_STATUS *nok_ltp_status,
#endif
       TNS_frame_info *tns, int ch, int max_spectral_line
       )
{
  int nsect, i, cb, top, bot, tot_sfb;
  byte sect[ 2*(MAXBANDS+1) ];

  /*
   * global gain
   */
  *global_gain = getbits(LEN_SCL_PCM);
  if (debug['f'])
    PRINT(SE,"global gain: %3d\n", *global_gain);

  if (!common_window)
    get_ics_info(win, wshape, group, max_sfb, lpflag, prstflag,
#ifdef MPEG4V1
                 nok_ltp_status, NULL, 0,
#endif
                 info);
  /*  memcpy(info, winmap[*win], sizeof(Info));*/

  /* calculate total number of sfb for this grouping */
  if (*max_sfb == 0) {
    tot_sfb = 0;
  }
  else {
    i=0;
    tot_sfb = info->sfb_per_sbk[0];
    if (debug['f'])PRINT(SE,"tot sfb %d %d\n", i, tot_sfb);
    while (group[i++] < info->nsbk) {
      tot_sfb += info->sfb_per_sbk[0];
      if (debug['f'])PRINT(SE,"tot sfb %d %d\n", i, tot_sfb);
    }
  }

  /* 
   * section data
   */
  nsect = huffcb(sect, info->sectbits, tot_sfb, info->sfb_per_sbk[0], *max_sfb);
  if(nsect==0 && *max_sfb>0)
    return 0;

  /* generate "linear" description from section info
   * stored as codebook for each scalefactor band and group
   */
  if (nsect) {
    bot = 0;
    for (i=0; i<nsect; i++) {
      cb = sect[2*i];
      top = sect[2*i + 1];
      for (; bot<top; bot++)
        *cb_map++ = cb;
      bot = top;
    }
  }  else {
    for (i=0; i<MAXBANDS; i++)
      cb_map[i] = 0;
  }

  /* calculate band offsets
   * (because of grouping and interleaving this cannot be
   * a constant: store it in info.bk_sfb_top)
   */
  calc_gsfb_table(info, group);

  /*
   * scale factor data
   */
  if(!hufffac(info, group, nsect, sect, *global_gain, factors))
    return 0;

  /*
   * PULSE noiseless coding
   */
  if ((pulse_info.pulse_data_present = getbits(LEN_PULSE_PRES))) {
    pulse_present=1;
    if (info->islong) {
      get_pulse_nc(&pulse_info);
    }
    else {
      PRINT(SE, "Pulse data not allowed for short blocks\n");
      myexit("pulse_nc");
    }
  }

  /*
   * tns data
   */
  if (getbits(LEN_TNS_PRES)) {
    tns_present=1;
    get_tns(info, tns);
  }
  else {
    clr_tns(info, tns);
  }

  /*
   * Sony gain control
   */
  if (getbits(LEN_GAIN_PRES)) {
    /* gc_present=1; "gain control data present" is not neccessarily "gain control used" */
    if(mc_info.profile == SSR_Profile) {
      unpac_gaincontrol_data(ch, *win);
    } else {
      myexit("gain_ctl");
    }
  }

  return huffspec(info, nsect, sect, factors, coef, max_spectral_line);
}


