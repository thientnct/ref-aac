/*
  "This software module was originally developed by 

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

  and edited by

  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)
  Ralph Sperschneider (Fraunhofer IIS-A)

  in the course of 
  development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
  14496-1,2 and 3. This software module is an implementation of a part of one or more 
  MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
  Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
  standards free license to this software module or modifications thereof for use in 
  hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
  Audio  standards. Those intending to use this software module in hardware or 
  software products are advised that this use may infringe existing patents. 
  The original developer of this software module and his/her company, the subsequent 
  editors and their companies, and ISO/IEC have no liability for use of this software 
  module or modifications thereof in an implementation. Copyright is not released for 
  non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
  retains full right to use the code for his/her  own purpose, assign or donate the 
  code to a third party and to inhibit third party from using the code for non 
  MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
  be included in all copies or derivative works." 
  Copyright(c)1998, 2000.
*/
#include "resorg.h"
#include "port.h"
#include "portio.h"

typedef struct {
  unsigned char reorderSpecPreSortFlag;
  unsigned char reorderSpecFlag;
  unsigned char consecutiveReorderSpecFlag;
  unsigned char resilienceScfFlag;
  unsigned char transcodeSectionDataFlag;
} Resilience_Info;

static Resilience_Info res_info;

void ResilienceInit ( )
{
  res_info.reorderSpecFlag = 0;
  res_info.resilienceScfFlag = 0;
  res_info.transcodeSectionDataFlag = 0;
  res_info.consecutiveReorderSpecFlag = 0;
}

void SetReorderSpecFlag ( unsigned short flag )
{
enum {
  PRE_SORTING     = 1,
  CONSECUTIVE_HCR = 2
};

  res_info.reorderSpecFlag = 1;
  if ( flag > 3 ) {
    myexit ("wrong hcr mode");
  }
  if ( flag & PRE_SORTING ) {
    res_info.reorderSpecPreSortFlag = 1;
  }
  if ( flag & CONSECUTIVE_HCR ) {
    res_info.consecutiveReorderSpecFlag = 1;
  }
}

void SetTranscodeSectionDataFlag ( )
{
  res_info.transcodeSectionDataFlag = 1;
}

void SetResilienceScfFlag ( unsigned short flag )
{
  /* flag == 1 : length of sf data */
  /* flag == 2 : 1 + rvlc coding of sf data */
  /* flag == 3 : 2 + enhanced concealment bit */
 if ( flag > 3 ) {
    myexit ("wrong scf resilience mode");
  }
  res_info.resilienceScfFlag = flag;
}

unsigned short GetReorderSpecPreSortFlag ( )
{
  return ( res_info.reorderSpecPreSortFlag );
}

unsigned short GetReorderSpecFlag ( )
{
  return ( res_info.reorderSpecFlag );
}

unsigned short GetConsecutiveReorderSpecFlag ( )
{
  return ( res_info.consecutiveReorderSpecFlag );
}

unsigned short GetResilienceScfFlag ( )
{
  return ( res_info.resilienceScfFlag );
}

unsigned short GetTranscodeSectionDataFlag ( )
{
  return ( res_info.transcodeSectionDataFlag );
}
