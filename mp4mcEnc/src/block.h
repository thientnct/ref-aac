/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Takashi Koike (Sony Corporation),
  Yasuhiro Toguri(Sony Corporation)

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
#ifndef BLOCK_H
#define BLOCK_H 1

#include "transfo.h"
#include "dolby_def.h"


#define IN_DATATYPE  double
#define OUT_DATATYPE double

#define BLOCK_LEN_LONG	   1024
#define BLOCK_LEN_SHORT    128
#define BLOCK_LEN_LONG_S   960
#define BLOCK_LEN_SHORT_S  120
#define BLOCK_LEN_LONG_SSR	  256 /* 980129 added for SSR */
#define BLOCK_LEN_SHORT_SSR   32  /* 980129 added for SSR */

#define NWINLONG	(BLOCK_LEN_LONG)
#define ALFALONG	4.0
#define NWINSHORT	(BLOCK_LEN_SHORT)
#define ALFASHORT	7.0

#define	NWINFLAT	(NWINLONG)					/* flat params */
#define	NWINADV		(NWINLONG-NWINSHORT)		/* Advanced flat params */
#define NFLAT		((NWINFLAT-NWINSHORT)/2)
#define NADV0		((NWINADV-NWINSHORT)/2)


typedef enum {
    WS_FHG, WS_DOLBY, N_WINDOW_SHAPES
} 
Window_shape;

/* YT 970615 for Son_PP  */
typedef enum {
	OVERLAPPED,
	NON_OVERLAPPED
}
Mdct_in,Imdct_out;

typedef enum {
    WT_LONG, 
    WT_SHORT, 
    WT_FLAT, 
    WT_ADV,			/* Advanced flat window */
    N_WINDOW_TYPES
} 
WINDOW_TYPE_AAC; 

typedef enum {                  /* ADVanced transform types */
    LONG_BLOCK,
    START_BLOCK,
    SHORT_BLOCK,
    STOP_BLOCK,
    START_ADV_BLOCK,
    STOP_ADV_BLOCK,
    START_FLAT_BLOCK,
    STOP_FLAT_BLOCK,
    N_BLOCK_TYPES
} 
BLOCK_TYPE;

typedef enum {  		/* Advanced window sequence (frame) types */
    ONLY_LONG,
    LONG_START, 
    LONG_STOP,
    SHORT_START, 
    SHORT_STOP,
    EIGHT_SHORT, 
    SHORT_EXT_STOP,
    NINE_SHORT,
    OLD_START,
    OLD_STOP,
    N_WINDOW_SEQUENCES
} 
WINDOW_SEQUENCE;


#endif	/*	BLOCK_H	*/

