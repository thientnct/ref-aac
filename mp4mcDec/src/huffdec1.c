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
#include "config.h"
#include "diagnose.h"
#include "huffdec.h"
#include "huffdec_intern.h"
#include "intrins.h"
#include "bitinput.h"

#ifdef MPEG4V1
#include "nok_lt_prediction.h"
#endif

#ifdef FRAMELENGTH_FLAG
extern int frameLengthFlag;
extern int copy_to_output;
#endif

/*
 * read a synthesis mask
 *  uses EXTENDED_MS_MASK
 *  and grouped mask 
 */
static int
getmask(Info *info, byte *group, byte max_sfb, byte *mask)
{
  int b, i, mp;

  mp = getbits(LEN_MASK_PRES);
  if( debug['m'] )
    PRINT(SE,"\nExt. Mask Present: %d\n",mp);

  /* special EXTENDED_MS_MASK cases */
  if(mp == 0) { /* no ms at all */
    return 0;
  }
  if(mp == 2) {/* MS for whole spectrum on, mask bits set to 1 */
    for(b = 0; b < info->nsbk; b = *group++)
      for(i = 0; i < info->sfb_per_sbk[b]; i ++)
        *mask++ = 1;
    return 2;
  }

  /* otherwise get mask */
  for(b = 0; b < info->nsbk; b = *group++){
    if( debug['m'] ) PRINT(SE," gr%1i:",b);
    for(i = 0; i < max_sfb; i ++) {

#ifdef FRAMELENGTH_FLAG
      if ( frameLengthFlag == 1 ) {
        if ( i >= maxSfb960FrameLen[mc_info.sampling_rate_idx] ) {
          copy_to_output = 0;
        }
      }
#endif

      *mask = getbits(LEN_MASK);
      if( debug['m'] )PRINT(SE,"%1i",*mask);
      mask++;
    }
    for( ; i < info->sfb_per_sbk[b]; i++){
      *mask = 0;
      if( debug['m'] ) PRINT(SE,"%1i",*mask);
      mask++;
    }
  }

#ifdef FRAMELENGTH_FLAG
  if ( frameLengthFlag == 1 ) {
    copy_to_output = 1;
  }
#endif

  if( debug['m'] ) PRINT(SE,"\n");
  return 1;
}


/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int
huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, 
    byte **cb_map, short **factors, 
    byte **group, byte *hasmask, byte **mask, byte *max_sfb, 
    int **lpflag, int **prstflag,
#ifdef MPEG4V1
    NOK_LT_PRED_STATUS **nok_ltp_status, 
#endif
    TNS_frame_info **tns, Float **coef
#ifdef CT_SBR
    , int* chIndx
#endif 
    ,int* first_cpe
    )
{
    int i, tag, common_window, ch, widx, first=0, last=0;

    int max_spectral_line=0;
    short global_gain;  /* not used in this routine */
    Info info;

    tag = getbits(LEN_TAG);

    switch(id) {
	case ID_SCE:
	case ID_LFE:
	    common_window = 0;
	    break;
	case ID_CPE:
	    common_window = getbits(LEN_COM_WIN);
	    break;
	default:
	    PRINT(SE,"Unknown id %d\n", id);
	    return(-1);
    }

    if ((ch = chn_config(id, tag, common_window, mip, first_cpe)) < 0)
	    return -1;

#ifdef CT_SBR
    *chIndx = ch;
#endif

    if ( ch != Chans - XChans ) {
      if (id==ID_CPE) set_log_level(id, LEN_COM_WIN+LEN_TAG, (tag<<LEN_COM_WIN)|common_window);
      else set_log_level(id, LEN_TAG, tag);
    } else set_log_level(ID_NUL, LEN_TAG, tag);  /* required for CRC and ADIF2MP4 */

    switch(id) {
	case ID_LFE:
            max_spectral_line=12;
	case ID_SCE:
	    widx = mip->ch_info[ch].widx;
	    first = ch;
	    last = ch;
	    hasmask[widx] = 0;
            memcpy(&info, winmap[win[widx]], sizeof(Info));
	    break;
	case ID_CPE:
	    first = ch;
	    last = mip->ch_info[ch].paired_ch;
      if (common_window) {
        widx = mip->ch_info[ch].widx;
        if ( ch == Chans - XChans ) win[widx]=(unsigned char)-1; /* scratch channel ignores old window */
        get_ics_info(&win[widx], &wshape[widx].this_bk, group[widx],
                     &max_sfb[widx], lpflag[widx], prstflag[widx],
#ifdef MPEG4V1
                     nok_ltp_status[widx],nok_ltp_status[mip->ch_info[ch].paired_ch], common_window,
#endif
                     &info);
        win[last] = win[widx];
        hasmask[widx] = getmask(winmap[win[widx]], group[widx], max_sfb[widx], mask[widx]);
      }
	    else { 
        hasmask[mip->ch_info[first].widx] = 0;
        hasmask[mip->ch_info[last].widx] = 0;
	    }
	    break;
    }

    if(debug['v']) {
      PRINT(SE,"tag %d, common window %d\n", tag, common_window);
      PRINT(SE,"nch %d, channels %d %d, widx %d %d\n",
      last-first+1, first, last, 
      mip->ch_info[first].widx, mip->ch_info[last].widx);
    }

    for (i=first; i<=last; i++) {
      widx = mip->ch_info[i].widx;
      fltclr(coef[i], LN2);

      if ((i==last) && (id==ID_CPE)) set_log_level(ID_2ND, 0, 0);  /* required for CRC and ADIF2MP4 */
      if ((ch >= Chans - XChans)&&(!common_window)) win[widx]=(unsigned char)-1; /* scratch channel ignores old window */

      if(!getics(&info, common_window, &win[widx], &wshape[widx].this_bk,
                 group[widx], &max_sfb[widx], lpflag[widx], prstflag[widx], 
                 cb_map[i], coef[i], &global_gain, factors[i],
#ifdef MPEG4V1
                   nok_ltp_status[widx],
#endif
                   tns[i], i, max_spectral_line))
        return -1;

      if (id==ID_LFE) {
        if (win[widx]!=ONLY_LONG_WINDOW) PRINT(SE, "WARNING: only long windows are allowed in LFEs (winseq=%i)\n",win[widx]);
        if (wshape[widx].this_bk!=0) PRINT(SE, "WARNING: only sine shaped windows are allowed in LFEs (winshape=%i)\n",wshape[widx].this_bk);
      }
    }

    set_log_level(LOG_FIN, 0, 0);  /* required for CRC and ADIF2MP4 */

    return 0;
}

void
get_ics_info(byte *win, byte *wshape, byte *group,
             byte *max_sfb, int *lpflag, int *prstflag,
#ifdef MPEG4V1
             NOK_LT_PRED_STATUS *nok_ltp_status, NOK_LT_PRED_STATUS *nok_ltp_status_right, int stereo_flag ,
#endif
             Info *wininfo)
{
  int i, j;
  int max_pred_sfb = pred_max_bands();
  Info *info;

  i = getbits(LEN_ICS_RESERV);	    /* reserved bit */
  if (i) PRINT(SE,"WARNING: ICS - reserved bit is not set to 0\n");
  i=*win;
  *win = getbits(LEN_WIN_SEQ);
  if (debug['w']) PRINT(SE,"winseq: %i -> %i\n",i,*win);
  if (bno>0)
    if ((((i==ONLY_LONG_WINDOW)||(i==LONG_STOP_WINDOW))&&((*win==EIGHT_SHORT_WINDOW)||(*win==LONG_STOP_WINDOW)))||
        (((i==LONG_START_WINDOW)||(i==EIGHT_SHORT_WINDOW))&&((*win==ONLY_LONG_WINDOW)||(*win==LONG_START_WINDOW))))
      PRINT(SE,"Non meaningfull window sequence transistion\n");
  *wshape = getbits(LEN_WIN_SH);
  if ((info = winmap[*win]) == NULL)
    myexit("bad window code");

  memcpy(wininfo, info, sizeof(Info));

  /*
   * max scale factor, scale factor grouping and prediction flags
   */
  prstflag[0] = 0;
  if (info->islong) {

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag == 1 ) {
      int max960Sfb = maxSfb960FrameLen[mc_info.sampling_rate_idx];
      copy_to_output = 0;
      *max_sfb = getbits(LEN_MAX_SFBL);
      writebits(LEN_MAX_SFBL, (*max_sfb >= max960Sfb)?max960Sfb:*max_sfb); /* only a hack for 32 khz !!! use table later !!! (btg) */
      copy_to_output = 1;
    }
    else
      *max_sfb = getbits(LEN_MAX_SFBL);
#else
      *max_sfb = getbits(LEN_MAX_SFBL);
#endif
    group[0] = 1;
#ifdef MPEG4V1
    if(mc_info.profile == LTP_Profile)
    {
      if(getbits(1))
      {
        ltp_present = 1;
        nok_lt_decode(*max_sfb, nok_ltp_status->sbk_prediction_used,
        nok_ltp_status->sfb_prediction_used,
        &nok_ltp_status->weight,
        nok_ltp_status->delay);
    
        if(stereo_flag)
          nok_lt_decode(*max_sfb, nok_ltp_status_right->sbk_prediction_used,
        nok_ltp_status_right->sfb_prediction_used,
        &nok_ltp_status_right->weight,
        nok_ltp_status_right->delay);
      }
      else
      {
        nok_ltp_status->sbk_prediction_used[0] = 0;
        if(stereo_flag)
          nok_ltp_status_right->sbk_prediction_used[0] = 0;
      }
    }
    else
#endif
      if ((lpflag[0] = getbits(LEN_PRED_PRES))) {
        if ((prstflag[0] = getbits(LEN_PRED_RST))) {
          for(i=1; i<LEN_PRED_RSTGRP+1; i++)
          prstflag[i] = getbits(LEN_PRED_RST);
        }
        j = ( (*max_sfb < max_pred_sfb) ? 
        *max_sfb : max_pred_sfb ) + 1;
        for (i = 1; i < j; i++)
          lpflag[i] = getbits(LEN_PRED_ENAB);
        for ( ; i < max_pred_sfb+1; i++)
          lpflag[i] = 0;
      }
    }
    else {
      *max_sfb = getbits(LEN_MAX_SFBS);
      getgroup(info, group);
      lpflag[0] = 0;
#ifdef MPEG4V1
       nok_ltp_status->sbk_prediction_used[0] = 0;
      if(stereo_flag)
        nok_ltp_status_right->sbk_prediction_used[0] = 0;
#endif
    }

    if(debug['v']) {
      PRINT(SE,"win %d, wsh %d\n", *win, *wshape);
      PRINT(SE,"max_sf %d\n", *max_sfb);
    }

    if (debug['p']) {
      if (lpflag[0]) {
        PRINT(SE,"prediction enabled (%2d):  ", *max_sfb);
        for (i=1; i<max_pred_sfb+1; i++)
          PRINT(SE," %d", lpflag[i]);
      PRINT(SE,"\n");
    }
  }
}

