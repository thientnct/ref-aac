/*
  "This software module was originally developed by

  Yoshiaki Oikawa (Sony Corporation) and
  Mitsuyuki Hatanaka (Sony Corporation) 

  in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
  Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
  software module is an implementation of a part of one or more MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
  MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
  gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards free license to this software module or modifications
  thereof for use in hardware or software products claiming conformance
  to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards. Those intending to use this software module in hardware or
  software products are advised that this use may infringe existing
  patents. The original developer of this software module and his/her
  company, the subsequent editors and their companies, and ISO/IEC have
  no liability for use of this software module or modifications thereof
  in an implementation. Copyright is not released for non MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
  original developer retains full right to use the code for his/her own
  purpose, assign or donate the code to a third party and to inhibit
  third party from using the code for non MPEG-2 AAC/MPEG-4
  System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
  notice must be included in all copies or derivative works."
  Copyright (C) 1996.
*/
#ifdef	SSR

/* This file includes the functions for restoring the values for the gain
control tool from the encoded bitstream. */

#include "all.h"
#include "mdct_fhg_1024.h"

extern  GAIN_CONTROL_DATA   g_info[MAX_CHANNELS];

/**************************************************************************
This function gets the n bits data from the encoded bitstream.
***************************************************************************/
int
get_value(int n)
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
void
get_max_band_info(int ch)
{
	/* Max Band Information */
	g_info[ch].max_band = get_value(NBANDSBITS) + 1;
}

/**************************************************************************
This function decodes the gain data from the encoded bitstream.
***************************************************************************/
void
unpac_gaincontrol_data(int ch, int window)
{

	int k,bd,wd;
	int block_loop;
	int loc_type;
	int loc_type2;

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

#endif
/*****************************************************************************
                               End of gc_unpac.c		
*****************************************************************************/





