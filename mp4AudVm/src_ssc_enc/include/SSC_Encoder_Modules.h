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

Source file: SSC_Encoder_Modules.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/




/*===========================================================================
 *
 *  Module      : SSC_Encoder_Modules
 *
 *  File        : SSC_Encoder_Modules.h
 *
 *  Description : This header file defines the SSC encoder modules, used for
 *                determining the source module of an error code.
 *
 ============================================================================*/

#ifndef _SSC_ENCODER_MODULES_H
#define _SSC_ENCODER_MODULES_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
#define SSCENC_AUDIO_READ_MODULE                1
#define SSCENC_BITSTREAM_MODULE                 2
#define SSCENC_FILTER_MODULE                    3
#define SSCENC_TRAN_DETECTION_MODULE            4
#define SSCENC_STEREO_ANALYSIS_MODULE           5
#define SSCENC_TRAN_ANALYSIS_MODULE             6
#define SSCENC_TRAN_SYNTHESIS_MODULE            7
#define SSCENC_TRAN_QUANTISATION_MODULE         8
#define SSCENC_SIN_ANALYSIS_MODULE              9
#define SSCENC_SIN_SYNTHESIS_MODULE            10
#define SSCENC_SIN_PSYCHO_MODULE               11
#define SSCENC_SIN_REMOVE_MODULE               12
#define SSCENC_SIN_TRACKING_MODULE             13
#define SSCENC_STEREO_QUANTISATION_MODULE      14
#define SSCENC_SIN_QUANTISATION_MODULE         15
#define SSCENC_SIN_CONTINUOUS_PHASE_MODULE     16
#define SSCENC_NOISE_ANALYSIS_MODULE           17
#define SSCENC_NOISE_TEMP_ENV_MODULE           18
#define SSCENC_NOISE_QUANTISATION_MODULE       19

#ifdef __cplusplus
}
#endif

#endif /* _SSC_ENCODER_MODULES_H */

