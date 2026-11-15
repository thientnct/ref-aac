/******************************************************************************
 * 
 * This software module was originally developed by 
 * 
 *   Matejko Michael (Fraunhofer IIS)
 * 
 * and edited by 
 *
 *  
 * 
 * in the course of development of the ISO/IEC 13818-3 and/or ISO/IEC
 * 14496 for reference purposes and its performance may not have been
 * optimized. This software module is an implementation of one or more
 * tools as specified by the ISO/IEC 13818-3 and/or ISO/IEC 14496.
 * ISO/IEC gives You a royalty-free, worldwide, non-exclusive, copyright
 * license to copy, distribute, and make derivative works of this
 * software module ormodifications thereof for use in implementations of
 * the ISO/IEC 13818-3 and/or ISO/IEC 14496 in products that satisfy
 * conformance criteria (if any).  Those intending to use this software
 * module in products are advised that its use may infringe existing
 * patents. ISO/IEC have no liability for use of this software module or
 * modifications thereof.  Copyright is not released for products that
 * do not conform to audiovisual and image-coding related ITU
 * Recommendations and/or ISO/IEC International Standards.
 * 
 ******************************************************************************/

#include <stdio.h>
#include <netinet/in.h>

/* Include MPEG12 stuff*/
#include "debug.h"
#include "bitBuffer.h"
#include "frameDeMux.h"

/* Include MPEG4 stuff */
#include "streamfile.h" 
#include "streamfile_helper.h" 
#include "obj_descr.h"
#include "flex_mux.h" /* for AOT */


typedef struct s_auBuffer
{
  unsigned char *buffer;    /* pointer to buffer */
  int wpos;                 /* write position in bytes */
  int begin;                /* begin of last MPEG frame */
  int size;                 /* in bytes */
  HANDLE_STREAMPROG prog;   /* mpeg4 prog */
  int track;                /* mpeg4 track */
  struct s_auBuffer *next;  /* next auBuffer in list */
} *auBuffer;


static
int beginNewTrack(
  HANDLE_STREAMFILE mpeg4stream,    /* in */
  HANDLE_STREAMPROG *mpeg4prog,     /* out */
  int bitrate, 
  int samplerate, 
  int channels
)
{
  int mpeg4track;
  
  *mpeg4prog = StreamFileAddProgram(mpeg4stream);
  /* ATTENTION! mpeg4prog has other structure than in libstreamfile.a because
   * of different DEFFLAGS in makefiles. Be careful while using it directly!! */
  
  mpeg4track = StreamFileAddTrackToProg(*mpeg4prog);
  StreamFileSetupTrack(*mpeg4prog, mpeg4track, 34, bitrate, bitrate, samplerate, channels);
  StreamFileFixProgram(*mpeg4prog);
  
  return mpeg4track;
}


/** Move all AUs from memory to bitstream */ 
static
int flushData(auBuffer first)
{
  HANDLE_STREAM_AU mpeg4au = StreamFileAllocateAU(first->size * 8);
  auBuffer au = first;
  auBuffer last;
  int cnt = 0;

  while (au)
  {
    StreamFile_AUcopyResize(mpeg4au, au->buffer, au->size * 8);
    StreamPutAccessUnit(au->prog, au->track, mpeg4au);
    last = au;
    au = au->next;
    free(last->buffer);
    free(last);
    cnt++;
  }
  StreamFileFreeAU(mpeg4au);
  return cnt;
}

#define MAX_IN_FILES 5
#define MAX_PROGS 10

int main( int argc, char** argv )
{
  /**************************************************
    Program variables
  **************************************************/

  char *              outfile;
  char *              infile[MAX_IN_FILES]; 
  int                 i;
  int                 infiles         = -1;
  int                 outprogs        = 0;
  
  /* MPEG12 stuff */
  HANDLE_debug        debug           = NULL;     /* global debug handler */
  HANDLE_frameDeMux   frameDeMux[MAX_IN_FILES];
  HANDLE_bitBuffer    mpeg12stream[MAX_IN_FILES]; /* bitstream buffers */
  HANDLE_bitBuffer    audioFrame[MAX_IN_FILES];   /* audio frame buffers */
  frameConfig         frameConfig[MAX_IN_FILES];
  
  /* MPEG4 stuff */
  HANDLE_STREAMFILE   mpeg4stream     = NULL;
  HANDLE_STREAMPROG   mpeg4prog       = NULL;
  int                 mpeg4track      = 0;
  int                 auCount;
  
  auBuffer AU_first   = NULL;
  auBuffer AU_current = NULL;
  
#if 0
  debug = debugInit(D_DEBUG, stdout);
#else
  debug = debugInit(D_WARNING, stdout);
#endif

  debugEnter(debug, "mpeg12dec");

  printf("MPEG12 to MPEG4\n");
  
  
  
  /**************************************************
    Check command line args
  **************************************************/

  if (argc < 3)
  {  
    printf("usage: %s <in.mpg> [<inX.mpg>...] <out.mp4>\n", argv[0]);
    goto bail;
  }
  
  infiles = argc-2;
  
  if (infiles > MAX_IN_FILES)
  {
    printf("ERROR: There are up to %d infiles allowed.", MAX_IN_FILES);
    goto bail;
  }
  
  for (i = 0; i < infiles; i++)
    infile[i] = argv[i+1];
    
  outfile = argv[argc-1];
  
  
  
  /**************************************************
    Initialize input/output buffers
  **************************************************/
  for (i = 0; i < infiles; i++)
  {
    mpeg12stream[i] = NULL;
    frameDeMux[i]   = NULL;
    audioFrame[i]   = NULL;
  }
  
  for (i = 0; i < infiles; i++)
  {
    mpeg12stream[i] = bitBufferInit(MAX_BS_FRAME_SIZE, "mpeg12stream", debug);
    if ( mpeg12stream[i] == NULL ) 
    {
      debugPrintf (debug, D_ERROR, "Could not initialize bitstream buffer.");
      goto bail;
    }

    /* Open MPEG12 file */
    if ( !bitBufferOpenInputFile(mpeg12stream[i], infile[i]) ) 
    {
      debugPrintf (debug, D_ERROR, "Could not open input file.");
      goto bail;
    }
    
    frameDeMux[i] = frameDeMuxInit(mpeg12stream[i], debug);
    if ( frameDeMux[i] == NULL )
    {
      debugPrintf (debug, D_ERROR, "Could not init frame demuxer.");
      goto bail;
    }
    
    audioFrame[i] = bitBufferInit(MAX_AU_FRAME_SIZE, "audioFrame", debug);
    if ( audioFrame[i] == NULL ) 
    {
      debugPrintf (debug, D_ERROR, "Could not initialize audioFrame buffer.");
      goto bail;
    }
  }
  
  /**************************************************
    Open MP4 File
  **************************************************/
  mpeg4stream = StreamFileOpenWrite(outfile, FILETYPE_AUTO, NULL);
  AU_first = calloc(sizeof(struct s_auBuffer), 1);
  AU_current  = AU_first; 


  /**************************************************
    Loop over all available MPEG12 frames
  **************************************************/
  while (1)
  {
    /* Init variables 
     * ============== */
    static int  frameCount      = 0;
    int         allFramesValid  = 1;
    int         allChannels     = 0;
    int         allSamplerate   = 0;
    int         allBitrate      = 0; 
    int         allVersion      = 0;
    int         allLayer        = 0;
    static int  oldChannels     = 0;
    static int  oldSamplerate   = 0;
    
    debugPrintf(debug, D_NOTICE_L1, "Frame %d", ++frameCount);
    
    /* get next frames from all MPEG12 files 
     * ===================================== */
    for (i = 0; i < infiles; i++)
    {
      switch (frameDeMuxNext(frameDeMux[i], audioFrame[i]))
      {
        case FMUX_NO_ERROR:
          /* do nothing */
          break;

        case FMUX_END_OF_STREAM:
          /* no more data available */
          printf("done.\n");
          goto bail;
          
          
        case FMUX_INVALID_FRAME_LENGTH:
          debugPrintf(debug, D_WARNING, "Invalid frame length or not enough data to decode in file '%s'.", infile[i]);
          allFramesValid = 0;
          continue;

        case FMUX_NOT_ENOUGH_DATA:
          debugPrintf(debug, D_WARNING, "Not enough data to decode in file '%s'.", infile[i]);
          continue;

        default:
          debugPrintf(debug, D_ERROR, "Error while demuxing next frame in file '%s'", infile[i]);
          goto bail;
      }

      /* Get frame configuration */
      frameConfig[i] = frameDeMuxGetCurrentConfig(frameDeMux[i]);
      allChannels += frameConfig[i].channels;
      allBitrate  += frameConfig[i].bitrate * 1000;
      
      /* Print file info  at beginning */
      if (frameCount == 0)
      {
        printf("%s: MPEG%d layer%d, %d kbps, %d kHz, %d channels\n", infile[i],
            frameConfig[i].version, frameConfig[i].layer, frameConfig[i].bitrate, 
            frameConfig[i].samplerate, frameConfig[i].channels);
      }
    }
    
    /* Check frames and configuration 
     * ============================== */
    if (! allFramesValid)
    {
      debugPrintf(debug, D_WARNING, "There are invalid MPEG12 bitstream frames in time frame %d. Frame will be discarted.", frameCount);
      continue;
    }
    
    if (frameConfig[0].layer < 3 && infiles > 1)
    {
      debugPrintf(debug, D_ERROR, "More than one input file is not allowed for layers 1 and 2.");
      goto bail;
    }
    
    if (allChannels > 7)
    {
      debugPrintf(debug, D_ERROR, "More than 7 channels are not allowed. Current channelcount: %d", allChannels);
      goto bail;
    }
    else
    {
      const int channelConf[8] = {0, 1, 1, 2, 3, 3, 4, 5}; 
      if (channelConf[allChannels] != infiles)
      {
        debugPrintf(debug, D_ERROR, "There are %d channels in %d files. Expected %d files.", allChannels, infiles, channelConf[allChannels]);
        goto bail;
      }
    }

    allSamplerate = frameConfig[0].samplerate;
    allVersion    = frameConfig[0].version;
    allLayer      = frameConfig[0].layer;
    for (i=1; i < infiles; i++)
    {
      if (frameConfig[i].samplerate != allSamplerate)
      {
        debugPrintf(debug, D_ERROR, "There are different samplerates in MPEG12 files.");
        goto bail;
      }
      if (frameConfig[i].version != allVersion)
      {
        debugPrintf(debug, D_ERROR, "There are different MPEG versions in input files.");
        goto bail;
      }
      if (frameConfig[i].layer != allLayer)
      {
        debugPrintf(debug, D_ERROR, "There are different layer versions in input files.");
        goto bail;
      }
    }
    
    if (allLayer != 3 && infiles > 1)
    {
      debugPrintf(debug, D_ERROR, "For layer 1 and 2 there is only one input file allowed.");
      goto bail;
    }
    
    /* Init or Reinit new MPEG4 Track 
     * ============================== */
    if ( allSamplerate != oldSamplerate || allChannels != oldChannels ) 
    {
      oldSamplerate   = allSamplerate;
      oldChannels     = allChannels;
      
      printf("Initiating new track with following configuration:\n"
          "  samplerate:%7d samples/s\n"
          "  channels:  %7d channels\n"
          "  bitrate:   %7d kbit/s\n",
          allSamplerate, allChannels, allBitrate / 1000);
      
      mpeg4track = beginNewTrack(mpeg4stream, &mpeg4prog, allBitrate, allSamplerate, allChannels);
    }
    
    /* Put all audio frames into next AU 
     * ================================= */
    AU_current->prog  = mpeg4prog;
    AU_current->track = mpeg4track; 
    for (i = 0; i < infiles; i++)
    {
      unsigned long mp3frameSize = (bitBufferGetWritten(audioFrame[i]) + 7) / 8;
      unsigned long wBytes;
      unsigned long wBits  = 0;
      /* add MPEG12 audio data to AU buffer 
       * ================================== */
      AU_current->begin = AU_current->wpos; 
      AU_current->size += mp3frameSize; /* in bytes */
      AU_current->buffer = realloc(AU_current->buffer, AU_current->size);
        
      /* copy all audio data to auBuffer */
      for (wBytes=0; wBytes<mp3frameSize; wBytes++)
      {
        unsigned long data;
        wBits = bitBufferRead(audioFrame[i], &data, 8);
        AU_current->buffer[AU_current->wpos++] = (unsigned char) data;
      } 

      if (wBits < 8)
      {
        /* last one may be shifted to right position */
        AU_current->buffer[AU_current->wpos-1] <<= 8-wBits;
      }

      /* Write 11 bit length information for layer3 */
      if (allLayer == 3)
      {
        unsigned char hi = (mp3frameSize >> 4) & 0xFF; /* ABC -> AB */
        unsigned char lo = (mp3frameSize & 0x0F) << 4; /* ABC -> C0 */

        /* replace first 12 bits with len_info */
        AU_current->buffer[AU_current->begin] = hi;
        AU_current->buffer[AU_current->begin+1] &= 0x0F; /* clear */
        AU_current->buffer[AU_current->begin+1] ^= lo;
      }
    }

    /* create new AU (access unit) list element */
    AU_current->next  = calloc(sizeof(struct s_auBuffer), 1);
    AU_current = AU_current->next;
  } /* while - for each frame! */
  
  

  
 
  /**************************************************
     Close everything
   **************************************************/

bail:
  /* MP4 stuff 
   * ========= */    
  /* write data to file */
  if (AU_first)
  {
    auCount = flushData(AU_first);
    printf("Written %d AUs to mpeg4 bitstream.\n", auCount);  
  }
  if (mpeg4stream) StreamFileClose(mpeg4stream);


  /* MP3 stuff
   * ========= */
  for (i = 0; i < infiles; i++)
  {
    frameDeMuxFree(frameDeMux[i]);
    bitBufferFree(mpeg12stream[i]);
    bitBufferFree(audioFrame[i]);
  }
  debugLeave(debug);
  debugFree(debug);

  return 0;  
}



