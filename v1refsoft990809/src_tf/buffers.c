/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "block.h"               /* handler, defines, enums */
#include "buffersHandle.h"       /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#include "buffers.h"
#include "bitfct.h"
#include "bitstream.h"
#include "common_m4a.h"

/*******************/
/*** definitions ***/
/*******************/

typedef struct tag_buffer
{
  unsigned char* physBuf;
  unsigned long  readBitCnt;
  unsigned long  readBitCntMemo;
  unsigned long  writeBitCnt;
  unsigned long  writeBitCntMemo;
  BIT_BUF        bitBuf;
  BIT_BUF        bitBufMemo;
} T_BUFFER;


/*****************/
/*** functions ***/
/*****************/

/****************************/
/**** framework handling ****/
static BsBitStream *vmBitStream;

void setHuffdec2BitBuffer ( BsBitStream* fixed_stream )
{
  vmBitStream = fixed_stream;
} 

/* align byte if necessary */
void byte_align ( void )
{
  int i;
  int curBitPos;

  curBitPos = BsCurrentBit ( vmBitStream );
  i = curBitPos % 8;
  if (!i) {
    i = 8-i;
    BsGetSkip( vmBitStream, i);
  }
}

HANDLE_BUFFER CreateBuffer ( unsigned long bufferSize )
{
  HANDLE_BUFFER handle;

  handle = (HANDLE_BUFFER) malloc ( sizeof(T_BUFFER) );
  if ( bufferSize )
    handle->physBuf = (unsigned char*) malloc ( sizeof ( char ) * bufferSize );
  else
    handle->physBuf = NULL;
  return ( handle );
}

long GetBits ( CODE_TABLE        code,    /* NEW ... to decode EP bitstream */
               int               n,
               HANDLE_RESILIENCE hResilience,
               HANDLE_BUFFER     handle,
               HANDLE_EP_INFO    hEpInfo )

{
  unsigned long  value;

  if ( n != 0 ) {
    if ( handle->physBuf == NULL ) {
        {
          handle->readBitCnt += n; 
          BsGetBit ( vmBitStream, &value, n );
        }
      if (debug['b'])
        fprintf(stderr, "  AAC: GetBits: val=%5ld      num=%5d\n", value, n );
    }
    { /* to avoid compiler warnings*/
      code = code;
      hResilience = hResilience;
      hEpInfo = hEpInfo;
    }
    return ( value );
  }
  else
    return ( 0 );
}

unsigned long GetReadBitCnt ( HANDLE_BUFFER handle )
{
  return ( handle->readBitCnt );
}


void ResetReadBitCnt ( HANDLE_BUFFER handle )
{
  handle->readBitCnt = 0;
}

void ResetWriteBitCnt ( HANDLE_BUFFER handle )
{
  handle->writeBitCnt = 0;
}

void RestoreBufferPointer ( HANDLE_BUFFER handle )
{
  handle->readBitCnt  = handle->readBitCntMemo;
  handle->writeBitCnt = handle->writeBitCntMemo;
  handle->bitBuf      = handle->bitBufMemo;
}

void StoreBufferPointer ( HANDLE_BUFFER handle )
{
  handle->readBitCntMemo  = handle->readBitCnt;
  handle->writeBitCntMemo = handle->writeBitCnt;
  handle->bitBufMemo      = handle->bitBuf;
}


