/***********

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
Copyright (c) ISO/IEC 1996.

***********/
/*********************************************************************
 *
 * Function prototypes for writing/counting AAC syntactic elements
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * RG    Ralf Geiger,  FhG/IIS
 *
 * Changes:
 * 07-jun-97   CL   Initial revision.
 * 14-sep-97   CL   Updated WritePredictorData to support prediction.
 * 20-oct-97   CL   Updated WriteTNSData to support TNS.
 * 22-Jan-98   CL   Added support for CPE's and common windows.
 * 07-Apr-98   RG   Added WriteLFE to write LFE channel element.
 *
**********************************************************************/


#ifndef AAC_SE_ENC
#define AAC_SE_ENC

#include "interface.h"
#include "bitstream.h"
#include "common.h"
#include "all.h"
#include "aac_qc.h"
#include "mp4.h"
#ifdef MPEG4V1
#include "nok_ltp_enc.h"
#endif

extern int max_pred_sfb;

/*******************************/
/* #defines                    */
/*******************************/
#define DEF_SE_DEBUG_LEVEL 1   /* debug level for aac syntax */
#define dbug(debugVal,threshold,x) { if ((threshold)<=(debugVal)) { x;} }
#define SE_DBUG(x) {dbug(seDebugLevel,DEF_SE_DEBUG_LEVEL,x)}       /* lowest detail */
#define SE_DBUG1(x) {dbug(seDebugLevel-1,DEF_SE_DEBUG_LEVEL,x)}    /* medium */
#define SE_DBUG2(x) {dbug(seDebugLevel-2,DEF_SE_DEBUG_LEVEL,x)}    /* highest detail */

void InitAACSyntax(int debugLevel);

/*****************************************************************************/
/* Write AAC fill bits to the bitStream                                      */
/*****************************************************************************/
int WriteAACFillBits(BsBitStream* ptrBs,  /* Pointer to bit stream */
		     int numBits);        /* Number of bits neede to fill */

/*****************************************************************************/
/* WriteSCE(...), write a single-channel element to the bitstream.           */
/*****************************************************************************/
int WriteSCE(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
	     int tag,
	     BsBitStream* fixedStream, /* Pointer to bitstream */
	     int writeFlag);           /* 1 means write, 0 means count only */

/*****************************************************************************/
/* WriteLFE(...), write a lfe-channel element to the bitstream.              */
/*****************************************************************************/
int WriteLFE(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
	     int tag,
	     BsBitStream* fixedStream, /* Pointer to bitstream */
	     int writeFlag);           /* 1 means write, 0 means count only */

/*****************************************************************************/
/* WriteCPE(...), write a channel_pair_element to the bitstream.           */
/*****************************************************************************/
int WriteCPE(AACQuantInfo* quantInfoL,   /* AACQuantInfo structure, left */
             AACQuantInfo* quantInfoR,   /* AACQuantInfo structure, right */
	     int tag,
	     int commonWindow,
	     MS_Info* ms_info,
	     BsBitStream* fixedStream,  /* Pointer to bitstream */
	     int writeFlag);            /* 1 means write, 0 means count only */

/*****************************************************************************/
/* WriteICS(...), write an individual_channel_stream element to the bitstream.*/
/*****************************************************************************/
int WriteICS(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
	     int commonWindow,
	     BsBitStream* fixedStream, /* Pointer to bitstream */
	     int writeFlag);           /* 1 means write, 0 means count only */ 

/*****************************************************************************/
/* WriteICSInfo(...), write a individual_channel_stream information          */
/*  to the bitstream.                                                        */
/*****************************************************************************/
int WriteICSInfo(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
		 BsBitStream* fixedStream, /* Pointer to bitstream */
		 int writeFlag,            /* 1 means write, 0 means count only */
		 int stereo_flag,
		 AACQuantInfo* quantInfoR);

#ifdef MPEG4V1
/*****************************************************************************/
/* WriteLTP_PredictorData(...), write LTP predictor data.                    */
/*****************************************************************************/
int WriteLTP_PredictorData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
                           BsBitStream* fixedStream, /* Pointer to bitstream */
                           int writeFlag);           /* 1 means write, 0 means count only */
#endif

/*****************************************************************************/
/* WritePredictorData(...), write predictor data.                            */
/*****************************************************************************/
int WritePredictorData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
		       BsBitStream* fixedStream, /* Pointer to bitstream */
		       int writeFlag);           /* 1 means write, 0 means count only */   

/*****************************************************************************/
/* WritePulseData(...), write pulse data.                            */
/*****************************************************************************/
int WritePulseData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
		   BsBitStream* fixedStream, /* Pointer to bitstream */
		   int writeFlag);           /* 1 means write, 0 means count only */ 

/*****************************************************************************/
/* WriteTNSData(...), write TNS data.                            */
/*****************************************************************************/
int WriteTNSData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
		 BsBitStream* fixedStream, /* Pointer to bitstream */
		 int writeFlag);           /* 1 means write, 0 means count only */

/*****************************************************************************/
/* WriteGainControlData(...), write gain control data.                       */
/*****************************************************************************/
int WriteGainControlData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
			 BsBitStream* fixedStream, /* Pointer to bitstream */
			 int writeFlag);           /* 1 means write, 0 means count only */

/*****************************************************************************/
/* WriteSpectralData(...), write spectral data.                              */
/*****************************************************************************/
int WriteSpectralData(AACQuantInfo* quantInfo,  /* Pointer to AACQuantInfo structure */
		      BsBitStream* fixedStream, /* Pointer to bitstream */
		      int writeFlag);           /* 1 means write, 0 means count only */

#endif




