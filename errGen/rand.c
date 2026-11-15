/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT Mobile
Communications Network, Inc. in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and
3. This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4
Audio standard. ISO/IEC gives users of the MPEG-2 NBC/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 NBC/MPEG-4 Audio standards. Those intending to use this
software module in hardware or software products are advised that this
use may infringe existing patents. The original developer of this
software module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-2 NBC/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 NBC/MPEG-4 Audio
conforming products. This copyright notice must be included in all
copies or derivative works." Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
/*
 *   Generate random value
 *   Copyright (C) 1994 NTT Mobile Communications Network, Inc. (DoCoMo)
 *                      Toshio  Miki     (miki@mlab.nttdocomo.co.jp)
 *                      Toshiro Kawahara (kawahara@mlab.nttdocomo.co.jp)
 */
#include <stdio.h>

/*
 *  Generate (0.0, 1.0) random value
 *  Ref.  D.E.Knuth, "The Art of Computer Programming", Chap.3
 */
#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

static int iy = 0;
static long iv[NTAB];


void
snap_frand()
{
  iy = 0;
}

void
retrieve_frand()
{
}


float
frand(iseed)
     long *iseed;
{
  int i;
  long k;
  float tmp;

  if(iy == 0 || *iseed<= 0){	/* Initialize */
    if(*iseed == 0) *iseed = 141421356;	/* iseed must not 0 */
    for(i=NTAB+10; i>=0; i--){
      k = *iseed / IQ;
      *iseed = IA * (*iseed - k*IQ) - k*IR;
      if(*iseed < 0) *iseed += IM;
      if(i < NTAB) iv[i] = *iseed;
    }
    iy = iv[0];
  }
  k = *iseed/IQ;
  *iseed = IA * (*iseed - k*IQ) - k*IR;
  if(*iseed < 0) *iseed += IM;
  i = iy/NDIV;
  iy = iv[i];
  iv[i] = *iseed;
  tmp = AM*iy;
  if(tmp > RNMX) 
    return RNMX;
  else 
    return tmp;
}
