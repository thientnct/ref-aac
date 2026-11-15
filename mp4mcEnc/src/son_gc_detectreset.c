/*******************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yasuhiro Toguri (Sony Corporation)

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
*******************************************************************/

#include	"sony_local.h"
#include	<stdlib.h>
#include	<math.h>


void	son_gc_detect_reset(
	double	*bandSigChForGCAnalysis[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	GAINC	*g_info_curCh[]
	)
{
	int	i;
    int wd;

	for (i = 0; i < NBANDS; i++) {
		switch(window_sequence) {
		case EIGHT_SHORT_SEQUENCE:
			for(wd=0;wd< SHORT_WIN_IN_LONG;wd++)
				g_info_curCh[i][wd].natks = 0;
			break;
		case ONLY_LONG_SEQUENCE:
			g_info_curCh[i][0].natks = 0;
			break;
		case LONG_START_SEQUENCE:
			g_info_curCh[i][0].natks = 0;
			break;
		case LONG_STOP_SEQUENCE:
			g_info_curCh[i][0].natks = 0;
			break;
		default:
			fprintf(stderr, "detect_reset:invalid window_sequence: %d\n", window_sequence);
			exit(-1);
			break;
		}
	}
}

