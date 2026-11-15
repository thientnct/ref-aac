/*
  "This software module was originally developed by
    
  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation) 

  and edited by

  Yoshiaki Oikawa (Sony Corporation),
  Mitsuyuki Hatanaka (Sony Corporation)

  in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
  Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
  software module is an implementation of a part of one or more MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
  MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
  gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards free license to this software module or modifications
  thereof for use in hardware or software products claiming conformance
  to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
  standards. Those intending to use this software module in hardware or
  software products are advised that this use may infringe existing
  patents. The original developer of this software module and his/her
  company, the subsequent editors and their companies, and ISO/IEC have
  no liability for use of this software module or modifications thereof
  in an implementation. Copyright is not released for non MPEG-2
  AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
  original developer retains full right to use the code for his/her own
  purpose, assign or donate the code to a third party and to inhibit
  third party from using the code for non MPEG-2 AAC/MPEG-4
  System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
  notice must be included in all copies or derivative works."
  Copyright (C) 1996.
*/

/* This header file is for the gain control tool. The common include files,
defines and typedefs are declared. */

#ifdef	SSR

#include	<stdio.h>

#define NBANDSBITS		2
#define NATKSBITS		3
#define IDGAINBITS		4
#define ATKLOCBITS		5
#define ATKLOCBITS_START_A	4
#define ATKLOCBITS_START_B	2
#define ATKLOCBITS_SHORT	2
#define ATKLOCBITS_STOP_A	4
#define ATKLOCBITS_STOP_B	5
#define	NBANDS			4
#define NPQFTAPS		96
#define	NPEPARTS		64	/* Num of PreEcho Inhibition Parts */ 

#define	SHORT_WIN_IN_LONG	8
#define	GC_BLOCKS_LONG	1
#define	GC_BLOCKS_SHORT	SHORT_WIN_IN_LONG
#define	GC_BLOCKS_START	2
#define	GC_BLOCKS_STOP	2

#define	mylog2(x)		(mylog10(x)/mylog10(2))
#define	mylog10(x)		((((double)x)>1e-20) ? \
				log10((double)(x)) : log10((double)1e-20))
#define	npow2(x)		(1L << (x))			/* 2^x */

typedef struct {
        int lev;
        int loc;
} GAIN_DATA;
 
typedef struct {
        int      num_gain_data;
        GAIN_DATA gData[7];
} GAIN_INF;

typedef struct {
        GAIN_INF gBlock[8];
} GAIN_BLOCK;

/* definition of filterbank control & info structure */

typedef struct {
        int     max_band;
        GAIN_BLOCK gInfo[4];
} GAIN_CONTROL_DATA;

enum	window_sequence {
	ONLY_LONG_SEQUENCE = 0x0,
	LONG_START_SEQUENCE,
	EIGHT_SHORT_SEQUENCE,
	LONG_STOP_SEQUENCE
	};

#endif
/*******************************************************************
                        End of gc.h
*******************************************************************/

