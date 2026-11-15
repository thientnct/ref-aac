/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)
  Mike Coleman (Five Bats Research)

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

2000-10-18 : Olaf Kaehler, Fraunhofer IIS  started using libaudio (AFsp)
 *                                                                           *
 ****************************************************************************/

#include "config.h"
#include "intrins.h"
#include "audioout.h"

#ifndef USELIB_AFsp
#include "aiff_support.h"
#else
#include <libtsp.h>
#include <libtsp/AFpar.h>
#endif

#ifdef ENABLE_RTPLAY
#include <audiolib.h>
#endif

/* --- local data --- */

#define	PATH_LEN    128


struct tagAudioOutput {
#ifdef USELIB_AFsp
  AFILE*	outfd[Chans];
#else
#ifdef ENABLE_RTPLAY
  hAudioChannel	outfd[Chans];
#else
  FILE*		outfd[Chans];
#endif
#endif
  int		opened;
  char		path[PATH_LEN];


#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
  unsigned int	multifiles;
#else
  unsigned long	frames;
#endif
  char*		format;
  unsigned int	channels;
  unsigned int	wordsize;
  unsigned long	sampleRate;
  MC_Info*	MC_Info;

#ifdef ENABLE_RTPLAY
  int		realtime;
  AuChanInfo	outinfo;
#endif
};

static HANDLE_AUDIOOUTPUT audioout_alloc()
{
  HANDLE_AUDIOOUTPUT ret = (HANDLE_AUDIOOUTPUT)malloc(sizeof(struct tagAudioOutput));
  int i;
  if (ret==NULL) return ret;
#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
  ret->multifiles = 0;
#else
  ret->frames = 0;
#endif
  ret->format = "raw";
  ret->channels = 1;
  ret->wordsize = 16;
  ret->sampleRate = 48000;
  ret->MC_Info = NULL;
#ifdef ENABLE_RTPLAY
  ret->realtime = 0;
  ret->outinfo.typeInfo.rt.level = 100;
#endif

  for (i=0; i<Chans; i++) ret->outfd[i]=NULL;

  ret->opened = 0;
  return ret;
}

HANDLE_AUDIOOUTPUT audioout_open(const char *filename, const char *format, int wordsize)
{
  HANDLE_AUDIOOUTPUT output;
  if (strlen(filename) + strlen("_f00.pcm") + 1 > PATH_LEN) {
    PRINT(SE, "output path too long %s\n", filename);
    return NULL;
  }
  output = audioout_alloc();
  if (output == NULL) return NULL;

  strcpy(output->path, filename);
  output->format = (char*)format; /* FIXME: strcpy? */
  output->wordsize = wordsize;
  
#ifdef ENABLE_RTPLAY
  if (!strcmp(filename,"-")) { /* prepare realtime output */
    output->realtime = 1;
    output->multifiles = 0;
  }
#endif

  return output;
}


#ifdef ENABLE_RTPLAY
void audioout_setRealtimePlay(HANDLE_AUDIOOUTPUT output, int outputLevel)
{
  if (output==NULL) return;
  if (!output->opened) {
    output->outinfo.typeInfo.rt.level = outputLevel;
    output->realtime = 1;
    output->multifiles = 0;
  }
}
#endif


/* -- functions for output-files -- */
static void
write_fext(Ch_Info *cip, char c, int n)
{
  cip->fext = (char *)mal1(5);
  sprintf(cip->fext, "_%c%02d", c, n);
}

static int
open_output_files(HANDLE_AUDIOOUTPUT output, MC_Info *mip)
{
    int i, j = 0;
    char file[PATH_LEN];

#ifdef USELIB_AFsp

    int format_id;

    struct {
      char *str;
      int fmt;
    } fmtstr[] = {    /* these types are also "auto-detected" from output-argument */
      /*{"au",FTW_AU*256},
        {"snd",FTW_AU*256},*/
      {"wav",FTW_WAVE*256},
      {"aiff",FTW_AIFF_C*256},
      /*{"aif",FTW_AIFF_C*256},
        {"aifc",FTW_AIFF_C*256},*/
      {"pcm",FTW_NH_EB*256},      /* no header big-endian */
      {"raw",FTW_NH_EB*256},      /* no header big-endian */
      {NULL,-1}
    };
#else
#ifdef ENABLE_RTPLAY
    AuChanType auChnType = TYPE_RT;
    AuChanError err;
    int format_id;
    struct {
      char *str;
      int fmt;
    } fmtstr[] = {
      {"wav",TYPE_RIFF},
      {"aiff",TYPE_AIFF},
      {"pcm",TYPE_PCM},
      {"raw",TYPE_PCM},
      {NULL,-1}
    };
#endif
#endif

    output->MC_Info = mip;
    output->sampleRate = /*sampling_rate[*/mip->sampling_rate/*_idx]*/; 
#ifdef CT_SBR
    output->sampleRate *= (int)mip->upsamplingFactor;
#endif

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
    for (i=strlen(output->path);(i>=(signed int)(strlen(output->path)-5))&&(i>=0);i--) {
      if (output->path[i]=='.') {
        j=-1;
        while ( fmtstr[++j].str ) {
          if (!strcmp(fmtstr[j].str,(output->path+i+1))) {
            output->format=output->path+i+1;
            *(output->path+i)=0;
            break;
          }
        }
      }
    }

    i=-1;j=0;
    while ( fmtstr[++i].str )
      if (!strcmp(fmtstr[i].str, output->format)) break;

    if ( fmtstr[i].str == NULL ) {
      PRINT(SE, "Internal Error: Unknown output-format\n");
      return -1;
    }

    format_id = fmtstr[i].fmt;

#ifdef USELIB_AFsp
    format_id += ((output->wordsize == 24)? FD_INT24 : FD_INT16);
#endif
#endif

    /* if FCenter == 1 (center speaker present)
     *	position    index   extension
     *	left	    1	    01
     *	center	    0	    00
     *	right	    2	    02
     * if FCenter == 0 (center speaker absent)
     *	position    index   extension
     *	left	    0	    01
     *	right	    1	    02
     */
    for (i=((FCenter==1) ? 0 : 1); i<FChans; i++) {
      write_fext(&mip->ch_info[j], 'f', i);
      j++;
    }
    for (i=0; i<SChans; i++) {
      write_fext(&mip->ch_info[j], 's', i);
      j++;
    }
    /* if BFCenter == 1 (center speaker present)
     *	position    index   extension
     *	left	    3	    00
     *	center	    5	    02
     *	right	    4	    01
     * if BCenter == 0 (center speaker absent)
     *	position    index   extension
     *	left	    3	    00
     *	right	    4	    01
     */
    for (i=0; i<BChans; i++) {
      write_fext(&mip->ch_info[j], 'b', i);
      j++;
    }
    for (i=0; i<LChans; i++) {
      write_fext(&mip->ch_info[j], 'l', i);
      j++;
    }

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
    if (!output->multifiles) {
      j=0;
      for (i=0; i<Chans; i++) 
        if (mip->ch_info[i].present) j++;
      if (j>2) {
        output->multifiles = 1;
        PRINT(SE, "warning: no standards for %i channels in one file... using multiple\n", j);
      } else {
        output->channels = j;
      }
#ifdef CT_SBR
#ifdef PARAMETRICSTEREO
      if (mip->sbrEnablePS) {
        if (output->channels == 1) {
          output->channels = 2;
        }
        else {
          PRINT(SE, "sbrEnablePS requires mono AAC channel config (%d)\n",output->channels);
          return -1;
        }
      }
#endif
#endif
    }
#endif
#ifdef ENABLE_RTPLAY
    if (output->realtime) {
      auChnType = TYPE_RT;
      if ((output->channels>2)||(output->multifiles)) {
        PRINT(SE, "error: can't play so many channels realtime\n");
        return -1;
      }
      output->outinfo.valid = 1;
      output->outinfo.bitsPerSample = 16;
      output->outinfo.sampleRate = output->sampleRate;
      output->outinfo.nChannels = output->channels;
      output->outinfo.nSamples = 0/*LN2*/;
      output->outinfo.fpScaleFactor = 32768.0;
      output->outinfo.typeInfo.rt.outputDrain = OUT_SPEAKER;
      err = AuChannelOpen(&output->outfd[0], NULL/*outname*/, (AuChanMode)AU_CHAN_WRITE, &auChnType, &output->outinfo);
      if (err != AU_CHAN_OK ) {
        PRINT(SE, "AuChannelOpen reported an error\n" );
        return -1;
      }
    } else {
      auChnType = (AuChanType)format_id;
      output->outinfo.valid = 1;
      output->outinfo.bitsPerSample = output->wordsize;
      output->outinfo.sampleRate = output->sampleRate;
      output->outinfo.nChannels = output->channels;
      output->outinfo.nSamples = 0/*LN2*/;
      output->outinfo.fpScaleFactor = 32768.0;
#endif

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
      if (output->multifiles) { /* output several files */
#endif
        j=0;
        for (i=0; i<Chans; i++) {
          if (!(mip->ch_info[i].present))
            continue;

          j++;

          strcpy(file, output->path);
          strcat(file, mip->ch_info[i].fext);
          strcat(file, ".");
          strcat(file, output->format);

          if (debug['o'])
            PRINT(SE, "opening file %s at position %d\n", file, i);

#ifdef ENABLE_RTPLAY
          err = AuChannelOpen(&output->outfd[i], file, (AuChanMode)AU_CHAN_WRITE, &auChnType, &output->outinfo);
          if (err != AU_CHAN_OK ) {
            PRINT(SE, "AuChannelOpen reported an error\n" );
            return -1;
          }
#else
#ifdef USELIB_AFsp
          output->outfd[i] = AFopenWrite(file, format_id, output->channels, output->sampleRate, (FILE*)NULL );
          if(output->outfd[i] == NULL) {
            PRINT(SE, "cannot create %s\n", file);
            return -1;
          }
#else
          /* no AFsp-lib support */
          output->outfd[i] = fopen(file, "w");
          if(output->outfd[i] == NULL) {
            PRINT(SE, "cannot create %s\n", file);
            return -1;
          }
          if ( !strcmp(output->format,"aiff") ) {
            /* write dummy header and position for audio data */
            if ( WriteAIFFHeader(output->outfd[i], output->channels, output->frames, output->wordsize, output->sampleRate) != 0 ) {
              PRINT( SE, "cannot write AIFF header for %s\n", file );
              return -1;
            }
          }
#endif
#endif
        }
#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
      } else {   /* place multiple channels in one file */
        strcpy(file, output->path);
        strcat(file, ".");
        strcat(file, output->format);

        if (debug['o'])
          PRINT(SE, "opening file %s with %d channels\n", file, output->channels);
#ifdef USELIB_AFsp
        output->outfd[0] = AFopenWrite(file, format_id, output->channels, output->sampleRate, (FILE*)NULL );

        if(output->outfd[0] == NULL) {
          PRINT(SE, "cannot create %s\n", file);
          return -1;
        }
#else
        err = AuChannelOpen(&output->outfd[0], file, (AuChanMode)AU_CHAN_WRITE, &auChnType, &output->outinfo);
        if (err != AU_CHAN_OK ) {
          PRINT(SE, "AuChannelOpen reported an error\n" );
          return -1;
        }
#endif
      }
#endif
#ifdef ENABLE_RTPLAY
    }
#endif
    return(1);
}

#if !((defined USELIB_AFsp)||(defined ENABLE_RTPLAY))
static void
fmtchan(char *p, Float *data, int block_length, int stride)
{
  int i, c;
  Float s;

  for(i=0; i<block_length; i++) {
    s = *data++;
    if(s < 0) {
      s -= .5;
      if(s < -0x7fff)
        s = (Float) -0x7fff;
    } else {
      s += .5;
      if(s > 0x7fff)
        s = (Float) 0x7fff;
    }
    c = (int) s;

    p[0] = (c >> 8) & 0xff;
    p[1] = c & 0xff;

    p += stride;
  }
}
#endif


int audioout_write(HANDLE_AUDIOOUTPUT output, Float **data, int block_length, MC_Info *mip)
{
   int i;

   /*   int block_length = LN2;*/

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
   int j;
   float *fl_obuf;
#else
   char *obuf;
   int stride = 2;
#endif

   if (output==NULL) return -1;

   /* open output files, if not done already */
   if (!output->opened) {
     if (open_output_files(output, mip) != 1) return -2;
     output->opened = 1;
   } else {
     if ((mip!=output->MC_Info)&&(mip!=NULL)) {
       PRINT(SE, "Warning: channel configuration seems to have changed...\n");
       /* TODO: close old output, open new output */
     }
   }

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
   fl_obuf = (float*)malloc(block_length*Chans*sizeof(float));
#else
   obuf = (char*)malloc(block_length*2*sizeof(char));
#endif

#if (defined ENABLE_RTPLAY)||(defined USELIB_AFsp)
   if (!output->multifiles) { /* multiple channels into one file */
     int k=0;
     for (i=0; i<Chans; i++) {
       if (!(output->MC_Info->ch_info[i].present))
         continue;
       for (j=0; j<block_length; j++)
         fl_obuf[j*output->channels+k] = data[i][j];
#ifdef CT_SBR
#ifdef PARAMETRICSTEREO
       if (output->MC_Info->sbrEnablePS) {
         for (j=0; j<block_length; j++)
           fl_obuf[j*output->channels+k+1] = data[i+1][j];
       }
#endif
#endif
       k++;
     }
#ifdef ENABLE_RTPLAY
     AuChannelWriteFloat( output->outfd[0], fl_obuf, block_length*output->channels, (unsigned int*)&i /*written bytes*/);
#endif
#ifdef USELIB_AFsp
     AFwriteData(output->outfd[0], fl_obuf, block_length*output->channels);
#endif
   } else {
#endif
     for (i=0; i<Chans; i++) {
       if (!(output->MC_Info->ch_info[i].present))
         continue;

#if (defined USELIB_AFsp)||(defined ENABLE_RTPLAY)
       for (j=0; j<block_length; j++)
         fl_obuf[j] = data[i][j];
#endif

#ifdef USELIB_AFsp
       AFwriteData(output->outfd[i], fl_obuf, block_length);
#else
#ifdef ENABLE_RTPLAY
       AuChannelWriteFloat(output->outfd[i], fl_obuf, block_length, (unsigned int*)&j /*written bytes*/);
#else
       fmtchan(obuf, data[i], block_length, stride);
       if (fwrite(obuf, 1, block_length*2, output->outfd[i]) != (size_t)(block_length*2)) {
         PRINT(SE, "write error\n");
         return -1;
       }
#endif
#endif
     }
#if ((defined USELIB_AFsp)||(defined ENABLE_RTPLAY))
   }
   free(fl_obuf);
#else
   free(obuf);
   if ( !strcmp(output->format,"aiff") )
     output->frames += block_length;
#endif
   return 0;
}



/* -- nice exit functions -- */

int audioout_close(HANDLE_AUDIOOUTPUT output)
{
  int i;

  if (output == NULL) return -1;
  if (output->opened==0) goto bail;

  for ( i = 0; i < Chans; i++ ) {
    if (output->MC_Info->ch_info[i].fext) free1(output->MC_Info->ch_info[i].fext);
    if ( !output->outfd[i] )
      continue;
#ifdef USELIB_AFsp
    AFclose(output->outfd[i]);
#else
#ifdef ENABLE_RTPLAY
    AuChannelClose(output->outfd[i]);
#else
    if ( !strcmp(output->format,"aiff") ) 
      WriteAIFFHeader( output->outfd[i], output->channels, output->frames, output->wordsize, output->sampleRate );
    fclose( output->outfd[i] );
#endif
#endif
  }
 bail:
  free(output);
  return 0;
}
