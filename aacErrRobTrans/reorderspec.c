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
#include "reorderspec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitfct.h"
#include "all.h"
#include "resorg.h"
#include "statistics.h"
#include "escHandling.h"
#include "interface.h"

#define PRE_SORTING_STEPS 6

#ifndef MAX    
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_NR_OF_SFB        51
#define MAX_NR_OF_SWB       120
#define MAX_SPEC_BITLENGTH 4096    /* maximum allowed length of spectral part of bitstream in one channel */
/* #define DEBUG_TESTVEC */
/* #define DEBUG_HCR_SQUARE */

typedef enum {
  LEFT,
  RIGHT,
  MAX_SIDES
} SIDES;

typedef struct {
  unsigned short number;           /* original codeword number */ 
  unsigned short section;          /* section */
  unsigned short groupNr;          /* group number */
  unsigned short nrOfWindows;      /* number of windows within current group */
  unsigned short sfb;              /* current scalefactorband */
  unsigned short sfbInGroup;       /* current scalefactorband in group */
  unsigned short window;           /* window */
  unsigned short unit;             /* unit number within window, a unit covers 4 lines (1 or 2 CWs) */
  unsigned short table;            /* huffman code table */
  unsigned short maxCWLen;         /* length of longest possible codeword of current table */
  unsigned short segmentWidth;     /* segment width for current table */
  unsigned short codeDim;          /* 2-dim or 4-dim code */
  unsigned long  code;             /* lower 32 bits of codeword, right aligned */
  unsigned long  codeExt;          /* higher 32 bits of codeword, right aligned */
  unsigned short len;              /* length of codeword */
  unsigned short pcwFlag;          /* signals, that this codeword is a PCW */
} CODEWORD_INFO;

typedef struct {
  unsigned short segmentStartPosition[MAX_SIDES];
  unsigned short usedBits[MAX_SIDES];
  unsigned short remainingBits;    /* help variable used to store the number of bits
                                      which were not used by a priority codeword */
} SEGMENT_INFO;

typedef struct {
  unsigned char   islong;                      /* 0: short, 1: long */
  unsigned char   maxSfb;
  unsigned char   numGroups;
  unsigned short  numCodewords;                /* number of codewords within block */
  unsigned short  nrOfSegments;                /* number of PCWs within block */
  unsigned char   lengthOfLongestCodeword;     /* length of the longest codeword within block */
  unsigned short  numGroupCodewords[MAX_SBK];  /* number of codewords within a group */
  unsigned short  numSpectrumBits;             /* total number of bits */
  unsigned short  numPcwBits;                  /* number of bits used by PCWs */
  unsigned short  numGapBits;                  /* number of gap bits */
  unsigned short  numNonPcwBits;               /* number of bits used by nonPCWs */
  unsigned short  numGroupBits[MAX_SBK];       /* number of bits within a group */
  unsigned short  nrOfLines;                   /* number of lines within block */
  unsigned short  nrOfLinesWithinPcws;         /* number of spectral lines within PCWs */
  CODEWORD_INFO   Huffdata[LN4];               /* codewords and lengths */
  SEGMENT_INFO    segmentInfo[LN4];
} SPECTRAL_DATA_INFO;

static SPECTRAL_DATA_INFO spectralData;
static unsigned short     linesPerSfb[NSHORT][MAX_NR_OF_SFB];


/****************************************************************************
 ***
 ***    BitRoutines
 ***
 ****************************************************************************/
static char  BitVector[MAX_SPEC_BITLENGTH] = {0};
static short bitPosition[MAX_SIDES];  
static char* value_true    = "1";
static char* value_false   = "0";

static void InitBitstream ( )
{
  int   i;
  char* init  = "-";
  for ( i = 0; i < MAX_SPEC_BITLENGTH; i++) {
    memcpy (BitVector+i, init, 1);
  }
  for ( i = 0; i < LN4; i++ ) {
    spectralData.segmentInfo[i].segmentStartPosition[LEFT]  = 0;
    spectralData.segmentInfo[i].segmentStartPosition[RIGHT] = 0;
    spectralData.segmentInfo[i].usedBits[LEFT]              = 0;
    spectralData.segmentInfo[i].usedBits[RIGHT]             = 0;
    spectralData.segmentInfo[i].remainingBits               = 0;
  }
}

static void PutBitstream ( CODEWORD_INFO codeword, 
                           int           len,
                           SIDES         side )
{
  unsigned short i             = codeword.len;
  unsigned short remainingBits = codeword.len - len;
  
  for ( ; i >MAX ( UINT_BITLENGTH, remainingBits ); i-- ) {
    if ( codeword.codeExt & ( 1 << ( i - 1 - UINT_BITLENGTH ) ) ) {
      memcpy ( BitVector + bitPosition[side], value_true,  1 );
    }
    else {
      memcpy ( BitVector + bitPosition[side], value_false, 1 );
    }
    side == LEFT ? bitPosition[side]++ : bitPosition[side]--;
    if ( bitPosition[side] < 0 || bitPosition[side] > spectralData.numSpectrumBits ) {
      printf ( "bitposition exception (1)\n");
      exit ( 1 );
    }
  }
  for ( ; i > remainingBits; i-- ) {
    if ( codeword.code & ( 1 << ( i - 1 ) ) ) {
      memcpy ( BitVector + bitPosition[side], value_true,  1 );
    }
    else {
      memcpy ( BitVector + bitPosition[side], value_false, 1 );
    }
    side == LEFT ? bitPosition[side]++ : bitPosition[side]--;
    if ( bitPosition[side] < 0 || bitPosition[side] > spectralData.numSpectrumBits ) {
      printf ( "bitposition exception (2)\n");
      exit ( 1 );
    }
  }
}

static void RewindBitstream ( )
{
  bitPosition[LEFT]  = 0;
  bitPosition[RIGHT] = spectralData.numSpectrumBits;
}

static void SkipBitstream ( unsigned short nrOfSkipBits,
                            SIDES          side )
{
  if ( side == LEFT ) {
    bitPosition[side] += nrOfSkipBits;
  }
  else {
    bitPosition[side] -= nrOfSkipBits;
  }
}

static void WriteBitVector (unsigned char *out, int len) 
{
  int i;
  int j;
  int Word;
  char bit[2]={0};
  int WordLength = 8; /* fwrite is using bytes as data-objects */
  
  len = ( (int) ( len / WordLength ) + 1 ); 
  
  for ( i = 0; i < len; i++ ) {
    Word = 0;
    for ( j = 0; j < WordLength; j++ ) {
      memcpy ( bit, BitVector + i * WordLength + 7 - j, 1 );
      Word += (1<<j) * atoi (bit);
    }
    out[i] = Word;
#ifdef DEBUG_TESTVEC
    fprintf(stderr,"\nout[%d]=%d",i,out[i]);
#endif
  }
}
/********** end of bitroutines ************/

void ResetHuffdataTable ( unsigned char islong,
                          unsigned char maxSfb,
                          unsigned char numGroups )
{
  unsigned short groupNr;
  unsigned short sfb;
  
  spectralData.islong                    = islong;
  spectralData.maxSfb                    = maxSfb;
  spectralData.numGroups                 = numGroups;
  spectralData.numCodewords              = 0; 
  spectralData.numSpectrumBits           = 0;
  spectralData.numPcwBits                = 0;
  spectralData.numGapBits                = 0;
  spectralData.numNonPcwBits             = 0;
  spectralData.lengthOfLongestCodeword   = 0;
  for ( groupNr = 0; groupNr < MAX_SBK; groupNr++ )
    {
      spectralData.numGroupBits[groupNr]      = 0;
      spectralData.numGroupCodewords[groupNr] = 0;
    }
  for ( groupNr = 0; groupNr < NSHORT; groupNr++ )
    for ( sfb = 0; sfb < MAX_NR_OF_SFB; sfb++ )
      linesPerSfb[groupNr][sfb] = 0;
}

void AddHuffdataTable ( unsigned short section,
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
                        unsigned long  codelen ) 
{
  spectralData.Huffdata[spectralData.numCodewords].number       = spectralData.numCodewords;
  spectralData.Huffdata[spectralData.numCodewords].section      = section;
  spectralData.Huffdata[spectralData.numCodewords].groupNr      = groupNr;
  spectralData.Huffdata[spectralData.numCodewords].nrOfWindows  = nrOfWindows;
  spectralData.Huffdata[spectralData.numCodewords].sfb          = sfb;
  spectralData.Huffdata[spectralData.numCodewords].sfbInGroup   = sfbInGroup;
  spectralData.Huffdata[spectralData.numCodewords].codeDim      = step;
  spectralData.Huffdata[spectralData.numCodewords].table        = table;
  spectralData.Huffdata[spectralData.numCodewords].maxCWLen     = maxCWLen;
  spectralData.Huffdata[spectralData.numCodewords].code         = codeword;
  spectralData.Huffdata[spectralData.numCodewords].codeExt      = codeword_ext;
  spectralData.Huffdata[spectralData.numCodewords].len          = codelen;
  spectralData.Huffdata[spectralData.numCodewords].window       = window;
  spectralData.Huffdata[spectralData.numCodewords].unit         = unit;
  spectralData.Huffdata[spectralData.numCodewords].pcwFlag      = 0;
  spectralData.numCodewords++;
  spectralData.numSpectrumBits += codelen;
  if ( spectralData.lengthOfLongestCodeword < codelen ) {
    spectralData.lengthOfLongestCodeword = codelen;
  }
  spectralData.numGroupCodewords[groupNr]++;
  spectralData.numGroupBits[groupNr] += codelen;

  linesPerSfb[groupNr][sfbInGroup] += step;
  if ( debug['S'] )
    StatisticsCountNumberOfCodewordsDependingOnLength ( codelen );
}

void UpdateVirtualCodebook ( unsigned short sfb,
                             unsigned short virtualCodebook,
                             unsigned short maxCWLen )
{
  unsigned short codeword;

  for ( codeword = 0; codeword < spectralData.numCodewords; codeword++ ) {
    if ( spectralData.Huffdata[codeword].sfb == sfb ) {
       spectralData.Huffdata[codeword].table    = virtualCodebook;
       spectralData.Huffdata[codeword].maxCWLen = maxCWLen;
    }
  }
}

void CompletionHuffdataTable ( )
{
  unsigned short currentCodeword;
  
  for ( currentCodeword = 0; currentCodeword < spectralData.numCodewords; currentCodeword++ ) {
    spectralData.Huffdata[currentCodeword].segmentWidth 
      = MIN ( spectralData.Huffdata[currentCodeword].maxCWLen, spectralData.lengthOfLongestCodeword );
#if 0
    if ( spectralData.lengthOfLongestCodeword < spectralData.Huffdata[currentCodeword].maxCWLen ) {
      PRINT(SE,"segWidth could successfully be reduced by %d (%d-%d) due to maxCWLen transmission\n",
            spectralData.Huffdata[currentCodeword].maxCWLen-spectralData.lengthOfLongestCodeword,
            spectralData.Huffdata[currentCodeword].maxCWLen,
            spectralData.lengthOfLongestCodeword);
    }
#endif
  }
}

void WindowInterleaveHuffdataTable ( )
{
  CODEWORD_INFO huffDataInterleaved[LN4];
  unsigned short currentCodeword;
  unsigned short demandedCodeword;
  unsigned short unitInterleaveLevel;
  unsigned short unitsInSpectralDirection;
  unsigned short unit;
  unsigned short preSortingIndex;

  struct {
    unsigned short min;
    unsigned short max;
  } preCodebookSorting[] = {{ 11, 11},
                            { 31, 31},
                            { 30, 30},
                            { 29, 29},
                            { 28, 28},
                            { 27, 27},
                            { 26, 26},
                            { 25, 25},
                            { 24, 24},
                            { 23, 23},
                            { 22, 22},
                            { 21, 21},
                            { 20, 20},
                            { 19, 19},
                            { 18, 18},
                            { 17, 17},
                            { 16, 16},
                            {  9, 10},
                            {  7,  8},
                            {  5,  6},
                            {  3,  4},
                            {  1,  2}};
  if ( spectralData.islong )
    unitsInSpectralDirection = LN2 / 4;
  else
    unitsInSpectralDirection = SN2 / 4;
  currentCodeword = 0;
  for ( preSortingIndex = 0; preSortingIndex < sizeof(preCodebookSorting)/ ( sizeof(short) * 2 ) ; preSortingIndex++ ) {
    for ( unit = 0; unit < unitsInSpectralDirection; unit++ ) {
      for ( demandedCodeword = 0; demandedCodeword < spectralData.numCodewords; demandedCodeword++ ) {
        if ( spectralData.Huffdata[demandedCodeword].unit == unit ) {
          if ( ( ! GetReorderSpecPreSortFlag ( ) ) ||
               ( ( spectralData.Huffdata[demandedCodeword].table >= preCodebookSorting[preSortingIndex].min ) &&
                 ( spectralData.Huffdata[demandedCodeword].table <= preCodebookSorting[preSortingIndex].max ) ) ) {
            huffDataInterleaved[currentCodeword++] =  spectralData.Huffdata[demandedCodeword];
          }
        }
      }
    }
    if ( ! GetReorderSpecPreSortFlag ( ) ) {
      break;
    }
  }
  currentCodeword = 0;
  for ( unitInterleaveLevel = 0; unitInterleaveLevel < UNIT_INTERLEAVING; unitInterleaveLevel++ ) {
    for ( demandedCodeword = 0; demandedCodeword < spectralData.numCodewords; demandedCodeword++ ) {
      if ( ( ( huffDataInterleaved[demandedCodeword].unit
               + UNIT_INTERLEAVING - unitInterleaveLevel ) % UNIT_INTERLEAVING ) == 0 ) {
        spectralData.Huffdata[currentCodeword++] =  huffDataInterleaved[demandedCodeword];
      }
    }
  }
}

void PrintHuffdataTable ( )
{
  unsigned int i;
  int max = 0;

#if 0
  PRINT(SE,"Huffdata: numCodewords: %3ld, numSpectrumBits: %4ld\n",
	spectralData.numCodewords,spectralData.numSpectrumBits);
#endif

  for( i = 0; i < spectralData.numCodewords; i++ ) 
    {
      if ( spectralData.Huffdata[i].len > max )
	max = spectralData.Huffdata[i].len;

#if 1
      fprintf(stderr,"code %3d, group %hu, section %2hu, sfb %2hu (%2hu)", 
              i,                                      /* comparison with decoder */
              spectralData.Huffdata[i].groupNr,       /* comparison with decoder */
              spectralData.Huffdata[i].section,       /* comparison with decoder */
              spectralData.Huffdata[i].sfb,           /* comparison with decoder */
              spectralData.Huffdata[i].sfbInGroup     /* comparison with decoder */
              );
#endif
#if 0
      fprintf(stderr,", nrOfWindows %hu, win %hu, unit %3hu", 
              spectralData.Huffdata[i].nrOfWindows,
              spectralData.Huffdata[i].window,
              spectralData.Huffdata[i].unit
              );
#endif
#if 1
      fprintf(stderr,", table %2d, segWidth %2d", 
              spectralData.Huffdata[i].table,     /* comparison with decoder */
              spectralData.Huffdata[i].segmentWidth
              );
/*       fprintf(stderr,", %3hu",  */
/*               spectralData.Huffdata[i].unit */
/*               ); */
#endif
#if 0
      fprintf(stderr,", LinesPerSfb %2hu, Dim %hu, Codeword %8lx%8lx, len  %2d", 
              linesPerSfb[spectralData.Huffdata[i].groupNr][spectralData.Huffdata[i].sfb],
              spectralData.Huffdata[i].codeDim,
              spectralData.Huffdata[i].codeExt, 
              spectralData.Huffdata[i].code, 
              spectralData.Huffdata[i].len
              );
#endif
      fprintf(stderr,"\n");
    }
#if 0
  PRINT(SE,"MaxLen: %3d    AvgLen: %f\n\n",max,(float) spectralData.numSpectrumBits / spectralData.numCodewords);
#endif
}

static void WritePcws ( )
{
  unsigned short len;
  unsigned short currentSegment = 0;
  unsigned short currentCodeword = 0;
  unsigned long  remainingBufferBits = spectralData.numSpectrumBits;
  SIDES          side = LEFT;

  while ( spectralData.Huffdata[currentCodeword].segmentWidth <= remainingBufferBits 
          && currentSegment < spectralData.numCodewords ) {
    spectralData.Huffdata[currentCodeword].pcwFlag = 1;
    if ( debug['S'] ) {
      StatisticsCountSegmentBits ( spectralData.Huffdata[currentCodeword].table,
                                   spectralData.Huffdata[currentCodeword].maxCWLen,
                                   spectralData.Huffdata[currentCodeword].segmentWidth,
                                   spectralData.Huffdata[currentCodeword].len );
    }
#ifdef DEBUG_HCR_SQUARE_1
    fprintf(stderr,"%2d, %2d, %2d, %2d, %4d\n",
            spectralData.Huffdata[currentCodeword].table,
            spectralData.Huffdata[currentCodeword].maxCWLen,
            spectralData.Huffdata[currentCodeword].segmentWidth,
            spectralData.Huffdata[currentCodeword].len,
            bitPosition[side] );
#endif /*DEBUG_HCR_SQUARE_1*/
    spectralData.segmentInfo[currentSegment].segmentStartPosition[LEFT] = bitPosition[side];
    spectralData.segmentInfo[currentSegment].segmentStartPosition[RIGHT] = bitPosition[side] + spectralData.Huffdata[currentCodeword].segmentWidth - 1;
    remainingBufferBits -= spectralData.Huffdata[currentCodeword].segmentWidth;
    len = spectralData.Huffdata[currentCodeword].len;
    spectralData.numPcwBits += len;      /* for statistics only */
    PutBitstream ( spectralData.Huffdata[currentCodeword], 
                   len,
                   side);
    spectralData.Huffdata[currentCodeword].len -= len;
    spectralData.segmentInfo[currentSegment].usedBits[LEFT] = len;
    spectralData.segmentInfo[currentSegment].remainingBits  = spectralData.Huffdata[currentCodeword].segmentWidth - len;
    SkipBitstream ( spectralData.segmentInfo[currentSegment].remainingBits, side );
    spectralData.nrOfLinesWithinPcws += spectralData.Huffdata[currentCodeword].codeDim; /* for statistics only */
#ifdef DEBUG_HCR_SQUARE
    fprintf (stderr,"\n%3d: %2d", currentSegment, spectralData.segmentInfo[currentSegment].remainingBits);
#endif /*DEBUG_HCR_SQUARE*/
    currentSegment  ++;
    currentCodeword ++;
  }
  spectralData.segmentInfo[currentSegment-1].remainingBits += remainingBufferBits; /* this adds the remaining buffer part to the last segment */
  spectralData.segmentInfo[currentSegment-1].segmentStartPosition[RIGHT] += remainingBufferBits;
  spectralData.nrOfSegments = currentSegment;
#ifdef DEBUG_HCR_SQUARE
  fprintf (stderr,", update: %3d: %2d\n", currentSegment-1, spectralData.segmentInfo[currentSegment-1].remainingBits);
  fprintf (stderr,"number of codewords : %3d, number of segments : %3d\n", spectralData.numCodewords, spectralData.nrOfSegments);
#endif /*DEBUG_HCR_SQUARE*/
}

static void WriteNonPcws ( SIDES side )
{
  unsigned short currentSegment = 0;
  unsigned short currentCodeword;

  SkipBitstream ( spectralData.segmentInfo[currentSegment].usedBits[side], side );
  for ( currentCodeword = 0; currentCodeword < spectralData.numCodewords; currentCodeword++ ) {
    spectralData.nrOfLines += spectralData.Huffdata[currentCodeword].codeDim; /* for statistics only */
    if ( spectralData.Huffdata[currentCodeword].len > 0 ) {             /* if something has to be written */
      while ( ( currentSegment < spectralData.nrOfSegments                 /* while there are segments left && */
                && spectralData.segmentInfo[currentSegment].remainingBits  /* there is not enough space within the current segment */
                < spectralData.Huffdata[currentCodeword].len ) ) {
        PutBitstream ( spectralData.Huffdata[currentCodeword],           /* write part of current codeword */
                       spectralData.segmentInfo[currentSegment].remainingBits,
                       side );
        spectralData.numGapBits += spectralData.segmentInfo[currentSegment].remainingBits; /* statistics only */
        if ( spectralData.Huffdata[currentCodeword].pcwFlag == 0 ) {
          spectralData.numNonPcwBits += spectralData.segmentInfo[currentSegment].remainingBits; /* statistics only */
        }
        spectralData.Huffdata[currentCodeword].len -= spectralData.segmentInfo[currentSegment].remainingBits;
        spectralData.segmentInfo[currentSegment].usedBits[side] += spectralData.segmentInfo[currentSegment].remainingBits;
        spectralData.segmentInfo[currentSegment].remainingBits  = 0;
        currentSegment++;
        if ( currentSegment >= spectralData.nrOfSegments ) {
          break;
        }
        SkipBitstream ( spectralData.segmentInfo[currentSegment].usedBits[side], side ); 
      }
      PutBitstream ( spectralData.Huffdata[currentCodeword],            /* write last part of current codeword */
                     spectralData.Huffdata[currentCodeword].len,
                     side );
      spectralData.numGapBits += spectralData.Huffdata[currentCodeword].len; /* statistics only */
      if ( spectralData.Huffdata[currentCodeword].pcwFlag == 0 ) {
        spectralData.numNonPcwBits  += spectralData.Huffdata[currentCodeword].len; /* statistics only */
      }
      if ( currentSegment < spectralData.nrOfSegments ) {
        spectralData.segmentInfo[currentSegment].remainingBits  -= spectralData.Huffdata[currentCodeword].len;
        spectralData.segmentInfo[currentSegment].usedBits[side] += spectralData.Huffdata[currentCodeword].len;
      }
      spectralData.Huffdata[currentCodeword].len = 0;
    }
  }
}

static void WriteNonPcwsNewCore (  unsigned short currentSegment,
                                  unsigned short currentCodeword,
                                  SIDES          side ) 
{
  unsigned short transferBits;

  transferBits = MIN ( spectralData.Huffdata[currentCodeword].len, 
                       spectralData.segmentInfo[currentSegment].remainingBits ); /* derive no of bits that can be written into the bap*/
  if ( transferBits ) {
    bitPosition[side] = spectralData.segmentInfo[currentSegment].segmentStartPosition[side];
    SkipBitstream ( spectralData.segmentInfo[currentSegment].usedBits[side], side ); /* jump to write position */
    PutBitstream ( spectralData.Huffdata[currentCodeword], transferBits, side ); /* write those bits */
    spectralData.Huffdata[currentCodeword].len -= transferBits; /* reduce the codeword len by the written bits */
    spectralData.segmentInfo[currentSegment].remainingBits  -= transferBits; /* reduce the gap by the written bits */
    spectralData.segmentInfo[currentSegment].usedBits[side] += transferBits;
    spectralData.numNonPcwBits += transferBits; /* statistics only */
#ifdef DEBUG_HCR_SQUARE
    fprintf(stderr, "segm:%3d cw:%3d transbits:%2d, remaining in cw: %2d, remaining in segment: %2d\n", 
            currentSegment, 
            currentCodeword, 
            transferBits,
            spectralData.Huffdata[currentCodeword].len, 
            spectralData.segmentInfo[currentSegment].remainingBits);
#endif /*DEBUG_HCR_SQUARE*/
  }
}

static void WriteNonPcwsNew ( )
{
  unsigned short set; /* a set contains a certain no of codewords, set zero contains the PCWs */
  unsigned short noOfSets;
  unsigned short codewordsInSet;
  unsigned short trial; /* each set is tried several times to be stored */
  unsigned short currentSegment;
  unsigned short currentCodeword;
  unsigned short deli1; /* delimiter for first part */
  unsigned short deli2; /* delimiter for second part */
  SIDES          side;  /* writes from left or from right */

  noOfSets = (unsigned short ) ( (float) ( spectralData.numCodewords + spectralData.nrOfSegments - 1 ) / spectralData.nrOfSegments );
  for ( set = 1; set < noOfSets; set++ ) {
    side = (SIDES) ( set % MAX_SIDES );
/*     side = LEFT; */
#ifdef DEBUG_HCR_SQUARE
    fprintf(stderr,"set %d of %d, side = %d \n",set, noOfSets-1, side);
#endif /*DEBUG_HCR_SQUARE*/
    codewordsInSet = MIN ( spectralData.nrOfSegments, spectralData.numCodewords - ( spectralData.nrOfSegments * set ) );
    for ( trial = 0; trial < spectralData.nrOfSegments; trial++ ) {
      currentCodeword = spectralData.nrOfSegments * set;
      deli1 = MIN ( spectralData.nrOfSegments, trial + codewordsInSet );
      deli2 = MAX ( 0, trial + codewordsInSet - spectralData.nrOfSegments );
#ifdef DEBUG_HCR_SQUARE
      fprintf(stderr,"trial %3d of %3d: ", trial, spectralData.nrOfSegments-1);
      fprintf(stderr,"%3d --> %3d, %3d --> %3d\n", trial, deli1, 0, deli2);
#endif /*DEBUG_HCR_SQUARE*/
      for ( currentSegment = trial; currentSegment < deli1; currentSegment++, currentCodeword++ ) {
        WriteNonPcwsNewCore( currentSegment, currentCodeword, side );
      }
      for ( currentSegment = 0    ; currentSegment < deli2; currentSegment++, currentCodeword++ ) {
        WriteNonPcwsNewCore( currentSegment, currentCodeword, side );
      }
    }
  }
}

void ReorderSpec ( ) 
{
  unsigned char  byteVector[MAX_SPEC_BITLENGTH / 8];

  assert( spectralData.numSpectrumBits <= MAX_SPEC_BITLENGTH );
  if ( spectralData.numCodewords == 0 ) {
    WriteElemToBuffer(LENGTH_OF_REORDERED_SPECTRAL_DATA, 0, LEN_SPEC_DATA);
    WriteElemToBuffer(LENGTH_OF_LONGEST_CODEWORD, 0, LEN_LONGEST_CW);
    return;
  }
  InitBitstream();
  RewindBitstream();
  spectralData.nrOfLines               = 0;
  spectralData.nrOfLinesWithinPcws     = 0;
  WritePcws  ( );
  if ( GetConsecutiveReorderSpecFlag () ) {
    WriteNonPcwsNew ( );
  }
  else {
    RewindBitstream ( );
    WriteNonPcws ( LEFT );
  }
  if ( debug['S'] && debug['r'] ) {
    StatisticsHcrCwAnalysis ( spectralData.numCodewords,
                              spectralData.nrOfSegments );
    StatisticsHcrBitAnalysis ( spectralData.numSpectrumBits,
                               spectralData.numPcwBits,
                               spectralData.numGapBits,
                               spectralData.numNonPcwBits);
  }
  if ( spectralData.numPcwBits + spectralData.numNonPcwBits != spectralData.numSpectrumBits )
    PRINT(SE,"ERROR: spectralData.numPcwBits + spectralData.numNonPcwBits != spectralData.numSpectrumBits\n");
  /* The data is stored in the string char-BitVector 
     it has to be written to a binary char ByteVector */
  WriteBitVector (&byteVector[0], spectralData.numSpectrumBits);
  /* This function writes the main (spectral) part of the reordered bitstream */
  /* (2) future use as a transcoder in a esc instance buffer */
  WriteElemToBuffer(LENGTH_OF_REORDERED_SPECTRAL_DATA, spectralData.numSpectrumBits, LEN_SPEC_DATA);
  WriteElemToBuffer(LENGTH_OF_LONGEST_CODEWORD, spectralData.lengthOfLongestCodeword, LEN_LONGEST_CW);
  WriteBufferToBuffer(REORDERED_SPECTRAL_DATA, &byteVector[0], spectralData.numSpectrumBits);
  if ( debug['S'] )
    StatisticsCountPcws ( spectralData.numCodewords, 
                          spectralData.nrOfSegments, 
                          spectralData.nrOfLines,
                          spectralData.nrOfLinesWithinPcws );
}

unsigned short GetLengthOfLongestCodeword ( )
{
  return ( spectralData.lengthOfLongestCodeword );
}

