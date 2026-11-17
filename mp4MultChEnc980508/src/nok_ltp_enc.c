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
  $Revision: 1.9 $
  $Date: 1998-05-08 15:01:36+03 $ (check in)
  $Author: suonio $
  *************************************************************************/


#undef DO_LTP
#ifdef MPEG4_VM 
#define DO_LTP
#endif

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



/**************************************************************************
  Object Definitions
  *************************************************************************/



/**************************************************************************
  Title:	nok_init_lt_pred

  Purpose:	Initialize the history buffer for long term prediction

  Usage:        nok_init_lt_pred (lt_status)

  Input:	lt_status
  			- buffer: history buffer
                        - pred_mdct: prediction transformed to frequency 
                          domain
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

  Output:	lt_status
  			- buffer: filled with 0
                        - pred_mdct: filled with 0
                        - weight_idx : filled with 0
                        - sbk_prediction_used: filled with 0
                        - sfb_prediction_used: filled with 0
                        - delay: filled with 0
                        - side_info: filled with 1

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

  for (i = 0; i < NOK_MAX_BLOCK_LEN_LONG; i++)
    lt_status->pred_mdct[i] = 0;

  lt_status->weight_idx = 0;
  for(i=0; i<NSHORT; i++)
    lt_status->sbk_prediction_used[i] = lt_status->delay[i] = 0;

  for(i=0; i<MAX_SCFAC_BANDS; i++)
    lt_status->sfb_prediction_used[i] = 0;

  lt_status->side_info = LEN_LTP_DATA_PRESENT;
}



/**************************************************************************
  Title:	nok_ltp_enc

  Purpose:      Performs long term prediction.

  Usage:	nok_ltp_enc(p_spectrum, p_time_signal, win_type, win_shape, 
                            sfb_offset, num_of_sfb, lt_status, buffer_update)

  Input:        p_spectrum    - spectral coefficients
                p_time_signal - time domain input samples
                win_type      - window sequence (frame, block) type
                win_shape     - shape of the mdct window
                sfb_offset    - scalefactor band boundaries
                num_of_sfb    - number of scalefactor bands in each block

  Output:	p_spectrum    - residual spectrum
                lt_status     - buffer: history buffer
                              - pred_mdct:prediction transformed to frequency domain
                                for subsequent use
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
                        
  References:	1.) estimate_delay in nok_pitch.c
                2.) pitch in nok_pitch.c
                3.) buffer2freq
                4.) snr_pred in nok_pitch.c
                5.) freq2buffer
                6.) double_to_int

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

int
nok_ltp_enc(double *p_spectrum, double *p_time_signal, WINDOW_TYPE_EXT win_type, 
            Window_shape win_shape, int block_size_long, int block_size_medium,
	    int block_size_short, int *sfb_offset, int num_of_sfb,
            NOK_LT_PRED_STATUS *lt_status)
{
    int i, j;
    int last_band;
#ifndef MPEG4V1
    int op_delay, max_side_info, sw, msw;
    int temp_weight_idx[MAX_SHORT_WINDOWS];
    double max_bits, total_bit;
#endif /* not MPEG4V1 */
    double *time_signal;
    double num_bit[MAX_SHORT_WINDOWS];
    double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];
    double overlap_buffer[2 * NOK_MAX_BLOCK_LEN_LONG];
    static Window_shape win_shape_prev[2]={WS_FHG, WS_FHG};	/* YB : 971113 */


    lt_status->global_pred_flag = 0;
    lt_status->side_info = 1;

    switch(win_type)
    {
      case ONLY_LONG_WINDOW:
      case LONG_SHORT_WINDOW:
      case SHORT_LONG_WINDOW:
      case LONG_MEDIUM_WINDOW:
      case MEDIUM_LONG_WINDOW:
        last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;

        lt_status->delay[0] = estimate_delay (p_time_signal, lt_status->buffer, 2 * block_size_long);

        /*fprintf(stdout, "(LTP) lag : %i ", lt_status->delay[0]);*/

        pitch (p_time_signal, predicted_samples, lt_status->buffer, 
               &lt_status->weight_idx, lt_status->delay[0], 2 * block_size_long);

#ifndef MPEG4V1
        /* Transform prediction to frequency domain. */
        buffer2freq (predicted_samples, lt_status->pred_mdct, NULL, win_type,
                     win_shape, win_shape_prev[0], 
		     block_size_long, block_size_medium,
                     block_size_short, NON_OVERLAPPED, 1, 0);
#else
        /* Transform prediction to frequency domain and save it for subsequent use. */
        buffer2freq (predicted_samples, lt_status->pred_mdct, NULL, win_type,
                     win_shape, block_size_long, block_size_medium,
                     block_size_short, NON_OVERLAPPED);
#endif /* not MPEG4V1 */
        
        lt_status->side_info = LEN_LTP_DATA_PRESENT + last_band + LEN_LTP_LAG + LEN_LTP_COEF;
            
        num_bit[0] = snr_pred (p_spectrum, lt_status->pred_mdct,
                               lt_status->sfb_prediction_used, sfb_offset, 
                               win_type, lt_status->side_info, last_band);

        /*fprintf(stdout, " bit gain : %f\n", num_bit[0]);*/
          
        lt_status->global_pred_flag = (num_bit[0] == 0.0) ? 0 : 1;
	  
        if(lt_status->global_pred_flag)
          for (i = 0; i < sfb_offset[last_band]; i++)
            p_spectrum[i] -= lt_status->pred_mdct[i];
        else
          lt_status->side_info = 1;
        break;

      case ONLY_SHORT_WINDOW:
#ifndef MPEG4V1
        last_band = (num_of_sfb < NOK_MAX_LT_PRED_SHORT_SFB) ? num_of_sfb : NOK_MAX_LT_PRED_SHORT_SFB;

        total_bit = 0;
        for (sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
        {
          time_signal = p_time_signal + SHORT_SQ_OFFSET +  block_size_short * sw;
          lt_status->delay[sw] = estimate_delay (time_signal, lt_status->buffer, 2 * block_size_short);

          pitch (time_signal, predicted_samples, lt_status->buffer,
                 temp_weight_idx + sw, lt_status->delay[sw], 2 * block_size_short);

          /* Transform prediction to frequency domain. */
          buffer2freq (predicted_samples, lt_status->pred_mdct, NULL, win_type,
                       win_shape, win_shape_prev[0],
		       block_size_long, block_size_medium,
                       block_size_short, NON_OVERLAPPED, 1, 0);
                       
          lt_status->side_info = LEN_LTP_DATA_PRESENT + last_band + LEN_LTP_LAG + LEN_LTP_COEF;

          num_bit[sw] = snr_pred (p_spectrum + block_size_short * sw, lt_status->pred_mdct,
                                  lt_status->sfb_prediction_used + last_band * sw, 
                                  sfb_offset, win_type, lt_status->side_info, last_band);

          total_bit += num_bit[sw];

          /*fprintf (stdout, "idealsubbits : %2i%5i%10f\n", sw, lt_status->delay[sw], num_bit[sw]);*/
        }

        /*fprintf (stdout, "total idealsubbits %f\n\n", total_bit);*/

        max_bits = 0;
        for (sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
          if (max_bits <= num_bit[sw])
          {
            max_bits = num_bit[sw];
            msw = sw;
          }
        
        op_delay = lt_status->delay[msw];
        for (sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
        {
          double temp;
          
          temp = op_delay - lt_status->delay[sw] + NOK_LTP_LAG_OFFSET;
          if (temp > (2 * NOK_LTP_LAG_OFFSET - 1) || temp < 0)
            lt_status->delay[sw] = op_delay;
        }
        
        for(i = 0; i < MAX_SHORT_WINDOWS; i++)
          lt_status->sbk_prediction_used[i] = 0;
        
        lt_status->weight_idx = temp_weight_idx[msw];

        /* Global LTP prediction flag. */
        max_side_info = LEN_LTP_DATA_PRESENT;

        lt_status->global_pred_flag = 0;
        
        for (sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
        {
          if (!lt_status->global_pred_flag)
            lt_status->side_info = LEN_LTP_DATA_PRESENT + LEN_LTP_LAG + LEN_LTP_COEF;
          else
          {
            lt_status->side_info = LEN_LTP_DATA_PRESENT +  LEN_LTP_SHORT_LAG_PRESENT;
            if(lt_status->delay[sw-1] - lt_status->delay[sw])
              lt_status->side_info += LEN_LTP_SHORT_LAG;
          }
          
          prediction (lt_status->buffer, predicted_samples, &codebook[lt_status->weight_idx],
                      lt_status->delay[sw], 2 * block_size_short);

          /* Transform prediction to frequency domain. */
          buffer2freq (predicted_samples, lt_status->pred_mdct + block_size_short * sw, NULL, 
                       win_type, win_shape, win_shape_prev[0],
		       block_size_long, block_size_medium,
                       block_size_short, NON_OVERLAPPED, 1, 0);
                       
          num_bit[sw] = snr_pred (p_spectrum + block_size_short * sw, 
                                  lt_status->pred_mdct + block_size_short * sw,
                                  lt_status->sfb_prediction_used + last_band * sw,
                                  sfb_offset, win_type, lt_status->side_info, last_band);

          /*fprintf (stdout, "subbits : %2i%10f\n", sw, num_bit[sw]);*/

          if (num_bit[sw])
          {
            lt_status->global_pred_flag = 1;
            lt_status->sbk_prediction_used[sw] = 1;
            max_side_info +=lt_status->side_info;
          }
          else
            if(lt_status->global_pred_flag)
            {
              lt_status->sbk_prediction_used[sw] = 0;
              max_side_info += LEN_LTP_DATA_PRESENT;
            }
          
          if(lt_status->sbk_prediction_used[sw])
            for (i = 0; i < sfb_offset[last_band]; i++)
              p_spectrum[sw * block_size_short + i] -= lt_status->pred_mdct[sw * block_size_short + i];
        }
        
        total_bit = 0.0;
        for (i = 0; i < MAX_SHORT_WINDOWS; i++)
          total_bit += num_bit[i];
        
        if (lt_status->global_pred_flag)
          lt_status->side_info = max_side_info;
        else
          lt_status->side_info = LEN_LTP_DATA_PRESENT;

        /*fprintf (stdout, "total subbits %f\n\n", total_bit);*/

#endif /* not MPEG4V1 */
        break;
	
      default:
        break;
    }
    
    return (lt_status->global_pred_flag);
}


/**************************************************************************
  Title:	nok_ltp_reconstruct

  Purpose:      Updates LTP history buffer.

  Usage:	nok_ltp_reconstruct(p_spectrum, win_type,  win_shape, 
                                    block_size_long, block_size_medium,
                                    block_size_short, sfb_offset, 
                                    num_of_sfb, lt_status)

  Input:        p_spectrum    - reconstructed spectrum
                win_type      - window sequence (frame, block) type
                win_shape     - shape of the mdct window
                sfb_offset    - scalefactor band boundaries
                num_of_sfb    - number of scalefactor bands in each block

  Output:	p_spectrum    - reconstructed spectrum
                lt_status     - buffer: history buffer
                           
  References:	1.) buffer2freq
                2.) freq2buffer
                3.) double_to_int

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

void
nok_ltp_reconstruct(double *p_spectrum, WINDOW_TYPE_EXT win_type, 
                    Window_shape win_shape,
		    int block_size_long, int block_size_medium,
                    int block_size_short, int *sfb_offset, int num_of_sfb,
                    NOK_LT_PRED_STATUS *lt_status)
{
  int i, j, sw, last_band;
  double predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];
  double overlap_buffer[2 * NOK_MAX_BLOCK_LEN_LONG];

    
  switch(win_type)
  {
    case ONLY_LONG_WINDOW:
    case LONG_SHORT_WINDOW:
    case SHORT_LONG_WINDOW:
    case LONG_MEDIUM_WINDOW:
    case MEDIUM_LONG_WINDOW:
      last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
      
      if(lt_status->global_pred_flag)
        for (i = 0; i < sfb_offset[last_band]; i++)
          p_spectrum[i] += lt_status->pred_mdct[i];

      /* Finally update the time domain history buffer. */
#ifndef MPEG4V1
      freq2buffer (p_spectrum, predicted_samples, overlap_buffer, win_type,
                   block_size_long, block_size_medium, block_size_short,
                   win_shape, WS_FHG, NON_OVERLAPPED, MAX_SHORT_WINDOWS);
#else
      freq2buffer (p_spectrum, predicted_samples, overlap_buffer, win_type,
                   block_size_long, block_size_medium, block_size_short,
                   win_shape, NON_OVERLAPPED);
#endif /* not MPEG4V1 */
      
      for (i = 0; i < NOK_LT_BLEN - block_size_long; i++)
        lt_status->buffer[i] = lt_status->buffer[i + block_size_long];
      
      j = NOK_LT_BLEN - 2 * block_size_long;
      for (i = 0; i < block_size_long; i++)
      {
        lt_status->buffer[i + j] =
            double_to_int (predicted_samples[i] + lt_status->buffer[i + j]);
        lt_status->buffer[NOK_LT_BLEN - block_size_long + i] =
            double_to_int (predicted_samples[i + block_size_long]);
      }
      break;
      
    case ONLY_SHORT_WINDOW:
      for (i = 0; i < NOK_LT_BLEN - block_size_long; i++)
        lt_status->buffer[i] = lt_status->buffer[i + block_size_long];
      
      for (i = NOK_LT_BLEN - block_size_long; i < NOK_LT_BLEN; i++)
        lt_status->buffer[i] = 0;

#ifndef MPEG4V1
      for(sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
        if(lt_status->sbk_prediction_used[sw])
          for (i = 0; i < sfb_offset[last_band]; i++)
            p_spectrum[sw * block_size_short + i] += lt_status->pred_mdct[sw * block_size_short + i];

      /* Finally update the time domain history buffer. */
      freq2buffer (p_spectrum, predicted_samples, overlap_buffer, win_type, block_size_long, 
                   block_size_medium, block_size_short, win_shape, WS_FHG, NON_OVERLAPPED, 
                   MAX_SHORT_WINDOWS);
#else
      /* Finally update the time domain history buffer. */
      freq2buffer (p_spectrum, predicted_samples, overlap_buffer, win_type, block_size_long, 
                   block_size_medium, block_size_short, win_shape, NON_OVERLAPPED);

#endif /* not MPEG4V1 */

      for(sw = 0; sw < MAX_SHORT_WINDOWS; sw++)
      {
        i = NOK_LT_BLEN - 2 * block_size_long + SHORT_SQ_OFFSET + sw * block_size_short;
        for (j = 0; j < 2 * block_size_short; j++)
          lt_status->buffer[i + j] = double_to_int (predicted_samples[sw * block_size_short * 2 + j] + 
                                                    lt_status->buffer[i + j]);
      }
      break;
      
    default:
      break;
  }

  return;
}                      


/**************************************************************************
  Title:	nok_ltp_encode

  Purpose:      Writes LTP parameters to the bit stream.

  Usage:	nok_ltp_encode (bs, win_type, num_of_sfb, lt_status)

  Input:        bs         - bit stream
                win_type   - window sequence (frame, block) type
                num_of_sfb - number of scalefactor bands
                lt_status  - side_info:
			     1, if prediction not used in this frame
			     >1 otherwise
                           - weight_idx : 
                             3 bit number indicating the LTP coefficient in 
                             the codebook 
                           - sfb_prediction_used:
                             1 bit for each scalefactor band (sfb) where LTP 
                             can be used indicating whether LTP is switched 
                             on (1) /off (0) in that sfb.
                           - delay: LTP lag

  Output:	-

  References:	1.) BsPutBit

  Explanation:  -

  Author(s):	Juha Ojanpera
  *************************************************************************/

int
nok_ltp_encode (BsBitStream *bs, WINDOW_TYPE_EXT win_type, int num_of_sfb, 
                NOK_LT_PRED_STATUS *lt_status, int write_flag)
{
  int i, j, last_band;
  int first_subblock;
  int prev_subblock;
  int bit_count = 0;


  bit_count += 1;
  
  if (lt_status->side_info > 1)
  {
    if(write_flag)
      BsPutBit (bs, 1, 1);    	/* LTP used */

    switch(win_type)
    {
      case ONLY_LONG_WINDOW:
      case LONG_SHORT_WINDOW:
      case SHORT_LONG_WINDOW:
      case LONG_MEDIUM_WINDOW:
      case MEDIUM_LONG_WINDOW:
        bit_count += LEN_LTP_LAG;
        bit_count += LEN_LTP_COEF;
        if(write_flag)
        {
          BsPutBit (bs, lt_status->delay[0], LEN_LTP_LAG);
          BsPutBit (bs, lt_status->weight_idx,  LEN_LTP_COEF);
        }

        last_band = (num_of_sfb < NOK_MAX_LT_PRED_LONG_SFB) ? num_of_sfb : NOK_MAX_LT_PRED_LONG_SFB;
        bit_count += last_band;
        if(write_flag)
        {
          for (i = 0; i < last_band; i++)
            BsPutBit (bs, lt_status->sfb_prediction_used[i], LEN_LTP_LONG_USED);
        }
        break;
        
      case ONLY_SHORT_WINDOW:
        for(i=0; i < MAX_SHORT_WINDOWS; i++)
        {
          if(lt_status->sbk_prediction_used[i])
          {
            first_subblock = i;
            break;
          }
        }
        bit_count += LEN_LTP_LAG;
        bit_count += LEN_LTP_COEF;
        
        if(write_flag)
        {
          BsPutBit (bs, lt_status->delay[first_subblock], LEN_LTP_LAG);
          BsPutBit (bs, lt_status->weight_idx,  LEN_LTP_COEF);
        }

        prev_subblock = first_subblock;
        for(i = 0; i < MAX_SHORT_WINDOWS; i++)
        {
          bit_count += LEN_LTP_SHORT_USED;
          if(write_flag)
            BsPutBit (bs, lt_status->sbk_prediction_used[i], LEN_LTP_SHORT_USED);

          if(lt_status->sbk_prediction_used[i])
          {
            if(i > first_subblock)
            {
              int diff;
            
              diff = lt_status->delay[prev_subblock] - lt_status->delay[i];
              if(diff)
              {
                bit_count += 1;
                bit_count += LEN_LTP_SHORT_LAG;
                if(write_flag)
                {
                  BsPutBit (bs, 1, 1);
                  BsPutBit (bs, diff + NOK_LTP_LAG_OFFSET, LEN_LTP_SHORT_LAG);
                }
              }
              else
              {
                bit_count += 1;
                if(write_flag)
                  BsPutBit (bs, 0, 1);
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

#endif
