/**************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Mikko Suonio (Nokia)

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

***************************************************************************/

#ifndef NOK_LTP_COMMON_H_
#define NOK_LTP_COMMON_H_

#ifdef MPEG4V1
/*
  Macro:	MAX_SHORT_WINDOWS
  Purpose:	Number of short windows in one long window.
  Explanation:	-  */
#ifndef MAX_SHORT_WINDOWS
#define MAX_SHORT_WINDOWS NSHORT
#endif

/*
  Macro:	MAX_SCFAC_BANDS
  Purpose:	Maximum number of scalefactor bands in one frame.
  Explanation:	-  */
#ifndef MAX_SCFAC_BANDS
#define MAX_SCFAC_BANDS MAXBANDS
#endif

/*
  Macro:	BLOCK_LEN_LONG
  Purpose:	Length of one long window
  Explanation:	-  */
#ifndef BLOCK_LEN_LONG
#define BLOCK_LEN_LONG LN2
#endif

#endif

/*
  Macro:	NOK_MAX_BLOCK_LEN_LONG
  Purpose:	Informs the routine of the maximum block size used.
  Explanation:	-  */
#define	NOK_MAX_BLOCK_LEN_LONG (BLOCK_LEN_LONG) 

/*
  Macro:	NOK_LT_BLEN
  Purpose:	Length of the history buffer.
  Explanation:	Has to hold 1.5 long windows of time domain data. */
#ifndef	NOK_LT_BLEN
#define NOK_LT_BLEN (3 * NOK_MAX_BLOCK_LEN_LONG)
#endif

/*
  Type:		NOK_LT_PRED_STATUS
  Purpose:	Type of the struct holding the LTP encoding parameters.
  Explanation:	-  */
typedef struct
{
  short buffer[NOK_LT_BLEN];
  int weight_idx;
  Float weight;
  int sbk_prediction_used[MAX_SHORT_WINDOWS];
  int sfb_prediction_used[MAX_SCFAC_BANDS];
  int *delay;
  int global_pred_flag;
  int side_info;
  double mdct_predicted[2 * NOK_MAX_BLOCK_LEN_LONG];

} NOK_LT_PRED_STATUS;

#endif /* NOK_LTP_COMMON_H_ */
