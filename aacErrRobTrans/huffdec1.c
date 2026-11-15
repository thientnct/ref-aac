/*
  "This software module was originally developed by 
  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 

  and edited by

  Ralph Sperschneider (Fraunhofer IIS-A)

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
  Copyright(c)1996, 2000.
*/
#include "all.h"
#include "escHandling.h"
#include "portio.h"

/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, 
	       byte **group, byte *hasmask, byte **mask, byte *max_sfb, 
	       int **lpflag, int **prstflag, TNS_frame_info **tns, Float **coef)
{
  int i, tag, common_window, ch, widx, first=0, last=0, nsect;
  
  unsigned short global_gain;  /* not used in this routine */
  Info info;
  extern short *factors[2];

  SetChannel(1);
  tag = getbits(LEN_TAG);
  WriteElemToBuffer(ELEMENT_INSTANCE_TAG, tag, LEN_TAG);

  switch(id) {
  case ID_SCE:
  case ID_LFE:
    common_window = 0;
    SetCommonWindow(common_window) ;
    break;
  case ID_CPE:
    common_window = getbits(LEN_COM_WIN);
    SetCommonWindow(common_window) ;
    WriteElemToBuffer(COMMON_WINDOW, common_window, LEN_COM_WIN);
    break;
  default:
    PRINT(SE,"Unknown id %d\n", id);
    return(-1);
  }

  if ((ch = chn_config(id, tag, common_window, mip)) < 0)
    return -1;
    
  switch(id) {
  case ID_SCE:
  case ID_LFE:
    widx = mip->ch_info[ch].widx;
    first = ch;
    last = ch;
    hasmask[widx] = 0;
    break;
  case ID_CPE:
    first = ch;
    last  = mip->ch_info[ch].paired_ch;
    if (common_window) {
      widx = mip->ch_info[ch].widx;
      get_ics_info(&win[widx], &wshape[widx].this_bk, group[widx],
                   &max_sfb[widx], lpflag[widx], prstflag[widx], common_window);
      hasmask[widx] = getmask(winmap[win[widx]], group[widx], 
                              max_sfb[widx], mask[widx]);
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

  for ( i = first; i <= last; i++) {
    SetChannel(i);
    widx = mip->ch_info[i].widx;
    fltclr(coef[i], LN2);
    if(!getics(&info, common_window, &win[widx], &wshape[widx].this_bk,
               group[widx], max_sfb[widx], lpflag[widx], prstflag[widx], 
               &nsect, sect, coef[i], &global_gain, factors[i-first], tns[i]))
      return -1;
  }

  /* identify intensity sections */
  if ((id == ID_CPE) && common_window) {
    /* sectioning info is now that of right channel */
    int i, j, k, bot, top, table, is_cb, is_sfb, is_sect;
    IS_Info *iip = &mip->ch_info[last].is_info;
      
    bot=0;
    is_sfb=0;
    is_sect=0;
    for (i=0, j=0; i<nsect; i++) {
      table=sect[j];
      top=sect[j+1];
      /* is IS used?
       * (but no IS if factor[left] is zero!)
       */
      is_cb = ((table == INTENSITY_HCB) ||
               (table == INTENSITY_HCB2)) ? 1 : 0;
      if (is_cb) {
        if (debug['i'])
          PRINT(SE,"\nintensity cb %d from %d to %d\n",
                table, bot, top);
        iip->is_present = 1;
        iip->bot[is_sect] = bot;
        iip->top[is_sect] = top;
        iip->sign[is_sect] = 
          (table == INTENSITY_HCB) ? 1 : -1;
        is_sect++;
      }
        
      for (k=bot; k<top; k++) {
        if (is_cb) {
          /* [0] is left, [1] is right */
          iip->fac[is_sfb] = factors[1][is_sfb];
          if (debug['i'])
            PRINT(SE,"IS factor %4d at sfb %3d\n",
                  iip->fac[is_sfb], is_sfb);
        }
        is_sfb++;
      }
      bot = top;
      j+=2;
    }
    iip->n_is_sect = is_sect;
  }
  return 0;
}

static void nok_lt_decode (int islong, int max_sfb)
{
  int i, ltp_data_present, ltp_lag_update, ltp_lag, ltp_coef, ltp_long_used, ltp_short_used, ltp_short_lag_present, ltp_short_lag;
  
  ltp_data_present = getbits(LEN_LTP_DATA_PRESENT);
  WriteElemToBuffer(LTP_DATA_PRESENT, ltp_data_present, LEN_LTP_DATA_PRESENT);
  
  if (ltp_data_present) {
    if (low_delay) {
      ltp_lag_update = getbits(LEN_LTP_LAG_UPDATE);
      WriteElemToBuffer(LTP_LAG_UPDATE, ltp_lag_update, LEN_LTP_LAG_UPDATE);
      if (ltp_lag_update) {
        ltp_lag = getbits(LEN_LTP_LAG_LD);
        WriteElemToBuffer(LTP_LAG, ltp_lag, LEN_LTP_LAG_LD);
      }
    } else {
      ltp_lag = getbits(LEN_LTP_LAG);
      WriteElemToBuffer(LTP_LAG, ltp_lag, LEN_LTP_LAG);
    }
    ltp_coef = getbits(LEN_LTP_COEF);
    WriteElemToBuffer(LTP_COEF, ltp_coef, LEN_LTP_COEF);

    if (islong) {
      for (i=0; i<max_sfb; i++) {
        ltp_long_used = getbits(LEN_LTP_LONG_USED);
        WriteElemToBuffer(LTP_LONG_USED, ltp_long_used, LEN_LTP_LONG_USED);
      }
    } else {
      for (i=0; i<NSHORT; i++) {
        ltp_short_used = getbits(LEN_LTP_SHORT_USED);
        WriteElemToBuffer(LTP_SHORT_USED, ltp_short_used, LEN_LTP_SHORT_USED);
        if (ltp_short_used) {
          ltp_short_lag_present = getbits(LEN_LTP_SHORT_LAG_PRESENT);
          WriteElemToBuffer(LTP_SHORT_LAG_PRESENT, ltp_short_lag_present, LEN_LTP_SHORT_LAG_PRESENT);
          if (ltp_short_lag_present) {
            ltp_short_lag = getbits(LEN_LTP_SHORT_LAG);
            WriteElemToBuffer(LTP_SHORT_LAG, ltp_short_lag, LEN_LTP_SHORT_LAG);
          }
        }
      }
    } /* if (islong) */
  } /* if (ltp_data_present) */
}

void get_ics_info(byte *win, byte *wshape, byte *group,
                  byte *max_sfb, int *lpflag, int *prstflag, int common_window)
{
  int i, j;
  int max_pred_sfb;
  Info *info;

  max_pred_sfb = pred_max_bands();
  j = getbits(LEN_ICS_RESERV);	    /* reserved bit */
  WriteElemToBuffer(ICS_RESERVED, j, LEN_ICS_RESERV);
  *win = getbits(LEN_WIN_SEQ);
  WriteElemToBuffer(WINDOW_SEQ, *win, LEN_WIN_SEQ);
  *wshape = getbits(LEN_WIN_SH);
  WriteElemToBuffer(WINDOW_SHAPE_CODE, *wshape, LEN_WIN_SH);
  if ((info = winmap[*win]) == NULL)
    myexit("bad window code");

  /*
   * max scale factor, scale factor grouping and prediction flags
   */
  prstflag[0] = 0;
  if (info->islong) {
    *max_sfb = getbits(LEN_MAX_SFBL);
    WriteElemToBuffer(MAX_SFB, *max_sfb, LEN_MAX_SFBL);
    group[0] = 1;
    lpflag[0] = getbits(LEN_PRED_PRES);
    WriteElemToBuffer(PREDICTOR_DATA_PRESENT, lpflag[0], LEN_PRED_PRES);

    /* LTP data */
    if ( lpflag[0] ) {
      nok_lt_decode(info->islong, *max_sfb);
      if (common_window)
        nok_lt_decode(info->islong, *max_sfb);
    }
#if 0
    /* AAC main profile is not supported in EP mode */
    if ( lpflag[0] ) {
      prstflag[0] = getbits(LEN_PRED_RST);
      WriteElemToBuffer(PREDICTOR_RESET, prstflag[0], LEN_PRED_RST);
      if ( prstflag[0]) {
        for(i=1; i<LEN_PRED_RSTGRP+1; i++) {
          prstflag[i] = getbits(LEN_PRED_RST);
          WriteElemToBuffer(PREDICTOR_RESET_GROUP_NUMBER, prstflag[i], LEN_PRED_RST);
        }
      }
      j = ( (*max_sfb < max_pred_sfb) ? 
            *max_sfb : max_pred_sfb ) + 1;
      for (i = 1; i < j; i++) {
        lpflag[i] = getbits(LEN_PRED_ENAB);
        WriteElemToBuffer(PREDICTION_USED , lpflag[i], LEN_PRED_ENAB);
      }
      for ( ; i < max_pred_sfb+1; i++)
        lpflag[i] = /* 0 */ 1 /* SRQ FIX!!!! */;
    }
#endif
  }
  else {
    *max_sfb = getbits(LEN_MAX_SFBS);
    WriteElemToBuffer(MAX_SFB, *max_sfb, LEN_MAX_SFBS);
    getgroup(info, group);
    lpflag[0] = 0;
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
