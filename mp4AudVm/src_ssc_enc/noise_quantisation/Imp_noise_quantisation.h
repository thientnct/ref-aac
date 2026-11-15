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

Source file: Imp_noise_quantisation.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001     AP/JD   Initial version
20-Sep-2002     NT      RM2 change
************************************************************************/

/**************************************************************************
 *
 * noise_quantisation module implementation
 *
 * Imp_noise_quantisation.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_noise_quantisation_h
#define __Imp_noise_quantisation_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct ssc_noise_quantisationinfo_tag
{
    SInt   DecoderLevel;
    SInt   Channels;
    SInt   Mode;
    SInt   ModeExt;
    SInt   SubframeSize;
    SInt   EnvOrder;
    Double dLar_Stepsize;
    SInt   nAbs_max_lar ;
    SInt   nRlGain_min;
    SInt   nRlGain_max;
    SInt   nRlGain_offset;
    Double dLsf_Stepsize;
    SInt   nLsf_min_val;
    SInt   nLsf_max_val;
}SSC_NOISE_QUANTISATIONINFO, *PNOISE_QUANTISATIONINFO ;

typedef struct ssc_noise_quantisationstate_tag
{
    SSCLOG *pLog;
    SSCPIPE *pPipe;
    Double dLar_Stepsize;
    SInt   nAbs_max_lar;
    SInt   nRlGain_min;
    SInt   nRlGain_max;
    SInt   nRlGain_offset;
    Double dLsf_Stepsize;
    SInt   nLsf_min_val;
    SInt   nLsf_max_val;
    SInt   nChannels;
}SSC_NOISE_QUANTISATIONSTATE, *PNOISE_QUANTISATIONSTATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void         IMP_NOISE_QUANTISATION_Initialise     ( void );
PNOISE_QUANTISATIONSTATE IMP_NOISE_QUANTISATION_CreateInstance ( PNOISE_QUANTISATIONINFO pnoise_quantisationInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*    IMP_NOISE_QUANTISATION_Process        ( PNOISE_QUANTISATIONSTATE pNQS, SSCBUNCH* pSscBunch );
void         IMP_NOISE_QUANTISATION_DestroyInstance( PNOISE_QUANTISATIONSTATE* ppnoise_quantisationState );
void         IMP_NOISE_QUANTISATION_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_noise_quantisation_h */

#endif



