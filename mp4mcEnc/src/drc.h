/* $Id: drc.h,v 1.1 1998/07/27 19:21:11 lueck Exp $
 */
/***********

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Dolby Laboratories

and edited by
  Sony Corporation


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

***********/

#ifndef DRC_H
#define DRC_H

/******************************************************************** 
/
/ filename : drc.h
/ project : MPEG-2 AAC
/ author : MCF, Matt Fellers, Dolby Laboratories, Inc.		   
/ date : July 17, 1998
/ contents/description : header file for dynamic range control 
/
/ Changes:
/ 17-July-98	  MCF  Created file
/
*******************************************************************/

typedef struct {
	short drc_num_bands;
	short prog_ref_level_present;
	short pce_tag_present;
	short excluded_chns_present;
	short drc_bands_present;
	short prog_ref_level;
	short pce_instance_tag;
	short drc_band_incr;
	short *drc_band_top;
	short *dyn_rng_sgn;
	short *dyn_rng_ctl;
	short *exclude_mask;
} DRC_VARS;

/* dynamic range */
typedef struct {
	short prog_ref_level;
	short *dyn_rng_ctl;
	short *dyn_rng_sgn;
	short dynrng_enabl;
} DRC_INTF;

typedef struct {
	short exists;
	short nbytes;
	DRC_VARS *dvars;
} DRC_PK;

void initDynRng(void);
int setDynRngVars( DRC_INTF *dvars_inpt, 
				   enum WINDOW_TYPE block_type[MAX_TIME_CHANNELS],
				   int nchans);
int WriteDRCArray(BsBitStream* ptrBs,	  /* Pointer to bitstream */
				 int nchans);
void ReadDynRngWords(DRC_INTF *dvars_inpt, FILE *dr_metadata);
int WriteDRCInfo  (BsBitStream *fixedStream, 
				   DRC_VARS *dvars,
				   int nchans);
int WriteExclDRCInfo(BsBitStream *fixedStream,
						DRC_VARS *dvars,
						int nchans);

#endif
