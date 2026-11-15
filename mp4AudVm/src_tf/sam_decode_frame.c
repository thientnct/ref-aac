#ifdef	VERSION2
/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1999-07-30

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

Copyright (c) 1999.

**********************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "sam_tns.h"             /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "flex_mux.h"
#include "common_m4a.h"
#include "bitstream.h"
#include "tf_main.h"
#include "sam_dec.h"
#include "sony_local.h"




#ifdef  __cplusplus
extern "C" {
#endif

static void sam_get_tns( int windowSequence, sam_TNS_frame_info *tns_frame_info );

#ifdef  __cplusplus
}
#endif


void bsacAUDecode ( int          numChannels, 
                    FRAME_DATA*  fd, /* config data , obj descr. etc. */
                    TF_DATA*     tfData,
                    int          target)
{
  HANDLE_BSBITSTREAM  bsac_bitStream, layer_stream;
  HANDLE_BSBITBUFFER  bsac_bitBuf;
  unsigned int i_ES;
  int AUno;

  /* combine all ES data into one bitstream */
  bsac_bitBuf = BsAllocBuffer(256000);
  for(i_ES=0; i_ES < fd->od->streamCount.value; i_ES++) {
     
     AUno =  fd->layer[i_ES].NoAUInBuffer;
     /* open the first layer bitbuffer  to read */
     if (AUno<=0) {
        CommonWarning("bsacAUDecode : %d-th ES has no Access unit; skiping frame", i_ES);
        return;
     }

     layer_stream= BsOpenBufferRead(fd->layer[i_ES].bitBuf);
     BsManipulateSetNumByte( (BsBufferNumBit(fd->layer[i_ES].bitBuf)+7)/8, layer_stream);
     if (fd->layer[i_ES].AULength[0] > 0) 
        BsGetBufferAppend(layer_stream, bsac_bitBuf, 1, fd->layer[i_ES].AULength[0]);
     BsCloseRemove(layer_stream,1);
     fd->layer[i_ES].NoAUInBuffer--;
  }

  bsac_bitStream = BsOpenBufferRead(bsac_bitBuf); 
  BsManipulateSetNumByte((BsBufferNumBit(bsac_bitBuf)+7)/8, bsac_bitStream);
  /* decode BSAC */
  sam_decode_frame(bsac_bitStream, 
                         target,
                         tfData->spectral_line_vector[0],
                         &(tfData->windowSequence[MONO_CHAN]), 
                         &(tfData->windowShape[MONO_CHAN]), 
                         numChannels);
  BsCloseRemove (bsac_bitStream,1);
  BsFreeBuffer (bsac_bitBuf);
}

void sam_FlexMuxDecode_bsac ( HANDLE_BSBITSTREAM        fixed_stream,
                                 WINDOW_SEQUENCE    windowSequence[MAX_TIME_CHANNELS],
                                 int*               flexMuxBits,
                                 double*            spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                                 int                numChannels,
                                 WINDOW_SHAPE*      windowShape,
                                 int                bsacDecBr,
                                 FRAME_DATA*        fd
)
{
  HANDLE_BSBITSTREAM  layer_stream, bsacStream;
  HANDLE_BSBITBUFFER  bsac_bitBuf;
  unsigned int x;
  static unsigned int     numES = 0;  
  unsigned int layer;
  int decodedBits;
  unsigned long totalLength;
  unsigned long AUlength[50];

  decodedBits       = 0;
  
  /*********************/
  numES = fd->od->streamCount.value;
  for ( x = 0; x < numES; x++ ){
    unsigned int AUindex;/* AUindex = scaleable layer Number , since we have not stream map table*/
    totalLength = 0;
    getAccessUnit ( fixed_stream, fd->layer[x].bitBuf, &AUindex , &totalLength,fd->od->ESDescriptor[x]);
    if (x != AUindex) CommonExit(-1,"\nError in FlexMuxDecode");
    AUlength[x] = totalLength;
  }
  *flexMuxBits = BsCurrentBit(fixed_stream);

  /* combine all AU data into one bitstream */
  bsac_bitBuf = BsAllocBuffer(10000);
  for(layer=0; layer < numES; layer++) {
      layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);
      BsManipulateSetNumByte( (BsBufferNumBit(fd->layer[layer].bitBuf)+7)/8, layer_stream);
      BsGetBufferAppend(layer_stream, bsac_bitBuf, 1,AUlength[layer]*8);
      BsCloseRemove(layer_stream,1);
  }

  bsacStream= BsOpenBufferRead(bsac_bitBuf);
  BsManipulateSetNumByte((BsBufferNumBit(bsac_bitBuf)+7)/8, bsacStream);
  decodedBits = sam_decode_frame(bsacStream, bsacDecBr,
                                  spectral_line_vector[0], 
                                  &windowSequence[MONO_CHAN], 
                                  &windowShape[MONO_CHAN], 
                                  numChannels);
  BsCloseRemove (bsacStream,1);
  BsFreeBuffer (bsac_bitBuf);
  return;
}


int     sam_decode_frame( HANDLE_BSBITSTREAM   fixed_stream,
                          int           target_br,
                          double**      coef,
                          WINDOW_SEQUENCE* window_Sequence,
                          WINDOW_SHAPE* window_Shape,
                          int nch)
{
  int           i, ch, b;
  int           usedBits;
  int           header_length;
  int           sba_mode;
  int           top_layer;
  int           base_snf_thr;
  int           base_band;
  int           max_scalefactor[2];
  int           scf_model0[2];
  int           scf_model1[2];
  int           cband_si_type[2];
  int			max_sfb_si_len[2];

  int           maxSfb;
  int           groupInfo[7];
  int           num_window_groups, window_group_length[8];
  int           stereo_mode;
  int           is_intensity;
  int           ms_mask[MAX_SCFAC_BANDS+1];
  int           is_info[MAX_SCFAC_BANDS];
  int           tns_data_present[2];
  int           ltp_data_present[2];
  int           scalefactors[2][MAX_SCFAC_BANDS];
  int           pns_data_present;
  int           pns_sfb_start = 63;
  int           pns_sfb_flag[2][MAX_SCFAC_BANDS];
  int           pns_sfb_mode[MAX_SCFAC_BANDS];
  int           used_bits = 0;
  int           ics_reserved;
  int           frameLength;
  int           sam_profile;
  sam_TNS_frame_info tns[2];
  double         spectrums[2][1024];
  int           samples[2][1024];


  sam_setBsacdec2BitBuffer(fixed_stream);

  /********* READ BITSTREAM **********/
  /* ***************************************************** */
  /* # the side information part for Mulit-channel       # */ 
  /* ***************************************************** */

  /* initialize variables */
  for(i = 0; i < MAX_SCFAC_BANDS; i++) {
    ms_mask[i] = 0;
    is_info[i] = 0;
	pns_sfb_flag[0][i] = 0;
	pns_sfb_flag[1][i] = 0;
	pns_sfb_mode[i] = 0;
  }

  sam_profile = 1;	/* set profile  LC_Profile */
  ms_mask[0] = 0;
  is_intensity = 0;
  stereo_mode = 0;


  /* ***************************************************** */
  /* #                  frame_length                     # */
  /* ***************************************************** */
  frameLength = sam_GetBits(11) * 8;
  /* shpark : 2000.03.16 */
  if(frameLength == 0)
	CommonExit(1, "Zero-length frame!\n");


  /* ***************************************************** */
  /* #                 bsac_header()                     # */
  /* ***************************************************** */

  header_length = sam_GetBits(4);
  sba_mode      = sam_GetBits(1);
  top_layer     = sam_GetBits(6);
  base_snf_thr  = sam_GetBits(2) + 1;

  for(ch = 0; ch < nch; ch++)
    max_scalefactor[ch] = sam_GetBits(8); 

  base_band     = sam_GetBits(5);

  for(ch = 0; ch < nch; ch++) {
    cband_si_type[ch] = sam_GetBits(5); 
    scf_model0[ch]    = sam_GetBits(3); 
    scf_model1[ch]   = sam_GetBits(3); 
    max_sfb_si_len[ch] = sam_GetBits(4);
  }

  /* ***************************************************** */
  /* #                general_header()                   # */
  /* ***************************************************** */
  /* ics_reserved_bit  */
  ics_reserved = sam_GetBits(1); 

  /*  window sequence */
  *window_Sequence = (WINDOW_SEQUENCE) sam_GetBits(2); 

  /*  window shape */
  *window_Shape = (WINDOW_SHAPE) sam_GetBits(1); 

  if (*window_Sequence == EIGHT_SHORT_SEQUENCE)
  {
      maxSfb = sam_GetBits(4);

      for (i = 0; i < 7; i++)
        groupInfo[i] =  sam_GetBits(1); 
  }
  else 
  {
      maxSfb = sam_GetBits(6);
  }

  num_window_groups = 1;
  window_group_length[0] = 1;
  if (*window_Sequence == 2) {
    for (b=0; b<7; b++) {
      if (groupInfo[b]==0) {
         window_group_length[num_window_groups] = 1;
         num_window_groups++;
      }
      else {
         window_group_length[num_window_groups-1]++;
      }
    }
  }

  /* ***************************************************** */
  /* #                  PNS data                         # */
  /* ***************************************************** */
  pns_data_present = sam_GetBits(1);
  if(pns_data_present)
    pns_sfb_start = sam_GetBits(6);

  /* ***************************************************** */
  /* #                  stereo mode                      # */
  /* ***************************************************** */
  if(nch == 2) {
    stereo_mode = sam_GetBits(2);
    if(stereo_mode == 1) {
      ms_mask[0] = 1;
    } else if(stereo_mode == 2) {
      ms_mask[0] = 2;
     for(i = 0; i < MAX_SCFAC_BANDS; i++)
        ms_mask[i+1] = 1;
    } else if(stereo_mode == 3) {
      ms_mask[0] = 1;
      is_intensity = 1;
    }
  }

  /* ***************************************************** */
  /* #                  TNS & LTP info                   # */
  /* ***************************************************** */
  for(ch = 0; ch < nch; ch++) {
    tns_data_present[ch] = sam_GetBits(1);
    if(tns_data_present[ch]) {
      sam_get_tns(*window_Sequence, &tns[ch]);
    }

    ltp_data_present[ch] = sam_GetBits(1);
  }

  if(nch == 2) {
    window_Sequence[1] = window_Sequence[0];
    window_Shape[1] = window_Shape[0];
  }

  used_bits = sam_getUsedBits();

  if(target_br == 0)  target_br = top_layer + 16;

  /* ***************************************************** */
  /* #                  BSAC  D E C O D I N G            # */
  /* ***************************************************** */
  sam_decode_bsac_data ( target_br,
                         frameLength,
                         sba_mode,
                         top_layer,
                         base_snf_thr,
                         base_band,
                         maxSfb,
                         *window_Sequence,
                         num_window_groups,
                         window_group_length,
                         max_scalefactor,
                         cband_si_type,
                         scf_model0,
                         scf_model1,
                         max_sfb_si_len,
                         stereo_mode,
                         ms_mask,
                         is_info,
                         pns_data_present,
                         pns_sfb_start,
                         pns_sfb_flag,
                         pns_sfb_mode,
                         used_bits,
                         samples,
                         scalefactors,
                         nch);

  /* ***************************************************** */
  /* #                  dequantization                   # */
  /* ***************************************************** */
  for(ch = 0; ch < nch; ch++) {

#if 0
    for (b=0; b<num_window_groups; b++) { 
      for (i=0; i<maxSfb; i++) {
         printf("%3d ", scalefactors[ch][b*maxSfb+i]); 
	  }
    }
    printf("\n");
#endif

    sam_dequantization(target_br,
                       *window_Sequence,
                       scalefactors[ch],
                       num_window_groups,
                       window_group_length,
                       samples[ch],
                       maxSfb,
                       is_info,
                       spectrums[ch],
                       ch);
  }


  /* ***************************************************** */
  /* #                  M/S stereo                       # */
  /* ***************************************************** */
  if(ms_mask[0])
    sam_ms_stereo(*window_Sequence, num_window_groups, window_group_length, 
                  spectrums, ms_mask, maxSfb);

  /* ***************************************************** */
  /* #                  PNS Spectrum                     # */
  /* ***************************************************** */
  if(pns_data_present)
    sam_pns(maxSfb, pns_sfb_flag, pns_sfb_mode, spectrums, scalefactors, nch);

  /* ***************************************************** */
  /* #                  Intensity/Prediction             # */
  /* ***************************************************** */
  if(nch == 2 && is_intensity)
      sam_intensity(*window_Sequence, num_window_groups, window_group_length, 
                    spectrums, scalefactors, is_info, maxSfb);

  for(ch = 0; ch < nch; ch++) {
    /* ***************************************************** */
    /* #                Temporal Noise Shaping             # */
    /* ***************************************************** */
    if(tns_data_present[ch])
      sam_tns_data(*window_Sequence, maxSfb, spectrums[ch], &tns[ch]);

    for(i = 0; i < 1024; i++)
      coef[ch][i] = (double)spectrums[ch][i];
  }

  usedBits = sam_getUsedBits();
  frameLength = sam_BsNumByte() * 8;

  while(usedBits < frameLength) {
    int remain, read_bits;

    remain = frameLength - usedBits;
    read_bits = remain > 8 ? 8 : remain;
    i = sam_GetBits(read_bits);
    usedBits = sam_getUsedBits();
  }
  return usedBits;
}

static void sam_get_tns(int windowSequence, sam_TNS_frame_info *tns_frame_info)
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s, k;
  int                       top_bands;
  short                     *sp, tmp, s_mask, n_mask;
  sam_TNSfilt                   *tns_filt;
  sam_TNSinfo                   *tns_info;
  static unsigned short     sgn_mask[] = { 
    0x2, 0x4, 0x8     };
  static unsigned short     neg_mask[] = { 
    0xfffc, 0xfff8, 0xfff0     };


  if(windowSequence == 2) {
    short_flag = 1;
    tns_frame_info->n_subblocks = 8;
  } else {
    short_flag = 0;
    tns_frame_info->n_subblocks = 1;
  }
  top_bands = sam_tns_top_band(windowSequence);


  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];

    tns_info->n_filt = sam_GetBits( short_flag ? 1 : 2 );
    if (!(tns_info->n_filt))
      continue;
                
    tns_info -> coef_res = res = sam_GetBits( 1 ) + 3;
    top = top_bands;
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      tns_filt->length = sam_GetBits( short_flag ? 4 : 6 );
      top = tns_filt->start_band = top - tns_filt->length;
      tns_filt->order = sam_GetBits( short_flag ? 3 : 5 );

      if (tns_filt->order)  {
        tns_filt->direction = sam_GetBits( 1 );
        compress = sam_GetBits( 1 );
        tns_filt->coef_compress = compress;

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        k = 0;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = sam_GetBits( res2 );
          tns_filt->coef1[k++] = tmp;
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
}

#endif	/* VERSION 2 */
