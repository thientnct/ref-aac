#ifndef _scaleable_h_
#define _scaleable_h_
/**********************************************************************
MPEG-4 Audio VM
Common module



This software module was originally developed by

Bodo Teichmann (FhG)

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

Copyright (c) 1997.



Source file: scaleable.h

$Id: scaleable.h,v 1.13 1999/04/19 11:06:38 purnhage Exp $

Authors:
tmn       Bodo Teichmann mailto:tmn@iis.fhg.de

Changes:
**********************************************************************/
#include "ntt_conf.h"



#if 0
enum DC_FLAG   { DC_DIFF, DC_SIMUL, DC_INVALID=-1 };
enum DOSS_MODE { DC_LOW_SR_CORE, DC_FULL_BW };
enum JS_MASK { JS_MASK_OFF=0, JS_MASK_MS=1, JS_MASK_IS=2, JS_MASK_INVALID=-1 };
#endif 

int aacScaleEncInit( 
  BsBitStream *headerStream,
  char        *encPara,		 /* in: encoder parameter string */
  int         numChannel,
  int         frameNumSample,
  long        samplRate,
  ENC_FRAME_DATA* frameData,
  enum CORE_CODEC coreCoderIdx ,
  int mode,
  PRED_TYPE predictor_type,
  ntt_INDEX* ntt_index,
  ntt_INDEX* ntt_index_scl
);

int aacScaleableEncode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_block_bits,			
  int         available_bitreservoir_bits,
  int         padding_limit,
  BsBitStream *fixed_stream,
  BsBitStream *var_stream,
  int         nr_of_chan,
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         useShortWindows,
  WINDOW_SHAPE     window_shape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  int         aacAllowScalefacs,
  int         blockSizeSamples,
  int         frameMaxNumBit,
  float       **timeSig,
  long        bitRate,
  long        samplRate,
  QC_MOD_SELECT qc_select,
  TNS_INFO    *tnsInfo[MAX_TIME_CHANNELS],
  ENC_FRAME_DATA* frameData,
  enum CORE_CODEC coreCodecIdx
);

int aacScaleableFlexMuxEncode(
  double      *p_spectrum[MAX_TIME_CHANNELS],
  double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence[MAX_TIME_CHANNELS],
  int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
  int         nr_of_sfb[MAX_TIME_CHANNELS],
  int         average_block_bits,			
  int         available_bitreservoir_bits,
  int         padding_limit,
  BsBitStream *fixed_stream,
  BsBitStream *var_stream,
  int         nr_of_chan,
  double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
  int         useShortWindows,
  WINDOW_SHAPE     window_shape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     window_shapePrev[MAX_TIME_CHANNELS],
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
  int    num_window_groups,
  int  window_group_length[],
  ntt_PARAM *param_ntt,
  double *base_layer_spectrum[MAX_TIME_CHANNELS],
  NOK_LT_PRED_STATUS* nok_lt_status,
  int med_win_in_long,
  int short_win_in_long,
  int *sfb_offset,
  double *nok_tmp_DTimeSigBuf[MAX_TIME_CHANNELS],
  ntt_INDEX* ntt_index,
  ntt_INDEX* ntt_index_scl
  );


#include "scal_dec.h"
#endif

