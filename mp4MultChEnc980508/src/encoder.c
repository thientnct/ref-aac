/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Source file: encoder.c (formerly mp4enc.c)

$Id: encoder.c,v 1.1.1.2 1998-05-07 17:21:25+03 suonio Exp $

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
21-aug-96   HP    added NBC_RM4 switches by BG
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
                  removed NBC_RM4 switches (new -nh, -ms, -bi options)
10-dec-96   HP    new -ni, -vr options, added variable bit rate
23-jan-97   HP    added audio i/o module
07-jun-97   CL    Renamed this file (mp4enc.c) to encoder.c for 
                  use in AAC technical report encoder.  Removed
                  support for lpc and parametric codecs.
07-apr-98   RG    added switch -lf to indicate presence of LFE channel   
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libtsp.h>	/* AFsp audio file library */

#include "common.h"	/* common module */
#include "cmdline.h"	/* command line module */
#include "bitstream.h"	/* bit stream module */
#include "audio.h"	/* audio i/o module */

#include "mp4.h"	/* frame work declarations */
#include "enc.h"	/* encoder cores */
#include "tf_main.h"	/* AAC_PROFILE enum */
#include "mp4mux.h"	/* MP4 container muxer */

/* ---------- declarations ---------- */

#define PROGVER "AAC Audio Encoder V0.4 11-mar-98"
#define CVSID "$Id: encoder.c,v 1.1.1.2 1998-05-07 17:21:25+03 suonio Exp $"

/* ---------- variables ---------- */

/* command line module */

static float bitRate;
static int varBitRate;
static int lfePresent;                    
static int bitReservSize;
static int bitReservInit;
static int bitReservInitUsed;
static float regionStart;
static float regionDurat;
static int regionDuratUsed;
static char *encPara;
static char *outFileName;
static int outFileNameUsed;
static char *oriPath,*bitPath;
static int oriPathUsed,bitPathUsed;
static char *oriExt,*bitExt;
static int oriExtUsed,bitExtUsed;
static int noInfo;
static int noHeader;
static char *magicString;
static int mainDebugLevel,audioDebugLevel,bitDebugLevel,cmdDebugLevel,quantDebugLevel;
static int mp4OutFlag;
static int *varArgIdx;

#ifdef PLOTMTV
int frame; /* global for debugging */
#else 
static  int frame;
#endif 

static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<audio file(s)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"r",&bitRate,"%f","6000",NULL,"bit rate [bit/s]"},
  {"vr",&varBitRate,NULL,NULL,NULL,"enable variable bit rate"},
  {"lf",&lfePresent,NULL,NULL,NULL,"LFE channel present"},             
  {"b",&bitReservSize,"%d","8184",NULL,"bit reservoir size [bit]"},
  {"bi",&bitReservInit,"%d",NULL,&bitReservInitUsed,
   "initial bit reservoir bits\n"
  "(dflt: bit reservoir size)"},
  {"c",&encPara,"%s","",NULL,
   "encoder parameter string"},
  {"ts",&regionStart,"%f","0",NULL,"start time of encoded region [sec]"},
  {"td",&regionDurat,"%f",NULL,&regionDuratUsed,
   "duration of encoded region [sec]\n"
   "(dflt: until end of file)"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,"output file name"},
  {"po",&oriPath,"%s",NULL,&oriPathUsed,
   "default path original audio files (dflt: $" MP4_ORI_PATH_ENV ")"},
  {"pb",&bitPath,"%s",NULL,&bitPathUsed,
   "default path bit stream files (dflt: $" MP4_BIT_PATH_ENV ")"},
  {"eo",&oriExt,"%s",NULL,&oriExtUsed,
   "original audio file extension\n"
   "supported file formats: .au, .snd, .wav, .aif, .raw\n"
   "(dflt: $" MP4_ORI_FMT_ENV " / " MP4_ORI_EXT ")\n"
   "(raw format: $" MP4_ORI_RAW_ENV ")"},
  {"eb",&bitExt,"%s",".aac",&bitExtUsed,"bit stream file extension"},
  {"ni",&noInfo,NULL,NULL,NULL,"disable bit info string"},
  {"nh",&noHeader,NULL,NULL,NULL,"disable mp4 bit stream header"},
  /*  {"ms",&magicString,"%s",MP4_MAGIC,NULL,"bit stream magic string"},*/
  {"d",&mainDebugLevel,"%i","0",NULL,"main debug level"},
  {"da",&audioDebugLevel,"%i","0",NULL,"audio file debug level"},
  {"db",&bitDebugLevel,"%i","0",NULL,"bit stream debug level"},
  {"dc",&cmdDebugLevel,"%i","0",NULL,"cmd line debug level"},
  {"dq",&quantDebugLevel,"%i","0",NULL,"quantizer  debug level"},
  {"mp4",&mp4OutFlag,NULL,NULL,NULL,"write MP4 container instead of raw bit stream"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};


/* ---------- functions ---------- */

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
  char *info,			/* in: info string for bit stream */
  int noHeader,			/* in: disable bit stream header */
  char *magicString,		/* in: bit stream magic string */
  float regionStart,		/* in: start time of region */
  float regionDurat,		/* in: duration of region */
  int mp4Out)			/* in: write MP4 container */
				/* returns: 0=OK  1=error */
{ 
  float fSample;
  long fileNumSample;
  long totNumSample;
  int numChannel;
  int frameNumSample,delayNumSample;

  int frameMinNumBit,frameMaxNumBit;
  long fSampleLong,bitRateLong;
  BsBitBuffer *bitBuf;
  float **sampleBuf;
  BsBitStream *bitStream;
  AudioFile *audioFile;
  BsBitBuffer *bitHeader;
  FILE *tmpFile;
  int startupNumFrame,startupFrame;
  int numSample;
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

  /* open audio file */
  audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
			    &fileNumSample);
  if (audioFile==NULL)
    CommonExit(1,"Encode: error opening audio file %s "
	       "(maybe unknown format)",
	       audioFileName);

  startSample = (long)(regionStart*fSample+0.5);
  if (regionDurat >= 0)
    /* encNumSample = (long)(regionDurat*fSample+0.5); */
    encNumSample = (long)(numChannel*regionDurat*fSample+0.5);
  else
    if (fileNumSample == 0)
      encNumSample = -1;
    else
      encNumSample = max(0,(fileNumSample-startSample)*numChannel);

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
	    bitHeader);

  frameNumBit = (int)(bitRate*frameNumSample/fSample+0.5);

  /* variable bit rate: minimum 8 bit/frame (1 byte) */
  /*                    to allow end_of_bitstream detection in decoder */
  frameMinNumBit = varBitRate ? 8 : frameNumBit;
  frameMaxNumBit = frameNumBit+bitReservSize;

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
    printf("bitHeaderNumBit=%d\n",BsBufferNumBit(bitHeader));
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

  /* MP4 container output validation */
  if (mp4Out) {
    int profile = EncTfGetProfile();
    if (profile != MAIN && profile != LOW)
      CommonExit(1,"Encode: -mp4 supports only Main or Low Complexity profile");
    if (numChannel != 1 && numChannel != 2)
      CommonExit(1,"Encode: -mp4 supports only mono or stereo");
  }

  /* disable any header for now */
  noHeader=1;

  /* open bit stream file */
  if (mp4Out) {
    int profile = EncTfGetProfile();
    int audioObjType = (profile == LOW) ? 2 : 1;  /* LC=2, Main=1 */
    int chanCfg = numChannel;                     /* 1=mono, 2=stereo */
    int flFlag = (frameNumSample == 960) ? 1 : 0;
    if (MP4MuxOpen(bitFileName,(unsigned long)fSampleLong,chanCfg,
                   (unsigned long)bitRateLong,frameNumSample,flFlag,
                   audioObjType))
      CommonExit(1,"Encode: error opening MP4 file %s",bitFileName);
    BsFreeBuffer(bitHeader);
    bitStream = NULL;
    headerNumBit = 0;
  } else {
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
  minBitsNoOverflow = 0;
  minUsedNumBit = minPadNumBit = minReservNumBit = frameMaxNumBit;
  maxUsedNumBit = maxPadNumBit = maxReservNumBit = 0;
  totReservNumBit = 0;

  do {
    if (mainDebugLevel >= 1 && mainDebugLevel <= 3) {
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
      minBitsNoOverflow = varBitRate ?
	                  frameMinNumBit : (frameAvailNumBit - bitReservSize);
      if (mainDebugLevel >= 5) printf("minBitsNoOverflow=%d\n",minBitsNoOverflow);  /*  YT for debug */

    }

    /* Encode frame */
    EncTfFrame(sampleBuf,
	       startupFrame ? (BsBitBuffer*)NULL : bitBuf,
	       startupFrame ? 0 : (frameAvailNumBit),
	       frameNumBit,
               minBitsNoOverflow,  /* use at least this number of bits */
	       frameMaxNumBit,
               lfePresent);    

    if (!startupFrame) {
      usedNumBit = BsBufferNumBit(bitBuf);

      if (mainDebugLevel >= 5)
	printf("usedNumBit=%d\n",usedNumBit);

      /* write bit stream */
      if (usedNumBit > frameAvailNumBit)
	CommonExit(1,"Encode: more bits used than available in frame+buffer");

      if (mp4Out) {
        if (MP4MuxAddFrame(bitBuf))
          CommonExit(1,"Encode: error writing MP4 sample");
      } else {
      if (BsPutBuffer(bitStream,bitBuf))
	CommonExit(1,"Encode: error writing bit stream data");
      }

      /* write padding bits */
      padNumBit = 0;
      if (usedNumBit < minBitsNoOverflow) {
	padNumBit = minBitsNoOverflow - usedNumBit;
       	if (mainDebugLevel >= 5)
	  printf("padNumBit=%d\n",padNumBit);

        if (padNumBit!=0) CommonExit(1,"Should be no padding bits for AAC (fill_elements)."); 
	if (!mp4Out) {
	  for (i=0; i<padNumBit; i++)
	    if (BsPutBit(bitStream,0,1))
	      CommonExit(1,"Encode: error writing bit stream padding bits");
	}
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
  } while ((encNumSample<0)&&(numSample==frameNumSample) ||
	   (encNumSample>=0)&&(frame*numChannel*(long)frameNumSample < encNumSample));

  if (mainDebugLevel >= 1 && mainDebugLevel <= 3)
    printf(" \n");

  totDataNumBit = mp4Out ? 0 : BsCurrentBit(bitStream)-headerNumBit-totPadNumBit;

  if (mainDebugLevel >= 3) {
    printf("totNumFrame=%d\n",frame);
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
    EncTfFree();


  /* close audio file */
  AudioClose(audioFile);
  
  /* close bit stream file */
  if (mp4Out) {
    if (MP4MuxClose())
      CommonExit(1,"Encode: error closing MP4 file");
  } else {
  if (BsClose(bitStream))
    CommonExit(1,"Encode: error closing bit stream file");
  }

  /* free buffers */
  if (numChannel>1)
    for (ch=0; ch<numChannel; ch++)
      free(sampleBuf[ch]);
  free(sampleBuf);
  BsFreeBuffer(bitBuf);

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
  if (!bitReservInitUsed)
    bitReservInit = bitReservSize;
  if (!regionDuratUsed)
    regionDurat = -1;
  if (!oriPathUsed)
    oriPath = getenv(MP4_ORI_PATH_ENV);
  if (!bitPathUsed)
    bitPath = getenv(MP4_BIT_PATH_ENV);
  if (mp4OutFlag && !bitExtUsed)
    bitExt = ".mp4";
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
	       noInfo ? "" : info,noHeader,
	       magicString,
	       regionStart,
	       regionDurat,
               mp4OutFlag))
      CommonWarning("error encoding audio file %s",oriFileName);

    fileIdx++;
  }
  
  CmdLineEvalFree(paraList);

  if (mainDebugLevel >= 1)
    printf("%s: finished\n",progName);

  return (0);
}

/* end of encoder.c */

