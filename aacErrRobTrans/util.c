/*
  "This software module was originally developed by 
  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
  development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
  14496-1,2 and 3. This software module is an implementation of a part of one or more 
  MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
  Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
  standards free license to this software module or modifications thereof for use in 
  hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
  Audio  standards. Those intending to use this software module in hardware or 
  software products are advised that this use may infringe existing patents. 
  The original developer of this software module and his/her company, the subsequent 
  editors and their companies, and ISO/IEC have no liability for use of this software 
  module or modifications thereof in an implementation. Copyright is not released for 
  non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
  retains full right to use the code for his/her  own purpose, assign or donate the 
  code to a third party and to inhibit third party from using the code for non 
  MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
  be included in all copies or derivative works." 
  Copyright(c)1996.
*/

#include "all.h"
#include "util.h"
#include <stdlib.h>


#define DEBUG 0

#ifndef MAXINT
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


