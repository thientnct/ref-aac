/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Heiko Purnhagen (University of Hannover)

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
Copyright (c) ISO/IEC 1996.



Source file: encoder.c (formerly mp4enc.c)

$Id: encoder.c,v 1.7 1998/07/30 15:51:15 lueck Exp $

Required libraries:
libtsp.a		AFsp audio file library

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module
enc_tf.o		encoder core (tf-based)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
CL    Chuck Lueck, TI <lueck@ti.com>
RG    Ralf Geiger, FhG/IIS
MCF	  Matt Fellers, Dolby Laboratories, Inc.

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
07-jun-97   CL    Renamed this file (mp4enc.c) to encoder.c for 
                  use in AAC technical report encoder.  Removed
                  support for lpc and parametric codecs.  
07-apr-98   RG    added switch -lf to indicate presence of LFE channel   
27-may-98   MCF   Added File I/O for independent coupling channel support.
24-jul-98   MCF   Added support for dynamic range control. 
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <libtsp.h>	/* AFsp audio file library */

#include "common.h"	/* common module */
#include "cmdline.h"	/* command line module */
#include "bitstream.h"	/* bit stream module */
#include "audio.h"	/* audio i/o module */

#include "aac_qc.h"
#include "mp4.h"	/* frame work declarations */
#include "interface.h"
#include "drc.h"
#include "enc.h"	/* encoder cores */

/* ---------- declarations ---------- */

#define CVSID "$Id: encoder.c,v 1.7 1998/07/30 15:51:15 lueck Exp $"

/* ---------- variables ---------- */

/* command line module */
static float bitRate;				   
static int varBitRate;
static int lfePresent;                    
static int pnsSfbStart;                 
static int bitReservSize;
static int bitReservSizeUsed;
static int bitReservInit;				  
static int bitReservInitUsed;
static float regionStart;
static float regionDurat;
static int regionDuratUsed;
static char *encPara;
static char *outFileName;
static char *cplFileName;
static int outFileNameUsed;
static int cplFileNameUsed;
static char *oriPath,*bitPath;
static int oriPathUsed,bitPathUsed;
static char *oriExt,*bitExt;
static int oriExtUsed;
static int noInfo;
static int noHeader;
static int cplenabl = 0;
static int isdisable = 0;
static int msdisable = 0;
static int tnsdisable = 0;
#ifdef CT_SBR
static int sbrenable = 0;
#ifdef PARAMETRICSTEREO
static int sbrpsenable = 0;
#endif
#endif
static char *magicString;
static int mainDebugLevel,audioDebugLevel,bitDebugLevel,cmdDebugLevel,quantDebugLevel;
static int *varArgIdx;
static int totalNumChans;
static int dynrng_enabl;
static double dynrng_import[DR_READLEN];
static short dynrng_shrtblk[MAX_SHORT_WINDOWS];
static short dynrng_sgn[MAX_SHORT_WINDOWS];

#ifdef PLOTMTV
int frame; /* global for debugging */
#else 
static  int frame;
#endif 

static double cplmix[MAX_TIME_CHANNELS] = {0.7, 0.7, 0.7, 0.7, 0.7, 0.7};
static int numCplChannel;
static int cplMixMode = 0;
static int cplMixModeUsed = 0;

static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<audio file(s)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
/*{switchName  ,argument        ,format,defaultValue,usedFlag          ,help}*/
  {"h"         ,NULL            ,NULL  ,NULL        ,NULL              ,"print help"},
  {"r"         ,&bitRate        ,"%f"  ,"64000"     ,NULL              ,"bit rate [bit/s]"},
  {"vr"        ,&varBitRate     ,NULL  ,NULL        ,NULL              ,"enable variable bit rate"},
  {"lf"        ,&lfePresent     ,NULL  ,NULL        ,NULL              ,"LFE channel present"},             
  {"b"         ,&bitReservSize  ,"%d"  ,NULL        ,&bitReservSizeUsed,"bit reservoir size [bit]"},
  {"bi"        ,&bitReservInit  ,"%d"  ,NULL        ,&bitReservInitUsed,"initial bit reservoir bits\n"
                                                                        "(dflt: bit reservoir size)"},
  {"c"         ,&encPara        ,"%s"  ,""          ,NULL              ,"encoder parameter string"},
  {"dr"        ,&dynrng_enabl   ,NULL  ,NULL        ,NULL              ,"enable dynamic range control metadata"},
  {"cp"        ,&cplFileName    ,"%s"  ,NULL        ,&cplFileNameUsed  ,"Independent couple chan file name, \n"
                                                                       "(must be same samplerate as input channels)"},
  {"cpn"       ,&cplMixMode     ,"%d"  ,0           ,&cplMixModeUsed   ,"Independent couple mix mode:\n"
                                                                        "0:  Cpl chan mixed to all chans w. 6dB atten. (default)\n"
                                                                        "1:  Cpl chan mixed to left and right w. 3dB atten.\n"
                                                                        "2:  Cpl chan mixed to center (or l/r if no center) \n"
                                                                        "    w. 0dB atten."},
#ifdef MPEG4V1
  {"pns"       ,&pnsSfbStart    ,"%d"  ,"1000"      ,NULL              ,"start band for pns"},
#endif
  {"ts"        ,&regionStart    ,"%f"  ,"0"         ,NULL              ,"start time of encoded region [sec]"},
  {"td"        ,&regionDurat    ,"%f"  ,NULL        ,&regionDuratUsed  ,"duration of encoded region [sec]\n"
                                                                        "(dflt: until end of file)"},
  {"o"         ,&outFileName    ,"%s"  ,NULL        ,&outFileNameUsed  ,"output file name"},
  {"po"        ,&oriPath        ,"%s"  ,NULL        ,&oriPathUsed      ,"default path original audio files (dflt: $" MP4_ORI_PATH_ENV ")"},
  {"pb"        ,&bitPath        ,"%s"  ,NULL        ,&bitPathUsed      ,"default path bit stream files (dflt: $" MP4_BIT_PATH_ENV ")"},
  {"eo"        ,&oriExt         ,"%s"  ,NULL        ,&oriExtUsed       ,"original audio file extension\n"
                                                                        "supported file formats: .au, .snd, .wav, .aif, .raw\n"
                                                                        "(dflt: $" MP4_ORI_FMT_ENV " / " MP4_ORI_EXT ")\n"
                                                                        "(raw format: $" MP4_ORI_RAW_ENV ")"},
  {"eb"        ,&bitExt         ,"%s"  ,".aac"      ,NULL              ,"bit stream file extension"},
  {"ni"        ,&noInfo         ,NULL  ,NULL        ,NULL              ,"disable bit info string"},
/*{"nh"        ,&noHeader       ,NULL  ,NULL        ,NULL              ,"disable mp4 bit stream header"},*/
/*{"ms"        ,&magicString    ,"%s"  ,MP4_MAGIC   ,NULL              ,"bit stream magic string"},*/
  {"nis"       ,&isdisable      ,NULL  ,NULL        ,NULL              ,"disable intensity stereo (default on)"}, 
  {"nms"       ,&msdisable      ,NULL  ,NULL        ,NULL              ,"disable ms stereo (default on)"}, 
  {"ntns"      ,&tnsdisable     ,NULL  ,NULL        ,NULL              ,"disable tns (default on)"}, 
  {"d"         ,&mainDebugLevel ,"%i"  ,"0"         ,NULL              ,"main debug level"},
  {"da"        ,&audioDebugLevel,"%i"  ,"0"         ,NULL              ,"audio file debug level"},
  {"db"        ,&bitDebugLevel  ,"%i"  ,"0"         ,NULL              ,"bit stream debug level"},
  {"dc"        ,&cmdDebugLevel  ,"%i"  ,"0"         ,NULL              ,"cmd line debug level"},
  {"dq"        ,&quantDebugLevel,"%i"  ,"0"         ,NULL              ,"quantizer  debug level"},
#ifdef CT_SBR
  {"sbr"       ,&sbrenable      ,NULL  ,NULL        ,NULL              ,"enable SBR (default off)"}, 
#ifdef PARAMETRICSTEREO
  {"ps"       ,&sbrpsenable      ,NULL  ,NULL        ,NULL              ,"enable Parametric stereo for SBR (default off)"}, 
#endif
#endif
  {NULL        ,NULL            ,NULL  ,NULL        ,NULL              ,NULL}
};

/* ---------- functions ---------- */

#ifdef CT_SBR

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
#endif

/* Encode() */
/* Encode audio file and generate bit stream. */
/* (This function evaluates the global xxxDebugLevel variables !!!) */

static int Encode (
  char *audioFileName,		/* in: audio file name */
  char *bitFileName,		/* in: bit stream file name */
  float bitRate,		/* in: bit rate [bit/sec] */
  int varBitRate,		/* in: variable bit rate */
  int lfePresent,               /* in: lfe channel present */
  int bitReservSize,		/* in: bit reservoir size [bit] */
  int bitReservInit,            /* in: number of bits in reservoir */
  char *encPara,		/* in: encoder parameter string */
  const char *info,		/* in: info string for bit stream */
  int noHeader,			/* in: disable bit stream header */
  char *magicString,		/* in: bit stream magic string */
  char *cplFileName,		/* in: name of coupling channel */
  float regionStart,		/* in: start time of region */
  float regionDurat)		/* in: duration of region */
				/* returns: 0=OK  1=error */
{ 
  float fSample;
  float cplfSample;
  long fileNumSample;
  long cplFileNumSample;
  int cplNumSample, samp;
  long totNumSample;
  int numChannel;
  int frameNumSample,delayNumSample;

  int frameMinNumBit,frameMaxNumBit;
  long fSampleLong,bitRateLong;
  BsBitBuffer *bitBuf;
  float **sampleBuf;
  float *cplSampleBuf[MAX_CPL_CHANNELS];
  BsBitStream *bitStream;
  AudioFile *audioFile;
  AudioFile *cplAudioFile = NULL;
  BsBitBuffer *bitHeader;
  FILE *tmpFile;
  int startupNumFrame,startupFrame;
  int numSample = 0;
  int frameNumBit,frameAvailNumBit,usedNumBit;
  int minBitsNoOverflow;
  int headerNumBit;
  int padNumBit;
  long totDataNumBit,totPadNumBit;
  int minUsedNumBit,maxUsedNumBit;
  int minPadNumBit,maxPadNumBit;
  int minReservNumBit,maxReservNumBit;
  long totReservNumBit;
  int ch,i;
  long startSample;
  long encNumSample;
  DRC_INTF dvars_inpt;
  char *dr_metadata_name = "metadata.txt";
  FILE *dr_metadata = NULL;
#ifdef CT_SBR
  int downSampleRatio = 1;
  int NcoefRS = 0;
  float *hRS = NULL;
  float **sampleBufRS = NULL;
  float **sampleBufRSold = NULL;
  float *tmpBufRS = NULL;
  float lowPassFreq = 0;
#endif

  /* init */
  if (mainDebugLevel >= 3) {
    printf("Encode:\n");
    printf("audioFileName=\"%s\"\n",audioFileName);
    printf("bitFileName=\"%s\"\n",bitFileName);
    printf("bitRate=%.3f\n",bitRate);
    printf("varBitRate=%d\n",varBitRate);
    printf("lfePresent=%d\n",lfePresent);     
    printf("bitReservSize=%d\n",bitReservSize);
    printf("bitReservInit=%d\n",bitReservInit);
    printf("encPara=\"%s\"\n",encPara?encPara:"(NULL)");
    printf("info=\"%s\"\n",info);
    printf("noHeader=%d\n",noHeader);
    /* printf("magicString=\"%s\"\n",magicString); */
    printf("regionStart=%.6f\n",regionStart);
    printf("regionDurat=%.6f\n",regionDurat);
  }

  BsInit(0,bitDebugLevel);
  AudioInit(getenv(MP4_ORI_RAW_ENV),	   /* headerless file format */
	    audioDebugLevel);

  /* check audio file */
  if ((tmpFile=fopen(audioFileName,"rb"))==NULL) {
    CommonWarning("Encode: error opening audio file %s",audioFileName);
    return 1;
  }
  fclose(tmpFile);

  /* open metadata file and set up dynamic range parameter buffers */
  if (dynrng_enabl)
  {
    if ((dr_metadata = fopen(dr_metadata_name,"rb"))==NULL) {
      CommonWarning("Encode: error importing metadata file %s", dr_metadata_name);
      return 1;
	}
    dvars_inpt.dyn_rng_sgn = dynrng_sgn;
    dvars_inpt.dyn_rng_ctl = dynrng_shrtblk;
  }

  /* open audio file */
  audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
			    &fileNumSample);
  if (audioFile==NULL)
    CommonExit(1,"Encode: error opening audio file %s "
	       "(maybe unknown format)", 
		   audioFileName);

  if (cplFileNameUsed) /* check audio file */
    {
      if ((tmpFile=fopen(cplFileName,"rb"))==NULL) {
        CommonWarning("Encode: error opening coupling channel audio file %s",cplFileName);
        return 1;
      }
      fclose(tmpFile);
      cplenabl = 1;
      totalNumChans = numChannel + MAX_CPL_CHANNELS;
    }  	
  else
    {
      cplenabl = 0;
      totalNumChans = numChannel;
    }

  if (cplenabl)						/* open coupling channel file */
    {
      numCplChannel = 1;
      cplAudioFile = AudioOpenRead(cplFileName,&numCplChannel,&cplfSample,
                                   &cplFileNumSample);			/* cplFileNumSample is ignored */
      if (cplAudioFile==NULL)
        CommonExit(1,"Encode: error opening coupling channel audio file %s "
                     "(maybe unknown format)", cplFileName);
      if (fSample != cplfSample)
        CommonExit(1,"Encode: error opening coupling channel audio file %s "
                     "(samplerate differs from input channels)", cplFileName);
      if (numCplChannel != 1)
        CommonExit(1,"Encode: error opening coupling channel audio file %s "
                     "(currently only supports one coupling channel)", cplFileName);
    }

  startSample = (long)(regionStart*fSample+0.5);
  if (regionDurat >= 0)
    encNumSample = (long)(regionDurat*fSample+0.5);  /* re-activated HP20040129 */
    /* encNumSample = (long)(numChannel*regionDurat*fSample+0.5); */
  else
    if (fileNumSample == 0)
      encNumSample = -1;
    else
      encNumSample = max(0,fileNumSample-startSample);

#ifdef CT_SBR
  if (sbrenable) {
    if (fSample != 44100.0f && fSample != 48000.0f) {
      CommonExit(1,"Encode: error AAC+SBR only with 44.1 or 48 kHz" 
          " input samplerate");
    } else {
      fSample /= 2.0f;
      downSampleRatio = 2;
    }
    if (cplenabl) {
      CommonExit(1,"Encode: error cplenabl currently not supported for SBR");
    }
#ifdef PARAMETRICSTEREO
    if (sbrpsenable) {
      if (numChannel!=2) {
        CommonExit(1,"Encode: error SBR parametric stereo requires stereo input");
      }
      totalNumChans = 1;   /* was set to numChannel above */
    }
#endif
    if (!((totalNumChans==1 && bitRate==24000) || 
          (totalNumChans==2 && bitRate==48000))) {
#ifndef PARAMETRICSTEREO
      CommonExit(1,"Encode: error supported AAC+SBR bitrates currently only\n"
                 "24kbps mono or 48 kbs stereo");
#else
      CommonExit(1,"Encode: error supported AAC+SBR bitrates currently only\n"
                 "24kbps mono/param-stereo or 48 kbs stereo");
#endif
    }
    lowPassFreq = 5000.0f;
    if (totalNumChans==1) {
      if (bitRate==24000 && fSample==22050)
        lowPassFreq = 5513.0f; /* startFreq 4 @ 22.05kHz */
      if (bitRate==24000 && fSample==24000)
        lowPassFreq = 5625.0f; /* startFreq 4 @ 24kHz */
    }
    if (totalNumChans==2) {
      if (bitRate==48000 && fSample==22050)
        lowPassFreq = 7924.0f; /* startFreq 9 @ 22.05kHz */
      if (bitRate==48000 && fSample==24000)
        lowPassFreq = 8250.0f; /* startFreq 9 @ 24kHz */
    }
  }
  else {
#ifdef PARAMETRICSTEREO
    if (sbrpsenable) {
      CommonExit(1,"Encode: error parametric stereo can only be used with SBR");
    }
#endif
  }
#endif


  /* init encoder */
  bitHeader = BsAllocBuffer(BITHEADERBUFSIZE);
  bitRateLong = (long)(bitRate+0.5);
  fSampleLong = (long)(fSample+0.5);

  EncTfInit(numChannel,
	    fSample,
	    bitRate,
	    encPara,
	    quantDebugLevel,
            bitReservInit,
            varBitRate,
	    &frameNumSample,
	    &delayNumSample,
	    bitHeader,
	    cplenabl,
	    isdisable,
	    msdisable,
	    tnsdisable,
	    pnsSfbStart
#ifdef CT_SBR
            , sbrenable
#ifdef PARAMETRICSTEREO
            , sbrpsenable
#endif
#endif
            );

  frameNumBit = (int)(bitRate*frameNumSample/fSample+0.5);

  /* automatically calculate total buffer size: 
   * multiply 6144 by number of channels */
  if (bitReservSize == 0) {
    if (!lfePresent) {
      bitReservSize = 6144*totalNumChans;
    } else {
      bitReservSize = 6144*(totalNumChans-1);
    }
  }
  if (frameNumBit>bitReservSize) {
    CommonWarning("Bit per frame greater than bitreservoir size (%i>%i)",frameNumBit,bitReservSize);
    printf("Maximum possible bitrate is %i bit/s\n",(int)(bitReservSize*fSample/frameNumSample+.5));
    CommonExit(-1,"Recommending a bitrate below %i bit/s",(int)((bitReservSize-16)*fSample/frameNumSample+.5));
  }
  if (bitReservInit == 0) {
    bitReservInit = bitReservSize - frameNumBit;
  }

  /* variable bit rate: minimum 8 bit/frame (1 byte) */
  /*                    to allow end_of_bitstream detection in decoder */
  frameMinNumBit = varBitRate ? 8 : frameNumBit;
  frameMaxNumBit = /*frameNumBit+*/bitReservSize;  /* keep at least one average frame in buffer */

  if (mainDebugLevel >= 3) {
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
  bitBuf = BsAllocBuffer(frameMaxNumBit);

  if ((sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((sampleBuf[ch]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");

  if (cplenabl)
  {
    if ((cplSampleBuf[0]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
  }

  /* disable any header for now */
  noHeader=1;

  /* open bit stream file */
  if (!noHeader)
    bitStream = BsOpenFileWrite(bitFileName,magicString,info);
  else
    bitStream = BsOpenFileWrite(bitFileName,NULL, NULL);

  if (bitStream==NULL)
    CommonExit(1,"Encode: error opening bit stream file %s",bitFileName);

  /* write bit stream header */
  if (!noHeader) {
    if (BsPutBuffer(bitStream,bitHeader))
      CommonExit(1,"Encode: error writing bit stream header");
    ByteAlign(bitStream);  /* byte align for AAC */
  }
  BsFreeBuffer(bitHeader);

  headerNumBit = BsCurrentBit(bitStream);
  if (mainDebugLevel >= 3)
    printf("headerNumBit=%d\n",headerNumBit);

  /* num frames to start up encoder due to delay compensation */
  startupNumFrame = (delayNumSample+frameNumSample-1)/frameNumSample;

  /* seek to beginning of first (startup) frame (with delay compensation) */
  AudioSeek(audioFile,
	    startSample+delayNumSample-startupNumFrame*frameNumSample);

  if (cplenabl)
  {
  /* seek to beginning of first (startup) frame (with delay compensation) */
	AudioSeek(cplAudioFile,
	    startSample+delayNumSample-startupNumFrame*frameNumSample);
  }

  if (mainDebugLevel >= 3)
    printf("startupNumFrame=%d\n",startupNumFrame);

  /* process audio file frame by frame */
  frame = -startupNumFrame;
  totNumSample = 0;
  totPadNumBit = 0;
  frameAvailNumBit = bitReservInit;
  minBitsNoOverflow = 0;
  minUsedNumBit = minPadNumBit = minReservNumBit = frameMaxNumBit;
  maxUsedNumBit = maxPadNumBit = maxReservNumBit = 0;
  totReservNumBit = 0;

#ifdef CT_SBR
  if (downSampleRatio==2) {
    /* init resampler and buffers for downsampling by 2 */
    float Fc = lowPassFreq / (fSample * downSampleRatio);
    float dF = 0.1f * Fc; /* 10% transition bw */
    float alpha = 7.865; /* 80 dB attenuation */
    float D = 5.015; /* transition width ? */
    NcoefRS = (int)ceil(D/dF) + 1;

    /* downsampling filter coefficients */
    if ((hRS = (float *)malloc(NcoefRS*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");
    RSKaiserLPF (hRS, NcoefRS, Fc, alpha);

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
#endif

  do {
    if (mainDebugLevel >= 1 && mainDebugLevel <= 3) {
      printf("\rframe %4d ",frame);
      fflush(stdout);
    }
    if (mainDebugLevel > 3)
      printf("frame %4d\n",frame);

    /* check for startup frame */
    startupFrame = frame < 0;

#ifndef CT_SBR
    /* read audio file */
    numSample = AudioReadData(audioFile,sampleBuf,frameNumSample);
    totNumSample += numSample;
#else
    if (downSampleRatio==1) {
      /* read audio file */
      numSample = AudioReadData(audioFile,sampleBuf,frameNumSample);
      totNumSample += numSample;
    }
    else if (downSampleRatio==2) {
      /* save last NcoefRS samples */
      for (ch=0; ch<numChannel; ch++)
        for (i=0; i<NcoefRS; i++)
          sampleBufRSold[ch][i] = sampleBufRS[ch][2*frameNumSample-NcoefRS+i];
      /* read audio file */
      numSample = AudioReadData(audioFile,sampleBufRS,2*frameNumSample);
      totNumSample += numSample;
      /* resample */
#ifdef PARAMETRICSTEREO
      if (sbrpsenable) {
        /* downmix to mono */
        for (i=0; i<NcoefRS; i++)
          tmpBufRS[i] = (sampleBufRSold[0][i] +
                         sampleBufRSold[1][i]) / 2.0;
        for (i=NcoefRS; i<2*frameNumSample+NcoefRS; i++)
          tmpBufRS[i] = (sampleBufRS[0][i-NcoefRS] +
                         sampleBufRS[1][i-NcoefRS]) / 2.0;
        FIdownsample(tmpBufRS, sampleBuf[0], frameNumSample, hRS, NcoefRS);
      }
      else
#endif
      {
        for (ch=0; ch<numChannel; ch++) {
          for (i=0; i<NcoefRS; i++)
            tmpBufRS[i] = sampleBufRSold[ch][i];
          for (i=NcoefRS; i<2*frameNumSample+NcoefRS; i++)
            tmpBufRS[i] = sampleBufRS[ch][i-NcoefRS];
          FIdownsample(tmpBufRS, sampleBuf[ch], frameNumSample, hRS, NcoefRS);
        }
      }
    }
    else {
      CommonExit(1,"Encode: invalid downSampleRatio");
    }
#endif
			
	if (cplenabl)
	{
	    /* read audio file */
		cplNumSample = AudioReadData(cplAudioFile,cplSampleBuf,frameNumSample);
		for (samp = cplNumSample; samp < frameNumSample; samp++)
		{
			cplSampleBuf[0][samp] = 0.0;
		}

		SetCplMix(cplmix, numChannel);
	}

/* Read and format dynamic range words */

	dvars_inpt.dynrng_enabl = dynrng_enabl;
	if (dynrng_enabl)
	{
		ReadDynRngWords(&dvars_inpt, dr_metadata);
	}

    /* compute available number of bits */
    if (!startupFrame) {
      /* frameAvailNumBit contains number of bits in reservoir */
      /* variable bit rate: don't exceed bit reservoir size */
      if (frameAvailNumBit > bitReservSize)
	frameAvailNumBit = bitReservSize;
 
      /* Add to frameAvailNumBit the number of bits for this frame */
      frameAvailNumBit += frameNumBit;

      if (mainDebugLevel >= 5)
	printf("frameAvailNumBit=%d\n",frameAvailNumBit);

      /* For constant rate, compute minimum number of bits needed to avoid 
      overflowing bit reservoir.  For variable rate, use at least frameMinNumBit */
      /* FIX: keep at least one average frame in buffer */
      minBitsNoOverflow = varBitRate ?
	                  frameMinNumBit : (frameAvailNumBit - (bitReservSize-frameNumBit));
      if (mainDebugLevel >= 5) printf("minBitsNoOverflow=%d\n",minBitsNoOverflow);  /*  YT for debug */

    }

    /* Encode frame */
    EncTfFrame(sampleBuf,
#ifdef CT_SBR
               sbrenable ? sampleBufRS : NULL,
#endif
	       startupFrame ? (BsBitBuffer*)NULL : bitBuf,
	       startupFrame ? 0 : (frameAvailNumBit),
	       frameNumBit,
               minBitsNoOverflow,  /* use at least this number of bits */
	       frameMaxNumBit,
		   lfePresent,
		   cplmix,
		   cplSampleBuf[0],
		   &dvars_inpt);

    if (!startupFrame) {
      usedNumBit = BsBufferNumBit(bitBuf);

      if (mainDebugLevel >= 5)
	printf("usedNumBit=%d\n",usedNumBit);

      /* write bit stream */
      if (usedNumBit > frameAvailNumBit)
	CommonExit(1,"Encode: more bits used than available in frame+buffer");

      if (BsPutBuffer(bitStream,bitBuf))
	CommonExit(1,"Encode: error writing bit stream data");

      /* write padding bits */
      padNumBit = 0;
      if (usedNumBit < minBitsNoOverflow) {
	padNumBit = minBitsNoOverflow - usedNumBit;
       	if (mainDebugLevel >= 5)
	  printf("padNumBit=%d\n",padNumBit);

        if (padNumBit!=0) CommonExit(1,"Should be no padding bits for AAC (fill_elements)."); 
	for (i=0; i<padNumBit; i++)
	  if (BsPutBit(bitStream,0,1))
	    CommonExit(1,"Encode: error writing bit stream padding bits");
	totPadNumBit += padNumBit;
      } /* if (usedNumBit < minBitsNoOverflow) */

      /* Adjust available bit counts */
      if (padNumBit != 0 ) {
	CommonExit(1,"PADDING BITS!\n"); 
      }
      frameAvailNumBit -= usedNumBit;    /* Subtract bits used */
      frameAvailNumBit -= padNumBit;     /* Subtract bits padded */

      /* Keep track of stats */
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
    }

    frame++;
#ifndef CT_SBR
  } while (((encNumSample<0)&&(numSample==frameNumSample)) ||
	   ((encNumSample>=0)&&(frame*(long)frameNumSample < encNumSample)));
#else
  } while (((encNumSample<0)&&(numSample==frameNumSample)) ||
	   ((encNumSample>=0)&&(frame*(long)frameNumSample*downSampleRatio < encNumSample)));
#endif

  if (mainDebugLevel >= 1 && mainDebugLevel <= 3)
    printf(" \n");

  totDataNumBit = BsCurrentBit(bitStream)-headerNumBit-totPadNumBit;

  if (mainDebugLevel >= 3) {
    printf("totNumFrame=%d\n",frame);
    printf("totNumSample=%ld\n",totNumSample);
    printf("totNumBit=%ld\n",BsCurrentBit(bitStream));
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
    EncTfFree();

  /* close audio file */
  AudioClose(audioFile);

  /* close coupling channel file */
  if (cplenabl)
  {
	AudioClose(cplAudioFile);
    free(cplSampleBuf[0]);
  }
  
  /* close bit stream file */
  if (BsClose(bitStream))
    CommonExit(1,"Encode: error closing bit stream file");

  /* close metadata file */
  if (dynrng_enabl)
  {
	fclose(dr_metadata);
  }

  /* free buffers */
  if (numChannel>1)
    for (ch=0; ch<numChannel; ch++)
      free(sampleBuf[ch]);
  free(sampleBuf);
  BsFreeBuffer(bitBuf);

#ifdef CT_SBR
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
#endif

  return 0;
}


int ByteAlign(BsBitStream* ptrBs) {

  int len, i,j;
  len = BsCurrentBit( ptrBs );
	   
  j = (8 - (len%8))%8;

  if ((len % 8) == 0) j = 0;
  for( i=0; i<j; i++ ) {
    BsPutBit( ptrBs, 0, 1 ); 
  }
  return j;
}
  

void SetCplMix(double *cplmix, int numChannel)
{
	int ch;

	for (ch = 0; ch < MAX_TIME_CHANNELS; ch++)
	{
		cplmix[ch] = 0.0;
	}
	if ((numChannel - lfePresent) % 2)	/* Center channel present */
	{
		switch (cplMixMode)
		{
			case 0:
			for (ch = 0; ch < MAX_TIME_CHANNELS; ch++)
			{
				cplmix[ch] = 0.5;
			}
			break;
		case 1:
			cplmix[0] = 0.1;
			cplmix[1] = 0.707;
			cplmix[2] = 0.707;
			break;
		case 2:
			cplmix[0] = 1.0;
			break;					    
		}
	}
	else								/* No center channel present */
	{
		switch (cplMixMode)
		{
			case 0:
			for (ch = 0; ch < MAX_TIME_CHANNELS; ch++)
			{
				cplmix[ch] = 0.5;
			}
			break;
		case 1:
			cplmix[0] = 0.707;
			cplmix[1] = 0.707;
			break;
		case 2:
			cplmix[0] = 1.0;
			cplmix[1] = 1.0;
			break;					    
		}
	}
} /* end SetCplMix */


/* read metadata from text file.  There are 4 dynamic range control words for each group of
   256 samples.  The input dyn range words maps to dB with the following bounds:
   dynrng_import[i] = -1.0 results in -24dB of gain reduction	   (floating pt input)
   dynrng_import[i] = 0.0 results in unity gain (0dB)
   dynrng_import[i] = +1.0 results in +24dB of gain boost

   the output dyn range words map as follows:
   dynrng_shrtblk[i] = -128 results in -31.75dB of gain reduction  (0.25 dB increment steps)
   dynrng_shrtblk[i] = 0 results in unity gain (0dB)
   dynrng_shrtblk[i] = +127 results in +31.75dB of gain boost

   Format this data into AAC metadata words for dynamic range control words, 
   and program reference level words.  The program_reference_level word is derived from a word
   that covers the same range in dB, but has a more course step size of 1dB per step. */

void ReadDynRngWords(DRC_INTF *dvars_inpt, FILE *dr_metadata)
{
	short ref_level_import, i, cnt;
	double drtemp;
	double fact = 0.39685026299;	/* fact = 2^(-128/24) / 2^(-4) */
	double ifact;
	static short ref_level_import_sav;
	static double dynrng_import_sav;

#ifndef WIN32
	/* Needed for byte-swapping */
	/*	unsigned long *longptr;
	unsigned long *longptrhigh;
	double *doubleptr;
	unsigned long longval;
	unsigned long longvalhigh;*/
	unsigned int j;
	unsigned char *byteptr,*byteptr2;
	unsigned char byteval;
#endif

	ifact = 1.0 / fact;

	if (dynrng_enabl)
	{
		if (fread(&ref_level_import, sizeof(short), 1, dr_metadata) != 1)
		{
			ref_level_import = ref_level_import_sav;
		}
#ifndef WIN32
		/* byte swap this short */
		else {
		  byteptr = (unsigned char *)(&ref_level_import);
		  byteval = *byteptr; 
		  *byteptr = *(byteptr+1);
		  *(byteptr+1) = byteval;
		}
#endif

		if (fread(dynrng_import, sizeof(double), 4, dr_metadata) != 4)
		{
			for (i = 0; i < 4; i++)
			{
				dynrng_import[i] = dynrng_import_sav;
			}
		}
#ifndef WIN32
		/* byte swap these doubles */
		else {
			for (i = 0; i < 4; i++)
			{
			  byteptr = (unsigned char *)(&dynrng_import[i]);
			  byteptr2 = byteptr + sizeof(double) -1;
			  for (j=0;j<(sizeof(double)/2);j++) {
			    byteval = *byteptr;
			    *byteptr = *(byteptr2);
			    *(byteptr2) = byteval;
			    byteptr++;
			    byteptr2--;
			  }
			}
		}
#endif

		cnt = 0;
		for (i = 0; i < 8; i++)
		{
			if ((i % 2) == 0)
			{
				if (dynrng_import[cnt] < 0.0)
				{
					drtemp = fact * pow(10, (8.0 + 32.0 * dynrng_import[cnt++]) * 0.05);
					dynrng_sgn[i] = 1;
					dynrng_shrtblk[i] = -(short)(24.0 * log10(drtemp) / LG2);
				}
				else
				{
					drtemp = ifact * pow(10, (-8.0 + 32.0 * dynrng_import[cnt++]) * 0.05);
					dynrng_sgn[i] = 0;
					dynrng_shrtblk[i] = (short)(24.0 * log10(drtemp) / LG2);
				}
			}
			else
			{
				dynrng_shrtblk[i] = dynrng_shrtblk[i - 1];
				dynrng_sgn[i] = dynrng_sgn[i - 1];
			}
		}

		dvars_inpt->prog_ref_level = (ref_level_import << 2);
		ref_level_import_sav = ref_level_import;
		dynrng_import_sav = dynrng_import[0];
	}
}

	
/* ---------- main ---------- */

int main (int argc, char *argv[])
{
  char *progName = "<no program name>";
  int result;
  char oriFileName[STRLEN], bitFileName[STRLEN], fullCplFileName[STRLEN];
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
      /*      EncParInfo(stdout);
      EncLpcInfo(stdout);*/
      EncTfInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"command line error (\"-h\" for help)");
  }

  if (mainDebugLevel >= 1)
    printf("%s: %s\n",progName,PROGVER);
  if (mainDebugLevel >= 2) {
    printf("CVS Id: %s\n",CVSID);
    /*    printf("%s\n",EncParInfo(NULL));
    printf("%s\n",EncLpcInfo(NULL));*/
    printf("%s\n",EncTfInfo(NULL));
  }

  CmdLineInit(cmdDebugLevel);

  /* calc variable default values */
  if (!bitReservSizeUsed)
    bitReservSize = 0;
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
  /*  if (cplFileNameUsed && varArgIdx[0]>=0 && varArgIdx[1]>=0)
      CommonExit(1,"only one coupling channel file allowed when using -cp");*/
  
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

	if (cplFileNameUsed)
	{
      if (ComposeFileName(cplFileName,0,oriPath,oriExt,fullCplFileName,
			  STRLEN))
	CommonExit(1,"composed file name too long");
    }
	else if (cplMixModeUsed)
	{
		CommonExit(1,"Must supply coupling channel file name");
	}

    /* complete info string */
    *infoTail = '\0';
    strcat(infoTail,"ori: ");
    strcat(infoTail,oriFileName);
    strcat(infoTail,"\n");
    strcat(infoTail,"bit: ");
    strcat(infoTail,bitFileName);
    strcat(infoTail,"\n");
	strcat(infoTail,fullCplFileName);
	strcat(infoTail, "\n");

    /* encode file */
    if (mainDebugLevel >= 1)
      printf("encoding %s -> %s\n",oriFileName,bitFileName);

    if (Encode(oriFileName,
	       bitFileName,
	       /*	codecMode, */
	       bitRate,
	       varBitRate,
               lfePresent,
	       bitReservSize,
	       bitReservInit,
	       encPara,
	       noInfo ? "" : info,
               noHeader,
	       magicString,
               fullCplFileName,
	       regionStart,
	       regionDurat))
      CommonWarning("error encoding audio file %s",oriFileName);

    fileIdx++;
  }
  
  CmdLineEvalFree(paraList);

  if (mainDebugLevel >= 1)
    printf("%s: finished\n",progName);

  return (0);
}

/* end of encoder.c */

