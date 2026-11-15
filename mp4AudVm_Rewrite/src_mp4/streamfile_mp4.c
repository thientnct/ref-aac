/**********************************************************************
MPEG-4 Audio VM
common stream file reader/writer, MP4ff part

This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

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

Copyright (c) 2003.
Source file: streamfile_mp4.c


Required modules:
cmdline.o               parse commandline
common_m4a.o            common module
bitstream.o             bitstream module
flex_mux.o		flexmux module
streamfile_helper.o     stream file helper module

libisomediafile

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj_descr.h"           /* structs */

#include "allHandles.h"
#include "common_m4a.h"          /* common module       */
#include "bitstream.h"           /* bit stream module   */
#include "flex_mux.h"            /* parse object descriptors */
#include "cmdline.h"             /* parse commandline options */

#include "ISOMovies.h"           /* MP4-support  */

/* defines missing in older ISOMovies.h */
#ifndef ISOOpenMovieNormal
#define ISOOpenMovieNormal MP4OpenMovieNormal
#define ISOAddMediaSamplesPad MP4AddMediaSamplesPad
#endif


#include "streamfile.h"          /* public functions */
#include "streamfile_mp4.h"
#include "streamfile_helper.h"

/* ---- mp4 specific structures ---- */
#define MODULE_INFORMATION "StreamFile transport lib: MP4ff module"

struct tagStreamSpecificInfo {
  ISOMovie             moov;
  int                  profileAndLevel;
};

struct tagProgSpecificInfo {
  ISOTrack           tracks[MAXTRACK];
  ISOMedia           media[MAXTRACK];
  ISOTrackReader     reader[MAXTRACK];
  ISOHandle          sampleEntryH[MAXTRACK];
};


/* ---- helper function ---- */

static const unsigned long initialObjectDescrTag = 0x02 ;


static HANDLE_BSBITSTREAM BsFromHandle (ISOHandle theHandle, int WriteFlag)
{
  u32    handleSize ;
  ISOErr err = ISOGetHandleSize (theHandle, &handleSize) ;
  if (!err)
  {
    HANDLE_BSBITBUFFER bb;

    bb = BsAllocPlainDirtyBuffer((unsigned char *) (*theHandle), handleSize * 8);
    if (WriteFlag == 0)
      return BsOpenBufferRead (bb) ;
    else
      return BsOpenBufferWrite (bb) ;
  }

  return NULL ;
}

static int commandline(HANDLE_STREAMFILE stream, int argc, char**argv)
{
  int profileAndLevel;

  CmdLineSwitch switchList[] = {
    {"pl",NULL,"%i","254",NULL,"audio profile and level"},
    {NULL,NULL,NULL,NULL,NULL,NULL}
  };
  CmdLinePara paraList[] = {
    {NULL,NULL,NULL}
  };

  switchList[0].argument = &profileAndLevel;

  if (stream!=NULL) {
    if (CmdLineEval(argc,argv,paraList,switchList,1,NULL)) return -10;

    stream->spec->profileAndLevel = profileAndLevel;
  } else {
    CmdLineHelp(NULL,paraList,switchList,stdout);
  }

  return 0;
}


static int MP4Audio_ReadUnknownElementFromStream ( HANDLE_BSBITSTREAM bs )
{
  unsigned long tmp;
  unsigned long sizeOfClass;

  /* read tag */
  BsGetBit (bs, &tmp, 8);

  /* read size */
  for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
       sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
    BsGetBit (bs, &tmp, 8) ;

  /* skip the element */
  DebugPrintf(3, "skipping %ld bytes of unknown element\n",sizeOfClass);
  while (sizeOfClass--) {
    BsGetBit (bs, &tmp, 8);
  }

  return 0;
}


static int MP4Audio_ReadInitialOD (ISOMovie theMovie, int ignoreProfiles)
{
  ISOHandle initialODH = NULL ;
  ISOErr err ;

  err = ISONewHandle( 0, &initialODH ); 
  err = MP4GetMovieInitialObjectDescriptor (theMovie, initialODH );

  if (!err)
  {
    unsigned long tag, tmp, tmp1 ;
    unsigned long sizeOfClass ;
    unsigned long startBitPos;
    unsigned long numESDs;

    unsigned long id ;
    unsigned long urlFlag ;
    unsigned long inlineFlag ;

    unsigned long odProfile = 0 ;
    unsigned long sceneProfile = 0 ;
    unsigned long audioProfile = 0 ;
    unsigned long visualProfile = 0 ;
    unsigned long graphicsProfile = 0 ;

    ES_DESCRIPTOR *esd = NULL;

    HANDLE_BSBITSTREAM bs = BsFromHandle (initialODH, 0) ;

    BsGetBit (bs, &tag, 8) ;
    if (tag != initialObjectDescrTag)
    {
      CommonWarning("Check MP4ff failed: IOD class tag == %ld \n", tag);
      err = ISOBadDataErr ; goto bail ;
    }

    for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
         sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
         BsGetBit (bs, &tmp, 8) ;

    startBitPos = BsCurrentBit(bs);

    BsGetBit (bs, &id, 10) ;
    DebugPrintf(3, "Initial Object Descriptor (id:%ld):", id);
    if ((id == 0) || (id == 1023)) {  /* 0 is forbidden and 1023 is reserved */
      CommonWarning("Check MP4ff failed: ID %ld invalid for IOD\n",id);
    }

    BsGetBit (bs, &urlFlag, 1) ;
    BsGetBit (bs, &inlineFlag, 1) ;
    BsGetBit (bs, &tmp, 4) ;

    if ( tmp != 0xf ) {
      CommonWarning("Check MP4ff failed: IOD reserved bits set to 0x%lx!\n", tmp);
    }

    if (!urlFlag)
    {
      BsGetBit (bs, &odProfile, 8) ;
      BsGetBit (bs, &sceneProfile, 8) ;
      BsGetBit (bs, &audioProfile, 8) ;
      BsGetBit (bs, &visualProfile, 8) ;
      BsGetBit (bs, &graphicsProfile, 8) ;
      DebugPrintf(3, "   odProfile       :%ld", odProfile);
      DebugPrintf(3, "   sceneProfile    :%ld", sceneProfile);
      DebugPrintf(3, "   audioProfile    :%ld", audioProfile);
      DebugPrintf(3, "   visualProfile   :%ld", visualProfile);
      DebugPrintf(3, "   graphicsProfile :%ld", graphicsProfile);
    } else {
      BsGetBit ( bs, &tmp1, 8); /* get URL length */
      DebugPrintf(3, "   URL string      :");
      for (;tmp1>0;tmp1--) {
        BsGetBit ( bs, &tmp, 8); /* get URL string */
        DebugPrintf(3, "%c",tmp);
      }
      DebugPrintf(3, "\n");
    }

    if (!ignoreProfiles) {
      if (odProfile != 0xFF) {
        CommonWarning("Check MP4ff: Can't handle OD profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (sceneProfile != 0xFF) {
        CommonWarning("Check MP4ff: Can't handle BIFS profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (visualProfile != 0xFF) {
        CommonWarning("Check MP4ff: Can't handle VIDEO profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }

      if (graphicsProfile != 0xFF) {
        CommonWarning("Check MP4ff: Can't handle GRAPHICS profile %ld \n", tag);
        err = ISOBadDataErr ; goto bail ;
      }
    }

    numESDs = 0;
    initESDescr(&esd);

    while ( (BsCurrentBit(bs) - startBitPos) < (sizeOfClass<<3) ) {
      if ( advanceESDescr (bs, esd, 0/*read*/, 1/*system-specific stuff*/) >= 0 ) {
        numESDs++;
      } else {
        MP4Audio_ReadUnknownElementFromStream( bs );
      }
    }

    if ( numESDs == 0 ) {
      DebugPrintf (1, "IOD does not contain ES descriptors\n");
      goto bail;
    }

bail :

    BsCloseRemove (bs, 0) ;
  }

  ISODisposeHandle (initialODH) ;

  return err ;
}



/* --------------------------------------------------- */
/* ---- MP4ff (MP4)                               ---- */
/* --------------------------------------------------- */


static int MP4initProgram(HANDLE_STREAMPROG prog)
{
  if ((prog->programData->spec = (struct tagProgSpecificInfo*)malloc(sizeof(struct tagProgSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initProgram: error in malloc");
    return -1;
  }
  memset(prog->programData->spec, 0, sizeof(struct tagProgSpecificInfo));
  return 0;
}


static int MP4openRead(HANDLE_STREAMFILE stream)
{
  ISOErr err = ISONoErr;
  u32 trackCount;

  err = ISOOpenMovieFile(&stream->spec->moov, stream->fileName, ISOOpenMovieNormal);
  if (err) {
    DebugPrintf(3,"StreamFile:openRead(MP4): file is not MP4ff");
    return -1;
  }
  if (MP4Audio_ReadInitialOD (stream->spec->moov, 0)) {
    MP4DisposeMovie(stream->spec->moov);
    return -2 ;
  }
  err = ISOGetMovieTrackCount(stream->spec->moov, &trackCount);if (err) return -2;

  genericOpenRead(stream, trackCount); /* open all tracks, handle dependencies */
  stream->providesIndependentReading = 0 /*1... 0 needed for access unit diagnosis*/;

  return trackCount;
}


static int MP4getDependency(HANDLE_STREAMFILE stream, int trackID)
{
  ISOErr err = ISONoErr;
  ISOTrack track;
  ISOTrack referencedTrack;
  u32 tmp;

  /* NOTE: trackID's range starting from 0 to trackCount-1, mp4 tracks are numbered from 1 to trackCount */
  err = ISOGetMovieIndTrack(stream->spec->moov, trackID+1, &track); if (err) return -3;
  err = ISOGetTrackEnabled(track, &tmp); if (err) return -3;
  if (!tmp) return -2;
  err = ISOGetTrackReferenceCount(track, MP4StreamDependencyReferenceType, &tmp); if (err) return -3;
  if (!tmp) return -1; /* non-dependant track */
  
  /* track depends on something */
  err = ISOGetTrackReference(track, MP4StreamDependencyReferenceType, 1, &referencedTrack); if (err) return -3;
  err = ISOGetTrackID(referencedTrack, &tmp); if (err) return -3;
  return tmp-1;
}


static int MP4openTrack(HANDLE_STREAMFILE stream, int trackID, int prog, int index)
{
  ISOErr err = ISONoErr;
  ISOTrack track;
  ISOHandle decoderConfigH;
  HANDLE_BSBITSTREAM bs = NULL;

  /* get dependencies */
  /*stream->prog[prog].dependencies[index] = stream->getDependency(stream, trackID);*/
  /*stream->prog[prog].programData->trackID = trackID+1;*/
  stream->prog[prog].dependencies[index] = index-1;

  /* open track and media, get track reader */
  /* NOTE: trackID's range starting from 0 to trackCount-1, mp4 tracks are numbered from 1 to trackCount */
  err = ISOGetMovieIndTrack(stream->spec->moov, trackID+1, &track); if (err) return -1;
  err = ISOGetTrackMedia(track, &stream->prog[prog].programData->spec->media[index]); if (err) return -1;
  err = ISOCreateTrackReader(track, &stream->prog[prog].programData->spec->reader[index]); if (err) return -1;

  /* get decoder configuration */
  err = ISONewHandle(0, &decoderConfigH); if (err) return -1;
  err = MP4TrackReaderGetCurrentDecoderConfig(stream->prog[prog].programData->spec->reader[index], decoderConfigH); if (err) goto bail;
  bs = BsFromHandle(decoderConfigH, 0);
  setupDecConfigDescr(&stream->prog[prog].decoderConfig[index]);
  err = advanceDecoderConfigDescriptor(bs, &stream->prog[prog].decoderConfig[index], 0/*read*/, 1/*do systems specific stuff*/
#ifndef CORRIGENDUM1
                                        ,1
#endif
      );

/* TODO get sampleEntry */
   
 bail:
  ISODisposeHandle(decoderConfigH);
  if (bs) BsClose(bs);
  return err;
}
  

static int MP4getAccessUnit(HANDLE_STREAMPROG stream, int trackNr, HANDLE_STREAM_AU au)
{
  ISOErr err;
  u32 unitSize;
  u32 sampleFlags;
  u32 cts;
  u32 dts;
  u8 pad = 0x7f;
  ISOHandle sampleDataH;

  err = ISONewHandle(0, &sampleDataH); if (err) return -1;
  err = MP4TrackReaderGetNextAccessUnitWithPad(stream->programData->spec->reader[trackNr], 
                                               sampleDataH, 
                                               &unitSize,
                                               &sampleFlags, 
                                               &cts, 
                                               &dts,
                                               &pad);

  if (pad > 7) pad = 0; /* padding unknown */

  DebugPrintf(6,"StreamFile:getAU(MP4): unitSize: %ld, padding bits: %ld",unitSize, pad);

  if (err) {
    if (err == ISOEOF) return -2;
    return -1;
  }

  StreamFile_AUcopyResize(au, (u8*)*sampleDataH, (unitSize<<3)-pad);

  ISODisposeHandle(sampleDataH);

  return 0;
}


static int MP4openWrite(HANDLE_STREAMFILE stream, int argc, char** argv)
{
  ISOErr err = ISONoErr;
  u32 initialObjectDescriptorID = 1;
  u8 OD_profileAndLevel = 0xff; 	/* none required */
  u8 scene_profileAndLevel = 0xff;	/* none required */
  u8 audio_profileAndLevel = 0xfe;
  u8 visual_profileAndLevel = 0xff; 	/* none required */
  u8 graphics_profileAndLevel = 0xff; 	/* none required */

  int result;

  /* - options parsing */

  result = commandline(stream, argc, argv);
  if (result) return result;

  audio_profileAndLevel = stream->spec->profileAndLevel;

  err = MP4NewMovie(&(stream->spec->moov),
		    initialObjectDescriptorID, 
		    OD_profileAndLevel,
		    scene_profileAndLevel,
		    audio_profileAndLevel,
		    visual_profileAndLevel,
		    graphics_profileAndLevel);

  stream->providesIndependentReading = 0 /*1... 0 needed for access unit diagnosis*/;

  return err;
}


static int MP4headerWrite(HANDLE_STREAMFILE stream)
{
  ISOErr err = ISONoErr;
  u32 x;
  int progIdx;

  for (progIdx=0; progIdx<stream->progCount; progIdx++) {
    HANDLE_STREAMPROG prog = &stream->prog[progIdx];
    for (x=0; x<prog->trackCount; x++) {
      ISOHandle audioSpecificInfo_out;
      HANDLE_BSBITSTREAM audioSpecificInfoStream;
      u32 tmp;

      err = ISONewMovieTrack(stream->spec->moov, ISONewTrackIsAudio, &prog->programData->spec->tracks[x]);if (err) goto bail;
      err = MP4AddTrackToMovieIOD( prog->programData->spec->tracks[x] ); if (err) goto bail;
      err = ISOSetTrackEnabled(prog->programData->spec->tracks[x], 1); if (err) goto bail;
      if ( prog->dependencies[x]!=-1 ) {
        err = ISOAddTrackReference(prog->programData->spec->tracks[x],
                                   prog->programData->spec->tracks[prog->dependencies[x]],
                                   MP4StreamDependencyReferenceType,
                                   &tmp/*outRefNum*/); if (err) goto bail;
      }

      err = ISONewTrackMedia(prog->programData->spec->tracks[x],
                             &(prog->programData->spec->media[x]),
                             ISOAudioHandlerType,
                             prog->decoderConfig[x].audioSpecificConfig.samplingFrequency.value,
                             NULL); if (err) goto bail;
      err = ISOBeginMediaEdits(prog->programData->spec->media[x]); if (err) goto bail;
      err = ISOSetMediaLanguage(prog->programData->spec->media[x], "und"); if (err) goto bail;

      /* create output decoder specific info */
      tmp = advanceAudioSpecificConfig( NULL,
                                        &prog->decoderConfig[x].audioSpecificConfig,
                                        2, /* 2 = get length */
                                        1, /* include mpeg4 systems specific stuff */
                                        -1 /* no length information */
#ifndef CORRIGENDUM1
                                        ,1
#endif
                                        );
      tmp = (tmp+7)/8;
      /* create output handle and bitstream with correct size */
      err = ISONewHandle(tmp, &audioSpecificInfo_out);if (err) goto bail;
      audioSpecificInfoStream = BsFromHandle ( audioSpecificInfo_out, 1 );

      /* write the struct */
      advanceAudioSpecificConfig( audioSpecificInfoStream,
                                  &prog->decoderConfig[x].audioSpecificConfig,
                                  1, /* 1 = write */
                                  1, /* include mpeg4 systems specific stuff */
                                  -1 /* no length information */
#ifndef CORRIGENDUM1
                                  ,1
#endif
                                  );
      
      err = ISONewHandle(0, &prog->programData->spec->sampleEntryH[x]);

      err = MP4NewSampleDescription(prog->programData->spec->tracks[x],
                                    prog->programData->spec->sampleEntryH[x],
                                    1,
                                    prog->decoderConfig[x].profileAndLevelIndication.value,
                                    prog->decoderConfig[x].streamType.value,
                                    prog->decoderConfig[x].bufferSizeDB.value,
                                    prog->decoderConfig[x].maxBitrate.value,
                                    prog->decoderConfig[x].avgBitrate.value,
                                    audioSpecificInfo_out); if (err) goto bail;

      BsClose( audioSpecificInfoStream );
    }

#if 0 /* #ifdef I2R_LOSSLESS */
    for (x=0; x<prog->ll_trackCount; x++) {
      ISOHandle audioSpecificInfo_out;
      HANDLE_BSBITSTREAM audioSpecificInfoStream;
      u32 tmp;

      err = ISONewMovieTrack(stream->spec->moov, ISONewTrackIsAudio, &prog->programData->spec->tracks[prog->trackCount + x]);if (err) goto bail;

      err = MP4AddTrackToMovieIOD( prog->programData->spec->tracks[prog->trackCount + x] ); if (err) goto bail;
      err = ISOSetTrackEnabled(prog->programData->spec->tracks[prog->trackCount + x], 1); if (err) goto bail;
#if 1  
	  if ( prog->dependencies[prog->trackCount + x]!=-1 ) {
        err = ISOAddTrackReference(prog->programData->spec->tracks[prog->trackCount + x],
                                   prog->programData->spec->tracks[prog->dependencies[prog->trackCount + x]],
                                   MP4StreamDependencyReferenceType,
                                   &tmp/*outRefNum*/); if (err) goto bail;
      }
#endif
      err = ISONewTrackMedia(prog->programData->spec->tracks[prog->trackCount + x],
                             &(prog->programData->spec->media[prog->trackCount + x]),
                             ISOAudioHandlerType,
                             prog->ll_decoderConfig[x].audioSpecificConfig.samplingFrequency.value,
                             NULL); if (err) goto bail;
      err = ISOBeginMediaEdits(prog->programData->spec->media[prog->trackCount + x]); if (err) goto bail;
      err = ISOSetMediaLanguage(prog->programData->spec->media[prog->trackCount + x], "und"); if (err) goto bail;

      /* create output decoder specific info */
      tmp = advanceAudioSpecificConfig( NULL,
                                        &prog->ll_decoderConfig[x].audioSpecificConfig,
                                        2, /* 2 = get length */
                                        1, /* include mpeg4 systems specific stuff */
                                        -1 /* no length information */
                                        );
      tmp = (tmp+7)/8;
      /* create output handle and bitstream with correct size */
      err = ISONewHandle(tmp, &audioSpecificInfo_out);if (err) goto bail;
      audioSpecificInfoStream = BsFromHandle ( audioSpecificInfo_out, 1 );

      /* write the struct */
      advanceAudioSpecificConfig( audioSpecificInfoStream,
                                  &prog->ll_decoderConfig[x].audioSpecificConfig,
                                  1, /* 1 = write */
                                  1, /* include mpeg4 systems specific stuff */
                                  -1 /* no length information */
                                  );


      err = ISONewHandle(0, &prog->programData->spec->sampleEntryH[prog->trackCount + x]);

      err = MP4NewSampleDescription(prog->programData->spec->tracks[prog->trackCount + x],
                                    prog->programData->spec->sampleEntryH[prog->trackCount + x],
                                    1,
                                    prog->ll_decoderConfig[x].profileAndLevelIndication.value,
                                    prog->ll_decoderConfig[x].streamType.value,
                                    prog->ll_decoderConfig[x].bufferSizeDB.value,
                                    prog->ll_decoderConfig[x].maxBitrate.value,
                                    prog->ll_decoderConfig[x].avgBitrate.value,
                                    audioSpecificInfo_out); if (err) goto bail;

      BsClose( audioSpecificInfoStream );

    }
#endif

  }
 bail:
  return err;
}


static int MP4putAccessUnit(HANDLE_STREAMPROG prog, int trackNr, HANDLE_STREAM_AU au)
{ 
  unsigned long i;
  ISOErr err = ISONoErr;
  ISOHandle sampleDataH;
  ISOHandle sampleSizeH;
  ISOHandle sampleDurationH;
  ISOHandle samplePaddingH;
  u32 length = (au->numBits+7) >> 3;

  err = ISONewHandle(sizeof(u32),&sampleSizeH); if (err) goto bail;
  err = ISONewHandle(sizeof(u32),&sampleDurationH); if (err) goto bail;
  err = ISONewHandle(sizeof(u8),&samplePaddingH); if (err) goto bail;
  err = ISONewHandle(length, &sampleDataH); if (err) goto bail;

  for (i=0; i<length; i++) {
    *(unsigned char*)(*sampleDataH+i) = au->data[i];
  }

  err = ISOGetHandleSize(sampleDataH, (u32*) *sampleSizeH); if (err) goto bail;
  *(u32*)*sampleDurationH = prog->programData->sampleDuration[trackNr];
  *(u8*)*samplePaddingH = (8-(au->numBits & 7))&7;

  DebugPrintf(6,"StreamFile:putAU(MP4): sampleSizeH: %ld, padding %i, sampleDurationH: %ld",*(u32*) *sampleSizeH, *(u8*)*samplePaddingH, *(u32*) *sampleDurationH);

  err = ISOAddMediaSamplesPad(prog->programData->spec->media[trackNr], 
                              sampleDataH, 
                              1, 
                              sampleDurationH, 
                              sampleSizeH, 
                              prog->programData->spec->sampleEntryH[trackNr], 
                              NULL, NULL,
                              samplePaddingH); if (err) goto bail;

  if (prog->programData->spec->sampleEntryH[trackNr]) {
    ISODisposeHandle(prog->programData->spec->sampleEntryH[trackNr]);
    prog->programData->spec->sampleEntryH[trackNr] = NULL;
  }
  ISODisposeHandle(sampleDataH);
  ISODisposeHandle(sampleSizeH);
  ISODisposeHandle(sampleDurationH);
  ISODisposeHandle(samplePaddingH);

 bail:
  return err;
}


static int MP4close(HANDLE_STREAMFILE stream)
{
  ISOErr err = ISONoErr;
  u32 x;
  int progIdx;

  if (stream->status == STATUS_WRITING) {

    /* close writing MP4 --- */
    for (progIdx=0; progIdx<stream->progCount; progIdx++) {
      HANDLE_STREAMPROG prog = &stream->prog[progIdx];
      for (x=0;x<prog->trackCount;x++) {
        u64 duration;
        err = ISOEndMediaEdits(prog->programData->spec->media[x]); if (err) goto bail;
        err = ISOGetMediaDuration(prog->programData->spec->media[x], &duration); if (err) goto bail;
        err = ISOInsertMediaIntoTrack(prog->programData->spec->tracks[x], 0, 0, duration, 1); if (err) goto bail;
      }
#ifdef I2R_LOSSLESS
#if 0
      for (x=0;x<prog->ll_trackCount;x++) {
        u64 duration;
        err = ISOEndMediaEdits(prog->programData->spec->media[prog->trackCount + x]); if (err) goto bail;
        err = ISOGetMediaDuration(prog->programData->spec->media[prog->trackCount + x], &duration); if (err) goto bail;
        err = ISOInsertMediaIntoTrack(prog->programData->spec->tracks[prog->trackCount + x], 0, 0, duration, 1); if (err) goto bail;
      }
#endif
#endif
    }
    err = ISOWriteMovieToFile(stream->spec->moov, stream->fileName);if (err) goto bail;

  } else {

    /* close reading MP4 --- */
    for (progIdx=0; progIdx<stream->progCount; progIdx++) {
      HANDLE_STREAMPROG prog = &stream->prog[progIdx];
      for (x=0;x<prog->trackCount;x++) {
        if (prog->programData->spec->reader[x])
          ISODisposeTrackReader(prog->programData->spec->reader[x]);
      }
    }
  }

  err = ISODisposeMovie(stream->spec->moov);
 bail:
  return err;
}


/* --------------------------------------------------- */
/* ---- Constructor                               ---- */
/* --------------------------------------------------- */

int MP4initStream(HANDLE_STREAMFILE stream)
{
  /*init and malloc*/
  stream->initProgram=MP4initProgram;
  /*open read*/
  stream->openRead=MP4openRead;
  stream->getDependency=MP4getDependency;
  stream->openTrack=MP4openTrack;
  /*open write*/
  stream->openWrite=MP4openWrite;
  stream->headerWrite=MP4headerWrite;
  /*close*/
  stream->close=MP4close;
  /*getAU, putAU*/
  stream->getAU=MP4getAccessUnit;
  stream->putAU=MP4putAccessUnit;

  if ((stream->spec = (struct tagStreamSpecificInfo*)malloc(sizeof(struct tagStreamSpecificInfo))) == NULL) {
    CommonWarning("StreamFile:initStream: error in malloc");
    return -1;
  }
  memset(stream->spec, 0, sizeof(struct tagStreamSpecificInfo));
  return 0;
}


void MP4showHelp( void )
{
  printf(MODULE_INFORMATION);
  commandline(NULL,0,NULL);
}
