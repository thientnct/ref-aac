/*******************************************************************************
MPEG-4 Audio VM
Pack gain_control_data()

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Takashi Koike (Sony Corporation)

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
*******************************************************************************/
#include	"sony_local.h"
#include	"bitstream.h"

/*******************************************************************************
	Pack gain_control_data
*******************************************************************************/
int	son_gc_pack(
	BsBitStream	*gc_stream,
	int	window_sequence,
	int	max_band,
	GAINC	*gcDataCh[]
	)
{
	int	i;
	int	bd, wd, ad;
	int	loc;

	loc = gc_stream->currentBit;
	BsPutBit(gc_stream, max_band, 2);

	switch (window_sequence) {
	case ONLY_LONG_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 1; wd++) {
				BsPutBit(gc_stream, gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_idgain[ad], 4);
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 5);
				}
			}
		}
		break;
	case LONG_START_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsPutBit(gc_stream, gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_idgain[ad], 4);
					if (wd == 0) {
						BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 4);
					}
					else {
						BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 2);
					}
				}
			}
		}
		break;
	case EIGHT_SHORT_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 8; wd++) {
				BsPutBit(gc_stream, gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_idgain[ad], 4);
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 2);
				}
			}
		}
		break;
	case LONG_STOP_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsPutBit(gc_stream, gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsPutBit(gc_stream, gcDataCh[bd][wd].a_idgain[ad], 4);
					if (wd == 0) {
						BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 4);
					}
					else {
						BsPutBit(gc_stream, gcDataCh[bd][wd].a_loc[ad], 5);
					}
				}
			}
		}
		break;
	default:
		return	-1;
		break;
	}
	return	gc_stream->currentBit - loc;
}
