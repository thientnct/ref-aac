/**********************************************************************
MPEG-4 Audio VM
audio i/o streams (.au format)



SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Heiko Purnhagen (University of Hannover)

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
Copyright (c) ISO/IEC 1999.



Header file: austream.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NM    Nikolaus Meine, Uni Hannover <meine@tnt.uni-hannover.de>

Changes:
16-sep-98   HP/NM   born, based on au_io.c
**********************************************************************/

/* Audio i/o streaming with stdin/stdout support. */
/* Header format: .au (Sun audio file format, aka SND or AFsp) */
/* Sample format: 16 bit twos complement, uniform quantisation */
/* Data size set to -1 (=unknown) */

/* Multi channel data is interleaved: l0 r0 l1 r1 ... */
/* Total number of samples (over all channels) is used. */


#ifndef _austream_h_
#define _austream_h_


/* ---------- declarations ---------- */

typedef struct AuStreamStruct AuStream;	/* audio stream handle */


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* AuInit() */
/* Init audio i/o streams. */

void AuInit (
  int debugLevel);		/* in: debug level */
				/*     0=off  1=basic  2=full */


/* AuOpenRead() */
/* Open audio stream for reading. */

AuStream *AuOpenRead (
  char *streamName,		/* in: stream name, "-" for stdin */
  int *numChannel,		/* out: number of channels */
  float *fSample,		/* out: sampling frequency [Hz] */
  long *numSample);		/* out: number of samples in stream */
				/*      or -1 if not available */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */


/* AuOpenWrite() */
/* Open audio stream for writing. */

AuStream *AuOpenWrite (
  char *streamName,		/* in: stream name, "-" for stdout */
  int numChannel,		/* in: number of channels */
  float fSample);		/* in: sampling frequency [Hz] */
				/* returns: */
				/*  audio stream (handle) */
				/*  or NULL if error */


/* AuReadData() */
/* Read data from audio stream. */

long AuReadData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* out: data[] */
  long numSample);		/* in: number of samples to be read */
				/* returns: */
				/*  number of samples read */


/* AuWriteData() */
/* Write data to audio stream. */

void AuWriteData (
  AuStream *stream,		/* in: audio stream (handle) */
  short *data,			/* in: data[] */
  long numSample);		/* in: number of samples to be written */


/* AuClose() */
/* Close audio stream.*/

void AuClose (
  AuStream *stream);		/* in: audio stream (handle) */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _austream_h_ */

/* end of austream.h */

