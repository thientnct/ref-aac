
/* $Id: dec_tf.c,v 1.68 1999/07/22 19:08:51 purnhage Exp $ */

/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) in the course of 
 development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
 Audio standards. Those intending to use this software module in hardware or 
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

/* CREATED BY :  Bernhard Grill -- June-96  */

/* fixed bit stream handling: Heiko Purnhagen, Uni Hannover, 960705 */
/* 21-aug-96   HP    added DecTfInfo(), DecTfFree() */
/*                   renamed to dec_tf.c, adapted to new dec.h */
/* 26-aug-96   HP   updated to vm0_tf_02a */
/* 28-aug-96   NI   integrated NTT's VQ coder. */
/* 09-feb-97   TK   integrated NBC compliant post processing module. */
/* 18-apr-97   NI   integrated scalable coder. */
/* 28-may-97   HP   -nttDecLyr help */
/* 25-aug-97   NI   bugfix */
/* 23-oct-97   HP   merged Nokia's predictor 971013 & 971020 */
/* 08-apr-98   HP   removed LDFB */

/*******************************************************************************************
 *
 * Master module for T/F based codecs
 *
 ******************************************************************************************/
#include <math.h>
#include <memory.h>
#include <string.h>
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
#include "bitstreamStruct.h"     /* structs */
#include "lpc_common.h"          /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "nok_prediction.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */


#include "allVariables.h"        /* variables */

#include "bitstream.h"
#include "common_m4a.h"
#include "dec_tf.h"
#include "flex_mux.h"
#include "mod_buf.h"
#include "tf_main.h"

/* son_NBCpp */
#include "sony_local.h"

/* NTT VQ */ 
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "mat_def_ntt.h"

/* Long term predictor */
#include "nok_lt_prediction.h"
#include "ntt_nok_lt_predict_dec.h"
#include "ntt_nok_pred.h"

/* ---  AAC --- */
#include "aac.h"


#include "buffers.h"
#include "statistics_aac.h"
#include "port.h" /* 981207 for tvq_tns */
TNS_frame_info tns_info[MAX_TIME_CHANNELS];


/* currently the psych module contains the sfb-width info 
   this should be moved into a separate file */
#include "psych.h"
#include "mp4_tf.h"
CH_PSYCH_OUTPUT chpo_long[MAX_TIME_CHANNELS];
CH_PSYCH_OUTPUT chpo_short[MAX_TIME_CHANNELS][MAX_SHORT_WINDOWS];
extern int samplFreqIndex[];	/* undesired, but ... */

static int     block_size_samples;      /* nr of samples per block in one! audio channel */
static int     med_win_in_long    = 2;  /* for NTT */
static int     short_win_in_long  = 8;

static int     max_ch;    /* no of of audio channels */

static double *spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS];
static double *time_sample_vector[MAX_TIME_CHANNELS];
static double *overlap_buffer[MAX_TIME_CHANNELS];

/* son_NBCpp */
static int    max_band[MAX_TIME_CHANNELS];
static double *spectral_line_vector_for_gc[MAX_TIME_CHANNELS];
static double *imdctBufForGC[MAX_TIME_CHANNELS];
static double **DBandSigBufForOverlapping[MAX_TIME_CHANNELS];
static double **DBandSigBuf[MAX_TIME_CHANNELS];
static GAINC  **gainInfo[MAX_TIME_CHANNELS];
static int    ssr_decoder_band = NBANDS;

static int frameNumBit;

static enum CH_CONFIG  channel_configuration = CHC_MONO; /* later f(encPara) */
static QC_MOD_SELECT qc_select = MDCT_VALUES_16BIT; /* later f(encPara) */
static enum TF_MOD_SELECT tf_select = MDCT_UER;          /* later f(encPara) */
static enum TRANSPORT_STREAM transport_stream =  LENINFO_TSTREAM;  /* later f(encPara) */
static int scalOutSelect = 100;
static int lopLong = 0, lopShort = 0;
static float normBw=0.0F;
/* Variable bit switch for the NTT_VQ coder */
static enum NTT_VARBIT ntt_varbit = NTT_VARBIT_OFF;
/* Number of scalable layers to be decoded */

static int ntt_NSclLayDec_gl;
static int ntt_NSclLay_gl;


/* AAC shorter windows 960-480-120 */
static int useShortWindows=0;

/* Predictor type */
static PRED_TYPE pred_type = MONOPRED;

/* Long term prediction */
static  NOK_LT_PRED_STATUS *nok_lt_status[MAX_TIME_CHANNELS];

/* Nokia LTP for TwinVQ */
/* static Info ntt_info; */
NOK_LT_PRED_STATUS *ntt_nok_lt_status[MAX_TIME_CHANNELS];

/********************************************************/
/* functions DecTfInfo() and DecTfFree()   HP 21-aug-96 */
/********************************************************/

#define PROGVER "t/f-based decoder core V5.0 20-nov-97"



/* DecTfInfo() */
/* Get info about t/f-based decoder core. */
char *DecTfInfo (FILE *helpStream)      /* in: print decPara help text to helpStream */
                                /*     if helpStream not NULL */
                                /* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
            PROGVER "\n"
            "decoder parameter string format:\n"
            "possible options:\n"
            "-qc_mdct16\n"
            "-qc_mdct16_960\n"
            "-qc_ntt\n"
            "-qc_aac_960\n"        
            "-qc_aac\n"
            "-aac_raw\n"
	    "     -ssr\n"
	    "     -main\n"
            "-aac_sca\n"
            "     -out <n>  select output layer for -aac_sca (n=0 :  base layer)\n"
            "     -lop <n long> <n short> <bandwidth> set lowpass line limits\n"
	    "-pred_ltp\n"
	    "-pred_nok_bwp\n"
	    "-nttDecLyr <n> (scalable, n=0: base layer)\n"
	    "\n");
  }

  return PROGVER;
}


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ( HANDLE_EP_INFO    hEpInfo,
                 HANDLE_RESILIENCE hResilience )
{
}


/*****************************************************************************************
 ***
 *** Function: DecTfInit
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
static Info** sfbInfo;
static char*  aacDebugString;
static long   sampling_rate_decoded;

void DecTfInit ( int                 numChannel,       /* in: num audio channels */
                 float               fSample,          /* in: sampling frequancy [Hz] */
                 float               bitRate,          /* in: bit rate [bit/sec] */
                 char*               decPara,          /* in: decoder parameter string */
                 int                 profile,          /* in: bitstream profile as indicated in the ADIF header */
                 BsBitBuffer*        bitHeader,        /* in: header from bit stream */
                 int*                frameNumSample,   /* out: num samples per frame */
                 int*                delayNumSample,   /* out: encoder delay (num samples) */
                 char*               aacDebugStr,
                 HANDLE_BUFFER*      hVm,
                 HANDLE_BUFFER*      hHcrSpecData,
                 FRAME_DATA*         frameData,
                 TF_DATA*            tfData,
                 ntt_DATA*           nttData,
                 int                 mainDebugLevel
                 )
{
  unsigned long ultmp;
  long          bit_rate_decoded = 0;
  int           delay_samples_decoded = 0;
  int           x; 
  int           i_ch;
  ntt_INDEX     ntt_index;
  ntt_INDEX     ntt_index_scl;
  int           ntt_NSclLayDec;
  int           ntt_NSclLay;
  int           ntt_IBPS;
  int           ntt_IBPS_SCL[8];
  int           ntt_LTP_SW;
  BsBitStream*  header_stream;
  /*   char * ssrPara; */
  WINDOW_SHAPE prev_windowShape[2] = {WS_FHG, WS_FHG}; 

#if 0
  if (numChannel != 1)
    CommonExit(1,"DecTfInit: audio data has more than one channel (%d)",
               numChannel);
#endif

  header_stream = BsOpenBufferRead(bitHeader);
 
  *hVm          = CreateBuffer(VIRTUELL);
  *hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);
   
  /* -----  Set qc_select ----- */
  if ( ( strstr(decPara, "-qc_aac")) ||
       ( strstr(decPara, "-qc_aac_960"))) {
    tf_select = MDCT_AAC;
    qc_select = AAC_QC;
  }


  /* set profile */
  mc_info.profile = profile;


  if( strstr( decPara, "-aac_raw" ) ) {

    if( mainDebugLevel ) {
      printf("AAC-");
      switch(profile) {
      case Main_Profile:
        printf("Main");
        break;
      case LC_Profile:
        printf("LC");
        break;
      case SSR_Profile:
        printf("SSR");
        break;
      default:
        printf("invalid");
        break;
      }
      printf(" profile decoding\n");
    }
    /* currently fix coded !!! */
    bit_rate_decoded = (long int )bitRate;
    sampling_rate_decoded = (long int)fSample;
    block_size_samples = 1024;
    delay_samples_decoded = 2048;
    channel_configuration = CHC_MONO;
    if (numChannel ==2){
      channel_configuration =CHC_DUAL;
    }
    qc_select = AAC_QC;
    tf_select = MDCT_AAC;
    transport_stream = AAC_RAWDATA_TSTREAM;
  }


  /*if( ( ssrPara = strstr( decPara, "-ssr" ) ) ) { */
  if( profile == SSR_Profile ) {
    /*	sscanf(&ssrPara[8],"%d",&ssr_decoder_band); currently not supported */
    if ( mainDebugLevel ) {
      printf(" AAC-SSR  %d band decoding\n",ssr_decoder_band);
    }
  }

  if( strstr( decPara, "-aac_sca" )  ) {
    char * outLayNr;
    char * lopPara;
    /* currently fix coded !!! */
    if ( mainDebugLevel ) {
      printf(" TF scaleable decoding OLD bitstream syntax\n");
    }
    bit_rate_decoded = (long int)bitRate;
    sampling_rate_decoded = (long int)fSample;
    block_size_samples = 960;
    delay_samples_decoded = 2048;
    channel_configuration = CHC_MONO;
    if (numChannel ==2){
      channel_configuration =CHC_DUAL;
    }
    qc_select = AAC_SCALABLE;
    tf_select = MDCT_AAC;
    transport_stream = AAC_RAWDATA_TSTREAM;

    if ( ( lopPara = strstr ( decPara, "-lop"))) {
      sscanf(&lopPara[4],"%d%d%f",&lopLong,&lopShort,&normBw);
      if ((lopLong < 0 ) || (lopLong > 950)) {
        CommonExit(-1,"\n lopLong parameter out of range: %d", lopLong);
      }
      if ((lopShort < 0 ) || (lopShort > 110)) {
        CommonExit(-1,"\n lopShort parameter out of range: %d", lopShort);
      }
      if ((normBw < 0.0F) || (normBw > 1.0F)) {
        CommonExit(-1,"\n normalized bandwidth out of range: %f", lopShort);
      }
      if ( mainDebugLevel ) {
        printf("lopLong=%d, lopShort=%d, normalized bandwidth=%3.2f\n", lopLong, lopShort, normBw);
      }
    }
    if ( ( outLayNr = strstr( decPara, "-out") ) ){
      sscanf(&outLayNr[4],"%d",&scalOutSelect);
      if ( (scalOutSelect < 0 ) || ( scalOutSelect > 7 ) ){
        CommonExit(-1,"\n errorn in output layer select %d  ",scalOutSelect);
      } 
      if ( mainDebugLevel ) {
        printf("\n layer nr. %d is selected \n",scalOutSelect);
      }
    }
    tfData->output_select = scalOutSelect;
    aacScaleableDecodeInit();
  }

  if( strstr( decPara, "-aac_sys" )   
      || ( strstr(decPara, "-mp4ff") != NULL)
      ) {
    unsigned int x;
    char * outLayNr;
    char * lopPara;
    AUDIO_SPECIFIC_CONFIG *audSpC = &frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig;
    
    switch (audSpC->channelConfiguration.value) {
    case 1 :numChannel=1;       
      break;
    case 2 :numChannel=2;       
      break;
    default: CommonExit(1,"wrong channel config");
    }

    /* currently fix coded !!! */
    if ( mainDebugLevel ) {
      printf(" TF scaleable decoding NEW flexmux  bitstream syntax\n");
    }
    bit_rate_decoded = (long int)bitRate;
    sampling_rate_decoded = (long int)fSample;
    block_size_samples = ( frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
    delay_samples_decoded = 2048;
    channel_configuration = CHC_MONO;
    if (numChannel ==2){
      channel_configuration =CHC_DUAL;
    }
    qc_select = AAC_SCALABLE;
    tf_select = MDCT_AAC;
    transport_stream = AAC_RAWDATA_TSTREAM;

    if ( ( lopPara = strstr ( decPara, "-lop"))) {
      sscanf(&lopPara[4],"%d%d%f",&lopLong,&lopShort,&normBw);
      if ((lopLong < 0 ) || (lopLong > 950)) {
        CommonExit(-1,"\n lopLong parameter out of range: %d", lopLong);
      }
      if ((lopShort < 0 ) || (lopShort > 110)) {
        CommonExit(-1,"\n lopShort parameter out of range: %d", lopShort);
      }
      if ((normBw < 0.0F) || (normBw > 1.0F)) {
        CommonExit(-1,"\n normalized bandwidth out of range: %f", lopShort);
      }
      if ( mainDebugLevel ) {
        printf("lopLong=%d, lopShort=%d, normalized bandwidth=%3.2f\n", lopLong, lopShort, normBw);
      }
    }
    if ( ( outLayNr = strstr( decPara, "-out") ) ){
      sscanf(&outLayNr[4],"%d",&scalOutSelect);
      if ( (scalOutSelect < 0 ) || ( scalOutSelect > 7 ) ){
        CommonExit(-1,"\n errorn in output layer select %d  ",scalOutSelect);
      } 
      if ( mainDebugLevel ) {
        printf("\n for output layer nr. %d is selected ",scalOutSelect);
      }
    }

    tfData->output_select = scalOutSelect;
    aacScaleableDecodeInit();
    if( strstr(decPara, "-mp4ff") == NULL)  {      
      for ( x = 0; x < frameData->od->streamCount.value; x++ ){
        frameData->layer[x].bitBuf = BsAllocBuffer(64000);
      }
    }
    
    tfData->tnsBevCore=0;     
    if( strstr( decPara, "-tnsBevCor" ) ) {
      if ( mainDebugLevel ) {
        printf("\n DON'T calc tns on core spectrum");
      }
      tfData->tnsBevCore=1;      
    }
  }  

  if( strstr( decPara, "-qc_ntt" ) ) {
    qc_select = NTT_VQ;
  }
  /* T.Ishikawa 980625 */
  if( strstr(decPara, "-tvq_sys") )
    {
      qc_select = NTT_VQ_SYS;
      for(x=0;x<(int)(frameData->od->streamCount.value);x++)
        {
          frameData->layer[x].bitBuf = BsAllocBuffer(8200);
        }
    }
  else if( strstr( decPara, "-nttDecLyr") != NULL)
    {
      qc_select = NTT_VQ;
    }

  if( strstr( decPara, "-qc_mdct16" ) ) {
    qc_select = MDCT_VALUES_16BIT;
  }
  
  if( strstr( decPara, "-pred_ltp" ) )
    pred_type = NOK_LTP;
  if( strstr( decPara, "-pred_nok_bwp" ) )
    pred_type = NOK_BWP;
  /*always NOK_LTP for TVQ or SCALABLE */
  if (qc_select == NTT_VQ_SYS || qc_select == NTT_VQ)
    pred_type = NOK_LTP;
  if (qc_select == AAC_SCALABLE )
    pred_type = NOK_LTP;
  /* ----- if no AAC_RAWDATA, get some header information -----*/
  if ((transport_stream != AAC_RAWDATA_TSTREAM) &&( qc_select != NTT_VQ_SYS) 
      && (transport_stream != NO_TSTREAM) ){ /* T.Ishikawa 980526 */
    BsGetBit( header_stream, &ultmp, 16 );
    if( ultmp != 0x1234 ) {
      CommonExit( 1,"Wrong Stream Header" );
    }
    BsGetBit( header_stream, &ultmp, 24 );
    bit_rate_decoded = ultmp;
    BsGetBit( header_stream, &ultmp, 24 );
    sampling_rate_decoded = ultmp;
    BsGetBit( header_stream, &ultmp, 16 );
    block_size_samples = ultmp;
    BsGetBit( header_stream, &ultmp, 16 );
    med_win_in_long = ultmp;
    BsGetBit( header_stream, &ultmp, 16 );
    short_win_in_long = ultmp;
    BsGetBit( header_stream, &ultmp, 16 );
    delay_samples_decoded = ultmp;
    BsGetBit( header_stream, &ultmp, 8 );
    channel_configuration = (enum CH_CONFIG)ultmp;
    BsGetBit( header_stream, &ultmp, 32 );
    tf_select = (enum TF_MOD_SELECT)ultmp;
    BsGetBit( header_stream, &ultmp, 32 );
    qc_select = (QC_MOD_SELECT)ultmp;
  }


  if((qc_select != NTT_VQ) && (qc_select != NTT_VQ_SYS)){ /* T.Ishikawa 980525*/
    if (numChannel > 2)
      CommonExit(1,"DecTfInit: audio data has more than one channel (%d)",
                 numChannel);
  }
  
  /* If short blocks, set blocksizeSample manually, 
     if encoder is up to date for 960s this should be 
     written/read to/from bitstreamheader */
  if (strstr(decPara, "-qc_aac_960")){
    useShortWindows=1;
    block_size_samples=960;
  }
  else
    useShortWindows=0;
  
  /* -- if AAC_RAWDATA, get "aacraw. "
     Note: you must have set -c -aac_raw for decoding  --- */
  /*
    if( transport_stream == AAC_RAWDATA_TSTREAM ) {
    BsGetBit( header_stream, &ultmp, 8 );
    BsGetBit( header_stream, &ultmp, 8 );
    BsGetBit( header_stream, &ultmp, 8 );
    BsGetBit( header_stream, &ultmp, 8 );
    BsGetBit( header_stream, &ultmp, 8 );
    BsGetBit( header_stream, &ultmp, 8 );
    }
  */

  if (qc_select == NTT_VQ){
    int iscl;
    /* NTT_VQ supports both LENINFO_STREAM and NO_SYNCWORD. */
    BsGetBit(header_stream, &ultmp, 2);
    transport_stream = (enum TRANSPORT_STREAM)ultmp;
    /* If the ntt_varbit is off, number of bits
       per frame should be in the header. */
    BsGetBit(header_stream, &ultmp, 1);
    ntt_varbit = (enum NTT_VARBIT)ultmp;
    if (ntt_varbit == NTT_VARBIT_OFF){
      BsGetBit(header_stream, &ultmp, 32);
      frameNumBit = ultmp;
    }
    /* long term predictor */
    BsGetBit(header_stream, &ultmp, 1);
    ntt_LTP_SW = ultmp;

    /* header for base coder */ /* T.Ishikawa 981118 */
    BsGetBit(header_stream, &ultmp, 16);
    ntt_IBPS= ultmp;
    /*
      ntt_BPS=(double)ntt_IBPS;
    */	
    /* header for scalable coder */
    BsGetBit(header_stream, &ultmp, ntt_NSclLay_BITS);
    ntt_NSclLay = (int)ultmp;
    for (iscl=0; iscl<ntt_NSclLay; iscl++){
      BsGetBit(header_stream, &ultmp, ntt_IBPS_BITS_SCL);
      ntt_IBPS_SCL[iscl] = ultmp;
      /*NN
        ntt_BPS_SCL[iscl]=(double)ntt_IBPS_SCL[iscl];
      */
      /*      ntt_BPS_SCL[iscl] = (double)ultmp;*/
      fprintf(stderr,"ntt_IBPS_SCL[%d] %d\n",iscl,(int)ultmp);
    }
  }

  /* NTT_TVQ_SYS T.Ishikawa 980525 */
  if (qc_select == NTT_VQ_SYS){
    int iscl,mat_block_size,mat_total_bitrate;
    float mat_sampleFreq = 0;

    /* Decoding Condition Setting T.Ishikawa */
    mat_block_size = 
      ( frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
    if ( mainDebugLevel>2 ) {
      printf("TTTTTTTTTTTT %5d TTTTTTTTTTT\n",mat_block_size);
    }

    switch
      (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value)
      {
      case 0:
        mat_sampleFreq=96000.0;
        sampling_rate_decoded=96000;
        break;
      case 1:
        mat_sampleFreq=88200.0;
        sampling_rate_decoded=88200;
        break;
      case 2:
        mat_sampleFreq=64000.0;
        sampling_rate_decoded=64000;
        break;
      case 3:
        mat_sampleFreq=48000.0;
        sampling_rate_decoded=48000;
        break;
      case 4:
        mat_sampleFreq=44100.0;
        sampling_rate_decoded=44100;
        break;
      case 5:
        mat_sampleFreq=32000.0;
        sampling_rate_decoded=32000;
        break;
      case 6:
        mat_sampleFreq=24000.0;
        sampling_rate_decoded=24000;
        break;
      case 7:
        mat_sampleFreq=22050.0;
        sampling_rate_decoded=22050;
        break;
      case 8:
        mat_sampleFreq=16000.0;
        sampling_rate_decoded=16000;
        break;
      case 9:
        mat_sampleFreq=12000.0;
        sampling_rate_decoded=12000;
        break;
      case 10:
        mat_sampleFreq=11025.0;
        sampling_rate_decoded=11025;
        break;
      case 11:
        mat_sampleFreq=8000.0;
        sampling_rate_decoded=8000;
        break;
      case 12:
      case 13:
      case 14:
      case 15:
        CommonWarning("reserved \n");
        break;
      default:
        CommonWarning("mat_sampleFreq Error !!\n");
        break;
      }
    /* NTT_VQ supports both LENINFO_STREAM and NO_SYNCWORD. */
    /*BsGetBit(header_stream, &ultmp, 2);
      transport_stream = (enum TRANSPORT_STREAM)ultmp;*/
    transport_stream = (enum TRANSPORT_STREAM)0;/* adhoc */

    /* If the ntt_varbit is off, number of bits
       per frame should be in the header. */
    /*BsGetBit(header_stream, &ultmp, 1);
      ntt_varbit = (enum NTT_VARBIT)ultmp;*/
    ntt_varbit = NTT_VARBIT_OFF;
    if (ntt_varbit == NTT_VARBIT_OFF){
      /*BsGetBit(header_stream, &ultmp, 32);
        frameNumBit = ultmp;*/
      /*            CommonWarning("TwinVQ(for System) is not supported !!\n");*/
      /*      frameNumBit = (int)(frameData->od->ESDescriptor[0]->DecConfigDescr.avg
              Bitrate.value*mat_block_size/mat_sampleFreq);
              if ( mainDebugLevel>2 ) {
              printf(" ###### %d ######\n",frameNumBit);
              }*/
    }
    /* long term predictor */
    /*    BsGetBit(header_stream, &ultmp, 1);
          ntt_LTP_SW = ultmp;*/
    ntt_LTP_SW = 0;/* adhoc */

    /* header for scalable coder */
    ntt_NSclLay = frameData->od->streamCount.value-1;
    /* ntt_NSclLay = the Number of Enhanced Layer */

    ntt_IBPS = 
      (int)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value);
    /*
      ntt_BPS = 
      (float)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value); */
    /* T.Ishikawa 980623 */
    if ( mainDebugLevel>2 ) {
      printf(" ///// ntt_IBPS %d /////\n",ntt_IBPS);
      /*
        printf(" ///// ntt_BPS %f /////\n",ntt_BPS);
      */
    }
    for (iscl=1; iscl<=ntt_NSclLay; iscl++){
      ntt_IBPS_SCL[iscl-1]=(int)
        (frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value);
      /*
        ntt_BPS_SCL[iscl-1]=(float)
        (frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value);
      */
      if ( mainDebugLevel>2 ) {
        printf(" ///// ntt_IBPS_SCL[%d] %d /////\n",
               iscl-1,ntt_IBPS_SCL[iscl-1]);
      }
    }



    for(mat_total_bitrate=0,iscl=0;iscl<ntt_NSclLay;iscl++)
      {
        mat_total_bitrate += ntt_IBPS_SCL[iscl];
      }
    mat_total_bitrate += ntt_IBPS;

    frameNumBit = (int)floor(mat_total_bitrate*mat_block_size/mat_sampleFreq);
    if ( mainDebugLevel >2) {
      printf(" ###### %d ######\n",frameNumBit);
    }
    block_size_samples = mat_block_size;
    delay_samples_decoded=mat_block_size;


  }

  BsCloseRemove(header_stream,1);

  /* currently initializes the scale factor band table; should be moved to a separate module */
  /*
    if( (qc_select != NTT_VQ) && ( qc_select != NTT_VQ_SYS) ) */{
    /* T.Ishikawa 98052*/
    EncTf_psycho_acoustic( sampling_rate_decoded , 
                           1, 
                           qc_select,
                           block_size_samples, 
                           chpo_long, 
                           chpo_short );
  }
 
  /* set the return values */
  *frameNumSample = block_size_samples;
  *delayNumSample = delay_samples_decoded;

  /* calc. no of audio channels */
  if ((qc_select != NTT_VQ) && ( qc_select != NTT_VQ_SYS)){ 
    /* T.Ishikawa 980525 */
    /* table to get the no. of audio channels depending on the value of channel_configuration */
    static int  chc_2_chan_nr[CHC_MODES] = { 1, 2, 2, 5 };

    max_ch  = chc_2_chan_nr[channel_configuration];
  }
  else{
    max_ch = numChannel;
  }

  for (x=0;x<MAX_TF_LAYER;x++){
    for (i_ch=0; i_ch<max_ch; i_ch++){
      spectral_line_vector[x][i_ch] = (double*)calloc(block_size_samples,
                                                      sizeof(double));
      tfData->spectral_line_vector[x][i_ch]=spectral_line_vector[x][i_ch];
    }
  }
  for (i_ch=0; i_ch<max_ch; i_ch++){
    time_sample_vector[i_ch] = (double*)calloc(block_size_samples,
                                               sizeof(double));
    tfData->time_sample_vector[i_ch]=time_sample_vector[i_ch];
  }
  /* son_NBCpp */
  {
    int i_ch; /* added 971010 YT*/
    int band;
    for (i_ch=0; i_ch<max_ch; i_ch++){ /* added 971010 YT*/
      
      spectral_line_vector_for_gc[i_ch] = (double *)calloc(block_size_samples, sizeof(double));
      imdctBufForGC[i_ch] = (double *)calloc(block_size_samples*2, sizeof(double));
      DBandSigBufForOverlapping[i_ch] = (double **)calloc(NBANDS, sizeof(double *));
      DBandSigBuf[i_ch] = (double **)calloc(NBANDS, sizeof(double *));
      gainInfo[i_ch] = (GAINC **)calloc(NBANDS, sizeof(GAINC *));
      for (band = 0; band < NBANDS; band++) {
        DBandSigBufForOverlapping[i_ch][band] =
          (double *)calloc(block_size_samples/NBANDS*2, sizeof(double));
        DBandSigBuf[i_ch][band] =
          (double *)calloc(block_size_samples/NBANDS, sizeof(double));
        gainInfo[i_ch][band] = (GAINC *)calloc(short_win_in_long, sizeof(GAINC));
      }
    } /* end of for(i_ch...)*/
  }
  
  /* initialize inverse Q&C */
  switch(qc_select){
  case NTT_VQ:
    {
      int iscl;
      float t_bit_rate_scl;
      char *p_ctmp;
      
      /* bitrate control */
      t_bit_rate_scl = 0.;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
      }
      /* initalization of base decoder */
      if ( mainDebugLevel>2 )  fprintf(stderr,"TTTTTT : bitRate %f ntt_BPS %d\n",bitRate,ntt_IBPS);
	  
      ntt_TfInit((float)fSample, (float)ntt_IBPS+t_bit_rate_scl/*(float)(ntt_IBPS+t_bit_rate_scl)*//*(float)bitRate*/, t_bit_rate_scl, max_ch,
                 block_size_samples, &ntt_index, &ntt_index_scl);
      /**TM9002
         ntt_nok_lt_pred_dec_init(&ntt_info);
      */
      aac_decode_init(sampling_rate_decoded,
                      aacDebugStr, block_size_samples, &sfbInfo, pred_type);

      /* initialization of scalable coders */
      /*NNNN
        ntt_TBITS_FR_SCL = 0;
        i*/
      /* set number of scalable layers to be decoded */
      ntt_NSclLayDec = ntt_NSclLay;
      if( ( p_ctmp=strstr( decPara, "-nttDecLyr " ) ) ) {
        if(sscanf(p_ctmp+11, "%d", &ntt_NSclLayDec) == 0){
          CommonExit(1, "Encode: parameter of -nttDecLyr switch not found");
        }
      }
      if (ntt_NSclLayDec > ntt_NSclLay) ntt_NSclLayDec = ntt_NSclLay;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        ntt_scale_init ( iscl, 
                         ntt_NSclLayDec, 
                         ntt_IBPS_SCL[iscl], 
                         (float)fSample, 
                         &ntt_index, 
                         &ntt_index_scl);
	nttData->nttDataScl = ntt_index_scl.nttDataScl;
        if( strstr( decPara, "-nttSclMsg" ) ) {
          ntt_scale_message(iscl, ntt_IBPS, ntt_IBPS_SCL);
        }
      }
      ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay;
      ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay;
      ntt_index.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      nttData->nttDataBase = ntt_index.nttDataBase;
      nttData->nttDataScl = ntt_index_scl.nttDataScl;

      /* T.Ishikawa 981014 */
      if ( mainDebugLevel>2 ) {
        printf("<<OLD>> BAND %f BPS %d\n",ntt_index.nttDataBase->bandUpper,
               ntt_IBPS);
        for(iscl=0;iscl<ntt_NSclLay;iscl++)
          printf("<<OLD>> BPS_SCL[%d] %d\n",iscl,ntt_IBPS_SCL[iscl]);
        printf("<<OLD>> CH %d\n",numChannel);
        for(iscl=0;iscl<4;iscl++)
          printf("<<OLD>> AC_TOP[%d] %f , AC_BTM[%d] %f\n",iscl,
                 ntt_index_scl.nttDataScl->ac_top[0][0][iscl],iscl,
                 ntt_index_scl.nttDataScl->ac_btm[0][0][iscl]);

        if (ntt_NSclLay > 0){
          printf( "<<>><<>> NUMBER OF LAYERS TO BE DECODED: %d\n",
                  ntt_NSclLayDec);
        }
      }
      break;
    }

  case NTT_VQ_SYS: /* T.Ishikawa 980525 */
    {
      int iscl;
      float t_bit_rate_scl,tomo_ftmp;
      char *p_ctmp;

      if ( mainDebugLevel ) {
        printf(" ####### TwinVQ(FlexMux) Init %x\n",qc_select);
      }
      /* bitrate control */
      t_bit_rate_scl = 0.;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
      }

      tomo_ftmp=(float)ntt_IBPS+t_bit_rate_scl;
      /*      bitRate -= (float)((int)( ((31.0*sampling_rate_decoded/1024.0)*(float)
              (ntt_NSclLay+1) + 25.0*sampling_rate_decoded/1024.0)));*/
      /*      bitRate -= (float)((int)( ((31.0*24000.0/1024.0)*(float)(ntt_NSclLay+1
              ) + 25.0*24000.0/1024.0)));*/
      if ( mainDebugLevel>2 ) {
        printf(" <<<< DecTfInit : tomo_ftmp %f >>> \n",tomo_ftmp);
      }
      /* initalization of base decoder */
      ntt_TfInit((float)fSample, (float)tomo_ftmp, t_bit_rate_scl, 
                 max_ch, block_size_samples, &ntt_index, &ntt_index_scl);
      /*TM9902*
        ntt_nok_lt_pred_dec_init(&ntt_info);
      */
      sampling_rate_decoded = (long int)fSample;
      fprintf(stderr,"sampling_rate_decoded %ld\n",sampling_rate_decoded);
      aac_decode_init(sampling_rate_decoded,
                      aacDebugStr, block_size_samples, &sfbInfo, pred_type);

      /* initialization of scalable coders */
      /*NN
        ntt_TBITS_FR_SCL = 0;
        i*/
      /* set number of scalable layers to be decoded */
      ntt_NSclLayDec = ntt_NSclLay;
      if( ( p_ctmp=strstr( decPara, "-nttDecLyr " ) ) ) {
        if(sscanf(p_ctmp+11, "%d", &ntt_NSclLayDec) == 0){
          CommonExit(1, "Encode: parameter of -nttDecLyr switch not found");
        }
      }
     
      if (ntt_NSclLayDec > ntt_NSclLay) ntt_NSclLayDec = ntt_NSclLay;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        ntt_scale_init ( iscl, 
                         ntt_NSclLayDec, 
                         ntt_IBPS_SCL[iscl], 
                         (float)fSample, 
                         &ntt_index, 
                         &ntt_index_scl);
	nttData->nttDataScl = ntt_index_scl.nttDataScl;
	
	if( strstr( decPara, "-nttSclMsg" ) ) {
          ntt_scale_message(iscl, ntt_IBPS, ntt_IBPS_SCL);
        }
      }
      ntt_index.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
      ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
      tfData->output_select = ntt_NSclLayDec;
      scalOutSelect = ntt_NSclLayDec;
      nttData->nttDataBase = ntt_index.nttDataBase;
      nttData->nttDataScl = ntt_index_scl.nttDataScl;
      
      /* T.Ishikawa 980623 */
      t_bit_rate_scl=0;
      for(iscl=0;iscl<ntt_NSclLay;iscl++)
        {
          /* T.Ishikawa 980703 */
          t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl]*numChannel;
        }


      if ( mainDebugLevel>2 ) {
        printf("BAND %f BPS %d\n",ntt_index.nttDataBase->bandUpper,
               ntt_IBPS);
        for(iscl=0;iscl<ntt_NSclLay;iscl++)
          printf("BPS_SCL[%d] %d\n",iscl,ntt_IBPS_SCL[iscl]);
        printf("CH %d\n",numChannel);
        for(iscl=0;iscl<4;iscl++)
          printf("AC_TOP[%d] %f , AC_BTM[%d] %f\n",iscl,
                 ntt_index_scl.nttDataScl->ac_top[0][0][iscl],iscl,
                 ntt_index_scl.nttDataScl->ac_btm[0][0][iscl]);
        if (ntt_NSclLay > 0){
          printf( "<<>><<>> NUMBER OF LAYERS TO BE DECODED: %d\n",
                  ntt_NSclLayDec);
        }
      }
      break;
    }

  case AAC_QC:
    {
      aacDebugString = aacDebugStr;
      aac_decode_init ( sampling_rate_decoded,
                        aacDebugStr,
                        block_size_samples,
                        &sfbInfo,
                        pred_type);
      break;    /* HP 970407 */
    }

  case AAC_SCALABLE:
    {

      aacDebugString = aacDebugStr;
      aac_decode_init ( sampling_rate_decoded,
                        aacDebugStr, 
                        block_size_samples, 
                        &sfbInfo,
                        pred_type);
      if (frameData && 
          (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2)){
        ntt_TfInit((float)fSample, 
		   (float)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value), 
		   0.0, numChannel, block_size_samples, &ntt_index, &ntt_index_scl);
        ntt_index.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
        ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
        ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
        ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
	nttData->nttDataBase = ntt_index.nttDataBase;
	nttData->nttDataScl = ntt_index_scl.nttDataScl;
      }
      break;
    }


  default:
    CommonExit(-1,"\nerror in switch");
  }
  for (i_ch=0; i_ch<max_ch; i_ch++){
    overlap_buffer[i_ch] = (double*)calloc(block_size_samples,
                                           sizeof(double));
    tfData->overlap_buffer[i_ch]=    overlap_buffer[i_ch];
  }
  tfData->sfbInfo = sfbInfo;
  tfData->block_size_samples = block_size_samples;
  tfData->prev_windowShape[0] = prev_windowShape[0];
  tfData->prev_windowShape[1] = prev_windowShape[1];
  StatisticsAacInitV1 ( );

  /* nok 990113 ... */
  tfData->pred_type=pred_type;
  if(pred_type == NOK_LTP)
    for(i_ch = 0; i_ch < numChannel; i_ch++)
      {
        nok_lt_status[i_ch] = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_lt_status[0]));
      
        nok_init_lt_pred(nok_lt_status[i_ch]);
        tfData->nok_lt_status[i_ch]=nok_lt_status[i_ch];
      }
  /* ... nok */
  tvq_debug_level = mainDebugLevel;

}

void DecTfInitNew (
                   char*               decPara,          /* in: decoder parameter string */    
                   char*               aacDebugStr,
                   HANDLE_BUFFER*      hVm,
                   HANDLE_BUFFER*      hHcrSpecData,
                   FRAME_DATA*         frameData,
                   TF_DATA*            tfData,
                   LPC_DATA*           lpcData,
                   ntt_DATA*           nttData,
                   int                 mainDebugLevel,
                   int                 layer
                   )
{

  int                 numChannel;       /* in: num audio channels */
  float               fSample;          /* in: sampling frequancy [Hz] */
  int                 profile;          /* in: bitstream profile as indicated in the ADIF header */
    
  int           delay_samples_decoded = 0;
  int           x; 
  int           i_ch;
  WINDOW_SHAPE prev_windowShape[2] = {WS_FHG, WS_FHG}; 
  float mat_sampleFreq = 0;
  int block_size_samples  ;
  /* +++++++++++++++++++++++++++++++++++++++++ */
  int ntt_NSclLay;
  int ntt_NSclLayDec;
  int ntt_LTP_SW; /*dh990414*/
  int ntt_IBPS, ntt_IBPS_SCL[8];
  float ntt_BPS, ntt_BPS_SCL[8];
  ntt_INDEX           ntt_index;
  ntt_INDEX           ntt_index_scl;
  /* +++++++++++++++++++++++++++++++++++++++++ */
  *hVm          = CreateBuffer(VIRTUELL);
  *hHcrSpecData = CreateBuffer(AAC_MAX_INPUT_BUF_BITS);


  if( strstr( decPara, "-aac_sys" )
      || ( strstr(decPara, "-mp4ff") != NULL)
      ) { profile = LC_Profile; }

  /* set profile */
  mc_info.profile = profile;

  block_size_samples 
    = ( frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024);

  switch
    (frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value)
    {
    case 0:
      mat_sampleFreq=96000.0;
      sampling_rate_decoded=96000;
      break;
    case 1:
      mat_sampleFreq=88200.0;
      sampling_rate_decoded=88200;
      break;
    case 2:
      mat_sampleFreq=64000.0;
      sampling_rate_decoded=64000;
      break;
    case 3:
      mat_sampleFreq=48000.0;
      sampling_rate_decoded=48000;
      break;
    case 4:
      mat_sampleFreq=44100.0;
      sampling_rate_decoded=44100;
      break;
    case 5:
      mat_sampleFreq=32000.0;
      sampling_rate_decoded=32000;
      break;
    case 6:
      mat_sampleFreq=24000.0;
      sampling_rate_decoded=24000;
      break;
    case 7:
      mat_sampleFreq=22050.0;
      sampling_rate_decoded=22050;
      break;
    case 8:
      mat_sampleFreq=16000.0;
      sampling_rate_decoded=16000;
      break;
    case 9:
      mat_sampleFreq=12000.0;
      sampling_rate_decoded=12000;
      break;
    case 10:
      mat_sampleFreq=11025.0;
      sampling_rate_decoded=11025;
      break;
    case 11:
      mat_sampleFreq=8000.0;
      sampling_rate_decoded=8000;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      CommonWarning("reserved \n");
      break;
    default:
      CommonWarning("mat_sampleFreq Error !!\n");
      break;
    }
  
  frameData->layer[layer].sampleRate = sampling_rate_decoded;
  frameData->layer[layer].bitRate = frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value;

  if( strstr( decPara, "-aac_sys" )   
      || ( strstr(decPara, "-mp4ff") != NULL)
      ) {
    unsigned int x;
    char * lopPara;
    AUDIO_SPECIFIC_CONFIG *audSpC = &frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig;
    
    switch (audSpC->channelConfiguration.value) {
    case 1 :numChannel=1;       
      break;
    case 2 :numChannel=2;       
      break;
    default: CommonExit(1,"wrong channel config");
    }

    for (i_ch=0; i_ch<numChannel; i_ch++) {
      if ((tfData->sampleBuf[i_ch]=(float*)malloc(block_size_samples*sizeof(float)))
          ==NULL)
        CommonExit(1,"Decode: memory allocation error");
    }
    /* currently fix coded !!! */
    if ( mainDebugLevel ) {
      printf(" TF scaleable decoding NEW flexmux  bitstream syntax\n");
    }

    fSample= sampling_rate_decoded ;

    block_size_samples = ( frameData->od->ESDescriptor[frameData->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;

    med_win_in_long = 2;
    short_win_in_long = 8;
    delay_samples_decoded = 2048;
    channel_configuration = CHC_MONO;
    if (numChannel ==2){
      channel_configuration =CHC_DUAL;
    }
    qc_select = AAC_SCALABLE;
    tf_select = MDCT_AAC;
    transport_stream = AAC_RAWDATA_TSTREAM;

    if ( ( lopPara = strstr ( decPara, "-lop"))) {
      sscanf(&lopPara[4],"%d%d%f",&lopLong,&lopShort,&normBw);
      if ((lopLong < 0 ) || (lopLong > 950)) {
        CommonExit(-1,"\n lopLong parameter out of range: %d", lopLong);
      }
      if ((lopShort < 0 ) || (lopShort > 110)) {
        CommonExit(-1,"\n lopShort parameter out of range: %d", lopShort);
      }
      if ((normBw < 0.0F) || (normBw > 1.0F)) {
        CommonExit(-1,"\n normalized bandwidth out of range: %f", lopShort);
      }
      if ( mainDebugLevel ) {
        printf("lopLong=%d, lopShort=%d, normalized bandwidth=%3.2f\n", lopLong, lopShort, normBw);
      }
    }

/*     aacScaleableDecodeInit(); */
    if( strstr(decPara, "-mp4ff") == NULL)  {      
      for ( x = 0; x < frameData->od->streamCount.value; x++ ){
        frameData->layer[x].bitBuf = BsAllocBuffer(64000);
      }
    }
    
    tfData->tnsBevCore=0;     
    if( strstr( decPara, "-tnsBevCor" ) ) {
      if ( mainDebugLevel ) {
        printf("\n DON'T calc tns on core spectrum");
      }
      tfData->tnsBevCore=1;      
    }
  }  



  /* NTT_TVQ_SYS T.Ishikawa 980525 */
  if (frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2
      && layer==0){
    int iscl,mat_block_size,mat_total_bitrate;
    
    /* Decoding Condition Setting T.Ishikawa */
    mat_block_size = 
      ( frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
    if ( mainDebugLevel>2 ) {
      printf("TTTTTTTTTTTT %5d TTTTTTTTTTT\n",mat_block_size);
    }
#if 0
    tvq_flag_960=0;
    if (mat_block_size==960) tvq_flag_960=1;
#endif

    /* NTT_VQ supports both LENINFO_STREAM and NO_SYNCWORD. */
    /*BsGetBit(header_stream, &ultmp, 2);
      transport_stream = (enum TRANSPORT_STREAM)ultmp;*/
    transport_stream = (enum TRANSPORT_STREAM)0;/* adhoc */

    /* If the ntt_varbit is off, number of bits
       per frame should be in the header. */
    /*BsGetBit(header_stream, &ultmp, 1);
      ntt_varbit = (enum NTT_VARBIT)ultmp;*/
    ntt_varbit = NTT_VARBIT_OFF;
    if (ntt_varbit == NTT_VARBIT_OFF){
      /*BsGetBit(header_stream, &ultmp, 32);
        frameNumBit = ultmp;*/
      /*            CommonWarning("TwinVQ(for System) is not supported !!\n");*/
      /*      frameNumBit = (int)(frameData->od->ESDescriptor[0]->DecConfigDescr.avg
              Bitrate.value*mat_block_size/mat_sampleFreq);
              if ( mainDebugLevel>2 ) {
              printf(" ###### %d ######\n",frameNumBit);
              }*/
    }
    /* long term predictor */
    /*    BsGetBit(header_stream, &ultmp, 1);
          ntt_LTP_SW = ultmp;*/
    ntt_LTP_SW = 0;/* adhoc */

    /* header for scalable coder */
    ntt_NSclLay = frameData->od->streamCount.value-1;
#if 1 /*990531*/
    /* ntt_NSclLay = the Number of Enhanced Layer */
    {
      int iexit;
      iexit =0;
      ntt_NSclLay = -1;
      for (iscl=0; ((iscl<(int)frameData->od->streamCount.value) && (iexit ==0)); iscl++)
      {
	  if(frameData->od->ESDescriptor[iscl]->DecConfigDescr.\
          audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2)
	       ntt_NSclLay ++;
	  else iexit = 1000;
      }
    }
#endif
    ntt_IBPS = 
      (int)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value);
    ntt_BPS = 
      (float)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value); /* T.Ishikawa 980623 */
    if ( mainDebugLevel>2 ) {
      printf(" ///// ntt_IBPS %d /////\n",ntt_IBPS);
      printf(" ///// ntt_BPS %f /////\n",ntt_BPS);
    }
    for (iscl=1; iscl<=ntt_NSclLay; iscl++){
      ntt_IBPS_SCL[iscl-1]=(int)
        (frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value);
      ntt_BPS_SCL[iscl-1]=(float)
        (frameData->od->ESDescriptor[iscl]->DecConfigDescr.avgBitrate.value);
      if ( mainDebugLevel>2 ) {
        printf(" ///// ntt_IBPS_SCL[%d] %d /////\n",
               iscl-1,ntt_IBPS_SCL[iscl-1]);
      }
    }



    for(mat_total_bitrate=0,iscl=0;iscl<ntt_NSclLay;iscl++)
      {
        mat_total_bitrate += ntt_IBPS_SCL[iscl];
      }
    mat_total_bitrate += ntt_IBPS;

    frameNumBit = (int)floor(mat_total_bitrate*mat_block_size/mat_sampleFreq);
    if ( mainDebugLevel >2) {
      printf(" ###### %d ######\n",frameNumBit);
    }
    block_size_samples = mat_block_size;
    delay_samples_decoded=mat_block_size;


  }



  /* calc. no of audio channels */
  max_ch = numChannel;
 
  for (x=0;x<MAX_TF_LAYER;x++){
    for (i_ch=0; i_ch<max_ch; i_ch++){
      spectral_line_vector[x][i_ch] = (double*)calloc(block_size_samples,
                                                      sizeof(double));
      tfData->spectral_line_vector[x][i_ch]=spectral_line_vector[x][i_ch];
    }
  }
  for (i_ch=0; i_ch<max_ch; i_ch++){
    time_sample_vector[i_ch] = (double*)calloc(block_size_samples,
                                               sizeof(double));
    tfData->time_sample_vector[i_ch]=time_sample_vector[i_ch];
  }
  if (frameData && 
      (frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2)){
    qc_select= NTT_VQ_SYS;
  } 
  
  if (frameData && 
      (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2)){
    /* acutally need to look at the audio object type; it  is 7 for tvq */
    qc_select= NTT_VQ_SYS;
  } 

  /*always NOK_LTP for TVQ or SCALABLE */
  if (qc_select == NTT_VQ_SYS || qc_select == NTT_VQ)
    pred_type = NOK_LTP;
#if 0
  if (qc_select == AAC_SCALEABLE )
    pred_type = NOK_LTP;
#endif

  /* initialize inverse Q&C */
  switch(qc_select){
  case NTT_VQ_SYS: /* T.Ishikawa 980525 */
    {
      int iscl;
      float t_bit_rate_scl,tomo_ftmp;
      char *p_ctmp;

      if ( mainDebugLevel ) {
        printf(" ####### TwinVQ(FlexMux) Init %x\n",qc_select);
      }
      /* bitrate control */
      t_bit_rate_scl = 0.;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl];
      }

      tomo_ftmp=(float)ntt_IBPS+t_bit_rate_scl;
      if ( mainDebugLevel>2 ) {
        printf(" <<<< DecTfInit : tomo_ftmp %f >>> \n",tomo_ftmp);
      }
      /* initalization of base decoder */
      ntt_TfInit((float)fSample, (float)tomo_ftmp, t_bit_rate_scl, 
                 max_ch, block_size_samples, &ntt_index, &ntt_index_scl);
      /*TM9902*
        ntt_nok_lt_pred_dec_init(&ntt_info);
      */
      sampling_rate_decoded = (long int)fSample;
      fprintf(stderr,"sampling_rate_decoded %ld\n",sampling_rate_decoded);
      aac_decode_init(sampling_rate_decoded,
                      aacDebugStr, block_size_samples, &sfbInfo, pred_type);

      /* initialization of scalable coders */
      /*NN
        ntt_TBITS_FR_SCL = 0;
        i*/
      /* set number of scalable layers to be decoded */
      ntt_NSclLayDec = ntt_NSclLay;
      if( ( p_ctmp=strstr( decPara, "-nttDecLyr " ) ) ) {
        if(sscanf(p_ctmp+11, "%d", &ntt_NSclLayDec) == 0){
          CommonExit(1, "Encode: parameter of -nttDecLyr switch not found");
        }
      }
     
      if (ntt_NSclLayDec > ntt_NSclLay) ntt_NSclLayDec = ntt_NSclLay;
      for (iscl=0; iscl<ntt_NSclLay; iscl++){
        ntt_scale_init ( iscl, 
                         ntt_NSclLayDec, 
                         ntt_IBPS_SCL[iscl], 
                         (float)fSample, 
                         &ntt_index, 
                         &ntt_index_scl);
	nttData->nttDataScl = ntt_index_scl.nttDataScl;
	
	if( strstr( decPara, "-nttSclMsg" ) ) {
          ntt_scale_message(iscl, ntt_IBPS, ntt_IBPS_SCL);
        }
      }
      ntt_index.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
      ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
      ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
      tfData->output_select = ntt_NSclLayDec;
      scalOutSelect = ntt_NSclLayDec;
      nttData->nttDataBase = ntt_index.nttDataBase;
      nttData->nttDataScl = ntt_index_scl.nttDataScl;
      
      /* T.Ishikawa 980623 */
      t_bit_rate_scl=0;
      for(iscl=0;iscl<ntt_NSclLay;iscl++)
        {
          /* T.Ishikawa 980703 */
          t_bit_rate_scl += (float)ntt_IBPS_SCL[iscl]*numChannel;
        }


      if ( mainDebugLevel>2 ) {
        printf("BAND %f BPS %d\n",ntt_index.nttDataBase->bandUpper,
               ntt_IBPS);
        for(iscl=0;iscl<ntt_NSclLay;iscl++)
          printf("BPS_SCL[%d] %d\n",iscl,ntt_IBPS_SCL[iscl]);
        printf("CH %d\n",numChannel);
        for(iscl=0;iscl<4;iscl++)
          printf("AC_TOP[%d] %f , AC_BTM[%d] %f\n",iscl,
                 ntt_index_scl.nttDataScl->ac_top[0][0][iscl],iscl,
                 ntt_index_scl.nttDataScl->ac_btm[0][0][iscl]);
        if (ntt_NSclLay > 0){
          printf( "<<>><<>> NUMBER OF LAYERS TO BE DECODED: %d\n",
                  ntt_NSclLayDec);
        }
      }
      break;
    }

  case AAC_SCALABLE:
    {
      aacDebugString = aacDebugStr;
      aac_decode_init ( sampling_rate_decoded,
                        aacDebugStr, 
                        block_size_samples, 
                        &sfbInfo,
                        pred_type);
#if 0 
      if (frameData && 
          (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value==2)){
        ntt_TfInit((float)fSample, 
                   (float)(frameData->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value), 
                   0.0, 
                   frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value,
                   (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) ,
                   &ntt_index,
                   &ntt_index_scl );
        ntt_index.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
        ntt_index_scl.nttDataScl->ntt_NSclLayDec = ntt_NSclLayDec;
        ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
        ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay; 
	nttData->nttDataBase = ntt_index.nttDataBase;
	nttData->nttDataScl = ntt_index_scl.nttDataScl;

      }
#endif
      break;
    }


  default:
    CommonExit(-1,"\nerror in switch");
  }
  
  for (i_ch=0; i_ch<max_ch; i_ch++){
    overlap_buffer[i_ch] = (double*)calloc(block_size_samples,
                                           sizeof(double));
    tfData->overlap_buffer[i_ch]=    overlap_buffer[i_ch];
  }
  tfData->sfbInfo = sfbInfo;
  tfData->block_size_samples = block_size_samples;
  tfData->output_select =   frameData->scalOutSelect;
  tfData->prev_windowShape[0] = prev_windowShape[0];
  tfData->prev_windowShape[1] = prev_windowShape[1];

  StatisticsAacInitV1 ( );

  /* nok 990113 ... */
  tfData->pred_type=pred_type;
/*
tfData->pred_type = NOK_LTP;
fprintf(stderr, "LLLLLLLin INInew %5d %5d \n", tfData->pred_type, pred_type);
*/
  if(pred_type == NOK_LTP)
    for(i_ch = 0; i_ch < numChannel; i_ch++)
      {
        nok_lt_status[i_ch] = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_lt_status[0]));
      
        nok_init_lt_pred(nok_lt_status[i_ch]);
        tfData->nok_lt_status[i_ch]=nok_lt_status[i_ch];
      }
  /* ... nok */
  tvq_debug_level = mainDebugLevel;
  /* create time sample modulo buffer for core coder */
#define     MAX_CORE_DELAY 16384
  if (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value){
    /* Celp core */
    /*      int MPE_conf=frameData->od->ESDescriptor[0]->DecConfigDescr\ */
    /*        .audioSpecificConfig.specConf.celpSpecificConfig.MPE_Configuration.value; */
     
    int coreDelay= frameData->od->ESDescriptor[1]->DecConfigDescr\
      .audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay.value;
    int samplIdxCore= frameData->od->ESDescriptor[0]\
      ->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;
    int samplIdxTF= frameData->od->ESDescriptor[1]\
      ->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;
       
    float delayS[MAX_CORE_DELAY];
     
    tfData->samplFreqFacCore= samplFreqIndex[samplIdxTF]/samplFreqIndex[samplIdxCore];
    tfData->tfSamplRate=samplFreqIndex[samplIdxTF];
     
    tfData->mdct_overlap_buffer=(double**)malloc(2*sizeof(double*));
    tfData->mdct_overlap_buffer[0] = (double*)malloc(1024*sizeof(double));

    tfData->coreModuloBuffer  =   CreateFloatModuloBuffer(coreDelay 
                                                          + (lpcData->frameNumSample
                                                             * tfData->samplFreqFacCore * 4));
    AddFloatModuloBufferValues( tfData->coreModuloBuffer,delayS  , coreDelay);
  }

}

/*****************************************************************************************
 ***
 *** Function:    DecTfFrame
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

void DecTfFrame ( BsBitBuffer*       bitBuf,           /* in: bit stream frame */
                  float**            sampleBuf,        /* out: frameNumSample audio samples */
                  int*               usedNumBit,       /* out: num bits used for this frame */
                  int                numChannels,
                  int                mainDebugLevel,
                  HANDLE_FAULT_TOLERANT hFault,   /**TM990217*/
		  HANDLE_RESILIENCE  hResilience,      /* in: handler for error resilience info */
                  HANDLE_BUFFER      hVm,
                  HANDLE_BUFFER      hHcrSpecData,
                  HANDLE_HCR         hHcrInfo,
                  FRAME_DATA*        frameData,
                  TF_DATA*           tfData,
                  ntt_DATA*          nttData,
                  HANDLE_EP_INFO     hEpInfo,
                  HANDLE_CONCEALMENT hConcealment)
     
{
  WINDOW_SEQUENCE     windowSequence[MAX_TIME_CHANNELS]/*  = {ONLY_LONG_SEQUENCE} */;
  unsigned long       ultmp;
  int                 decoded_bits = 0;
  int                 i;
  int                 i_ch;  /* added 971010 YT */
  BsBitStream*        fixed_stream;
  /* son_NBCpp */
  BsBitStream*        gc_stream[MAX_TIME_CHANNELS];
  BsBitStream*        gc_WRstream[MAX_TIME_CHANNELS];
  BsBitBuffer*        gcBitBuf[MAX_TIME_CHANNELS];     /* bit buffer for gain_control_data() */
  static WINDOW_SHAPE windowShape[2]      = {WS_FHG, WS_FHG};
  static WINDOW_SHAPE prev_windowShape[2] = {WS_FHG, WS_FHG}; /* YB : 971113 */
  
  /* VQ: Variables for NTT_VQ decoder */
  ntt_INDEX           ntt_index;
  ntt_INDEX           ntt_index_scl;
  static int          InitFlag = 1;
  int                 isampf;
  int                 ntt_NSclLay, ntt_NSclLayDec;
  PRED_TYPE           pred_type;

  int mat_usedNumBit =0; /* T.Ishikawa 980526 */


  pred_type=tfData->pred_type;
  if(qc_select == NTT_VQ_SYS || qc_select == NTT_VQ){
    ntt_index_scl.nttDataScl = nttData->nttDataScl;
    ntt_index.nttDataBase = nttData->nttDataBase;
    ntt_index.nttDataScl = nttData->nttDataScl;
    block_size_samples = tfData->block_size_samples;
    ntt_index.block_size_samples = block_size_samples;
    ntt_index_scl.block_size_samples = block_size_samples;
    ntt_index.numChannel = max_ch;
    ntt_index_scl.numChannel = max_ch;
  }
  if(qc_select == NTT_VQ_SYS){
    ntt_NSclLay = frameData->od->streamCount.value-1;
    ntt_NSclLayDec = tfData->output_select;
    ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay;
    ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay;
    switch 
      (frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value)
      {
      case 0: isampf=96; break;
      case 1: isampf=88; break;
      case 2: isampf=64; break;
      case 3: isampf=48; break;
      case 4: isampf=44; break;
      case 5: isampf=32; break;
      case 6: isampf=24; break;
      case 7: isampf=22; break;
      case 8: isampf=16; break;
      case 9: isampf=12; break;
      case 10: isampf=11; break;
      case 11: isampf=8; break;
      case 12:
      case 13:
      case 14:
      case 15:
        break;
      }
    ntt_index.isampf = isampf;
    ntt_index_scl.isampf = isampf;
  }
  else if(qc_select == NTT_VQ){
    ntt_NSclLay = ntt_NSclLay_gl;
    ntt_NSclLayDec = ntt_NSclLayDec_gl;
    ntt_index.nttDataScl->ntt_NSclLay = ntt_NSclLay;
    ntt_index_scl.nttDataScl->ntt_NSclLay = ntt_NSclLay;
    isampf = sampling_rate_decoded/1000;
    ntt_index.isampf = isampf;
    ntt_index_scl.isampf = isampf;
  }
  ResetReadBitCnt ( hVm );
  fixed_stream = BsOpenBufferRead(bitBuf);
  /* son_NBCpp */
  for(i_ch=0;i_ch<MAX_TIME_CHANNELS;i_ch++){
    gcBitBuf[i_ch] = BsAllocBuffer(4096);
    gc_WRstream[i_ch] = BsOpenBufferWrite(gcBitBuf[i_ch]);
    gc_stream[i_ch] = BsOpenBufferRead(gcBitBuf[i_ch]);
  } /* for i_ch...*/
  
  /* get sync word */
  /* a 7 bit alternative to the strange MPEG-1 audio syncword */
  /* KEMAL: syncword makes problems with qc_aac, because it
     needs byte_alignment, which would be done in AacDecodeFrame() */
  if (qc_select!=AAC_SCALABLE) { 
    if( (transport_stream != AAC_RAWDATA_TSTREAM) &&
        (transport_stream != NO_TSTREAM) &&
        (transport_stream != NO_SYNCWORD) ) {
      BsGetBit( fixed_stream, &ultmp, 7 );     
      decoded_bits += 7;
      if( ultmp != 0x6e ) {
        CommonExit( 1," Wrong Syncword %d ", ultmp);
      }
    }
  }

  /* inverse Q&C */
  switch(qc_select) {

  case MDCT_VALUES_16BIT: /* debug option unpack MDCT output values */
    {
      double scal;
      unsigned long iscal;
      int i;

      /* get window type */
      BsGetBit( fixed_stream, &ultmp, 4 );
      decoded_bits += 4;
      windowSequence[MONO_CHAN] = (WINDOW_SEQUENCE)ultmp;
      if (mainDebugLevel) {
        printf("\nblocktype %ld",ultmp);
      }
      /* get block scaling factor */
      BsGetBit( fixed_stream, &iscal, 12 );
      decoded_bits += 12;

      scal = pow( 2.0, -(((int)iscal)-2048) );
      for( i=0; i<block_size_samples; i++ ) {
        BsGetBit( fixed_stream, &ultmp, 16 );
        decoded_bits += 16;
        spectral_line_vector[0][MONO_CHAN][i] = (double)(((int)ultmp)-32768) * scal;
      }
    }
    break;
   
  case AAC_QC:
    {
      byte max_sfb[Winds];
      byte dummy[2][MAXBANDS]; /* needed for PNS in scalaeble case ! */

      decoded_bits += AacDecodeFrame ( fixed_stream,
                                       gc_WRstream,
                                       spectral_line_vector[0], 
                                       windowSequence, 
                                       windowShape,
                                       MULTICHANNEL,
                                       max_sfb,
				       numChannels, 
				       0,
				       sfbInfo,dummy,
                                       hResilience,
                                       hVm,
                                       hHcrSpecData,
                                       hHcrInfo, 
                                       hEpInfo,
                                       hConcealment,
                                       qc_select,
				       nok_lt_status); 
    }   
    break;

  case AAC_SCALABLE:
    {
      if  (frameData==NULL){
        CommonExit(1,"no frameData");
      } else {
	aacScaleableFlexMuxDecode ( fixed_stream, 
                                    gc_WRstream,
                                    &windowSequence[MONO_CHAN], 
                                    &decoded_bits, 
                                    spectral_line_vector, 
                                    scalOutSelect,
                                    sfbInfo,
                                    numChannels,
                                    lopLong, 
                                    lopShort, 
                                    normBw,
                                    &windowShape[MONO_CHAN],
                                    qc_select, 
                                    hResilience,
                                    hVm,
                                    hHcrSpecData,
                                    hHcrInfo,
                                    frameData, 
                                    tfData,
                                    nttData,
				    hEpInfo,
                                    hConcealment,
				    nok_lt_status,
				    med_win_in_long,
				    short_win_in_long,
				    pred_type);
      }
    }
    break;
      
  
  case NTT_VQ:
    {
      int ntt_available_bits,tomo_tmp, i, j;
      /* get block type */
      decoded_bits=0;
      BsGetBit( fixed_stream, &ultmp, 2);
      windowSequence[MONO_CHAN] = (WINDOW_SEQUENCE)ultmp;
      windowSequence[max_ch-1] = (WINDOW_SEQUENCE)ultmp;
      ntt_index.w_type=windowSequence[MONO_CHAN];

      decoded_bits += 2;
      BsGetBit( fixed_stream, &ultmp, 1);
      windowShape[MONO_CHAN] = (WINDOW_SHAPE)ultmp;
      windowShape[max_ch-1] = (WINDOW_SHAPE)ultmp;
      decoded_bits += 1; /* window_shape */
      ntt_index.ms_mask = 0;
      ntt_index.group_code = 0x7F ; /*default */
      ntt_index.last_max_sfb[0] =0;
      /* block-wise MS stereo */
      ntt_headerdec( -1, fixed_stream,  &ntt_index,  sfbInfo, &decoded_bits,
                     tns_info,
		     nok_lt_status, pred_type,
		     hResilience,
                     hVm, 
                     hEpInfo );
      ntt_available_bits = /*frameNumBit*/ ntt_index.nttDataBase->ntt_NBITS_FR - decoded_bits;
      if ( mainDebugLevel>2 ) {
        printf("___ntt_available_bits %d %d %d\n",ntt_available_bits,
               ntt_index.nttDataBase->ntt_NBITS_FR,decoded_bits);
      }
      /*--- base decoder ---*/
      {
        float current_frame[ntt_T_FR_MAX];
        int itmp;
        /* bit unpacking */
        tomo_tmp=decoded_bits;

        decoded_bits += ntt_BitUnPack ( fixed_stream, 
                                        ntt_available_bits,
                                        windowSequence[MONO_CHAN], 
                                        &ntt_index );
        if ( mainDebugLevel>2 ) {
          printf("<<<< BaseLayer(OLD) BitUnPack %d\n",
                 decoded_bits -tomo_tmp);
        }
	/* decoding tools*/
	ntt_vq_decoder(&ntt_index, spectral_line_vector[0],  sfbInfo);
	
	/*-- long term predictor --*/
	if(pred_type == NOK_LTP)
          {
            Info *info = sfbInfo[ntt_index.w_type];

            for(i_ch = 0; i_ch < max_ch; i_ch++)
              {
                for (itmp = 0; itmp < block_size_samples; itmp++)
                  current_frame[itmp] = spectral_line_vector[0][i_ch][itmp];
	    
                if(nok_lt_status[i_ch]->sbk_prediction_used[0]) 
                  nok_lt_predict (sfbInfo[ntt_index.w_type], 
                                  ntt_index.w_type, 
                                  windowShape[i_ch],
                                  prev_windowShape[i_ch],
                                  nok_lt_status[i_ch]->sbk_prediction_used,
                                  nok_lt_status[i_ch]->sfb_prediction_used,
                                  nok_lt_status[i_ch], 
                                  nok_lt_status[i_ch]->weight,
                                  nok_lt_status[i_ch]->delay,
                                  current_frame,
                                  block_size_samples,
                                  block_size_samples/med_win_in_long,
                                  block_size_samples/short_win_in_long,
                                  &tns_info[i_ch], qc_select);
	    
                /* This is passed to upper layers. */
                for (itmp = 0; itmp < block_size_samples; itmp++)
                  spectral_line_vector[0][i_ch][itmp] = current_frame[itmp];
	    
                /* TNS synthesis filtering. */  
                for(i = j = 0; i < tns_info[i_ch].n_subblocks; i++)
                  {
                    tns_decode_subblock(current_frame + j,
                                        ntt_index.max_sfb[0],
                                        info->sbk_sfb_top[i],
                                        info->islong,
                                        &(tns_info[i_ch].info[i]),
                                        qc_select);
	      
                    j += info->bins_per_sbk[i];
                  }

                nok_ltp_buf_update(ntt_index.w_type, windowShape[MONO_CHAN], 
                                   prev_windowShape[i_ch],
                                   nok_lt_status[i_ch],
                                   current_frame,
                                   block_size_samples,
                                   block_size_samples/med_win_in_long,
                                   block_size_samples/short_win_in_long,
                                   short_win_in_long);
              }
          }

      }
      /*--- scalable decoders ---*/
      {
        int iscl, iii, jjj;
        for(iii=0; iii<8; iii++){
          for(jjj=0; jjj<ntt_index.max_sfb[0]; jjj++){
            ntt_index_scl.msMask[iii][jjj] = ntt_index.msMask[iii][jjj];
          }
	}
	ntt_index_scl.w_type = windowSequence[MONO_CHAN];
        ntt_index_scl.group_code = ntt_index.group_code ;
        ntt_index_scl.pf = ntt_index.pf ;
        ntt_index_scl.max_sfb[0] = ntt_index.max_sfb[0];
        for (iscl=0; iscl<ntt_NSclLay; iscl++){
          ntt_index_scl.last_max_sfb[iscl+1] = ntt_index_scl.max_sfb[iscl];
          decoded_bits=0;
          ntt_headerdec( iscl, fixed_stream,  &ntt_index_scl,  sfbInfo, 
                         &decoded_bits,
                         tns_info,
			 nok_lt_status, pred_type,
			 hResilience,
                         hVm, 
                         hEpInfo );
          tomo_tmp=decoded_bits;
          ntt_available_bits =  
            ntt_index_scl.nttDataScl->ntt_NBITS_FR_SCL[iscl]-decoded_bits;
          if ( mainDebugLevel>2 ) {
            printf("TTTTTTT available %5d %5d[%d]\n",ntt_available_bits,
                   ntt_index_scl.nttDataScl->ntt_VQTOOL_BITS_SCL[iscl],iscl);
          }
          decoded_bits += ntt_SclBitUnPack(fixed_stream, &ntt_index_scl,
                                           ntt_available_bits, iscl);
          if ( mainDebugLevel>2 ) {
            printf("<<<< ScaleLayer(OLD) [%d] BitUnPack %d %d>>>>\n",
                   iscl,decoded_bits-tomo_tmp, ntt_index_scl.w_type);
          }
          /* decoding tools */
          if (iscl < ntt_NSclLayDec){
            /*NNN
              mat_scale_set_shift_para2(iscl);
            */
	    ntt_scale_vq_decoder(&ntt_index_scl, &ntt_index,
                                 spectral_line_vector[0], iscl,
                                 sfbInfo);
            if ( mainDebugLevel >2) {
              printf("<<<< ScaleLayer(OLD) [%d] Decoding Finished >>>\n",iscl);
            }
          }
        }
      }
#if 1
      {
        int ch;
        for (ch=0;ch<max_ch;ch++){
          int i, k, j, max_sfb[8];
          Info *info = sfbInfo[windowSequence[MONO_CHAN]];
          if(ntt_NSclLayDec==0) max_sfb[0] = ntt_index.max_sfb[0];
          else max_sfb[ntt_NSclLayDec] = ntt_index_scl.max_sfb[ntt_NSclLayDec];

          for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
            float tmp_spec[1024];
            for( k=0; k<info->bins_per_sbk[i]; k++ ) {
              tmp_spec[k] = spectral_line_vector[0][ch][j+k];
            }
            tns_decode_subblock( tmp_spec,
                                 max_sfb[ntt_NSclLayDec],
                                 info->sbk_sfb_top[i],
                                 info->islong,
                                 &(tns_info[ch].info[i]),
                                 qc_select);

            for( k=0; k<info->bins_per_sbk[i]; k++ ) {
              spectral_line_vector[0][ch][j+k] = tmp_spec[k];
            }

            j += info->bins_per_sbk[i];
          }
        }
      }
#endif
      InitFlag = 0;
    }
    break;

  case NTT_VQ_SYS:
    {   
      tvqAUDecode(   numChannels,
                     frameData, /* config data , obj descr. etc. */
                     tfData,
                     hFault,
                     qc_select,
                     &ntt_index, &ntt_index_scl,
                     fixed_stream);
      mat_usedNumBit = tfData->decoded_bits;
      windowSequence[MONO_CHAN]=tfData->windowSequence[MONO_CHAN];
      windowShape[MONO_CHAN] =   tfData->windowShape[MONO_CHAN];
      if(numChannels == 2){
        windowSequence[1]=windowSequence[MONO_CHAN];
        windowShape[1] =  windowShape[MONO_CHAN];
      } 
    }
    break;
  default:
    CommonExit(-1,"\nerror in switch");  
  } /* switch( qc_select ) */

  /* son_NBCpp */
  /* unpack gain_control_data() */
  for(i_ch=0;i_ch<max_ch;i_ch++){
    if (gcBitBuf[i_ch]->numBit > 0) {
      /* decoded_bits = */
      if (mc_info.profile  == SSR_Profile) {
        son_gc_unpack(gc_stream[i_ch], windowSequence[i_ch],
                      &max_band[i_ch], gainInfo[i_ch]);
      }
      else {
          CommonExit ( 2, "DecTfFrame: ssr not allowed in this profile");
      }
    }
  } /* for(i_ch... */
  /* F -> T */ 
  if (mc_info.profile  == SSR_Profile) {
    int band;
    int i_ch; /* 971010 YT*/
    for(i_ch=0;i_ch<max_ch;i_ch++){
      
      son_gc_arrangeSpecDec( spectral_line_vector[0][i_ch],
                             block_size_samples,
                             windowSequence[i_ch],
                             spectral_line_vector_for_gc[i_ch] );

      for (band = 0; band < ssr_decoder_band; band++) {
        freq2buffer(  &spectral_line_vector_for_gc[i_ch][block_size_samples/NBANDS*band],
                      &imdctBufForGC[i_ch][block_size_samples/NBANDS*2*band],
                      &overlap_buffer[i_ch][block_size_samples/NBANDS*band],
                      windowSequence[i_ch],
                      block_size_samples/NBANDS,
                      block_size_samples/NBANDS/med_win_in_long,
                      block_size_samples/NBANDS/short_win_in_long, 
                      windowShape[i_ch],
                      prev_windowShape[i_ch], /* YB : 971113 */
                      NON_OVERLAPPED_MODE,
                      short_win_in_long );
      }
      /* gain compensator */
      son_gc_compensate( imdctBufForGC[i_ch],
                         gainInfo[i_ch],
                         block_size_samples,
                         windowSequence[i_ch],
                         i_ch,
                         DBandSigBufForOverlapping[i_ch],
                         DBandSigBuf[i_ch],
                         ssr_decoder_band );
      /* ipqf */
      son_ipqf_main( DBandSigBuf[i_ch],
                     block_size_samples,
                     i_ch,
                     time_sample_vector[i_ch] );
      prev_windowShape[i_ch] = windowShape[i_ch];
    } /* for i_ch ...*/
  }
  else {
    int i_ch;
    for (i_ch=0; i_ch<max_ch; i_ch++){

      if (pred_type == NOK_LTP && qc_select != AAC_SCALABLE &&
	  qc_select != NTT_VQ && qc_select != NTT_VQ_SYS)
        {
          Float tmpBuffer[2048];
	
          for(i = 0; i < block_size_samples; i++)
            tmpBuffer[i] = spectral_line_vector[0][i_ch][i];
	
          nok_ltp_buf_update(windowSequence[MONO_CHAN], 
                             windowShape[i_ch],
                             prev_windowShape[i_ch], 
                             nok_lt_status[i_ch], 
                             tmpBuffer, /*spectral_line_vector[0][i_ch],*/
                             block_size_samples,
                             block_size_samples/med_win_in_long,
                             block_size_samples/short_win_in_long,
                             short_win_in_long);
        }

      freq2buffer(
                  spectral_line_vector[0][i_ch],
                  time_sample_vector[i_ch],
                  overlap_buffer[i_ch],
                  windowSequence[i_ch],
                  block_size_samples, /* changed to variables (NI) */
                  block_size_samples/med_win_in_long,
                  block_size_samples/short_win_in_long,
                  windowShape[i_ch],
                  prev_windowShape[i_ch],   /* YB : 971113 */
                  OVERLAPPED_MODE,
                  short_win_in_long             /* HP 971023 */
                  );
      prev_windowShape[i_ch] = windowShape[i_ch];
    }
  }

  /* Write accumulation_buffer[0..block_size_samples-1] to pcm output */
  {
    int i, i_ch;
    for (i_ch=0; i_ch<max_ch; i_ch++){
      for( i=0; i<block_size_samples; i++ ) {
        sampleBuf[i_ch][i] = (float)time_sample_vector[i_ch][i];
      }
    }
  }    
  if (qc_select==AAC_QC || qc_select==AAC_BSAC
      ) { /* read align bits */

      while (decoded_bits & ((1<<3)-1)) {
        BsGetBit(fixed_stream,&ultmp,1);
        decoded_bits++;
        if ( decoded_bits != BsCurrentBit ( fixed_stream ) ) {
          CommonExit (1, "DecTfFrame: IMPLEMENTATION ERROR\n" );
        }
      }
  }

  /* T.Ishikawa 980526 */
  if(qc_select==NTT_VQ_SYS)
    {
      *usedNumBit = mat_usedNumBit;
    }
  else
    {
      *usedNumBit = decoded_bits;
    }

  
  BsCloseRemove(fixed_stream,1);
  for (i_ch=0; i_ch<max_ch; i_ch++){
    BsCloseRemove(gc_stream[i_ch],1);
    BsCloseRemove(gc_WRstream[i_ch],1);
    BsFreeBuffer(gcBitBuf[i_ch]);
  }/* for(i_ch...*/
}

void DecTfFrameNew(
                   int          numChannels, 
                   FRAME_DATA*  fd, /* config data ,one bitstream buffer for each layer, obj descr. etc. */
                   TF_DATA*     tfData,
                   LPC_DATA*    lpcData,
                   ntt_DATA*    nttData,
                   HANDLE_FAULT_TOLERANT hFault,
                   QC_MOD_SELECT      qc_select  
                   )
{
  int i_ch;

  ResetReadBitCnt ( hFault->hVm );
  tfScaleableAUDecode ( numChannels,
                        fd, /* static configuration data,
                               one bitstream buffer for each layer,  obj descr. etc. */
                        tfData, /* actual frame input/output data */
                        lpcData,
                        nttData,
                        hFault , /* additional data for error protection, huffman reorder
                                    error conceilment */
                        qc_select );
  
  for (i_ch=0; i_ch<numChannels; i_ch++){
    freq2buffer(tfData->spectral_line_vector[0][i_ch],
                tfData->time_sample_vector[i_ch],
                tfData->overlap_buffer[i_ch],
                tfData->windowSequence[i_ch],
                tfData->block_size_samples, /* changed to variables (NI) */
                tfData->block_size_samples/2,/* not used */
                tfData->block_size_samples/8,
                tfData->windowShape[i_ch],
                tfData->prev_windowShape[i_ch],   
                OVERLAPPED_MODE,
                8);
    tfData->prev_windowShape[i_ch] = tfData->windowShape[i_ch];
  }
}

