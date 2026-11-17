/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by
Takashi Koike (Sony Corporation)
Yasuhiro Toguri (Sony Corporation)

in the course of development of the MPEG-2 NBC/MPEG-4 System/MPEG-4
Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
software module is an implementation of a part of one or more MPEG-2
NBC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
MPEG-2 NBC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
gives users of the MPEG-2 NBC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 NBC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 NBC/MPEG-4
System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
notice must be included in all copies or derivative works.

Copyright (C) 1996.
*******************************************************************/

#include	<stdio.h>
#include	"tf_main.h"
#include	"bitstream.h"

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
#define PQFDELAY	    44  /* YT 970615 for pqf delay*/ 	
#define SPECTRAL_SCALING	1.0
#define	NPEPARTS		64	/* Num of PreEcho Inhibition Parts */ 
#define	SHORT_WIN_IN_LONG	8
#define	mylog2(x)		(mylog10(x)/mylog10(2))
#define	mylog10(x)		((((double)x)>1e-20) ? \
				log10((double)(x)) : log10((double)1e-20))
#define	npow2(x)		(1L << (x))			/* 2^x */


/* Gain Control Information */
typedef	struct	{
	int	natks;				/* Number of Attacks */
	int	a_loc[npow2(NATKSBITS)-1];	/* Location of Attack */
	int	a_idgain[npow2(NATKSBITS)-1];	/* ID of Gain Control Coef */
	double	peak;			/* Peak Abso Values in Latter Half */
}	GAINC;

typedef enum {
    GC_NON_PRESENT, /* gain_control_data_present off */
    GC_PRESENT
} GC_DATA_SWITCH ;


#ifndef MONO_CHAN
#define	MONO_CHAN		0
#endif
#ifndef	MAX_TIME_CHANNELS
#define	MAX_TIME_CHANNELS	1
#endif

/* enum	window_sequence {EIGHT_SHORT_SEQUENCE = 5, ONLY_LONG_SEQUENCE = 0, LONG_START_SEQUENCE = 8, LONG_STOP_SEQUENCE = 9}; */


/**
 * function prototypes
 */
#ifdef __cplusplus
extern "C" {
#endif

void	son_pqf_main(
		/* input */
		double	timeSignalCh[],
		int	block_size_samples,
		int	ch,
		/* output */
		double	*bandSignalCh[NBANDS]
	);

void	son_gc_detect(
		/* input */
		double	*bandSignalChForGCAnalysis[NBANDS],
		int	block_size_samples,
		int	window_sequence,
		int	ch,
		/* output */
		GAINC	*gainInfoCh[NBANDS]
	);

void	son_gc_modifier(
		/* input */
		double	*bandSignalChForGCAnalysis[NBANDS],
		GAINC	*gainInfoCh[NBANDS],
		int	block_size_samples,
		int	window_sequence,
		int	ch,
		/* output */
		double	*gainModifiedBandSignalCh[NBANDS]
	);

void	son_mdct_main(
		/* input */
		double	*gainModifiedBandSignalCh[NBANDS],
		int	window_sequence,
		double	freqSignalCh[]
	);

void	son_imdct_main(
		/* input */
		double	freqSignalCh[],
		int	window_sequence,
		/* output */
		double	timeSignalChWithGCandOverlapping[]
	);

void	son_gc_compensate(
		/* input */
		double	timeSignalChWithGCandOverlapping[],
		GAINC	*gainInfoCh[],
		int	block_size_samples,
		int	window_sequence,
		int	ch,
		/* input/output */
		double	*gcOverlapBufferCh[],
		/* output */
		double	*bandSignalCh[]
	);

void	son_ipqf_main(
		/* input */
		double	*bandSignalCh[NBANDS],
		int	block_size_samples,
		int	ch,
		/* output */
		double	timeSignalCh[]
	);

int	son_gc_pack(
		/* input/output */
		BsBitStream	*fixed_stream,
		/* input */
		int	window_sequence,
		int	max_band,
		GAINC	*gcDataCh[]
	);

int	son_gc_unpack(
		/* input */
		BsBitStream	*fixed_stream,
		int	window_sequence,
		/* output */
		int	*max_band,
		GAINC	*gcDataCh[]
	);

void    son_gc_arrangeSpecEnc(
        /* input */
        double  *freqSignalChForPP,
        int block_size_samples,
        int window_sequence,
        /* output */
        double  *freqSignalCh
    );

void    son_gc_detect_reset(
        /* input */
        double  *bandSignalChForGCAnalysis[NBANDS],
        int block_size_samples,
        int window_sequence,
        int ch,
        /* output */
        GAINC   *gainInfoCh[NBANDS]
    );



#ifdef __cplusplus
}
#endif
