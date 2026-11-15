/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A / University of Erlangen (UER)

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
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- June-96  */

/* 28-Aug-1996  NI: added "NO_SYNCWORD" to enum TRANSPORT_STREAM */
/* 17-Spe-1997  CL: added AAC_PROFILE enum */

#ifndef _TF_MAIN_H_INCLUDED
#define _TF_MAIN_H_INCLUDED

#include "bitstream.h"
#include "block.h"

/* AAC Profile */
enum AAC_PROFILE { MAIN, LOW, SSR
#ifdef MPEG4V1
, LTP 
#endif
};

/* select different pre-/post- processing modules TK */
enum PP_MOD_SELECT { NONE=0x0, AAC_PP=0x1 };

/* select different T/F modules */
enum TF_MOD_SELECT { VM_TF_SOURCE=0x1, MDCT_AAC=0x2, MDCT_UER=0x4, QMF_MDCT_SONY=0x8, LOW_DELAY_UNH=0x10 };

/* select different Q&C modules */
enum QC_MOD_SELECT { VM_QC_SOURCE=0x1, AAC_QC=0x2, MDCT_VALUES_16BIT=0x4, UER_QC=0x8, NTT_VQ=0x10 , AAC_PRED=0x20};

/* name the audio channels */
enum CHANN_ASS { 
  MONO_CHAN=0,
  LEFT_CHAN=0, 
  RIGHT_CHAN=1
};

/* audio channel configuration coding */
enum CH_CONFIG { CHC_MONO, CHC_DUAL, CHC_JOINT_DUAL, CHC_5CHAN, CHC_MODES };

/* transport layer type */ /* added "NO_SYNCWORD" by NI (28 Aug. 1996) */
enum TRANSPORT_STREAM { NO_TSTREAM, AAC_RAWDATA_TSTREAM, LENINFO_TSTREAM,
		        NO_SYNCWORD};

enum SR_CODING { SR8000, SR11025, SR12000, SR16000, SR22050, SR24000, SR32000, SR44100, SR48000, SR64000, SR88200, SR96000, MAX_SAMPLING_RATES };
/*enum SR_CODING { SR8000, SR11025, SR12000, SR16000, SR22050, SR24000, SR32000, SR44100, SR48000, SR96000, MAX_SAMPLING_RATES };*/

enum WINDOW_TYPE { 
  ONLY_LONG_WINDOW, 
  LONG_SHORT_WINDOW, 
  ONLY_SHORT_WINDOW,
  SHORT_LONG_WINDOW,
  
  LONG_START_WINDOW,
  EIGHT_SHORT_WINDOW,
  LONG_STOP_WINDOW
};    

#ifdef MPEG4V1
#define _WINDOW_TYPE_EXT
typedef enum WINDOW_TYPE WINDOW_TYPE_EXT;
#endif

enum AAC_WINDOW_SEQUENCE { /* TK */
  ONLY_LONG_SEQUENCE = ONLY_LONG_WINDOW,
  LONG_START_SEQUENCE = LONG_SHORT_WINDOW,
  EIGHT_SHORT_SEQUENCE = ONLY_SHORT_WINDOW,
  LONG_STOP_SEQUENCE = SHORT_LONG_WINDOW
};

enum WIN_SWITCH_MODE {
  STATIC_LONG,
  STATIC_SHORT,
  LS_STARTSTOP_SEQUENCE,
  LM_STARTSTOP_SEQUENCE,
  MS_STARTSTOP_SEQUENCE,
  LONG_SHORT_SEQUENCE,
  FFT_PE_WINDOW_SWITCHING
};

#define NSFB_LONG  51
#define NSFB_SHORT 15
/*#define NSFB_LONG  49
  #define NSFB_SHORT 14*/
#define MAX_SHORT_IN_LONG_BLOCK 8

#define MAX_SHORT_WINDOWS 8

/* max. number of scale factor bands */
#define MAX_SCFAC_BANDS ((NSFB_SHORT+1)*MAX_SHORT_IN_LONG_BLOCK)


#if 0 
#pragma warning libary no more supported 
/*** T2F / F2T FUNCTION PROTOTYPE(S) ***/
int tf_make_bimodal_state_table(
   long long_block_size,
   long med_block_size,
   long short_block_size
);

void tf_free_bimodal_state_table(
   long long_block_size,
   long med_block_size,
   long short_block_size
);

void tf_buffer2freq(
   double      time_sample_vector[],
   double      spectral_line_vector[],
   double      old_input_vector[],
   long        mdct_win_state,
   BsBitStream *TfSiBistream         /* out: side information bitstream required to use this module */
);

void tf_freq2buffer(
   double time_sample_vector[],
   double spectral_line_vector[],
   double overlap_buffer[],
   long   mdct_win_state,
   BsBitStream *TfSiBistream         /* in: side information bitstream required to use this module */
);
#endif

void buffer2freq(                    /* Input: Time signal              */
  double           p_in_data[],      /* Output: MDCT cofficients        */
  double           p_out_mdct[],
  double           p_overlap[],
  enum WINDOW_TYPE block_type,
  Window_shape     wfun_select,      /* offers the possibility to select different window functions */
  int              nlong,            /* shift length for long windows   */
  int              nshort,            /* shift length for short windows  */
  Mdct_in        overlap_select      /* YT 970615 for Son_PP */
);


void freq2buffer(
  double           p_in_data[],     /* Input: MDCT coefficients                */
  double           p_out_data[],    /* Output:time domain reconstructed signal */
  double           p_overlap[],  
  enum WINDOW_TYPE block_type,
  int              nlong,           /* shift length for long windows   */
  int              nshort,          /* shift length for short windows  */
  Window_shape     wfun_select,     /* offers the possibility to select different window functions */
  Mdct_in	   overlap_select    /* select imdct output *TK*	*/
                                     /* switch (overlap_select) {       */
                                     /* case OVERLAPPED:                */
                                     /*   p_out_data[]                  */
                                     /*   = overlapped and added signal */
                      
               /*         (bufferlength: nlong)   */
                                     /* case NON_OVERLAPPED:            */
                                     /*   p_out_data[]                  */
                                     /*   = non overlapped signal       */
                                     /*         (bufferlength: 2*nlong) */
);

void imdct(double in_data[], double out_data[], int len);

void fft( double in[], double out[], int len );

#endif	/* #ifndef _TF_MAIN_H_INCLUDED */


