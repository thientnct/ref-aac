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

#ifndef	_tns_h_
#define	_tns_h_

#include "adifdec.h"
#include "interface.h"

#define TNS_MAX_BANDS	49
#define TNS_MAX_ORDER	31
#define	TNS_MAX_WIN	8
#define TNS_MAX_FILT	3

typedef struct
{
    int start_band;
    int stop_band;
    int order;
    int direction;
    int coef_compress;
    short coef[TNS_MAX_ORDER];
} TNSfilt;

typedef struct 
{
    int n_filt;
    int coef_res;
    TNSfilt filt[TNS_MAX_FILT];
} TNSinfo;

typedef struct 
{
    int n_subblocks;
    TNSinfo info[TNS_MAX_WIN];
} TNS_frame_info;

void		clr_tns( Info *info, TNS_frame_info *tns_frame_info );
int		get_tns( Info *info, TNS_frame_info *tns_frame_info );
void		print_tns( TNSinfo *tns_info);
void		tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order );
void		tns_decode_coef( int order, int coef_res, short *coef, Float *a );
void		tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong, TNSinfo *tns_info);
void            tns_filter_subblock(Float *spec, int nbands, short *sfb_top, int islong,
				    TNSinfo *tns_info);


#endif	/* _tns_h_ */
