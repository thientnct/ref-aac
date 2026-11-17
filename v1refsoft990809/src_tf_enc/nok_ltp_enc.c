/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works.
Copyright (c)1997.  

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
  $Revision: 1.15 $
  $Date: 1999/04/23 16:02:07 $ (check in)
  $Author: purnhage $
  *************************************************************************/


/**************************************************************************
  External Objects Needed
  *************************************************************************/
/*
  Standard library declarations.  */
#include <stdio.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
/*
  Interface to related modules.  */
#include "common_m4a.h"	/* HP 980211 */
#include "nttNew.h"
#include "tf_main.h"
#include "interface.h"
#include "bitstream.h"
#include "nok_ltp_common.h"
#include "nok_pitch.h"
#include "tns3.h"

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
  Call:		buffer2freq (p_in_data, p_out_mdct, p_overlap, windowSequence,
  			     wfun_select, nlong, nmed, nshort, overlap_select,
			     num_short_win, save_window);
  Purpose:	Modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: 2*shift length)
		p_overlap[]	- buffer containing the previous frame
				  in OVERLAPPED_MODE (length: 2*shift length)
		windowSequence	- selects the type of window to use
  		wfun_select	- select window function
  		nlong		- shift length for long windows
                nmed		- shift length for medium windows
		nshort		- shift length for short windows
		overlap_select	- OVERLAPPED_MODE select
		num_short_win	- number of short windows to transform
		save_window	- if true, save the windowSequence
				  for future use
  Output:	p_out_mdct[]	- transformed signal (length: shift length)
		p_overlap[]	- a copy of p_in_data in OVERLAPPED_MODE
  				  (length: shift length)
  Explanation:	-  */

/*
  Function:	freq2buffer
  Call:		freq2buffer (p_in_data, p_out_data, p_overlap, windowSequence,
  			     nlong, nmed, nshort, wfun_select, fun_select_prev,
			     overlap_select, num_short_win);
  Purpose:	Inverse of modified discrete cosine transform
  Input:	p_in_data[]	- input signal (length: shift length)
		p_overlap[]	- the overlap buffer; does not have
				  an effect in NON_OVERLAPPED_MODE,
				  but have to be allocated
				  (length: shift length)
		windowSequence	- selects the type of window to use
  		nlong		- shift length for long windows
                nmed		- shift length for medium windows
		nshort		- shift length for short windows
  		wfun_select	- select window function
		overlap_select	- OVERLAPPED_MODE select
		num_short_win	- number of short windows to transform
 		save_window	- if true, save windowSequence for future use
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
  Purpose:	Encoding mode of short blocks.
  Explanation:	- */
/*#define NOK_LTP_SHORT_FAST*/

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

  Input:	lt_status :
  		 buffer              - history buffer
		 weight_idx          - 3 bit number indicating the LTP 
		                       coefficient in the codebook 
                 sbk_prediction_used - 1 bit for each subblock indicating 
		                       whether LTP is used in that subblock 
                 sfb_prediction_used - 1 bit for each scalefactor band (sfb) 
                                       where LTP  can be used indicating 
				       whether LTP is switched on (1) /off (0)
				       in that sfb.
                 delay               - LTP lag
                 side_info           - LTP side information
		 mdct_predicted      - predicted spectrum

  Output:	lt_status :
  		 buffer              - filled with 0
		 weight_idx          - filled with 0
		 sbk_prediction_used - filled with 0
		 sfb_prediction_used - filled with 0
		 delay               - filled with 0
		 side_info           - filled with 1
		 mdct_predicted      - filled with 0

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

  for(i=0; i<MAXBANDS; i++)
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
			   block_size_long, block_size_medium, 
			   block_size_short, sfb_offset, num_of_sfb, 
			   last_band, side_info, sfb_prediction_used, 
			   tnsInfo, sw)

  Input:        p_spectrum        - spectral coefficients
                predicted_samples - predicted time domain samples	       
                win_type          - window sequence (frame, block) type
                win_shape         - shape of the mdct window
		win_shape_prev    - shape from previous block
		block_size_long   - size of the long block
		block_size_medium - size of the medium block
		block_size_short  - size of the short block
                sfb_offset        - scalefactor band boundaries
                num_of_sfb        - number of scalefactor bands in each block
		last_band         - last scalefactor band where LTP is used
		side_info         - LTP side information bits
		tnsInfo           - TNS encoding parameters
		sw                - subblock number

  Output:	y                   - prediction gain
                mdct_predicted      - predicted spectrum
                p_spectrum          - error spectrum (if LTP used)
		sfb_prediction_used - 1 bit for each scalefactor band (sfb) 
		                      where LTP can be used indicating whether 
				      LTP is switched on (1) /off (0) in 
				      that sfb.
                        
  References:	1.) buffer2freq in imdct.c
                2.) TnsEncode2 in tns3.c
		3.) TnsEncodeShortBlock in tns3.c
                4.) snr_pred in nok_pitch.c

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

static double
ltp_enc_tf(double *p_spectrum, double *predicted_samples, 
	   double *mdct_predicted, WINDOW_SEQUENCE win_type, 
	   WINDOW_SHAPE win_shape, WINDOW_SHAPE win_shape_prev, 
	   int block_size_long, int block_size_medium, int block_size_short, 
	   int *sfb_offset, int num_of_sfb, int last_band, int side_info, 
	   int *sfb_prediction_used, TNS_INFO *tnsInfo, int sw)
{
  int frame_len;
  double bit_gain;
  

  /* Transform prediction to frequency domain. */
  buffer2freq (predicted_samples, mdct_predicted, NULL, 
               win_type, win_shape, win_shape_prev,
	       block_size_long, block_size_medium, block_size_short, 
	       NON_OVERLAPPED_MODE, 1);
  
  /* Apply TNS analysis filter to the predicted spectrum. */
  if(tnsInfo != NULL)
  {
    if(win_type != EIGHT_SHORT_SEQUENCE)
      TnsEncode2(num_of_sfb, num_of_sfb, win_type, sfb_offset, 
		 mdct_predicted, tnsInfo, 0);
    else
      /* Filter this subblock. */
      TnsEncodeShortBlock(/**TM last_band, last_band,*/
			  num_of_sfb, num_of_sfb, sw, sfb_offset, 
			  mdct_predicted, tnsInfo);
  }

  if(win_type != EIGHT_SHORT_SEQUENCE)
    frame_len = block_size_long;
  else
    frame_len = block_size_short;
  
  /* Get the prediction gain. */
  bit_gain = snr_pred (p_spectrum, mdct_predicted, sfb_prediction_used, 
		       sfb_offset, win_type, side_info, last_band, frame_len);
  return (bit_gain);
}


/**************************************************************************
  Title:	nok_ltp_enc

  Purpose:      Performs long term prediction.

  Usage:	y = nok_ltp_enc(p_spectrum, p_time_signal, win_type, win_shape,
                                win_shape_prev, block_size_long, 
				block_size_medium, block_size_short, 
				sfb_offset, num_of_sfb, lt_status, tnsInfo, 
				qc_select)

  Input:        p_spectrum        - spectral coefficients
                p_time_signal     - current input samples
                win_type          - window sequence (frame, block) type
                win_shape         - shape of the mdct window
		win_shape_prev    - shape from previous block
		block_size_long   - size of the long block
		block_size_medium - size of the medium block
		block_size_short  - size of the short block
                sfb_offset        - scalefactor band boundaries
                num_of_sfb        - number of scalefactor bands in each block
		lt_status :
		 buffer           - history buffer
		tnsInfo           - TNS encoding parameters
		qc_select         - MPEG4/TF codec type

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
nok_ltp_enc(double *p_spectrum, double *p_time_signal, 
	    WINDOW_SEQUENCE win_type, WINDOW_SHAPE win_shape, 
	    WINDOW_SHAPE win_shape_prev, int block_size_long, 
	    int block_size_medium, int block_size_short, int *sfb_offset, 
	    int num_of_sfb, NOK_LT_PRED_STATUS *lt_status, TNS_INFO *tnsInfo, 
	    QC_MOD_SELECT qc_select)
{
  int i, j, sw, sw_ltp_on;
  int first_sblck_found, lag0, lag1;
  int last_band, max_side_info;
  int weight_idx;
  Float weight_tmp;
  double max_bits, max_gain;
  double *time_signal;
  double num_bit[MAX_SHORT_WINDOWS];
  double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];
  double overlap_buffer[2 * NOK_MAX_BLOCK_LEN_LONG];

  lt_status->global_pred_flag = 0;
  lt_status->side_info = 0;
  
  switch(win_type)
  {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
      last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
      

      lt_status->delay[0] = 
	pitch(p_time_signal, lt_status->buffer, 2 * block_size_long, 
	      0, 2 * block_size_long, predicted_samples, &lt_status->weight, 
	      &lt_status->weight_idx, qc_select, win_type, -1);
      
	lt_status->side_info = LEN_LTP_DATA_PRESENT + last_band + 
	  LEN_LTP_LAG + LEN_LTP_COEF;

      num_bit[0] = 
	ltp_enc_tf(p_spectrum, predicted_samples, lt_status->mdct_predicted, 
		   win_type, win_shape, win_shape_prev, block_size_long, 
		   block_size_medium, block_size_short, sfb_offset, num_of_sfb,
		   last_band, lt_status->side_info,
		   lt_status->sfb_prediction_used, tnsInfo, -1);

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

      for(i = sfb_offset[last_band]; i < block_size_long; i++)
	lt_status->mdct_predicted[i] = 0.0f;

      
      break;
      
    case EIGHT_SHORT_SEQUENCE:
      /* Short windows are used only in a scalable mode. */
      if(qc_select == AAC_QC || qc_select == AAC_BSAC)
	return (0);

      last_band = (num_of_sfb < NOK_MAX_LT_PRED_SHORT_SFB) ? 
	num_of_sfb : NOK_MAX_LT_PRED_SHORT_SFB;
      
      for(i = 0; i < MAX_SHORT_WINDOWS; i++)
	lt_status->sbk_prediction_used[i] = 0;
      
      /* Global LTP prediction flag. */
      max_side_info = LEN_LTP_DATA_PRESENT;

      /* Initial lag parameters. */
      lag0 = 0;
      lag1 = DELAY;

      first_sblck_found = 0;
      
      for (sw = sw_ltp_on = 0; sw < MAX_SHORT_WINDOWS; sw++)
      {
	time_signal = p_time_signal + SHORT_SQ_OFFSET + block_size_short * sw;
	
	/* Compute the lag range for this subblock. */
	if(lt_status->global_pred_flag)
	{
	  lag0 = lt_status->delay[sw_ltp_on] - (NOK_LTP_LAG_OFFSET >> 1);
	  if(lag0 < 0)
	    lag0 = 0;
	  
	  lag1 = lag0 + NOK_LTP_LAG_OFFSET - 1;
	  if(lag1 > DELAY)
	    lag1 = DELAY;
	}
	
	if(lt_status->global_pred_flag == 0)
	{
	  /* Compute the lag, gain and get the predicted signal. */
          { int ii;
           for(ii=0; ii<2*block_size_long; ii++)predicted_samples[ii]= 0.0;
          }
	  lt_status->delay[sw] = 
	    pitch(time_signal, lt_status->buffer, 2 * block_size_short,
		  DELAY / 2 - 2 * block_size_short, 
		  2 * block_size_long + DELAY / 2 - 2 * block_size_short, 
		  predicted_samples, &lt_status->weight, &weight_idx, 
		  qc_select, win_type, first_sblck_found);
	  
	  /* Common to all subblocks. */
	  lt_status->weight_idx = weight_idx;
	  
	  /* Side information bits needed for this subblock. */
	  lt_status->side_info = 
	    LEN_LTP_DATA_PRESENT + LEN_LTP_LAG + LEN_LTP_COEF;
	  
	  num_bit[sw] = 
	    ltp_enc_tf(p_spectrum + block_size_short * sw, predicted_samples, 
		       lt_status->mdct_predicted + block_size_short * sw, 
		       win_type, win_shape, win_shape_prev, block_size_long, 
		       block_size_medium, block_size_short, sfb_offset, 
		       num_of_sfb, last_band, lt_status->side_info, 
		       lt_status->sfb_prediction_used + last_band * sw,
		       tnsInfo, sw);
	}
	else
	{
	  lt_status->side_info = 
	    LEN_LTP_DATA_PRESENT + LEN_LTP_SHORT_LAG_PRESENT;
	  
#ifdef NOK_LTP_SHORT_FAST
	  /* 
	   * The purpose of this function call here is to find the lag
	   * within the allowable lag range. The gain and the predicted
	   * frame are ignored.
	   */
	  lt_status->delay[sw] = 
	    pitch(time_signal, lt_status->buffer, 2 * block_size_short, 
		  lag0, lag1, predicted_samples, &weight_tmp, 
		  &weight_idx, qc_select, win_type, first_sblck_found);
	  
	  /* Just get the predicted signal, gain is already computed. */
	  prediction (lt_status->buffer, predicted_samples, 
		      &codebook[lt_status->weight_idx], lt_status->delay[sw], 
		      2 * block_size_short, qc_select, win_type);
	  
	  if(lt_status->delay[sw_ltp_on] - lt_status->delay[sw])
	    lt_status->side_info += LEN_LTP_SHORT_LAG;
	  
#else
	  /* 
	   * Try all possible lags and choose the lag that gives the best 
	   * output in terms of prediction gain. 
	   */
	  for(i = lag0, max_gain = 0.0; i < lag1; i++)
	  {
	    lt_status->side_info = 
	      LEN_LTP_DATA_PRESENT + LEN_LTP_SHORT_LAG_PRESENT;
	    if(lt_status->delay[sw_ltp_on] - i)
	      lt_status->side_info += LEN_LTP_SHORT_LAG;
	    
	    prediction (lt_status->buffer, predicted_samples, 
			&codebook[lt_status->weight_idx], i, 
			2 * block_size_short, qc_select, win_type);
	    
#endif /* NOK_LTP_SHORT_FAST */
	    
	    num_bit[sw] = 
	      ltp_enc_tf(p_spectrum + block_size_short * sw, predicted_samples,
			 lt_status->mdct_predicted + block_size_short * sw, 
			 win_type, win_shape, win_shape_prev, block_size_long,
			 block_size_medium, block_size_short, sfb_offset, 
			 num_of_sfb, last_band, lt_status->side_info, 
			 lt_status->sfb_prediction_used + last_band * sw,
			 tnsInfo, sw);

#ifndef NOK_LTP_SHORT_FAST	    
	    if(num_bit[sw] >= max_gain) /***TM  */
	    {
	      max_gain = num_bit[sw];
	      lt_status->delay[sw] = i;
	      for(j = 0; j < block_size_short; j++)
		overlap_buffer[j] =  
		  lt_status->mdct_predicted[block_size_short * sw + j];
	    }
	  }
	  
	  num_bit[sw] = max_gain;
	  for(j = 0; j < block_size_short; j++)
	    lt_status->mdct_predicted[block_size_short * sw + j] = 
	      overlap_buffer[j];

#endif /* not NOK_LTP_SHORT_FAST */

	} /* else */

	/**/
#ifdef LTP_DEBUG
	  fprintf(stdout, "%i. subblock parameters for LTP :\n", sw);
	  fprintf(stdout, "gain : %f\n", codebook[lt_status->weight_idx]);
	  fprintf(stdout, "lag : %i\n", lt_status->delay[sw]);
	  fprintf(stdout, "sfb flags (%i bands): ", last_band);
	  for(i = 0; i < last_band; i++)
	  fprintf(stdout, "%i", lt_status->sfb_prediction_used[last_band * sw + i]);
	  fprintf(stdout, "\n");
	  fprintf (stdout, "prediction gain : %10f\n", num_bit[sw]);
#endif /* LTP_DEBUG */
	
	if (num_bit[sw])
	{
	  first_sblck_found = 1;
	  lt_status->global_pred_flag = 1;
	  lt_status->sbk_prediction_used[sw] = 1;
	  max_side_info +=lt_status->side_info;
	  sw_ltp_on = sw;
	}
	else
	  max_side_info += LEN_LTP_DATA_PRESENT;
	
	if(lt_status->sbk_prediction_used[sw])
	  for (i = 0; i < sfb_offset[last_band]; i++)
	    p_spectrum[sw * block_size_short + i] -= 
	      lt_status->mdct_predicted[sw * block_size_short + i];
	
	for(i = sfb_offset[last_band]; i < block_size_short; i++)
	  lt_status->mdct_predicted[sw * block_size_short + i] = 0.0f;

      } /* for (sw = sw_ltp_on = 0 ... */
      
      if (lt_status->global_pred_flag)
	lt_status->side_info = max_side_info;
      else
	lt_status->side_info = LEN_LTP_DATA_PRESENT;
#ifdef LTP_DEBUG
      {
	double total_bit;
	
	total_bit = 0.0;
	for (i = 0; i < MAX_SHORT_WINDOWS; i++)
	  total_bit += num_bit[i];
	fprintf (stdout, "total subbits %f\n\n", total_bit);
      }
#endif /* LTP_DEBUG */
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
		                    num_of_sfb, block_size_short, lt_status)
				   
  Input:	p_spectrum       - reconstructed (error) spectrum
		win_type         - window sequence (frame, block) type
		sfb_offset       - scalefactor band boundaries
                num_of_sfb       - number of scalefactor bands in each block
		block_size_short - size of the short block
		lt_status :
		 mdct_predicted  - predicted spectrum

  Output:	p_spectrum       - reconstructed spectrum containing also 
                                   LTP contribution

  References:	-

  Explanation:	-

  Author(s):	Juha Ojanpera
  *************************************************************************/

void
nok_ltp_reconstruct(double *p_spectrum, WINDOW_SEQUENCE win_type, 
		    int *sfb_offset, int num_of_sfb, int block_size_short, 
		    NOK_LT_PRED_STATUS *lt_status)
{
  int i, sw;
  int last_band;


  if(lt_status->global_pred_flag)
  {
    switch(win_type)
    {
      case ONLY_LONG_SEQUENCE:
      case LONG_START_SEQUENCE:
      case LONG_STOP_SEQUENCE:
	last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	  num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
	  
	for (i = 0; i < sfb_offset[last_band]; i++)
	  p_spectrum[i] += lt_status->mdct_predicted[i];
	break;
	
      case EIGHT_SHORT_SEQUENCE:
	last_band = (num_of_sfb < NOK_MAX_LT_PRED_SHORT_SFB) ? 
	  num_of_sfb : NOK_MAX_LT_PRED_SHORT_SFB;
	
	for(sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
	  if(lt_status->sbk_prediction_used[sw])
	    for (i = 0; i < sfb_offset[last_band]; i++)
	      p_spectrum[sw * block_size_short + i] += 
		lt_status->mdct_predicted[sw * block_size_short + i];
	break;
	
      default:
	break;
    }
  }
  
}


/**************************************************************************
  Title:        nok_ltp_update

  Purpose:	Updates the time domain buffer of LTP.

  Usage:        nok_ltp_update(p_spectrum, win_type, win_shape, win_shape_prev,
                               block_size_long, block_size_medium, 
			       block_size_short, lt_status)
				   
  Input:	p_spectrum        - reconstructed spectrum
		win_type          - window sequence (frame, block) type
		win_shape         - shape of the mdct window
  		win_shape_prev    - shape from previous block
		block_size_long   - size of the long block
		block_size_medium - size of the medium block
		block_size_short  - size of the short block

  Output:	lt_status :
		 buffer           - history buffer for prediction

  References:	1.) freq2buffer in imdct.c
		2.) double_to_int

  Explanation:	-

  Author(s):	Juha Ojanpera
  *************************************************************************/

void
nok_ltp_update(double *p_spectrum, WINDOW_SEQUENCE win_type, 
	       WINDOW_SHAPE win_shape, WINDOW_SHAPE win_shape_prev, 
	       int block_size_long, int block_size_medium, 
	       int block_size_short, NOK_LT_PRED_STATUS *lt_status)
{
  int i, j, sw;
  double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];
  double overlap_buffer[2 * NOK_MAX_BLOCK_LEN_LONG];


  for (i = 0; i < NOK_LT_BLEN - block_size_long; i++)
    lt_status->buffer[i] = lt_status->buffer[i + block_size_long];

  switch(win_type)
  {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
      /* Finally update the time domain history buffer. */
       for (i = 0; i < block_size_long; i++){  /***TM  */
         overlap_buffer[i] = 0.;
         overlap_buffer[i+block_size_long] = 0.;
         predicted_samples[i] = 0.;
         predicted_samples[i+block_size_long] = 0.;
       }
       freq2buffer (p_spectrum, predicted_samples, overlap_buffer, win_type,
		   block_size_long, block_size_medium, block_size_short,
		   win_shape, win_shape_prev, NON_OVERLAPPED_MODE, -1);

      j = NOK_LT_BLEN - 2 * block_size_long;
      
      for (i = 0; i < block_size_long; i++)
      {
	lt_status->buffer[i + j] =
	  double_to_int (predicted_samples[i] + lt_status->buffer[i + j]);
	lt_status->buffer[NOK_LT_BLEN - block_size_long + i] =
	  double_to_int (predicted_samples[i + block_size_long]);
      }
      break;
      
    case EIGHT_SHORT_SEQUENCE:
      for (i = NOK_LT_BLEN - block_size_long; i < NOK_LT_BLEN; i++)
	lt_status->buffer[i] = 0;
      
      for(sw = 0; sw < MAX_SHORT_WINDOWS; sw++) {  /***TM */
         for (i = 0; i < block_size_short; i++){
           overlap_buffer[i+ sw *2* block_size_short] = 0.;
           overlap_buffer[i+ sw *2* block_size_short+block_size_short] = 0.;
	   predicted_samples[i+ sw *2* block_size_short] = 0.;
           predicted_samples[i+ sw *2* block_size_short+block_size_short] = 0.;
         }
      }

      for(sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
      {
	/* Finally update the time domain history buffer. */
	freq2buffer (p_spectrum + sw * block_size_short, predicted_samples, 
		     overlap_buffer, win_type, block_size_long, 
		     block_size_medium, block_size_short, win_shape, 
		     win_shape_prev, NON_OVERLAPPED_MODE, 1);
	
	i = NOK_LT_BLEN - 2 * block_size_long + SHORT_SQ_OFFSET + 
	  sw * block_size_short;
	
	for (j = 0; j < 2 * block_size_short; j++)
	  lt_status->buffer[i + j] = 
	    double_to_int (predicted_samples[j] + lt_status->buffer[i + j]);
      }
      break;
      
    default:
      break;
  }

}


/**************************************************************************
  Title:	nok_ltp_encode

  Purpose:      Writes LTP parameters to the bit stream.

  Usage:	nok_ltp_encode (bs, win_type, num_of_sfb, lt_status, qc_select,
		                coreCodecIdx, max_sfb_tvq_set)

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
		qc_select            - MPEG4/TF codec type
		max_sfb_tvq_set      - 1 if max_sfb need to be written into the
		                       bit stream, 0 otherwise

  Output:	-

  References:	1.) BsPutBit

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

int
nok_ltp_encode (BsBitStream *bs, WINDOW_SEQUENCE win_type, int num_of_sfb, 
                NOK_LT_PRED_STATUS *lt_status, QC_MOD_SELECT qc_select,
		enum CORE_CODEC coreCodecIdx, int max_sfb_tvq_set)

{
  int i, last_band;
  int first_subblock;
  int prev_subblock;
  int bits_written;
  
  bits_written = 1;

  if (lt_status->global_pred_flag)
  {
    BsPutBit (bs, 1, 1);    	/* LTP used */

    if(coreCodecIdx ==  NTT_TVQ && !max_sfb_tvq_set)
    {
      BsPutBit (bs, num_of_sfb, 6);    	/* max sfb. */
      bits_written += 6;
    }
    
    switch(win_type)
    {
      case ONLY_LONG_SEQUENCE:
      case LONG_START_SEQUENCE:
      case LONG_STOP_SEQUENCE:
	{
          BsPutBit (bs, lt_status->delay[0], LEN_LTP_LAG);
	  bits_written += LEN_LTP_LAG;
	}

        BsPutBit (bs, lt_status->weight_idx,  LEN_LTP_COEF);
	
	bits_written += LEN_LTP_COEF;

        last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? 
	  num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
        for (i = 0; i < last_band; i++)
          BsPutBit (bs, lt_status->sfb_prediction_used[i], LEN_LTP_LONG_USED);

	 bits_written += last_band;
        break;
        
      case EIGHT_SHORT_SEQUENCE:
        for(i=0; i < MAX_SHORT_WINDOWS; i++)
        {
          if(lt_status->sbk_prediction_used[i])
          {
            first_subblock = i;
            break;
          }
        }

        BsPutBit (bs, lt_status->delay[first_subblock], LEN_LTP_LAG);
        BsPutBit (bs, lt_status->weight_idx,  LEN_LTP_COEF);
	bits_written += LEN_LTP_LAG + LEN_LTP_COEF;

        prev_subblock = first_subblock;
        for(i = 0; i < MAX_SHORT_WINDOWS; i++)
        {
          BsPutBit (bs, lt_status->sbk_prediction_used[i], LEN_LTP_SHORT_USED);
	  bits_written += LEN_LTP_SHORT_USED;
          if(lt_status->sbk_prediction_used[i])
          {
            if(i > first_subblock)
            {
              int diff;
            
              diff = lt_status->delay[prev_subblock] - lt_status->delay[i];
	      prev_subblock = i;
              if(diff)
              {
                BsPutBit (bs, 1, 1);
                BsPutBit (bs, diff + NOK_LTP_LAG_OFFSET, LEN_LTP_SHORT_LAG);
		bits_written += 1 + LEN_LTP_SHORT_LAG;
              }
              else
	      {
                BsPutBit (bs, 0, 1);
		bits_written += 1;
	      }
            }
          }
        }  
        break;

      default:
        CommonExit(1, "nok_ltp_encode : unsupported window sequence %i", win_type);
        break;
    }
  }
  else
    BsPutBit (bs, 0, 1);    	/* LTP not used */
  
  return (bits_written);
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
  short sig_out;


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
