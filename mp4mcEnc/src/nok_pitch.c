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
/**************************************************************************
  nok_pitch.c  - Long Term Prediction (LTP) subroutines.
 
  Author(s): Juha Ojanpera, Lin Yin
  	     Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/


/**************************************************************************
  Version Control Information			Method: CVS
  Identifiers:
  $Revision: 1.6 $
  $Date: 1998/12/17 08:27:14 $ (check in)
  $Author: jojanper $
  *************************************************************************/


#undef DO_LTP

#ifdef MPEG4V1
#define DO_LTP
#endif

#ifdef DO_LTP
/**************************************************************************
  External Objects Needed
  *************************************************************************/
/*
  Standard library declarations.  */
#include <math.h>
#include <stdio.h>

/*
  Interface to related modules.  */
#include "tf_main.h"
#include "block.h"
#include "interface.h"
#include "nok_ltp_common.h"
#include "nok_ltp_enc.h"


/*************************************************************************
  External Objects Provided
  *************************************************************************/
#include "nok_pitch.h"


/**************************************************************************
  Internal Objects
  *************************************************************************/
#include "nok_ltp_common_internal.h"

static void w_quantize (Float *freq, int *ltp_idx);


/**************************************************************************
  Title:	snr_pred

  Purpose:	Determines is it feasible to employ long term prediction in 
                the encoder.

  Usage:        y = snr_pred(mdct_in, mdct_pred, sfb_flag, sfb_offset, 
                             side_info, num_of_sfb, frame_len)

  Input:        mdct_in         - spectral coefficients
                mdct_pred       - predicted spectral coefficients
                sfb_flag        - an array of flags indicating whether LTP is 
                                  switched on (1) /off (0). One bit is reseved 
                                  for each sfb.
                sfb_offset      - scalefactor boundaries
		side_info       - LTP side information
		num_of_sfb      - number of scalefactor bands
		frame_len       - number of spectral samples in a frame

  Output:	y - number of bits saved by using long term prediction     

  References:	-

  Explanation:	-

  Author(s):	Juha Ojanpera, Lin Yin
  *************************************************************************/

double
snr_pred (double *mdct_in, double *mdct_pred, int *sfb_flag, int *sfb_offset,
	  int side_info, int num_of_sfb, int frame_len)
{
  int i, j;
  double snr_limit;
  double num_bit, snr[NSFB_LONG];
  double temp1, temp2;
  double energy[BLOCK_LEN_LONG], snr_p[BLOCK_LEN_LONG];

  snr_limit = 1.e-20;

  for (i = 0; i < frame_len; i++)
    {
      energy[i] = mdct_in[i] * mdct_in[i];
      snr_p[i] = (mdct_in[i] - mdct_pred[i]) * (mdct_in[i] - mdct_pred[i]);
    }

  for (i = 0; i < num_of_sfb; i++)
    {
      temp1 = 0.0;
      temp2 = 0.0;
      for (j = sfb_offset[i]; j < sfb_offset[i + 1]; j++)
	{
	  temp1 += energy[j];
	  temp2 += snr_p[j];
	}

      if (temp2 < snr_limit)
	temp2 = snr_limit;

      if (temp1 > 1.e-20)
	snr[i] = -10. * log10 (temp2 / temp1);
      else
	snr[i] = 0.0;

      sfb_flag[i] = 1;

      if (snr[i] <= 0.0)
      {
	sfb_flag[i] = 0;
	for (j = sfb_offset[i]; j < sfb_offset[i + 1]; j++)
	  mdct_pred[j] = 0.0;
      }
    }

  num_bit = 0.0;
  for (i = 0; i < num_of_sfb; i++)
    if (snr[i] > 0.0)
      num_bit += snr[i] / 6. * (sfb_offset[i + 1] - sfb_offset[i]);

  if (num_bit < side_info)
    {
      num_bit = 0.0;
      for (j = 0; j < frame_len; j++)
	mdct_pred[j] = 0.0;
      for (i = 0; i < num_of_sfb; i++)
	sfb_flag[i] = 0;
    }
  else
    num_bit -= side_info;

  return (num_bit);
}


/**************************************************************************
  Title:	prediction

  Purpose:	Predicts current frame from the past output samples.

  Usage:        prediction(buffer, predicted_samples, weight, delay, flen)

  Input:	buffer	          - past reconstructed output samples
                weight            - LTP gain (scaling factor)
		delay             - LTP lag
                flen              - length of the frame

  Output:	predicted_samples - predicted frame

  References:	-

  Explanation:  -

  Author(s):	Juha Ojanpera, Lin Yin
  *************************************************************************/

void
prediction (short *buffer, double *predicted_samples, Float *weight, int lag, 
	    int flen)
{
  int i, offset;
  int num_samples;


  offset = NOK_LT_BLEN - flen / 2 - lag;

  num_samples = flen;
  if(NOK_LT_BLEN - offset < flen)
    num_samples = NOK_LT_BLEN - offset;
  
  for(i = 0; i < num_samples; i++)
    predicted_samples[i] = *weight * buffer[offset++];
  for( ; i < flen; i++)
    predicted_samples[i] = 0.0f;

}


/**************************************************************************
  Title:	pitch

  Purpose:      Finds optimal delay between the current frame and the past
                reconstructed output samples. The lag between 'lag0'...'lag1'-1 
                having the maximum correlation becomes the LTP lag (or delay).
		In addition, determines the gain and finally computes the
		predicted signal.

  Usage:        y = pitch(sb_samples, x_buffer, flen, lag0, lag1, 
                          predicted_samples, gain, cb_idx)

  Input:	sb_samples        - current frame
                x_buffer          - past reconstructed output samples
                flen              - length of the frame
		lag0              - minimum lag
		lag1              - maximum lag

  Output:	y                 - LTP lag
                predicted_samples - predicted (and scaled) frame
		gain              - scaling factor for the predicted frame
		cb_idx            - codebook index that will be written into 
		                    the bit stream

  References:	1.) w_quantize
                2.) prediction

  Explanation:	-

  Author(s):	Juha Ojanpera, Lin Yin
  *************************************************************************/

int
pitch(double *sb_samples, short *x_buffer, int flen, int lag0, int lag1, 
      double *predicted_samples, Float *gain, int *cb_idx)
{
  int i, j, delay, start;
  int offset;
  double corr1, corr2, lag_corr;
  double p_max, energy = 0, lag_energy;

  p_max = 0.0;
  lag_corr = lag_energy = 0.0;
  delay = lag0;

  /* Find the lag. */
  for (i = lag0; i < lag1; i++)
  {
    corr1 = corr2 = 0.0;

    start = 0;
    offset = i;

    /*
     * Below is a figure illustrating how the lag and the
     * samples in the buffer relate to each other.
     *
     * ------------------------------------------------------------------
     * |              |               |                |                 |
     * |    slot 1    |      2        |       3        |       4         |
     * |              |               |                |                 |
     * ------------------------------------------------------------------
     *
     * lag = 0 refers to the end of slot 4 and lag = DELAY refers to the end
     * of slot 2. The start of the predicted frame is then obtained by
     * adding the length of the frame to the lag. Remember that slot 4 doesn't
     * actually exist, since it is always filled with zeros.
     *
     * The above short explanation was for long blocks. For short blocks the
     * zero lag doesn't refer to the end of slot 4 but to the start of slot
     * 4 - the frame length of a short block.
     *
     * Some extra code is then needed to handle those lag values that refer
     * to slot 4.
     */
    if(i < DELAY / 2)
    {
      offset = i - lag0;
      start = flen / 2 + i;
    }
    else
    {
      offset = i - DELAY / 2; 
      start = 0;
    }

    if(start || offset == 0)
    {
      energy = 0.0f;
      for (j = start; j < flen; j++)
      {
	corr1 += x_buffer[NOK_LT_BLEN - offset - j - 1] * sb_samples[flen - j - 1];
	energy += x_buffer[NOK_LT_BLEN - offset - j - 1] * x_buffer[NOK_LT_BLEN - offset - j - 1];
      }
    }
    else /* start == 0 && offset != 0 */
    {
      /* No need to compute the whole energy. */
      energy -= x_buffer[NOK_LT_BLEN - offset] * x_buffer[NOK_LT_BLEN - offset];
      energy += x_buffer[NOK_LT_BLEN - offset - flen] * x_buffer[NOK_LT_BLEN - offset - flen];
      
      for (j = 0; j < flen; j++)
	corr1 += x_buffer[NOK_LT_BLEN - offset - j - 1] * sb_samples[flen - j - 1];
    }
    
    if (energy != 0.0)
      corr2 = corr1 / sqrt (energy);
    else
      corr2 = 0.0;

    if (p_max < corr2)
    {
      p_max = corr2;
      delay = i;
      lag_corr = corr1;
      lag_energy = energy;
    }
  }

  /* Compute the gain. */
  if(lag_energy != 0.0)
    *gain =  lag_corr / (1.010 * lag_energy);
  else
    *gain = 0.0;

  /* Quantize the gain. */
  w_quantize(gain, cb_idx);

  /* Get the predicted signal. */
  prediction(x_buffer, predicted_samples, gain, delay, flen);

  return (delay);
}


/**************************************************************************
  Title:	w_quantize

  Purpose:	Quantizes LTP gain.

  Usage:	w_quantize(freq, ltp_idx)

  Input:	freq    - unquantized LTP gain

  Output:	freq    - quantized LTP gain selected from the codebook
                ltp_idx - codebook index of the LTP gain

  References:	-

  Explanation:	-

  Author(s):	Juha Ojanpera, Lin Yin
  *************************************************************************/

void
w_quantize (Float *freq, int *ltp_idx)
{
  int i;
  double dist, low;

  low = 1.0e+10;
  dist = 0.0;
  for (i = 0; i < CODESIZE; i++)
  {
    dist = (*freq - codebook[i]) * (*freq - codebook[i]);
    if (dist < low)
    {
      low = dist;
      *ltp_idx = i;
    }
  }
  
  *freq = codebook[*ltp_idx];
}

#endif /* DO_LTP */
