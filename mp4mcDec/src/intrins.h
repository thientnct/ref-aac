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

#ifndef _intrins_h_
#define _intrins_h_

#include "adifdec.h"

extern	long		total1;
extern	long		total2;


void		byteclr(byte *ip1, int cnt);
long		f2ir(Float x);
void		fltclr(Float *dp1, int cnt);
void		fltcpy(Float *dp1, Float *dp2, int cnt);
void		fltset(Float *dp1, Float dval, int cnt);
void		intclr(int *ip1, int cnt);
void		intcpy(int *ip1, int *ip2, int cnt);
void*		mal1(long size);
void            free1(void* pointer);
void*		mal2(long size);
void		shortclr(short *ip1, int cnt);

#endif
