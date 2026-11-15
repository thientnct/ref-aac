/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bernhard Grill (University of Erlangen)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.

*/

/* 23-oct-97   HP   merged Nokia's predictor 971013 & 971020 */
/*                  tried to fix PRED_TYPE */
/* 24-oct-97   Mikko Suonio   fixed PRED_TYPE */


#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "tf_main.h"
#include "common_m4a.h"
#include "bitstream.h"
#include "lpc_common.h"
#include "util.h"

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "aac_qc.h"
#include "util.h"
#include "fir_filt.h"
#include "scaleable.h"
#include "dec_tf.h"
#include "enc_tf.h"
#include "bitmux.h"
#include "bitstream.h"
#include "celp_encoder.h"
#include "celp_decoder.h"
#include "flex_mux.h"
#define MAX_TF_LAYERS 5

/* --- TVQ --- */
#include "ntt_conf.h"
#include "ntt_encode.h"
#include "ntt_scale_conf.h"

typedef struct {
  int ch_no_core;
  int ch_no_tf[MAX_TF_LAYERS];
  int ch_no_max;
  int tf_layers;
} OP_MODE_INFO;

OP_MODE_INFO opm_info[16] = {
  { 1, {0,0,0,0,0}, 1, 0 },
  { 2, {0,0,0,0,0}, 2, 0 },
  { 0, {1,0,0,0,0}, 1, 1 },
  { 0, {2,0,0,0,0}, 2, 1 },
  { 1, {1,0,0,0,0}, 1, 1 },
  { 1, {2,0,0,0,0}, 2, 1 },
  { 2, {2,0,0,0,0}, 2, 1 },
  { 1, {1,1,0,0,0}, 1, 2 },
  { 1, {1,2,0,0,0}, 2, 2 },
  { 1, {2,2,0,0,0}, 2, 2 },
  { 2, {2,2,0,0,0}, 2, 2 },
  { 0, {1,1,0,0,0}, 1, 2 },
  { 0, {1,2,0,0,0}, 2, 2 },
  { 0, {2,2,0,0,0}, 2, 2 },
  { 0, {1,1,1,0,0}, 1, 3 },
};

static FIR_FILT *lowpassFilt;
static int  downsamplFac;
static   HANDLE_BSBITBUFFER tmpBitBuf;
static   HANDLE_BSBITBUFFER dynpartBuf;
static   HANDLE_BSBITBUFFER fixedPartBuf[3];
static float *coreDecodedSampleBuf[MAX_TIME_CHANNELS];
static double *diffSpectrum[MAX_TIME_CHANNELS];
static int granule=0;
static int numGranules=3;

static int op_mode = 4;
static int diffContrSimu = 0;
static int intermediate_layers = 0;


/* Prediction */
static PRED_TYPE pred_type = PRED_NONE;

/* MPEG 4 Celp core */
static long  bit_rate, sampling_frequency, frame_size, n_subframes, sbfrm_size, lpc_order;
static long  num_lpc_indices, num_shape_cbks, num_gain_cbks, *org_frame_bit_allocation;
static float min_pitch_frequency, max_pitch_frequency;    
#ifdef CORE_BROKEN	/* HP990409 */
static long ExcitationMode     = MultiPulseExc;
static long SampleRateMode     = fs8kHz;  /* Default: 16 kHz */
static long QuantizationMode   = VectorQuantizer;  /* Default: Scalar Quantizer */
#else
static long ExcitationMode     = 0;
static long SampleRateMode     = 0;
static long QuantizationMode   = 0;
#endif
static long FineRateControl    = 1;  /* Default: FineRateControl is ON */
static long LosslessCodingMode = 0;  /* Default: Lossless coding is OFF */
static long RPE_configuration;
static long Wideband_VQ = 0;
static long NB_Configuration;
static long NumEnhLayers;
static long BandwidthScalabilityMode;
static long BWS_configuration;
static long BWS_nb_bitrate;

static long n_lpc_analysis;
static long *window_offsets;
static long *window_sizes;
static long n_lag_candidates; 

/* NTT Twin VQ */
static int bitsPerFrame;

/* added by NTT 20001121 */
#ifdef ESC_SUPPORT
void ntt_writeFlexMuxPDU(
   int index,
   HANDLE_BSBITSTREAM bitStream,
   int vq_bits,
   HANDLE_BSBITBUFFER  AUBuffer,
   int idx_base
);
#endif

static	void *InstanceContextLPCEnc;	/* one instance context */
static	void *InstanceContextLPCDec;	/* one instance context */

static long samplRateCore;

extern int samplFreqIndex[];

int aacScaleEncInit( 
  HANDLE_BSBITSTREAM headerStream,
  char        *encPara,		 /* in: encoder parameter string */
  int         numChannel,
  int         frameNumSample,
  long        samplRate,
  ENC_FRAME_DATA* frameData,
  enum CORE_CODEC coreCodecIdx,
  int mode,
  PRED_TYPE predictor_type,
  ntt_INDEX* ntt_index,
  ntt_INDEX* ntt_index_scl,
  int mainDebugLevel
  )
{
  int frameNumSampleCore;
  
  
  int ch, i, core_delay;
  char *pc;
  /* added by NTT 001204 */
#ifdef ESC_SUPPORT
  int tvq_ep_base=0, tvq_ep_enh=0;
#endif

  pred_type = predictor_type;
  
  core_delay = 0;

  if( (pc=strstr( encPara, "-aac_simulcast" )) ) {
    sscanf( pc+14, "%1d", &diffContrSimu );
    if (diffContrSimu)
      fprintf (stdout,"aac scalable mode : using simulcast (no difference coding)\n");
    else
      fprintf (stdout,"aac scalable mode : using difference coding  for all bands \n");
  }
  if (frameData){
    op_mode=frameData->opMode;
  } else {
    op_mode=mode;
  }
  switch( op_mode ) {
    case 1:
    case 3:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
      if( numChannel < 2 ) {
        CommonExit(1,"Encode scalab.: Stereo scalability mode selected, but mono input file");
      }
  }


  /* write object descriptor in case of aac_sys */
  if( frameData!=NULL ) {
    int sx;
    int layer, numLayers;
    int coreLayer;

    coreLayer = (opm_info[op_mode].ch_no_core==0)?0:1;

#ifdef ESC_SUPPORT
    /* added by NTT 001201 */
    if ((pc=strstr( encPara, "-tvq_ER" ))) {
      if((pc=strstr( encPara, "-tvq_ep1" ))){
	coreLayer += (opm_info[op_mode].ch_no_core==0)?0:1;
	tvq_ep_base=1;
      }
    }
#endif

    if(coreCodecIdx==NTT_TVQ){
     coreLayer += ntt_index_scl->nttDataScl->ntt_NSclLay;
#ifdef ESC_SUPPORT
     /* added by NTT 001201 */
     if ((pc=strstr( encPara, "-tvq_ER" ))) {
       if((pc=strstr( encPara, "-tvq_ep1" ))){
	 coreLayer += ntt_index_scl->nttDataScl->ntt_NSclLay;
	 tvq_ep_enh=1;
       }
     }
#endif
    }

    /* get the total number of layers: AAC + core */
    numLayers = opm_info[op_mode].tf_layers + coreLayer;
    initObjDescr(frameData->od,mainDebugLevel);
    presetObjDescr(frameData->od, numLayers);
    frameData->od->streamCount.value=numLayers;

    /* added by NTT 001201 */
#ifdef ESC_SUPPORT
    if ((tvq_ep_base==1) && (tvq_ep_enh==1))
      for(layer=coreLayer/2-1; layer>=0; layer--){
	frameData->layer[layer*2+1].sampleRate = frameData->layer[layer].sampleRate;
	frameData->layer[layer*2].sampleRate = frameData->layer[layer].sampleRate;
      }
#endif      

    /* write ES descriptor of core layer */

    for(layer = 0; layer<coreLayer; layer++) {
      initESDescr( &(frameData->od->ESDescriptor[layer])); /* only TF coders */
      presetESDescr( frameData->od->ESDescriptor[layer], layer);

      /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
      if((tvq_ep_base==1) && (tvq_ep_enh==1)) { 
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.epConfig.value=1;
      }
#endif      
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value=
      frameData->layer[layer].bitRate;

      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = TWIN_VQ; /* all tvq scaleable */


#ifdef VERSION2      
      if ((pc=strstr( encPara, "-tvq_ER" ))) {
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_TWIN_VQ; /* all tvq scaleable */
      }
#endif      

      for (sx=0;sx<16;sx++) { 
	if (frameData->layer[layer].sampleRate== samplFreqIndex[sx]) break;
      };
      /* HP20001012 samplingFreqencyIndex==0xf not handled */
      if (sx >= 16) CommonExit(-1,"\nSampling rate not supported");

      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = sx;  
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value=6122;

      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value=0;/* 1024 */
    if ((pc=strstr( encPara, "-tvq_960" ))) {
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value=1;/* 960 */
    }
    frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value=numChannel;

#ifdef AL_CONFIG
	  /* Ad-hoc 981105 */
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;
#endif
    }

    for(layer=coreLayer; layer<numLayers; layer++) {
      initESDescr( &(frameData->od->ESDescriptor[layer]));
      presetESDescr( frameData->od->ESDescriptor[layer], layer);
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value=frameData->layer[layer].bitRate;
      if(tvq_debug_level>2)
          fprintf(stderr, "UUUUUUUUYY %5d %5ld \n",
          layer, frameData->od->ESDescriptor[layer]->DecConfigDescr.\
                 avgBitrate.value);
      for (sx=0;sx<16;sx++) { if (frameData->layer[layer].sampleRate== samplFreqIndex[sx]) break;} ; 
      /* HP20001012 samplingFreqencyIndex==0xf not handled */
      if (sx >= 16) CommonExit(-1,"\nSampling rate not supported");
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = sx; 
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value=6122;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = AAC_SCAL; /* AAC scalable */

      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value=0;/* 0: 1023; 1: 960 */
    if ((pc=strstr( encPara, "-tvq_960" ))) {
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value=1;/* 960 */
    }
/* YB 991122 */
    frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value=numChannel;
#ifdef AL_CONFIG
	  /* Ad-hoc 981105 */
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
	  frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;
#endif

    }

    advanceODescr (  headerStream, frameData->od,1) ;
    for (layer=0; layer<numLayers; layer++) {
      advanceESDescr ( headerStream, frameData->od->ESDescriptor[layer] ,1) ;      
    }

  }
  /* allocate memory */
  tmpBitBuf = BsAllocBuffer(8000);

  for(i=0;i<numGranules;i++){
    fixedPartBuf[i] =BsAllocBuffer(300);
  }
  dynpartBuf =BsAllocBuffer(numGranules*6000);

  for (ch=0; ch<numChannel; ch++){
    if ((coreDecodedSampleBuf[ch]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode scalab.: memory allocation error");
    if ((diffSpectrum[ch]=(double*)malloc(frameNumSample*sizeof(double)))==NULL)
      CommonExit(1,"Encode scalab.: memory allocation error");
  }

  /* core coder initialization */
  samplRateCore = 8000;
  if( (samplRate%11025) == 0 ) {
    samplRateCore = 44100/6;
  }


  switch( coreCodecIdx ) {
   case CC_CELP_MPEG4_60:
    {

#ifdef CORE_BROKEN
     BITSTREAM *p_bitstream;
     int bit_rate = 6000;
     HANDLE_BSBITBUFFER coreBitBuf;
     
     lowpassFilt = initFirLowPass( samplRate/4000, 120 );
     downsamplFac = samplRate/samplRateCore;
     coreBitBuf = BsAllocBuffer(2000);

     if((p_bitstream=(BITSTREAM *)malloc(sizeof(BITSTREAM)))==NULL) {
       CommonExit(1, "Memory allocation error for enc_lpc init in scal_enc_frame.c");
     }
     
     p_bitstream->p_bitstream_buffer_start = coreBitBuf->data;
     p_bitstream->buffer_length            = ((coreBitBuf->size)+7)/8;
     p_bitstream->valid_bits               = 0;
     p_bitstream->start_offset             = 0;
     
     SampleRateMode           = fs8kHz;
     QuantizationMode         = VectorQuantizer;
     FineRateControl          = OFF;
     LosslessCodingMode       = OFF;
     NumEnhLayers             = 0;
     BandwidthScalabilityMode = OFF;
     Wideband_VQ              = OFF;
     BWS_nb_bitrate           = 0;

     celp_initialisation_encoder ( 
       p_bitstream,                 /*1                     */   
       bit_rate,		    /*2in: bit rate */
       samplRateCore,               /*3in: sampling frequency */
       ExcitationMode,	            /*In: Excitation Mode */
       SampleRateMode,              /*4In: SampleRate Mode               */
       QuantizationMode,            /*5In: Type of Quantization          */
       FineRateControl,             /*6In: Fine Rate Control switch      */
       LosslessCodingMode,          /*7In: Lossless Coding Mode          */
       &RPE_configuration,           /*8In: RPE_configuration        */
       Wideband_VQ,                 /*9Out: Wideband VQ mode             */
       &NB_Configuration,           /*0Out: Narrowband configuration     */
       NumEnhLayers,                /*1In: Number of Enhancement Layers  */
       BandwidthScalabilityMode,    /*2In: bandwidth switch              */
       &BWS_configuration,          /*3Out: BWS_configuration            */
       BWS_nb_bitrate,              /*4In: narrowband bitrate for BWS    */
       &frame_size, 		/*5out: frame size */
       &n_subframes,	    /*6out: number of subframes */
       &sbfrm_size,	    /*7out: subframe size */
       &lpc_order,			/*8out: LP analysis order */
       &num_lpc_indices,	/*9out: number of LPC indices */
       &num_shape_cbks,	/* out:0number of shape indices */
       &num_gain_cbks,	    /*1out: number of gain indices */
       &n_lpc_analysis,    /*2out: number of LP analysis per frame */
       &window_offsets,    /*3out: window size for each LP analysis */
       &window_sizes,      /*4out: window offset for each LP analysis */
       &n_lag_candidates,	/*5out: number of pitch candidates */
       &min_pitch_frequency,    /*6out: minimum pitch frequency */
       &max_pitch_frequency,    /*7out: maximum pitch frequency */
       &org_frame_bit_allocation, /*8out: bit number for each index */
       InstanceContextLPCEnc /*9in/out: instance context */
     );

     celp_initialisation_decoder (
       p_bitstream,
       bit_rate,		 /* in: bit rate */
       0,                   /* In: complexity level decoder*/
       0,                   /* In: reduced order decoder   */
       0,                   /* DecEnhStage */
       0,                   /* DecBwsMode */
       0,                   /* disable postfilter */
       &frame_size, 	 /* out: frame size */
       &n_subframes,	 /* out: number of  subframes */
       &sbfrm_size,	 /* out: subframe size */ 
       &lpc_order,		 /* out: LP analysis order */
       &num_lpc_indices,	 /* out: number of LPC indices */
       &num_shape_cbks,	 /* out: number of shape indices */
       &num_gain_cbks,	 /* out: number of gain indices */
       &org_frame_bit_allocation, /* out: bit number of each index */
       &ExcitationMode,	           /* Out: Excitation Mode */
       &SampleRateMode,            /* Out: SampleRate Mode            */    
       &QuantizationMode,          /* Out: Type of Quantization   */        
       &FineRateControl,           /* Out: Fine Rate Control switch*/       
       &LosslessCodingMode,        /* Out: Lossless Coding Mode   */        
       &RPE_configuration,          /* Out: RPE_configuration */        
       &Wideband_VQ,               /* Out: Wideband VQ mode           */
       &NB_Configuration,          /* Out: Narrowband configuration*/       
       &NumEnhLayers,              /* Out: Number of Enhancement Layers */
       &BandwidthScalabilityMode,  /* Out: bandwidth switch           */    
       &BWS_configuration,          /* Out: BWS_configuration              */
       InstanceContextLPCDec,       /* in/out: instance context */
       0
     );

     core_delay = 660;

     free(p_bitstream);
     BsFreeBuffer( coreBitBuf );
#endif

    }
    break;
  case NTT_TVQ:
    /* only for TVQ core coder with AAC enhancement layers 
       */
    {

      float bit_rate_scl = 0.0F;
      /*long numChannel = 1;*/
      int short_win_in_long;
      bit_rate = frameData->layer[0].bitRate;
      fprintf(stdout,"\ncore bitrate: %ld  kbit/sec \n",bit_rate);
      core_delay = 0;
		 short_win_in_long=8;
      frameNumSampleCore = frameNumSample;
#if 0
      if ((frameNumSampleCore != 1024) ||
          (short_win_in_long !=8)){
	  fprintf(stderr, "%5d %5d \n", frameNumSampleCore,short_win_in_long);
        CommonExit( 1, "Initialization of TVQ core failed (parameters incompatible with AAC settings)");
       }
#endif
    }
    break;
  case NO_CORE:
    core_delay=0;
    break;     
  default:
    CommonExit( 1, "Encode scalab. init: core coder not yet supported" );
  }

  return( core_delay );
}

/********************************************************************************/

/********************************************************************************/

static void aacEncodeScalFrameHeader(
  HANDLE_BSBITSTREAM fixedStream,
  int             op_mode,
  enum CORE_CODEC core_cc,
  long            samplRate,
  long            bitRate,
  int             intermediate_layers,
  int             mainDataBegin,
  ENC_FRAME_DATA* frameData
)
{
    BsPutBit( fixedStream, 0x37 , 7 );        /* syncword */

    BsPutBit( fixedStream, op_mode , 4 );     /* opMode */

    if( opm_info[op_mode].ch_no_core > 0 ) {
      BsPutBit( fixedStream,  core_cc , 4 );    /* coreCodecIdx*/
    }
    
    if( ( op_mode >= 7 ) && ( op_mode <= 13 ) ){
      BsPutBit( fixedStream, intermediate_layers, 2 );        /* number of intermediate layers */
    }
    
    BsPutBit( fixedStream, getSamplIdx(samplRate) , 4 ); /* mainSamplingFreqIdx */
    
    BsPutBit( fixedStream, getBitrateIdx(bitRate)  , 4 );/* bitrateIndex */
    
    BsPutBit( fixedStream, 0 , 1 );  /* paddingBit */
    BsPutBit( fixedStream, 1 , 1 );  /* protectionBit  */
    BsPutBit( fixedStream, mainDataBegin , 10 ); /* mainDataBegin */
}

/********************************************************************************/

/********************************************************************************/

static void EncodeCore(
  HANDLE_BSBITSTREAM fixedStream,
  float p_time_hsf[],
  float p_time_decoded[],
  int blockSizeSamples,
  int downsamplFac,
  enum CORE_CODEC coreCodecIdx
)
{
  int postProcMode = 1; /* don't use the postproc filter in g729 decoder for use with high layer signal*/
  int numSampleCore = blockSizeSamples;
  float *coreSampleBuf[MAX_TIME_CHANNELS], core_buff[4096], *p_out[MAX_TIME_CHANNELS];
  HANDLE_BSBITBUFFER coreBitBuf;

  coreSampleBuf[MONO_CHAN] = core_buff;
  p_out[MONO_CHAN]         = p_time_decoded;

  coreBitBuf = BsAllocBuffer(2000);

  firLowPass( p_time_hsf, coreSampleBuf[MONO_CHAN], blockSizeSamples, lowpassFilt );
  subSampl( coreSampleBuf[MONO_CHAN], coreSampleBuf[MONO_CHAN], downsamplFac, &numSampleCore );

  switch( coreCodecIdx ) {
   case CC_CELP_MPEG4_60:
    {

#ifdef CORE_BROKEN
     BITSTREAM bitstream;

     bitstream.p_bitstream_buffer_start = coreBitBuf->data;
     bitstream.buffer_length            = ((coreBitBuf->size)+7)/8;
     bitstream.start_offset             = 0;
     bitstream.valid_bits               = 0;

     celp_coder(
       coreSampleBuf,            /* in: input speech */ 
       &bitstream,               /* out: coded bitstream */
       sampling_frequency,       /* in: sampling frequency */
       bit_rate,	         /* in: bit rate */
       ExcitationMode,
       SampleRateMode,
       QuantizationMode,
       FineRateControl,
       LosslessCodingMode,
       RPE_configuration,
       Wideband_VQ,
       NB_Configuration,
       NumEnhLayers,
       BandwidthScalabilityMode,
       BWS_configuration,
       0,
       frame_size,           /* in: frame size */
       n_subframes,          /* in: number of subframes */
       sbfrm_size,           /* in: subframe size */
       lpc_order,            /* in: order of LPC */
       num_lpc_indices,      /* in: number of LPC indices */
       num_shape_cbks,       /* in: number of shape codebooks */
       num_gain_cbks,        /* in: number of gain codebooks */
       n_lpc_analysis,       /* in: number of LP analysis per frame */
       window_offsets,       /* out: window size for each LP analysis */
       window_sizes,         /* out: window offset for each LP analysis */
       n_lag_candidates,     /* in: number of pitch candidates */
       min_pitch_frequency,  /* in: minimum pitch frequency */
       max_pitch_frequency,  /* in: maximum pitch frequency */
       org_frame_bit_allocation, /* out: bit number for each index */
       InstanceContextLPCEnc /* in/out: instance context */
     );

     BsBufferManipulateSetNumBit(bitstream.valid_bits,coreBitBuf);
     BsPutBuffer( fixedStream, coreBitBuf );

     celp_decoder (
       &bitstream,        /* in: received bitstream */
       p_out,             /* out: decoded signal (multi-channel) */
       ExcitationMode,	            /* In: Excitation Mode */
       SampleRateMode,              /* In: SampleRate Mode            */
       QuantizationMode,            /* In: Type of Quantization       */
       FineRateControl,             /* In: Fine Rate Control switch   */
       RPE_configuration,            /* In: RPE_configuration     */
       Wideband_VQ,                 /* In: Wideband VQ mode           */
       NB_Configuration,            /* In: Narrowband configuration   */
       BandwidthScalabilityMode,    /* In: bandwidth switch           */
       frame_size,        /* in: frame size */
       n_subframes,       /* in: number of subframes */
       sbfrm_size,        /* in: subframe size */
       lpc_order,         /* in: order of LPC */
       num_lpc_indices,   /* in: number of LPC indices */
       num_shape_cbks,    /* in: number of shape codebooks */
       num_gain_cbks,     /* in: number of gain codebooks */
       org_frame_bit_allocation,  /* in: bit number for each index */
       InstanceContextLPCDec /* in/out: instance context */
     );
#endif

    }
    break;

   default:
     CommonExit( 1, "Encode scalab.: core coder not yet supported" );
  }

  BsFreeBuffer( coreBitBuf );
}

/********************************************************************************/

/********************************************************************************/

#define FSS_BANDS 16
typedef struct {
  int code[FSS_BANDS];
  int length[FSS_BANDS];
}FSS_CODE;

static FSS_CODE fssCode={
  {0,20,21,22,23,24,25,8,9,26,27,28,29,30,31,1},
  {2, 5, 5, 5, 5, 5, 5,4,4, 5, 5, 5, 5, 5, 5, 2}
};

static void aacEncodeFssData(
  HANDLE_BSBITSTREAM fixedStream,
  WINDOW_SEQUENCE      windowSequence,
  unsigned int     numDiffCtrlBands,
  enum DC_FLAG     FssControl[],
  enum JS_MASK     ms_mask[]
)
{
  int i;

  if( windowSequence != EIGHT_SHORT_SEQUENCE ) {
    if( !ms_mask ) {
      unsigned int fssGroup;
      unsigned int fssNoGroups = numDiffCtrlBands >> 2 ;
      for( fssGroup=0; fssGroup<fssNoGroups;  fssGroup++ ) {
        unsigned int group = 0;
        for( i=0; i<4; i++ ) {
          group <<= 1;
          if( FssControl[(fssGroup<<2)+i] == DC_SIMUL ) {
            group |= 0x1;
          }
        }
        BsPutBit( fixedStream, fssCode.code[group], fssCode.length[group] ); 
      }
    } else {
      for( i=0; (unsigned int) i < numDiffCtrlBands; i++ ) {
        if( ms_mask[i] == JS_MASK_OFF ) {
          BsPutBit( fixedStream, FssControl[i], 1 );
        }
      }
    }
  } else {
    for( i=0; i<8; i++ ) {
      BsPutBit( fixedStream, FssControl[i], 1 ); 
    }
  }
}

/********************************************************************************/

/********************************************************************************/

/* FSS: switch decision */  

static void CalcFssControl(
  double       p_core[],
  double       p_full[],
  double       p_diff[],
  WINDOW_SEQUENCE  blockType,
  enum DC_FLAG FssControl[],
  int          blockSizeSamples,
  int          sfb_width_table[],
  long         samplRate,
  int diffContrSimu
)
{
  int win, no_win, i, offset, sb, no_spec_coeff, *diffBandWidth;
  int numDiffBands = 0;
  int FssBwShort;
  double e_full[SFB_NUM_MAX];
  double e_diff[SFB_NUM_MAX];
#ifdef DEL_ADJ
  static double avg_gain[SFB_NUM_MAX];
  static int _cnt;
#endif

  for (i=0;i<blockSizeSamples ;i++) {  /* for now just always the difference signal */
    p_diff[i] = p_full[i] - p_core[i];
  }

  if( blockType == EIGHT_SHORT_SEQUENCE ) {
    no_win        = 8;
    no_spec_coeff = blockSizeSamples/no_win;
    numDiffBands  = 1;
    diffBandWidth = &FssBwShort;


    switch( samplRate ) {
     case 48000:
     case 44100: 
      FssBwShort = 18; break;
     case 32000: 
      FssBwShort = 28; break;
     case 22050:
     case 24000:
      FssBwShort = 36; break;
     case 16000:
      FssBwShort = 56; break;
    }

    for( i=0; i<8; i++ ) {  /* always diff for short for now */
      if (diffContrSimu){
        FssControl[i] = DC_SIMUL;
      } else {
        FssControl[i] = DC_DIFF;
      }
    }
   } else {
    no_win        = 1;
    no_spec_coeff = blockSizeSamples;
    diffBandWidth = sfb_width_table;

    switch( samplRate ) {
     case 48000 :
     case 44100 : 
      numDiffBands = 20; break;
     case 32000 : 
      numDiffBands = 24; break;
     case 22050 :
     case 24000 :
      numDiffBands = 32; break;
     case 16000 :
      numDiffBands = 32; break;
    default :
      CommonExit(-1,"\nerror in CalcFssControl");
      break;
    }

    for( sb=0; sb<numDiffBands; sb++ ) {
      if (diffContrSimu){
        FssControl[sb] = DC_SIMUL;
      } else {
        FssControl[sb] = DC_DIFF;
      }
    }

    offset = 0;
    for( sb=0; sb<numDiffBands; sb++ ) {
      e_full[sb] = 0;
      e_diff[sb] = 0;

      for( i=0; i<sfb_width_table[sb]; i++ ) {
        e_full[sb] +=  p_full[offset] * p_full[offset];
        e_diff[sb] +=  p_diff[offset] * p_diff[offset];
        offset++;
      }
      if( e_diff[sb] >= e_full[sb] ) {
        FssControl[sb] = DC_SIMUL;
      }
#ifdef DEL_ADJ
      if( log10(e_full[sb] + FLT_MIN) > 0 ) {
        avg_gain[sb] += log10(e_full[sb] + FLT_MIN) - log10(e_diff[sb] + FLT_MIN);
      }
#endif
    }
    
#ifdef DEL_ADJ
    printf( "\nE_full :  " );
    for( sb=0; sb<numDiffBands; sb++ ) {
      printf( "%8.3f", 10*log10(e_full[sb] + FLT_MIN) );
    }
    printf( "\nE_diff :  " );
    for( sb=0; sb<numDiffBands; sb++ ) {
      printf( "%8.3f", 10*log10(e_diff[sb] + FLT_MIN) );
    }
    printf( "\nA_gain :  " );
    for( sb=0; sb<numDiffBands; sb++ ) {
      printf( "%8.3f", 10*avg_gain[sb]/_cnt );
    }
    _cnt++;
    printf( "\n" );
#endif
  }
 
  for( win=0; win<no_win; win++ ) {
    int offset   = win*no_spec_coeff;
    int dcf_offs = win;
    int sum      = 0;
    for( sb=0; sb<numDiffBands; sb++ ) {
      if( FssControl[dcf_offs+sb] == DC_SIMUL ) {
        for( i=0; i<diffBandWidth[sb]; i++ ) {
          p_diff[offset+i] = p_full[offset+i];
        }
      }
      offset += diffBandWidth[sb];
      sum    += diffBandWidth[sb];
    }
    for( i=0; i<(no_spec_coeff-sum); i++ ) {
      p_diff[offset+i] = p_full[offset+i];
    }
  }
}

/********************************************************************************/

/********************************************************************************/

static void aacEncodeScalHeader(
  HANDLE_BSBITSTREAM fixed_stream,
  HANDLE_BSBITBUFFER gcBitBufCh,   /* bit buffer for gain_control_data() */
  WINDOW_SEQUENCE  windowSequence,
  int          max_sfb,
  int          ch_no_layer,   /* no of channels in the current layer */
  int          ch_no_max,     /* no of channels of the last layer */
  int          ch_no_core,
  long         samplRate,
  enum DC_FLAG FssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX],
  enum JS_MASK ms_mask[],
  int          num_window_groups,
  int          window_group_length[],
  TNS_INFO     *tnsInfo,
  ENC_FRAME_DATA* frameData,
  QC_MOD_SELECT qc_select,
  NOK_LT_PRED_STATUS* nok_lt_status)
{
  int ch;
  int reducedInfo=0;
  int tvq_tns_present = 0; /* <- to be implemented */
  int iscl=0;
#if 0 
  if ( frameData && (frameData->od->streamCount.value > 1) 
       && 
(frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==0) 
       && 
      (frameData->od->ESDescriptor[iscl]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == TWIN_VQ) ) {
    /* core is a TwinVQ */
    fprintf(stderr, "TWINVQ \n");
    reducedInfo=1;
  }
#endif
  write_ics_info( max_sfb, windowSequence, 
                  fixed_stream, WS_FHG, 
                  num_window_groups, 
                  window_group_length,
                  reducedInfo,
                  qc_select,
                  pred_type, 
                  (nok_lt_status) ? nok_lt_status : NULL,
		  (nok_lt_status) ? nok_lt_status + 1 : NULL,
		  (ch_no_layer == 2));

  if( ch_no_layer > 1 ) {
    BsPutBit( fixed_stream, 2, 2 ); /*all bands MS */
  }

  
  /* tns_channel_mono_layer data */
  if(tvq_tns_present)
    BsPutBit(fixed_stream, 0, 1);
    

  for( ch=0; ch<ch_no_max; ch++ ) {

    /* TNS data */
    if (tnsInfo) {              /* TNS active */
      write_tns_data( tnsInfo, windowSequence, fixed_stream, 1 );
    } else {                    /* switch it off */
      BsPutBit(fixed_stream,0,1);
    }
    
    /* FSS data */
    if( (ch_no_core > 0) && (ch < ch_no_layer) ) { 
      int fss_bands = 0;
      if( windowSequence == EIGHT_SHORT_SEQUENCE) {
        fss_bands = 8;
      } else {
        switch (samplRate) {
        case 48000: fss_bands = 20; break;
        case 32000: fss_bands = 24; break;
        case 24000: fss_bands = 32; break;
        case 16000: fss_bands = 32; break;
        default: CommonExit( 1, "scalable encoder doesn't support this sampling rate currently" );
        }
      }
      aacEncodeFssData( fixed_stream, windowSequence, fss_bands, FssControl[ch], NULL );
    } else { /* if (ch_no_core >0) */

      /* LTP data */
      if(pred_type==NOK_LTP)
        {fprintf(stderr, "NNNN LTP active \n");
	nok_ltp_encode(fixed_stream, windowSequence, max_sfb,
                       nok_lt_status + ch, qc_select, NO_CORE, -1);
      }
      else
        BsPutBit( fixed_stream, 0, 1); /* no LTP in use */
    } 
  }
}


/********************************************************************************/

/********************************************************************************/

static void aacEncodeLayerHeader(
  HANDLE_BSBITSTREAM fixed_stream,
  WINDOW_SEQUENCE  windowSequence,
  int          max_sfb,
  int          max_sfb_prev_layer,
  enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX],
  enum JS_MASK ms_mask[],
  int          mono_stereo_flag,
  int          ch_no_layer,    /* no of channels in the current layer */
  ENC_FRAME_DATA* frameData
)
{
  if( windowSequence == EIGHT_SHORT_SEQUENCE ){
    BsPutBit( fixed_stream, max_sfb, 4 ) ;/* max_sfb*/
  } else {
    BsPutBit( fixed_stream, max_sfb, 6 ) ;/* max_sfb */
  }

  /* ms mask */
  if( ch_no_layer > 1 ) {
    BsPutBit( fixed_stream, 2, 2 ); /*all bands MS */
  }

  if( mono_stereo_flag ) {
    int ch;
    for( ch=0; ch<2; ch++ ) {
      aacEncodeFssData( fixed_stream, windowSequence, max_sfb_prev_layer, msFssControl[ch], ms_mask );
    }
  }

}


/********************************************************************************/

/********************************************************************************/

int aacScaleableEncode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  blockType[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_block_bits,			
  int         available_bitreservoir_bits,
  int         padding_limit,
  HANDLE_BSBITSTREAM mainStream,
  HANDLE_BSBITSTREAM var_stream,
  int         nr_of_chan,
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         useShortWindows,
  WINDOW_SHAPE     windowShape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         frameMaxNumBit,
  float       **timeSig,
  long        bitRate,
  long        samplRate,
  QC_MOD_SELECT qc_select,
  TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
  ENC_FRAME_DATA* frameData,
  enum CORE_CODEC coreCodecIdx
) 
{
  static int codedFixedBits;
  static HANDLE_BSBITSTREAM dynpartStream;
  int dynpartBits;
  int i, lay, ch;
  int frameBits, rem_bits;
  OP_MODE_INFO omi;
  enum DC_FLAG FssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  HANDLE_BSBITSTREAM fixedStream;
  enum JS_MASK ms_mask[SFB_NUM_MAX];
  int num_window_groups, window_group_length[8];


  if(pred_type == NOK_LTP)
    pred_type = PRED_NONE;
  
  memcpy( &omi, &opm_info[op_mode], sizeof(OP_MODE_INFO) );
  /* intermediate_layers = (op_mode >= 7) ? 1 : 0; */
  omi.tf_layers += intermediate_layers;

  /* start of bitstream composition */
  frameBits = bitRate * blockSizeSamples /samplRate;
  fixedStream = BsOpenBufferWrite(fixedPartBuf[granule]);

  if( granule == 0 ) {
    codedFixedBits = 0;

    dynpartStream = BsOpenBufferWrite( dynpartBuf );
    if (frameData==NULL){
      aacEncodeScalFrameHeader( fixedStream, op_mode, coreCodecIdx, samplRate, bitRate, intermediate_layers, 0 ,frameData);
    }
  }
  
  /* ms matrix */
  if( omi.ch_no_max == 2 ) {  /* always MS for now */
    for( i=0; i<blockSizeSamples; i++ ) {
      double dtmp = p_spectrum[0][i] + p_spectrum[1][i];
      p_spectrum[0][i] = (p_spectrum[0][i] - p_spectrum[1][i]) * 0.5;
      p_spectrum[1][i] = dtmp * 0.5;
    }
    for( i=0; i<SFB_NUM_MAX; i++ ) {
      ms_mask[i] = JS_MASK_MS;
    }
  }

  /* Core Coder */
  if( omi.ch_no_core > 0 ) {
    double coreSpectrum[MAX_TIME_CHANNELS][2048];

    if( omi.ch_no_max > omi.ch_no_core ) {
      for( i=0; i<blockSizeSamples; i++ ) {
        for( ch=1; ch<omi.ch_no_max; ch++ ) {
          timeSig[MONO_CHAN][i] += timeSig[ch][i];
        }
        timeSig[MONO_CHAN][i] *= 0.5;
      }
      for( ch=1; ch<omi.ch_no_max; ch++ ) {   /* copy S-spectrum */
        for( i=0; i<blockSizeSamples; i++ ) {
          diffSpectrum[ch][i] = p_spectrum[ch][i];
        }
      }
    }
    EncodeCore( fixedStream, timeSig[MONO_CHAN], 
                coreDecodedSampleBuf[MONO_CHAN], 
                blockSizeSamples, downsamplFac,
                coreCodecIdx);

    mdct_core(NULL,
              coreDecodedSampleBuf[MONO_CHAN], 
              coreSpectrum[MONO_CHAN],
              0, 
/*               samplRate,  */
              blockSizeSamples );

    if (tnsInfo) {

      int sfb, k, sfb_offset[ 100 ];
      
      /* calc. sfb offset table */
      for (sfb=k=0; sfb<nr_of_sfb[MONO_CHAN]; sfb++) {
        sfb_offset[sfb] = k;
        k += sfb_width_table[MONO_CHAN][sfb];
      }
      sfb_offset[sfb] = k;
      
      TnsEncode2(nr_of_sfb[MONO_CHAN],                  /* Number of bands per window */
                 nr_of_sfb[MONO_CHAN],                  /* max_sfb */ 
                 blockType[MONO_CHAN],                  /* block type */
                 sfb_offset,                            /* Scalefactor band offset table */
                 coreSpectrum[MONO_CHAN],               /* Spectral data array */
                 tnsInfo[MONO_CHAN],                    /* TNS info */
		 0);                                    /* Analysis filter. */
    }

    CalcFssControl( coreSpectrum[MONO_CHAN], p_spectrum[MONO_CHAN], diffSpectrum[MONO_CHAN], blockType[MONO_CHAN],
                    FssControl[MONO_CHAN], blockSizeSamples, sfb_width_table[MONO_CHAN], samplRate ,diffContrSimu);
  } else {
    for( ch=0; ch<omi.ch_no_max; ch++ ) {
      for( i=0; i<blockSizeSamples; i++ ) {
        diffSpectrum[ch][i] = p_spectrum[ch][i];
      }
    }
  }

  /* first T/F layer */
  if (blockType[MONO_CHAN] == EIGHT_SHORT_SEQUENCE) {
    num_window_groups=4;
    window_group_length[0] = 1;
    window_group_length[1] = 2;
    window_group_length[2] = 3;
    window_group_length[3] = 2;
  } else {
    num_window_groups = 1;
    window_group_length[0] = 1;
  }

  aacEncodeScalHeader( fixedStream, NULL, blockType[MONO_CHAN],
                       blockType[MONO_CHAN]==EIGHT_SHORT_SEQUENCE ? nr_of_sfb[MONO_CHAN]*num_window_groups : nr_of_sfb[MONO_CHAN] /*max_sfb*/,
                       omi.ch_no_tf[0], omi.ch_no_max, omi.ch_no_core,
                       samplRate, FssControl, ms_mask, num_window_groups, window_group_length, tnsInfo ? tnsInfo[MONO_CHAN]:NULL ,frameData, qc_select, NULL);

  rem_bits = frameBits - BsCurrentBit(fixedStream) - (7/numGranules+1); /* reserve bits for byte allignment */

  {
    int _nr_of_sfb[MAX_TIME_CHANNELS];
    memcpy( _nr_of_sfb, nr_of_sfb, sizeof(int)*MAX_TIME_CHANNELS );

    rem_bits -= tf_encode_spectrum_aac( diffSpectrum, 
                                        energy, 
                                        allowed_dist, 
                                        blockType,
                                        sfb_width_table, 
                                        _nr_of_sfb, 
                                        (rem_bits/omi.tf_layers),
                                        0, 
                                        padding_limit, 
                                        dynpartStream, 
                                        var_stream, 
                                        NULL,
                                        omi.ch_no_tf[0],
                                        p_reconstructed_spectrum, 
                                        useShortWindows,
                                        windowShape, 
                                        aacAllowScalefacs, 
                                        samplRate,
                                        qc_select, 
                                        pred_type,
                                        NULL, 
                                        blockSizeSamples, 
                                        num_window_groups, 
                                        window_group_length,
                                        NULL,
                                        15000/* bandwidth in Hz */
                                      );
  }

  /* other T/F layers */
  for( lay=1; lay<omi.tf_layers; lay++ ) {
    for( ch=0; ch<omi.ch_no_tf[lay-1]; ch++ ) {
      for( i=0; i<blockSizeSamples; i++ ) {  /* for now just always the difference signal */
        diffSpectrum[ch][i] -= p_reconstructed_spectrum[ch][i];
      }
      for( i=0; i<8; i++ ) {
        msFssControl[ch][i] = DC_DIFF;
      }
    }

   {
    int hbits = -BsCurrentBit( fixedStream );
    aacEncodeLayerHeader( fixedStream, blockType[MONO_CHAN], nr_of_sfb[MONO_CHAN] /* max_sfb */, nr_of_sfb[MONO_CHAN]
                          /* max_sfb_prev_layer */, msFssControl, ms_mask, (omi.ch_no_tf[lay]>omi.ch_no_tf[lay-1]) , omi.ch_no_tf[lay],frameData );
    hbits += BsCurrentBit( fixedStream );
    rem_bits -= hbits;
   }

   {	/* HP 971120 */
    int _nr_of_sfb[MAX_TIME_CHANNELS];
    memcpy( _nr_of_sfb, nr_of_sfb, sizeof(int)*MAX_TIME_CHANNELS );

    rem_bits -= tf_encode_spectrum_aac( diffSpectrum, energy, allowed_dist, blockType,
                                        sfb_width_table, _nr_of_sfb, (rem_bits/(omi.tf_layers-lay)),
                                        0, padding_limit, dynpartStream, var_stream, NULL,
                                        omi.ch_no_tf[lay],
                                        p_reconstructed_spectrum, useShortWindows,
                                        windowShape, aacAllowScalefacs, samplRate,
                                        qc_select, pred_type,
                                        NULL, blockSizeSamples, num_window_groups, window_group_length,
                                        NULL,
                                        15000 /* bandwidth in Hz */
                                      );
   }
  }
 

  dynpartBits = BsCurrentBit( dynpartStream );
  codedFixedBits +=BsCurrentBit( fixedStream );


   
  /* new clean up and write superframe that consists out of 3 granules(=normal frames) */ 

  if( granule == 2 ) { 
    int fillBits;
    int alignBits;

    /* bytaalign bits for fixed stream */
    alignBits = 8 - codedFixedBits%8;
    alignBits = (alignBits == 8) ? 0 : alignBits;
    BsPutBit( fixedStream, 0, alignBits ); 
    codedFixedBits += alignBits;

    /* fillBits for the dynamic part*/
    fillBits = (numGranules*frameBits) - codedFixedBits - dynpartBits;
    if( fillBits < 0 )
      CommonExit(-1,"\n negativ amount of fillbits");
    for( i=0; i<fillBits; i++ ) {
      BsPutBit( dynpartStream, 1, 1 ); 
    }
    BsDirtyForceRewindAndWrite(dynpartStream);
    for( i=0; i<numGranules; i++ ) {
      BsPutBuffer( mainStream, fixedPartBuf[i] );
      dynpartBits= frameBits *(i+1) - BsCurrentBit(mainStream);
      BsGetBuffer( dynpartStream, tmpBitBuf, dynpartBits );
      BsPutBuffer( mainStream, tmpBitBuf );
    }
  }   

  (granule>=2) ? granule=0 : granule++;

  return( BsCurrentBit(mainStream) );
}

void printOpModeInfo(FILE *fp) {
  int x,y;
  fprintf(fp,"\n-mode <int> : operating modes for scalable  coding");
  for (x=0; (unsigned int) x < sizeof(opm_info)/sizeof(OP_MODE_INFO);x++){
    fprintf(fp,"\n mode:  %d  ",x);
    fprintf(fp," ch_no_core: %d  ", opm_info[x].ch_no_core );
    fprintf(fp," ch_no_max : %d  ", opm_info[x].ch_no_max );
    fprintf(fp," tf_layers : %d  \n", opm_info[x].  tf_layers );
    for (y=0;y<    opm_info[x]. tf_layers ;y++){
      fprintf(fp,"  no_channel for tf_layer %d: %d  ", y,opm_info[x].ch_no_tf[y]);      
    }    
  }
}

static void degroup (double *rec_spectrum, int num_window_groups, int window_group_length[])
{
  int i, j, k;
  int index, group_offset;
  double tmp[BLOCK_LEN_LONG];

  index = 0;
  group_offset = 0;
  for (i = 0; i < num_window_groups; i++)
    {
      for (j = 0; j < BLOCK_LEN_SHORT; j++)
	for (k = 0; k < window_group_length[i]; k++)
	  tmp[j + BLOCK_LEN_SHORT * k + group_offset] = rec_spectrum[index++];

      group_offset += BLOCK_LEN_SHORT * window_group_length[i];
    }

  for (i = 0; i < BLOCK_LEN_LONG; i++)
    rec_spectrum[i] = tmp[i];
}

static void MSInverseMatrix(int num_samples, double *left, double *right)
{
  int i;
  double dtmp;
	
  for(i = 0; i < num_samples; i++)
  {
    dtmp = left[i] + right[i];
    
    right[i] = left[i] - right[i];
    left[i] = dtmp;
  }
}

static void ltp_scal_reconstruct(WINDOW_SEQUENCE blockType,
                                 WINDOW_SHAPE windowShape,
                                 WINDOW_SHAPE windowShapePrev,
                                 int num_channels,
                                 double *p_reconstructed_spectrum_left, 
                                 double *p_reconstructed_spectrum_right,
                                 int num_window_groups, 
                                 int *window_group_length,
                                 int blockSizeSamples,
                                 int med_win_in_long,
                                 int short_win_in_long,
                                 int *sfb_offset,
                                 int *nr_of_sfb,
                                 NOK_LT_PRED_STATUS *nok_lt_status,
                                 TNS_INFO **tnsInfo)
{
  int i, ch;
  double tmpbuffer[BLOCK_LEN_LONG];
  double *rec_spectrum[2];
  
  
  rec_spectrum[0] = p_reconstructed_spectrum_left;
  rec_spectrum[1] = p_reconstructed_spectrum_right;

  if(blockType == EIGHT_SHORT_SEQUENCE && num_window_groups != -1)
  {
    degroup (rec_spectrum[0], num_window_groups, window_group_length);
    if(num_channels == 2)
      degroup (rec_spectrum[1], num_window_groups, window_group_length);
  }
      
  /* Inverse MS matrix */
  if(num_channels == 2)
    MSInverseMatrix(blockSizeSamples, rec_spectrum[0], rec_spectrum[1]);
      
  for(ch = 0; ch < num_channels; ch++)
  {
    /* Add the LTP contribution to the reconstructed spectrum. */
    nok_ltp_reconstruct(rec_spectrum[ch], blockType, sfb_offset, nr_of_sfb[ch], 
			blockSizeSamples/short_win_in_long, &nok_lt_status[ch]);
	
    for(i = 0; i < blockSizeSamples; i++)
      tmpbuffer[i] = rec_spectrum[ch][i];
	
    /* TNS synthesis filtering. */
    if(tnsInfo[ch] != NULL)
      TnsEncode2(nr_of_sfb[ch], nr_of_sfb[ch], blockType, sfb_offset,
		 &(tmpbuffer[0]), tnsInfo[ch], 1);
	
    /* Update the time domain buffers of LTP. */
    nok_ltp_update(&(tmpbuffer[0]), nok_lt_status[ch].overlap_buffer,
		   blockType, windowShape, windowShapePrev, 
		   blockSizeSamples, 
		   blockSizeSamples/med_win_in_long, 
		   blockSizeSamples/short_win_in_long, 
		   short_win_in_long,
		   &nok_lt_status[ch]);

    /* Disable the use of LTP in the upper layers. */
    nok_lt_status[ch].global_pred_flag = 0;
    nok_lt_status[ch].side_info = 0;
  }
  
  /* MS matrix */
  if(num_channels == 2)
    MSInverseMatrix(blockSizeSamples, rec_spectrum[0], rec_spectrum[1]);  
  
}


int aacScaleableFlexMuxEncode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  blockType[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_block_bits,			
  int         available_bitreservoir_bits,
  int         padding_limit,
  HANDLE_BSBITSTREAM mainStream,
  HANDLE_BSBITSTREAM var_stream,
  int         nr_of_chan,
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         useShortWindows,
  WINDOW_SHAPE     windowShape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     windowShapePrev[MAX_TIME_CHANNELS],  /* needed for LTP - tool */
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         frameMaxNumBit,
  float       **timeSig,
  long        bitRate,
  long        samplRate,
  QC_MOD_SELECT qc_select,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  ENC_FRAME_DATA* frameData,
  enum CORE_CODEC coreCodecIdx,
  int num_window_groups, 
  int window_group_length[8],
  ntt_PARAM *param_ntt,
  double *baselayer_spectrum[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS* nok_lt_status,
  int med_win_in_long,
  int short_win_in_long,
  int *sfb_offset,
  double *nok_tmp_DTimeSigBuf[MAX_TIME_CHANNELS],
  ntt_INDEX* ntt_index,
  ntt_INDEX* ntt_index_scl
  )
{
  int layerBits;
  int i, lay, ch,tfLayer;
  int frameBits;
  int rem_bits = 0;
  OP_MODE_INFO omi;
  enum DC_FLAG FssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  enum DC_FLAG msFssControl[MAX_TIME_CHANNELS][SFB_NUM_MAX];
  HANDLE_BSBITSTREAM layerStream;
  enum JS_MASK ms_mask[SFB_NUM_MAX];
  int lastLayer,  flexmuxOverhead;
  int fullBandwidth = 0;
  int core_coder_bits, ubits;

  memcpy( &omi, &opm_info[frameData->opMode], sizeof(OP_MODE_INFO) );
  /* intermediate_layers = (op_mode >= 7) ? 1 : 0; */
  omi.tf_layers += intermediate_layers;

  lastLayer=omi.tf_layers+ ((omi.ch_no_core > 0) ? 1:0);

  /* start of bitstream composition */
/*   frameBits = bitRate * blockSizeSamples /samplRate - bitsPerFrame; *Q&D for TVQ * */
  frameBits = average_block_bits + available_bitreservoir_bits - 150 ;
  layerStream = BsOpenBufferWrite(dynpartBuf);

  /* ms matrix */
 if(coreCodecIdx != NTT_TVQ){
  if( omi.ch_no_max == 2 ) {  /* always MS for now */
    double dtmp;

    for( i=0; i<blockSizeSamples; i++ ) {
      if(pred_type == NOK_LTP)
      {
	dtmp = baselayer_spectrum[0][i] + baselayer_spectrum[1][i];

	baselayer_spectrum[0][i] = (baselayer_spectrum[0][i] - baselayer_spectrum[1][i]) * 0.5;
	baselayer_spectrum[1][i] = dtmp * 0.5;
      }

      dtmp = p_spectrum[0][i] + p_spectrum[1][i];
      p_spectrum[0][i] = (p_spectrum[0][i] - p_spectrum[1][i]) * 0.5;
      p_spectrum[1][i] = dtmp * 0.5;
    }
    for( i=0; i<SFB_NUM_MAX; i++ ) {
      ms_mask[i] = JS_MASK_MS;
    }
  }
 }
  /* Core Coder */
  lay=0;
  tfLayer=0;
  layerBits=0;
  if( omi.ch_no_core > 0 ) {
    double coreSpectrum[MAX_TIME_CHANNELS][2048];

    if( omi.ch_no_max > omi.ch_no_core ) {
      for( i=0; i<blockSizeSamples; i++ ) {
        for( ch=1; ch<omi.ch_no_max; ch++ ) {
          timeSig[MONO_CHAN][i] += timeSig[ch][i];
        }
        timeSig[MONO_CHAN][i] *= 0.5;
      }
      for( ch=1; ch<omi.ch_no_max; ch++ ) {   /* copy S-spectrum */
        for( i=0; i<blockSizeSamples; i++ ) {
          diffSpectrum[ch][i] = p_spectrum[ch][i];
        }
      }
    }

    switch (coreCodecIdx) {
    case CC_CELP_MPEG4_60:
      EncodeCore( layerStream, timeSig[MONO_CHAN], 
                  coreDecodedSampleBuf[MONO_CHAN], 
                  blockSizeSamples, downsamplFac,
                  coreCodecIdx);
#if 0
      mdct_core( NULL,
                 blockType[MONO_CHAN], 
                 windowShape, 
                 coreDecodedSampleBuf[MONO_CHAN], 
                 coreSpectrum[MONO_CHAN], 0, samplRate, blockSizeSamples );
#else
      mdct_core(NULL,
                coreDecodedSampleBuf[MONO_CHAN], 
                coreSpectrum[MONO_CHAN],
                0, 
/*                 samplRate,  */
                blockSizeSamples );
#endif
      
      break;
    case NTT_TVQ:
      /* assume that NTT_VQ has the same FS as AAC 
         we don't have to apply sampling rate conversion */
      {
        int j, ch;
        double nttTimeSigAr[MAX_TIME_CHANNELS][1024];
        double dlpc_spectrum[ntt_T_FR_MAX];
        int used_bits;
	/* added by NTT 001218 */
#ifdef ESC_SUPPORT
        int vq_bits;
#endif
        double *nttTimeSig[MAX_TIME_CHANNELS];
        double *nttTimeSigInit[MAX_TIME_CHANNELS];        
        double *nttCoreSpectrum[MAX_TIME_CHANNELS];

        for (i=0; i<omi.ch_no_core; i++) {
          /* TVQ don't need the time signal, this is only for the interface */
          nttTimeSigInit[i] = nttTimeSigAr[i];
          nttTimeSig[i] = nttTimeSigAr[i]; 
          nttCoreSpectrum[i] = coreSpectrum[i];

          /* copy time signal */
          for (j=0; j<1024; j++)
            nttTimeSigAr[i][j] = (double)timeSig[i][j];
        }
        ntt_index->group_code =0x7f;
        ntt_index->last_max_sfb[0] =0;
        ntt_index->restore_flag_tns=0;
        ntt_index->w_type = blockType[MONO_CHAN];
        ntt_select_ms( p_spectrum,
	        ntt_index, sfb_width_table, nr_of_sfb, -1);
/* this belongs to ICS info, but has to be written within the first TF layer */
        used_bits=0;
	BsPutBit(layerStream,(int)blockType[MONO_CHAN],2);/*window sequence */
        BsPutBit(layerStream, (int)windowShape[MONO_CHAN], 1);       /* window shape */
        used_bits += 3;
#if 1
	if(pred_type == NOK_LTP)

	  for(ch = 0; ch < omi.ch_no_core; ch++)
	  {
	    for(i = 0; i < blockSizeSamples; i++)
	      baselayer_spectrum[ch][i] = p_spectrum[ch][i];
	    
	    nok_ltp_enc(p_spectrum[ch],
			nok_tmp_DTimeSigBuf[ch],
			blockType[ch],
			windowShape[ch], windowShapePrev[ch], 
			blockSizeSamples, 
			blockSizeSamples/med_win_in_long,
			blockSizeSamples/short_win_in_long,
			sfb_offset, nr_of_sfb[ch],
			&nok_lt_status[ch], 
			tnsInfo[ch],
			qc_select);
	  }
#endif
  if(tvq_debug_level>5)
        fprintf(stderr, "MSMSMS %5d \n", ntt_index->ms_mask);
        ntt_headerenc( -1, layerStream, ntt_index, &used_bits,
		       nr_of_sfb, tnsInfo, nok_lt_status, pred_type);	
  if(tvq_debug_level>2)
  fprintf(stderr, "end tns_enc used_bits %5d\n", used_bits);

	ntt_vq_coder(nttTimeSigInit,
	            nttTimeSig,
		    p_spectrum,
		    NULL,
		    NTT_PW_INTERNAL,
                    blockType[MONO_CHAN],
                    ntt_index,
	            param_ntt,
	            sfb_width_table,
                    nr_of_sfb,
		    ntt_index->nttDataBase->ntt_NBITS_FR-used_bits, /* T.Ishikawa 981118 */
                    dlpc_spectrum,
                    nttCoreSpectrum);

          /* pack the TVQ layer in the flexmuxstream */
		if(tvq_debug_level>2)
		fprintf(stderr," %5ld\n",BsCurrentBit(layerStream));
	 
          ntt_index->er_tvq = 0;

#if VERSION2
          if(
          frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_TWIN_VQ)
           ntt_index->er_tvq=1;
	  /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
          if(
          frameData->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value==1)
            ntt_index->nttDataBase->epFlag=1;
          else
            ntt_index->nttDataBase->epFlag=0;
#endif    
#endif         
          ntt_BitPack(ntt_index, layerStream);

          core_coder_bits = BsCurrentBit(layerStream);
		if(tvq_debug_level>2)
		fprintf(stderr,"core_coder_bits %5d\n",core_coder_bits);
		
		/* added by NTT 20001121 */
#ifdef ESC_SUPPORT
          if(blockType[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)
            vq_bits = ntt_index->nttDataBase->ntt_VQTOOL_BITS_S;
          else
            vq_bits = ntt_index->nttDataBase->ntt_VQTOOL_BITS;
#endif

          /* write to flexmux PDU */
          BsClose(layerStream);

	  /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
          if((ntt_index->er_tvq==1) && (ntt_index->nttDataBase->epFlag==1))
            { /* objecttype == 21 */
              ntt_writeFlexMuxPDU(lay,mainStream,vq_bits,dynpartBuf,0);
            }else{ /* objecttype == 7 */
              writeFlexMuxPDU(lay,mainStream,dynpartBuf);
            }
#else
          writeFlexMuxPDU(lay,mainStream,dynpartBuf);
#endif
          layerBits = BsCurrentBit(mainStream);

	  /* 
	   * Restore the spectrum of the base layer and update LTP buffer. 
	   */
	  if(pred_type == NOK_LTP)
	  {
	    double baselayer_rec_spectrum[BLOCK_LEN_LONG];
	    
	    for(ch = 0; ch < omi.ch_no_core; ch++)
	    {   
	      for(i = 0; i < blockSizeSamples; i++)
		baselayer_rec_spectrum[i] = nttCoreSpectrum[ch][i];
	      
	      /* Add the LTP contribution to the reconstructed spectrum. */
	      nok_ltp_reconstruct(baselayer_rec_spectrum, 
				  blockType[ch], sfb_offset, nr_of_sfb[ch],
				  blockSizeSamples/short_win_in_long,
				  &nok_lt_status[ch]);
	     
	      /* This is passed to the scaleable encoder. */
	    for(i = 0; i < blockSizeSamples; i++)
		nttCoreSpectrum[ch][i] = baselayer_rec_spectrum[i];
	    for(i = 0; i < blockSizeSamples; i++)
	        p_spectrum[ch][i]=baselayer_spectrum[ch][i];
	      
	      /* TNS synthesis filtering. */
	      if(ntt_index->tvq_tns_enable)
		TnsEncode2(nr_of_sfb[ch], nr_of_sfb[ch], 
			   blockType[ch], sfb_offset, 
			   baselayer_rec_spectrum, tnsInfo[MONO_CHAN], 1);
	      
	      /* Update the time buffer of LTP. */
	      nok_ltp_update(baselayer_rec_spectrum, nok_lt_status[ch].overlap_buffer,
			     blockType[ch], windowShape[ch], windowShapePrev[ch], 
			     blockSizeSamples, blockSizeSamples/med_win_in_long, 
			     blockSizeSamples/short_win_in_long, 
			     short_win_in_long, &nok_lt_status[ch]);
	    }
	  }
#if 1

	 /* Scalable encoder */
         {
           int iscl;
           int  iii, jjj;
           for(iii=0; iii<8; iii++){
              for(jjj=0; jjj<ntt_index->max_sfb[0]; jjj++){
                ntt_index_scl->msMask[iii][jjj]=ntt_index->msMask[iii][jjj];
              }
           }
           ntt_index_scl->max_sfb[0] = ntt_index->max_sfb[0];
            ntt_index_scl->w_type = ntt_index->w_type;
            ntt_index_scl->group_code = ntt_index->group_code;
           ntt_index_scl->pf = ntt_index->pf;
            if(tvq_debug_level >5)
             fprintf(stderr, "LLL %5d %5d %5d %5d \n", 
	                      ntt_index_scl->max_sfb[0],
			      ntt_index_scl->w_type,
			      ntt_index_scl->group_code,
			      ntt_index_scl->pf);
            if(tvq_debug_level >5)
             fprintf(stderr, "LLL %5d ntt_NSclLay \n", 
	                      ntt_index->nttDataScl->ntt_NSclLay);

          for (iscl=0; iscl<ntt_index->nttDataScl->ntt_NSclLay; iscl++){
	    /* added by NTT 20001121 */
#ifdef VERSION2
#ifdef ESC_SUPPORT
	    if(
	       frameData->od->ESDescriptor[((ntt_index->nttDataBase->epFlag)?1:0)+1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_TWIN_VQ)
	      ntt_index_scl->er_tvq=1;

	    if(
	       frameData->od->ESDescriptor[((ntt_index->nttDataBase->epFlag)?1:0)+1]->DecConfigDescr.audioSpecificConfig.epConfig.value==1)
	      ntt_index_scl->nttDataScl->epFlag=1;
	    else
	      ntt_index_scl->nttDataScl->epFlag=0;

#else
	    if(
	       frameData->od->ESDescriptor[(ntt_index->nttDataScl->ntt_NSclLay)-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_TWIN_VQ)
	      ntt_index_scl->er_tvq=1;
#endif
#endif


             tvqScalEnc(
                 p_spectrum,
                 ntt_index,
                 ntt_index_scl,
                 param_ntt,
                 mainStream,
                 tnsInfo,
                 sfb_width_table,
                 nr_of_sfb,
                 nttCoreSpectrum,
                 pred_type,
                 iscl);
	   }
	    for(ch = 0; ch < omi.ch_no_core; ch++) {   
	      /*
	      for(i = 0; i < blockSizeSamples; i++)
		nttCoreSpectrum[ch][i] = p_reconstructed_spectrum[ch][i];
              */
	      for(i = 0; i < blockSizeSamples; i++)
		p_reconstructed_spectrum[ch][i] = nttCoreSpectrum[ch][i] ;
	    }
          layerBits = BsCurrentBit(mainStream);
    lay += ntt_index->nttDataScl->ntt_NSclLay;
        }
#endif
      }
      break;
    default:
      CommonExit( 1, "Encode scalab.: core coder not yet supported" );
    }
    lay++;
    if (tnsInfo[MONO_CHAN]) {
      
      int sfb, k, sfb_offset[ 100 ];
      
      /* calc. sfb offset table */
      for (sfb=k=0; sfb<nr_of_sfb[MONO_CHAN]; sfb++) {
        sfb_offset[sfb] = k;
        k += sfb_width_table[MONO_CHAN][sfb];
      }
      sfb_offset[sfb] = k;
      
      TnsEncode2(nr_of_sfb[MONO_CHAN],                  /* Number of bands per window */
                 nr_of_sfb[MONO_CHAN],                  /* max_sfb */ 
                 blockType[MONO_CHAN],                  /* block type */
                 sfb_offset,                            /* Scalefactor band offset table */
                 coreSpectrum[MONO_CHAN],               /* Spectral data array */
                 tnsInfo[MONO_CHAN],                    /* TNS info */
		 0);                                    /* Analysis filter. */
    }

  if (omi.tf_layers > 0) {
    CalcFssControl( coreSpectrum[MONO_CHAN], p_spectrum[MONO_CHAN], diffSpectrum[MONO_CHAN], blockType[MONO_CHAN],
                    FssControl[MONO_CHAN], blockSizeSamples, sfb_width_table[MONO_CHAN], samplRate ,diffContrSimu);
  }
  } else {
    for( ch=0; ch<omi.ch_no_max; ch++ ) {
      for( i=0; i<blockSizeSamples; i++ ) {
	if(pred_type == NOK_LTP)
	  diffSpectrum[ch][i] = baselayer_spectrum[ch][i];
	else
	  diffSpectrum[ch][i] = p_spectrum[ch][i];
      }
    }
  }

  if (omi.tf_layers > 0) {
    /* first T/F layer */
    frameBits -= layerBits ;
    flexmuxOverhead =  3*8*(1 + (frameBits/2040));
    frameBits -=   flexmuxOverhead;

    layerStream = BsOpenBufferWrite(dynpartBuf);


    if (qc_select == AAC_SYS) {
    aacEncodeScalHeader( layerStream, NULL, blockType[MONO_CHAN],
                         blockType[MONO_CHAN]==EIGHT_SHORT_SEQUENCE ? nr_of_sfb[MONO_CHAN]*num_window_groups : nr_of_sfb[MONO_CHAN] /*max_sfb*/,
                         omi.ch_no_tf[0], omi.ch_no_max, omi.ch_no_core,
                         samplRate, FssControl, ms_mask, num_window_groups, window_group_length ,tnsInfo[MONO_CHAN] ,frameData, qc_select, nok_lt_status);
    } 

    rem_bits = frameBits - BsCurrentBit(layerStream)  ; /*  */
    fullBandwidth= (int)(  samplRate/2);
    if (fullBandwidth>14000) fullBandwidth=14000;
    {
      int available_bits;
      int limitedBandwidth;
      int _nr_of_sfb[MAX_TIME_CHANNELS];
      memcpy( _nr_of_sfb, nr_of_sfb, sizeof(int)*MAX_TIME_CHANNELS );

      {
        available_bits = rem_bits/omi.tf_layers;
        limitedBandwidth = fullBandwidth/omi.tf_layers;
      }
      ubits = tf_encode_spectrum_aac( /*diffSpectrum*/p_spectrum, energy, allowed_dist, blockType,
                                          sfb_width_table, _nr_of_sfb, available_bits,
                                          0, padding_limit, layerStream, var_stream, NULL, /* write to layerStream directly after the header */
                                          omi.ch_no_tf[tfLayer],
                                          p_reconstructed_spectrum, useShortWindows,
                                          windowShape , aacAllowScalefacs, samplRate,
                                          qc_select, pred_type,
                                          nok_lt_status, blockSizeSamples, num_window_groups, window_group_length,NULL,
                                          limitedBandwidth

                                          );

      /* Update the LTP buffers of the lowest layer. */
      if(coreCodecIdx == NO_CORE && pred_type == NOK_LTP)
	ltp_scal_reconstruct(blockType[MONO_CHAN], windowShape[MONO_CHAN], windowShapePrev[MONO_CHAN], 
			     omi.ch_no_max, p_reconstructed_spectrum[0], 
			     p_reconstructed_spectrum[1], num_window_groups, 
			     window_group_length, blockSizeSamples, 
			     med_win_in_long, short_win_in_long, sfb_offset, 
			     _nr_of_sfb, nok_lt_status, tnsInfo);

      rem_bits -= ubits;
    }
    /* pack 1 aac layer to flexMux packet and write to bitstream */
    BsClose(layerStream);

    writeFlexMuxPDU(lay,mainStream,dynpartBuf);

    layerBits = BsCurrentBit(mainStream);
    lay++;
  }

  /* other T/F layers */
  for( ; lay<lastLayer; lay++ ) {
    tfLayer++;    
    layerStream = BsOpenBufferWrite(dynpartBuf);

    for( ch=0; ch<omi.ch_no_tf[tfLayer]; ch++ ) {
      for( i=0; i<blockSizeSamples; i++ ) {  /* for now just always the difference signal */
        diffSpectrum[ch][i] -= p_reconstructed_spectrum[ch][i];
      }
      for( i=0; i<8; i++ ) {
        msFssControl[ch][i] = DC_DIFF;
      }
    }

   {
    int hbits = -BsCurrentBit( layerStream );
    aacEncodeLayerHeader( layerStream, blockType[MONO_CHAN], nr_of_sfb[MONO_CHAN] /* max_sfb */, nr_of_sfb[MONO_CHAN]
                          /* max_sfb_prev_layer */, msFssControl, ms_mask, (omi.ch_no_tf[tfLayer]>omi.ch_no_tf[tfLayer-1]) , omi.ch_no_tf[tfLayer],frameData );
    hbits += BsCurrentBit( layerStream );
    rem_bits -= hbits;
   }

   {	/* HP 971120 */
    int _nr_of_sfb[MAX_TIME_CHANNELS];
    memcpy( _nr_of_sfb, nr_of_sfb, sizeof(int)*MAX_TIME_CHANNELS );

    rem_bits -= tf_encode_spectrum_aac( diffSpectrum, energy, allowed_dist, blockType,
                                        sfb_width_table, _nr_of_sfb, (rem_bits/(omi.tf_layers-tfLayer)),
                                        0, padding_limit, layerStream, var_stream, NULL,
                                        omi.ch_no_tf[tfLayer],
                                        p_reconstructed_spectrum, useShortWindows,
                                        windowShape, aacAllowScalefacs, samplRate,
                                        qc_select, pred_type,
                                        nok_lt_status, blockSizeSamples, num_window_groups, window_group_length,NULL,
                                        fullBandwidth/(omi.tf_layers-tfLayer)
                                      );
   }

   BsClose(layerStream);
   writeFlexMuxPDU(lay,mainStream,dynpartBuf);
   layerBits = BsCurrentBit(mainStream);
  }
 
  return( BsCurrentBit(mainStream) );
}
