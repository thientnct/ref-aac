/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
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
#include <stdio.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "monopredHandle.h"      /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "nok_prediction.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "buffers.h"
#include "common_m4a.h"
#include "huffdec2.h"
#include "nok_lt_prediction.h"
#include "port.h"

#include "allVariables.h"        /* variables */
/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int huffdecode ( int                      id, 
                 MC_Info*                 mip, 
                 WINDOW_SEQUENCE*             win, 
                 Wnd_Shape*               wshape, 
                 byte**                   cb_map, 
                 short**                  factors, 
                 byte**                   group, 
                 byte*                    hasmask, 
                 byte**                   mask, 
                 byte*                    max_sfb, 
                 PRED_TYPE                pred_type, 
                 int**                    lpflag, 
                 int**                    prstflag,
                 NOK_LT_PRED_STATUS**     nok_ltp_status, 
                 TNS_frame_info**         tns, 
                 BsBitStream*             gc_stream[], 
                 Float**                  coef, 
                 enum AAC_BIT_STREAM_TYPE bitStreamType,
                 int                      common_window,
                 Info*                    sfbInfo,
                 HANDLE_RESILIENCE        hResilience,
                 HANDLE_BUFFER            hVm,
                 HANDLE_BUFFER            hHcrSpecData,
                 HANDLE_HCR               hHcrInfo,
                 HANDLE_EP_INFO           hEpInfo,
                 HANDLE_CONCEALMENT       hConcealment,
                 QC_MOD_SELECT qc_select)
{
  int i, tag, ch, widx, first=0, last=0, chCnt;

  short global_gain;  /* not used in this routine */
  if (bitStreamType != SCALABLE) {
    tag = GetBits ( ELEMENT_INSTANCE_TAG, 
                    LEN_TAG,
                    hResilience,
                    hVm, 
                    hEpInfo );   
  }  else {
    tag=0;
  }
   


  switch(id) {
  case ID_SCE:
  case ID_LFE:
    common_window = 0;
    break;
  case ID_CPE:
    if (bitStreamType != SCALABLE) {
      common_window = GetBits ( COMMON_WINDOW, 
                                LEN_COM_WIN,
                                hResilience,
                                hVm, 
                                hEpInfo );
    }
    break;
  default:
    fprintf(stderr,"Unknown id %d\n", id);
    return(-1);
  }
  
  if ((ch = chn_config(id, tag, common_window, mip)) < 0) {
    CommonWarning("Number of channels negativ (huffdec1.c)");
    return -1;
    }

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
    last = mip->ch_info[ch].paired_ch;
    if ( (common_window) && ( bitStreamType != SCALABLE )) {
      widx = mip->ch_info[ch].widx;
      get_ics_info ( &win[widx], 
                     &wshape[widx].this_bk, 
                     group[widx],
                     &max_sfb[widx], 
                     pred_type, 
                     lpflag[widx], 
                     prstflag[widx], 
                     bitStreamType,
                     hResilience,
                     hVm, 
                     hEpInfo,
                     nok_ltp_status[first-1],
		     nok_ltp_status[last-1],
		     common_window,
                     qc_select);

      hasmask[widx] = getmask ( winmap[win[widx]], 
                                group[widx], 
                                max_sfb[widx], 
                                mask[widx],
                                hResilience,
                                hVm, 
                                hEpInfo );
    }
    else { 
      hasmask[mip->ch_info[first].widx] = 0;
      hasmask[mip->ch_info[last].widx] = 0;
    }
    break;
  }
  
  if(debug['v']) {
    fprintf(stderr,"tag %d, common window %d\n", tag, common_window);
    fprintf(stderr,"nch %d, channels %d %d, widx %d %d\n",
          last-first+1, first, last, 
          mip->ch_info[first].widx, mip->ch_info[last].widx);
  }
  
  chCnt = 0;
  for (i=first; i<=last; i++) {
    if (bitStreamType != SCALABLE) 
      widx = mip->ch_info[i].widx;
    else
      widx = 0;

    fltclr(coef[i], LN2);
    if ( ! getics ( sfbInfo, 
                    common_window, 
                    &win[widx], 
                    &(wshape[widx].this_bk),
                    group[widx], 
                    &max_sfb[widx], 
                    pred_type, 
                    lpflag[widx], 
                    prstflag[widx], 
                    cb_map[i], 
                    coef[i], 
                    &global_gain, 
                    factors[i], 
                    nok_ltp_status[chCnt], 
                    tns[i],
                    gc_stream[i-first], 
                    bitStreamType,
                    hResilience,
                    hVm,
                    hHcrSpecData,
                    hHcrInfo, hEpInfo,
                    hConcealment,
                    qc_select) ) 
      {
        CommonWarning("getics returns error (huffdec1.c)");
        return -1;
      }
    chCnt++;
  }
  return 0;
}

void get_ics_info ( WINDOW_SEQUENCE*          win, 
                    WINDOW_SHAPE*             wshape, 
                    byte*                     group,
                    byte*                     max_sfb, 
                    PRED_TYPE                 pred_type, 
                    int*                      lpflag, 
                    int*                      prstflag, 
                    enum  AAC_BIT_STREAM_TYPE bitStreamType,
                    HANDLE_RESILIENCE         hResilience,
                    HANDLE_BUFFER             hVm,
                    HANDLE_EP_INFO            hEpInfo,
                    NOK_LT_PRED_STATUS*       nok_ltp_left,
		    NOK_LT_PRED_STATUS*       nok_ltp_right,
		    int                       stereoFlag,
                    QC_MOD_SELECT             qc_select)

{
  int i, j;
  int max_pred_sfb = pred_max_bands();
  Info *info;

  if (bitStreamType != SCALABLE) {
    int tmp;

    GetBits ( ICS_RESERVED, 
              LEN_ICS_RESERV,
              hResilience,
              hVm, 
              hEpInfo );	    /* reserved bit */
     *win    = (WINDOW_SEQUENCE)GetBits ( WINDOW_SEQUENCE_CODE, 
                                          LEN_WIN_SEQ,
                                          hResilience,
                                          hVm, 
                                          hEpInfo );

    tmp = GetBits ( WINDOW_SHAPE_CODE, 
                    LEN_WIN_SH,
                    hResilience,
                    hVm, 
                    hEpInfo );
       *wshape = (WINDOW_SHAPE)tmp;
     
  }
  if ( ( info = winmap[*win] ) == NULL ) {
    CommonExit(1,"bad window code");
  }
  /*
   * max scale factor, scale factor grouping and prediction flags
   */
  prstflag[0] = 0;
  if (info->islong) {
    *max_sfb = GetBits ( MAX_SFB, 
                         LEN_MAX_SFBL,
                         hResilience,
                         hVm, 
                         hEpInfo );
    group[0] = 1;

    switch(pred_type) {
    case PRED_NONE:
    case MONOPRED:
    case NOK_BWP:

      if ((lpflag[0] = GetBits ( PREDICTOR_DATA_PRESENT, 
                                 LEN_PRED_PRES,
                                 hResilience,
                                 hVm, 
                                 hEpInfo ))) {
        if ((prstflag[0] = GetBits ( PREDICTOR_RESET, 
                                     LEN_PRED_RST,
                                     hResilience,
                                     hVm, 
                                     hEpInfo ))) {
          for(i=1; i<LEN_PRED_RSTGRP+1; i++)
            prstflag[i] = GetBits ( PREDICTOR_RESET_GROUP_NUMBER, 
                                    LEN_PRED_RST,
                                    hResilience,
                                    hVm, 
                                    hEpInfo );
        }
        j = ( (*max_sfb < max_pred_sfb) ? 
              *max_sfb : max_pred_sfb ) + 1;
        
        for (i = 1; i < j; i++)
	  lpflag[i] = GetBits ( PREDICTION_USED, 
                                LEN_PRED_ENAB,
                                hResilience,
                                hVm, 
                                hEpInfo );  
        for ( ; i < max_pred_sfb+1; i++)
          lpflag[i] = 0;

      }       
      break;

    case NOK_LTP:
      if(GetBits(LTP_DATA, 1, hResilience, hVm, hEpInfo)) {
	nok_lt_decode(*win, max_sfb, 
		      nok_ltp_left->sbk_prediction_used,
		      nok_ltp_left->sfb_prediction_used, 
		      &nok_ltp_left->weight,
		      nok_ltp_left->delay,
		      hResilience, hVm, hEpInfo, 
		      qc_select, NO_CORE, -1);

	if(stereoFlag)
	  nok_lt_decode(*win, max_sfb, 
			nok_ltp_right->sbk_prediction_used,
			nok_ltp_right->sfb_prediction_used, 
			&nok_ltp_right->weight,
			nok_ltp_right->delay,
			hResilience, hVm, hEpInfo, 
			qc_select, NO_CORE, -1);
      }
      else
      {
	nok_ltp_left->sbk_prediction_used[0] = 0;
	if(stereoFlag)
	  nok_ltp_right->sbk_prediction_used[0] = 0;
      }
      break;
      
    default:
      CommonExit(-1,"\nUnknown predictor type");
      break;
    }
  }

  else {  /* EIGHT_SHORT_SEQUENCE */
    *max_sfb = GetBits ( MAX_SFB, 
                         LEN_MAX_SFBS,
                         hResilience,
                         hVm, 
                         hEpInfo );
    getgroup ( info, 
               group, 
               hResilience,
               hVm, 
               hEpInfo );
    lpflag[0] = 0;

    /* Reset LTP flags. */
    if(pred_type == NOK_LTP)
    {
      nok_ltp_left->sbk_prediction_used[0] = 0;
      if(stereoFlag)
	nok_ltp_right->sbk_prediction_used[0] = 0;
    }
  }

  if ( *max_sfb > info->sfb_per_sbk[0] ) {
      CommonExit ( 2, "get_ics_info: max_sfb (%2d) > sfb_per_sbk (%2d) (huffdec1.c)", *max_sfb, info->sfb_per_sbk[0]);
  }
  
  if(debug['v']) {
    fprintf(stderr,"win %d, wsh %d\n", *win, *wshape);
    fprintf(stderr,"max_sf %d\n", *max_sfb);
  }
  
  if (debug['p']) {
    if (lpflag[0]) {
      fprintf(stderr,"prediction enabled (%2d):  ", *max_sfb);
      for (i=1; i<max_pred_sfb+1; i++)
        fprintf(stderr," %d", lpflag[i]);
      fprintf(stderr,"\n");
    }
  }
}
