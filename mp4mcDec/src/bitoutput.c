/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T

and edited by
  Yasuhiro Toguri (Sony),
  Olaf Kähler (Fraunhofer IIS-A)

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

/* file writing routines */

#include "adifdec.h"
#include "extension_type.h"
#include "config.h"
#include "diagnose.h"
#include "huffinit.h"
#include "intrins.h"

#include "bitinput.h" /* getDecoderBufferSize() */
#include "bitoutput.h"

#ifdef MPEG4V1
#include "ISOMovies.h"

#define MAX_DSI_LEN			1024	    /* bytes */


/* defines missing in older ISOMovies.h */
#ifndef ISOOpenMovieNormal
#define ISOOpenMovieNormal MP4OpenMovieNormal
#define ISOGetMovieDuration MP4GetMovieDuration
#define ISOGetTrackDuration MP4GetTrackDuration
#endif
#else
typedef unsigned int u32;
typedef unsigned char u8;
#endif


/* --- local data --- */

struct tagBitOutput {
  int format;
  FILE *fd;
  char *filename;
  int opened;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISOHandle sampleDataH;

  ISOMovie mp4_moov;
  ISOMedia mp4_media;
  ISOTrack mp4_trak;
  ISOHandle mp4_sampleEntryH;
  ISOHandle mp4_sampleDurationH;
  ISOHandle mp4_sampleSizeH;
#endif
#else
  char **sampleDataH;
#endif
  HANDLE_CRC_CALCULATOR crc;

  /* for buffer i/o */
  char		*bp;              /* pointing to byte to place next bits into */
  int		bp_bits_buffered; /* remember how many bits are already used in that byte */
  char		*block_start;

  int bitrate;   /* indicating the bitrate for ADIF and ADTS... negative value means variable bitrate
                    with -bitrate as peak rate, otherwise specifies the average bitrate */

  /* for ADTS */
  ADTS_Header adts_header;
  int rdb_per_frame;
  int num_rdb_this_frame;
  int bits_from_old_frame;
  int block_byte_counts[MAX_RDB_PER_FRAME];
  int lastBufferFullness;
  int create_CRC;

  /* logging */
  /*static int output_bits_buffered = 0;*/
  struct {
    u8 ele_id;
    u32 bit_position;
  } part_list[1024]; /* 2*max. number of elements in one frame */
  int part_list_len;
};



static void bitoutput_free(HANDLE_BITOUTPUT output);
static void bitoutput_reset(HANDLE_BITOUTPUT output);
static unsigned long bitoutput_reset_CRC(HANDLE_BITOUTPUT output);
static unsigned long bitoutput_getBitCount(HANDLE_BITOUTPUT output);


static HANDLE_BITOUTPUT bitoutput_alloc()
{
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISOErr err = ISONoErr;
#endif
#endif
  int i;
  HANDLE_BITOUTPUT ret = (HANDLE_BITOUTPUT)malloc(sizeof(struct tagBitOutput));

  ret->format = -1;
  ret->fd = NULL;
  ret->filename = NULL;
  ret->opened = 0;
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  err = ISONewHandle( MAX_FRAME_LEN*MAX_RDB_PER_FRAME, &ret->sampleDataH );
  if (err) {
    free(ret); return NULL;
  }
  ret->mp4_moov = NULL;
  ret->mp4_media = NULL;
  ret->mp4_trak = NULL;
  ret->mp4_sampleEntryH = NULL;
  ret->mp4_sampleDurationH = NULL;
  ret->mp4_sampleSizeH = NULL;
#endif
#else
  ret->sampleDataH = (char**)mal1(sizeof(char*));
  if (ret->sampleDataH==NULL) {
    free(ret); return NULL;
  }
  *(ret->sampleDataH) = (char*)mal1(MAX_FRAME_LEN*MAX_RDB_PER_FRAME);
  if (*(ret->sampleDataH)==NULL) {
    free(ret->sampleDataH);
    free(ret); return NULL;
  }
#endif

  ret->crc = CRC_calculator_create();

  bitoutput_reset(ret);
  bitoutput_reset_CRC(ret);

  ret->bitrate = 0;

  /* for ADTS */
  ret->rdb_per_frame = 1;
  ret->num_rdb_this_frame = 0;
  for (i=0; i<MAX_RDB_PER_FRAME; i++) ret->block_byte_counts[i] = 0;
  ret->lastBufferFullness = 0;
  ret->create_CRC = 0;

  ret->part_list_len = 0;

  return ret;
}


static void bitoutput_free(HANDLE_BITOUTPUT output)
{
  if (output==NULL) return;
  if (output->filename!=NULL) free(output->filename);
  CRC_calculator_free(output->crc);
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISODisposeHandle(output->sampleDataH );
#endif
#else
  free(*output->sampleDataH);
  free(output->sampleDataH);
#endif
  free(output);
}




/* --- functions --- */
static int create_prog_config(MC_Info* mip, ProgConfig* pc)
     /*
      * create a program config element using given MC_Info
      */
{
  int i;

  pc->profile = mip->profile;
  pc->sampling_rate_idx = mip->sampling_rate_idx;

  for (i = pc->front.num_ele = 0; i<FChans; i++) {
    if (mip->ch_info[i].present && ((!mip->ch_info[i].cpe)^mip->ch_info[i].ch_is_left)) {
      pc->front.ele_is_cpe[pc->front.num_ele] = mip->ch_info[i].cpe;
      pc->front.ele_tag[pc->front.num_ele] = mip->ch_info[i].tag;
      pc->front.num_ele++;
    }
  }
  for (pc->side.num_ele = 0; i<FChans+SChans; i++) {
    if (mip->ch_info[i].present && ((!mip->ch_info[i].cpe)^mip->ch_info[i].ch_is_left)) {
      pc->side.ele_is_cpe[pc->side.num_ele] = mip->ch_info[i].cpe;
      pc->side.ele_tag[pc->side.num_ele] = mip->ch_info[i].tag;
      pc->side.num_ele++;
    }
  }
  for (pc->back.num_ele = 0; i<FChans+SChans+BChans; i++) {
    if (mip->ch_info[i].present && ((!mip->ch_info[i].cpe)^mip->ch_info[i].ch_is_left)) {
      pc->back.ele_is_cpe[pc->back.num_ele] = mip->ch_info[i].cpe;
      pc->back.ele_tag[pc->back.num_ele] = mip->ch_info[i].tag;
      pc->back.num_ele++;
    }
  }
  for (pc->lfe.num_ele = 0; i<FChans+SChans+BChans+LChans; i++) {
    if (mip->ch_info[i].present && ((!mip->ch_info[i].cpe)^mip->ch_info[i].ch_is_left)) {
      pc->lfe.ele_is_cpe[pc->lfe.num_ele] = mip->ch_info[i].cpe;
      pc->lfe.ele_tag[pc->lfe.num_ele] = mip->ch_info[i].tag;
      pc->lfe.num_ele++;
    }
  }
  for (i = pc->coupling.num_ele = 0; i<mip->ncch; i++) {
    pc->coupling.ele_is_cpe[pc->coupling.num_ele] = mip->cc_ind[i];
    pc->coupling.ele_tag[pc->coupling.num_ele] = mip->cc_tag[i];
    pc->coupling.num_ele++;
  }

  for (pc->data.num_ele = 0; pc->data.num_ele<prog_config.data.num_ele; ) {
    pc->data.ele_tag[pc->data.num_ele++] = prog_config.data.ele_tag[pc->data.num_ele];
  }

  pc->mono_mix.present = 0;
  pc->stereo_mix.present = 0;
  pc->matrix_mix.present = 0;

  strcpy( pc->comments, prog_config.comments );

  return 0;
}






static unsigned long bitoutput_reset_CRC(HANDLE_BITOUTPUT output)
{
  output->part_list_len = 0;
  return CRC_reset(output->crc);
}


static void bitoutput_reset(HANDLE_BITOUTPUT output)
{
  /* for buffer i/o */
  output->bp_bits_buffered = 0;
  output->bp = *(output->sampleDataH);
  output->block_start = output->bp;
}



void bitoutput_startNewElement(HANDLE_BITOUTPUT output, int id)
{
  output->part_list[output->part_list_len].ele_id = id;
  output->part_list[output->part_list_len++].bit_position = bitoutput_getBitCount(output);
}





/*read the stream again for CRC-calculation, before writing to disk*/
static void feed_outstream_into_CRC(HANDLE_CRC_CALCULATOR crc,
                                    char *start_of_out_frame,
                                    unsigned long pos,
                                    int length_of_CRC_ele,
                                    int length_of_stream_ele)
{
  char* out_stream_ptr = start_of_out_frame + (pos>>3);
  int bitpos = pos&7, bitsToFeed;

  if (length_of_CRC_ele==-1) length_of_CRC_ele = length_of_stream_ele;
  if (length_of_stream_ele>length_of_CRC_ele) length_of_stream_ele=length_of_CRC_ele;

  bitsToFeed = length_of_stream_ele;

  CRC_enable(crc, length_of_CRC_ele, 0);

  if (bitpos) {
    int len = bitsToFeed, tmp = (unsigned char)*out_stream_ptr++;
    if (len>(8-bitpos)) len = 8-bitpos;
    if (bitsToFeed<(8-bitpos)) tmp>>=(8-bitpos-bitsToFeed);
    CRC_feed_calculator(crc, len, tmp&((1<<len)-1));
    bitsToFeed-=len;
  }
  while (bitsToFeed>=8) {
    CRC_feed_calculator(crc, 8,(unsigned char)*out_stream_ptr++);
    bitsToFeed-=8;
  }
  if (bitsToFeed) {
    CRC_feed_calculator(crc, bitsToFeed, ((unsigned char)*out_stream_ptr)>>(8-bitsToFeed));
  }

  if (length_of_stream_ele!=length_of_CRC_ele) {
    CRC_feed_calculator(crc, length_of_CRC_ele-length_of_stream_ele,0);
  }
}


static unsigned short calculate_output_CRC(HANDLE_BITOUTPUT output)
{
  int i = 0;
  unsigned long pos1, pos2;
  char *frame_pointer = *output->sampleDataH;
  HANDLE_CRC_CALCULATOR crc = output->crc;

  while (i<output->part_list_len) {
    switch (output->part_list[i].ele_id) {
    case ID_SCE:
    case ID_CCE:
    case ID_LFE:
      pos1 = output->part_list[i].bit_position + LEN_SE_ID;
      if (output->part_list[++i].ele_id != LOG_FIN) {
        PRINT(SE,"calculate_output_CRC: unexpected combination!\n");
        return -1;
      }
      feed_outstream_into_CRC(crc, frame_pointer, pos1, CRC_LEN_ELE, output->part_list[i++].bit_position - pos1);
      break;
    case ID_CPE:
      pos1 = output->part_list[i].bit_position + LEN_SE_ID;
      if (output->part_list[++i].ele_id != ID_2ND) {
        PRINT(SE,"calculate_output_CRC: unexpected combination!\n");
        return -1;
      }
      pos2 = output->part_list[i].bit_position;
      if (output->part_list[++i].ele_id != LOG_FIN) {
        PRINT(SE,"calculate_output_CRC: unexpected combination!\n");
        return -1;
      }
      feed_outstream_into_CRC(crc, frame_pointer, pos1, CRC_LEN_ELE, output->part_list[i].bit_position-pos1);
      feed_outstream_into_CRC(crc, frame_pointer, pos2, CRC_LEN_2ND, output->part_list[i++].bit_position-pos2);
      break;
    case ID_DSE:
    case ID_PCE:
      pos1 = output->part_list[i].bit_position + LEN_SE_ID;
      if (output->part_list[++i].ele_id != LOG_FIN) {
        PRINT(SE,"calculate_output_CRC: unexpected combination!\n");
        return -1;
      }
      feed_outstream_into_CRC(crc, frame_pointer, pos1, CRC_LEN_ALL, output->part_list[i++].bit_position-pos1);
      break;
    case ID_FIL:
    case ID_HDR:
    case ID_NUL:
      if (output->part_list[++i].ele_id != LOG_FIN) {
        PRINT(SE,"calculate_output_CRC: unexpected combination!\n");
        return -1;
      }
    case ID_END:
#ifdef NEW_ADTS
    case END_HDR:
#endif
      i++;
      break;
    case ID_2ND:
    default:
      PRINT(SE,"calculate_output_CRC: can't calculate CRC\n");
    }
  }
  return (unsigned short)bitoutput_reset_CRC(output);
}



/* bits and bytes */
static void put_bits(char **buffer, int *bits_buffered, unsigned long value, int length)
{
  int buf_bits = *bits_buffered;
  char *buf_ptr = *buffer;

  /*PRINT(SE, "putbits: %x %i (%x) + %i %i", buf_ptr, buf_bits, *buf_ptr, length, value);*/

  if (length<32) value &= (1<<length)-1;
  if (buf_bits) {
    int write_bits=(8-buf_bits);
    if (write_bits>length) write_bits=length;
    *buf_ptr |= (unsigned char)((value>>(length-write_bits))<<(8-buf_bits-write_bits));
    length -= write_bits;
    if ((buf_bits+=write_bits)==8) {
      buf_ptr++;buf_bits=0;
    }
  }
  while (length >= 8) {
    length-=8;
    *buf_ptr++ = (unsigned char)(value>>length);
  }
  if (length) {
    *buf_ptr=(unsigned char)(value<<(8-length));
    buf_bits = length;
  }

  /*PRINT(SE, " => %x %i (%x)\n", buf_ptr, buf_bits, *buf_ptr);*/

  *bits_buffered = buf_bits;
  *buffer = buf_ptr;
}

static void rewind_bits(char **buffer, int *bits_buffered, int length)
{
  int buf_bits = *bits_buffered;
  char *buf_ptr = *buffer;
  int bits = length;

  if (buf_bits<bits) {
    bits -=buf_bits;
    *buf_ptr = 0;
    buf_bits = 0;
  }

  while (bits>=8) {
    buf_ptr--;
    *buf_ptr = 0;
    bits -= 8;
  }

  if (bits>0) {
    buf_bits -= bits;
    if (buf_bits<0) {
      buf_bits+=8;
      buf_ptr--;
    }
    *buf_ptr &= ((1<<buf_bits)-1)<<(8-buf_bits);
  }
    
  *bits_buffered = buf_bits;
  *buffer = buf_ptr;
}



static int put_byte_align(char **buffer, int *bits_buffered)
{
  int len = (8-*bits_buffered) & 7;
  put_bits(buffer, bits_buffered, 0, len);
  return len;
}



/* wrappers to access the frame buffer */
static unsigned long bitoutput_getBitCount(HANDLE_BITOUTPUT output)
{ return ((output->bp - *output->sampleDataH)<<3) + output->bp_bits_buffered; }
unsigned long bitoutput_getBlockBitCount(HANDLE_BITOUTPUT output)
{ return ((output->bp - output->block_start)<<3) + output->bp_bits_buffered; }

static unsigned long bitoutput_getBlockByteCount(HANDLE_BITOUTPUT output)
{ return (bitoutput_getBlockBitCount(output)+7)>>3; }

void bitoutput_PutBits(HANDLE_BITOUTPUT output, int length, unsigned long value)
{ put_bits(&output->bp, &output->bp_bits_buffered, value, length); }

void bitoutput_RewindBits(HANDLE_BITOUTPUT output, int length)
{ rewind_bits(&output->bp, &output->bp_bits_buffered, length); }


int bitoutput_ByteAlign(HANDLE_BITOUTPUT output)
{ return put_byte_align(&output->bp, &output->bp_bits_buffered); }





/* ------------------------------------- 
 *  -     Program Configuration       -
 * ------------------------------------- */


static void put_ele_list(char **pb, int *bbit, EleList *p, int flag )
{
  int i;
  for (i=0; i<p->num_ele; i++) {
    if (flag)
      put_bits(pb, bbit, p->ele_is_cpe[i], LEN_ELE_IS_CPE);
    /* tag must agree with that in bitstream! */
    put_bits(pb, bbit, p->ele_tag[i], LEN_TAG);
  }
}

static void put_prog_config(char **pb, int *bbit, int tag, ProgConfig *p )
{
  int i, j;
  
  put_bits(pb, bbit, tag, LEN_TAG);
  put_bits(pb, bbit, p->profile, LEN_PROFILE);
  put_bits(pb, bbit, p->sampling_rate_idx, LEN_SAMP_IDX);
  put_bits(pb, bbit, p->front.num_ele, LEN_NUM_ELE);
  put_bits(pb, bbit, p->side.num_ele, LEN_NUM_ELE);
  put_bits(pb, bbit, p->back.num_ele, LEN_NUM_ELE);
  put_bits(pb, bbit, p->lfe.num_ele, LEN_NUM_LFE);
  put_bits(pb, bbit, p->data.num_ele, LEN_NUM_DAT);
  put_bits(pb, bbit, p->coupling.num_ele, LEN_NUM_ELE);
  put_bits(pb, bbit, 0, LEN_MIX_PRES);   /* no mono mixdown */
  put_bits(pb, bbit, 0, LEN_MIX_PRES);   /* no stereo mixdown */
  put_bits(pb, bbit, 0, LEN_MIX_PRES);   /* no matrix mixdown */
  
  /* front, side, back sce or cpe */
  put_ele_list(pb, bbit, &p->front, 1);
  put_ele_list(pb, bbit, &p->side, 1);
  put_ele_list(pb, bbit, &p->back, 1);
  put_ele_list(pb, bbit, &p->lfe, 0);
  put_ele_list(pb, bbit, &p->data, 0);
  put_ele_list(pb, bbit, &p->coupling, 1);
  
  /* byte align */
  put_byte_align(pb, bbit);
  
  /* comment */
  j = strlen(p->comments);
  put_bits(pb, bbit, j, LEN_COMMENT_BYTES);
  for (i=0; i<j; i++)
    put_bits(pb, bbit, p->comments[i], 8);
}



/* ------------------------------------- 
 *  -     A D I F                     -
 * ------------------------------------- */


static long put_adif_header(ADIF_Header source, ProgConfig my_prog_config, char *dest )
{
  char* startpos=dest;
  int i;

  int bbit = 0; /* byte align bit stream */

  for (i=0; i<LEN_ADIF_ID; i++)
    put_bits(&dest, &bbit, source.adif_id[i], LEN_BYTE); 

  put_bits(&dest, &bbit, source.copy_id_present, LEN_COPYRT_PRES);
  if (source.copy_id_present) {
    for (i=0; i<LEN_COPYRT_ID; i++)
      put_bits(&dest, &bbit, source.copy_id[i], LEN_BYTE); 
  }

  put_bits(&dest, &bbit, source.original_copy, LEN_ORIG);
  put_bits(&dest, &bbit, source.home, LEN_HOME);
  put_bits(&dest, &bbit, source.bitstream_type, LEN_BS_TYPE);
  put_bits(&dest, &bbit, source.bitrate, LEN_BIT_RATE);
  put_bits(&dest, &bbit, 0 /*num_program_config_elements*/, LEN_NUM_PCE);

  if (source.bitstream_type == 0)
    put_bits(&dest, &bbit, source.buffer_fullness, LEN_ADIF_BF);

  put_prog_config(&dest, &bbit, 0 /*tag*/, &my_prog_config);

  /*byte align*/
  put_byte_align(&dest, &bbit);

  return (dest-startpos);
}

static int write_adif_header(HANDLE_BITOUTPUT output, MC_Info* mip, int bitrate, int fullness)
     /*
      * write ADIF header to file
      */
{
  char buf[1024];
  ProgConfig my_prog_config;
  ADIF_Header my_adif_header;
  long length;

  my_adif_header.adif_id[0] = 'A';
  my_adif_header.adif_id[1] = 'D';
  my_adif_header.adif_id[2] = 'I';
  my_adif_header.adif_id[3] = 'F';

  my_adif_header.copy_id_present = 0;
  my_adif_header.original_copy = 0;
  my_adif_header.home = 0;
  my_adif_header.bitstream_type = (fullness==0x7fffffff);
  my_adif_header.bitrate = bitrate;
  my_adif_header.buffer_fullness = fullness;

  create_prog_config(mip, &my_prog_config);

  length = put_adif_header( my_adif_header, my_prog_config, buf );

  if (fwrite(buf, 1, length, output->fd) != (size_t)length) return -1;
  return 0;
}





/* ------------------------------------- 
 *  -     A D T S                     -
 * ------------------------------------- */


static long put_adts_header(ADTS_Header source, char* dest)
{
  char* startpos=dest;
#ifdef WRITE_NEW_ADTS
  int i;
#endif

  int bbit = 0; /* byte align bit stream */

  put_bits(&dest, &bbit, source.syncword, LEN_SYNCWORD);
  put_bits(&dest, &bbit, source.id, LEN_ID);
  put_bits(&dest, &bbit, source.layer, LEN_LAYER);
  put_bits(&dest, &bbit, source.protection_abs, LEN_PROTECT_ABS);
  put_bits(&dest, &bbit, source.profile, LEN_PROFILE);
  put_bits(&dest, &bbit, source.sampling_freq_idx, LEN_SAMP_IDX);
  put_bits(&dest, &bbit, source.private_bit, LEN_PRIVTE_BIT);
  put_bits(&dest, &bbit, source.channel_config, LEN_CHANNEL_CONF);
  put_bits(&dest, &bbit, source.original_copy, LEN_ORIG);
  put_bits(&dest, &bbit, source.home, LEN_HOME);
  put_bits(&dest, &bbit, source.copyright_id_bit, LEN_COPYRT_ID_ADTS);
  put_bits(&dest, &bbit, source.copyright_id_start, LEN_COPYRT_START);
  put_bits(&dest, &bbit, source.frame_length, LEN_FRAME_LEN);
  put_bits(&dest, &bbit, source.adts_buffer_fullness, LEN_ADTS_BUF_FULLNESS);
  put_bits(&dest, &bbit, source.num_of_rdb, LEN_NUM_OF_RDB);
  if(source.protection_abs == 0){
#ifdef WRITE_NEW_ADTS
    for (i=0;i<source.num_of_rdb;i++)
      put_bits(&dest, &bbit, source.rdb_position[i], LEN_RDB_POSITION);
#endif
    put_bits(&dest, &bbit, source.crc_check, LEN_CRC);
  }

  /*byte align*/

  put_byte_align(&dest, &bbit);

  return (dest-startpos);
}


static int write_adts_header(HANDLE_BITOUTPUT output, MC_Info* mip, int fullness, int *blockLen, int num_RDB)
     /*
      * write one ADTS-Header to file
      */
{
  char buf[16];
  int length;
  int i;

  output->adts_header.syncword = 0xfff;
#ifndef MPEG4V1
  output->adts_header.id = 1;
#endif
  output->adts_header.layer = 0;
  output->adts_header.protection_abs = (output->create_CRC)?0:1;
  output->adts_header.profile = mip->profile;
  output->adts_header.sampling_freq_idx = mip->sampling_rate_idx;
  output->adts_header.private_bit = 0;
  output->adts_header.original_copy = 0;
  output->adts_header.home = 0;
  output->adts_header.copyright_id_bit = 0;
  output->adts_header.copyright_id_start = 0;
  output->adts_header.frame_length = 0;
  output->adts_header.adts_buffer_fullness = fullness;
  output->adts_header.num_of_rdb = num_RDB -1;

  for (i=0; i<num_RDB; i++) {
    output->adts_header.frame_length+=blockLen[i];
  }
  output->adts_header.frame_length+=ADTS_HEADER_SIZE(output->adts_header);

#ifdef WRITE_NEW_ADTS
  length=0;
  for (i=0; i<output->adts_header.num_of_rdb; i++) {
    length+=blockLen[i];
    output->adts_header.rdb_position[i]=length;
  }
#endif

  if (mip->nch<=6) output->adts_header.channel_config = mip->nch;else
  if (mip->nch==8) output->adts_header.channel_config = 7;else {
    PRINT(SE, "number of channels\n");
    return 0;
  }

  output->adts_header.crc_check = 0;
  length = put_adts_header( output->adts_header, buf );

  if (output->create_CRC) {
    CRC_reset(output->crc);
    CRC_enable(output->crc, CRC_LEN_HDR, 0);
    for (i=0;i<(CRC_LEN_HDR>>3);i++)
      CRC_feed_calculator(output->crc, 8,(unsigned char)buf[i]);
#ifdef WRITE_NEW_ADTS
    CRC_enable(output->crc, (output->adts_header.num_of_rdb*16), 0);
    for (i=0; i<output->adts_header.num_of_rdb; i++) {
      CRC_feed_calculator(output->crc, 16,output->adts_header.rdb_position[i]);
    }
    if (output->adts_header.num_of_rdb) output->adts_header.crc_check = CRC_reset(output->crc);
    else
#endif
      output->adts_header.crc_check = calculate_output_CRC(output);

    length = put_adts_header(output->adts_header, buf);
  }

  if (fwrite(buf, 1, length, output->fd) != (size_t)length) return 0;
  return length;
}





#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
/* ------------------------------------- 
 *  -     M P 4 f f                   -
 * ------------------------------------- */


/* fill in values into a ISOMP4 decoderSpecificInfo handle */
static int put_decoderSpecificInfo(ISOHandle decoderSpecificInfoH, int channelConfig, int sampRate, ProgConfig *pc
#ifdef CT_SBR
                                   ,int sbrPresentInFillElem
                                   ,int signallingType
                                   ,int sbrPresentFlag
                                   ,int downSampledSbr
#ifdef PARAMETRICSTEREO
                                   ,int psSignallingType
                                   ,int psPresentFlag
#endif
#endif
#ifdef FRAMELENGTH_FLAG
                                   ,int frameLengthFlag
#endif
                                   )
{
  ISOErr err;
  char *dsi, *plen;
  int i,n;
  int bbit = 0;

  dsi = (char *) *decoderSpecificInfoH;

  /* tag is DecSpecificInfoTag, 14496-1, Sec 8.2.2.2, Table 1 */
  put_bits( &dsi, &bbit, 0x05, 8 );
  /* dummy sizeOfInstance, 14496-1, Sec 12.3.3 */
  plen = dsi;
  put_bits( &dsi, &bbit, 0, 8 );

#if CT_SBR
  /* Non-backwards compatible explicit signalling*/
  if((signallingType == 1
#ifdef PARAMETRICSTEREO
      || psSignallingType == 1
#endif
      ) && sbrPresentInFillElem) {
    int aot = 5;
#ifdef PARAMETRICSTEREO
    aot = psSignallingType == 1 ? 29 /* PS */ : 5 /* SBR */;
#endif
    /* Audio Specific Configuration */
    put_bits( &dsi, &bbit, aot, lenAudioObjectType ); /* SBR */
    i = getNextSampFreqIndex(sampRate);
    if (i==-1) {
      PRINT(SE, "can not handle sampling frequency %d\n",sampRate);
      return -1;
    }
    if ( sampRate != sampling_rate[i] ) {
      put_bits( &dsi, &bbit, samplingFrequencyEscape, lenSamplingFrequencyIndex );
      put_bits( &dsi, &bbit, sampRate, lenSamplingFrequency );
    } else {
      put_bits( &dsi, &bbit, i, lenSamplingFrequencyIndex );
    }
    put_bits( &dsi, &bbit, channelConfig, lenChannelConfiguration );


    sampRate = sampRate*(2-downSampledSbr);
  
    i = getNextSampFreqIndex(sampRate);
    if (i==-1) {
      PRINT(SE, "can not handle sampling frequency %d\n",sampRate);
      return -1;
    }
    if ( sampRate != sampling_rate[i] ) {
      put_bits( &dsi, &bbit, samplingFrequencyEscape, lenSamplingFrequencyIndex );
      put_bits( &dsi, &bbit, sampRate, lenSamplingFrequency );
    } else {
      put_bits( &dsi, &bbit, i, lenSamplingFrequencyIndex );
    }

    /* Audio Specific Configuration */
    put_bits( &dsi, &bbit, pc->profile+1, lenAudioObjectType ); /* For Main LC SSR LTP */
  }
  else{

    /* Audio Specific Configuration */
    put_bits( &dsi, &bbit, pc->profile+1, lenAudioObjectType ); /* For Main LC SSR LTP */
    i = getNextSampFreqIndex(sampRate);
    if (i==-1) {
      PRINT(SE, "can not handle sampling frequency %d\n",sampRate);
      return -1;
    }
    if ( sampRate != sampling_rate[i] ) {
      put_bits( &dsi, &bbit, samplingFrequencyEscape, lenSamplingFrequencyIndex );
      put_bits( &dsi, &bbit, sampRate, lenSamplingFrequency );
    } else {
      put_bits( &dsi, &bbit, i, lenSamplingFrequencyIndex );
    }
    put_bits( &dsi, &bbit, channelConfig, lenChannelConfiguration );
  }

#else
  
  /* Audio Specific Configuration */
  put_bits( &dsi, &bbit, pc->profile+1, lenAudioObjectType ); /* For Main LC SSR LTP */
  i = getNextSampFreqIndex(sampRate);
  if (i==-1) {
    PRINT(SE, "can not handle sampling frequency %d\n",sampRate);
    return -1;
  }
  if ( sampRate != sampling_rate[i] ) {
    put_bits( &dsi, &bbit, samplingFrequencyEscape, lenSamplingFrequencyIndex );
    put_bits( &dsi, &bbit, sampRate, lenSamplingFrequency );
  } else {
    put_bits( &dsi, &bbit, i, lenSamplingFrequencyIndex );
  }
  put_bits( &dsi, &bbit, channelConfig, lenChannelConfiguration );
  
#endif


#ifdef FRAMELENGTH_FLAG
  put_bits( &dsi, &bbit, frameLengthFlag, lenFrameLengthFlag );
#else
  put_bits( &dsi, &bbit,               0, lenFrameLengthFlag );
#endif
  put_bits( &dsi, &bbit, 0, lenDependsOnCoreCoder );
  put_bits( &dsi, &bbit, 0, lenExtensionFlag );

  if (!channelConfig) {  /* pce */
    put_prog_config( &dsi, &bbit, 0, pc);

    if (bbit != 0) {
      PRINT(SE, "PCE write byte alignment error\n");
      return -1;
    }
  }

#if CT_SBR

  /* Backward compatible explicit signalling*/
  if((signallingType == 2
#ifdef PARAMETRICSTEREO
      || psSignallingType == 2
#endif
      ) && sbrPresentInFillElem){
    put_bits( &dsi, &bbit, 0x2b7, 11 );
    put_bits( &dsi, &bbit, 5, lenAudioObjectType ); /* SBR */

    put_bits( &dsi, &bbit, sbrPresentFlag, 1 );

    sampRate = sampRate*(2-downSampledSbr);

    i = getNextSampFreqIndex(sampRate);
    if (i==-1) {
      PRINT(SE, "can not handle sampling frequency %d\n",sampRate);
      return -1;
    }
    if ( sampRate != sampling_rate[i] ) {
      put_bits( &dsi, &bbit, samplingFrequencyEscape, lenSamplingFrequencyIndex );
      put_bits( &dsi, &bbit, sampRate, lenSamplingFrequency );
    } else {
      put_bits( &dsi, &bbit, i, lenSamplingFrequencyIndex );
    }
#ifdef PARAMETRICSTEREO
    if (psSignallingType == 2) {
      put_bits( &dsi, &bbit, 0x548, 11 );
      put_bits( &dsi, &bbit, psPresentFlag, 1 );
    }
#endif
  }

  /* byte align */
  put_byte_align(&dsi, &bbit);
#endif

  /* set exact handle size */
  n = (dsi - *decoderSpecificInfoH) + ((7+bbit)>>3);
  err = ISOSetHandleSize( decoderSpecificInfoH, n ); if (err) return err;

  /* set length of decoderSpecificInfoH part in header,
   * encoded as sizeOfInstance
   */
  if (n < 128) {
    *plen = n-2;
  } else {
    /* encoding of size >= 128 bytes currently not supported */
    PRINT(SE, "decoderSpecificInfo too large\n");
    return -1;
  }

#if 0
  PRINT(SE, "decoderSpecificInfo length %d\n", n);
  dsi = *decoderSpecificInfoH;
  for (i=0; i<n; i++)
    PRINT(SE, "%3i 0x%02x\n", i, *dsi++); 
#endif

  return 0;
}
#endif
#endif 





/* try to put <count> FILL bits into output-stream, return achived number of bits
 * if round_up == 0: put [<count>-6, ... , <count>] bit
 * if round_up == 1: put [<count>, ... , <count>+6] bit
 */
static int put_fill_bits(HANDLE_BITOUTPUT output, int totalcount,int round_up)
{
  int bitsLeft=totalcount-(LEN_SE_ID+LEN_F_CNT),count;

  if (totalcount<=0) {return 0;}
  if ((bitsLeft<0)&&(!round_up)) {return 0;}
  if ((bitsLeft>>3)>=((1<<LEN_F_CNT)-1)) {
    count=(1<<LEN_F_CNT)-1;
  } else {
    count=bitsLeft>>3;
    if (count<0) count=0;
  }
  bitoutput_PutBits(output, LEN_SE_ID+LEN_F_CNT, (ID_FIL<<LEN_F_CNT)|count);
  bitsLeft -= (count<<3);

  if (count==(1<<LEN_F_CNT)-1) { /* more bit for length info */
    int more_cnt;
    bitsLeft -= LEN_F_ESC;
    if ((more_cnt = (bitsLeft>>3)+1)>(1<<LEN_F_ESC)-1)
      more_cnt=(1<<LEN_F_ESC)-1; /* will need another FILL-Element then */
    bitoutput_PutBits(output, LEN_F_ESC, more_cnt);
    count+=more_cnt-1;
    bitsLeft-=(more_cnt-1)<<3;
  }

  if (count) {bitoutput_PutBits(output, LEN_EXT_TYPE+LEN_NIBBLE, EXT_FILL_DATA<<LEN_NIBBLE);}
  for (--count;count>0;count--) bitoutput_PutBits(output, LEN_BYTE, 0xa5); /* fill in "10100101"; see 138187 Cor 1 */
  if (bitsLeft>0) {
    bitsLeft-=put_fill_bits(output, bitsLeft, round_up);
  }

  return totalcount-bitsLeft;
}

/* remove ID_END, add FILL-elements (<count> _bits_) and write a new ID_END, returns number of added _bytes_ */
int bitoutput_MoveEnd(HANDLE_BITOUTPUT output, int count, int round_up)
{
  int fill_cnt,extracount=(8-output->bp_bits_buffered)&7;

  /* correctly handle byte_align bits */
  if (round_up) {
    fill_cnt=(((count+7)>>3)<<3)-(7-extracount);
  } else {
    fill_cnt=((count>>3)<<3)+extracount ;
  }

  /* delete old ID_END */
  bitoutput_RewindBits(output, LEN_SE_ID);

  /* add the FILL elements */
  count = put_fill_bits(output, fill_cnt, round_up);

  /* add new ID_END */
  bitoutput_PutBits(output,LEN_SE_ID,ID_END);

  return count;
}





/* open output file ----------------------------------------------- */
HANDLE_BITOUTPUT bitoutput_openFile(const char *filename, int format, MC_Info *mip, int channelConfig,
                                    long maxBitrate,
                                    long avgBitrate
#ifdef CT_SBR
                                    ,int sbrPresentInFillElem
                                    ,int signallingType
                                    ,int sbrPresentFlag
                                    ,int downSampledSbr
#ifdef PARAMETRICSTEREO
                                    ,int psSignallingType
                                    ,int psPresentFlag
#endif
#endif
#ifdef FRAMELENGTH_FLAG
                                    ,int frameLengthFlag
#endif
                                    )
{
  int err = 0;
  HANDLE_BITOUTPUT output = bitoutput_alloc();
  if (output==NULL) return NULL;

#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  if (format==OUT_MP4FF) {
    u8 OD_profileAndLevel;
    u8 scene_profileAndLevel;
    u8 audio_profileAndLevel;
    u8 visual_profileAndLevel;
    u8 graphics_profileAndLevel;
    u32 initialObjectDescriptorID;
    u32 objectTypeIndication;
    u32 streamType;
    long decoderBufferSize;
    ISOHandle decoderSpecificInfoH;

    output->filename = malloc(strlen(filename)+1);
    strcpy(output->filename, filename);

    /*    if ((!mpeg4_header_present)||(mc_info_changed)) */{
    /*    int channelConfig = 0;   */
      ProgConfig pc;
      create_prog_config(mip, &pc);
      getDecoderBufferSize(&pc, channelConfig, &decoderBufferSize);

      err = ISONewHandle( MAX_DSI_LEN, &decoderSpecificInfoH ); if (err) goto bail;
      err = put_decoderSpecificInfo(decoderSpecificInfoH, channelConfig, mip->sampling_rate, &pc
#ifdef CT_SBR
                                    ,sbrPresentInFillElem
                                    ,signallingType
                                    ,sbrPresentFlag
                                    ,downSampledSbr
#ifdef PARAMETRICSTEREO
                                    ,psSignallingType
                                    ,psPresentFlag
#endif
#endif
#ifdef FRAMELENGTH_FLAG
                                    ,frameLengthFlag
#endif
                                    );
    }
    initialObjectDescriptorID = 1;
    OD_profileAndLevel = 0xff; /* none required */
    scene_profileAndLevel = 0xff; /* none required */
    audio_profileAndLevel = 0xfe; /* TODO: some realistic value needed */
    visual_profileAndLevel = 0xff; /* none required */
    graphics_profileAndLevel = 0xff; /* none required */
    err = MP4NewMovie( &output->mp4_moov,
                       initialObjectDescriptorID,
                       OD_profileAndLevel,
                       scene_profileAndLevel,
                       audio_profileAndLevel,
                       visual_profileAndLevel,
                       graphics_profileAndLevel ); if (err) goto bail;
    err = ISONewMovieTrack( output->mp4_moov, ISONewTrackIsAudio, &output->mp4_trak ); if (err) goto bail;
    err = MP4AddTrackToMovieIOD( output->mp4_trak ); if (err) goto bail;
    err = ISONewTrackMedia( output->mp4_trak, &output->mp4_media, ISOAudioHandlerType, mip->sampling_rate, NULL ); if (err) goto bail;
    err = ISOBeginMediaEdits( output->mp4_media ); if (err) goto bail;
    err = ISOSetMediaLanguage( output->mp4_media, "und" ); if (err) goto bail;
    objectTypeIndication = 0x40; /* mpeg-4 audio */
    streamType = 0x05; /* audio stream */
    err = ISONewHandle( 0, &output->mp4_sampleEntryH ); if (err) goto bail;
    err = MP4NewSampleDescription( output->mp4_trak, 
                                   output->mp4_sampleEntryH,
                                   1,
                                   objectTypeIndication,
                                   streamType,
                                   (u32)(decoderBufferSize>>3), /* bytes, not bits */
                                   (maxBitrate<0)?0:maxBitrate,
                                   (avgBitrate<0)?0:avgBitrate,
                                   decoderSpecificInfoH ); if (err) goto bail;
    err = ISONewHandle( sizeof(u32), &output->mp4_sampleDurationH ); if (err) goto bail;
    err = ISONewHandle( sizeof(u32), &output->mp4_sampleSizeH ); if (err) goto bail;
    *(u32*)*(output->mp4_sampleDurationH) = aacFrameLength;
    ISODisposeHandle(decoderSpecificInfoH);
  } else
#endif
#endif
    if ((format==OUT_RAW)||(format==OUT_ADIF)||(format==OUT_ADTS)) {
      if (avgBitrate<=0) output->bitrate = -maxBitrate;
      else output->bitrate = avgBitrate;
      output->fd = fopen(filename, "wb");
      if(output->fd == NULL) {
        PRINT(SE, "cannot create %s\n", filename);
        err = -1;
      }
    }

  output->format = format;

  return output;

 bail:
  bitoutput_free(output);
  return NULL;
}


void bitoutput_setADTSparameters(HANDLE_BITOUTPUT output, int use_crc, int rdb_per_frame
#ifdef MPEG4V1
                                 , int mpeg2
#endif
                                 )
{
  if (output==NULL) return;
  output->create_CRC = use_crc;
  output->rdb_per_frame = rdb_per_frame;
#ifdef MPEG4V1
  output->adts_header.id = (mpeg2 != 0);
#endif
}



int bitoutput_getBlockOverhead(HANDLE_BITOUTPUT output)
{
  int ret = 0;
  if (output == NULL) return -1;

  if (output->format == OUT_ADTS) {
    output->adts_header.protection_abs = (output->create_CRC?0:1);
    output->adts_header.num_of_rdb = output->rdb_per_frame-1;
    if (output->num_rdb_this_frame >= output->rdb_per_frame) {
      ret += ADTS_HEADER_SIZE(output->adts_header)<<3;
    }
#ifdef WRITE_NEW_ADTS
    else {
      /* hmm... buffer fullness does only contain raw_data_block, not CRC thereafter??? */
      ret += ((output->create_CRC)&&(output->rdb_per_frame>1))?LEN_CRC:0;
    }
#endif
  }

  return ret;
}

  


int bitoutput_WriteBlock(HANDLE_BITOUTPUT output, double bufferFullness, int iseof)
     /*
      * write current frame to disk
      */
{
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISOErr err;
#endif
#endif
  int retVal;
  int block_size;
  static int cnt = 0;

  bitoutput_ByteAlign(output);

  block_size = bitoutput_getBlockByteCount(output);
  if (block_size <= 0) return 0;
  
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  if (output->format==OUT_MP4FF) {
    err = ISOSetHandleSize( output->sampleDataH, block_size ); if (err) return -1;
    *(u32*)*(output->mp4_sampleSizeH) = block_size;
    err = ISOAddMediaSamples( output->mp4_media, output->sampleDataH, 1, output->mp4_sampleDurationH,
                              output->mp4_sampleSizeH, output->mp4_sampleEntryH, NULL, NULL ); if (err) return -1;
    if ( output->mp4_sampleEntryH ) {
      err = ISODisposeHandle( output->mp4_sampleEntryH ); if (err) return -1;
      output->mp4_sampleEntryH = NULL;
    }
    bitoutput_reset(output);
    bitoutput_reset_CRC(output);
  } else
#endif
#endif
  if ((output->format==OUT_RAW)||(output->format==OUT_ADIF)) {
    if ((output->format==OUT_ADIF)&&(!output->opened)) { /* start of ADIF file */
      retVal = write_adif_header( output, &mc_info, (output->bitrate>0)?output->bitrate:-output->bitrate,
                                  (output->bitrate<=0)?0x7fffffff:bufferFullness );
      output->opened = 1;
      if (retVal) return retVal;
    }
    
    if (fwrite((char*)*(output->sampleDataH), 1, block_size, output->fd) != (size_t) block_size) return -1;
    bitoutput_reset(output);
    bitoutput_reset_CRC(output);
  } else
  if (output->format==OUT_ADTS) {
    int header_length;

    if (iseof) output->rdb_per_frame=output->num_rdb_this_frame;

#ifdef WRITE_NEW_ADTS
    if ((output->create_CRC)&&(output->rdb_per_frame>1)) {
      unsigned long crcValue;
      CRC_reset(output->crc);
      crcValue=calculate_output_CRC(output);

      /* byte_align the CRC-values */
      /*written_bits += bitoutput_byteAlign(output);*/
      bitoutput_PutBits(output, LEN_CRC, crcValue);
      /*written_bits += LEN_CRC;*/
      bitoutput_reset_CRC(output);

      block_size = bitoutput_getBlockByteCount(output);
    }
#endif

    output->block_byte_counts[output->num_rdb_this_frame]=block_size;

    if (output->num_rdb_this_frame==0) output->lastBufferFullness = (output->bitrate<=0)?0x7ff:((int)(bufferFullness/channel_num))>>5;

    output->num_rdb_this_frame++;
    if (output->num_rdb_this_frame>=output->rdb_per_frame) {
      int frame_size = 0;
      int b;
      if (!(header_length=
            write_adts_header(output, &mc_info, output->lastBufferFullness,
                              output->block_byte_counts, output->rdb_per_frame))) return -1;
      for (b=0; b<output->rdb_per_frame; b++) {
        frame_size += output->block_byte_counts[b];
      }
      if (fwrite((char*)*(output->sampleDataH), 1, frame_size, output->fd) != (size_t) frame_size) return -1;
      output->num_rdb_this_frame=0;

      bitoutput_reset(output);
      bitoutput_reset_CRC(output);
    }
    output->block_start = output->bp;
  }

  return 0;
}



/* close output file ---------------------------------------------- */

int bitoutput_close(HANDLE_BITOUTPUT output)
{
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISOErr err = ISONoErr;
  u64 mediaDuration;
#endif
#else
  int err = 0;
#endif
  if (output==NULL) return -1;

  bitoutput_WriteBlock(output, 0.0, 1 /*iseof*/);

#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  if (output->format==OUT_MP4FF) {
    err = ISOGetMediaDuration( output->mp4_media, &mediaDuration ); if (err) goto bail;
    err = ISOEndMediaEdits( output->mp4_media ); if (err) goto bail;
    err = ISOInsertMediaIntoTrack( output->mp4_trak, 0, 0, mediaDuration, 1 ); if (err) goto bail;
    err = ISOSetTrackEnabled( output->mp4_trak, 1 ); if (err) goto bail;
    err = ISOWriteMovieToFile( output->mp4_moov, output->filename ); if (err) goto bail;
    err = ISODisposeMovie( output->mp4_moov ); if (err) goto bail;
  } else
#endif
#endif
    if ((output->format==OUT_RAW)||(output->format==OUT_ADIF)||(output->format==OUT_ADTS)) {
      if (output->fd) fclose(output->fd);
    }
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  ISODisposeHandle(output->mp4_sampleDurationH);
  ISODisposeHandle(output->mp4_sampleSizeH);
#endif
#endif

  bitoutput_free(output);

 bail:
  return err;
}


