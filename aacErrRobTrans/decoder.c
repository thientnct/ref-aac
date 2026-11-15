/*
  "This software module was originally developed by 

  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 

  and edited by

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
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
  Copyright(c)1996, 1999, 2000.
*/

#include "all.h"
#include "statistics.h"
#ifdef	DOLBY_MDCT
#include "block.h"
#include "dolby_def.h"
#include "escHandling.h"
#include "portio.h"
#endif

#define ID_FIL_SUPPORT
/* #define ID_FIL_WRITE_SUPPORT */

 
/* prediction */
static	PRED_STATUS	*sp_status[Chans];

/* error protection */
static ESC_INSTANCE_DATA *escInstanceData;


void init ( )
{
#ifndef	DOLBY_MDCT
  imdctinit();
#endif
#if (CChan > 0)
  init_cc();
#endif
  huffdecinit();
  huffbookinit();
  predinit();
  restarttio();
  if ( debug ['S'])
    StatisticsInit();

  winmap[0] = win_seq_info[ONLY_LONG_WINDOW];
  winmap[1] = win_seq_info[ONLY_LONG_WINDOW];
  winmap[2] = win_seq_info[EIGHT_SHORT_WINDOW];
  winmap[3] = win_seq_info[ONLY_LONG_WINDOW];      
}

void predinit ( )
{
  int i, ch;
  for (ch = 0; ch < Chans; ch++) {
    for (i = 0; i < LN2; i++) {
      init_pred_stat(&sp_status[ch][i],
                     PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
    }
  }
}

int getdata(int *tag, int *dt_cnt, byte *data_bytes)
{
  int i, align_flag, cnt, j;

  *tag = getbits(LEN_TAG);
#ifdef ID_DSE_WRITE_SUPPORT
  WriteElemToBuffer(ELEMENT_INSTANCE_TAG, *tag,  LEN_TAG);
#endif
  align_flag = getbits(LEN_D_ALIGN);
#ifdef ID_DSE_WRITE_SUPPORT
  WriteElemToBuffer(DATA_BYTE_ALIGN_FLAG, align_flag, LEN_D_ALIGN);
#endif
  cnt = getbits(LEN_D_CNT);
#ifdef ID_DSE_WRITE_SUPPORT
  WriteElemToBuffer(COUNT, cnt, LEN_D_CNT);
#endif
  if ( cnt == (1<<LEN_D_CNT)-1) {
    j = getbits(LEN_D_ESC);
    /*WriteElemToBuffer(ESC_COUNT, j, LEN_D_ESC);*/
    cnt +=  j;
  }
  *dt_cnt = cnt;
  if (debug['x'])
    PRINT(SE, "data element %d has %d bytes\n", *tag, cnt);
  if (align_flag)
    byte_align();

  for (i=0; i<cnt; i++) {
    data_bytes[i] = getbits(LEN_BYTE);
    /*WriteElemToBuffer(DATA_STREAM_BYTE, data_bytes[i], LEN_BYTE);*/
    if (debug['X'])
      PRINT(SE, "%6d %2x\n", i, data_bytes[i]);
  }
    
  return 0;
}

void getfill ( )
{
  int i, cnt, cntHelp, j;
  
  cnt = getbits(LEN_F_CNT);
#ifdef ID_FIL_WRITE_SUPPORT
  WriteElemToBuffer(COUNT, cnt, LEN_F_CNT);
#endif
  if (cnt == (1<<LEN_F_CNT)-1) {
    cntHelp = getbits(LEN_F_ESC);
#ifdef ID_FIL_WRITE_SUPPORT
    WriteElemToBuffer(ESC_COUNT, cntHelp, LEN_F_ESC);
#endif
    cnt +=  cntHelp - 1;
  }
  if (debug['x'])
    PRINT(SE, "fill element has %d bytes\n", cnt);
  for (i=0; i<cnt; i++) {
    j = getbits(LEN_BYTE);
#ifdef ID_FIL_WRITE_SUPPORT
    WriteElemToBuffer(FILL_BYTE, j, LEN_BYTE);
#endif
  }
}

int main(int argc, char *argv[])
{
  Float *coef[Chans], *data[Chans], *state[Chans];
  byte hasmask[Winds], *mask[Winds], *group[Chans],
    wnd[Chans], max_sfb[Chans];
  Wnd_Shape wnd_shape[Chans];
  int *lpflag[Chans], *prstflag[Chans];
  TNS_frame_info *tns[Chans];
  
#if (CChans > 0)
  Float *cc_coef[CChans], *cc_gain[CChans][Chans];
  byte cc_wnd[CChans]
    Wnd_Shape cc_wnd_shape[CChans];
#if (ICChans > 0)
  Float *cc_data[ICChans], *cc_state[ICChans];
#endif
#endif

  int i, j,  ch, wn, ele_id;
#ifdef ID_DSE_WRITE_SUPPORT
  byte d_bytes[Avjframe];
  int  d_tag, d_cnt;
#endif
  int left, right;
  Info *info;
  MC_Info *mip = &mc_info;
  Ch_Info *cip;
  unsigned short firstTime = 1; 
   
  for(i=0; i<Chans; i++) {
    coef[i] = (Float *)mal1(LN2*sizeof(*coef[0]));
    data[i] = (Float *)mal1(LN2*sizeof(*data[0]));
    state[i] = (Float *)mal1(LN*sizeof(*state[0]));	/* changed LN4 to LN 1/97 mfd */
    fltclr(state[i], LN);
    group[i] = (byte *)mal1(NSHORT*sizeof(group[0]));
    lpflag[i] = (int *)mal1(MAXBANDS*sizeof(*lpflag[0]));
    prstflag[i] = (int *)mal1((LEN_PRED_RSTGRP+1)*sizeof(*prstflag[0])); 
    tns[i] = (TNS_frame_info *)mal1(sizeof(*tns[0]));
    sp_status[i]  = (PRED_STATUS *)mal1(LN*sizeof(*sp_status[0]));
    wnd_shape[i].prev_bk = 0;
  }
  for(i=0; i<Winds; i++) {
    mask[i] = (byte *)mal1(MAXBANDS*sizeof(mask[0]));
  }
#if (CChans > 0)
  for(i=0; i<CChans; i++){
    cc_coef[i] = (Float *)mal1(LN2*sizeof(*cc_coef[0]));
    for(j=0; j<Chans; j++)
      cc_gain[i][j] = (Float *)mal1(MAXBANDS*sizeof(*cc_gain[0][0]));
#if (ICChans > 0)
    if (i < ICChans) {
      cc_data[i] = (Float *)mal1(LN2*sizeof(*data[0]));
      cc_state[i] = (Float *)mal1(LN*sizeof(*state[0]));  /* changed LN4 to LN 1/97 mfd */
      fltclr(cc_state[i], LN);
      cc_wnd_shape[i].prev_bk = 0;
    }
#endif
  }
#endif

  initio ( argc, argv );  /* parse command line */
  init();		    /* initialize data structures */
  escInstanceData = GetEscInstanceData();

  InitEscInstanceBuffers ( escInstanceData ); /* init buffers for ESC instances */

  /* main loop */
  for(bno = 0;; bno++) {
    if(debug['n']){
      PRINT(SE, "\n-------\nBlock: %ld\n", bno);
    }
    /* call transport layer */
    if(startblock() < 0) 
      myexit(0);

    reset_mc_info(mip);
       
    while ( 1 ) {
      ele_id = getbits ( LEN_SE_ID );
      if (firstTime)
      {
        SetElementID (ele_id);
        firstTime = 0;
      }

      if ( ele_id == ID_END ) {
      /*if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID );
        }*/
        break;
      }
      /* get audio syntactic element */
      if(debug['v'])
        PRINT(SE, "\nele_id %d\n", ele_id);
      switch (ele_id) {
      case ID_SCE:		/* single channel */
      case ID_CPE:		/* channel pair */
      case ID_LFE:		/* low freq effects channel */                  
        /*if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID);
        }*/
        if (huffdecode(ele_id, mip, wnd, wnd_shape, group, 
                       hasmask, mask, max_sfb, lpflag, prstflag, tns,
                       coef) < 0)
          myexit("huffdecode");
        break;
#if (CChans > 0)
#ifdef ID_CCE_SUPPORT
      case ID_CCE:		/* coupling channel */
#ifdef ID_CCE_WRITE_SUPPORT
        if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID);
        }
#endif
        if (getcc(mip, cc_wnd, cc_wnd_shape, cc_coef, cc_gain) < 0)
          myexit("getcc");
        break;
#endif
#endif /* (CChans > 0) */
#ifdef ID_DSE_SUPPORT
      case ID_DSE:		/* data element */
#ifdef ID_DSE_WRITE_SUPPORT
        if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID);
        }
#endif
        if (getdata(&d_tag, &d_cnt, d_bytes) < 0)
          myexit("data channel");
        break;
#endif
#ifdef ID_PCE_SUPPORT
      case ID_PCE:		/* program config element */
#ifdef ID_PCE_WRITE_SUPPORT
        if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID);
        }
#endif
        get_prog_config(&prog_config);
        break;
#endif
#ifdef ID_FIL_SUPPORT
      case ID_FIL:		/* fill element */
#ifdef ID_FIL_WRITE_SUPPORT
        if ( GetInfFilePtr() == NULL ) {
          WriteElemToBuffer ( ID_SYN_ELE, ele_id, LEN_SE_ID);
        }
#endif
        getfill();
        break;
#endif
      default:
        PRINT(SE, "Element not supported: %d\n", ele_id);
        myexit("program terminated");
        break;
      }
    }
    check_mc_info(mip, (bno==0 && default_config));

    /* call transport layer */
    if(endblock() < 0)
      myexit("endblock");

#if (ICChans > 0)
    /* transform independently switched coupling channels */
    ind_coupling(mip, wnd, wnd_shape, cc_wnd, cc_wnd_shape, cc_coef);
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
          synt(info, group[wn], mask[wn], coef[right], coef[left]);
        }
      }
    }

    /* intensity stereo and prediction */
    for (ch=0; ch<Chans; ch++) {
      if (!(mip->ch_info[ch].present)) continue;
      wn = mip->ch_info[ch].widx;
      info = winmap[wnd[wn]];
      intensity ( mip, info, lpflag[wn], ch, coef );
#if 0
      /* AAC main profile is not supported in EP mode */
      predict(info, mip->profile, 
              lpflag[wn], sp_status[ch], coef[ch]);
#endif
    }
	
    for (ch=0; ch<Chans; ch++) {
      if (!(mip->ch_info[ch].present)) continue;
      wn = mip->ch_info[ch].widx;
      info = winmap[wnd[wn]];

      /* predictor reset */
      left = ch;
      right = left;
      if ( (mip->ch_info[ch].cpe) &&
           (mip->ch_info[ch].common_window) )
        /* prstflag's shared by channel pair */
        right = mip->ch_info[ch].paired_ch;
      predict_reset(info, prstflag[wn], sp_status, left, right);

#if (CChans > 0)
      /* if cc_domain indicates before TNS */
      coupling(mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

      /* tns */
      for (i=j=0; i<tns[ch]->n_subblocks; i++) {
        if (debug['T']) {
          PRINT(SE, "%ld %d %d\n", bno, ch, i);
          print_tns( &(tns[ch]->info[i]));
        }

        tns_decode_subblock(&coef[ch][j],
                            info->sbk_sfb_top[i],
                            info->islong,
                            &(tns[ch]->info[i]) );
        j += info->bins_per_sbk[i];
      }

#if (CChans > 0)
      /* if cc_domain indicated after TNS */
      coupling(mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

#ifdef DOLBY_MDCT
      /* inverse transform */
      freq2time_adapt (wnd [wn], &wnd_shape [wn], coef [ch], state [ch], data [ch]);
#else 
      imdct(wnd[wn], wnd_shape[wn].this_bk, coef[ch], state[ch], data[ch]);
      {
        /* scale imdct output */
        Float scale = 1.0 / sqrt (2.0);
        for (i = 0; i < LN2; i++)
          data[ch][i] *= scale;
      }
#endif
#if (CChans > 0)
      /* independently switched coupling */
      coupling(mip, coef, cc_coef, cc_gain, ch, CC_DOM, CC_IND);
#endif
	}
    /* frame ready, write it to file */
    WriteBufferToFile ( escInstanceData );
    WriteInfoFile     ( escInstanceData );
    /* skip first two blocks so output is time aligned with input, only pcm file! */
#ifdef PCM_OUTPUT
    if (bno > 1) {
      writeout(data, mip);
    }
#endif /*PCM_OUTPUT*/
  }
}
