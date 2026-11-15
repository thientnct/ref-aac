/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Philips Digital Systems Laboratory

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

Copyright (c) 1996.

Source file: SscEnc.h


Required modules:


Authors:
RJ    Robert Janssen, Philips <robert.janssen@philips.com>

Changes:
02-may-2003   RJ    initial version
**********************************************************************/


#ifndef _SSCENC_H_
#define _SSCENC_H_

#include "PlatformTypes.h"
#include "audio.h"

#ifdef _SSC_LIB_BUILD
#include "SscEncP.h"
#else
typedef struct tagProcessInfo PROCESSINFO;
typedef struct encoderinfo_tag ENCODERINFO;
typedef struct sscbunch_tag SSCBUNCH;
#endif

/* maximum buffer length in bytes, 2 stands for max SSC channels */
#define SSC_BSC_MAX_BUFFER_LENGTH  (2000*2)

typedef struct tag_CHUNK_LENGTH_TYPE
{
  UByte	Value[8];
  SInt	Position;
  SInt	Bytes;
} CHUNK_LENGTH_TYPE;

typedef struct tag_CHUNK_LENGTHS
{
  CHUNK_LENGTH_TYPE	FileLength;
  CHUNK_LENGTH_TYPE	AudioLength;
  CHUNK_LENGTH_TYPE	NrOfFrames;
} CHUNK_LENGTHS;

/* Interface file for the SSC encoder library */

char *EncSSCInfo (FILE *helpStream);   /* in: print encPara help text to helpStream    */
                                       /*     if helpStream not NULL                   */
                                       /* returns: core version string                 */

void SSC_GetEmptyEncoderInfo(ENCODERINFO *pEI, SInt nDecoder_level);

Bool EncSSCInit(int numChannel,
                float fSample,
                long fileNumSample,
                char *AudioFileName,  /* in: audio file name */
                char *BitFileName,    /* in: output file name */
                char        *encPara, /* in: encoder parameter string */
                PROCESSINFO**    ppPI,
                ENCODERINFO**    ppEI,
                SSCBUNCH** ppSSCBunch,
                int *pFrameNumSample,
                int *pDelayNumSample,
                HANDLE_BSBITBUFFER bitHeader
                );

Bool EncSSCFrame (
                  float **p_time_signal,   /* array of pointers to the modulo time 
                                              signal input buffers for each channel */
                  HANDLE_BSBITBUFFER fixed_bitbuffer,             /* bits before bitres size ptr */
                  ENCODERINFO*   pEI,
                  PROCESSINFO*   pPI,
                  SSCBUNCH**     ppSSCBunch);

void SSC_Encoder_Close(PROCESSINFO *pPI, CHUNK_LENGTHS * pChunkLengths);

void SSC_WriteChunkLengths(CHUNK_LENGTHS ChunkLengths, HANDLE_BSBITSTREAM stream);

void SSC_FreeEncoderInfo(ENCODERINFO *pEI);

#endif
