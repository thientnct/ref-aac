/**********************************************************************
MPEG-4 Audio VM
Frame work



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



Header file: mp4.h

$Id: mp4.h,v 1.4 1998/06/13 00:27:53 lueck Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, Fraunhofer IIS

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
#define MP4_ORI_EXT ".wav"
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

