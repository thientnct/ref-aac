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
  nok_ltp_enc.c  -  Performs Long Term Prediction for the MPEG-4 
                    T/F Encoder.
  Author(s): Mikko Suonio, Juha Ojanpera
  	     Nokia Research Center, Speech and Audio Systems, 1997.
  *************************************************************************/


/**************************************************************************
  Version Control Information			Method: CVS
  Identifiers:
  $Revision: 1.12 $
  $Date: 1998/12/17 08:26:32 $ (check in)
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
#include <stdio.h>

/*
  Interface to related modules.  */
#include "common.h"	/* HP 980211 */
#include "tf_main.h"
#include "interface.h"
#include "bitstream.h"
#include "nok_ltp_common.h"
#include "nok_pitch.h"
#include "tns.h"

/*
  Macro:	PRINT
  Purpose:	Interface to function for printing error messages.  */

/*
  Variable:	debug
  Purpose:	Provided debug options.
  Explanation:	If element debug['p'] is true, we give debug information
  		on long term prediction.  */

/*
  Function:	buffer2freq
  Call:		buffer2freq (p_in_data, p_out_mdct, p_overlap, block_type,
  			     wfun_select, nlong, nshort, overlap_select,
			     num_short_win, save_window);
  Purpose:	Modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: 2*shift length)
		p_overlap[]	- buffer containing the previous frame
				  in OVERLAPPED-mode (length: 2*shift length)
		block_type	- selects the type of window to use
  		wfun_select	- select window function
  		nlong		- shift length for long windows
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
  			     nlong, nshort, wfun_select, fun_select_prev,
			     overlap_select, num_short_win);
  Purpose:	Inverse of modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: shift length)
		p_overlap[]	- the overlap buffer; does not have
				  an effect in NON_OVERLAPPED-mode,
				  but have to be allocated
				  (length: shift length)
		block_type	- selects the type of window to use
  		nlong		- shift length for long windows
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
  Function:	BsPutBit
  Purpose:	Writes specified number of bits to the open
  		output bitstream.  */


/**************************************************************************
  External Objects Provided
  *************************************************************************/
#include "nok_ltp_enc.h"


/**************************************************************************
  Internal Objects
  *************************************************************************/
#include "nok_ltp_common_internal.h"

static short double_to_int (double sig_in);

/*
  Purpose:	Debug switch for LTP.
  Explanation:	- */
/*#define LTP_DEBUG*/

/**************************************************************************
  Object Definitions
  *************************************************************************/


/**************************************************************************
  Title:	nok_init_lt_pred

  Purpose:	Initialize the history buffer for long term prediction

  Usage:        nok_init_lt_pred (lt_status)

  Input:	lt_status
  			- buffer: history buffer
                        - weight_idx : 
                          3 bit number indicating the LTP coefficient in 
                          the codebook 
                        - sbk_prediction_used: 
                          1 bit for each subblock indicating wheather
                          LTP is used in that subblock 
                        - sfb_prediction_used:
                          1 bit for each scalefactor band (sfb) where LTP 
                          can be used indicating whether LTP is switched 
                          on (1) /off (0) in that sfb.
                        - delay: LTP lag
                        - side_info: LTP side information
			- mdct_predicted: predicted spectrum

  Output:	lt_status
  			- buffer: filled with 0
                        - weight_idx : filled with 0
                        - sbk_prediction_used: filled with 0
                        - sfb_prediction_used: filled with 0
                        - delay: filled with 0
                        - side_info: filled with 1
			- mdct_predicted: filled with 0

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

  lt_status->weight_idx = 0;
  for(i=0; i<NSHORT; i++)
    lt_status->sbk_prediction_used[i] = lt_status->delay[i] = 0;

  for(i=0; i<MAX_SCFAC_BANDS; i++)
    lt_status->sfb_prediction_used[i] = 0;

  lt_status->side_info = LEN_LTP_DATA_PRESENT;

  for(i = 0; i < 2 * NOK_MAX_BLOCK_LEN_LONG; i++)
    lt_status->mdct_predicted[i] = 0.0;
}


/**************************************************************************
  Title:	ltp_enc_tf

  Purpose:      Transfers the predicted time domain signal into frequency 
                domain and determines whether coding gain can be achieved.

  Usage:        ltp_enc_tf(p_spectrum, predicted_samples, mdct_predicted, 
	                   win_type, win_shape, win_shape_prev, 
			   block_size_long, 
			   block_size_short, sfb_offset, num_of_sfb, 
			   last_band, side_info, sfb_prediction_used, 
			   tnsInfo)

  Input:        p_spectrum        - spectral coefficients
                predicted_samples - predicted time domain samples	       
                win_type          - window sequence (frame, block) type
                win_shape         - shape of the mdct window
		win_shape_prev    - shape from previous block
		block_size_long   - size of the long block
		block_size_short  - size of the short block
                sfb_offset        - scalefactor band boundaries
                num_of_sfb        - number of scalefactor bands in each block
		last_band         - last scalefactor band where LTP is used
		side_info         - LTP side information bits
		tnsInfo           - TNS encoding parameters

  Output:	y                   - prediction gain
                mdct_predicted      - predicted spectrum
                p_spectrum          - error spectrum (if LTP used)
		sfb_prediction_used - 1 bit for each scalefactor band (sfb) 
		                      where LTP can be used indicating whether 
				      LTP is switched on (1) /off (0) in 
				      that sfb.
                        
  References:	1.) buffer2freq in imdct.c
                2.) TnsEncodeFilterOnly in tns.c
                3.) snr_pred in nok_pitch.c

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

static double
ltp_enc_tf(double *p_spectrum, double *predicted_samples, 
	   double *mdct_predicted, WINDOW_TYPE_EXT win_type, 
	   Window_shape win_shape, Window_shape win_shape_prev,
	   int block_size_long, int block_size_short, 
	   int *sfb_offset, int num_of_sfb, int last_band, int side_info, 
	   int *sfb_prediction_used, TNS_INFO *tnsInfo)
{
  int frame_len;
  double bit_gain;

  /* Transform prediction to frequency domain. */
  buffer2freq (predicted_samples, mdct_predicted, NULL, win_type, win_shape, 
	       block_size_long, block_size_short, 
	       NON_OVERLAPPED);
  
  /* Apply TNS analysis filter to the predicted spectrum. */
  if(tnsInfo != NULL)
    TnsEncodeFilterOnly(num_of_sfb, num_of_sfb, win_type, sfb_offset, 
			mdct_predicted, tnsInfo);
  
  /* Get the prediction gain. */
  bit_gain = snr_pred (p_spectrum, mdct_predicted, sfb_prediction_used, 
		       sfb_offset, side_info, last_band, block_size_long);
  
  return (bit_gain);
}


/**************************************************************************
  Title:	nok_ltp_enc

  Purpose:      Performs long term prediction.

  Usage:	y = nok_ltp_enc(p_spectrum, p_time_signal, win_type, win_shape,
                                win_shape_prev, block_size_long, 
				block_size_short, 
				sfb_offset, num_of_sfb, lt_status, tnsInfo)

  Input:        p_spectrum        - spectral coefficients
                p_time_signal     - current input samples
                win_type          - window sequence (frame, block) type
                win_shape         - shape of the mdct window
		win_shape_prev    - shape from previous block
		block_size_long   - size of the long block
		block_size_short  - size of the short block
                sfb_offset        - scalefactor band boundaries
                num_of_sfb        - number of scalefactor bands in each block
		lt_status :
		 buffer           - history buffer
		tnsInfo           - TNS encoding parameters

  Output:	y                    - 1 if LTP used, 0 otherwise
                p_spectrum           - error spectrum (if LTP used)
                lt_status :
		 weight_idx          - 3 bit number indicating the LTP 
		                       coefficient in the codebook 
	         sbk_prediction_used - 1 bit for each subblock indicating 
		                       wheather LTP is used in that subblock 
		 sfb_prediction_used - 1 bit for each scalefactor band (sfb) 
		                       where LTP can be used indicating whether
				       LTP is switched on (1) /off (0) in 
				       that sfb.
                 delay               - LTP lag
                 side_info           - LTP side information
                        
  References:	1.) pitch in nok_pitch.c
                2.) prediction in nok_pitch.c
		3.) ltp_enc_tf

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

int
nok_ltp_enc(double *p_spectrum, double *p_time_signal, WINDOW_TYPE_EXT win_type, 
            Window_shape win_shape, Window_shape win_shape_prev, 
	    int block_size_long, int block_size_short, 
	    int *sfb_offset, int num_of_sfb, NOK_LT_PRED_STATUS *lt_status, 
	    TNS_INFO *tnsInfo)
{
  int i, last_band;
  double *time_signal;
  double num_bit[MAX_SHORT_WINDOWS];
  double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];


  lt_status->global_pred_flag = 0;
  lt_status->side_info = 0;
  
  switch(win_type)
  {
    case ONLY_LONG_WINDOW:
    case LONG_SHORT_WINDOW:
    case SHORT_LONG_WINDOW:
      last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;

      lt_status->delay[0] = 
	pitch(p_time_signal, lt_status->buffer, 2 * block_size_long, 
	      0, 2 * block_size_long, predicted_samples, &lt_status->weight, 
	      &lt_status->weight_idx);

      num_bit[0] = 
	ltp_enc_tf(p_spectrum, predicted_samples, lt_status->mdct_predicted, 
		   win_type, win_shape, win_shape_prev, block_size_long, 
		   block_size_short, sfb_offset, num_of_sfb,
		   last_band, lt_status->side_info, lt_status->sfb_prediction_used, 
		   tnsInfo);

#ifdef LTP_DEBUG
      fprintf(stdout, "(LTP) lag : %i ", lt_status->delay[0]);
      fprintf(stdout, "(LTP) bit gain : %f\n", num_bit[0]);
#endif /* LTP_DEBUG */
      
      lt_status->global_pred_flag = (num_bit[0] == 0.0) ? 0 : 1;
      
      if(lt_status->global_pred_flag)
	for (i = 0; i < sfb_offset[last_band]; i++)
	  p_spectrum[i] -= lt_status->mdct_predicted[i];
      else
	lt_status->side_info = 1;
      
      break;
      
    default:
      break;
  }
  
  return (lt_status->global_pred_flag);
}


/**************************************************************************
  Title:        nok_ltp_reconstruct

  Purpose:	Adds the predicted spectrum to the reconstructed error
                spectrum.

  Usage:        nok_ltp_reconstruct(p_spectrum, win_type, sfb_offset, 
		                    num_of_sfb, lt_status)
				   
  Input:	p_spectrum       - reconstructed (error) spectrum
		win_type         - window sequence (frame, block) type
		sfb_offset       - scalefactor band boundaries
                num_of_sfb       - number of scalefactor bands in each block
		lt_status :
		 mdct_predicted  - predicted spectrum

  Output:	p_spectrum       - reconstructed spectrum containing also 
                                   LTP contribution

  References:	-

  Explanation:	-

  Author(s):	Juha Ojanpera
  *************************************************************************/

void
nok_ltp_reconstruct(double *p_spectrum, WINDOW_TYPE_EXT win_type, 
                    int *sfb_offset, int num_of_sfb, 
		    NOK_LT_PRED_STATUS *lt_status)
{
  int i, last_band;
  
  if(lt_status->global_pred_flag)
  {
    switch(win_type)
    {
      case ONLY_LONG_WINDOW:
      case LONG_SHORT_WINDOW:
      case SHORT_LONG_WINDOW:
	last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	  num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
	
        for (i = 0; i < sfb_offset[last_band]; i++)
          p_spectrum[i] += lt_status->mdct_predicted[i];
	break;
	
      default:
	break;
    }
  }
  
}


/**************************************************************************
  Title:        nok_ltp_update

  Purpose:	Updates the time domain buffer of LTP.

  Usage:        nok_ltp_update(lt_status, time_signal, overlap_signal, 
                               block_size_long)
				   
  Input:        time_signal     - reconstructed time signal
                overlap_signal  - alias buffer
		block_size_long - size of the long block

  Output:	lt_status :
		 buffer         - history buffer for prediction

  References:	1.) double_to_int

  Explanation:	-

  Author(s):	Juha Ojanpera
  *************************************************************************/

void 
nok_ltp_update(NOK_LT_PRED_STATUS *lt_status, double *time_signal, 
	       double *overlap_signal, int block_size_long) 
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
  Title:	nok_ltp_encode

  Purpose:      Writes LTP parameters to the bit stream.

  Usage:	y = nok_ltp_encode (bs, win_type, num_of_sfb, lt_status, 
                                    write_flag)

  Input:        bs                   - bit stream
                win_type             - window sequence (frame, block) type
                num_of_sfb           - number of scalefactor bands
                lt_status  
		 side_info           - 1, if prediction not used in this frame 
		                       > 1 otherwise
                 weight_idx          - 3 bit number indicating the LTP coefficient 
		                       in  the codebook 
                 sfb_prediction_used - 1 bit for each scalefactor band (sfb) 
                                       where LTP can be used indicating whether 
				       LTP is switched on (1) /off (0) in that 
				       sfb.
                 delay               - LTP lag
		write_flag           - 1 for writing, 0 for counting

  Output:	y - # of bits written

  References:	1.) BsPutBit

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

int
nok_ltp_encode (BsBitStream *bs, WINDOW_TYPE_EXT win_type, int num_of_sfb, 
                NOK_LT_PRED_STATUS *lt_status, int write_flag)
{
  int i, last_band;
  int bit_count;


  bit_count = 1;
  
  if (lt_status->global_pred_flag)
  {
    if(write_flag)
      BsPutBit (bs, 1, 1);    	/* LTP used */

    switch(win_type)
    {
      case ONLY_LONG_WINDOW:
      case LONG_SHORT_WINDOW:
      case SHORT_LONG_WINDOW:
        bit_count += LEN_LTP_LAG;
        bit_count += LEN_LTP_COEF;
        if(write_flag)
        {
          BsPutBit (bs, lt_status->delay[0], LEN_LTP_LAG);
          BsPutBit (bs, lt_status->weight_idx,  LEN_LTP_COEF);
        }

        last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	  num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;

        bit_count += last_band;
        if(write_flag)
          for (i = 0; i < last_band; i++)
            BsPutBit (bs, lt_status->sfb_prediction_used[i], LEN_LTP_LONG_USED);
        break;
        
      default:
        CommonExit(1, "nok_ltp_encode : unsupported window sequence %i", win_type);
        break;
    }
  }
  else
    if(write_flag)
      BsPutBit (bs, 0, 1);    	/* LTP not used */

  return (bit_count);
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
double_to_int (double sig_in)
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

#endif /* DO_LTP */
