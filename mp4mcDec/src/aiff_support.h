#ifndef AIFF_SUPPORT_H
#define AIFF_SUPPORT_H
/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Five Bats Research

and edited by
  Yasuhiro Toguri (Sony)

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

  (created 1997-Jan-01 mc@fivebats.com)
 *                                                                           *
 ****************************************************************************/

/*
  WriteAIFFHeader()

  Writes AIFF header information at head of file *fp,
  and positions write pointer at audio data.
  Returns 0 if successful, -1 for error.

  NOTE: It is customary to call this TWICE: Once to
  put a placeholder in at the beginning of writing,
  and again at the end (when #frames is known). The
  size of the header is not dependent on the data.
*/

int WriteAIFFHeader( FILE*         fd,            /* already opened file descriptor */
                     short         nChannels,     /* #channels in this file */
                     unsigned long nFrames,       /* #sample frames in this file */
                     short         sampleSize,    /* word size in bits (1-32) */
                     unsigned long sampleRate     /* sampling rate */
);

#endif
