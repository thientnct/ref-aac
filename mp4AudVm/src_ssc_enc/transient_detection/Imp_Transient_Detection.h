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

Source file: Imp_Transient_Detection.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
************************************************************************/

/**************************************************************************
 *
 * TransientDetection module implementation
 *
 * Imp_TransientDetection.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_TransientDetection_h
#define __Imp_TransientDetection_h

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

typedef struct ssc_transientdetectioninfo_tag
{
    SInt     DecoderLevel;
    SInt     Channels;
    SInt     SubframeSize;
    Double   Tau ;
    Double   Threshold ;
    Double   yPrev[SSC_MAX_CHANNELS];
    Double   uPrev[SSC_MAX_CHANNELS];
    SInt     attack_duration ;
    SInt     attack_count_down[SSC_MAX_CHANNELS] ;
}SSC_TRANSIENTDETECTION_INFO, *PTRANSIENTDETECTIONINFO ;

typedef struct ssc_transientdetectionstate_tag
{
    SSCLOG  *pLog ;
    SSCPIPE *pPipe ;
    SInt     nChannels ;
    Double   Tau ;
    Double   Threshold ;
    Double   yPrev[SSC_MAX_CHANNELS];
    Double   uPrev[SSC_MAX_CHANNELS];
    SInt     attack_duration ;
    SInt     attack_count_down[SSC_MAX_CHANNELS] ;
}SSC_TRANSIENTDETECTION_STATE, *PTRANSIENTDETECTIONSTATE ;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

void                     IMP_TRANSIENTDETECTION_Initialise     ( void );
PTRANSIENTDETECTIONSTATE IMP_TRANSIENTDETECTION_CreateInstance ( PTRANSIENTDETECTIONINFO pTDI, Char *szLogFile, SInt nTraceLevel );
SSCBUNCH *               IMP_TRANSIENTDETECTION_Process        ( PTRANSIENTDETECTIONSTATE pTDS, SSCBUNCH *pSscBunch );
void                     IMP_TRANSIENTDETECTION_DestroyInstance( PTRANSIENTDETECTIONSTATE *ppTDS );
void                     IMP_TRANSIENTDETECTION_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_TransientDetection_h */

#endif



