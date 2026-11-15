
#ifndef FILE_IO_H
#define FILE_IO_H

/*

====================================================================
                        Copyright Header            
====================================================================

This software module was originally developed by Ralf Funken
(Philips, Eindhoven, The Netherlands) and edited by Takehiro Moriya 
(NTT Labs. Tokyo, Japan) in the course of development of the
MPEG-2/MPEG-4 Conformance ISO/IEC 13818-4, 14496-4. This software
module is an implementation of one or more of the Audio Conformance
tools as specified by MPEG-2/MPEG-4 Conformance. ISO/IEC gives users
of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC 13818-3,7, 14496-3) free
license to this software module or modifications thereof for use in
hardware or software products. Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Philips retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party. This copyright notice must be included in 
all copies or derivative works. Copyright 2000.


Source file: AUDIOFILE_IO.H

*/


#include <stdio.h>

#include "libtsp.h"
#include "libtsp/AFpar.h"

#include "common.h"

/***************************************************************************/

int open_inputaudio (AFILE          **streampointer, 
                     string         input_filename, 
                     audiofile      *audio_file);

void close_inputaudio (AFILE *streampointer);

int check_audioheader (audiofile audio_file);

int get_audiosample (float              pcm [],
                     AFILE              *streampointer, 
                     long int           Nchannels, 
                     long int           offset,
                     int                segment_length);

void copy_sample (float     temp_pcm_buffer[], 
                  float     pcm_buffer1[],
                  long int  ch, 
                  long int  n_ch, 
                  int       segment_length);

void apply_zeropadding (float           pcm [], 
                        long int        Nchannels,
                        int             start, 
                        int             end);

/***************************************************************************/


#endif   /*   FILE_IO_H   */
