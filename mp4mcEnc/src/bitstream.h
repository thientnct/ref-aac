/**********************************************************************
MPEG-4 Audio VM
Bit stream module



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
Copyright (c) ISO/IEC 1996.



Header file: bitstream.h

$Id: bitstream.h,v 1.1.1.1 1998/06/09 01:05:51 lueck Exp $

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
04-jun-96   HP    added buffers, ASCII-header and BsGetBufferAhead()
07-jun-96   HP    use CommonWarning(), CommonExit()
10-jun-96   HP    ...
13-jun-96   HP    changed BsGetBit(), BsPutBit(), BsGetBuffer(),
                  BsGetBufferAhead(), BsPutBuffer()
26-aug-96   HP    CVS
23-oct-96   HP    free for BsOpenFileRead() info string
15-nov-96   HP    changed int to long where required
                  added BsGetBitChar(), BsGetBitShort(), BsGetBitInt()
                  improved file header handling
23-dec-96   HP    renamed mode to write in BsBitStream
10-jan-97   HP    added BsGetBitAhead(), BsGetSkip()
19-feb-97   HP    made internal data structures invisible
**********************************************************************/


/**********************************************************************

Bit data is stored LSB-justified and transmitted MSB first.

Example: 5 bit stored in unsigned long
	 ("1st" is the first bit transmitted, "x" indicates unused bit)

	 x  ...  x  1st 2nd 3rd 4th 5th
	 MSB                         LSB

Headerless bit stream files contain N*8 bits, where N is the file size in
bytes. The last byte is padded with 0s if necessary.

Example: bit stream file containing 13 bits (file size: 2 bytes)

	 1st 2nd 3rd 4th 5th 6th 7th 8th   9th 10th 11th 12th 13th 0 0 0
	 MSB ....... 1st byte ...... LSB   MSB ...... 2nd byte ...... LSB

A bit stream file can have an optional ASCII header consisting of a
magic string (e.g. ".mp4") followed by '\n' and an (optional) info string.
The info string might also contain '\n'. The ASCII header is
terminated by '\0'. If no magic string is specified when opening a
bit stream file, a file without ASCII-header is read/written.

Format of a bit stream file:
[ <magic string> [<info string> '\0'] ] <bit stream>

  <string> refers to string without terminal '\0'
  <bit stream> refers to data bytes as described above for headerless files

**********************************************************************/


#ifndef _bitstream_h_
#define _bitstream_h_


/* ---------- declarations ---------- */

#ifdef CLEANBITSTREAM

typedef struct BsBitBufferStruct BsBitBuffer;	/* bit buffer */

typedef struct BsBitStreamStruct BsBitStream; 	/* bit stream */

#else

/*
 * Although they should not do so, some functions using the bitstream
 * module still want to have access to internal variables of the
 * BsBitBuffer and BsBitStream structures ...
 * HP 970219
 */

#include <stdio.h>              /* typedef FILE */

typedef struct			/* bit buffer */
{
  unsigned char *data;		/* data bits */
  long numBit;			/* number of bits in buffer */
  long size;			/* buffer size in bits */
} BsBitBuffer;

typedef struct			/* bit stream */
{
  FILE *file;			/* file or NULL for buffer i/o */
  int write;			/* 0=read  1=write */
  long streamId;		/* stream id (for debug) */
  char *info;			/* info string (for read file) */
  BsBitBuffer *buffer[2];	/* bit buffers */
				/* (buffer[1] is only used for read file) */
  long currentBit;		/* current bit position in bit stream */
				/* (i.e. number of bits read/written) */
  long numByte;			/* number of bytes read/written (only file) */
} BsBitStream;

#endif

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* BsInit() */
/* Init bit stream module. */

void BsInit (
  long maxReadAhead,		/* in: max num bits to be read ahead */
				/*     (determines read file buffer size) */
  int debugLevel);		/* in: debug level */
				/*     0=off  1=basic  2=medium  3=full */


/* BsOpenFileRead() */
/* Open bit stream file for reading. */

BsBitStream *BsOpenFileRead (
  char *fileName,		/* in: file name */
  char *magic,			/* in: expected magic string */
				/*     or NULL (no ASCII header in file) */
  char **info);			/* out: info string */
				/*      or NULL (no info string in file) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */


/* BsOpenFileWrite() */
/* Open bit stream file for writing. */

BsBitStream *BsOpenFileWrite (
  char *fileName,		/* in: file name */
  char *magic,			/* in: magic string */
				/*     or NULL (no ASCII header) */
  const char *info);		/* in: info string */
				/*     or NULL (no info string) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */


/* BsOpenBufferRead() */
/* Open bit stream buffer for reading. */

BsBitStream *BsOpenBufferRead (
  BsBitBuffer *buffer);		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */


/* BsOpenBufferWrite() */
/* Open bit stream buffer for writing. */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

BsBitStream *BsOpenBufferWrite (
  BsBitBuffer *buffer);		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */


/* BsCurrentBit() */
/* Get number of bits read/written from/to stream. */

long BsCurrentBit (
  BsBitStream *stream);		/* in: bit stream */
				/* returns: */
				/*  number of bits read/written */


/* BsEof() */
/* Test if end of bit stream file occurs within the next numBit bits. */

int BsEof (
  BsBitStream *stream,		/* in: bit stream */
  long numBit);			/* in: number of bits ahead scanned for EOF */
				/* returns: */
				/*  0=not EOF  1=EOF */


/* BsClose() */
/* Close bit stream. */

int BsClose (
  BsBitStream *stream);		/* in: bit stream */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBit() */
/* Read bits from bit stream. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitAhead() */
/* Read bits from bit stream. */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAhead (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitAheadChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitAheadShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBitAheadInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit);			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBuffer() */
/* Read bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit);			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetBufferAhead() */
/* Read bits ahead of current position from bit stream to buffer. */
/* (Current position in bit stream is NOT advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBufferAhead (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit);			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */


/* BsGetSkip() */
/* Skip bits in bit stream (read). */
/* (Current position in bit stream is advanced !!!) */

int BsGetSkip (
  BsBitStream *stream,		/* in: bit stream */
  long numBit);			/* in: num bits to skip */
				/* returns: */
				/*  0=OK  1=error */


/* BsPutBit() */
/* Write bits to bit stream. */

int BsPutBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long data,		/* in: bits to write */
  int numBit);			/* in: num bits to write */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */


/* BsPutBuffer() */
/* Write bits from buffer to bit stream. */

int BsPutBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer);		/* in: buffer to write */
				/* returns: */
				/*  0=OK  1=error */


/* BsAllocBuffer() */
/* Allocate bit buffer. */

BsBitBuffer *BsAllocBuffer (
  long numBit);			/* in: buffer size in bits */
				/* returns: */
				/*  bit buffer (handle) */


/* BsFreeBuffer() */
/* Free bit buffer. */

void BsFreeBuffer (
  BsBitBuffer *buffer);		/* in: bit buffer */


/* BsBufferNumBit() */
/* Get number of bits in buffer. */

long BsBufferNumBit (
  BsBitBuffer *buffer);		/* in: bit buffer */
				/* returns: */
				/*  number of bits in buffer */


/* BsClearBuffer() */
/* Clear bit buffer (set numBit to 0). */

void BsClearBuffer (
  BsBitBuffer *buffer);		/* in: bit buffer */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _bitstream_h_ */

/* end of bitstrm.h */
