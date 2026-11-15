/*
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
*/

/*
//---------------------------------------------------------------------------\ 
//
//   filename: PnsConformanceMain.h
//             
//   author  : Martin Weishart (IIS)
//   date    : 2001-10-26
//   contents/description: provides a struct  
//                         to calculate PNS conformance criteria
//
//--------------------------------------------------------------------------\
*/

#ifndef __PNS_H__
#define __PNS_H__

#include "libtsp.h"
#include "libtsp/AFpar.h"

#include "common.h"


typedef struct pns_obj * pns_ptr;



pns_ptr pnsInit (void);

void pnsSetTest(pns_ptr p, int test);

void pnsSetFrameSize(pns_ptr p, int frameSize);

int pnsGetTest(pns_ptr p);

int pnsGetFrameSize(pns_ptr p);


int CalculateTemporalConformance( AFILE             *Pcmfile,
                                  AFILE             *refPcmfile,
                                  audiofile         Pcm_header,
                                  audiofile         refPcm_header,
                                  pns_ptr           p,
                                  int               delay,
                                  int               verbose);


int CalculateSpectralConformance( AFILE             *Pcmfile,
                                  AFILE             *refPcmfile,
                                  audiofile         Pcm_header,
                                  audiofile         refPcm_header,
                                  pns_ptr           p,
                                  int               delay,
                                  int               verbose);


void pnsFree (pns_ptr p);



#endif /* __PNS_H__ */



