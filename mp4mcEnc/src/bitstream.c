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



Source file: bitstream.c

$Id: bitstream.c,v 1.1.1.1 1998/06/09 01:05:51 lueck Exp $

Required modules:
common.o		common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-jun-96   HP    added buffers, ASCII-header and BsGetBufferAhead()
07-jun-96   HP    use CommonWarning(), CommonExit()
11-jun-96   HP    ...
13-jun-96   HP    changed BsGetBit(), BsPutBit(), BsGetBuffer(),
                  BsGetBufferAhead(), BsPutBuffer()
14-jun-96   HP    fixed bug in BsOpenFileRead(), read header
18-jun-96   HP    fixed bug in BsGetBuffer()
26-aug-96   HP    CVS
23-oct-96   HP    free for BsOpenFileRead() info string
07-nov-96   HP    fixed BsBitStream info bug
15-nov-96   HP    changed int to long where required
                  added BsGetBitChar(), BsGetBitShort(), BsGetBitInt()
                  improved file header handling
04-dec-96   HP    fixed bug in BsGetBitXxx() for numBit==0		  
23-dec-96   HP    renamed mode to write in BsBitStream
10-jan-97   HP    added BsGetBitAhead(), BsGetSkip()
17-jan-97   HP    fixed read file buffer bug
30-jan-97   HP    minor bug fix in read bitstream file magic string
19-feb-97   HP    made internal data structures invisible
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"		/* bit stream module */
#include "common.h"		/* common module */


/* ---------- declarations ---------- */

#define MIN_FILE_BUF_SIZE 1024	/* min num bytes in file buffer */
#define HEADER_BUF_SIZE 2048	/* max size of ASCII header */

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define BYTE_NUMBIT 8		/* bits in byte (char) */
#define LONG_NUMBIT 32		/* bits in unsigned long */
#define bit2byte(a) (((a)+BYTE_NUMBIT-1)/BYTE_NUMBIT)
#define byte2bit(a) ((a)*BYTE_NUMBIT)


/* ---------- declarations (structures) ---------- */

#ifdef CLEANBITSTREAM

/* see bitstream.h for details   HP 970219 */

struct BsBitBufferStruct	/* bit buffer */
{
  unsigned char *data;		/* data bits */
  long numBit;			/* number of bits in buffer */
  long size;			/* buffer size in bits */
};

struct BsBitStreamStruct	/* bit stream */
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
};

#endif

/* ---------- variables ---------- */

static int BSdebugLevel = 4;			/* debug level */
static long BSbufSizeByte = MIN_FILE_BUF_SIZE;	/* num bytes file buf */
static long BSstreamId = 0;			/* stream id counter */


/* ---------- internal functions ---------- */


/* BsReadFile() */
/* Read one buffer from file. */

static int BsReadFile (
  BsBitStream *stream)		/* in: stream */
				/* returns: 0=OK  1=error */
{
  long numByte;
  long numByteRead;
  long curBuf;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
	   stream->streamId,stream->numByte,stream->currentBit);

  if (feof(stream->file))
    return 0;

  numByte = bit2byte(stream->buffer[0]->size);
  if (stream->numByte % numByte != 0) {
    CommonWarning("BsReadFile: bit stream buffer error");
    return 1;
  }
  curBuf = (stream->numByte / numByte) % 2;
  numByteRead = fread(stream->buffer[curBuf]->data,sizeof(char),numByte,
		      stream->file);
  if (ferror(stream->file)) {
    CommonWarning("BsReadFile: error reading bit stream file");
    return 1;
  }
  stream->numByte += numByteRead;

  if (BSdebugLevel >= 3)
    printf("BsReadFile: numByte=%ld  numByteRead=%ld\n",numByte,numByteRead);

  return 0;
}


/* BsWriteFile() */
/* Write one buffer to file. */

static int BsWriteFile (
  BsBitStream *stream)		/* in: stream */
				/* returns: 0=OK  1=error */
{
  long numByte;
  long numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: id=%ld  streamNumByte=%ld  curBit=%ld\n",
	   stream->streamId,stream->numByte,stream->currentBit);

  if (stream->numByte % bit2byte(stream->buffer[0]->size) != 0) {
    CommonWarning("BsWriteFile: bit stream buffer error");
    return 1;
  }
  numByte = bit2byte(stream->currentBit) - stream->numByte;
  numByteWritten = fwrite(stream->buffer[0]->data,sizeof(char),numByte,
			  stream->file);
  if (numByteWritten != numByte || ferror(stream->file)) {
    CommonWarning("BsWriteFile: error writing bit stream file");
    return 1;
  }
  stream->numByte += numByteWritten;

  if (BSdebugLevel >= 3)
    printf("BsWriteFile: numByteWritten=%ld\n",numByteWritten);

  return 0;
}


/* BsReadAhead() */
/* If possible, ensure that the next numBit bits are available */
/* in read file buffer. */

static int BsReadAhead (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits */
				/* returns: 0=OK  1=error */
{
  if (stream->write==1 || stream->file==NULL)
    return 0;

  if (bit2byte(stream->currentBit+numBit) > stream->numByte)
    if (BsReadFile(stream)) {
      CommonWarning("BsReadAhead: error reading bit stream file");
      return 1;
    }
  return 0;
}


/* BsCheckRead() */
/* Check if numBit bits could be read from stream. */

static int BsCheckRead (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits */
				/* returns: */
				/*  0=numBit bits could be read */
				/*    (or write mode) */
				/*  1=numBit bits could not be read */
{
  if (stream->write == 1)
    return 0;		/* write mode */
  else
    if (stream->file == NULL)
      return (stream->currentBit+numBit > stream->buffer[0]->numBit) ? 1 : 0;
    else
      return (bit2byte(stream->currentBit+numBit) > stream->numByte) ? 1 : 0;
}


/* BsReadByte() */
/* Read byte from stream. */

static int BsReadByte (
  BsBitStream *stream,		/* in: stream */
  unsigned long *data,		/* out: data (max 8 bit, right justified) */
  int numBit)			/* in: num bits [1..8] */
				/* returns: 0=OK  1=error */
{
  long numUsed,idx,buf;

  if (stream->file!=NULL && stream->currentBit==stream->numByte*BYTE_NUMBIT)
    if (BsReadFile(stream)) {
      CommonWarning("BsReadByte: error reading bit stream file");
      return 1;
    }

  if (BsCheckRead(stream,numBit)) {
    CommonWarning("BsReadByte: not enough bits left in stream");
    return 1;
  }

  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  buf = (stream->currentBit / BYTE_NUMBIT /
	 bit2byte(stream->buffer[0]->size)) % 2;
  numUsed = stream->currentBit % BYTE_NUMBIT;
  *data = (stream->buffer[buf]->data[idx] >> (BYTE_NUMBIT-numUsed-numBit)) &
    ((1<<numBit)-1);
  stream->currentBit += numBit;
  return 0;
}


/* BsWriteByte() */
/* Write byte to stream. */

static int BsWriteByte (
  BsBitStream *stream,		/* in: stream */
  unsigned long data,		/* in: data (max 8 bit, right justified) */
  int numBit)			/* in: num bits [1..8] */
				/* returns: 0=OK  1=error */
{
  long numUsed,idx;

  if (stream->file == NULL &&
      stream->buffer[0]->numBit+numBit > stream->buffer[0]->size) {
    CommonWarning("BsWriteByte: not enough bits left in buffer");
    return 1;
  }
  idx = (stream->currentBit / BYTE_NUMBIT) % bit2byte(stream->buffer[0]->size);
  numUsed = stream->currentBit % BYTE_NUMBIT;
  if (numUsed == 0)
    stream->buffer[0]->data[idx] = 0;
  stream->buffer[0]->data[idx] |= (data & ((1<<numBit)-1)) <<
    (BYTE_NUMBIT-numUsed-numBit);
  stream->currentBit += numBit;
  if (stream->file==NULL)
    stream->buffer[0]->numBit = stream->currentBit;
  if (stream->file!=NULL && stream->currentBit%stream->buffer[0]->size==0)
    if (BsWriteFile(stream)) {
      CommonWarning("BsWriteByte: error writing bit stream file");
      return 1;
    }
  return 0;
}


/* ---------- functions ---------- */


/* BsInit() */
/* Init bit stream module. */

void BsInit (
  long maxReadAhead,		/* in: max num bits to be read ahead */
				/*     (determines read file buffer size) */
  int debugLevel)		/* in: debug level */
				/*     0=off  1=basic  2=medium  3=full */
{
  BSbufSizeByte = max(MIN_FILE_BUF_SIZE,bit2byte(maxReadAhead));
  BSdebugLevel = debugLevel;
  if (BSdebugLevel >= 1)
    printf("BsInit: debugLevel=%d  bufSizeByte=%ld\n",
	   BSdebugLevel,BSbufSizeByte);
}


/* BsOpenFileRead() */
/* Open bit stream file for reading. */

BsBitStream *BsOpenFileRead (
  char *fileName,		/* in: file name */
  char *magic,			/* in: magic string */
				/*     or NULL (no ASCII header in file) */
  char **info)			/* out: info string */
				/*      or NULL (no info string in file) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */
{
  BsBitStream *stream;
  char header[HEADER_BUF_SIZE];
  int tmp = 0;
  long i,len;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileRead: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
	   fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL)
      printf("magic=\"%s\"\n",magic);
    else
      printf("no header\n");
  }

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenFileRead: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));
  stream->buffer[1]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;
 
  if ((stream->file=fopen(fileName,"rb")) == NULL) {
    CommonWarning("BsOpenFileRead: error opening bit stream file %s",
		  fileName);
    BsFreeBuffer(stream->buffer[0]);
    BsFreeBuffer(stream->buffer[1]);
    free(stream);
    return NULL;
  }

  if (magic != NULL) {
    /* read ASCII header */
    /* read magic string */
    len = strlen(magic);
    if (len >= HEADER_BUF_SIZE) {
      CommonWarning("BsOpenFileRead: magic string too long");
      BsClose(stream);
      return NULL;
    }
    for (i=0; i<len; i++)
      header[i]=tmp=fgetc(stream->file);
      if (tmp == EOF) {			/* EOF requires int (not char) */
	CommonWarning("BsOpenFileRead: "
		      "error reading bit stream file (header)");
	BsClose(stream);
	return NULL;
      }
    header[i] = '\0';

    if (strcmp(header,magic) != 0) {
      CommonWarning("BsOpenFileRead: magic string error "
		    "(found \"%s\", need \"%s\")",header,magic);
      BsClose(stream);
      return NULL;
    }
    
    if (info != NULL) {
      /* read info string */
      i = 0;
      while ((header[i]=tmp=fgetc(stream->file)) != '\0') {
	if (tmp == EOF) {		/* EOF requires int (not char) */
	  CommonWarning("BsOpenFileRead: "
			"error reading bit stream file (header)");
	  BsClose(stream);
	  return NULL;
	}
	i++;
	if (i >= HEADER_BUF_SIZE) {
	  CommonWarning("BsOpenFileRead: info string too long");
	  BsClose(stream);
	  return NULL;
	}
      }

      if (BSdebugLevel >= 1)
	printf("BsOpenFileRead: info=\"%s\"\n",header);

      if ((stream->info=(char*)malloc((strlen(header)+1)*sizeof(char)))
	  == NULL)
	CommonExit(1,"BsOpenFileRead: memory allocation error (info)");
      strcpy(stream->info,header);
      *info = stream->info;
    }
  }

  /* init buffer */
  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}


/* BsOpenFileWrite() */
/* Open bit stream file for writing. */

BsBitStream *BsOpenFileWrite (
  char *fileName,		/* in: file name */
  char *magic,			/* in: magic string */
				/*     or NULL (no ASCII header) */
  const char *info)		/* in: info string */
				/*     or NULL (no info string) */
				/* returns: */
				/*  bit stream (handle) */
				/*  or NULL if error */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 1) {
    printf("BsOpenFileWrite: fileName=\"%s\"  id=%ld  bufSize=%ld  ",
	   fileName,BSstreamId,byte2bit(BSbufSizeByte));
    if (magic != NULL) {
      printf("magic=\"%s\"\n",magic);
      if (info != NULL)
	printf("BsOpenFileWrite: info=\"%s\"\n",info);
      else
	printf("BsOpenFileWrite: no info\n");
    }
    else
      printf("no header\n");
  }

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenFileWrite: memory allocation error (stream)");
  stream->buffer[0]=BsAllocBuffer(byte2bit(BSbufSizeByte));

  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  if ((stream->file=fopen(fileName,"wb")) == NULL) {
    CommonWarning("BsOpenFileWrite: error opening bit stream file %s",
		  fileName);
    BsFreeBuffer(stream->buffer[0]);
    free(stream);
    return NULL;
  }

  if (magic!=NULL) {
    /* write ASCII header */
    /* write magic string */
    if (fputs(magic,stream->file) == EOF) {
      CommonWarning("BsOpenFileWrite: error writing bit stream file (header)");
      BsClose(stream);
      return NULL;
    }
    if (info!=NULL) {
      /* write info string */
      if (fputs(info,stream->file) == EOF) {
	CommonWarning("BsOpenFileWrite: "
		      "error writing bit stream file (header)");
	BsClose(stream);
	return NULL;
      }
      if (fputc('\0',stream->file) == EOF) {
	CommonWarning("BsOpenFileWrite: "
		      "error writing bit stream file (header)");
	BsClose(stream);
	return NULL;
      }
    }
  }

  stream->currentBit = 0;
  stream->numByte = 0;

  return stream;
}


/* BsOpenBufferRead() */
/* Open bit stream buffer for reading. */

BsBitStream *BsOpenBufferRead (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferRead: id=%ld  bufNumBit=%ld  bufSize=%ld  "
	   "bufAddr=0x%lx\n",
	   BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenBufferRead: memory allocation error");

  stream->file = NULL;
  stream->write = 0;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  stream->currentBit = 0;
  
  return stream;
}


/* BsOpenBufferWrite() */
/* Open bit stream buffer for writing. */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

BsBitStream *BsOpenBufferWrite (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  bit stream (handle) */
{
  BsBitStream *stream;

  if (BSdebugLevel >= 2)
    printf("BsOpenBufferWrite: id=%ld  bufNumBit=%ld  bufSize=%ld  "
	   "bufAddr=0x%lx\n",
	   BSstreamId,buffer->numBit,buffer->size,(unsigned long)buffer);

  if ((stream=(BsBitStream*)malloc(sizeof(BsBitStream))) == NULL)
    CommonExit(1,"BsOpenBufferWrite: memory allocation error");

  stream->file = NULL;
  stream->write = 1;
  stream->streamId = BSstreamId++;
  stream->info = NULL;

  stream->buffer[0] = buffer;
  BsClearBuffer(buffer);
  stream->currentBit = 0;
  
  return stream;
}


/* BsCurrentBit() */
/* Get number of bits read/written from/to stream. */

long BsCurrentBit (
  BsBitStream *stream)		/* in: bit stream */
				/* returns: */
				/*  number of bits read/written */
{
  return stream->currentBit;
}


/* BsEof() */
/* Test if end of bit stream file occurs within the next numBit bits. */

int BsEof (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: number of bits ahead scanned for EOF */
				/* returns: */
				/*  0=not EOF  1=EOF */
{
  int eof;

  if (BSdebugLevel >= 2)
    printf("BsEof: %s  id=%ld  curBit=%ld  numBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,stream->currentBit,numBit);

  if (stream->file != NULL && numBit > stream->buffer[0]->size)
    CommonExit(1,"BsEof: "
	       "number of bits to look ahead too high (%ld)",numBit);

  if (BsReadAhead(stream,numBit+1)) {
    CommonWarning("BsEof: error reading bit stream");
    eof = 0;
  }
  else
    eof = BsCheckRead(stream,numBit+1);

  if (BSdebugLevel >= 2)
    printf("BsEof: eof=%d\n",eof);

  return eof;
}


/* BsClose() */
/* Close bit stream. */

int BsClose (
  BsBitStream *stream)		/* in: bit stream */
				/* returns: */
				/*  0=OK  1=error */
{
  int returnFlag = 0;

  if (((stream->file != NULL) && (BSdebugLevel >= 1)) || (BSdebugLevel >= 2))
    printf("BsClose: %s  %s  id=%ld  curBit=%ld\n",
	   (stream->write)?"write":"read",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,stream->currentBit);

  if (stream->file != NULL) {
    if (stream->write == 1)
      /* flush buffer to file */
      if (BsWriteFile(stream)) {
	CommonWarning("BsClose: error writing bit stream");
	returnFlag = 1;
      }

    BsFreeBuffer(stream->buffer[0]);
    if (stream->write == 0)
      BsFreeBuffer(stream->buffer[1]);

    if (fclose(stream->file)) {
      CommonWarning("BsClose: error closing bit stream file");
      returnFlag = 1;
    }
  }

  if (stream->info != NULL)
    free(stream->info);
  free(stream);
  return returnFlag;
}


/* BsGetBit() */
/* Read bits from bit stream. */
/* (Current position in bit stream is advanced !!!) */

int BsGetBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  int num,maxNum,curNum;
  unsigned long bits;

  if (BSdebugLevel >= 3)
    printf("BsGetBit: %s  id=%ld  numBit=%d  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit,stream->currentBit);

  if (stream->write != 0)
    CommonExit(1,"BsGetBit: stream not in read mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsGetBit: number of bits out of range (%d)",numBit);

  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;

  /* read bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    if (BsReadByte(stream,&bits,curNum)) {
      CommonWarning("BsGetBit: error reading bit stream");
      return 1;
    }
    *data |= bits<<(numBit-num-curNum);
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  if (BSdebugLevel >= 3)
    printf("BsGetBit: data=0x%lx\n",*data);

  return 0;
}


/* BsGetBitChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 8)
    CommonExit(1,"BsGetBitChar: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitShort: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is advanced !!!) */

int BsGetBitInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  unsigned long ultmp;
  int result;

  if (numBit > 16)
    CommonExit(1,"BsGetBitInt: number of bits out of range (%d)",numBit);
  if (data != NULL)
    *data = 0;
  if (numBit == 0)
    return 0;
  result = BsGetBit(stream,&ultmp,numBit);
  *data = ultmp;
  return result;
}


/* BsGetBitAhead() */
/* Read bits from bit stream. */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAhead (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAhead: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBit(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAhead: error reading bit stream");

  return result;
}


/* BsGetBitAheadChar() */
/* Read bits from bit stream (char). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadChar (
  BsBitStream *stream,		/* in: bit stream */
  unsigned char *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..8] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadChar: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitChar(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadChar: error reading bit stream");

  return result;
}


/* BsGetBitAheadShort() */
/* Read bits from bit stream (short). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadShort (
  BsBitStream *stream,		/* in: bit stream */
  unsigned short *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadShort: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitShort(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadShort: error reading bit stream");

  return result;
}


/* BsGetBitAheadInt() */
/* Read bits from bit stream (int). */
/* (Current position in bit stream is NOT advanced !!!) */

int BsGetBitAheadInt (
  BsBitStream *stream,		/* in: bit stream */
  unsigned int *data,		/* out: bits read */
				/*      (may be NULL if numBit==0) */
  int numBit)			/* in: num bits to read */
				/*     [0..16] */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 3)
    printf("BsGetBitAheadInt: %s  id=%ld  numBit=%d\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBitInt(stream,data,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBitAheadInt: error reading bit stream");

  return result;
}


/* BsGetBuffer() */
/* Read bits from bit stream to buffer. */
/* (Current position in bit stream is advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit)			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */
{
  long i,numByte,numRemain;
  unsigned long data;

  if (BSdebugLevel >= 2) {
    printf("BsGetBuffer: %s  id=%ld  numBit=%ld  ",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit);
      if (buffer != NULL)
	printf("bufSize=%ld  bufAddr=0x%lx  ",
	       buffer->size,(unsigned long)buffer);
      else
	printf("(bufAddr=(NULL)  ");
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetBuffer: stream not in read mode");

  if (numBit == 0)
    return 0;

  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsGetBuffer: can not get buffer from itself");
  if (numBit < 0 || numBit > buffer->size)
    CommonExit(1,"BsGetBuffer: number of bits out of range (%ld)",numBit);

  BsClearBuffer(buffer);

  numByte = bit2byte(numBit)-1;
  for (i=0; i<numByte; i++) {
    if (BsGetBit(stream,&data,BYTE_NUMBIT)) {
      CommonWarning("BsGetBuffer: error reading bit stream");
      buffer->numBit = i*BYTE_NUMBIT;
      return 1;
    }
    buffer->data[i] = data;
  }
  numRemain = numBit-numByte*BYTE_NUMBIT;
  if (BsGetBit(stream,&data,numRemain)) {
    CommonWarning("BsGetBuffer: error reading bit stream");
    buffer->numBit = numByte*BYTE_NUMBIT;
    return 1;
  }
  buffer->data[i] = data<<(BYTE_NUMBIT-numRemain);
  buffer->numBit = numBit;

  return 0;
}


/* BsGetBufferAhead() */
/* Read bits ahead of current position from bit stream to buffer. */
/* (Current position in bit stream is NOT advanced !!!) */
/* (Buffer is cleared first - previous data in buffer is lost !!!) */

int BsGetBufferAhead (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer,		/* out: buffer read */
				/*      (may be NULL if numBit==0) */
  long numBit)			/* in: num bits to read */
				/* returns: */
				/*  0=OK  1=error */
{
  long oldCurrentBit;
  int result;

  if (BSdebugLevel >= 2)
    printf("BsGetBufferAhead: %s  id=%ld  numBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",stream->streamId,numBit);

  if (numBit > stream->buffer[0]->size)
    CommonExit(1,"BsGetBufferAhead: "
	       "number of bits to look ahead too high (%ld)",numBit);

  oldCurrentBit = stream->currentBit;
  result = BsGetBuffer(stream,buffer,numBit);
  stream->currentBit = oldCurrentBit;
  if (result)
    CommonWarning("BsGetBufferAhead: error reading bit stream");

  return result;
}


/* BsGetSkip() */
/* Skip bits in bit stream (read). */
/* (Current position in bit stream is advanced !!!) */

int BsGetSkip (
  BsBitStream *stream,		/* in: bit stream */
  long numBit)			/* in: num bits to skip */
				/* returns: */
				/*  0=OK  1=error */
{
  if (BSdebugLevel >= 2) {
    printf("BsGetSkip: %s  id=%ld  numBit=%ld  ",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit);
    printf("curBit=%ld\n",stream->currentBit);
  }

  if (stream->write != 0)
    CommonExit(1,"BsGetSkip: stream not in read mode");
  if (numBit < 0)
    CommonExit(1,"BsGetSkip: number of bits out of range (%ld)",numBit);

  if (numBit == 0)
    return 0;

  if (BsReadAhead(stream,numBit) || BsCheckRead(stream,numBit)) {
    CommonWarning("BsGetSkip: error reading bit stream");
    return 1;
  }
  stream->currentBit += numBit;
  return 0;
}


/* BsPutBit() */
/* Write bits to bit stream. */

int BsPutBit (
  BsBitStream *stream,		/* in: bit stream */
  unsigned long data,		/* in: bits to write */
  int numBit)			/* in: num bits to write */
				/*     [0..32] */
				/* returns: */
				/*  0=OK  1=error */
{
  int num,maxNum,curNum;
  unsigned long bits;

  if (BSdebugLevel >= 1)
    printf("BsPutBit: %s  id=%ld  numBit=%d  data=0x%lx  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,numBit,data,stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBit: stream not in write mode");
  if (numBit<0 || numBit>LONG_NUMBIT)
    CommonExit(1,"BsPutBit: number of bits out of range (%d)",numBit);
  if (numBit < 32 && data > ((unsigned long)1<<numBit)-1)
    CommonExit(1,"BsPutBit: data requires more than %d bits (0x%lx)",
	       numBit,data);

  if (numBit == 0)
    return 0;

  /* write bits in packets according to buffer byte boundaries */
  num = 0;
  maxNum = BYTE_NUMBIT - stream->currentBit % BYTE_NUMBIT;
  while (num < numBit) {
    curNum = min(numBit-num,maxNum);
    bits = data>>(numBit-num-curNum);
    if (BsWriteByte(stream,bits,curNum)) {
      CommonWarning("BsPutBit: error writing bit stream");
      return 1;
    }
    num += curNum;
    maxNum = BYTE_NUMBIT;
  }

  return 0;
}


/* BsPutBuffer() */
/* Write bits from buffer to bit stream. */

int BsPutBuffer (
  BsBitStream *stream,		/* in: bit stream */
  BsBitBuffer *buffer)		/* in: buffer to write */
				/* returns: */
				/*  0=OK  1=error */
{
  long i,numByte,numRemain;

  if (buffer->numBit == 0)
    return 0;

  if (BSdebugLevel >= 1)
    printf("BsPutBuffer: %s  id=%ld  numBit=%ld  bufAddr=0x%lx  curBit=%ld\n",
	   (stream->file!=NULL)?"file":"buffer",
	   stream->streamId,buffer->numBit,(unsigned long)buffer,
	   stream->currentBit);

  if (stream->write != 1)
    CommonExit(1,"BsPutBuffer: stream not in write mode");
  if (stream->buffer[0] == buffer)
    CommonExit(1,"BsPutBuffer: can not put buffer into itself");

  numByte = bit2byte(buffer->numBit)-1;
  for (i=0; i<numByte; i++)
    if (BsPutBit(stream,buffer->data[i],BYTE_NUMBIT)) {
      CommonWarning("BsPutBuffer: error writing bit stream");
      return 1;
    }
  numRemain = buffer->numBit-numByte*BYTE_NUMBIT;
  if (BsPutBit(stream,buffer->data[i]>>(BYTE_NUMBIT-numRemain),numRemain)) {
    CommonWarning("BsPutBuffer: error reading bit stream");
    return 1;
  }

  return 0;
}


/* BsAllocBuffer() */
/* Allocate bit buffer. */

BsBitBuffer *BsAllocBuffer (
  long numBit)			/* in: buffer size in bits */
				/* returns: */
				/*  bit buffer (handle) */
{
  BsBitBuffer *buffer;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: size=%ld\n",numBit);

  if ((buffer=(BsBitBuffer*)malloc(sizeof(BsBitBuffer))) == NULL)
    CommonExit(1,"BsAllocBuffer: memory allocation error (buffer)");
  if ((buffer->data=(unsigned char*)malloc(bit2byte(numBit)*sizeof(char)))
      == NULL)
    CommonExit(1,"BsAllocBuffer: memory allocation error (data)");
  buffer->numBit = 0;
  buffer->size = numBit;

  if (BSdebugLevel >= 2)
    printf("BsAllocBuffer: bufAddr=0x%lx\n",(unsigned long)buffer);

  return buffer;
}


/* BsFreeBuffer() */
/* Free bit buffer. */

void BsFreeBuffer (
  BsBitBuffer *buffer)		/* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsFreeBuffer: size=%ld  bufAddr=0x%lx\n",
	   buffer->size,(unsigned long)buffer);

  free(buffer->data);
  free(buffer);
}


/* BsBufferNumBit() */
/* Get number of bits in buffer. */

long BsBufferNumBit (
  BsBitBuffer *buffer)		/* in: bit buffer */
				/* returns: */
				/*  number of bits in buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsBufferNumBit: numBit=%ld  size=%ld  bufAddr=0x%lx\n",
	   buffer->numBit,buffer->size,(unsigned long)buffer);

  return buffer->numBit;
}


/* BsClearBuffer() */
/* Clear bit buffer (set numBit to 0). */

void BsClearBuffer (
  BsBitBuffer *buffer)		/* in: bit buffer */
{
  if (BSdebugLevel >= 2)
    printf("BsClearBuffer: size=%ld  bufAddr=0x%lx\n",
	   buffer->size,(unsigned long)buffer);

  buffer->numBit = 0;
}


/* end of bitstream.c */

