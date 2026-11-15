/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER)
 and edited by
 Takashi Koike (Sony Corporation)
 in the course of 
 development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- June-96  */

/* 28-Aug-1996  NI: added "NO_SYNCWORD" to enum TRANSPORT_STREAM */

#ifndef _TF_MAIN_H_INCLUDED
#define _TF_MAIN_H_INCLUDED

#include "nttNew.h"
#include "block.h"

void buffer2freq( double           p_in_data[],      /* Input: Time signal              */   
                  double           p_out_mdct[],     /* Output: MDCT cofficients        */
                  double           p_overlap[],
                  WINDOW_SEQUENCE  windowSequence,
                  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                  WINDOW_SHAPE     wfun_select_prev,
                  int              nlong,            /* shift length for long windows   */
                  int              nmed,             /* shift length for medium windows */
                  int              nshort,           /* shift length for short windows  */
                  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                  int              num_short_win);   /* number of short windows to      
                                                        transform                       */
void freq2buffer( double           p_in_data[],      /* Input: MDCT coefficients                */
                  double           p_out_data[],     /* Output:time domain reconstructed signal */
                  double           p_overlap[],  
                  WINDOW_SEQUENCE  windowSequence,
                  int              nlong,            /* shift length for long windows   */
                  int              nmed,             /* shift length for medium windows */
                  int              nshort,           /* shift length for short windows  */
                  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                  WINDOW_SHAPE     wfun_select_prev, /* YB : 971113 */
                  Imdct_out        overlap_select,   /* select imdct output *TK*        */
                  int              num_short_win );  /* number of short windows to  transform */

void mdct( double in_data[], double out_data[], int len);

void imdct(double in_data[], double out_data[], int len);

void fft( double in[], double out[], int len );

void aacScaleableDecodeInit( void );

void aacScaleableFlexMuxDecode ( HANDLE_BSBITSTREAM        fixed_stream,
                                 HANDLE_BSBITSTREAM        gc_WRstream[],
                                 WINDOW_SEQUENCE        windowSequence[MAX_TIME_CHANNELS],
                                 int*               flexMuxBits,
                                 double*            spectral_line_vector[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                                 int                output_select,
                                 Info**             sfbInfo,
                                 int                numChannels,
                                 int                lopLong,
                                 int                lopShort,
                                 float              normBw,
                                 WINDOW_SHAPE*      windowShape,
                                 HANDLE_DECODER_GENERAL hFault,
                                 QC_MOD_SELECT      qc_select,
                                 FRAME_DATA*        fd,
                                 TF_DATA*           tfData,
                                 ntt_DATA*          nttData,
				 NOK_LT_PRED_STATUS** nok_lt_status,
				 int                med_win_in_long,
				 int                short_win_in_long,
				 PRED_TYPE          pred_type);

void    printOpModeInfo(FILE * helpStream);


#endif  /* #ifndef _TF_MAIN_H_INCLUDED */


