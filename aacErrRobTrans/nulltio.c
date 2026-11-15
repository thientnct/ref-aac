/*
  "This software module was originally developed by 
  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
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
  Copyright(c)1996.
*/
#include "all.h"
#include "statistics.h"

int	istrans = 0;
long	byte_count=0;


long getshort ( )
{
    int c1, c2;

    c1 = getbyte();
    c2 = getbyte();
    byte_count += 2;
    if (debug['G']) {
      PRINT(SE,"get: %02x %x %ld\n", c1, c2, byte_count);
    }
    return (c1<<8) | c2;
}

void
restarttio(void)
{
    cword = 0;
    nbits = 0;
}

int startblock ( )
{
  /* get adif header */
  if (adif_header_present) 
    {
      if (get_adif_header() < 0) 
	return -1;
      adif_header_present = 0;
      byte_align();	/* start of block is byte aligned */
    }
  ResetBitCount(); /* nBitCount = 0 */
  if ( debug['S'])
    StatisticsResetBitsForBitstreamParts (); /* nBitCountHelp = 0 */
  return 1;
}

int endblock ( )
{
  byte_align();	/* start of block is byte aligned */
  return 1;
}
