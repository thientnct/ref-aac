/********************************************************************** 
MPEG-4 Audio VM
Decoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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



Source file: mp4dec.c


Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bit stream module
audio.o                 audio i/o module
dec_par.o               decoder core (parametric)
dec_lpc.o               decoder core (CELP)
dec_tf.o                decoder core (T/F)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
CCETT N.N., CCETT <@ccett.fr>
OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>

Changes:
18-jun-96   HP    first version
19-jun-96   HP    added .wav format / using new ComposeFileName()
26-jun-96   HP    improved handling of switch -o
04-jul-96   HP    joined with t/f code by BG
09-aug-96   HP    adapted to new cmdline module, added mp4.h
16-aug-96   HP    adapted to new dec.h
                  added multichannel signal handling
                  added cmdline parameters for numChannel, fSample
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
30-oct-96   HP    additional frame work options
15-nov-96   HP    adapted to new bitstream module
18-nov-96   HP    changed int to long where required
                  added bit stream header options
10-dec-96   HP    added variable bit rate
10-jan-97   HP    using BsGetSkip()
23-jan-97   HP    added audio i/o module
03-feb-97   HP    audio module bug fix
07-feb-97   NT    added PICOLA speed control
14-mar-97   HP    merged FhG AAC code
21-mar-97   BT    various changes (t/f, AAC)
04-apr-97   HP    new option -r for scalable decoder
26-mar-97   CCETT added G729 decoder
07-apr-97   HP    i/o filename handling improved / "-" supported
05-nov-97   HP    update by FhG/UER
30-mar-98   HP    added ts option
06-may-98   HP    aacEOF
07-jul-98   HP    improved bitrate statistics debug output
02-dec-98   HP    merged most FDIS contributions ...
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
		  code and/or its readability -- sorry ...
21-jan-99   HP	  trying to clean up a bit ...
22-jan-99   HP    stdin/stdout "-" support, -d 1 uses stderr
22-apr-99   HP    merging all contributions
19-nov-99   HP    added MOUSECHANGE
19-jun-01   OK    removed G723/G729 dummy calls
**********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "adif.h"

#include "common_m4a.h"          /* common module       */
#include "cmdline.h"             /* command line module */
#include "bitstream.h"           /* bit stream module   */
#include "audio.h"               /* audio i/o module    */

#include "mp4au.h"               /* frame work common declarations */

#include "flex_mux.h"
#include "lpc_common.h"

#include "hvxc_struct.h"	/* struct for HVXC(AI 990616) */

#include "dec_par.h"            /* decoder cores ... */
#include "dec_lpc.h"
#include "dec_tf.h"
#include "plotmtv.h"

#include "pan_picola.h"          /* for speed control (PICOLA) */

#ifdef VERSION2
#include "resilience.h"
#include "statistics.h"
#endif

#include "buffers.h"

#ifdef MP4FF
#include "mp4ifc.h"
#endif

#ifdef MOUSECHANGE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif


/* ---------- declarations ---------- */

#define PROGVER "MPEG-4 Natural Audio Decoder (Edition 2001)   2003-08-27"
#define CVSID "$Id: mp4audec.c,v 1.50 2003/08/27 23:15:57 ct1 Exp $"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ---------- variables ---------- */

static char *MP4ModeName[] = {MODENAME_UNDEF, 
                              MODENAME_PAR, 
                              MODENAME_LPC,
                              MODENAME_TF, 
                              MODENAME_HVXC};

/* command line module */

static float decBitRate;
static int   decBitRateUsed;
static int   skipPaddingBits;
static int   numChannelOut;
static float fSampleOut;
static float speedFact;
static float pitchFact;
static float regionStart;
static float regionDurat;
static int   regionDuratUsed;
static char* decPara;
static char* outFileName;
static int   outFileNameUsed;
static int   loopInFile;
static char* bitPath;
static char* decPath;
static int   bitPathUsed;
static int   decPathUsed;
static char* bitExt;
static int   int24flag;
static char* decExt;
static int   bitExtUsed,decExtUsed;
static int   testBitStream = 0;
static int   bitBufSize;
static int   mainDebugLevel;
static int   audioDebugLevel;
static int   bitDebugLevel;
static int   cmdDebugLevel;
static char* aacDebugString; /* Debug options for aac */
static int   epDebugLevel;
static int*  varArgIdx;



#ifdef VERSION2
#define INF_FILE_LEN 1024
static int   epFlag;
static char  infoFileName[INF_FILE_LEN];/* for EP mode only */
#endif /* VERSION2 */

/* memory for following data should be allocated dynamically in the future */
static FRAME_DATA frameData;
static TF_DATA tfData;
static LPC_DATA  lpcData;
static ntt_DATA nttData;
static T_DECODERGENERAL faultData;
static HANDLE_DECODER_GENERAL hFault =  &faultData;

static HvxcDecStatus *hvxcData;		/* HVXC decoder handle (AI 990616) */
static ParaDecStatus *paraData;		/* PARA/HILN decoder handle HP20000330 */

#ifdef DEBUGPLOT
extern int framePlot; /* global for debugging purposes */
extern int firstFrame;
#endif
static int frame;

static int aacEOF = 0;		/* HP 980506   AAC bitstream EOF detection */

extern int samplFreqIndex[];	/* undesired, but ... */

/* for speed control (PICOLA) */
static int SpeedControlMode;

static CmdLinePara paraList[] = {
  {&varArgIdx,NULL,"<bit stream file(s) (- = stdin)>"},
  {NULL,NULL,NULL}
};

static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"r",&decBitRate,"%f",NULL,&decBitRateUsed,
   "decoder bit rate for scalability [bit/s]\n"
   "(dflt: encoder bit rate)"},
#ifndef PROPER_PADDING
  {"spb",&skipPaddingBits,"%d","0",NULL,"skip padding bits"},
#else
  {"spb",&skipPaddingBits,"%d","1",NULL,"skip padding bits"},
#endif
  {"n",&numChannelOut,"%d","0",NULL,"number of audio channels "
   "(0=as original)"},
  {"s",&fSampleOut,"%f","0",NULL,"sampling frequency [Hz] "
   "(0=as original)"},
#ifdef VERSION2
  {"ep",&epFlag,NULL,NULL,NULL,
   "enables error protection (requires error resilient syntax)"},
#endif /*VERSION2*/
  {"sf",&speedFact,"%f","1",NULL,"speed change factor"},
  {"pf",&pitchFact,"%f","1",NULL,"pitch change factor"},
  {"c",&decPara,"%s","",NULL,
   "decoder parameter string"},
  {"ts",&regionStart,"%f","0",NULL,"start time of decoded region [sec]\n"
   "[is broken   HP990811]"},
  {"td",&regionDurat,"%f",NULL,&regionDuratUsed,
   "duration of decoded region [sec]\n"
   "(dflt: until end of bit stream)"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,
   "output file name (- = stdout)"},
  {"li",&loopInFile,"%d","1",NULL,
   "loop input files n times (use with -o -)"},
  {"pb",&bitPath,"%s",NULL,&bitPathUsed,
   "default path bit stream files (dflt: $" MP4_BIT_PATH_ENV ")"},
  {"pd",&decPath,"%s",NULL,&decPathUsed,
   "default path decoded audio files (dflt: $" MP4_DEC_PATH_ENV ")"},
  {"eb",&bitExt,"%s",MP4_BIT_EXT,&bitExtUsed,"bit stream file extension"},
  {"ed",&decExt,"%s",NULL,&decExtUsed,
   "decoded audio file extension\n"
#ifdef USE_AFSP
   "supported file formats: .au, .snd, .wav, .aif, .raw\n"
   "(dflt: $" MP4_DEC_FMT_ENV " / " MP4_DEC_EXT ")\n"
   "(raw format: integer16, native)"},
  {"int24",&int24flag,NULL,NULL,NULL,
  "output is: two's-complement 24-bit integer data\n"
  "(dflt: two's-complement 16-bit integer data)"
#else
   "supported file formats: .au, .snd\n"
   "(dflt: $" MP4_DEC_FMT_ENV " / " MP4_DEC_EXT ")\n"
   "(raw format: integer16, native)"
#endif /*USE_AFSP*/
  },
  {"t",&testBitStream,NULL,NULL,NULL,"test bit stream (decoder disabled)"},
  {"bb",&bitBufSize,"%i",def2str(MAXREADAHEAD),NULL,
   "bitstream buffer size [bit]"},
  {"d",&mainDebugLevel,"%i","0",NULL,"main debug level"},
#ifdef DEBUGPLOT
  {"ps",&firstFrame,"%i","9999",NULL,"start plot at frame%"},
#endif
  {"da",&audioDebugLevel,"%i","0",NULL,"audio file debug level"},
  {"db",&bitDebugLevel,"%i","0",NULL,"bit stream debug level"},
  {"dc",&cmdDebugLevel,"%i","0",NULL,"cmd line debug level"},
  {"daac",&aacDebugString,"%s","",NULL,"[a-Z] Debug Options for aac-Decoder"},
  {"dep",&epDebugLevel,"%i","0",NULL,"error protection debug level"},
  {"picola", &SpeedControlMode, NULL, NULL, NULL, "speed control by PICOLA"},
  {"aaceof", &aacEOF, NULL, NULL, NULL, "AAC bitstream end detection"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* ---------- functions ---------- */

#ifdef MOUSECHANGE

/*static*/ Display *MOUSEdisp = NULL;
static Window MOUSEwin;
static int MOUSEscreen;
static float MOUSEspeed,MOUSEpitch;
static int MOUSEoff,MOUSEx,MOUSEy;

/* negative speed/pitch factor to enable "mousechange" */
/* dflt. range 0..abs(xxxFactor) (dflt. 2) */

static void InitMouse(float speedFactor, float pitchFactor)
{
  Window rwin,cwin;
  int mcx,mcy,xsize,ysize;
  unsigned int but;

  MOUSEdisp = NULL;
  if (speedFactor>=0 && pitchFactor>=0)
    return;
  MOUSEdisp = XOpenDisplay("");
  MOUSEwin = DefaultRootWindow(MOUSEdisp);
  MOUSEscreen = DefaultScreen(MOUSEdisp);
  XQueryPointer(MOUSEdisp,MOUSEwin,&rwin,&cwin,&MOUSEx,&MOUSEy,&mcx,&mcy,&but);
  if (speedFactor<0)
    speedFactor = -speedFactor;
  else
    speedFactor = 2;
  if (pitchFactor<0)
    pitchFactor = -pitchFactor;
  else
    pitchFactor = 2;
  MOUSEspeed = DisplayWidth(MOUSEdisp,MOUSEscreen)/speedFactor;
  MOUSEpitch = DisplayHeight(MOUSEdisp,MOUSEscreen)/pitchFactor;
  MOUSEoff = DisplayHeight(MOUSEdisp,MOUSEscreen);
  XWarpPointer(MOUSEdisp,(Window)0,MOUSEwin,0,0,0,0,
	       (int)(MOUSEspeed+.5),MOUSEoff-1-(int)(MOUSEpitch+.5));
}

/*static*/ void GetMouse(float *speed, float *pitch)
{
  Window rwin,cwin;
  int mrx,mry,mcx,mcy,xsize,ysize;
  unsigned int but;

  if (!MOUSEdisp)
    return;
  XQueryPointer(MOUSEdisp,MOUSEwin,&rwin,&cwin,&mrx,&mry,&mcx,&mcy,&but);
  *speed = mrx/MOUSEspeed;
  *pitch = (MOUSEoff-1-mry)/MOUSEpitch;
}

static void FreeMouse()
{
  if (MOUSEdisp) {
    XWarpPointer(MOUSEdisp,(Window)0,MOUSEwin,0,0,0,0,MOUSEx,MOUSEy);
    XCloseDisplay(MOUSEdisp);
  }
}

#endif


static void    flexMuxDecInit ( int*         numChannel,                               
                                char*        decPara,     /* in: decoder parameter string */
                                FRAME_DATA*  fD,          /* config data ,one bitstream buffer for each layer, 
                                                             obj descr. etc. */
                                TF_DATA*     tfD,
                                LPC_DATA*    lpcD,
                                ntt_DATA*    nttData,
				HvxcDecStatus**	hvxcD,	/* AI 990616 */
				ParaDecStatus**	paraD,	/* HP20000330 */
#ifdef VERSION2
                                char*        audioFileName,
#endif /*VERSION2*/
                                float*       fSampleOut )

{
  unsigned int layer,numLayer;
  float fSample;
  int   tfInit = 0;
  int   celpInit = 0;
  int   hvxcInit = 0;
  int   paraInit = 0;
  char* outLayNr;
  /*   float delayNumSample; */
  /*   float decBitRate; */
  /*   int   frameMaxNumSample; */

  /* To avoid Heisenbugs or "inverse" Heisenbugs, it might be worthwhile
     to initialise variables _before_ they are used for the first time!
     HP20001025 */
  fD->scalOutSelect = fD->od->streamCount.value-1;
  if ( ( outLayNr = strstr( decPara, "-out") ) ){
    outLayNr += 4;
    if (*outLayNr=='=' || *outLayNr==',')
      outLayNr++;
    sscanf(outLayNr,"%ud",&(fD->scalOutSelect));
    if ( fD->scalOutSelect>10 ) {
      CommonExit(-1,"error in output layer select %d  ",fD->scalOutSelect);
    } 
    if ( mainDebugLevel ) {
      fprintf(stderr,"layer nr. %d is selected \n",fD->scalOutSelect);
    }
  }

#ifndef EP_CONFIG_MISSING
  /* HP20010214 */
  if (fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value)
    CommonWarning("flexMuxDecInit: epConfig!=0 not yet supported for .fl4");
#endif

  if (fD->scalOutSelect>=fD->od->streamCount.value)
    CommonWarning("selected layer %d, but only layer 0..%d available",
		  fD->scalOutSelect,fD->od->streamCount.value-1);
  fD->scalOutSelect = min(fD->scalOutSelect+1,fD->od->streamCount.value)-1;
  numLayer=fD->scalOutSelect+1;
  if (fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
    *fSampleOut = fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;
  else
    *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];

  if (fD->scalOutSelect == 0 ) {
    if ((strstr(decPara, "-celp_mdct") == NULL)){    
      /* write only core signal at core sampling rate */
      *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
      /* HP20001012 samplingFreqencyIndex==0xf not handled */
    } else {
      numLayer++; /* for debugging purposes: init tf layer as well */
    } 
  } else if ( fD->od->ESDescriptor[fD->scalOutSelect]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == CELP ) {
    *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[fD->scalOutSelect]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
    /* HP20001012 samplingFreqencyIndex==0xf not handled */
#ifdef VERSION2
  } else if ( fD->od->ESDescriptor[fD->scalOutSelect]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_CELP ) {
    *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[fD->scalOutSelect]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
    /* HP20001012 samplingFreqencyIndex==0xf not handled */
#endif
  }

  for ( layer = 0; layer < numLayer; layer++ ){

    AUDIO_SPECIFIC_CONFIG *audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;
    if (fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
      fSample = fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;
    else 
      fSample = samplFreqIndex[fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];

    fD->layer[layer].bitBuf = BsAllocBuffer(MAX_BITBUF); /* currently in dec_tf.c, line 529 */
    fD->layer[layer].AULength = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));
 
    switch (audSpC->channelConfiguration.value) {
    case 1 :*numChannel=1;       
      break;
    case 2 :*numChannel=2;       
      break;
    default: CommonExit(1,"wrong channel config");
    }
    switch (audSpC->audioDecoderType.value) {

#ifdef VERSION2	
    case ER_HILN:	
    case ER_PARA:
      if (paraInit==0){ /* call it only once for all hiln/para layers */
        paraInit=1;
	*paraD = DecParInitNew(decPara,
			       fD,
			       layer,
			       epFlag, 
			       epDebugLevel, 
			       infoFileName, 
			       &hFault->hEscInstanceData
			       );
      }
      break;
#endif

    case HVXC:	
#ifdef VERSION2	
    case ER_HVXC:
#endif
      if (hvxcInit==0){ /* call it only once for all hvxc layers */
        hvxcInit=1;
	*hvxcD = DecHvxcInitNew(decPara,
				fD,
				layer
#ifdef VERSION2		/* appended by Sony on 990616 */
				,epFlag, 
				epDebugLevel, 
				infoFileName, 
				&hFault->hEscInstanceData
#endif
				);
      }
      break;
      
#ifdef VERSION2	
    case ER_CELP:
#endif
    case CELP:
      if (celpInit==0){ /* call it only once for all celp layers */
        celpInit=1;
	DecLpcInitNew(decPara,
		      fD,
		      lpcD
		      );
      }
      break;
    case AAC_SCAL:
    case TWIN_VQ:
#ifdef VERSION2	
    case ER_BSAC:
    case ER_TWIN_VQ:
#endif
      if (!aacEOF) {
        CommonWarning("Decode: option '-aaceof' is recommended in AAC mode");
      }
      if (tfInit==0){ /* call it only once for all aac layers */
        tfInit=1;
        DecTfInitNew ( 
                      decPara,
                      aacDebugString,
                      hFault,
                      fD,
                      tfD,
                      lpcD,
                      nttData,
                      mainDebugLevel,
                      layer
#ifdef VERSION2
                      ,epDebugLevel,
                      infoFileName,
                      audioFileName
#endif
                      );
      }
      break;
    default : CommonExit(1,"unknown decoder type");
    }
  }   /* for (layer= ...) */
}

static void    flexMuxDec(HANDLE_BSBITSTREAM  bitStream,
                          int          numChannel, 
                          FRAME_DATA*  fD, /* config data ,one bitstream buffer for each layer, 
                                              obj descr. etc. */
                          TF_DATA*     tfD,
                          LPC_DATA*    lpcD,                         
                          ntt_DATA*    nttData,
			  HvxcDecStatus* hvxcD,	/* AI 990616 */
			  ParaDecStatus* paraD,	/* HP20000330 */
                          AudioFile*   audioFile,
                          HANDLE_DECODER_GENERAL hFault                
                          )
{
  int i_ch,numBits;
  unsigned int i;
  unsigned int layer;
  unsigned int lastLayer = fD->od->streamCount.value-1 ;
  unsigned int audioDecType,coreDecType ,decType ;
  unsigned int haveAllAU;
  int          eofFlag=0;
  /* loop while bit in bitstream */

  if (  fD->od->ESDescriptor[lastLayer]->DecConfigDescr.streamType.value != 5){
    CommonWarning("not a audio object");
    if (  fD->od->ESDescriptor[lastLayer]->DecConfigDescr.streamType.value == 6)
      CommonWarning("old style flex mux stream with wrong streamType = 6");
    else
      CommonExit(1,"not a audio object");
  }      

 
  audioDecType =  fD->od->ESDescriptor[lastLayer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  coreDecType =  fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  decType= audioDecType;
  lastLayer=fD->scalOutSelect;
  if (tfD && (tfD->output_select==0) ) {
    if ((strstr(decPara, "-celp_mdct") == NULL)){    
      decType= coreDecType;    /* write only core signal at core sampling rate */
    } else {
      CommonWarning("output of core at hightlayer sampling rate just for debugging purposes");
      /*       lastLayer++; */
    }
    
  }
  
  for ( i=0; i<=lastLayer; i++ ){
    fD->layer[i].NoAUInBuffer=0;/* each decoder must decrease this by the number of decoded AU */
  }
  
  while ( ! BsEof ( bitStream, 8)) {
    /* read until next frame of highest layer 
       and fill bitstream buffers for each layer*/
    int debugCnt=0;
#ifdef FHG_DEBUGPLOT
    plotInit(framePlot,BsGetFileName(bitStream),0);
#endif
    
    haveAllAU=1;
    for ( i=0; i<=lastLayer; i++ ){
      /* does not yet really work if more than one celp AU is needed to decode one
         combined scalable tf frame ,
         but it will work as long as celp AU are transmitted/stored bevor corresponding AAC AUs */
      haveAllAU = haveAllAU &  ( (fD->layer[i].NoAUInBuffer>0)?1:0) ;
    }      
    while((!haveAllAU) && (eofFlag==0)) {
      eofFlag = nextAccessUnit( bitStream, /* read directly from file */
                                &layer, /* output : No. of current layer to which this AU belongs to */
                                fD);

      debugCnt++;
      if (debugCnt>19) CommonExit(1,"access unit not found "); 
      /* up to 8 layers are possible, but in case of Celp+AAC up to 12 Celp frame belongs 
         to one AAC frame; 
         (total num of scal. layers <=8; one  celp layer (with 12 frames per aac-frame) plus 7 aac layers:
         12+7=19 AUs for one frame)
         this loop assumes that data of all layer are interleaved , order of the AUs is free .
         but if AU that belongs to one frame are not transmitted/stored together buffer overflow could occure 
      */                 
      haveAllAU=1;
      for ( i=0; i<=lastLayer; i++ ){
        /* does not yet really work if more than one celp AU is needed to decode one
           combined scalable tf frame ,
           but it will work as long as celp AU are transmitted/stored bevor corresponding AAC AUs */
        haveAllAU = haveAllAU &  ( (fD->layer[i].NoAUInBuffer>0)?1:0) ;
      }      
    }  
    if (eofFlag==0) {
      if ((mainDebugLevel == 1) && (frame % 20 == 0)) {
	fprintf(stderr,"\rframe %4d ",frame);
	fflush(stderr);
      }
      if (mainDebugLevel >= 2 && mainDebugLevel <= 3) {
	printf("\rframe %4d ",frame);
	fflush(stdout);
      }
      if (mainDebugLevel > 3)
	printf("frame %4d\n",frame);
      frame++;

#ifdef MOUSECHANGE
      GetMouse(&speedFact,&pitchFact);
      if (MOUSEdisp) {
	if (mainDebugLevel == 1)
	  fprintf(stderr,"speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
	if (mainDebugLevel >= 2)
	  printf("speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
      }
#endif

      /* call actual decoding functions */
      switch (decType) {
        
      case AAC_SCAL:
      case TWIN_VQ:
#ifdef VERSION2	
      case ER_BSAC:
      case ER_TWIN_VQ:
#endif
        DecTfFrameNew(numChannel,
                      fD,
                      tfD,
                      lpcD,
                      nttData,
                      hFault ,
                      AAC_SCALABLE);
        
        /* convert time data from double to float */
        for (i_ch=0; i_ch<numChannel; i_ch++){
          /* needed to prevent clipping for a 0dB sine wave, tmn990507*/
/*           double fact=((double)32767.0/(double)37594.0);  */
          for( i=0; i<(unsigned int)tfD->block_size_samples; i++ ) {
/*          tfD->sampleBuf[i_ch][i] = (float)((tfD->time_sample_vector[i_ch][i])*fact); */
            tfD->sampleBuf[i_ch][i] = (float)((tfD->time_sample_vector[i_ch][i]));
          }
        }
        AudioWriteData(audioFile,tfD->sampleBuf,tfD->block_size_samples);
      
        break;
#ifdef VERSION2	
      case ER_CELP:
#endif
      case CELP :
        DecLpcFrameNew(fD, lpcD, &numBits);
        AudioWriteData(audioFile,lpcD->sampleBuf,lpcD->frameNumSample);  
        break;
        
      case HVXC:
#ifdef VERSION2	
      case ER_HVXC:
#endif
      if(SpeedControlMode) {
	DecHvxcFrameNew(fD,
			hvxcD,
			&numBits,
			1,
			pitchFact
#ifdef VERSION2		
			,epFlag,
			hFault->hEscInstanceData
#endif
		     );
      }
      else {
	DecHvxcFrameNew(fD,
			hvxcD,
			&numBits,
			speedFact,
			pitchFact
#ifdef VERSION2		
			,epFlag,
			hFault->hEscInstanceData
#endif
		     );
      }
      AudioWriteData(audioFile,hvxcD->sampleBuf,hvxcD->frameNumSample);  
      break;
      
#ifdef VERSION2	
      case ER_HILN:
      case ER_PARA:
      if(SpeedControlMode) {
	DecParFrameNew(fD,
		       paraD,
		       &numBits,
		       1,
		       pitchFact,
		       epFlag,
		       hFault->hEscInstanceData
		       );
      }
      else {
	DecParFrameNew(fD,
		       paraD,
		       &numBits,
		       speedFact,
		       pitchFact,
		       epFlag,
		       hFault->hEscInstanceData
		       );
      }
      AudioWriteData(audioFile,paraD->sampleBuf,paraD->frameNumSample);  
      break;
#endif
      
    default :
      CommonExit(1," streamType not yet supported ");
      break;
    }

#ifdef DEBUGPLOT    
      framePlot++;
      plotDisplay(0);
#endif
    }
  }
  /* endloop , bitstream is empty */
}


/* Decode() */
/* Decode bit stream and generate audio file. */
/* (This function evaluates the global xxxDebugLevel variables !!!) */

static int Decode (
                   char *bitFileName,            /* in: bit stream file name */
                   char *audioFileName,          /* in: audio file name */
                   char *audioFileFormat,        /* in: audio file format (au, wav, aif, raw) */
                   float decBitRate,             /* in: decoder bit rate (0=as encoder) */
                   int   testBitStream,          /* in: 0=normal decoding 
                                                    1=test bit stream (disable decoder) */
                   int   numChannelOut,          /* in: num audio channels */
                   float fSampleOut,             /* in: sampling frequency [Hz] */
                   float speedFact,              /* in: speed change factor */
                   float pitchFact,              /* in: pitch change factor */
                   char *decPara,                /* in: decoder parameter string */
                   float regionStart,	 	/* in: start time of region */
                   float regionDurat             /* in: duration of region */
#ifdef VERSION2
                   ,int   epFlag,                /* in: error protection flag */
                   char  *infoFileName           /* in: info file name */
#endif
                   )
                                /* returns: 0=OK  1=error */
{
  unsigned int       x;
  int                mode;                    /* enum MP4Mode */
  float              fSample,fSampleOri,bitRate;
  long               totNumSample;
  int                numChannel,numChannelOri;
  int                frameMaxNumSample,frameBufNumSample,delayNumSample;
  int                frameMinNumBit,frameMaxNumBit;
  long               fSampleLong,bitRateLong;
  char*              info;
  char*              codecMode;
  int                bitReservSize;
  int                bitReservInit;
  int                bitHeaderNumBit;
  ADIF_INFO          adifInfo;
  HANDLE_BSBITBUFFER        bitBuf = NULL;
  float**            sampleBuf = NULL;
  HANDLE_BSBITSTREAM        bitStream;
  AudioFile*         audioFile = NULL;
  HANDLE_BSBITBUFFER        bitHeader;
  int                numSample;
  int                frameNumBit,frameAvailNumBit,usedNumBit;
  int                frameDecNumBit;
  int                headerNumBit;
  long               totDataNumBit,totPadNumBit;
  int                ch;
  int                bsVersion;
  int                decNumFrame;
  OBJECT_DESCRIPTOR  objDescr;
  int                decStartFrame;
  int                decodeFrame;
  /* for speed control (PICOLA) */
  float**            PicolaBuffer = NULL;
  static int         PicolaDecodeFlag = 1;
  static int         PicolaOutputFlag = 1;
  long               PicolaNumSample;
  int                PicolaNumInputSample;
  int                sn_ovrhd = 0;
  int refillBits=0;
  int adifHeaderPresent = 0;

  int                EOFreached = 0;
  unsigned char      decodeNext;
#if 1 /* I assume you didn't know what you did ...   HP990811 */
  int                padNumBit;
#endif 
  
  /* init */
  if (mainDebugLevel >= 3) {
    printf("Decode:\n");
    printf("bitFileName=\"%s\"\n",bitFileName);
#ifdef VERSION2
    printf("epFlag=%d\n",epFlag);
#endif /*VERSION2*/
    printf("audioFileName=\"%s\"\n",audioFileName);
    printf("audioFileFormat=\"%s\"\n",audioFileFormat);
    printf("decBitRate=%.3f\n",decBitRate);
    printf("testBitStream=%d\n",testBitStream);
    printf("decPara=\"%s\"\n",decPara?decPara:"(NULL)");
    printf("regionStart=%.6f\n",regionStart);
    printf("regionDurat=%.6f\n",regionDurat);
  }
  numChannelOri = 1 ;
  BsInit(bitBufSize,bitDebugLevel,aacEOF);
  initObjDescr( &objDescr,mainDebugLevel );
  AudioInit(NULL, int24flag, audioDebugLevel);
  info = NULL;
  
  /* open bit stream file */
  if ((strstr(decPara, "-aac_raw") != NULL) ||
#ifdef VERSION2
      (strstr(decPara, "-aac_ld") != NULL) || 
#endif
      (strstr(decPara, "-aac_sca") != NULL) || 
      (strstr(decPara, "-aac_core_bsac") != NULL) )
    bitStream = BsOpenFileRead(bitFileName,NULL,&info);
  else
    bitStream = BsOpenFileRead(bitFileName,MP4_MAGIC,&info);
  
  if (bitStream==NULL) {
    CommonWarning("Decode: error opening bit stream file %s",bitFileName);
    return 1;
  }

  if ((mainDebugLevel >= 3) && (info != NULL))
    printf("info=\"%s\"\n",info);

  /* read bit stream header */
  bitHeader = BsAllocBuffer(BITHEADERBUFSIZE);

  if (!((strstr(decPara, "-aac_raw") != NULL) ||
        (strstr(decPara, "-aac_sca") != NULL) ||
#ifdef VERSION2
        (strstr(decPara, "-aac_ld") != NULL) ||
#endif
        (strstr(decPara, "-aac_core_bsac") != NULL))) {
    if (strstr(decPara, "-aac_sys") != NULL || strstr(decPara, "-aac_sys_bsac") != NULL 
        || strstr(decPara, "-mp4ff") != NULL) {
      unsigned long length,align,dummy,odStartByte;
      bitHeaderNumBit=0;
      adifInfo.profile = LC_Profile;
      if (BsGetBit(bitStream,&(objDescr.ODLength.value),objDescr.ODLength.length) )
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      odStartByte=BsCurrentBit(bitStream)/8;

      if (BsCurrentBit(bitStream) % 8 != 0 )CommonExit(-1,"error in reading object descr");

      if ( BsGetBit(bitStream,&(objDescr.ODescrId.value),objDescr.ODescrId.length) ||
           BsGetBit(bitStream,&(objDescr.streamCount.value),objDescr.streamCount.length) ||
           BsGetBit(bitStream,&(objDescr.extensionFlag.value),objDescr.extensionFlag.length))
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      if (mainDebugLevel >= 3) {
        printf("ObjectDescriptor.streamCount: %ld \n",objDescr.streamCount.value);
      }

      frameNumBit=0;
      bitRateLong=0;
      bitReservSize = 0;
      if   (    objDescr.streamCount.value == 0) CommonExit(-1," no streams found in object descriptor (streamcount=0)");
      for (x=0;x<objDescr.streamCount.value;x++){
        int frameNumBitLay, frameLength, celp_sys_align;
        

        frameNumBitLay=0;
        initESDescr(&(objDescr.ESDescriptor[x]));
#ifndef CORRIGENDUM1
#if 0
        if ( ( x>0 ) && (objDescr.ESDescriptor[x-1]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.bandwidthScalabilityMode.value==1)){
          objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.BWS_on=1       ;                  
        }
#else
        if ( ( x>0 ) && (objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.bandwidthScalabilityMode.value==1) &&
	     (x == (objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value + 1)) ){
          objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.BWS_on=1       ;                  
        }
#endif
#endif
        advanceESDescr(bitStream,objDescr.ESDescriptor[x],0);
   
        if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
	  fSampleLong = objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;
	else
	  fSampleLong = samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];

	switch (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value) {
	case TWIN_VQ:
	case AAC_SCAL:
#ifdef VERSION2	
	case ER_BSAC:
	case ER_TWIN_VQ:
#endif
	  frameNumBitLay = ( objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) 
	    *  objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value / fSampleLong ;
	  sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;
	  frameNumBitLay += (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
	  break;
#ifdef VERSION2	
	case ER_CELP:
	  frameLength = erlpcframelength( &(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig), x );
	  frameNumBitLay = (int)(objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value * (float)frameLength / fSampleLong + 0.5);
	  sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;
	  celp_sys_align = 8 - (frameNumBitLay % 8);
	  if (celp_sys_align == 8) celp_sys_align = 0;
	  frameNumBitLay += celp_sys_align + (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
	  break;
#endif
	case CELP :
	  frameLength = lpcframelength( &(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig), x );
	  frameNumBitLay = (int)(objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value * (float)frameLength / fSampleLong + 0.5);
	  sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;
	  celp_sys_align = 8 - (frameNumBitLay % 8);
	  if (celp_sys_align == 8) celp_sys_align = 0;
	  frameNumBitLay += celp_sys_align + (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
	  break;
	case HVXC :
	case ER_HVXC :
	  frameLength = 160;
	  frameNumBitLay = (int)(objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value * (float)frameLength / fSampleLong + 0.5);
	  sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;
	  frameNumBitLay += (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
	  /* no alignmnet required at average */
	  break;
#ifdef VERSION2	
	case ER_HILN :
	case ER_PARA :
	  frameLength = objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength.value;
	  frameNumBitLay = (int)(objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value * (float)frameLength / fSampleLong + 0.5);
	  sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;
	  frameNumBitLay += (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
	  /* no alignmnet required at average */
	  break;
#endif
	default :
	  CommonExit(-1,"audioDecoderType not implemented");
	  break;
	}

        frameNumBit += frameNumBitLay;
        bitRateLong += objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value;
        bitReservSize += objDescr.ESDescriptor[x]->DecConfigDescr.bufferSizeDB.value;
        if (mainDebugLevel >= 3) {
          printf("\nESDescriptor.ESNumber: %ld \n",objDescr.ESDescriptor[x]->ESNumber.value);
          printf("  ESDescriptor.streamDependence: %ld \n",objDescr.ESDescriptor[x]->streamDependence.value);
          printf("  ESDescriptor.dependsOn_Es_number: %ld \n",objDescr.ESDescriptor[x]->dependsOn_Es_number.value);
          printf("  ESDescriptor.DecConfigDescr.streamType: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.streamType.value);
          printf("  ESDescriptor.DecConfigDescr.bufferSizeDB: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.bufferSizeDB.value);
          printf("  ESDescriptor.DecConfigDescr.avgBitrate: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.audioDecoderType: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value);
	  if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.samplingFrequency: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.channelConfiguration: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value);

	  switch (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value) {
	  case TWIN_VQ:
	  case AAC_SCAL:
#ifdef VERSION2	
	  case ER_BSAC:
	  case ER_TWIN_VQ:
#endif
/*           printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType: %ld \n", */
/*                  objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value); */
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay.value);

#ifdef VERSION2
	    if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.extension.value ==0) break;
	    if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_BSAC) {
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.TFSpecificConfig.numOfSubFrame: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.numOfSubFrame.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.TFSpecificConfig.layer_length: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.layer_length.value);
        }
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.extension3: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.extension3.value);
#endif
	    break;

#ifdef VERSION2	
	  case ER_CELP:
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.ercelpSpecificConfig.excitationMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.ercelpSpecificConfig.sampleRateMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.sampleRateMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.ercelpSpecificConfig.fineRateControl: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl.value);
	    break;
#endif
	  case CELP :
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.sampleRateMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.sampleRateMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl.value);
	    break;
	  case HVXC :
#ifdef VERSION2
	  case ER_HVXC :
#endif
	    if (objDescr.ESDescriptor[x]->streamDependence.value == 0) {
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value);
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag.value);
#ifdef VERSION2
#if 0
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.packMode: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.packMode.value);
#endif
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.vrScalFlag: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.vrScalFlag.value);
#endif
	    }
	    break;
#ifdef VERSION2
	  case ER_HILN :
	  case ER_PARA :
#ifdef CORRIGENDUM1
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value);
	    if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value == 1) /*BaseLayer*/
#else
      	    if (objDescr.ESDescriptor[x]->streamDependence.value == 0)  /*BaseLayer*/
#endif
	    {
	      printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode: %ld \n",
		     objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value);
	      
	      if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value != 1) {
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCvarMode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCvarMode.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCrateMode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCrateMode.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value);
		if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value) {
		  printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.vrScalFlag: %ld \n",
			 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.vrScalFlag.value);
		}
	      }
	      
	      if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value != 0) {
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNquantMode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNquantMode.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNmaxNumLine: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNmaxNumLine.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNsampleRateCode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNsampleRateCode.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength.value);
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNcontMode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNcontMode.value);
	      }
	      
	      printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAextensionFlag: %ld \n",
		     objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAextensionFlag.value);
	    }
	    else {
	      printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer: %ld \n",
		     objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value);
	      if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value) {
		printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaQuantMode: %ld \n",
		       objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaQuantMode.value);
	      }
	    }
#ifndef EP_CONFIG_MISSING
      printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.epConfig: %ld \n", objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.epConfig.value);
#endif
	    break;
#endif
	  default :
	    CommonExit(-1,"audioDecoderType not implemented");
	    break;
	  }

          printf("  ESDescriptor.ALConfigDescriptor.useAccessUnitStartFlag.value: %ld \n",
                 objDescr.ESDescriptor[x]->ALConfigDescriptor.useAccessUnitStartFlag.value);
          printf("  ESDescriptor.ALConfigDescriptor.useRandomAccessPointFlag.value: %ld \n",
                 objDescr.ESDescriptor[x]->ALConfigDescriptor.useRandomAccessPointFlag.value);
          printf("  ESDescriptor.ALConfigDescriptor.usePaddingFlag.value: %ld \n",
                 objDescr.ESDescriptor[x]->ALConfigDescriptor.usePaddingFlag.value);
          printf("  ESDescriptor.ALConfigDescriptor.seqNumLength.value: %ld \n",
                 objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value);
        }
      }
      frameMinNumBit=0;
      length=  BsCurrentBit(bitStream)/8;
      align = 8 -  BsCurrentBit(bitStream) % 8;
      if (align == 8) align = 0;
      if (align != 0) {
        length += 1;
      }
      BsGetBit(bitStream,&(dummy),align) ;
      if ( (length-odStartByte) != objDescr.ODLength.value) CommonExit(-1,"error in object descriptor read");

      bitReservInit= bitReservSize;        /* BT 971105 */  
      bitHeaderNumBit=0;
      switch (objDescr.ESDescriptor[objDescr.streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value) {
      case TWIN_VQ:
      case AAC_SCAL:
#ifdef VERSION2	
      case ER_BSAC:
      case ER_TWIN_VQ:
#endif
	mode = MODE_TF;
	break;
#ifdef VERSION2	
      case ER_CELP:
#endif
      case CELP :
	mode = MODE_LPC;
	break;
      case HVXC :
#ifdef VERSION2	
      case ER_HVXC :
#endif
	mode = MODE_HVXC;
	break;
#ifdef VERSION2	
      case ER_HILN :
      case ER_PARA :
	mode = MODE_PAR;
	break;
#endif
      default :
	CommonExit(-1,"audioDecoderType not implemented");
	break;
      }
      numChannelOri=objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
      frameData.od = &objDescr;
    
    }
    else if(strstr(decPara, "-tvq_sys") !=NULL) {
      unsigned long length,align,dummy,odStartByte;
      int frameNumBitLay=0;/* T.Ishikawa 980703 */
      int FlexNumBitLay=0; /* T.Ishikawa 980727 */
      adifInfo.profile = LC_Profile;
      bitHeaderNumBit=0;
      if (mainDebugLevel >= 3) {
        printf("Decode: Init : %ld\n",BsCurrentBit(bitStream));
      }
      if (BsGetBit(bitStream,&(objDescr.ODLength.value),objDescr.ODLength.length) )
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      odStartByte=BsCurrentBit(bitStream)/8;

      if (BsCurrentBit(bitStream) % 8 != 0 )CommonExit(-1,"error in reading object descr");

      if ( BsGetBit(bitStream,&(objDescr.ODescrId.value),objDescr.ODescrId.length) ||
           BsGetBit(bitStream,&(objDescr.streamCount.value),objDescr.streamCount.length) ||
           BsGetBit(bitStream,&(objDescr.extensionFlag.value),objDescr.extensionFlag.length))
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      if (mainDebugLevel >= 3) {
        printf("ObjectDescriptor.streamCount: %ld \n",objDescr.streamCount.value);
      }

      frameNumBit=0;
      bitRateLong=0;

      for (x=0;x<objDescr.streamCount.value;x++){
        int frameNumBitLay;
        initESDescr(&(objDescr.ESDescriptor[x]) );
        advanceESDescr(bitStream,objDescr.ESDescriptor[x],0);
        if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
	  fSampleLong = objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;
	else
	  fSampleLong = samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
        /* T.Ishikawa 980703 */
        frameNumBitLay =(int)( (float)( objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) *  (float)objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value / (float)fSampleLong+0.5) ;
        frameNumBit += frameNumBitLay;
        if (mainDebugLevel >= 3) {
          printf("mp4dec: frameNumBitLay[%d] %d\n",x,frameNumBitLay);
        }
        bitRateLong += (int)(objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value);
        frameNumBit += 3*8*(1+(frameNumBitLay/2040));
        FlexNumBitLay += 3*8*(1+(frameNumBitLay/2040));/* T.Ishikawa 980727 */
		
        /*	if (mainDebugLevel) {	
                printf("mp4dec : avgBitrate %d[%d]\n",objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value,x);
                }*/
		
      }	  
      if (mainDebugLevel >= 3) {
        printf("mp4dec: FlexNumBitLay %d\n",FlexNumBitLay);
      }
      frameNumBit = (int)(bitRateLong*( objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024)/(float)fSampleLong + 0.5)+FlexNumBitLay; /* T.Ishikawa 980727 */
	  
      /*	  bitRateLong += ((int)( (31.0*fSampleLong/(float)(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024)) * (float)objDescr.streamCount.value + 25.0*fSampleLong/(float)(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024)));*/

      if (mainDebugLevel >= 3) {
        printf("mp4dec : bitRateLong %ld\n",bitRateLong);	  
      }
      /*	  frameNumBit=(int)(((float)bitRateLong*(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024)/fSampleLong)+0.5);*/

      if (mainDebugLevel >= 3) {
        printf("mp4dec : frameNumBit(tvq_sys Init) %d, frameNumBitLay %d\n",frameNumBit,frameNumBitLay);	  
      }
      /* Notice! : frameNumBit=1141 */
      /*        frameNumBit -= 3*8*(1 + (objDescr.streamCount.value/2040)) ;*/ /* overhead for flexMux transport */	  

	  
#if	  0
      for (x=1;x<objDescr.streamCount.value;x++){
        int frameNumBitLay=0;
        frameNumBitLay +=(int)( (float)( objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) 
                                *  (float)objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value / (float)fSampleLong) ;

        frameNumBitLay += 3*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
      }
#endif

      for (x=0;x<objDescr.streamCount.value;x++){
        if (mainDebugLevel >= 3) {
          printf("\nESDescriptor.ESNumber: %ld \n",objDescr.ESDescriptor[x]->ESNumber.value);
          printf("  ESDescriptor.streamDependence: %ld \n",objDescr.ESDescriptor[x]->streamDependence.value);
          printf("  ESDescriptor.dependsOn_Es_number: %ld \n",objDescr.ESDescriptor[x]->dependsOn_Es_number.value);
          printf("  ESDescriptor.DecConfigDescr.streamType: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.streamType.value);
          printf("  ESDescriptor.DecConfigDescr.bufferSizeDB: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.bufferSizeDB.value);
          printf("  ESDescriptor.DecConfigDescr.avgBitrate: %ld \n",objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.audioDecoderType: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value);
          if (objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
	    printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.samplingFrequency: %ld \n",
		   objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.channelConfiguration: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value);

/*           printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType: %ld \n", */
/*                  objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value); */
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay.value);
        }
      }

      frameMinNumBit=frameNumBit;
      length=  BsCurrentBit(bitStream)/8;
      align = 8 -  BsCurrentBit(bitStream) % 8;
      if (align == 8) align = 0;
      if (align != 0) {
        length += 1;
      }
      BsGetBit(bitStream,&(dummy),align) ;
      if ( (length-odStartByte) != objDescr.ODLength.value) {
        CommonExit(-1,"error in object descriptor read");
      }
      if (mainDebugLevel >= 3) {
        printf("####  Decoder : length %ld  #####\n",length);
      }
      bitReservSize= 0; /* T.Ishikawa */
      bitReservInit= 0; 
      bitHeaderNumBit=0;
      mode = MODE_TF;
      numChannelOri=objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
      frameData.od = &objDescr;

    } else {
      frameData.od = NULL;
      /* for qc_aac we read */
      if (BsGetBitInt(bitStream,(unsigned int*)&bsVersion,16) ||
          BsGetBitInt(bitStream,(unsigned int*)&numChannelOri,8) ||
          BsGetBit(bitStream,(unsigned long*)&fSampleLong,32) ||
          BsGetBit(bitStream,(unsigned long*)&bitRateLong,32) ||
          BsGetBitInt(bitStream,(unsigned int*)&frameNumBit,16) ||
          BsGetBitInt(bitStream,(unsigned int*)&frameMinNumBit,16) ||
          BsGetBitInt(bitStream,(unsigned int*)&bitReservSize,16) ||
          BsGetBitInt(bitStream,(unsigned int*)&bitReservInit,16) ||
          BsGetBitInt(bitStream,(unsigned int*)&mode,8) ||
          BsGetBitInt(bitStream,(unsigned int*)&bitHeaderNumBit,16))
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      
      if (bsVersion != MP4_BS_VERSION) {
        CommonWarning("Decode: wrong bitstream version "
                      "(found 0x%4x, need 0x%4x)",bsVersion,MP4_BS_VERSION);
        return 1;
      }
    }

    if (bitHeaderNumBit > BITHEADERBUFSIZE)
      CommonExit(1,"Decode: bit stream header too long");
    
    if (BsGetBuffer(bitStream,bitHeader,bitHeaderNumBit))
      CommonExit(1,"Decode: error reading bit stream header (core)");
  } 
  else{

    /* Check if the bitstream has an ADIF header */
    if ( GetAdifHeader(bitStream, &adifInfo)) {
      if ( mainDebugLevel >= 3 ) {
        printf ( "ADIF Header present\n" );
      }

      adifHeaderPresent = 1;
      numChannelOri = adifInfo.numChannels;
      fSampleLong   = (long) adifInfo.samplingRate;
      bitRateLong   = (long) adifInfo.bitrate;
      bitReservInit = (long) adifInfo.bitresFullnes;
    } else {

      /* raw data */
  
      /* set the profile of the bitstream */
      if( strstr( decPara, "-aac_raw" ) ) {
        if (strstr(decPara, "-main"))
          adifInfo.profile = Main_Profile;
        else if( strstr( decPara, "-ssr" ) )
          adifInfo.profile = SSR_Profile;
        else
          adifInfo.profile = LC_Profile;
#ifdef BODO_COMMENT
        printf ( "#################DECODING AAC RAW BITSTREAM:###########\n " );        
        printf ( "ATTENTIONE !!!!!!! \n make sure that you specify sampling"
                 "freq and number of channels on cmdline: -s <samplFreq> -n <numchan>\n " );  
#endif
      }
      numChannel = (numChannelOut != 0) ? numChannelOut : numChannelOri;
      numChannelOri=numChannel;
      fSampleLong= (long int) fSampleOut;
      if (fSampleLong < 8000 ){
        CommonExit(-1,"wrong sample rate %d \n",fSampleLong);
      }
      bitRateLong=128000;
    }
    numChannel = numChannelOri;
    frameNumBit= 1024 * bitRateLong / fSampleLong; 
    if (strstr(decPara, "-qc_aac_960") != NULL)
      frameNumBit = 960*bitRateLong/fSampleLong ;
    if (strstr(decPara, "-aac_sca") != NULL || 
        strstr(decPara, "-aac_core_bsac") != NULL)
      frameNumBit = 960*bitRateLong/fSampleLong ;
    frameMinNumBit=0;
    bitReservSize= 6144*numChannel;        /* 6144-1366; */
    bitReservInit= 6144*numChannel;        /* BT 971105 */  
    bitHeaderNumBit=0;
    mode = MODE_TF;
  }


  /* --- Set debugging options for aac_decoder --- */
  /*
    if (strstr(decPara, "-qc_aac") != NULL){
    extern int     aacSamplingRate;
    aacSamplingRate = (int)fSampleLong;
    } 
  */ 
  /*   if (aacDebugString != NULL){ */
  /*     aacDebugString    */
  /*   } */

  headerNumBit = BsCurrentBit(bitStream);

  frameMaxNumBit = frameNumBit+bitReservSize;
  fSampleOri = fSampleLong;
  bitRate = bitRateLong;
  if (decBitRate == 0) {
    decBitRate = bitRate;
    frameDecNumBit = 0;
  }
  else {
    if (decBitRate > bitRate) {
      CommonWarning("Decode: decBitRate > bitRate, using bitRate");
      decBitRate = bitRate;
    }
#ifdef VERSION2 /* for HVXC VR scalability (YM 990802) */ 
    if (frameMinNumBit < frameNumBit && (mode != MODE_HVXC || strstr(decPara, "-mp4ff") !=NULL))
#else
    if (frameMinNumBit < frameNumBit)
#endif 
      CommonExit(1,"Decode: scalability not possible for variable bit rate");
    if (bitReservSize != 0)
      CommonExit(1,"Decode: scalability not possible for bit reservoir");
    frameDecNumBit  = (int)(frameNumBit/bitRate*decBitRate+.5);
  }

  if (regionDurat < 0)
    decNumFrame = -1;
  else
    decNumFrame = max(0,(int)(regionDurat*bitRate/frameNumBit));
  decStartFrame = max(0,(int)(regionStart*bitRate/frameNumBit+.5));

  numChannel = (numChannelOut != 0) ? numChannelOut : numChannelOri;
  fSample = (fSampleOut != 0) ? fSampleOut : fSampleOri;

  if(SpeedControlMode) {
    if(numChannel>1) {
      CommonWarning("Decode: "
                    "numChannel>1 : PICOLA speed control is disabled\n");
      SpeedControlMode = 0;
    }
  }
  PicolaNumSample = 0;

  if (mainDebugLevel >= 3) {
    printf("\nmode=%d (3=MODE_TF) \n",mode);
    printf("numChannelOri=%d\n",numChannelOri);
    printf("fSampleOri=%.3f Hz  (int=%ld)\n",fSampleOri,fSampleLong);
    printf("numChannel=%d\n",numChannel);
    printf("fSample=%.3f Hz\n",fSample);
    printf("speedFact=%.3f",speedFact);
    if(SpeedControlMode) {
      printf(" :picola\n");
    }else {
      printf("\n");
    }
    printf("pitchFact=%.3f\n",pitchFact);
    if (!adifHeaderPresent && (strstr(decPara, "-mp4ff") == NULL))  
      printf("raw bitstream: assumed ");
    printf("bitRate=%.3f bit/sec  (int=%ld)\n",bitRate,bitRateLong);
    printf("bitReservSize=%d bit  (%.6f sec)\n",
           bitReservSize,bitReservSize/bitRate);
    printf("bitReservInit=%d bit\n",bitReservInit);
    if (!adifHeaderPresent && (strstr(decPara, "-mp4ff") == NULL))
      printf("raw bitstream: assumed ");
    printf("frameNumBit=%d  (%.6f sec)\n",frameNumBit,frameNumBit/bitRate);
    printf("frameDecNumBit=%d\n",frameDecNumBit);
    printf("frameMinNumBit=%d\n",frameMinNumBit);
    printf("frameMaxNumBit=%d\n",frameMaxNumBit);
    printf("bitHeaderNumBit=%d\n",bitHeaderNumBit);
    printf("headerNumBit=%d\n",headerNumBit);
    printf("decStartFrame=%d\n",decStartFrame);
    printf("decNumFrame=%d\n",decNumFrame);
  }

  if (testBitStream && frameNumBit != frameMinNumBit)
    CommonExit(1,"Decode: variable bit rate, bit stream test NOT possible!");

  if (regionStart != 0 && frameNumBit != frameMinNumBit)
    CommonExit(1,"Decode: variable bit rate, start time MUST be 0!");

  if (frameMaxNumBit > bitBufSize)
    CommonExit(1,"Decode: max bit stream frame size too long");

  if (mode<=0 || mode>=MODE_NUM)
    CommonExit(1,"Decode: unknown codec mode %d",mode);
  codecMode = MP4ModeName[mode];

  if (mainDebugLevel >= 3)
    printf("codecMode=\"%s\"\n",codecMode);



  if (strstr(decPara, "-mp4ff") != NULL) {
#ifdef VERSION2
    /* should be moved to into tf part */
    hFault->hResilience = CreateErrorResilience ( decPara, epFlag );
#endif /*VERSION2*/

    flexMuxDecInit ( &numChannel, 
                     decPara,
                     &frameData,
                     &tfData,
                     &lpcData,
                     &nttData,
		     &hvxcData,	/* AI 990616 */
		     &paraData,	/* HP 20000330 */
#ifdef VERSION2
                     audioFileName,
#endif /*VERSION2*/
                     &fSample );    

    if (mode == MODE_HVXC) {	
      /* delay compensation for HVXC(AI 99060XX) */
      delayNumSample = hvxcData->delayNumSample;
    }
    else if (mode == MODE_PAR) {	
      /* delay compensation for PARA/HILN HP 20000330 */
      delayNumSample = paraData->delayNumSample;
    }
    else {
      delayNumSample=0;
    }
  } else {
    if (!testBitStream) {
      switch (mode) {
      case MODE_PAR:
	CommonWarning("HILN raw bitstream decoder needs \"-spb 1\"");
	DecParInit(numChannel,fSample,decBitRate,decPara,
		   bitHeader,&frameMaxNumSample,&delayNumSample
#ifdef VERSION2		/* appended by Sony on 990616 */
		   , epFlag, epDebugLevel, infoFileName,
		   &hFault->hEscInstanceData,-1,-1
#endif
		   );
	break;
      case MODE_HVXC:
	hvxcData = DecHvxcInit(numChannel,
			    fSample,
			    decBitRate,
			    decPara,
			    bitHeader,
			    &frameMaxNumSample,
			    &delayNumSample
#ifdef VERSION2		/* appended by Sony on 990616 */
			    , epFlag, epDebugLevel, infoFileName,
			    &hFault->hEscInstanceData
#endif
			    );	
	break;
      case MODE_LPC:
	DecLpcInit(numChannel,fSample,decBitRate,decPara,
		   bitHeader,&frameMaxNumSample,&delayNumSample);
	break;
      case MODE_TF:
#ifdef VERSION2
        hFault->hResilience = CreateErrorResilience ( decPara, epFlag );
#endif /*VERSION2*/
        if (strstr(decPara, "-aac_bsac") != NULL) {
	      CommonWarning("Decode : BSAC raw bitstream decoder needs option \"-spb 1\"");
        }
        else if (!aacEOF) {
          CommonWarning("Decode: option '-aaceof' is recommended in AAC mode");
        }
        DecTfInit ( numChannel,
                    fSample,
                    decBitRate,
                    decPara,
                    adifInfo.profile,
                    bitHeader,
                    &frameMaxNumSample,
                    &delayNumSample,
                    aacDebugString,
                    hFault,
                    (frameData.od == NULL ? NULL : &frameData),
                    &tfData,
                    &nttData,
                    mainDebugLevel
#ifdef VERSION2
                    ,epDebugLevel,
                    infoFileName,
                    audioFileName
#endif
                    );
        break;
      }
    }     /* if (!testBitStream) */
  }

  /* speed control initialization */
  if(SpeedControlMode)
    PicolaInit(frameMaxNumSample, fSample, speedFact);

  BsFreeBuffer(bitHeader);

  if (!testBitStream) {
    if (strstr(decPara, "-mp4ff") == NULL) {
      if (mainDebugLevel >= 3) {
        printf("frameMaxNumSample=%d  (%.6f sec/frame)\n",
               frameMaxNumSample,frameMaxNumSample/fSample);
        printf("delayNumSample=%d  (%.6f sec)\n",
               delayNumSample,delayNumSample/fSample);
      }
      
      /* allocate buffers */
      bitBuf = BsAllocBuffer(frameMaxNumBit);
      if ((sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Decode: memory allocation error");
      for (ch=0; ch<numChannel; ch++)
        if ((sampleBuf[ch]=(float*)malloc(frameMaxNumSample*sizeof(float)))
            ==NULL)
          CommonExit(1,"Decode: memory allocation error");
      
      /* for speed control (PICOLA) */
      if(NULL==(PicolaBuffer=(float **)calloc(numChannel, sizeof(float)))) {
        CommonExit(1, "Decode: memory allocation error");
      }
      for(ch=0;ch<numChannel;ch++) {
        if(NULL==(*(PicolaBuffer+ch)=(float *)calloc(frameMaxNumSample, 
                                                     sizeof(float)))) {
          CommonExit(1, "Decode: memory allocation error");
        }
      }
    }
    /* open audio file */
    /* (sample format: 16 bit twos complement, uniform quantisation) */
    audioFile = AudioOpenWrite(audioFileName,audioFileFormat,
                               numChannel,fSample);
    if (audioFile==NULL)
      CommonExit(1,"Decode: error opening audio file %s "
                 "(maybe unknown format \"%s\")",
                 audioFileName,audioFileFormat);

    /* seek to beginning of first frame (with delay compensation) */
    AudioSeek(audioFile,-delayNumSample);

  } /* if (!testBitStream) */

  /* process bit stream frame by frame */
  frame = -decStartFrame;
  totNumSample = 0;
  totPadNumBit = 0;
  if(strstr(decPara,"-tvq_sys") != NULL)
    {
      /*frameAvailNumBit = frameNumBit - (18*(frameNumBit/(250*8) +1) +7);*/
      frameAvailNumBit = frameNumBit; /* T.Ishikawa 980624 */
      if (mainDebugLevel >= 3) {
        printf("frameAvailNumBit %5d , frameNumBit %5d\n",
               frameAvailNumBit,frameNumBit);
      }
    }
  else
    {
      frameAvailNumBit = frameNumBit+bitReservInit;
    }

  usedNumBit=0;

  if (strstr(decPara, "-mp4ff") != NULL) {
    flexMuxDec(bitStream,
               numChannel, 
               &frameData,
               &tfData,
               &lpcData,
               &nttData,
	       hvxcData,	/* AI 990616 */
	       paraData,	/* HP 20000330 */
               audioFile,
               hFault                
               );
  } else {

    /*  for constant rate decoders the following applies:
        to decode one frame we need frameNumBit ;
        for variale rate decoder (eg. AAC)  the following applies:
        to decode one frame we don't exactly know the minimum number of bit a frame can have, but you in most cases you need 
        at least 8 bit (eg. for the header)*/
    decodeNext = (!BsEof ( bitStream, (aacEOF) ? 8 : frameAvailNumBit - 1 ) &&  (decNumFrame < 0 || frame < decNumFrame))  ;
    

    if (decodeNext) {
      /* initialize buffer with the number of bits needed to decode  one frame. 
         for constant rate decoders the following applies:
         frameAvailNumBit = frameNumBit (which is the average number of bits per frame including overhead for transport)
         for variale rate decoder (eg. AAC)  the following applies:
         the number of bits for the next frame is not known but the maximum size is:
         frameAvailNumBit = frameNumBit +  bitReservSize 
         in case of ADIF header the          maximum size actually is:
         frameAvailNumBit = frameNumBit +  bitReservInit (but this is currently not taken into acount)
         in case of mpeg4 file format   maximum size could be calculated from the timestamps 
      */
      BsGetBuffer(bitStream,bitBuf,frameAvailNumBit); 

    }
    refillBits=0;
    
    while ( decodeNext)
      {
#ifdef FHG_DEBUGPLOT
        plotInit(frame,BsGetFileName(bitStream),0);
#endif

        decodeFrame = !testBitStream && frame >= 0;
        if (decodeFrame) {
#ifdef VERSION2
          /* if( epFlag == 1 ) { */
	  /* modified by Sony on 990616 */
          if( epFlag == 1 && mode != MODE_PAR && mode != MODE_HVXC ) {
            BsReadInfoFile ( hFault->hEscInstanceData );
          }
#endif /* VERSION2  */


          {
            /* what is this???   HP 990422 */
            /*keep the frame buffer full if end of bitstream is not reached */
            int statusBitstream;
            if ( ! EOFreached ) {
              statusBitstream = BsGetBufferAppend (bitStream, bitBuf, 1, refillBits);
              if ( statusBitstream == -1 ) {
                EOFreached = 1;
              } 
              else {
                if ( statusBitstream !=0 ) {
                  CommonExit(1,"Decode: error reading bit stream data");
                }
              }
            }
          }
    
	  /* AI 990528 */
	  /* if number of bits in bitBuf is less than minimum bits to decode, break */

	  /* replaced  BsGetBufferFullness(bitBuf)  by  bitBuf->numBit  ... */
	  /* ... although bitBuf struct def shouldn't be used here HP 990722 */

	  if (BsBufferNumBit(bitBuf) < frameMinNumBit) break;

	  if ((mainDebugLevel == 1) && (frame % 20 == 0)) {
	    fprintf(stderr,"\rframe %4d ",frame);
	    fflush(stderr);
	  }
	  if (mainDebugLevel >= 2 && mainDebugLevel <= 3) {
	    printf("\rframe %4d ",frame);
	    fflush(stdout);
	  }
	  if (mainDebugLevel > 3)
	    printf("frame %4d\n",frame);
	  if (mainDebugLevel >= 5)
	    printf("frameAvailNumBit=%d\n",frameAvailNumBit);
	  /* AI 990528 */

#ifdef MOUSECHANGE
	  GetMouse(&speedFact,&pitchFact);
	  if (MOUSEdisp) {
	    if (mainDebugLevel == 1)
	      fprintf(stderr,"speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
	    if (mainDebugLevel >= 2)
	      printf("speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
	  }
#endif

          /* decode one frame */
          switch ( mode ) {
	  case MODE_PAR: 
	    if(SpeedControlMode)
	      DecParFrame(bitBuf,(HANDLE_BSBITSTREAM*)NULL,
			  sampleBuf,&usedNumBit,&frameBufNumSample,
			  1,pitchFact
#ifdef VERSION2		/* appended by Sony on 990616 */
			  ,epFlag,hFault->hEscInstanceData
#endif
			  );
	    else
	      DecParFrame(bitBuf,(HANDLE_BSBITSTREAM*)NULL,
			  sampleBuf,&usedNumBit,&frameBufNumSample,
			  speedFact,pitchFact
#ifdef VERSION2		/* appended by Sony on 990616 */
			  ,epFlag,hFault->hEscInstanceData
#endif
			  );
	    break;
	  case MODE_HVXC:
	    if(SpeedControlMode)
	      DecHvxcFrame(hvxcData,
			   bitBuf,sampleBuf,
			   &usedNumBit,
			   &frameBufNumSample,
			   1,
			   pitchFact
#ifdef VERSION2		/* appended by Sony on 990616 */
			   ,epFlag,hFault->hEscInstanceData
#endif
			   );
	    else
	      DecHvxcFrame(hvxcData,
			   bitBuf,sampleBuf,
			   &usedNumBit,
			   &frameBufNumSample,
			   speedFact,
			   pitchFact
#ifdef VERSION2		/* appended by Sony on 990616 */
			   ,epFlag,hFault->hEscInstanceData
#endif
			   );
	    break;
          case MODE_LPC:
            DecLpcFrame(bitBuf,sampleBuf,&usedNumBit);
            frameBufNumSample = frameMaxNumSample;
            break;
          case MODE_TF:
            DecTfFrame ( bitBuf,
                         sampleBuf,
                         &usedNumBit,
                         numChannel, 
                         mainDebugLevel,
                         hFault,
                         (frameData.od == NULL ? NULL : &frameData),
                         &tfData,
                         &nttData );
            frameBufNumSample = frameMaxNumSample;
            break;
          }
        }
	else        /* if (decodeFrame) */
	  usedNumBit = frameNumBit;

        if (mainDebugLevel >= 5) {
          if (decodeFrame)
            printf("frameBufNumSample=%d\n",frameBufNumSample);
          printf("usedNumBit=%d\n",usedNumBit);
        }

        if (usedNumBit > frameAvailNumBit)
          CommonWarning("Decode: more bits used than available in frame+buffer");

        if ((usedNumBit != BsBufferFreeBits(bitBuf)) && !EOFreached ){
          CommonWarning ("\nsome bits are missing!!\n");
          usedNumBit = BsBufferFreeBits(bitBuf);
        }
            
        /* byte alignment */
#if 0
        alignBits = 8 - usedNumBit - ((usedNumBit/8)*8);
        if (alignBits<8){
          HANDLE_BSBITSTREAM  stream = BsOpenBufferRead(bitBuf);
          BsGetSkip(stream,alignBits);
          usedNumBit += alignBits;
          BsCloseRemove(stream,1);          
        }
#endif

        if(skipPaddingBits==1){
#if 1 /* I assume you didn't know what you did ...   HP990811 */
        /* at this level padding bits should not occur:
           for flexmux or mp4 file format, the padding bits are at the end of an access unit and are handled therefor 
           by the flexmux or mp4 file format framework
           for raw AAC bitstreams padding bits only occur in 'fillelements' and are decoded in the aac decoder itself.
           for all other  coders padding should not occure in raw bitstreams because padding makes only sense in transprort
           streams. but neither raw bitstreams nor flexmux or file format bitstremas are actually transport streams.


	   Two comments on the above: (Heiko Purnhagen 990811)
	   ==========================

	   1) Please state your name when doing such things to the framework!

	   2) Padding bits _are_ needed in the (hopefully soon obsolete)
	   raw bistream format. Since this raw bistream format is acutally a
	   transport format, the padding is required to maintain sync. e.g.
	   in case of scalable or variable rate bistreams!

	   [see also: A. N. Onymous: "On bi-directional communication by
	   means of normative software comments," submitted to Journal of
	   Concurrent Developement.]

        */
        /* check for padding bits */
        if (frameAvailNumBit-usedNumBit-frameNumBit+frameMinNumBit
            > bitReservSize) {
          padNumBit = frameAvailNumBit-usedNumBit-frameNumBit+frameMinNumBit
            -bitReservSize;

          if (mainDebugLevel >= 5)
            printf("padNumBit=%d\n",padNumBit);

          /* T.Ishikawa 980526 */
          if(strstr(decPara,"-tvq_sys")==NULL)
            {
              HANDLE_BSBITSTREAM  stream = BsOpenBufferRead(bitBuf);
              BsGetSkip(stream,padNumBit);
              usedNumBit += padNumBit;
              BsCloseRemove(stream,1);

            }
          totPadNumBit += padNumBit;
        }
#endif
        }

        refillBits=usedNumBit;
        
        
	if(skipPaddingBits!=1){
#if 1
        /* advance in bit stream */
        if ( frameDecNumBit ) {
          usedNumBit = frameNumBit;
        }
#endif
	}

#if 0 /* I assume you didn't know what you did ...   HP990811 */
	/* we need this   HP 990422 */
        /* well, actually a decoder should read the bits it decodes from bitBuf.
           this should be done in the following way:
           open a bitstream from the bitBuf:
           
           decBitStream = BsOpenBufferRead(bitBuf);
           
           read the bits with

           BsGetBit( decBitStream ,&ultmp, 1 );  
           
           and finally if decoding of the frame is finished close the bitStream with 

           BsCloseRemove ( decBitStream  ,1);
           so that the bits that are allready read are really removed from the bitBuf.
           
           then its no longer needed to call  BsGetSkip(bitStream,usedNumBit) here.

           This model is more similar to the System Decoder Model in Mpeg4 systems, where the Decoder Buffer is actually a FIFO.
           

	   Another comment on the above: (Heiko Purnhagen 990811)
	   =============================

	   I simply don't see the point why a decoder core should modify
	   the bitstream buffer it is decoding (e.g. delete bits by
	   BsCloseRemove()). The "usedNumBit" field is of interest anyway
	   to permit the decoder framework of doing bit rate statistics,
	   buffer usage verification and similar things.

        */

	if (BsGetSkip(bitStream,usedNumBit))
	  CommonExit(1,"Decode: error advancing in bit stream data");
#endif

        frameAvailNumBit -= usedNumBit;

        /* variable bit rate: don't exceed bit reservoir size */
        if (frameAvailNumBit > bitReservSize)
          frameAvailNumBit = bitReservSize;

        if (!adifHeaderPresent && (strstr(decPara, "-mp4ff") == NULL)
	    && (skipPaddingBits!=1))
          frameAvailNumBit += usedNumBit;
        else
          frameAvailNumBit += frameNumBit;

        if (decodeFrame) {
          if(SpeedControlMode) {
            PicolaDecodeFlag = 0;
            PicolaNumInputSample = frameBufNumSample;
            totNumSample += frameBufNumSample;
        
            while(0==PicolaDecodeFlag) {
              mod_picola(sampleBuf[0], PicolaNumInputSample, 
                         PicolaBuffer[0], speedFact, 
                         &PicolaDecodeFlag, &PicolaOutputFlag);
              PicolaNumInputSample = 0;
              if(PicolaOutputFlag) {
                /* write audio file */
                numSample = frameMaxNumSample;
                AudioWriteData(audioFile,PicolaBuffer,numSample);
                PicolaNumSample += numSample;
              }
            }
          }
          else {
            /* write audio file */
            AudioWriteData(audioFile,sampleBuf,frameBufNumSample);
            totNumSample += frameBufNumSample;
          }
        }    /* if (decodeFrame) */
#ifdef DEBUGPLOT /* usefull not only for fhg decoders but for all decoders */
        plotDisplay(0);
#endif

        if ( EOFreached ) {
	  /* if EOF is reached, check whether minimum bits are left for
	     decode in bitBuf  */
          decodeNext = ((BsBufferNumBit(bitBuf) > ((aacEOF) ? 8 : frameMinNumBit-1))
                        && (( decNumFrame < 0 ) || (frame < decNumFrame))) ;
        }  else {
	  /* if EOF is not reached, check whether bits to be append to
	     bitBuf exist(even 1bit..) */
          decodeNext = ((!BsEof ( bitStream, (aacEOF) ? 8 : 0)) 
                        && (( decNumFrame < 0 ) || (frame < decNumFrame))) ;
        }
	if (mainDebugLevel >= 5) {
	  printf("*** EOFreached=%d, decodeNext=%d\n", EOFreached, decodeNext);
	}

        frame++;
      }     /* while (decodeNext) */
  }

  if (mainDebugLevel == 1)
    fprintf(stderr," \n");
  if (mainDebugLevel >= 2 && mainDebugLevel <= 3)
    printf(" \n");

  totDataNumBit = BsCurrentBit(bitStream)-headerNumBit-totPadNumBit;

  frameAvailNumBit -= frameNumBit;

  if (mainDebugLevel >= 3) {
    printf("totNumFrame=%d\n",frame);
    printf("NOTE: all tot*NumBit info is broken   HP990811\n");
    printf("totNumBit=%ld\n",BsCurrentBit(bitStream));
    if (!testBitStream) {
      /* HP 980707 */
      printf("totDataNumBit=%ld  (%.3f bit/frame  %.3f bit/sec)\n",
             totDataNumBit,totDataNumBit/(float)frame,
             totDataNumBit*fSample/(float)totNumSample);
      printf("bitRate=%.3f bit/sec  (%.6f sec/frame)\n",
             bitRate,(totDataNumBit+totPadNumBit)/(float)frame/bitRate);
      printf("totPadNumBit=%ld\n",totPadNumBit);
      printf("frameRemainNumBit=%d\n",frameAvailNumBit);
      if(SpeedControlMode) {
        printf("totNumSample=%ld  (%.3f sec)\n",
               PicolaNumSample,PicolaNumSample/fSample);
        printf("Speed Control Ratio: Target=%.3f, Result=%.3f\n", 
               speedFact, totNumSample/(float)PicolaNumSample);
      }else {
        printf("totNumSample=%ld  (%.3f sec)\n",
               totNumSample,totNumSample/fSample);
      }
      printf("delayNumSample=%d  (%.3f sec)\n",
             delayNumSample,delayNumSample/fSample);
      printf("fileNumSample=%ld  (%.3f sec)\n",
             totNumSample-delayNumSample,
             (totNumSample-delayNumSample)/fSample);
      printf("frameAvgNumSample=%.3f  (%.6f sec/frame)\n",
             totNumSample/(float)frame,
             totNumSample/(float)frame/fSample);
    }
  }

  /* free decoder memory */
  switch (mode) {
  case MODE_PAR:
    DecParFree();
    break;
  case MODE_HVXC:	
    DecHvxcFree(hvxcData);
    break;
  case MODE_LPC:
    DecLpcFree();
    break;
  case MODE_TF:
    DecTfFree (
#ifdef VERSION2
               hFault->hEscInstanceData, 
               hFault->hResilience
#endif /*VERSION2*/
               );
    break;
  }

  if(SpeedControlMode)
    PicolaFree();

  /* close bit stream file */
  if (BsClose(bitStream))
    CommonExit(1,"Decode: error closing bit stream file");

  if (!testBitStream) {
    /* close audio file */
    AudioClose(audioFile);
    if (strstr(decPara, "-mp4ff") == NULL) {
      /* free buffers */
      if (numChannel>1)
        for (ch=0; ch<numChannel; ch++)
          free(sampleBuf[ch]);
      free(sampleBuf);
      BsFreeBuffer(bitBuf);
      
      /* for speed control (PICOLA) */
      if (numChannel>1)
        for (ch=0; ch<numChannel; ch++)
          free(*(PicolaBuffer+ch));
      free(PicolaBuffer);
    }     /* if (!testBitStream) */
  }
  return 0;
}

#ifdef VERSION2

static void MakeInfoFileName(char *bitFileName, char *infoFileName)
{
  char *writepos;
  short len;

  assert( INF_FILE_LEN > (int) strlen(bitFileName) );
  strcpy(infoFileName, bitFileName);
  len = strlen(bitFileName);
  writepos = infoFileName + len;
  /* Modified by Sony on 990616 */
  /* if ((len >= 4) && (!strcmp(writepos - 4, ".bit"))) */
  if ((len >= 4) && (!strcmp(writepos - 4, ".bit") || !strcmp(writepos - 4, MP4_BIT_EXT)))
    writepos -= 4;
  sprintf(writepos,".inf");
}

#endif /* VERSION2 */

/* ---------- main ---------- */

int main (int argc, char *argv[])
{
  char *progName = "<no program name>";
  int  result;
  char bitFileName[STRLEN];
  char decFileName[STRLEN];
  char *decFormat;
  int  fileIdx;

  /* evaluate command line  */
  CmdLineInit(0);
  result = CmdLineEval(argc,argv,paraList,switchList,1,&progName);
  if (result) {
    if (result==1) {
      printf("%s: %s\n",progName,PROGVER);
      CmdLineHelp(progName,paraList,switchList,stdout);
      DecParInfo(stdout);
      DecLpcInfo(stdout);
      DecTfInfo(stdout);
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
    printf("%s\n",DecParInfo(NULL));
    printf("%s\n",DecLpcInfo(NULL));
    printf("%s\n",DecTfInfo(NULL));
  }

  CmdLineInit(cmdDebugLevel);

  /* calc variable default values */
  if (!decBitRateUsed)
    decBitRate = 0;
  if (!regionDuratUsed)
    regionDurat = -1;
  if (!bitPathUsed)
    bitPath = getenv(MP4_BIT_PATH_ENV);
  if (!decPathUsed)
    decPath = getenv(MP4_DEC_PATH_ENV);
  if (!decExtUsed)
    decExt = getenv(MP4_DEC_FMT_ENV);
  if (decExt==NULL)
    decExt = MP4_DEC_EXT;

  /* check command line options */
  if (decBitRate < 0)
    CommonExit(1,"decoder bit rate < 0");
  if (numChannelOut < 0)
    CommonExit(1,"number of audio channels < 0");
  if (fSampleOut < 0)
    CommonExit(1,"sampling frequency < 0");
#ifndef MOUSECHANGE
  if (speedFact <= 0)
    CommonExit(1,"speech change factor <= 0");
  if (pitchFact <= 0)
    CommonExit(1,"pitch change factor <= 0");
#endif
  if (regionStart < 0)
    CommonExit(1,"start of region < 0");
  if (regionDuratUsed && regionDurat < 0)
    CommonExit(1,"duration of region < 0");
  if (outFileNameUsed && varArgIdx[0]>=0 && varArgIdx[1]>=0 &&
      strcmp(outFileName,"-")!=0)
    CommonExit(1,"only one input file allowed when using -o");
  if (varArgIdx[0]<0)
    CommonExit(1,"no input file specified");

  if (!bitExtUsed && strstr(decPara,"-mp4ff"))
    bitExt = ".fl4";

  /* process all files on command line */
  fileIdx = 0;
  if ((!outFileNameUsed || strcmp(outFileName,"-"))
      && strstr(decPara,"-out")) {
    /* prefix decExt with _layX[.]   HP 20001208 */
    unsigned int scalOutSelect ;
    char *outLayNr,*tmpStr;
    tmpStr = (char*)malloc((strlen(decExt)+10)*sizeof(char));
    outLayNr = strstr(decPara,"-out");
    outLayNr += 4;
    if (*outLayNr=='=' || *outLayNr==',')
      outLayNr++;
    sscanf(outLayNr,"%ud",&scalOutSelect);
    if ( scalOutSelect>10 ) {
      CommonExit(-1,"error in output layer select %d  ",scalOutSelect);
    } 
    sprintf(tmpStr,"_lay%1d%s%s",
	    scalOutSelect,strchr(decExt,'.')?"":".",decExt);
    decExt = tmpStr;
  }

#ifdef MOUSECHANGE
  InitMouse(speedFact,pitchFact);
#endif

  while (loopInFile) {

    /* compose file names */
    if (ComposeFileName(argv[varArgIdx[fileIdx]],0,bitPath,bitExt,bitFileName,
                        STRLEN))
      CommonExit(1,"composed file name too long");
#ifdef VERSION2
    if ( epFlag )
      MakeInfoFileName(bitFileName, infoFileName);
#endif
    if ( outFileNameUsed ) {
      if (ComposeFileName(outFileName,0,decPath,decExt,decFileName,STRLEN))
        CommonExit(1,"composed file name too long");
    }
    else
      if (ComposeFileName(argv[varArgIdx[fileIdx]],1,decPath,decExt,
                          decFileName,STRLEN))
        CommonExit(1,"composed file name too long");

    /* extract file format (extension) */
    if (strcmp(decFileName,"-") == 0)
      decFormat = decExt;
    else {
      decFormat = decFileName+strlen(decFileName);
      do
        decFormat--;
      while (decFormat>decFileName && *decFormat!='.');
    }

    /* decode file */
    if (mainDebugLevel == 1)
      fprintf(stderr,"decoding %s -> %s\n",bitFileName,decFileName);
    if (mainDebugLevel >= 2)
      printf("decoding %s -> %s\n",bitFileName,decFileName);

    /* info file ------- appended by Sony on 990616 */
#ifdef VERSION2
    if ( epFlag ) {
      if (mainDebugLevel == 1) 
        fprintf(stderr,"crc information - %s\n",infoFileName);
      if (mainDebugLevel >= 2)
        printf("crc information - %s\n",infoFileName);
    }
#endif

#ifdef MP4FF

    if (MP4Audio_ProbeFile (bitFileName))
      {
        MP4Audio_DecodeFile (bitFileName,
                             decFileName,
                             decFormat,
                             int24flag,
                             numChannelOut,
                             fSampleOut,
                             speedFact,
			     pitchFact,
                             decPara,
                             mainDebugLevel,
                             audioDebugLevel,
                             aacDebugString) ;
      }
    else
      {

#endif

        if ( Decode ( bitFileName,
                      decFileName,
                      decFormat,
                      decBitRate,
                      testBitStream,
                      numChannelOut,
                      fSampleOut,
                      speedFact,
                      pitchFact,
                      decPara,
                      regionStart,
                      regionDurat
#ifdef VERSION2
                      ,epFlag,
                      infoFileName
#endif
                      ))
          CommonWarning("error decoding audio file %s",bitFileName);

#ifdef MP4FF
      }
#endif

    fileIdx++;
    if (varArgIdx[fileIdx] < 0) {
      fileIdx = 0;
      loopInFile--;
    }
  }

#ifdef MOUSECHANGE
  FreeMouse();
#endif

  CmdLineEvalFree(paraList);

#ifdef COREHACK
  /* Use   make all CFLAGS=-DCOREHACK   to enable this hack */
  /* It must not be enabled when using   make all TF=0   HP 980212 */
  {
    /* dirty hack for now. What we need is a de-init function for the tf / scalable decoder module */
    extern void CloseCoreDecoder( void );
    CloseCoreDecoder();  /* flush the core bitstream */
  }
#endif

#ifdef VERSION2
  StatisticsPrint ( epFlag ) ;
#endif
  if (mainDebugLevel == 1)
    fprintf(stderr,"%s: finished\n",progName);
  if (mainDebugLevel >= 2)
    printf("%s: finished\n",progName);

  return 0;
}

/* end of mp4dec.c */

