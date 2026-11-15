/**********************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Texas Instruments

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
 * tns.h  -  AAC temporal noise shaping
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 *
 * Changes:
 * 20-oct-97   CL   Initial revision.
 *
**********************************************************************/
 
#ifndef _TNS_H_INCLUDED
#define _TNS_H_INCLUDED
 
#include <math.h>
#include <stdio.h>
#include "tf_main.h"
#include "interface.h"
#include "mp4.h"
#include "common.h"

/*************************/
/* #defines              */
/*************************/
#define TNS_MAX_ORDER 20				   
#define DEF_TNS_GAIN_THRESH 1.4
#define DEF_TNS_COEFF_THRESH 0.1
#define DEF_TNS_COEFF_RES 4
#define DEF_TNS_RES_OFFSET 3
#define PI C_PI

/**************************/
/* Structure definitions  */
/**************************/
typedef struct {
  int order;                           /* Filter order */
  int direction;		       /* Filtering direction */
  int coefCompress;		       /* Are coeffs compressed? */
  int length;                          /* Length, in bands */                     
  double aCoeffs[TNS_MAX_ORDER+1];     /* AR Coefficients */
  double kCoeffs[TNS_MAX_ORDER+1];     /* Reflection Coefficients */
  int index[TNS_MAX_ORDER+1];	       /* Coefficient indices */
} TNS_FILTER_DATA;


typedef struct {
  int numFilters;				/* Number of filters */
  int coefResolution;				/* Coefficient resolution */
  TNS_FILTER_DATA tnsFilter[1<<LEN_TNS_NFILTL];	/* TNS filters */
} TNS_WINDOW_DATA;


typedef struct {
  int tnsDataPresent;
  int tnsMinBandNumberLong;
  int tnsMinBandNumberShort;
  int tnsMaxBandsLong;
  int tnsMaxBandsShort;
  int tnsMaxOrderLong;
  int tnsMaxOrderShort;
  TNS_WINDOW_DATA windowData[NSHORT];	/* TNS data per window */
} TNS_INFO;


/*************************/
/* Function prototypes   */
/*************************/
void Autocorrelation(int maxOrder,        /* Maximum autocorr order */
		     int dataSize,		  /* Size of the data array */
		     double* data,		  /* Data array */
		     double* rArray);	  /* Autocorrelation array */

double LevinsonDurbin(int maxOrder,       /* Maximum filter order */
		      int dataSize,		   /* Size of the data array */
		      double* data,		   /* Data array */
		      double* kArray);	   /* Reflection coeff array */

void StepUp(int fOrder,
	    double* kArray,
	    double* aArray);

void QuantizeReflectionCoeffs(int fOrder,int coeffRes,double* rArray,int* indexArray);
int TruncateCoeffs(int fOrder,double threshold,double* kArray);
void TnsFilter(int length,double* spec,TNS_FILTER_DATA* filter);
void TnsInvFilter(int length,double* spec,TNS_FILTER_DATA* filter);
void TnsInit(long samplingRate,enum AAC_PROFILE profile,TNS_INFO* tnsInfo); 
void TnsEncode(int numberOfBands,int maxSfb,enum WINDOW_TYPE blockType,int* sfbOffsetTable,double* spec,TNS_INFO* tnsInfo);
void TnsEncodeFilterOnly(int numberOfBands, int maxSfb, enum WINDOW_TYPE blockType,
			 int *sfbOffsetTable, double *spec, TNS_INFO *tnsInfo);
void TnsDecodeFilterOnly(int numberOfBands, int maxSfb, enum WINDOW_TYPE blockType,
			 int *sfbOffsetTable, double *spec, TNS_INFO *tnsInfo);

#endif






