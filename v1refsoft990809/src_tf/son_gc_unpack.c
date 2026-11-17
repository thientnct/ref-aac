/*******************************************************************************
MPEG-4 Audio VM
Unpack gain_control_data()

This software module was originally developed by

Takashi Koike (Sony Corporation)

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
*******************************************************************************/

#include <stdio.h>

#include "buffersHandle.h"       /* handler, defines, enums */
#include "resilienceHandle.h"    /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "bitstreamStruct.h"     /* structs */

#include "sony_local.h"
#include "bitstream.h"

/* local function(s) */
int	son_BsSkipBit (BsBitStream *, int);

/*******************************************************************************
	Unpack gain_control_data
*******************************************************************************/
int	son_gc_unpack(
	BsBitStream	*gc_stream,
	int	window_sequence,
	int	*max_band,
	GAINC	*gcDataCh[]
	)
{
	int	bd, wd, ad;
	int	loc;

	loc = gc_stream->currentBit;
	if (gc_stream->buffer[0]->numBit == 0) {
		return 0;
	}
	BsGetBit(gc_stream, (unsigned long *)max_band, 2);

	/* 		 0 < max_band <= 3 */

#if 0
	for (bd = 0; bd < NBANDS; bd++) {
		for (wd = 0; wd < 8; wd++) {
			gcDataCh[bd][wd].natks = 0;
		}
	}
#endif
	switch (window_sequence) {
	case ONLY_LONG_SEQUENCE:
		for (bd = 1; bd <= *max_band; bd++) {
			for (wd = 0; wd < 1; wd++) {
				BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_idgain[ad], 4);
					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 5);
				}
			}
		}
		break;
	case LONG_START_SEQUENCE:
		for (bd = 1; bd <= *max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_idgain[ad], 4);
					if (wd == 0) {
						BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 4);
					}
					else {
						BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 2);
					}
				}
			}
		}
		break;
	case EIGHT_SHORT_SEQUENCE:
		for (bd = 1; bd <= *max_band; bd++) {
			for (wd = 0; wd < 8; wd++) {
				BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_idgain[ad], 4);

					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 2);
				}
			}
		}
		break;
	case LONG_STOP_SEQUENCE:
		for (bd = 1; bd <= *max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].natks, 3);
				for (ad = 0; ad < gcDataCh[bd][wd].natks; ad++) {
					BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_idgain[ad], 4);
					if (wd == 0) {
						BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 4);
					}
					else {
						BsGetBit(gc_stream, (unsigned long *)&gcDataCh[bd][wd].a_loc[ad], 5);
					}
				}
			}
		}
		break;
	default:
		return	-1;
	}
	return	gc_stream->currentBit - loc;
}

/*******************************************************************************
	get buffersize for gain_control_data
*******************************************************************************/
int	son_gc_getBufSize(
	BsBitStream	*fixed_stream,
	int		window_sequence
	)
{
	int	tmp_read;
	int	bd, wd, ad;
	int	max_band, natks, bufSize;
	BsBitStream	*tmp_stream;
	BsBitBuffer	*tmpBitBuf;

	tmp_read = BsBufferNumBit(fixed_stream->buffer[0])
			- fixed_stream->currentBit;

	tmpBitBuf = BsAllocBuffer(tmp_read);
	tmp_stream = BsOpenBufferWrite(tmpBitBuf);
	BsGetBufferAhead(fixed_stream, tmpBitBuf, tmp_read);

	BsGetBit(tmp_stream, (unsigned long *)&max_band, 2);
						/* 	// 0 < max_band <= 3 */

	switch (window_sequence) {
	case ONLY_LONG_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 1; wd++) {
				BsGetBit(tmp_stream, (unsigned long *)&natks, 3);
				for (ad = 0; ad < natks; ad++) {
					son_BsSkipBit(tmp_stream, (4+5));
				}
			}
		}
		break;
	case LONG_START_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsGetBit(tmp_stream, (unsigned long *)&natks, 3);
				for (ad = 0; ad < natks; ad++) {
					son_BsSkipBit(tmp_stream, 4);
					if (wd == 0) {
						son_BsSkipBit(tmp_stream, 4);
					}
					else {
						son_BsSkipBit(tmp_stream, 2);
					}
				}
			}
		}
		break;
	case EIGHT_SHORT_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 8; wd++) {
				BsGetBit(tmp_stream, (unsigned long *)&natks, 3);
				for (ad = 0; ad < natks; ad++) {
					son_BsSkipBit(tmp_stream, (4+2));
				}
			}
		}
		break;
	case LONG_STOP_SEQUENCE:
		for (bd = 1; bd <= max_band; bd++) {
			for (wd = 0; wd < 2; wd++) {
				BsGetBit(tmp_stream, (unsigned long *)&natks, 3);
				for (ad = 0; ad < natks; ad++) {
					son_BsSkipBit(tmp_stream, 4);
					if (wd == 0) {
						son_BsSkipBit(tmp_stream, 4);
					}
					else {
						son_BsSkipBit(tmp_stream, 5);
					}
				}
			}
		}
		break;
	default:
		return	-1;
	}
	bufSize = tmp_stream->currentBit;

	BsClose(tmp_stream);
	BsFreeBuffer(tmpBitBuf);

	return	bufSize;
}

/*******************************************************************************
	skip read pointer of bit stream
*******************************************************************************/
/* BsSkipBit() */
/* Skip read pointer of bit stream */
int	son_BsSkipBit (
	BsBitStream	*stream,	/* in: bit stream */
	int		numBit		/* in: num bits to skip */
	)				/* returns: */
					/*  0 = OK  1=error */
{
#if 1
	unsigned long	ltmp;
	BsGetBit(stream, &ltmp, numBit);
	return 0;
#endif

#if 0
	if (BSdebugLevel >= 3)
		printf("son_BsSkipBit: %s  id=%ld  numBit=%d  curBit=%ld\n",
		(stream->file!=NULL)?"file":"buffer",
		stream->streamId,numBit,stream->currentBit);

	if (stream->mode != 0)
		CommonExit(1,"son_BsSkipBit: stream not in read mode");
	if (numBit<0 || numBit>LONG_NUMBIT)
		CommonExit(1,"son_BsSkipBit: number of bits out of range (%d)",numBit);

	if (BsCheckRead(stream, numBit)) {
		CommonWarning("son_BsSkipBit: not enough bits left in stream");
		return 1;
	}
	else {
		stream->currentBit += numBit;
		return 0;
	}
#endif
}
