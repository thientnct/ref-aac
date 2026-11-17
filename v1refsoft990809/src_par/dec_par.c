/**********************************************************************
MPEG-4 Audio VM
Decoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Masayuki Nishiguchi, Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)

and edited by

Akira Inoue (Sony Corporation) and
Yuji Maeda (Sony Corporation)

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

Copyright (c) 1997.



Source file: dec_par.c

$Id: dec_par.c,v 1.41 1999/08/09 16:21:39 purnhage Exp $

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module
indilinedec.o		indiline bitstream decoder module
indilinesyn.o		indiline synthesiser module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>
MN    Masayuki Nishiguchi, Sony IPC <nishi@pcrd.sony.co.jp>
AI    Akira Inoue, Sony IPC <akira@pal.arch.sony.co.jp>

Changes:
18-jun-96   HP    first version (dummy)
25-jun-96   HP    implemented "individual spectral lines"
28-jun-96   HP    joined with HVXC code by Sony IPC
01-jul-96   HP    added HVXC header file, included modifications by IPC
15-aug-96   HP    added DecParInfo(), DecParFree()
                  adapted to new dec.h
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for "individual lines"
06-sep-96   HP    incorporated changes by Sony IPC for a new HVXC with
                  speed change & pitch change functionality
10-sep-96   BE
12-sep-96   HP    incorporated indiline modules as source code
26-sep-96   HP    adapted to new indiline module interfaces
26-sep-96   HP    incorporated changes by Sony IPC
15-nov-96   HP    adapted to new bitstream module
04-dec-96   HP    fix call to BsGetBitChar(), included ISO copyright
03-feb-97   HP    splitting enc/dec
06-jul-97   HP    switch/mix
17-jul-97   HP    HXVC delayNumSample
23-oct-97   HP    merged IL/HVXC switching
07-nov-97   MN    bug-fixes
11-nov-97   HP    added HVXC variable rate switches
12-nov-97   HP    fixed mixed HVXC/IL mode
05-feb-99   HP    marked HILN as Version 2
XX-feb-99   AI    HVXC status variables defined,
                  DecHvxcFrame(), DecHvxcInit(), DecHvxcFree() implemented

VMIL stuff
10-apr-97   HP    harmonic stuff ...
22-apr-97   HP    noisy stuff ...
10-jun-97   HP    added para file support
23-jun-97   HP    ANSI-C para file fixes
24-jun-97   HP    adapted env/harm/noise interface
29-aug-97   HP    added random start phase
13-nov-97   HP    fixed harm speech change
16-nov-97   HP    adapted HILN decoder config header
16-dec-97   HP    noise para factor
30-mar-98   BE/HP initPrevNumLine
06-apr-98   HP    ILD/ILS config
11-jun-98   HP    enha support for para file
02-dec-98   HP    new noise quant as dflt.
18-jun-98   HP    CD-nx bsFormat noise quant

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "obj_descr.h"		/* AI 990616 */



#include "dec_par.h"		/* decoder cores */

#include "dec_hvxc.h"		/* HVXC decoder core */

#include "mp4_par.h"		/* parametric core common declarations */

#include "hvxc.h"		/* hvxcDec module */
#include "hvxcDec.h"		/* hvxcDec module */


/* ---------- declarations ---------- */

#define PROGVER "parametric decoder core DIS V0.1 05-feb-99"

#define SEPACHAR " ,="

#define MAXSAMPLE 32768		/* full scale sample value */

#define STRLEN 255

#define MINSPEEDFACT (0.2)	/* expected min. value: speedFact=0.2 */
				/* AI 990128 / HP 990422 */


/* ---------- variables ---------- */

static float DPfSample;
static float DPbitRate;
static int DPmode;		/* enum MP4ModePar */
static int DPframeMaxNumSample;
static int DPframeTypNumSample;
static int DPdelayNumSample;


/* command line module */

static int DPdelayMode;
static int DPdebugLevel;
static int DPdebugLevelIl;

/* HILN decoder control HP970514 */
static int DPnoIL;
static int DPnoHarm;
static int DPnoNoise;
static int DPmaxNumLine;
static int DPmaxNumHarm;
static int DPmaxNumHarmLine;
static int DPmaxNumNoisePara;
static int DPmaxNumSynth;
static float DPnoiseParaFact;
static int DPenhaFlag;
static int DPinitPrevNumLine;

/* HVXC decoder configuration (AI 990210) */
static int DPvarMode;
static int DPrateMode;
static int DPextensionFlag;



static int DPdecMode;

static int DPtestMode;	/* HVXC test_mode for decoder conformance testing */
/* 0000 0000 0000 0000 : normal operation */
/* xxxx xxxx xxxx xxx1 : postfilter and post processing are skipped */
/* xxxx xxxx xxxx xx1x : initial values of harmonic phase are reset to zeros */
/* xxxx xxxx xxxx x1xx : noise component addition are disabled */
/* xxxx xxxx xxxx 1xxx : the output of Time Domain Decoder is disabled */

/* HVXC decoder handle(used in DecParFrame() currently) */
static HvxcDecStatus *decHvxcHandle;


static int DPtest;
static int DPfree;

static int sysFlag;	/* a flag to use system interface(flexmux) (AI 990616) */

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"dm",&DPdelayMode,"%i","1",NULL,"HVXC delay mode (0=short 1=normal)"},
  {"dl",&DPdebugLevel,"%i","0",NULL,"debug level"},
  {"tm",&DPtestMode,"%d","0",NULL,"HVXC test_mode (for decoder conformance testing)"},
  {"-hvxc_sys",&sysFlag,NULL,NULL,NULL,"use system interface(flexmux)"},	/* AI 990616 */
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* switch/mix stuff */
static float *tmpSampleBuf;
static float *mixSampleBuf;
static int mixSampleBufSize = 0;
static BsBitBuffer *dmyBuf;
static int ILdelayNumSample = 0;
static int HVXdelayNumSample = 0;
static BsBitBuffer *tmpBSbuf;
static BsBitStream *tmpBSstream;



/* ---------- internal functions ---------- */



/* DecParInitHvx() */
/* Init: harmonic vector exitation */

static void DecParInitHvx (
  BsBitStream *hdrStream)	/* in: header for bit stream */
{
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPvarMode,1))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPrateMode,2))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPextensionFlag,1))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");

  /*
  DPframeMaxNumSample = 160 * (int) ceil(1.0 / DPspeedFact);
  */
  /* DPframeMaxNumSample = 320;	*//* expected max.value, speedFact=0.5 (AI 990128) */
  DPframeMaxNumSample = 160 * (int) ceil(1.0 / MINSPEEDFACT);
  
  if (DPdelayMode == DM_LONG)	/* HP 971023 */
    DPdelayNumSample = 80;	/* 10 ms */
  else
    DPdelayNumSample = 60;	/* 7.5 ms */

  decHvxcHandle = hvxc_decode_init(DPvarMode,
				   DPrateMode,
				   DPextensionFlag,
				   DPdelayMode,
				   DPtestMode);
}




/* DecParFrameHvx() */
/* Decode frame: harmonic vector exitation */

void DecParFrameHvx (
  BsBitStream *stream,		/* in: bit stream */
  float *sampleBuf,		/* out: frameNumSample audio samples */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
  float speedFact,		/* in: speed change factor(AI 990209) */
  float pitchFact		/* in: pitch change factor(AI 990209) */
  )
{
    int	i, j;

    unsigned char tmpIdVUV;
    unsigned char tmpEncBit;

    unsigned char encBit[10];

    if (speedFact<MINSPEEDFACT) {
      speedFact = MINSPEEDFACT;
      CommonWarning("DecParFrameHvx: speedFact below %f",MINSPEEDFACT);
    }

    {
      /* HP 970706 970708 970709 */
      if (!stream){
        DPdecMode = DEC0K; 
      }
      else if (!BsEof(stream,80-1)){  /* MN 971107 */
        DPdecMode = DEC4K; 
      }
      else if (!BsEof(stream,80-6-1)){ /* MN 971107 */
        DPdecMode = DEC3K;
      }
      else{
        DPdecMode = DEC2K;
      }
    }

    if (DPdebugLevel >= 2) {
      printf("DecParFrameHvx: stream=%s\n",stream?"valid":"NULL");
      printf("DecParFrameHvx: decMode=");
      switch (DPdecMode) {
      case DEC0K : printf("DEC0K\n"); break;
      case DEC2K : printf("DEC2K\n"); break;
      case DEC3K : printf("DEC3K\n"); break;
      case DEC4K : printf("DEC4K\n"); break;
      default : printf("ERROR!!!\n");
      }
    }

    for (i = 0; i < 5; i++) encBit[i] = 0;
    
    if(DPdecMode == DEC4K || DPdecMode == DEC3K)
	/* Modified on 07/04/97 by Y.Maeda */
    {
      {
        for(j = 0; j < 9; j++)
          if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
        if (DPdecMode == DEC4K) {
	  if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
        }
        else {
	  /* MN 971107 */
	  if(BsGetBitChar(stream, &tmpEncBit, 8-6))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[9] = (tmpEncBit & 0x3) << 6;
        }
      }
    }
    else if(DPdecMode == DEC2K)
    {
      if(DPvarMode == BM_VARIABLE)
      {
        if(BsGetBitChar(stream, &tmpIdVUV, 2))
          CommonExit(1,"DecParFrameHvx: error reading bit stream");

	encBit[0] = (tmpIdVUV << 6) & 0xc0;

	/* HP 971111 */
	if (DPdebugLevel >= 2)
	  printf("DecParFrameHvx: tmpIdVUV=%d\n", tmpIdVUV);

	switch(tmpIdVUV)
        {
        case 0:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);

          for(j = 1; j < 3; j++)
            if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");

	  if(BsGetBitChar(stream, &tmpEncBit, 4))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[3] = (tmpEncBit & 0xf) << 4;
	  break;
	case 1:
	  /*** reading padded bits necessary(AI 990528) ***/
	  if(BsGetBitChar(stream, &tmpEncBit, 6))	
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  break;
	case 2:
	case 3:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);

          for(j = 1; j < 5; j++)
            if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  break;
	}
      }
      else
      {
        for(j = 0; j < 5; j++)
          if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
      }
    }

    hvxc_decode(decHvxcHandle,
		encBit,
		sampleBuf,
		frameBufNumSample,
		speedFact,
		pitchFact,
		DPdecMode
		);
}




/* DecParFreeHvx() */
/* Free memory: harmonic vector exitation */

void DecParFreeHvx ()
{
  hvxc_decode_free(decHvxcHandle);
}


/* ---------- functions ---------- */


char *DecParInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "decoder parameter string format:\n"
	    "  list of tokens (tokens separated by characters in \"%s\")\n",
	    SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
  }
  return PROGVER;
}


/* DecParInit() */
/* Init parametric decoder core. */

void DecParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample		/* out: decoder delay (num samples) */
  )
{
  int parac;
  char **parav;
  int result;
  BsBitStream *hdrStream;
  char *paraMode;
  int i;

  /* evaluate decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      DecParInfo(stdout);
      CommonExit(1,"decoder core aborted ...");
    }
    else
      CommonExit(1,"decoder parameter string error");
  }


  if (DPdebugLevel >= 1) {
    printf("DecParInit: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecParInit: decPara=\"%s\"\n",
	   decPara);
    printf("DecParInit: debugLevel=%d\n",
	   DPdebugLevel);
  }

  if (numChannel != 1)
    CommonExit(1,"DecParInit: audio data has more the one channel (%d)",
	       numChannel);

  CmdLineParseFree(parav);

  DPfSample = fSample;
  DPbitRate = bitRate;

  hdrStream = BsOpenBufferRead(bitHeader);
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPmode,2))
    CommonExit(1,"DecParInit: error reading bit stream header");

  if (DPmode<0 || DPmode>MODEPAR_NUM)
    CommonExit(1,"DecParInit: unknown parametric codec mode %d",DPmode);
  paraMode = MP4ModeParName[DPmode];

  if (DPdebugLevel >= 1)
    printf("DecParInit: modeInt=%d  mode=\"%s\"\n",DPmode,paraMode);

  switch (DPmode) {
  case MODEPAR_HVX:
    DecParInitHvx(hdrStream);
    break;
  }
  BsClose(hdrStream);

  *frameMaxNumSample = DPframeMaxNumSample;
  *delayNumSample = DPdelayNumSample;
  mixSampleBufSize = DPframeMaxNumSample+
    max(DPdelayNumSample-ILdelayNumSample,DPdelayNumSample-HVXdelayNumSample);

  if (CommonFreeAlloc((void**)&tmpSampleBuf,
		      *frameMaxNumSample*sizeof(float)) == NULL)
    CommonExit(1,"DecParInit: memory allocation error");
  if (CommonFreeAlloc((void**)&mixSampleBuf,
		      mixSampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"DecParInit: memory allocation error");
  for (i=0; i<mixSampleBufSize; i++)
    mixSampleBuf[i] = 0;

  if (DPdebugLevel >= 1) {
    printf("DecParInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   DPdelayMode,DPvarMode,DPrateMode);
    printf("DecParInit: dly=%d  frm=%d  mixBuf=%d  ILdly=%d  HVXdly=%d\n",
	   DPframeMaxNumSample,DPdelayNumSample,
	   mixSampleBufSize,ILdelayNumSample,HVXdelayNumSample);
  }
}


/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core. */

void DecParFrame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor (AI 990209) */
  float pitchFact		/* in: pitch change factor (AI 990209) */
  )
{
  BsBitStream *stream;
  unsigned int il_hvxc;
  int i;
  int hvxcFrame1;

  if (DPdebugLevel >= 1) {
    printf("DecParFrame: availNum=%ld\n",BsBufferNumBit(bitBuf));
    printf("DecParFrame: speedFact=%f  pitchFact=%f\n",
	   speedFact,pitchFact);
  }

  stream = BsOpenBufferRead(bitBuf);
  switch (DPmode) {
  case MODEPAR_HVX:
    DecParFrameHvx(stream,sampleBuf[0],frameBufNumSample,speedFact,pitchFact
		   );
    break;
  }
  *usedNumBit = BsCurrentBit(stream);
  BsCloseRemove(stream,1);

  if (DPdebugLevel >= 1)
    printf("DecParFrame: numBit=%d  numSample=%d\n",
	   *usedNumBit,*frameBufNumSample);

}


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree ()
{
  if (DPdebugLevel >= 1)
    printf("DecParFree: ...\n");

  if (!DPfree)
    return;
  if (DPmode != MODEPAR_IL)
    DecParFreeHvx();
}


/* DecHvxcInitNew() */
/* Init HVXC decoder core for system interface(AI 99060X) */

HvxcDecStatus *DecHvxcInitNew (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer			/* in: number of layer */
  )
{
  int i;
  int parac;
  char **parav;
  int result;

  BsBitBuffer *bitHeader;
  BsBitStream *hdrStream;

  AUDIO_SPECIFIC_CONFIG* audSpC;

  int numChannel;
  float fSample;
  float bitRate;

  HvxcDecStatus *hvxcD;

  sysFlag = 1;	/* use system interface(flexmux) */
  
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      DecParInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"decoder parameter string error");
  }

  audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;

  switch (audSpC->channelConfiguration.value) {
  case 1 :numChannel=1;       
    break;
  default: CommonExit(1,"wrong channel config");
  }
  bitRate = fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value;

  if (fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value != 11) {
    CommonExit(1,"illegal sampling frequency config");
  }
  else {
    fSample = 8000.0f;
  }
  
  if (DPdebugLevel >= 1) {
    printf("DecHvxcInit: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecHvxcInit: decPara=\"%s\"\n",
	   decPara);
    printf("DecHvxcInit: debugLevel=%d\n",
	   DPdebugLevel);

    if (DPtestMode != TM_NORMAL) {
      printf("DecHvxcInit: test_mode=%x(for HVXC decoder conformance testing)\n", DPtestMode);
      if (DPtestMode & TM_POSFIL_DISABLE) {
	printf("  - postfilter and post processing are skipped.\n");
      }
      if (DPtestMode & TM_INIT_PHASE_ZERO) {
	printf("  - initial values of harmonic phase are reset to zeros.\n");
      }
      if (DPtestMode & TM_NOISE_ADD_DISABLE) {
	printf("  - noise component addition are disabled.\n");
      }
      if (DPtestMode & TM_VXC_DISABLE) {
	printf("  - the output of Time Domain Decoder is disabled.\n");
      }
    }
  }

  if (numChannel != 1)
    CommonExit(1,"DecHvxcInit:audio data has more the one channel (%d)",
	       numChannel);

  CmdLineParseFree(parav);

  DPfSample = fSample;
  DPbitRate = bitRate;

  if (sysFlag) {
    DPvarMode = audSpC->specConf.hvxcSpecificConfig.HVXCvarMode.value;
    DPrateMode = audSpC->specConf.hvxcSpecificConfig.HVXCrateMode.value;
    DPextensionFlag = audSpC->specConf.hvxcSpecificConfig.extensionFlag.value;

  }
  else {	/* not necessary any more ? */
    
    hdrStream = BsOpenBufferRead(bitHeader);

    if (BsGetBitInt(hdrStream,(unsigned int*)&DPvarMode,1))
      CommonExit(1,"DecHvxcInit: error reading bit stream header");
    if (BsGetBitInt(hdrStream,(unsigned int*)&DPrateMode,2))
      CommonExit(1,"DecHvxcInit: error reading bit stream header");
    if (BsGetBitInt(hdrStream,(unsigned int*)&DPextensionFlag,1))
      CommonExit(1,"DecHvxcInit: error reading bit stream header");
    BsClose(hdrStream);
  }
  
  DPframeMaxNumSample = 160 * (int) ceil(1.0 / MINSPEEDFACT);
  
  if (DPdelayMode == DM_LONG)	/* HP 971023 */
    DPdelayNumSample = 80;	/* 10 ms */
  else
    DPdelayNumSample = 60;	/* 7.5 ms */


  hvxcD = hvxc_decode_init(DPvarMode,
			   DPrateMode,
			   DPextensionFlag,
			   DPdelayMode,
			   DPtestMode);

  hvxcD->frameNumSample = DPframeMaxNumSample;
  hvxcD->delayNumSample = DPdelayNumSample;

  if (DPdebugLevel >= 1) {
    printf("DecHvxcInitNew: decDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   DPdelayMode,DPvarMode,DPrateMode);
  }
  
  if ((hvxcD->sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"DecHvxcInitNew: memory allocation error");
  for (i=0; i<numChannel; i++)
    if ((hvxcD->sampleBuf[i]=(float*)malloc(DPframeMaxNumSample*sizeof(float)))==NULL)
      CommonExit(1,"DecHvxcInitNew: memory allocation error");

  return(hvxcD);
}

/* DecHvxcInit() */
/* Init parametric decoder core. */

HvxcDecStatus *DecHvxcInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  BsBitBuffer *bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample		/* out: decoder delay (num samples) */
  )
{
  int parac;
  char **parav;
  int result;
  BsBitStream *hdrStream;

  HvxcDecStatus *hvxcD;
  
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      DecParInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"decoder parameter string error");
  }

  if (DPdebugLevel >= 1) {
    printf("DecHvxcInit: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecHvxcInit: decPara=\"%s\"\n",
	   decPara);
    printf("DecHvxcInit: debugLevel=%d\n",
	   DPdebugLevel);

    if (DPtestMode != TM_NORMAL) {
      printf("DecHvxcInit: test_mode=%x(for HVXC decoder conformance testing)\n", DPtestMode);
      if (DPtestMode & TM_POSFIL_DISABLE) {
	printf("  - postfilter and post processing are skipped.\n");
      }
      if (DPtestMode & TM_INIT_PHASE_ZERO) {
	printf("  - initial values of harmonic phase are reset to zeros.\n");
      }
      if (DPtestMode & TM_NOISE_ADD_DISABLE) {
	printf("  - noise component addition are disabled.\n");
      }
      if (DPtestMode & TM_VXC_DISABLE) {
	printf("  - the output of Time Domain Decoder is disabled.\n");
      }
    }
  }

  if (numChannel != 1)
    CommonExit(1,"DecHvxcInit: audio data has more the one channel (%d)",
	       numChannel);

  CmdLineParseFree(parav);

  DPfSample = fSample;
  DPbitRate = bitRate;

  hdrStream = BsOpenBufferRead(bitHeader);

  if (BsGetBitInt(hdrStream,(unsigned int*)&DPvarMode,1))
    CommonExit(1,"DecHvxcInit: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPrateMode,2))
    CommonExit(1,"DecHvxcInit: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPextensionFlag,1))
    CommonExit(1,"DecHvxcInit: error reading bit stream header");


  DPframeMaxNumSample = 160 * (int) ceil(1.0 / MINSPEEDFACT);
  
  if (DPdelayMode == DM_LONG)	/* HP 971023 */
    DPdelayNumSample = 80;	/* 10 ms */
  else
    DPdelayNumSample = 60;	/* 7.5 ms */


  hvxcD = hvxc_decode_init(DPvarMode,
			 DPrateMode,
			 DPextensionFlag,
			 DPdelayMode,
			 DPtestMode);
  
  BsClose(hdrStream);

  *frameMaxNumSample = DPframeMaxNumSample;
  *delayNumSample = DPdelayNumSample;

  if (DPdebugLevel >= 1) {
    printf("DecHvxcInit: decDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   DPdelayMode,DPvarMode,DPrateMode);
  }

  return(hvxcD);
}


/* DecHvxcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core. */

void DecHvxcFrame (
  HvxcDecStatus *hvxcD,		/* in: HVXC decoder status handle */
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor(AI 990127) */
  float pitchFact		/* in: pitch change factor(AI 990127) */
  )
{
  BsBitStream *stream;
  int i, j;

  unsigned char tmpIdVUV;
  unsigned char tmpEncBit;

  unsigned char encBit[10];

  if (DPdebugLevel >= 1)
    printf("DecHvxcFrame: availNum=%ld\n",BsBufferNumBit(bitBuf));

  if (speedFact<MINSPEEDFACT) {
    speedFact = MINSPEEDFACT;
    CommonWarning("DecParFrameIl: speedFact below %f",MINSPEEDFACT);
  }
  
  stream = BsOpenBufferRead(bitBuf);

  if (DPdebugLevel >= 2) {
    printf("DecHvxcFrame: stream=%s\n",stream?"valid":"NULL");
    printf("DecHvxcFrame: decMode=");
    switch (DPrateMode) {
    case DEC0K : printf("DEC0K\n"); break;
    case DEC2K : printf("DEC2K\n"); break;
    case DEC3K : printf("DEC3K\n"); break;
    case DEC4K : printf("DEC4K\n"); break;
    default : printf("ERROR!!!\n");
    }
  }

  for (i = 0; i < 5; i++) encBit[i] = 0;
  
  if (DPrateMode == DEC4K || DPrateMode == DEC3K)
	/* Modified on 07/04/97 by Y.Maeda */
  {
    {
      for(j = 0; j < 9; j++)
        if(BsGetBitChar(stream, &encBit[j], 8))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
      if (DPrateMode == DEC4K) {
        if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
      }
      else {
        /* MN 971107 */
        if(BsGetBitChar(stream, &tmpEncBit, 8-6))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
        encBit[9] = (tmpEncBit & 0x3) << 6;
      }
    }
  }
  else if(DPrateMode == DEC2K)
  {
    if(DPvarMode == BM_VARIABLE)
    {
      if(BsGetBitChar(stream, &tmpIdVUV, 2))
	CommonExit(1,"DecParFrameHvx: error reading bit stream");

      encBit[0] = (tmpIdVUV << 6) & 0xc0;

      /* HP 971111 */
      if (DPdebugLevel >= 2)
	printf("DecHvxcFrame: tmpIdVUV=%d\n", tmpIdVUV);

      switch(tmpIdVUV)
      {
      case 0:
	if(BsGetBitChar(stream, &tmpEncBit, 6))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
	encBit[0] |= (tmpEncBit & 0x3f);

	for(j = 1; j < 3; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");

	if(BsGetBitChar(stream, &tmpEncBit, 4))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
	encBit[3] = (tmpEncBit & 0xf) << 4;
	
	if (sysFlag) {	
	  /* when system interface(flexmux) is used, data is byte aligned, */
	  /* then extra reading is necessary(AI 990616) */
	  if(BsGetBitChar(stream, &tmpEncBit, 4))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	}
	
	break;
      case 1:
	/*** reading padded bits necessary(AI 990528) ***/
	if(BsGetBitChar(stream, &tmpEncBit, 6))	
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
	break;
      case 2:
      case 3:
	if(BsGetBitChar(stream, &tmpEncBit, 6))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
	encBit[0] |= (tmpEncBit & 0x3f);

	for(j = 1; j < 5; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	break;
      }
    }
    else	/* DPvarMode == BM_CONSTANT */
    {
      for(j = 0; j < 5; j++)
	if(BsGetBitChar(stream, &encBit[j], 8))
	  CommonExit(1,"DecParFrameHvx: error reading bit stream");
    }
  }

/* scalable mode (YM 990730) */
  if (DPbitRate == 2000.0) DPdecMode = DEC2K;
  else DPdecMode = DPrateMode;

  hvxc_decode(hvxcD, 
	      encBit, 
	      sampleBuf[0], 
	      frameBufNumSample,
	      speedFact,
	      pitchFact,
	      DPdecMode	/* DPrateMode : scalability (YM 990730) */
	      );

  *usedNumBit = BsCurrentBit(stream);
  BsCloseRemove(stream,1);

  if (DPdebugLevel >= 1)
    printf("DecHvxcFrame: numBit=%d  numSample=%d\n",
	   *usedNumBit,*frameBufNumSample);
}


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree(
HvxcDecStatus *hvxcD		/* in: HVXC decoder status handle */
) 
{
  if (DPdebugLevel >= 1)
    printf("DecHvxcFree: ...\n");

  if (sysFlag) {
      free( hvxcD->sampleBuf[0] );
      free( hvxcD->sampleBuf );
  }
  hvxc_decode_free(hvxcD);
}



/* end of dec_par.c */

