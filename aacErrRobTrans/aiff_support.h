/*
  "This software module was originally developed by Five Bats Research in the
  course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC
  13818-7, 14496-1,2 and 3. This software module is an implementation of a
  part of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
  MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives users of the MPEG-2
  AAC/MPEG-4 Audio standards free license to this software module or
  modifications thereof for use in hardware or software products claiming
  conformance to the MPEG-2 AAC/MPEG-4 Audio standards. Those intending to
  use this software module in hardware or software products are advised that
  this use may infringe existing patents.  The original developer of this
  software module and his/her company, the subsequent editors and their
  companies, and ISO/IEC have no liability for use of this software module or
  modifications thereof in an implementation. Copyright is not released for
  non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
  retains full right to use the code for his/her own purpose, assign or
  donate the code to a third party and to inhibit third party from using the
  code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright
  notice must be included in all copies or derivative works."
  Copyright (c) 1997

  (created 1997-Jan-01 mc@fivebats.com)
*/

#ifndef AIFF_SUPPORT_H
#define AIFF_SUPPORT_H

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

int WriteAIFFHeader( int           fd,            /* already opened file descriptor */
                     short         nChannels,     /* #channels in this file */
                     unsigned long nFrames,       /* #sample frames in this file */
                     short         sampleSize,    /* word size in bits (1-32) */
                     unsigned long sampleRate     /* sampling rate */
);

#endif
