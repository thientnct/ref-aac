/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

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

/* This header file is for the gain control tool. The common include files,
defines and typedefs are declared. */

#ifndef	_gc_h_
#define _gc_h_

#include "block.h"
#include "adifdec.h"

#define NBANDSBITS		2
#define NATKSBITS		3
#define IDGAINBITS		4
#define ATKLOCBITS		5
#define ATKLOCBITS_START_A	4
#define ATKLOCBITS_START_B	2
#define ATKLOCBITS_SHORT	2
#define ATKLOCBITS_STOP_A	4
#define ATKLOCBITS_STOP_B	5
#define	NBANDS			4
#define NPQFTAPS		96
#define	NPEPARTS		64	/* Num of PreEcho Inhibition Parts */ 

#define	SHORT_WIN_IN_LONG	8
#define	GC_BLOCKS_LONG	1
#define	GC_BLOCKS_SHORT	SHORT_WIN_IN_LONG
#define	GC_BLOCKS_START	2
#define	GC_BLOCKS_STOP	2

#define	mylog2(x)		(mylog10(x)/mylog10(2))
#define	mylog10(x)		((((double)x)>1e-20) ? \
				log10((double)(x)) : log10((double)1e-20))
#define	npow2(x)		(1L << (x))			/* 2^x */

typedef struct {
        int lev;
        int loc;
} GAIN_DATA;
 
typedef struct {
        int      num_gain_data;
        GAIN_DATA gData[7];
} GAIN_INF;

typedef struct {
        GAIN_INF gBlock[8];
} GAIN_BLOCK;

/* definition of filterbank control & info structure */

typedef struct {
        int     max_band;
        GAIN_BLOCK gInfo[4];
} GAIN_CONTROL_DATA;

enum	window_sequence {
	ONLY_LONG_SEQUENCE = 0x0,
	LONG_START_SEQUENCE,
	EIGHT_SHORT_SEQUENCE,
	LONG_STOP_SEQUENCE
	};

typedef	struct	{
	double	re;		/* Real part */
	double	im;		/* Imaginary part */
}	COMPLEX; /* Complex number */


extern   int ssr_decoder_band;


void gc_ipqf_main(double    *bandSignalCh[NBANDS], 
                  int       block_size_samples,
		  int       ch, 
                  double    timeSignalCh[]);
void gc_gainc_window(int        nsamples,
                     GAIN_INF   gain0,	
                     GAIN_BLOCK gain1,
                     double     *p_ad,
                     int        block_type,
                     int        block_id);
void gain_control(Float     *freqSigCh,
                  Wnd_Shape *wnd_shape,
                  int       window_sequenceCh,
                  int       ch,
                  Float     *timeSigCh);
void gc_compensate(double input[], 
                   int block_size_samples,
                   int window_sequence, 
                   int	ch,
                   int band, 
                   double *out[]);
void	gc_imdct_main(Float  *freqSignalCh,Wnd_Shape *wnd_shape,
                      int    window_sequence,
		      double *timeSignalChWithGCandOverlapping);

void    gc_set_protopqf(double *p_proto);
void    gc_fft(COMPLEX	*p_c0, COMPLEX	*p_c1, int	ln, int	idir);
int     gc_nlog(int m, int n);
COMPLEX gc_cexp(double r, double a);
COMPLEX gc_cmul(COMPLEX c0, COMPLEX c1);
COMPLEX gc_cgen(double x, double y);

void    destroyGC(void);

#endif	/* _gc_h_ */
/*******************************************************************
                        End of gc.h
*******************************************************************/

