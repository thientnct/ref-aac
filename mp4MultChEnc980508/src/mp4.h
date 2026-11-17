/**********************************************************************
MPEG-4 Audio VM
Frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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



Header file: mp4.h

$Id: mp4.h,v 1.1.1.2 1998-05-07 17:21:27+03 suonio Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
14-aug-96   HP    first version
21-aug-96   HP    adjusted MAXREADAHEAD, BITHEADBUFFERSIZE to BG's code
26-aug-96   HP    CVS
30-oct-96   HP    additional frame work options
18-nov-96   HP    added bit stream version
10-dec-96   HP    incremented MP4_BS_VERSION
17-jan-97   HP    reduced MAXREADAHEAD, BITHEADBUFFERSIZE after
                  bug fix in bitstream.c
14-mar-97   HP    merged FhG AAC code
21-mar-97   BT    made strings static
**********************************************************************/


#ifndef _mp4_h_
#define _mp4_h_


/* ---------- declarations ---------- */

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define def2str(a) def2str_(a)
#define def2str_(a) #a

#define STRLEN 255

#define MP4_ORI_PATH_ENV "MP4_ORI_PATH"
#define MP4_BIT_PATH_ENV "MP4_BIT_PATH"
#define MP4_DEC_PATH_ENV "MP4_DEC_PATH"
#define MP4_ORI_FMT_ENV "MP4_ORI_FORMAT"
#define MP4_DEC_FMT_ENV "MP4_DEC_FORMAT"
#define MP4_ORI_EXT ".au"
#define MP4_BIT_EXT ".mp4"
#define MP4_DEC_EXT ".au"
#define MP4_ORI_RAW_ENV "MP4_RAWAUDIOFILE"

#define MP4_MAGIC ".mp4"

#define MP4_BS_VERSION 0x4003

#define MAXREADAHEAD (65536)
#define BITHEADERBUFSIZE (65536)

/* codec mode */

enum MP4Mode {MODE_UNDEF, MODE_PAR, MODE_LPC, MODE_TF, MODE_AAC, MODE_NUM};
static char _MName0[] = "undefined";
static char _MName1[] = "par";
static char _MName2[] = "lpc";
static char _MName3[] = "tf";

static char *MP4ModeName[4] = {_MName0,_MName1,_MName2,_MName3};
/* static char *MP4ModeName[4] = {"undefined", "par", "lpc", "tf"}; */
#define MODE_LIST "par, lpc, tf"


#endif	/* #ifndef _mp4_h_ */

/* end of mp4.h */

