/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

$Id: ct_sbr.c,v 1.1.2.2 2007/06/12 00:13:14 samsung1 Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr encoding functions $Revision: 1.1.2.2 $
*/

/* 20060107 */
#undef PARAMETRICSTEREO /*temporally disabled for BSAC Extensions*/

#include <math.h>
#include "ct_sbr.h"
#include "interface.h" /* ID_FIL, LEN_SE_ID */
#ifdef PARAMETRICSTEREO
#include "ct_psenc.h"
#endif

#define EPS                                     1e-12
#define ILOG2                                   1.442695041f /* 1/LOG2 */

#define    LEN_EX_TYPE  (4)

#define SI_SBR_STD_EXTENSION     13  /* 1101 */
#define SI_SBR_CRC_EXTENSION     14  /* 1110 */

#define SI_SBR_HDR_BIT            1

/* header */

#define SI_SBR_PROTOCOL_VERSION_BITS            2
#define SI_SBR_AMP_RES_BITS                     1
#define SI_SBR_START_FREQ_BITS                  4
#define SI_SBR_STOP_FREQ_BITS                   4
#define SI_SBR_XOVER_BAND_BITS                  3
#define SI_SBR_RESERVED_BITS_HDR                2
#define SI_SBR_HEADER_EXTRA_1_BITS              1
#define SI_SBR_HEADER_EXTRA_2_BITS              1

/* data */
#define SI_SBR_COUPLING_BITS                    1

#define SBR_CLA_BITS    2
#define SBR_RES_BITS    1
#define FIXFIX  0

#define SI_SBR_INVF_MODE_BITS                   2


#define SI_SBR_RESERVED_D2_BITS                 1
#define SI_SBR_ADD_HARMONIC_ENABLE_BITS         1

#define SI_SBR_START_ENV_BITS_AMP_RES_3_0           6
#define SI_SBR_START_NOISE_BITS_AMP_RES_3_0         5


#define SI_SBR_EXTENDED_DATA_BITS               1


/* direction: freq
   contents : codewords
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

/* 20060107 */
const int v_Huff_envelopeLevelC11F[63] = {
  0x000FFFF0, 0x000FFFF1, 0x000FFFF2, 0x000FFFF3, 0x000FFFF4, 0x000FFFF5,
    0x000FFFF6, 0x0003FFF3,
  0x0007FFF5, 0x0007FFEE, 0x0007FFEF, 0x0007FFF6, 0x0003FFF4, 0x0003FFF2,
    0x000FFFF7, 0x0007FFF0,
  0x0001FFF5, 0x0003FFF0, 0x0001FFF4, 0x0000FFF7, 0x0000FFF6, 0x00007FF8,
    0x00003FFB, 0x00000FFD,
  0x000007FD, 0x000003FD, 0x000001FD, 0x000000FD, 0x0000003E, 0x0000000E,
    0x00000002, 0x00000000,
  0x00000006, 0x0000001E, 0x000000FC, 0x000001FC, 0x000003FC, 0x000007FC,
    0x00000FFC, 0x00001FFC,
  0x00003FFA, 0x00007FF9, 0x00007FFA, 0x0000FFF8, 0x0000FFF9, 0x0001FFF6,
    0x0001FFF7, 0x0003FFF5,
  0x0003FFF6, 0x0003FFF1, 0x000FFFF8, 0x0007FFF1, 0x0007FFF2, 0x0007FFF3,
    0x000FFFF9, 0x0007FFF7,
  0x0007FFF4, 0x000FFFFA, 0x000FFFFB, 0x000FFFFC, 0x000FFFFD, 0x000FFFFE,
    0x000FFFFF
};


/* direction: freq
   contents : codeword lengths
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

/* 20060107 */
const int v_Huff_envelopeLevelL11F[63] = {
  0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014,
    0x00000014, 0x00000012,
  0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000012, 0x00000012,
    0x00000014, 0x00000013,
  0x00000011, 0x00000012, 0x00000011, 0x00000010, 0x00000010, 0x0000000F,
    0x0000000E, 0x0000000C,
  0x0000000B, 0x0000000A, 0x00000009, 0x00000008, 0x00000006, 0x00000004,
    0x00000002, 0x00000001,
  0x00000003, 0x00000005, 0x00000008, 0x00000009, 0x0000000A, 0x0000000B,
    0x0000000C, 0x0000000D,
  0x0000000E, 0x0000000F, 0x0000000F, 0x00000010, 0x00000010, 0x00000011,
    0x00000011, 0x00000012,
  0x00000012, 0x00000012, 0x00000014, 0x00000013, 0x00000013, 0x00000013,
    0x00000014, 0x00000013,
  0x00000013, 0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014,
    0x00000014
};


static int countSbrHdr(int nChan)
{
  int bit_count = 0;

  bit_count += SI_SBR_AMP_RES_BITS +
               SI_SBR_START_FREQ_BITS +
               SI_SBR_STOP_FREQ_BITS +
               SI_SBR_XOVER_BAND_BITS +
               SI_SBR_RESERVED_BITS_HDR +
               SI_SBR_HEADER_EXTRA_1_BITS +
               SI_SBR_HEADER_EXTRA_2_BITS;

  return bit_count;
}


/*	static void writeSbrHdr(BsBitStream *ptrBS, SBR_CHAN *sbrChan, int nChan)*/
static void writeSbrHdr(HANDLE_BSBITSTREAM ptrBS, SBR_CHAN *sbrChan, int nChan)
{
  BsPutBit(ptrBS, 1, SI_SBR_AMP_RES_BITS);
  BsPutBit(ptrBS, sbrChan->startFreq, SI_SBR_START_FREQ_BITS);
  BsPutBit(ptrBS, sbrChan->stopFreq, SI_SBR_STOP_FREQ_BITS);
  BsPutBit(ptrBS, 0,  SI_SBR_XOVER_BAND_BITS);
  BsPutBit(ptrBS, 0,  SI_SBR_RESERVED_BITS_HDR);
  BsPutBit(ptrBS, 0,  SI_SBR_HEADER_EXTRA_1_BITS);
  BsPutBit(ptrBS, 0,  SI_SBR_HEADER_EXTRA_2_BITS);
}

static int countSbrGrid(void)
{
  int bit_count=0;

  bit_count += SBR_CLA_BITS; /* frame_class */
  bit_count += 2; /* temp */
  bit_count += SBR_RES_BITS; /* freq_res */

  return bit_count;
}

/*	static void writeSbrGrid(BsBitStream *ptrBS) */
static void writeSbrGrid(HANDLE_BSBITSTREAM ptrBS)
{
  BsPutBit(ptrBS, FIXFIX, SBR_CLA_BITS);
  BsPutBit(ptrBS, 1, 2); /* 2 envelopes per frame */
  BsPutBit(ptrBS, 1, SBR_RES_BITS);
}

static int countSbrDtdf(void)
{
  int bit_count=0;
  int env, noise;

  for (env=0; env<2; env++)
    bit_count += 1;

  for (noise=0; noise<2; noise++)
    bit_count += 1;

  return bit_count;
}

/*static void writeSbrDtdf(BsBitStream *ptrBs)*/
static void writeSbrDtdf(HANDLE_BSBITSTREAM ptrBs)
{
  int env, noise;

  for (env=0; env<2; env++)
    BsPutBit(ptrBs, 0, 1); /* delta f */

  for (noise=0; noise<2; noise++)
    BsPutBit(ptrBs, 0, 1); /* delta f */
}

static int countSbrEnvelope(SBR_CHAN *sbrChan)
{
  int bit_count=0;
  int env, band;
  int delta;

  for (env=0; env<2; env++) {
    bit_count += SI_SBR_START_ENV_BITS_AMP_RES_3_0;
    for (band=1; band<sbrChan->numBands; band++) {
      delta = sbrChan->envData[env][band] - sbrChan->envData[env][band-1] + 31;
      bit_count += v_Huff_envelopeLevelL11F[delta];
    }
  }

  return bit_count;
}

/*static void writeSbrEnvelope(BsBitStream *ptrBs, SBR_CHAN *sbrChan)*/
static void writeSbrEnvelope(HANDLE_BSBITSTREAM ptrBs, SBR_CHAN *sbrChan)
{
  int env, band;
  int delta;

  for (env=0; env<2; env++) {
    BsPutBit(ptrBs, sbrChan->envData[env][0], 
             SI_SBR_START_ENV_BITS_AMP_RES_3_0);
    for (band=1; band<sbrChan->numBands; band++) {
      delta = sbrChan->envData[env][band] - sbrChan->envData[env][band-1] + 31;
      BsPutBit(ptrBs, v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
  }
}

static int countSbrNoise(SBR_CHAN *sbrChan)
{
  int bit_count=0;
  int noise, band;
  int delta;

  for (noise=0; noise<2; noise++) {
    bit_count += SI_SBR_START_NOISE_BITS_AMP_RES_3_0;
    for (band=1; band<sbrChan->numNoiseBands; band++) {
      delta = sbrChan->noiseData[noise][band] - sbrChan->noiseData[noise][band]
              + 31;
      bit_count += v_Huff_envelopeLevelL11F[delta];
    }
  }

  return bit_count;
}

/*static void writeSbrNoise(BsBitStream *ptrBs, SBR_CHAN *sbrChan)*/
static void writeSbrNoise(HANDLE_BSBITSTREAM ptrBs, SBR_CHAN *sbrChan)
{
  int noise, band;
  int delta;

  for (noise=0; noise<2; noise++) {
    BsPutBit(ptrBs, sbrChan->noiseData[noise][0], 
             SI_SBR_START_NOISE_BITS_AMP_RES_3_0);
    for (band=1; band<sbrChan->numNoiseBands; band++) {
      delta = sbrChan->noiseData[noise][band] - sbrChan->noiseData[noise][band]
              + 31;
      BsPutBit(ptrBs, v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
  }
}

static int countSbrSCEdata(SBR_CHAN *sbrChan)
{
  int bit_count = 0;
  int band;

  bit_count += 1; /* reserved bits present */

  bit_count += countSbrGrid();
  bit_count += countSbrDtdf();

  for (band=0; band<sbrChan->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }

  bit_count += countSbrEnvelope(sbrChan);

  bit_count += countSbrNoise(sbrChan);

  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;

  bit_count += SI_SBR_EXTENDED_DATA_BITS;

#ifdef PARAMETRICSTEREO
  if (sbrChan->psAvailable) {
    bit_count += CountSbrPs(&sbrChan->psData);
  }
#endif

  return bit_count;
}


/*static void writeSbrSCEdata(BsBitStream *ptrBs, */
static void writeSbrSCEdata(HANDLE_BSBITSTREAM ptrBs,
                            SBR_CHAN *sbrChan)
{
  int band;

  BsPutBit(ptrBs, 0, 1); /* no reserved bits */

  writeSbrGrid(ptrBs);
  writeSbrDtdf(ptrBs);

  for (band=0; band<sbrChan->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS); 

  writeSbrEnvelope(ptrBs, sbrChan);

  writeSbrNoise(ptrBs, sbrChan);

  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);

#ifdef PARAMETRICSTEREO
  if (sbrChan->psAvailable) {
    BsPutBit(ptrBs, 1, SI_SBR_EXTENDED_DATA_BITS); /* PS in extended data */
    WriteSbrPs(&sbrChan->psData, ptrBs);
  }
  else
#endif
  {
    BsPutBit(ptrBs, 0, SI_SBR_EXTENDED_DATA_BITS); /* no extended data */
  }
}


static int countSbrCPEdata(SBR_CHAN *sbrChanL, SBR_CHAN *sbrChanR)
{
  int bit_count = 0;
  int band;

  bit_count += 1; /* flag reserved bits */

  bit_count += SI_SBR_COUPLING_BITS;

  bit_count += countSbrGrid();
  bit_count += countSbrGrid();
  bit_count += countSbrDtdf();
  bit_count += countSbrDtdf();

  for (band=0; band<sbrChanL->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }
  for (band=0; band<sbrChanR->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }
  
  bit_count += countSbrEnvelope(sbrChanL);
  bit_count += countSbrEnvelope(sbrChanR);

  bit_count += countSbrNoise(sbrChanL);
  bit_count += countSbrNoise(sbrChanR);

  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;
  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;

  bit_count += SI_SBR_EXTENDED_DATA_BITS;

  return bit_count;
}


/*static void writeSbrCPEdata(BsBitStream *ptrBs, */
static void writeSbrCPEdata(HANDLE_BSBITSTREAM ptrBs,
                            SBR_CHAN *sbrChanL, 
                            SBR_CHAN *sbrChanR)
{
  int band;

  BsPutBit(ptrBs, 0, 1); /* no reserved bits */

  BsPutBit(ptrBs, 0, SI_SBR_COUPLING_BITS);

  writeSbrGrid(ptrBs);
  writeSbrGrid(ptrBs);
  writeSbrDtdf(ptrBs);
  writeSbrDtdf(ptrBs);

  for (band=0; band<sbrChanL->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS); 
  for (band=0; band<sbrChanR->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS); 

  writeSbrEnvelope(ptrBs, sbrChanL);
  writeSbrEnvelope(ptrBs, sbrChanR);

  writeSbrNoise(ptrBs, sbrChanL);
  writeSbrNoise(ptrBs, sbrChanR);

  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);
  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);

  BsPutBit(ptrBs, 0, SI_SBR_EXTENDED_DATA_BITS); /* no extended data */
}

static int countSbrData(int nChan, SBR_CHAN *sbrChanL, SBR_CHAN *sbrChanR)
{
  int bit_count=0;

  if (nChan==1)
    bit_count += countSbrSCEdata(sbrChanL);
  else if (nChan==2)
    bit_count += countSbrCPEdata(sbrChanL, sbrChanR);

  return bit_count;
}

/*static void writeSbrData(BsBitStream *ptrBs, */
static void writeSbrData(HANDLE_BSBITSTREAM ptrBs, 
                         int nChan, 
                         SBR_CHAN *sbrChanL, 
                         SBR_CHAN *sbrChanR)
{
  if (nChan==1)
    writeSbrSCEdata(ptrBs, sbrChanL);
  else if (nChan==2)
    writeSbrCPEdata(ptrBs, sbrChanL, sbrChanR);
}


static void calcEnv(SBR_CHAN *sbrChan,
                    float *samples
#ifdef PARAMETRICSTEREO
                    , float *samplesR   /* NULL if not parametric stereo */
#endif
                    )
{
  int i, j, k, l, li, ui, count;
  float rBuffer[64], iBuffer[64], nrgBuffer[32][64];
#ifdef PARAMETRICSTEREO
  SBR_CHAN *sbrChanR = sbrChan+1;
  float lrBuffer[32][64];
  float liBuffer[32][64];
  float rrBuffer[32][64];
  float riBuffer[32][64];
#endif

  /* update timeBuffer */
  for (i=0; i<TIME_BUF_SIZE-2048; i++)
    sbrChan->timeBuffer[i] = sbrChan->timeBuffer[i+2048];
  for (i=0; i<2048; i++)
    sbrChan->timeBuffer[TIME_BUF_SIZE-2048+i] = samples[i];
#ifdef PARAMETRICSTEREO
  if (samplesR) {
    sbrChan->psAvailable = 1;

    for (i=0; i<TIME_BUF_SIZE-2048; i++)
      sbrChanR->timeBuffer[i] = sbrChanR->timeBuffer[i+2048];
    for (i=0; i<2048; i++)
      sbrChanR->timeBuffer[TIME_BUF_SIZE-2048+i] = samplesR[i];

    /* filter bank */
    for (i=0; i<32; i++) {
      CalculateEncAnaFilterbank(&sbrChan->sbrFbank,
                                sbrChan->timeBuffer + i*64,
                                lrBuffer[i], liBuffer[i]);
      CalculateEncAnaFilterbank(&sbrChanR->sbrFbank,
                                sbrChanR->timeBuffer + i*64,
                                rrBuffer[i], riBuffer[i]);
      /* downmix, calc energy */
      for (j=0; j<64; j++)
        nrgBuffer[i][j] =
          (lrBuffer[i][j]+rrBuffer[i][j])*(lrBuffer[i][j]+rrBuffer[i][j])/4 +
          (liBuffer[i][j]+riBuffer[i][j])*(liBuffer[i][j]+riBuffer[i][j])/4;
    }

    /* estimate PS data */
    SbrPsEncode(&sbrChan->psData,
                lrBuffer,
                liBuffer,
                rrBuffer,
                riBuffer);
  }
  else
#endif
  {
    /* filter bank */
    for (i=0; i<32; i++) {
      CalculateEncAnaFilterbank(&sbrChan->sbrFbank,
                                sbrChan->timeBuffer + i*64,
                                rBuffer, iBuffer);
      /* calc energy */
      for (j=0; j<64; j++)
        nrgBuffer[i][j] = rBuffer[j] * rBuffer[j] + iBuffer[j] * iBuffer[j];
    }
  }

  /* group to bands in 2 envelopes */
  for (i=0; i<2; i++) {
    for (j=0; j<sbrChan->numBands; j++) {
      float nrg=0.0f;
      li = sbrChan->freqBandTable[j];
      ui = sbrChan->freqBandTable[j+1];
      count = 16 * (ui-li);
      for (k=li; k<ui; k++)
        for (l=16*i; l<16*(i+1); l++)
          nrg += nrgBuffer[l][k];
      nrg = (float)(log(nrg/(count*64)+EPS) * ILOG2);
      nrg = (nrg < 0.0f ? 0.0f : nrg);
      sbrChan->envData[i][j] = (int)(nrg + 0.5);
    }
  }
}

/******************************************************************/

static int freqBandTableSF7[] = {
  15, 16, 17, 18, 19, 20, 22, 24, 26, 28, 30, 33, 37, 41, 45};
static int freqBandTableSF12[] = {
  22, 23, 25, 27, 29, 31, 33, 36, 39, 42, 45};

void SbrInit(int bitRate, int nChan, SBR_CHAN *sbrChan)
{
  int env, noise, band;
  int i;

  if (nChan==1) {
    sbrChan->startFreq = 7;
    sbrChan->stopFreq = 9;
    sbrChan->numBands = 14;
    sbrChan->freqBandTable = freqBandTableSF7;
    sbrChan->numNoiseBands = 3;
    sbrChan->bufOffset = 214;
  } 
  else {
    sbrChan->startFreq = 12;
    /*sbrChan->stopFreq = 9;*/
    /*sbrChan->numBands = 10;*/
	sbrChan->stopFreq = 9;
    sbrChan->numBands = 10;
    sbrChan->freqBandTable = freqBandTableSF12;
    sbrChan->numNoiseBands = 2;
    sbrChan->bufOffset = 146;
  }

  InitEncAnaFilterbank(&sbrChan->sbrFbank);

  for (i=0; i<TIME_BUF_SIZE; i++)
    sbrChan->timeBuffer[i] = 0.0f;

  for (env=0; env<MAX_ENVELOPES; env++)
    for (band=0; band<MAX_FREQ_COEFFS; band++)
      sbrChan->envData[env][band] = 0;

  for (noise=0; noise<MAX_NOISE_ENVELOPES; noise++)
    for (band=0; band<MAX_NOISE_COEFFS; band++)
      sbrChan->noiseData[noise][band] = 0;

  sbrChan->sendHdrCnt = -1;
  sbrChan->sendHdrFrame = 10; /* send new header every 10th frame */
  
#ifdef PARAMETRICSTEREO
  sbrChan->psAvailable = 0;
  SbrPsInit(&sbrChan->psData);
#endif
}

int SbrEncodeSingleChannel_BSAC(SBR_CHAN *sbrChan, 
                           float *samples
#ifdef PARAMETRICSTEREO
                           , float *samplesRight
#endif
                           )
{
  int noise, band;
  int hdrBits, dataBits, sbrBits, idBits;
  int cnt, maxCount;

  /* calculate envelopes */
  calcEnv(sbrChan, samples
#ifdef PARAMETRICSTEREO
          , samplesRight
#endif
          );

  for (noise=0; noise<2; noise++)
    for (band=0; band<sbrChan->numNoiseBands; band++)
       sbrChan->noiseData[noise][band] = 28;


  /* update counter for sending a header */
  sbrChan->sendHdrCnt++;
  if (sbrChan->sendHdrCnt==sbrChan->sendHdrFrame) 
    sbrChan->sendHdrCnt=0;

  /* count bits needed for fill element */
  if (sbrChan->sendHdrCnt==0)
    hdrBits = countSbrHdr(1);
  else
    hdrBits = 0;
  dataBits = countSbrData(1, sbrChan, NULL);

#ifdef BSACCONF
  hdrBits = countSbrHdr(2);
#endif

/* 20060107 */
  sbrBits = /*LEN_EX_TYPE + */SI_SBR_HDR_BIT + hdrBits + dataBits;
  sbrChan->totalBits = sbrBits + LEN_F_CNT;
  /* byte align */
  if (sbrChan->totalBits%8 !=0)
	sbrChan->fillBits = 8-(sbrChan->totalBits%8);
  else
	  sbrChan->fillBits = 0;
  sbrChan->totalBits += sbrChan->fillBits;

  /* 20060110 */
  cnt = (sbrChan->totalBits) / 8;
  maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

  if (cnt < maxCount) {
    sbrChan->count = cnt;
    sbrChan->esc_count = 0;
  }
  else {
    sbrChan->count = maxCount;
	/* 20060110 */
	cnt += 1;
    sbrChan->esc_count = cnt - sbrChan->count + 1;
  }

  if (sbrChan->esc_count) sbrChan->totalBits += LEN_BYTE;

  return sbrChan->totalBits;
}

int SbrEncodeChannelPair_BSAC(SBR_CHAN *sbrChanL, 
                         SBR_CHAN *sbrChanR, 
                         float *samplesL, 
                         float *samplesR)
{
  int noise, band;
  int hdrBits, dataBits, sbrBits, idBits;
  int cnt, maxCount;

  /* calculate envelopes */
#ifndef PARAMETRICSTEREO
  calcEnv(sbrChanL, samplesL);
  calcEnv(sbrChanR, samplesR);
#else
  calcEnv(sbrChanL, samplesL, NULL);
  calcEnv(sbrChanR, samplesR, NULL);
#endif

  for (noise=0; noise<2; noise++)
    for (band=0; band<sbrChanL->numNoiseBands; band++) {
       sbrChanL->noiseData[noise][band] = 28;
       sbrChanR->noiseData[noise][band] = 28;
    }


  /* update counter for sending a header */
  sbrChanL->sendHdrCnt++;
  if (sbrChanL->sendHdrCnt==sbrChanL->sendHdrFrame) 
    sbrChanL->sendHdrCnt=0;

  /* count bits needed for fill element */
  if (sbrChanL->sendHdrCnt==0)
    hdrBits = countSbrHdr(2);
  else
    hdrBits = 0;
  dataBits = countSbrData(2, sbrChanL, sbrChanR);

#ifdef BSACCONF
  hdrBits = countSbrHdr(2);
#endif
/* 20060107 */
  sbrBits = /*LEN_EX_TYPE +*/ SI_SBR_HDR_BIT + hdrBits + dataBits;
  sbrChanL->totalBits = sbrBits + LEN_F_CNT;

  /* byte align */
  if (sbrChanL->totalBits%8 !=0)
	sbrChanL->fillBits = 8-(sbrChanL->totalBits%8);
  else
	  sbrChanL->fillBits = 0;

  sbrChanL->totalBits += sbrChanL->fillBits;

  /* 20060110 */
  cnt = (sbrChanL->totalBits) / 8;
  maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

  if (cnt < maxCount) {
    sbrChanL->count = cnt;
    sbrChanL->esc_count = 0;
  }
  else {
    sbrChanL->count = maxCount;
	/* 20060110 */
	cnt += 1;
	sbrChanL->esc_count = cnt - sbrChanL->count + 1;
  }

  if (sbrChanL->esc_count) sbrChanL->totalBits += LEN_BYTE;

  return sbrChanL->totalBits;
}

int WriteSbrSCE(SBR_CHAN *sbrChan,  
                /*BsBitStream *ptrBs)*/
				HANDLE_BSBITSTREAM ptrBs)
{
  BsPutBit(ptrBs, ID_FIL, LEN_SE_ID); /* Write fill_element ID */

  BsPutBit(ptrBs, sbrChan->count, LEN_F_CNT); /* count */
  if (sbrChan->esc_count) {
    BsPutBit(ptrBs, sbrChan->esc_count, LEN_BYTE); /* esc_count */
  }

  /* extension type */
  BsPutBit(ptrBs, SI_SBR_STD_EXTENSION, LEN_EX_TYPE); /* no crc */

  if (sbrChan->sendHdrCnt==0)
    BsPutBit(ptrBs, 1, SI_SBR_HDR_BIT); /* header will follow */
  else
    BsPutBit(ptrBs, 0, SI_SBR_HDR_BIT); /* no header */

   /* header */
   if (sbrChan->sendHdrCnt==0)
     writeSbrHdr(ptrBs, sbrChan, 1);

  /* data */
  writeSbrData(ptrBs, 1, sbrChan, NULL);

  /* fill bits */
  BsPutBit(ptrBs, 0, sbrChan->fillBits);

  return sbrChan->totalBits;
}

int WriteSbrCPE(SBR_CHAN *sbrChanL,  
                SBR_CHAN *sbrChanR,
                /*BsBitStream *ptrBs)*/
				HANDLE_BSBITSTREAM ptrBs)
{
  BsPutBit(ptrBs, ID_FIL, LEN_SE_ID); /* Write fill_element ID */

  BsPutBit(ptrBs, sbrChanL->count, LEN_F_CNT); /* count */
  if (sbrChanL->esc_count) {
    BsPutBit(ptrBs, sbrChanL->esc_count, LEN_BYTE); /* esc_count */
  }

  BsPutBit(ptrBs, SI_SBR_STD_EXTENSION, LEN_EX_TYPE); /* no crc */

  if (sbrChanL->sendHdrCnt==0)
    BsPutBit(ptrBs, 1, SI_SBR_HDR_BIT); /* header will follow */
  else
    BsPutBit(ptrBs, 0, SI_SBR_HDR_BIT); /* no header */

  /* header */
  if (sbrChanL->sendHdrCnt==0)
    writeSbrHdr(ptrBs, sbrChanL, 2);

  /* data */
  writeSbrData(ptrBs, 2, sbrChanL, sbrChanR);

  /* fill bits */
  BsPutBit(ptrBs, 0, sbrChanL->fillBits);

  return sbrChanL->totalBits;
}

