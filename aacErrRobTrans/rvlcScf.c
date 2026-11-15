/*
  "This software module was originally developed by 
  
  Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS) 
  
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
  Copyright(c)1997, 1998.
*/

#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include "all.h"
#include "rvlcScf.h"
#include "bitfct.h"
#include "port.h"
#include "escapeScf.h"
#include "escHandling.h"
#include "resorg.h"
#include "rvlcConceal.h"

#define ABS_ESC_VAL 7
#define ABS(x) ( (x)>0 ? (x): -(x) )
#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define ZERO_HCB 0
/* #define WRITE_SCF */

/************************************ Prototypes **********************************************/

static int ScfDecoding( Info*    info, 
                        byte*    group, 
                        int      nsect, 
                        byte*    sect,
                        short    global_gain, 
                        short*   factors, 
                        BIT_BUF* data, 
                        short*   revGG );

static int GetScfBits( Huffman *h, BIT_BUF *data );

static int ChangeTable( Info *info, 
                        byte *group, 
                        int nsect, 
                        byte *sect,
                        short global_gain, 
                        short *factors, 
                        BIT_BUF *readBuff, 
                        BIT_BUF *writeBuff, 
                        Escape escStruct );

static int EscapeBits( Huffman *h, 
                       BIT_BUF *readBuff, 
                       BIT_BUF *writeBuff, 
                       Escape escStruct );

static void EscapeHuffCoding( int numOfValues, 
                              int * pEscValues, 
                              BIT_BUF * writeBuff);

static int MaxAbsValue( int * p_vec, int size );

#ifdef WRITE_SCF
static void  WriteFactors( short * factors, int numberOfFactors );
static void  WriteEscapes( int * escapes, int numberOfEscapes );
#endif

/*****************************************************************************************/

static void shortclr(short *ip1, int cnt)
{
  memset(ip1, 0, cnt*sizeof(*ip1));
}

/*****************

  RVLCScfDecoding

******************/
int RVLCScfDecoding( Info*  info, 
                     byte*  group, 
                     int    nsect, 
                     byte*  sect,
                     short  global_gain, 
                     short* factors )
{
  unsigned int i;
  int returnValue;
  unsigned long bitCount;
  BIT_BUF bitBuff; 
  unsigned char buffer[100];
  short reversedGlobalGain = -1;
  unsigned char escapeBuffer[100];
  BIT_BUF escapeBitBuff; 
  Escape pEscStrc;
  int numOfEscVal;
  unsigned long escBitCount;

  BIT_BUF txEpBitBuff; 
  int encEpVal[MAXBANDS];
  unsigned char txEpBuffer[100];
  unsigned long txEpbitCount;

  unsigned short elemenLen;
  int maxAbsVal;
  unsigned long escPresent;

  unsigned short scfFlag;
  char enhc_con_bit;
  short * pLastFacs;
  short grp_facs[MAXBANDS];

  /* static variables */
  static int onlyOnce = 0;
  static short lastShortFactors[MAXBANDS];
  static short lastLongFactors[MAXBANDS];

  /* initialize buffers */
  intclr(encEpVal, MAXBANDS);
  for(i=0; i<100; i++) {
    buffer[i] = 0;
    escapeBuffer[i] = 0;
    txEpBuffer[i] = 0;
  }

  /* initialization of static variables */
  if ( !onlyOnce ) {
    onlyOnce = 1;
    /* initialize vectors */
    shortclr(lastShortFactors, MAXBANDS);
    shortclr(lastLongFactors, MAXBANDS);
  }

  /* decode sane data to get huffman bitstream */
  shortclr(grp_facs, MAXBANDS);
  InitWriteBitBuf( &bitBuff, buffer );
  returnValue = ScfDecoding(info, group, nsect, sect, global_gain, grp_facs, &bitBuff, &reversedGlobalGain);
  FlushWriteBitBuf( &bitBuff );
  bitCount = GetBitCount( &bitBuff );

  
  /*******************************************

         RVLC Codebook + ESCAPE Values             

  *******************************************/

  /* check mode */
  scfFlag = GetResilienceScfFlag();
  switch ( scfFlag ) {
  case 2:
    break;

  case 3:
    if (info->islong) {
      pLastFacs = lastLongFactors;
    } else {
      pLastFacs = lastShortFactors;
    }
    /* write out the enhanced concealment bit */
    enhc_con_bit = DecideConcealment( grp_facs, pLastFacs ); 
    WriteElemToBuffer( SF_CONCEALMENT, enhc_con_bit, LEN_SF_CONCEALMENT );

    if (info->islong) {
      /* overwrite scf's with those of actual Long-Frame */
      memcpy( lastLongFactors, grp_facs, (MAXBANDS * sizeof(short)) );
    } else {
      /* overwrite scf's with those of actual Short-Frame */
      memcpy( lastShortFactors, grp_facs, (MAXBANDS * sizeof(short)) );
    }
    break;

  default:
    myexit("rvlcScf.c");
    break;
  }

  /* write reversed global gain */
  assert( reversedGlobalGain >= 0 );
  assert( reversedGlobalGain < (short) pow(2, LEN_REV_GLOBAL_GAIN) );
  WriteElemToBuffer(REV_GLOBAL_GAIN, reversedGlobalGain, LEN_REV_GLOBAL_GAIN);

  /* convert bitstream to RVLC+ESCAPE bitstream */
  pEscStrc = CreateEscape( );
  InitReadBitBuf( &bitBuff, buffer );
  InitWriteBitBuf( &escapeBitBuff, escapeBuffer );
  InitEscape( pEscStrc, encEpVal);
  ChangeTable(info, group, nsect, sect, global_gain, factors, &bitBuff, &escapeBitBuff, pEscStrc);
  FlushWriteBitBuf( &escapeBitBuff );

  /* written and read bits have to be equal */
  assert( bitCount == GetBitCount( &bitBuff ) ); 

  /* determine the length of the length element */
  elemenLen = ( info->islong ? LEN_LONG_LENGTH_OF_RVLC_SF : LEN_SHORT_LENGTH_OF_RVLC_SF );

  /* write length of rvlc in bitstream */
  escBitCount = GetBitCount( &escapeBitBuff ); 
  WriteElemToBuffer(LENGTH_OF_RVLC_SF, escBitCount, elemenLen);

  /* write rvlc in bitstream */
  WriteBufferToBuffer(RVLC_CODE_SF, escapeBuffer, escBitCount);

  /* number of escapes */
  numOfEscVal = GetNumOfValues( pEscStrc ); 

  /* if all esc are 0 don't transmit'em */
  maxAbsVal = MaxAbsValue( encEpVal, numOfEscVal);
  if (maxAbsVal == 0) {
    numOfEscVal = 0;
  }

  /* transmit escapes, if there are */
  if (numOfEscVal == 0) {
    escPresent = 0;
    WriteElemToBuffer(SF_ESCAPES_PRESENT, escPresent, LEN_SF_ESCAPES_PRESENT);

  } else {
    escPresent = 1;
    WriteElemToBuffer(SF_ESCAPES_PRESENT, escPresent, LEN_SF_ESCAPES_PRESENT);

    assert(numOfEscVal <= 120);
    InitWriteBitBuf( &txEpBitBuff, txEpBuffer );
    EscapeHuffCoding( numOfEscVal, encEpVal, &txEpBitBuff);
    txEpbitCount = GetBitCount( &txEpBitBuff ); 

    /* write length of escapes */
    assert( txEpbitCount < (unsigned long) pow(2, LEN_LENGTH_OF_RVLC_ESCAPES) ); /*desaprovecho el cero*/
    assert( txEpbitCount > 0 );
    WriteElemToBuffer(LENGTH_OF_RVLC_ESCAPES, txEpbitCount, LEN_LENGTH_OF_RVLC_ESCAPES);

    /* write escapes */
    WriteBufferToBuffer( RVLC_ESC_SF,  txEpBuffer, txEpbitCount);
  }

  return returnValue;
}


/*******************************

 * ScfDecoding

 *******************************/
static int
ScfDecoding(Info *info, byte *group, int nsect, byte *sect,
            short global_gain, short *factors, BIT_BUF *data, short *revGG)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
  
  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);

  for ( i = 0; i < MAXBANDS; i++ )
    factors[i] = 0;
  
  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ )
    {
      top = sect[1];		/* top of section in sfb */
      t = sect[0];		/* codebook for this section */
      sect += 2;
      for ( ; sfb < top; sfb++ ) 
        fac_trans[sfb] = t;
    }
  
  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  
  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    PRINT(SE,"\nscale factors\n");
  for(b = 0; b < info->nsbk; )
    {
      n = info->sfb_per_sbk[b];
      b = *group++;
      for(i = 0; i < n; i++)
	{
	  if(fac_trans[i])
	    {
	      /* decode intensity positions */
	      if ( (info->nsbk==1) &&	    /* No short blocks yet! */
		   ( (fac_trans[i] == INTENSITY_HCB) || 
		     (fac_trans[i] == INTENSITY_HCB2) ) ) 
		{
                  assert(0);
                  /* decode intensity position */
		  t = GetScfBits(hcw, data);
		  is_pos += t - MIDFAC;
		  if (debug['f'])
		    PRINT(SE,"%3d %3d (is_pos)\n", i, is_pos);
		  factors[i] = is_pos;
		  continue;
		}
	      /* decode scale factor */
	      t = GetScfBits(hcw, data);
	      fac += t - MIDFAC;    /* 1.5 dB */
	      
	      if (debug['f'])
		PRINT(SE,"%3d", fac-global_gain); /* (-global_gain) included by RS */
	      
	      if( fac >= 2 * maxfac || fac < 0 )
		return 0;
	      
	      factors[i] = fac;
	    }
	}
      if (debug['f'])
	PRINT(SE,"\n");

#if 0      
      /* expand short block grouping */
      if ( ! ( info->islong ) ) 
        for ( bb++; bb < b; bb++ ) 
          {
            for ( i = 0; i < n; i++ ) 
              factors[i + n] = factors[i];
            factors += n;
          }
#endif

      fac_trans += n;
      factors += n;
    }

  *revGG = fac;
  return 1;
}


/******************************************************

    GetScfBits

*******************************************************/
static int
GetScfBits(Huffman *h, BIT_BUF *data )
{
  int i, j;
  unsigned long cw;
    
  i = h->len;
  cw = getbits(i);
  while (cw != h->cw) {
    h++;
    j = h->len-i;
    i += j;
    cw <<= j;
    cw |= getbits(j);
  }
  WriteBitBuf( data, cw, (unsigned long) i );

  return(h->index);
}


/***********************************************************************
  ChangeTable

 serves to substitute the Huffman

 ***********************************************************************/
static int
ChangeTable(Info *info, byte *group, int nsect, byte *sect,
            short global_gain, short *factors, 
            BIT_BUF *readBuff, BIT_BUF *writeBuff, Escape escStruct)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
  
  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);

  for ( i = 0; i < MAXBANDS; i++ )
    factors[i] = 0;
  
  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ )
    {
      top = sect[1];		/* top of section in sfb */
      t = sect[0];		/* codebook for this section */
      sect += 2;
      for ( ; sfb < top; sfb++ ) 
        fac_trans[sfb] = t;
    }
  
  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  
  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    PRINT(SE,"\nscale factors\n");
  for(b = 0; b < info->nsbk; )
    {
      n = info->sfb_per_sbk[b];
      b = *group++;
      for(i = 0; i < n; i++)
	{
	  if(fac_trans[i])
	    {
	      /* decode intensity positions */
	      if ( (info->nsbk==1) &&	    /* No short blocks yet! */
		   ( (fac_trans[i] == INTENSITY_HCB) || 
		     (fac_trans[i] == INTENSITY_HCB2) ) ) 
		{
                  /* decode intensity position */
                  assert(0);
		  t = EscapeBits(hcw, readBuff, writeBuff, escStruct);
		  is_pos += t - MIDFAC;
		  if (debug['f'])
		    PRINT(SE,"%3d %3d (is_pos)\n", i, is_pos);
		  factors[i] = is_pos;
		  continue;
		}
	      /* decode scale factor */
	      t = EscapeBits(hcw, readBuff, writeBuff, escStruct);
	      fac += t - MIDFAC;    /* 1.5 dB */
	      
	      if (debug['f'])
		PRINT(SE,"%3d", fac-global_gain); /* (-global_gain) included by RS */
	      
	      if( fac >= 2 * maxfac || fac < 0 )
		return 0;
	      
	      factors[i] = fac;
	    }
	}
      if (debug['f'])
	PRINT(SE,"\n");
      
      /* expand short block grouping */
      if ( ! ( info->islong ) ) 
        for ( bb++; bb < b; bb++ ) 
          {
            for ( i = 0; i < n; i++ ) 
              factors[i + n] = factors[i];
            factors += n;
          }
      fac_trans += n;
      factors += n;
    }
  return 1;
}

/**************************************************************

  EscapeBits

  convert to escape bitstream

**************************************************************/
static int
EscapeBits(Huffman *h, BIT_BUF *readBuff, BIT_BUF *writeBuff, Escape escStruct)
{
  int i, j;
  unsigned long cw;

  int       diffScf;
  int       index;

  i = h->len;
  cw = ReadBitBuf(readBuff, i);
  while (cw != h->cw) {
    h++;
    j = h->len-i;
    i += j;
    cw <<= j;
    cw |= ReadBitBuf(readBuff, j);
  }

  /* decoded value + last escape value */
  diffScf = h->index - MIDFAC ;/* + GetLastValue( escStruct ); */

  if ( ABS( diffScf ) < ABS_ESC_VAL ) {

    /* no escape value needed */
    index = diffScf + ABS_ESC_VAL;
    WriteBitBuf( writeBuff, bookRvlc[index].cw, (unsigned long) bookRvlc[index].len );

  } else { /* ABS( diffScf ) >= ABS_ESC_VAL */ 

    if ( diffScf >= ABS_ESC_VAL ) {
      /* write +ABS_ESC_VAL */
      WriteBitBuf( writeBuff, bookRvlc[2*ABS_ESC_VAL].cw, (unsigned long) bookRvlc[2*ABS_ESC_VAL].len);
      AddEscValue( escStruct, (diffScf - ABS_ESC_VAL) );
        
    } else { /* diffScf <= -ABS_ESC_VAL */
      /* write -ABS_ESC_VAL */
      WriteBitBuf( writeBuff, bookRvlc[0].cw, (unsigned long) bookRvlc[0].len);
      AddEscValue( escStruct, ABS(diffScf + ABS_ESC_VAL) );
    }
  }

  return(h->index);
}


/******************************************************************************************************/

/***************************************************

 EscapeHuffCoding

 ***************************************************/
static void
EscapeHuffCoding(
                 int   numOfValues,
                 int * pEscValues,
                 BIT_BUF * writeBuff
                 )
{
  int i;
  int value;

  for (i=0; i<numOfValues; i++) {
    value = pEscValues[i];
    assert( value >= 0 );
    assert( value <= 53 );
    WriteBitBuf( writeBuff, bookEscape[value].cw, (unsigned long) bookEscape[value].len);
  }
  
  FlushWriteBitBuf( writeBuff );
}

/******************************************************************************************************/
/* MaxAbsValue */
static int 
MaxAbsValue( 
            int * p_vec, 
            int size 
            )
{
  int   i;
  int maxAbsVal;
  int absValue;
  
  maxAbsVal = INT_MIN;
  
  for( i=0; i<size; i++ ) {
    absValue = ABS( *p_vec );
    if( maxAbsVal < absValue ) {
      maxAbsVal = absValue;
    }
    p_vec++;
  }
  return( maxAbsVal );
}

/******************************************************************************************************/

#ifdef WRITE_SCF
/***************************************************

 WriteFactors                                                   

 ***************************************************/
static void
WriteFactors(
             short * factors,
             int numberOfFactors
             )
{
  int i;

  fprintf(stderr, "scalefactors : ");
  for(i=0; i<numberOfFactors; i++) {
    if (fprintf( stderr, "%hd \t", factors[i] ) < 0 ) {
      fprintf (stderr, "fprintf failed : %m\n");  
    }
  }
  fprintf(stderr, "\n");
}

/***************************************************

 WriteEscapes

 ***************************************************/
static void
WriteEscapes(
             int * escapes,
             int numberOfEscapes
             )
{
  int i;

  for(i=0; i<numberOfEscapes; i++) {
    if (fprintf( stderr, "%hd \t", escapes[i] ) < 0 ) {
      fprintf (stderr, "fprintf failed : %m\n");  
    }
  }
  fprintf(stderr, "\n");
}
#endif
