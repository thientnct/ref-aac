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
#include "mdct_fhg_1024.h"
#include "weave.h"


/* interleave routines */
/*****************************************************************************
*
*	interleave several non-long blocks into one long block
*	input: 
*		data_in		array with several concatonated spectra
*	output:
*		data_out	array with spectra interleaved 
*	local static: none
*	globals: none
*
*****************************************************************************/

void weaveShort( double *data_in, float *data_out )  {
    /*	interleave 8 short windows */
    register int   i;
    int            step, j;
    double			*srcPtr;
    float  		*destPtr;

    step = BLOCK_LEN_SHORT;
    srcPtr = data_in;
    for (i=SHORT_BLOCKS_IN_LONG_BLOCK-1; i-->=0; )  {
	destPtr = data_out;
	for (j=step-1; j-->=0; )  {
	    *destPtr = *srcPtr++;
	    destPtr += SHORT_BLOCKS_IN_LONG_BLOCK;
	}
	data_out++;
    }
} /* end of weaveShort */


void weaveStart( double *data_in, float *data_out )  {
    /*	interleave 1 start window with 4 short windows */
    register int   i;
    int            j;
    register float *destPtr;
    double			*startPtr,*shortPtr,*shortPtr2;

    startPtr = data_in;
    shortPtr = data_in+576;
    destPtr = data_out;
    for(j=59; j-->=0; )  {				/* repeat interleave pattern 60 times */
	shortPtr2 = shortPtr;
	for(i=4; i-->=0; )  {				/* 5 from start */
	    *destPtr++ = *startPtr++;
	}
	for (i=3; i-->=0; )  {			/* 1 from each short block */
	    *destPtr++ = *shortPtr2;
	    shortPtr2 += 128;
	}
	shortPtr++;
	shortPtr2 = shortPtr;
	for(i=3; i-->=0; )  {				/* 4 from start */
	    *destPtr++ = *startPtr++;
	}
	for (i=3; i-->=0; )  {			/* 1 from each short block */
	    *destPtr++ = *shortPtr2;
	    shortPtr2 += 128;
	}
	shortPtr++;
    }
    for(j=3; j-->=0; )  *destPtr++ = 0.0;	/* clear to end of buffer */
} /* end of sort_start */


void weaveStop( double *data_in, float *data_out )  {
    /*	interleave 3 short windows with 1 stop window */
    register int    i;
    int             j;
    register float *destPtr;
    double			*startPtr,*shortPtr,*shortPtr2;

    shortPtr = data_in;
    startPtr = data_in+3*128;
    destPtr = data_out;
    for(j=63; j-->=0; )  {				/* basic interleave pattern repeats 64 times */
	shortPtr2 = shortPtr;
	for (i=2; i-->=0; )  {			/* 1 from each (of 3) short block */
	    *destPtr++ = *shortPtr2;
	    shortPtr2 += 128;
	}
	for(i=4; i-->=0; )  {				/* 5 from start */
	    *destPtr++ = *startPtr++;
	}
	shortPtr++;
	shortPtr2 = shortPtr;
	for (i=2; i-->=0; )  {			/* 1 from each (of 3) short block */
	    *destPtr++ = *shortPtr2;
	    shortPtr2 += 128;
	}
	for(i=3; i-->=0; )  {				/* 4 from start */
	    *destPtr++ = *startPtr++;
	}
	shortPtr++;
    }
    for(j=63; j-->=0; )  {				/* clear last 64 bins */
	*destPtr++ = 0.0;
    }
} /* end of sort_stop */



/* un-interleave routines */

/*****************************************************************************
*
*	unfold
*	create full spectrum by reflecting-inverting first half over to second
*	input: see below 
*	output: see below
*	local static: none
*	globals: none
*
*****************************************************************************/
void unfold ( 
	Float *data_in,	/* input: 1/2 spectrum */
	Float *data_out,	/* output: full spectrum */
	int inStep,			/* input: input array increment */
	int inLeng)			/* input: length of input vector */
{
    register int   i;
    /* int            step, j;
 double			*srcPtr, *destPtr, *mirrorPtr; unused - SRQ */

    /* fill transBuff w/ full MDCT sequence from freqInPtr */
    for (i=0;i<inLeng;i++) {
	data_out[i] = *data_in;
	data_out[2*inLeng-i-1] = -(*data_in);
	data_in += inStep;
    }
} /* end of unfold */

/*****************************************************************************
*
*	unsortShort
*	copy and unfold one short spectum from short block
*	=> use unfold() with inStep = 8
*
*****************************************************************************/

/*****************************************************************************
*
*	unweave
*	un-interleave and unfold start or stop window from spectra block
*	input: see below
*	output: see below
*	local static: 
*		step		array of interleave index increments
*		segLeng		length of repeating sements = sum of steps
*	globals: none
*
*****************************************************************************/
void unweave ( 
float *inPtr,	/* input: interleaved spectra block */
double *outPtr,	/* output: contiguous spectrum */
int type)		/* input: window type to extract 0:start, 1:stop */
{
    static int	step[2][9] = {
	{1,1,1,1,5,1,1,1,5},
	{1,1,1,1,4,1,1,1,4}
    };

    static int	segCount[2] = {
	60,64    };
    register int	j /* ,rem unused - SRQ */;
    int			i,*stepPtr;
    double 		*mirrorOutPtr;


    i = segCount[type];
    mirrorOutPtr = outPtr+BLOCK_LEN_LONG+BLOCK_LEN_SHORT-1;
    while( i-- > 0 )  {
	j = 9;							/* # elements in step arrays */
	stepPtr = step[type];
	while( j-- > 0 )  {
	    *outPtr++ = *inPtr;
	    *mirrorOutPtr-- = -(*inPtr);
	    inPtr += *stepPtr++;
	}
    }
    while( mirrorOutPtr>outPtr )  {
	*outPtr++ = 0.0;
	*mirrorOutPtr-- = 0.0;
    }
} /* end of unweave */

/*****************************************************************************
*
*	unweaveShortFrom
*	un-interleave and unfold short window from start or stop block
*	input: see below
*	output: see below
*	local static: 
*		step		array of interleave index increments
*		segLeng		length of repeating sements = sum of steps
*	globals: none
*
*****************************************************************************/
void unweaveShortFrom (
float *inPtr,	/* input: ptr to first element of interleaved spectrum */
double *outPtr,	/* output: contiguous spectrum */
int type)		/* input: block type to extract from  0:start, 1:stop */
{
    static int	step[2][2] = {
	{8,9},
	{8,7}
    };

    static int	segCount[2] = {
	60,64    };
    register int	j;
    int			i, *stepPtr;
    double 		*mirrorOutPtr;

    i = segCount[type];
    mirrorOutPtr = outPtr + 2*BLOCK_LEN_SHORT -1;
    while( (i-- > 0) && (mirrorOutPtr>outPtr) )  {
	j = 2;							/* # elements in step arrays */
	stepPtr = step[type];
	while( j-- > 0 )  {
	    *outPtr++ = *inPtr;
	    *mirrorOutPtr-- = -(*inPtr);
	    inPtr += *stepPtr++;
	}
    }
    while( mirrorOutPtr>outPtr )  {
	*outPtr++ = 0.0;
	*mirrorOutPtr-- = 0.0;
    }
} /* end of unweaveShortFrom */
