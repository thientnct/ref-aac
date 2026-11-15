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



Source file: common.c

$Id: common.c,v 1.1.1.1 1998/06/09 01:05:52 lueck Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-jun-96   HP    new module based on ErrorMsg() from cmdline module
25-jun-96   HP    added CommonFreeAlloc()
26-aug-96   HP    CVS
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"		/* common module */


/* ---------- variables ---------- */

static char *CMprogName = "<progname>";		/* program name */


/* ---------- functions ---------- */

/* CommonFreeAlloc() */
/* Free previously allocated memory if present, then allocate new memory. */

void *CommonFreeAlloc(
  void **ptr,			/* in/out: addr of memory pointer */
  size_t size)			/* in: memory size */
				/* returns: memory pointer */
				/*          or NULL if error*/
{
  if (*ptr != NULL)
    free(*ptr);
  return *ptr = malloc(size);
}


/* CommonProgName() */
/* Set program name for error/warning message. */

void CommonProgName(
  char *progName)		/* in: program name */
{
  CMprogName = progName;
}


/* CommonWarning() */
/* Print program name and warning message to stderr. */

void CommonWarning(
  char *message,		/* in: warning message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  fflush(stdout);
  fprintf(stderr,"%s: WARNING: ",CMprogName);
  vfprintf(stderr,message,args);
  fprintf(stderr,"\n");
  va_end(args);
}


/* CommonExit() */
/* Print program name and error message to stderr and exit program. */

void CommonExit(
  int errorCode,		/* in: error code for exit() */
  char *message,		/* in: error message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  fflush(stdout);
  fprintf(stderr,"%s: ERROR[%d]: ",CMprogName,errorCode);
  vfprintf(stderr,message,args);
  fprintf(stderr,"\n");
  va_end(args);
  exit (errorCode);
}


/* end of common.c */

