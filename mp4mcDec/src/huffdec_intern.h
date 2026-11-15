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

#ifndef _huffdec_intern_h_
#define _huffdec_intern_h_

#include "adifdec.h"
#include "huffinit.h"
#include "tns.h"

#ifdef MPEG4V1
#include "nok_lt_prediction.h"
#endif

void	get_sign_bits(int *q, int n);

int	getescape(int q);
Float	esc_iquant(int q);
int	decode_huff_cw(Huffman *h);
void	unpack_idx(int *qp, int idx, Hcb *hcb);

void	getgroup(Info *info, byte *group);

void	get_ics_info(byte *win, byte *wshape, byte *group,byte *max_sfb, int *lpflag, int *prstflag,
#ifdef MPEG4V1
                     NOK_LT_PRED_STATUS *nok_ltp_status, NOK_LT_PRED_STATUS *nok_ltp_status_right, int stereo_flag,
#endif
                     Info *wininfo);
int	getics(Info *info, int common_window, byte *win, byte *wshape, byte *group, byte *max_sfb, int *lpflag, int *prstflag, byte *cb_map, Float *coef, short *global_gain, short *factors,
#ifdef MPEG4V1
               NOK_LT_PRED_STATUS *nok_ltp_status,
#endif
               TNS_frame_info *tns, int ch, int max_spectral_line );

#endif
