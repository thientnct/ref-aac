/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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

/*-r 192000 -cp -po e:\ch2_48 sly32shrt.wav
  -r 192000 -po e:\spatial mancsh_3ch.wav*/

#include "all.h"
#include "util.h"
#include <stdlib.h>

#define DEBUG 0

#ifndef MAXINT
#undef MAXINT
#define MAXINT	32767.
#endif

Float *NewFloat (int N) {
/* Allocate array of N Floats */

    Float *temp;

#if DEBUG
    printf ("Allocating a double array of %d data points\n", N);
#endif

    temp = (Float *) malloc (N * sizeof (Float));
    if (!temp) {
		printf ("\nERROR util.c 154: Could not allocate memory for array\nExiting program...\n");
		exit (1);
		}
    return temp;
}

typedef Float *pFloat;

Float **NewFloatMatrix (int N, int M) {
/* Allocate NxM matrix of Floats */

    Float **temp;
    int i;

#if DEBUG
    printf("Allocating %d x %d matrix of Floats\n", N, M);
#endif

/* allocate N pointers to Float arrays */
    temp = (pFloat *) malloc (N * sizeof (pFloat));
    if (!temp) {
		printf ("\nERROR: util.c 252: Could not allocate memory for array\nExiting program...\n");
		exit (1);
		}

/* Allocate a Float array M long for each of the N array pointers */

    for (i = 0; i < N; i++) {
		temp [i] = (Float *) malloc (M * sizeof (Float));
		if (! temp [i]) {
			printf ("\nERROR: Could not allocate memory for array\nExiting program...\n");
			exit (1);
			}
		}
    return temp;
}


