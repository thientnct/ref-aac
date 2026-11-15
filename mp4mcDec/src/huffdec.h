/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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

#ifndef _huffdec_h_
#define _huffdec_h_

#include "block.h"
#include "config.h"
#include "adifdec.h"
#include "huffinit.h"
#include "tns.h"

#ifdef MPEG4V1
#include "nok_lt_prediction.h"
#endif

#ifdef FRAMELENGTH_FLAG
static const int maxSfb960FrameLen[16] = {  40, /*  0 : 96000 */
                                            40, /*  1 : 88200 */
                                            46, /*  2 : 64000 */
                                            49, /*  3 : 48000 */
                                            49, /*  4 : 44100 */
                                            49, /*  5 : 32000 */
                                            46, /*  6 : 24000 */
                                            46, /*  7 : 22050 */
                                            42, /*  8 : 16000 */
                                            42, /*  9 : 12000 */
                                            42, /* 10 : 11025 */
                                            40, /* 11 :  8000 */
                                            40, /* 12 :  7350 */
                                             0, /* 13 :  res. */
                                             0, /* 14 :  res. */
                                             0  /* 15 :  res. */ };
#endif

int	huffdecode(int id, MC_Info *mip,
               byte *win,
               Wnd_Shape *wshape,
               byte **cb_map,
               short **factors,
               byte **group,
               byte *hasmask,
               byte **mask,
               byte *max_sfb,
               int **lpflag,
               int **prstflag,
#ifdef MPEG4V1
               NOK_LT_PRED_STATUS **nok_ltp,
#endif
               TNS_frame_info **tns,
               Float **coef
#ifdef CT_SBR
              , int* chIndx
#endif   
              ,int* first_cpe
               );


#endif
