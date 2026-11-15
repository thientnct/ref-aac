#ifdef VERSION2
/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

and edited by

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
 *                                                                           *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "resilience.h"
#include "common_m4a.h"

typedef struct tag_resilience
{
  char epFlag;
  char aacLenOfLongestCwFlag;
  char aacReorderSpecPreSortFlag;
  char aacReorderSpecFlag;
  char aacConsecutiveReorderSpecFlag;
  char aacVcb11Flag; 
  char aacLenOfScfDataFlag;
  char aacRvlcScfFlag;
  char aacScfConcealment;
  char aacScfBitFlag;
} T_RESILIENCE;

HANDLE_RESILIENCE CreateErrorResilience ( char* decPara, int epFlag )
{
  char* hcrStart;
  char* valueStart;
  char* nextPara;
  char* stringEnd;
  short valueChars;
  char  valueString[10] = {'\0'};
  short value;

  HANDLE_RESILIENCE handle;

  enum {
    PRE_SORTING  = 1,
    CONSECUTIVE_HCR = 2
  };
  handle = (HANDLE_RESILIENCE) malloc(sizeof(T_RESILIENCE));
  handle->epFlag = ( epFlag ? 1 : 0 );
  handle->aacLenOfLongestCwFlag         = 0;
  handle->aacReorderSpecFlag            = 0;
  handle->aacReorderSpecPreSortFlag     = 0;
  handle->aacConsecutiveReorderSpecFlag = 0;
  handle->aacVcb11Flag                  = 0;
  handle->aacLenOfScfDataFlag           = 0;
  handle->aacRvlcScfFlag                = 0;
  handle->aacScfConcealment             = 0;
  handle->aacScfBitFlag                 = 0;

  if ( strstr ( decPara, "-aac_raw" ) || strstr ( decPara, "-aac_sca" ) )  {
    handle->aacLenOfLongestCwFlag = ( ( strstr ( decPara, "-lcw" ) ) || ( strstr ( decPara, "-hcr" ) ) ? 1 : 0 );
    if (  ( strstr ( decPara, "-hcr" ) ) ) {
      handle->aacReorderSpecFlag = 1;
      hcrStart   = strstr  ( decPara, "-hcr" );
      valueStart = hcrStart + 4;
      nextPara   = strchr ( valueStart, '-');
      stringEnd  = decPara + strlen ( decPara );
      if ( nextPara ) {
        valueChars = nextPara - valueStart;
      }
      else {
        valueChars = stringEnd - valueStart;
      }
      strncpy ( valueString, valueStart, valueChars );
      value = atoi ( valueString );
      if ( value > 3 ) {
        CommonExit(1, "wrong hcr mode");
      }
      if ( value & PRE_SORTING ) {
        handle->aacReorderSpecPreSortFlag = 1;
      }
      if ( value & CONSECUTIVE_HCR ) {
        handle->aacConsecutiveReorderSpecFlag = 1;
      }
    }
    /* section data coding of table 11 flag */
    if( strstr(decPara, "-vcb11") ) {
      handle->aacVcb11Flag = 1;
    }
    
    /* scalefactor data part */
    if ( strstr( decPara, "-lsf" ) && strstr( decPara, "-rvlc" ) ) {
      CommonExit (1,"-lsf AND -rvlc makes no sense!" );
    }
    if ( strstr( decPara, "-lsf" ) ) {
      handle->aacLenOfScfDataFlag = 1;
    }
    if ( strstr( decPara, "-rvlc" ) ) {
      handle->aacRvlcScfFlag = 1;
    }
    if ( strstr( decPara, "-scfCon" ) ) {
      handle->aacScfConcealment = 1;
    }
    if ( strstr( decPara, "-scfBit" ) ) {
      if ( !strstr( decPara, "-scfCon" ) || !strstr( decPara, "-rvlc" ) ) {
        CommonExit (1,"-scfBit makes no sense without -rvlc & -scfCon!" );
      } 
      else {
        handle->aacScfBitFlag = 1;
      }
    }
  }
  return ( handle );
}

char GetEPFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->epFlag );
}

/* aac only */

char GetLenOfLongestCwFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacLenOfLongestCwFlag );
}

char GetLenOfScfDataFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacLenOfScfDataFlag );
}

char GetReorderSpecPreSortFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacReorderSpecPreSortFlag );
}
char GetReorderSpecFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacReorderSpecFlag );
}

char GetConsecutiveReorderSpecFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacConsecutiveReorderSpecFlag );
}

char GetRvlcScfFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacRvlcScfFlag );
}

char GetScfBitFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacScfBitFlag );
}

char GetScfConcealmentFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacScfConcealment );
}

char GetVcb11Flag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacVcb11Flag );
}

#endif /*VERSION2*/
