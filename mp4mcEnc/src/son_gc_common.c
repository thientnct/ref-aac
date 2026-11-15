/*******************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation) and Mitsuyuki Hatanaka (Sony Corporation)

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

#include	<stdlib.h>
#include	"sony_local.h"
#include	<math.h>

/*******************************************************************************
	Return Log2(Gain) of ID
*******************************************************************************/
int	son_lngainof_id(int id)
{
	static int	a_lngain[npow2(IDGAINBITS)] = {
		-4, -3, -2, -1,  0,  1,  2,  3, 
		 4,  5,  6,  7,  8,  9, 10, 11
	};

	if (id < 0 || id >= npow2(IDGAINBITS)) {
		fprintf(stderr, "lngainof_id(): Wrong ID. id=%d\n", id);
		exit(-1);
	}

	return(a_lngain[id]);
}

/*******************************************************************************
	Return ID of Log2(Gain)
*******************************************************************************/
int	son_idof_lngain(int lngain)
{
	int	id;

	for (id = 0; id < npow2(IDGAINBITS); id++){
		if (lngain == son_lngainof_id(id)) {
			return(id);
		}
	}
	fprintf(stderr, "idof_lngain(): No ID. lngain=%d\n", lngain);
	exit(-1);
	return 0;    /* This doesn't matter, it's never executed */
}


/*******************************************************************************
	Interpolation Function for Gain Control 

	I: alev0	: Level at the Preceding Point
	I: alev1	: Level at the Following Point
	I: iloc		: Distance from the Preceding Point

	Interpolated Value is returned.
*******************************************************************************/
static double	son_gainc_interpolate(double alev0, double alev1, int iloc)
{
	double	val;
	double	a0, a1;

	a0 = mylog2(alev0);
	a1 = mylog2(alev1);
	val = pow(2.0, (double) (((8-iloc)*a0 + iloc*a1)/8));

	return(val);
}

/*******************************************************************************
	Set Gain Control Window

	I: nsamples	: Num of Samples
	I: gainc0	: Gain Control Information for 1st Half
	I: gainc1	: Gain Control Information for 2st Half
	O: *p_ad	: Gain Control Window Function
			(Inverse of Level Augmentaion)
	I: block_type : Each block_type ID

	p_gcwind[nsamples] is obtained.
*******************************************************************************/
void son_gainc_window(
	int	nsamples,
	GAINC	gainc0,	
	GAINC	gainc1,
	GAINC	gainc2,
	double	*p_ad,
	int	block_type
	)
{
	int	a_aloc[10], *p_m;
	double	a_alev[10];
	double	a_alev2nd;
	int	lngain;
	int	i, j;
	double	*p_fmd0, *p_fmd1, *p_fmd2, *p_mod;
	int	max_loc_gainc0 = 0;
        int     max_loc_gainc1 = 0;
        int     max_loc_gainc2 = 0;
	int flat_length;

	p_m = (int *) calloc(nsamples/2, sizeof(double));
	p_mod = (double *) calloc(nsamples, sizeof(double));
	p_fmd0 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd1 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd2 = (double *) calloc(nsamples/2, sizeof(double));

	switch(block_type){
	  case ONLY_LONG_SEQUENCE: max_loc_gainc0 = nsamples/2;
		               max_loc_gainc1 = nsamples/2;
		               max_loc_gainc2 = 0;
                       break;
	  case EIGHT_SHORT_SEQUENCE: max_loc_gainc0 = nsamples/2;
		                max_loc_gainc1 = nsamples/2;
		                max_loc_gainc2 = 0;
                        break;
	  case LONG_START_SEQUENCE: max_loc_gainc0 = nsamples/2;
		                max_loc_gainc1 = nsamples*7/32;
		                max_loc_gainc2 = nsamples/16;
                        break;
	  case LONG_STOP_SEQUENCE: max_loc_gainc0 = nsamples/16;
		               max_loc_gainc1 = nsamples*7/32;
		               max_loc_gainc2 = nsamples/2;
                       break;
	  default:
            fprintf(stderr, "Block Type Error :%d \n",block_type);
            exit(1);
            break;
	}

/* 1st Area */
	for (i = 0; i < gainc0.natks; i++) {
		a_aloc[i+1] = 8 * gainc0.a_loc[i];
		if ((lngain = son_lngainof_id(gainc0.a_idgain[i])) < 0) {
			a_alev[i+1] = (double)1/npow2(-lngain);
		}
		else {
			a_alev[i+1] = (double)npow2(lngain);
		}
	}

	a_aloc[0] = 0;
	if (gainc0.natks == 0) {
		a_alev[0] = 1.0;
	}
	else {
		a_alev[0] = a_alev[1];
	}

	a_aloc[gainc0.natks+1] = max_loc_gainc0;
	a_alev[gainc0.natks+1] = 1.0;

	for (i = 0; i < max_loc_gainc0; i++) {
		p_m[i] = 0;
		for (j = 0; j <= gainc0.natks+1; j++) {
			if (a_aloc[j] <= i) {
				p_m[i] = j;
			}
		}
	}

	for (i = 0; i < max_loc_gainc0; i++) {
		if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
			p_fmd0[i] = son_gainc_interpolate(a_alev[p_m[i]],
				a_alev[p_m[i]+1], i - a_aloc[p_m[i]]);
		}
		else {
			p_fmd0[i] = a_alev[p_m[i]+1];
		}
	}

/* 2st Area */

	for (i = 0; i < gainc1.natks; i++) {
		a_aloc[i+1] = 8 * gainc1.a_loc[i];
		if ((lngain = son_lngainof_id(gainc1.a_idgain[i])) < 0) {
			a_alev[i+1] = (double)1/npow2(-lngain);
		}
		else {
			a_alev[i+1] = (double)npow2(lngain);
		}
	}

	a_aloc[0] = 0;
	if (gainc1.natks == 0) {
		a_alev[0] = 1.0;
	}
	else {
		a_alev[0] = a_alev[1];
	}
	
	a_alev2nd = a_alev[0];

	a_aloc[gainc1.natks+1] = max_loc_gainc1;
	a_alev[gainc1.natks+1] = 1.0;

	for (i = 0; i < max_loc_gainc1; i++) {
		p_m[i] = 0;
		for (j = 0; j <= gainc1.natks+1; j++) {
			if (a_aloc[j] <= i) {
				p_m[i] = j;
			}
		}
	}

	for (i = 0; i < max_loc_gainc1; i++) {
		if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
			p_fmd1[i] = son_gainc_interpolate(a_alev[p_m[i]],
					a_alev[p_m[i]+1], i - a_aloc[p_m[i]]);
		}
		else {
			p_fmd1[i] = a_alev[p_m[i]+1];
		}
	}

/* 3rd Area */
	if((block_type == LONG_START_SEQUENCE)||(block_type == LONG_STOP_SEQUENCE)){
		for (i = 0; i < gainc2.natks; i++) {
			a_aloc[i+1] = 8 * gainc2.a_loc[i];
			if ((lngain = son_lngainof_id(gainc2.a_idgain[i])) < 0) {
				a_alev[i+1] = (double)1/npow2(-lngain);
			}
			else {
				a_alev[i+1] = (double)npow2(lngain);
			}
		}

		a_aloc[0] = 0;
		if (gainc2.natks == 0) {
			a_alev[0] = 1.0;
		}
		else {
			a_alev[0] = a_alev[1];
		}

		a_aloc[gainc2.natks+1] = max_loc_gainc2;
		a_alev[gainc2.natks+1] = 1.0;

		for (i = 0; i < max_loc_gainc2; i++) {
			p_m[i] = 0;
			for (j = 0; j <= gainc2.natks+1; j++) {
				if (a_aloc[j] <= i) {
					p_m[i] = j;
				}
			}
		}

		for (i = 0; i < max_loc_gainc2; i++) {
			if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
				p_fmd2[i] = son_gainc_interpolate(
						a_alev[p_m[i]],
						a_alev[p_m[i]+1],
						i - a_aloc[p_m[i]]);
			}
			else {
				p_fmd2[i] = a_alev[p_m[i]+1];
			}
		}
	}

/* Generate Complete Gain Control Function */

	flat_length = 0;
	if(block_type == LONG_STOP_SEQUENCE){
		flat_length = nsamples/2-max_loc_gainc0-max_loc_gainc1;
		for(i = 0; i < flat_length; i++){
			p_mod[i] = 1.0;
		}
	}
	if((block_type == ONLY_LONG_SEQUENCE)||(block_type == EIGHT_SHORT_SEQUENCE)){
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
	for (i = 0; i < nsamples; i++) {
		p_ad[i] = 1.0 / p_mod[i];
	}
	
	free((char *)p_m);
	free((char *)p_mod);
	free((char *)p_fmd0);
	free((char *)p_fmd1);
	free((char *)p_fmd2);
}
