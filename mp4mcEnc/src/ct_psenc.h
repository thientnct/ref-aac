/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

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
Copyright (c) ISO/IEC 2002.

$Id: ct_sbr.c,v 1.6 2003/01/29 15:29:39 ct1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  SBR PS encoding functions $Revision: 1.6 $
*/

#ifndef _CT_PSENC_H
#define _CT_PSENC_H

#include "bitstream.h"
#include "ct_polyphase.h"

#define PS_BANDS 10

typedef struct {
  float pwrLbuf[PS_BANDS];
  float pwrRbuf[PS_BANDS];
  float pwrCbuf[PS_BANDS];
  
  int iidIdx[PS_BANDS];
  int iccIdx[PS_BANDS];
  int iidCode[PS_BANDS];
  int iccCode[PS_BANDS];
  int bs_extension_size;
  int bs_esc_count;
  int num_bits_left;
} PS_DATA;

void SbrPsInit(PS_DATA *psData);

void SbrPsEncode(PS_DATA *psData,
                 float lrBuffer[32][64],
                 float liBuffer[32][64],
                 float rrBuffer[32][64],
                 float riBuffer[32][64]);

void WriteSbrPs(PS_DATA *psData,
               BsBitStream *ptrBs);

int CountSbrPs(PS_DATA *psData);

#endif
