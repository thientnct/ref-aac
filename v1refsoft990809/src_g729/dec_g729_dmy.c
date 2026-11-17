/**********************************************************************
MPEG-4 Audio VM
Decoder core (G729-based)



This software module was originally developed by

Sebastien Etienne (University of Hannover)

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Source file: dec_g729_dmy.c

$Id: dec_g729_dmy.c,v 1.7 1999/05/20 10:23:32 purnhage Exp $

Required modules:
common.o		common module
bitstream.o		bits stream module

Authors:
CCETT    Sebastien Etienne, CCETT Rennes

Changes:
26-mar-97   CCETT    g729 core
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "concealmentHandle.h"   /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "reorderspecHandle.h"   /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"		/* common module */
#include "bitstream.h"		/* bit stream module */

#include "flex_mux.h"
#include "lpc_common.h"
#include "dec_g729.h"		/* decoder cores */


/* ---------- declarations ---------- */



/* ---------- functions ---------- */
char *DecG729Info (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	   "g729 decoder dummy module , g729 decoder not compiled in\n");
  }
  return "g729 decoder dummy module";
}
/* DecG729Init() */
/* Init G729-based decoder core. */

void DecG729Init (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample)		/* out: decoder delay (num samples) */
{
  CommonExit(1,"DecG729Init: dummy module , g729 decoder not compiled in ");
}


/* DecG729Frame() */
/* Decode one bit stream frame into one audio frame with */
/* G729-based decoder core. */

void DecG729Frame (
  BsBitBuffer *bitBuf,		/* in: bit stream frame */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int procMode)
{
  CommonExit(1,"DecG729Frame: dummy module , g729 decoder not compiled in");
}




/* end of dec_g729_dmy.c */

