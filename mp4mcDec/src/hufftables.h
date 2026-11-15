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

#ifndef _hufftables_h_
#define _hufftables_h_

#include "huffinit.h"

extern	Huffman		book1[];
extern	Huffman		book2[];
extern	Huffman		book3[];
extern	Huffman		book4[];
extern	Huffman		book5[];
extern	Huffman		book6[];
extern	Huffman		book7[];
extern	Huffman		book8[];
extern	Huffman		book9[];
extern	Huffman		book10[];
extern	Huffman		book11[];
extern	Huffman		bookscl[];

typedef struct {
    int	    samp_rate;
    int	    nsfb1024;
    short*  SFbands1024;
    int	    nsfb128;
    short*  SFbands128;
} SR_Info;

extern	SR_Info		samp_rate_info[(1<<LEN_SAMP_IDX)] ;

#endif
