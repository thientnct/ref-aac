/* This file has been created automatically. */
/* Any changes in this file will be lost.    */

#define D(a) {const void *dummyfilepointer; dummyfilepointer = &a;}
#include <assert.h>
#include <stdio.h>
#include "statistics.h"
#include "bitfct.h"
#include "all.h"
#include "port.h"
#include "resorg.h"
#include "reorderspec.h"
#include "interface.h"
#define NR_OF_DIFFERENT_ESC_LENGTHS 10
#ifndef MAX    
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
void StatisticsHcrCwAnalysis ( unsigned short numCodewords,
                               unsigned short numPcws )
{
  D(numCodewords)
  D(numPcws)
}

void StatisticsHcrBitAnalysis ( unsigned short numSpectrumBits,
                                unsigned short numPcwBits,
                                unsigned short numGapBits,
                                unsigned short numNonPcwBits )
{
  D(numSpectrumBits)
  D(numPcwBits)
  D(numGapBits)
  D(numNonPcwBits)
}

void StatisticsResetBitsForBitstreamParts ( )
{
}

void StatisticsCountBitsForBitstreamParts ( char *string )
{
  D(string)
}

void StatisticsTnsUsed( unsigned long  bitsUsedForTns, 
                        unsigned short shortflag )
{
  D(bitsUsedForTns)
  D(shortflag)
}

void StatisticsCountNumberOfSectionsShort ( unsigned short nsect )
{
  D(nsect)
}

void StatisticsCountNumberOfSectionsLong ( unsigned short nsect )
{
  D(nsect)
}

void StatisticsCountCbProbability ( unsigned char cb, 
                                    unsigned char nrOfSfbs )
{
  D(cb)
  D(nrOfSfbs)
}

void StatisticsCountPcws ( unsigned short nrOfCodewords, 
                           unsigned short nrOfPcws, 
                           unsigned short nrOfLines,
                           unsigned short nrOfLinesWithinPcws )
{
  D(nrOfCodewords)
  D(nrOfPcws)
  D(nrOfLines)
  D(nrOfLinesWithinPcws)
}

#if 0
#endif
void StatisticsCountSegmentBits ( unsigned short table, 
                                  unsigned short maxCWLen,
                                  unsigned short segmentWidth, 
                                  unsigned short codewordLength )
{
  D(table)
  D(maxCWLen)
  D(segmentWidth)
  D(codewordLength)
}

void StatisticsUsedEscapes ( unsigned short value1, 
                             unsigned short value2, 
                             int sfbNr, 
                             int section )
{
  D(value1)
  D(value2)
  D(sfbNr)
  D(section)
}

void StatisticsCountNumberOfCodewordsDependingOnLength ( unsigned long codewordLength )
{
  D(codewordLength)
}

void StatisticsCountBitsUsedForCodebooks ( unsigned short table,
                                           unsigned short bits,
                                           unsigned short maxValue )
{
  D(table)
  D(bits)
  D(maxValue)
}

void StatisticsInit( )
{
}

void StatisticsPrint ( )
{
}

#undef D
