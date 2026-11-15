
#ifndef COMMON_H
#define COMMON_H

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


Source file: COMMON.H

*/

#define         PROGRAM_NAME                    "SBRCONF - Spectral Band Replication Conformance Tool"
#define         EXECUTABLE_NAME                 "sbr_conftool"
                                                 
#define         TRUE                            (1)
#define         FALSE                           (0)
                                                 
#define         SBRCONF_VERSION                 "1.1"
#define         MAX_STRING_LENGTH               (256)
#define         MAX_PARAMETER_SETS              (10)
#define         MAX_RESULT_LINES                (6)
                              
#define         PCM_SCALEFACTOR                 (32768.0)
#define         MAX_PCM_CHANNELS                (64)
#define         DEFAULT_DELAY                   (0)     /* default delay (offset between input files) in samples    */
#define         GREP_STRING                     "==="

/*  macros for min and max                      */

#ifndef MIN
#define         MIN(a,b)                        (((a) < (b)) ? (a) : (b))
#endif  /* MIN */

#ifndef MAX
#define         MAX(a,b)                        (((a) > (b)) ? (a) : (b))
#endif  /* MAX */

/*****************************************************************************/

typedef char   string [MAX_STRING_LENGTH];

typedef struct
{
    long int                    channels;
    float                       samplefreq;
    long int                    nrofsamples;
    int                         bits_per_sample;
} audioheader;

/*****************************************************************************/

#endif    /*    COMMON_H    */


