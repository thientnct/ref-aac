/*******************************************************************************
MPEG-4 Audio VM

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
#include <string.h>
#include	"sony_local.h"
#include	"bitstream.h"

/*******************************************************************************
	arrange spectral_line_vector (for encoding)
*******************************************************************************/
void	son_gc_arrangeSpecEnc(
	double	*freqSignalChForPP,
	int	block_size_samples,
	int	window_sequence,
	double	*freqSignalCh
	)
{
	int	i, j;
	int	band;
	double	tmp;

	for (band = 0; band < NBANDS; band++) {
		if (window_sequence == EIGHT_SHORT_SEQUENCE) {
			for (i = 0; i < 8; i++) {
				memcpy(
					(char *)&freqSignalCh[band * block_size_samples/NBANDS/8 + i * block_size_samples/8],
					(char *)&freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i],
					block_size_samples/NBANDS/8*sizeof(double)
				);
				/* reverse the order of the MDCT coefficients in each even PQF band */
				if (band % 2 == 1) {
					for (j = 0; j < block_size_samples/NBANDS/8/2; j++) {
						tmp = freqSignalCh[band * block_size_samples/NBANDS/8 + i * block_size_samples/8 + j];
						freqSignalCh[band * block_size_samples/NBANDS/8 + i * block_size_samples/8 + j]
							= freqSignalCh[(band + 1) * block_size_samples/NBANDS/8 + i * block_size_samples/8 - 1 - j];
						freqSignalCh[(band + 1) * block_size_samples/NBANDS/8 + i * block_size_samples/8 - 1 - j]
							= tmp;
					}
				}
			}
		}
		else {
			memcpy(
				(char *)&freqSignalCh[block_size_samples/NBANDS *band],
				(char *)&freqSignalChForPP[block_size_samples/NBANDS * band],
				block_size_samples/NBANDS * sizeof(double)
			);
			/* reverse the order of the MDCT coefficients in each even PQF band */
			if (band % 2 == 1) {
				for (i = 0; i < block_size_samples/NBANDS/2; i++) {
					tmp = freqSignalCh[block_size_samples/NBANDS * band + i];
					freqSignalCh[block_size_samples/NBANDS * band + i]
						= freqSignalCh[block_size_samples/NBANDS * (band + 1) - 1 - i];
					freqSignalCh[block_size_samples/NBANDS * (band + 1) - 1 - i]
						= tmp;
				}
			}
		}
	}
}

#if 0 /* this function is never called. RS 2004-03-10*/
/*******************************************************************************
	arrange spectral_line_vector (for decoding)
*******************************************************************************/
static void	son_gc_arrangeSpecDec(
	double	*freqSignalCh,
	int	block_size_samples,
	int	window_sequence,
	double	*freqSignalChForPP
	)
{
	int	i, j;
	int	band;
	double	tmp;

	for (band = 0; band < NBANDS; band++) {
		if (window_sequence == EIGHT_SHORT_SEQUENCE) {
			for (i = 0; i < 8; i++) {
				memcpy(
					(char *)&freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i],
					(char *)&freqSignalCh[band * block_size_samples/NBANDS/8 + i * block_size_samples/8],
					block_size_samples/NBANDS/8*sizeof(double)
				);
				/* reverse the order of the MDCT coefficients in each even PQF band */
				if (band % 2 == 1) {
					for (j = 0; j < block_size_samples/NBANDS/8/2; j++) {
						tmp = freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i + j];
						freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * i + j]
							= freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * (i + 1) - 1 - j];
						freqSignalChForPP[block_size_samples/NBANDS * band + block_size_samples/NBANDS/8 * (i + 1) - 1 - j]
							= tmp;
					}
				}
			}
		}
		else {
			memcpy(
				(char *)&freqSignalChForPP[block_size_samples/NBANDS * band],
				(char *)&freqSignalCh[block_size_samples/NBANDS *band],
				block_size_samples/NBANDS * sizeof(double)
			);
			/* reverse the order of the MDCT coefficients in each even PQF band */
			if (band % 2 == 1) {
				for (i = 0; i < block_size_samples/NBANDS/2; i++) {
					tmp = freqSignalChForPP[block_size_samples/NBANDS * band + i];
					freqSignalChForPP[block_size_samples/NBANDS * band + i]
						= freqSignalChForPP[block_size_samples/NBANDS * (band + 1) - 1 - i];
					freqSignalChForPP[block_size_samples/NBANDS * (band + 1) - 1 - i]
						= tmp;
				}
			}
		}
	}
}
#endif
