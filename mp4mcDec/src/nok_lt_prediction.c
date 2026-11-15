/*****************************************************************************
 *                                                                           *
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
 *                                                                           *
 ****************************************************************************/

/**************************************************************************
  nok_lt_prediction.c  -  Performs Long Term Prediction for the MPEG-4
  			  T/F Decoder.
  Author(s): Mikko Suonio, Juha Ojanpera
  	     Nokia Research Center, Speech and Audio Systems, 1997.
  *************************************************************************/


/**************************************************************************
  Version Control Information			Method: CVS
  Identifiers:
  $Revision: 1.1 $
  $Date: 2000/10/16 10:01:58 $ (check in)
  $Author: sps $
  *************************************************************************/

/**************************************************************************
  External Objects Needed
  *************************************************************************/
/*
  Standard library declarations.  */

/*
  Interface to related modules.  */
#include "block.h"
#include "dolby_adapt.h"
#include "gc.h"
#include "nok_lt_prediction.h"
#include "nok_ltp_common.h"
#include "bitinput.h"
#include "tns.h"

/*
  Macro:	PRINT
  Purpose:	Interface to function for printing error messages.  */

/*
  Enum constant:MAX_SHORT_WINDOWS
  Purpose:	Gives the maximum number of subblocks (short windows).
  Explanation:	-  */

/*
  Variable:	debug
  Purpose:	Provided debug options.
  Explanation:	If element debug['p'] is true, we give debug information
  		on long term prediction.  */

/*
  Function:	buffer2freq
  Call:		buffer2freq (p_in_data, p_out_mdct, p_overlap, block_type,
  			     wfun_select, nlong, nmed, nshort, overlap_select,
			     num_short_win, save_window);
  Purpose:	Modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: 2*shift length)
		p_overlap[]	- buffer containing the previous frame
				  in OVERLAPPED-mode (length: 2*shift length)
		block_type	- selects the type of window to use
  		wfun_select	- select window function
  		nlong		- shift length for long windows
                nmed		- shift length for medium windows
		nshort		- shift length for short windows
		overlap_select	- OVERLAPPED-mode select
		num_short_win	- number of short windows to transform
		save_window	- if true, save the block_type
				  for future use
  Output:	p_out_mdct[]	- transformed signal (length: shift length)
		p_overlap[]	- a copy of p_in_data in OVERLAPPED-mode
  				  (length: shift length)
  Explanation:	-  */

/*
  Function:	freq2buffer
  Call:		freq2buffer (p_in_data, p_out_data, p_overlap, block_type,
  			     nlong, nmed, nshort, wfun_select, fun_select_prev,
			     overlap_select, num_short_win);
  Purpose:	Inverse of modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: shift length)
		p_overlap[]	- the overlap buffer; does not have
				  an effect in NON_OVERLAPPED-mode,
				  but have to be allocated
				  (length: shift length)
		block_type	- selects the type of window to use
  		nlong		- shift length for long windows
                nmed		- shift length for medium windows
		nshort		- shift length for short windows
  		wfun_select	- select window function
		overlap_select	- OVERLAPPED-mode select
		num_short_win	- number of short windows to transform
 		save_window	- if true, save block_type for future use
  Output:	p_out_data[]	- transformed signal (length: 2*shift length)
		p_overlap[]	- the overlap buffer; always modified
				  (length: shift length)
  Explanation:	-  */

/*
  Function:	getbits
  Purpose:	Reads specified number of bits from the open
  		input bitstream.  */


/**************************************************************************
  External Objects Provided
  *************************************************************************/
#include "nok_lt_prediction.h"


/**************************************************************************
  Internal Objects
  *************************************************************************/
#include "nok_ltp_common_internal.h"

static short double_to_int (Float sig_in);


/**************************************************************************
  Object Definitions
  *************************************************************************/


/**************************************************************************
  Title:	nok_init_lt_pred

  Purpose:	Initialize the history buffer for long term prediction

  Usage:	nok_init_lt_pred (lt_status)


  Input:	lt_status
  			- buffer: history buffer

  Output:	lt_status
  			- buffer: filled with 0

  References:	-

  Explanation:	-

  Author(s):	Mikko Suonio
  *************************************************************************/

void
nok_init_lt_pred (NOK_LT_PRED_STATUS *lt_status)
{
  int i;

  for (i = 0; i < NOK_LT_BLEN; i++)
    lt_status->buffer[i] = 0;
}


/**************************************************************************
  Title:	nok_lt_predict

  Purpose:	Adds the LTP contribution to the reconstructed spectrum.

  Usage:        y = nok_ltp_predict(info, win_type, winShape_this_bk, 
                                    winShape_prev_bk, sbk_prediction_used, 
                                    sfb_prediction_used, lt_status, weight, 
				    delay,  current_frame, block_size_long,  
				    block_size_medium, block_size_short, 
				    tns_frame_info)

  Input:        info:
                 nsbk               - number of subblocks in a block
		 bins_per_sbk       - number of spectral coefficients
			   	      in each subblock; currently we 
				      assume that they are of equal size
		 sfb_per_bk         - total # scalefactor bands in a block
		 sfb_per_sbk        - # scalefactor bands in each subblock
	        win_type            - window sequence (frame, block) type
	        winShape_this_bk    - shape of the latter half of the mdct window
		winShape_prev_bk    - shape of the first half of the mdct window
  	        sbk_prediction_used - first item toggles prediction on(1)/off(0) 
	                              for all subblocks, next items toggle it 
		 	              on/off on each subblock separately
  	        sfb_prediction_used - first item is not used, but the following 
	                              items toggle prediction on(1)/off(0) 
				      on each scalefactor-band of every subblock
	        lt_status :
		 buffer             - history buffer for prediction
	        weight              - a weight factor to apply to all subblocks
	        delay	            - array of delays to apply to each subblock
	        current_frame       - the dequantized spectral coeffients and
		 	              prediction errors where prediction is used
	        block_size_long     - size of the long block
	        block_size_medium   - size of the medium block
	        block_size_short    - size of the short block
	        tns_frame_info      - TNS parameters

  Output:       y - reconstructed spectrum containing also LTP contribution

  References:	1.) time2freq_adapt in dolby_adapt.c
                2.) tns_filter_subblock in tns.c
		3.) double_to_int

  Explanation:	-

  Author(s):	Mikko Suonio, Juha Ojanpera
  *************************************************************************/

void
nok_lt_predict(Info *info, WINDOW_TYPE_EXT win_type, 
	       byte winShape_this_bk, byte winShape_prev_bk,
	       int *sbk_prediction_used, int *sfb_prediction_used, 
	       NOK_LT_PRED_STATUS *lt_status, Float weight, int *delay, 
	       Float *current_frame, int block_size_long, 
               TNS_frame_info *tns_frame_info)
{
  Wnd_Shape win_shape;
  int i, j, num_samples;
  float_ext mdct_predicted[2 * NOK_MAX_BLOCK_LEN_LONG];
  float_ext predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];


  switch(win_type)
  {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
      if (sbk_prediction_used[0])
      {
	/* Prediction for time domain signal */
	num_samples =  2 * block_size_long;
	j = NOK_LT_BLEN - 2 * block_size_long - (delay[0] - DELAY / 2);
	if(NOK_LT_BLEN - j <  2 * block_size_long)
	  num_samples = NOK_LT_BLEN - j;

	for(i = 0; i < num_samples; i++)
	  predicted_samples[i] = weight * lt_status->buffer[i + j];
	for( ; i < 2 * block_size_long; i++)
	  predicted_samples[i] = 0.0f;

	/* Transform prediction to frequency domain. */
	win_shape.this_bk = winShape_this_bk;
	win_shape.prev_bk = winShape_prev_bk;
	time2freq_adapt (win_type, &win_shape, predicted_samples, mdct_predicted);
	
	/* Apply the TNS analysis filter to the predicted spectrum. */
	if(tns_frame_info != NULL)
	  tns_filter_subblock(mdct_predicted, info->sfb_per_bk, info->sbk_sfb_top[0], 
			      1, &tns_frame_info->info[0]);
	
	/* Clean those sfb's where prediction is not used. */
	for (i = 0, j = 0; i < info->sfb_per_bk; i++)
	  if (sfb_prediction_used[i + 1] == 0)
	    for (; j < info->sbk_sfb_top[0][i]; j++)
	      mdct_predicted[j] = 0.0;
	  else
	    j = info->sbk_sfb_top[0][i];
	
	/* Add the prediction to dequantized spectrum. */
	for (i = 0; i < block_size_long; i++)
	  current_frame[i] = current_frame[i] + mdct_predicted[i];
      }
      break;

    default:
      break;
  }

}


/**************************************************************************
  Title:        nok_lt_update

  Purpose:	Updates the time domain buffer of LTP.

  Usage:        nok_lt_update(lt_status, time_signal, overlap_signal, 
                              block_size_long)
				   
  Input:        time_signal     - reconstructed time signal
                overlap_signal  - alias buffer
		block_size_long - size of the long block

  Output:	lt_status :
		 buffer         - history buffer for prediction

  References:	1.) double_to_int

  Explanation:	-

  Author(s):	Mikko Suonio, Juha Ojanpera
  *************************************************************************/

void 
nok_lt_update(NOK_LT_PRED_STATUS *lt_status, Float *time_signal, 
	      Float *overlap_signal, int block_size_long) 
{
  int i;
  
  for(i = 0; i < NOK_LT_BLEN - 2 * block_size_long; i++)
    lt_status->buffer[i] = lt_status->buffer[i + block_size_long];
  
  for(i = 0; i < block_size_long; i++) 
  {   
    lt_status->buffer[NOK_LT_BLEN - 2 * block_size_long + i] = 
      double_to_int (time_signal[i]);
    
    lt_status->buffer[NOK_LT_BLEN - block_size_long + i] = 
      double_to_int (overlap_signal[i]);
  }
  
}


/**************************************************************************
  Title:	double_to_int

  Purpose:      Converts floating point format to integer (16-bit).

  Usage:	y = double_to_int(sig_in)

  Input:	sig_in  - floating point number

  Output:	y  - integer number

  References:	-

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

static short
double_to_int (Float sig_in)
{
  short sig_out = 0;


  if (sig_in > 32767)
    sig_out = 32767;
  else if (sig_in < -32768)
    sig_out = -32768;
  else if (sig_in > 0.0)
    sig_out = (short) (sig_in + 0.5);
  else if (sig_in <= 0.0)
    sig_out = (short) (sig_in - 0.5);

  return (sig_out);
}

 
/**************************************************************************
  Title:	nok_lt_decode

  Purpose:	Decode the bitstream elements for long term prediction

  Usage:	nok_lt_decode(max_sfb, sbk_prediction_used,
                              sfb_prediction_used, weight, delay)

  Input:        max_sfb         - # scalefactor bands used in the current frame

  Output:       sbk_prediction_used - first item toggles prediction 
		                      on(1)/off(0) for all subblocks, 
				      next items toggle it on/off on
				      each subblock separately
  		sfb_prediction_used - first item is not used, but the 
		                      following items toggle prediction 
				      on(1)/off(0) on each scalefactor-band 
				      of every subblock
		weight	        - a weight factor to apply to all subblocks
		delay	        - array of delays to apply to each subblock

  References:	1.) getbits
  		2.) PRINT

  Explanation:	-

  Author(s):	Mikko Suonio
  *************************************************************************/

void
nok_lt_decode(int max_sfb, int *sbk_prediction_used, int *sfb_prediction_used, 
	      Float *weight, int *delay)
{
  int i;
  int last_band = 0;


  if ((sbk_prediction_used[0] = getbits (LEN_LTP_DATA_PRESENT)))
  {
    delay[0] = getbits (LEN_LTP_LAG);
    *weight = codebook[getbits (LEN_LTP_COEF)];
    
    last_band = (max_sfb < NOK_MAX_LT_PRED_LONG_SFB
		 ? max_sfb : NOK_MAX_LT_PRED_LONG_SFB) + 1;
    
    sfb_prediction_used[0] = sbk_prediction_used[0];
    for (i = 1; i < last_band; i++)
      sfb_prediction_used[i] = getbits (LEN_LTP_LONG_USED);
    for (; i < max_sfb + 1; i++)
      sfb_prediction_used[i] = 0;

    for( ; i < MAX_SCFAC_BANDS; i++)
      sfb_prediction_used[i] = 0;
  }
  
  if (debug['p'])
  {
    if (sbk_prediction_used[0])
    {
      PRINT (SE, "long term prediction enabled (%2d):  ", last_band);
      for (i = 1; i < max_sfb + 1; i++)
	PRINT (SE, " %d", sfb_prediction_used[i]);
      PRINT (SE, "\n");
      for (i = 1; i < MAX_SHORT_WINDOWS + 1; i++)
	PRINT (SE, " %d", sbk_prediction_used[i]);
      PRINT (SE, "\n");
    }
  }

}
