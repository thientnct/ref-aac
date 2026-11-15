/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura (NEC Corporation)

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

Copyright (c) 1996.



Source file: dec_lpc.c
Required modules:
common.o        common module
cmdline.o        command line module
bitstream.o        bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
AG    Andy Gerrits, Philips <gerritsa@natlab.research.philips.com>
PD    Paul Dillen, Philips Consumer Electronics
TN    Toshiyuki Nomura, NEC <t-nomura@dsp.cl.nec.co.jp>
RF    Ralf Funken, Philips <ralf.funken@ehv.ce.philips.com>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
04-Sep-96   NT    LPC Core Ver. 1.11
26-Sep-96   AG    adapted for PHILIPS
25-oct-96   HP    joined Panasonic and Philips LPC frameworks
                  made function names unique
                    e.g.: abs_lpc_decode() -> PAN_abs_lpc_decode()
                    NOTE: pan_xxx() and PAN_xxx() are different functions!
          I changed the PAN (and not the PHI) function names
                  just because thus less files had to be modified ...
28-oct-96   HP    added auto-select for pan/phi lpc modes
08-Nov-96   NT    Narrowband LPC Ver.2.00
                    - replaced synthesis filter module with Philips module
                    - I/Fs revision for common I/Fs with Philips modules
15-nov-96   HP    adapted to new bitstream module
26-nov-96   AG    changed abs_xxx() to celp_xxx()
27-Feb-97   TN    adapted to rate control functionality
27-Mar-97   AG    new functionalities for decoder added
18-Mar-98   PD    Philips private data (for multi-instance) added


**********************************************************************/


/* =====================================================================*/
/* Standard Includes                                                    */
/* =====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================*/
/* MPEG-4 VM Includes                                                   */
/* =====================================================================*/
#include "allHandles.h"
#include "block.h"

#include "lpc_common.h"          /* structs                 */
#include "nok_ltp_common.h"      /* structs                 */
#include "obj_descr.h"           /* structs                 */
#include "tf_mainStruct.h"       /* structs                 */

#include "common_m4a.h"          /* common module           */
#include "cmdline.h"             /* command line module     */
#include "bitstream.h"           /* bit stream module       */
#include "lpc_common.h"
#include "dec_lpc.h"             /* decoder cores           */
#include "mp4_lpc.h"             /* common lpc defs         */


/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/
#include "phi_cons.h"
#include "celp_decoder.h"

/* =====================================================================*/
/* Panasonic Includes                                                   */
/* =====================================================================*/
#include "celp_proto_dec.h"
#include "pan_celp_const.h"

/* =====================================================================*/
/* NEC Includes                                                         */
/* =====================================================================*/
#include "nec_abs_const.h"

#include "flex_mux.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "CELP decoder core FDAM1 16-Feb-2001"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
extern int samplFreqIndex[];    /* undesired, but ... */


/*
Table 5.2   CELP coder (Mode II) configuration for 8 kHz sampling rate 
MPE_Configuration | Frame_size (#samples) | nrof_subframes | sbfrm_size (#samples) 
0,1,2        320          4   80 
3,4,5        240          3   80 
6 ... 12     160          2   80  
13 ... 21    160          4   40 
22 ... 26    80           2   40 
27           240          4   60 
28 ... 31 Reserved
*/
#if 0
static int MPE_Table[]={
    320,/*0 */
    320,/*1 */
    320,/*2 */
    240,/*3 */
    240,/*4 */
    240,/*5 */
    160,/*6 */
    160,/*7*/
    160,/*8 */
    160,/*9 */
    160,/*10 */
    160,/*11 */
    160,/*12 */
    160,/*13 */
    160,/*14 */
    160,/*15 */
    160,/*16 */
    160,/*17 */
    160,/*18 */
    160,/*19 */
    160,/*20 */
    160,/*21 */
    80,/*22 */
    80,/*23 */
    80,/*24 */
    80,/*25 */
    80,/*26 */
    240,/*27*/
    0,0,0,0,0,0,0,0,0,0,0
};
#endif

static long bit_rate;
static long sampling_frequency;
static long frame_size;
static long n_subframes;
static long sbfrm_size;
static long lpc_order;
static long num_lpc_indices;
static long num_shape_cbks;
static long num_gain_cbks;

static long *org_frame_bit_allocation;    

static long SampleRateMode     = 1;  /* Default: 16 kHz */
static long QuantizationMode   = 1;  /* Default: Vector Quantizer */
static long FineRateControl    = 0;  /* Default: FineRateControl is OFF */
static long LosslessCodingMode = 0;  /* Default: Lossless coding is OFF */
static long RPE_configuration;
static long Wideband_VQ = Optimized_VQ;
static long MPE_Configuration;
static long NumEnhLayers;
static long BandwidthScalabilityMode;
static long BWS_configuration;
static long PostFilterSW;
#ifdef VERSION2
static long SilenceCompressionSW;
#endif

static long ExcitationMode;

extern int CELPdecDebugLevel;   /* HP 971120 */

static long  DecEnhStage;
static long  DecBwsMode;
static int  dummysw;
static int  mp4ffFlag;

static int celpAacMode, outputLayerNumber, outputTrackNumber;
#ifdef VERSION2
static int dec_objectVerType;
#endif

static CmdLineSwitch switchList[] =
{
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"lpc_p",&PostFilterSW,"%d","0",NULL,"Post filter OFF (0) or ON (1)"},
  {"lpc_d",&CELPdecDebugLevel,"%d","0",NULL,"debug level"},
/* to make celp ignor aac scalable switches in case of celp + aac decoding" */
  {"-mp4ff",&mp4ffFlag,NULL,NULL,NULL,"use system interface(flexmux)"}, 
  {"-out",&dummysw,"%d",NULL,NULL,"select output layer"},
  {"lpc_n",&DecEnhStage,"%d","0",NULL,"Decoding number of BRS enhancement layers (0,1,2,3)\n"
                                      "# this swith is valid only when -mp4ff is disable\n\b"},
  {"lpc_b",&DecBwsMode,"%d","0",NULL,"Decoding NarrowBand speech (0) or WideBand speech (1)\n"
                                     "# this swith is valid only when -mp4ff is disable\n\b"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* =====================================================================*/
/* instance context                                                     */
/* =====================================================================*/

static  void    *InstanceContext;   /*handle to one instance context */


/* =====================================================================*/
/* L O C A L    F U N C T I O N      D E F I N I T I O N S              */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* DecLpcInfo()                                                         */
/* Get info about LPC-based decoder core.                               */
/* ---------------------------------------------------------------------*/
char *DecLpcInfo (FILE *helpStream)       
{
    if (helpStream != NULL)
    {
      fprintf(helpStream,
	      PROGVER "\n"
	      "decoder parameter string format:\n"
	      "  list of tokens (tokens separated by characters in \"%s\")\n",
	      SEPACHAR);
      CmdLineHelp(NULL,NULL,switchList,helpStream);
    }
    return PROGVER;
}


/* ------------------------------------------------------------------- */
/* DecLpcInit()                                                        */
/* Init LPC-based decoder core.                                        */
/* ------------------------------------------------------------------- */
void DecLpcInit (int          numChannel,         /* in: num audio channels           */
                 float        fSample,            /* in: sampling frequency [Hz]      */
                 float        bitRate,            /* in: total bit rate [bit/sec]     */
                 char         *decPara,           /* in: decoder parameter string     */
                 HANDLE_BSBITBUFFER bitHeader,         /* in: header from bit stream       */
                 int          *frameNumSample,    /* out: num samples per frame       */
                 int          *delayNumSample)    /* out: decoder delay (num samples) */
{
    HANDLE_BSBITSTREAM hdrStream;
    int parac;
    char **parav;
    int result;
    
    mp4ffFlag    =0;
    if (numChannel != 1 ) 
    {
        CommonExit(1,"EncLpcInit: Multi-channel audio input is not supported");
    }
    
    /* evalute decoder parameter string */
    parav = CmdLineParseString(decPara,SEPACHAR,&parac);
    result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);

    free(parav[0]);
    free(parav);

    if (result) 
    {
        if (result==1) 
        {
            DecLpcInfo(stdout);
            exit (1);
        }
        else 
        {
            CommonExit(1,"Decoder parameter string error");
        }
    }  
    
    if ( (ExcitationMode == MultiPulseExc) && (SampleRateMode==fs16kHz) ) 
    {
        Wideband_VQ = Optimized_VQ;
    }
    
    /* -------------------------------------------------------------------*/
    /* Memory allocation                                                  */
    /* -------------------------------------------------------------------*/
    hdrStream = BsOpenBufferRead(bitHeader);
    
    /* ---------------------------------------------------------------- */
    /* Conversion of parameters from float to longs                     */
    /* ---------------------------------------------------------------- */
    bit_rate           = (long)(bitRate+.5);
    sampling_frequency = (long)(fSample+.5);
    
    /* ---------------------------------------------------------------- */
    /* Decoder Initialisation                                           */
    /* ---------------------------------------------------------------- */
    celp_initialisation_decoder (hdrStream,
                                 DecEnhStage, DecBwsMode, PostFilterSW, &frame_size, 
                                 &n_subframes, &sbfrm_size, &lpc_order, &num_lpc_indices,
                                 &num_shape_cbks, &num_gain_cbks, &org_frame_bit_allocation,
                                 &ExcitationMode, &SampleRateMode, &QuantizationMode, 
                                 &FineRateControl, &LosslessCodingMode, &RPE_configuration, 
                                 &Wideband_VQ, &MPE_Configuration, &NumEnhLayers, 
                                 &BandwidthScalabilityMode, &BWS_configuration,
#ifdef VERSION2
                                 &SilenceCompressionSW,
                                 CelpObjectV2,
#endif
                                 &InstanceContext, mp4ffFlag);
    
    *frameNumSample = frame_size;
    *delayNumSample = 0;
    
    BsClose(hdrStream);
}

/* ------------------------------------------------------------------- */
/* DecLpcInitNew()                                                     */
/* Init LPC-based decoder core.                                        */
/* ------------------------------------------------------------------- */

void DecLpcInitNew (char*           decPara,
                    FRAME_DATA*     fD,
                    LPC_DATA*       lpcData
)             
{
    int parac;
    char **parav;
    int result;
    int numChannel = 0;
#if 0
    int MPE_conf;
#endif
    double frameLengthTime;
    
#if 0
    HANDLE_BSBITBUFFER bitHeader;
#endif
    HANDLE_BSBITSTREAM hdrStream=NULL;
    AUDIO_SPECIFIC_CONFIG* audSpC ;
    int i;
    int numLayer, numTrack;
    int coreDecType, enhDecType;
    
    mp4ffFlag = 1;
    
    coreDecType = fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
    enhDecType = fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
    
    if ( coreDecType == enhDecType ) 
    {
        celpAacMode = 0; /* CELP only */
    }
    else 
    {
        celpAacMode = 1; /* CELP core in AAC Scalable */
    }

#ifdef ESC_SUPPORT
    numTrack = fD->od->streamCount.value;
    outputLayerNumber = fD->scalOutSelect;
    if ( celpAacMode ) { /* To be fixed */
      numLayer = numTrack;
      outputTrackNumber = outputLayerNumber;
    } else {
      audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
      if ( (audSpC->audioDecoderType.value == 8) ||
	   (audSpC->audioDecoderType.value == 24 && audSpC->epConfig.value == 0) ) {
	numLayer = numTrack;
	outputTrackNumber = outputLayerNumber;
      } else {
	numLayer = numTrack - 4;
	if ( numLayer < 1 ) {
	  CommonExit(1,"wrong fD->od->streamCount");
	}
	outputTrackNumber = outputLayerNumber + 4;
	if ( outputTrackNumber >= numTrack ) {
	  outputTrackNumber = numTrack - 1;
	  outputLayerNumber = outputTrackNumber - 4;
	}
      }
      if ( numLayer > 5 || numTrack > 9 ) {
        CommonExit(1,"wrong fD->od->streamCount");
      }
    }
#else
    numLayer = fD->od->streamCount.value;
    numTrack = numLayer;
    outputLayerNumber = fD->scalOutSelect;
    outputTrackNumber = outputLayerNumber;
#endif
    
    if ( outputLayerNumber >= numLayer ) 
    {
        CommonExit(1,"wrong fD->scalOutSelect");
    }
#ifdef ESC_SUPPORT
    if ( outputTrackNumber >= numTrack ) 
    {
        CommonExit(1,"wrong fD->scalOutSelect");
    }
#endif

#ifdef VERSION2
    if ( coreDecType == 8 ) 
    {
        dec_objectVerType = CelpObjectV1;
    }
    else if ( coreDecType == 24 ) 
    {
        dec_objectVerType = CelpObjectV2;
    }
#endif

    audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;

    switch (audSpC->channelConfiguration.value) 
    {
        case 1   :  numChannel=1;       
                    break;
        case 2   :  numChannel=2; /* two channel configuration does not yet work properly! */
                    break;
        default  :  CommonExit(1,"wrong channel config");
    }

    
    lpcData->coreBitBuf = BsAllocBuffer( 4000 );      /* just some large number. For G729 required: 160 */
    
    /* allocate the correct amount of memory even for tho channel core */
    /* Rem.: two-channel core configuration does not yet work properly */
    if((lpcData->sampleBuf=(float**)(malloc(numChannel * sizeof (float*) )))==NULL) 
    {
        CommonExit(1, "Memory allocation error in enc_lpc");
    }

    for(i=0; i<numChannel; i++)
    {
      if((lpcData->sampleBuf[i] =(float*)malloc(sizeof(float)*1024))==NULL)     
      {
          CommonExit(1, "Memory allocation error in enc_lpc");
      }
    }    


#if 0
    bitHeader=NULL;
#endif
    
    if (!celpAacMode )
    {
        for ( i = 1; i < numTrack; i++ ) 
        {
            if ( fD->od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value != 1 )
            {
                CommonExit(1,"wrong channel config");
            }
        }
    }
    
    /* evalute decoder parameter string */
    parav = CmdLineParseString(decPara,SEPACHAR,&parac);
    result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
    
    /* HP20010201  force mp4ffFlag=1 again after CmdLineEval() */
    mp4ffFlag = 1;
    
    if (result==1) 
    {
        DecLpcInfo(stdout);
        exit (1);
    }
    
    if ((ExcitationMode == MultiPulseExc) && (SampleRateMode==fs16kHz) ) 
    {
        Wideband_VQ = Optimized_VQ;
    }
    

#if 0
    /* -------------------------------------------------------------------*/
    /* Memory allocation                                                  */
    /* -------------------------------------------------------------------*/
    if (bitHeader != NULL)
    {
        hdrStream = BsOpenBufferRead(bitHeader);
    }


    /* ---------------------------------------------------------------- */
    /* Conversion of parameters from float to longs                     */
    /* ---------------------------------------------------------------- */
    bit_rate           = (long)(fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value +.5);
    sampling_frequency = (long)(samplFreqIndex [fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value]+.5);
    /* HP20001012 samplingFreqencyIndex==0xf not handled */
    
    if (sampling_frequency == 7350) 
    {
        sampling_frequency = 8000;
    }
    
    MPE_conf=fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.MPE_Configuration.value;
    lpcData->frameNumSample=MPE_Table[MPE_conf];
    frameLengthTime=(double)lpcData->frameNumSample/(double)sampling_frequency;
    
    lpcData->bitsPerFrame = (int)(bit_rate*frameLengthTime);     
#endif
    
    if (mp4ffFlag==1) 
    {
        int layer=0; 
        CELP_SPECIFIC_CONFIG *celpConf;

#ifdef VERSION2
        CELP_SPECIFIC_CONFIG *erCelpConf;
#endif
        
#ifdef VERSION2
        if ( dec_objectVerType == CelpObjectV1 ) 
        {
#endif
            celpConf= &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
            
            ExcitationMode           =   celpConf->excitationMode.value ;
            SampleRateMode           =   celpConf->sampleRateMode.value ;
            FineRateControl          =   celpConf->fineRateControl.value;
            RPE_configuration        =   celpConf->RPE_Configuration.value;
            MPE_Configuration        =   celpConf->MPE_Configuration.value ;
            NumEnhLayers             =   celpConf->numEnhLayers.value;
            BandwidthScalabilityMode =   celpConf->bandwidthScalabilityMode.value;
            
#ifdef VERSION2
            SilenceCompressionSW     = OFF;
        } 
        else 
        {
            erCelpConf= &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
            
            ExcitationMode           =   erCelpConf->excitationMode.value ;
            SampleRateMode           =   erCelpConf->sampleRateMode.value ;
            FineRateControl          =   erCelpConf->fineRateControl.value;
            RPE_configuration        =   erCelpConf->RPE_Configuration.value;
            MPE_Configuration        =   erCelpConf->MPE_Configuration.value ;
            NumEnhLayers             =   erCelpConf->numEnhLayers.value;
            BandwidthScalabilityMode =   erCelpConf->bandwidthScalabilityMode.value;
            SilenceCompressionSW     =   erCelpConf->silenceCompressionSW.value;
        }
#endif

	/* BandwidthScalabilityMode shouldn't be left uninitialised   HP20001025 */
	if (fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value != MultiPulseExc)
	  BandwidthScalabilityMode = OFF;

        if ( !celpAacMode && (BandwidthScalabilityMode == ON) )
        {
#ifdef CORRIGENDUM1
            CELP_SPECIFIC_CONFIG *celpEnhConf = &fD->od->ESDescriptor[numTrack-1]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
#else
            CELP_ENH_SPECIFIC_CONFIG *celpEnhConf = &fD->od->ESDescriptor[numTrack-1]->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig;
#endif
            BWS_configuration = celpEnhConf->BWS_Configuration.value;
        }
        
        if (!celpAacMode) 
        {
            if (outputLayerNumber > (NumEnhLayers+BandwidthScalabilityMode)) 
            {
                CommonExit(1,"wrong fD->scalOutSelect");
            }
            
            if (BandwidthScalabilityMode == ON) 
            {
                if (outputLayerNumber <= NumEnhLayers) 
                {
                    DecEnhStage = outputLayerNumber;
                    DecBwsMode = 0;
                }
                else 
                {
                    DecEnhStage = NumEnhLayers;
                    DecBwsMode = 1;
                }
            } 
            else 
            {
                DecEnhStage = outputLayerNumber;
                DecBwsMode = 0;
            }
        } 
        else 
        {
            outputLayerNumber = min(outputLayerNumber,NumEnhLayers);
            outputTrackNumber = outputLayerNumber;
#ifdef ESC_SUPPORT
            if ( audSpC->audioDecoderType.value == 24 && audSpC->epConfig.value == 1 ) {
              outputTrackNumber += 4;
            }
#endif
            DecEnhStage = 0;
            DecBwsMode = 0;
        }
        
        if (NumEnhLayers>0)
	{ 
          DecEnhStage = min(outputLayerNumber,NumEnhLayers);
	}

        NumEnhLayers = DecEnhStage;
        BandwidthScalabilityMode = DecBwsMode;
    }
    
    /* ---------------------------------------------------------------- */
    /* Decoder Initialisation                                           */
    /* ---------------------------------------------------------------- */
    celp_initialisation_decoder (hdrStream, 
                                 DecEnhStage, 
                                 DecBwsMode, 
                                 PostFilterSW, 
                                 &frame_size, 
                                 &n_subframes, 
                                 &sbfrm_size,      
                                 &lpc_order,
                                 &num_lpc_indices,
                                 &num_shape_cbks,
                                 &num_gain_cbks,
                                 &org_frame_bit_allocation,
                                 &ExcitationMode,
                                 &SampleRateMode, 
                                 &QuantizationMode, 
                                 &FineRateControl, 
                                 &LosslessCodingMode, 
                                 &RPE_configuration, 
                                 &Wideband_VQ, 
                                 &MPE_Configuration, 
                                 &NumEnhLayers, 
                                 &BandwidthScalabilityMode, 
                                 &BWS_configuration,
#ifdef VERSION2
                                 &SilenceCompressionSW,
                                 dec_objectVerType,
#endif
                                 &InstanceContext,
                                 mp4ffFlag);
    
    lpcData->frameNumSample = frame_size;
    lpcData->delayNumSample = 0;
    
    bit_rate = (long)(fD->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value +.5);

    /* Calculate total core bitrate */
    for (i = 1; i <= outputTrackNumber; i++)
    {
      bit_rate += (long)(fD->od->ESDescriptor[i]->DecConfigDescr.avgBitrate.value +.5);
    }
        
    sampling_frequency = (long)(samplFreqIndex[fD->od->ESDescriptor[outputTrackNumber]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value]+.5);
    /* HP20001012 samplingFreqencyIndex==0xf not handled */
    
    if (sampling_frequency==7350) 
    {
        sampling_frequency=8000;
    }
    
    frameLengthTime = (double)lpcData->frameNumSample/(double)sampling_frequency;
    lpcData->bitsPerFrame = (int)(bit_rate*frameLengthTime);     

#ifdef ESC_SUPPORT
     if (!celpAacMode) {
        fD->od->streamCount.value = outputTrackNumber + 1;
        fD->scalOutSelect = outputLayerNumber;
     } 
     /* streamCount and scalOutSelect should be correctly set in
        somewhere (scal_dec_frame.c ?) */
#endif
}


/* ------------------------------------------------------------------- */
/* DecLpcFrame()                                                       */
/* Decode one bit stream frame into one audio frame with               */
/* LPC-based decoder core.                                             */
/* ------------------------------------------------------------------- */
void DecLpcFrame (HANDLE_BSBITBUFFER bitBuf,         /* in: bit stream frame                      */
                  float       **sampleBuf,     /* out: audio frame samples                  */
                  int         *usedNumBit)     /* out: num bits used for this frame         */
{
    HANDLE_BSBITSTREAM bitStream[1];
    
    mp4ffFlag = 0;
    /* ----------------------------------------------------------------- */
    /* Memory allocation                                                 */
    /* ----------------------------------------------------------------- */
    bitStream[0] = BsOpenBufferRead(bitBuf);
    
    /* ----------------------------------------------------------------- */
    /*Call Decoder                                                       */
    /* ----------------------------------------------------------------- */
  
    celp_decoder (bitStream, sampleBuf, ExcitationMode, 
                  SampleRateMode, QuantizationMode, FineRateControl,
                  RPE_configuration, Wideband_VQ, BandwidthScalabilityMode,
#ifdef VERSION2
                  MPE_Configuration,
                  SilenceCompressionSW,
                  CelpObjectV2,
#ifdef	ESC_SUPPORT
		  0,
#endif
#endif
                  frame_size, n_subframes, sbfrm_size, lpc_order,
                  num_lpc_indices, num_shape_cbks, num_gain_cbks,           
                  org_frame_bit_allocation,
                  InstanceContext, mp4ffFlag);
    
    *usedNumBit = BsCurrentBit(bitStream[0]);
    BsCloseRemove(bitStream[0],1);

#if 0
    {
        HANDLE_BSBITSTREAM  layer_stream ;
        
        layer_stream= BsOpenBufferRead(bitBuf);
        BsGetSkip(layer_stream,p_bitstream->valid_bits); 
        BsCloseRemove(layer_stream,1);
    }
#endif    
    
}


/* ------------------------------------------------------------------- */
/* DecLpcFrameNew()                                                    */
/* Decode one bit stream frame into one audio frame with               */
/* LPC-based decoder core.                                             */
/* ------------------------------------------------------------------- */

void DecLpcFrameNew (FRAME_DATA*    fD,
                     LPC_DATA*      lpcData,
                     int            *usedNumBit)        /* out: num bits used for this frame         */
{
    int i, tempNumBit;
    HANDLE_BSBITSTREAM bitStream[5+4]; 
    
    mp4ffFlag = 1;
    
    for (i = 0; i <= outputTrackNumber; i++) 
    {
        bitStream[i] = BsOpenBufferRead(fD->layer[i].bitBuf) ; 
    }
    
    /* ----------------------------------------------------------------- */
    /*Call Decoder                                                       */
    /* ----------------------------------------------------------------- */
    celp_decoder (bitStream, lpcData->sampleBuf, ExcitationMode, SampleRateMode, 
                  QuantizationMode, FineRateControl,
                  RPE_configuration, Wideband_VQ, 
                  BandwidthScalabilityMode,
#ifdef VERSION2
                  MPE_Configuration,
                  SilenceCompressionSW,
                  dec_objectVerType,
#ifdef	ESC_SUPPORT
		  fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value,
#endif
#endif
                  frame_size, n_subframes, sbfrm_size, lpc_order,           
                  num_lpc_indices, num_shape_cbks, num_gain_cbks,           
                  org_frame_bit_allocation,
                  InstanceContext, mp4ffFlag);
    
    *usedNumBit = 0;

    for ( i = 0; i <= outputTrackNumber; i++ )
    {
        tempNumBit = BsCurrentBit(bitStream[i]);
        removeAU(bitStream[i],tempNumBit,fD,i);
        *usedNumBit += tempNumBit;
        BsCloseRemove(bitStream[i],1);
    }
    /*  BsClose(bitStream); */
    /* #pragma warning "BsCloseRemove must be used instead of BsClose" */
}


/* ------------------------------------------------------------------- */
/* DeLpcrFree()                                                        */
/* Free memory allocated by LPC-based decoder core.                    */
/* ------------------------------------------------------------------- */

void DecLpcFree ()
{
    celp_close_decoder (ExcitationMode, 
                        BandwidthScalabilityMode, 
                        org_frame_bit_allocation, 
                        &InstanceContext);
}


/* ------------------------------------------------------------------- */
/* lpcframelength()                                                    */
/*                                                                     */
/* ------------------------------------------------------------------- */

int lpcframelength (CELP_SPECIFIC_CONFIG    *celpConf, 
                    int                     layer )
{
    int frame_size = 0;
    
    if (celpConf->excitationMode.value == RegularPulseExc) 
    {
        if (celpConf->RPE_Configuration.value == 0)	
        {
            frame_size = FIFTEEN_MS;
        }
        else if (celpConf->RPE_Configuration.value == 1) 
        {
            frame_size  = TEN_MS;
        }
        else if (celpConf->RPE_Configuration.value == 2) 
        {
            frame_size  = FIFTEEN_MS;
        }
        else if (celpConf->RPE_Configuration.value == 3) 
        {
            frame_size  = FIFTEEN_MS;
        }
        else 
        {
            fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
            CommonExit(1,"celp error"); 
        }
    }
    
    if (celpConf->excitationMode.value == MultiPulseExc)
    {
        if (celpConf->sampleRateMode.value == fs8kHz) 
        {
            if (celpConf->MPE_Configuration.value < 3)
            {
                frame_size = NEC_FRAME40MS;
            }
            
            if (celpConf->MPE_Configuration.value >= 3 && celpConf->MPE_Configuration.value < 6)
            {
                frame_size = NEC_FRAME30MS;
            }
            
            if (celpConf->MPE_Configuration.value >= 6 && celpConf->MPE_Configuration.value < 22)
            {
                frame_size = NEC_FRAME20MS;
            }
            
            if (celpConf->MPE_Configuration.value >= 22 && celpConf->MPE_Configuration.value < 27)
            {
                frame_size = NEC_FRAME10MS;
            }
            
            if (celpConf->MPE_Configuration.value == 27)
            {
                frame_size = NEC_FRAME30MS;
            }
            
            if (celpConf->MPE_Configuration.value > 27)
            {
                fprintf(stderr,"Error: Illegal MPE Configuration.\n");
                CommonExit(1,"celp error"); 
            }
            
            if (celpConf->bandwidthScalabilityMode.value == ON)
            {
                if (layer == (int) (celpConf->numEnhLayers.value + 1)) 
                {
                    frame_size *= 2;
                }
            }
        }
        
        if (celpConf->sampleRateMode.value == fs16kHz) 
        {
            if (celpConf->MPE_Configuration.value < 16)
            {
                frame_size = NEC_FRAME20MS_FRQ16;
            }
            
            if (celpConf->MPE_Configuration.value >= 16 && celpConf->MPE_Configuration.value < 32) 
            {
                frame_size = NEC_FRAME10MS_FRQ16;
            }
            
            if (celpConf->MPE_Configuration.value == 7 || celpConf->MPE_Configuration.value == 23)
            {
                fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                CommonExit(1,"celp error"); 
            }
        }
    }
    
    return (frame_size);
}


#ifdef VERSION2

/* ------------------------------------------------------------------- */
/* erlpcframelength()                                                  */
/*                                                                     */
/* ------------------------------------------------------------------- */

int erlpcframelength (CELP_SPECIFIC_CONFIG   *erCelpConf, 
                      int                       layer )
{
    int frame_size = 0;
    
    if (erCelpConf->excitationMode.value == RegularPulseExc)
    {
        if (erCelpConf->RPE_Configuration.value == 0)   
        {
            frame_size = FIFTEEN_MS;
        }
        else if (erCelpConf->RPE_Configuration.value == 1) 
        {
            frame_size  = TEN_MS;
        }
        else if (erCelpConf->RPE_Configuration.value == 2) 
        {
            frame_size  = FIFTEEN_MS;
        }
        else if (erCelpConf->RPE_Configuration.value == 3) 
        {
            frame_size  = FIFTEEN_MS;
        }
        else
        {
            fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
            CommonExit(1,"celp error"); 
        }
    }
    
    if ( erCelpConf->excitationMode.value == MultiPulseExc ) 
    {
        if ( erCelpConf->sampleRateMode.value == fs8kHz) 
        {
            if ( erCelpConf->MPE_Configuration.value < 3 ) 
            {
                frame_size = NEC_FRAME40MS;
            }
            
            if ( erCelpConf->MPE_Configuration.value >= 3 && erCelpConf->MPE_Configuration.value < 6 ) 
            {
                frame_size = NEC_FRAME30MS;
            }
            
            if ( erCelpConf->MPE_Configuration.value >= 6 && erCelpConf->MPE_Configuration.value < 22 ) 
            {
                frame_size = NEC_FRAME20MS;
            }
            
            if ( erCelpConf->MPE_Configuration.value >= 22 && erCelpConf->MPE_Configuration.value < 27 ) 
            {
                frame_size = NEC_FRAME10MS;
            }
            
            if ( erCelpConf->MPE_Configuration.value == 27 ) 
            {
                frame_size = NEC_FRAME30MS;
            }
            
            if ( erCelpConf->MPE_Configuration.value > 27 ) 
            {
                fprintf(stderr,"Error: Illegal MPE Configuration.\n");
                CommonExit(1,"celp error"); 
            }
            
            if ( erCelpConf->bandwidthScalabilityMode.value == ON ) 
            {
                if ( layer == (int) (erCelpConf->numEnhLayers.value+1) ) frame_size *= 2;
            }
        }
        
        if ( erCelpConf->sampleRateMode.value == fs16kHz) 
        {
            if ( erCelpConf->MPE_Configuration.value < 16 ) 
            {
                frame_size = NEC_FRAME20MS_FRQ16;
            }
            
            if ( erCelpConf->MPE_Configuration.value >= 16 && erCelpConf->MPE_Configuration.value < 32 ) 
            {
                frame_size = NEC_FRAME10MS_FRQ16;
            }
            
            if ( erCelpConf->MPE_Configuration.value == 7 || erCelpConf->MPE_Configuration.value == 23 ) 
            {
                fprintf(stderr,"Error: Illegal BitRate configuration.\n");
                CommonExit(1,"celp error"); 
            }
        }
    }
    
    return( frame_size );
}

#endif    /*  VERSION2   */

/* end of dec_lpc.c */
