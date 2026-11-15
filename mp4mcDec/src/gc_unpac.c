/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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

/* This file includes the functions for restoring the values for the gain
control tool from the encoded bitstream. */

#include "bitinput.h"
#include "gc.h"
#include "gc_unpac.h"
#include "chandefs.h"
#include "diagnose.h"

extern  GAIN_CONTROL_DATA   g_info[Chans];

/**************************************************************************
This function gets the n bits data from the encoded bitstream.
***************************************************************************/
static int get_value(int n)
{
	int i, value = 0;
	int tmp[100];
	
	for(i = 0; i < n; i++){
		tmp[i] = getbits(1);
	}
	for(i = 0; i < n; i++){
		if (tmp[n-(i+1)] == 1) {
			value += (int)pow(2.0,(double)i);
		}
	}
	return(value);
}

/**************************************************************************
This function decodes the number of maximum band from the encoded
bitstream.
***************************************************************************/
static void get_max_band_info(int ch)
{
	/* Max Band Information */
	g_info[ch].max_band = get_value(NBANDSBITS) + 1;
}

/**************************************************************************
This function decodes the gain data from the encoded bitstream.
***************************************************************************/
void unpac_gaincontrol_data(int ch, int window)
{

	int k,bd,wd;
	int block_loop = 0;
	int loc_type = 0;
	int loc_type2 = 0;

	/* get max band */
	get_max_band_info(ch);

	/* num_gain_data for lowest band equal 0 */
	for( wd = 0; wd < GC_BLOCKS_SHORT; wd++){
		g_info[ch].gInfo[0].gBlock[wd].num_gain_data = 0;
	}

	switch(window){
	  case	ONLY_LONG_SEQUENCE:
		block_loop = GC_BLOCKS_LONG;
		loc_type = ATKLOCBITS;
		break;
	  case	EIGHT_SHORT_SEQUENCE:
		block_loop = GC_BLOCKS_SHORT;
		loc_type = ATKLOCBITS_SHORT;
		break;
	  case LONG_START_SEQUENCE:
		block_loop = GC_BLOCKS_START;
		loc_type = ATKLOCBITS_START_A;
		loc_type2 = ATKLOCBITS_START_B;
		break;
	  case	LONG_STOP_SEQUENCE:
		block_loop = GC_BLOCKS_STOP;
		loc_type = ATKLOCBITS_STOP_A;
		loc_type2 = ATKLOCBITS_STOP_B;
		break;
	  default:
		exit(1);
	}

	for(bd = 1; bd < g_info[ch].max_band; bd++) {
		for(wd = 0; wd < block_loop; wd++){
			g_info[ch].gInfo[bd].gBlock[wd].num_gain_data = get_value(NATKSBITS);
			for(k = 0; k < g_info[ch].gInfo[bd].gBlock[wd].num_gain_data; k++){
                                gc_present=1;
				/* Gain Level Information */
				g_info[ch].gInfo[bd].gBlock[wd].gData[k].lev = get_value(IDGAINBITS);
				/* Gain Locate Information */
				if(window != EIGHT_SHORT_SEQUENCE){
					if(wd == 0){
						g_info[ch].gInfo[bd].gBlock[wd].gData[k].loc = get_value(loc_type);
					}
					else{
						g_info[ch].gInfo[bd].gBlock[wd].gData[k].loc = get_value(loc_type2);
					}
				}
				else{
					g_info[ch].gInfo[bd].gBlock[wd].gData[k].loc = get_value(loc_type);
				}
			}
		}
	}
} /* unpac_gaincontrol_data() */

/*****************************************************************************
                               End of gc_unpac.c		
*****************************************************************************/





