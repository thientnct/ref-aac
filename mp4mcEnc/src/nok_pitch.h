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
#ifndef NOK_PITCH_H_
#define NOK_PITCH_H_


extern double 
snr_pred (double *mdct_in, double *mdct_pred, int *sfb_flag, int *sfb_offset, 
	  int side_info, int num_of_sfb, int frame_len);

extern void 
prediction (short *buffer, double *predicted_samples, Float *weight, int lag,
	    int flen);

extern int
pitch(double *sb_samples, short *x_buffer, int flen, int lag0, int lag1, 
      double *predicted_samples, Float *gain, int *cb_idx);

#endif /* NOK_PITCH_H_ */
