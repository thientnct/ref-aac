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

/* This file contains the encoder and decoder common functions for the gain
compensater in the gain control tool. */

#include	<stdlib.h>
#include	<math.h>
#include	"gc.h"

/*****************************************************************************
The id of the gain change point is transformed to the exponent value of the 
gain.
*****************************************************************************/
int
gc_lngainof_id(int id)
{
	static int	a_lngain[npow2(IDGAINBITS)] = {
		-4, -3, -2, -1,  0,  1,  2,  3, 
		 4,  5,  6,  7,  8,  9, 10, 11
	};

	if (id < 0 || id >= npow2(IDGAINBITS)) {
		fprintf(stderr, "lngainof_id(): Wrong ID. id=%d\n", id);
		exit(1);
	}

	return(a_lngain[id]);
} /* gc_lngainof_id() */

/*****************************************************************************
The exponent value of the gain is transformed to the id of the gain change
point.
*****************************************************************************/
int
gc_idof_lngain(int lngain)
{
	int	id;

	for (id = 0; id < npow2(IDGAINBITS); id++){
		if (lngain == gc_lngainof_id(id)) {
			return(id);
		}
	}
	fprintf(stderr, "idof_lngain(): No ID. lngain=%d\n", lngain);
	exit(1);

	return 0;
} /* gc_idof_lngain() */


/*****************************************************************************
This function is used for calculating a fragment modification function. 
The interpolated gain value between the gain values of two gain change 
positions is calculated by using the following function.
    Inter(a,b,j)=2^(((8-j)log2(a)+jlog2(b))/8)
The interpolation are done within 8 samples.

	I: alev0	: Level at the Preceding Point
	I: alev1	: Level at the Following Point
	I: iloc	: Distance from the Preceding Point

	O: Interpolated Value is returned.
*****************************************************************************/
double
gc_gainc_interpolate(double alev0, double alev1, int iloc)
{
	double	val;
	double	a0, a1;

	a0 = mylog2(alev0);
	a1 = mylog2(alev1);
	val = pow(2.0, (double) (((8-iloc)*a0 + iloc*a1)/8));

	return(val);
} /* gc_gainc_interpolate() */

/*****************************************************************************
This function calculate a fragment modification function. The calculation is
carried out by interporation the gain values of the gain change positions. 

	 I: gainc: a gain control information
    I: max_loc_gainc: a number of maximum samples

    O: fmd: a fragment modification function
	 O: first gain of gain change position
*****************************************************************************/
double
gc_fmd(
	   GAIN_INF gainc,
	   int max_loc_gainc,
	   int nsamples,
		int *aloc,
		double *alev,
	   double *p_fmd
	   )
{
	int i, j;
	int *m;
	int lngain;
	double alev2nd;

	m = (int *) calloc(nsamples/2, sizeof(int));

	for (i = 0; i < gainc.num_gain_data; i++) {
		aloc[i+1] = 8 * gainc.gData[i].loc;
		if ((lngain = gc_lngainof_id(gainc.gData[i].lev)) < 0)
			alev[i+1] = (double)1/npow2(-lngain);
		else
			alev[i+1] = (double)npow2(lngain);
	}

	/* Set start point values */
	aloc[0] = 0;
	if (gainc.num_gain_data == 0) alev[0] = 1.0;
	else                          alev[0] = alev[1];
	alev2nd = alev[0];

	/* Set end point values */
	aloc[gainc.num_gain_data+1] = max_loc_gainc;
	alev[gainc.num_gain_data+1] = 1.0;

	for (i = 0; i < max_loc_gainc; i++) {
		m[i] = 0;
		for (j = 0; j <= gainc.num_gain_data+1; j++) {
			if (aloc[j] <= i) {
				m[i] = j;
			}
		}
	}

	for (i = 0; i < max_loc_gainc; i++) {
		if ((i >= aloc[m[i]]) && (i <= aloc[m[i]] + 7)) {
			p_fmd[i] = gc_gainc_interpolate(alev[m[i]],
													  alev[m[i]+1], 
													  i - aloc[m[i]]);
		}
		else {
			p_fmd[i] = alev[m[i]+1];
		}
	}

	free((char *)m);

	return(alev2nd);
} /* gc_fmd() */

/*****************************************************************************
This function produces a gain control function data. A gain control function
is a reciprocal function of a gain modification function. The calculation for 
a gain modification function differences on the three regions which are
the first half window region, the latter half window region and the non-
overlapped region. 

	I: nsamples	: Num of Samples
	I: gainc0	: Gain Control Information for first falf window
	I: gainc1	: Gain Control Information for latter falf window
	O: *p_ad	: Gain Control Function
			(Inverse of Level Augmentaion)
	I: block_type : Each block_type ID

	p_gcwind[nsamples] is obtained.
*****************************************************************************/
void
gc_gainc_window(
	int	nsamples,
	GAIN_INF	gain0,	
	GAIN_BLOCK	gain1,
	double	*p_ad,
	int	block_type,
    int block_id
	)
{
	int	i;
	int		a_aloc[10];
	double	a_alev[10];
	double	a_alev2nd;
	double	*p_fmd0, *p_fmd1, *p_fmd2, *p_mod;
	int	max_loc_gainc0, max_loc_gainc1, max_loc_gainc2;
	int flat_length;
	int block_number;

	p_mod = (double *) calloc(nsamples, sizeof(double));
	p_fmd0 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd1 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd2 = (double *) calloc(nsamples/2, sizeof(double));

	switch(block_type){
	  case ONLY_LONG_SEQUENCE:
	  case EIGHT_SHORT_SEQUENCE:
		max_loc_gainc0 = max_loc_gainc1 = nsamples/2;
		max_loc_gainc2 = 0;
		break;
	  case LONG_START_SEQUENCE:
		max_loc_gainc0 = nsamples/2;
		max_loc_gainc1 = nsamples*7/32;
		max_loc_gainc2 = nsamples/16;
		break;
	  case LONG_STOP_SEQUENCE:
		max_loc_gainc0 = nsamples/16;
		max_loc_gainc1 = nsamples*7/32;
		max_loc_gainc2 = nsamples/2;
		break;
	  default:
		fprintf(stderr, "Block Type Error :%d \n",block_type);
		exit(1);
		break;
	}

	/* Calculate the fragment modification functions */

	/* for the first half region */
	gc_fmd(gain0, max_loc_gainc0, nsamples, a_aloc, a_alev, p_fmd0);

	/* for the latter half region */
	if (block_type == EIGHT_SHORT_SEQUENCE) block_number = block_id;
	else                                    block_number = 0;
	a_alev2nd = gc_fmd(gain1.gBlock[block_number], max_loc_gainc1, 
							 nsamples, a_aloc, a_alev, p_fmd1);

	/* for the non-overlapped region */
	if ((block_type == LONG_START_SEQUENCE)
	  ||(block_type == LONG_STOP_SEQUENCE)) {
		gc_fmd(gain1.gBlock[1], max_loc_gainc2, nsamples, 
				 a_aloc, a_alev, p_fmd2);
	}

	/* Calculate a gain modification function */
	flat_length = 0;
	if (block_type == LONG_STOP_SEQUENCE) {
		flat_length = nsamples/2-max_loc_gainc0-max_loc_gainc1;
		for (i = 0; i < flat_length; i++) {
			p_mod[i] = 1.0;
		}
	}
	if ((block_type == ONLY_LONG_SEQUENCE)
	  ||(block_type == EIGHT_SHORT_SEQUENCE)) {
		a_alev[0] = 1.0;
	}

	for (i = 0; i < max_loc_gainc0; i++) {
		p_mod[i+flat_length] = a_alev[0] * a_alev2nd * p_fmd0[i];
	}
	for (i = 0; i < max_loc_gainc1; i++) {
		p_mod[i+flat_length+max_loc_gainc0] = a_alev[0] * p_fmd1[i];
	}

	if(block_type == LONG_START_SEQUENCE){
		for (i = 0; i < max_loc_gainc2; i++) {
			p_mod[i+max_loc_gainc0+max_loc_gainc1] = p_fmd2[i];
		}
		flat_length = nsamples/2 - max_loc_gainc1 - max_loc_gainc2;
		for (i = 0; i < flat_length; i++) {
			p_mod[i+max_loc_gainc0+max_loc_gainc1+max_loc_gainc2] = 1.0;
		}
	}
	else if(block_type == LONG_STOP_SEQUENCE){
		for (i = 0; i < max_loc_gainc2; i++) {
			p_mod[i+flat_length+max_loc_gainc0+max_loc_gainc1] = p_fmd2[i];
		}
	}

	/* Calculate a gain control function */
	for (i = 0; i < nsamples; i++) {
		p_ad[i] = 1.0 / p_mod[i];
	}
	
	free((char *)p_mod);
	free((char *)p_fmd0);
	free((char *)p_fmd1);
	free((char *)p_fmd2);

} /* gc_gainc_window() */


#endif
/*******************************************************************
                       End of gc_common.c
*******************************************************************/
