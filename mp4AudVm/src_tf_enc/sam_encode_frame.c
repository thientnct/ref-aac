#ifdef  VERSION2
/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-11-06

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

**********************************************************************/
#include <stdio.h>
#include <math.h>

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "tf_mainHandle.h"

#include "tns3.h"                /* structs */

#include "common_m4a.h"
#include "sam_encode.h"


#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "util.h"

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "aac_qc.h"
#include "enc_tf.h"
#include "bitmux.h"
#include "flex_mux.h"

#define  ID_SCE  0x00
#define  ID_CPE  0x01
#define  MAX_PRED_SFB  40

static int num_of_channels;
static int bitrate;
static int sampling_frequency_index;
static int sampling_frequency;
static int frame_length_flag;
static int block_size_samples;
static int  fs_index[16]={
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025,  8000,     0,     0,     0,     0
};

#ifdef __cplusplus
extern "C" {
#endif

static int my_log2(int value);

#ifdef __cplusplus
}
#endif


#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"
#include "bitstream.h"
#include "util.h"

/* ---  AAC --- */
#include "aac.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "aac_qc.h"
#include "util.h"
#include "enc_tf.h"
#include "bitmux.h"
#include "bitstream.h"
#include "flex_mux.h"

#ifdef VERSION2
/* --- BSAC --- */
#include "sam_encode.h"
#endif

static   HANDLE_BSBITBUFFER tmpBitBuf;
static   HANDLE_BSBITBUFFER dynpartBuf;

static   long  samplingRate;

void sam_FlexMuxEncInit_bsac( 
  HANDLE_BSBITSTREAM headerStream,
  int         numChannel,
  long        samplRate,
  ENC_FRAME_DATA * frameData,
  int numES,
  int mainDebugLevel
)
{
  samplingRate = samplRate;
  /* write object descriptor in case of aac_sys */
  if( frameData!=NULL ) {
    int sx;
    int layer;

    /* get the total number of layers: BSAC large-step layer */
    initObjDescr(frameData->od,mainDebugLevel);
    presetObjDescr(frameData->od, numES);
    frameData->od->streamCount.value=numES;

    for(layer=0; layer<numES; layer++) {
      initESDescr( &(frameData->od->ESDescriptor[layer]));
      presetESDescr( frameData->od->ESDescriptor[layer], layer);
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value=frameData->layer[layer].bitRate;
      for (sx=0;sx<16;sx++) { 
         if (samplRate== fs_index[sx]) break;
      } 
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = sx; 
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value=6122;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_BSAC; /* BSAC scalable */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value=0;/* 0: 1023; 1: 960 */

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
    for (layer=0; layer<numES; layer++) {
      advanceESDescr ( headerStream, frameData->od->ESDescriptor[layer] ,1) ; 
    }
  }

  /* allocate memory */
  tmpBitBuf = BsAllocBuffer(8000);
  dynpartBuf =BsAllocBuffer(6000);

  return;
}


/*****************************************************************************/
/*****************************************************************************/

int sam_FlexMuxEncode_bsac(
			   double      *p_spectrum[MAX_TIME_CHANNELS],
			   double      energy[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   WINDOW_SEQUENCE  windowSequence[MAX_TIME_CHANNELS],
			   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			   int         nr_of_sfb[MAX_TIME_CHANNELS],
			   int         average_block_bits,
			   int         available_bitreservoir_bits,
			   int         padding_limit,
			   HANDLE_BSBITSTREAM fixed_stream,
			   HANDLE_BSBITSTREAM var_stream,
			   HANDLE_BSBITBUFFER *gcBitBuf,	/* bit buffer for gain_control_data() */
			   int         nr_of_chan,
			   double      *p_reconstructed_spectrum[MAX_TIME_CHANNELS],
			   int         useShortWindows,
			   WINDOW_SHAPE     window_shape[MAX_TIME_CHANNELS],      /* offers the possibility to select different window functions */
			   int aacAllowScalefacs,
               long        sampling_rate,
			   QC_MOD_SELECT qc_select,
			   PRED_TYPE predictor_type,
			   NOK_LT_PRED_STATUS nok_lt_status[MAX_TIME_CHANNELS],
               int blockSizeSamples,
               int num_window_groups,
               int window_group_length[],
               TNS_INFO *tnsInfo[MAX_TIME_CHANNELS],
               int bandwidth,
               int numES
) 
{

  int lay;

  HANDLE_BSBITSTREAM layerStream;
  int ubits;
  int frameBits = average_block_bits + available_bitreservoir_bits - 150 ;
  int flexmuxOverhead =  3*8*(1 + (frameBits/2040)) * numES;
  int read_bits, layer_bits;
  int total_bits = 0;


  frameBits -=   flexmuxOverhead;

  /* start of bitstream composition */
  layerStream = BsOpenBufferWrite(dynpartBuf);
  ubits = tf_encode_spectrum_aac( p_spectrum,
                                  energy,
                                  allowed_dist,
                                  windowSequence,
                                  sfb_width_table,
                                  nr_of_sfb,
                                  frameBits,
                                  0,
                                  padding_limit,
                                  layerStream, /*tmp_stream,*/
                                  var_stream,
                                  gcBitBuf,
                                  nr_of_chan, /* nr of audio channels */
                                  p_reconstructed_spectrum,
                                  useShortWindows,
                                  window_shape,
                                  aacAllowScalefacs,
                                  sampling_rate,
                                  qc_select,
                                  predictor_type,
                                  nok_lt_status,
                                  blockSizeSamples,
                                  num_window_groups,
                                  window_group_length,
                                  tnsInfo,
                                  bandwidth /* bandwidth in Hz */
                                  );
  BsClose(layerStream);

  /* make bsac_lstep_stream() from dynpartBuf as many as numES and
     pack the bsac_lstep_stream(s) to flexMux packet(s) and
     write to bitstream */

  layerStream = BsOpenBufferRead(dynpartBuf);
  BsManipulateSetNumByte( ubits/8, layerStream);

  /* Base Layer ES */
  BsBufferManipulateSetNumBit(0,tmpBitBuf);
  layer_bits = (int)(blockSizeSamples * 16000. * nr_of_chan / samplingRate);
  layer_bits = (layer_bits + 7) / 8 * 8;

  ubits = ubits - 8 * numES; /* used bits - total minimum payload */

  read_bits = (ubits > layer_bits) ? layer_bits : ubits;
  total_bits = read_bits;
  
  read_bits += 8; /* minimum payload */
  BsGetBuffer(layerStream, tmpBitBuf, read_bits);
  writeFlexMuxPDU(0,fixed_stream,tmpBitBuf);

  /* Enhancement Layers ES */
  if (numES > 1) {
    layer_bits = (ubits-layer_bits)/(numES-1);
    layer_bits = (layer_bits + 7) / 8 * 8;
  }
  for(lay=1; lay<numES; lay++) {
    BsBufferManipulateSetNumBit(0,tmpBitBuf);
    if (total_bits >= ubits) {
        read_bits = 0;
    }
    else if (lay == (numES-1)) {
       read_bits = ubits - total_bits;
       total_bits = ubits;
    }
    else {
      read_bits = layer_bits;
      total_bits += layer_bits;
      if (total_bits > ubits) {
        read_bits = total_bits - ubits;
        total_bits = ubits;
      }
    } 

    read_bits += 8; /* minimum payload */
    BsGetBuffer(layerStream, tmpBitBuf, read_bits);
    writeFlexMuxPDU(lay,fixed_stream,tmpBitBuf);
  }
  BsClose(layerStream);

  return(BsCurrentBit(fixed_stream));
}


void sam_init_encode_bsac(
    int numChannel,
    int sampling_rate,
    int bit_rate,
    int frame_size
)
{
  int  i;

  num_of_channels = numChannel;
  block_size_samples = frame_size;
  if(frame_size == 1024)
    frame_length_flag = 0;
  else
    frame_length_flag = 1;
  for(i = 0; i < 12; i++) if(sampling_rate == fs_index[i]) {
    sampling_frequency_index = i;
    break;
  }
  sampling_frequency = sampling_rate;
  bitrate = bit_rate/num_of_channels;
  if(bitrate > 64000) {
    printf("BSAC: Maximum bitrate 64kbps/ch!\n");
    bitrate = 64000;
  }

  sam_scale_bits_init_enc(sampling_rate, block_size_samples);
}

int startf=1;
int sam_encode_frame(
  int num_of_chan,
  WINDOW_SEQUENCE windowSequence,
  WINDOW_SHAPE  window_shape,
  int sfb_offset[],
  int nr_of_sfs,
  int num_window_groups,
  int window_group_length[],
  int quant[][1024],
  int scfacs[][MAX_SCFAC_BANDS],
  HANDLE_BSBITSTREAM fixed_stream,
  int i_ch,
  int w_flag,
  int avr_bits)
{
  int i, k, ch, w, g, s;
  int b = 0;
  int sfb, maxSfb;
  int frameSize;
  int abits;
  int nch;
  int cband;
  int nr_of_sfb = 0;
  int end_cband[8];
  int stereo_mode;
  int stereo_info[MAX_SCFAC_BANDS];
  int ModelIndex[2][8][32];
  int scalefactors[2][MAX_SCFAC_BANDS];
  int scf[2][8][MAX_SCFAC_BANDS];
  int swb_offset[8][MAX_SCFAC_BANDS];
  int sample_buf[2][1024];
  int *sample[2][8];

  nch = i_ch + 1;

  /* initialize */
  stereo_mode = 0;

  for(sfb = 0; sfb < MAX_SCFAC_BANDS; sfb++) {
    scalefactors[i_ch][sfb] = scfacs[i_ch][sfb];
    stereo_info[sfb] = 0;
  }
  if(w_flag) {
    nch = 1;
    if(num_of_chan == 2) {
      for(i = 0; i < MAX_SCFAC_BANDS; i++) 
        scalefactors[1][i] = scfacs[1][i];
      nch = 2;
    }
  }
  maxSfb = nr_of_sfs / num_window_groups - 3;

  for(ch = i_ch; ch < nch; ch++) {

    if(windowSequence == EIGHT_SHORT_SEQUENCE) {
      int short_block_size=block_size_samples/8;

      s = 0;
      nr_of_sfb = nr_of_sfs / num_window_groups;
      for(g = 0; g < num_window_groups; g++) { 
        sample[ch][g] = &(sample_buf[ch][short_block_size * s]);
        s += window_group_length[g];

        swb_offset[g][0] = 0;
        for(sfb = 0; sfb < nr_of_sfb; sfb++)
          swb_offset[g][sfb+1] = sfb_offset[sfb+1] * window_group_length[g];
        end_cband[g] = (swb_offset[g][maxSfb]+31)/32;
      }

      /* reshuffling */
      s = 0;
      for(g = 0; g < num_window_groups; g++) {
        for (i = 0; i < short_block_size; i+=4) {
          int offset = (128 * s) + (i * window_group_length[g]);
          for (k = 0; k < 4; k++) 
          for(w=0; w<window_group_length[g]; w++, b++)  
            sample_buf[ch][offset+4*w+k] = quant[ch][short_block_size*(w+s)+i+k];
        }
        s += window_group_length[g];
      }

    } else {

      nr_of_sfb = nr_of_sfs;
      num_window_groups = 1;
      window_group_length[0] = 1;
      swb_offset[0][0] = 0;
      for(sfb = 0; sfb < nr_of_sfb; sfb++)
        swb_offset[0][sfb+1] = sfb_offset[sfb+1];

      sample[ch][0] = &(quant[ch][0]);

      end_cband[0] = (swb_offset[0][maxSfb]+31)/32;
    }

    for (g = 0; g < num_window_groups; g++) 
      for (cband = 0; cband < 32; cband++) 
        ModelIndex[ch][g][cband] = 0;

    for(g = 0; g < num_window_groups; g++) {
      int bal, max;

      for (cband = 0; cband < end_cband[g]; cband++) {
        max = 0;
        for (i = cband*32; i < (cband+1)*32; i++) {
          if (abs(sample[ch][g][i])>max) 
            max = abs(sample[ch][g][i]);
        }
        if(max > 0) bal = my_log2(max);
        else    bal = 0;
        if(bal >= 8)
          ModelIndex[ch][g][cband] = bal + 7;
        else
          ModelIndex[ch][g][cband] = bal * 2  - 1;
        if (ModelIndex[ch][g][cband] < 0) 
          ModelIndex[ch][g][cband] = 0; 
      }
    }

    for (g = 0; g < num_window_groups; g++) {
      for (sfb = 0; sfb < maxSfb; sfb++) {
        scf[ch][g][sfb] = scalefactors[ch][(g*nr_of_sfb)+sfb];
      }
    }
  }

  if(avr_bits == 0)
    abits = 10000;
  else
    abits = avr_bits - 8;

  /********** B I T S T R E A M   M A K I N G *********/
  if(w_flag) {
    frameSize = sam_encode_bsac(bitrate, windowSequence, window_shape, sample,
      scf, maxSfb, num_window_groups, window_group_length, swb_offset, nr_of_sfb,
      ModelIndex, stereo_mode, stereo_info, abits, i_ch, nch);
   abits = frameSize;
  }

  frameSize = sam_encode_bsac(bitrate, windowSequence, window_shape, sample,
      scf, maxSfb, num_window_groups, window_group_length, swb_offset, nr_of_sfb,
      ModelIndex, stereo_mode, stereo_info, abits, i_ch, nch);

/*
if(startf) {
printf("INIT\n");
for(i = 0; i < 16; i++)
printf("%2d: %d\n", i, sample[0][0][i]);
}
printf("SIZE:%d, %d\n", frameSize, abits);
startf = 0;
*/
  if(w_flag) {
    /* byte alignment */
    if((frameSize % 8)) {
      frameSize += sam_putbits2bs(0x00, (8 - (frameSize % 8)));
    }
    sam_frame_length_rewrite2bs(frameSize, 0, 11);
/*
printf("FLEN:%d\n", frameSize);
for(i = 0; i < 16; i++)
printf("%2d: %d\n", i, sample[0][0][i]);
startf = 1;
*/

    sam_bsflush(fixed_stream, frameSize);
  } else {
    if(frameSize % 8) 
       frameSize += (8 - (frameSize % 8));
  }

  return frameSize;
}

static int my_log2(int value)
{
  int  i, step;

  if(value < 0) {
    fprintf(stderr, "BSC:my_log2:error : %d\n", value);
    return 0;
  }
  if(value == 0)  return 0;

  step = 2;
  for(i = 1; i < 24; i++) {
    if(value < step) return i;
    step *= 2;
  }
  return ( 1 + (int)(log10((double)value)/log10(2.0)));
}

#endif  /* VERSION 2 */
