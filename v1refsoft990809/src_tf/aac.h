/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by


and edited by

Takashi Koike (Sony Corporation)

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



Source file: 

$Id: aac.h,v 1.12 1999/04/19 11:06:36 purnhage Exp $

**********************************************************************/
#ifndef _aac_h_
#define _aac_h_

/* ----------------------------------------------------------
   This part is connection between DecTfFrame in dec_tf.c 
   and the aac-decoder starting in decoder.c                           
  ----------------------------------------------------------- */

/* --- Init AAC-decoder --- */
void
aac_decode_init(int  sampling_rate_decoded,
                char *aacDebugStr, 
                int  block_size_samples,
                Info *** sfbInfo, 
                int  predictor_type);

/* --- Set current bitBuffer for AAC --- */
/* void set_aac_bitBuffer(BsBitStream *fixed_stream);   */

/* --- Set debugging options, sampling rate given in command line --- */
void set_aac_options(int);

/* ----- Decode one frame with the AAC-decoder  ----- */
int AacDecodeFrame ( BsBitStream*             fixed_stream,
                     BsBitStream*             gc_stream[MAX_TIME_CHANNELS],  /* bitstream for gain control TK */
                     double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                     WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                     WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                     enum AAC_BIT_STREAM_TYPE bitStreamType, 
                     unsigned char            max_sfb[Winds],
                     int                      numChannels,
                     int                      commonWindow,
                     Info**                   sfbInfo, 
                     unsigned char            sfbCbMap[MAX_TIME_CHANNELS][MAXBANDS],
                     HANDLE_RESILIENCE        hResilience,
                     HANDLE_BUFFER            hVm,
                     HANDLE_BUFFER            hHcrSpecData,
                     HANDLE_HCR               hHcrInfo,
                     HANDLE_EP_INFO           hEpInfo,
                     HANDLE_CONCEALMENT       hConcealment,
                     QC_MOD_SELECT            qc_select,
		     NOK_LT_PRED_STATUS**     nok_lt_status);

void decodeBlockType( int coded_types, int granules, int gran_block_type[], int act_gran );

#endif
