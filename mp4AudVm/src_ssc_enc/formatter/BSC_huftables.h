/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright © 2001.

Source file: BSC_huftables.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
05-Dec-2001 JD      m7724 implemented
25 Jul 2002 TM      m8540 improved noise coding + 8 sf/frame
20 Sep 2002 NT      RM2 change
************************************************************************/

/**************************************************************************
 *
 * Bit Stream Conversion tools
 *
 * bsc_struct.h
 *
 * contains all structures of the tools
 *
 **************************************************************************/

#ifndef __bsc_huftables_h
#define __bsc_huftables_h

#ifdef __cplusplus
extern "C" {
#endif

#include "HuffmanEncoder.h"

extern HUFFMAN_CODE_STRUCT huf_s_cont;
extern HUFFMAN_CODE_STRUCT huf_s_nrof_births;

extern HUFFMAN_CODE_STRUCT hufNoiRelNdgE;
extern HUFFMAN_CODE_STRUCT hufNoiRelLsfE;
extern HUFFMAN_CODE_STRUCT hufNoiRelLag;

extern HUFFMAN_CODE_STRUCT hufAmpBirthAbs;
extern HUFFMAN_CODE_STRUCT hufAmpBirthRel;
extern HUFFMAN_CODE_STRUCT hufAmpContAbs;

extern HUFFMAN_CODE_STRUCT hufAmpContRel_0;
extern HUFFMAN_CODE_STRUCT hufAmpContRel_1;
extern HUFFMAN_CODE_STRUCT hufAmpContRel_2;
extern HUFFMAN_CODE_STRUCT hufAmpContRel_3;

extern HUFFMAN_CODE_STRUCT hufFreqBirthAbs;
extern HUFFMAN_CODE_STRUCT hufFreqBirthRel;
extern HUFFMAN_CODE_STRUCT hufFreqContAbs;

extern HUFFMAN_CODE_STRUCT hufFreqContRel_0;
extern HUFFMAN_CODE_STRUCT hufFreqContRel_1;
extern HUFFMAN_CODE_STRUCT hufFreqContRel_2;
extern HUFFMAN_CODE_STRUCT hufFreqContRel_3;

extern HUFFMAN_CODE_STRUCT OCS_HUF_iidDeltaFreqFineHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_iidDeltaTimeFineHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_iidDeltaFreqCoarseHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_iidDeltaTimeCoarseHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_iccDeltaFreqHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_iccDeltaTimeHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_ipdDeltaFreqHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_ipdDeltaTimeHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_opdDeltaFreqHET;
extern HUFFMAN_CODE_STRUCT OCS_HUF_opdDeltaTimeHET;

extern HUFFMAN_CODE_STRUCT hufAdpcmGrid;

#ifdef __cplusplus
}
#endif

#endif
