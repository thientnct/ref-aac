/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T

and edited by
  Olaf Kähler (Fraunhofer IIS-A)

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
 *                                                                           *
 ****************************************************************************/

#ifndef _BITOUTPUT_H_
#define _BITOUTPUT_H_

#include "config.h"

typedef struct tagBitOutput *HANDLE_BITOUTPUT;

enum {
#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
  OUT_MP4FF,
#endif
#endif
  OUT_ADIF, OUT_RAW, OUT_ADTS};

/* create and open a file */
HANDLE_BITOUTPUT bitoutput_openFile(const char *filename, int format, MC_Info *mip, int channelConfig,
                                    long maxBitrate,
                                    long avgBitrate
#ifdef CT_SBR
                                    ,int sbrPresentInFillElem
                                    ,int signallingType
                                    ,int sbrPresentFlag
                                    ,int downSampledSbr
#ifdef PARAMETRICSTEREO
                                    ,int psSignallingType
                                    ,int psPresentFlag
#endif
#endif
#ifdef FRAMELENGTH_FLAG
                                    ,int frameLengthFlag
#endif
                                    );

void bitoutput_setADTSparameters(HANDLE_BITOUTPUT output, int use_crc, int rdb_per_frame
#ifdef MPEG4V1
                                 , int mpeg2
#endif
                                 );

/* elementary bit manipulations */
void bitoutput_PutBits(HANDLE_BITOUTPUT output, int length, unsigned long value);
int bitoutput_ByteAlign(HANDLE_BITOUTPUT output);
void bitoutput_RewindBits(HANDLE_BITOUTPUT output, int length);

/* overview of the elements, blocks and frames */
void bitoutput_startNewElement(HANDLE_BITOUTPUT output, int id);
int bitoutput_getBlockOverhead(HANDLE_BITOUTPUT output);
unsigned long bitoutput_getBlockBitCount(HANDLE_BITOUTPUT output);

/* add fill elements with length of bits given in count */
int bitoutput_MoveEnd(HANDLE_BITOUTPUT output, int count, int round_up);

/* finish this block, start new one */
int bitoutput_WriteBlock(HANDLE_BITOUTPUT output, double bufferFullness, int iseof);

/* close the file */
int bitoutput_close(HANDLE_BITOUTPUT output);

#endif
