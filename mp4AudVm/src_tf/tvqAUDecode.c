 /*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1996-12-06,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Takehiro Moriya (NTT) on 1997-08-01,                                    */
/*   Naoki Iwakami (NTT) on 1997-08-25                                       */
/*   Takehiro Moriya (NTT) on 1998-06-25,                                    */
/*   Takehiro Moriya (NTT) on 1999-02-25,                                    */
/*   Takeshi Mori (NTT) on 2000-11-21,                                       */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

#include <memory.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "lpc_common.h"          /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#ifdef VERSION2		/* HP 9904226 */
#include "sam_tns.h"             /* structs */
#endif

#include "allVariables.h"        /* variables */

#include "bitstream.h"
#include "common_m4a.h"
#include "dec_tf.h"
#include "flex_mux.h"
#include "mod_buf.h"
#include "tf_main.h"

/* son_AACpp */
#include "sony_local.h"

/* NTT VQ */ 
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "mat_def_ntt.h"

/* Long term predictor */
#include "nok_lt_prediction.h"
#include "ntt_nok_lt_predict_dec.h"
#include "ntt_nok_pred.h"


#include "aac.h"
#ifdef VERSION2
#include "reorderspec.h"
#include "resilience.h"
#include "concealment.h"
#endif
#include "buffers.h"


#include "port.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"


static void tvqCoreDec ( int                   numChannels,
                         FRAME_DATA*           fd, /* config data , obj descr. etc. */
                         TF_DATA*              tfData,
                         HANDLE_DECODER_GENERAL hFault,
                         ntt_INDEX*            index,
                         HANDLE_BSBITSTREAM           mat_layerStream,
                         TNS_frame_info        tns_info[MAX_TIME_CHANNELS],
                         QC_MOD_SELECT         qc_select,
                         int                   *nextLayer)

     /*--- base decoder ---*/
{
  Float current_frame[ntt_T_FR_MAX];
  /*
    TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  */
  int tomo_tmp, itmp, i, j, i_ch, decoded_bits;
  int ntt_available_bits;
  int samplFreqIdx = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
        samplingFreqencyIndex.value;

  /* bit unpacking */
  decoded_bits = tfData->decoded_bits;
  index->ms_mask = 0;
  index->group_code = 0x7F ; /*default */
  index->last_max_sfb[0] = 0;
  if ( index->mainDebugLevel>2 ) {
    printf("____ windowSequence %d \n",tfData->windowSequence[MONO_CHAN]);
    printf("____ decoded_bit %d \n",decoded_bits);
    fprintf(stderr, "tfData->pred_type %5d \n", tfData->pred_type); 
  }

  ntt_headerdec(-1, mat_layerStream,  index, tfData->sfbInfo, 
                &decoded_bits, tns_info,
                tfData->nok_lt_status, 
                tfData->pred_type,
                hFault );
  tomo_tmp=decoded_bits;
  ntt_available_bits = index->nttDataBase->ntt_NBITS_FR - decoded_bits; /* T.Ishikawa980624 */

  if ( index->mainDebugLevel>2 ) {
    printf("____ ntt_available_bits %d %d %d\n",ntt_available_bits,
           index->nttDataBase->ntt_NBITS_FR,decoded_bits);
    printf("____ ntt_available_bits %ld %d \n",BsBufferNumBit(fd->layer[0].bitBuf), decoded_bits);
  }
  
   decoded_bits += ntt_BitUnPack ( mat_layerStream,
                                  BsBufferNumBit(fd->layer[0].bitBuf)
                                  - tomo_tmp,
				   /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
                                  tomo_tmp,
                                  fd,
#endif
                                  tfData->windowSequence[MONO_CHAN],
                                  index);
  
  tfData->decoded_bits = decoded_bits;
  if ( index->mainDebugLevel>2 ) {
    printf("<<<< ntt_BitUnPack %d %d >>>>\n",
           decoded_bits-tomo_tmp,decoded_bits);
  }

  /* decoding tools*/
  ntt_vq_decoder(index,tfData->spectral_line_vector[0],
                 tfData->sfbInfo);
  if ( index->mainDebugLevel>2 ) {
    printf("<<<< Base Layer Decoding Finished!!! >>>>\n");
  }
	    
  /* nok 990113 ... */

  /*-- long term predictor --*/
  if(tfData->pred_type == NOK_LTP)
    {
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];

      for(i_ch = 0; i_ch < numChannels; i_ch++)
        {
	  double time_signal[BLOCK_LEN_LONG], tmpBuf[BLOCK_LEN_LONG];

          for (itmp = 0; itmp < info->bins_per_bk; itmp++)
            current_frame[itmp] = 
              tfData->spectral_line_vector[0][i_ch][itmp];
          /*if(tfData->nok_lt_status[i_ch]->sbk_prediction_used[0])*/	
            nok_lt_predict(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
                           tfData->windowSequence[MONO_CHAN],
                           tfData->windowShape[i_ch],
                           tfData->prev_windowShape[i_ch],
                           tfData->nok_lt_status[i_ch]->sbk_prediction_used,
                           tfData->nok_lt_status[i_ch]->sfb_prediction_used,
                           tfData->nok_lt_status[i_ch],
                           tfData->nok_lt_status[i_ch]->weight,
                           tfData->nok_lt_status[i_ch]->delay,
                           current_frame,
                           info->bins_per_bk,
                           info->bins_per_bk/2,
                           info->bins_per_bk/8, 
                           /*(tvqTnsPresent) ?*/ &(tns_info[i_ch]) /*: NULL */,
                           NTT_VQ_SYS);

          /* This is passed to upper layers. */
          for (i = 0; i < info->bins_per_bk; i++)
            tfData->spectral_line_vector[0][i_ch][i] = current_frame[i];

          {	
            /* TNS synthesis filtering. */  
            for(i = j = 0; i <  tns_info[i_ch].n_subblocks; i++)
              {
                tns_decode_subblock(current_frame + j,
                                    index->max_sfb[0],
                                    info->sbk_sfb_top[i],
                                    info->islong,
                                    &(tns_info[i_ch].info[i]), 
                                    qc_select,samplFreqIdx);
		  
                j += info->bins_per_sbk[i];
              }
          }	

	  for (i = 0; i < info->bins_per_bk; i++)
	    tmpBuf[i] = current_frame[i];
	  
	  freq2buffer(tmpBuf, time_signal, 
		      tfData->nok_lt_status[i_ch]->overlap_buffer,
		      tfData->windowSequence[MONO_CHAN], 
		      info->bins_per_bk, info->bins_per_bk/2,
		      info->bins_per_bk/8, tfData->windowShape[MONO_CHAN], 
		      tfData->prev_windowShape[i_ch], OVERLAPPED_MODE, 8);
	  
	  nok_lt_update(tfData->nok_lt_status[i_ch], time_signal, 
			tfData->nok_lt_status[i_ch]->overlap_buffer, 
			info->bins_per_bk);
        }
    }

  /* ... nok */
  fd->layer[*nextLayer].NoAUInBuffer--;


  /* added by NTT 20001121 */
#ifdef ESC_SUPPORT
  if((index->er_tvq==1) && (fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value==1)){
    (*nextLayer)++;
    fd->layer[*nextLayer].NoAUInBuffer--;

  }
#endif

  (*nextLayer)++;

}

static void tvqScalDec(
                       FRAME_DATA*  fd, /* config data , obj descr. etc. */
                       TF_DATA*     tfData,
                       HANDLE_DECODER_GENERAL hFault,
                       ntt_INDEX*   index,
                       ntt_INDEX*   index_scl,
                       
                       TNS_frame_info tns_info[MAX_TIME_CHANNELS],
                       int *nextLayer)
     /*--- scalable decoders ---*/
{
  HANDLE_BSBITSTREAM  mat_layerStream ;
  /*
    TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  */
  int decoded_bits, iscl,tomo_tmp,tomo_tmp2;
  int   ntt_available_bits;
  int  iii, jjj, ntt_NSclLay, ntt_NSclLayDec;
  /* added by NTT 001206 */
#ifdef ESC_SUPPORT
  HANDLE_BSBITSTREAM layerStream_tmp;
#endif

  /*ntt_NSclLay = fd->od->streamCount.value-1; */
  ntt_NSclLay = index_scl->nttDataScl->ntt_NSclLay;

  if (( fd->od->streamCount.value > ( unsigned int )*nextLayer ) 
      && (((fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==TWIN_VQ))
#ifdef VERSION2
        ||
         ((fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ))
#endif
)) {
    ntt_NSclLayDec = tfData->output_select;
  }  else {
    ntt_NSclLayDec=0;
  }

  if(ntt_NSclLayDec > ntt_NSclLay) ntt_NSclLayDec = ntt_NSclLay;

  if(tvq_debug_level > 5)
    fprintf(stderr, "WWWWWW %5d %5d NSclLay\n", ntt_NSclLay, ntt_NSclLayDec);
  for(iii=0; iii<8; iii++){
    for(jjj=0; jjj<index->max_sfb[0]; jjj++){
      index_scl->msMask[iii][jjj] = 
        index->msMask[iii][jjj];
    }
  }
  index_scl->w_type = tfData->windowSequence[MONO_CHAN];
  index_scl->max_sfb[0] = index->max_sfb[0];
  index_scl->group_code = index->group_code ;
  index_scl->pf = index->pf ;
  for (iscl=0; iscl<ntt_NSclLayDec; iscl++){
    index_scl->last_max_sfb[iscl+1] = 
      index_scl->max_sfb[iscl];
    /* added by NTT 001205 */
#ifdef VERSION2
    index_scl->er_tvq=0;
    if(fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ)
      index_scl->er_tvq=1;
#ifdef ESC_SUPPORT
    if((index_scl->er_tvq==1) && (fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.epConfig.value==1)){
      layerStream_tmp = BsOpenBufferRead(fd->layer[(*nextLayer)+1].bitBuf);
      BsManipulateSetNumByte(BsBufferNumBit(BsGetBitBufferHandle(0,layerStream_tmp))/8, layerStream_tmp);
      BsGetBufferAppend( layerStream_tmp, fd->layer[*nextLayer].bitBuf, 1,fd->layer[(*nextLayer)+1].AULength[0]);
      BsCloseRemove(layerStream_tmp, 1);
    }
#endif
#endif

    mat_layerStream = BsOpenBufferRead((fd->layer[*nextLayer].bitBuf));

    /* fixed by NTT 001206 */

    index_scl->ms_mask = 0;

    decoded_bits=0;
    ntt_headerdec(iscl, mat_layerStream,  
                  index_scl, tfData->sfbInfo, 
                  &decoded_bits, tns_info,
                  tfData->nok_lt_status, 
                  tfData->pred_type,
                  hFault );
    ntt_available_bits =  
      index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl]-decoded_bits;
    if ( index->mainDebugLevel>2 ) {
      printf("TTTTTTT available %5d %5d[%d]\n",
             ntt_available_bits,
             index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl],iscl);
    }
    /* bit unpacking */
    tomo_tmp=decoded_bits;
    tomo_tmp2=BsCurrentBit(mat_layerStream);
     decoded_bits += ntt_SclBitUnPack ( mat_layerStream,
                                       index_scl,
					/* T.Mori 20001121 */
#ifdef ESC_SUPPORT
                                        tomo_tmp,
                                        fd,
					nextLayer,
#endif
                                       BsBufferNumBit(fd->layer[iscl+1].bitBuf)
                                       - tomo_tmp,
                                       iscl);

    tfData->decoded_bits += decoded_bits;
    if ( index->mainDebugLevel>2 ) {
      printf("<<<< ntt_SclBitUnPack[%d] %d %d >>>>\n", iscl,decoded_bits-tomo_tmp,decoded_bits);
      printf("<<<< mat_Stream[%d] %ld >>>>\n",
             iscl,BsCurrentBit(mat_layerStream)-tomo_tmp2);
    }
    /* decoding tools */
    if (iscl < tfData->output_select){
      /*NN
        mat_scale_set_shift_para2(iscl);
      */
      ntt_scale_vq_decoder(index_scl, 
                           index,
                           tfData->spectral_line_vector[0], iscl, 
                           tfData->sfbInfo);
      if ( index->mainDebugLevel>2 ) {
        printf("<<<< Scale Layer[%d] Decoding Finished!! ! >>>>\n",iscl);
      }
    }
    
    fd->layer[*nextLayer].NoAUInBuffer--; /* one AU is decoded and removed from bitbuffer*/ /* fixed by T.Mori 1206 */

    /* T.Mori 20001121 */
#ifdef ESC_SUPPORT
    if((index_scl->er_tvq==1) && (fd->od->ESDescriptor[iscl+1]->DecConfigDescr.audioSpecificConfig.epConfig.value==1)){
      (*nextLayer)++;
      fd->layer[*nextLayer].NoAUInBuffer--;
    }
#endif

    (*nextLayer)++;
    BsCloseRemove(mat_layerStream,1);
  }
}

static void tvqHeaderDec (
                          int          numChannels,
                          TF_DATA*     tfData,
                          ntt_INDEX*   index,
                          HANDLE_BSBITSTREAM  mat_layerStream )
{
  int decoded_bits, codedBlockType;
  unsigned long  ultmp;

  decoded_bits = 0;
  BsGetBit( mat_layerStream, &ultmp,2); /* window_sequence */
  codedBlockType = (int)ultmp;
  decoded_bits += 2;
  switch (   codedBlockType )
    {
    case 0:
      tfData->windowSequence[MONO_CHAN] =  ONLY_LONG_SEQUENCE;
      break;
    case 1:
      tfData->windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE;
      break;
    case 2:
      tfData->windowSequence[MONO_CHAN] =  EIGHT_SHORT_SEQUENCE;
      break;
    case 3:
      tfData->windowSequence[MONO_CHAN] =  LONG_STOP_SEQUENCE;
      break;
    default:
      CommonExit(-1,"wrong blocktype %d",   codedBlockType);
    }
  BsGetBit( mat_layerStream, &ultmp,1);
  tfData->windowShape[0] = (WINDOW_SHAPE)ultmp; /* window shape */
  decoded_bits += 1;
  tfData->decoded_bits = decoded_bits;
 

  if (numChannels==2){
    tfData->windowSequence[1]=tfData->windowSequence[MONO_CHAN];
    tfData->windowShape[1] =   tfData->windowShape[MONO_CHAN];
  }

  index->w_type=tfData->windowSequence[MONO_CHAN];
}

void tvqAUDecode( 
                 int          numChannels,
                 FRAME_DATA*  fd, /* config data , obj descr. etc. */
                 TF_DATA*     tfData,
                 HANDLE_DECODER_GENERAL hFault,
                 QC_MOD_SELECT      qc_select,
                 ntt_INDEX*   index,
                 ntt_INDEX*   index_scl,
                 HANDLE_BSBITSTREAM  layer_stream
                 )
     /*  case NTT_VQ_SYS: */
{
  int mat_usedNumBit;
  unsigned int x;
  unsigned long totalLength;
  unsigned int AUindex;
  HANDLE_BSBITSTREAM  mat_layerStream = 0;
  /*HANDLE_BSBITSTREAM  layer_stream ;*/
  int ntt_output_select,dummy;
  int samplFreqIdx = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
        samplingFreqencyIndex.value;

  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  /*
    getModeFromOD(fd->od,&lowRateChannelPresent ,layNumChan ,&lastAacLay ,&firstSt
    Lay,&coreCodecIdx,
    commonWindow);
    mat_layerStream = BsOpenBufferRead( fd->layer[0].bitBuf);
  */
  mat_usedNumBit =0;
  index->mainDebugLevel=tvq_debug_level ;
  if(tvq_debug_level > 5)
    fprintf(stderr, "TTTTTT %5ld %5ld \n", BsCurrentBit(layer_stream),
            fd->od->streamCount.value); 
  for(x=0;x<fd->od->streamCount.value;x++)
    {
      if(tvq_debug_level > 5)
        fprintf(stderr, "TTTTTT %5ld %5d xxx\n", BsCurrentBit(layer_stream), x); 
      totalLength=0;
      getAccessUnit(layer_stream, fd->layer[x].bitBuf , &AUindex, &totalLength, fd->od->ESDescriptor[x]);
      mat_usedNumBit += totalLength * 8 ; /* T.Ishikawa 9807 01 */
      mat_usedNumBit += ((int)((totalLength-(totalLength%255))/255)+1)*24;

      if ( index->mainDebugLevel>2 ) {
        printf(" ##### [%d] totalLength %ld mat_usedNum Bit %d #####\n",x,totalLength, mat_usedNumBit);
      }
      if(x != AUindex) CommonExit(-1,"\n Error in FlexMux De code NTT_VQ_SYS\n");
    }
  /*BsCloseRemove(layer_stream,1);*/
  /*  if ( index->mainDebugLevel>2 ) {
      printf(" ##### totalLength %d #####\n",totalLength);
      } */
  ntt_output_select = tfData->output_select;
  tvqHeaderDec( numChannels,
                tfData,
                index, mat_layerStream);
     
  tvqCoreDec( numChannels,
              fd, /* config data , obj descr. etc. */
              tfData,
              hFault, index, mat_layerStream, tns_info,	qc_select,&dummy);
  tvqScalDec( fd, /* config data , obj descr. etc. */
              tfData,
              hFault, index, index_scl, tns_info,&dummy);
  
  {
    int ch;
    for (ch=0;ch<numChannels;ch++){
      int i, k, j, max_sfb[8];
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      if(ntt_output_select==0) max_sfb[0] = index->max_sfb[0];
      else max_sfb[ntt_output_select] = index_scl->max_sfb[ntt_output_select];

      for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
        Float tmp_spec[1024];
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
        }
        tns_decode_subblock( tmp_spec,
                             max_sfb[ntt_output_select],
                             info->sbk_sfb_top[i],
                             info->islong,
                             &(tns_info[ch].info[i]) ,
                             qc_select,samplFreqIdx);

        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
        }

        j += info->bins_per_sbk[i];
      }
    }
  }
  tfData->decoded_bits = mat_usedNumBit;
}

void tvqAUDecode2( 
                  FRAME_DATA*  fd, /* config data , obj descr. etc. */
                  TF_DATA*     tfData,
                  HANDLE_DECODER_GENERAL hFault,
                  QC_MOD_SELECT      qc_select,
                  ntt_INDEX*   index,
                  ntt_INDEX*   index_scl,
                  int *nextLayer)
     /*  case NTT_VQ_SYS: */
{
  int mat_usedNumBit;
  int samplFreqIdx = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.\
    samplingFreqencyIndex.value;
  HANDLE_BSBITSTREAM  mat_layerStream;
  /*HANDLE_BSBITSTREAM  layer_stream ;*/
  int ntt_output_select;
  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  /* added by NTT 001205 */
#ifdef ESC_SUPPORT
  HANDLE_BSBITSTREAM layerStream_tmp;
#endif
  index->mainDebugLevel=tvq_debug_level ;

  /* added by NTT 001205 */
#ifdef VERSION2
  index->er_tvq=0;
  if( fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value==ER_TWIN_VQ)
   index->er_tvq=1;

#ifdef ESC_SUPPORT
  if((index->er_tvq) && (fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value)){

    layerStream_tmp = BsOpenBufferRead(fd->layer[1].bitBuf);
    BsManipulateSetNumByte(BsBufferNumBit(BsGetBitBufferHandle(0,layerStream_tmp)) / 8, layerStream_tmp);

    BsGetBufferAppend( layerStream_tmp, fd->layer[0].bitBuf, 1, fd->layer[1].AULength[0]);

    BsCloseRemove(layerStream_tmp, 1);

  }
#endif
#endif

  mat_layerStream = BsOpenBufferRead( fd->layer[0].bitBuf);

  ntt_output_select = tfData->output_select;
  tvqHeaderDec( index->numChannel,
                tfData,
                index, mat_layerStream);
     
  mat_usedNumBit =BsCurrentBit(mat_layerStream);

  tvqCoreDec( index->numChannel,
              fd, /* config data , obj descr. etc. */
              tfData,
              hFault, index, mat_layerStream, tns_info,	qc_select,nextLayer);
  mat_usedNumBit =BsCurrentBit(mat_layerStream);
  BsCloseRemove(mat_layerStream,1);     
  
  tvqScalDec( fd,/* config data , obj descr. etc. */
              tfData,
              hFault, index, index_scl, tns_info,nextLayer);
  
  if(ntt_output_select <= index->nttDataScl->ntt_NSclLay)
  {
    int ch;
    for (ch=0;ch<index->numChannel;ch++){
      int i, k, j, max_sfb[8];
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      if(ntt_output_select==0) max_sfb[0] = index->max_sfb[0];
      else max_sfb[ntt_output_select] = index_scl->max_sfb[ntt_output_select];

      for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
        Float tmp_spec[1024];
        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
        }
        tns_decode_subblock( tmp_spec,
                             max_sfb[ntt_output_select],
                             info->sbk_sfb_top[i],
                             info->islong,
                             &(tns_info[ch].info[i]) ,
                             qc_select,samplFreqIdx);

        for( k=0; k<info->bins_per_sbk[i]; k++ ) {
          tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
        }

        j += info->bins_per_sbk[i];
      }
    }
  }
  tfData->decoded_bits = mat_usedNumBit;
}

