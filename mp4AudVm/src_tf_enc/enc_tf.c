/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) 
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
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- June-96  */

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 * Authors:
 * BG    Bernhard Grill, Fraunhofer Gesellshaft / University of Erlangen
 * HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover,de>
 * OS    Otto Schnurr, Motorola <schnurr@email.mot.com>
 * NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
 * TK    Takashi Koike, Sony Corporation <koike@av.crl.sony.co.jp>
 * SK    Sang-Wook Kim, Samsung, <swkim@dspsun.sait.samsung.co.kr>
 * MS    Mikko Suonio, Nokia, <Mikko.Suonio@research.nokia.com>
 *
 *
 * Changes:
 * fixed bit stream handling: Heiko Purnhagen, Uni Hannover, 960705 
 * 29-jul-96   OS   Modified source for 80 column width display.
 * 01-aug-96   OS   Integrated HQ source code module.
 *                  Integrated original TF mapping source code module.
 *                  Re-activated command line strings for run-time selection
 *                  of these modules.
 * 19-aug-96   BG   merge + inclusion of window switching / first short block support 
 * 21-aug-96   HP   renamed to enc_tf.c, adapted to new enc.h
 * 26-aug-96   HP   updated to vm0_tf_02a
 * HP 960826   spectra_index2sf_index alloc bug fix (still repeated alloc !!!)
 * 28-aug-96   NI   Integrated NTT's VQ coder.
 *                  Provided automatic window switching.
 * 05-feb-97   TK   integrated AAC compliant pre processing module
 * 18-apr-97   NI   Integrated scalable coder.
 * 09-may-97   HP   merged contributions by NTT, Sony, FhG
 * 16-may-97   HP   included bug fix by TK (Sony)
 * 28-may-97   HP   -nttScl help
 * 17-Jul-97   NI   add include file for VQ encoder
 * 25-aug-97   NI   bug fixes
 * 29-aug-97   SK   added BSAC
 * 11-sep-97   HP   merged BSAC into current VM
 * 23-oct-97   HP   merged Nokia's predictor 971013 & 971020
 *                  tried to fix nok_PredCalcPrediction() call, PRED_TYPE
 * 26-nov-97   MS   adjusted AAC grouping, energy and allowed distortion
 *                  calculations
 * 08-apr-98   HP   removed LDFB
 ******************************************************************************************/

#include <limits.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"
#include "tf_mainHandle.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "enc_tf.h"
#include "tf_main.h"
#include "psych.h"
#include "common_m4a.h"	/* common module */
/* son_AACpp */
#include "sony_local.h"

/* LTP */
#include "nok_ltp_enc.h"

/* aac */
#include "tns3.h"
#include "aac_qc.h"
#include "scaleable.h"

#ifdef VERSION2
/* SAM BSAC */ /* YB : 970825 */
#include "sam_encode.h"
#endif

/* NTT VQ: */
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "ntt_encode.h"
#include "mat_def_ntt.h"
#include "ntt_nok_pred.h"
#include "plotmtv.h"
ntt_PARAM *test_pointer;


static int     block_size_samples = 1024;  /* nr of samples per block in one! audio channel */
static int     coreDelay=-1; /* delay in samples for the timesignal of the core coder , maximum is the block_size_samles */
static int     med_win_in_long    = 2;     /* for NTT */
static int     short_win_in_long  = 8;
static int     bw_lines_long, bw_lines_med, bw_lines_short;  /* no of lines of the spectrum which are processed */
static int     start_line = 0;
static int     max_ch;    /* no of of audio channels */
static int     debugLevel;
static double *spectral_line_vector[MAX_TIME_CHANNELS];
static double *baselayer_spectral_line_vector[MAX_TIME_CHANNELS];
static double *reconstructed_spectrum[MAX_TIME_CHANNELS];
static double *ntt_ms_buf[MAX_TIME_CHANNELS];
static double *overlap_buffer[MAX_TIME_CHANNELS];
static double *DTimeSigBuf[MAX_TIME_CHANNELS];
static double *DTimeSigLookAheadBuf[MAX_TIME_CHANNELS];
static float *coreTimeSigBuf[MAX_TIME_CHANNELS];
static float *coreTimeSigLookAheadBuf[MAX_TIME_CHANNELS];
static TNS_INFO *tnsInfo[MAX_TIME_CHANNELS];

/* static variables for the pre processing module */
/* son_AACpp */
static int     max_band[MAX_TIME_CHANNELS];
static double *spectral_line_vector_for_gc[MAX_TIME_CHANNELS];
static double *DTimeSigLookAheadBufForGC[MAX_TIME_CHANNELS];
static double *DTimeSigPQFDelayCompensationBuf[MAX_TIME_CHANNELS];
static double **DBandSigLookAheadBuf[MAX_TIME_CHANNELS];
static double **DBandSigBufForGCAnalysis[MAX_TIME_CHANNELS];
static double **DBandSigWithGCBuf[MAX_TIME_CHANNELS];
static GAINC  **gainInfo[MAX_TIME_CHANNELS];

/* static variables used by the T/F mapping */
static enum WIN_SWITCH_MODE win_switch_mode = STATIC_SHORT;
static WINDOW_SHAPE windowShape[MAX_TIME_CHANNELS]     = {WS_FHG};
static WINDOW_SHAPE windowShapePrev[MAX_TIME_CHANNELS] = {WS_FHG};

static WINDOW_SEQUENCE last_windowSequence[MAX_TIME_CHANNELS];
static enum CH_CONFIG        channel_configuration = CHC_MONO;     /* later f(encPara) */
static enum PP_MOD_SELECT    pp_select = NONE;                     /* latef f(encPara) */
static GC_DATA_SWITCH gc_switch = GC_PRESENT; /*later f(encPara)*/
/*static enum GC_DATA_SWITCH gc_switch = GC_NON_PRESENT ; */
static QC_MOD_SELECT    qc_select = NTT_VQ;                   /* later f(encPara) */
static enum TF_MOD_SELECT    tf_select = MDCT_UER;                 /* later f(encPara) */
static enum TRANSPORT_STREAM transport_stream =  LENINFO_TSTREAM;  /* later f(encPara) */
static enum CORE_CODEC coreCodecIdx;                /* alm */

/* Prediction */
static PRED_TYPE pred_type = PRED_NONE;
static NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS];
static double *LTP_overlap_buffer[MAX_TIME_CHANNELS];
/*
static int sfb_offset[MAX_TIME_CHANNELS][(MAX_SCFAC_BANDS > ntt_N_CRB_MAX * numChannel_MAX ? MAX_SCFAC_BANDS : ntt_N_CRB_MAX * numChannel_MAX) + 1];
*/
static int sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];
static double *nok_tmp_DTimeSigBuf[MAX_TIME_CHANNELS]; /* temporary fix to the buffer size problem. */

/* Perceptual weight mode for the NTT_VQ coder */
static enum NTT_PW_SELECT    ntt_pw_select = NTT_PW_INTERNAL;

/* Variable bit switch for the NTT_VQ coder */
static enum NTT_VARBIT ntt_varbit = NTT_VARBIT_OFF;

/* TNS for TWIN VQ layer */
static TNS_INFO *tvqTnsInfo[MAX_TIME_CHANNELS];
int tvq_tns_enable = 0;

/* AAC shorter windows 960-480-120 */
static int useShortWindows=0;


#ifdef VERSION2
enum  WINDOW_SHAPE_ADAPT{ 
  STATIC, 
  TOGGLE, 
  DYNAMIC };

static enum WINDOW_SHAPE_ADAPT windowShapeAdapt;
static const WINDOW_SHAPE windowShapeSeq[] = { WS_FHG, WS_ZPW, WS_ZPW, WS_FHG };
static int windowShapeSeqCounter = 0;
#endif

static int aacAllowScalefacs=1;

/* NTT_VQ_SYS T.Ishikawa 980525 */
static HANDLE_BSBITBUFFER mat_dynpartBuf;
extern int samplFreqIndex[];
#include "flex_mux.h"

/* TNS options */
int tns_enable = 0;

int   tvq_flag_960;
int   tvq_flag_wshape;
int   tvq_flag_ppc;
int   tvq_flag_bandlimit;
int   tvq_flag_postproc;
int   tvq_flag_msmask1;
int   tvq_flag_msmask2;

/********************************************************/
/* functions EncTfInfo() and EncTfFree()   HP 21-aug-96 */
/********************************************************/

#define PROGVER "general audio encoder core FDAM1 16-Feb-2001"


/* EncTfInfo() */
/* Get info about t/f-based encoder core. */
char *EncTfInfo (
                 FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "encoder parameter string format:\n"
	    "possible options:\n"
	    "-qc_mdct16\n"
	    "-qc_mdct16_960\n"
	    "-qc_uer\n"
	    "-qc_src\n"
	    "-qc_ntt [-no_syncword] [-ntt_pw_{int,ext}] [-ntt_varbit_{on,off}] \n"
	    "-pp_aac\n"
	    "-qc_aac\n"
            "-qc_aac_960\n"
	    "-aac_sca\n"
	    "-mode <n> :  see below   for more info \n"
	    "-core_coder <n> :  n=5: MPEG-4 Celp 6 kbps  n=9: NTT VQ 6 kbps 10: NO_CORE\n"
            "-aac_raw   caution: works only with framework switch -vr !\n");
    fprintf(helpStream,
#ifdef VERSION2
            "-aac_ld : AAC Low Delay mode\n"
            "     -aac_ld_512 : block size 512 (default: 480)\n"
            "     -wsa <n> : window shape adaptation mode n=0: static, n=1: toggle\n"
#endif
	    "-aac_pns <start_band_for_perceptual_noise_subst>\n"
            "-aac_tns\n"
#ifdef VERSION2
        "-aac_bsac !\n"
        "-aac_sys_bsac\n"
#endif
	    "-aac_nosfacs: make all scalefacs be equal\n"
	    "-no_tstream\n"
	    "-len_tstream\n"
	    "-raw_tstream: raw AAC frames without syncword/leninfo (for MP4 muxing)\n"
	    "-pred_ltp\n"
	    "-wsm <win_switch_mode>\n"
	    "-wlp <blksize> <med> <short>\n");
    fprintf(helpStream,
	    "-nttScl <Br1> [<Br2> [<Br3> [<Br4> ...]]] (scalable, Br in kbit/s)\n"
			"-core_br <Br>\n"
            "-tvq_tns\n"
            "-tvq_960\n"
            "-tvq_wshape\n"
            "-tvq_ppc\n"
            "-tvq_bandlimit\n"
            "-tvq_postproc\n"
            "-tvq_msmask1\n"
#ifdef VERSION2
#ifndef EP_CONFIG_MISSING
            "-tvq_ep1\n"
#endif
#endif	    
	    "\n");
    printOpModeInfo(helpStream);
  }

  return PROGVER;
}


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree ()
{
}



/*****************************************************************************************
 ***
 *** Function: EncTfInit
 ***
 *** Purpose:  Initialize the T/F-part and the macro blocks of the T/F part of the VM
 ***
 *** Description:
 *** 
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/

void EncTfInit (
                int         numChannel,	 /* in: num audio channels */
                float       sampling_rate_f,   /* in: sampling frequancy [Hz] */         
                float       bit_rate_f,        /* in: bit rate [bit/sec] */
                char        *encPara,		 /* in: encoder parameter string */
                int         quantDebugLevel,   /* in : quantDebugLevel */
                int         *frameNumSample,	 /* out: num samples per frame */
                int         *delayNumSample,	 /* out: encoder delay (num samples) */
                HANDLE_BSBITBUFFER bitHeader    ,     /* out: header for bit stream */
                ENC_FRAME_DATA* frameData,
		TF_DATA*    tfData,
		ntt_DATA*   nttData,
                int mainDebugLevel) 
{
  HANDLE_BSBITSTREAM header_stream;

  ntt_INDEX    ntt_index_scl;
  ntt_INDEX    ntt_index;
  int          ntt_IBPS = 0;
  int          ntt_IBPS_SCL[8];
  int          ntt_NSclLay = 0;
  int          aac_BPS_SCL[8];
  int          ntt_NSclLay_aac;

  int reserved_bits = 0;	/* was earlier given as function parameter */
  int sampling_rate = (int)(sampling_rate_f+.5);
  int bit_rate = (int)(bit_rate_f+.5);
  char *p_ctmp;
  long superframe_blocks     = 1;        /* no of blocks in a superframe; later f(encPara) */
  int   op_mode = NO_CORE  ;

  int ntt_LTP_SW;

  debugLevel=  quantDebugLevel;
  tvq_debug_level = debugLevel;

  header_stream = BsOpenBufferWrite(bitHeader);

  if( strstr( encPara, "-qc_mdct16" ) ) {
    qc_select = MDCT_VALUES_16BIT;
  }
  if( strstr( encPara, "-qc_mdct16_960" ) ) {
    qc_select = MDCT_VALUES_16BIT;
    block_size_samples=960;
  }
  if( strstr( encPara, "-qc_uer" ) ) {
    qc_select = UER_QC;
  }
  if( strstr( encPara, "-qc_src" ) ) {
    qc_select = VM_QC_SOURCE;
  }
  if( strstr( encPara, "-qc_ntt" ) ) {
    qc_select = NTT_VQ;
  }

  /* T.Ishikawa 980526 */
  if( strstr( encPara,"-nttScl") )
    {
      if( strstr( encPara, "-tvq_sys" ) )
        {
          qc_select = NTT_VQ_SYS;
        }	  
      else
        {
          qc_select = NTT_VQ;
        }
    }
  /* T.Ishikawa 980625 */
  if( strstr( encPara,"-tvq_sys") /*&& strstr( encPara,"-core_br")*/)
    {
      qc_select = NTT_VQ_SYS;
    }
  
  if( strstr( encPara, "-pp_aac" ) ) {
    pp_select = AAC_PP;
  }
  if( strstr( encPara, "-qc_aac" ) ) {
    qc_select = AAC_QC;
    tf_select = MDCT_AAC;
  }
  
  if( strstr( encPara, "-qc_aac_960" ) ) {
    qc_select = AAC_QC;
    tf_select = MDCT_AAC;
    useShortWindows=1;
    block_size_samples=960;
  }

  /* case aac_sys: allows the use of TVQ as core */
  if( strstr( encPara, "-aac_sys" ) 
  || ( strstr(encPara, "-mp4ff") != NULL)
#ifdef VERSION2
      || strstr( encPara, "-aac_sys_bsac" )
#endif
    ) {
    qc_select = AAC_SYS;
#ifdef VERSION2
    if(strstr( encPara, "-aac_sys_bsac"))
      qc_select = AAC_SYS_BSAC;
#endif
    tf_select = MDCT_AAC;
    useShortWindows=1;
    win_switch_mode = STATIC_LONG;
    
    frameData->opMode = 2;
    if( (p_ctmp = strstr( encPara, "-mode" )) ) {
      sscanf( p_ctmp+5, "%d", &frameData->opMode );
    }
    frameData->layer[0].sampleRate=sampling_rate ;
    coreCodecIdx=NO_CORE;

    switch (frameData->opMode) {
    case 0 : 
      coreCodecIdx=NTT_TVQ;
      if ((p_ctmp=strstr(encPara, "-core_coder"))) {
        int utmp;
        sscanf(p_ctmp+11,"%d", &utmp);
        coreCodecIdx = (enum CORE_CODEC) utmp;
      } 
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &frameData->layer[0].bitRate  );
      }

      switch(coreCodecIdx) {
      case NTT_TVQ:  
        block_size_samples=1024;
        break;
      default:
         CommonExit(-1,"core coder not yet supported for this mode"); 
      }
      break;
    case 1 :
      coreCodecIdx=NTT_TVQ;
      if ((p_ctmp=strstr(encPara, "-core_coder"))) {
        int utmp;
        sscanf(p_ctmp+11,"%d", &utmp);
        coreCodecIdx = (enum CORE_CODEC) utmp;
      } 
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &frameData->layer[0].bitRate  );
      }

      switch(coreCodecIdx) {
      case NTT_TVQ:  
        block_size_samples=1024;
#if 0
        frameData->layer[0].sampleRate=sampling_rate ;
#endif
        break;
      default:
         CommonExit(-1,"core coder not yet supported for this mode"); 
      }
      break;
    
    case 2 : 
      block_size_samples = 1024; /* AAC sys mode without core codec */
      frameData->layer[0].bitRate=bit_rate ; /* bitrate for AAC */
      frameData->layer[0].sampleRate=sampling_rate ;
      break;
    case 3 : CommonExit(-1,"this mode is not supported for aac_sys"); 
      break;
    case 4 :
    case 5 :
    case 6 :
      block_size_samples = 1024; /* AAC sys mode with core codec */
      coreCodecIdx=NTT_TVQ;
      if ( ( p_ctmp=strstr(encPara, "-core_coder" ) ) ) {
        int utmp;
        sscanf(p_ctmp+11,"%d", &utmp);
        coreCodecIdx = (enum CORE_CODEC) utmp;
      }       
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &frameData->layer[0].bitRate  );
      }

      switch(coreCodecIdx) {
      case NTT_TVQ:  
        block_size_samples=1024;
#if 0 /*NN*/
        frameData->layer[0].sampleRate=sampling_rate ;
        frameData->layer[1].bitRate=bit_rate - frameData->layer[0].bitRate ; /* bitrate for AAC */
        frameData->layer[1].sampleRate=sampling_rate ;
        frameData->layer[2].sampleRate=sampling_rate ;
        frameData->layer[3].sampleRate=sampling_rate ;
        frameData->layer[4].sampleRate=sampling_rate ;
#endif
        break;
      default:
         CommonExit(-1,"core coder not yet supported for this mode"); 
      }
      break;
    
    case 7 :
      block_size_samples = 1024; /* AAC sys mode without core codec */
      coreCodecIdx=NTT_TVQ;
      if ( ( p_ctmp=strstr(encPara, "-core_coder" ) ) ) {
        int utmp;
        sscanf(p_ctmp+11,"%d", &utmp);
        coreCodecIdx = (enum CORE_CODEC) utmp;
      }             
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &frameData->layer[0].bitRate  );
      }

      switch(coreCodecIdx) {
      case NTT_TVQ:  
        block_size_samples=1024;
        frameData->layer[0].sampleRate=sampling_rate ;
        frameData->layer[1].bitRate= (bit_rate - frameData->layer[0].bitRate)/2 ; /* bitrate for AAC */
        frameData->layer[1].sampleRate=sampling_rate ;
        frameData->layer[2].bitRate=bit_rate 
          -  frameData->layer[1].bitRate - frameData->layer[0].bitRate; /* bitrate for AAC */
        frameData->layer[2].sampleRate=sampling_rate ;
        break;
      default:
         CommonExit(-1,"core coder not yet supported for this mode"); 
      }
      break;
    case 11 : 
      block_size_samples = 1024; /* AAC sys mode without core codec */
      frameData->layer[0].bitRate=bit_rate/2 ; /* bitrate for AAC */
      frameData->layer[0].sampleRate=sampling_rate ;
      frameData->layer[1].bitRate=bit_rate - frameData->layer[0].bitRate ; /* bitrate for AAC */
      frameData->layer[1].sampleRate=sampling_rate ;
      break;

    default: CommonExit(-1,"this mode is not yet supported for aac_sys"); 
      break;
    }
    /* check if core coder is TVQ */
    if(coreCodecIdx==NTT_TVQ){
       double tvq_br;
       int iscl;
       initTvqSys( numChannel,	   /* in: num audio channels */
                sampling_rate_f,   /* in: sampling frequancy [Hz] */         
                bit_rate_f,        /* in: bit rate [bit/sec] */
                encPara,	   /* in: encoder parameter string */
                quantDebugLevel,   /* in : quantDebugLevel */
                frameNumSample,	   /* out: num samples per frame */
                delayNumSample,	   /* out: encoder delay (num samples) */
                frameData,
		&ntt_index,
		&ntt_index_scl,
		nttData); 
        ntt_NSclLay=ntt_index.nttDataScl->ntt_NSclLay; 
	
        tvq_br = frameData->layer[0].bitRate;
	for(iscl=0;iscl<ntt_NSclLay;iscl++) {
	    frameData->layer[iscl+1].sampleRate
		 =frameData->layer[0].sampleRate;
	    tvq_br += frameData->layer[iscl+1].bitRate;
        }
     if(frameData->opMode > 1){	
       frameData->layer[ntt_NSclLay+1].bitRate = (int) (floor(bit_rate_f) - tvq_br);
          frameData->layer[ntt_NSclLay+1].sampleRate=sampling_rate ;
	ntt_NSclLay_aac = 0;
	if(( p_ctmp=strstr( encPara, "-aacScl " ) )){
	if((ntt_NSclLay_aac=sscanf( p_ctmp+8, "%d %d %d %d %d %d %d %d",
		   &aac_BPS_SCL[0],
	           &aac_BPS_SCL[1],
		   &aac_BPS_SCL[2],
		   &aac_BPS_SCL[3],
		   &aac_BPS_SCL[4],
		   &aac_BPS_SCL[5],
		   &aac_BPS_SCL[6],
		   &aac_BPS_SCL[7])) == 0) {
	CommonExit(1, "Encode: parameter of -aacScl switch not found");
	}
	for(iscl=0;iscl<ntt_NSclLay_aac;iscl++) {
	  frameData->layer[iscl+ntt_NSclLay+1].bitRate= aac_BPS_SCL[iscl];
          frameData->layer[iscl+ntt_NSclLay+1].sampleRate=sampling_rate ;
        }
      }
    }
    }

    transport_stream = NO_TSTREAM;    
    if(qc_select != AAC_SYS_BSAC)
    win_switch_mode = FFT_PE_WINDOW_SWITCHING ;
  }
  /* case aac_sca: supports only speech codecs as core */
  if( strstr( encPara, "-aac_sca" ) ) {
    qc_select = AAC_SCALABLE;
    tf_select = MDCT_AAC;
    useShortWindows=1;
    block_size_samples=960;
    transport_stream = NO_TSTREAM;    
    win_switch_mode = STATIC_LONG;

    if ( (p_ctmp = strstr( encPara, "-mode" ) ) ) {
      sscanf( p_ctmp+6, "%d", &op_mode );
    }

    if ( ( p_ctmp=strstr(encPara, "-core " ) ) ) {
        int utmp;
        sscanf(p_ctmp+6,"%d", &utmp);
        coreCodecIdx = (enum CORE_CODEC) utmp;
      if (coreCodecIdx==NTT_TVQ)
        CommonExit(1,"NTT TVQ core not supported in this mode");
    }
  }

  if( strstr( encPara, "-aac_raw" ) ) {
    qc_select = AAC_QC;
    tf_select = MDCT_AAC;
    win_switch_mode = STATIC_LONG;
    transport_stream = AAC_RAWDATA_TSTREAM;
  }

#ifdef VERSION2
  /* AAC Low Delay mode */
  if( strstr( encPara, "-aac_ld" ) ) {
    qc_select = AAC_LD;
    tf_select = MDCT_AAC;
    useShortWindows = 0;
    win_switch_mode = STATIC_LONG;
    block_size_samples=480;
    if( strstr( encPara, "-aac_ld_512" ) ) 
      block_size_samples=512;
    transport_stream = AAC_RAWDATA_TSTREAM;

    /* Window Shape Mode for Low Delay */
    windowShapeAdapt = STATIC;
    if( (p_ctmp = strstr( encPara, "-wsa" ) ) ) {
      if( sscanf( p_ctmp+5, "%u",(unsigned int*)(&windowShapeAdapt) ) == 0 )
        CommonExit( 1, "Encode: parameter of -wsa not found" );
    }
  }
#endif

  if( (p_ctmp=strstr( encPara, "-aac_pns " )) ) {
    if( sscanf( p_ctmp+9, "%d",&pns_sfb_start ) == 0 ) {
      CommonExit( 1, "Encode: parameter of -aac_pns switch not found" );
    }
    tf_select = MDCT_AAC;
    win_switch_mode = STATIC_LONG;
    transport_stream = AAC_RAWDATA_TSTREAM;
  }

  if( strstr( encPara, "-aac_tns" ) ) {
    int n_ch, i_ch;
    n_ch = numChannel;
    for (i_ch=0; i_ch<n_ch; i_ch++) {
      tnsInfo[i_ch] = (TNS_INFO*)calloc(1, sizeof(TNS_INFO));
      TnsInit( (long)sampling_rate_f, MAIN, tnsInfo[i_ch], qc_select, block_size_samples);
    }
    tns_enable = 1;
  }
#ifdef VERSION2
  if( strstr( encPara, "-aac_bsac" ) ) { /* YB : 970825 */
    qc_select = AAC_BSAC;
    tf_select = MDCT_AAC;
    win_switch_mode = STATIC_LONG;
    transport_stream = NO_SYNCWORD;
  }
#endif
  if( strstr( encPara, "-aac_nosfacs" ) ) {
    aacAllowScalefacs=0;
  }
  if( strstr( encPara, "-no_tstream" ) ) {
    transport_stream = NO_TSTREAM;
  }
  if( strstr( encPara, "-len_tstream" ) ) {
    transport_stream = LENINFO_TSTREAM;
  }
  if ((qc_select == NTT_VQ) || (qc_select == NTT_VQ_SYS)){ /* T.Ishikawa 980525 */
    transport_stream = NO_SYNCWORD;
    win_switch_mode = FFT_PE_WINDOW_SWITCHING;
  }
  if ( strstr( encPara, "-no_syncword") ){ /* supported by NTT_VQ only */
    transport_stream = NO_SYNCWORD;
  }
  if ( strstr( encPara, "-raw_tstream") ){ /* raw AAC frames, no syncword/leninfo */
    transport_stream = AAC_RAWDATA_TSTREAM;
  }
  if( (p_ctmp=strstr( encPara, "-wsm " )) ) {  /* window length switching parameters */
    if( sscanf( p_ctmp+5, "%d",(int*)&win_switch_mode ) == 0 ) {
      CommonExit( 1, "Encode: parameter of -wsm switch not found" );
    }
  }
  if( (p_ctmp=strstr( encPara, "-wlp " )) ) {  /* window length parameters */
    if( sscanf( p_ctmp+5, "%i %i %i", &block_size_samples, &med_win_in_long, &short_win_in_long) == 0 ) {
      CommonExit( 1, "Encode: parameters of -wlp switch not found" );
    }
  }
  if( (p_ctmp=strstr( encPara, "-cde " )) ) {  /* window length switching parameters */
    if( sscanf( p_ctmp+5, "%d",(int*)&coreDelay ) == 0 ) {
      CommonExit( 1, "Encode: parameter of -cde switch not found (coreDelay)" );
    }
    if (coreDelay>block_size_samples)
      CommonExit( 1, "\nillegal core delay: greater than blocksize" );      
  }
  if( strstr( encPara, "-pred_ltp" ) )
    pred_type = NOK_LTP;
  /* set the return values (including NTT_VQ global initialization) */
  /* number of channels */
  max_ch = numChannel;
  {
    if ((qc_select != NTT_VQ) && (qc_select != NTT_VQ_SYS) ){ /* set the return values for non NTT_VQ coders */ /* T.Ishikawa 980525 */
      *frameNumSample = block_size_samples;
      *delayNumSample = 2*block_size_samples; 
    }
    else if( qc_select==NTT_VQ_SYS ){ 
    /* global init. of the NTT_VQ_SYS coder T.Ishikawa 980526 */
#if 1
    initTvqSys( numChannel,	   /* in: num audio channels */
                sampling_rate_f,   /* in: sampling frequancy [Hz] */         
                bit_rate_f,        /* in: bit rate [bit/sec] */
                encPara,	   /* in: encoder parameter string */
                quantDebugLevel,   /* in : quantDebugLevel */
                frameNumSample,	   /* out: num samples per frame */
                delayNumSample,	   /* out: encoder delay (num samples) */
                frameData,
		&ntt_index,
		&ntt_index_scl,
		nttData); 
        ntt_NSclLay=ntt_index.nttDataScl->ntt_NSclLay; 
#endif
    }
    else if( qc_select==NTT_VQ ){
      /* global init. of the NTT_VQ coder T.Ishikawa 980526 */
      int    iscl;
      float  t_bit_rate_scl;
	  int ntt_core_br;
	  int mat_total_tvq_bits,total_frame_bits;
	  if(debugLevel >= 1)
	  fprintf(stderr,"#### TwinVQ(Conventional) Global Init %x \n",qc_select);

	  mat_total_tvq_bits=0;
	  
      /* bitrate control */
      ntt_NSclLay = 0;

      block_size_samples = 1024;
	  tvq_flag_960=0;
      if ((p_ctmp=strstr( encPara, "-tvq_960" ))) { 
          block_size_samples = 960;
	  tvq_flag_960=1;
      } 

      /* TNS handling for TWIN VQ codec E. Allamache 981117 Added by T.Ishikawa 981211 */
      if ((p_ctmp=strstr( encPara, "-tvq_tns" ))) {
        int i_ch;
        for (i_ch=0; i_ch<max_ch; i_ch++) {
          tnsInfo[i_ch] = (TNS_INFO*)calloc(1, sizeof(TNS_INFO));
          TnsInit( (long)sampling_rate_f, LOW /*MAIN*/, tnsInfo[i_ch], qc_select, block_size_samples);
        }
        tvq_tns_enable = 1;
      }

      /* T.Ishikawa 981118 */
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &ntt_core_br  );
		ntt_IBPS=(int)floor(ntt_core_br/(long)max_ch);
      }	  
      else
        {
          ntt_core_br = 8000;
          fprintf(stderr,"Warning!! : TwinVQ(System) BaseLayer_Br is set %d bps(Default)\n",ntt_core_br);
        }

      if(( p_ctmp=strstr( encPara, "-nttScl " ) )) {
	/* HP 970509   added an 8th "%d" */
	if((ntt_NSclLay=sscanf( p_ctmp+8, "%d %d %d %d %d %d %d %d",
                                &ntt_IBPS_SCL[0],
                                &ntt_IBPS_SCL[1],
                                &ntt_IBPS_SCL[2],
                                &ntt_IBPS_SCL[3],
                                &ntt_IBPS_SCL[4],
                                &ntt_IBPS_SCL[5],
                                &ntt_IBPS_SCL[6],
                                &ntt_IBPS_SCL[7])) == 0) {
	  CommonExit(1, "Encode: parameter of -nttScl switch not found");
	}
      }
      t_bit_rate_scl = 0.;

      for (iscl=0; iscl<ntt_NSclLay; iscl++){
	t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
      }
      /* initialization of base coder */


      tvq_flag_wshape =0;
      if ((p_ctmp=strstr( encPara, "-tvq_wshape" ))) { 
	tvq_flag_wshape =1;
      }
      tvq_flag_ppc =0;
      if ((p_ctmp=strstr( encPara, "-tvq_ppc" ))) { 
	tvq_flag_ppc =1;
      }
      tvq_flag_bandlimit =0;
      if ((p_ctmp=strstr( encPara, "-tvq_bandlimit" ))) { 
	tvq_flag_bandlimit =1;
      }
      tvq_flag_postproc =0;
      if ((p_ctmp=strstr( encPara, "-tvq_postproc" ))) { 
	tvq_flag_postproc =1;
      }
      tvq_flag_msmask1 =0;
      if ((p_ctmp=strstr( encPara, "-tvq_msmask1" ))) { 
	tvq_flag_msmask1 =1;
      }
      *frameNumSample = block_size_samples;
      *delayNumSample = block_size_samples; 

      if(tvq_debug_level >= 1)
        fprintf(stderr," \n encoding with TwinVQ mode at %8d bit/s/ch \n",
	  ntt_IBPS);
      if(ntt_IBPS > 16000 )
        fprintf(stderr,"AAC or BSAC is recommended to use at this rate\n");
     if(tvq_debug_level >5 )
       fprintf(stderr, "JJJJJJJJJJJJ ntt_IBPS ntt_NSclLay %5d %5d \n", 
       ntt_IBPS, ntt_NSclLay);

      ntt_TfInit(sampling_rate_f, (float)ntt_core_br+t_bit_rate_scl 
                 , t_bit_rate_scl, (long)max_ch, block_size_samples
		 , &ntt_index, &ntt_index_scl); /* global init. */
      t_bit_rate_scl = 0.;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
	ntt_scale_init(iscl, ntt_NSclLay,  
	ntt_IBPS_SCL[iscl], sampling_rate_f, &ntt_index, &ntt_index_scl);
	nttData->nttDataScl = ntt_index.nttDataScl;
	t_bit_rate_scl += 
	    ntt_index_scl.nttDataScl->ntt_NBITS_FR_SCL[iscl] 
	       *sampling_rate_f/block_size_samples;
	if( strstr( encPara, "-nttSclMsg" ) ) {
	  ntt_scale_message(iscl,  ntt_IBPS, ntt_IBPS_SCL);
	}
      }
        ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
        ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay;
	nttData->nttDataBase = ntt_index.nttDataBase;
	nttData->nttDataScl = ntt_index_scl.nttDataScl;

	  total_frame_bits = (int)floor(bit_rate_f*block_size_samples/sampling_rate_f+0.5);

	  mat_total_tvq_bits += ntt_index.nttDataBase->ntt_NBITS_FR;
	  for(iscl=0;iscl<ntt_NSclLay;iscl++)
	  mat_total_tvq_bits += ntt_index_scl.nttDataScl->ntt_NBITS_FR_SCL[iscl];

	  if(mat_total_tvq_bits>total_frame_bits)
		  {
			  CommonExit(1,"TwinVQ : Bitrate mismatch. more than %7.1f bps/ch",(mat_total_tvq_bits-0.5)*sampling_rate_f/block_size_samples);
		  }
	  
				  

	  fprintf(stderr,"<<OLD>> BAND %f BPS %d\n",
		ntt_index.nttDataBase->bandUpper,ntt_IBPS);
	  for(iscl=0;iscl<ntt_NSclLay;iscl++)
	  fprintf(stderr,"<<OLD>> BPS_SCL[%d] %d\n",iscl,ntt_IBPS_SCL[iscl]);

	  fprintf(stderr,"<<OLD>> CH %d\n",numChannel);

      if ( strstr( encPara, "-ntt_pw_ext" ) ){/* perceptual weight selection */
	ntt_pw_select = NTT_PW_EXTERNAL;
	if (max_ch > 1)
	  CommonExit(1, "EncTfInit: Error in NTT_VQ: No stereo support with NTT_PW_EXTERNAL.");
      }
      if ( strstr( encPara, "-ntt_pw_int" ) ){
	ntt_pw_select = NTT_PW_INTERNAL;
      }
      if (strstr( encPara, "-ntt_varbit_on") ){ /* Variable bit switch */
	ntt_varbit = NTT_VARBIT_ON;
      }
      if (strstr (encPara, "-ntt_varbit_off")){
	ntt_varbit = NTT_VARBIT_OFF;
      }
    }
  }

  /* ----- if no AAC_RAWDATA, write some header information -----*/
  if( (transport_stream != AAC_RAWDATA_TSTREAM) && 
      (transport_stream != NO_TSTREAM) && (qc_select !=NTT_VQ_SYS)) { /* T.Ishikawa 980525 */
	  fprintf(stderr,"Warning : Write some header information\n");
    BsPutBit( header_stream, 0x1234, 16 );
    BsPutBit( header_stream, bit_rate, 24 );
    BsPutBit( header_stream, sampling_rate, 24 );
    BsPutBit( header_stream, block_size_samples, 16 );
    BsPutBit( header_stream, med_win_in_long, 16 );
    BsPutBit( header_stream, short_win_in_long, 16 );
    BsPutBit( header_stream, *delayNumSample, 16 );
    BsPutBit( header_stream, channel_configuration, 8 );
    BsPutBit( header_stream, tf_select, 32 );           /* encode T/F mapping select */
    BsPutBit( header_stream, qc_select, 32 );           /* encode Q&C select */
  }
  
  /* -- if AAC_RAWDATA, write  "aacraw" --- */
  /*
    if( transport_stream == AAC_RAWDATA_TSTREAM ) {
    BsPutBit( header_stream, 'a', 8 );
    BsPutBit( header_stream, 'a', 8 );
    BsPutBit( header_stream, 'c', 8 );
    BsPutBit( header_stream, 'r', 8 );
    BsPutBit( header_stream, 'a', 8 );
    BsPutBit( header_stream, 'w', 8 );
    }
  */

  if (qc_select == NTT_VQ){
    int iscl;
    /* NTT_VQ supports both LENINFO_STREAM and NO_SYNCWORD. */
    BsPutBit(header_stream, transport_stream, 2);
    /* put ntt_varbit switch */
    /* If the ntt_varbit is off, number of bits
       per frame should be in the header. */
    BsPutBit(header_stream, ntt_varbit, 1);
    if (ntt_varbit == NTT_VARBIT_OFF){
      int frameNumBit;
      frameNumBit = (int)floor((double)bit_rate_f*(double)block_size_samples
			  /(double)sampling_rate+0.5);
      BsPutBit(header_stream, frameNumBit, 32);
    }
    /* long term predictor */
    if (pred_type == NOK_LTP){
      ntt_LTP_SW = 1;
      BsPutBit(header_stream, 1, 1);
    }
    else{
      ntt_LTP_SW = 0;
      BsPutBit(header_stream, 0, 1);
    }
    /* header for base coder */	 /* T.Ishikawa 981118 */
	
	BsPutBit(header_stream, (unsigned long)(ntt_IBPS*numChannel),16);
    BsPutBit(header_stream, (unsigned long)ntt_NSclLay, ntt_NSclLay_BITS);
    for (iscl=0; iscl<ntt_NSclLay; iscl++){
      BsPutBit(header_stream, (unsigned long)(ntt_IBPS_SCL[iscl]),
	       ntt_IBPS_BITS_SCL);
  }
  }
  else if(qc_select == NTT_VQ_SYS)
	  {
    int iscl,sx;
    if (pred_type == NOK_LTP){ntt_LTP_SW = 1;}else{ntt_LTP_SW = 0;}

    initObjDescr(frameData->od,mainDebugLevel);
    presetObjDescr(frameData->od, ntt_NSclLay);

    frameData->od->streamCount.value=(ntt_NSclLay+1);

    for(iscl=0;iscl<ntt_NSclLay+1;iscl++){

    initESDescr( &(frameData->od->ESDescriptor[iscl]) );/* SET audioDecoderType = 0x0 */
    presetESDescr( frameData->od->ESDescriptor[iscl],iscl );
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value = (frameData->layer[iscl].bitRate) ;
    for(sx=0;sx<16;sx++){ if ( frameData->layer[iscl].sampleRate == samplFreqIndex[sx]) break; }
    /* HP20001012 samplingFreqencyIndex==0xf not handled */
    if (sx >= 16) CommonExit(-1,"\nSampling rate not supported");
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = sx;
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.bufferSizeDB.value = 6122;
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = TWIN_VQ; /* all tvq scaleable */

#ifdef VERSION2
    if ((p_ctmp=strstr( encPara, "-tvq_ER" ))){
      frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_TWIN_VQ; /* all tvq scaleable */
      /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
      if((p_ctmp==strstr( encPara, "-tvq_ep1" ))){
	ntt_index.nttDataBase->epFlag=1;
	ntt_index_scl.nttDataScl->epFlag=1;
      }
#endif
    }
#endif

	/* T.Ishikawa 980813 */
    if ((p_ctmp=strstr( encPara, "-tvq_960" ))) { 
    tvq_flag_960=1;
   frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = 1; /* 1: 960 0:1024*/	    
    }
    else{
    tvq_flag_960=0;
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = 0; /* 1: 960 0:1024*/	    
    }
    
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = numChannel;/* T.Ishikawa 980703 */
    /* -- NEXT 2 formula is ad-hoc !! */
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value = 0 ; /* tsushima ad-hoc */	    
    frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.extension.value = 0; /* tsushima ad-hoc */	    

#ifdef AL_CONFIG	
	/* Adhoc 981013 T.Ishikawa*/
	frameData->od->ESDescriptor[iscl]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
	frameData->od->ESDescriptor[iscl]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
	frameData->od->ESDescriptor[iscl]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
	frameData->od->ESDescriptor[iscl]->ALConfigDescriptor.usePaddingFlag.value=0;
	frameData->od->ESDescriptor[iscl]->ALConfigDescriptor.seqNumLength.value=0;
#endif
    }

   frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = 0; /* 1:960 0:1024 */
    if ((p_ctmp=strstr( encPara, "-tvq_960" ))) {
    tvq_flag_960=1;
   frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = 1; /* 1: 960 0:1024*/	    
    }
    else{
    tvq_flag_960=0;
    frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value = 0; /* 1: 960 0:1024*/	    
    }
    advanceODescr( header_stream,frameData->od,1 ); /* write down */

    for (iscl=0;iscl<ntt_NSclLay+1;iscl++){ /* revised by Tomokazu Ishikawa */
    advanceESDescr( header_stream, frameData->od->ESDescriptor[iscl],1 );

     }
  } /* qc_select == NTT_VQ_SYS T.Ishikawa 980525 */
		  

  /* some global initializations */

  {
    int n_ch, i_ch;
    n_ch = max_ch;
    for (i_ch=0; i_ch<n_ch; i_ch++)
      DTimeSigBuf[i_ch] = (double*)calloc(block_size_samples, sizeof(double));
{
	int jj;
    for (i_ch=0; i_ch<n_ch; i_ch++)
	for (jj=0; jj<block_size_samples; jj++)
	DTimeSigBuf[i_ch][jj] = 0.0;
}
    for (i_ch=0; i_ch<n_ch; i_ch++)
      DTimeSigLookAheadBuf[i_ch] =
        (double*)calloc(block_size_samples, sizeof(double));

    for (i_ch=0; i_ch<n_ch; i_ch++) {
      coreTimeSigBuf[i_ch] = (float*)calloc(block_size_samples, sizeof(float));
      coreTimeSigLookAheadBuf[i_ch] = (float*)calloc(block_size_samples, sizeof(float));
    }
    for (i_ch=0; i_ch<n_ch; i_ch++)
    {
      spectral_line_vector[i_ch] =
        (double*)calloc(block_size_samples*2, sizeof(double));

      {
	int scal_codec = 0;
	
	scal_codec = (qc_select == AAC_SYS || qc_select == NTT_VQ || qc_select == NTT_VQ_SYS);
	if(pred_type == NOK_LTP && scal_codec)
	  baselayer_spectral_line_vector[i_ch] =
	    (double*)calloc(block_size_samples*2, sizeof(double));
      }
    }
    for (i_ch=0; i_ch<n_ch; i_ch++)
      reconstructed_spectrum[i_ch] =
        (double*)calloc(block_size_samples, sizeof(double));
    for (i_ch=0; i_ch<n_ch; i_ch++)
      ntt_ms_buf[i_ch] =
        (double*)calloc(block_size_samples, sizeof(double));
    for (i_ch=0; i_ch<n_ch; i_ch++)
      nok_tmp_DTimeSigBuf[i_ch] = (double*)calloc(2*block_size_samples, sizeof(double));

     { int ii;
      for (i_ch=0; i_ch<n_ch; i_ch++)
       for(ii=0; ii<2*block_size_samples; ii++)
        nok_tmp_DTimeSigBuf[i_ch][ii] = 0.0;
     }

  }

  /* calc. no of audio channels */
#if 0 /* commented out by NI, because no. of ch. is handed by the framework */
  {
    /* table to get the no. of audio channels depending on the value of channel_configuration */
    static int  chc_2_chan_nr[CHC_MODES] = { 1, 2, 2, 5 };

    max_ch  = chc_2_chan_nr[channel_configuration];
  }
#endif

  /* calculate max. audio bandwidth */
  {
    /* table: audio_bandwidth( bit_rate ) */
    static long bandwidth[8][2] = { {64000,20000},{56000,16000},{48000,14000},
                                    {40000,12000},{32000,9000},{24000,6000},
                                    {16000,3500},{-1,0} 
    };
    int i;

    i = 0;
    while( bandwidth[i][0] > bit_rate ) {
      if( bandwidth[i][0] < 0 ) {
        i--;
        break;
      }
      i++;
    }
    bw_lines_long  = bandwidth[i][1] * block_size_samples *2 / sampling_rate;
    bw_lines_med   = bandwidth[i][1] * block_size_samples *2 / sampling_rate / med_win_in_long;
    bw_lines_short = bandwidth[i][1] * block_size_samples *2 / sampling_rate / short_win_in_long;


    /* Use a minimal band-width for a bit rate <= 6 kbps. */
    start_line = 0;
    if ( bit_rate <= 6000 )
      {
        bw_lines_long = 140;
        start_line = 6;
      }
  }
  

  /* son_AACpp */
  if (pp_select == AAC_PP)
    {
      int band;
      /* added 971010 YT */
      int i_ch, n_ch;
      n_ch = max_ch;
      for (i_ch=0; i_ch<n_ch; i_ch++){

        max_band[i_ch] = (NBANDS * (start_line + bw_lines_long - 1) /
                          block_size_samples); /* max_bands[] =0,1,2,3 */
        spectral_line_vector_for_gc[i_ch]      = (double *)calloc(block_size_samples, sizeof(double));
        DTimeSigPQFDelayCompensationBuf[i_ch]  = (double *)calloc(PQFDELAY, sizeof(double));
        DTimeSigLookAheadBufForGC[i_ch]        = (double *)calloc(block_size_samples, sizeof(double));
        DBandSigLookAheadBuf[i_ch]             = (double **)calloc(NBANDS, sizeof(double *));
        DBandSigBufForGCAnalysis[i_ch]         = (double **)calloc(NBANDS, sizeof(double *));
        DBandSigWithGCBuf[i_ch]                = (double **)calloc(NBANDS, sizeof(double *));
        gainInfo[i_ch]                         = (GAINC  **)calloc(NBANDS, sizeof(GAINC *));
    
        for (band = 0; band < NBANDS; band++) {
          DBandSigLookAheadBuf[i_ch][band]     = (double *)calloc(block_size_samples/NBANDS, sizeof(double));
          DBandSigBufForGCAnalysis[i_ch][band] = (double *)calloc(block_size_samples/NBANDS*3, sizeof(double));
          DBandSigWithGCBuf[i_ch][band]        = (double *)calloc(block_size_samples/NBANDS*2, sizeof(double));
          gainInfo[i_ch][band]                 = (GAINC  *)calloc(short_win_in_long,sizeof(GAINC));
        }
      } /* for(i_ch...*/
    }/* if (pp_select...*/

  /* initialize psychoacoustic module */
  EncTf_psycho_acoustic_init();

  /* initialize t/f mapping */
  {
    int i_ch, n_ch;
    n_ch = max_ch;
    for (i_ch=0; i_ch<n_ch; i_ch++)
      overlap_buffer[i_ch] =
        (double*)calloc(block_size_samples, sizeof(double));
  }

  /* initialize spectrum processing */
  

  /* initialize quantization and coding */
  {
    int average_block_bits;

    /* calculate the avaerage amount of bits available for one block */
    average_block_bits = (int)floor((double)bit_rate*(double)block_size_samples*(double)max_ch/(double)sampling_rate);
    /* the frame program may reserve a constant amount of bits e.g. for other 
       scalability layers or ancillary bits or ... 
    */
    average_block_bits -= ((reserved_bits/superframe_blocks)+1);

    /* The above two lines make shure that the used bit no is smaller than the
       available amount by (line 1) integer division and (line 2) the +1 
       Any integer or non integer remaining bits are counted and accumulated
       in the master frame program, and may be made available via the bit reservoir 
       technique to the coding process 
    */

    /* The maximum allowed and actual value of bits in the bit reservoir is calculated
       in the master frame program. The numbers are updated for every block and are
       parameters to the tf_encoder module */

    /* maybe it is better to move the above two lines to the master frame program */
      
    switch( qc_select ) {
    case AAC_QC:
#ifdef VERSION2
    case AAC_BSAC: /* YB : 971106 */
    case AAC_LD:
#endif
      tf_init_encode_spectrum_aac( qc_select, quantDebugLevel);	/* YB : 971106 */
      break;
    case  AAC_SCALABLE:
    case AAC_SYS:
      {
      int tmp;

      tf_init_encode_spectrum_aac( qc_select, quantDebugLevel); 	
      tmp = aacScaleEncInit( header_stream, (char*)encPara, (int)numChannel, 
                             (int)block_size_samples, (long)sampling_rate,
                             (  ENC_FRAME_DATA* ) frameData, (enum CORE_CODEC) coreCodecIdx, (int)op_mode,
			     (PRED_TYPE)pred_type,
			     &ntt_index, &ntt_index_scl,mainDebugLevel);
      nttData->nttDataBase = ntt_index.nttDataBase;
      nttData->nttDataScl = ntt_index_scl.nttDataScl;
      if( coreDelay < 0 ) {  /* if not set from the cmdline */
        coreDelay = tmp;
      }
     }
      break;
#ifdef VERSION2
    case AAC_SYS_BSAC:
      {
        int numES = 2;

        tf_init_encode_spectrum_aac( qc_select, quantDebugLevel); 
        sam_FlexMuxEncInit_bsac( header_stream, (int)numChannel, 
                             (long)sampling_rate, (ENC_FRAME_DATA*) frameData, 
                              numES, mainDebugLevel);
       }
       break;
#endif

    case UER_QC:
      /* tf_init_encode_spectrum( ); */
      CommonExit(1,"tf_init_encode_spectrum() is no more available\n"); 
      break;

    case NTT_VQ:
    case NTT_VQ_SYS: /* T.Ishikawa 980525 */
      coreDelay =0;
      break;
      
    default:
      /*   CommonExit(-1,"\ncase not handled in swicht" ); */
      CommonWarning("case not handled in swicht" );	/* HP 970509 */
      break;
    }
#ifdef VERSION2
    switch( qc_select ) {
    case AAC_BSAC:
    case AAC_SYS_BSAC:
      sam_init_encode_bsac(numChannel, sampling_rate, bit_rate, block_size_samples);
      break;
    default:
      break;
    }
#endif
  }
  
  /* initialize local T/F bitstream mux */
  {
    /* adjust/set absolute level/gain factor depending on the arithmetic and algorithmit
       implementation (T/F- module normalization factors, ... ) */

  }

  /* initialize Nokia's predictors */
  if (pred_type == NOK_LTP)
  {
    int ch;
    
    for(ch = 0; ch < numChannel; ch++)
    {
      nok_init_lt_pred (&nok_lt_status[ch]);
      LTP_overlap_buffer[ch] = (double*)calloc(block_size_samples, sizeof(double));
      nok_lt_status[ch].overlap_buffer = LTP_overlap_buffer[ch];
    }
  }

  BsClose(header_stream);

   ntt_index.numChannel = numChannel;
   ntt_index_scl.numChannel = numChannel;
   ntt_index.block_size_samples = block_size_samples;
   ntt_index_scl.block_size_samples = block_size_samples;
   tfData->block_size_samples = block_size_samples;
}


/*****************************************************************************************
 ***
 *** Function:    EncTfFrame
 ***
 *** Purpose:     processes a block of time signal input samples into a bitstream
 ***              based on T/F encoding 
 ***
 *** Description:
 *** 
 ***
 *** Parameters:
 ***
 ***
 *** Return Value:  returns the number of used bits
 ***
 *** **** MPEG-4 VM ****
 ***
 ****************************************************************************************/
ntt_PARAM  param_ntt;


void EncTfFrame (
		 float *p_time_signal[MAX_TIME_CHANNELS],   /* array of pointers to the modulo time 
							       signal input buffers for each channel */
		 HANDLE_BSBITBUFFER fixed_bitstream,             /* bits before bitres size ptr */
		 int frameAvailNumBit,           	    /* in: total num bits available for 
							       this frame (incl. bit reservoir) */
		 int frameNumBit,		            /* in: average num bits per frame */
		 int frameMaxNumBit,		            /* in: max num bits per frame */
		 long bitRateLong,
		 long fSampleLong,
                 ENC_FRAME_DATA* frameData,
		 TF_DATA*  tfData,
		 ntt_DATA*  nttData
		 )
{
  /* determine the function parameters used earlier:   HP 21-aug-96 */
  int i;
  int          average_bits = frameNumBit;
  int          available_bitreservoir_bits = frameAvailNumBit-frameNumBit;
  /* actual amount of bits currently in the bit reservoir */
  /* it is the job of this module to determine 
     the no of bits to use in addition to average_block_bits
     max. available: average_block_bits + available_bitreservoir_bits */
  int          max_bitreservoir_bits = frameMaxNumBit-frameNumBit;
  /* max. allowed amount of bits in the reservoir  (used to avoid padding bits) */
  long         num_bits_available;
  /* the generated bitstreams are copied into the buffers designated by the */
  /* two pointers fixed_bitstream and bitres_bitstream */
  HANDLE_BSBITBUFFER bitres_bitstream = NULL;            /* bits after bitres size ptr */

  double *p_ratio[MAX_TIME_CHANNELS], allowed_distortion[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS], p_energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    nr_of_sfb[MAX_TIME_CHANNELS], sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS];

  int   no_sub_win, sub_win_size, bw_lines;
  int i_ch, ch, k;

  
  /*-----------------------------------------------------------------------*/
  /* VQ: Variables for NTT_VQ coder */
  static int ntt_InitFlag=1;
  ntt_INDEX  ntt_index, ntt_index_scl;
  int          ntt_NSclLay;
  /*-----------------------------------------------------------------------*/
  
  /* structures holding the output of the psychoacoustic model */
  CH_PSYCH_OUTPUT chpo_long[MAX_TIME_CHANNELS];
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS];

  /* AAC window grouping information */
  int num_window_groups = 0;
  int window_group_length[8];
  
  HANDLE_BSBITSTREAM fixed_stream;
  HANDLE_BSBITSTREAM var_stream;
  HANDLE_BSBITSTREAM tmp_stream;
  HANDLE_BSBITBUFFER tmpBitBuf;
  /* son_AACpp */
  HANDLE_BSBITSTREAM gc_stream[MAX_TIME_CHANNELS];
  HANDLE_BSBITBUFFER gcBitBuf[MAX_TIME_CHANNELS];	/* bit buffer for gain_control_data() */


  { /* convert float input to double, which is the internal format */
    /* store input data in look ahead buffer which may be necessary for the window switching decision and AAC pre processing */
    int i, i_ch;
    for (i_ch=0; i_ch<max_ch; i_ch++){ 
      for( i=0; i<block_size_samples; i++ ) {
	/* temporary fix: a linear buffer for LTP containing the whole time frame */
	nok_tmp_DTimeSigBuf[i_ch][i] = DTimeSigBuf[i_ch][i];
	nok_tmp_DTimeSigBuf[i_ch][block_size_samples + i] = DTimeSigLookAheadBuf[i_ch][i];
	/* last frame input data are encoded now */
	DTimeSigBuf[i_ch][i]          = DTimeSigLookAheadBuf[i_ch][i];
	/* new data are stored here for use in window switching decision 
	   and AAC pre processing */
	if (pp_select == AAC_PP) {
	  DTimeSigLookAheadBuf[i_ch][i] = (i < PQFDELAY) ? DTimeSigPQFDelayCompensationBuf[i_ch][i] : (double)p_time_signal[i_ch][i-PQFDELAY] ;
	  DTimeSigLookAheadBufForGC[i_ch][i] = (double)p_time_signal[i_ch][i];
	}
	else { /* pp_select != AAC_PP */
	  DTimeSigLookAheadBuf[i_ch][i] = (double)p_time_signal[i_ch][i];
	}
      }
    }
    if (pp_select == AAC_PP) {
      int i, i_ch; /* added 971010 YT*/
      for (i_ch=0; i_ch<max_ch; i_ch++){ 
        for (i = 0; i < PQFDELAY; i++) {
          DTimeSigPQFDelayCompensationBuf[i_ch][i] = (double)p_time_signal[MONO_CHAN][block_size_samples-PQFDELAY+i];
        }
      } /* for i_ch...*/
    }/* if pp_select...*/
  }


  if (fixed_bitstream == NULL)
    return;	/* quick'n'dirty fix for encoder startup    HP 21-aug-96 */

  fixed_stream = BsOpenBufferWrite(fixed_bitstream);
  if (bitres_bitstream != NULL)
    var_stream = BsOpenBufferWrite(bitres_bitstream);
  else
    var_stream = NULL;
  tmpBitBuf = BsAllocBuffer(32768);
  tmp_stream = BsOpenBufferWrite(tmpBitBuf);
  /* son_AACpp */
  /* added 971010 YT*/
  {
    int i_ch;
    for (i_ch=0; i_ch<max_ch; i_ch++){ /* added 971010 YT*/
      gcBitBuf[i_ch] = BsAllocBuffer(4096);
      gc_stream[i_ch] = BsOpenBufferWrite(gcBitBuf[i_ch]);
    }
  }
  if ((qc_select != AAC_QC)
#ifdef VERSION2
      && (qc_select != AAC_LD)
      && (qc_select != AAC_BSAC)
      && (qc_select != AAC_SYS_BSAC)
#endif
      )
  {
	ntt_index_scl.nttDataScl =  nttData->nttDataScl;
	ntt_index.nttDataScl =  nttData->nttDataScl;
	ntt_index.nttDataBase =  nttData->nttDataBase;
        block_size_samples = tfData->block_size_samples;
        ntt_index.block_size_samples = block_size_samples;
        ntt_index_scl.block_size_samples = block_size_samples;
        ntt_index.numChannel = max_ch;
        ntt_index_scl.numChannel = max_ch;
        ntt_index.isampf = fSampleLong/1000;
        ntt_index_scl.isampf = fSampleLong/1000;
	ntt_NSclLay = ntt_index_scl.nttDataScl->ntt_NSclLay;
        windowShape[MONO_CHAN] = WS_FHG;
        if(tvq_flag_wshape ==1) windowShape[MONO_CHAN] = WS_DOLBY;
        windowShape[max_ch-1] = windowShape[MONO_CHAN];
        ntt_index.tvq_tns_enable = 0;
        if (tvq_tns_enable) ntt_index.tvq_tns_enable = 1;
        ntt_index.tvq_ppc_enable = 0;
        if (tvq_flag_ppc) ntt_index.tvq_ppc_enable = 1;
        ntt_index.bandlimit =0;
        if (tvq_flag_bandlimit ) ntt_index.bandlimit = 1;
        ntt_index.pf=0; 
        if (tvq_flag_postproc ) ntt_index.pf=1;
        ntt_index.ms_mask = 0;
        if ( tvq_flag_msmask1) ntt_index.ms_mask = 1;
   }

  /*-----------------------------------------------------------------------*/

  /*-----------------------------------------------------------------------*/
  /* VQ: Frame initialization for NTT_VQ coder*/

  param_ntt.ntt_param_set_flag = 0;
  /*-----------------------------------------------------------------------*/



#if 0
  { /* convert float input to double, which is the internal format */
    /* store input data in look ahead buffer which may be necessary for the window switching decision and AAC pre processing */
    for( i=0; i<block_size_samples; i++ ) {
      /* last frame input data are encoded now */
      DTimeSigBuf[MONO_CHAN][i]          = DTimeSigLookAheadBuf[MONO_CHAN][i];
      /* new data are stored here for use in window switching decision 
         and AAC pre processing */
      if (pp_select == AAC_PP) {
        DTimeSigLookAheadBuf[MONO_CHAN][i] = (i < PQFDELAY) ? DTimeSigPQFDelayCompensationBuf[MONO_CHAN][i] : (double)p_time_signal[MONO_CHAN][i-PQFDELAY] ;
        DTimeSigLookAheadBufForGC[MONO_CHAN][i] = (double)p_time_signal[MONO_CHAN][i];
      }
      else { /* pp_select != AAC_PP */
        DTimeSigLookAheadBuf[MONO_CHAN][i] = (double)p_time_signal[MONO_CHAN][i];
      }
    }
    if (pp_select == AAC_PP) {
      for (i = 0; i < PQFDELAY; i++) {
        DTimeSigPQFDelayCompensationBuf[MONO_CHAN][i] = (double)p_time_signal[MONO_CHAN][block_size_samples-PQFDELAY+i];
      }
    }
  }
#endif

  {
    for( i=0; i<coreDelay; i++ ) {
      /* last frame input data are encoded now */
      coreTimeSigBuf[MONO_CHAN][i]          = coreTimeSigLookAheadBuf[MONO_CHAN][i];
      /* new data are stored here for use in window switching decision 
         look ahead can be shorter, but for the moment we don't care */
      coreTimeSigLookAheadBuf[MONO_CHAN][i] = (float)p_time_signal[MONO_CHAN][i+(block_size_samples - coreDelay)];
    }
    for ( i=coreDelay;i<block_size_samples; i++ ) {
      coreTimeSigBuf[MONO_CHAN][i] = (float)p_time_signal[MONO_CHAN][(i - coreDelay)];
    }
  }


  /******************************************************************************************************************************
  *
  * pre processing 
  *
  ******************************************************************************************************************************/

  {
    switch( win_switch_mode ) {
    case STATIC_LONG:
      if (debugLevel>2)
        printf("WSM: ONLY_LONG_SEQUENCE\n");
      windowSequence[MONO_CHAN] = ONLY_LONG_SEQUENCE;
      break;
    case STATIC_SHORT :
      if (debugLevel>2)
	printf("WSM:EIGHT_SHORT_SEQUENCE\n");
      windowSequence[MONO_CHAN] = EIGHT_SHORT_SEQUENCE;
      break;
    case LS_STARTSTOP_SEQUENCE :
      if (debugLevel>2)
	printf("WSM: LS_STARTSTOP_SEQUENCE\n");
      {
        static WINDOW_SEQUENCE long_short_seq[3] = 
        { ONLY_LONG_SEQUENCE, LONG_START_SEQUENCE, LONG_STOP_SEQUENCE };
        static int ls_seq_no;

        windowSequence[MONO_CHAN] = long_short_seq[ls_seq_no++];
        if( ls_seq_no == 3 ) ls_seq_no = 0;
      }
      break;
    case LONG_SHORT_SEQUENCE :
      if (debugLevel>2)
	printf("WSM: LONG_SHORT_SEQUENCE\n");
      {
        static WINDOW_SEQUENCE long_short_seq[4] = 
        { ONLY_LONG_SEQUENCE, LONG_START_SEQUENCE, EIGHT_SHORT_SEQUENCE, LONG_STOP_SEQUENCE };
        static int ls_seq_no;

        windowSequence[MONO_CHAN] = long_short_seq[ls_seq_no++];
        if( ls_seq_no == 4 ) ls_seq_no = 0;
      }
      break;
    case FFT_PE_WINDOW_SWITCHING :
      /* VQ: Stand-alone automatic window switch provided by NTT */
      {
        static double *sig_tmptmp, *sig_tmp;
        int ismp, med_sw, i_ch, top;
        /* initialize */
        if (ntt_InitFlag){
          sig_tmptmp = (double *)calloc((size_t)(max_ch*block_size_samples*3),
                                        sizeof(double));
          sig_tmp = sig_tmptmp + block_size_samples * max_ch;
          for (i_ch=0; i_ch<max_ch; i_ch++){
            top = i_ch * block_size_samples;
            for (ismp=0; ismp<block_size_samples; ismp++){
              sig_tmp[ismp+top] = DTimeSigBuf[i_ch][ismp];
            }
          }
        }
        for (i_ch=0; i_ch<max_ch; i_ch++){
          top = i_ch * block_size_samples;
          for (ismp=0; ismp<block_size_samples; ismp++){
            sig_tmp[ismp+block_size_samples*max_ch+top] =
              DTimeSigLookAheadBuf[i_ch][ismp];
          }
        }
	
        /* automatic switching module */ /* T.Ishikawa 980525 */
        if ((qc_select == NTT_VQ)||(qc_select ==NTT_VQ_SYS)) med_sw = 1; /* if med_sw == 1            */
        else                    med_sw = 0; /* medium block is activated */
        
	
	ntt_tf_pre_process(sig_tmp, &ntt_index, &param_ntt, med_sw,
                           ntt_InitFlag);
        windowSequence[MONO_CHAN] = (WINDOW_SEQUENCE)ntt_index.w_type;
/*         windowSequence[MONO_CHAN] =     ntt_index.w_type =    EIGHT_SHORT_SEQUENCE; */
        /* prepare for the next frame */
        for (ismp=0; ismp<block_size_samples*2*max_ch; ismp++){
          sig_tmp[ismp-block_size_samples*max_ch] = sig_tmp[ismp];
        }
        param_ntt.ntt_param_set_flag = 1;

      }
      break;

    default:
      CommonExit( 1, "Encode: unsupported window switching mode %d", win_switch_mode );
    }

#ifdef VERSION2
    /* Perform window shape adaptation depending on the selected mode */
    if( qc_select == AAC_LD ) {
      switch( windowShapeAdapt ) {
      case STATIC:
        for(i_ch=0; i_ch<max_ch; i_ch++)
          windowShape[i_ch] = windowShapePrev[i_ch];
        break;
      case TOGGLE:
        if( windowShapeSeqCounter == 4 )
          windowShapeSeqCounter = 0;

        for(i_ch=0; i_ch<max_ch; i_ch++)
          windowShape[i_ch] = windowShapeSeq[windowShapeSeqCounter];

        windowShapeSeqCounter++;
        break;
      default:
        CommonExit( 1, "Encode: unsupported window shape adaptation mode (for Low Delay AAC)", windowShapeAdapt );
        break;
      }
    }
#endif
                    
    if (debugLevel>1)
      printf("Blocktype   %d \n",windowSequence[MONO_CHAN] );

    switch( windowSequence[MONO_CHAN] ) {
    case EIGHT_SHORT_SEQUENCE  :
      no_sub_win   = short_win_in_long;
      sub_win_size = block_size_samples/short_win_in_long;
      bw_lines     = bw_lines_short;
      break;
      
    default:
      no_sub_win   = 1;
      sub_win_size = block_size_samples;
      bw_lines     = bw_lines_long;
      break;
    }
   
    /* son_AACpp */
    if (pp_select == AAC_PP)
      {
        int i_ch; /* 971010 added YT*/
        int band;
	for(i_ch=0;i_ch<max_ch;i_ch++){ /* 971010 added YT*/

          /* polyphase quadrature filter */
          son_pqf_main(DTimeSigLookAheadBufForGC[i_ch], block_size_samples,
                       i_ch, DBandSigLookAheadBuf[i_ch]);
          /* set analysis buffer for gain control */
          for (band = 0; band < NBANDS; band++) {
            memcpy((char *)&DBandSigBufForGCAnalysis[i_ch][band][block_size_samples/NBANDS*2],
                   (char *)DBandSigLookAheadBuf[i_ch][band],
                   block_size_samples/NBANDS*sizeof(double)
                   );
          }

		if(gc_switch == GC_PRESENT){

          /* gain detector */
          son_gc_detect(DBandSigBufForGCAnalysis[i_ch], block_size_samples,
                        windowSequence[MONO_CHAN],
                        i_ch, gainInfo[i_ch]
                        );
		}
		else { /* if GC_NON_PRESENT */
		son_gc_detect_reset(DBandSigBufForGCAnalysis[i_ch], block_size_samples, windowSequence[i_ch], i_ch, gainInfo[i_ch]);
		}

          /* gain modifier */
          son_gc_modifier(DBandSigBufForGCAnalysis[i_ch],
                          gainInfo[i_ch],
                          block_size_samples,
                          windowSequence[MONO_CHAN], i_ch,
                          DBandSigWithGCBuf[i_ch]
                          );
          /* shift analysis buffer */
          for (band = 0; band < NBANDS; band++) {
            memcpy((char *)DBandSigBufForGCAnalysis[i_ch][band],
                   (char *)&DBandSigBufForGCAnalysis[i_ch][band][block_size_samples/NBANDS],
                   block_size_samples/NBANDS*2*sizeof(double)
                   );
          }

		if(gc_switch == GC_PRESENT){
          /* packing gain data into the gc_stream */
          son_gc_pack(gc_stream[i_ch], windowSequence[MONO_CHAN],
                      max_band[i_ch], gainInfo[i_ch]);
		}

        } /* for i_ch...*/
      } /* if (pp_select == AAC_PP).. */
   
  } /* pre processing */


  /******************************************************************************************************************************
  *
  * psychoacoustic
  *
  ******************************************************************************************************************************/
   /*
   if((qc_select != NTT_VQ) && (qc_select != NTT_VQ_SYS))*/
   {
    EncTf_psycho_acoustic( fSampleLong,
                           1,
                           qc_select,
                           block_size_samples,
                           chpo_long, chpo_short );
   }
   if (debugLevel>2) fprintf(stderr,"NNNNN %d \n",chpo_long[0].no_of_cb);
  
  /******************************************************************************************************************************
  *
  * T/F mapping
  *
  ******************************************************************************************************************************/
  {
    /* son_AACpp */
    if (pp_select == AAC_PP)
      {
	int band;
	/* added 971010 YT*/
        int i_ch, n_ch;
        n_ch = max_ch;
        for (i_ch=0; i_ch<n_ch; i_ch++){

          for (band = 0; band < NBANDS; band++) {
            buffer2freq(
                        DBandSigWithGCBuf[i_ch][band],
                        &spectral_line_vector_for_gc[i_ch][block_size_samples/NBANDS*band],
                        overlap_buffer[i_ch],	/* dummy */
                        windowSequence[MONO_CHAN],
                        windowShape[i_ch],
                        windowShapePrev[i_ch],
                        block_size_samples/NBANDS, 
                        block_size_samples/NBANDS/med_win_in_long,
                        block_size_samples/NBANDS/short_win_in_long,
                        NON_OVERLAPPED_MODE,
                        short_win_in_long
                        );
          }
          son_gc_arrangeSpecEnc(
                                spectral_line_vector_for_gc[i_ch],
                                block_size_samples,
                                windowSequence[MONO_CHAN],
                                spectral_line_vector[i_ch]
                                );
        } /* for(i_ch...*/
      }
    else /* if (pp_select != AAC_PP) */{
      int i_ch, n_ch;
      n_ch = max_ch;
#ifdef DEBUGPLOT 
      plotSend("l", "timeL",  MTV_DOUBLE,block_size_samples, DTimeSigBuf[0], NULL);
      if (n_ch==2)
        plotSend("r", "timeR",  MTV_DOUBLE,block_size_samples,DTimeSigBuf[1],  NULL);
#endif    

      for (i_ch=0; i_ch<n_ch; i_ch++){
	buffer2freq(
		    DTimeSigBuf[i_ch],
		    spectral_line_vector[i_ch],
		    overlap_buffer[i_ch],
		    windowSequence[MONO_CHAN],
                    windowShape[i_ch],
                    windowShapePrev[i_ch],
		    block_size_samples, 
		    block_size_samples/med_win_in_long,
		    block_size_samples/short_win_in_long,
		    OVERLAPPED_MODE,
		    short_win_in_long		/* HP 971023 */
		    );
      }
#ifdef DEBUGPLOT 
      plotSend("l", "specL",  MTV_DOUBLE_SQA,block_size_samples,spectral_line_vector[0] , NULL);
      if (n_ch==2)
        plotSend("r", "specR",  MTV_DOUBLE_SQA,block_size_samples,spectral_line_vector[1] ,  NULL);
#endif    

    }
    
  
    /* used_bits += missing if tf_buffer2freq adds something to the fixed stream */

#if 0
    {
      int i, k;
      for( k=0; k<8; k++ ) {
        printf( "\n" );
        for( i=0; i<128; i++ ) {
          printf( "%12.6f ",  spectral_line_vector[MONO_CHAN][(k*128)+i] );
          if( (i%8) ==0 ) printf( "\n" );
        }
      }
    }
#endif
  }



  /******************************************************************************************************************************
  *
  * adapt ratios of psychoacoustic module to codec scale factor bands
  *
  ******************************************************************************************************************************/
    {
      int    par_set;

      par_set = 0;

      switch( windowSequence[MONO_CHAN] ) {
      case ONLY_LONG_SEQUENCE :
        memcpy( (char*)sfb_width_table[MONO_CHAN], (char*)chpo_long[MONO_CHAN].cb_width, (NSFB_LONG+1)*sizeof(int) );
        nr_of_sfb[MONO_CHAN] = chpo_long[MONO_CHAN].no_of_cb;
        p_ratio[MONO_CHAN]   = chpo_long[MONO_CHAN].p_ratio;
        par_set = 1;
        break;
      case LONG_START_SEQUENCE :
        memcpy( (char*)sfb_width_table[MONO_CHAN], (char*)chpo_long[MONO_CHAN].cb_width, (NSFB_LONG+1)*sizeof(int) );
        nr_of_sfb[MONO_CHAN] = chpo_long[MONO_CHAN].no_of_cb;
        p_ratio[MONO_CHAN]   = chpo_long[MONO_CHAN].p_ratio;
        par_set = 1;
        break;
      case EIGHT_SHORT_SEQUENCE :
        memcpy( (char*)sfb_width_table[MONO_CHAN], (char*)chpo_short[MONO_CHAN][0].cb_width, (NSFB_SHORT+1)*sizeof(int) );
        nr_of_sfb[MONO_CHAN] = chpo_short[MONO_CHAN][0].no_of_cb;
        p_ratio[MONO_CHAN]   = chpo_short[MONO_CHAN][0].p_ratio;
        par_set = 1;
        break;
      case LONG_STOP_SEQUENCE :
        memcpy( (char*)sfb_width_table[MONO_CHAN], (char*)chpo_long[MONO_CHAN].cb_width, (NSFB_LONG+1)*sizeof(int) );
        nr_of_sfb[MONO_CHAN] = chpo_long[MONO_CHAN].no_of_cb;
        p_ratio[MONO_CHAN]   = chpo_long[MONO_CHAN].p_ratio;
        par_set = 1;
        break;
      default:
        break;
      
      }

      /* to prevent undefined ptr just use a meaningful value to allow for experiments with the meachanism above */
      if( !par_set) {   
        memcpy( (char*)sfb_width_table[MONO_CHAN], (char*)chpo_long[MONO_CHAN].cb_width, (NSFB_LONG+1)*sizeof(int) );
        nr_of_sfb[MONO_CHAN]  = chpo_long[MONO_CHAN].no_of_cb;
        p_ratio[MONO_CHAN] = chpo_long[MONO_CHAN].p_ratio;
      }
    }


  /* copy sfb tables */
  for(i=0; i<max_ch; i++)
    nr_of_sfb[i] = nr_of_sfb[MONO_CHAN];


  /* Set the AAC grouping information.  */
  switch( qc_select ) {
  case AAC_QC:
  case AAC_SCALABLE:
  case AAC_SYS:
#ifdef VERSION2
  case AAC_BSAC:
  case AAC_SYS_BSAC:
  case AAC_LD:
#endif
    if (windowSequence[MONO_CHAN] == EIGHT_SHORT_SEQUENCE) {
#if 1
      num_window_groups=8;
      window_group_length[0] = 1;
      window_group_length[1] = 1;
      window_group_length[2] = 1;
      window_group_length[3] = 1;
      window_group_length[4] = 1;
      window_group_length[5] = 1;
      window_group_length[6] = 1;
      window_group_length[7] = 1;
#else
      num_window_groups=1;
      window_group_length[0] = 8;
      window_group_length[1] = 0;
      window_group_length[2] = 0;
      window_group_length[3] = 0;
#endif
    } else {
      num_window_groups = 1;
      window_group_length[0] = 1;
    }
    break;
  
  default:
    break;
    } /* switch qc_select */

  /* Calculate sfb-offset table. Needed by (at least) TNS and LTP. */
  if ((qc_select != AAC_QC)
#ifdef VERSION2
      && (qc_select != AAC_LD)
      && (qc_select != AAC_BSAC)
      && (qc_select != AAC_SYS_BSAC)
#endif
      )
  ntt_maxsfb( &ntt_index, sfb_width_table, nr_of_sfb, -1);
  for(ch = 0; ch < max_ch; ch++)
  {
    sfb_offset[ch][0] = 0;
    k=0;
    for(i = 0; i < nr_of_sfb[ch]; i++ )
    {
      sfb_offset[ch][i] = k;
      k += sfb_width_table[MONO_CHAN /***TM ch **/][i];
    }
    sfb_offset[ch][i] = k;
  }

  /******************************************************************************************************************************
  *
  * carry out TNS processing
  *
  ******************************************************************************************************************************/
  if (tns_enable || tvq_tns_enable) {
    for (i_ch=0; i_ch<max_ch; i_ch++) {
      TnsEncode(nr_of_sfb[MONO_CHAN],                  /* Number of bands per window */
                nr_of_sfb[MONO_CHAN],                  /* max_sfb */ 
                windowSequence[MONO_CHAN],                 /* block type */
                sfb_offset[MONO_CHAN],                 /* Scalefactor band offset table */
                spectral_line_vector[i_ch],            /* Spectral data array */
                tnsInfo[i_ch]);                        /* TNS info */
        }
  }


  /******************************************************************************************************************************
  *
  * compute energy and allowed distortion from the original spectrum
  *
  ******************************************************************************************************************************/
  /* These have to be done before prediction (not, for example, inside
     the quantizer) since the signal going to the quantizer is changed
     by the prediction. Do not use p_ratio after this. Use
     allowed_distortion (and p_energy) instead.
     1997-11-23 Mikko Suonio  */
  switch( qc_select ) {
  case AAC_QC:
  case AAC_SCALABLE:
  case AAC_SYS:
#ifdef VERSION2
  case AAC_BSAC:
  case AAC_SYS_BSAC:
  case AAC_LD:
#endif
    {
      int group, i_ch, index, i, j, sfb;
      double dtmp;

      for (i_ch = 0; i_ch < max_ch; i_ch++) {
	index = 0;
	/* calculate the scale factor band energy in window groups  */
	for (group = 0; group < num_window_groups; group++) {
	  for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++)
	    p_energy[i_ch][sfb + group * nr_of_sfb[i_ch]] = 0.0;
	  for (i = 0; i < window_group_length[group]; i++) {
	    for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++) {
	      for (j = 0; j < sfb_width_table[MONO_CHAN][sfb]; j++) {
		dtmp = spectral_line_vector[i_ch][index++];   
		p_energy[i_ch][sfb + group * nr_of_sfb[i_ch]] += dtmp * dtmp;

		if (debugLevel > 6 )
		  fprintf(stderr,"enc_tf.c: spectral_line_vector index %d; %d %d %d %d %d\n", index, i_ch, group, i, sfb, j);
	      }
	    }
	  }
	}
	/* calculate the allowed distortion */
	/* Currently the ratios from psychoacoustic model
	   are common for all subwindows, that is, there is only
	   num_of_sfb[i_ch] elements in p_ratio[i_ch]. However, the
	   distortion has to be specified for scalefactor bands in all
	   window groups. */
	for (group = 0; group < num_window_groups; group++) {
	  for (sfb = 0; sfb < nr_of_sfb[i_ch]; sfb++) {
	    index = sfb + group * nr_of_sfb[i_ch];
	    if ((10 * log10(p_energy[i_ch][index] + 1e-15)) > 70)
	      allowed_distortion[i_ch][index] = p_energy[i_ch][index] * p_ratio[MONO_CHAN][sfb];
	    else
	      allowed_distortion[i_ch][index] = p_energy[i_ch][index] * 1.1;
	  }
	}
      }
      break;
    }
  default:
    break;
  } /* switch qc_select */


  /******************************************************************************************************************************
  *
  * prediction
  *
  ******************************************************************************************************************************/
      
  switch (qc_select)
  {
    case AAC_SYS:
    case AAC_QC:
#ifdef VERSION2
    case AAC_LD:
#endif
     if (pred_type == NOK_LTP)
      {
	for(ch = 0; ch < max_ch; ch++)
	{ 
	  if(qc_select == AAC_SYS ) {
	    for(i = 0; i < block_size_samples; i++)
	      baselayer_spectral_line_vector[ch][i] = 
		spectral_line_vector[ch][i];
	    if(coreCodecIdx != NO_CORE)
	      break;
	  }
#if 1
	  nok_ltp_enc(spectral_line_vector[ch],
		      nok_tmp_DTimeSigBuf[ch],
		      windowSequence[ch],
		      windowShape[ch], windowShapePrev[ch], 
		      block_size_samples, 
		      block_size_samples/med_win_in_long,
		      block_size_samples/short_win_in_long,
		      &sfb_offset[ch][0], nr_of_sfb[ch], 
		      &nok_lt_status[ch],
		      /*((tns_enable) ? */tnsInfo[ch] /*: NULL)*/ , 
		      qc_select);
#endif	
	}  
      }
      break;
      
    default:
      break;
  }

  
/******************************************************************************************************************************/
/*
 * quantization and coding
 *
 ******************************************************************************************************************************/

  { 
    int used_bits;
    int qc_bits;
    int padding_limit = max_bitreservoir_bits;    /* f( max__bitreservoir_bits ) */

    used_bits = 0;

   /* a 7 bit alternative to the strange MPEG-1 audio syncword */
    if( (transport_stream != AAC_RAWDATA_TSTREAM) &&
        (transport_stream != NO_TSTREAM) &&
        (transport_stream != NO_SYNCWORD) ) {
      BsPutBit( fixed_stream, 0x6e, 7 ); 
      used_bits += 7;
    }
   

    /* bit budget */
    num_bits_available = (long)(average_bits + available_bitreservoir_bits - used_bits);

    /* QC selection is currently assumed to be mutually exclusive. */
    switch( qc_select ) {
     
    case AAC_QC:
#ifdef VERSION2
    case AAC_BSAC: /* YB : 971106 */
    case AAC_LD:
#endif
      {
        qc_bits = tf_encode_spectrum_aac( spectral_line_vector,
                                          p_energy,
                                          allowed_distortion,
                                          windowSequence,
                                          sfb_width_table,
                                          nr_of_sfb,
                                          average_bits - used_bits,
                                          available_bitreservoir_bits,
                                          padding_limit,
                                          fixed_stream, /*tmp_stream,*/
                                          var_stream,
                                          gcBitBuf,
                                          max_ch,                        /* nr of audio channels */
                                          reconstructed_spectrum,
                                          useShortWindows,
                                          windowShape,
                                          aacAllowScalefacs,
                                          fSampleLong,
                                          qc_select,
                                          pred_type,
                                          nok_lt_status,
                                          block_size_samples,
                                          num_window_groups,
                                          window_group_length,
                                          (tns_enable) ? tnsInfo : NULL,
                                          15000 /* bandwidth in Hz */
                                          );
      

        /* fill in the overall length information 
           if( (transport_stream != AAC_RAWDATA_TSTREAM) && (transport_stream != NO_TSTREAM) ) {
           BsPutBit( fixed_stream, qc_bits+used_bits, 12 );
           }   
        */
     
        used_bits += qc_bits;
        /* add tmp_stream 
           BsPutBuffer ( fixed_stream, BsGetBitBufferHandle(0,tmp_stream)); */
        /* BsPutBuffer ( fixed_stream, tmpBitBuf ); */
     

        {
          int len, i,j;
          len = BsCurrentBit( fixed_stream );
          /* for( i=0; i<(8-(len%8)); i++ ) {  OLD WAY */
	   
          j = (8 - (len%8))%8;

          if (debugLevel > 2 ) {
            fprintf(stderr,"\nHave to write  %d bits for bytealignment %d (test len=%d) bits", j, qc_bits, len);
          }
          if ((len % 8) == 0) j = 0;
          for( i=0; i<j; i++ ) {
            BsPutBit( fixed_stream, 1, 1 ); 
          }
        }
      }
      break;
    case  AAC_SCALABLE:
         qc_bits = aacScaleableEncode(  spectral_line_vector,
                                       p_energy,
                                       allowed_distortion,
                                       windowSequence,
                                       sfb_width_table,
                                       nr_of_sfb,
                                       average_bits - used_bits,
                                       available_bitreservoir_bits,
                                       padding_limit,
                                       fixed_stream, /*tmp_stream,*/
                                       var_stream,
                                       1,                        /* nr of audio channels */
                                       reconstructed_spectrum,
                                       useShortWindows,
                                       windowShape,
                                       aacAllowScalefacs,
                                       block_size_samples, 
                                       frameMaxNumBit,
                                       coreTimeSigBuf,
                                       bitRateLong,
                                       fSampleLong,
                                       qc_select,
                                       tnsInfo,
                                       frameData,
                                       coreCodecIdx
                                       );

         used_bits += qc_bits;
         break;
    case AAC_SYS:
/*      ntt_index.tvq_ppc_enable = tvq_flag_ppc;  */

      qc_bits = aacScaleableFlexMuxEncode(  spectral_line_vector,
                                            p_energy,
                                            allowed_distortion,
                                            windowSequence,
                                            sfb_width_table,
                                            nr_of_sfb,
                                            average_bits - used_bits,
                                            available_bitreservoir_bits,
                                            padding_limit,
                                            fixed_stream, /*tmp_stream,*/
                                            var_stream,
                                            max_ch /*1*/,                        /* nr of audio channels */
                                            reconstructed_spectrum,
                                            useShortWindows,
                                            windowShape,
                                            windowShapePrev,
                                            aacAllowScalefacs,
                                            block_size_samples, 
                                            frameMaxNumBit,
                                            coreTimeSigBuf,
                                            bitRateLong,
                                            fSampleLong,
                                            qc_select,
                                            tnsInfo,                                            
                                            frameData,
                                            coreCodecIdx ,
                                            num_window_groups,
                                            window_group_length,
                                            &param_ntt,
					    baselayer_spectral_line_vector,
					    nok_lt_status,
					    med_win_in_long,
					    short_win_in_long,
					    &sfb_offset[MONO_CHAN][0],
					    nok_tmp_DTimeSigBuf,
					    &ntt_index,
					    &ntt_index_scl);
    
        used_bits += qc_bits;    
        
#ifdef DEBUGPLOT 
        plotSend("l", "recspecL",  MTV_DOUBLE_SQA,block_size_samples,reconstructed_spectrum[0] , NULL);
        if (max_ch==2)
          plotSend("r", "recspecR",  MTV_DOUBLE_SQA,block_size_samples,reconstructed_spectrum[1] ,  NULL);
#endif    

        break;

#ifdef VERSION2
    case AAC_SYS_BSAC:
      {
        int num_ES = 2;
        qc_bits = sam_FlexMuxEncode_bsac( spectral_line_vector,
                                          p_energy,
                                          allowed_distortion,
                                          windowSequence,
                                          sfb_width_table,
                                          nr_of_sfb,
                                          average_bits - used_bits,
                                          available_bitreservoir_bits,
                                          padding_limit,
                                          fixed_stream, /*tmp_stream,*/
                                          var_stream,
                                          gcBitBuf,
                                          max_ch, /* nr of audio channels */
                                          reconstructed_spectrum,
                                          useShortWindows,
                                          windowShape,
                                          aacAllowScalefacs,
                                          fSampleLong,
                                          qc_select,
                                          pred_type,
                                          nok_lt_status,
                                          block_size_samples,
                                          num_window_groups,
                                          window_group_length,
                                          (tns_enable) ? tnsInfo : NULL,
                                          15000 /* bandwidth in Hz */,
                                          num_ES );
        used_bits += qc_bits;    
      }
      break;
#endif


#if 0
        /* VQ: NTT VQ coder.  The quantizer can work with variable bits, 
           but bit budget control is missing. */
      case NTT_VQ:
        {
          int     ismp, iscl;
          int     ntt_available_bits;
          double  *ntt_external_pw[MAX_TIME_CHANNELS];
          double  lpc_spectrum[ntt_T_FR_MAX];
	  double  spectrum_orig[MAX_TIME_CHANNELS][ntt_T_FR_MAX]; /* T.Ishikawa 981215 */
	  double *tvq_target_spectrum[MAX_TIME_CHANNELS];
          int i_sup, tns_data_present[MAX_TIME_CHANNELS];
          windowShape[MONO_CHAN] = WS_FHG;
          if(tvq_flag_wshape) windowShape[MONO_CHAN] = WS_DOLBY;

	  ntt_index.bandlimit =0;
	  if(tvq_flag_bandlimit) ntt_index.bandlimit = 1; 
          ntt_index.pf=0; /* Postprocess flag */
          if(tvq_flag_postproc==1) ntt_index.pf=1;
          ntt_index.ms_mask = 0;
          if(tvq_flag_msmask1) ntt_index.ms_mask = 1;
	  
	  used_bits=0;
          BsPutBit(fixed_stream, windowSequence[MONO_CHAN], 2);
          ntt_index.w_type = windowSequence[MONO_CHAN];
          windowSequence[max_ch-1] = windowSequence[MONO_CHAN];
	  used_bits += 2;
          BsPutBit(fixed_stream,  (unsigned long)windowShape[0], 1);
          used_bits += 1;
          ntt_index.ms_mask = 0;
	  ntt_index.group_code =0x7f;
	  ntt_index.last_max_sfb[0] =0;
	  ntt_index.tvq_ppc_enable = tvq_flag_ppc;
	  ntt_index.restore_flag_tns=0;

          ntt_maxsfb( &ntt_index, sfb_width_table, nr_of_sfb, -1);
          ntt_select_ms( spectral_line_vector,
		        &ntt_index, sfb_width_table, nr_of_sfb, -1);

	  /* Backup original spectrum */
	  for(i_sup=0;i_sup<max_ch;i_sup++)
	    for(ismp=0;ismp<block_size_samples;ismp++)
	      spectrum_orig[i_sup][ismp]=spectral_line_vector[i_sup][ismp];
	  
	  ntt_tns_enc(spectral_line_vector, 
		      &ntt_index, sfb_width_table, windowSequence[MONO_CHAN],
		      nr_of_sfb, tnsInfo);
          if(ntt_index.restore_flag_tns==1) {
             if(tvq_debug_level>5)fprintf(stderr,"Restore spectrum.\n");
             for(i_sup=0;i_sup<max_ch;i_sup++)
             for(ismp=0;ismp<block_size_samples;ismp++)
             spectral_line_vector[i_sup][ismp]=spectrum_orig[i_sup][ismp];
          }

	  tvq_target_spectrum[0] = spectral_line_vector[0];
	  tvq_target_spectrum[1] = spectral_line_vector[1];
#if 1
	  if(pred_type == NOK_LTP)
	    for(ch = 0; ch < max_ch; ch++)
	    {
	      tvq_target_spectrum[ch] = baselayer_spectral_line_vector[ch];
	      
	      for(i = 0; i < block_size_samples; i++)
		baselayer_spectral_line_vector[ch][i] = spectral_line_vector[ch][i];
	      nok_ltp_enc(spectral_line_vector[ch],
			  nok_tmp_DTimeSigBuf[ch],
			  windowSequence[ch],
			  windowShape[ch], windowShapePrev[ch], 
			  block_size_samples, 
			  block_size_samples/med_win_in_long,
			  block_size_samples/short_win_in_long,
			  &sfb_offset[ch][0], nr_of_sfb[ch], 
			  &nok_lt_status[ch], 
			  /*(tvq_tns_enable) ? */ tnsInfo[ch] /* : NULL*/,
			  qc_select);
	    }
#endif
	  ntt_headerenc( -1, fixed_stream, &ntt_index, &used_bits,
			 nr_of_sfb,
			 tnsInfo, nok_lt_status, pred_type);

	/*--- External perceptual weight
            (Supported for long blocks only so far.) ---*/
          if (ntt_pw_select == NTT_PW_EXTERNAL &&
              (windowSequence[MONO_CHAN] == ONLY_LONG_SEQUENCE ||
               windowSequence[MONO_CHAN] == LONG_START_SEQUENCE ||
               windowSequence[MONO_CHAN] == LONG_STOP_SEQUENCE ))
            {
              int ii, maxfreqnum;
              double freq_scale;

              if (max_ch > 1)
                CommonExit(1, "EncTfFrame: Error in NTT_VQ ext. PE calc: No stereo support.");
       
              ntt_external_pw[MONO_CHAN] =
                (double *)malloc((size_t)(block_size_samples*sizeof(double)));

              /* calculate maximum frequency number
                 and frequency scaling factor */
              for (ii=0, maxfreqnum=0; ii<chpo_long[MONO_CHAN].no_of_cb; ii++){
                maxfreqnum += chpo_long[MONO_CHAN].cb_width[ii];
             }
              if (maxfreqnum > 0){
                freq_scale = (double)block_size_samples / (double)maxfreqnum;
              }
              else{
                CommonExit(1,
                           "EncTfFrame: Error in NTT_VQ external PE calculation");
              }
       
              /* project the cb-scale "p_ratio" to linear scale */
              maxfreqnum = 0;
              for (ii=0, ismp=0; ii<chpo_long[MONO_CHAN].no_of_cb; ii++){
                maxfreqnum += 
                  (int)floor((double)chpo_long[MONO_CHAN].cb_width[ii]*freq_scale+0.5);
                for (; ismp<maxfreqnum; ismp++){
                  ntt_external_pw[MONO_CHAN][ismp] =
                    chpo_long[MONO_CHAN].p_ratio[ii];
                }
              }
            }
	  ntt_available_bits = ntt_index.nttDataBase->ntt_NBITS_FR - used_bits;
	  ntt_vq_coder(DTimeSigBuf,
                       DTimeSigLookAheadBuf,
                       spectral_line_vector,
                       lpc_spectrum /*ntt_external_pw[MONO_CHAN]*/,
                       ntt_pw_select,
                       windowSequence[MONO_CHAN],
                       &ntt_index,
                       &param_ntt,
                       sfb_width_table,
		       nr_of_sfb,
		       ntt_available_bits,
                       lpc_spectrum,
                       reconstructed_spectrum);
#if 0
		  if(windowSequence[MONO_CHAN]==ONLY_LONG_SEQUENCE)
			  {
				  for(ismp=0;ismp<20;ismp++)
					  {
						  printf("%d\n",ntt_index.wvq[ismp]);
					  }
			  }
#endif

          /* Bit packing */
	  if (debugLevel>2)  fprintf(stderr,"%5ld\n",BsCurrentBit(fixed_stream));
          used_bits += ntt_BitPack(&ntt_index, fixed_stream);
	  if (debugLevel>2)  fprintf(stderr,"%5ld\n",BsCurrentBit(fixed_stream));

	  /* 
	   * Restore the spectrum of the base layer and update LTP buffer. 
	   */
	  if(pred_type == NOK_LTP)
	  {
	    double baselayer_rec_spectrum[BLOCK_LEN_LONG];
	    
	    for(ch = 0; ch < max_ch; ch++)
	    {   
	      for(i = 0; i < block_size_samples; i++)
		baselayer_rec_spectrum[i] = reconstructed_spectrum[ch][i];
	      
	      /* Add the LTP contribution to the reconstructed spectrum. */
	      nok_ltp_reconstruct(baselayer_rec_spectrum, 
				  windowSequence[ch], 
				  &sfb_offset[ch][0], nr_of_sfb[ch],
				  block_size_samples/short_win_in_long,
				  &nok_lt_status[ch]);
	     
	      /* This is passed to the scaleable encoder. */
	      for(i = 0; i < block_size_samples; i++)
		reconstructed_spectrum[ch][i] = baselayer_rec_spectrum[i];
	      
	      /* TNS synthesis filtering. */
	      if(tvq_tns_enable && (ntt_index.restore_flag_tns==0))
		TnsEncode2(nr_of_sfb[ch], nr_of_sfb[ch], 
			   windowSequence[ch], &sfb_offset[ch][0], 
			   baselayer_rec_spectrum, tnsInfo[ch] , 1);
	      
	      /* Update the time buffer of LTP. */
	      nok_ltp_update(baselayer_rec_spectrum, LTP_overlap_buffer[ch],
			     windowSequence[ch], windowShape[ch], 
			     windowShapePrev[ch], block_size_samples, 
			     block_size_samples/med_win_in_long, 
			     block_size_samples/short_win_in_long, short_win_in_long,
			     &nok_lt_status[ch]);
	    }
	  }

          /* Scalable encoder */
#if 1
if (debugLevel>2)  fprintf(stderr,"III %5d\n",used_bits);
if (debugLevel>2)  fprintf(stderr, "TTT %5d %5d %5d\n", 
	      ntt_NSclLay, ntt_available_bits, 
	      ntt_index_scl.nttDataScl->ntt_NBITS_FR_SCL[0]);
          {
            int ntt_available_bits;
            int iscl;
	    int  iii, jjj;
	    for(iii=0; iii<8; iii++){
	       for(jjj=0; jjj<ntt_index.max_sfb[0]; jjj++){
	         ntt_index_scl.msMask[iii][jjj]=ntt_index.msMask[iii][jjj];
	       }
	    }
            ntt_index_scl.w_type = ntt_index.w_type;
            ntt_index_scl.group_code = ntt_index.group_code;
	    ntt_index_scl.max_sfb[0] = ntt_index.max_sfb[0];
	    ntt_index_scl.pf = ntt_index.pf;
	    for (iscl=0; iscl<ntt_NSclLay; iscl++){
	    ntt_index_scl.last_max_sfb[iscl+1] = ntt_index_scl.max_sfb[iscl];
              ntt_index_scl.ms_mask = 0;
	      used_bits=0;
	      if(max_ch > 1){
                 ntt_index_scl.ms_mask = 2;
	         for(ismp=0; ismp<block_size_samples; ismp++){
		   ntt_ms_buf[0][ismp] = 
		     tvq_target_spectrum[0][ismp] - reconstructed_spectrum[0][ismp];
		   
                   ntt_ms_buf[1][ismp] = 
		     tvq_target_spectrum[1][ismp] - reconstructed_spectrum[1][ismp];
		 }
	      }	
              ntt_maxsfb( &ntt_index_scl, sfb_width_table, nr_of_sfb, iscl);
              ntt_select_ms( ntt_ms_buf,
		        &ntt_index_scl, sfb_width_table, nr_of_sfb, iscl);
	      ntt_headerenc( iscl, fixed_stream, &ntt_index_scl, &used_bits,
			     nr_of_sfb, tnsInfo, nok_lt_status, pred_type);
	      ntt_available_bits = 
		ntt_index_scl.nttDataScl->ntt_NBITS_FR_SCL[iscl]-used_bits;
	      ntt_scale_vq_coder(tvq_target_spectrum, /*spectral_line_vector,*/
                                 lpc_spectrum,
                                 &ntt_index,
                                 &ntt_index_scl,
                                 &param_ntt,
                                 sfb_width_table,
		                 nr_of_sfb,
                                 ntt_available_bits,
				 reconstructed_spectrum, iscl);

              /* Scalable bit packing */
              used_bits += ntt_SclBitPack(&ntt_index_scl, fixed_stream, iscl);
#if 0
		  if((iscl==0)&&(windowSequence[MONO_CHAN]==ONLY_LONG_SEQUENCE))
			  {
				  for(ismp=0;ismp<20;ismp++)
					  {
						  printf("%d\n",ntt_index_scl.wvq[ismp]);
					  }
			  }
#endif
            }
          }
#endif
	 
        }
        break;
#endif
      case NTT_VQ_SYS: /* T.Ishikawa 980525 */
       {
 /***TM9902*/
          
            ntt_index_scl.er_tvq =0;
            if(frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_TWIN_VQ)
            ntt_index_scl.er_tvq =1;
            used_bits += tvqScaleableFlexMuxEncode(
                    DTimeSigBuf,
                    DTimeSigLookAheadBuf,
		            spectral_line_vector,
                    nok_tmp_DTimeSigBuf,
                    baselayer_spectral_line_vector,
		            ntt_ms_buf,
                    &windowShape[MONO_CHAN],
                    &windowSequence[MONO_CHAN],
                    &ntt_index,
                    &ntt_index_scl,
                    &param_ntt,
                    fixed_stream,
                    tnsInfo,
                    sfb_width_table,
                    nr_of_sfb,
                    reconstructed_spectrum,
                    pred_type,
                    block_size_samples,
                    sfb_offset,
                    nok_lt_status
                    );
         	 
        }
        break;
      case MDCT_VALUES_16BIT:  /* debug option: pack MDCT output values */    
        { 
          double       max, scal;
          int          i, itmp, iscal;
          max = 0.0; /* KEMAL */

          BsPutBit( fixed_stream, windowSequence[MONO_CHAN], 4 );
          used_bits += 4;

          /* find maximum of spectrum */ 
          for (i=0; i<block_size_samples;i++){
            double dtest = abs(spectral_line_vector[MONO_CHAN][i]);
            max = (max<=dtest)? dtest:max;
          }
          iscal = 0;
          if( max != 0.0 ) {
            while( max > 32767.0 ) {
              max   *= 0.5;
              iscal -= 1;
            }
            while( max < 16383.0 ) {
              max   *= 2.0;
              iscal += 1;
            }
          }
          BsPutBit( fixed_stream, iscal+2048, 12 );
          used_bits += 12;

          scal = pow( 2.0, iscal );
          for( i=0; i<block_size_samples; i++ ) {
            itmp = (int)floor((scal * spectral_line_vector[MONO_CHAN][i])+.5);
            BsPutBit( fixed_stream, itmp+32768, 16 );
            used_bits += 16;
          }
        }
        break;
     
      default:
        CommonExit(-1,"\ncase not in switch");
        break;

      } /* switch qc_select */

    /* Nokia LTP buffer updates */
    if (pred_type == NOK_LTP)
      switch (qc_select)
      {
	case AAC_QC:
#ifdef VERSION2
	case AAC_LD:
#endif    
	  for(ch = 0; ch < max_ch; ch++)
	  {  
	    /* Add the LTP contribution to the reconstructed spectrum. */
	    nok_ltp_reconstruct(reconstructed_spectrum[ch], 
				windowSequence[ch], 
				&sfb_offset[ch][0], nr_of_sfb[ch],
				block_size_samples/short_win_in_long,
				&nok_lt_status[ch]);
	    
	    /* TNS synthesis filtering. */
	    if(tns_enable)
	      TnsEncode2(nr_of_sfb[ch], nr_of_sfb[ch], windowSequence[ch], 
			 &sfb_offset[ch][0], reconstructed_spectrum[ch], 
			 tnsInfo[ch], 1);
	    
	    /* Update the time buffer of LTP. */
	    nok_ltp_update(reconstructed_spectrum[ch], LTP_overlap_buffer[ch],
			   windowSequence[ch], windowShape[ch], windowShapePrev[ch], 
			   block_size_samples, 
			   block_size_samples/med_win_in_long, 
			   block_size_samples/short_win_in_long, short_win_in_long,
			   &nok_lt_status[ch]);
	  }
	  break;
	  
	default:
	  break;
      }
    
      /* save the actual window shape for next frame */
      for(i_ch=0; i_ch<max_ch; i_ch++)
        windowShapePrev[i_ch] = windowShape[i_ch];

      BsClose(fixed_stream);
      if (var_stream != NULL)
        BsClose(var_stream);
      BsClose(tmp_stream);
      BsFreeBuffer(tmpBitBuf);
      { /* added 971010 YT*/
	int i_ch;
	for(i_ch=0;i_ch<max_ch;i_ch++){ 
          BsClose(gc_stream[i_ch]);
          BsFreeBuffer(gcBitBuf[i_ch]);
	} /* for*i_ch...*/
      }


      /* Quantization and coding block */
      /* VQ: preparation for the next frame */
      ntt_InitFlag = 0;
    
  }
}

void initTvqSys( int        numChannel,	     /* in: num audio channels */
                float       sampling_rate_f, /* in: sampling frequancy [Hz] */ 
                float       bit_rate_f,       /* in: bit rate [bit/sec] */
                char        *encPara,	 /* in: encoder parameter string */
                int         quantDebugLevel,   /* in : quantDebugLevel */
                int         *frameNumSample, /*out: num samples per frame */
                int         *delayNumSample, /*out:encoder delay(num samples)*/
                ENC_FRAME_DATA* frameData,
		ntt_INDEX*    ntt_index,
		ntt_INDEX*    ntt_index_scl,
		ntt_DATA*   nttData) 
{
  int          ntt_IBPS, ntt_IBPS_SCL[8];
  int          ntt_NSclLay;
  
  int sampling_rate = (int)(sampling_rate_f+.5);
  int bit_rate = (int)(bit_rate_f+.5);
  char *p_ctmp;
  {
      int    iscl,ntt_core_br,tomo_tmp;
      float  t_bit_rate_scl,tomo_ftmp;
      float total_tvq_bitrate;
      int flex_bits,flexnum,total_frame_bits;/* T.Ishikawa 980702 */

      fprintf(stderr,"#### TwinVQ(FlexMux) Global Init %x \n",qc_select);
      
      /* bitrate control */
      ntt_NSclLay = 0;
      flexnum = 0;
      

      /* TNS handling for TWIN VQ codec E. Allamache 981117 */
      if ((p_ctmp=strstr( encPara, "-tvq_tns" ))) {
        int i_ch;
	    for (i_ch=0; i_ch<numChannel; i_ch++) {
          tnsInfo[i_ch] = (TNS_INFO*)calloc(1, sizeof(TNS_INFO));
          TnsInit( (long)sampling_rate_f, LOW /*MAIN*/, tnsInfo[i_ch], qc_select, block_size_samples);
        }
        tvq_tns_enable = 1;
      }


      /* T.Ishikawa 980525 */
      if( (p_ctmp=strstr( encPara, "-core_br" )) ) {
        sscanf( p_ctmp+8, "%d", &ntt_core_br  );
      }	  
      else
        {
          ntt_core_br = 8000;
          fprintf(stderr,"Warning!! : TwinVQ(System) BaseLayer_Br is set %d bps(Default)\n",ntt_core_br);
        }

      if(( p_ctmp=strstr( encPara, "-nttScl " ) )) {
	/* HP 970509   added an 8th "%d" */
	if((ntt_NSclLay=sscanf( p_ctmp+8, "%d %d %d %d %d %d %d %d",
                                &ntt_IBPS_SCL[0],
                                &ntt_IBPS_SCL[1],
                                &ntt_IBPS_SCL[2],
                                &ntt_IBPS_SCL[3],
                                &ntt_IBPS_SCL[4],
                                &ntt_IBPS_SCL[5],
                                &ntt_IBPS_SCL[6],
                                &ntt_IBPS_SCL[7])) == 0) {
	  CommonExit(1, "Encode: parameter of -nttScl switch not found");
	}
      }
      t_bit_rate_scl = 0.;


      for (iscl=0; iscl<ntt_NSclLay; iscl++){
	t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
      }

	  /* TwinVQ for System T.Ishikawa 980525 */
      {
          /* BaseLayer */
          frameData->layer[0].bitRate = (int)ntt_core_br;
          frameData->layer[0].sampleRate = sampling_rate;
          ntt_IBPS = frameData->layer[0].bitRate;     
          /* Scl-Layer */
          for(iscl=1;iscl<ntt_NSclLay+1;iscl++) {
              frameData->layer[iscl].bitRate = ntt_IBPS_SCL[iscl-1];
              frameData->layer[iscl].sampleRate = sampling_rate;
           }
        }
      
     
      /* T.Ishikawa 980813 */
      block_size_samples = *frameNumSample;
      
      if ((p_ctmp=strstr( encPara, "-tvq_960" ))) { 
	  tvq_flag_960=1;
          block_size_samples = 960;
    }
    else{
         tvq_flag_960=0;
          block_size_samples = 1024;
      } 
      
      tomo_ftmp=bit_rate_f;
      total_tvq_bitrate = (float)ntt_core_br + t_bit_rate_scl;
      mat_total_tvq_bits = 0;

      /* T.Ishikawa 980702 */
      total_frame_bits = (int)floor(bit_rate_f*block_size_samples/sampling_rate_f+0.5);
	  
/*	  fprintf(stderr,"<<< bitrate %e  , block_size_sample %d , fs %e\n ",total_tvq_bitrate, block_size_samples, sampling_rate_f);*/
	  

      tvq_flag_wshape =0;
      if ((p_ctmp=strstr(encPara, "-tvq_wshape"))) { tvq_flag_wshape =1; }
      tvq_tns_enable =0;
      if ((p_ctmp=strstr(encPara, "-tvq_tns"))) { tvq_tns_enable =1; }
      tvq_flag_ppc =0;
      if ((p_ctmp=strstr(encPara, "-tvq_ppc"))) { tvq_flag_ppc =1; }
      tvq_flag_bandlimit =0;
      if ((p_ctmp=strstr(encPara, "-tvq_bandlimit"))){tvq_flag_bandlimit =1; }
      tvq_flag_postproc =0;
      if ((p_ctmp=strstr(encPara, "-tvq_postproc" ))) { tvq_flag_postproc =1; }
      tvq_flag_msmask1 =0;
      if ((p_ctmp=strstr(encPara, "-tvq_msmask1" ))) { tvq_flag_msmask1 =1; }
      
      *frameNumSample = block_size_samples;
      *delayNumSample = block_size_samples; 

      /* initialization of base coder */
      if(tvq_debug_level > 1)
        fprintf(stderr," \n encoding with TwinVQ mode at %8d bit/s/ch \n",
	  ntt_IBPS/numChannel);
      if(ntt_IBPS/numChannel > 16000 )
        fprintf(stderr,"AAC or BSAC is recommended to use at this rate\n");
     
      ntt_TfInit(sampling_rate_f, total_tvq_bitrate, 
        t_bit_rate_scl, (long)numChannel, block_size_samples, 
        ntt_index, ntt_index_scl); /* global init. */
      /* initialization of scalable coders */
      t_bit_rate_scl = 0.;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
	ntt_scale_init(iscl, ntt_NSclLay, 
	  ntt_IBPS_SCL[iscl], sampling_rate_f, ntt_index, ntt_index_scl);
        nttData->nttDataScl=ntt_index_scl->nttDataScl;
	if( strstr( encPara, "-nttSclMsg" ) ) {
	  ntt_scale_message(iscl,  ntt_IBPS, ntt_IBPS_SCL);
	}
        {
	int jscl;
	if(debugLevel> 2)
	  for(jscl=0;jscl<4;jscl++)
	  fprintf(stderr, "AC_TOP[%d][%d] %f , AC_BTM[%d][%d] %f\n",
	  iscl,jscl, ntt_index_scl->nttDataScl->ac_top[iscl][0][jscl],
	  iscl,jscl, ntt_index_scl->nttDataScl->ac_btm[iscl][0][jscl]);
        }
      }
        ntt_index->nttDataScl->ntt_NSclLay = ntt_NSclLay; 
        ntt_index_scl->nttDataScl->ntt_NSclLay = ntt_NSclLay;
	nttData->nttDataBase = ntt_index->nttDataBase;
	nttData->nttDataScl = ntt_index_scl->nttDataScl;
	 flexnum += (int)floor(ntt_index->nttDataBase->ntt_NBITS_FR/2040 + 1);
	  for(iscl=0;iscl<ntt_NSclLay;iscl++)
	  {
	    flexnum += 
	    (int)floor(ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]/2040+1);
	  }

	  flex_bits = flexnum*24;
	  if(debugLevel> 2)fprintf(stderr,"FFFFFF flex_bits %d\n",flex_bits);
	  
	  /* T.Ishikawa 981118 */
	  mat_total_tvq_bits += ntt_index->nttDataBase->ntt_NBITS_FR;
	  for(iscl=0;iscl<ntt_NSclLay;iscl++)
	  mat_total_tvq_bits += ntt_index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl];
	  
 	  tomo_tmp=(mat_total_tvq_bits + flex_bits - total_frame_bits);/* T.Ishikawa 980702 */
          if(debugLevel >2)
	  fprintf(stderr,"EncTfInit : %5d %5d %5d %5d\n",mat_total_tvq_bits,flex_bits,total_frame_bits,flexnum);

	  /* Check FlexMux_Bit and TwinVQ_Bit T.Ishikawa 980525 */
	  if(tomo_tmp>0)
		  {
			  CommonExit(1,"This TwinVQ(System) mode is more than Total %7.2f bps/ch\n",(mat_total_tvq_bits+flex_bits-0.5)*sampling_rate/(float)block_size_samples);
		  }

	  if(debugLevel >2)
	  fprintf(stderr,"BAND %f BPS %d\n",ntt_index->nttDataBase->bandUpper,ntt_IBPS);
	  for(iscl=0;iscl<ntt_NSclLay;iscl++)
	  if(debugLevel >2)
	  fprintf(stderr,"BPS_SCL[%d] %d\n",iscl,ntt_IBPS_SCL[iscl]);

	  if(debugLevel >2)
	  fprintf(stderr,"CH %d\n",numChannel);

      if ( strstr( encPara, "-ntt_pw_ext" ) ){/* perceptual weight selection */
	ntt_pw_select = NTT_PW_EXTERNAL;
	if (numChannel > 1)
	  CommonExit(1, "EncTfInit: Error in NTT_VQ: No stereo support with NTT_PW_EXTERNAL.");
      }
      if ( strstr( encPara, "-ntt_pw_int" ) ){
	ntt_pw_select = NTT_PW_INTERNAL;
      }
      if (strstr( encPara, "-ntt_varbit_on") ){ /* Variable bit switch */
	ntt_varbit = NTT_VARBIT_ON;
      }
      if (strstr (encPara, "-ntt_varbit_off")){
	ntt_varbit = NTT_VARBIT_OFF;
      }
  }
}
