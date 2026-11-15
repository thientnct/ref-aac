/**********************************************************************
MPEG-4 Audio VM
Common module


SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Heiko Purnhagen (University of Hannover)

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



Header file: common.h

$Id: common.h,v 1.10 1998/12/01 22:28:50 lueck Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-jun-96   HP    new module based on ErrorMsg() from cmdline module
25-jun-96   HP    added CommonFreeAlloc()
04-jul-96   HP    added stdlib.h
26-aug-96   HP    CVS
**********************************************************************/


#ifndef _common_h_
#define _common_h_

#include <stdlib.h>		/* typedef size_t */

#ifdef MPEG4V1
#define PROGVER "MPEG-4 AAC Audio Encoder   2003-08-27"
#else
#define PROGVER "MPEG-2 AAC Audio Encoder   2003-08-27"
#endif

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

/* if static memory allocation is used, this value tells the max. nr of 
   audio channels to be supported */

#define MAX_CHANNELS 9
#define MAX_CPL_CHANNELS 1
#define MAX_LFE_CHANNELS 1
#define MAX_TIME_CHANNELS (MAX_CHANNELS+MAX_CPL_CHANNELS+MAX_LFE_CHANNELS)

/* CommonFreeAlloc() */
/* Free previously allocated memory if present, then allocate new memory. */
void *CommonFreeAlloc(
  void **ptr,			/* in/out: addr of memory pointer */
  size_t size);			/* in: memory size */
				/* returns: memory pointer */
				/*          or NULL if error*/


/* CommonProgName() */
/* Set program name for error/warning message. */
void CommonProgName(
  char *progName);		/* in: program name */


/* CommonWarning() */
/* Print program name and warning message to stderr. */
void CommonWarning(
  char *message,		/* in: warning message */
  ...);				/* in: args as for printf */

/* CommonExit() */
/* Print program name and error message to stderr and exit program. */
void CommonExit(
  int errorCode,		/* in: error code for exit() */
  char *message,		/* in: error message */
  ...);				/* in: args as for printf */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _common_h_ */

/* end of common.h */

