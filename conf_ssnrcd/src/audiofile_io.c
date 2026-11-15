
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


Source file: AUDIOFILE_IO.C

*/

#include <stdio.h>
#include <string.h>

#include "libtsp.h"
#include "libtsp/AFpar.h"

#include "common.h"
#include "audiofile_io.h"


/***************************************************************************/

int open_inputaudio (AFILE          **streampointer, 
                     string         input_filename, 
                     audiofile      *audio_file)
{

    int status = 0;
    
    /*   Read header of audio file                                                 */
    
    *streampointer = AFopenRead (input_filename, 
                                 &(audio_file->nrofsamples), 
                                 &(audio_file->channels), 
                                 &(audio_file->samplefreq),
                                 NULL);

    if (*streampointer == NULL)
    {
        status = 1;
    }
    else
    {
        audio_file->nrofsamples = audio_file->nrofsamples / audio_file->channels;
        
        switch ((*streampointer)->Format)
        {
            case    FD_MULAW8   :   audio_file->bits_per_sample = 8;
                                    break;
            case    FD_ALAW8    :   audio_file->bits_per_sample = 8;
                                    break;
            case    FD_UINT8    :   audio_file->bits_per_sample = 8;
                                    break;
            case    FD_INT16    :   audio_file->bits_per_sample = 16;
                                    break;
            case    FD_INT24    :   audio_file->bits_per_sample = 24;
                                    break;
            case    FD_INT32    :   audio_file->bits_per_sample = 32;
                                    break;
            case    FD_FLOAT32  :   audio_file->bits_per_sample = 32;
                                    break;
            case    FD_FLOAT64  :   audio_file->bits_per_sample = 64;
                                    break;
            default             :   audio_file->bits_per_sample = 0;
                                    break;
        }
    }

    return (status);
}

/***************************************************************************/

void close_inputaudio (AFILE *streampointer)
{
    if (streampointer != NULL)
    {
        AFclose (streampointer);
    }
}

/***************************************************************************/

int check_audioheader (audiofile audio_file)
{

    int status = 0;

    if (audio_file.channels > MAX_PCM_CHANNELS) 
    {
        status = 1;
    }

    if (audio_file.nrofsamples < 1)
    {
        status = 1;
    }

    if (audio_file.bits_per_sample == 0)
    {
        status = 1;
    }

    return (status);
}

/***************************************************************************/

int get_audiosample (float              pcm [],
                     AFILE              *streampointer, 
                     long int           Nchannels, 
                     long int           offset,
                     int                segment_length)
{

    int     samples_read;

    samples_read = AFreadData (streampointer, 
                               Nchannels * offset, 
                               pcm, 
                               Nchannels * segment_length);
    
    samples_read /= Nchannels;
    
    return (samples_read);
}

/***************************************************************************/

void copy_sample (float     target[], 
                  float     source[],
                  long int  ch, 
                  long int  n_ch, 
                  int       segment_length)
{
    int n;

    for (n = 0; n < segment_length; n++)
    {
        target [n] = source [(n * n_ch) + ch];
    }
}

/***************************************************************************/

void apply_zeropadding (float           pcm [], 
                        long int        Nchannels, 
                        int             start, 
                        int             end)
{
    int         n;
    long int    ch;

    for (ch = 0; ch < Nchannels; ch++)
    {
        for (n = start; n < end; n++)
        {
            pcm [(n * Nchannels) + ch] = 0.0F;
        }
    }
}

/***************************************************************************/
