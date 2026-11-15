/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Olaf Kaehler (Fraunhofer IIS-A)

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



Source file: mp4enc.c


Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bit stream module
audio.o                 audio i/o module
enc_par.o               encoder core (parametric)
enc_lpc.o               encoder core (CELP)
enc_tf.o                encoder core (T/F)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
SE    Sebastien ETienne, Jean Bernard Rault CCETT <jbrault@ccett.fr>
OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>

Changes:
13-jun-96   HP    first version
14-jun-96   HP    added debug code
17-jun-96   HP    added bit reservoir control / switches -r -b
18-jun-96   HP    added delay compensation
19-jun-96   HP    using new ComposeFileName()
25-jun-96   HP    changed frameNumBit
26-jun-96   HP    improved handling of switch -o
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
14-aug-96   HP    adapted to new cmdline module, added mp4.h
16-aug-96   HP    adapted to new enc.h
                  added multichannel signal handling
21-aug-96   HP    added AAC_RM4 switches by BG
26-aug-96   HP    CVS
28-aug-96   NI    moved "static CmdLineSwitch switchList[]" into main routine.
29-aug-96   HP    moved "switchlist[]" back again
                  NI stated, that it just was a locally required bug-fix:
                  This table is moved into the main routine,
                  because there was a run-time problem on a
                  SGI (IRIX-5.3) system (using g++).
30-oct-96   HP    additional frame work options
15-nov-96   HP    adapted to new bitstream module
18-nov-96   HP    changed int to long where required
                  added bit stream header options
                  removed AAC_RM4 switches (new -nh, -ms, -bi options)
10-dec-96   HP    new -ni, -vr options, added variable bit rate
23-jan-97   HP    added audio i/o module
07-apr-97   HP    i/o filename handling improved / "-" supported
08-apr-97   SE    added G729-based coder
15-may-97   HP    clean up
30-jun-97   HP    fixed totNumSample bug / calc encNumFrame
05-nov-97   HP    update by FhG/UER
12-nov-97   HP    added options -n & -s
02-dec-98   HP    merged most FDIS contributions ...
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP    trying to clean up a bit ...
22-jan-99   HP    stdin/stdout "-" support, -d 1 uses stderr
22-apr-99   HP    merging all contributions
19-jun-01   OK    removed G723/G729 dummy calls
22-jul-03   AR/RJ add extension 2 parametric coding
19-oct-03   OK    clean up for rewrite
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "allHandles.h"
#include "encoder.h"

#include "streamfile.h"
#include "common_m4a.h" /* common module */
#include "cmdline.h"    /* command line module */
#include "bitstream.h"  /* bit stream module */
#include "audio.h"      /* audio i/o module */
#include "mp4au.h"      /* frame work declarations */
#if 0
#include "enc_par.h"    /* encoder cores ... */
#include "enc_lpc.h"
#endif
#include "enc_tf.h"
#include "fir_filt.h"   /* fir lowpass filter */
#include "plotmtv.h"
#ifdef EXT2PAR
#include "SscEnc.h"
#endif

#include "flex_mux.h"

#ifdef I2R_LOSSLESS
#include "int_mdct_defs.h"
#endif
#include "streamfile.h"
#include "streamfile_helper.h"

/* ---------- declarations ---------- */

#define PROGVER "MPEG-4 Natural Audio Encoder (Edition 2001)   2003-08-27"
#define CVSID "$Id: mp4auenc.c,v 1.10.4.10 2007/06/07 11:30:08 samsung1 Exp $"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ---------- variables ---------- */
#ifdef DEBUGPLOT
extern int framePlot; /* global for debugging purposes */
extern int firstFrame;
#endif

static struct EncoderMode encoderCodecsList[] = {
  {MODE_TF,   
   MODENAME_TF,   
   EncTfInfo, 
   EncTfInit, 
   EncTfFrame, 
   EncTfFree
  },
  {-1,        
   NULL,          
   NULL, 
   NULL, 
   NULL, 
   NULL
  }
};

/* command line module */

static char *codecMode[MAX_CODECS];
static char *encPara[MAX_CODECS];
static int numChannelOut[MAX_CODECS];
static float regionStart;
static float regionDurat;
static int regionDuratUsed;
static char *outFileName;
static int outFileNameUsed;
static float fSampleOut;
static char *oriPath,*bitPath;
static int oriPathUsed,bitPathUsed;
static char *oriExt,*bitExt;
static int oriExtUsed,bitExtUsed;
static int bitBufSize;
static int mainDebugLevel;
static int audioDebugLevel;
static int bitDebugLevel;
static int cmdDebugLevel;
static int *varArgIdx;

#ifdef I2R_LOSSLESS
static float ll_bitRate;
static char * ll_bitExt;
static int coredisable = 0;
static int msStereoIntMDCTMode; /* 0: always off, 1: always on, 2: dynamic (default) */ 
#endif
static int osf = 1; /* is always 1 for non-SLS, might become 1,2,4 for SLS (command line parameter) */

#ifdef DEBUGPLOT
int frame; /* global for debugging */
#else 
static  int frame;
#endif 


static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<audio file(s) (- = stdin)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"m",&codecMode[0],"%s",MODENAME_TF,NULL,"codec mode (" MODENAME_LIST ")"},
  {"m2",&codecMode[1],"%s",NULL,NULL,"codec mode for ext. layer (" MODENAME_LIST ")"},
  {"c",&encPara[0],"%s","",NULL, "encoder parameter string"},
  {"c2",&encPara[1],"%s","",NULL, "encoder parameter string for ext. layer"},
  {"n",&numChannelOut[0],"%d","0",NULL,
   "number of audio channels remixed for encoder\n"
   "(0=as input file)"},
  {"n2",&numChannelOut[1],"%d","0",NULL,
   "number of audio channels remixed for ext. layer encoder\n"
   "(0=as input file)"},
  {"s",&fSampleOut,"%f","0",NULL,
   "sampling frequency [Hz] for bit stream header\n"
   "(0=as input file)"},
  {"ts",&regionStart,"%f","0",NULL,"start time of encoded region [sec]"},
  {"td",&regionDurat,"%f",NULL,&regionDuratUsed,
   "duration of encoded region [sec]\n"
   "(dflt: until end of file)"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,
   "output file name (- = stdout)"},
  {"po",&oriPath,"%s",NULL,&oriPathUsed,
   "default path original audio files (dflt: $" MP4_ORI_PATH_ENV ")"},
  {"pb",&bitPath,"%s",NULL,&bitPathUsed,
   "default path bit stream files (dflt: $" MP4_BIT_PATH_ENV ")"},
  {"eo",&oriExt,"%s",NULL,&oriExtUsed,
   "original audio file extension\n"
#ifdef USE_AFSP
   "supported file formats: .au, .snd, .wav, .aif, .raw\n"
#else
   "supported file formats: .au, .snd\n"
#endif /*USE_AFSP*/
   "(dflt: $" MP4_ORI_FMT_ENV " / " MP4_ORI_EXT ")\n"
   "(raw format: $" MP4_ORI_RAW_ENV ")"},
  {"eb",&bitExt,"%s",MP4_BIT_EXT,&bitExtUsed,"bit stream file extension"},
  {"bb",&bitBufSize,"%i","0",NULL,"bitstream buffer size [bit] (0=auto)"},
#ifdef I2R_LOSSLESS
  /* {"ll_eb",&ll_bitExt,"%s",".aaz",NULL,"lossless bit stream file extension"}, */
  {"ncore",&coredisable,NULL,NULL,NULL,"disable AAC core (default enable)"}, 
  {"msStereoIntMDCTMode",&msStereoIntMDCTMode,"%d","2",NULL,"0: always off, 1: always on, 2: dynamic (default)"}, 
  {"osf",&osf,"%d","1",NULL,"oversampling factor for lossless enhancement (dflt: 1, options: 1,2,4)"},
#endif
  {"d",&mainDebugLevel,"%i","0",NULL,"main debug level"},
  {"da",&audioDebugLevel,"%i","0",NULL,"audio file debug level"},
  {"db",&bitDebugLevel,"%i","0",NULL,"bit stream debug level"},
  {"dc",&cmdDebugLevel,"%i","0",NULL,"cmd line debug level"},
#ifdef DEBUGPLOT
  {"ps",&firstFrame,"%i","9999",NULL,"start plot at frame%"},
#endif
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* hack to make buffer control independent of -c -mp4ff  HP20010212 */
int PARusedNumBit;

/* SSC data */
#ifdef EXT2PAR
PROCESSINFO*  pPI;
ENCODERINFO*  pEI;
SSCBUNCH*     pSSCBunch;
CHUNK_LENGTHS ChunkLengths;
#endif

/* ---------- temp scratch ------- */
#if 0
  if (!strcmp(codecMode,"ssc"))
  {
    bitRate       = 0;      /* Set to 0, because SSC Encoding is not bitrate controlled yet */
    bitReservSize = 0;
    bitReservInit = 0;
    varBitRate    = 1;      /* Set to 1, because SSC Encoding is not bitrate controlled yet */
    bitExt        = ".ssc"; /* Force ssc file format */
  }
#endif
/* ---------- functions ---------- */

static HANDLE_ENCODER newEncoder()
{
  HANDLE_ENCODER ret = (HANDLE_ENCODER)malloc(sizeof(struct tagEncoderData));
  ret->data = NULL;
  ret->getNumChannels = NULL;
  ret->getReconstructedTimeSignal = NULL;
  ret->getEncoderMode = NULL;
  return ret;
}

/* 20060107 */
#include <math.h>
#include "libtsp.h"
static double
FIxKaiser (double x, double alpha)

{
  static double alpha_prev = 0.0;
  static double Ia = 1.0;
  double beta, win;

  /* Avoid recalculating the denominator Bessel function */
  if (alpha != alpha_prev) {
    Ia = FNbessI0 (alpha);
    alpha_prev = alpha;
  }

  if (x < -1.0 || x > 1.0)
    win = 0.0;
  else {
    beta = alpha * sqrt (1.0 - x * x);
    win = FNbessI0 (beta) / Ia;
  }

  return win;
}

static void
RSKaiserLPF (float h[], int N, double Fc, double alpha)
{
  int i, k;
  double t, u, T, Ga;

  /* Multiply the Kaiser window by the sin(x)/x response */
  /* Use symmetry to reduce computations */
  Ga = 2.0 * Fc;
  T = 0.5 * (N-1);
  for (i = 0, k = N-1; i <= k; ++i, --k) {
    t = i - T;
    u = k - T;
    h[i] = (float) (Ga * FNsinc (2.0 * Fc * t) * FIxKaiser (t / T, alpha));
    if (-t == u)
      h[k] = h[i];
    else
      h[k] = (float) (Ga * FNsinc (2.0 * Fc * u) * FIxKaiser (u / T, alpha));
  }
}


static void
FIdownsample(const float* input, float* out, int NOut, const float*win, int Ncof)
{
  int i, j, k, l;
  double sum;
  
  k = Ncof-1;
  for (i=0; i<NOut; i++) {
    sum = 0.0;
    l = k;
    for (j=0; j<Ncof; j++) {
      sum += win[j]*input[l--];
    }
    out[i] = sum;
    k+=2;
  }
}
/* 20060107 */
/* Encode() */
/* Encode audio file and generate bit stream. */
/* (This function evaluates the global xxxDebugLevel variables !!!) */

static int Encode ( 
                   char *audioFileName,          /* in: audio file name */
                   char *bitFileName,            /* in: bit stream file name */
                   char *codecMode[],            /* in: codec mode string(s) */
                   char *encPara[],              /* in: encoder parameter string(s) */
                   int num_codecs,               /* in: number of codecMode's */
                   float regionStart,            /* in: start time of region */
                   float regionDurat,            /* in: duration of region */
                   int numChannelOut[],          /* in: number of channels (0 = as input) */
                   float fSampleOut)             /* in: sampling frequency (0 = as input) */
                                /* returns: 0=OK  1=error */
{
  AudioFile *audioFile;
  float **sampleBuf;
  float **reSampleBuf;
  float **encSampleBuf;
  float fSample=0;
  long fileNumSample;
  long totNumSample;
  int numSample;
  int numChannel;
  int frameNumSample;
  long fSampleLong;
  HANDLE_STREAMFILE outfile;
  HANDLE_STREAMPROG outprog;
  HANDLE_BSBITBUFFER *au_buffers;
#ifdef I2R_LOSSLESS
#if 0
  HANDLE_BSBITBUFFER *ll_au_buffers;
#endif
#endif
  int startupNumFrame,startupFrame;
  int ch, i, i_enc;
  long startSample;
  long encNumSample;
  int numChannelBS[MAX_CODECS];
  long fSampleLongBS;
  ENC_FRAME_DATA frameData;
  int track_count_total;
  int delayNumSample = 0;

  FIR_FILT *lowpassFilt = NULL;
  int downsamplFac = 1;

/* 20060107 */
  int sbrenable = 0;
  int downSampleRatio = 1;
  int bitRate;
  int NcoefRS = 0;
  float *hRS = NULL;
  float **sampleBufRS = NULL;
  float **sampleBufRSold = NULL;
  float *tmpBufRS = NULL;
  float lowPassFreq = 0;
/* 20060107 */
#ifdef I2R_LOSSLESS
  int type_PCM = 0;
  /* long ll_bits_to_use; */
  /* int frameLength = 0; */
  /* int ll_track_count_total; */
#endif
  int frameMaxNumBit; /* SAMSUNG_2005-09-30 : maximum number of bits per frame */

  /* ---- init ---- */
  if (mainDebugLevel >= 3) {
    printf("Encode:\n");
    printf("audioFileName=\"%s\"\n",audioFileName);
    printf("bitFileName=\"%s\"\n",bitFileName);
    for (i_enc=0; i_enc<num_codecs; i_enc++) {
      printf("codecMode[%i]=\"%s\"\n",i_enc,codecMode[i_enc]);
      printf("encPara[%i]=\"%s\"\n",i_enc,encPara[i_enc]?encPara[i_enc]:"(NULL)");
    }
    printf("regionStart=%.6f\n",regionStart);
    printf("regionDurat=%.6f\n",regionDurat);
  }

  SetDebugLevel(mainDebugLevel);
  BsInit(bitBufSize,bitDebugLevel,0);
  AudioInit(getenv(MP4_ORI_RAW_ENV),       /* headerless file format */
            audioDebugLevel);

  /* ---- open audio file ---- */
  audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
                            &fileNumSample);
  if (audioFile==NULL)
    CommonExit(1,"Encode: error opening audio file %s "
               "(maybe unknown format)",
               audioFileName);

#ifdef I2R_LOSSLESS
  /******  test osf  */
  fileNumSample /= osf;
  type_PCM = AudioType(audioFile);
  fSample = fSample / osf;
#endif

  startSample = (long)(regionStart*fSample+0.5);

  if (regionDurat >= 0)
    encNumSample = (long)(regionDurat*fSample+0.5);
  else
    if (fileNumSample < 0) /* unkown */
      encNumSample = -1;
    else
      encNumSample = max(0,fileNumSample-startSample);


  /* ---- init encoder ---- */
  fSampleLong = (long)(fSample+0.5);
  for (i_enc=0; i_enc<MAX_CODECS; i_enc++) {
    numChannelBS[i_enc] = (numChannelOut[i_enc]==0) ? numChannel : numChannelOut[i_enc];
    if (numChannelBS[i_enc] > numChannel) {
      CommonWarning("Desired number of channels too large, using %i instead of %i\n",numChannel,numChannelBS[i_enc]);
      numChannelBS[i_enc] = numChannel;
    }
  }
  fSampleLongBS = (fSampleOut==0) ? fSampleLong : (long)(fSampleOut+0.5);

  /* ---- get selected codec ---- */
  track_count_total = 0;
  frameData.encoder_count = num_codecs;
  frameData.trackCount = (int*)malloc(frameData.encoder_count*sizeof(int));
  frameData.enc_mode = (struct EncoderMode*)malloc(frameData.encoder_count*sizeof(EncoderMode));
  frameData.enc_data = (HANDLE_ENCODER*)malloc(frameData.encoder_count*sizeof(HANDLE_ENCODER));
  for (i=0; i<MAX_TRACKS; i++) {
    setupDecConfigDescr(&frameData.dec_conf[i]);
    frameData.dec_conf[i].profileAndLevelIndication.value = 0x40;
    frameData.dec_conf[i].streamType.value = 0x05;
    frameData.dec_conf[i].upsteam.value = 0;
  }
#ifdef I2R_LOSSLESS
#if 0
  for (i=0; i<LL_MAX_TRACKS; i++) {
    setupDecConfigDescr(&frameData.ll_dec_conf[i]);
    frameData.ll_dec_conf[i].profileAndLevelIndication.value = 0x40;
    frameData.ll_dec_conf[i].streamType.value = 0x05;
    frameData.ll_dec_conf[i].upsteam.value = 0;
  }
#endif
#endif



  for (i_enc=0; i_enc<frameData.encoder_count; i_enc++) {
    struct EncoderMode *mode;
    i = 0;

    do {
      mode = &encoderCodecsList[i++];
    } while ((mode->mode != -1) && strcmp(codecMode[i_enc],mode->modename));
    if (mode->mode == -1)
      CommonExit(1,"Encode: unknown codec mode %s",codecMode[i_enc]);

    frameData.enc_mode[i_enc] = *mode;

    /*varBitRate*/
    frameData.enc_data[i_enc] = newEncoder();
    i = frameData.enc_mode[i_enc].EncInit(numChannelBS[i_enc], 
		    			  fSample,
                                          encPara[i_enc],
                                          &frameNumSample,
                                          &frameMaxNumBit,   /* SAMSUNG_2005-09-30 */
                                          &(frameData.dec_conf[track_count_total]),
                                          &(frameData.trackCount[i_enc]),
#ifdef I2R_LOSSLESS
#if 0
                                          &(frameData.ll_dec_conf[0]),
                                          &(frameData.ll_trackCount),
#endif
                                          type_PCM,
                                          osf,
#endif
                                          (i_enc==0)?NULL:frameData.enc_data[i_enc-1],
                                          frameData.enc_data[i_enc]);
    if (i!=0) CommonExit(-1,"EncInit [%i] returned %i", i_enc, i);
    DebugPrintf(1,"initialized encoder: %i tracks",frameData.trackCount[i_enc]);
    for (i=0; i<frameData.trackCount[i_enc]; i++) {
      DebugPrintf(2,"                   track %i: AOT %i",i,frameData.dec_conf[track_count_total+i].audioSpecificConfig.audioDecoderType.value);
    }
    track_count_total += frameData.trackCount[i_enc];

#if 0
#ifdef EXT2PAR
  case MODE_SSC:
    if (!EncSSCInit(numChannel,fSample,fileNumSample,audioFileName,
                    bitFileName,encPara,&pPI,&pEI,&pSSCBunch,
                    &frameNumSample,&delayNumSample,bitHeader))
    {
      CommonExit(1,"SSC initialisation error\n");
    }
    break;
#endif
#endif
  }

/* 20060107 */
  i_enc = 0;
  sbrenable = Get_sbrenable(frameData.enc_data[i_enc], &bitRate);
  if (sbrenable) {
	if (fSample != 44100.0f && fSample != 48000.0f) {
#ifndef SAIT
      CommonExit(1,"Encode: error BSAC+SBR only with 44.1 or 48 kHz" 
          " input samplerate");
#else
	  fSample /= 2.0f;
	  downSampleRatio = 2;
#endif
    } else {
      fSample /= 2.0f;
	  downSampleRatio = 2;
    }
    if (!((numChannelBS[i_enc]==1 && bitRate==24000) || 
          (numChannelBS[i_enc]==2 && bitRate==48000))) {
      /*CommonExit(1,"Encode: error supported BSAC+SBR bitrates currently only\n"
                 "24kbps mono or 48 kbs stereo");*/
    }
    lowPassFreq = 5000.0f;
    if (numChannelBS[i_enc]==1) {
      if (bitRate==24000 && fSample==22050)
        lowPassFreq = 5513.0f; /* startFreq 4 @ 22.05kHz */
      if (bitRate==24000 && fSample==24000)
        lowPassFreq = 5625.0f; /* startFreq 4 @ 24kHz */
    }
    if (numChannelBS[i_enc]==2) {
      if (bitRate==48000 && fSample==22050)
        lowPassFreq = 7924.0f; /* startFreq 9 @ 22.05kHz */
      if (bitRate==48000 && fSample==24000)
        lowPassFreq = 8250.0f; /* startFreq 9 @ 24kHz */
    }
	lowPassFreq = fSample/2.;

	{
    /* init resampler and buffers for downsampling by 2 */
    float Fc = lowPassFreq / (fSample * 2);
    float dF = 0.1f * Fc; /* 10% transition bw */
    float alpha = 7.865; /* 80 dB attenuation */
    float D = 5.015; /* transition width ? */
    NcoefRS = (int)ceil(D/dF) + 1;

    /* downsampling filter coefficients */
    if ((hRS = (float *)malloc(NcoefRS*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
    RSKaiserLPF (hRS, NcoefRS, (double)Fc, (double)alpha);

    /* input buffers */
    if ((sampleBufRS=(float**)malloc(numChannel*sizeof(float*)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
    if ((sampleBufRSold=(float**)malloc(numChannel*sizeof(float*)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
    for (ch=0; ch<numChannel; ch++) {
      if ((sampleBufRS[ch]=(float*)calloc(2*frameNumSample,sizeof(float)))==
          NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((sampleBufRSold[ch]=(float*)calloc(NcoefRS,sizeof(float)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
    }
    if ((tmpBufRS=(float*)calloc(NcoefRS+2*frameNumSample,sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
  }
  }
/* 20060107 */
#ifdef I2R_LOSSLESS
#if 0
  ll_track_count_total = frameData.ll_trackCount;
#endif
#endif

  if (mainDebugLevel >= 3) {
    /*printf("mode=%d\n",mode);*/
    printf("fSample=%.3f Hz  (int=%ld)\n",fSample,fSampleLong);
    printf("frameNumSample=%d  (%.6f sec/frame)\n",
           frameNumSample,frameNumSample/fSample);
    printf("fileNumSample=%ld  (%.3f sec  %.3f frames)\n",
           fileNumSample,fileNumSample/fSample,
           fileNumSample/(float)frameNumSample);
    printf("startSample=%ld\n",startSample);
    printf("encNumSample=%ld  (%.3f sec  %.3f frames)\n",
           encNumSample,encNumSample/fSample,
           encNumSample/(float)frameNumSample);
  }

  /* allocate buffers */
#ifdef EXT2PAR
  if (mode == MODE_SSC)
  {
    bitBuf = BsAllocBuffer((SSC_BSC_MAX_BUFFER_LENGTH*8));
  }
  else
#endif
  {		
    au_buffers = (HANDLE_BSBITBUFFER*)malloc(track_count_total*sizeof(HANDLE_BSBITBUFFER));
    for (i=0; i<track_count_total; i++) {
      au_buffers[i] = BsAllocBuffer(frameMaxNumBit); /* SAMSUNG_2005-09-30 : modified for Multichanel */  /* todo chk haertlml: big enough for sls ? */
    }
#ifdef I2R_LOSSLESS
#if 0
    ll_au_buffers = (HANDLE_BSBITBUFFER*)malloc(ll_track_count_total*sizeof(HANDLE_BSBITBUFFER));
    for (i=0; i<ll_track_count_total; i++) {
      ll_au_buffers[i] = BsAllocBuffer(712856/*ll_bits_to_use*/);
    }
#endif
#endif
  }
  
  if ((sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((sampleBuf[ch]=(float*)malloc(MAX_OSF*frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");

  if ((reSampleBuf=(float**)malloc(MAX_OSF*numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((reSampleBuf[ch]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");

  /* ---- open bit stream file ---- */
  outfile = StreamFileOpenWrite(bitFileName, FILETYPE_AUTO, NULL);

  if (outfile==NULL)
    CommonExit(1,"Encode: error opening bit stream file %s",bitFileName);

  outprog = StreamFileAddProgram(outfile);
  outprog->trackCount = track_count_total;

#if 0 /* #ifdef I2R_LOSSLESS */
  outprog->ll_trackCount = ll_track_count_total;
#endif

  for (i=0; i<track_count_total; i++) {
    outprog->decoderConfig[i] = frameData.dec_conf[i];
    outprog->dependencies[i] = i-1;
    DebugPrintf(2," track %i: AOT %i", i, outprog->decoderConfig[i].audioSpecificConfig.audioDecoderType.value);
  }

#if 0 /* #ifdef I2R_LOSSLESS */
 
  for (i=0; i<ll_track_count_total; i++) {
    outprog->ll_decoderConfig[i] = frameData.ll_dec_conf[i];
    outprog->dependencies[i+track_count_total] = i+track_count_total-1;
    DebugPrintf(2," track %i: AOT %i", i, outprog->decoderConfig[i].audioSpecificConfig.audioDecoderType.value);
  }
  
#endif

  DebugPrintf(2,"total track count: %i\n",(int)outprog->trackCount);

  /* num frames to start up encoder due to delay compensation */
  startupNumFrame = (delayNumSample+frameNumSample-1)/frameNumSample;

  /* seek to beginning of first (startup) frame (with delay compensation) */
  AudioSeek(audioFile,
            startSample+delayNumSample-startupNumFrame*frameNumSample);

  if (mainDebugLevel >= 3)
    printf("startupNumFrame=%d\n",startupNumFrame);

  /* process audio file frame by frame */
  frame = -startupNumFrame;
  totNumSample = 0;

for (i=0; i<track_count_total; i++) {
  DebugPrintf(2,"                   track %i: AOT %i",i,frameData.dec_conf[i].audioSpecificConfig.audioDecoderType.value);
}
  do {
#ifdef FHG_DEBUGPLOT
    plotInit(framePlot,BsGetFileName(bitStream),1);
#endif    
    if ((mainDebugLevel == 1 )&& (frame%20 == 0)) {      
      fprintf(stderr,"\rframe %4d ",frame);
      fflush(stderr);
    }
    if (mainDebugLevel >= 2 && mainDebugLevel <= 3) {
      printf("\rframe %4d ",frame);
      fflush(stdout);
    }
    if (mainDebugLevel > 3)
      fprintf(stderr,"frame %4d\n",frame);

    /* check for startup frame */
    startupFrame = frame < 0;

/* 20060107 */
	if(sbrenable)
	/* save last NcoefRS samples */
  for (ch=0; ch<numChannel; ch++)
    for (i=0; i<NcoefRS; i++)
      sampleBufRSold[ch][i] = sampleBufRS[ch][2*frameNumSample-NcoefRS+i];
/* 20060107 */
    /* read audio file */
/* 20060107 */
	if(sbrenable)
		numSample = AudioReadData(audioFile,sampleBufRS,frameNumSample*2);
    /* read audio file */
	else {
    numSample = AudioReadData(audioFile,sampleBuf,osf*frameNumSample);
	numSample /= osf;
	}
    totNumSample += numSample;
    if (numSample != frameNumSample && encNumSample == -1)
      encNumSample = totNumSample;

/* 20060107 */
	if(sbrenable)
	for (ch=0; ch<numChannel; ch++) {
      for (i=0; i<NcoefRS; i++)
        tmpBufRS[i] = sampleBufRSold[ch][i];
      for (i=NcoefRS; i<2*frameNumSample+NcoefRS; i++)
        tmpBufRS[i] = sampleBufRS[ch][i-NcoefRS];
      FIdownsample(tmpBufRS, sampleBuf[ch], frameNumSample, hRS, NcoefRS);
    }
/* 20060107 */

    /* encode one frame */
    track_count_total = 0;
    for (i_enc=0; i_enc<frameData.encoder_count; i_enc++) {

      /* mix down stereo to mono */
      encSampleBuf = sampleBuf;
      if ((numChannelBS[i_enc]==1)&&(numChannel==2)) {
        for (i=0; i<frameNumSample; i++) {
          reSampleBuf[0][i] = (sampleBuf[0][i] + sampleBuf[1][i]) / 2;
        }
        encSampleBuf = reSampleBuf;
      }

      frameData.enc_mode[i_enc].EncFrame(encSampleBuf,
                                         &au_buffers[track_count_total],
#ifdef I2R_LOSSLESS
#if 0
                                         &ll_au_buffers[0],
#endif
#endif
                                         frameData.enc_data[i_enc],
				sbrenable ? sampleBufRS : NULL);
      track_count_total += frameData.trackCount[i_enc];
    }

    for (i=0; i<track_count_total; i++) {
      HANDLE_STREAM_AU au = StreamFileAllocateAU(0);
      au->numBits = BsBufferNumBit(au_buffers[i]);
      au->data = BsBufferGetDataBegin(au_buffers[i]);
      StreamPutAccessUnit(outprog, i, au);
      BsClearBuffer(au_buffers[i]);
    }

#ifdef I2R_LOSSLESS
#if 0
    {
      int j;
      for (j=0; j<ll_track_count_total;j++, i++) 
        {
          HANDLE_STREAM_AU au = StreamFileAllocateAU(0);
          au->numBits = BsBufferNumBit(ll_au_buffers[j]);
          au->data = BsBufferGetDataBegin(ll_au_buffers[j]);
          StreamPutAccessUnit(outprog, i, au);
          BsClearBuffer(ll_au_buffers[j]);
        }
    }
#endif
#endif

    frame++;
#ifdef DEBUGPLOT    
    framePlot++;
    plotDisplay(0);
#endif
/* 20060107 */
  } while (encNumSample < 0 || frame*(long)frameNumSample*downSampleRatio < encNumSample
#ifdef I2R_LOSSLESS
           +2*(long)frameNumSample
#endif
           ); 

#if 0
#ifdef EXT2PAR
  if (mode == MODE_SSC)
  {
    /* Flush the last frames */
    while (EncSSCFrame (sampleBuf,bitBuf,pEI,pPI,&pSSCBunch))
    {
      if (BsPutBuffer(bitStream,bitBuf))
      {
        CommonExit(1,"Encode: error writing bit stream data");
      }
    }
  }
#endif
#endif

  if (mainDebugLevel == 1)
    fprintf(stderr," \n");
  if (mainDebugLevel >= 2 && mainDebugLevel <= 3)
    printf(" \n");

  if (mainDebugLevel >= 3) {
    printf("totNumFrame=%d\n",frame);
    printf("encNumSample=%ld  (%.3f sec  %.3f frames)\n",
           encNumSample,encNumSample/fSample,
           encNumSample/(float)frameNumSample);
    printf("totNumSample=%ld\n",totNumSample);
  }

  /* free encoder memory */
  for (i_enc=0; i_enc<frameData.encoder_count; i_enc++) {
    frameData.enc_mode[i_enc].EncFree(frameData.enc_data[i_enc]);
  }

#if 0
#ifdef EXT2PAR
  case MODE_SSC:
    SSC_Encoder_Close(pPI,&ChunkLengths);
    SSC_WriteChunkLengths(ChunkLengths, bitStream);
    SSC_FreeEncoderInfo(pEI);
    break;
#endif
#endif

  /* close audio file */
  AudioClose(audioFile);
  
  /* close bit stream file */
  if (StreamFileClose(outfile))
    CommonExit(1,"Encode: error closing bit stream file");

  /* free buffers */
  for (ch=0; ch<numChannel; ch++) {
    free(sampleBuf[ch]);
    free(reSampleBuf[ch]);
  }
  free(sampleBuf);
  free(reSampleBuf);
  for (i=0; i<track_count_total; i++) {
    BsFreeBuffer(au_buffers[i]);
  }

/* 20060107 */
  /* free downsampling buffers */
  if (downSampleRatio==2) {
    free(hRS);
    for (ch=0; ch<numChannel; ch++) {
      free(sampleBufRS[ch]);
      free(sampleBufRSold[ch]);
    }
    free(sampleBufRS);
    free(sampleBufRSold);
    free(tmpBufRS);
  }
/* 20060107 */
  return 0;
}



/* ---------- main ---------- */

int main (int argc, char *argv[])
{
  char *progName = "<no program name>";
  int result;
  char oriFileName[STRLEN],bitFileName[STRLEN];
  int i;
  int fileIdx;

  /* evaluate command line  */
  CmdLineInit(0);
  result = CmdLineEval(argc,argv,paraList,switchList,1,&progName);
  if (result) {
    if (result==1) {
      printf("%s: %s\n",progName,PROGVER);
      CmdLineHelp(progName,paraList,switchList,stdout);
      for (i=0; encoderCodecsList[i].mode!=-1; i++) {
        encoderCodecsList[i].EncInfo(stdout);
      }
      exit (1);
    }
    else
      CommonExit(1,"command line error (\"-h\" for help)");
  }

  if (mainDebugLevel == 1)
    fprintf(stderr,"%s: %s\n",progName,PROGVER);
  if (mainDebugLevel >= 2) {
    printf("%s: %s\n",progName,PROGVER);
    printf("CVS Id: %s\n",CVSID);
    for (i=0; encoderCodecsList[i].mode!=-1; i++) {
      printf("%s\n",encoderCodecsList[i].EncInfo(NULL));
    }
  }

  CmdLineInit(cmdDebugLevel);

  /* calc variable default values */
  if (!regionDuratUsed)
    regionDurat = -1;
  if (!oriPathUsed)
    oriPath = getenv(MP4_ORI_PATH_ENV);
  if (!bitPathUsed)
    bitPath = getenv(MP4_BIT_PATH_ENV);
  if (!oriExtUsed)
    oriExt = getenv(MP4_ORI_FMT_ENV);
  if (oriExt==NULL)
    oriExt = MP4_ORI_EXT;

  /* check command line options */
  if (regionDuratUsed && regionDurat < 0)
    CommonExit(1,"duration of region < 0");
  if (outFileNameUsed && varArgIdx[0]>=0 && varArgIdx[1]>=0)
    CommonExit(1,"only one input file allowed when using -o");
  if (varArgIdx[0]<0)
    CommonExit(1,"no input file specified");
  

#ifdef EXT2PAR
  if (!strcmp(codecMode,"ssc"))
  {
    bitExt        = ".ssc"; /* Force ssc file format */
  } else
#endif
    bitExt        = ".mp4";

  /* process all files on command line */
  fileIdx = 0;
  while (varArgIdx[fileIdx] >= 0) {

    /* compose file names */
    if (ComposeFileName(argv[varArgIdx[fileIdx]],0,oriPath,oriExt,oriFileName,
                        STRLEN))
      CommonExit(1,"composed file name too long");
    if (outFileNameUsed) {
      if (ComposeFileName(outFileName,0,bitPath,bitExt,bitFileName,
                          STRLEN))
        CommonExit(1,"composed file name too long");
    }
    else
      if (ComposeFileName(argv[varArgIdx[fileIdx]],1,bitPath,bitExt,
                          bitFileName,STRLEN))
        CommonExit(1,"composed file name too long");

    /* encode file */
    if (mainDebugLevel == 1)
      fprintf(stderr,"encoding %s -> %s\n",oriFileName,bitFileName);
    if (mainDebugLevel >= 2)
      printf("encoding %s -> %s\n",oriFileName,bitFileName);

    if (Encode(oriFileName,
               bitFileName,
               codecMode,
               encPara,
               (codecMode[1]!=NULL)?2:1,
               regionStart,
               regionDurat,
               numChannelOut,
               fSampleOut))
      CommonWarning("error encoding audio file %s",oriFileName);

    fileIdx++;
  }
  
  CmdLineEvalFree(paraList);

  if (mainDebugLevel == 1)
    fprintf(stderr,"%s: finished\n",progName);
  if (mainDebugLevel >= 2)
    printf("%s: finished\n",progName);

  return 0;
}

/* end of mp4enc.c */

