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

#include "huffinit.h"
#include "huffdec_intern.h"
#include "bitinput.h" /* getbits() */


#ifdef ARITHMETIC_TORTURE_TEST
int *hcw_touched[NBOOKS]={NULL};
Huffman* huff_book_list[NBOOKS];
int not_touched_coeffs, not_touched_scfs;
#endif


/*
 * Cword is working buffer to which shorts from
 *   bitstream are written. Bits are read from msb to lsb
 * Nbits is number of lsb bits not yet consumed
 * 
 * this uses a minimum-memory method of Huffman decoding
 */
int decode_huff_cw(Huffman *h)
{
    int i, j;
    unsigned long cw;
#ifdef ARITHMETIC_TORTURE_TEST
    int*touch_hcb=NULL, b;
    for (b=0;b<NBOOKS;b++)
      if (huff_book_list[b]==h) {
        touch_hcb=hcw_touched[b];break;
      }
#endif
    
    i = h->len;
    cw = getbits(i);
    while (cw != h->cw) {
	h++;
	j = h->len-i;
	i += j;
	cw <<= j;
	cw |= getbits(j);
    }

#ifdef ARITHMETIC_TORTURE_TEST
    if (touch_hcb) {
      if (!touch_hcb[h->index]) {
        touch_hcb[h->index]=1;
        if (b!=BOOKSCL-1) not_touched_coeffs--;
        else not_touched_scfs--;
      }
    }
#endif

    return(h->index);
}


void unpack_idx(int *qp, int idx, Hcb *hcb)
{
    int dim = hcb->dim;
    int mod = hcb->mod;
    int off = hcb->off;
    if(dim == 4){
	qp[0] = (idx/(mod*mod*mod)) - off;
	idx -= (qp[0] + off)*(mod*mod*mod);
	qp[1] = (idx/(mod*mod)) - off;
	idx -= (qp[1] + off)*(mod*mod);
	qp[2] = (idx/(mod)) - off;
	idx -= (qp[2] + off)*(mod);
	qp[3] = (idx) - off;
    }
    else {
	qp[0] = (idx/(mod)) - off;
	idx -= (qp[0] + off)*(mod);
	qp[1] = (idx) - off;
    }
}


#ifdef ARITHMETIC_TORTURE_TEST
int not_touched_escapes=0;
int *esc_touched=NULL;
#endif

int getescape(int q)
{
  int i, off, neg;

  if(q < 0){
    if(q != -16)
      return q;
    neg = 1;
  } else{
    if(q != +16)
      return q;
    neg = 0;
  }

  for(i=4;; i++){
    if(getbits(1) == 0)
      break;
  }

  if(i > 16){
    /* this may not happen, as 'i' is limited by 12 (see 138187-1 10.3) */
    off = getbits(i-16) << 16;
    off |= getbits(16);
  } else
    off = getbits(i);

  i = off + (1<<i);
#ifdef ARITHMETIC_TORTURE_TEST
  if (!esc_touched[i]) {
    esc_touched[i]=1;
    not_touched_escapes--;
  }
#endif
  if(neg)
    i = -i;
  return i;
}


/* rm2 inverse quantization
 * escape books need ftn call
 * other books done via macro
 */
#define iquant( q ) ( q >= 0 ) ? \
	(Float)iq_exp_tbl[ q ] : (Float)(-iq_exp_tbl[ - q ])

Float esc_iquant(int q)
{
  if (q > 0) {
    if (q < MAX_IQ_TBL) {
      return((Float)iq_exp_tbl[q]);
    }
    else {
      return(pow(q, 4./3.));
    }
  }
  else {
    q = -q;
    if (q < MAX_IQ_TBL) {
      return((Float)(-iq_exp_tbl[q]));
    }
    else {
      return(-pow(q, 4./3.));
    }
  }
}


/* get sign bits */
void get_sign_bits(int *q, int n)
{
  while (n) {
    if (*q) {
      if (getbits(1)) {      /* 1 signals negative, as in 2's complement' */
        *q = -*q;
      }
    }
    n--;q++;
  }
}
