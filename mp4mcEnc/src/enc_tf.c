/* $Id: enc_tf.c,v 1.9 2003/01/29 13:11:56 boe Exp $
 */
/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A / University of Erlangen (UER)

and edited by
  Sony Corporation

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
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * ADD   Alberto Duenas, NDS <alberto@ndsuk.com>
 * JB    Jeremy Bennett, NDS <jbennett@ndsuk.com>
 * RG    Ralf Geiger, FhG/IIS
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
 * 08-jun-97   CL   Modified for use in MPEG-2 AAC technical report encoder software.
 *                  Removed support for medium windows and NTT VQ.  Added the use of
 *                  AAC fill elements for padding the frame when needed.  Added call
 *                  to WriteSCE for writing AAC single_channel_element.  Added
 *                  use of AACQuantInfo structure to reduce the number of parameters
 *                  required for some function calls.
 * 14-sep-97   CL   Integrated psychoacoustic module and prediction.
 * 24-oct-97   CL   Modified call to prediction to enable reset for short windows.
 *                  Integrated TNS.  Moved call to psycho model prior to SSR pre-
 *                  processing.  Modified call to psycho model to use look ahead
 *                  buffer for window switching decision.  Added window switching
 *                  control to WINDOW_SWITCH_MODE of FFT_PE_WINDOW_SWITCHING.  Moved
 *                  energy computation for short and long filterbank data prior to
 *                  TNS.  Added control of max_sfb and scalefactor_grouping to
 *                  EncTfFrame. 
 * 03-dec-97   ADD & JB   Added the concept of prediction reset.
 * 17-dec-97   CL   Increased the dimension on spectral_line_vector and
 *                  spectral_line_vector_for_gc to 2*block_size_samples since the mdct
 *                  function was using them as such.
 * 22-Jan-98   CL   Added multichannel support.  Added initial MS and IS joint coding
 *                  modules.
 * 07-apr-98   RG   Added encoding of LFE channel: window shape is sine, block type is
 *                  ONLY_LONG_WINDOW, spectral coefficients with index higher 12 are 
 *                  zero, TNS is disabled, prediction is disabled, relevant ratios 
 *                  from psycho model are fixed to -24dB, LFE-bitrate is reduced 
 *                  compared to other channels.
 * 27-May-98   MCF  Added support for independent coupling channel.
 * 24-July-98  MCF  Added support for dynamic range control.   
 *
 ******************************************************************************************/
#include <memory.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "tf_main.h"
#include "bitstream.h"
#include "interface.h" 
#include "common.h"	/* common module */
#include "drc.h"
#include "enc.h"
#include "block.h"
#include "psych.h"
#include "aac_back_pred.h"
#include "assert.h"
#include "mc_enc.h"
#include "ms.h"
#include "is.h"

/* son_AACpp */
#include "sony_local.h"

#ifdef MPEG4V1
/* LTP */
#include "nok_ltp_enc.h"
#endif 

/* other aac */
#include "aac_qc.h"
#include "all.h"
#include "aac_se_enc.h"

#ifdef CT_SBR
/* SBR */
#include "ct_sbr.h"
#endif

static int     block_size_samples = 1024;  /* nr of samples per block in one! audio channel */
static int     med_win_in_long    = 2;     
static int     short_win_in_long  = 8;
static int     bw_lines_long, bw_lines_med, bw_lines_short;  /* no of lines of the spectrum which are processed */
static int     start_line = 0;
static int     max_ch;    /* no of of audio channels */
static int	   nchans;
static int     debugLevel;
static double *spectral_line_vector[MAX_TIME_CHANNELS];
static double *residual_spectrum[MAX_TIME_CHANNELS];
static double *reconstructed_spectrum[MAX_TIME_CHANNELS];
static double *overlap_buffer[MAX_TIME_CHANNELS];
static double *DTimeSigBuf[MAX_TIME_CHANNELS];
static double *DTimeSigLookAheadBuf[MAX_TIME_CHANNELS];
#ifdef MPEG4V1
static double *time_buffer[MAX_TIME_CHANNELS];
static double *ltp_overlap_buffer[MAX_TIME_CHANNELS];
#endif

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

/* LT prediction */
#ifdef MPEG4V1
static double *nok_tmp_DTimeSigBuf[MAX_TIME_CHANNELS]; /* temporary fix to the buffer size problem. */
#endif

/* static variables used by the T/F mapping */
/*static enum WIN_SWITCH_MODE win_switch_mode = STATIC_SHORT; */          /* Only short windows */
/*static enum WIN_SWITCH_MODE win_switch_mode = STATIC_LONG; */      /* Only long windows */
/*static enum WIN_SWITCH_MODE win_switch_mode = LS_STARTSTOP_SEQUENCE ;*/   /* Start-stop windows */
/*static enum WIN_SWITCH_MODE win_switch_mode = LONG_SHORT_SEQUENCE;*/     /* Cycle through windows */
static enum WIN_SWITCH_MODE win_switch_mode = STATIC_LONG;  /* Only long windows: avoids short-window noise */
static enum WINDOW_TYPE last_block_type[MAX_TIME_CHANNELS];
static enum WINDOW_TYPE desired_block_type[MAX_TIME_CHANNELS];
static enum WINDOW_TYPE next_desired_block_type[MAX_TIME_CHANNELS];
static enum CH_CONFIG        channel_configuration = CHC_MONO;     /* later f(encPara) */
static enum PP_MOD_SELECT    pp_select = NONE;                     /* latef f(encPara) */
static GC_DATA_SWITCH   gc_switch = GC_PRESENT; 
/*static enum GC_DATA_SWITCH   gc_switch = GC_NON_PRESENT;*/ 
static enum QC_MOD_SELECT    qc_select = AAC_QC;                   /* later f(encPara) */
static enum TF_MOD_SELECT    tf_select = MDCT_AAC;                 /* later f(encPara) */
static enum TRANSPORT_STREAM transport_stream =  LENINFO_TSTREAM;  /* later f(encPara) */
static short cplenabl = 0;					/* Enables coupling from the command line */
static short isenable = 1;                                      /* Enable intensity stereo */
static short msenable = 1;                                      /* Enable ms stereo */
static short tnsenable = 1;                                     /* Enable tns */

static enum AAC_PROFILE      profile = MAIN;

/* Additional variables for AAC */
static int aacAllowScalefacs=1;              /* Allow AAC scalefactors to be nonconstant */
static TNS_INFO tnsInfo[MAX_TIME_CHANNELS];
#ifdef MPEG4V1
static NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS];
#endif
#ifdef CT_SBR
static SBR_CHAN sbrChan[MAX_TIME_CHANNELS];
#ifdef PARAMETRICSTEREO
static int sbrpsenc = 0;
#endif
#endif

AACQuantInfo quantInfo[MAX_TIME_CHANNELS];               /* Info structure for AAC quantization and coding */

/* Channel information */
Ch_Info channelInfo[MAX_TIME_CHANNELS];

/* AAC shorter windows 960-480-120 */
static int useShortWindows=0;  /* don't use shorter windows */

static int cplidx;
static CPL_VARS cvars;
static DSPshort chincpl[MAX_TIME_CHANNELS] = {1, 1, 1, 1, 1, 1};
static DSPfract cplmix[MAX_TIME_CHANNELS];
static DSPshort **cplexps;

static int sampling_rate;
int max_pred_sfb;

/********************************************************/
/* functions EncTfInfo() and EncTfFree()   HP 21-aug-96 */
/********************************************************/


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
            "-main      Main profile. (default)\n"
            "-lc        Low-complexity profile.\n"
            "-ssr       SSR profile.\n"
#ifdef MPEG4V1
            "-ltp       LTP profile.\n"
#endif
/* Moved to encoder.c, CL */
/*
#ifdef MPEG4V1
	    "-pns <start_band_for_perceptual_noise_subst>\n"
#endif
*/
	    "\n");
  }
  return PROGVER;
}


/* EncTfFree() */
/* Free memory allocated by t/f-based encoder core. */

void EncTfFree (void)
{
}

/* EncTfGetProfile() */
/* Return AAC profile selected by EncTfInit() (enum AAC_PROFILE). */

int EncTfGetProfile (void)
{
  return (int)profile;
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
  int         bitReservInit,     /* in : number of bits in reservoir */
  int         varBitRate,        /* in : equals 1 if variable bit rate, else 0 */
  int         *frameNumSample,	 /* out: num samples per frame */
  int         *delayNumSample,	 /* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader,        /* out: header for bit stream */
  int		  icplenabl,			 /* couple enable flag */
  int         isdisable,         /* intensity stereo enable */
  int         msdisable,         /* ms stereo enable */
  int         tnsdisable,        /* tns enable */
  int         pnsSfbStart        /* Start band for PNS */
#ifdef CT_SBR
  , int       sbrenable
#ifdef PARAMETRICSTEREO
  , int       sbrpsenable
#endif
#endif
  )
{
  BsBitStream *header_stream;
  int chanNum;
  int bit_rate = (int)(bit_rate_f+.5);

  char *p_ctmp;
  long superframe_blocks     = 1;        /* no of blocks in a superframe; later f(encPara) */

#ifdef CT_SBR
#ifdef PARAMETRICSTEREO
  if (sbrenable && sbrpsenable) {
    numChannel = 1;
  }
#endif
#endif

  debugLevel=  quantDebugLevel;
  cplenabl = icplenabl;
  nchans = numChannel + cplenabl;

  sampling_rate = (int)(sampling_rate_f+.5);

#if 0
  if (numChannel != 1)
    CommonExit(1,"EncTfInit: audio data has more the one channel (%d)",
	       numChannel);
#else
  if (nchans > MAX_TIME_CHANNELS)
    CommonExit(1,"EncTfInit: audio data has more than %d channels (%d)",
	       MAX_TIME_CHANNELS,nchans);
#endif

  header_stream = BsOpenBufferWrite(bitHeader);

  /* Set defaults for main profile */
  profile = MAIN;
  qc_select = AAC_PRED;           /* enable prediction */
  tf_select = MDCT_AAC;          
  pp_select = NONE;               /* disable pre-processing */
  transport_stream = AAC_RAWDATA_TSTREAM;

  /* Check for pre-processing command line switch */
  if( strstr( encPara, "-ssr" ) ) {
    profile = SSR;
    pp_select = AAC_PP;           /* enable pre-processing */
    qc_select = AAC_QC;           /* disable prediction */
    transport_stream = AAC_RAWDATA_TSTREAM;
  }

  /* Check for low-complexity command line switch */
  if( strstr( encPara, "-lc" ) ) {
    profile = LOW;
    qc_select = AAC_QC;          /* disable prediction */
  }

  /* Check for window switching mode override: -wsm <mode>
     0=STATIC_LONG 1=STATIC_SHORT 2=LS_STARTSTOP 5=LONG_SHORT_SEQ 6=FFT_PE */
  if( (p_ctmp=strstr( encPara, "-wsm " )) ) {
    if( sscanf( p_ctmp+5, "%d",(int*)&win_switch_mode ) == 0 ) {
      CommonExit( 1, "Encode: parameter of -wsm switch not found" );
    }
  }

#ifdef MPEG4V1
  /* Check for LTP command line switch */
  if( strstr( encPara, "-ltp" ) ) {
    profile = LTP;
    qc_select = AAC_QC;
  }
#endif

  /* Disable is, ms, tns if desired */
  isenable = !isdisable;
  msenable = !msdisable;
  tnsenable = !tnsdisable;

/* Moved to encoder.c, CL */
/*
#ifdef MPEG4V1
  if( (p_ctmp=strstr( encPara, "-pns" )) ) {
    if( sscanf( p_ctmp+5, "%d",&pns_sfb_start ) == 0 ) {
      CommonExit( 1, "Encode: parameter of -pns switch not found" );
    }
    qc_select = AAC_QC;
 }
#endif
*/
/* Replaced with this, CL */
#ifdef MPEG4V1
  pns_sfb_start = pnsSfbStart;
  if (pns_sfb_start<=PNS_MAX_START) {
    qc_select = AAC_QC;
  }
#endif

  /* set the return values */
  max_ch = numChannel;
  {
      *frameNumSample = block_size_samples;
      *delayNumSample = 2*block_size_samples;
  }

  /* some global initializations */
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    DTimeSigBuf[chanNum]            = (double*)calloc(block_size_samples,sizeof(double));
    DTimeSigLookAheadBuf[chanNum]   = (double*)calloc(block_size_samples,sizeof(double) );
    spectral_line_vector[chanNum]   = (double*)calloc(2*block_size_samples,sizeof(double) );
#ifdef MPEG4V1
    time_buffer[chanNum] = (double*)calloc(block_size_samples,sizeof(double) );
    ltp_overlap_buffer[chanNum] = (double*)calloc(block_size_samples,sizeof(double) );
    nok_lt_status[chanNum].delay =  (int*)calloc(MAX_SHORT_WINDOWS ,sizeof(int));
    nok_tmp_DTimeSigBuf[chanNum]  = (double*)calloc(2*block_size_samples, sizeof(double));
#endif
    if (qc_select == AAC_PRED) {
      residual_spectrum[chanNum]    = (double*)calloc(block_size_samples,sizeof(double) );
    }  

    reconstructed_spectrum[chanNum] = (double*)calloc(block_size_samples,sizeof(double) );
  }

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

/* Init dynamic range parameters */

	initDynRng();
}
  

  /* initialize pre-processing */
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    last_block_type[chanNum] = ONLY_LONG_WINDOW;
    desired_block_type[chanNum] = ONLY_LONG_WINDOW;

    /* son_AACpp */
    if (pp_select == AAC_PP)
      {
	int band;
	max_band[chanNum] = (NBANDS * (start_line + bw_lines_long - 1) /
			       block_size_samples); /* max_bands[] =0,1,2,3 */

	spectral_line_vector_for_gc[chanNum] = (double*)calloc(2*block_size_samples, sizeof(double));
	DTimeSigPQFDelayCompensationBuf[chanNum]=(double *)calloc(PQFDELAY, sizeof(double));
	DTimeSigLookAheadBufForGC[chanNum]=(double *)calloc(block_size_samples,sizeof(double));

	DBandSigLookAheadBuf[chanNum] = (double **)calloc(NBANDS, sizeof(double *));
	DBandSigBufForGCAnalysis[chanNum] = (double **)calloc(NBANDS, sizeof(double *));
	DBandSigWithGCBuf[chanNum] = (double **)calloc(NBANDS, sizeof(double *));
	gainInfo[chanNum] = (GAINC **)calloc(NBANDS, sizeof(GAINC *));
    
	for (band = 0; band < NBANDS; band++) {
	  DBandSigLookAheadBuf[chanNum][band]     = (double*)calloc(block_size_samples/NBANDS, sizeof(double));
	  DBandSigBufForGCAnalysis[chanNum][band] = (double*)calloc(block_size_samples/NBANDS*3, sizeof(double));
	  DBandSigWithGCBuf[chanNum][band]        = (double*)calloc(block_size_samples/NBANDS*2, sizeof(double));
	  gainInfo[chanNum][band]                 = (GAINC *)calloc(short_win_in_long,sizeof(GAINC));
	}
      }
  }

    cplexps = (DSPshort **)calloc(MAX_TIME_CHANNELS, sizeof(DSPshort *));
    for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++)
      cplexps[chanNum] = (DSPshort *)calloc(NSFB_LONG,sizeof(DSPshort));
	cvars.cplexps = cplexps;
	cplidx = max_ch;

  /* initialize psychoacoustic module */
  EncTf_psycho_acoustic_init();

  /* initialize t/f mapping */
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    overlap_buffer[chanNum] = (double*)calloc(sizeof(double), block_size_samples );
  }

  /* initialize spectrum processing */
  /* initialize quantization and coding */
  {
    /* init encode spectrum aac */
    tf_init_encode_spectrum_aac( quantDebugLevel);
  }

  /* Init TNS */
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    TnsInit(sampling_rate,profile,&tnsInfo[chanNum]);
    quantInfo[chanNum].tnsInfo = &tnsInfo[chanNum];         /* Set pointer to TNS data */
  }
  /* Get max_sfb_pred */
  max_pred_sfb = GetMaxPredSfb(sampling_rate);

  /* Set syntax debug level */
  InitAACSyntax(debugLevel);

#ifdef MPEG4V1
  /* Init LTP predictor */
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    nok_init_lt_pred (&nok_lt_status[chanNum]);
    quantInfo[chanNum].ltpInfo = &nok_lt_status[chanNum];  /* Set pointer to LTP data */
  }
#endif

#ifdef CT_SBR
  if (sbrenable) {

    if (sampling_rate != 22050 && sampling_rate != 24000) {
      CommonExit(1,"SBR encoder only supports 44100 or 48000 Hz input !");
    }

    /* Init SBR */
    for (chanNum=0; chanNum<MAX_TIME_CHANNELS; chanNum++) {
      SbrInit(bit_rate, numChannel, &sbrChan[chanNum]);
      quantInfo[chanNum].sbrChan = &sbrChan[chanNum];
    }
  }
  else {
    for (chanNum=0; chanNum<MAX_TIME_CHANNELS; chanNum++) {
      quantInfo[chanNum].sbrChan = NULL;
    }
  }
#ifdef PARAMETRICSTEREO
  sbrpsenc = sbrpsenable;
#endif
#endif

  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++)
    quantInfo[chanNum].profile = profile;
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

void EncTfFrame (
		 float *p_time_signal[MAX_TIME_CHANNELS],   /* array of pointers to the modulo time 
													   signal input buffers for each channel */
#ifdef CT_SBR
                 float *p_time_signal_orig[MAX_TIME_CHANNELS],
#endif
		 BsBitBuffer  *fixed_bitstream,     /* bits before bitres size ptr */
		 int frameAvailNumBit,           	/* in: total num bits available for 
											   this frame (incl. bit reservoir) */
		 int frameNumBit,		            /* in: average num bits per frame */
                 int minBitsNoOverflow,     /* in: minimum number of bits needed to be used
                                               to avoid overflowing the bit reservoir */
		 int frameMaxNumBit,		        /* in: max num bits per frame */
		 int lfePresent,
		 double *icplmix,					/* independent coupling coefficients */
		 float *cplSampleBuf,
		 DRC_INTF *dvars_inpt
		 )
{
  BsBitStream *fixed_stream;
  BsBitStream *var_stream;
  BsBitStream *tmp_stream;
  BsBitBuffer *tmpBitBuf;
  BsBitStream *gc_stream[MAX_TIME_CHANNELS];  /* Bitstream pointer */
 
  int used_bits;
  int drcbits;
  int chanNum;
  int nfbwchans = 0;

#ifdef CT_SBR
  int sbr_bits = 0;
#endif

  /* Energy array (computed before prediction for long windows) */
  double energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];

  /* determine the function parameters used earlier:   HP 21-aug-96 */
  int          average_bits = frameNumBit;
  int          available_bitreservoir_bits = frameAvailNumBit-frameNumBit;

  /* actual amount of bits currently in the bit reservoir */
  /* it is the job of this module to determine 
     the no of bits to use in addition to average_block_bits
     max. available: average_block_bits + available_bitreservoir_bits */
  int max_bitreservoir_bits = frameMaxNumBit-frameNumBit;

  /* max. allowed amount of bits in the reservoir  (used to avoid padding bits) */
  long num_bits_available;

  /* the generated bitstreams are copied into the buffers designated by the */
  /* two pointers fixed_bitstream and bitres_bitstream */
  BsBitBuffer  *bitres_bitstream = NULL;            /* bits after bitres size ptr */

  double *p_ratio[MAX_TIME_CHANNELS], allowed_distortion[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  static double p_ratio_long[2][MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  static double p_ratio_short[2][MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int    nr_of_sfb[MAX_TIME_CHANNELS], sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS];
  int sfb_offset_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1];
  static enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS];

  int   win, no_sub_win, sub_win_size, bw_lines;
  
  /* structures holding the output of the psychoacoustic model */
  CH_PSYCH_OUTPUT chpo_long[MAX_TIME_CHANNELS];
  CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS];
  static int psychNum=0;
  psychNum = !psychNum;      /* Toggle psychNum, psycho model using look-ahead buffer */

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
  for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
    quantInfo[chanNum].gcBitBuf = BsAllocBuffer(4096);   /* Allocate bit buffer in quantInfo */
    gc_stream[chanNum] = BsOpenBufferWrite(quantInfo[chanNum].gcBitBuf);  /* Use bit buffer in quantInfo */
  }

  { /* convert float input to double, which is the internal format */
    /* store input data in look ahead buffer which may be necessary for the window switching decision */
   int i = 0;
   int chanNum;
   
   for (chanNum=0;chanNum<max_ch;chanNum++) {
     for( i=0; i<block_size_samples; i++ ) {
#ifdef MPEG4V1
         /* temporary fix: a linear buffer for LTP containing the whole time frame */
         nok_tmp_DTimeSigBuf[chanNum][i] = DTimeSigBuf[chanNum][i];
         nok_tmp_DTimeSigBuf[chanNum][block_size_samples + i] = DTimeSigLookAheadBuf[chanNum][i];
#endif
       /* last frame input data are encoded now */
       DTimeSigBuf[chanNum][i]          = DTimeSigLookAheadBuf[chanNum][i];

       /* YT 970615 for son_gc */
       if (pp_select == AAC_PP) {
         DTimeSigLookAheadBuf[chanNum][i] =  
	   (i < PQFDELAY) ? DTimeSigPQFDelayCompensationBuf[chanNum][i] : (double)p_time_signal[chanNum][i-PQFDELAY] ;
         DTimeSigLookAheadBufForGC[chanNum][i] = 
	   (double)p_time_signal[chanNum][i];
       }

       else { 
         DTimeSigLookAheadBuf[chanNum][i] = (double)p_time_signal[chanNum][i];
       } /* end if(pp_select ...) */
     } /* end for(i ..) */
   } /* end for(chanNum ... ) */

   if (cplenabl)
   {
	  lfePresent ? (nfbwchans = max_ch - 1) : (nfbwchans = max_ch);
	  for (chanNum = 0; chanNum < nfbwchans; chanNum++)
	  {
	 	 cplmix[chanNum] = icplmix[chanNum];
	  }
	  couple_e(cplmix, nfbwchans, cplSampleBuf);				/* independent coupling */

	  for( i=0; i<block_size_samples; i++ ) 
	  {
        /* last frame input data are encoded now */
        DTimeSigBuf[max_ch][i]          = DTimeSigLookAheadBuf[max_ch][i];

        /* YT 970615 for son_gc */
        if (pp_select == AAC_PP) {
         DTimeSigLookAheadBuf[max_ch][i] =  
	   /*	    (i < PQFDELAY) ? DTimeSigPQFDelayCompensationBuf[max_ch][i] : (double)p_time_signal[max_ch][i-PQFDELAY] ; */
	    (i < PQFDELAY) ? DTimeSigPQFDelayCompensationBuf[max_ch][i] : (double)cplSampleBuf[i-PQFDELAY] ;
          DTimeSigLookAheadBufForGC[max_ch][i] = (double)cplSampleBuf[i];
        }

        else { 
          DTimeSigLookAheadBuf[max_ch][i] = (double)cplSampleBuf[i];
        } /* end if(pp_select ...) */
	 }
   }
	
  /* Keep track of number of bits used */
   used_bits = 0;

	/* YT 960615 for son_pp */
   if (pp_select == AAC_PP) {
     for (chanNum=0;chanNum<max_ch;chanNum++) {
       for (i = 0; i < PQFDELAY; i++) {
	 DTimeSigPQFDelayCompensationBuf[chanNum][i] = 
	   (double)p_time_signal[chanNum][block_size_samples-PQFDELAY+i];
       }
     }
     if (cplenabl) {
	 DTimeSigPQFDelayCompensationBuf[max_ch][i] = 
	   (double)cplSampleBuf[block_size_samples-PQFDELAY+i];
     }       
   } 
  }

   /***********************************************************************/
   /* Determine channel elements      */
   /***********************************************************************/
   DetermineChInfo(channelInfo, nchans, cplenabl, lfePresent);


#ifdef CT_SBR
  /***********************************************************************/
  /* SBR encoding */
  /***********************************************************************/
  if (p_time_signal_orig != NULL) {
    for (chanNum=0;chanNum<max_ch;chanNum++) {
      if (channelInfo[chanNum].present) {
        /* single_channel_element */
        if (!channelInfo[chanNum].cpe) {
          if (!channelInfo[chanNum].lfe) {
            sbr_bits = SbrEncodeSingleChannel(quantInfo[chanNum].sbrChan,
                                              p_time_signal_orig[chanNum]
#ifdef PARAMETRICSTEREO
                                              , sbrpsenc ? p_time_signal_orig[chanNum+1] : NULL
#endif
                                              );
          } 
        } else {
          if (channelInfo[chanNum].ch_is_left) {
            /* Write out cpe */
            sbr_bits = 
              SbrEncodeChannelPair(quantInfo[chanNum].sbrChan,     /* Left */
                quantInfo[channelInfo[chanNum].paired_ch].sbrChan, /* Right */
                p_time_signal_orig[chanNum],                       /* Left */
                p_time_signal_orig[channelInfo[chanNum].paired_ch] /* Right */);
          }
        }  /* if (!channelInfo[chanNum].cpe)  else */
      } /* if (chann...*/
    } /* for (chanNum...*/
  }
#endif

  /******************************************************************************************************************************
  *
  * psychoacoustic
  *
  ******************************************************************************************************************************/

  {
    int chanNum;
    for (chanNum=0;chanNum<nchans;chanNum++) {
      EncTf_psycho_acoustic( sampling_rate, chanNum, &DTimeSigLookAheadBuf[chanNum], &next_desired_block_type[chanNum],
			     (int)qc_select,block_size_samples ,&chpo_long[chanNum], &chpo_short[chanNum] );

      if (channelInfo[chanNum].lfe) {        /* dummy psychoacoustic for LFE */
        int sfb;
        for (sfb=0;sfb<4;sfb++) { 
          chpo_long[chanNum].p_ratio[sfb]=0.004; /* -24 dB */
        } 
        next_desired_block_type[chanNum] = ONLY_LONG_WINDOW;
      }
    }
  }

  /******************************************************************************************************************************
  *
  * block_switch processing 
  *
  ******************************************************************************************************************************/

  {
    int chanNum;
    for (chanNum=0;chanNum<nchans;chanNum++) {

      if (channelInfo[chanNum].lfe) {
	if (debugLevel>2)
	  printf("WSM: ONLY_LONG_WINDOW\n");
	block_type[chanNum] = ONLY_LONG_WINDOW;   /* For LFE only long windows */
      } else { 
        switch( win_switch_mode ) {
        case STATIC_LONG:
          if (debugLevel>2)
	    printf("WSM: ONLY_LONG_WINDOW\n");
  	  block_type[chanNum] = ONLY_LONG_WINDOW;
 	  break;
        case STATIC_SHORT :
	  if (debugLevel>2)
	    printf("WSM:ONLY_SHORT_WINDOW\n");
	  block_type[chanNum] = ONLY_SHORT_WINDOW;
	  break;
        case LS_STARTSTOP_SEQUENCE :
	  if (debugLevel>2)
	    printf("WSM: LS_STARTSTOP_SEQUENCE\n");
	  {
	    static enum WINDOW_TYPE long_short_seq[3] = 
            { ONLY_LONG_WINDOW, LONG_SHORT_WINDOW, SHORT_LONG_WINDOW };
	    static int ls_seq_no[MAX_TIME_CHANNELS];
	  
	    block_type[chanNum] = long_short_seq[ls_seq_no[chanNum]++];
	    if( ls_seq_no[chanNum] == 3 ) ls_seq_no[chanNum] = 0;
	  }
	  break;
        case LONG_SHORT_SEQUENCE :
	  if (debugLevel>2)
	    printf("WSM: LONG_SHORT_SEQUENCE\n");
	  {
	    static enum WINDOW_TYPE long_short_seq[4] = 
            { ONLY_LONG_WINDOW, LONG_SHORT_WINDOW, ONLY_SHORT_WINDOW, SHORT_LONG_WINDOW };
	    static int ls_seq_no[MAX_TIME_CHANNELS];

	    block_type[chanNum] = long_short_seq[ls_seq_no[chanNum]++];
	    if( ls_seq_no[chanNum] == 4 ) ls_seq_no[chanNum] = 0;
	  }
	  break;
        case FFT_PE_WINDOW_SWITCHING :
	  {
	  
	    /* A few definitions:                                                      */
	    /*   block_type:  Initially, the block_type used in the previous frame.    */
	    /*                Will be set to the block_type to use this frame.         */
	    /*                A block type will be selected to ensure a meaningful     */
	    /*                window transition.                                       */
	    /*   next_desired_block_type:  Block_type (LONG or SHORT) which the psycho */
	    /*                model wants to use next frame.  The psycho model is      */
	    /*                using a look-ahead buffer.                               */
	    /*   desired_block_type:  Block_type (LONG or SHORT) which the psycho      */
  	    /*                previously wanted to use.  It is the desired block_type  */
	    /*                for this frame.                                          */
	    if ( (block_type[chanNum]==ONLY_SHORT_WINDOW)||(block_type[chanNum]==LONG_SHORT_WINDOW) ) {
	      if ( (desired_block_type[chanNum]==ONLY_LONG_WINDOW)&&(next_desired_block_type[chanNum]==ONLY_LONG_WINDOW) ) {
	        block_type[chanNum]=SHORT_LONG_WINDOW;
	      } else {
	        block_type[chanNum]=ONLY_SHORT_WINDOW;
	      }
	    } else if (next_desired_block_type[chanNum]==ONLY_SHORT_WINDOW) {
	    block_type[chanNum]=LONG_SHORT_WINDOW;
	    } else {
	    block_type[chanNum]=ONLY_LONG_WINDOW;
	    }
	    desired_block_type[chanNum]=next_desired_block_type[chanNum];

	  }
	  break;

        default:
	  CommonExit( 1, "Encode: unsupported window switching mode %d", win_switch_mode );
        }
      }
    }

    {
      int chanNum;
      /*      for (chanNum=0;chanNum<max_ch;chanNum++) {*/
      for (chanNum=0;chanNum<nchans;chanNum++) {
	switch( block_type[chanNum] ) {
	case ONLY_SHORT_WINDOW  :

	  /*NOTE:  following 3 vars are either dead or NYI.*/
	  sub_win_size = block_size_samples/short_win_in_long;
	  bw_lines     = bw_lines_short;
	  no_sub_win   = short_win_in_long;

#ifndef CT_SBR
	  quantInfo[chanNum].max_sfb = 12;           /* Currently not fs dependent (should be) */
#else
          if (nchans==1)
            quantInfo[chanNum].max_sfb = 11;
          else
            quantInfo[chanNum].max_sfb = 13;
#endif
	  quantInfo[chanNum].num_window_groups=4;
	  quantInfo[chanNum].window_group_length[0] = 1;
	  quantInfo[chanNum].window_group_length[1] = 2;
	  quantInfo[chanNum].window_group_length[2] = 3;
	  quantInfo[chanNum].window_group_length[3] = 2;
	  break;
      
	default:

	  /*NOTE:  following 3 vars are either dead or NYI.*/
	  sub_win_size = block_size_samples;
	  bw_lines     = bw_lines_long;
	  no_sub_win   = 1;

#ifndef CT_SBR
	  quantInfo[chanNum].max_sfb = 40;          
#else
          if (nchans==1)
            quantInfo[chanNum].max_sfb = 37;          
          else
            quantInfo[chanNum].max_sfb = 42;          
#endif
	  quantInfo[chanNum].num_window_groups = 1;
	  quantInfo[chanNum].window_group_length[0]=1;
	  break;
	} /* end case */
	} /* end for loop */

      /*	  quantInfo[cplidx].max_sfb = 40;
      	  quantInfo[cplidx].num_window_groups = 1;
	  quantInfo[cplidx].window_group_length[0]=1;*/
    }


  /******************************************************************************************************************************
  *
  * pre processing 
  *
  ******************************************************************************************************************************/

#ifdef SON_AAC_PP
    {
    int chanNum;
    for (chanNum=0;chanNum<nchans;chanNum++) {

      /* son_AACpp */
      if (pp_select == AAC_PP)
	{
	  int band;
	  if(debugLevel>2) printf("pp_aac selected Gain Controled\n");	
	  /* polyphase quadrature filter */
	  /* son_pqf_main(DTimeSigLookAheadBufForGC[chanNum], block_size_samples, MONO_CHAN, DBandSigLookAheadBuf[chanNum]);*/
	  son_pqf_main(DTimeSigLookAheadBufForGC[chanNum], block_size_samples, chanNum, DBandSigLookAheadBuf[chanNum]);

	  /* set analysis buffer for gain control */
	  for (band = 0; band < NBANDS; band++) {
	    memcpy((char *)&DBandSigBufForGCAnalysis[chanNum][band][block_size_samples/NBANDS*2],
		   (char *)DBandSigLookAheadBuf[chanNum][band],
		   block_size_samples/NBANDS*sizeof(double)
		   );

	  }

	if(gc_switch == GC_PRESENT){

	  /* gain detector */
	  son_gc_detect(DBandSigBufForGCAnalysis[chanNum], block_size_samples, block_type[chanNum], chanNum, gainInfo[chanNum]);
	}
	else /* if GC_NON_PRESENT */
	{
	 /* reset detect if no gain control */
     son_gc_detect_reset(DBandSigBufForGCAnalysis[chanNum], block_size_samples, block_type[chanNum], chanNum, gainInfo[chanNum]);
	}


	  /* gain modifier */
	  son_gc_modifier(DBandSigBufForGCAnalysis[chanNum],
			  gainInfo[chanNum],
			  block_size_samples,
			  block_type[chanNum], 
			  chanNum,
			  DBandSigWithGCBuf[chanNum]
			  );

	  /* shift analysis buffer */
	  for (band = 0; band < NBANDS; band++) {
	    memcpy((char *)DBandSigBufForGCAnalysis[chanNum][band],
		   (char *)&DBandSigBufForGCAnalysis[chanNum][band][block_size_samples/NBANDS],
		   block_size_samples/NBANDS*2*sizeof(double)
		   );
	  }

		if(gc_switch == GC_PRESENT){
	  	/* packing gain data into the gc_stream */
	  	son_gc_pack(gc_stream[chanNum], block_type[chanNum],
		      max_band[chanNum], gainInfo[chanNum]);
		}
	}

      /* Count number of bits used for gain_control_data */
      /*      used_bits += WriteGainControlData(&quantInfo[chanNum], */    /* quantInfo contains packed gain control data */
      WriteGainControlData(&quantInfo[chanNum],     /* quantInfo contains packed gain control data */
					NULL,           /* NULL BsBitStream.  Only counting bits, no need to write yet */
					0);             /* Zero write flag means don't write */
    }
    }
#else   /* Gain control calls bypassed */
      BsPutBit(gc_stream[chanNum],0,LEN_GAIN_PRES); /* switches off the gain control */
      /*used_bits += LEN_GAIN_PRES;*/
      printf("PP_AAC not defined \n");	
#endif
  }
    


  /******************************************************************************************************************************
  *
  * T/F mapping
  *
  ******************************************************************************************************************************/

  {
    /* son_AACpp */
    if (pp_select == AAC_PP)
      {
	int i, band;
	double tmp;
	int chanNum;
	for (chanNum=0;chanNum<nchans;chanNum++) {
	  for (band = 0; band < NBANDS; band++) {
	    buffer2freq(
			DBandSigWithGCBuf[chanNum][band],
			&spectral_line_vector_for_gc[chanNum][block_size_samples/NBANDS*band],
			overlap_buffer[chanNum],
			block_type[chanNum],
			WS_FHG ,
			block_size_samples/NBANDS, 
			block_size_samples/NBANDS/short_win_in_long,	 
			NON_OVERLAPPED
			);

	  } /* for(band=...) */


	  son_gc_arrangeSpecEnc(spectral_line_vector_for_gc[chanNum],
				block_size_samples,block_type[chanNum],
				spectral_line_vector[chanNum]
				);
	}
      } /* if (pp_select == AAC_PP) */
    else
      {
	int chanNum;
	for (chanNum=0;chanNum<nchans;chanNum++) {
	  buffer2freq(
		      DTimeSigBuf[chanNum],
		      spectral_line_vector[chanNum],
		      overlap_buffer[chanNum],
		      block_type[chanNum],
		      WS_FHG,
		      block_size_samples,
		      block_size_samples/short_win_in_long,
		      OVERLAPPED
		      );
	}
    
      }
  }


  /******************************************************************************************************************************
  *
  * adapt ratios of psychoacoustic module to codec scale factor bands
  *
  ******************************************************************************************************************************/
    {
      int chanNum;
      for (chanNum=0;chanNum<nchans;chanNum++) {

	/* Save p_ratio from psychoacoustic model for next frame.  */
	/* Psycho model is using a look-ahead window for block switching */

	memcpy( (char*)p_ratio_long[!psychNum][chanNum], (char*)chpo_long[chanNum].p_ratio, (NSFB_LONG)*sizeof(double) );
	memcpy( (char*)p_ratio_short[!psychNum][chanNum],(char*)chpo_short[chanNum][0].p_ratio,(MAX_SHORT_WINDOWS*NSFB_SHORT)*sizeof(double) );
      }
    }

  {
    int    sfb, i, sl, par_set;
    double energy, dtmp;
    int chanNum;
    for (chanNum=0;chanNum<nchans;chanNum++) {
      par_set = 0;
      switch( block_type[chanNum] ) {
      case ONLY_LONG_WINDOW :
	memcpy( (char*)sfb_width_table[chanNum], (char*)chpo_long[chanNum].cb_width, (NSFB_LONG+1)*sizeof(int) );
	nr_of_sfb[chanNum] = chpo_long[chanNum].no_of_cb;
	p_ratio[chanNum]   = p_ratio_long[psychNum][chanNum];
	par_set = 1;
	break;
      case LONG_SHORT_WINDOW :
	memcpy( (char*)sfb_width_table[chanNum], (char*)chpo_long[chanNum].cb_width, (NSFB_LONG+1)*sizeof(int) );
	nr_of_sfb[chanNum] = chpo_long[chanNum].no_of_cb;
	p_ratio[chanNum]   = p_ratio_long[psychNum][chanNum];
	par_set = 1;
	break;
      case ONLY_SHORT_WINDOW :
	memcpy( (char*)sfb_width_table[chanNum], (char*)chpo_short[chanNum][0].cb_width, (NSFB_SHORT+1)*sizeof(int) );
	nr_of_sfb[chanNum] = chpo_short[chanNum][0].no_of_cb;
	p_ratio[chanNum]   = p_ratio_short[psychNum][chanNum];
	par_set = 1;
	break;
      case SHORT_LONG_WINDOW :
	memcpy( (char*)sfb_width_table[chanNum], (char*)chpo_long[chanNum].cb_width, (NSFB_LONG+1)*sizeof(int) );
	nr_of_sfb[chanNum] = chpo_long[chanNum].no_of_cb;
	p_ratio[chanNum]   = p_ratio_long[psychNum][chanNum];
	par_set = 1;
	break;
      default:
	CommonWarning( "Encode: unsupported block type in adapt ratios: %d", block_type[chanNum] );
	break;
      }

      /* to prevent undefined ptr just use a meaningful value to allow for experiments with the meachanism above */
      if( !par_set) {   
	memcpy( (char*)sfb_width_table[chanNum], (char*)chpo_long[chanNum].cb_width, (NSFB_LONG+1)*sizeof(int) );
	nr_of_sfb[chanNum]  = chpo_long[chanNum].no_of_cb;
	p_ratio[chanNum]   = p_ratio_long[psychNum][chanNum];
      }
    }
  }

    {
      int chanNum;   
      for (chanNum=0;chanNum<nchans;chanNum++) {
	/* Construct sf band offset table */
	int offset=0;
	int sfb;
	for (sfb=0;sfb<nr_of_sfb[chanNum];sfb++) {
	  sfb_offset_table[chanNum][sfb] = offset;
	  offset+=sfb_width_table[chanNum][sfb];
	}
	sfb_offset_table[chanNum][nr_of_sfb[chanNum]]=offset;
      }
    }


  /************************************************************************
   Set upper spectral coefficients to zero for LFE
   ************************************************************************/
  {
    int chanNum;
    for (chanNum=0;chanNum<max_ch;chanNum++) { 
      if (channelInfo[chanNum].lfe) {           
        int i;    
        for (i=12 /*13*/;i<sfb_offset_table[chanNum][nr_of_sfb[chanNum]];i++)      
          spectral_line_vector[chanNum][i] = 0;
      }
    }               
  }


  /******************************************************************************************************************************
  *
  * quantization and coding
  *
  ******************************************************************************************************************************/
  { 
    int padding_limit = max_bitreservoir_bits;    /* f( max__bitreservoir_bits ) */
    int maxNumBitsByteAligned;
    int chanNum;   
    int numWindows;
    int windowLength;
    int j,w;
    int bandNumber;
    int numFillBits;
    int bitsLeftAfterFill;
    int cpl_bits = 0;

    /* bit budget */
    num_bits_available = (long)(average_bits + available_bitreservoir_bits - used_bits);
    
    /* find the largest byte-aligned section with fewer bits than num_bits_available */
    maxNumBitsByteAligned = ((num_bits_available >> 3) << 3);

    /* Compute how many reservoir bits can be used and still be able to byte */
    /* align without exceeding num_bits_available, and have room for an ID_END marker   */
    available_bitreservoir_bits = maxNumBitsByteAligned - LEN_SE_ID - average_bits;

    for (chanNum=0;chanNum<nchans;chanNum++) {

	  /* Set window shape paremeter in quantInfo */
      if (channelInfo[chanNum].lfe) {
        quantInfo[chanNum].window_shape = WS_FHG;   /* for LFE window_shape must be sine */
      } else {
        quantInfo[chanNum].window_shape = WS_FHG;   /* other window_shapes can be selected */
      }

      /* Compute energy in each scalefactor band of each window */
      numWindows = (block_type[chanNum]==ONLY_SHORT_WINDOW) ?
	short_win_in_long : 1;
      windowLength = block_size_samples/numWindows;
      bandNumber=0;
      for (w=0;w<numWindows;w++) {
	int offset=0;
	int sfb;
	j = w*windowLength;

	/* Only compute energy up to max_sfb */
	for(sfb=0; sfb< quantInfo[chanNum].max_sfb; sfb++ ) {
	  /* calculate scale factor band energy */
	  int width,i;
	  energy[chanNum][bandNumber] = 0.0;
	  width=sfb_width_table[chanNum][sfb];
	  for(i=offset; i<(offset+width); i++ ) {
	    double dtmp = spectral_line_vector[chanNum][j++];
	    energy[chanNum][bandNumber] += dtmp*dtmp;
	  }
	  bandNumber++;
	  offset+=width;
	}  
      }
    }  /* for (chanNum... */

    /******************************************/
    /* Perform Coupling Coordinate calculation*/
    /******************************************/
 
	if (cplenabl)
	{
		couple_init(&cvars, nfbwchans, quantInfo, channelInfo);
	}

    /******************************************/
    /* Perform TNS analysis and filtering     */
    /******************************************/
    for (chanNum=0;chanNum<nchans;chanNum++) {
      if ((!channelInfo[chanNum].lfe)&&(tnsenable)) {
        TnsEncode(nr_of_sfb[chanNum],            /* Number of bands per window */
	          quantInfo[chanNum].max_sfb,              /* max_sfb */
                  block_type[chanNum],
                  sfb_offset_table[chanNum],
                  spectral_line_vector[chanNum],
                  &tnsInfo[chanNum]);
      } else {
        tnsInfo[chanNum].tnsDataPresent=0;      /* TNS not used for LFE */
      }
    }

    /*******************************************************************************/
    /* If LTP prediction is used, compute LTP predictor info and residual spectrum */
    /*******************************************************************************/
#ifdef MPEG4V1
    if(profile == LTP)
    {
      TNS_INFO *tnsInfo_for_LTP;

      for(chanNum=0;chanNum<max_ch;chanNum++)
      {
	if((tnsInfo[chanNum].tnsDataPresent != 0) && tnsenable)
	  tnsInfo_for_LTP = &tnsInfo[chanNum];
	else
	  tnsInfo_for_LTP = NULL;
	  
	if(channelInfo[chanNum].present && (!channelInfo[chanNum].lfe) &&
	   block_type[chanNum] != ONLY_SHORT_WINDOW) 
	{
	  nok_ltp_enc(spectral_line_vector[chanNum], 
		      nok_tmp_DTimeSigBuf[chanNum], 
		      block_type[chanNum],
		      WS_FHG, WS_FHG,
		      block_size_samples,
		      block_size_samples/short_win_in_long, 
		      &sfb_offset_table[chanNum][0], 
		      nr_of_sfb[chanNum],
		      &nok_lt_status[chanNum],
		      tnsInfo_for_LTP);
	} /* if(channelInfo[chanNum].present... */
	else
	  quantInfo[chanNum].ltpInfo->global_pred_flag = 0;
      } /* for(chanNum... */
    }
#endif

    /******************************************/
    /* Apply Intensity Stereo                 */
    /******************************************/
#ifdef MPEG4V1
    if ( pns_sfb_start > PNS_MAX_START )  /* do intensity only if pns is off  */
#endif
      {
        ISEncode(spectral_line_vector,
                 channelInfo,
                 sfb_offset_table,
                 block_type,
                 quantInfo,
                 max_ch,
		 cplenabl,
		 isenable);
      }

    /***********************************************************************/
    /* If prediction is used, compute predictor info and residual spectrum */
    /***********************************************************************/
    for (chanNum=0;chanNum<nchans;chanNum++) {      
      if ((qc_select == AAC_PRED)&&(!channelInfo[chanNum].lfe)) {  /* no prediction for LFE channel*/      
	int numPredBands = min(max_pred_sfb,nr_of_sfb[chanNum]);
	PredCalcPrediction( spectral_line_vector[chanNum], 
			   reconstructed_spectrum[chanNum], 
			   (int)block_type[chanNum], 
			   numPredBands,
			   sfb_width_table[chanNum], 
			   /* Added by CL */
			   quantInfo,
			   channelInfo,
			   chanNum); 
      } else {
	quantInfo[chanNum].pred_global_flag = 0;
      }
    } /* for(chanNum... */

    /******************************************/
    /* Apply MS stereo                        */
    /******************************************/
    MSEncode(spectral_line_vector,
	     channelInfo,
	     sfb_offset_table,
	     block_type,
	     quantInfo,
	     max_ch,
	     msenable);

#if 0
   /* Call the AAC quantization and coding module. */
   tf_encode_spectrum_aac( spectral_line_vector,
			   p_ratio,
			   allowed_distortion,
			   energy,
			   block_type,
			   sfb_width_table,
			   nr_of_sfb,
			   (int)(average_bits - used_bits)/2,
			   available_bitreservoir_bits,
			   padding_limit,
			   fixed_stream, /*tmp_stream,*/
			   var_stream,
			   1,                        /* nr of audio channels */
			   reconstructed_spectrum,
			   useShortWindows,
			   aacAllowScalefacs,
			   &quantInfo[MONO_CHAN],
			   );
#endif

   /************************************************/
   /* Set the dynamic range parameters             */
   /************************************************/


	if (dvars_inpt->dynrng_enabl)
	{
		drcbits = setDynRngVars(dvars_inpt, block_type, nchans);
	}
	else
	{
		drcbits = 0;
	}

   /************************************************/
   /* Call the AAC quantization and coding module. */
   /************************************************/
   for (chanNum=0;chanNum<nchans;chanNum++) {

     int bitsToUse; 
     if (!lfePresent) {
#ifndef CT_SBR
       bitsToUse = (int)((average_bits - used_bits)/nchans);
#else
       bitsToUse = (int)((average_bits - (used_bits+sbr_bits))/nchans);
#endif
       bitsToUse += (int)(0.2*available_bitreservoir_bits/nchans);
     } else {
       /* Calculation of reduced bitrate for LFE */
       double lfeBitRatio = 0.14;       /* ratio of LFE bits to bits of one SCE */
       int lfeBits = max(200,(int)((average_bits - used_bits) * lfeBitRatio / (nchans - 1))); /* number of bits for LFE */ 
       
       if (channelInfo[chanNum].lfe) {
         bitsToUse = lfeBits;
         bitsToUse += (int)(0.2*available_bitreservoir_bits 
                            * lfeBitRatio / (nchans - 1));
       } else {
         bitsToUse = (int)((average_bits - used_bits - lfeBits) / (nchans - 1));
         bitsToUse += (int)(0.2*available_bitreservoir_bits 
                            * (1 - lfeBitRatio / (nchans - 1)) / (nchans - 1));
       } /* if (channelInfo[chanNum].lfe) */
     } /* if (!lfePresent) */  

	if (chanNum == max_ch)
	{
		cpl_bits = calc_cplbits(nfbwchans, cplmix, cplenabl, channelInfo, &cvars);
		bitsToUse -= cpl_bits;
	}

	if (chanNum == 0)	   /* subtract off bits used for dynamic range control */
	{
		bitsToUse -= drcbits;		
	}

#ifdef CT_SBR
     /* for AAC+SBR restrict number of used bits per channel to a maximum
        of 3072 */
     if (quantInfo[chanNum].sbrChan) {
       bitsToUse = min(bitsToUse,
                       3072-(used_bits+sbr_bits+cpl_bits+drcbits)/nchans);
     }
#endif

     tf_encode_spectrum_aac( &spectral_line_vector[chanNum],
			     &p_ratio[chanNum],
			     &allowed_distortion[chanNum],
			     &energy[chanNum],
			     &block_type[chanNum],
			     &sfb_width_table[chanNum],
			     &nr_of_sfb[chanNum],
			     bitsToUse,
			     padding_limit,
			     fixed_stream, /*tmp_stream,*/
			     var_stream,
			     1,                        /* nr of audio channels */
			     &reconstructed_spectrum[chanNum],
			     useShortWindows,
			     aacAllowScalefacs,
			     &quantInfo[chanNum],
			     &(channelInfo[chanNum])
				 );
   }

   /**********************************************************/
   /* Reconstruct MS Stereo bands for prediction             */
   /**********************************************************/
   MSReconstruct(reconstructed_spectrum,
		 channelInfo,
		 sfb_offset_table,
		 block_type,
		 quantInfo,
		 max_ch);

   /**********************************************************/
    /* Reconstruct Intensity Stereo bands for prediction     */
   /**********************************************************/
   ISReconstruct(reconstructed_spectrum,
		 channelInfo,
		 sfb_offset_table,
		 block_type,
		 quantInfo,
		 max_ch);

   /**********************************************************/
   /* Add the predicted spectrum to the so far recontructed  */
   /* spectrum, then TNS decode (if needed) and finally      */
   /* update LTP history buffer                              */
   /**********************************************************/
#ifdef MPEG4V1
   if(profile == LTP)
   {
     TNS_INFO *tnsDecInfo;

     for (chanNum=0;chanNum<max_ch;chanNum++) 
     {
       if((tnsInfo[chanNum].tnsDataPresent != 0) && tnsenable)
	 tnsDecInfo = &tnsInfo[chanNum];
       else
	 tnsDecInfo = NULL;
       
       if (!channelInfo[chanNum].lfe) {  /* no reconstruction needed for LFE channel*/

	 nok_ltp_reconstruct(reconstructed_spectrum[chanNum], 
			     block_type[chanNum],
			     &sfb_offset_table[chanNum][0], 
			     nr_of_sfb[chanNum],
			     &nok_lt_status[chanNum]);

	 if(tnsDecInfo != NULL)
	   TnsDecodeFilterOnly(nr_of_sfb[chanNum], quantInfo[chanNum].max_sfb, 
			       block_type[chanNum], &sfb_offset_table[chanNum][0], 
			       reconstructed_spectrum[chanNum], tnsDecInfo);

	 freq2buffer(reconstructed_spectrum[chanNum], time_buffer[chanNum], 
		     ltp_overlap_buffer[chanNum], block_type[chanNum], 
		     block_size_samples,
		     block_size_samples/short_win_in_long, WS_FHG, OVERLAPPED);

	 nok_ltp_update(&nok_lt_status[chanNum], time_buffer[chanNum],
			ltp_overlap_buffer[chanNum], block_size_samples);
       }
     }
   }
#endif

   /* If short window, reconstruction not needed for prediction */
   for (chanNum=0;chanNum<max_ch;chanNum++) {
     if ((block_type[chanNum]==ONLY_SHORT_WINDOW)) {
       int sind;
       for (sind=0;sind<1024;sind++) {
	 reconstructed_spectrum[chanNum][sind]=0.0;
       }
     }
   }

   /**********************************/
   /* Write out all encoded channels */
   /**********************************/
   { 
     for (chanNum=0;chanNum<max_ch;chanNum++) {
       if (channelInfo[chanNum].present) {
	 /* Write out a single_channel_element */
	 if (!channelInfo[chanNum].cpe) {
           if (channelInfo[chanNum].lfe) {
    	     /* Write out lfe */ 
	     used_bits += WriteLFE(&quantInfo[chanNum],   /* Quantization information */
				  channelInfo[chanNum].tag,
				  fixed_stream,           /* Bitstream */
				  1);                     /* Write flag, 1 means write */
           } else {
             /* Write out sce */ /* BugFix by YT  '+=' sould be '=' */
	     used_bits += WriteSCE(&quantInfo[chanNum],   /* Quantization information */
				  channelInfo[chanNum].tag,
				  fixed_stream,           /* Bitstream */
				  1);                     /* Write flag, 1 means write */

#ifdef CT_SBR
             if (quantInfo[chanNum].sbrChan)
               used_bits += WriteSbrSCE(quantInfo[chanNum].sbrChan,
                                        fixed_stream);
             
#endif
           } /* if (channelInfo[chanNum].lfe ... else */
	 } else {
	   if (channelInfo[chanNum].ch_is_left) {
	     /* Write out cpe */
	     used_bits += WriteCPE(&quantInfo[chanNum],   /* Quantization information,left */
				  &quantInfo[channelInfo[chanNum].paired_ch],   /* Right */
				  channelInfo[chanNum].tag,
				  channelInfo[chanNum].common_window,    /* common window */
				  &(channelInfo[chanNum].ms_info),
				  fixed_stream,           /* Bitstream */
				  1);                     /* Write flag, 1 means write */

#ifdef CT_SBR
             if (quantInfo[chanNum].sbrChan)
               used_bits += WriteSbrCPE(quantInfo[chanNum].sbrChan,
                                        quantInfo[channelInfo[chanNum].paired_ch].sbrChan,
                                        fixed_stream);

#endif
	   }
	 }  /* if (!channelInfo[chanNum].cpe)  else */
       } /* if (chann...*/
     } /* for (chanNum...*/
   }

	if (cplenabl)
	{
		used_bits += WriteCplData (fixed_stream,		/* in: bit stream */
								   &cvars,				/* coupling params */
								   &quantInfo[max_ch],	/* quantization params */
								   max_ch);				/* number of channels */
	}

	if (dvars_inpt->dynrng_enabl)
	{
		used_bits += WriteDRCArray (fixed_stream, nchans);
	}

	/* Compute how many fill bits are needed to avoid overflowing bit reservoir */
   /* Save room for ID_END terminator */
   if (used_bits < (minBitsNoOverflow - LEN_SE_ID) ) {
     numFillBits = minBitsNoOverflow - LEN_SE_ID - used_bits;
   } else {
     numFillBits = 0;
   }
   
   /* Write AAC fill_elements, smallest fill element is 7 bits. */
   /* Function may leave up to 6 bits left after fill, so tell it to fill a few extra */

   numFillBits += 6;
   bitsLeftAfterFill=WriteAACFillBits(fixed_stream,numFillBits);
   used_bits += (numFillBits - bitsLeftAfterFill);

   /* Write ID_END terminator */
   BsPutBit(fixed_stream,ID_END,LEN_SE_ID);
   used_bits += LEN_SE_ID;
   
   /* Now byte align the bitstream */
   used_bits += ByteAlign(fixed_stream);
   BsClose(fixed_stream);
   if (var_stream != NULL)
     BsClose(var_stream);
   BsClose(tmp_stream);
   BsFreeBuffer(tmpBitBuf);
   for (chanNum=0;chanNum<MAX_TIME_CHANNELS;chanNum++) {
     BsClose(gc_stream[chanNum]);
     BsFreeBuffer(quantInfo[chanNum].gcBitBuf);
   }
  } /* Quantization and coding block */

}


void couple_e(DSPfract *cplmix, DSPshort nfchans, float *cpltc)
{
	int bin, chan;
	double invcplmix;

	if ((cplmix[0] > 0.01) || (cplmix[0] < -0.01))
	{
		for (bin = 0; bin < block_size_samples; bin++)
		{
			cpltc[bin] *= cplmix[0];
		}

		invcplmix = 1.0 / cplmix[0];
		for (chan = 1; chan < nfchans; chan++)
		{
			cplmix[chan] *= invcplmix;
		}
	}
	else
	{
		CommonExit(1, "Must not exceed 40dB attenuation in the left channel\n");
	}
}












