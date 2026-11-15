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

#ifndef BLOCK_H
#define BLOCK_H 1

#include "adifdec.h"
#include "interface.h"
#include "transfo.h"

#define IN_DATATYPE  Float
#define OUT_DATATYPE Float

#define SHORT_BLOCKS_IN_LONG_BLOCK      8       /* no of short blocks replacing one long block */
#define N_SHORT_IN_START                4       /* GAD 10/30/95 */
#define N_SHORT_IN_STOP                 3

#define NWINLONG	(BLOCK_LEN_LONG)
#define NWINSHORT	(BLOCK_LEN_SHORT)
#define	NWINFLAT	(NWINLONG)					/* flat params */
#define NFLAT		((NWINFLAT-NWINSHORT)/2)

typedef enum {
    WS_FHG, WS_DOLBY, N_WINDOW_SHAPES
} Window_shape;

typedef enum {
    WT_LONG, 
    WT_SHORT, 
    WT_FLAT, 
    N_WINDOW_TYPES
} WINDOW_TYPE;

typedef struct {
    byte    this_bk;
    byte    prev_bk;
} Wnd_Shape;


void InitBlock(void);

void ITransformBlock (Float* dataPtr, int wT, Wnd_Shape *ws);
void TransformBlock (Float* dataPtr, int bT, Wnd_Shape *wnd_shape);

#if 0
void getmydata (int *blocktype, int *windowtype, short *ch,
                Float *coefficients, int *ncoefficients);
#endif

#endif	/*	BLOCK_H	*/
