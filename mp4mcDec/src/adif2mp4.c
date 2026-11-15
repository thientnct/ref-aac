/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Apple Computer, Inc.

and edited by
  Olaf Kaehler (Fraunhofer IIS)

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
Copyright (c) ISO/IEC 1999.
 *                                                                           *
 ****************************************************************************/

#include "config.h"
#include "diagnose.h"
#include "intrins.h"
#include "adifdec.h"
#include "bitinput.h"
#include "bitoutput.h"

#define ROUND_UP(x)			(int)(x+.9999999999999999999)

struct {
  char *str;
  int fmt;
} fmtstr[] = {
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  {"mp4",OUT_MP4FF},
#endif
#endif
  {"adif",OUT_ADIF},
  {"adts",OUT_ADTS},
  {"raw",OUT_RAW},
  {NULL,-1}
};

static MC_Info mc_info_cmdline;
static int mc_info_changed = 0;

static int param_blocks_per_frame = 1;
static int param_CRC = 0;
static int param_format = -1;

static char *inFile, *outFile;
static long decoderBufferSize = 0;  /* bit */

static HANDLE_BITINPUT bitinput;
static HANDLE_BITOUTPUT bitoutput;


#ifdef CT_SBR
static int signallingType = 2;
static int sbrPresentFlag = 1;
static int downSampledSbr = 0;
#ifdef PARAMETRICSTEREO
static int psSignallingType = 0;
static int psPresentFlag = 1;
#endif
#endif
#ifdef FRAMELENGTH_FLAG
int frameLengthFlag = 0;
#endif


/* bitBuffer simulations */
static long output_maxBitrate = -1, output_avgBitrate =-1;
static long input_maxBitrate = 0, input_avgBitrate = 0;
static long avgBitrate_cmdline = -1;
static int input_start_bufferFullness = -1, output_start_bufferFullness =-1;
static int input_bitrate_known = 1; /*0:unknown bitrate, 1:known, 2:variable*/
static long calculated_maxBitrate = 0;
static BUFFER_SIMULATION input_sim;
static BUFFER_SIMULATION output_sim;


/* other control variables */
#ifdef MPEG4V1
static int mpeg2_compatible = 0;
#endif
static int buffer_test = -1, buffer_test_rep=0;
static int add_fill = 0;
static int del_fill = 0;
int copy_to_output = 0;



static void usage(char* s)
{
  PRINT(SE, "ADIF2MP4   2003-08-27\n");
  PRINT(SE, "usage:  %s  [options] input_file output_file\n", s);
  PRINT(SE, "	options are:\n");
  PRINT(SE, "	-h     show this help\n");
  PRINT(SE, "	-i     program_number (ADIF header present, 0 is first program)\n");
  PRINT(SE, "	-s     sampling_frequency, Hz (only necessary for RAW input)\n");
  PRINT(SE, "	-r     max. bitrate for output, bit/s (optional)\n");
  PRINT(SE, "	-b     average bitrate for output, bit/s (optional)\n");
  PRINT(SE, "	-bi    average bitrate for input, bit/s (optional)\n");
  PRINT(SE, "	-f     initial buffer fullness (adif_buffer_fullness) (optional)\n");
  PRINT(SE, "	-vbr   create variable bitrate output stream\n");
  PRINT(SE, "	-F[ad] allow changing bitstream by [a]dding/[d]eleting FILL-elements\n");
  PRINT(SE, "	-F[tT] <x> add buffer-test in block <x> (t), repeat all <x> blocks (T)\n");
  PRINT(SE, "	-E <x> allows to copy only specified elements to output\n");
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  PRINT(SE, "	-p     profile  (Main or LC or SSR or LTP, Main is default)\n");
  PRINT(SE, "	-o     output bitstream format: mp4ff (default), adif, adts, raw\n");
#endif
  PRINT(SE, "	-mpeg2 ADTS: create a MPEG2-AAC compatible bitstream (default: MPEG4 compatible)\n");
#else
  PRINT(SE, "	-p     profile  (Main or LC or SSR, Main is default)\n");
  PRINT(SE, "	-o     output bitstream format: adif (default), adts, raw\n");
#endif
  PRINT(SE, "	-n     ADTS: number of raw data blocks per output frame (default:1)\n");
  PRINT(SE, "	-c     ADTS: enable CRC in headers (default:disable)\n");
  PRINT(SE, "	-D [a-Z]    (enable debugging printouts)\n");
#ifdef CT_SBR
  PRINT(SE, "	-k  SBR: signalling: 0 implicit, 1 hierarchical explicit, 2 backwards compatible explicit (default: 2)\n");
  PRINT(SE, "	-l  SBR: sbrPresent flag (only transmitted for the backwards compatible signalling): 0 SBR not present, 1 SBR present (default: 1)\n");
  PRINT(SE, "	-q  SBR: downSampledSBR: 0 off 1 on (default: 0)\n");
#ifdef PARAMETRICSTEREO
  PRINT(SE, "	-g  PS: signalling: 0 implicit, 1 hierarchical explicit, 2 backwards compatible explicit (default: 0)\n");
  PRINT(SE, "	-j  PS: psPresent flag (only transmitted for the backwards compatible signalling): 0 PS not present, 1 PS present (default: 1)\n");
#endif
#endif

#ifdef FRAMELENGTH_FLAG
  PRINT(SE, "	-l     set AAC framelength flag: 0:1024, 1:960 (default: 0)\n");
#endif

  exit(-1);
}

static void parse_args(int argc, char **argv)
     /*
      * parse commandline and set the specified variables
      */
{
  char *cmd=argv[0],*s;
  int i, j;
  int *p;
  int prog = -1;

  ARGBEGIN {
  default:
  case 'h':
    PRINT(SE, "unknown option\n");
    usage(cmd);

  case 'D':
    for (s = ARGF(); *s; s++) {
      debug[(int)(*s)] = 1;
      PRINT(SE, "debug option %c enabled\n", *s);
    }
    break;

  case 'i':
    prog = atoi(ARGF());
    if(prog > 15) {
      PRINT(SE, "program number out of range: %d\n", prog);
      exit(-1);
    }
    break;

  case 'p':
    s = ARGF();
    mc_info_changed = 1;
    p = &mc_info_cmdline.profile;
    if ((!strncmp(s, "Main", 4)) || (!strncmp(s, "main", 4))) {
      *p = Main_Profile; 
      break;
    }
    if ((!strncmp(s, "LC", 2)) || (!strncmp(s, "lc", 2))) {
      *p = LC_Profile; 
      break;
    }
    if ((!strncmp(s, "SSR", 3)) || (!strncmp(s, "ssr", 3))) {
      *p = SSR_Profile;
      break;
    }
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
    if ((!strncmp(s, "LTP", 3)) || (!strncmp(s, "ltp", 3))) {
      *p = LTP_Profile;
      break;
    }
#endif
#endif
    PRINT(SE, "Unsupported profile %s\n", s);
    exit(-1);
    break;

  case 's':
    j = atoi(ARGF());
    i = getNextSampFreqIndex(j);
    if (i == -1) {
      PRINT(SE, "Unsupported sampling frequency %d\n", j);
      exit(-1);
    }
    mc_info_cmdline.sampling_rate_idx = i;
    mc_info_cmdline.sampling_rate = j;
    mc_info_changed = 1;
    break;

#ifdef MPEG4V1
  case 'm':
    s = ARGF();
    if (!strncmp(s, "peg2", 4)) {
      mpeg2_compatible=1;
      break;
    } else usage(cmd);
#endif

  case 'F':
    for (s = ARGF(); *s; ) {
      switch (*s++) {
      case 'a': add_fill = 1;break;
      case 'd': del_fill = 1;break;
      case 't': buffer_test = atoi(ARGF());break;
      case 'T': buffer_test_rep = atoi(ARGF());break;
      default:
        PRINT(SE, "Unsupported FILL-element option %c: choose a for add or d for delete or both\n",*s);
        exit(-1);
      }
    }
    break;


  case 'E':
    s = ARGF();j=0;
    if (s[j]=='+') {
      int type=ID_SCE;
      int tag=0;
      char pos;

      j++;pos=s[j++];

      if ((pos=='f')||(pos=='s')||(pos=='b')) {
        if (s[j]=='p') type=ID_CPE;else 
        if (s[j]=='s') type=ID_SCE;else {
          PRINT(SE, "Syntax error in '-E' argument: '%c' unexpected!\n", s[j]);
          exit(-1);
        }
        j++;
      } else if (pos=='c') {
        if (s[j]=='d') type=0;else 
        if (s[j]=='i') type=1;else {
          PRINT(SE, "Syntax error in '-E' argument: '%c' unexpected!\n", s[j]);
          exit(-1);
        }
        j++;
      }
      while (s[j]) {
        if ((s[j]>='0')&&(s[j]<='9')) {
          tag *= 10;
          tag += s[j] - '0';
        } else {
          PRINT(SE, "Syntax error in '-E' argument: '%c' unexpected!\n", s[j]);
          exit(-1);
        }
        j++;
      }
      if (pos=='c') {
        mc_info_cmdline.cc_tag[mc_info_cmdline.ncch] = tag;
        mc_info_cmdline.cc_ind[mc_info_cmdline.ncch++] = type;
      } else
        enter_chn(type, tag, pos, 0, &mc_info_cmdline);
      
      mc_info_changed = 1;
    } else {
      PRINT(SE, "Syntax error in '-E' argument: '%c' unexpected!\n", s[j]);
      exit(-1);
    }
    break;

#ifdef FRAMELENGTH_FLAG
  case 'l':
    frameLengthFlag = atoi(ARGF());
    break;
#endif
  case 'v':
    s = ARGF();
    if (!strcmp(s,"br")) output_avgBitrate = -2;
    break;
  case 'b':
    s = ARGF();
    if (!strcmp(s,"i")) avgBitrate_cmdline = atoi(ARGF());
    else output_avgBitrate = atoi(s);
    break;
  case 'r':
    output_maxBitrate = atoi(ARGF());
    break;
  case 'f':
    output_start_bufferFullness = atoi(ARGF());
    break;
  case 'n':
    param_blocks_per_frame = atoi(ARGF());
    if (param_blocks_per_frame>MAX_RDB_PER_FRAME) {
      PRINT(SE, "maxiumum of %i raw_data_blocks per frame\n",MAX_RDB_PER_FRAME);
      exit(-1);
    }
    break;
  case 'c':
    param_CRC = 1;
    break;

  case 'o':
    s = ARGF();
    param_format = j = -1;
    while ( fmtstr[++j].str ) {
      if (!strcmp(fmtstr[j].str,s)) {
        param_format = fmtstr[j].fmt;
        break;
      }
    }
    if (param_format==-1) usage(cmd);
    break;

#ifdef CT_SBR
  case 'k':
    signallingType = atoi(ARGF());
    break;
  case 'l':
    sbrPresentFlag = atoi(ARGF());
    break;
  case 'q':
    downSampledSbr = atoi(ARGF());
    break;
#ifdef PARAMETRICSTEREO
  case 'g':
    psSignallingType = atoi(ARGF());
    break;
  case 'j':
    psPresentFlag = atoi(ARGF());
    break;
#endif
#endif
    
  }
  ARGEND

  if(argc != 2) {
    usage(cmd);
  }

  decoder_selectProgram(prog);
  {
    int sampling_rate_idx = mc_info_cmdline.sampling_rate_idx;
    if (sampling_rate_idx==-1) sampling_rate_idx=3;
    decoder_setupParameters(sampling_rate_idx);
  }

  inFile = argv[0];
  outFile = argv[1];

  if (param_format==-1) {
    for (i=strlen(outFile);(i>=(signed int)(strlen(outFile)-5))&&(i>=0);i--) {
      if (outFile[i]=='.') {
        j=-1;
        while ( fmtstr[++j].str ) {
          if (!strcmp(fmtstr[j].str,(outFile+i+1))) {
            param_format = fmtstr[j].fmt;
            break;
          }
        }
      }
    }
  }
  if (param_format==-1) {
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
    PRINT(SE, "output will be mp4ff\n");
    param_format = OUT_MP4FF;
#endif
#else
    PRINT(SE, "output will be adif\n");
    param_format = OUT_ADIF;
#endif
  }

#ifndef WRITE_NEW_ADTS
  if ((param_format!=OUT_ADTS)&&(param_blocks_per_frame!=1)&&(param_CRC)) {
    PRINT(SE, "CRC not supported for multiple raw data blocks per frame... CRC disabled!\n");
    param_CRC = 0;
  }
#endif

#ifdef MPEG4V1
  if ((param_format!=OUT_MP4FF)&&(mc_info_cmdline.sampling_rate_idx!=-1))
#else
  if (mc_info_cmdline.sampling_rate_idx!=-1)
#endif
    if (mc_info_cmdline.sampling_rate!=sampling_rate[mc_info_cmdline.sampling_rate_idx]) {
      PRINT(SE, "Sampling rate %i Hz not supported for output...", mc_info_cmdline.sampling_rate);
      mc_info_cmdline.sampling_rate=sampling_rate[mc_info_cmdline.sampling_rate_idx];
      PRINT(SE, " using %i instead\n",mc_info_cmdline.sampling_rate);
    }
}


static int fillup_mc_info(MC_Info input, MC_Info* output)
     /*
      * complete the information in <input> writing to <output>
      */
{
  int i, j;

  if (input.profile!=-1) output->profile = input.profile;
  if (input.sampling_rate_idx!=-1) {
    output->sampling_rate_idx = input.sampling_rate_idx;
    output->sampling_rate = input.sampling_rate;
  }

  /* input MC_Info has channel information: take these */
  for (i=j=0; i<Chans; i++)
    if (input.ch_info[i].present) j++;

  if (j) {
    memcpy(&(output->ch_info[0]),&(input.ch_info[0]),Chans*sizeof(Ch_Info));

    output->nch=input.nch;
    output->nfsce=input.nfsce;
    output->nfch=input.nfch;
    output->nsch=input.nsch;
    output->nbch=input.nbch;
    output->nlch=input.nlch;
    output->ncch=input.ncch;
  }

  check_mc_info(output, 1);

  return 0;
}




static int addMySamples(HANDLE_BITINPUT input)
     /*
      * parse input while writing output
      */
{
  int retVal;

  for(bno = 0;; bno++) {
    double bufferFullness;
    long block_length_bits;

    /* --- get input */
    retVal = bitinput_getNextBlock(bitinput);
    if ( retVal ) {
      if ( retVal == -2 ) retVal = 0;
      break;
    }

    /* parse this block */
    dec_parse();

    /* get length */
    block_length_bits = bitoutput_getBlockBitCount(bitoutput);
    if ( (block_length_bits == 0) || (block_length_bits > MAX_FRAME_LEN<<3) ) {
      PRINT(SE, "frameLength error: %ld\n", block_length_bits);
      return -1;
    }

#ifdef MPEG4V1
    if ((mpeg2_compatible)&&(stream_is_mpeg4)) { /* user wants mpeg2-aac, but mpeg4 is used */
      PRINT(SE, "Non MPEG2-AAC compatible content found! Aborting...\n\n");
      myexit(NULL);
    }
#endif

    /* --- input bitBuffer simulation */
    {
      const ADTS_Header *tmp_head = bitinput_getCurrentADTSHeader(input);
      update_bitBuffer_sim(input_sim, bitinput_resetCounter(bitinput)<<3, tmp_head);
    }
    /* initial buffer fullness */
    if ((bno==0)&&(input_start_bufferFullness!=-1)) {
      bitBuffer_sim_adjustFullness(input_sim, input_start_bufferFullness);
    }
    check_bitBuffer_sim(input_sim, "input", debug['b'], debug['B'], debug['R']);

    /* --- output bitBuffer simulation */
    {
      double new_fullness;
      int overhead_bits;

      overhead_bits = bitoutput_getBlockOverhead(bitoutput);
      block_length_bits += overhead_bits;

      new_fullness = update_bitBuffer_sim(output_sim, ((block_length_bits+7)>>3)<<3, NULL);
      if ((bno==0)&&(output_start_bufferFullness!=-1)) {
        bitBuffer_sim_adjustFullness(output_sim, output_start_bufferFullness);
        new_fullness = output_start_bufferFullness;
      }

      /*if ((output_format==OUT_ADTS)&&(adts_header_present))
        internal_simulated_bitBuffer_len = simulated_bitBuffer_len;*/
      /* keep input bufferFullness => output bitrate implicitly changes */

      {
        int fill_len=0,round_up=0;
#ifdef FRAMELENGTH_FLAG
        int state;

        if ( frameLengthFlag )
          state = (add_fill)&&(output_avgBitrate!=-2)&&(new_fullness>=decoderBufferSize-BITRATE2FRAMELEN960(output_avgBitrate));
        else
          state = (add_fill)&&(output_avgBitrate!=-2)&&(new_fullness>=decoderBufferSize-BITRATE2FRAMELEN(output_avgBitrate));

        if ( state ) {
          /* place FILL to hold bitrate */
        if ( frameLengthFlag )
          fill_len = ROUND_UP(new_fullness-decoderBufferSize+BITRATE2FRAMELEN960(output_avgBitrate));
        else
          fill_len = ROUND_UP(new_fullness-decoderBufferSize+BITRATE2FRAMELEN(output_avgBitrate));

#else
        if ((add_fill)&&(output_avgBitrate!=-2)&&(new_fullness>=decoderBufferSize-BITRATE2FRAMELEN(output_avgBitrate))) {
          /* place FILL to hold bitrate */
          fill_len = ROUND_UP(new_fullness-decoderBufferSize+BITRATE2FRAMELEN(output_avgBitrate));
#endif
          round_up = 1;
        }
        if (bno==buffer_test) {
          /* place FILL, because user requested buffer-test this block */
          buffer_test += buffer_test_rep;
          fill_len = (int)new_fullness;
          round_up = 0;
        }

        if (fill_len > 0) {
          int old_padding = (8-(block_length_bits&7))&7;
          int new_padding;
          if (debug['F'])
            PRINT(SE,"-- Adding FILL elements: moving end by %s %i bit, old block-length %ld\n",
                  round_up?"at least":"at most",fill_len,(block_length_bits+7)>>3);
          fill_len = bitoutput_MoveEnd(bitoutput, fill_len, round_up);
          block_length_bits += fill_len;
          new_padding = (8-(block_length_bits&7))&7;
          feed_bitBuffer_sim(output_sim, fill_len - old_padding + new_padding);
          if (debug['F']) PRINT(SE,"-- OK, moved by %i bit, new block length %ld\n", fill_len, (block_length_bits+7)>>3);
        }
      }
    }

    block_length_bits += bitoutput_ByteAlign(bitoutput);

    if (calculated_maxBitrate<FRAMELEN2BITRATE(block_length_bits))
      calculated_maxBitrate=ROUND_UP(FRAMELEN2BITRATE(block_length_bits));

    check_bitBuffer_sim(output_sim, "output", debug['b'], debug['B'], debug['R']);

    /* bufferFullnes after "encoding" first raw-data-block in frame */
    bufferFullness = bitBuffer_sim_getFullness(output_sim);

    /* --- write output */
    retVal = bitoutput_WriteBlock(bitoutput, bufferFullness, 0 /* not eof*/);
    if (retVal) goto bail;
  }
 bail:
  return retVal;
}


/* decoderBufferSize and bitrates --------------------------------- */

static void prepareBitratesAndBufferRequirements(ProgConfig *pc, int channelConfig)
{
  int retVal = getDecoderBufferSize(pc, channelConfig,  &decoderBufferSize ); if (retVal) return;

  if (input_start_bufferFullness==0x7fffffff) {
    PRINT(SE, "input has variable bitrate\n");
    input_bitrate_known = 2;
    input_start_bufferFullness = -1;
  }


  if (avgBitrate_cmdline!=-1) input_avgBitrate = avgBitrate_cmdline;
  if (output_start_bufferFullness==-1) output_start_bufferFullness = input_start_bufferFullness;
  if (output_avgBitrate==-1) {   /* if output-bitrate not explicitly specified... */
    if (input_bitrate_known==2) {
      output_avgBitrate=-2;      /* ... if input has variable bitrate => output as well */
    }
    else if (input_avgBitrate) { /* ... else try to use input bitrate */
      double tmp = BITRATE2FRAMELEN(input_avgBitrate);
        /* TODO: estimation incorrect */
        /*-(adts_header_present?ADTS_HEADER_SIZE(adts_header)<<3:0)
          +(output_format==OUT_ADTS?ADTS_HEADER_SIZE(out_adts_header)<<3:0)*/;
      output_avgBitrate = (long int)(FRAMELEN2BITRATE(tmp)+.5);
    } else {
      PRINT(SE, "bitrates unknown... expect bitBuffer problems!\n");
      input_bitrate_known = 0;
      output_avgBitrate = 0;
    }
  }
  if (output_avgBitrate==-2) {
    PRINT(SE, "creating output bitstream with variable bitrate\n");
    output_start_bufferFullness = decoderBufferSize;
  }

  if (input_maxBitrate==0) input_maxBitrate = input_avgBitrate;
  if (output_maxBitrate==-1) output_maxBitrate = input_maxBitrate;

  if ((avgBitrate_cmdline==-1)&&(NULL != bitinput_getCurrentADTSHeader(bitinput))) input_avgBitrate = 0; /* get more accurate estimate from ADTS-Headers */

  input_sim = create_bitBuffer_sim();
  {
    double avgFrameLen = .0;
    if (input_bitrate_known == 1) avgFrameLen = BITRATE2FRAMELEN(input_avgBitrate);
    if (input_bitrate_known == 2) avgFrameLen = -1.;
    bitBuffer_sim_setFrameLen(input_sim, avgFrameLen, decoderBufferSize);
  }
  output_sim = create_bitBuffer_sim();
  {
    /* TODO: use different decoderBufferSize if channel number has changed */
    double avgFrameLen = .0;

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag )
      if (output_avgBitrate > 0) avgFrameLen = BITRATE2FRAMELEN960(output_avgBitrate);
    else
      if (output_avgBitrate > 0) avgFrameLen = BITRATE2FRAMELEN(output_avgBitrate);

#else
    if (output_avgBitrate > 0) avgFrameLen = BITRATE2FRAMELEN(output_avgBitrate);
#endif
    if (output_avgBitrate < 0) avgFrameLen = -1.;
    bitBuffer_sim_setFrameLen(output_sim, avgFrameLen, decoderBufferSize);
  }

  if ((output_start_bufferFullness==-1)&&(output_avgBitrate>0)
      /*&&((output_format==OUT_ADIF)||(output_format==OUT_ADTS))*/) {

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag )
      output_start_bufferFullness = (double)decoderBufferSize - BITRATE2FRAMELEN960(output_avgBitrate);
    else
      output_start_bufferFullness = (double)decoderBufferSize - BITRATE2FRAMELEN(output_avgBitrate);
#else
    output_start_bufferFullness = (double)decoderBufferSize - BITRATE2FRAMELEN(output_avgBitrate);
#endif
  }

  return;
}


/* close output file ---------------------------------------------- */

static int checkBufferRequirements()
{
  double output_calculatedBitrate, input_calculatedBitrate;
  long   output_proposedBitrate, input_usedBitrate;
  double output_max_fullness, output_min_fullness;
  long tmp = 0;
  double tmp1;

  bitBuffer_sim_getMinMax(output_sim, &output_min_fullness, &output_max_fullness);
  output_calculatedBitrate = bitBuffer_sim_calculateBitrate(output_sim);

  if (input_bitrate_known==1)
    tmp1 = ((double)bitBuffer_sim_getTotalBits(output_sim)-input_start_bufferFullness+bitBuffer_sim_getFullness(input_sim))/(bno-1);
  else
    tmp1 = (double)(bitBuffer_sim_getTotalBits(output_sim))/(bno-1);
  output_proposedBitrate = (long int) FRAMELEN2BITRATE(tmp1);

  input_calculatedBitrate = bitBuffer_sim_calculateBitrate(input_sim);
  input_usedBitrate = bitBuffer_sim_getCurrentBitrate(input_sim);

  /* report bitrates ------------------------------------------------ */

  /* --- input bitrate */
  if (input_usedBitrate==.0)
    PRINT(SE, "INPUT:  used bitrate was:      --unknown--  change with '-bi'\n");
  else if (input_usedBitrate<.0)
    PRINT(SE, "INPUT:  used bitrate was:     --variable--  change with '-bi'\n");
  else
    PRINT(SE, "INPUT:  used bitrate was: %10ld bit/s (avgFrameLen %8.2f bit)  change with '-bi'\n",
          input_usedBitrate, BITRATE2FRAMELEN(input_usedBitrate));
  PRINT(SE,   "        accurate        : %10ld bit/s (avgFrameLen %8.2f bit)\n",
        (long)input_calculatedBitrate, BITRATE2FRAMELEN(input_calculatedBitrate));

  /* --- output bitrate */
  if (output_avgBitrate==.0)
    PRINT(SE, "OUTPUT: used bitrate was:      --unknown--  change with '-b'\n");
  else if (output_avgBitrate<.0)
    PRINT(SE, "OUTPUT: used bitrate was:     --variable--  change with '-b'\n");
  else
    PRINT(SE, "OUTPUT: used bitrate was: %10ld bit/s (avgFrameLen %8.2f bit)  change with '-b'\n",
          output_avgBitrate, BITRATE2FRAMELEN(output_avgBitrate));
  PRINT(SE,   "        accurate        : %10ld bit/s (avgFrameLen %8.2f bit)\n"
              "        proposing       : %10ld bit/s (avgFrameLen %8.2f bit)\n",
        (long)output_calculatedBitrate, BITRATE2FRAMELEN(output_calculatedBitrate),
        (long)output_proposedBitrate, BITRATE2FRAMELEN(output_proposedBitrate));

  /* --- peak bitrate in case of variable bitrate */
  if (output_avgBitrate==-2)
    PRINT(SE, "        used max bitrate: %10ld bit/s (FrameLen    %8.2f bit)\n"
              "        proposing       : %10ld bit/s (FrameLen    %8.2f bit)\n",
          output_maxBitrate, BITRATE2FRAMELEN(output_maxBitrate),
          calculated_maxBitrate, BITRATE2FRAMELEN(calculated_maxBitrate));

  /* report errors -------------------------------------------------- */

  PRINT( SE, "problems:\n");

  /* --- bufferFullnes exceeds limits */
  if (
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
      (param_format!=OUT_MP4FF)&&
#endif
#endif
      (param_format!=OUT_RAW)) {
    if (output_max_fullness > decoderBufferSize - ((output_avgBitrate==-2)?0:BITRATE2FRAMELEN(output_avgBitrate))) {
      tmp = 1;
      PRINT(SE, "bitBuffer-simulation: violated upper bitBuffer-border... (%8.2f>%8.2f)\n",
            output_max_fullness, decoderBufferSize - ((output_avgBitrate==-2)?0:BITRATE2FRAMELEN(output_avgBitrate)));
    }
    if (output_min_fullness < 0) {
      tmp = 1;
      PRINT(SE, "bitBuffer-simulation: violated lower bitBuffer-border... (%8.2f<0)\n",
            output_min_fullness);
    }
  }
  if (tmp == 1)
    PRINT(SE,
          "       You should either specify a different average bitrate with '-b'\n"
          "       or give an initial bufferFullness ('-f') of\n"
          "            at least %6i, and\n"
          "            at most  %6i (currently %i)\n",
          ROUND_UP(output_start_bufferFullness-output_min_fullness),
          (int)((decoderBufferSize-((output_avgBitrate==-2)?0:BITRATE2FRAMELEN(output_avgBitrate)))-output_max_fullness+output_start_bufferFullness),
          output_start_bufferFullness);

  /* --- buffer requirements exceeded */
  if ((output_max_fullness-output_min_fullness) > decoderBufferSize-((output_avgBitrate==-2)?0:BITRATE2FRAMELEN(output_avgBitrate)))
    tmp += 2;
  if (tmp >= 2) {
    PRINT(SE,
          "ERROR in bitBuffer-simulation: buffer_min_size: %i, buffer_max_size %i\n"
          "                            => buffer-limits exceeded!\n"
          "       It may help to give a different bitrate with '-b' (currently %ld bit/s)\n",
          (int)output_min_fullness, (int)output_max_fullness, output_avgBitrate);
  }

  /* --- peak bitrate exceeded */
  if (
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
      (param_format==OUT_MP4FF)||
#endif
#endif
      (param_format==OUT_ADIF)) {
    if ((output_avgBitrate==-2)&&(output_maxBitrate<calculated_maxBitrate)) {
      tmp += 4;
      PRINT(SE,
            "ERROR: specified maximum bitrate is not correct!\n"
            "       try to give a higher value with '-r'\n");
    }
  }

  if (!tmp) PRINT(SE, "no problems found\n");

  return 0;
}



/* callbacks... */
static void feed_output_buffer(int length, long value)
{
  if (copy_to_output) {
    bitoutput_PutBits(bitoutput, length, value);
  }
}


static void set_log_level_conv(int id, int add_bits_num, int add_bits)
     /*
      * to be called once for each element in the stream
      * unfortunally this additional-bits construction is necessary, because
      * these bits indicate the elements tag
      */
{
  bitoutput_startNewElement(bitoutput, id);
  switch(id) {
  case ID_SCE:
  case ID_CPE:
  case ID_CCE:
  case ID_LFE:
  case ID_DSE:
  case ID_PCE: /*log*/
    copy_to_output = 1;
    break;
  case ID_2ND: /*logging is unchanged*/
    return;
  case ID_FIL: /*log if FILL elements are not to be deleted*/
    copy_to_output = !del_fill;
    if ((del_fill)&&(debug['F'])) PRINT(SE, "deleting FILL-element\n");
    break;
#ifdef NEW_ADTS
  case END_HDR: /*ignore*/
    break;
#endif
  case ID_END:
    copy_to_output = 1;
    feed_output_buffer(LEN_SE_ID, ID_END);
    copy_to_output = 0;
    return;
  case LOG_FIN: /*stop log*/
    copy_to_output = 0;
    return;
  case ID_HDR:  /*dont log (cut away this element)*/
  case ID_NUL:
    copy_to_output = 0;
    return;
  default:
    myexit("logger: HELP!!!"); /* will never happen */
  }
  feed_output_buffer(LEN_SE_ID, id);
  if (add_bits_num) feed_output_buffer(add_bits_num, add_bits);
}












int main( int argc, char **argv )
{
  int err;
  /* set defaults */
  default_config = 1;
  mc_info_cmdline.profile=-1;
  mc_info_cmdline.sampling_rate_idx=-1;

  decoder_init();

  parse_args(argc,argv);


  {
    ProgConfig pc;
    int channelConfig;

    /* preparations depending on commandline */
    mc_info.profile = mc_info_cmdline.profile;
    mc_info.sampling_rate_idx = mc_info_cmdline.sampling_rate_idx;
    mc_info.sampling_rate = mc_info_cmdline.sampling_rate;

    /* open input file */
    bitinput = bitinput_openFile(inFile,
                                 &pc,
                                 &mc_info,
                                 &channelConfig,
                                 &input_maxBitrate,
                                 &input_avgBitrate,
                                 &input_start_bufferFullness,
                                 0 /*not verbose*/);
    if (bitinput == NULL) {
      err = -1;
      PRINT(SE, "bitinput_openFile returns error\n");
      goto bail;
    }

    bitinput_setCallbacks(bitinput,
                          feed_output_buffer,
                          set_log_level_conv);

    decoder_setupParameters(mc_info.sampling_rate_idx);

    /* modify mc_info if necessary */
    if (mc_info_changed) {
      fillup_mc_info(mc_info_cmdline, &mc_info);
    }
    if (channelConfig==0) memcpy(&prog_config,&pc,sizeof(ProgConfig));

#ifdef FRAMELENGTH_FLAG
    if ( frameLengthFlag ) {
        output_maxBitrate = (int)ceil((input_maxBitrate * 1024.0f) / 960.0);
        output_avgBitrate = (int)ceil((input_avgBitrate * 1024.0f) / 960.0);
    }
#endif

    /* prepare bit buffer simulations and calculate estimate for output bitrate */
    prepareBitratesAndBufferRequirements(&pc, channelConfig);

    /* open file for writing */
    bitoutput = bitoutput_openFile(outFile, param_format, &mc_info, channelConfig,
                                   output_maxBitrate,
                                   (output_avgBitrate>=0)?output_avgBitrate:0
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

    if (bitoutput == NULL) {
      err = -1;
      PRINT(SE, "bitoutput_openFile returns error\n");
      goto bail;
    }

    bitoutput_setADTSparameters(bitoutput, param_CRC, param_blocks_per_frame
#ifdef MPEG4V1
                                , mpeg2_compatible
#endif
                                );

    /* copy block by block */
    if ((err = addMySamples(bitinput))) {
      PRINT(SE, "addMySamples returns %d\n", err );
      goto bail;
    }
  }

 bail:

  myexit(NULL);
  return err;
}



void myexit(char *s)
{
  bitoutput_close(bitoutput);
  checkBufferRequirements();
  decoder_free();
  bitinput_close(bitinput);

  if (s) PRINT(SE, "\n%s\n", s);
  exit(s ? 1 : 0);
}

