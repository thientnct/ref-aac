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

Source file: Imp_sinusoid_cont_phase.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
20-Sep-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/


/**************************************************************************
 *
 * sinusoid_cont_phase module implementation
 *
 * Imp_sinusoid_cont_phase.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_sinusoid_cont_phase_h
#define __Imp_sinusoid_cont_phase_h

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

typedef struct ssc_sinusoid_cont_phaseinfo_tag
{
    SInt   DecoderLevel;
    SInt   Channels;
    SInt   Mode;
    SInt   ModeExt;
    SInt   SubframeSize;
    Double fs ;
    Double freq_rl_base ;
    Double phase_rl_base ;
}SSC_SINUSOID_CONT_PHASEINFO, *PSINUSOID_CONT_PHASEINFO ;

typedef struct ssc_sinusoid_cont_phasestate_tag
{
    SSCLOG  *pLog ;
    SSCPIPE *pPipe ;
    SInt    Channels;
    SInt    SubframeSize;
    Double  fs ;
    Double  freq_rl_base ;
    Double  phase_rl_base ;
    Double *pPrevFreqDeq ;
    Double *pCurrFreqDeq ;
    Double *pPcDeq ;
}SSC_SINUSOID_CONT_PHASESTATE, *PSINUSOID_CONT_PHASESTATE ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void         IMP_SINUSOID_CONT_PHASE_Initialise     ( void );
PSINUSOID_CONT_PHASESTATE IMP_SINUSOID_CONT_PHASE_CreateInstance ( PSINUSOID_CONT_PHASEINFO psinusoid_cont_phaseInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*    IMP_SINUSOID_CONT_PHASE_Process        ( PSINUSOID_CONT_PHASESTATE psinusoid_cont_phaseState, SSCBUNCH* pSscWorkBunch );
void         IMP_SINUSOID_CONT_PHASE_DestroyInstance( PSINUSOID_CONT_PHASESTATE* ppsinusoid_cont_phaseState );
void         IMP_SINUSOID_CONT_PHASE_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_sinusoid_cont_phase_h */

#endif



