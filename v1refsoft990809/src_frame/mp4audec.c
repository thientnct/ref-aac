/**********************************************************************
MPEG-4 Audio VM
Decoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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



Source file: mp4dec.c

$Id: mp4audec.c,v 1.15 1999/08/09 16:21:39 purnhage Exp $

Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bit stream module
audio.o                 audio i/o module
dec_par.o               decoder core (parametric)
dec_lpc.o               decoder core (CELP)
dec_tf.o                decoder core (T/F)
dec_g729.o              decoder core (G729)
dec_g723.o              decoder core (G723)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
CCETT N.N., CCETT <@ccett.fr>

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
**********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "bitstreamStruct.h"     /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "adif.h"

#include "common_m4a.h"     /* common module */
#include "cmdline.h"    /* command line module */
#include "bitstream.h"  /* bit stream module */
#include "audio.h"      /* audio i/o module */

#include "mp4au.h"        /* frame work common declarations */

#include "flex_mux.h"
#include "lpc_common.h"

#include "hvxc_struct.h"	/* struct for HVXC(AI 990616) */

#include "dec_par.h"    /* decoder cores ... */
#include "dec_lpc.h"
#include "dec_tf.h"
#include "dec_g723.h"
#include "dec_g729.h"

#include "plotmtv.h"

#include "pan_picola.h" /* for speed control (PICOLA) */

#include "buffers.h"

#ifdef MP4FF
#include "mp4ifc.h"
#endif

/* ---------- declarations ---------- */

#define PROGVER "MPEG-4 Natural Audio Decoder V1.0 30-Jul-1999"
#define CVSID "$Id: mp4audec.c,v 1.15 1999/08/09 16:21:39 purnhage Exp $"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ---------- variables ---------- */

/* command line module */

static float decBitRate;
static int   decBitRateUsed;
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
static char* bitPath;
static char* decPath;
static int   bitPathUsed;
static int   decPathUsed;
static char* bitExt;
static char* decExt;
static int   decExtUsed;
static int   testBitStream = 0;
static int   bitBufSize;
static int   mainDebugLevel;
static int   audioDebugLevel;
static int   bitDebugLevel;
static int   cmdDebugLevel;
static char* aacDebugString; /* Debug options for aac */
static int   epDebugLevel;
static int*  varArgIdx;




/* memory for following data should be allocated dynamically in the future */
static FRAME_DATA frameData;
static TF_DATA tfData;
static LPC_DATA  lpcData;
static ntt_DATA nttData;
static FAULT_TOLERANT_DATA faultData;
static  HANDLE_FAULT_TOLERANT hFault =  &faultData;

static HvxcDecStatus *hvxcData;		/* HVXC decoder handle (AI 990616) */

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
  {"n",&numChannelOut,"%d","0",NULL,"number of audio channels "
   "(0=as original)"},
  {"s",&fSampleOut,"%f","0",NULL,"sampling frequency [Hz] "
   "(0=as original)"},
  {"sf",&speedFact,"%f","1",NULL,"speed change factor"},
  {"pf",&pitchFact,"%f","1",NULL,"pitch change factor"},
  {"c",&decPara,"%s","",NULL,
   "decoder parameter string"},
  {"ts",&regionStart,"%f","0",NULL,"start time of decoded region [sec]"},
  {"td",&regionDurat,"%f",NULL,&regionDuratUsed,
   "duration of decoded region [sec]\n"
   "(dflt: until end of bit stream)"},
  {"o",&outFileName,"%s",NULL,&outFileNameUsed,
   "output file name (- = stdout)"},
  {"pb",&bitPath,"%s",NULL,&bitPathUsed,
   "default path bit stream files (dflt: $" MP4_BIT_PATH_ENV ")"},
  {"pd",&decPath,"%s",NULL,&decPathUsed,
   "default path decoded audio files (dflt: $" MP4_DEC_PATH_ENV ")"},
  {"eb",&bitExt,"%s",MP4_BIT_EXT,NULL,"bit stream file extension"},
  {"ed",&decExt,"%s",NULL,&decExtUsed,
   "decoded audio file extension\n"
   "supported file formats: .au, .snd, .wav, .aif, .raw\n"
   "(dflt: $" MP4_DEC_FMT_ENV " / " MP4_DEC_EXT ")\n"
   "(raw format: integer16, native)"},
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
static void    flexMuxDecInit ( int*         numChannel,                               
                                char*        decPara,     /* in: decoder parameter string */
                                FRAME_DATA*  fD,          /* config data ,one bitstream buffer for each layer, 
                                                             obj descr. etc. */
                                TF_DATA*     tfD,
                                LPC_DATA*    lpcD,
                                ntt_DATA*    nttData,
				HvxcDecStatus**	hvxcD,	/* AI 990616 */
                                char*        audioFileName,
                                float*       fSampleOut )
{
  unsigned int layer,numLayer;
  float fSample;
  int   tfInit = 0;
  char* outLayNr;
  /*   float delayNumSample; */
  /*   float decBitRate; */
  /*   int   frameMaxNumSample; */

  if ( ( outLayNr = strstr( decPara, "-out") ) ){
    sscanf(&outLayNr[4],"%d",&(fD->scalOutSelect));
    if ( fD->scalOutSelect > 7  ){
      CommonExit(-1,"\n errorn in output layer select %d  ",fD->scalOutSelect);
    } 
    if ( mainDebugLevel ) {
      printf("\n layer nr. %d is selected \n",fD->scalOutSelect);
    }
  }

  fD->scalOutSelect = min(fD->scalOutSelect+1,fD->od->streamCount.value)-1;
  numLayer=fD->scalOutSelect+1;
  *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];

  if (fD->scalOutSelect == 0 ) {
    if ((strstr(decPara, "-celp_mdct") == NULL)){    
      /* write only core signal at core sampling rate */
      *fSampleOut = samplFreqIndex[fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
    } else {
      numLayer++; /* for debugging purposes: init tf layer as well */
    } 
  }

  for ( layer = 0; layer < numLayer; layer++ ){

    AUDIO_SPECIFIC_CONFIG *audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;
    fSample=samplFreqIndex[fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];

    fD->layer[layer].bitBuf = BsAllocBuffer(64000); /* currently in dec_tf.c, line 529 */
    
    switch (audSpC->channelConfiguration.value) {
    case 1 :*numChannel=1;       
      break;
    case 2 :*numChannel=2;       
      break;
    default: CommonExit(1,"wrong channel config");
    }
    switch (audSpC->audioDecoderType.value) {
    case PARA:
#if 0
        DecParInit(*numChannel,fSample,decBitRate,decPara,
                   bitHeader,&frameMaxNumSample,&delayNumSample
		   );
#else
#pragma warning "DecParInit is still missing for mp4ff mode "
      CommonExit(1,"mp4ff for para not yet supported");
#endif
      break;
      /* AI 990616 */
    case HVXC:	
      *hvxcD = DecHvxcInitNew(decPara,
			      fD,
			      layer
			      );
      break;
      
    case CELP:

      DecLpcInitNew(decPara,
                    fD,
                    lpcD,
                    layer
                    );
      break;
    case GA:
      if (!aacEOF) {
        CommonWarning("Decode: option '-aaceof' is recommended in AAC mode");
      }
      if (tfInit==0){ /* call it only once for all aac layers */
        tfInit=1;
        DecTfInitNew ( 
                      decPara,
                      aacDebugString,
                      &hFault->hVm,
                      &hFault->hHcrSpecData,
                      fD,
                      tfD,
                      lpcD,
                      nttData,
                      mainDebugLevel,
                      layer
                      );
      }
      break;
    default : CommonExit(1,"unknown decoder type");
    }
  }   /* for (layer= ...) */
}

static void    flexMuxDec(BsBitStream* bitStream,
                          int          numChannel, 
                          FRAME_DATA*  fD, /* config data ,one bitstream buffer for each layer, 
                                              obj descr. etc. */
                          TF_DATA*     tfD,
                          LPC_DATA*    lpcD,                         
                          ntt_DATA*    nttData,
			  HvxcDecStatus* hvxcD,	/* AI 990616 */
                          AudioFile*   audioFile,
                          HANDLE_FAULT_TOLERANT hFault                
                          )
{
  int i_ch,numBits;
  unsigned int i;
  unsigned int layer,frame=0;
  unsigned int lastLayer = fD->od->streamCount.value-1 ;
  unsigned int audioDecType,coreDecType ,decType ;
  unsigned int haveAllAU;
  int          eofFlag=0;
  /* loop while bit in bitstream */

  if (  fD->od->ESDescriptor[lastLayer]->DecConfigDescr.streamType.value != 6)
    CommonExit(1,"not a audio object");
 
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
    plotInit(framePlot);
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
      /* call actual decoding functions */
      switch (decType) {
        
      case 0 :
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
          double fact=((double)32767.0/(double)37594.0); 
          for( i=0; i<(unsigned int)tfD->block_size_samples; i++ ) {
            tfD->sampleBuf[i_ch][i] = (float)((tfD->time_sample_vector[i_ch][i])*fact);
          }
        }
        AudioWriteData(audioFile,tfD->sampleBuf,tfD->block_size_samples);
      
        break;
      case 1 :
        DecLpcFrameNew(fD->layer[0].bitBuf,
                       lpcD->sampleBuf,
                       lpcD,
                       &numBits
                       );
/*        fD->layer[0].bitBuf->numBit -=numBits; * work around as long as celp decoder 
                                                  does not use official bitstream functions */
        fD->layer[0].NoAUInBuffer--; /* normaly done by the decoder itself */
        AudioWriteData(audioFile,lpcD->sampleBuf,lpcD->frameNumSample);  
        break;
        
    case 5:	/* hvxc decode using system interface(AI 990616)  */
      if(SpeedControlMode) {
	DecHvxcFrame(hvxcD,
		     fD->layer[0].bitBuf,
		     hvxcD->sampleBuf,
		     &numBits,
		     &hvxcD->frameNumSample,
		     1,
		     pitchFact
		     );
      }
      else {
	DecHvxcFrame(hvxcD,
		     fD->layer[0].bitBuf,
		     hvxcD->sampleBuf,
		     &numBits,
		     &hvxcD->frameNumSample,
		     speedFact,
		     pitchFact
		     );
      }
      fD->layer[0].NoAUInBuffer--; /* normaly done by the decoder itself */
      AudioWriteData(audioFile,hvxcD->sampleBuf,hvxcD->frameNumSample);  
      break;
      
    default :
      CommonExit(1," streamType not yet supported ");
      break;
    }


      /* write audio buffers to file */

      if (mainDebugLevel >= 1) {
        fprintf(stderr,"\rframe %4d ",frame);
        fflush(stderr);
      }
      frame++;
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
                   int   testBitStream,          /* in: 0=normal decoding */
				/*     1=test bit stream (disable decoder) */
                   int   numChannelOut,          /* in: num audio channels */
                   float fSampleOut,             /* in: sampling frequency [Hz] */
                   float speedFact,              /* in: speed change factor */
                   float pitchFact,              /* in: pitch change factor */
                   char *decPara,                /* in: decoder parameter string */
                   float regionStart,		/* in: start time of region */
                   float regionDurat             /* in: duration of region */
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
  BsBitBuffer*       bitBuf = NULL;
  float**            sampleBuf = NULL;
  BsBitStream*       bitStream;
  AudioFile*         audioFile = NULL;
  BsBitBuffer*       bitHeader;
  int                numSample;
  int                frameNumBit,frameAvailNumBit,usedNumBit;
  int                frameDecNumBit;
  int                headerNumBit;
  long               totDataNumBit,totPadNumBit;
  int                ch;
  int                bsVersion;
  int                decNumFrame;
  HANDLE_RESILIENCE  hResilience  = NULL;
  HANDLE_BUFFER      hVm          = NULL;
  HANDLE_BUFFER      hHcrSpecData = NULL;
  HANDLE_HCR         hHcrInfo     = NULL;
  HANDLE_EP_INFO     hEpInfo      = NULL;
  HANDLE_CONCEALMENT hConcealment = NULL;
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

  int                EOFreached = 0;
  unsigned char      decodeNext;
#if 0
  int                alignBits;
  int                padNumBit;
#endif 
  
  /* init */
  if (mainDebugLevel >= 3) {
    printf("Decode:\n");
    printf("bitFileName=\"%s\"\n",bitFileName);
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
  initObjDescr( &objDescr);
  AudioInit(NULL,audioDebugLevel);
  info = NULL;
  
  /* open bit stream file */
  if ((strstr(decPara, "-aac_raw") != NULL) ||
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
      for (x=0;x<objDescr.streamCount.value;x++){
        int frameNumBitLay;
        initESDescr(&(objDescr.ESDescriptor[x]));
        advanceESDescr(bitStream,objDescr.ESDescriptor[x],0);
        fSampleLong=samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
        frameNumBitLay = ( objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) 
          *  objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value / fSampleLong ;

        sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;

        frameNumBitLay += (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
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
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.channelConfiguration: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value);

          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.dependsOnCoreCoder.value);
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.coreCoderDelay.value);

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
      mode = MODE_TF;
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
        fSampleLong=samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
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
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.channelConfiguration: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value);

          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.TFCodingType.value);
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

    } else if(strstr(decPara, "-hvxc_sys") !=NULL) {	/* AI 990616 */
      unsigned long length,align,dummy,odStartByte;
      bitHeaderNumBit=0;

      if (BsGetBit(bitStream,&(objDescr.ODLength.value),objDescr.ODLength.length) )
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      odStartByte=BsCurrentBit(bitStream)/8;

      if (BsCurrentBit(bitStream) % 8 != 0 )CommonExit(-1,"error in reading object descr");

      if ( BsGetBit(bitStream,&(objDescr.ODescrId.value),objDescr.ODescrId.length) ||
           BsGetBit(bitStream,&(objDescr.streamCount.value),objDescr.streamCount.length) ||
           BsGetBit(bitStream,&(objDescr.extensionFlag.value),objDescr.extensionFlag.length))
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      if (mainDebugLevel >= 3) {
        printf("ObjectDescriptor.ODLength: %ld \n",objDescr.ODLength.value);
        printf("ObjectDescriptor.ODescrId: %ld \n",objDescr.ODescrId.value);
        printf("ObjectDescriptor.streamCount: %ld \n",objDescr.streamCount.value);
        printf("ObjectDescriptor.extensionFlag: %ld \n",objDescr.extensionFlag.value);
      }

      frameNumBit=0;
      bitRateLong=0;
      bitReservSize = 0;
      for (x=0;x<objDescr.streamCount.value;x++){
        int frameNumBitLay;
        initESDescr(&(objDescr.ESDescriptor[x]));
        advanceESDescr(bitStream,objDescr.ESDescriptor[x],0);
        fSampleLong=samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
        frameNumBitLay = ( objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.frameLength.value ? 960:1024) 
          *  objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value / fSampleLong ;

        sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;

        frameNumBitLay += (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
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
          printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.channelConfiguration: %ld \n",
                 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value);
	  
	  printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode: %ld \n",
		 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value);
	  printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode: %ld \n",
		 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value);
	  printf("  ESDescriptor.DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag: %ld \n",
		 objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag.value);
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

      bitReservInit= bitReservSize;       
      bitHeaderNumBit=0;
      mode = MODE_HVXC;
      numChannelOri=objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
      frameData.od = &objDescr;

    } else if(strstr(decPara, "-celp_sys") !=NULL) {
      unsigned long length,align,dummy,odStartByte;
      bitHeaderNumBit=0;

      if (BsGetBit(bitStream,&(objDescr.ODLength.value),objDescr.ODLength.length) )
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      odStartByte=BsCurrentBit(bitStream)/8;

      if (BsCurrentBit(bitStream) % 8 != 0 )CommonExit(-1,"error in reading object descr");

      if ( BsGetBit(bitStream,&(objDescr.ODescrId.value),objDescr.ODescrId.length) ||
           BsGetBit(bitStream,&(objDescr.streamCount.value),objDescr.streamCount.length) ||
           BsGetBit(bitStream,&(objDescr.extensionFlag.value),objDescr.extensionFlag.length))
        CommonExit(1,"Decode: error reading bit stream header (frame)");
      if (mainDebugLevel >= 3) {
        printf("ObjectDescriptor.ODLength: %ld \n",objDescr.ODLength.value);
        printf("ObjectDescriptor.ODescrId: %ld \n",objDescr.ODescrId.value);
        printf("ObjectDescriptor.streamCount: %ld \n",objDescr.streamCount.value);
        printf("ObjectDescriptor.extensionFlag: %ld \n",objDescr.extensionFlag.value);
      }

      frameNumBit=0;
      bitRateLong=0;
      bitReservSize = 0;
      for (x=0;x<objDescr.streamCount.value;x++){
        int frameNumBitLay, frameLength, celp_sys_align;

        initESDescr(&(objDescr.ESDescriptor[x]));
        advanceESDescr(bitStream,objDescr.ESDescriptor[x],0);

        fSampleLong=samplFreqIndex[objDescr.ESDescriptor[x]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
        frameLength = lpcframelength( &(objDescr.ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig) );
        frameNumBitLay = objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value * frameLength / fSampleLong ;

        sn_ovrhd = objDescr.ESDescriptor[x]->ALConfigDescriptor.seqNumLength.value%8 ? 1:0;

	celp_sys_align = 8 - (frameNumBitLay % 8);
	if (celp_sys_align == 8) celp_sys_align = 0;

        frameNumBitLay += celp_sys_align + (3+sn_ovrhd)*8*(1 + (frameNumBitLay/2040)) ; /* overhead for flexMux transport */
        frameNumBit += frameNumBitLay;
        bitRateLong += objDescr.ESDescriptor[x]->DecConfigDescr.avgBitrate.value;
        bitReservSize += objDescr.ESDescriptor[x]->DecConfigDescr.bufferSizeDB.value;
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

      bitReservInit= bitReservSize;       
      bitHeaderNumBit=0;
      mode = MODE_LPC;
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
        CommonExit(-1,"\n wrong sample rate %d \n",fSampleLong);
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
    if (frameMinNumBit < frameNumBit)
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
    printf("bitRate=%.3f bit/sec  (int=%ld)\n",bitRate,bitRateLong);
    printf("bitReservSize=%d bit  (%.6f sec)\n",
           bitReservSize,bitReservSize/bitRate);
    printf("bitReservInit=%d bit\n",bitReservInit);
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



  if (strstr(decPara, "-mp4ff") != NULL ||
      strstr(decPara, "-hvxc_sys") != NULL ||	/* AI 990616 */
      strstr(decPara, "-celp_sys") != NULL ) {

    flexMuxDecInit ( &numChannel, 
                     decPara,
                     &frameData,
                     &tfData,
                     &lpcData,
                     &nttData,
		     &hvxcData,	/* AI 990616 */
                     audioFileName,
                     &fSample );    
    if (strstr(decPara, "-hvxc_sys") != NULL) {	
      /* delay compensation for HVXC(AI 99060XX) */
      delayNumSample = hvxcData->delayNumSample;
    }
    else {
      delayNumSample=0;
    }
  } else {
    if (!testBitStream) {
      switch (mode) {
      case MODE_PAR:
	DecParInit(numChannel,fSample,decBitRate,decPara,
		   bitHeader,&frameMaxNumSample,&delayNumSample
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
			    );	
	break;
      case MODE_LPC:
	DecLpcInit(numChannel,fSample,decBitRate,decPara,
		   bitHeader,&frameMaxNumSample,&delayNumSample);
	break;
      case MODE_G729:
	DecG729Init(numChannel,fSample,decBitRate,
                    &frameMaxNumSample,&delayNumSample);
	break;
      case MODE_G723:
	DecG723Init(numChannel,fSample,decBitRate,decPara,
		    &frameMaxNumSample,&delayNumSample);
	break;
      case MODE_TF:
        if (!aacEOF) {
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
                    &hVm,
                    &hHcrSpecData,
                    (frameData.od == NULL ? NULL : &frameData),
                    &tfData,
                    &nttData,
                    mainDebugLevel
                    );
        hFault->hVm =  hVm ;
        hFault->hHcrSpecData =hHcrSpecData  ;
        hFault->hHcrInfo =hHcrInfo  ;
        hFault->hEpInfo = hEpInfo ;
        hFault->hConcealment =  hConcealment ;
        hFault->hResilience  = hResilience  ;
        
        break;
      }
    }     /* if (!testBitStream) */
  }

  /* speed control initialization */
  if(SpeedControlMode)
    PicolaInit(frameMaxNumSample, fSample, speedFact);

  BsFreeBuffer(bitHeader);

  if (!testBitStream) {
    if (strstr(decPara, "-mp4ff") == NULL &&
	strstr(decPara, "-celp_sys") == NULL &&
	strstr(decPara, "-hvxc_sys") == NULL) {	/* AI 990616 */
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

  if (strstr(decPara, "-mp4ff") != NULL ||
      strstr(decPara, "-celp_sys") != NULL ||
      strstr(decPara, "-hvxc_sys") != NULL) {	/* AI 990616 */
    flexMuxDec(bitStream,
               numChannel, 
               &frameData,
               &tfData,
               &lpcData,
               &nttData,
	       hvxcData,	/* AI 990616 */
               audioFile,
               hFault                
               );
  } else {
#ifdef FHG_DEBUGPLOT
    plotInit(frame);
#endif

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

        decodeFrame = !testBitStream && frame >= 0;
        if (decodeFrame) {


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

	  if (bitBuf->numBit < frameMinNumBit) break;

	  if (mainDebugLevel == 1) {
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

          /* decode one frame */
          switch ( mode ) {
	  case MODE_PAR: 
	    if(SpeedControlMode)
	      DecParFrame(bitBuf,sampleBuf,&usedNumBit,&frameBufNumSample,
			  1,pitchFact
			  );
	    else
	      DecParFrame(bitBuf,sampleBuf,&usedNumBit,&frameBufNumSample,
			  speedFact,pitchFact
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
			   );
	    else
	      DecHvxcFrame(hvxcData,
			   bitBuf,sampleBuf,
			   &usedNumBit,
			   &frameBufNumSample,
			   speedFact,
			   pitchFact
			   );
	    break;
          case MODE_LPC:
            DecLpcFrame(bitBuf,sampleBuf,&usedNumBit);
            frameBufNumSample = frameMaxNumSample;
            break;
          case MODE_G729:
            DecG729Frame(bitBuf,sampleBuf,&usedNumBit,0);
            frameBufNumSample = frameMaxNumSample;
            break;
          case MODE_G723:
            DecG723Frame(bitBuf,sampleBuf,&usedNumBit);
            frameBufNumSample = frameMaxNumSample;
            break;
          case MODE_TF:
            DecTfFrame ( bitBuf,
                         sampleBuf,
                         &usedNumBit,
                         numChannel, 
                         mainDebugLevel,
                         hFault,
			 hResilience,
                         hVm,
                         hHcrSpecData,
                         hHcrInfo,
                         (frameData.od == NULL ? NULL : &frameData),
                         &tfData,
                         &nttData,
                         hEpInfo,
                         hConcealment);
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

        if ((usedNumBit != (bitBuf->size - bitBuf->numBit)) && !EOFreached ){
          CommonWarning ("\nsome bits are missing!!\n");
          usedNumBit = (bitBuf->size - bitBuf->numBit);
        }
            
        /* byte alignment */
#if 0
        alignBits = 8 - usedNumBit - ((usedNumBit/8)*8);
        if (alignBits<8){
          BsBitStream * stream = BsOpenBufferRead(bitBuf);
          BsGetSkip(stream,alignBits);
          usedNumBit += alignBits;
          BsCloseRemove(stream,1);          
        }
#endif

        refillBits=usedNumBit;
        
        
#if 0
        /* at this level padding bits should not occur:
           for flexmux or mp4 file format, the padding bits are at the end of an access unit and are handled therefor 
           by the flexmux or mp4 file format framework
           for raw AAC bitstreams padding bits only occur in 'fillelements' and are decoded in the aac decoder itself.
           for all other  coders padding should not occure in raw bitstreams because padding makes only sense in transprort
           streams. but neither raw bitstreams nor flexmux or file format bitstremas are actually transport streams.
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
              BsBitStream * stream = BsOpenBufferRead(bitBuf);
              BsGetSkip(stream,padNumBit);
              usedNumBit += padNumBit;
              BsCloseRemove(stream,1);

            }
          totPadNumBit += padNumBit;
        }
#endif
        
        /* advance in bit stream */
        if ( frameDecNumBit ) {
          usedNumBit = frameNumBit;
        }

#if 0	/* we need this   HP 990422 */
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
           
        */

	if (BsGetSkip(bitStream,usedNumBit))
	  CommonExit(1,"Decode: error advancing in bit stream data");
#endif
        frameAvailNumBit -= usedNumBit;

        /* variable bit rate: don't exceed bit reservoir size */
        if (frameAvailNumBit > bitReservSize)
          frameAvailNumBit = bitReservSize;

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

#if 1	/* AI 990528 */
        if ( EOFreached ) {
	  /* if EOF is reached, check whether minimum bits are left for
	     decode in bitBuf  */
          decodeNext = (( bitBuf->numBit /*BsGetBufferFullness (bitBuf)*/ > ((aacEOF) ? 8 : frameMinNumBit-1))
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
#else
        if ( EOFreached ) {
          decodeNext = (( bitBuf->numBit > ((aacEOF) ? 8 : frameAvailNumBit))
                        && (( decNumFrame < 0 ) || (frame < decNumFrame))) ;
        }  
        else {
          decodeNext = ((!BsEof ( bitStream, (aacEOF) ? 8 : frameAvailNumBit - 1 )) 
                        && (( decNumFrame < 0 ) || (frame < decNumFrame))) ;
        }
#endif        

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
  case MODE_G729:
    printf("DecG729Free to be defined.\n");
    break;
  case MODE_G723:
    printf("DecG723Free to be defined.\n");
    break;
  case MODE_TF:
    DecTfFree ( hEpInfo, hResilience );
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
    if (strstr(decPara, "-mp4ff") == NULL &&
	strstr(decPara, "-celp_sys") == NULL &&
	strstr(decPara, "-hvxc_sys") == NULL) {	/* AI 990616 */
      
    
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
      DecG729Info(stdout);
      DecG723Info(stdout);
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
    printf("%s\n",DecG723Info(NULL));   
    printf("%s\n",DecG729Info(NULL));
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
  if (speedFact <= 0)
    CommonExit(1,"speech change factor <= 0");
  if (pitchFact <= 0)
    CommonExit(1,"pitch change factor <= 0");
  if (regionStart < 0)
    CommonExit(1,"start of region < 0");
  if (regionDuratUsed && regionDurat < 0)
    CommonExit(1,"duration of region < 0");
  if (outFileNameUsed && varArgIdx[0]>=0 && varArgIdx[1]>=0)
    CommonExit(1,"only one input file allowed when using -o");
  if (varArgIdx[0]<0)
    CommonExit(1,"no input file specified");
  
  /* process all files on command line */
  fileIdx = 0;
  if ( outFileNameUsed ) {
    char tmpStr[1000];
    int scalOutSelect ;
    char* tmpOutfname;
    char* outLayNr;

    /* add 5 chars */
    tmpOutfname= (char*)malloc((strlen(outFileName)+10)*sizeof(char));
    strcpy(tmpOutfname,outFileName);         
    outFileName=tmpOutfname;
 
    if ( ( outLayNr = strstr( decPara, "-out") ) ){
      sscanf(&outLayNr[4],"%d",&scalOutSelect);
      if ( (scalOutSelect < 0 ) || ( scalOutSelect > 7 ) ){
        CommonExit(-1,"\n errorn in output layer select %d  ",scalOutSelect);
      } 
      sprintf(tmpStr,"_lay%1d",scalOutSelect);
      strcat(outFileName,tmpStr);         
    }
  }
  
  while (varArgIdx[fileIdx] >= 0) {

    /* compose file names */
    if (ComposeFileName(argv[varArgIdx[fileIdx]],0,bitPath,bitExt,bitFileName,
                        STRLEN))
      CommonExit(1,"composed file name too long");
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

#ifdef MP4FF

    if (MP4Audio_ProbeFile (bitFileName))
      {
        MP4Audio_DecodeFile (bitFileName,
                             decFileName,
                             decFormat,
                             numChannelOut,
                             fSampleOut,
                             decPara) ;
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
                      ))
          CommonWarning("error decoding audio file %s",bitFileName);

#ifdef MP4FF
      }
#endif

    fileIdx++;
  }

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

  if (mainDebugLevel == 1)
    fprintf(stderr,"%s: finished\n",progName);
  if (mainDebugLevel >= 2)
    printf("%s: finished\n",progName);

  return 0;
}

/* end of mp4dec.c */

