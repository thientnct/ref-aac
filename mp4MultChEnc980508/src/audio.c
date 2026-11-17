/**********************************************************************
MPEG-4 Audio VM
Audio i/o module



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



Source file: audio.c

$Id: audio.c,v 1.1.1.2 1998-05-07 17:21:16+03 suonio Exp $

Required libraries:
libtsp.a		AFsp audio file library

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
21-jan-97   HP    born (using AFsp-V2R2)
27-jan-97   HP    set unavailable samples to 0 in AudioReadData()
03-feb-97   HP    fix bug AudioInit formatString=NULL
19-feb-97   HP    made internal data structures invisible
21-feb-97   BT    raw: big-endian
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libtsp.h>		/* AFsp audio file library */
#include <libtsp/AFpar.h>	/* AFsp audio file library - definitions */

#include "audio.h"		/* audio i/o module */
#include "common.h"		/* common module */


/* ---------- declarations ---------- */

#define SAMPLE_BUF_SIZE 16384	/* local sample buffer size */

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))


/* ---------- declarations (structures) ---------- */

struct AudioFileStruct		/* audio file handle */
{
  AFILE *file;			/* AFILE handle */
  int numChannel;		/* number of channels */
  long currentSample;		/* number of samples read/written */
				/* (samples per channel!) */
  int write;			/* 0=read  1=write */
};


/* ---------- variables ---------- */

static int AUdebugLevel = 0;	/* debug level */


/* ---------- functions ---------- */


/* AudioInit() */
/* Init audio i/o module. */
/* formatString options: see AFsp documentation */

void AudioInit (
  char *formatString,		/* in: file format for headerless files */
  int debugLevel)		/* in: debug level */
				/*     0=off  1=basic  2=full */
{
  AUdebugLevel = debugLevel;
  if (AUdebugLevel >= 1) {
    printf("AudioInit: formatString=\"%s\"\n",
	   (formatString!=NULL)?formatString:"(null)");
    printf("AudioInit: debugLevel=%d\n",AUdebugLevel);
  }
  if (formatString!=NULL)
    AFsetNH(formatString);   /* headerless file support */
}


/* AudioOpenRead() */
/* Open audio file for reading. */

AudioFile *AudioOpenRead (
  char *fileName,		/* in: file name */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample)		/* out: number of samples in file */
				/*      (samples per channel!) */
				/*      or 0 if not available */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */
{
  AudioFile *file;
  AFILE *af;
  long ns;
  long nc;
  float fs;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: fileName=\"%s\"\n",fileName);

  if ((file=(AudioFile*)malloc(sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenRead: memory allocation error");

  af = AFopenRead(fileName,&ns,&nc,&fs,
		  AUdebugLevel?stdout:(FILE*)NULL);

  if (af==NULL) {
    CommonWarning("AudioOpenRead: error opening audio file %s",fileName);
    free(file);
    return (AudioFile*)NULL;
  }

  file->file = af;
  file->numChannel = nc;
  file->currentSample = 0;
  file->write = 0;
  *numChannel = nc;
  *fSample = fs;
  *numSample = ns;

  if (AUdebugLevel >= 1)
    printf("AudioOpenRead: numChannel=%d  fSample=%.1f  numSample=%ld\n",
	   *numChannel,*fSample,*numSample);

  return file;
}


/* AudioOpenWrite() */
/* Open audio file for writing. */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Supported file formats: (matching substring of format) */
/*  au, snd:  Sun (AFsp) audio file */
/*  wav:      RIFF WAVE file */
/*  aif:      AIFF-C audio file */
/*  raw:      headerless (raw) audio file (native byte order) */

AudioFile *AudioOpenWrite (
  char *fileName,		/* in: file name */
  char *format,			/* in: file format */
				/*     (au, snd, wav, aif, raw) */
  int numChannel,		/* in: number of channels */
  float fSample)		/* in: sampling frequency [Hz] */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */
{
  AudioFile *file;
  AFILE *af;
  int fmt;

  if (AUdebugLevel >= 1) {
    printf("AudioOpenWrite: fileName=\"%s\"  format=\"%s\"\n",fileName,format);
    printf("AudioOpenWrite: numChannel=%d  fSample=%.1f\n",
	   numChannel,fSample);
  }

  fmt = FD_INT16;
  if (strstr(format,"au")!=NULL ||
      strstr(format,"snd")!=NULL)
    fmt += FW_SUN;
  else if (strstr(format,"wav")!=NULL)
    fmt += FW_WAVE;
  else if (strstr(format,"aif")!=NULL)
    fmt += FW_AIFF_C;
  else if (strstr(format,"raw")!=NULL)
    fmt +=  FW_NH_EB;	/* we allways want same endian type on all machines */
			/* not the native one 'FW_NH_NATIVE'; */
			/* lets take big-endian */
  else {
    CommonWarning("AudioOpenWrite: unkown audio file format \"%s\"",format);
    return (AudioFile*)NULL;
  }

  if ((file=(AudioFile*)malloc(sizeof(AudioFile))) == NULL)
    CommonExit(1,"AudioOpenWrite: memory allocation error");

  af = AFopenWrite(fileName,fmt,numChannel,fSample,
		  AUdebugLevel?stdout:(FILE*)NULL);

  file->file = af;
  file->numChannel = numChannel;
  file->currentSample = 0;
  file->write = 1;

  return file;
}


/* AudioReadData() */
/* Read data from audio file. */
/* Requested samples that could not be read from the file are set to 0. */

long AudioReadData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* out: data[channel][sample] */
				/*      (range [-32768 .. 32768]) */
  long numSample)		/* in: number of samples to be read */
				/*     (samples per channel!) */
				/* returns: */
				/*  number of samples read */
{
  long tot,cur,num,tmp;
  long i;
  long numRead;
  float buf[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 2)
    printf("AudioReadData: numSample=%ld\n",numSample);

  if (file->write != 0)
    CommonExit(1,"AudioReadData: audio file not in read mode");

  tot = file->numChannel*numSample;
  cur = 0;
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
    tmp = AFreadData(file->file,
		     file->numChannel*file->currentSample+cur,buf,num);
    for (i=0; i<tmp; i++)
      data[(cur+i)%file->numChannel][(cur+i)/file->numChannel] = buf[i];
    cur += tmp;
    if (tmp < num)
      break;
  }
  numRead = cur/file->numChannel;
  file->currentSample += numRead;

  /* reset unavailable samples */
  for (i=cur; i<tot; i++)
    data[i%file->numChannel][i/file->numChannel] = 0;

  return numRead;
}


/* AudioWriteData() */
/* Write data to audio file. */

void AudioWriteData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* in: data[channel][sample] */
				/*     (range [-32768 .. 32768]) */
  long numSample)		/* in: number of samples to be written */
				/*     (samples per channel!) */
{
  long tot,cur,num;
  long i;
  float buf[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 2)
    printf("AudioWriteData: numSample=%ld\n",numSample);

  if (file->write != 1)
    CommonExit(1,"AudioWriteData: audio file not in write mode");

  tot = file->numChannel*numSample;
  cur = max(0,-file->numChannel*file->currentSample);
  while (cur < tot) {
    num = min(tot-cur,SAMPLE_BUF_SIZE);
    for (i=0; i<num; i++)
      buf[i] = data[(cur+i)%file->numChannel][(cur+i)/file->numChannel];
    AFwriteData(file->file,buf,num);
    cur += num;
  }
  file->currentSample += tot/file->numChannel;
}


/* AudioSeek() */
/* Set position in audio file to curSample. */
/* (Beginning of file: curSample=0) */
/* NOTE: It is not possible to seek backwards in a output file if */
/*       any samples were already written to the file. */

void AudioSeek (
  AudioFile *file,		/* in: audio file (handle) */
  long curSample)		/* in: new position [samples] */
				/*     (samples per channel!) */
{
  long tot,cur,num;
  float buf[SAMPLE_BUF_SIZE];

  if (AUdebugLevel >= 1)
    printf("AudioSeek: curSample=%ld\n",curSample);

  if (file->write==0)
    file->currentSample = curSample;
  else {
    if (file->currentSample <= 0) {
      /* nothing written to file yet */
      if (curSample <= 0)
	file->currentSample = curSample;
      else
	file->currentSample = 0;
    }
    if (curSample < file->currentSample)
      CommonExit(1,"AudioSeek: error seeking backwards in output file");
    if (curSample > file->currentSample) {
      /* seek forward, fill skipped region with silence */
      memset(buf,0,SAMPLE_BUF_SIZE*sizeof(float));
      tot = file->numChannel*(curSample-file->currentSample);
      cur = 0;
      while (cur < tot) {
	num = min(tot-cur,SAMPLE_BUF_SIZE);
	AFwriteData(file->file,buf,num);
	cur += num;
      }
      file->currentSample = curSample;
    }
  }
}


/* AudioClose() */
/* Close audio file.*/

void AudioClose (
  AudioFile *file)		/* in: audio file (handle) */
{
  if (AUdebugLevel >= 1)
    printf("AudioClose:\n");

  AFclose(file->file);
  free(file);
}


/* end of audio.c */

