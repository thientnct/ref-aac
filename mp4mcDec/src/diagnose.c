/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A

and edited by
  -

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
Copyright (c) ISO/IEC 2001.
 *                                                                           *
 ****************************************************************************/

#include "diagnose.h"
#include "adifdec.h"
#include "block.h"
#include "config.h"
#include "monopred.h"

/*******************
 * show used tools *
 *******************/

int is_present;    /* intensity */
int tns_present;   /* TNS */
int pulse_present; /* pulse data */
int gc_present;    /* gain control */
int dse_present;   /* data stream element */
#ifdef DRC
int drc_present;   /* DRC data */
#endif
#ifdef MPEG4V1
int noise_present; /* noise energy */
int ltp_present;   /* LTP data */
int stream_is_mpeg4 = 0; /* is one of PNS and LTP used? */
#endif

void show_tools(MC_Info* mip,Wnd_Shape* wnd_shape,byte *hasmask)
     /*
      * analyze the used tools
      */
{
  static int wndshape_inactive=0,wndshape_active=0;
  int ch, ms_detected=0, pred_detected=0, wndshape_detected=0;
  char s[50]="";

  for (ch=0; ch<Chans; ch++) {
    if (!(mip->ch_info[ch].present)) continue;

    if (wnd_shape[ch].this_bk) wndshape_active=1;
    if (!wnd_shape[ch].this_bk) wndshape_inactive=1;
    if (wndshape_active&&wndshape_inactive) wndshape_detected=1;/* window shape change */
    if (*lpflag[mip->ch_info[ch].widx]) pred_detected=1;    /* prediction */

    if (!(mip->ch_info[ch].cpe && mip->ch_info[ch].ch_is_left)) continue;
    if (hasmask[mip->ch_info[ch].widx]) ms_detected=1;      /* m/s */
  }
  if (dse_present) sprintf(s,"DSE ");               /* data stream element */
#ifdef DRC
  if (drc_present) sprintf(s,"%sDRC ",s);           /* dynamic range control */
#endif
  if (is_present) sprintf(s,"%sIS ",s);             /* intensity */
  if (pred_detected) sprintf(s,"%sPred ",s);        /* prediction */
  if (ms_detected) sprintf(s,"%sMS ",s);            /* m/s */
  if (pulse_present) sprintf(s,"%sPulseData ",s);   /* pulse data */
  if (gc_present) sprintf(s,"%sGC ",s);             /* gain control */
  if (wndshape_detected) sprintf(s,"%sWndShapeSW ",s);/* window shape switsching */
  if (tns_present) sprintf(s,"%sTNS ",s);           /* TNS */
#ifdef MPEG4V1
  if (noise_present) sprintf(s,"%sPNS ",s);         /* PNS */
  if (ltp_present) sprintf(s,"%sLTP ",s);           /* LTP */
#endif
  if (strncmp(s,"",1)) PRINT(SE,"# %sdetected\n",s);

#ifdef DRC
  drc_present =
#endif
#ifdef MPEG4V1
    ltp_present = noise_present =
#endif
    is_present = pulse_present = gc_present = tns_present = dse_present = 0;

}


/********************
 * CRC-calculations *
 ********************/

/* CRC-variables */
struct tagCRCcalculator {
  int bitsToLog, bitsToLog_buffer, bits_in_buffer;
  unsigned char buffer[16]; /* maximum of 128 bit in buffer */
  unsigned long crcRegister;
};


HANDLE_CRC_CALCULATOR CRC_calculator_create(void)
{
  HANDLE_CRC_CALCULATOR ret = (HANDLE_CRC_CALCULATOR)malloc(sizeof(struct tagCRCcalculator));
  ret->bitsToLog = 0;
  ret->bitsToLog_buffer = 0;
  ret->bits_in_buffer = 0;
  /*ret->buffer = 0  [16]; maximum of 128 bit in buffer */
  CRC_reset(ret);
  return ret;
}

void CRC_calculator_free(HANDLE_CRC_CALCULATOR crc)
{
  free(crc);
}


unsigned long CRC_reset(HANDLE_CRC_CALCULATOR crc)
     /*
      * reset CRC-calculator, returns current CRC-register
      */
{
  int retval;
  if (crc==NULL) return -1;
  retval = crc->crcRegister & 0xffff;
  crc->crcRegister = 0xffff;
  return retval;
}


void CRC_feed_calculator(HANDLE_CRC_CALCULATOR crc, int length, unsigned long value)
     /*
      * to be called at each getbits()-call;
      * CRC-calculation shuold be controlled with enable_CRC() and fillup_CRC()
      */
{
  unsigned long flag, tmp=0;
  int bitsToFeed = length;

  if ((crc->bitsToLog < length)&&(crc->bitsToLog >= 0)) {
    bitsToFeed = crc->bitsToLog;
    tmp = crc->bitsToLog-length;
  }

  if (length<32) value = value & ((1<<length)-1);

  crc->bitsToLog-=bitsToFeed;

  while (bitsToFeed) {
    flag = crc->crcRegister & 0x8000 ? 1:0;
    flag ^= (value>>(bitsToFeed-1-tmp)) & 1;
    crc->crcRegister<<=1;
    if (flag) crc->crcRegister^=0x8005;

    /*PRINT(SE,"crcReg:%08lx,pos:%i (%ld=>%ld)\n",crcRegister,bitsToLog_CRC,(value>>(bitsToFeed-1-tmp))&1,flag);*/

    bitsToFeed--;
  }

  bitsToFeed = length;
  if (crc->bitsToLog_buffer < length) bitsToFeed = crc->bitsToLog_buffer;
  if (bitsToFeed>0) {
    int where_byte = crc->bits_in_buffer>>3;
    value>>= length-bitsToFeed;
    crc->bitsToLog_buffer-= bitsToFeed;

    if (crc->bits_in_buffer & 0x7) { /*fill up old byte in buffer_CRC*/
      int tmp=8-(crc->bits_in_buffer & 0x7);
      if (tmp>bitsToFeed) tmp=bitsToFeed;

      bitsToFeed-=tmp;
      crc->buffer[where_byte++] |= (unsigned char)((value >> bitsToFeed)<<(8-(crc->bits_in_buffer&7)-tmp));
      crc->bits_in_buffer+=tmp;
    }

    crc->bits_in_buffer+= bitsToFeed;
    while (bitsToFeed >= 8) { /*add the full bytes to buffer_CRC*/
      bitsToFeed-=8;
      crc->buffer[where_byte++]=(unsigned char)(value>>bitsToFeed);
    }
    if (bitsToFeed) { /*add the remainder*/
      crc->buffer[where_byte]=(unsigned char)(value<<(8-bitsToFeed));
    }
  }
}


void CRC_enable(HANDLE_CRC_CALCULATOR crc, int length, int second)
     /*
      * enables CRC for <length> bits. These may be logged into a buffer, if <second> is set;
      * if <length> is negative, CRC is activated until fillup_CRC() is called
      */
{
  if (crc->bitsToLog) {
    if (!second) {
      PRINT(SE, "CRC-logger: unexpected overlap\n");
      return;
    }
    if (crc->bits_in_buffer) {
      PRINT(SE, "CRC-logger: unexpected overlap\n");
      return;
    }
    if ((length>128)||(length==-1)) {
      PRINT(SE, "CRC-logger: internal buffer exceeded\n");
      return;
    }
    crc->bitsToLog_buffer = length;
    crc->bits_in_buffer = 0;
    return;
  }
  crc->bitsToLog = length;
}


void CRC_fillup(HANDLE_CRC_CALCULATOR crc)
     /*
      * fill up remaining bits for CRC-calculation with zero;
      * clear buffered bits (if necessary), feeding them into feed_CRC_calculator()
      */
{
  if (crc->bitsToLog > 0) {
    CRC_feed_calculator(crc, crc->bitsToLog, 0);
  } else if (crc->bitsToLog < 0) crc->bitsToLog=0;

  if (crc->bits_in_buffer) {
    unsigned char *tmp = crc->buffer;
    int missing = crc->bitsToLog_buffer;
    crc->bitsToLog_buffer = 0;
    CRC_enable(crc, crc->bits_in_buffer+missing, 0);

    while (crc->bits_in_buffer >= 8) {
      CRC_feed_calculator(crc, 8,(unsigned char)*tmp++);
      crc->bits_in_buffer-=8;
    }
    if (crc->bits_in_buffer) CRC_feed_calculator(crc, crc->bits_in_buffer,*tmp>>(8-crc->bits_in_buffer));
    crc->bits_in_buffer=0;
    if (missing > 0) CRC_feed_calculator(crc, missing, 0);
  }
}


/************************
 * bitbuffer simulation *
 ************************/

struct tagBufferSimulation {
  /* variables */
  double new_min_bitBuffer_len, new_max_bitBuffer_len;
  double min_bitBuffer_len, max_bitBuffer_len;
  double current_fullness;
  int last_block_size;

  /* parameters */
  double peakBitrateToTest;
  double avgFrameLen;
  int bufferSize;

  /* know, when we started with a bufferFullness */
  int start_fullness;
  int total_frame_count;
  long total_bit_count;
  /* calculate bitrate from total file size */
  double calculatedBitrate;

  /* checks for ADTS with several blocks per frame */
  int rdb_positions[MAX_RDB_PER_FRAME];
  int rdb_position_check;
  long frame_bit_count;

  /* calculate bitrate from ADTS headers */
  int use_fixed_avgFrameLen;
  int prev_adts_fullness;
  long internal_bit_count;
  int internal_block_count;
};

BUFFER_SIMULATION create_bitBuffer_sim(void)
{
  int i;
  BUFFER_SIMULATION ret = malloc(sizeof(struct tagBufferSimulation));
  if (ret==NULL) return NULL;

  ret->min_bitBuffer_len = 0; ret->new_min_bitBuffer_len = 0;
  ret->max_bitBuffer_len = 0; ret->new_max_bitBuffer_len = 0;
  ret->current_fullness = .0;
  ret->start_fullness = -1;
  ret->bufferSize = ret->last_block_size = 0;
  ret->avgFrameLen = .0;
  ret->peakBitrateToTest = -1;
  ret->internal_bit_count = ret->frame_bit_count = 0;
  ret->internal_block_count = 0;
  ret->rdb_position_check = 0;
  ret->use_fixed_avgFrameLen = 1;
  ret->prev_adts_fullness = -1;
  ret->total_frame_count = ret->total_bit_count = ret->calculatedBitrate = 0;

  for (i=0; i<MAX_RDB_PER_FRAME; i++) ret->rdb_positions[i]=-1;

  return ret;
}

void bitBuffer_sim_setFrameLen(BUFFER_SIMULATION sim, double frameLen, int bufferSize)
     /* frameLen of 0 indicates a not fixed frame length (i.e. ADTS framelength calculation)
      * negative frameLen indicates variable frame length
      */
{
  if (sim==NULL) return;
  sim->avgFrameLen = frameLen;
  if (sim->avgFrameLen == .0) sim->use_fixed_avgFrameLen = 0;
  else sim->use_fixed_avgFrameLen = 1;

  sim->bufferSize = bufferSize;
  if (debug['b']) {
    if (sim->avgFrameLen<.0) PRINT(SE, "variable bitrate detected\n");
  }

  sim->total_frame_count = 0;
  sim->total_bit_count = 0;
}

void bitBuffer_sim_adjustFullness(BUFFER_SIMULATION sim, int fullness)
{
  double diff;
  if (sim==NULL) return;

  diff = sim->current_fullness - fullness;

  sim->total_frame_count = 0;
  sim->total_bit_count = 0;
  sim->start_fullness = fullness;
  sim->min_bitBuffer_len -= diff;
  sim->max_bitBuffer_len -= diff;
  sim->new_min_bitBuffer_len -= diff;
  sim->new_max_bitBuffer_len -= diff;
  sim->current_fullness -= diff;

  if (debug['b']) {
    PRINT(SE, "initiating buffer_fullness at %i\n", (int)sim->current_fullness);
  }
}

void feed_bitBuffer_sim(BUFFER_SIMULATION sim, int bits)
{
  sim->last_block_size += bits;
  sim->total_bit_count += bits;
  sim->internal_bit_count += bits;
  sim->current_fullness -= bits;

  sim->new_min_bitBuffer_len = sim->min_bitBuffer_len;
  sim->new_max_bitBuffer_len = sim->max_bitBuffer_len;
  if (sim->current_fullness < sim->min_bitBuffer_len) sim->new_min_bitBuffer_len = sim->current_fullness;
  if (sim->current_fullness > sim->max_bitBuffer_len) sim->new_max_bitBuffer_len = sim->current_fullness;
}


double update_bitBuffer_sim(BUFFER_SIMULATION sim, int bits, const ADTS_Header *adts_head)
     /* needs channel_num
      * set parameter adts_head to NULL, if there is no new header
      */
{
  if (sim==NULL) return 0;

  if (sim->new_min_bitBuffer_len < sim->min_bitBuffer_len) sim->min_bitBuffer_len = sim->new_min_bitBuffer_len;
  if (sim->new_max_bitBuffer_len > sim->max_bitBuffer_len) sim->max_bitBuffer_len = sim->new_max_bitBuffer_len;

  sim->last_block_size = bits;

  sim->total_bit_count += bits;
  sim->total_frame_count++;

  sim->internal_bit_count+= bits;
  sim->internal_block_count++;

  if (adts_head!=NULL) {
    /* if not other known, calculate avgFrameLen */
    if (!sim->use_fixed_avgFrameLen) {
      if (sim->prev_adts_fullness!=-1)
        sim->avgFrameLen=(((((adts_head->adts_buffer_fullness-sim->prev_adts_fullness)<<2)*channel_num)<<3)+sim->internal_bit_count)/sim->internal_block_count;
      else
        sim->avgFrameLen=sim->internal_bit_count/sim->internal_block_count;
    }

    /* +++ check ADTS header fields */
    if ((sim->avgFrameLen < .0) && (adts_head->adts_buffer_fullness != 0x7ff))
      PRINT(SE, "ERROR: ADTS-Header changes from variable to constant bitrate!\n");
    if ((sim->avgFrameLen >= .0) && (adts_head->adts_buffer_fullness == 0x7ff))
      PRINT(SE, "ERROR: ADTS-Header changes from constant to variable bitrate!\n");

    /* +++ check ADTS frame length */
    if ((sim->rdb_positions[sim->internal_block_count-1]!=-1)&&
        (((sim->frame_bit_count+7)>>3)!=sim->rdb_positions[sim->internal_block_count-1]))
      PRINT(SE, "WARNING: ADTS-Header frame_length incorrect (%i!=%ld)\n",
            sim->rdb_positions[sim->internal_block_count-1],(sim->frame_bit_count+7)>>3);

    /* +++ prepare check for next frame */
    sim->rdb_positions[adts_head->num_of_rdb] = adts_head->frame_length;
    sim->rdb_position_check=0;
#ifdef NEW_ADTS
    if (!adts_head->protection_abs) {
      int i;
      for (i=0; i<adts_head->num_of_rdb; i++) sim->rdb_positions[i] = adts_head->rdb_position[i] + ADTS_HEADER_SIZE(*adts_head);
      sim->rdb_position_check=1;
    }
#endif
    
    sim->frame_bit_count = 0;
    sim->internal_bit_count = 0;
    sim->internal_block_count = 0;
      
  }
#ifdef NEW_ADTS
  else if (sim->rdb_position_check) {
    if (((sim->frame_bit_count+7)>>3)!=sim->rdb_positions[sim->internal_block_count-1]) {
      PRINT(SE, "WARNING: ADTS-header raw_data_block position incorrect (%i!=%ld)\n",
            sim->rdb_positions[sim->internal_block_count-1],(sim->frame_bit_count+7)>>3);
    }
  }
#endif
  sim->frame_bit_count += bits;

  if (sim->avgFrameLen<.0) sim->current_fullness = sim->bufferSize;
  else sim->current_fullness+=sim->avgFrameLen;  /* these bits were processed */
  sim->current_fullness-=bits;                   /* these bits arrived in bitBuffer */

  if (sim->current_fullness < sim->min_bitBuffer_len) sim->new_min_bitBuffer_len = sim->current_fullness;
  if (sim->current_fullness > sim->max_bitBuffer_len) sim->new_max_bitBuffer_len = sim->current_fullness;

#if 0
  if (adts_buffer_fullness >= 0) {
    sim->calculatedBitrate = ((double)((sim->total_byte_count+(variable_bitrate?(bufferSizePerAudioChn>>3):(adts_header.adts_buffer_fullness<<2))*channel_num)<<3)-start_bitBuffer_len)/(bno-simulation_start_bno);
  }/* else */
#endif

  /* +++ calculate new global bitrate */
  bitBuffer_sim_calculateBitrate(sim);

  
  /* +++ check ADTS buffer fullness */
  if (adts_head!=NULL) {
    int truncated_fullness = (int)((sim->current_fullness/channel_num)/32);
    if ((sim->avgFrameLen>.0) && (truncated_fullness != adts_head->adts_buffer_fullness) && (sim->prev_adts_fullness != -1))
      PRINT(SE, "WARNING: ADTS-Header bufferFullness might be incorrect (%i != %i)\n",
            (adts_head->adts_buffer_fullness), truncated_fullness);
    sim->prev_adts_fullness = adts_head->adts_buffer_fullness;
  }

  return sim->current_fullness;
}


int check_bitBuffer_sim(BUFFER_SIMULATION sim,
                        const char *name, int show_buffer, int show_details, int show_rate)
     /*
      * writes some diagnostic output
      */
{
  int retval=0;
  double effective_bufferSize;

  if (sim==NULL) return -1;

  if (sim->new_min_bitBuffer_len < sim->min_bitBuffer_len) sim->min_bitBuffer_len = sim->new_min_bitBuffer_len;
  if (sim->new_max_bitBuffer_len > sim->max_bitBuffer_len) sim->max_bitBuffer_len = sim->new_max_bitBuffer_len;

  effective_bufferSize = sim->bufferSize;
  if (sim->avgFrameLen>.0) {
    if (sim->calculatedBitrate > .0)
      effective_bufferSize -= BITRATE2FRAMELEN(sim->calculatedBitrate);
    else
      effective_bufferSize -= sim->avgFrameLen;
  }

  if (name==NULL) name = "";

  if (show_rate) {
    if (sim->avgFrameLen>=.0) {
      PRINT(SE, "bitBuffer %s: frameLen from header: %8.2f (bitrate %8.2f)\n",
            name, sim->avgFrameLen, FRAMELEN2BITRATE(sim->avgFrameLen));
    } else {
      PRINT(SE, "bitBuffer %s: frameLen this frame : %5i.00 (bitrate %8.2f)\n",
            name, sim->last_block_size, FRAMELEN2BITRATE(sim->last_block_size));
    }
    PRINT(SE, "bitBuffer %s: frameLen calculated : %8.2f (bitrate %8.2f)\n",
          name, BITRATE2FRAMELEN(sim->calculatedBitrate), sim->calculatedBitrate);
  }

  if ((sim->last_block_size >= sim->bufferSize-16)&&(show_buffer)) {
    PRINT(SE, "Buffer-Test detected (block length %i >= %i bit)\n",(int)sim->last_block_size,sim->bufferSize -16);
  }
  if (sim->last_block_size > sim->bufferSize) {
    PRINT(SE, "bitBuffer %s: ERROR: Buffer-limits exceeded (block length %i >= %i bit)\n", name, sim->last_block_size, sim->bufferSize);
    retval = -2;
  }

  if (show_details) {
    if (sim->avgFrameLen>=.0)
      PRINT(SE, "bitBuffer %s: having %i free bit(processed: %i, got: %i)\n",
            name, (int)sim->current_fullness, (int)sim->avgFrameLen, sim->last_block_size);
    else
      PRINT(SE, "bitBuffer %s: having %i free bit(processed: all, got: %i)\n",
            name, (int)sim->current_fullness, sim->last_block_size);
  }
  
  if (show_buffer) {
    if ((sim->peakBitrateToTest > 0) && (FRAMELEN2BITRATE(sim->avgFrameLen)>sim->peakBitrateToTest)) {
      PRINT(SE, "ERROR: peak bitrate exceeded!\n");
      retval = -3;
    }
    
    if (sim->start_fullness != -1) {
      if (sim->current_fullness > effective_bufferSize) {
        PRINT(SE, "bitBuffer %s: WARNING, decoder had to wait for data\n",name);
        retval = -1;
      }
      if (sim->current_fullness < .0) {
        PRINT(SE, "bitBuffer %s: WARNING, decoder recieves too much data\n",name);
        retval = -1;
      }
    }

    if ((sim->max_bitBuffer_len-sim->min_bitBuffer_len) > effective_bufferSize) {
      PRINT(SE, "bitBuffer %s: ERROR: buffer_min_size: %.2f, buffer_max_size %.2f => buffer-limits exceeded (%.2f > %.2f)\n",
            name, sim->min_bitBuffer_len, sim->max_bitBuffer_len,
            sim->max_bitBuffer_len - sim->min_bitBuffer_len, effective_bufferSize);
      retval = -2;
    }
  }
  return retval;
}

double bitBuffer_sim_getFullness(BUFFER_SIMULATION sim)
{ if (sim==NULL) return .0; return sim->current_fullness; }

long bitBuffer_sim_getTotalBits(BUFFER_SIMULATION sim)
{ if (sim==NULL) return .0; return sim->total_bit_count; }

double bitBuffer_sim_getCurrentBitrate(BUFFER_SIMULATION sim)
{
  if (sim==NULL) return .0;
  if (sim->use_fixed_avgFrameLen==0) return .0;
  return FRAMELEN2BITRATE(sim->avgFrameLen);
}

void bitBuffer_sim_getMinMax(BUFFER_SIMULATION sim, double *min, double *max)
{
  if (sim==NULL) return;
  if (min!=NULL) *min=sim->min_bitBuffer_len;
  if (max!=NULL) *max=sim->max_bitBuffer_len;
}

void bitBuffer_sim_testPeakBitrate(BUFFER_SIMULATION sim, double peakRate)
{
  if (sim==NULL) return;
  sim->peakBitrateToTest = peakRate;
}

double bitBuffer_sim_calculateBitrate(BUFFER_SIMULATION sim)
{
  if (sim==NULL) return 0;
  
  if (sim->avgFrameLen>=.0) {
    if (sim->start_fullness!=-1)
      /* constant bitrate with buffer fullness */
      sim->calculatedBitrate = ((double)sim->total_bit_count+sim->current_fullness-sim->start_fullness)/sim->total_frame_count;
    else
      /* constant bitrate without buffer fullness */
      sim->calculatedBitrate = ((double)sim->total_bit_count+sim->current_fullness)/sim->total_frame_count;
  } else {
    /* variable bitrate */
    sim->calculatedBitrate = ((double)sim->total_bit_count)/sim->total_frame_count;
  }
  sim->calculatedBitrate = (int)FRAMELEN2BITRATE(sim->calculatedBitrate);
  return sim->calculatedBitrate;
}
