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

#include "intrins.h"
#include "bitinput.h" /* getbits() */
#include "tns.h"

#ifndef min
#define min(x, y)   ( ((x) < (y) ) ? (x) : (y) )
#endif

#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )

/* Decoder transmitted coefficients for one TNS filter */
void
tns_decode_coef( int order, int coef_res, short *coef, Float *a )
{
    int i, m;
    Float iqfac,  iqfac_m;
    Float tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];

    /* Inverse quantization */
    iqfac   = ((1 << (coef_res-1)) - 0.5) / (C_PI/2.0);
    iqfac_m = ((1 << (coef_res-1)) + 0.5) / (C_PI/2.0);
    for (i=0; i<order; i++)  {
	tmp[i+1] = sin( coef[i] / ((coef[i] >= 0) ? iqfac : iqfac_m) );
    }
    /* Conversion to LPC coefficients
     * Markel and Gray,  pg. 95
     */
    a[0] = 1;
    for (m=1; m<=order; m++)  {
	b[0] = a[0];
	for (i=1; i<m; i++)  {
	    b[i] = a[i] + tmp[m] * a[m-i];
	}
	b[m] = tmp[m];
	for (i=0; i<=m; i++)  {
	    a[i] = b[i];
	}
    }

}

/* apply the TNS filter */
void
tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order )
{
    /*
     *	- Simple all-pole filter of order "order" defined by
     *	  y(n) =  x(n) - a(2)*y(n-1) - ... - a(order+1)*y(n-order)
     *
     *	- The state variables of the filter are initialized to zero every time
     *
     *	- The output data is written over the input data ("in-place operation")
     *
     *	- An input vector of "size" samples is processed and the index increment
     *	  to the next data sample is given by "inc"
     */
    int i, j;
    Float y, state[TNS_MAX_ORDER];

    for (i=0; i<order; i++)
	state[i] = 0;

    if (inc == -1)
	spec += size-1;

    for (i=0; i<size; i++) {
	y = *spec;
	for (j=0; j<order; j++)
	    y -= lpc[j+1] * state[j];
	for (j=order-1; j>0; j--)
	    state[j] = state[j-1];
	state[0] = y;
	*spec = y;
	spec += inc;
    }
}

/* TNS decoding for one channel and frame */
void
tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong,
	TNSinfo *tns_info)
{
    int f, m, start, stop, size, inc;
    int n_filt, coef_res, order, direction;
    short *coef;
    Float lpc[TNS_MAX_ORDER+1];
    TNSfilt *filt;

    n_filt = tns_info->n_filt;
    for (f=0; f<n_filt; f++)  {
	coef_res = tns_info->coef_res;
	filt = &tns_info->filt[f];
	order = filt->order;
	direction = filt->direction;
	coef = filt->coef;
	start = filt->start_band;
	stop = filt->stop_band;

	m = tns_max_order(islong);
	if (order > m) {
	    PRINT(SE, "Error in tns max order: %d %d\n",
		order, tns_max_order(islong));
	    order = m;
	}
	if (!order)  continue;

	tns_decode_coef( order, coef_res, coef, lpc );

	start = min(start, tns_max_bands(islong));
        start = min(start, nbands);
	start = sfb_offset( start );
	
	stop = min(stop, tns_max_bands(islong));
        stop = min(stop, nbands);
	stop = sfb_offset( stop );
	if ((size = stop - start) <= 0)  continue;

	if (direction)  {
	    inc = -1;
	}  else {
	    inc =  1;
	}

	tns_ar_filter( &spec[start], size, inc, lpc, order );
    }
}


/********************************************************/
/* TnsInvFilter:                                        */
/*   Inverse filter the given spec with specified       */
/*   length using the coefficients specified in filter. */
/*   Not that the order and direction are specified     */
/*   withing the TNS_FILTER_DATA structure.             */
/********************************************************/
static
void TnsInvFilter(int length, Float *spec, TNSfilt *filter, Float *coef) 
{
  int i,j,k = 0;
  int order = filter->order;
  Float *a = coef;
  Float *temp;
  
  temp = (Float *) mal1 (length * sizeof (Float));
  
  /* Determine loop parameters for given direction */
  if (filter->direction) 
  {
    /* Startup, initial state is zero */
    temp[length-1]=spec[length-1];
    for (i=length-2;i>(length-1-order);i--) {
      temp[i]=spec[i];
      k++;
      for (j=1;j<=k;j++) {
	spec[i]+=temp[i+j]*a[j];
      }
    }
    
    /* Now filter the rest */
    for	(i=length-1-order;i>=0;i--) {
      temp[i]=spec[i];
      for (j=1;j<=order;j++) {
	spec[i]+=temp[i+j]*a[j];
      }
    }
    
    
  } else {
    /* Startup, initial state is zero */
    temp[0]=spec[0];
    for (i=1;i<order;i++) {
      temp[i]=spec[i];
      for (j=1;j<=i;j++) {
	spec[i]+=temp[i-j]*a[j];
      }
    }
    
    /* Now filter the rest */
    for	(i=order;i<length;i++) {
      temp[i]=spec[i];
      for (j=1;j<=order;j++) {
	spec[i]+=temp[i-j]*a[j];
      }
    }
  }
  
  free1(temp);
}


void
tns_filter_subblock(Float *spec, int nbands, short *sfb_top, int islong,
		    TNSinfo *tns_info)
{
  int f, m, start, stop, size;
  int n_filt, coef_res, order;
  short *coef;
  Float lpc[TNS_MAX_ORDER+1];
  TNSfilt *filt;
  
  n_filt = tns_info->n_filt;
  for (f=0; f<n_filt; f++)  
  {
    coef_res = tns_info->coef_res;
    filt = &tns_info->filt[f];
    order = filt->order;
    coef = filt->coef;
    start = filt->start_band;
    stop = filt->stop_band;
    
    m = tns_max_order(islong);
    if (order > m)
      order = m;
    
    if (!order)  continue;
    
    tns_decode_coef( order, coef_res, coef, lpc );
    
    start = min(start, tns_max_bands(islong));
    start = min(start, nbands);
    start = sfb_offset( start );
    
    stop = min(stop, tns_max_bands(islong));
    stop = min(stop, nbands);
    stop = sfb_offset( stop );
    if ((size = stop - start) <= 0)  
      continue;
 
    TnsInvFilter(size, spec + start, filt, lpc);  
  }
}


void
clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int s;

  tns_frame_info->n_subblocks = info->nsbk;
  for (s=0; s<tns_frame_info->n_subblocks; s++)
    tns_frame_info->info[s].n_filt = 0;
}

int
get_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static short              sgn_mask[] = { 
    0x2, 0x4, 0x8     };
  static unsigned short              neg_mask[] = { 
    0xfffc, 0xfff8, 0xfff0     };


  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];

    if (!(tns_info->n_filt = getbits( short_flag ? 1 : 2 )))
      continue;
	    
    tns_info -> coef_res = res = getbits( 1 ) + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      top = tns_filt->start_band = top - getbits( short_flag ? 4 : 6 );
      tns_filt->order = getbits( short_flag ? 3 : 5 );

      if (tns_filt->order)  {
        tns_filt->direction = getbits( 1 );
        compress = getbits( 1 );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = getbits( res2 );
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
  return 1;
}


void
print_tns( TNSinfo *tns_info)
{
    int           f, t;
    static char   *s = "TNS>> ";

    PRINT(SE, "%s n_filt: %d\n", s, tns_info -> n_filt );
    if (tns_info->n_filt)
	PRINT(SE, "%s res   : %d\n", s, tns_info -> coef_res );
    for (f=0; f<tns_info->n_filt; f++)  {

	PRINT(SE, "%s filt %d: [%d %d] o=%d", s, f, 
	    tns_info->filt[f].start_band,
	    tns_info->filt[f].stop_band,
	    tns_info->filt[f].order );

	if (tns_info->filt[f].order)  {
	    PRINT(SE, " d=%d | " ,tns_info->filt[f].direction  );

	    for (t=0; t<tns_info->filt[f].order; t++)
		PRINT(SE, "%d ", tns_info->filt[f].coef[t] );

	}
	PRINT(SE, "\n" );
    }
    PRINT(SE, "%s ------------\n", s );
}
