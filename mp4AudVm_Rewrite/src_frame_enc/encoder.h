/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



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

*/

#ifndef _ENCODER_H_INCLUDED
#define _ENCODER_H_INCLUDED

#define MAX_CODECS 2

#include "obj_descr.h"
#include "mp4au.h"

typedef float** ENCODER_DATA_TYPE;

typedef struct tagEncoderData *HANDLE_ENCODER;
struct tagEncoderData {
  struct tagEncoderSpecificData *data;
  int (*getNumChannels)(HANDLE_ENCODER enc);
  ENCODER_DATA_TYPE (*getReconstructedTimeSignal)(HANDLE_ENCODER enc);
  enum MP4Mode (*getEncoderMode)(HANDLE_ENCODER enc);
#ifdef I2R_LOSSLESS
  int  osf;
#endif
  int ch_elm_tot; /* SAMSUNG_2005-09-30 : number of channel elements for BSAC Multichannel */
};


typedef struct EncoderMode {
  int  mode;
  char *modename;
  char* (*EncInfo)(FILE*);
  int (*EncInit)(int numChannel,        /* in: num audio channels */
                 float fSample,         /* in: sampling frequency [Hz] */
                 char *encPara,         /* in: encoder parameter string */
                 int *frameNumSample,   /* out: num samples per frame */
		 int *frameMaxNumBit,   /* out: num samples per frame */
                 DEC_CONF_DESCRIPTOR *dec_conf, /* out: decoder config */
                                                /* descriptors for each track */
                 int *numTrack,         /* out: number of tracks */
#ifdef I2R_LOSSLESS
#if 0
                 DEC_CONF_DESCRIPTOR *ll_dec_conf, /* out: ll_decoder config */
                                                /* descriptors for each SLS track */
                 int *ll_numTrack,
#endif
                 int type_PCM,
                 int osf,
#endif
                 HANDLE_ENCODER core,   /* in:  core encoder or NULL */
                 HANDLE_ENCODER enc     /* out: internal id-datastructure */
                 );

  int (*EncFrame)(ENCODER_DATA_TYPE input, /*in: input signal */
                  HANDLE_BSBITBUFFER *au,  /*in: buffers to hold output AccessUnits */
#ifdef I2R_LOSSLESS
#if 0
                  HANDLE_BSBITBUFFER *ll_au,  /*in: buffers to hold output SLS AccessUnits */
#endif
#endif
                  HANDLE_ENCODER enc,
				  float **p_time_signal_orig);/*20060107-2*/
  void (*EncFree)(HANDLE_ENCODER enc);
} EncoderMode;


typedef struct {
  int                 encoder_count;
  int                 *trackCount;
  struct EncoderMode  *enc_mode;
  HANDLE_ENCODER      *enc_data;
  DEC_CONF_DESCRIPTOR dec_conf[MAX_TRACKS];
#ifdef I2R_LOSSLESS
  DEC_CONF_DESCRIPTOR ll_dec_conf[MAX_TRACKS];
  int				  ll_trackCount;
#endif
} ENC_FRAME_DATA ;

#endif
