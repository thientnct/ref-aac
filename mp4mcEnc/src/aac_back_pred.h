/**********************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Nokia

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
Copyright (c) ISO/IEC 1997.
**********************************************************************/
/*********************************************************************
 *
 * aac_back_pred.c  -  AAC backward prediction subroutine
 *
 * Authors:
 * LY    Lin Yin, Nokia Research Center
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * ADD   Alberto Duenas, NDS <alberto@ndsuk.com>
 * JB    Jeremy Bennett, NDS <jbennett@ndsuk.com>
 *
 * Changes:
 * 07-jun-97   LY   Initial revision.
 * 14-sep-97   CL   Updated ALPHA, A, and B coefficients.
 * 03-dec-97   ADD & JB   Added prediction reset.
 *
**********************************************************************/

#ifndef _AAC_BACK_H_INCLUDED
#define _AAC_BACK_H_INCLUDED

#include <math.h>
#include "interface.h"
#include "all.h"
#include "aac_qc.h"

#define         FLEN_LONG              2048
#define         SBMAX_L                49
#define         LPC                    2
#define         ALPHA                  PRED_ALPHA
#define         A                      PRED_A
#define         B                      PRED_B
#define         MINVAR                 1.e-10

/* Reset every RESET_FRAME frames. */
#define		RESET_FRAME	       8 
 
int GetMaxPredSfb(int samplingRate);

void PredCalcPrediction( double *act_spec, 
			 double *last_spec, 
			 int btype, 
			 int nsfb, 
			 int *isfb_width,
			AACQuantInfo *quantInfo,
			 Ch_Info *channel_info,
			 int chanNum); 

void PredInit(void);

void CopyPredInfo(AACQuantInfo *right,AACQuantInfo *left);


#endif
