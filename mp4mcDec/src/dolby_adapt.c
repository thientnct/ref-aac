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

#include "block.h"
#include "dolby_adapt.h"
#include "transfo.h"


/*****************************************************************************
*
*	unfold (moved from weave.c)
*	create full spectrum by reflecting-inverting first half over to second
*	input: see below 
*	output: see below
*	local static: none
*	globals: none
*
*****************************************************************************/
static void unfold ( 
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


/*
*	Interleave Definitions for start and stop blocks
*
*	Start block contains 1 576-pt spectrum (A) and 4 128-pt spectra (B-E)
*	  Input spectra are interleaved in repeating segements of 17 bins,
*		9 bins from A (A0-A8), and 2 bins from each of the shorts.
*	  Within the segments the bins are interleaved as:
*		A0 A1 A2 A3 A4 B0 C0 D0 E0 A5 A6 A7 A8 B1 C1 D1 E1
*
*	Stop block contains 3 128-pt spectra (A-C) and 1 576-pt spectrum (D)
*	  Input spectra are interleaved in repeating segements of 15 bins,
*		2 bins from each of the shorts, and 9 bins from D (D0-D8).
*	  Within the segments the bins are interleaved as:
*		A0 B0 C0 D0 D1 D2 D3 D4 A1 B1 C1 D5 D6 D7 D8
*	  The last 64 bins are (should be) set to 0.
*/


/*****************************************************************************
*
*	freq2time_adapt
*	transform freq. domain data to time domain.  
*	Overlap and add transform output to recreate time sequence.
*	Blocks composed of multiple segments (i.e. all but long) have 
*	  input spectrums interleaved.
*	input: see below
*	output: see below
*	local static:
*	  timeBuff		time domain data fifo
*	globals: none
*
*****************************************************************************/
void freq2time_adapt (
                      byte blockType,		/* input: blockType 0-3	*/
                      Wnd_Shape *wnd_shape, 	/* input/output */
                      Float *freqInPtr, 	/* input: interleaved spectrum */
                      Float *timeBuff, 		/* transform state needed for each channel */
                      Float *ftimeOutPtr)	/* output: 1/2 block of new time values	*/ 
{
  static Float	transBuff [2*BLOCK_LEN_LONG], *transBuffPtr;
  static Float	timeOutPtr[BLOCK_LEN_LONG];
  Float		*srcPtr;
  Float		*timeBuffPtr, *destPtr;
  int		windShape;
  int		i, j;

  windShape = wnd_shape->this_bk;		/*	window_shape [ch];	*/

  if (blockType == ONLY_LONG_WINDOW)  {
    unfold (freqInPtr, transBuff, 1, BLOCK_LEN_LONG);

    /* Do 1 LONG transform */
    ITransformBlock (transBuff, ONLY_LONG_WINDOW, wnd_shape);

    /* Add first half and old data */
    transBuffPtr = transBuff;
    timeBuffPtr = timeBuff;
    destPtr = timeOutPtr;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
    }

    /* Save second half as old data */
    timeBuffPtr = timeBuff;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *timeBuffPtr++ = *transBuffPtr++;
    }
  }

  else if (blockType == EIGHT_SHORT_WINDOW)  {

    /* Do 8 SHORT transforms */
    destPtr = timeBuff + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2; /* = 448 */

    for (i = 0; i < SHORT_BLOCKS_IN_LONG_BLOCK; i++) {
      unfold (freqInPtr, transBuff, 1, BLOCK_LEN_SHORT ); /* was freqinPtr++, 8 .. mfd */
      freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
      ITransformBlock (transBuff, EIGHT_SHORT_WINDOW, wnd_shape);

      /* Add first half of short window and old data */
      transBuffPtr = transBuff;
      for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
        *destPtr++ += *transBuffPtr++;
      }

      /* Save second half of short window */
      for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
        *destPtr++ += *transBuffPtr++;        /* FhG bug fix */
      }
      destPtr -= BLOCK_LEN_SHORT;
    }

    /* Copy data to output buffer */
    destPtr = timeOutPtr;
    timeBuffPtr = timeBuff;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *timeBuffPtr++;
    }

    /* Update timeBuff fifo */
    destPtr = timeBuff;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *timeBuffPtr++;
    }
  }

  else if (blockType == LONG_START_WINDOW)  {
    unfold(freqInPtr, transBuff, 1, BLOCK_LEN_LONG);
    ITransformBlock (transBuff, LONG_START_WINDOW, wnd_shape);

    /* Add first half and old data */
    transBuffPtr = transBuff;
    timeBuffPtr = timeBuff;
    destPtr = timeOutPtr;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
    }

    /* Save second half as old data */
    timeBuffPtr = timeBuff;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *timeBuffPtr++ = *transBuffPtr++;
    }
  }

  else if (blockType == LONG_STOP_WINDOW)  {
    unfold (freqInPtr, transBuff, 1, BLOCK_LEN_LONG);

    /* Do 1 LONG transforms */
    ITransformBlock (transBuff, LONG_STOP_WINDOW, wnd_shape);

    /* Add first half and old data */
    transBuffPtr = transBuff;
    timeBuffPtr = timeBuff;
    destPtr = timeOutPtr;

    /* FhG bug fix */
    for (i = 0; i < BLOCK_LEN_LONG; i++)  { /* -NFLAT */
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
    }

    /* Save second half as old data */
    timeBuffPtr = timeBuff;
    for (i = 0; i < BLOCK_LEN_LONG; i++ )  {
      *timeBuffPtr++ = *transBuffPtr++;
    }
  }

  else {
    printf( "Illegal Block_type %d in time2freq_adapt(), aborting ...\n", blockType );
    myexit( "block_type illegal" );
  }

  for (i = 0; i < BLOCK_LEN_LONG; i++)  {

    ftimeOutPtr [i] = (Float) timeOutPtr [i];
    /* FhG bug fix */
    timeBuff[LN2+i] = 0.0f;
    /* ftimeOutPtr [i] = 1.;*/

  }

}

/*****************************************************************************
*
*	time2freq_adapt
*	transform to time domain data to freq. domain.  
*	Blocks composed of multiple segments (i.e. all but long) have 
*	  input spectrums interleaved.
*	Notice: currently verified only for certain blocktypes
*	input: see below
*	output: see below
*	local static:
*	  none
*	globals: none
*
*****************************************************************************/
void time2freq_adapt (

                      byte blockType,			/* input: blockType 0-3	*/
                      Wnd_Shape *wnd_shape, 		/* input/output */
                      Float *timeInPtr, 		/* input: time domain data */
                      Float *ffreqOutPtr)		/* output: 1/2 block of new freq values */ 
{
  static Float	transBuff [2*BLOCK_LEN_LONG], *transBuffPtr;
  int		i, j;
  Float		*srcPtr;
  Float		*destPtr;
  static Float	freqOutPtr[BLOCK_LEN_LONG];
  int		windShape;

  windShape = wnd_shape->this_bk;		/*	window_shape [ch];	*/

  if (blockType == ONLY_LONG_WINDOW)  {
    srcPtr = timeInPtr;
    destPtr = transBuff;
    for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
    /* Do 1 LONG transform */
    TransformBlock (transBuff, ONLY_LONG_WINDOW, wnd_shape);

    srcPtr = transBuff;
    destPtr = freqOutPtr;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
  }

  else if (blockType == EIGHT_SHORT_WINDOW)  {

    /* Do 8 SHORT transforms */
    srcPtr = timeInPtr + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;
    destPtr = freqOutPtr;

    for (i = 0; i < SHORT_BLOCKS_IN_LONG_BLOCK; i++) {
      transBuffPtr = transBuff;
      for (i = 0; i < 2 * BLOCK_LEN_SHORT; i++)  {
        *transBuffPtr++ = *srcPtr++;
      }
      srcPtr -= BLOCK_LEN_SHORT;
      TransformBlock (transBuff, EIGHT_SHORT_WINDOW, wnd_shape);

      /* Copy data to output buffer */
      transBuffPtr = transBuff;
      for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
        *destPtr++ = *transBuffPtr++;
      }
    }
  }

  else if (blockType == LONG_START_WINDOW)  {
    srcPtr = timeInPtr;
    destPtr = transBuff;
    for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
    TransformBlock (transBuff, LONG_START_WINDOW, wnd_shape);

    srcPtr = transBuff;
    destPtr = freqOutPtr;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
  }

  else if (blockType == LONG_STOP_WINDOW)  {
    srcPtr = timeInPtr;
    destPtr = transBuff;
    for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
    TransformBlock (transBuff, LONG_STOP_WINDOW, wnd_shape);

    srcPtr = transBuff;
    destPtr = freqOutPtr;
    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
      *destPtr++ = *srcPtr++;
    }
  }

  else {
    printf( "Illegal Block_type %d in time2freq_adapt(), aborting ...\n", blockType );
    myexit( "block_type illegal" );
  }

  for (i = 0; i < BLOCK_LEN_LONG; i++)  {
    ffreqOutPtr [i] = (Float) freqOutPtr [i];
    /*ftimeOutPtr [i] = 1.;*/
  }

}

