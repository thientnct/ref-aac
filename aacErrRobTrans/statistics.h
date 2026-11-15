#define STAT_MAX_SECTIONS 40
#define LONGEST_POSSIBLE_CODEWORD 49

void            StatisticsInit( void );
void            StatisticsPrint ( void );
void            StatisticsCountNumberOfSectionsShort ( unsigned short nsect );
void            StatisticsCountNumberOfSectionsLong  ( unsigned short nsect );
void            StatisticsCountBitsForBitstreamParts ( char* string );
void            StatisticsCountCbProbability ( unsigned char cb,
                                               unsigned char nrOfSfbs );
void            StatisticsTnsUsed ( unsigned long  bitsUsedForTns,
                                    unsigned short short_flag );
void            StatisticsHcrCwAnalysis ( unsigned short numCodewords,
                                          unsigned short numPcws );
void            StatisticsHcrBitAnalysis ( unsigned short numSpectrumBits,
                                           unsigned short numPcwBits,
                                           unsigned short numGapBits,
                                           unsigned short numNonPcwBits );
void            StatisticsResetBitsForBitstreamParts ( void );
void            StatisticsCountPcws          ( unsigned short numCodewords, 
                                               unsigned short nrOfPcws, 
                                               unsigned short nrOfLines,
                                               unsigned short nrOfLinesWithinPcws );
void            StatisticsCountSegmentBits   ( unsigned short table, 
                                               unsigned short maxCWLen, 
                                               unsigned short segmentWidth, 
                                               unsigned short codewordLength );
void            StatisticsUsedEscapes        ( unsigned short value1, 
                                               unsigned short value2,
                                               int            sfbNr,
                                               int            section );
void            StatisticsCountNumberOfCodewordsDependingOnLength ( unsigned long codewordLength );
void            StatisticsCountBitsUsedForCodebooks ( unsigned short table,
                                                      unsigned short bits,
                                                      unsigned short maxValue );
