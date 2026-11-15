#ifdef ESC_SUPPORT

/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by


and edited by


in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1998.



Source file: ntt_flex_mux.c


Required modules:
common.o		common module
bitstream.o             bitstream handling module


**********************************************************************/
#include <string.h>
#include <stdio.h>

#include "obj_descr.h"           /* structs */
#include "lpc_common.h"          /* structs */

#include "common_m4a.h"
#include "bitstream.h"

#include "flex_mux.h"

void ntt_writeFlexMuxPDU(int index,HANDLE_BSBITSTREAM  bitStream ,int vq_bits, HANDLE_BSBITBUFFER  AUBuffer, int idx_base)
{
  int maxBytes=255, total_bits, fc_bits /* first class number of bits */;
  int AUnumber=0, length_tmp=0;
  HANDLE_BSBITSTREAM AUStream;
  HANDLE_BSBITBUFFER ntt_tmpBuf;
  int w_index;

  ntt_tmpBuf = BsAllocBuffer(256*8);

  total_bits = BsBufferNumBit(AUBuffer);
  fc_bits = total_bits - vq_bits;

  AUStream = BsOpenBufferRead(AUBuffer);

  /* first class AU writing */

  BsGetBuffer(AUStream, ntt_tmpBuf, fc_bits);
  if(idx_base==0)
    w_index = index; /* base layer index */
  else
    w_index = idx_base + (index-1)*2;  /* enhanced layer index */

  writeFlexMuxPDU(w_index, bitStream, ntt_tmpBuf);

  /* second class AU writing */

  BsGetBuffer(AUStream, ntt_tmpBuf, vq_bits);
  writeFlexMuxPDU(w_index+1, bitStream, ntt_tmpBuf);

  /* post processing */

  BsCloseRemove(AUStream, 1);
  BsFreeBuffer(ntt_tmpBuf);
}

#endif   /* ifdef ESC_SUPPORT */

