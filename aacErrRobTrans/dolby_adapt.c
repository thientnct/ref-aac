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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dolby_def.h"
#include "weave.h"
#include "block.h"

#if 0
#include "mdct_fhg_1024.h"
#endif

#define NORM_TYPE 0
#define START_TYPE 1
#define SHORT_TYPE 2
#define STOP_TYPE 3

extern int frame_cnt;

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

#define N_SHORT_IN_START 4
#define	START_OFFSET 0
#define	SHORT_IN_START_OFFSET 5
#define N_SHORT_IN_STOP	3
#define	STOP_OFFSET 3
#define SHORT_IN_STOP_OFFSET 0
#define N_SHORT_IN_4STOP	4


/* #define	B2B */
#if defined(B2B)	/* write transform output for Back to Back test */
FILE* stream;
long	fileRdCnt;
#endif

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

	byte blockType,			/* input: blockType 0-3						*/
	Wnd_Shape *wnd_shape, 	/* input/output								*/
	Float *freqInPtr, 		/* input: interleaved spectrum				*/
	Float *timeBuff, 		/* transform state needed for each channel	*/
	Float *ftimeOutPtr)		/* output: 1/2 block of new time values		*/ 
{
    static int		dolbyShortOffset = 1;
    static Float	transBuff [2*BLOCK_LEN_LONG], *transBuffPtr;
    int				i, j;
    Float			*srcPtr;
    Float			*timeBuffPtr, *destPtr;
    static Float	timeOutPtr[BLOCK_LEN_LONG];
    int				windShape;

    windShape = wnd_shape->this_bk;		/*	window_shape [ch];	*/


#if !defined(DOLBYBS)
    /*	mapping from old FhG blocktypes to new window sequence types */

    switch (blockType)
    {
    case NORM_TYPE:
	blockType = ONLY_LONG;
	break;
    case START_TYPE:
	blockType = OLD_START;
	break;
    case SHORT_TYPE:
	blockType = EIGHT_SHORT;
	break;
    case STOP_TYPE:
	blockType = OLD_STOP;
	break;
    default:
	printf("dolby_adapt.c: Illegal block type %d - aborting\n",
	    blockType);
	exit(1);
	break;
    }
#endif

#ifdef BTOB
    getmydata (&blockType, &windShape, &ch, freqInPtr, &i);
    window_shape [ch] = windShape;
#endif

#ifdef MFDBG
    printf ("translated bT %d\n", blockType);
#endif


#if defined(B2B)	/* write transform output for Back to Back test */
    {
#include	"util.h"

	int static	cnt=0;
	int			bscnt;
	int			numread;
	double		inputArray[BLOCK_LEN_LONG];		/* for debug */
	short		i;


	printf ("B2Bis defined!\n");
	exit (1);

	if (cnt == 0)  {
	    if ((stream = fopen ("xform.out", "rt")) == NULL)  {
			printf ("Problem opening the file 'xform.out'\n");
			}
		}
	numread = fscanf (stream, "%d,%d", &bscnt, &blockType );
	fileRdCnt += 1;
	printf (">>>>%d,%d<<<<\n", bscnt, blockType);
	if (bscnt != cnt++)  {
	    printf ("Frame count error!!!!!!!!!!!!!!!!!!!!!!\n");
		}

	if (!ReadFile (stream, freqInPtr, BLOCK_LEN_LONG))  {
	    puts ("file read error");
		}
	else {
	    fileRdCnt += BLOCK_LEN_LONG;
	    for( i=0; i<BLOCK_LEN_LONG; i++)  {
		inputArray[i] = freqInPtr[i];
	    }
	}
    }
#endif


    if (blockType == ONLY_LONG)  {
		unfold (freqInPtr, transBuff, 1, BLOCK_LEN_LONG);
		/* Do 1 LONG transform */
		ITransformBlock (transBuff, LONG_BLOCK, wnd_shape );	/* ch ); */
/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;		/*	 [ch];	*/
		destPtr = timeOutPtr;
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
		}
/* Save second half as old data */
		timeBuffPtr = timeBuff;		/*		 [ch];		*/
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*timeBuffPtr++ = *transBuffPtr++;
		}
    }

    else if (blockType == SHORT_START)  {
	/* Do 1 START, 4 SHORT transforms */

	unfold (freqInPtr, transBuff, 1, (BLOCK_LEN_SHORT + BLOCK_LEN_LONG) / 2);

	ITransformBlock (transBuff, START_BLOCK, wnd_shape );
	/* Add first half and old data */
	transBuffPtr = transBuff;
	timeBuffPtr = timeBuff;	/*	 [ch];	*/
	destPtr = timeOutPtr;
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
	}
/* Save second half as old data */
	timeBuffPtr = timeBuff;		/*		 [ch];		*/
	for (i = 0; i < BLOCK_LEN_SHORT; i++)  {
	    *timeBuffPtr++ = *transBuffPtr++;
	}
	srcPtr = freqInPtr + ((BLOCK_LEN_LONG + BLOCK_LEN_SHORT) / 2); /* SHORT_IN_START_OFFSET;	*/
	timeBuffPtr = timeBuff;   /*	 [ch];		*/
	for (i = 0; i < N_SHORT_IN_START; i++)  {
	    unfold (srcPtr, transBuff, 1, BLOCK_LEN_SHORT);
	    srcPtr += BLOCK_LEN_SHORT;

	    ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape );
	    /* Add first half of short window and old data */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
			*timeBuffPtr++ += *transBuffPtr++;
		    }
	    /* Save second half of short window */
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
			*timeBuffPtr++ = *transBuffPtr++;
		    }
	    timeBuffPtr -= BLOCK_LEN_SHORT;		/* go back for overlap add */
		}
	dolbyShortOffset = 1;
    }

    else if (blockType == EIGHT_SHORT)  {
	/* Do 8 SHORT transforms */

	if (dolbyShortOffset)
	    destPtr = timeBuff + 4 * BLOCK_LEN_SHORT;		/* DBS */
	else
	    destPtr = timeBuff + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;	/*	448	*/

	for (i = 0; i < 8; i++) {
	    unfold (freqInPtr, transBuff, 1, BLOCK_LEN_SHORT );
	    /*was freqinPtr++, 8 .. mfd */
	    freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
	    ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape );

	    /* Add first half of short window and old data */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
		*destPtr++ += *transBuffPtr++;
	    }
	    /* Save second half of short window */
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
		*destPtr++ = *transBuffPtr++;
	    }
	    destPtr -= BLOCK_LEN_SHORT;
	}
	/* Copy data to output buffer */
	destPtr = timeOutPtr;
	timeBuffPtr = timeBuff;		/*		 [ch];		*/
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *timeBuffPtr++;
	}
	/* Update timeBuff fifo */
	destPtr = timeBuff;		/*		 [ch];		*/
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *timeBuffPtr++;
	}
    }

    else if (blockType == SHORT_STOP)  {
	/* Do 3 SHORT, 1 STOP transforms */
	destPtr = timeBuff + 4 * BLOCK_LEN_SHORT;
	srcPtr = freqInPtr; /*  + SHORT_IN_STOP_OFFSET;	*/
	for (i = 0; i < N_SHORT_IN_STOP; i++)  {
	    unfold (srcPtr, transBuff, 1, BLOCK_LEN_SHORT );
	    srcPtr += BLOCK_LEN_SHORT;
	    ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape );
	    /* Add first half of short window and old data */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++ )  {
		*destPtr++ += *transBuffPtr++;
	    }
	    /* Save second half of short window */
	    for (j = 0; j<BLOCK_LEN_SHORT; j++ )  {
		*destPtr++ = *transBuffPtr++;
	    }
	    destPtr -= BLOCK_LEN_SHORT;
	}	/*	i loop */

	unfold (srcPtr, transBuff, 1, (BLOCK_LEN_SHORT + BLOCK_LEN_LONG) / 2);
	ITransformBlock( transBuff, STOP_BLOCK, wnd_shape );
	/* Add first half of short window and old data */
	transBuffPtr = transBuff;
	for( i=0; i < BLOCK_LEN_SHORT; i++ )  {
	    *destPtr++ += *transBuffPtr++;
	}
	/* Copy new data to output buffer and update timeBuff fifo */
	destPtr = timeOutPtr;
	timeBuffPtr = timeBuff;
	for (i = 0; i < BLOCK_LEN_LONG; i++ )  {
	    *destPtr++ = *timeBuffPtr;
	    *timeBuffPtr++ = *transBuffPtr++;
	}
    }

    else if (blockType == LONG_START)  {
	unfold(freqInPtr, transBuff, 1, 960);

#ifdef MFDBGx
	plot_ary (transBuff, 1920, 0, 0., 0., "STARTADV POST UNFOLD");
#endif

	ITransformBlock (transBuff, START_ADV_BLOCK, wnd_shape );

	/* Add first half and old data */
	transBuffPtr = transBuff;
	timeBuffPtr = timeBuff;
	destPtr = timeOutPtr;
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
	}
	/* Save second half as old data */
	timeBuffPtr = timeBuff;	/*	NWINADV = 896	*/
	for (i = 0; i < NWINADV; i++)  { /* mab changed to i<512  from i < NWINADV*/
	    *timeBuffPtr++ = *transBuffPtr++;
	}
	for ( ; i < (2*(BLOCK_LEN_LONG)); i++)  { /* mab changed to i<960 from i<BLOCK_LEN_LONG */
	    *timeBuffPtr++ = 0.;
	}
    }

    else if (blockType == LONG_STOP)  {
	unfold (freqInPtr, transBuff, 1, 960);
	/* Do 1 LONG transforms */
	ITransformBlock (transBuff, STOP_ADV_BLOCK, wnd_shape );
	/* Add first half and old data */
	transBuffPtr = transBuff;
	timeBuffPtr = timeBuff;
	destPtr = timeOutPtr;
	for (i = 0 ; i < (BLOCK_LEN_LONG - 896); i++)  {
	    *destPtr++ = *timeBuffPtr++;
	}
	for ( ; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
	}
	/* Save second half as old data */
	timeBuffPtr = timeBuff;
	for ( ; i < (2*(BLOCK_LEN_LONG)); i++ )  {
	    *timeBuffPtr++ = *transBuffPtr++;
	}
    }

    else if (blockType == SHORT_EXT_STOP)  {
	/* Do 4 SHORT, 1 STOP transforms */
	destPtr = timeBuff + 3 * BLOCK_LEN_SHORT;
	for (i = 0; i < 4; i++)  {
	    unfold (freqInPtr, transBuff, 1, BLOCK_LEN_SHORT);
	    freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
	    ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape );
	    /* Add first half of short window and old data */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++ )  {
		*destPtr++ += *transBuffPtr++;
	    }
	    /* Save second half of short window */
	    for( j=0; j<BLOCK_LEN_SHORT; j++ )  {
		*destPtr++ = *transBuffPtr++;
	    }
	    destPtr -= BLOCK_LEN_SHORT;
	}
	unfold (freqInPtr, transBuff, 1, (BLOCK_LEN_SHORT+BLOCK_LEN_LONG)/2);
	ITransformBlock (transBuff, STOP_BLOCK, wnd_shape );
	/* Add first half of short window and old data */
	transBuffPtr = transBuff;
	for( i=0; i < BLOCK_LEN_SHORT; i++ )  {
	    *destPtr++ += *transBuffPtr++;
	}
	/* Copy new data to output buffer and update timeBuff fifo */
	destPtr = timeOutPtr;
	timeBuffPtr = timeBuff;
	for (i = 0; i < BLOCK_LEN_LONG; i++ )  {
	    *destPtr++ = *timeBuffPtr;
	    *timeBuffPtr++ = *transBuffPtr++;
	}
    }

    else if (blockType == NINE_SHORT)  {
	/* Do 9 SHORT transforms */
	destPtr = timeBuff + 3 * BLOCK_LEN_SHORT;

	for (i = 0; i < 9; i++) {
	    unfold( freqInPtr, transBuff, 1, BLOCK_LEN_SHORT );  /* !!! needs to be adjusted for "cramming" */
	    freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
	    ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape );

	    /* Add first half of short window and old data */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
		*destPtr++ += *transBuffPtr++;
	    }
#ifdef DISPLAY_NT /* MFDBG	*/
	    {
		plot_ary (destPtr - BLOCK_LEN_SHORT, BLOCK_LEN_SHORT, 0, 5000., -5000., "9Short OvlpAdd");
	    }
#endif
	    /* Save second half of short window */
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
		*destPtr++ = *transBuffPtr++;
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
	for ( ; i < (2*(BLOCK_LEN_LONG)); i++)  {
	    *destPtr++ = *timeBuffPtr++;
	}
	dolbyShortOffset = 1;
    }

    else if (blockType == OLD_START)  {
	unfold(freqInPtr, transBuff, 1, BLOCK_LEN_LONG);
	ITransformBlock (transBuff, START_FLAT_BLOCK, wnd_shape );
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
	dolbyShortOffset = 0;
    }

    else if (blockType == OLD_STOP)  {
	unfold (freqInPtr, transBuff, 1, BLOCK_LEN_LONG);
	/* Do 1 LONG transforms */
	ITransformBlock (transBuff, STOP_FLAT_BLOCK, wnd_shape );
	/* Add first half and old data */
	transBuffPtr = transBuff;
	timeBuffPtr = timeBuff;
	destPtr = timeOutPtr;
	for (i = 0; i < BLOCK_LEN_LONG - NFLAT; i++)  {
	    *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
	}
	for ( ; i < BLOCK_LEN_LONG; i++)  {
	    *destPtr++ = *transBuffPtr++;
	}
	/* Save second half as old data */
	timeBuffPtr = timeBuff;
	for (i = 0; i < BLOCK_LEN_LONG; i++ )  {
	    *timeBuffPtr++ = *transBuffPtr++;
	}
    }

    else {
	printf( "Illegal Block_type %d in time2freq_adapt(), aborting ...\n", blockType );
	exit( 1 );
    }

    for (i = 0; i < BLOCK_LEN_LONG; i++)  {

	ftimeOutPtr [i] = (float) timeOutPtr [i];
	/*		ftimeOutPtr [i] = 1.;	*/

    }
#ifdef DISPLAY_NT /* MFDBG	*/
    plot_fary (ftimeOutPtr, BLOCK_LEN_LONG, 0, 0., 0., "OVLPADD OUT");
#endif

}

#ifdef BTOB
/*	getdata.c		*/

#include <stdlib.h>

void getmydata (int *blocktype, int *windowtype, short *ch,
float *coefficients, int *ncoefficients) {

    static int first = 1, i, k;
    static FILE *ptr;
    static double ary [4096];
    union  {
	int ii;
	unsigned char cii [4];
    } u1;
    union _u2 {
	double jj;
	unsigned char cjj [8];
    } u2;
    char c [8];




    if (first) {
	first = 0;
	if (! (ptr = fopen ("savedata.dat", "rb"))) {
	    printf ("Cannot open b-to-b infile savedata.dat\n");
	    exit (1);
	}
	printf ("Read b-to-b datafile savedata.dat\n");
    }
    if (feof (ptr)) {
	printf ("getdata: end of file savedata.dat\n");
	*blocktype = *windowtype = *ncoefficients = 0;
	exit (1);
	return;
    }

    fread (blocktype,  sizeof (int), 1, ptr);
    if (feof (ptr)) {
	printf ("getdata: end of file savedata.dat\n");
	*blocktype = *windowtype = *ncoefficients = 0;
	return;
    }
#ifdef DOS
    u1.ii = *blocktype;

    for (i = 0; i < 4; i++)
	c [3-i] = u1.cii [i];
    for (i = 0; i < 4; i++)
	u1.cii [i] = c [i];
    *blocktype = u1.ii;
#endif

    fread (windowtype, sizeof (int), 1, ptr);
#ifdef DOS
    u1.ii = *windowtype;

    for (i = 0; i < 4; i++)
	c [3-i] = u1.cii [i];
    for (i = 0; i < 4; i++)
	u1.cii [i] = c [i];
    *windowtype = u1.ii;
#endif


    fread (ncoefficients, sizeof (int), 1, ptr);
#ifdef DOS
    u1.ii = *ncoefficients;

    for (i = 0; i < 4; i++)
	c [3-i] = u1.cii [i];
    for (i = 0; i < 4; i++)
	u1.cii [i] = c [i];
    *ch = u1.ii;
#endif


    fread (ncoefficients, sizeof (int), 1, ptr);
#ifdef DOS
    u1.ii = *ncoefficients;

    for (i = 0; i < 4; i++)
	c [3-i] = u1.cii [i];
    for (i = 0; i < 4; i++)
	u1.cii [i] = c [i];
    *ncoefficients = u1.ii;
#endif

    if (*ncoefficients > 1152) {
	printf ("Oops, *ncoefficients = %d\n", *ncoefficients);
	exit (1);
    }


    fread (ary, sizeof (double), *ncoefficients, ptr);


    for (i = 0; i < *ncoefficients; i++) {
#ifdef DOS
	u2.jj = ary [i];

	for (k = 0; k < 8; k++)
	    c [7-k] = u2.cjj [k];
	for (k = 0; k < 8; k++)
	    u2.cjj [k] = c [k];
	ary [i] = u2.jj;
#endif
	coefficients [i] = ary [i];
    }
    printf ("Bt %d  wt %d nc %d  ch %d\n", 
        *blocktype, *windowtype, *ncoefficients, *ch);
}
#endif
