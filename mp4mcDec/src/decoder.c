/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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

#include "adifdec.h"
#include "diagnose.h"
#include "audioout.h"
#include "bitinput.h"
#include "config.h"

#ifdef DRC
#include "drc.h" /* to control drc settings */
#endif
#include "gc.h"  /* to contol ssr settings */

static void		usage(char *s);
static int		parse_cmdline(int argc, char *argv[]);

/* parameters */
static int param_outMultifiles = 0;
static int param_outWordsize = 16;
static char *param_outFormat = "wav";
#ifdef ENABLE_RTPLAY
static int param_outLevel = -1;
#endif

static char *inputfile;
static char *outputfile;


static void usage(char *s)
{
#ifdef MPEG4V1
    PRINT(SE, "MPEG-4 AAC Decoder   2003-08-27\n");
#else
    PRINT(SE, "MPEG-2 AAC Decoder   2003-08-27\n");
#endif
    PRINT(SE, "usage:  %s [options] chan_file pcm_file\n", s);
#ifdef ENABLE_RTPLAY
    PRINT(SE, "	  if pcm_file is '-' the output is played realtime\n");
#endif
    PRINT(SE, "	options are:\n");
    PRINT(SE, "	-h show this help\n");
    PRINT(SE, "	-i program_number (ADIF header present, 0 is first program)\n");
    PRINT(SE, "	-s sampling_frequency, Hz   (48 kHz is default)\n");
    PRINT(SE, "	-a create AIFF output files\n");
#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
    PRINT(SE, "	-w create WAV output files\n");
    PRINT(SE, "	-3 create 24bit output files\n");
    PRINT(SE, "	-m create multiple output files\n");
#endif
#ifdef ENABLE_RTPLAY
    PRINT(SE, "	-ole [1-100] (parameter to control volume)\n");
#endif
    PRINT(SE, "	-D [a-Z]    (enable debugging printouts)\n");
#ifndef MPEG4V1
    PRINT(SE, "	-p profile  (Main or LC or SSR, Main is default)\n");
#else
    PRINT(SE, "	-p profile  (Main or LC or SSR or LTP, Main is default)\n");
#endif
    PRINT(SE, "	-b Band For SSR Profile	(1 to 4, 4 is default)\n");
#ifdef	DRC
    PRINT(SE, "	-d [cut_factor] [boost_factor] [ref_level] for handling DRC\n");
    PRINT(SE, "	   where 0.0 is no compression, 1.0 is max compression for factors\n");
    PRINT(SE, "	   ref_level is quantized in 0.25dB steps using values [0..127]\n");
    PRINT(SE, "	   ref_level is '-' for analog scaling\n");
#endif
    PRINT(SE, "    %d Channels Supported\n",Chans-XChans-BCenter);
#ifdef CT_SBR
    PRINT(SE, "	-l level  (Level of the HE-AAC Profile decoder (2,3,4,5), 5 is default)\n");
    PRINT(SE, "	-r (Forces the decoder to work in down sampled SBR mode, default is off)\n");
#ifdef PARAMETRICSTEREO
    PRINT(SE, "	-c (Enables parametric stereo decoding for SBR, the HE-AAC v2 Profile)\n");
#endif
#endif
    myexit(0);
}

static int
parse_cmdline(int argc, char *argv[])
{
    int i, j, *p;
    char *cmd, *s;

    MC_Info *mip = &mc_info;

#ifdef	DRC
    Float param_drcHigh=0;
    Float param_drcLow=0;
    int param_drcTargetRefLevel=-1;
#endif
    int param_selectedProgram = -1;
    int param_ssrDecoderBand = -1;

    /* save cmd 
     * ARGBEGIN is so clever that it throws away argv[0] and 
     * increments the argv pointer
     */
    cmd = argv[0];
    
    ARGBEGIN {


    default:
      PRINT(SE, "unknown option\n");
    case 'h':
      usage(cmd);
    case 'a':
      param_outFormat = "aiff";
      break;
#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
    case 'w':
      param_outFormat = "wav";
      break;
    case '3':
      param_outWordsize = 24;
      break;
    case 'm':
      param_outMultifiles = 1;
      break;
#endif
#ifdef ENABLE_RTPLAY
    case 'o':
      if (!strcmp(ARGF(),"le")) {  /* -ole */
        s = ARGF();
        param_outLevel = atoi(s);
      }
      break;
#endif
    case 'i':
      s = ARGF();
      param_selectedProgram = atoi(s);
      if ((param_selectedProgram < 0) || (param_selectedProgram > 15)) {
        PRINT(SE, "program number out of range: %d\n", param_selectedProgram);
        myexit("PCE");
      }

      if (debug['v'])
        PRINT(SE, "program with tag %d selected\n", param_selectedProgram);
      break;
    case 'p':
      s = ARGF();
      p = &mip->profile;
      if ((!strncmp(s, "Main", 4)) || (!strncmp(s, "main", 4))) {
        *p = Main_Profile; 
      }
      else if ((!strncmp(s, "LC", 2)) || (!strncmp(s, "lc", 2))) {
        *p = LC_Profile; 
      }
      else if ((!strncmp(s, "SSR", 3)) || (!strncmp(s, "ssr", 3))) {
        *p = SSR_Profile;
      }
#ifdef  MPEG4V1
      else if ((!strncmp(s, "LTP", 3)) || (!strncmp(s, "ltp", 3))) {
        *p = LTP_Profile;
      }
#endif
      else {
        PRINT(SE, "Unsupported profile %s\n", s);
        myexit("profile");
      }
      break;

#ifdef CT_SBR
    case 'l':
      s = ARGF();
      mip->HE_AAC_level = atoi(s);
      break;
    case 'r':
      mip->bDownSampledSbr = 1;
      break;
#ifdef PARAMETRICSTEREO
    case 'c':
#ifdef LOW_POWER_SBR
      PRINT(SE, "PS not available for LP-SBR\n");
      myexit("sbrEnablePS");
#endif
#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
      if (param_outMultifiles)
#endif
        {
          PRINT(SE, "sbrEnablePS does not support multiple output files yet\n");
          myexit(0);
        }
      mip->sbrEnablePS = 1;
      break;
#endif
#endif
    case 's':
      j = atoi(ARGF());
      i = getNextSampFreqIndex(j);
      if (i == -1) {
        PRINT(SE, "Unsupported sampling frequency %d\n", j);
        myexit("samling_rate_idx");
      }
      mip->sampling_rate_idx = i;
      mip->sampling_rate = j;
      break;
    case 'D':
      for (s = ARGF(); *s; s++) {
        debug[(int)(*s)] = 1;
        PRINT(SE, "debug option %c enabled\n", *s);
      }
      break;
    case 'b':
      s = ARGF();
      param_ssrDecoderBand = atoi(s);
      if (param_ssrDecoderBand == 1)
        PRINT(SE, "%d-band is decoded in SSR profile\n", param_ssrDecoderBand);
      else
        PRINT(SE, "%d-bands are decoded in SSR profile\n", param_ssrDecoderBand);
      break;
#ifdef	DRC
    case 'd':
      s = ARGF();
      param_drcHigh /*cut*/ = atof(s);
      s = ARGF();
      param_drcLow /*boost*/ = atof(s);
      s = ARGF();
      if (s[0]!='-') param_drcTargetRefLevel /*ref_level*/ = atoi(s);
      break;
#endif
    }
    ARGEND
      
    if((argc != 1)&&(argc != 2)) {
      usage(cmd);
    }

    /* set decoder-global variables */
    decoder_selectProgram(param_selectedProgram);
    if (param_ssrDecoderBand>0) ssr_decoder_band = param_ssrDecoderBand;
    decoder_setupParameters(mc_info.sampling_rate_idx);
#ifdef DRC
    decoder_setDrc(param_drcHigh, param_drcLow, param_drcTargetRefLevel);
#endif


    inputfile = argv[0];

    if (argc==1) {  /* only one filename on commandline => show header */
      return 1;
    }
    /* two filenames on commandline => prepare output file */

    outputfile = argv[1];

    return argc;
}



/* global, so that it can be closed in myexit() */
static HANDLE_BITINPUT input;
static HANDLE_AUDIOOUTPUT output;

int
main(int argc, char *argv[])
{
  long avgBitrate = 0;
  long maxBitrate = 0;
  int bufferFullness = -1;
  int channelConfig;
  int remainingArgs;
  BUFFER_SIMULATION sim;

  decoder_init();
  remainingArgs = parse_cmdline(argc, argv);	    /* parse command line */

  input = bitinput_openFile(inputfile,
                            &prog_config,
                            &mc_info,
                            &channelConfig,
                            &maxBitrate,
                            &avgBitrate,
                            &bufferFullness,
                            (remainingArgs==1));

  decoder_setupParameters(mc_info.sampling_rate_idx);

  if (outputfile != NULL) {
    output = audioout_open(outputfile, param_outFormat, param_outWordsize);
  }
#ifdef ENABLE_RTPLAY
  if (param_outLevel>=0) audioout_setRealtimePlay(output, param_outLevel);
#endif

  if (input==NULL) myexit("error opening input file");
  if (remainingArgs==1) myexit("error opening output file");

  /* prepare buffer simulation */
  sim = create_bitBuffer_sim();
  {
    long bufferSize;
    double avgFrameLen = .0;

    getDecoderBufferSize(&prog_config, channelConfig, &bufferSize);
    if (NULL != bitinput_getCurrentADTSHeader(input)) avgBitrate = 0; /* get more accurate estimate from ADTS-Headers */

    if (avgBitrate>0) avgFrameLen = BITRATE2FRAMELEN(avgBitrate);
    else if (avgBitrate==0) avgFrameLen = 0; /* unknown, calculate on the fly */
    else if (avgBitrate<0) avgFrameLen = -1; /* variable bitrate */

    if (bufferFullness == 0x7fffffff) { /* also indicates variable bitrate */
      bufferFullness = 0;
      avgFrameLen = -1;
    }

    bitBuffer_sim_setFrameLen(sim, avgFrameLen, bufferSize);
    if (maxBitrate>0) bitBuffer_sim_testPeakBitrate(sim, maxBitrate);
  }

  /* main loop */
  for(bno = 0;; bno++) {
    Float **output_data;
    int output_blockLen;
    int err = bitinput_getNextBlock(input);
    if (err == -2) break; /* eof */
    if (err) myexit("error reading next block");

    dec(&output_data, &output_blockLen);

    audioout_write(output, output_data, output_blockLen, &mc_info);

    /* do buffer simulation */
    {
      const ADTS_Header *tmp_head = bitinput_getCurrentADTSHeader(input);
      update_bitBuffer_sim(sim, bitinput_resetCounter(input)<<3, tmp_head);
    }
    /* initial buffer fullness */
    if ((bno==0)&&(bufferFullness!=-1)) {
      bitBuffer_sim_adjustFullness(sim, bufferFullness);
    }
    check_bitBuffer_sim(sim, "", debug['b'], debug['B'], debug['R']);
  }

  myexit(NULL);
  return (EXIT_SUCCESS);
}


void
myexit(char *s)
{
  audioout_close(output);
  bitinput_close(input);
  decoder_free();
  if (s) PRINT(SE,"\n%s\n", s);
  exit(s ? 1 : 0);
}
