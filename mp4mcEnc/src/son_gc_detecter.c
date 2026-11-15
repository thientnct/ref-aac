/*******************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation) and Mitsuyuki Hatanaka (Sony Corporation)

and edited by
  Takashi Koike (Sony Corporation)
  Yasuhiro Toguri (Sony Corporation)

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


/*******************************************************************************
  Detect Peak Value
*******************************************************************************/
static double	son_get_peak(
	double	*p_buf,
	int	nsamples,
	int	block_type
	)
{

	int	isamp0, nsamps_inpart;
	double	absd, peakmax, peak;
	double	a_peak[NPEPARTS];
	int	i, j, start, end;

	nsamps_inpart = nsamples / NPEPARTS;
	isamp0 = nsamples/2;
	for (i = 0; i < NPEPARTS; ++i) {
		a_peak[i] = 0.0;
		for (j = 0; j < nsamps_inpart; ++j) {
			absd = fabs ((double) p_buf[isamp0 + j]);
			if (absd > a_peak[i]) {
				a_peak[i] = absd;
			}
		}
		isamp0 += nsamps_inpart;
	}

	if(block_type == LONG_START_SEQUENCE){
		start = 14;
		end = 18;
	}
	else{
		start = 0;
		end = NPEPARTS/2;
	}

	peakmax = 0.0;
	for (i = start; i < end; ++i) {
		if (a_peak[i] > peakmax) {
			peakmax = a_peak[i];
		}
	}
	peak = peakmax;

	return(peak);
}
	

/*****************************************************************************
  son_detect_gc_short

  Gain changes in the short windows are detected.

*****************************************************************************/

void 
son_detect_gc_short(double prev_peak, 
					 double *a_peak,
					 GAINC *p_gainc1)
{
	int i, j;

	int s = 1; /* for starting the argument of array from -1 */
	int N;
	float r[5], v[6];
	int g[5]; 

	for( i = 0; i < 8; i++ ) {

		N = 4*i; /* constant value to specify the short window */

		/* peak values are copied to region values */
		if(i==0) v[-1+s] = prev_peak;
		else     v[-1+s] = a_peak[N-1];

		for(j=0; j<4; j++) {
			v[j+s] = a_peak[j+N];
		}
		v[4+s] = a_peak[4+N];

		/* calculate Gain Ratio */
		for(j=-1; j<3; j++) {
			if(v[3+s] >  0.0 && v[j+s] > 0.0) {
				if(v[j+s] > 1.0) r[j+s] = v[3+s] / v[j+s];
				else                     r[j+s] = v[3+s] / 1.0;
			}
			else
			   r[j+s] = 1.0;
		}
		if(v[4+s] >  0.0 && v[3+s] > 0.0) {
			if(v[3+s] > 1.0) r[3+s] = v[4+s] / v[3+s];
			else                     r[3+s] = v[4+s] / 1.0;
		}
		else
		   r[3+s] = 1.0;

		/* calculate log . YT slight modify added (int) */
		for(j=-1; j<4; j++) {
			if(r[j+s] >= 1.0) g[j+s] = (int)floor(log10(r[j+s])/log10(2));
			else             g[j+s] = (int)ceil(log10(r[j+s])/log10(2));
		}

		/* */
      for(j=-1; j<4; j++) {
			if(j == 3) g[j+s] = g[3+s];
			else       g[j+s] = g[j+s] + g[3+s];
		}

		/* limiting */
		for(j=-1; j<4; j++) {
			if(g[j+s] > 11) g[j+s] = 11;
			if(g[j+s] < -4) g[j+s] = -4;
		}

		/* detect gain change */
		p_gainc1[i].natks = 0;
		for(j=0; j<3; j++) {

			/* detect attack */
			if(g[j-1+s] >= g[j+s] && g[j+s] > g[j+1+s]) {
				p_gainc1[i].a_loc[p_gainc1[i].natks] = j;
				p_gainc1[i].a_idgain[p_gainc1[i].natks] 
				= son_idof_lngain(g[j+s]);
				p_gainc1[i].natks++;
			}
			if(p_gainc1[i].natks > 7) break;

			/* detect release */
			if(g[j-1+s] < g[j+s] && g[j+s] <= g[j+1+s]) {
				if(j != 0 || g[j-1+s] != 0) { 
					p_gainc1[i].a_loc[p_gainc1[i].natks] = j;
					p_gainc1[i].a_idgain[p_gainc1[i].natks] 
					= son_idof_lngain(g[j-1+s]);
					p_gainc1[i].natks++;
				}
			}
			if(p_gainc1[i].natks > 7) break;
		}

		/* detect attack for the end of the region */
		if(p_gainc1[i].natks < 7) {
			if(g[3+s] > 1) {
				p_gainc1[i].a_loc[p_gainc1[i].natks] = 3;
				p_gainc1[i].a_idgain[p_gainc1[i].natks] 
				= son_idof_lngain(g[3+s]);
				p_gainc1[i].natks++;
			}
		}

		if(p_gainc1[i].natks > 0 
		&& p_gainc1[i].a_idgain[p_gainc1[i].natks-1] == son_idof_lngain(0)) {
			p_gainc1[i].natks--;
		}
	}

	p_gainc1[j].peak = v[3+s]; /* set the peak value for next frame */

} /* son_detect_gc_short */


/*******************************************************************************
	Locate PreEcho Position 

	I: *p_buf	: Data Buffer to be Scanned
	I: nsamples	: Num of Samples in Data Buffer
	I: *p_gainc0	: Gain Control Info for 1st Half
	O: *p_gainc1	: Gain Control Info for 2nd Half

	"nsamples*(NPEPARTS+1)/NPEPARTS" samples are assumed to be
	in the p_buf[].
*******************************************************************************/

static void son_set_gainc(double *p_buf, 
                          int nsamples, 
                          GAINC *p_gainc0, 
                          GAINC *p_gainc1)
{
	int	i, j;
	int	isamp0, nsamps_inpart, idgain;
	int	idgain_total;
	double	absd;
	double	a_peak[NPEPARTS];
	GAINC a_attack;

	a_attack.natks = 0;
	idgain_total = 0;
	idgain = 0;

	/* search the max value of each sub_block  */
	nsamps_inpart = nsamples / NPEPARTS;
	isamp0 = 144;
	for (i = 0; i < NPEPARTS; ++i) {
		a_peak[i] = 0.0;
		for (j = 0; j < nsamps_inpart; ++j) {
			absd = fabs ((double) p_buf[j + isamp0]);
			if (absd > a_peak[i]) {
				a_peak[i] = absd;
			}
		}
		isamp0 += nsamps_inpart;
	}

	/**************************/
	/** Detect in Short Area **/
	/**************************/
	son_detect_gc_short(p_gainc0->peak, a_peak, p_gainc1);

} /* set_gainc */


/*******************************************************************************
  gain detect
*******************************************************************************/
void	son_gc_detect(
	double	*bandSigChForGCAnalysis[],
	int	block_size_samples,
	int	window_sequence,
	int	ch,
	GAINC	*g_info_curCh[]
	)
{
	/* This parameter will be removed outside */
	static	GAINC	g_info_prev[MAX_TIME_CHANNELS][NBANDS];

	int	i;

	for (i = 1; i < NBANDS; i++) {
		switch(window_sequence) {
		case EIGHT_SHORT_SEQUENCE:
			son_set_gainc(bandSigChForGCAnalysis[i],
					block_size_samples/NBANDS*2,
					&g_info_prev[ch][i],
					g_info_curCh[i]);
			g_info_prev[ch][i] = g_info_curCh[i][SHORT_WIN_IN_LONG - 1];
			break;
		case ONLY_LONG_SEQUENCE:
			g_info_curCh[i][0].peak =
				son_get_peak(bandSigChForGCAnalysis[i],
						block_size_samples/NBANDS*2,
						ONLY_LONG_SEQUENCE);
			g_info_prev[ch][i] = g_info_curCh[i][0];
			g_info_curCh[i][0].natks = 0;
			break;
		case LONG_START_SEQUENCE:
			g_info_curCh[i][1].peak =
				son_get_peak(bandSigChForGCAnalysis[i],
						block_size_samples/NBANDS*2,
						LONG_START_SEQUENCE);
			g_info_prev[ch][i] = g_info_curCh[i][1];
			g_info_curCh[i][0].natks = 0;
			break;
		case LONG_STOP_SEQUENCE:
			g_info_curCh[i][1].peak =
				son_get_peak(bandSigChForGCAnalysis[i],
						block_size_samples/NBANDS*2,
						LONG_STOP_SEQUENCE);
			g_info_prev[ch][i] = g_info_curCh[i][1];
			g_info_curCh[i][0].natks = 0;
			break;
		default:
			fprintf(stderr, "invalid window_sequence: %d\n", window_sequence);
			exit(-1);
			break;
		}
	}
}

