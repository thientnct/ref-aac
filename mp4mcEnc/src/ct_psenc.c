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

#include <math.h>
#include "ct_psenc.h"

#define SI_SBR_EXTENSION_SIZE_BITS              4
#define SI_SBR_EXTENSION_ESC_COUNT_BITS         8
#define SI_SBR_EXTENSION_ID_BITS                2

#define EXTENSION_ID_PS_CODING                  2

#define FLOAT_EPSILON 1.0
#define PS_OFFSET 31

/* 10 band config */

static const int borderQmf[PS_BANDS] = {
  1, 1, 2, 3, 5, 7, 9, 14, 23, 64
};

/* IID (coarse) & ICC quantization */

static const float iidQuant[15] = {
  -25, -18, -14, -10, -7, -4, -2, 0, 2, 4, 7, 10, 14, 18, 25
};

static const float iccQuant[8] = {
  1, 0.937, 0.84118, 0.60092, 0.36764, 0, -0.589, -1
};

/* IID (coarse) & ICC Huffman codebooks */

const int aBookPsIidTimeCode[29] = 
{
 0x0007FFF9,  0x0007FFFA,  0x0007FFFB,  0x000FFFF8,  0x000FFFF9,  0x000FFFFA, 
 0x0001FFFD,  0x00007FFE,  0x00000FFE,  0x000003FE,  0x000000FE,  0x0000003E, 
 0x0000000E,  0x00000002,  0x00000000,  0x00000006,  0x0000001E,  0x0000007E, 
 0x000001FE,  0x000007FE,  0x00001FFE,  0x00003FFE,  0x0001FFFC,  0x0007FFF8, 
 0x000FFFFB,  0x000FFFFC,  0x000FFFFD,  0x000FFFFE,  0x000FFFFF
};

const int aBookPsIidTimeLength[29] = 
{
 0x00000013,  0x00000013,  0x00000013,  0x00000014,  0x00000014,  0x00000014, 
 0x00000011,  0x0000000F,  0x0000000C,  0x0000000A,  0x00000008,  0x00000006, 
 0x00000004,  0x00000002,  0x00000001,  0x00000003,  0x00000005,  0x00000007, 
 0x00000009,  0x0000000B,  0x0000000D,  0x0000000E,  0x00000011,  0x00000013, 
 0x00000014,  0x00000014,  0x00000014,  0x00000014,  0x00000014
};

const int aBookPsIidFreqCode[29] = 
{
 0x0001FFFB,  0x0001FFFC,  0x0001FFFD,  0x0001FFFA,  0x0000FFFC,  0x00007FFC, 
 0x00001FFD,  0x000003FE,  0x000001FE,  0x0000007E,  0x0000003C,  0x0000001D, 
 0x0000000D,  0x00000005,  0x00000000,  0x00000004,  0x0000000C,  0x0000001C, 
 0x0000003D,  0x0000003E,  0x000000FE,  0x000007FE,  0x00001FFC,  0x00003FFC, 
 0x00003FFD,  0x00007FFD,  0x0001FFFE,  0x0003FFFE,  0x0003FFFF
};

const int aBookPsIidFreqLength[29] = 
{
 0x00000011,  0x00000011,  0x00000011,  0x00000011,  0x00000010,  0x0000000F, 
 0x0000000D,  0x0000000A,  0x00000009,  0x00000007,  0x00000006,  0x00000005, 
 0x00000004,  0x00000003,  0x00000001,  0x00000003,  0x00000004,  0x00000005, 
 0x00000006,  0x00000006,  0x00000008,  0x0000000B,  0x0000000D,  0x0000000E, 
 0x0000000E,  0x0000000F,  0x00000011,  0x00000012,  0x00000012
};

const int aBookPsIccTimeCode[15] = 
{
 0x00003FFE,  0x00001FFE,  0x000007FE,  0x000001FE,  0x0000007E,  0x0000001E, 
 0x00000006,  0x00000000,  0x00000002,  0x0000000E,  0x0000003E,  0x000000FE, 
 0x000003FE,  0x00000FFE,  0x00003FFF
};

const int aBookPsIccTimeLength[15] = 
{
 0x0000000E,  0x0000000D,  0x0000000B,  0x00000009,  0x00000007,  0x00000005, 
 0x00000003,  0x00000001,  0x00000002,  0x00000004,  0x00000006,  0x00000008, 
 0x0000000A,  0x0000000C,  0x0000000E
};

const int aBookPsIccFreqCode[15] = 
{
 0x00003FFF,  0x00003FFE,  0x00000FFE,  0x000003FE,  0x0000007E,  0x0000001E, 
 0x00000006,  0x00000000,  0x00000002,  0x0000000E,  0x0000003E,  0x000000FE, 
 0x000001FE,  0x000007FE,  0x00001FFE
};

const int aBookPsIccFreqLength[15] = 
{
 0x0000000E,  0x0000000E,  0x0000000C,  0x0000000A,  0x00000007,  0x00000005, 
 0x00000003,  0x00000001,  0x00000002,  0x00000004,  0x00000006,  0x00000008, 
 0x00000009,  0x0000000B,  0x0000000D
};


void SbrPsInit(PS_DATA *psData)
{
  int i;

  for (i=0; i<PS_BANDS; i++) {
    psData->pwrLbuf[i] = 0;
    psData->pwrRbuf[i] = 0;
    psData->pwrCbuf[i] = 0;
  }
}

void SbrPsEncode(PS_DATA *psData,
                 float lrBuffer[32][64],
                 float liBuffer[32][64],
                 float rrBuffer[32][64],
                 float riBuffer[32][64])
{
  int i, j, last;
  int s, b;
  float f;
  float lr,li,rr,ri;

  float pwrL[PS_BANDS];
  float pwrR[PS_BANDS];
  float pwrC[PS_BANDS];
  float iid[PS_BANDS];
  float icc[PS_BANDS];

  /* calculate energies and correlation */
  b = 0;
  for (i=0; i<PS_BANDS; i++) {
    if (i==1)
      b = 0; /* split first QMF by simple hybrid filter */
    for (; b<borderQmf[i]; b++) {
      for (s=(i>1)?0:1; s<32; s++) {
        lr = lrBuffer[s][b];
        li = liBuffer[s][b];
        rr = rrBuffer[s][b];
        ri = riBuffer[s][b];
        if (i<2) {
          f = (i)?-1:1;   /* i=0: lowpass   i=1: highpass */
          lr += f*lrBuffer[s-1][b];
          li += f*liBuffer[s-1][b];
          rr += f*rrBuffer[s-1][b];
          ri += f*riBuffer[s-1][b];
        }
        psData->pwrLbuf[i] += lr*lr+li*li;
        psData->pwrRbuf[i] += rr*rr+ri*ri;
        psData->pwrCbuf[i] += lr*rr+li*ri;
        if (s==PS_OFFSET && b+1==borderQmf[i]) {
          /* this analysis window is still shifted by 16 QMF slots */
          pwrL[i] = psData->pwrLbuf[i] + FLOAT_EPSILON;
          pwrR[i] = psData->pwrRbuf[i] + FLOAT_EPSILON;
          pwrC[i] = psData->pwrCbuf[i] + FLOAT_EPSILON;
          psData->pwrLbuf[i] = 0;
          psData->pwrRbuf[i] = 0;
          psData->pwrCbuf[i] = 0;
        }
      }
    }
  }

  /* calculate iid and icc */
  for (i=0; i<PS_BANDS; i++) {
    iid[i] = 20*log(sqrt(pwrL[i]/pwrR[i]))/log(10.);
    icc[i] = pwrC[i]/sqrt(pwrL[i]*pwrR[i]);
#if 0
    fprintf(stderr,"BAND %2d   IID %6.2f dB   ICC %6.3f\n",i,iid[i],icc[i]);
#endif
  }

  /* quantization */
  for (i=0; i<PS_BANDS; i++) {
    f = iid[i];
    j = -7;
    while (f>(iidQuant[j+7]+iidQuant[j+1+7])/2 && j<7)
      j++;
    psData->iidIdx[i] = j;
    f = icc[i];
    j = 0;
    while (f<(iccQuant[j]+iccQuant[j+1])/2 && j<7)
      j++;
    psData->iccIdx[i] = j;
  }

  /* frequency differential encoding */
  last = 0;
  for (i=0; i<PS_BANDS; i++) {
    psData->iidCode[i] = psData->iidIdx[i]-last+14;
    last = psData->iidIdx[i];
  }
  last = 0;
  for (i=0; i<PS_BANDS; i++) {
    psData->iccCode[i] = psData->iccIdx[i]-last+7;
    last = psData->iccIdx[i];
  }
}

void WriteSbrPs(PS_DATA *psData,
               BsBitStream *ptrBs)
{
  int i;

  BsPutBit(ptrBs, psData->bs_extension_size, SI_SBR_EXTENSION_SIZE_BITS);
  if (psData->bs_extension_size==15)
    BsPutBit(ptrBs, psData->bs_esc_count, SI_SBR_EXTENSION_ESC_COUNT_BITS);

  BsPutBit(ptrBs, EXTENSION_ID_PS_CODING, SI_SBR_EXTENSION_ID_BITS);
  BsPutBit(ptrBs, 1, 1); /* enable_ps_header */
  BsPutBit(ptrBs, 1, 1); /* enable_iid */
  BsPutBit(ptrBs, 0, 3); /* iid_mode 10 bands, -7..7 */
  BsPutBit(ptrBs, 1, 1); /* enable_icc */
  BsPutBit(ptrBs, 0, 3); /* icc_mode 10 bands, 0..7, Ra */
  BsPutBit(ptrBs, 0, 1); /* enable_ext */
  BsPutBit(ptrBs, 0, 1); /* frame_class FIX */
  BsPutBit(ptrBs, 1, 2); /* num_env_idx 1 env */

  BsPutBit(ptrBs, 0, 1); /* iid_dt 0 */
  for (i=0; i<PS_BANDS; i++) {
    BsPutBit(ptrBs, aBookPsIidFreqCode[psData->iidCode[i]],
             aBookPsIidFreqLength[psData->iidCode[i]]); /* iid_par_df[i] */
  }

  BsPutBit(ptrBs, 0, 1); /* icc_dt 0 */
  for (i=0; i<PS_BANDS; i++) {
    BsPutBit(ptrBs, aBookPsIccFreqCode[psData->iccCode[i]],
             aBookPsIccFreqLength[psData->iccCode[i]]); /* icc_par_df[i] */
  }

  BsPutBit(ptrBs, 0, psData->num_bits_left);
}

int CountSbrPs(PS_DATA *psData)
{
  int bit_count = 0;
  int i,numByte;

  bit_count += 1; /* enable_ps_header */
  bit_count += 1; /* enable_iid */
  bit_count += 3; /* iid_mode 10 bands, -7..7 */
  bit_count += 1; /* enable_icc */
  bit_count += 3; /* icc_mode 10 bands, 0..7, Ra */
  bit_count += 1; /* enable_ext */
  bit_count += 1; /* frame_class FIX */
  bit_count += 2; /* num_env_idx 1 env */

  bit_count += 1; /* iid_dt 0 */
  for (i=0; i<PS_BANDS; i++) {
    bit_count += aBookPsIidFreqLength[psData->iidCode[i]]; /* iid_par_df[i] */
  }

  bit_count += 1; /* icc_dt 0 */
  for (i=0; i<PS_BANDS; i++) {
    bit_count += aBookPsIccFreqLength[psData->iccCode[i]]; /* icc_par_df[i] */
  }

  bit_count += SI_SBR_EXTENSION_ID_BITS;
#if 0
  fprintf(stderr,"\n BIT_COUNT %3d\n",bit_count);
#endif
  numByte = (bit_count+7)/8;
  psData->num_bits_left = 8*numByte-bit_count;
  psData->bs_extension_size = (numByte>15)?15:numByte;
  psData->bs_esc_count = numByte-15;
  bit_count += SI_SBR_EXTENSION_SIZE_BITS;
  if (psData->bs_extension_size==15)
    bit_count += SI_SBR_EXTENSION_ESC_COUNT_BITS;
  bit_count += psData->num_bits_left;
#if 0
  fprintf(stderr,"NUM_BITS_LEFT %d   SIZE %2d   ESC %3d   TOT_BIT_COUNT %3d\n",
          psData->num_bits_left,psData->bs_extension_size,
          psData->bs_extension_size,bit_count);
  for (i=0; i<PS_BANDS; i++) {
    fprintf(stderr,"BAND %2d   IID %3d df %3d   ICC %3d df %3d\n",i,
            psData->iidIdx[i],psData->iidCode[i],
            psData->iccIdx[i],psData->iccCode[i]);
  }
#endif

  return bit_count;
}
