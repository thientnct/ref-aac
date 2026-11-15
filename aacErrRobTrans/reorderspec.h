/*
  "This software module was originally developed by 

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

  and edited by

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
  Copyright(c)1998.
*/
#ifndef _reorderspec_h_
#define _reorderspec_h_

#include "interface.h"

#define UINT_BITLENGTH     ( sizeof(unsigned long) * 8 )  /* lenght in bits of an unsigned int variable */
#define UNIT_INTERLEAVING 1

void AddHuffdataTable                      ( unsigned short section, 
                                             unsigned short groupNr, 
                                             unsigned short nrOfWindows,
                                             unsigned short sfb,
                                             unsigned short sfbInGroup,
                                             unsigned short window,
                                             unsigned short unit,
                                             unsigned short step, 
                                             unsigned short table,
                                             unsigned short maxCWLen,
                                             unsigned long  codeword, 
                                             unsigned long  codeword_ext, 
                                             unsigned long  codelen);
void UpdateVirtualCodebook                 ( unsigned short sfb,
                                             unsigned short virtualCodebook,
                                             unsigned short maxCWLen );
void CompletionHuffdataTable               ( void );
void WindowInterleaveHuffdataTable         ( void );
void PrintHuffdataTable                    ( void );
void ReorderSpec                           ( void );
void ResetHuffdataTable                    ( unsigned char islong,
                                             unsigned char maxSfb,
                                             unsigned char numGroups );
unsigned short GetLengthOfLongestCodeword  ( void );

#endif /* _reorderspec_h_  */
