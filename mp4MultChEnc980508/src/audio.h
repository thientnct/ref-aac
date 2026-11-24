/**********************************************************************
MPEG-4 Audio VM
Audio i/o module



SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by
  -

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
Copyright (c) ISO/IEC 1996, 1999.



Header file: audio.h

 

Required libraries:
libtsp.a		AFsp audio file library

Required modules:
common.o		common module
austream.o		audio i/o streams (.au format)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
21-jan-96   HP    born (using AFsp-V2R2)
19-feb-97   HP    made internal data structures invisible
30-dec-98   HP    uses austream for stdin/stdout, evaluates USE_AFSP
07-jan-99   HP    AFsp-v4r1 (AFsp-V3R2 still supported)
**********************************************************************/


/**********************************************************************

The audio i/o module provides an interface to the basic functions for
PCM audio data stream input and output.

The current implementation of the audio i/o module is based on the
AFsp audio library.  If USE_AFSP is undefined, only 16 bit .au files
are supported using the austream module. Audio i/o on stdin/stdout
(only 16 bit .au) is also based on the austream module.

Other implementations of this module are possible as long as the basic
functions required by the MPEG-4 Audio VM framework are provided.
Optional functions could be substituted by dummy functions and
optional parameters could be ignored.  The VM framework uses only
sequential access to the input and output audio data streams.  The
seek function is only use for initial compensation of the coding delay
in the encoder and decoder.

Basic functions and parameters required by the VM framework:

AudioOpenRead():	fileName, numChannel, fSample
AudioOpenWrite():	fileName, numChannel, fSample
AudioReadData():	file, data, numSample
AudioWriteData():	file, data, numSample
AudioClose():		file

NOTE: For multi channel audio files, the number of samples per channel
      (numSample) is used as parameter!!!  The total number of samples
      (numSample*numChannel) is not used here.

**********************************************************************/


#ifndef _audio_h_
#define _audio_h_


/* ---------- declarations ---------- */

typedef struct AudioFileStruct AudioFile;	/* audio file handle */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* AudioInit() */
/* Init audio i/o module. */
/* formatString options: see AFsp documentation */

void AudioInit (
  char *formatString,		/* in: file format for headerless files */
  int debugLevel);		/* in: debug level */
				/*     0=off  1=basic  2=full */


/* AudioOpenRead() */
/* Open audio file for reading. */

AudioFile *AudioOpenRead (
  char *fileName,		/* in: file name */
				/*     "-": stdin (only 16 bit .au) */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample);		/* out: number of samples in file */
				/*      (samples per channel!) */
				/*      or 0 if not available */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */


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
				/*     "-": stdout (only 16 bit .au) */
  char *format,			/* in: file format (ignored if stdout) */
				/*     (au, snd, wav, aif, raw) */
  int numChannel,		/* in: number of channels */
  float fSample);		/* in: sampling frequency [Hz] */
				/* returns: */
				/*  audio file (handle) */
				/*  or NULL if error */


/* AudioReadData() */
/* Read data from audio file. */
/* Requested samples that could not be read from the file are set to 0. */

long AudioReadData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* out: data[channel][sample] */
				/*      (range [-32768 .. 32767]) */
  long numSample);		/* in: number of samples to be read */
				/*     (samples per channel!) */
				/* returns: */
				/*  number of samples read */
				/*  (samples per channel!) */


/* AudioWriteData() */
/* Write data to audio file. */

void AudioWriteData (
  AudioFile *file,		/* in: audio file (handle) */
  float **data,			/* in: data[channel][sample] */
				/*     (range [-32768 .. 32767]) */
  long numSample);		/* in: number of samples to be written */
				/*     (samples per channel!) */


/* AudioSeek() */
/* Set position in audio file to curSample. */
/* (Beginning of file: curSample=0) */
/* NOTE: It is not possible to seek backwards in a output file if */
/*       any samples were already written to the file. */

void AudioSeek (
  AudioFile *file,		/* in: audio file (handle) */
  long curSample);		/* in: new position [samples] */
				/*     (samples per channel!) */


/* AudioClose() */
/* Close audio file.*/

void AudioClose (
  AudioFile *file);		/* in: audio file (handle) */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _audio_h_ */

/* end of audio.h */

