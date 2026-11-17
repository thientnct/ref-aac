/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by HP, NT, AG and TN in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/**********************************************************************
MPEG-4 Audio VM
Encoder core (LPC-based)

This software module was originally developed by

Heiko Purnhagen (University of Hannover)
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura (NEC Corporation)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.

Source file: enc_lpc.c

$Id: enc_lpc.c,v 1.25 1999/07/28 14:08:13 purnhage Exp $

Required modules:
common.o         common module
cmdline.o        command line module
bitstream.o      bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
AG    Andy Gerrits, Philips <gerritsa@natlab.research.philips.com>
TN    Toshiyuki Nomura, NEC <t-nomura@dsp.cl.nec.co.jp>

Changes:
20-jun-96   HP    dummy core
14-aug-96   HP    added EncLpcInfo(), EncLpcFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
03-Sep-96   NT    LPC-abs Core Ver. 1.11
26-Sep-96   AG    adapted for PHILIPS
25-oct-96   HP    joined Panasonic and Philips LPC frameworks
                  made function names unique
                    e.g.: abs_lpc_decode() -> PAN_abs_lpc_decode()
                    NOTE: pan_xxx() and PAN_xxx() are different functions!
		  I changed the PAN (and not the PHI) function names
                  just because thus less files had to be modified ...
28-oct-96   HP    added auto-select for pan/phi lpc modes
08-Nov-96   NT    Narrowband LPC Ver. 2.00
                    - replased preprocessing, LP analysis and weighting modules
                    with Philips modules
                    - I/Fs revision for common I/Fs with Philips modules
15-nov-96   HP    adapted to new bitstream module
26-nov-96   AG    abs_xxx() changed to celp_xxx()
27-Feb-97   TN    adapted to Rate Control functionality
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
#include "buffersHandle.h"       /* handler, defines, enums */

#include "common_m4a.h"        /* common module                             */
#include "cmdline.h"       /* command line module                       */
#include "bitstream.h"     /* bit stream module                         */
#include "enc_lpc.h"       /* encoder cores                             */
#include "mp4_lpc.h"       /* common lpc defs                           */
#include "flex_mux.h"

#include "lpc_common.h"		/* HP 961025 */

/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/
#include "phi_cons.h"
#include "celp_encoder.h"

/* =====================================================================*/
/* Panasonic Includes                                                   */
/* =====================================================================*/
#include "celp_proto_enc.h"
#include "pan_celp_const.h"

/* =====================================================================*/
/* NEC Includes                                                         */
/* =====================================================================*/
#include "nec_abs_const.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "LPC-based encoder core V5.0 13-nov-97"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
static long bit_rate;
static long sampling_frequency;
static long frame_size;
static long n_subframes;
static long sbfrm_size;
static long lpc_order;
static long num_lpc_indices;

static long num_shape_cbks;
static long num_gain_cbks;

static long n_lpc_analysis;
static long *window_offsets;
static long *window_sizes;
static long n_lag_candidates; 
static float min_pitch_frequency;    
static float max_pitch_frequency;    
static long *org_frame_bit_allocation;    

static long QuantizationMode; 
static long FineRateControl;  
static long LosslessCodingMode;

static long SampleRateMode;
static long RPE_configuration;
static long Wideband_VQ;
static long MPE_Configuration;
static long NumEnhLayers;
static long BandwidthScalabilityMode;
static long BWS_configuration;
static long BWS_nb_bitrate;
static long PreProcessingSW;

static long ExcitationMode;
static long InputConfiguration = -1;

extern int CELPencDebugLevel;	/* HP 971120 */

static int ELmode;	/* enum MP4ModeLpc */
static int sysFlag;

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"e",&ExcitationMode,"%d",NULL,NULL,NULL},
  {"n",&NumEnhLayers,"%d",NULL,NULL,NULL},
  {"f",&FineRateControl,"%d",NULL,NULL,NULL},
  {"b",&BWS_nb_bitrate,"%d",NULL,NULL,NULL},
  {"p",&PreProcessingSW,"%d",NULL,NULL,NULL},
  {"cf",&InputConfiguration,"%d",NULL,NULL,NULL},
  {"d",&CELPencDebugLevel,"%d","0",NULL,"debug level"},
  {"-celp_sys",&sysFlag,NULL,NULL,NULL,"use system interface(flexmux)"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

static BsBitBuffer *tmpBitBuf;

/* =====================================================================*/
/* instance context                                                     */
/* =====================================================================*/

static	void	*InstanceContext;	/*handle to one instance context */


/* =====================================================================*/
/* L O C A L    F U N C T I O N      D E F I N I T I O N S              */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* EncLpcInfo()                                                         */
/* Get info about LPC-based encoder core.                               */
/* ---------------------------------------------------------------------*/
char *EncLpcInfo (
  FILE *helpStream)     /* in: print encPara help text to helpStream    */
                        /*     if helpStream not NULL                   */
                        /* returns: core version string                 */
{
  if (helpStream != NULL)
  {
    fprintf(helpStream, "--------------------------------------------------------\n");
    fprintf(helpStream, PROGVER "\n");
    fprintf(helpStream, "Usage:\n");
    fprintf(helpStream, "     mp4enc -m lpc -r <rate> -c \"<options>\" <speech file>\n");
    fprintf(helpStream, "      where <rate> is the desired bit rate\n");
    fprintf(helpStream, "            <speech file> is a 8kHz/16kHz speech file\n");
    fprintf(helpStream, "            <options> are:\n");
    fprintf(helpStream, "                e <0/1> : Multi-Pulse Excitation (0) or Regular-Pulse Excitation (1)\n");
    fprintf(helpStream, "                n <NumEnhStage> : the number of enhancement layers \n");
    fprintf(helpStream, "                f <0/1> : Fine Rate Control OFF (0) or ON (1)\n");
    fprintf(helpStream, "                b <NBbitrate> : the bitrate for NarrowBand speech in the BandWidthScalable coder\n");
    fprintf(helpStream, "                p <0/1> : Pre-processing OFF (0) or ON (1)\n");
    fprintf(helpStream, "                cf <0,...,max>   : RPE/MPE Configuration\n");
    fprintf(helpStream, "                                 : max = 3 for RPE \n");
    fprintf(helpStream, "                                 : max = 31 for MPE \n");
    fprintf(helpStream, "                -celp_sys : use system interface(flexmux)\n");
    fprintf(helpStream, "                d <0/1> : Debug Level OFF (0) or ON (1)\n");
    fprintf(helpStream, "\n");
    fprintf(helpStream, "--------------------------------------------------------\n");
 }  
 return PROGVER;
}

/* ---------------------------------------------------------------------*/
/* EncLpcInit()                                                         */
/* Init LPC-based encoder core.                                         */
/* ---------------------------------------------------------------------*/
void EncLpcInit (
    int numChannel,                 /* in: num audio channels           */
    float fSample,                  /* in: sampling frequency [Hz]      */
    float bitRate,                  /* in: total bit rate [bit/sec]     */
    char *encPara,                  /* in: encoder parameter string     */
    int varBitRate,		    /* in: variable bit rate            */
    int *frameNumSample,            /* out: num samples per frame       */
    int *delayNumSample,            /* out: encoder delay (num samples) */
    BsBitBuffer *bitHeader,         /* out: header for bit stream       */
    ENC_FRAME_DATA* frameData)	    /* out: system interface */
{
  int parac;
  char **parav;
  int result;
  BsBitStream *hdrStream;

  int layer, numLayer;

  /* default selection */
  Wideband_VQ = Optimized_VQ;
  QuantizationMode = VectorQuantizer;
  LosslessCodingMode = OFF;
  if (fSample < 12000.)
  {
          ExcitationMode = MultiPulseExc;
	  SampleRateMode = fs8kHz;
	  FineRateControl  = OFF;
	  NumEnhLayers = 0;
	  BandwidthScalabilityMode = OFF;
	  BWS_nb_bitrate = 0;
  }
  else
  {
        ExcitationMode = RegularPulseExc;
	SampleRateMode = fs16kHz;
	FineRateControl = OFF;
	NumEnhLayers = 0;
	BandwidthScalabilityMode = OFF;
	BWS_nb_bitrate = 0;
  }
  PreProcessingSW = ON;

  /* evalute encoder parameter string */
  parav = CmdLineParseString(encPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
     if (result==1) {
        EncLpcInfo(stdout);
        exit (1);
     }
     else
        CommonExit(1,"encoder parameter string error");
  }

  if (strstr(encPara, "-celp_sys") != NULL) 
    sysFlag = 1;

  if (numChannel != 1 ) 
  {
    CommonExit(1,"EncLpcInit: Multi-channel audio input is not supported");
  }

  if ( BWS_nb_bitrate != 0 ) {
    BandwidthScalabilityMode = ON;
    ExcitationMode = MultiPulseExc;
    SampleRateMode = fs8kHz;
    FineRateControl  = OFF;
  }

  if ( (ExcitationMode == MultiPulseExc) && (SampleRateMode==fs16kHz) ) {
    BandwidthScalabilityMode = OFF;
    BWS_nb_bitrate = 0;
  }

  if (((LosslessCodingMode==ON) &&
       ((QuantizationMode==VectorQuantizer) ||
	(FineRateControl==OFF))) ) {
    CommonExit(1,"EncLpcInit: Specified combination of options (1) is not supported");
  }

  if (((BandwidthScalabilityMode==ON) &&
       ((ExcitationMode==RegularPulseExc) ||
	(FineRateControl==ON) ||
	(SampleRateMode==fs16kHz))) ) {
    CommonExit(1,"EncLpcInit: Specified combination of options (2) is not supported");
  }

  if (((NumEnhLayers>0) && (ExcitationMode==RegularPulseExc))) {
    CommonExit(1,"EncLpcInit: Specified combination of options (3) is not supported");
  }

  if (((SampleRateMode==fs8kHz) && (ExcitationMode==RegularPulseExc))) {
    CommonExit(1,"EncLpcInit: Specified combination of options (4) is not supported");
  }

  if ((ExcitationMode == MultiPulseExc) &&
      (SampleRateMode==fs16kHz) &&
      (FineRateControl == ON) &&
      ((InputConfiguration < 0) || (InputConfiguration > 31))) {
    CommonExit(1,"EncLpcInit: MPE_Configuration 0...31 must be specified when FRC is used");
  }
  
  if ((FineRateControl == ON) && (varBitRate == 0))
  {
    CommonExit(1,"EncLpcInit: FineRateControl must be used in combination with -vr switch");
  }
  
  /* -------------------------------------------------------------------*/
  /* Memory allocation                                                  */
  /* -------------------------------------------------------------------*/
  hdrStream = BsOpenBufferWrite(bitHeader);
#if 0 /* dh 990412 */
  if (bitHeader != NULL)
  {
	  p_bitstream->p_bitstream_buffer_start = bitHeader->data;
	  p_bitstream->buffer_length = (bitHeader->size+7)/8;
	  p_bitstream->start_offset = 0;
	  p_bitstream->valid_bits = 0;
  }
  else
  {
	  p_bitstream->p_bitstream_buffer_start = NULL;
	  p_bitstream->buffer_length = 0;
	  p_bitstream->start_offset = 0;
	  p_bitstream->valid_bits = 0;
  }
#endif
  /* ---------------------------------------------------------------- */
  /* Conversion of parameters from float to longs                     */
  /* ---------------------------------------------------------------- */
  bit_rate           = (long)(bitRate+.5);
  sampling_frequency = (long)(fSample+.5);

  /* ---------------------------------------------------------------- */
  /* Encoder Initialisation                                           */
  /* ---------------------------------------------------------------- */
  celp_initialisation_encoder (hdrStream, bit_rate, sampling_frequency, ExcitationMode,
                               SampleRateMode, QuantizationMode, FineRateControl, LosslessCodingMode,
                               &RPE_configuration, Wideband_VQ, &MPE_Configuration,
                               NumEnhLayers, BandwidthScalabilityMode, &BWS_configuration,
                               BWS_nb_bitrate, InputConfiguration, &frame_size, &n_subframes, &sbfrm_size,
                               &lpc_order, &num_lpc_indices, &num_shape_cbks, &num_gain_cbks,
                               &n_lpc_analysis, &window_offsets, &window_sizes, 
                               &n_lag_candidates, &min_pitch_frequency, &max_pitch_frequency, 
                               &org_frame_bit_allocation, &InstanceContext,
			       sysFlag);

  /* system interface(flexmux) configuration */
  /* only single CELP layer(layer=0) */
  if (sysFlag) {
    numLayer = 1;
    initObjDescr(frameData->od);
    presetObjDescr(frameData->od, numLayer);
    
    for (layer=0; layer<numLayer; layer++) {
      initESDescr(&(frameData->od->ESDescriptor[layer]));
      presetESDescr(frameData->od->ESDescriptor[layer], layer);

      /* elementary stream configuration */
      /* DecConfigDescriptor configuration */
      frameData->layer[layer].bitRate = (int)bitRate;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = frameData->layer[layer].bitRate;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value = 0;

      /* ALConfigDescriptor configuration */
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;

      /* AudioSpecificConfig configuration */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = 1;
      if (fSample < 12000.) {
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0xb;	/* 8000Hz */
      } else {
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0x8;	/* 16000Hz */
      }
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;	/* mono */
      
      /* CelpSpecificConfig configuration */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value = ExcitationMode;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.sampleRateMode.value = SampleRateMode;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl.value = FineRateControl;

      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.RPE_Configuration.value = RPE_configuration;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.MPE_Configuration.value = MPE_Configuration;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value = NumEnhLayers;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.bandwidthScalabilityMode.value = BandwidthScalabilityMode;
    }

    advanceODescr(hdrStream, frameData->od, 1);

    for (layer=0; layer<numLayer; layer++){ 
      advanceESDescr(hdrStream, frameData->od->ESDescriptor[layer], 1);
    }
    
    tmpBitBuf = BsAllocBuffer(512);
  }

  if (ExcitationMode == RegularPulseExc)
  {
      *frameNumSample = frame_size;
      *delayNumSample = 2*frame_size;
  }
  
  if (ExcitationMode == MultiPulseExc)
  {
    *frameNumSample = frame_size;
    if (SampleRateMode == fs16kHz)
    {
      *delayNumSample = NEC_FRQ16_NLA;
    }
    else
    {
      if (BandwidthScalabilityMode == OFF)
      {
        *delayNumSample = NEC_PAN_NLA;
      }
      else
      {
        *delayNumSample = NEC_FRQ16_NLA+NEC_LPF_DELAY;
      }
    }
  }

  BsClose(hdrStream);
}

/* ------------------------------------------------------------------- */
/* EncLpcFrame()                                                       */
/* Encode one audio frame into one bit stream frame with               */
/* LPC-based encoder core.                                             */
/* ------------------------------------------------------------------- */
void EncLpcFrame (
  float **sampleBuf,      /* in: audio frame samples                   */
                          /*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,    /* out: bit stream frame                     */
                          /*      or NULL during encoder start up      */
  int frameAvailNumBit,   /* in: total num bits available for          */
                          /*     this frame (incl. bit reservoir)      */
  int frameNumBit,        /* in: average num bits per frame            */
  int frameMaxNumBit)     /* in: max num bits per frame                */
{
  BsBitStream *bitStream;
  
  /* -------------------------------------------------------------------*/
  /* Memory allocation                                                  */
  /* -------------------------------------------------------------------*/
  if (sysFlag) {	/* for flexmux bitstream, use writeFlexMuxPDU() */
    if (bitBuf)
      bitStream = BsOpenBufferWrite(tmpBitBuf);
    else
      bitStream = NULL;

    celp_coder(sampleBuf, bitStream, sampling_frequency, bit_rate,
	       ExcitationMode,
		  SampleRateMode,
		  QuantizationMode, FineRateControl,
		  LosslessCodingMode,RPE_configuration, Wideband_VQ,
		  MPE_Configuration,NumEnhLayers, BandwidthScalabilityMode,
		  BWS_configuration,PreProcessingSW,
		  frame_size, 
	  n_subframes, sbfrm_size, lpc_order, num_lpc_indices, num_shape_cbks, num_gain_cbks, 
	  n_lpc_analysis, window_offsets, window_sizes, n_lag_candidates, 
	  min_pitch_frequency, max_pitch_frequency, org_frame_bit_allocation,
	  InstanceContext);

    if (bitBuf) {
      BsClose(bitStream);

      bitStream = BsOpenBufferWrite(bitBuf);
      writeFlexMuxPDU(0 , bitStream , tmpBitBuf);
      BsClose(bitStream);
    }
  } else {
    if (bitBuf)
      bitStream = BsOpenBufferWrite(bitBuf);
    else
      bitStream = NULL;

    celp_coder(sampleBuf, bitStream, sampling_frequency, bit_rate,
	       ExcitationMode,
		  SampleRateMode,
		  QuantizationMode, FineRateControl,
		  LosslessCodingMode,RPE_configuration, Wideband_VQ,
		  MPE_Configuration,NumEnhLayers, BandwidthScalabilityMode,
		  BWS_configuration,PreProcessingSW,
		  frame_size, 
	  n_subframes, sbfrm_size, lpc_order, num_lpc_indices, num_shape_cbks, num_gain_cbks, 
	  n_lpc_analysis, window_offsets, window_sizes, n_lag_candidates, 
	  min_pitch_frequency, max_pitch_frequency, org_frame_bit_allocation,
	  InstanceContext);
    
    if (bitBuf!=NULL)
      BsClose(bitStream);
  }

}
/* ---------------------------------------------------------------------*/
/* EncLpcFree()                                                         */
/* Free memory allocated by LPC-based encoder core.                     */
/* ---------------------------------------------------------------------*/
void EncLpcFree ()
{
  /* -----------------------------------------------------------------*/
  /* Free all the arrays that were initialised                        */
  /* -----------------------------------------------------------------*/ 
  celp_close_encoder(ExcitationMode, SampleRateMode,
		     BandwidthScalabilityMode, sbfrm_size, 
                     org_frame_bit_allocation, window_offsets, window_sizes, 
		     n_lpc_analysis, &InstanceContext);
}

/* end of enc_lpc.c */

