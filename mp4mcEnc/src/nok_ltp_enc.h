/**************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Nokia

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
Copyright (c) ISO/IEC 1997.

***************************************************************************/

#ifndef NOK_LTP_ENC_H_
#define NOK_LTP_ENC_H_

#include "nok_ltp_common.h"
#include "tns.h"

extern void 
nok_init_lt_pred (NOK_LT_PRED_STATUS * lt_status);

extern int 
nok_ltp_enc(double *p_spectrum, double *p_time_signal,
	    WINDOW_TYPE_EXT win_type, Window_shape win_shape,
	    Window_shape win_shape_prev, int block_size_long, 
	    int block_size_short, int *sfb_offset, int num_of_sfb, 
	    NOK_LT_PRED_STATUS *lt_status, TNS_INFO *tnsInfo);

extern void 
nok_ltp_reconstruct(double *p_spectrum, WINDOW_TYPE_EXT win_type, 
		    int *sfb_offset, int num_of_sfb,
		    NOK_LT_PRED_STATUS *lt_status);

extern void
nok_ltp_update(NOK_LT_PRED_STATUS *lt_status, double *time_signal, 
	       double *overlap_signal, int block_size_long);

extern int 
nok_ltp_encode (BsBitStream *bs, WINDOW_TYPE_EXT win_type, int num_of_sfb, 
		NOK_LT_PRED_STATUS *lt_status, int write_flag);

#endif /* NOK_LTP_ENC_H_ */
