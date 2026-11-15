/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

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
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "allHandles.h"

#include "common_m4a.h" /* common module */
#include "cmdline.h"    /* command line module */
#include "bitstream.h"  /* bit stream module */
#include "audio.h"      /* audio i/o module */
#include "mp4au.h"      /* frame work declarations */
#include "enc_par.h"    /* encoder cores ... */
#include "enc_lpc.h"
#include "enc_tf.h"
#include "fir_filt.h"   /* fir lowpass filter */
#include "tf_main.h"    /* TM9902 */
#include "plotmtv.h"
#include "mp4mux.h"
#ifdef EXT2PAR
#include "SscEnc.h"
#endif

/* ---------- declarations ---------- */

#define PROGVER "MPEG-4 Natural Audio Encoder (Edition 2001)   2003-08-27"
#define CVSID "$Id: mp4auenc.c,v 1.26 2003/08/27 23:15:59 ct1 Exp $"

/* the framework should be independent from files in the codec */
/* core directories    HP 970515*/
/* #include "tf_main.h" */
#ifndef MONO_CHAN
#define MONO_CHAN 0
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ---------- variables ---------- */
#ifdef DEBUGPLOT
extern int framePlot; /* global for debugging purposes */
extern int firstFrame;
#endif
static char *MP4ModeName[] = {MODENAME_UNDEF, 
                              MODENAME_PAR, 
                              MODENAME_LPC,
                              MODENAME_TF, 
                              MODENAME_HVXC
#ifdef EXT2PAR
                              ,MODENAME_SSC};
#else
                              };
#endif

/* command line module */

static char *codecMode;
static float bitRate;
static int varBitRate;
static int bitReservSize;
static int bitReservInit;
static int bitReservInitUsed;
static float regionStart;
static float regionDurat;
static int regionDuratUsed;
static char *encPara;
static char *outFileName;
static int outFileNameUsed;
static int numChannelOut;
static float fSampleOut;
static char *oriPath,*bitPath;
static int oriPathUsed,bitPathUsed;
static char *oriExt,*bitExt;
static int oriExtUsed,bitExtUsed;
static int noInfo;
static int noHeader;
static char *magicString;
static int bitBufSize;
static int mainDebugLevel;
static int audioDebugLevel;
static int bitDebugLevel;
static int cmdDebugLevel;
static int quantDebugLevel;
static int mp4OutFlag;
static int *varArgIdx;
#ifdef DEBUGPLOT
extern int framePlot; /* global for debugging purposes */
extern int firstFrame;
#endif
static ENC_FRAME_DATA encFrameData;
static OBJECT_DESCRIPTOR objDescr;

#ifdef DEBUGPLOT
int frame; /* global for debugging */
#else 
static  int frame;
#endif 

/* hack to make buffer control independent of -c -mp4ff  HP20010212 */
int PARusedNumBit;



static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<audio file(s) (- = stdin)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"m",&codecMode,"%s",MODENAME_HVXC,NULL,"codec mode (" MODENAME_LIST ")"},
  {"r",&bitRate,"%f","2000",NULL,"bit rate [bit/s]"},
  {"vr",&varBitRate,NULL,NULL,NULL,"enable variable bit rate"},
  {"b",&bitReservSize,"%d","0",NULL,"bit reservoir size [bit]"},
  {"bi",&bitReservInit,"%d",NULL,&bitReservInitUsed,
   "initial bit reservoir bits\n"
   "(dflt: bit reservoir size)"},
  {"c",&encPara,"%s","",NULL,
   "encoder parameter string"},
  {"ts",&regionStart,"%f","0",NULL,"start time of encoded region [sec]"},
  {"td",&regionDurat,"%f",NULL,&regionDuratUsed,
   "duration of encoded region [sec]\n"
   "(dflt: until end of file)"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,
   "output file name (- = stdout)"},
  {"n",&numChannelOut,"%d","0",NULL,
   "number of audio channels for bit stream header\n"
   "(0=as input file)"},
  {"s",&fSampleOut,"%f","0",NULL,
   "sampling frequency [Hz] for bit stream header\n"
   "(0=as input file)"},
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
  {"ni",&noInfo,NULL,NULL,NULL,"disable bit info string"},
  {"nh",&noHeader,NULL,NULL,NULL,"disable mp4 bit stream header"},
  {"ms",&magicString,"%s",MP4_MAGIC,NULL,"bit stream magic string"},
  {"bb",&bitBufSize,"%i","0",NULL,"bitstream buffer size [bit] (0=auto)"},
  {"mp4",&mp4OutFlag,NULL,NULL,NULL,
   "write MP4 container instead of raw bit stream\n"
   "(AAC LC only: -m tf -c \"-qc_aac\")"},
  {"d",&mainDebugLevel,"%i","0",NULL,"main debug level"},
  {"da",&audioDebugLevel,"%i","0",NULL,"audio file debug level"},
  {"db",&bitDebugLevel,"%i","0",NULL,"bit stream debug level"},
  {"dc",&cmdDebugLevel,"%i","0",NULL,"cmd line debug level"},
  {"dq",&quantDebugLevel,"%i","0",NULL,"quantizer  debug level"},
#ifdef DEBUGPLOT
  {"ps",&firstFrame,"%i","9999",NULL,"start plot at frame%"},
#endif
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

ntt_DATA  nttData;
TF_DATA   tfData;

/* SSC data */
#ifdef EXT2PAR
PROCESSINFO*  pPI;
ENCODERINFO*  pEI;
SSCBUNCH*     pSSCBunch;
CHUNK_LENGTHS ChunkLengths;
#endif

/* ---------- functions ---------- */

/* Encode() */
/* Encode audio file and generate bit stream. */
/* (This function evaluates the global xxxDebugLevel variables !!!) */

static int Encode ( 
                   char *audioFileName,            /* in: audio file name */
                   char *bitFileName,            /* in: bit stream file name */
                   char *codecMode,              /* in: codec mode string */
                   float bitRate,                /* in: bit rate [bit/sec] */
                   int varBitRate,               /* in: variable bit rate */
                   int bitReservSize,            /* in: bit reservoir size [bit] */
                   int bitReservInit,            /* in: initial bit reservoir bits */
                   char *encPara,                /* in: encoder parameter string */
                   char *info,                   /* in: info string for bit stream */
                   int noHeader,                 /* in: disable bit stream header */
                   char *magicString,            /* in: bit stream magic string */
                   float regionStart,            /* in: start time of region */
                   float regionDurat,            /* in: duration of region */
                   int numChannelOut,            /* in: number of channels (0 = as input) */
                   float fSampleOut,            /* in: sampling frequency (0 = as input) */
                   int mp4Out)                  /* in: write MP4 container */
                                /* returns: 0=OK  1=error */
{
  int mode;                     /* enum MP4Mode */
  float fSample=0;
  long fileNumSample;
  long totNumSample;
  int numChannel;
  int frameNumSample,delayNumSample;
  int frameMinNumBit,frameMaxNumBit;
  long fSampleLong,bitRateLong;
  HANDLE_BSBITBUFFER bitBuf;
  float **sampleBuf;
  HANDLE_BSBITSTREAM bitStream;
  AudioFile *audioFile;
  HANDLE_BSBITBUFFER bitHeader;
  FILE *tmpFile;
  int startupNumFrame,startupFrame;
  int numSample;
  int frameNumBit,frameAvailNumBit,usedNumBit;
  int headerNumBit;
  int padNumBit = 0;
  long totDataNumBit,totPadNumBit;
  int minUsedNumBit,maxUsedNumBit;
  int minPadNumBit,maxPadNumBit;
  int minReservNumBit,maxReservNumBit;
  long totReservNumBit;
  int ch,i;
  long startSample;
  long encNumSample;
  int numChannelBS;
  long fSampleLongBS;
  ENC_FRAME_DATA* frameData=NULL;

  FIR_FILT *lowpassFilt = NULL;
  int downsamplFac = 1;
  float *tmpBuff;

  int numLayer, frameNumBitLay, padBitsLay;

  int celp_sys_overhead = 0;
  int hvxc_sys_overhead = 0;
  int para_sys_overhead;

  /* init */
  if (mainDebugLevel >= 3) {
    printf("Encode:\n");
    printf("audioFileName=\"%s\"\n",audioFileName);
    printf("bitFileName=\"%s\"\n",bitFileName);
    printf("codecMode=\"%s\"\n",codecMode);
    printf("bitRate=%.3f\n",bitRate);
    printf("varBitRate=%d\n",varBitRate);
    printf("bitReservSize=%d\n",bitReservSize);
    printf("bitReservInit=%d\n",bitReservInit);
    printf("encPara=\"%s\"\n",encPara?encPara:"(NULL)");
    printf("info=\"%s\"\n",info);
    printf("noHeader=%d\n",noHeader);
    printf("magicString=\"%s\"\n",magicString);
    printf("regionStart=%.6f\n",regionStart);
    printf("regionDurat=%.6f\n",regionDurat);
  }

  BsInit(bitBufSize,bitDebugLevel,0);
  AudioInit(getenv(MP4_ORI_RAW_ENV),       /* headerless file format */
            0, /* for encoding, data format of the input file does not have to be specified */
            audioDebugLevel);

  mode = 0;
  do
    mode++;
  while (mode < MODE_NUM && strcmp(codecMode,MP4ModeName[mode]) != 0);
  if (mode >= MODE_NUM)
    CommonExit(1,"Encode: unknown codec mode %s",codecMode);

  /* check audio file */
  if ((tmpFile=fopen(audioFileName,"rb"))==NULL) {
    CommonWarning("Encode: error opening audio file %s",audioFileName);
    return 1;
  }
  fclose(tmpFile);

  /* open audio file */
  audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
                            &fileNumSample);
  if (audioFile==NULL)
    CommonExit(1,"Encode: error opening audio file %s "
               "(maybe unknown format)",
               audioFileName);

  startSample = (long)(regionStart*fSample+0.5);

  if (regionDurat >= 0)
    encNumSample = (long)(regionDurat*fSample+0.5);
  else
    if (fileNumSample < 0) /* unkown */
      encNumSample = -1;
    else
      encNumSample = max(0,fileNumSample-startSample);

  /* init encoder */
  bitHeader = BsAllocBuffer(BITHEADERBUFSIZE);
  bitRateLong = (long)(bitRate+0.5);
  fSampleLong = (long)(fSample+0.5);
  numChannelBS = (numChannelOut==0) ? numChannel : numChannelOut;
  fSampleLongBS = (fSampleOut==0) ? fSampleLong : (long)(fSampleOut+0.5);

  if ((strstr(encPara, "-aac_sys") != NULL)||
      (strstr(encPara, "-aac_sys_bsac") != NULL)||
      (strstr(encPara, "-tvq_sys") != NULL)||
      (strstr(encPara, "-mp4ff") != NULL)) {
    frameData= &encFrameData;
    frameData->od= &objDescr;
  }

  switch (mode) {
  case MODE_PAR:
    EncParInit(numChannel,fSample,bitRate,encPara,
               &frameNumSample,&delayNumSample,bitHeader,
	       frameData,mainDebugLevel);
    break;
  case MODE_HVXC:
    EncHvxcInit(numChannel,fSample,bitRate,encPara,
                &frameNumSample,&delayNumSample,bitHeader,
                frameData,mainDebugLevel);      /* AI 990616 */
    break;
  case MODE_LPC:
    EncLpcInit(numChannel,fSample,bitRate,encPara, varBitRate,
               &frameNumSample,&delayNumSample,bitHeader,frameData,mainDebugLevel);

#if 0   /* what is this???   HP 990120 990422 */
    frameNumBit = (int)(bitRate*frameNumSample/fSample+0.5);
    bitRate = (float) (frameNumBit) * fSample / (float) (frameNumSample);
    bitRateLong = (long) (bitRate + 0.5F);
#endif

    break;
  case MODE_TF:
    EncTfInit(numChannel,fSample,bitRate,encPara,quantDebugLevel,
              &frameNumSample,&delayNumSample,bitHeader,frameData, 
              &tfData, &nttData,mainDebugLevel);
    break;
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
  }


  frameNumBit = (int)(bitRate*frameNumSample/fSample+0.5);
  /* variable bit rate: minimum 8 bit/frame (1 byte) */
  /*                    to allow end_of_bitstream detection in decoder */
  frameMinNumBit = varBitRate ? 8 : frameNumBit;
  frameMaxNumBit = frameNumBit+bitReservSize;

  if ( (mode == MODE_LPC) && (strstr(encPara, "-mp4ff") != NULL)) {
    numLayer = objDescr.streamCount.value;
    celp_sys_overhead = 0; /* overhead for flexMux transport */
    for ( i = 0; i < numLayer; i++ ) {
      frameNumBitLay = (int)(objDescr.ESDescriptor[i]->DecConfigDescr.avgBitrate.value * (float) frameNumSample / fSample + 0.5);
      padBitsLay = 8 - (frameNumBitLay % 8);
      if (padBitsLay == 8) padBitsLay = 0;
      celp_sys_overhead += (24 + padBitsLay);
    }
    frameMaxNumBit += celp_sys_overhead;
  }

  if ( (mode == MODE_HVXC) && (strstr(encPara, "-mp4ff") != NULL)) {
    numLayer = objDescr.streamCount.value;
    hvxc_sys_overhead = 0; /* overhead for flexMux transport */
    for ( i = 0; i < numLayer; i++ ) {
      frameNumBitLay = (int)(objDescr.ESDescriptor[i]->DecConfigDescr.avgBitrate.value * (float) frameNumSample / fSample + 0.5);
      padBitsLay = 8 - (frameNumBitLay % 8);
      if (padBitsLay == 8) padBitsLay = 0;
      hvxc_sys_overhead += (24 + padBitsLay);
    }
    frameMaxNumBit += hvxc_sys_overhead;
  }

  para_sys_overhead = 0;
  if ( (mode == MODE_PAR) && (strstr(encPara, "-mp4ff") != NULL)) {
    numLayer = objDescr.streamCount.value;
    para_sys_overhead = 0; /* overhead for flexMux transport */
    for ( i = 0; i < numLayer; i++ ) {
      frameNumBitLay = (int)(objDescr.ESDescriptor[i]->DecConfigDescr.avgBitrate.value * (float) frameNumSample / fSample + 0.5);
      padBitsLay = 8 - (frameNumBitLay % 8);
      if (padBitsLay == 8) padBitsLay = 0;
      para_sys_overhead += (24 + padBitsLay); /* HP20001010 */
      para_sys_overhead += 8; /* hack for ESC bit->byte padding, should never hurt  HP20010212 */
      if (frameNumBitLay > 255*8)
	/* HP20010124   max 255 byte/AU */
	para_sys_overhead += 24*((frameNumBitLay-1)%(255*8)); 
    }
    frameMaxNumBit += para_sys_overhead;
  }

  if (mainDebugLevel >= 3) {
    printf("mode=%d\n",mode);
    printf("fSample=%.3f Hz  (int=%ld)\n",fSample,fSampleLong);
    printf("bitRate=%.3f bit/sec  (int=%ld)\n",bitRate,bitRateLong);
    printf("bitReservSize=%d bit  (%.6f sec)\n",
           bitReservSize,bitReservSize/bitRate);
    printf("bitReservInit=%d bit\n",bitReservInit);
    printf("frameNumSample=%d  (%.6f sec/frame)\n",
           frameNumSample,frameNumSample/fSample);
    printf("delayNumSample=%d  (%.6f sec)\n",
           delayNumSample,delayNumSample/fSample);
    printf("frameNumBit=%d\n",frameNumBit);
    printf("frameMinNumBit=%d\n",frameMinNumBit);
    printf("frameMaxNumBit=%d\n",frameMaxNumBit);
    printf("bitHeaderNumBit=%ld\n",BsBufferNumBit(bitHeader));
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
    bitBuf = BsAllocBuffer(frameMaxNumBit);
  }
  
  if ((sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((sampleBuf[ch]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
  if ((tmpBuff=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
    CommonExit(1,"Encode: memory allocation error");

  /* if we have aac_raw , we should treat it, as with no header, 
     if we have scaleable aac the header is will be written in
     aacScaleableEncode*/
  if ((strstr(encPara, "-aac_raw") != NULL)||
      (strstr(encPara, "-aac_sca") != NULL)
#ifdef EXT2PAR
      || (mode == MODE_SSC)
#endif
#ifdef VERSION2
      || (strstr(encPara, "-aac_ld") != NULL)
#endif
      )
    noHeader=1;
  /* T.Ishikawa 980525 */
  if ((strstr(encPara, "-aac_sys") != NULL) || 
      (strstr(encPara, "-aac_sys_bsac") != NULL) || 
      (strstr(encPara, "-tvq_sys") != NULL) ||
      (strstr(encPara, "-mp4ff") != NULL))
    noHeader=0;


  /* MP4 container output: only plain AAC LC supported */
  if (mp4Out) {
    if (mode != MODE_TF ||
        strstr(encPara,"-mp4ff") || strstr(encPara,"-aac_sys") ||
        strstr(encPara,"-tvq_sys") || strstr(encPara,"-aac_sca") ||
        strstr(encPara,"-aac_ld") || strstr(encPara,"-aac_bsac") ||
        strstr(encPara,"-qc_ntt") || strstr(encPara,"-qc_uer") ||
        strstr(encPara,"-qc_src") || strstr(encPara,"-qc_mdct") ||
        !strstr(encPara,"-qc_aac"))
      CommonExit(1,"Encode: -mp4 supports only '-m tf' with '-qc_aac' (AAC LC)");
  }

  /* open bit stream file */
  if (mp4Out) {
    int flFlag = strstr(encPara,"-qc_aac_960") ? 1 : 0;
    if (MP4MuxOpen(bitFileName,(unsigned long)fSample,numChannel,
                   (unsigned long)bitRateLong,frameNumSample,flFlag))
      CommonExit(1,"Encode: error opening MP4 file %s",bitFileName);
    BsFreeBuffer(bitHeader);
    bitStream = NULL;
  } else {
  if (!noHeader)
    bitStream = BsOpenFileWrite(bitFileName,magicString,info);
  else
    bitStream = BsOpenFileWrite(bitFileName,NULL, NULL);

  if (bitStream==NULL)
    CommonExit(1,"Encode: error opening bit stream file %s",bitFileName);

  /* write bit stream header */
  if ((strstr(encPara, "-aac_sys") == NULL) && 
      (strstr(encPara, "-aac_sys_bsac") == NULL) && 
      (strstr(encPara,"-tvq_sys") == NULL) &&
      (strstr(encPara, "-mp4ff") == NULL)) {
    if (!noHeader)
      if (BsPutBit(bitStream,MP4_BS_VERSION,16) ||
          BsPutBit(bitStream,numChannelBS,8) ||
          BsPutBit(bitStream,fSampleLongBS,32) ||
          BsPutBit(bitStream,bitRateLong,32) ||
          BsPutBit(bitStream,frameNumBit,16) ||
          BsPutBit(bitStream,frameMinNumBit,16) ||
          BsPutBit(bitStream,bitReservSize,16) ||
          BsPutBit(bitStream,bitReservInit,16) ||
          BsPutBit(bitStream,mode,8) ||
          BsPutBit(bitStream,BsBufferNumBit(bitHeader),16))
        CommonExit(1,"Encode: error writing bit stream header (frame)");
    if (BsPutBuffer(bitStream,bitHeader))
      CommonExit(1,"Encode: error writing bit stream header (core)");
    BsFreeBuffer(bitHeader);
    
    headerNumBit = BsCurrentBit(bitStream);
  } else {
    /* write object descriptor  length */
    int length= BsBufferNumBit(bitHeader)/8;
    int align = 8 - BsBufferNumBit(bitHeader) % 8;
    if (align == 8) align = 0;
    if (align != 0) {
      length += 1;
    }
    BsPutBit(bitStream,length,32);
    if (BsPutBuffer(bitStream,bitHeader))
      CommonExit(1,"Encode: error writing bit stream header (core)");
    BsPutBit(bitStream,0,align);

    BsFreeBuffer(bitHeader);
    headerNumBit = BsCurrentBit(bitStream);
  }
  if (mainDebugLevel >= 3)
    printf("headerNumBit=%d\n",headerNumBit);
  }

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
  totPadNumBit = 0;
  frameAvailNumBit = bitReservInit;
  minUsedNumBit = minPadNumBit = minReservNumBit = frameMaxNumBit;
  maxUsedNumBit = maxPadNumBit = maxReservNumBit = 0;
  totReservNumBit = 0;

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
      printf("frame %4d\n",frame);

    /* check for startup frame */
    startupFrame = frame < 0;

    /* read audio file */
    numSample = AudioReadData(audioFile,sampleBuf,frameNumSample);
    totNumSample += numSample;
    if (numSample != frameNumSample && encNumSample == -1)
      encNumSample = totNumSample;
      

    /* encode one frame */
    if (!startupFrame) {
      /* variable bit rate: don't exceed bit reservoir size */
      if (frameAvailNumBit > bitReservSize)
        frameAvailNumBit = bitReservSize;
      
      if (frameData!=NULL) {
        /* T.Ishikawa 980701 */
        if(strstr(encPara,"-tvq_sys")) {
          float Fflexnum;
          int flexnum;
          Fflexnum = (float)(frameNumBit)/(float)(254*8)+1.0f;
          flexnum = (int)(frameNumBit/(254*8)+1);
          /*fprintf(stderr," %f %d\n",Fflexnum,flexnum);*/
          
          if(Fflexnum>(float)flexnum) {
            frameAvailNumBit += frameNumBit+ 24*(flexnum+1);
          }
          else {
            frameAvailNumBit += frameNumBit+ 24*flexnum;
          }
          /*fprintf(stderr,"frameAvailNumBit %d , frameNumBit %d\n",
            frameAvailNumBit,frameNumBit);*/
        }
	else if ( (mode == MODE_PAR) && (strstr(encPara, "-mp4ff") != NULL)) {
	  frameAvailNumBit += frameNumBit + para_sys_overhead;
	}
        else if ( (mode == MODE_HVXC) && (strstr(encPara, "-mp4ff") != NULL)) {
          frameAvailNumBit += frameNumBit + hvxc_sys_overhead;
        }
        else if ( (mode == MODE_LPC) && (strstr(encPara, "-mp4ff") != NULL)) {
          frameAvailNumBit += frameNumBit + celp_sys_overhead;
        }
        else {
          frameAvailNumBit += frameNumBit-(18* (frameNumBit/(250*8)+1) + 7);
          /* minus flexmux overhead:
             18 bits per flexmux packet plus max 7 align bits*/
        }
        
      }
      else {
        frameAvailNumBit += frameNumBit;
      }
      
      if (mainDebugLevel >= 5)
        printf("frameAvailNumBit=%d\n",frameAvailNumBit);
    }

    switch (mode) {
    case MODE_PAR:
      EncParFrame(sampleBuf,
                  startupFrame ? (HANDLE_BSBITBUFFER )NULL : bitBuf,
                  startupFrame ? 0 : frameAvailNumBit-para_sys_overhead,
                  frameNumBit,frameMaxNumBit-para_sys_overhead,
		  frameData);
      break;
    case MODE_HVXC:
      EncHvxcFrame(sampleBuf,
                   startupFrame ? (HANDLE_BSBITBUFFER )NULL : bitBuf,
                   startupFrame ? 0 : frameAvailNumBit-hvxc_sys_overhead,
                   frameNumBit,frameMaxNumBit-hvxc_sys_overhead,
                   frameData);
      break;
    case MODE_LPC:
      EncLpcFrame(sampleBuf,
                  startupFrame ? (HANDLE_BSBITBUFFER )NULL : bitBuf,
                  frameData);
      break;
    case MODE_TF:
      EncTfFrame(sampleBuf,
                 startupFrame ? (HANDLE_BSBITBUFFER )NULL : bitBuf,
                 startupFrame ? 0 : frameAvailNumBit,
                 frameNumBit,frameMaxNumBit,bitRateLong,fSampleLong,
                 frameData, &tfData, &nttData);
      break;
#ifdef EXT2PAR
    case MODE_SSC:
      EncSSCFrame (sampleBuf,bitBuf,pEI,pPI,&pSSCBunch);
    break;
#endif
    }

    if (!startupFrame) {
      usedNumBit = BsBufferNumBit(bitBuf);
      /* hack to make buffer control independent of -c -mp4ff  HP20010212 */
      if ( (mode == MODE_PAR) && (strstr(encPara, "-mp4ff") != NULL))
        usedNumBit = PARusedNumBit + para_sys_overhead;

      if (mainDebugLevel >= 5)
        printf("usedNumBit=%d\n",usedNumBit);

#ifdef EXT2PAR
      /* No bitrate control for SSC */
      /* force available number of bits equal to bits used */
      if (mode == MODE_SSC)
      {
        frameAvailNumBit = usedNumBit;
      }
#endif

      /* write bit stream */
      if (usedNumBit > frameAvailNumBit)
        CommonExit(1,"Encode: more bits used than available in frame+buffer");
      if (mp4Out) {
        if (MP4MuxAddFrame(bitBuf))
          CommonExit(1,"Encode: error writing MP4 sample");
      } else if (BsPutBuffer(bitStream,bitBuf))
        CommonExit(1,"Encode: error writing bit stream data");
      frameAvailNumBit -= usedNumBit;

#ifdef EXT2PAR
      if (!mp4Out && (frameData==NULL) && (mode != MODE_SSC)) /* Avoid padding for SSC */
#else
      if (frameData==NULL)
#endif
      {
        /* write padding bits */
        padNumBit = 0;
        if (frameAvailNumBit-frameNumBit+frameMinNumBit > bitReservSize) {
          padNumBit = frameAvailNumBit-frameNumBit+
            frameMinNumBit-bitReservSize;
          
          if (mainDebugLevel >= 5)
            printf("padNumBit=%d\n",padNumBit);
          
          for (i=0; i<padNumBit; i++)
            if (BsPutBit(bitStream,0,1))
              CommonExit(1,"Encode: error writing bit stream padding bits");
          frameAvailNumBit -= padNumBit;
          totPadNumBit += padNumBit;
        }
      }

      if (minUsedNumBit > usedNumBit)
        minUsedNumBit = usedNumBit;
      if (maxUsedNumBit < usedNumBit)
        maxUsedNumBit = usedNumBit;
      if (minPadNumBit > padNumBit)
        minPadNumBit = padNumBit;
      if (maxPadNumBit < padNumBit)
        maxPadNumBit = padNumBit;
      if (minReservNumBit > frameAvailNumBit)
        minReservNumBit = frameAvailNumBit;
      if (maxReservNumBit < frameAvailNumBit)
        maxReservNumBit = frameAvailNumBit;
      totReservNumBit += frameAvailNumBit;
    
    }
    frame++;
#ifdef DEBUGPLOT    
    framePlot++;
    plotDisplay(0);
#endif
  } while (encNumSample < 0 || frame*(long)frameNumSample < encNumSample);

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

  if (mainDebugLevel == 1)
    fprintf(stderr," \n");
  if (mainDebugLevel >= 2 && mainDebugLevel <= 3)
    printf(" \n");

  totDataNumBit = mp4Out ? 0 : BsCurrentBit(bitStream)-headerNumBit-totPadNumBit;


  if (mainDebugLevel >= 3) {
    printf("totNumFrame=%d\n",frame);
    printf("encNumSample=%ld  (%.3f sec  %.3f frames)\n",
           encNumSample,encNumSample/fSample,
           encNumSample/(float)frameNumSample);
    printf("totNumSample=%ld\n",totNumSample);
    printf("totNumBit=%ld\n",mp4Out ? 0L : BsCurrentBit(bitStream));
    printf("totDataNumBit=%ld  (%.3f bit/frame  %.3f bit/sec)\n",
           totDataNumBit,totDataNumBit/(float)frame,
           totDataNumBit/(float)frame*fSample/frameNumSample);
    printf("totPadNumBit=%ld\n",totPadNumBit);
    printf("lastPadNumBit=%d\n",frameAvailNumBit);
    printf("minUsedNumBit=%d\n",minUsedNumBit);
    printf("maxUsedNumBit=%d\n",maxUsedNumBit);
    printf("minPadNumBit=%d\n",minPadNumBit);
    printf("maxPadNumBit=%d\n",maxPadNumBit);
    printf("minReservNumBit=%d\n",minReservNumBit);
    printf("maxReservNumBit=%d\n",maxReservNumBit);
    printf("avgReservNumBit=%.1f\n",totReservNumBit/(float)frame);
  }

  /* free encoder memory */
  switch (mode) {
  case MODE_PAR:
    EncParFree();
    break;
  case MODE_HVXC:
    EncHvxcFree();
    break;
  case MODE_LPC:
    EncLpcFree();
    break;
  case MODE_TF:
    EncTfFree();
    break;
#ifdef EXT2PAR
  case MODE_SSC:
    SSC_Encoder_Close(pPI,&ChunkLengths);
    SSC_WriteChunkLengths(ChunkLengths, bitStream);
    SSC_FreeEncoderInfo(pEI);
    break;
#endif
  }

  /* close audio file */
  AudioClose(audioFile);
  
  /* close bit stream / MP4 file */
  if (mp4Out) {
    if (MP4MuxClose())
      CommonExit(1,"Encode: error closing MP4 file");
  } else if (BsClose(bitStream))
    CommonExit(1,"Encode: error closing bit stream file");

  /* free buffers */
  if (numChannel>1)
    for (ch=0; ch<numChannel; ch++)
      free(sampleBuf[ch]);
  free(sampleBuf);
  free(tmpBuff);
  BsFreeBuffer(bitBuf);

  return 0;
}



/* ---------- main ---------- */

int main (int argc, char *argv[])
{
  char *progName = "<no program name>";
  int result;
  char oriFileName[STRLEN],bitFileName[STRLEN];
  char infoDate[STRLEN];
  int i,j,len;
  int fileIdx;
  char *info,*infoTail;
  time_t curTime;

  /* evaluate command line  */
  CmdLineInit(0);
  result = CmdLineEval(argc,argv,paraList,switchList,1,&progName);
  if (result) {
    if (result==1) {
      printf("%s: %s\n",progName,PROGVER);
      CmdLineHelp(progName,paraList,switchList,stdout);
      EncParInfo(stdout);
      EncLpcInfo(stdout);
      EncTfInfo(stdout);
#ifdef EXT2PAR
      EncSSCInfo(stdout);
#endif
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
    printf("%s\n",EncParInfo(NULL));
    printf("%s\n",EncLpcInfo(NULL));
    printf("%s\n",EncTfInfo(NULL));
#ifdef EXT2PAR
    printf("%s\n",EncSSCInfo(NULL));
#endif
  }

  CmdLineInit(cmdDebugLevel);

  /* calc variable default values */
  if (!bitReservInitUsed)
    bitReservInit = bitReservSize;
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
  if (bitRate <= 0)
    CommonExit(1,"bit rate <= 0");
  if (bitReservSize < 0)
    CommonExit(1,"bit reservoir size < 0");
  if (bitReservInit < 0)
    CommonExit(1,"bit reservoir initial bits < 0");
  if (bitReservInit > bitReservSize)
    CommonExit(1,"bit reservoir initial bits > size");
  if (regionDuratUsed && regionDurat < 0)
    CommonExit(1,"duration of region < 0");
  if (outFileNameUsed && varArgIdx[0]>=0 && varArgIdx[1]>=0)
    CommonExit(1,"only one input file allowed when using -o");
  if (varArgIdx[0]<0)
    CommonExit(1,"no input file specified");
  
  if( strstr( encPara, "-aac_sca" ) ) {
    if (bitReservSize<6000)
      bitReservSize=6000 ;
    bitReservInit=0;    
    varBitRate=1;
  } 

  if (!bitExtUsed && strstr(encPara,"-mp4ff"))
    bitExt = ".fl4";
  if (mp4OutFlag && !bitExtUsed)
    bitExt = ".mp4";

  /* MP4 samples must be raw AAC frames (no syncword/leninfo transport) */
  if (mp4OutFlag &&
      !strstr(encPara,"-aac_raw") && !strstr(encPara,"-raw_tstream")) {
    char *tmpPara = (char*)malloc(strlen(encPara)+32);
    if (tmpPara==NULL)
      CommonExit(1,"memory allocation error");
    strcpy(tmpPara,encPara);
    strcat(tmpPara," -raw_tstream");
    encPara = tmpPara;
  }

  /* MP4/AAC-LC: default to STATIC_LONG windows (STATIC_SHORT default is ~17x slower) */
  if (mp4OutFlag && !strstr(encPara,"-wsm")) {
    char *tmpPara = (char*)malloc(strlen(encPara)+16);
    if (tmpPara==NULL)
      CommonExit(1,"memory allocation error");
    strcpy(tmpPara,encPara);
    strcat(tmpPara," -wsm 0");
    encPara = tmpPara;
  }

#ifdef EXT2PAR
  if (!strcmp(codecMode,"ssc"))
  {
    bitRate       = 0;      /* Set to 0, because SSC Encoding is not bitrate controlled yet */
    bitReservSize = 0;
    bitReservInit = 0;
    varBitRate    = 1;      /* Set to 1, because SSC Encoding is not bitrate controlled yet */
    bitExt        = ".ssc"; /* Force ssc file format */
  }
#endif

  /* generate info string for bit stream */
  len = 1;
  curTime = time((time_t*)NULL);
  len += strftime(infoDate,STRLEN,"%Y/%m/%d %H:%M:%S UTC",gmtime(&curTime));
  len += strlen(PROGVER);
  j = 0;
  for (i=0; i<argc; i++)
    if (varArgIdx[j] == i)
      j++;
    else
      len += strlen(argv[i])+1;
  len += 2*STRLEN;
  len += 5*7+1;
  if ((info=(char*)malloc(len))==NULL)
    CommonExit(1,"memory allocation error");
  strcpy(info,"\n");
  strcat(info,"date: ");
  strcat(info,infoDate);            
  strcat(info,"\n");
  strcat(info,"prog: ");
  strcat(info,PROGVER);     
  strcat(info,"\n");
  strcat(info,"para:");
  j = 0;
  for (i=0; i<argc; i++) {
    if (varArgIdx[j] == i)
      j++;
    else {
      strcat(info," ");
      strcat(info,argv[i]);
    }
  }
  strcat(info,"\n");
  infoTail = info+strlen(info);

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

    /* complete info string */
    *infoTail = '\0';
    strcat(infoTail,"ori: ");
    strcat(infoTail,oriFileName);
    strcat(infoTail,"\n");
    strcat(infoTail,"bit: ");
    strcat(infoTail,bitFileName);
    strcat(infoTail,"\n");

    /* encode file */
    if (mainDebugLevel == 1)
      fprintf(stderr,"encoding %s -> %s\n",oriFileName,bitFileName);
    if (mainDebugLevel >= 2)
      printf("encoding %s -> %s\n",oriFileName,bitFileName);

    if (Encode(oriFileName,
               bitFileName,
               codecMode,
               bitRate,
               varBitRate,
               bitReservSize,
               bitReservInit,
               encPara,
               noInfo ? (char*)"" : info,
               noHeader,
               magicString,
               regionStart,
               regionDurat,
               numChannelOut,
               fSampleOut,
               mp4OutFlag))
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

