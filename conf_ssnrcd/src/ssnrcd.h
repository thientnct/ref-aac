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


Source file: SSNRCD.H

*/

#ifndef SSNRCD_H_
#define SSNRCD_H_

#include "common.h"


typedef struct ssnrcd_obj * ssnrcd_ptr;


ssnrcd_ptr ssnrcdInit ( void );


void compare_audiodata (AFILE               *stream1,
                        AFILE               *stream2,
                        audiofile           file1,
                        audiofile           file2,
                        unsigned long       samples_to_compare,
                        signed long         delay,
                        int                 segment_length,
                        int                 bits_resolution,
                        int                 exponential_output,
                        int                 max_resolution,
                        int                 verbose,
                        ssnrcd_ptr          ssnrcd);


int  generate_report (int                no_channels,
                       string             input_filename1, 
                       string             input_filename2,
                       unsigned long      samples_to_compare,
                       signed long        delay, 
                       unsigned long      samples_lost[2],
                       int                segment_length,
                       int                bits_resolution,
                       int                show_accuracy_distribution,
                       int                exponential_output,
                       int                max_resolution,
                       unsigned int       apply_criteria,
                       ssnrcd_ptr         ssnrcd);


void ssnrcdFree ( ssnrcd_ptr s );



#endif /*SSNRCD_H_*/
