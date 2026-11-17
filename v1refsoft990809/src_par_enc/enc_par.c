/**********************************************************************
MPEG-4 Audio VM
Encoder core (parametric)



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



Source file: enc_par.c

$Id: enc_par.c,v 1.36 1999/08/09 16:18:44 purnhage Exp $

Required libraries:
HVXC.a			HVXC library

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bit stream module
indilinextr.o		indiline extraction module
indilineenc.o		indiline bitstream encoder module
( indilinesyn.o		indiline synthesiser module )

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>
MN    Masayuki Nishiguchi, Sony IPC <nishi@pcrd.sony.co.jp>

Changes:
14-jun-96   HP    first version (dummy)
18-jun-96   HP    added bit reservoir handling
24-jun-96   HP    implemented "individual spectral lines"
25-jun-96   HP    using CommonFreeAlloc(), small bug fixes
28-jun-96   HP    joined with HVXC code by Sony IPC
01-jul-96   HP    added HVXC header file, included modifications by IPC
02-jul-96   HP    included modifications by IPC
14-aug-96   HP    added EncParInfo(), EncParFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
10-sep-96   BE
12-sep-96   HP    incorporated indiline modules as source code
26-sep-96   HP    adapted to new indiline module interfaces
26-sep-96   HP    incorporated changes by Sony IPC
04-dec-96   HP    included ISO copyright
10-apr-97   HP    harmonic stuff ...
22-apr-97   HP    noisy stuff ...
15-may-97   HP    clean up
06-jul-97   HP    switch/mix
17-jul-97   HP    "clas" option
23-oct-97   HP    merged IL/HVXC switching
                  swit mode: 2 HVXC frames / 40 ms
07-nov-97   MN    bug-fixes
11-nov-97   HP    added HVXC variable rate switches
12-nov-97   HP    fixed mixed HVXC/IL mode
05-feb-99   HP    marked HILN as Version 2

VMIL stuff
24-jun-97   HP    adapted env/harm/noise interface
16-nov-97   HP    adapted HILN decoder config header
09-apr-98   HP    ILX/ILE config
02-dec-98   HP    new noise quant as dflt.

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "obj_descr.h"		/* AI 990616 */
#include "flex_mux.h"		/* AI 990616 */




#include "enc_par.h"		/* encoder cores */

#include "mp4_par.h"		/* parametric core declarations */

#include "hvxc.h"		/* hvxc module */
#include "hvxcEnc.h"		/* hvxcEnc module */

#include "pan_par_const.h"

/* ---------- declarations ---------- */

#define PROGVER "parametric encoder core DIS V0.1 05-feb-99"

#define SEPACHAR " ,="

#define MAXSAMPLE 32768		/* full scale sample value */

#define MAXNUMBITFACTOR 1.2	/* for indiline bit reservoir control */

#define STRLEN 255



/* ---------- variables ---------- */

/* init parameters (global) */
static float EPfSample;
static float EPbitRate;
static int EPmode;		/* enum MP4ModePar */
static int EPframeNumSample;
static int EPdelayNumSample;

/* sample buffer (frame-to-frame memory) */
static float *EPsampleBuf = NULL;
static float *EPsampleBuf2 = NULL;
static int EPsampleBufSize;


/* extensionFlag for HVXC (AI990209) */
static int EPextensionFlag = 0;

/* command line module */

static char *EPparaMode;
static int EPdelayMode;
static int EPvrMode;
static int EPmixMode;
static int EPdebugLevel;
static int EPdebugLevelIl;
static float EPframeDur;
static int EPmaxNumLine;
static int EPmaxNumLineUsed;
static float EPmaxLineFreq;
static int EPmaxLineFreqUsed;
static int EPmaxNumEnv;
static int EPmaxNumHarm;
static int EPmaxNumHarmLine;
static int EPmaxNumNoisePara;

static int EPtest;
static int EPnewEncDelay;
static int EPfree;

static int EPrateMode;	/* for VR scalable mode (YM 990728) */



int HVXCclassifierMode;

/* HILN encoder control HP971116 */
static int EPenhaFlag;
static float EPbaseRate;
static int EPbaseRateUsed;


static int sysFlag;	/* a flag to use system interface(flexmux) (AI 990616) */

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"dm",&EPdelayMode,"%i","0",NULL,"HVXC delay mode (0=short 1=long)"},
  {"vr",&EPvrMode,"%i","0",NULL,"HVXC variable rate mode (0=fixed 1=var)\n"
   "(set frame work option -vr)"},
  {"dl",&EPdebugLevel,"%i","0",NULL,"debug level"},
  {"-hvxc_sys",&sysFlag,NULL,NULL,NULL,"use system interface(flexmux)"},	/* AI 990616 */
  {"ed",&EPnewEncDelay,"%i","0",NULL,  /* MN 971114 */
   "HVXC only mode encoder delay comp.: 0=0/20ms 1=6/26ms\n\b"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* variables for HVXC */
int ipc_encMode = ENC4K;
int ipc_bitstreamMode = BM_CONSTANT;
int ipc_rateMode = 2;



/* switch/mix stuff */
extern int judge;

#define JUDGE_DLY 124	/* (124+1)*40ms=5s*/
static int numBsBuf;
static int actBsBuf;
static BsBitBuffer *bsBuf[JUDGE_DLY+1];
static BsBitBuffer *bsTmp;
static BsBitStream *bsStream;
static int ILdelayNumSample = 0;
static int HVXdelayNumSample = 0;
static int ILsampleBufSize = 0;
static int HVXsampleBufSize = 0;
static int ILdelayOff = 0;
static int HVXdelayOff = 0;
static BsBitBuffer *dmyBuf;
static int testFrame;

int ipc_encDelayMode = DM_SHORT;

static BsBitBuffer *tmpBitBuf;	/* AI 990616 */

/* HILN */
static float maxNoiseFreq;


/* ---------- internal functions ---------- */




/* Akira 980506 */
#ifdef CELP_LPC_TOOL
typedef struct
{
  PHI_PRIV_TYPE *PHI_Priv;
  /* add private data pointers here for other coding varieties */
}
INST_CONTEXT_LPC_ENC_TYPE;

static INST_CONTEXT_LPC_ENC_TYPE *InstCtxt;
#endif


/* EncParInitHvx() */
/* Init: harmonic vector exitation */

static void EncParInitHvx (
  BsBitStream *hdrStream)	/* out: header for bit stream */
{
#ifdef CELP_LPC_TOOL
  int i;
  long window_sizes[PAN_NUM_ANA_PAR];
  
  for(i=0;i<PAN_NUM_ANA_PAR;i++)
    window_sizes[i] = PAN_WIN_LEN_PAR;
#endif

  EPframeNumSample = 160;	/* 20 ms */
  EPsampleBufSize = 160;	/* 20 ms in buffer EPsampleBuf[] */

  if ( EPmode == MODEPAR_HVX && !EPnewEncDelay){ /* MN 971114 */
    if (ipc_encDelayMode == DM_LONG)
      EPdelayNumSample = 160;
    else
      EPdelayNumSample = 0;
  }
  else {			/* these are the correct values! */
    if (ipc_encDelayMode == DM_LONG)	/* HP 971023 */
      EPdelayNumSample = 208; 	/* 46 ms - 20 ms = 26 ms   HP 971111 */
    else
      EPdelayNumSample = 48; 	/* 26 ms - 20 ms = 6 ms */
  }

  EPrateMode = ipc_rateMode;

  if (!sysFlag) {	/* configuration for raw bitstream(AI 990616) */
    if (BsPutBit(hdrStream,ipc_bitstreamMode,1))
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
    /* if (BsPutBit(hdrStream,ipc_rateMode,2)) */
    if (BsPutBit(hdrStream,EPrateMode,2))	/* YM 990728 */
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
    if (BsPutBit(hdrStream,EPextensionFlag,1))	/* AI 990209 */
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
  }
  
  IPC_HVXCInit();

#ifdef CELP_LPC_TOOL
  /*
  PAN_InitLpcAnalysisEncoder(window_sizes, PAN_NUM_ANA_PAR, 
			     PAN_LPC_ORDER_PAR, PAN_GAMMA_BE_PAR,
			     PAN_BITRATE_PAR);
			     */

  /* Akira 980506 */

  /* -----------------------------------------------------------------*/
  /* Create & initialise private storage for instance context         */
  /* -----------------------------------------------------------------*/
  if (( InstCtxt = (INST_CONTEXT_LPC_ENC_TYPE*)
	malloc(sizeof(INST_CONTEXT_LPC_ENC_TYPE))) == NULL)
    CommonExit(1,"MALLOC FAILURE in celp_initialisation_encoder\n");

  if (( InstCtxt->PHI_Priv = (PHI_PRIV_TYPE*)
	malloc(sizeof(PHI_PRIV_TYPE))) == NULL )
    CommonExit(1,"MALLOC FAILURE in celp_initialisation_encoder\n");
    
  PHI_Init_Private_Data(InstCtxt->PHI_Priv);

  PAN_InitLpcAnalysisEncoder(window_sizes, PAN_NUM_ANA_PAR, 
			     PAN_LPC_ORDER_PAR, PAN_GAMMA_BE_PAR,
			     PAN_BITRATE_PAR, InstCtxt->PHI_Priv);
  /* Akira 980506 */
#endif
}




/* EncParFrameHvx() */
/* Encode frame: harmonic vector exitation */

void EncParFrameHvx (
  float *sampleBuf,
  BsBitStream *stream,		/* out: bit stream */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit)		/* in: max num bits per frame */
{
    int		i;

    short	frmBuf[FRM];
    IdLsp	idLsp;
    int		idVUV;
    IdCelp	idCelp;
    float	mfdpch;
    IdAm	idAm;

    unsigned char encBit[10];	/* for 4kbps(80bits) */
    unsigned int	bit;


    static int ENC3Kflag;

    /* HP 970706 */
    if (frameAvailNumBit >= 80-6) {  /* MN 971106 */
      ipc_encMode = ENC4K;
      if (frameAvailNumBit >= 80){
	ENC3Kflag = 0;
      }
      else{
	ENC3Kflag = 1;
      }
    }
    else{
      ipc_encMode = ENC2K;
    }

    if (EPdebugLevel >= 2) {
      printf("EncParFrameHvx: stream=%s\n",stream?"valid":"NULL");
      printf("EncParFrameHvx: availBit=%d\n",frameAvailNumBit);
      printf("EncParFrameHvx: encMode=");
      switch (ipc_encMode) {
      case ENC2K : printf("ENC2K\n"); break;
      case ENC4K : printf("%s\n",ENC3Kflag?"ENC3K":"ENC4K"); break;
      default : printf("ERROR!!!\n");
      }	
    }

    for(i = 0; i < FRM; i++)
    {
	frmBuf[i] = (short) sampleBuf[i];
    }
    
    IPC_HVXCEncParFrm(frmBuf, &idLsp, &idVUV, &idCelp, &mfdpch, &idAm);
    
    IPC_PackPrm2Bit(&idLsp, idVUV, &idCelp, mfdpch, &idAm, encBit);

    if(ipc_encMode == ENC4K)
    {
      {
        for(i = 0; i < 9; i++) {
	  bit = encBit[i];
	  if(BsPutBit(stream, bit, 8))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
        if (!ENC3Kflag) {
	  bit = encBit[9]; /* MN 971106 */
	  if(BsPutBit(stream, bit, 8))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
        else {
	  /* MN 971107 */
  	  bit = (encBit[9] & 0xc0) >> 6;
	  if(BsPutBit(stream, bit, 8-6))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }

      }
    }
    else if(ipc_encMode == ENC2K)
    {
      if(ipc_bitstreamMode == BM_VARIABLE)
      {
	/* HP 971111 */
	if (EPdebugLevel >= 2)
	  printf("EncParFrameHvx: idVUV=%d  bit=%d\n",
		 idVUV,(encBit[0] & 0xc0) >> 6);

        switch(idVUV)
        {
        case 0:
          for(i = 0; i < 3; i++)
          {
  	      bit = encBit[i];
	      if(BsPutBit(stream, bit, 8))
	        CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }

  	  bit = (encBit[3] & 0xf0) >> 4;
	  if(BsPutBit(stream, bit, 4))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
          break;
        case 1:
  	  bit = (encBit[0] & 0xc0) >> 6;
	  if(BsPutBit(stream, bit, 2))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
          break;
        case 2:
        case 3:
          for(i = 0; i < 5; i++)
          {
  	      bit = encBit[i];
	      if(BsPutBit(stream, bit, 8))
	        CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }
          break;
        }
      }
      else
      {
        for(i = 0; i < 5; i++)
        {
  	    bit = encBit[i];
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
      }
    }
}




/* EncParFreeHvx() */
/* Free memory: harmonic vector exitation */

void EncParFreeHvx ()
{
  /* Akira 980506 */
  PAN_FreeLpcAnalysisEncoder(1, InstCtxt->PHI_Priv);
  /*
  PAN_FreeLpcAnalysisEncoder(1);
  */
  IPC_HVXCFree();
}


/* ---------- functions ---------- */


char *EncParInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "encoder parameter string format:\n"
	    "  list of tokens (tokens separated by characters in \"%s\")\n",
	    SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
  }
  return PROGVER;
}


/* EncParInit() */
/* Init parametric encoder core. */

void EncParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader)	/* out: header for bit stream */
{
  int parac;
  char **parav;
  int result;
  BsBitStream *hdrStream;
  int i;
  int maxCh;

  /* evalute encoder parameter string */
  parav = CmdLineParseString(encPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      EncParInfo(stdout);
      CommonExit(1,"encoder core aborted ...");
    }
    else
      CommonExit(1,"encoder parameter string error");
  }
  EPparaMode = MP4ModeParName[MODEPAR_HVX];

  
  
  if (EPdelayMode)
    ipc_encDelayMode = DM_LONG;
  else
    ipc_encDelayMode = DM_SHORT;

  if (EPvrMode)
    ipc_bitstreamMode = BM_VARIABLE;
  else
    ipc_bitstreamMode = BM_CONSTANT;


  /* AI 990209 */
  ipc_rateMode = ENC4K;		/* 4000 kbps */
  if (bitRate <= 3999.5)
    ipc_rateMode = ENC3K;	/* 3850 kbps */
  if (bitRate <= 3849.5)
    ipc_rateMode = ENC2K;	/* 2000 kbps */

#if 0
  ipc_rateMode = 2;	/* 4000 kbps */
  if (bitRate <= 3999.5)
    ipc_rateMode = 1;	/* 3850 kbps */
  if (bitRate <= 3849.5)
    ipc_rateMode = 0;	/* 2000 kbps */
#endif

  if (EPdebugLevel >= 1) {
    printf("EncParInit: numChannel=%d  fSample=%f  bitRate=%f  "
	   "encPara=\"%s\"\n",
	   numChannel,fSample,bitRate,encPara);
    printf("EncParInit: debugLevel=%d  mode=\"%s\"\n",
	   EPdebugLevel,EPparaMode);
    printf("EncParInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   ipc_encDelayMode,ipc_bitstreamMode,ipc_rateMode);
  }

  EPmode = -1;
  do
    EPmode++;
  while (EPmode < MODEPAR_NUM &&
	 strcmp(EPparaMode,MP4ModeParName[EPmode]) != 0);
  if (EPmode >= MODEPAR_NUM)
    CommonExit(1,"EncParInit: unknown parametric codec mode %s",EPparaMode);

  maxCh = (EPmode == MODEPAR_MIX) ? 2 : 1;
  if (numChannel != maxCh)
    CommonExit(1,"EncParInit: audio data must have %d channel(s) (%d)",
	       maxCh,numChannel);

  CmdLineParseFree(parav);

  EPfSample = fSample;
  EPbitRate = bitRate;

  hdrStream = BsOpenBufferWrite(bitHeader);
  if (BsPutBit(hdrStream,EPmode,2))
    CommonExit(1,"EncParInit: error generating bit stream header");
  switch (EPmode) {
  case MODEPAR_HVX:
    EncParInitHvx(hdrStream);
    break;
  }
  BsClose(hdrStream);

  testFrame = 0;	/* for EPtest!=0 or EPmixMode<0 */

  /* init sample buffer */
  if (CommonFreeAlloc((void**)&EPsampleBuf,
		      EPsampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"EncParInit: memory allocation error");
  for (i=0; i<EPsampleBufSize; i++)
    EPsampleBuf[i] = 0;
  if (EPmode == MODEPAR_MIX) {
    if (CommonFreeAlloc((void**)&EPsampleBuf2,
			EPsampleBufSize*sizeof(float)) == NULL)
      CommonExit(1,"EncParInit: memory allocation error");
    for (i=0; i<EPsampleBufSize; i++)
      EPsampleBuf2[i] = 0;
  }

  *frameNumSample = EPframeNumSample;
  *delayNumSample = EPdelayNumSample;

  if (EPdebugLevel >= 1) {
    printf("EncParInit: modeInt=%d\n",EPmode);
    printf("EncParInit: frmNumSmp=%d  dlyNumSmp=%d  hdrNumBit=%ld\n",
	   *frameNumSample,*delayNumSample,BsBufferNumBit(bitHeader));
    printf("EncParInit: dly=%d  buf=%d  frm=%d\n",
	   EPdelayNumSample,EPsampleBufSize,EPframeNumSample);
    printf("EncParInit: ILdly=%d  ILbuf=%d  ILoff=%d\n",
	   ILdelayNumSample,ILsampleBufSize,ILdelayOff);
    printf("EncParInit: HVXdly=%d  HVXbuf=%d  HVXoff=%d\n",
	   HVXdelayNumSample,HVXsampleBufSize,HVXdelayOff);
  }
}


/* EncParFrame() */
/* Encode one audio frame into one bit stream frame with */
/* parametric encoder core. */

void EncParFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit)		/* in: max num bits per frame */
{
  BsBitStream *stream;
  int i;
  int hvxcFrame1;
  int mixMode;
  int ilBits;

  if (EPdebugLevel >= 1)
    printf("EncParFrame: %s  availNum=%d  num=%d  maxNum=%d\n",
	   (bitBuf==NULL)?"start up":"normal",
	   frameAvailNumBit,frameNumBit,frameMaxNumBit);

  /* update sample buffer */
  for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
    EPsampleBuf[i] = EPsampleBuf[i+EPframeNumSample];
  for (i=0; i<EPframeNumSample; i++)
    EPsampleBuf[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[0][i];
  if (EPmode == MODEPAR_MIX) {
    for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
      EPsampleBuf2[i] = EPsampleBuf2[i+EPframeNumSample];
    for (i=0; i<EPframeNumSample; i++)
      EPsampleBuf2[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[1][i];
  }

  if (bitBuf)
    stream = BsOpenBufferWrite(bitBuf);
  else
    stream = NULL;
  switch (EPmode) {
  case MODEPAR_HVX:
    if (bitBuf)
      EncParFrameHvx(EPsampleBuf,stream,
		     frameAvailNumBit,frameNumBit,frameMaxNumBit);
    break;
  }
  if (bitBuf==NULL)
    /* encoder start up */
    return;
  BsClose(stream);

  if (EPdebugLevel >= 1)
    printf("EncParFrame: numBit=%ld\n",BsBufferNumBit(bitBuf));

}


/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree ()
{
  if (EPdebugLevel >= 1)
    printf("EncParFree: ...\n");

  if (!EPfree)
    return;
  if (EPmode != MODEPAR_IL)
    EncParFreeHvx();
}


/* EncHvxcInit() */
/* Init HVXC encoder core. */

void EncHvxcInit(
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  BsBitBuffer *bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData	/* out: system interface(AI 990616) */
  )
{
  int parac;
  char **parav;
  int result;
  BsBitStream *hdrStream;
  int i;
  int maxCh;

  int layer, numLayer;	/* AI 990616 */

  /* evalute encoder parameter string */
  parav = CmdLineParseString(encPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      EncParInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"encoder parameter string error");
  }

  if (strstr(encPara, "-hvxc_sys") != NULL) 
    sysFlag = 1;	/* use system interface(flexmux) (AI 990616) */
  
  EPparaMode = MP4ModeParName[MODEPAR_HVX];

  if (EPdelayMode)
    ipc_encDelayMode = DM_LONG;
  else
    ipc_encDelayMode = DM_SHORT;

  if (EPvrMode)
    ipc_bitstreamMode = BM_VARIABLE;
  else
    ipc_bitstreamMode = BM_CONSTANT;


  ipc_rateMode = ENC4K;		/* 4000 kbps */
  if (bitRate <= 3999.5)
    ipc_rateMode = ENC3K;	/* 3850 kbps */
  if (bitRate <= 3849.5)
    ipc_rateMode = ENC2K;	/* 2000 kbps */

#if 0
  ipc_rateMode = 2;	/* 4000 kbps */
  if (bitRate <= 3999.5)
    ipc_rateMode = 1;	/* 3850 kbps */
  if (bitRate <= 3849.5)
    ipc_rateMode = 0;	/* 2000 kbps */
#endif

  EPrateMode = ipc_rateMode;

  if (EPdebugLevel >= 1) {
    printf("EncHvxcInit: numChannel=%d  fSample=%f  bitRate=%f  "
	   "encPara=\"%s\"\n",
	   numChannel,fSample,bitRate,encPara);
    printf("EncHvxcInit: debugLevel=%d  mode=\"%s\"\n",
	   EPdebugLevel,EPparaMode);
    printf("EncHvxcInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   ipc_encDelayMode,ipc_bitstreamMode,ipc_rateMode);
  }

  EPmode = 0;	/* HVXC only */
  
  CmdLineParseFree(parav);

  EPfSample = fSample;
  EPbitRate = bitRate;

  hdrStream = BsOpenBufferWrite(bitHeader);

  /* system interface(flexmux) configuration (AI 990616) */
  /* only single HVXC layer(layer=0) */
  if (sysFlag) {
    numLayer = 1;
    initObjDescr(frameData->od);
    presetObjDescr(frameData->od, numLayer-1);
    frameData->od->streamCount.value=numLayer;
    
    for (layer=0; layer<numLayer; layer++) {
      initESDescr(&(frameData->od->ESDescriptor[layer]));
      presetESDescr(frameData->od->ESDescriptor[layer], layer);

      /* elementary stream configuration */
      /* DecConfigDescriptor configuration */
      frameData->layer[layer].bitRate = bitRate;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = frameData->layer[layer].bitRate;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value = 0;
      
      /* ALConfigDescriptor configuration */
#ifdef AL_CONFIG        
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;
#endif
      /* AudioSpecificConfig configuration */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = 5;	/* HVXC core, actual ObjectTypeID = 9 ? */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0xb;	/* 8000Hz */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;	/* mono */
      
      /* hvxcSpecificConfig configuration */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value = ipc_bitstreamMode;	/* HVXCvarMode */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value = EPrateMode;	/* HVXCrateMode */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag.value = EPextensionFlag;	/* extensionFlag */
    }

    advanceODescr(hdrStream, frameData->od, 1);

    for (layer=0; layer<numLayer; layer++){ 
      advanceESDescr(hdrStream, frameData->od->ESDescriptor[layer], 1);
    }
    
    tmpBitBuf = BsAllocBuffer(80);	/* temporary bitBuf for flexmux(AI 990616) */
  }

  EncParInitHvx(hdrStream);
  BsClose(hdrStream);

  /* init sample buffer */
  if (CommonFreeAlloc((void**)&EPsampleBuf,
		      EPsampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"EncHvxcInit: memory allocation error");
  for (i=0; i<EPsampleBufSize; i++)
    EPsampleBuf[i] = 0;

  *frameNumSample = EPframeNumSample;
  *delayNumSample = EPdelayNumSample;

  if (EPdebugLevel >= 1) {
    printf("EncHvxcInit: modeInt=%d\n",EPmode);
    printf("EncHvxcInit: frmNumSmp=%d  dlyNumSmp=%d  hdrNumBit=%ld\n",
	   *frameNumSample,*delayNumSample,BsBufferNumBit(bitHeader));
    printf("EncHvxcInit: dly=%d  buf=%d  frm=%d\n",
	   EPdelayNumSample,EPsampleBufSize,EPframeNumSample);
    printf("EncHvxcInit: HVXdly=%d  HVXbuf=%d  HVXoff=%d\n",
	   HVXdelayNumSample,HVXsampleBufSize,HVXdelayOff);
  }
}

/* EncHvxcFrame() */
/* Encode one audio frame into one bit stream frame with */
/* HVXC encoder core. */

void EncHvxcFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  BsBitBuffer *bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  ENC_FRAME_DATA* frameData	/* in/out: system interface(AI 990616) */
  )
{
  BsBitStream *stream;
  int i;

  if (EPdebugLevel >= 1)
    printf("EncParFrame: %s  availNum=%d  num=%d  maxNum=%d\n",
	   (bitBuf==NULL)?"start up":"normal",
	   frameAvailNumBit,frameNumBit,frameMaxNumBit);

  /* update sample buffer */
  for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
    EPsampleBuf[i] = EPsampleBuf[i+EPframeNumSample];
  for (i=0; i<EPframeNumSample; i++)
    EPsampleBuf[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[0][i];

  if (bitBuf) {
    if (sysFlag) {	/* for flexmux bitstream, use writeFlexMuxPDU() */
      /* firstly, audio data are stored in tmpBitBuf temporarily */
      stream = BsOpenBufferWrite(tmpBitBuf);
      EncParFrameHvx(EPsampleBuf,stream,
		     frameAvailNumBit,frameNumBit,frameMaxNumBit);
    
      BsClose(stream);
      /* secondly, (Header+Payload) are output in bitBuf */
      stream = BsOpenBufferWrite(bitBuf);
      writeFlexMuxPDU(0 , stream , tmpBitBuf);
      BsClose(stream);
    }
    else {	/* for raw bitstresm, output directly */
      stream = BsOpenBufferWrite(bitBuf);
      EncParFrameHvx(EPsampleBuf,stream,
		     frameAvailNumBit,frameNumBit,frameMaxNumBit);
    
      BsClose(stream);
    }
  }
  else {
    /* encoder start up */
  }

  if (EPdebugLevel >= 1)
    printf("EncParFrame: numBit=%ld\n",BsBufferNumBit(bitBuf));
}

/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncHvxcFree ()
{
  if (EPdebugLevel >= 1)
    printf("EncHvxcFree: ...\n");

  EncParFreeHvx();
}



/* end of enc_par.c */

