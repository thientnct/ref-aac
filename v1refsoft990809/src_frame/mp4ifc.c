/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by


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

Copyright (c) 1999.

*/

#include <stdio.h>
#include <string.h>

#include "MP4Movies.h"
#include "mp4ifc.h"

#include "bitstreamHandle.h"     /* handler, defines, enums */
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

#include "lpc_common.h"
#include "dec_tf.h"

#include "bitstream.h"
#include "common_m4a.h"

#include "audio.h"      /* audio i/o module */

typedef struct
{
  unsigned long objectTypeIndication ;
  unsigned long streamType ;
  unsigned long upStream ;
  unsigned long bufferSizeDB ;
  unsigned long maxBitrate ;
  unsigned long avgBitrate ;

  /* Audio */

  unsigned long audioObjectType ;
  unsigned long samplingFrequencyIndex ;
  unsigned long samplingFrequency ;
  unsigned long channelConfiguration ;

  /* GA */

  unsigned long frameLengthFlag ;
  unsigned long dependsOnCoreCoder ;
  unsigned long coreCoderDelay ;
  unsigned long extensionFlag ;

} DecoderConfigDescr ;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

static const unsigned long initialObjectDescrTag = 0x02 ;
static const unsigned long decoderConfigDescrTag = 0x04 ;
static const unsigned long decSpecificInfoTag    = 0x05 ;

#define maxTracks 8

static int MP4Audio_ReadInitialOD (MP4Movie theMovie, int ignoreProfiles) ;
static int MP4Audio_ReadDecoderConfig (MP4Handle decConfigH, DecoderConfigDescr *decConfig) ;
static int MP4Audio_LookForSuitableTracks (MP4Movie theMovie, unsigned long trackNumber [maxTracks]) ;

int MP4Audio_SetupDecoders (FRAME_DATA *fD, DecoderConfigDescr decConfig [maxTracks],
                            int maxLayer,          /* in: number of Layers */
                            char *audioFileName,   /* in: audio file name */
                            char *audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                            int   numChannelOut,   /* in: num audio channels */
                            float fSampleOut,      /* in: sampling frequency [Hz] */
                            char *decPara) ;       /* in: decoder parameter string */
/*
static int MP4Audio_DecodeFrame (FRAME_DATA *fD, int maxLayers,
                                 MP4Handle theSample, unsigned long auLength,
                                 int numChannelOut) ;
*/
static BsBitStream *MP4Audio_BsFromHandle (MP4Handle theHandle) ;

static FRAME_DATA frameData;
static OBJECT_DESCRIPTOR objDescr;
static TF_DATA tfData ;
static LPC_DATA lpcData ;
static ntt_DATA nttData ;

static FAULT_TOLERANT_DATA faultData;
static HANDLE_FAULT_TOLERANT hFault = &faultData;
static AudioFile *audioFile = NULL;

int MP4Audio_ProbeFile (char *inFile)
{
  MP4Movie theMovie = NULL ;
  MP4Err err = MP4OpenMovieFile ( &theMovie, inFile, MP4OpenMovieNormal ) ;
  if (err == MP4NoErr)
  {
    MP4DisposeMovie (theMovie) ;
    return 1 ;
  }
  else
    return 0 ;
}

int MP4Audio_DecodeFile (char *inFile,          /* in: bitstream input file name */
                         char *audioFileName,   /* in: audio file name */
                         char *audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                         int   numChannelOut,   /* in: num audio channels */
                         float fSampleOut,      /* in: sampling frequency [Hz] */
                         char *decPara)         /* in: decoder parameter string */
{
  MP4Err err;
  unsigned long framesDone ;

  unsigned long trackNumber [maxTracks] ;
  DecoderConfigDescr decConfig [maxTracks] ;
	MP4TrackReader theReader [maxTracks] ;

  unsigned long idx, suitableTracks = 0 ;

  MP4Movie theMovie = NULL;
	MP4Track theTrack;
	MP4Media theMedia;
	MP4Handle sampleH = NULL ;

  MP4Handle decoderConfigH = NULL ;

	err = MP4NoErr;
	err = MP4OpenMovieFile( &theMovie, inFile, MP4OpenMovieNormal ); if (err) goto bail;

  suitableTracks = MP4Audio_LookForSuitableTracks (theMovie, trackNumber) ;

  if ( strstr( decPara, "-out") ){
    char *outLayNr = strstr( decPara, "-out") ;
    sscanf(&outLayNr[4],"%d",&frameData.scalOutSelect);
    suitableTracks = min(suitableTracks, frameData.scalOutSelect) ;
  }

  if (!suitableTracks) return MP4BadDataErr ;

  frameData.scalOutSelect = suitableTracks ;

  /* if we got here, we found at least one audio track */

  BsInit (0,0,0);

  /* check the iod */

  if (MP4Audio_ReadInitialOD (theMovie, 0))
    return MP4BadDataErr ;

  err = MP4NewHandle( 0, &decoderConfigH ); if (err) goto bail;

  /* read the theTrack */

  for (idx = 0 ; idx < suitableTracks ; idx++)
  {
    err = MP4GetMovieIndTrack( theMovie, trackNumber [idx], &theTrack ); if (err) goto bail;
	  err = MP4GetTrackMedia( theTrack, &theMedia ); if (err) goto bail;
	  err = MP4CreateTrackReader( theTrack, &theReader [idx] ); if (err) goto bail;
	  err = MP4TrackReaderGetCurrentDecoderConfig( theReader [idx], decoderConfigH ); if (err) goto bail;

    if (MP4Audio_ReadDecoderConfig (decoderConfigH, &decConfig [idx]))
      return MP4BadDataErr ;
  }

	/*
	 instantiate proper decoder for decoderconfig
	*/

  fSampleOut = (float) decConfig [suitableTracks-1].samplingFrequency ;
  numChannelOut = decConfig [suitableTracks-1].channelConfiguration ;

  MP4Audio_SetupDecoders (&frameData, decConfig, suitableTracks,
                          audioFileName, audioFileFormat,
                          numChannelOut, fSampleOut,
                          decPara) ;

	err = MP4NewHandle( 0, &sampleH ); if (err) goto bail;	

  /* loop through the file */

  for (framesDone = 0 ;; framesDone++)
	{
    for (idx = 0 ; idx < suitableTracks ; idx++)
    {
		  u32 unitSize, cts, dts, sampleFlags;
		  
		  err = MP4TrackReaderGetNextAccessUnit( theReader [idx], sampleH, &unitSize,&sampleFlags, &cts, &dts );
		  if ( err )
		  {
			  if ( err == MP4EOF )
				  err = MP4NoErr;
        goto bail;
		  }

      memcpy (frameData.layer [idx].bitBuf->data, *sampleH, unitSize) ;

      frameData.layer [idx].bitBuf->numBit = unitSize * 8 ;
      frameData.layer [idx].NoAUInBuffer = 1 ;
    }

    /*
		  send AU to decoder
		*/

    DecTfFrameNew(numChannelOut,
                  &frameData,
                  &tfData,
                  &lpcData,
                  &nttData,
                  hFault ,
                  AAC_SCALABLE);

    {
      int i, i_ch ;

      /* convert time data from double to float */

      for (i_ch=0; i_ch<numChannelOut; i_ch++){
        for( i=0; i<(int)tfData.block_size_samples; i++ ) {
          tfData.sampleBuf[i_ch][i] = (float)tfData.time_sample_vector[i_ch][i];
        }
      }
    }

    AudioWriteData(audioFile,tfData.sampleBuf,tfData.block_size_samples);
	}	

bail:

  if (audioFile) AudioClose(audioFile);

	if ( decoderConfigH) MP4DisposeHandle (decoderConfigH) ;
  if ( theMovie ) MP4DisposeMovie (theMovie) ;
  if ( sampleH ) MP4DisposeHandle (sampleH) ;

  return err;
}

static int MP4Audio_LookForSuitableTracks (MP4Movie theMovie, unsigned long trackNumber [maxTracks])
{
  u32 trackCount;
  unsigned long idx ;
  unsigned long suitableTracks = 0 ;

  MP4Track baseTrack = NULL ;

  MP4Err err = MP4GetMovieTrackCount (theMovie, &trackCount) ;
  if (!err)
  {
    /* look for base layer */

    for (idx = 1 ; idx <= trackCount ; idx++)
    {
      u32  enabled = 0;

      err = MP4GetMovieIndTrack (theMovie, idx, &baseTrack) ; if (err) goto bail;
      err = MP4GetTrackEnabled (baseTrack, &enabled ); if (err) goto bail;

      if (enabled)
      {
        MP4Media theMedia;
      	u32    handlerType;
        u32    outRefIndex = 0 ;

	      err = MP4GetTrackMedia(baseTrack, &theMedia ); if (err) goto bail;
	      err = MP4GetMediaHandlerDescription(theMedia, &handlerType, NULL ); if (err) goto bail;
        err = MP4GetTrackReferenceCount (baseTrack, MP4StreamDependencyReferenceType, &outRefIndex) ;

        if ((handlerType == MP4AudioHandlerType) && (outRefIndex == 0))
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
          MP4Track theTrack;
          u32    enabled = 0;

          err = MP4GetMovieIndTrack (theMovie, idx, &theTrack) ; if (err) goto bail;
          err = MP4GetTrackEnabled (theTrack, &enabled ); if (err) goto bail;

          if (enabled)
          {
            MP4Media theMedia;
      	    u32    handlerType;

	          err = MP4GetTrackMedia(theTrack, &theMedia ); if (err) goto bail;
	          err = MP4GetMediaHandlerDescription(theMedia, &handlerType, NULL ); if (err) goto bail;

            if (handlerType == MP4AudioHandlerType)
            {
              MP4Track refTrack ;
              u32 outRefIndex = 0, i ;

              err = MP4GetTrackReferenceCount (theTrack, MP4StreamDependencyReferenceType, &outRefIndex) ;

              for (i = 1 ; i <= outRefIndex ; i++)
              {
                err = MP4GetTrackReference (theTrack, MP4StreamDependencyReferenceType, i, &refTrack) ;
                if (refTrack == baseTrack)
                {
                  trackNumber [suitableTracks++] = idx ;
                  baseTrack = theTrack ;
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

static int MP4Audio_ReadInitialOD (MP4Movie theMovie, int ignoreProfiles)
{
  MP4Handle initialODH = NULL ;
  MP4Err err ;

  err = MP4NewHandle( 0, &initialODH ); 
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

    BsBitStream *bs = MP4Audio_BsFromHandle (initialODH) ;

    BsGetBit (bs, &tag, 8) ;
    if (tag != initialObjectDescrTag)
    {
      fprintf (stderr, "Failure: IOD class tag == %ld \n", tag);
      err = MP4BadDataErr ; goto bail ;
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
        err = MP4BadDataErr ; goto bail ;
      }

      if (sceneProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle BIFS profile %ld \n", tag);
        err = MP4BadDataErr ; goto bail ;
      }

      if (visualProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle VIDEO profile %ld \n", tag);
        err = MP4BadDataErr ; goto bail ;
      }

      if (graphicsProfile != 0xFF)
      {
        fprintf (stderr, "Failure: Can't handle GRAPHICS profile %ld \n", tag);
        err = MP4BadDataErr ; goto bail ;
      }
    }

bail :

    BsCloseRemove (bs, 0) ;
  }

  MP4DisposeHandle (initialODH) ;

  return err ;
}

static int MP4Audio_ReadDecoderConfig (MP4Handle decConfigH, DecoderConfigDescr *decConfig)
{
  MP4Err err = MP4NoErr ;

  unsigned long tag, tmp ;
  unsigned long sizeOfClass ;

  BsBitStream *bs = MP4Audio_BsFromHandle (decConfigH) ;

  BsGetBit (bs, &tag, 8) ;
  if (tag != decoderConfigDescrTag)
  {
    fprintf (stderr, "Failure: DecoderConfig class tag == %ld\n", tag);
    err = MP4BadDataErr ; goto bail ;
  }

  for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
       sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
       BsGetBit (bs, &tmp, 8) ;

  BsGetBit (bs, &decConfig->objectTypeIndication, 8) ;
  if (decConfig->objectTypeIndication != 0x40)
  {
    fprintf (stderr, "Failure: objectTypeIndication not Audio\n");
    err = MP4BadDataErr ; goto bail ;
  }

  BsGetBit (bs, &decConfig->streamType, 6) ;
  if (decConfig->streamType != 0x5)
  {
    fprintf (stderr, "Failure: streamType not Audio\n");
    err = MP4BadDataErr ; goto bail ;
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
    err = MP4BadDataErr ; goto bail ;
  }

  for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
       sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
       BsGetBit (bs, &tmp, 8) ;

  BsGetBit (bs, &decConfig->audioObjectType, 5) ;
  BsGetBit (bs, &decConfig->samplingFrequencyIndex, 4) ;
  if (decConfig->samplingFrequencyIndex == 0x0F)
  {
    BsGetBit (bs, &decConfig->samplingFrequency, 4) ;
  }
  else
  {
    static const unsigned long samplingRate [] = 
    {
      96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
    } ;

    decConfig->samplingFrequency = samplingRate [decConfig->samplingFrequencyIndex] ;
  }

  BsGetBit (bs, &decConfig->channelConfiguration, 4) ;

  switch (decConfig->audioObjectType)
  {
    case 1 : /* AAC Main */
    case 2 : /* AAC LC */
    case 6 : /* AAC Scalable */

      BsGetBit (bs, &decConfig->frameLengthFlag, 1) ;
      BsGetBit (bs, &decConfig->dependsOnCoreCoder, 1) ;
      if (decConfig->dependsOnCoreCoder)
      {
        BsGetBit (bs, &decConfig->coreCoderDelay, 14) ;
      }
      else
        decConfig->coreCoderDelay = 0 ;

      BsGetBit (bs, &decConfig->extensionFlag, 1) ;
      break ;

    default :

      fprintf (stderr, "Failure: audioObjectType == %ld\n", decConfig->audioObjectType);
      err = MP4BadDataErr ; goto bail ;
  }

bail :

  BsCloseRemove (bs, 0) ;

  return err ;
}

static BsBitStream *MP4Audio_BsFromHandle (MP4Handle theHandle)
{
  u32    handleSize ;
  MP4Err err = MP4GetHandleSize (theHandle, &handleSize) ;
  if (!err)
  {
    BsBitBuffer *bb = (BsBitBuffer *)malloc (sizeof (BsBitBuffer)) ;
    bb->data = (unsigned char *) (*theHandle) ;
    bb->numBit = handleSize * 8 ;
    bb->size = handleSize * 8 ;

    return BsOpenBufferRead (bb) ;
  }

  return NULL ;
}

int MP4Audio_SetupDecoders (FRAME_DATA *fD, DecoderConfigDescr decConfig [maxTracks],
                            int maxLayer,          /* in: number of Layers */
                            char *audioFileName,   /* in: audio file name */
                            char *audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                            int   numChannelOut,   /* in: num audio channels */
                            float fSampleOut,      /* in: sampling frequency [Hz] */
                            char *decPara)         /* in: decoder parameter string */
{
  int layer ;
  


  memset (fD, sizeof (FRAME_DATA), 0) ;
  AudioInit(NULL,0 /* audioDebugLevel */);

  initObjDescr(&objDescr);
  fD->od = &objDescr;

  fD->od->streamCount.value = maxLayer ;

  for ( layer = 0; layer < maxLayer; layer++ )
  {
    AUDIO_SPECIFIC_CONFIG *audSpC ;
    
    initESDescr(&(fD->od->ESDescriptor[layer]));
    
    audSpC = &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig;

    audSpC->audioDecoderType.value = decConfig [layer].audioObjectType ;
    audSpC->channelConfiguration.value = decConfig [layer].channelConfiguration ;
    audSpC->samplingFreqencyIndex.value = decConfig [layer].samplingFrequencyIndex ;

    audSpC->specConf.TFSpecificConfig.frameLength.value = decConfig [layer].frameLengthFlag ;
    fD->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = decConfig [layer].avgBitrate ;
   
    fD->layer [layer].bitBuf = BsAllocBuffer(64000);
    fD->layer [layer].sampleRate = decConfig [layer].samplingFrequency ;
    fD->layer [layer].bitRate = decConfig [layer].avgBitrate ;

    if (decConfig [layer].audioObjectType == 6)
    {
      /* patch for getModeFromOD() scal_dec_frame.c */

      fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = 0 ;
    }
  }

  /* audioObjectType AAC Scalable */

  DecTfInitNew (decPara,
                NULL, /* aacDebugString */
                &hFault->hVm,
                &hFault->hHcrSpecData,
                fD,
                &tfData,
                &lpcData,
                &nttData,
                0, /* mainDebugLevel */
                0
                ) ; /* layer */

  /* open audio file */
  /* (sample format: 16 bit twos complement, uniform quantisation) */

  audioFile = AudioOpenWrite(audioFileName,audioFileFormat,
                             numChannelOut,fSampleOut);
  if (audioFile==NULL)
    CommonExit(1,"Decode: error opening audio file %s "
               "(maybe unknown format \"%s\")",
               audioFileName,audioFileFormat);

  return 0 ;
}
