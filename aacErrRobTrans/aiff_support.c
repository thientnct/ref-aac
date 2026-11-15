/*
  "This software module was originally developed by Five Bats Research in the
  course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC
  13818-7, 14496-1,2 and 3. This software module is an implementation of a
  part of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
  MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives users of the MPEG-2
  AAC/MPEG-4 Audio standards free license to this software module or
  modifications thereof for use in hardware or software products claiming
  conformance to the MPEG-2 AAC/MPEG-4 Audio standards. Those intending to
  use this software module in hardware or software products are advised that
  this use may infringe existing patents.  The original developer of this
  software module and his/her company, the subsequent editors and their
  companies, and ISO/IEC have no liability for use of this software module or
  modifications thereof in an implementation. Copyright is not released for
  non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
  retains full right to use the code for his/her own purpose, assign or
  donate the code to a third party and to inhibit third party from using the
  code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright
  notice must be included in all copies or derivative works."
  Copyright(c)1997

  (created 1997-Jan-01 mc@fivebats.com)
*/

#include "aiff_support.h"
#include <unistd.h>
#include <string.h>
#include <math.h>

#define IFF_ID_FORM 0x464f524d /* "FORM" */
#define IFF_ID_AIFF 0x41494646 /* "AIFF" */
#define IFF_ID_COMM 0x434f4d4d /* "COMM" */
#define IFF_ID_SSND 0x53534e44 /* "SSND" */

/* inlining helps these if it is available */
#ifdef __cplusplus
  #define INLINE inline
#else
  #define INLINE
#endif

/* private helper functions defined below */
INLINE static int w16hl( int fd, int val );
INLINE static int w32hl( int fd, int val );
INLINE static int wieee80hl( int fd, unsigned long val );



/*
  Functions that are private to this file
*/

INLINE static int w16hl( int fd, int val )
{  /* write 16 bits big-endian */
   int ret;
   char c;
   c = (val >>8 ) & 0xff;
   ret = write( fd, &c, 1 );
   if ( ret == 1 )
   {
      c = val & 0xff;
      ret = write( fd, &c, 1 );
   }
   return (ret != 1);
}

INLINE static int w32hl( int fd, int val )
{  /* write 32 bits big-endian */
   int ret;
   ret = w16hl( fd, ((val >> 16) & 0xffff) );
   if ( ret == 0 )
      ret = w16hl( fd, (val & 0xffff) );
   return ret;
}

INLINE static int wieee80hl( int fd, unsigned long val )
{  /* write an ieee extended float */
   int ret;
   int i;
   unsigned long exponent;

   /* exponent */
   exponent = val;
   for ( i = 0; exponent; i += 1 )
      exponent >>= 1;
   i += 16382;
   ret = w16hl( fd, i );

   /* mantissa */
   for ( i = 32; i; i-- )
   {
      if ( val & 0x80000000 )
         break;
      val <<= 1;
   }
   if ( ret == 0 ) ret = w32hl( fd, val ); /* high mantissa */
   if ( ret == 0 ) ret = w32hl( fd, 0 );   /* low mantissa */

   return ret;
}

/*
  Functions that are part of the public interface
*/

/* documented in the header file */
int WriteAIFFHeader( int           fd,
                     short         nChannels,
                     unsigned long nFrames,
                     short         sampleSize,
                     unsigned long sampleRate )
{
   int chunkSize;
   int ret;
   int sampleBytes = (sampleSize / 8) + (sampleSize % 8 ? 1 : 0);
    
   /* reposition at head of file */
   if ( lseek(fd, 0L, SEEK_SET) != 0 )
      return -1;
    
   /* write FORM chunk */
   chunkSize = 8 + 18 + 8 + nChannels * nFrames * sampleBytes;
   ret = w32hl( fd, IFF_ID_FORM );
   if ( ret == 0 ) ret = w32hl( fd, chunkSize );
   if ( ret == 0 ) ret = w32hl( fd, IFF_ID_AIFF );

   /* write COMM chunk */
   if ( ret == 0 ) ret = w32hl( fd, IFF_ID_COMM );
   if ( ret == 0 ) ret = w32hl( fd, 18 ); /* chunk size */
   if ( ret == 0 ) ret = w16hl( fd, nChannels );
   if ( ret == 0 ) ret = w32hl( fd, nFrames );
   if ( ret == 0 ) ret = w16hl( fd, sampleSize );
   if ( ret == 0 ) ret = wieee80hl( fd, sampleRate );

   /* write SSND chunk header */
   chunkSize = 8 + nChannels * nFrames * sampleBytes;
   if ( ret == 0 ) ret = w32hl( fd, IFF_ID_SSND );
   if ( ret == 0 ) ret = w32hl( fd, chunkSize );
   if ( ret == 0 ) ret = w32hl( fd, 0 ); /* offset */
   if ( ret == 0 ) ret = w32hl( fd, 0 ); /* block size */
   return ret;
}
