/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by


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

Copyright (c) 1999.

*/

#include <stdio.h>
#include <string.h>

#include "ISOMovies.h"
/* defines missing in older ISOMovies.h */
#ifndef ISOOpenMovieNormal
#define ISOOpenMovieNormal MP4OpenMovieNormal
#endif

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "hvxc_struct.h"	 /* structs */
#include "monopredStruct.h"      /* structs */

#include "lpc_common.h"
#include "dec_tf.h"
#include "dec_lpc.h"
#include "dec_par.h"

#include "bitstream.h"
#include "common_m4a.h"
#include "mp4ifc.h"

#include "audio.h"      /* audio i/o module */
#include "plotmtv.h"

#include "allVariables.h"     /* for PCE support */
#include "port.h"             
#include "all.h"

#ifdef MOUSECHANGE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
extern Display *MOUSEdisp;
void GetMouse(float *speed, float *pitch);
#endif

int default_config=1; /* defined in allVariables.h as 'extern int default_config' */

typedef struct
{
  unsigned long objectTypeIndication ;
  unsigned long streamType ;
  unsigned long upStream ;
  unsigned long bufferSizeDB ;
  unsigned long maxBitrate ;
  unsigned long avgBitrate ;

  /* Audio */

  AUDIO_OBJECT_TYPE_ID audioObjectType ;
  unsigned long samplingFrequencyIndex ;
  unsigned long samplingFrequency ;
  unsigned long channelConfiguration ;
  ProgConfig    progConfig;


  /* GA */

  unsigned long frameLengthFlag ;
  unsigned long dependsOnCoreCoder ;
  unsigned long coreCoderDelay ;
  unsigned long extensionFlag ;
  unsigned long layerNr ;
#ifdef VERSION2
  unsigned long numOfSubFrame ;
  unsigned long layer_length ;
  unsigned long aacSectionDataResilienceFlag ;
  unsigned long aacScalefactorDataResilienceFlag ;
  unsigned long aacSpectralDataResilienceDataFlag ;
  unsigned long extension3Flag ;
#endif


  /* celp */
  unsigned long  excitationMode ;
  unsigned long  sampleRateMode ;
  unsigned long  fineRateControl ;

  unsigned long  silenceCompressionSW;
  unsigned long  RPE_Configuration ;

  unsigned long  MPE_Configuration ;
  unsigned long  numEnhLayers ;
  unsigned long  bandwidthScalabilityMode ;

  unsigned long  BWS_Configuration ;

  /* HVXC */
  unsigned long  HVXCvarMode ;
  unsigned long  HVXCrateMode ;
  unsigned long  excitationFlag ;
#ifdef VERSION2
  unsigned long  packMode ;
  unsigned long  vrScalFlag ;
#endif

#ifdef VERSION2
  /* HILN/PARA */
  unsigned long  PARAmode;

  unsigned long  HILNquantMode;
  unsigned long  HILNmaxNumLine;
  unsigned long  HILNsampleRateCode;
  unsigned long  HILNframeLength;
  unsigned long  HILNcontMode;

  unsigned long  PARAextensionFlag;

  unsigned long  HILNenhaLayer;
  unsigned long  HILNenhaQuantMode;
#endif
  
#ifdef CORRIGENDUM1
  unsigned long  isBaseLayer ;	/* common for CELP,HVXC and HILN */
  unsigned long  isBWSLayer ;	/* only for CELP */
  unsigned long  CELP_BRS_id ;	/* only for CELP */
#endif

  unsigned long  epConfig;

} DecoderConfigDescr ;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define MAXREADAHEAD 65536

static const unsigned long initialObjectDescrTag = 0x02 ;
static const unsigned long decoderConfigDescrTag = 0x04 ;
static const unsigned long decSpecificInfoTag    = 0x05 ;

#define MAXTRACKS 50

static int MP4Audio_ReadInitialOD (ISOMovie theMovie, int ignoreProfiles) ;
static int MP4Audio_ReadDecoderConfig (ISOHandle decConfigH, 
                                       DecoderConfigDescr *decConfig,
#ifndef CORRIGENDUM1
                                       DecoderConfigDescr *decConfig0, 
                                       unsigned long idx,
#endif
                                       HANDLE_DECODER_GENERAL hFault);
static int MP4Audio_LookForSuitableTracks (ISOMovie theMovie, unsigned long trackNumber [MAXTRACKS]) ;

static int MP4Audio_SetupDecoders (FRAME_DATA *fD, 
                                   DecoderConfigDescr decConfig [MAXTRACKS],
                                   int   streamCount,       /* in: number of ESs */
                                   char* decPara ,          /* in: decoder parameter string */
                                   HANDLE_DECODER_GENERAL hFault,
                                   int   mainDebugLevel,
                                   char* aacDebugString);
/*
static int MP4Audio_DecodeFrame (FRAME_DATA *fD, int maxLayers,
                                 ISOHandle theSample, unsigned long auLength,
                                 int numChannelOut) ;
*/
static HANDLE_BSBITBUFFER MP4Audio_BbFromHandle (ISOHandle theHandle) ;

static FRAME_DATA frameData;
static OBJECT_DESCRIPTOR objDescr;
static TF_DATA tfData ;
static LPC_DATA lpcData ;
static ntt_DATA nttData ;
static HvxcDecStatus *hvxcData;
static ParaDecStatus *paraData;

static T_DECODERGENERAL faultData;
static HANDLE_DECODER_GENERAL hFault = &faultData;
static AudioFile *audioFile = NULL;
static int delayNumSample = 0;
static int locMainDebugLevel=0;
int MP4Audio_ProbeFile (char *inFile)
{
  ISOMovie theMovie = NULL ; 
  ISOErr err = ISOOpenMovieFile ( &theMovie, inFile, ISOOpenMovieNormal ) ; 
  if (err == ISONoErr) 
  { 
    ISODisposeMovie (theMovie) ; 
    return 1 ; 
  } 
  else 
    return 0 ; 
}
extern  int framePlot;

#define SUBTRACKSIZE  16

int MP4Audio_DecodeFile (char* inFile,          /* in: bitstream input file name */
                         char* audioFileName,   /* in: audio file name */
                         char* audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                         int   int24flag,
                         int   numChannelOut,   /* in: num audio channels */
                         float fSampleOut,      /* in: sampling frequency [Hz] */
			 float speedFact,	/* in: speed change factor */
			 float pitchFact,	/* in: pitch change factor */
                         char* decPara,         /* in: decoder parameter string */
                         int   mainDebugLevel,
                         int   audioDebugLevel,
                         char* aacDebugString)
{
  ISOErr err;
  unsigned long framesDone ,EOFmask[4];
  int numBits,needNewAU;
  unsigned long trackNumber [MAXTRACKS] ;
  DecoderConfigDescr decConfig [MAXTRACKS] ;
  ISOTrackReader theReader [MAXTRACKS] ;
  HANDLE_BSBITBUFFER        locBitBuf = NULL;
  HANDLE_BSBITBUFFER     AUBuffer  = NULL;
  HANDLE_BSBITSTREAM       locBitStream = NULL;
  ISOMovie theMovie;
  ISOTrack theTrack;
  ISOMedia theMedia;
  ISOHandle sampleH ;
  ISOHandle decoderConfigH = NULL ;
  unsigned long idx, t, suitableTracks = 0;
  unsigned long sub_idx1, sub_idx2;
#ifdef VERSION2
  int epFlag=0;
#endif
  char buffer_empty;
  
  locBitBuf = BsAllocBuffer(MAXREADAHEAD);
  locMainDebugLevel= mainDebugLevel;

  sampleH = NULL ;
	

	theMovie = NULL;
	err = ISONoErr;
	err = ISOOpenMovieFile( &theMovie, inFile, ISOOpenMovieNormal ); if (err) goto bail;

  suitableTracks = MP4Audio_LookForSuitableTracks (theMovie, trackNumber) ;

  if (mainDebugLevel >= 1)
    fprintf((mainDebugLevel==1)?stderr:stdout,
	    "Found MP4 file with  %ld suitable Tracks \n",suitableTracks);

  if ( strstr( decPara, "-out") ){
    char *outLayNr = strstr(decPara,"-out")+4;
    if (*outLayNr=='=' || *outLayNr==',')
      outLayNr++;
    sscanf(outLayNr,"%ud",&frameData.scalOutSelect);
    if (frameData.scalOutSelect>=suitableTracks)
      CommonWarning("selected layer %d, but only layer 0..%d available",
		    frameData.scalOutSelect,suitableTracks-1);

#if 0
    /* to parse epConfig=1 bitstream,
       configurations of all tracks should be parsed to identify the number of tracks to parse) */
    suitableTracks = min(suitableTracks, frameData.scalOutSelect+1) ;
#endif

  }
  else
    /* Note: for epConfig==1 scalOutSelect need to be limited later! */
    frameData.scalOutSelect = suitableTracks-1 ;

  if (!suitableTracks) return ISOBadDataErr ;

#if 0
  /* the layer number to be output(scalOutSelect) is not determined by suitableTracks. */
  frameData.scalOutSelect = suitableTracks-1 ;
#endif

  /* if we got here, we found at least one audio track */

  BsInit (0,0,0);
  decConfig[0].epConfig = 0; /* default  HP20010214 */

  /* check the iod */

  if (MP4Audio_ReadInitialOD (theMovie, 0))
    return ISOBadDataErr ;

  err = ISONewHandle( 0, &decoderConfigH ); if (err) goto bail;

  /* read the theTrack */

  for (idx = 0 ; idx < suitableTracks ; idx++)
    {
      err = ISOGetMovieIndTrack( theMovie, trackNumber [idx], &theTrack ); if (err) goto bail;
      err = ISOGetTrackMedia( theTrack, &theMedia ); if (err) goto bail;
      err = ISOCreateTrackReader( theTrack, &theReader [idx] ); if (err) goto bail;
      err = MP4TrackReaderGetCurrentDecoderConfig( theReader [idx], decoderConfigH ); if (err) goto bail;
      
      if (MP4Audio_ReadDecoderConfig (decoderConfigH, 
                                      &decConfig [idx],
#ifndef CORRIGENDUM1

                                     
                                      &decConfig[idx], /* 0 --> idx */ 
                                      idx,

#endif
                                      hFault))
        return ISOBadDataErr ;
      frameData.layer[idx].AULength = (unsigned int *)malloc(MAX_AU*sizeof(unsigned int));

    }

	/*
	 instantiate proper decoder for decoderconfig
	*/

  MP4Audio_SetupDecoders ( &frameData, 
                           decConfig, 
                           suitableTracks,
                           decPara,
                           hFault,
                           mainDebugLevel,
                           aacDebugString) ;
  /* for epConfig==1, frameData.od->streamCount.value might have been modified */

  if (decConfig[0].epConfig == 1 
      && decConfig[0].audioObjectType == ER_HVXC) {
    /* for audioObjectType supporting epConfig=1 */
    /* modifiy suitableTracks */
    suitableTracks=frameData.od->streamCount.value;	
  }
  else
    /* HP20010214 */
    if ( decConfig[0].epConfig == 1
	 && (decConfig[0].audioObjectType == ER_HILN ||
	 decConfig[0].audioObjectType == ER_PARA)){
      suitableTracks = frameData.od->streamCount.value;	
    }
  else
    /* added by NTT 001213 */
    if ( decConfig[0].epConfig == 1
	 && decConfig[0].audioObjectType == ER_TWIN_VQ){
      suitableTracks=frameData.od->streamCount.value;	
    }
  else
    /* added by NEC 010110 */
    if ( decConfig[0].epConfig == 1
	 && decConfig[0].audioObjectType == ER_CELP){
      suitableTracks=frameData.od->streamCount.value;
    }
  else 
    /* added by Samsung 010215 */
    if ( decConfig[0].epConfig == 1
     && decConfig[0].audioObjectType == ER_BSAC) {
       if (frameData.scalOutSelect<=0)
         frameData.scalOutSelect = 1;
       suitableTracks=frameData.scalOutSelect+1;
       frameData.od->streamCount.value=suitableTracks;
    }
  else {
    /* for audioObjectType NOT supporting epConfig=1 */
    suitableTracks=frameData.scalOutSelect+1;
    frameData.od->streamCount.value=suitableTracks;
  }

  /* suitableTracks=frameData.od->streamCount.value gives the number of ES */
  /* actually used, depends on frameData.scalOutSelect  HP20010214 */
  fSampleOut = (float) decConfig[suitableTracks-1].samplingFrequency ;
  numChannelOut = decConfig[suitableTracks-1].channelConfiguration ;

  /* open audio file */
  /* (sample format: 16 bit twos complement, uniform quantisation) */

  AudioInit(NULL, int24flag, audioDebugLevel);

  audioFile = AudioOpenWrite(audioFileName,audioFileFormat,
                             numChannelOut,fSampleOut);
  if (audioFile==NULL)
    CommonExit(1,"Decode: error opening audio file %s "
               "(maybe unknown format \"%s\")",
               audioFileName,audioFileFormat);

  /* seek to beginning of first frame (with delay compensation) */
  AudioSeek(audioFile,-delayNumSample);


  err = ISONewHandle( 0, &sampleH ); if (err) goto bail;	

  /* loop through the file */
  EOFmask[0] = 0; EOFmask[1] = 0; 
  EOFmask[2] = 0; EOFmask[3] = 0; 
  sub_idx1 = suitableTracks / SUBTRACKSIZE;
  sub_idx2 = suitableTracks % SUBTRACKSIZE;
  EOFmask[sub_idx1] = (1<<sub_idx2)-1;
  for (idx = 0; idx < sub_idx1; idx++)   
     EOFmask[idx] = (1<<SUBTRACKSIZE) - 1; 

  idx=0;
  needNewAU=1;
  for (framesDone = 0, buffer_empty=0; buffer_empty==0; framesDone++)  {
#ifdef FHG_DEBUGPLOT
    plotInit(framePlot,audioFileName,0);
#endif
    while (needNewAU)  {
      u32 unitSize, cts, dts, sampleFlags;
      sub_idx1 = idx / SUBTRACKSIZE; 
      sub_idx2 = idx % SUBTRACKSIZE;
      if (  EOFmask[sub_idx1] & (1<<sub_idx2)) {
        err = MP4TrackReaderGetNextAccessUnit( theReader [idx], sampleH, &unitSize,&sampleFlags, &cts, &dts );
        if ( err ) {
          if ( err == ISOEOF ) {
            err = ISONoErr;
            EOFmask[sub_idx1] ^= 1<<sub_idx2;
            /* on end of file: terminate program except for AAC scalable with celp core (buffers need to be emptied)  (lzm) */
            if (EOFmask[0]==0 && EOFmask[1]==0 && EOFmask[2]==0 && EOFmask[3]==0) {
              if( !((decConfig[0].audioObjectType == 8) && (decConfig[suitableTracks-1].audioObjectType != 8)) ) {
                goto bail;
              }
            }
          } else {
            goto bail;
          }
        }

        memcpy (BsBufferGetDataBegin(locBitBuf), *sampleH, unitSize) ;        
        BsBufferManipulateSetNumBit(unitSize * 8,locBitBuf);  
        locBitStream=BsOpenBufferRead (locBitBuf);	       
        if  (BsBufferNumBit(BsGetBitBufferHandle(0,locBitStream)) %8 )
          CommonExit(1,"BsOpenBufferRead: byte align error");
        BsManipulateSetNumByte(BsBufferNumBit(BsGetBitBufferHandle(0,locBitStream)) /8, locBitStream);

        AUBuffer = frameData.layer[idx].bitBuf ;
        if (AUBuffer!=0 ) {
          if ((BsBufferFreeBits(AUBuffer)) > (long)unitSize*8 )  {
            BsGetBufferAppend( locBitStream, AUBuffer,1,unitSize*8);    
            frameData.layer[idx].AULength[frameData.layer[idx].NoAUInBuffer]= unitSize*8;
            frameData.layer[idx].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */
            /* each decoder must remove the first element in the list  by shifting the elements down */
          }  else {
            BsGetSkip(locBitStream,unitSize*8);
            CommonWarning (" input buffer overflow for layer %d ; skiping next AU",idx);
          }
        } else {
          BsGetSkip(locBitStream,unitSize*8);
        }
        BsCloseRemove (locBitStream,1);        
      }

      needNewAU=0;        
      if (  (decConfig [0].audioObjectType == 8) 
            && (decConfig [suitableTracks-1].audioObjectType != 8) ) {
        /* read at least MAX_AU AU for celp if celp + aac scalable */
        for (t = 0 ; t < suitableTracks ; t++){
          if  (( frameData.layer [t].NoAUInBuffer <= MAX_AU ) &&
               (  EOFmask[0] & (1<<t) )) {
            needNewAU=1;
            idx=t;
          } 
        }                
      } else {
        unsigned long s_idx1, s_idx2;
        for (t = 0 ; t < suitableTracks ; t++){
          s_idx1 = t / SUBTRACKSIZE; 
          s_idx2 = t % SUBTRACKSIZE;
          if  (( frameData.layer [t].NoAUInBuffer == 0 ) &&
               (  EOFmask[s_idx1] & (1<<s_idx2) )) {
            needNewAU=1;
            idx=t;
          }
        }
      }            
    }
    
    /*
      send AU to decoder
    */
    if ((mainDebugLevel == 1) && (framesDone % 20 == 0)) {
      fprintf(stderr,"\rFrame [%ld] ",framesDone);
      fflush(stderr);
    }
    if (mainDebugLevel >= 2 && mainDebugLevel <= 3) {
      printf("\rFrame [%ld] ",framesDone);
      fflush(stdout);
    }
    if (mainDebugLevel > 3)
      printf("Frame [%ld]\n",framesDone);
    /* HP20001208
    if (mainDebugLevel >= 2) {
      fprintf(stdout,"Frame [%ld]  \r",framesDone);
    } else if (mainDebugLevel >= 1) {
      if (framesDone % 20 == 0)
        fprintf(stdout,"Frame [%ld]  \r",framesDone);
    } else {
      if (framesDone % 100 == 0)
        fprintf(stdout,"Frame [%ld]  \r",framesDone);
    }
    fflush(stdout);
    */
    /* HP20010815:   you can't write to stdout here!!! */
    /* This would corrupt .au files sent to stdout by "mp4audec -o -". */
    /* For basic progress info, use "mp4audec -d 1". */
    /*
    if ((mainDebugLevel == 0) && (framesDone % 100 == 0)) 
        fprintf(stdout,"Frame [%ld]  \r",framesDone);
    */

#ifdef MOUSECHANGE
    GetMouse(&speedFact,&pitchFact);
    if (MOUSEdisp) {
      if (mainDebugLevel == 1)
	fprintf(stderr,"speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
      if (mainDebugLevel >= 2)
	printf("speed=%4.2f pitch=%4.2f ",speedFact,pitchFact);
    }
#endif

    switch ( decConfig [frameData.scalOutSelect].audioObjectType )
      {
      case AAC_LC:
      case AAC_MAIN:                 /* 110300 spz */
      case AAC_LTP:
        {
          int i, i_ch ;
          
          DecTfFrameNew(numChannelOut,
                        &frameData,
                        &tfData,
                        &lpcData,
                        &nttData,
                        hFault ,
                        AAC_QC      
                        );
         
          /* convert time data from double to float */
          
          for (i_ch=0; i_ch<numChannelOut; i_ch++){
            for( i=0; i<(int)tfData.block_size_samples; i++ ) {
              tfData.sampleBuf[i_ch][i] = (float)tfData.time_sample_vector[i_ch][i];
            }
          }
        }
        AudioWriteData(audioFile,tfData.sampleBuf,tfData.block_size_samples);
        break;

      case AAC_SCAL:
      case TWIN_VQ:
      case ER_TWIN_VQ:
      case ER_BSAC:
        {
          int i, i_ch ;

          DecTfFrameNew(numChannelOut,
                        &frameData,
                        &tfData,
                        &lpcData,
                        &nttData,
                        hFault ,
                        AAC_SCALABLE);
          
          /* convert time data from double to float */
          
          for (i_ch=0; i_ch<numChannelOut; i_ch++){
            for( i=0; i<(int)tfData.block_size_samples; i++ ) {
              tfData.sampleBuf[i_ch][i] = (float)tfData.time_sample_vector[i_ch][i];
            }
          }
        }
        AudioWriteData(audioFile,tfData.sampleBuf,tfData.block_size_samples);
        break;

      case CELP:
      case ER_CELP:
        {
          if (frameData.layer[0].NoAUInBuffer>0) {
            DecLpcFrameNew(&frameData,
                           &lpcData,
                           &numBits
                           );
          } else {
            CommonExit(-1,"\nNO more Access Units for Celp\n");
          }
          AudioWriteData(audioFile,lpcData.sampleBuf,lpcData.frameNumSample);
        }
        break;

      case HVXC:
#ifdef VERSION2 
      case ER_HVXC:
#endif
        if (frameData.layer[0].NoAUInBuffer>0) {
	DecHvxcFrameNew(&frameData,
			hvxcData,
			&numBits,
			speedFact,
			pitchFact
#ifdef VERSION2         
			,epFlag,
			hFault->hEscInstanceData
#endif
			);
        } else {
          CommonExit(-1,"\nNO more Access Units for Hvxc\n");
        }
	AudioWriteData(audioFile,hvxcData->sampleBuf,hvxcData->frameNumSample);  
	break;
	
#ifdef VERSION2 
      case ER_HILN:
      case ER_PARA:
        if (frameData.layer[0].NoAUInBuffer>0) {
	DecParFrameNew(&frameData,
		       paraData,
		       &numBits,
		       speedFact,
		       pitchFact,
		       epFlag,
		       hFault->hEscInstanceData
		       );
        } else {
          CommonExit(-1,"\nNO more Access Units for Para\n");
        }
	AudioWriteData(audioFile,paraData->sampleBuf,paraData->frameNumSample);  
	break;
#endif
	
  default :
    CommonExit(1,"unsupported audio object type %ld " ,decConfig[frameData.scalOutSelect].audioObjectType);
    break;            
    }
  
#ifdef DEBUGPLOT    
    framePlot++;
    plotDisplay(0);
#endif

    needNewAU=0;        
    if (  (decConfig [0].audioObjectType == 8) 
          && (decConfig [suitableTracks-1].audioObjectType != 8) ) {
      /* read at least MAX_AU AU for celp if celp + aac scalable */
      for (t = 0 ; t < suitableTracks ; t++){
        if (EOFmask[0] & (1<<t)) {
          if ( frameData.layer[t].NoAUInBuffer <= MAX_AU ) {
            needNewAU=1;
            idx=t;
          }
        } else {
          /* new condition for program halt (lzm) */
          if ( frameData.layer[t].NoAUInBuffer <= 1 ) {
            buffer_empty=1;
          }
        } 
      }                
      
    } else {
      unsigned long s_idx1, s_idx2;

      for (t = 0 ; t < suitableTracks ; t++){
        s_idx1 = t / SUBTRACKSIZE; 
        s_idx2 = t % SUBTRACKSIZE;
        if  (( frameData.layer [t].NoAUInBuffer == 0 ) &&
             (  EOFmask[s_idx1] & (1<<s_idx2) )) {
          needNewAU=1;
          idx=t;
        }
      }
    }            
    
  }	
  if (mainDebugLevel >= 2)
    fprintf(stdout,"\n");
  
 bail:
  
  if (audioFile) AudioClose(audioFile);
  
  if ( decoderConfigH) ISODisposeHandle (decoderConfigH) ;
  if ( theMovie ) ISODisposeMovie (theMovie) ;
  if ( sampleH ) ISODisposeHandle (sampleH) ;
  
  return err;
}

static int MP4Audio_LookForSuitableTracks (ISOMovie theMovie, unsigned long trackNumber [MAXTRACKS])
{
  u32 trackCount;
  unsigned long idx ;
  unsigned long suitableTracks = 0 ;

  ISOTrack baseTrack = NULL ;

  ISOErr err = ISOGetMovieTrackCount (theMovie, &trackCount) ;
  
  if (!err)
  {
    /* look for base layer */

    for (idx = 1 ; idx <= trackCount ; idx++)
    {
      u32  enabled = 0;

      err = ISOGetMovieIndTrack (theMovie, idx, &baseTrack) ; if (err) goto bail;
      err = ISOGetTrackEnabled (baseTrack, &enabled ); if (err) goto bail;

      if (enabled)
      {
        ISOMedia theMedia;
      	u32    handlerType;
        u32    outRefIndex = 0 ;

        err = ISOGetTrackMedia(baseTrack, &theMedia ); if (err) goto bail;
        err = ISOGetMediaHandlerDescription(theMedia, &handlerType, NULL ); if (err) goto bail;
        err = ISOGetTrackReferenceCount (baseTrack, MP4StreamDependencyReferenceType, &outRefIndex) ;

        if ((handlerType == ISOAudioHandlerType) && (outRefIndex == 0))
        {
          trackNumber [suitableTracks++] = idx ;
          break ;
        }
      }
    }

    if (suitableTracks)
    {
      /* then look for extension layers */

      do
      {
        for (idx = 1 ; idx <= trackCount ; idx++)
        {
          ISOTrack theTrack;
          u32    enabled = 0;

          err = ISOGetMovieIndTrack (theMovie, idx, &theTrack) ; if (err) goto bail;
          err = ISOGetTrackEnabled (theTrack, &enabled ); if (err) goto bail;

          if (enabled)
          {
            ISOMedia theMedia;
      	    u32    handlerType;

	          err = ISOGetTrackMedia(theTrack, &theMedia ); if (err) goto bail;
	          err = ISOGetMediaHandlerDescription(theMedia, &handlerType, NULL ); if (err) goto bail;

            if (handlerType == ISOAudioHandlerType)
            {
              ISOTrack refTrack ;
              u32 outRefIndex = 0, i ;

              err = ISOGetTrackReferenceCount (theTrack, MP4StreamDependencyReferenceType, &outRefIndex) ;

              for (i = 1 ; i <= outRefIndex ; i++)
              {
                err = ISOGetTrackReference (theTrack, MP4StreamDependencyReferenceType, i, &refTrack) ;
                if (refTrack == baseTrack)
                {
                  trackNumber [suitableTracks++] = idx ;
#if 1	/* AI 2000/01/21 */
                  baseTrack = theTrack ;
#endif
                  break ;
                } 

              }
            }
          }
        }
      }
      while (idx < trackCount) ;
    }
  }

bail :

  return suitableTracks ;
}

static int MP4Audio_ReadInitialOD (ISOMovie theMovie, int ignoreProfiles)
{
  ISOHandle initialODH = NULL ;
  ISOErr err ;

  err = ISONewHandle( 0, &initialODH ); 
  err = MP4GetMovieInitialObjectDescriptor (theMovie, initialODH );

  if (!err)
  {
    unsigned long tag, tmp ;
    unsigned long sizeOfClass ;

    unsigned long id ;
    unsigned long urlFlag ;
    unsigned long inlineFlag ;

    unsigned long odProfile = 0 ;
    unsigned long sceneProfile = 0 ;
    unsigned long audioProfile = 0 ;
    unsigned long visualProfile = 0 ;
    unsigned long graphicsProfile = 0 ;

    HANDLE_BSBITBUFFER bb = MP4Audio_BbFromHandle (initialODH) ;
    HANDLE_BSBITSTREAM bs = BsOpenBufferRead (bb);

    BsGetBit (bs, &tag, 8) ;
    if (tag != initialObjectDescrTag)
    {
      fprintf (stderr, "Failure: IOD class tag == %ld \n", tag);
      err = ISOBadDataErr ; goto bail ;
    }

    for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
         sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
         BsGetBit (bs, &tmp, 8) ;

    BsGetBit (bs, &id, 10) ;
    BsGetBit (bs, &urlFlag, 1) ;
    BsGetBit (bs, &inlineFlag, 1) ;
    BsGetBit (bs, &tmp, 4) ;

    if (!urlFlag)
    {
      BsGetBit (bs, &odProfile, 8) ;
      BsGetBit (bs, &sceneProfile, 8) ;
      BsGetBit (bs, &audioProfile, 8) ;
      BsGetBit (bs, &visualProfile, 8) ;
      BsGetBit (bs, &graphicsProfile, 8) ;
    }

    if (!ignoreProfiles)
    {
      if (odProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle OD profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (sceneProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle BIFS profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (visualProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle VIDEO profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (graphicsProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle GRAPHICS profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }
    }

bail :

    BsCloseRemove (bs, 0) ;
    free(bb);
  }

  ISODisposeHandle (initialODH) ;

  return err ;
}

static int MP4Audio_ReadDecoderConfig (ISOHandle decConfigH, 
                                       DecoderConfigDescr *decConfig, 
#ifndef CORRIGENDUM1
                                       
                                       DecoderConfigDescr *decConfig0, 
                                       unsigned long idx,
#endif
                                       HANDLE_DECODER_GENERAL hFault)
{
  ISOErr err = ISONoErr ;

  unsigned long tag, tmp ;
  unsigned long sizeOfClass ;

  HANDLE_BSBITBUFFER bb = MP4Audio_BbFromHandle (decConfigH) ;
  HANDLE_BSBITSTREAM bs = BsOpenBufferRead (bb);

  BsGetBit (bs, &tag, 8) ;
  if (tag != decoderConfigDescrTag)
  {
    fprintf (stderr, "Failure: DecoderConfig class tag == %ld\n", tag);
    err = ISOBadDataErr ; goto bail ;
  }

  for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
       sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
       BsGetBit (bs, &tmp, 8) ;

  BsGetBit (bs, &decConfig->objectTypeIndication, 8) ;
  if (decConfig->objectTypeIndication != 0x40)
  {
    fprintf (stderr, "Failure: objectTypeIndication not Audio\n");
    err = ISOBadDataErr ; goto bail ;
  }

  BsGetBit (bs, &decConfig->streamType, 6) ;
  if (decConfig->streamType != 0x5)
  {
    fprintf (stderr, "Failure: streamType not Audio\n");
    err = ISOBadDataErr ; goto bail ;
  }

  BsGetBit (bs, &decConfig->upStream, 1) ;
  BsGetBit (bs, &tmp, 1) ;
  BsGetBit (bs, &decConfig->bufferSizeDB, 24) ;
  BsGetBit (bs, &decConfig->maxBitrate, 32) ;
  BsGetBit (bs, &decConfig->avgBitrate, 32) ;

  /* DecoderSpecificInfo */

  BsGetBit (bs, &tag, 8) ;
  if (tag != decSpecificInfoTag)
  {
    fprintf (stderr, "Failure: DecoderSpecificInfo class tag == %ld\n", tag);
    err = ISOBadDataErr ; goto bail ;
  }

  for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
       sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
       BsGetBit (bs, &tmp, 8) ;

  /* HP 20000117 */
  /*BsGetBit (bs, &(long unsigned int)decConfig->audioObjectType, 5) ;*/
  BsGetBit (bs, &tmp, 5) ;
  decConfig->audioObjectType = (AUDIO_OBJECT_TYPE_ID) tmp;
  BsGetBit (bs, &decConfig->samplingFrequencyIndex, 4) ;
  if (decConfig->samplingFrequencyIndex == 0x0F)
  {
    /* BsGetBit (bs, &decConfig->samplingFrequency, 4) ; */
    BsGetBit (bs, &decConfig->samplingFrequency, 24) ;
  }
  else
  {
    static const unsigned long samplingRate [] = 
    {
      96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
    } ;

    decConfig->samplingFrequency = samplingRate [decConfig->samplingFrequencyIndex] ;
  }

  BsGetBit (bs, &decConfig->channelConfiguration, 4) ;

  switch (decConfig->audioObjectType)
  {
  case AAC_MAIN:
  case AAC_LC:
  case AAC_LTP:
  case AAC_SCAL:
  case TWIN_VQ:
#ifdef VERSION2
  case ER_TWIN_VQ:
  case ER_BSAC:
#endif
      BsGetBit (bs, &decConfig->frameLengthFlag, 1) ;
      BsGetBit (bs, &decConfig->dependsOnCoreCoder, 1) ;
      if (decConfig->dependsOnCoreCoder)
      {
        BsGetBit (bs, &decConfig->coreCoderDelay, 14) ;
      }
      else
        decConfig->coreCoderDelay = 0 ;
      BsGetBit (bs, &decConfig->extensionFlag, 1) ;   
      if (!decConfig->channelConfiguration){                                 /* 110300 spz */
          get_prog_config_from_bs ( &decConfig->progConfig,
                                    (decConfig->frameLengthFlag ? 960:1024),
#ifdef VERSION2
                                    hFault->hResilience, 
                                    hFault->hEscInstanceData,
#endif /*VERSION2*/
                                    bs );
      decConfig->channelConfiguration = get_channel_config_from_progConfig ( &decConfig->progConfig );
      }

#ifdef CORRIGENDUM1
      if (decConfig->audioObjectType == 6 || decConfig->audioObjectType == 20) {
        BsGetBit (bs, &decConfig->layerNr, 3) ;   
      }
#endif

#ifdef VERSION2
      if (decConfig->extensionFlag != 0) {
	if (decConfig->audioObjectType == ER_BSAC) {
	  BsGetBit (bs, &decConfig->numOfSubFrame, 5) ;
	  BsGetBit (bs, &decConfig->layer_length, 11) ;
	}
	BsGetBit (bs, &decConfig->extension3Flag, 1) ;
      }
#endif

      /* added by NTT 20010215 */
#ifndef EP_CONFIG_MISSING
      if ( decConfig->audioObjectType == 21 ){
        BsGetBit (bs, &decConfig->epConfig, 2) ;
      }
#endif

      break ;

    case CELP:
    case ER_CELP:
#ifdef CORRIGENDUM1
      BsGetBit (bs, &decConfig->isBaseLayer, 1) ;	/* 14496-3 COR1 */
      if ( decConfig->isBaseLayer == 1 ) {
#else
      if ( idx == 0 ) {
#endif
	BsGetBit (bs, &decConfig->excitationMode, 1) ;
	BsGetBit (bs, &decConfig->sampleRateMode, 1) ;
	BsGetBit (bs, &decConfig->fineRateControl, 1) ;
	if (decConfig->audioObjectType == 24) {
	  BsGetBit (bs, &decConfig->silenceCompressionSW, 1) ;
	}
	if (decConfig->excitationMode == RegularPulseExc) {
	  BsGetBit (bs, &decConfig->RPE_Configuration, 3) ;
	} 
	if (decConfig->excitationMode == MultiPulseExc){
	  BsGetBit (bs, &decConfig->MPE_Configuration, 5) ;
	  BsGetBit (bs, &decConfig->numEnhLayers, 2) ;
	  BsGetBit (bs, &decConfig->bandwidthScalabilityMode, 1) ;
	}
      } else {
#ifdef CORRIGENDUM1
	BsGetBit (bs, &decConfig->isBWSLayer, 1) ;	/* 14496-3 COR1 */
	if ( decConfig->isBWSLayer == 1 ) {
	  BsGetBit (bs, &decConfig->BWS_Configuration, 2) ;
	} else {
	  BsGetBit (bs, &decConfig->CELP_BRS_id, 2) ;	/* 14496-3 COR1 */
	}
#else
	if ( decConfig0->bandwidthScalabilityMode == ON &&
	     idx == (decConfig0->numEnhLayers + 1)) {
	  BsGetBit (bs, &decConfig->BWS_Configuration, 2) ;
	}
#endif
      }
#ifndef EP_CONFIG_MISSING
      if ( decConfig->audioObjectType == 24 ) {
	BsGetBit (bs, &decConfig->epConfig, 2) ;
      }
#endif
      break ;

  case HVXC:
#ifdef CORRIGENDUM1
      BsGetBit (bs, &decConfig->isBaseLayer, 1) ;	/* 14496-3 COR1(AI 2000/10) */
      if (decConfig->isBaseLayer) {
	BsGetBit (bs, &decConfig->HVXCvarMode, 1) ;
	BsGetBit (bs, &decConfig->HVXCrateMode, 2) ;
	BsGetBit (bs, &decConfig->extensionFlag, 1) ;
      }
#else
      BsGetBit (bs, &decConfig->HVXCvarMode, 1) ;
      BsGetBit (bs, &decConfig->HVXCrateMode, 2) ;
      BsGetBit (bs, &decConfig->extensionFlag, 1) ;
#endif
      break;

#ifdef VERSION2      
  case ER_HVXC:
#ifdef CORRIGENDUM1
      BsGetBit (bs, &decConfig->isBaseLayer, 1) ;	/* 14496-3 COR1(AI 2000/10) */
      if (decConfig->isBaseLayer) {
	BsGetBit (bs, &decConfig->HVXCvarMode, 1) ;
	BsGetBit (bs, &decConfig->HVXCrateMode, 2) ;
	BsGetBit (bs, &decConfig->extensionFlag, 1) ;
	if (decConfig->extensionFlag) {
	  BsGetBit (bs, &decConfig->vrScalFlag, 1) ;
	}
      }
#else
      BsGetBit (bs, &decConfig->HVXCvarMode, 1) ;
      BsGetBit (bs, &decConfig->HVXCrateMode, 2) ;
      BsGetBit (bs, &decConfig->extensionFlag, 1) ;
      if (decConfig->extensionFlag) {
	BsGetBit (bs, &decConfig->vrScalFlag, 1) ;
      }
#endif
#ifndef EP_CONFIG_MISSING
      BsGetBit (bs, &decConfig->epConfig, 2) ;
#endif
      break;
#endif

#ifdef VERSION2      
  case ER_HILN:
  case ER_PARA:
#ifdef CORRIGENDUM1
    BsGetBit (bs, &decConfig->isBaseLayer, 1) ;	/* 14496-3 COR1 */
    if ( decConfig->isBaseLayer == 1 )
#else
    if ( idx == 0 )
#endif
    {
      BsGetBit (bs,  &decConfig->PARAmode, 2);
      if (decConfig->PARAmode != 1) {
	BsGetBit (bs, &decConfig->HVXCvarMode, 1) ;
	BsGetBit (bs, &decConfig->HVXCrateMode, 2) ;
	BsGetBit (bs, &decConfig->extensionFlag, 1) ;
	if (decConfig->extensionFlag) {
	  BsGetBit (bs, &decConfig->vrScalFlag, 1) ;
	}
      }
      if (decConfig->PARAmode != 0) {
	BsGetBit (bs, &decConfig->HILNquantMode, 1);
	BsGetBit (bs, &decConfig->HILNmaxNumLine, 8);
	BsGetBit (bs, &decConfig->HILNsampleRateCode, 4);
	BsGetBit (bs, &decConfig->HILNframeLength, 12);
	BsGetBit (bs, &decConfig->HILNcontMode, 2);
      }
      BsGetBit (bs, &decConfig->PARAextensionFlag, 1);
    }
    else {
      BsGetBit (bs, &decConfig->HILNenhaLayer, 1);
      if (decConfig->HILNenhaLayer == 1) {
	BsGetBit (bs, &decConfig->HILNenhaQuantMode, 2);
      }
    }
#ifndef EP_CONFIG_MISSING
    BsGetBit (bs, &decConfig->epConfig, 2);
#endif

    break;
#endif

    default :

      fprintf (stderr, "Failure: audioObjectType == %d\n", decConfig->audioObjectType);
      err = ISOBadDataErr ; goto bail ;
  }

bail :

  BsCloseRemove (bs, 0) ;
  free(bb);

  return err ;
}

static HANDLE_BSBITBUFFER MP4Audio_BbFromHandle (ISOHandle theHandle)
{
  u32    handleSize ;
  ISOErr err = ISOGetHandleSize (theHandle, &handleSize) ;
  if (!err)
  {
    HANDLE_BSBITBUFFER bb;

    bb = BsAllocPlainDirtyBuffer((unsigned char *) (*theHandle), handleSize * 8);
    return bb ;
  }

  return NULL ;
}

static int MP4Audio_SetupDecoders (FRAME_DATA *fD, 
                                   DecoderConfigDescr decConfig [MAXTRACKS],
                                   int   streamCount,     /* in: number of ESs */
                                   char* decPara,         /* in: decoder parameter string */
                                   HANDLE_DECODER_GENERAL hFault,
                                   int   mainDebugLevel,
                                   char* aacDebugString)
                            
  {
  int layer ;
  int tfInit=0;
  int hvxcInit=0;
  int celpInit=0;
#ifdef VERSION2
  int paraInit=0;
#endif
  
#ifdef VERSION2
  int 		     epFlag=0;
  int                epDebugLevel=0;
  char*               infoFileName=NULL;
  char*               audioFileName2=NULL;
#endif

  memset (fD, sizeof (FRAME_DATA), 0) ;

  initObjDescr(&objDescr,mainDebugLevel);
  fD->od = &objDescr;

  fD->od->streamCount.value = streamCount ;

  for ( layer = 0; layer < streamCount; layer++ )   {
    /* goes through all ESs, which outnumbers layers for epConfig==1 */
    AUDIO_SPECIFIC_CONFIG *audSpC ;
    
    initESDescr(&(fD->od->ESDescriptor[layer]));
    
    audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;

    audSpC->audioDecoderType.value = decConfig [layer].audioObjectType ;
    audSpC->channelConfiguration.value = decConfig [layer].channelConfiguration ;
    audSpC->samplingFreqencyIndex.value = decConfig [layer].samplingFrequencyIndex ;
    audSpC->samplingFrequency.value = decConfig [layer].samplingFrequency ;
    audSpC->epConfig.value = decConfig[layer].epConfig;

    fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = decConfig [layer].avgBitrate ;
   
    fD->layer [layer].bitBuf = BsAllocBuffer(MAX_BITBUF);
    fD->layer [layer].sampleRate = decConfig [layer].samplingFrequency ;
    fD->layer [layer].bitRate = decConfig [layer].avgBitrate ;

    if (mainDebugLevel >= 2) {
      fprintf(stdout,"ES (LAYER) :%d  \n", layer);
      fprintf(stdout,"  audioObjType   :%d  \n", decConfig [layer].audioObjectType );
      fprintf(stdout,"  chanConfig     :%ld  \n", decConfig [layer].channelConfiguration );
      fprintf(stdout,"  samplFreqIdx   :%ld  \n", decConfig [layer].samplingFrequencyIndex);
      fprintf(stdout,"  samplFreq      :%ld  \n", decConfig [layer].samplingFrequency  );
      fprintf(stdout,"  avgBitrate     :%ld  \n", decConfig [layer].avgBitrate );
#ifndef EP_CONFIG_MISSING
      switch (decConfig[layer].audioObjectType) {
      case ER_AAC_LC:
      case ER_AAC_LTP:
      case ER_AAC_SCAL:
      case ER_TWIN_VQ:
      case ER_BSAC:
      case ER_AAC_LD:
      case ER_CELP:
      case ER_HVXC:
      case ER_HILN:
      case ER_PARA:
	fprintf(stdout,"  epConfig       :%ld  \n", decConfig [layer].epConfig );
	break;
      default:
        /* not an er-aot */
        break;

      }
#endif
    }
    switch ( decConfig [layer].audioObjectType )
      {
      case AAC_SCAL:
      case TWIN_VQ:
      case ER_TWIN_VQ:
      case AAC_LC:
      case AAC_MAIN:              /* 110200 spz */
      case ER_BSAC:
      case AAC_LTP:
        audSpC->specConf.TFSpecificConfig.frameLength.value = decConfig[layer].frameLengthFlag ;
        audSpC->specConf.TFSpecificConfig.dependsOnCoreCoder.value=decConfig[layer].dependsOnCoreCoder ;
        audSpC->specConf.TFSpecificConfig.coreCoderDelay.value=decConfig[layer].coreCoderDelay ;
        audSpC->specConf.TFSpecificConfig.extension.value=decConfig[layer].extensionFlag ;  

#ifdef VERSION2
        if (decConfig[layer].extensionFlag != 0) {
          if (decConfig[layer].audioObjectType == ER_BSAC) {
            audSpC->specConf.TFSpecificConfig.numOfSubFrame.value=decConfig[layer].numOfSubFrame;  
            audSpC->specConf.TFSpecificConfig.layer_length.value=decConfig[layer].layer_length;  
          }
          audSpC->specConf.TFSpecificConfig.extension3.value=decConfig[layer].extension3Flag;  
        }
#endif
        if (mainDebugLevel >= 2) {
          fprintf(stdout,"  frameLengthFlag   :%ld  \n", decConfig[layer].frameLengthFlag );          
          fprintf(stdout,"  dependsOnCoreCoder:%ld  \n", decConfig[layer].dependsOnCoreCoder );          
          fprintf(stdout,"  coreCoderDelay:%ld  \n", decConfig[layer].coreCoderDelay );          
          fprintf(stdout,"  extensionFlag:%ld  \n", decConfig[layer].extensionFlag );          
#ifdef VERSION2
          if (decConfig[layer].extensionFlag != 0) {
            if (decConfig[layer].audioObjectType == ER_BSAC) {
              fprintf(stdout,"  numOfSubFrame:%ld  \n", decConfig[layer].numOfSubFrame );          
              fprintf(stdout,"  layer_length:%ld  \n", decConfig[layer].layer_length );          
            }
            fprintf(stdout,"  extension3Flag:%ld  \n", decConfig[layer].extension3Flag );          
          }
#endif
        }
        break;

      case CELP:
      case ER_CELP:
#ifdef CORRIGENDUM1
	audSpC->specConf.celpSpecificConfig.isBaseLayer.value = decConfig[layer].isBaseLayer;
	if ( decConfig[layer].isBaseLayer == 1 ) {
#else
	if (layer==0) {
#endif
	  audSpC->specConf.celpSpecificConfig.excitationMode.value = decConfig[layer].excitationMode;
	  audSpC->specConf.celpSpecificConfig.sampleRateMode.value = decConfig[layer].sampleRateMode;
	  audSpC->specConf.celpSpecificConfig.fineRateControl.value = decConfig[layer].fineRateControl;        
#ifdef VERSION2
	  if (decConfig [layer].audioObjectType==24) {
	    audSpC->specConf.celpSpecificConfig.silenceCompressionSW.value = decConfig[layer].silenceCompressionSW;
	  }
#endif
        /* RPE mode */
	  audSpC->specConf.celpSpecificConfig.RPE_Configuration.value = decConfig[layer].RPE_Configuration;        
        /* MPE mode */
	  audSpC->specConf.celpSpecificConfig.MPE_Configuration.value = decConfig[layer].MPE_Configuration ;
	  audSpC->specConf.celpSpecificConfig.numEnhLayers.value = decConfig[layer].numEnhLayers;
	  audSpC->specConf.celpSpecificConfig.bandwidthScalabilityMode.value = decConfig[layer].bandwidthScalabilityMode;        
	  if (mainDebugLevel >= 2) {
	    fprintf(stdout,"  excitationMode:%ld  \n", decConfig[layer].excitationMode );                    
	    fprintf(stdout,"  sampleRateMode:%ld  \n", decConfig[layer].sampleRateMode  );                    
	    fprintf(stdout,"  fineRateControl:%ld  \n", decConfig[layer].fineRateControl  );                    
	    if (decConfig[layer].excitationMode == RegularPulseExc)
	      fprintf(stdout,"  RPE_Configuration:%ld  \n", decConfig[layer].RPE_Configuration  );                    
	    else {
	      fprintf(stdout,"  MPE_Configuration:%ld  \n", decConfig[layer].MPE_Configuration  );                    
	      fprintf(stdout,"  numEnhLayers:%ld  \n", decConfig[layer].numEnhLayers  );                    
	      fprintf(stdout,"  bandwidthScalabilityMode:%ld  \n", decConfig[layer].bandwidthScalabilityMode  );
	    }
	  }
	} else {
#ifdef CORRIGENDUM1
	  audSpC->specConf.celpSpecificConfig.isBWSLayer.value = decConfig[layer].isBWSLayer;
	  if ( decConfig[layer].isBWSLayer == 1 ) {
	    audSpC->specConf.celpSpecificConfig.BWS_Configuration.value = decConfig[layer].BWS_Configuration;
	    if (mainDebugLevel >= 2) {
	      fprintf(stdout,"  BWS_Configuration:%ld  \n", decConfig[layer].BWS_Configuration );
	    }
	  } else {
	    audSpC->specConf.celpSpecificConfig.CELP_BRS_id.value = decConfig[layer].CELP_BRS_id;
	    if (mainDebugLevel >= 2) {
	      fprintf(stdout,"  BRS-Layer Index:%ld  \n", decConfig[layer].CELP_BRS_id);
	    }
	  }
#else
	  if ( decConfig[0].bandwidthScalabilityMode == ON &&
	       layer == (int)(decConfig[0].numEnhLayers + 1) ) {
	    audSpC->specConf.celpEnhSpecificConfig.BWS_Configuration.value = decConfig[layer].BWS_Configuration;
	    if (mainDebugLevel >= 2) {
	      fprintf(stdout,"  BWS_Configuration:%ld  \n", decConfig[layer].BWS_Configuration );
	    }
	  } else {
	    if (mainDebugLevel >= 2) {
	      fprintf(stdout,"  CELP BRS-Layer: No Config.  \n");
	    }
	  }
#endif
	}
        break;

      case HVXC:
#ifdef CORRIGENDUM1
        audSpC->specConf.hvxcSpecificConfig.isBaseLayer.value = decConfig[layer].isBaseLayer;
	/* 14496-3 COR1(AI 2000/10) */
#endif
        audSpC->specConf.hvxcSpecificConfig.HVXCvarMode.value = decConfig[layer].HVXCvarMode;
        audSpC->specConf.hvxcSpecificConfig.HVXCrateMode.value = decConfig[layer].HVXCrateMode;
        audSpC->specConf.hvxcSpecificConfig.extensionFlag.value = decConfig[layer].extensionFlag;
        if (mainDebugLevel >= 2) {
          fprintf(stdout,"  HVXCvarMode    :%ld  \n", decConfig[layer].HVXCvarMode );
          fprintf(stdout,"  HVXCrateMode   :%ld  \n", decConfig[layer].HVXCrateMode );
          fprintf(stdout,"  extensionFlag  :%ld  \n", decConfig[layer].extensionFlag );
	}
	break;

#ifdef VERSION2
      case ER_HVXC:
#ifdef CORRIGENDUM1
        audSpC->specConf.hvxcSpecificConfig.isBaseLayer.value = decConfig[layer].isBaseLayer;
	/* 14496-3 COR1(AI 2000/10) */
#endif
        audSpC->specConf.hvxcSpecificConfig.HVXCvarMode.value = decConfig[layer].HVXCvarMode;
        audSpC->specConf.hvxcSpecificConfig.HVXCrateMode.value = decConfig[layer].HVXCrateMode;
        audSpC->specConf.hvxcSpecificConfig.extensionFlag.value = decConfig[layer].extensionFlag;
	if (decConfig[layer].extensionFlag) {
#if 0
	  audSpC->specConf.hvxcSpecificConfig.packMode.value = decConfig[layer].packMode;
#endif
	  audSpC->specConf.hvxcSpecificConfig.vrScalFlag.value = decConfig[layer].vrScalFlag;
	}
        if (mainDebugLevel >= 2) {
#ifdef CORRIGENDUM1
          fprintf(stdout,"  isBaseLayer    :%ld  \n", decConfig[layer].isBaseLayer );
	  if (decConfig[layer].isBaseLayer) {	/* 14496-3 COR1(AI 2000/10) */
#endif
	    fprintf(stdout,"  HVXCvarMode    :%ld  \n", decConfig[layer].HVXCvarMode );
	    fprintf(stdout,"  HVXCrateMode   :%ld  \n", decConfig[layer].HVXCrateMode );
	    fprintf(stdout,"  extensionFlag  :%ld  \n", decConfig[layer].extensionFlag );
	    if (decConfig[layer].extensionFlag) {
#if 0
	      fprintf(stdout,"  packMode       :%ld  \n", decConfig[layer].packMode );
#endif
	      fprintf(stdout,"  vrScalFlag     :%ld  \n", decConfig[layer].vrScalFlag );
	    }
#ifdef CORRIGENDUM1
	  }
#endif	
	}
	break;
#endif	

#ifdef VERSION2
      case ER_HILN:
      case ER_PARA:
#ifdef CORRIGENDUM1
	audSpC->specConf.paraSpecificConfig.isBaseLayer.value = decConfig[layer].isBaseLayer;
	if ( decConfig[layer].isBaseLayer == 1 )
#else
	if (layer==0)
#endif
	{
	  audSpC->specConf.paraSpecificConfig.PARAmode.value = decConfig[layer].PARAmode;
	  if (decConfig[layer].PARAmode != 1) {
	    audSpC->specConf.paraSpecificConfig.HVXCvarMode.value = decConfig[layer].HVXCvarMode;
	    audSpC->specConf.paraSpecificConfig.HVXCrateMode.value = decConfig[layer].HVXCrateMode;
	    audSpC->specConf.paraSpecificConfig.extensionFlag.value = decConfig[layer].extensionFlag;
	    if (decConfig[layer].extensionFlag) {
	      audSpC->specConf.paraSpecificConfig.vrScalFlag.value = decConfig[layer].vrScalFlag;
	    }
	  }
	  if (decConfig[layer].PARAmode != 0) {
	    audSpC->specConf.paraSpecificConfig.HILNquantMode.value = decConfig[layer].HILNquantMode;
	    audSpC->specConf.paraSpecificConfig.HILNmaxNumLine.value = decConfig[layer].HILNmaxNumLine;
	    audSpC->specConf.paraSpecificConfig.HILNsampleRateCode.value = decConfig[layer].HILNsampleRateCode;
	    audSpC->specConf.paraSpecificConfig.HILNframeLength.value = decConfig[layer].HILNframeLength;
	    audSpC->specConf.paraSpecificConfig.HILNcontMode.value = decConfig[layer].HILNcontMode;
	  }
	  audSpC->specConf.paraSpecificConfig.PARAextensionFlag.value = decConfig[layer].PARAextensionFlag;
	  
	  if (mainDebugLevel >= 2) {
#ifdef CORRIGENDUM1
	    fprintf(stdout,"  isBaseLayer        :%ld  \n", decConfig[layer].isBaseLayer );
#endif
	    fprintf(stdout,"  PARAmode           :%ld  \n", decConfig[layer].PARAmode );
	    if (decConfig[layer].PARAmode != 1) {
	      fprintf(stdout,"  HVXCvarMode        :%ld  \n", decConfig[layer].HVXCvarMode );
	      fprintf(stdout,"  HVXCrateMode       :%ld  \n", decConfig[layer].HVXCrateMode );
	      fprintf(stdout,"  extensionFlag      :%ld  \n", decConfig[layer].extensionFlag );
	      if (decConfig[layer].extensionFlag) {
		fprintf(stdout,"  vrScalFlag         :%ld  \n", decConfig[layer].vrScalFlag );
	      }
	    }
	    if (decConfig[layer].PARAmode != 0) {
	      fprintf(stdout,"  HILNquantMode      :%ld  \n", decConfig[layer].HILNquantMode);
	      fprintf(stdout,"  HILNmaxNumLine     :%ld  \n", decConfig[layer].HILNmaxNumLine);
	      fprintf(stdout,"  HILNsampleRateCode :%ld  \n", decConfig[layer].HILNsampleRateCode);
	      fprintf(stdout,"  HILNframeLength    :%ld  \n", decConfig[layer].HILNframeLength);
	      fprintf(stdout,"  HILNcontMode       :%ld  \n", decConfig[layer].HILNcontMode);
	    }
	    fprintf(stdout,"  PARAextensionFlag  :%ld  \n", decConfig[layer].PARAextensionFlag);
	  }
	}
	else { /* layer!=0 */
	  audSpC->specConf.paraSpecificConfig.HILNenhaLayer.value = decConfig[layer].HILNenhaLayer;
	  if (decConfig[layer].HILNenhaLayer) {
	    audSpC->specConf.paraSpecificConfig.HILNenhaQuantMode.value = decConfig[layer].HILNenhaQuantMode;
	  }
	  
	  if (mainDebugLevel >= 2) {
#ifdef CORRIGENDUM1
	    fprintf(stdout,"  isBaseLayer       :%ld  \n", decConfig[layer].isBaseLayer );
#endif
	    fprintf(stdout,"  HILNenhaLayer     :%ld  \n", decConfig[layer].HILNenhaLayer );
	    if (decConfig[layer].HILNenhaLayer == 1) {
	      fprintf(stdout,"  HILNenhaQuantMode :%ld  \n", decConfig[layer].HILNenhaQuantMode );
	    }
	  }
	}
	break;
#endif	

      default :
        CommonExit(1,"unsupported audio object type %ld " ,decConfig [layer].audioObjectType);
        break;            
	}
  
  }

#ifdef VERSION2
  /* audioObjectType AAC Scalable */
  /* should be moved to into tf part */
  hFault->hResilience = CreateErrorResilience ( decPara, 0 );
#endif

  for ( layer = 0; layer < streamCount; layer++ )  {
    /* goes through all ESs, which outnumbers layers for epConfig==1 */
    switch ( decConfig [layer].audioObjectType )
      {
      case AAC_SCAL:
      case TWIN_VQ:
      case AAC_LC:  
      case AAC_MAIN:               /* spz 110200 */
      case AAC_LTP:
#ifdef VERSION2
      case ER_TWIN_VQ:
      case ER_BSAC:
#endif	
        if (tfInit==0){ /* call it only once for all GA layers */
          tfInit=1;           

          DecTfInitNew (decPara,
                        aacDebugString,
                        hFault,
                      fD,
                      &tfData,
                      &lpcData,
                      &nttData,
                      mainDebugLevel ,
                      layer
#ifdef VERSION2
                      ,epDebugLevel,
                      infoFileName,
                      audioFileName2
#endif
                      ) ; /* layer */

        }
        break;

      case CELP:
      case ER_CELP:
        if (celpInit==0){ /* call it only once for all CELP layers */
	  celpInit=1;
	  DecLpcInitNew(decPara,
			fD,
			&lpcData
			);
        }
        break;

      case HVXC:
#ifdef VERSION2
      case ER_HVXC:
#endif	
	if (hvxcInit==0){ /* call it only once for all hvxc layers */
	  hvxcInit=1;
	  hvxcData = DecHvxcInitNew(decPara,
				    fD,
				    layer
#ifdef VERSION2        
				    ,epFlag, 
				    epDebugLevel, 
				    infoFileName, 
				    &hFault->hEscInstanceData
#endif
				    );
	  delayNumSample = hvxcData->delayNumSample;
	}
	break;

#ifdef VERSION2        
      case ER_HILN:
      case ER_PARA:
	if (paraInit==0){ /* call it only once for all hiln layers */
	  paraInit=1;
	  paraData = DecParInitNew(decPara,
				   fD,
				   layer,
				   epFlag, 
				   epDebugLevel, 
				   infoFileName, 
				   &hFault->hEscInstanceData
				   );
	  delayNumSample = paraData->delayNumSample;
	}
	break;
#endif

      default :
        CommonExit(1,"unsupported audio object type %ld " ,decConfig [layer].audioObjectType);
        break;            
      }
  
  }
  
  return 0 ;
}
