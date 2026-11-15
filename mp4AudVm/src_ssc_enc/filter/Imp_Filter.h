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

Source file: Imp_Filter.h

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
 * Filter module implementation
 *
 * Imp_Filter.h
 *
 * contains all prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_Filter_h
#define __Imp_Filter_h

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

typedef struct ssc_filterinfo_tag
{
    SInt DecoderLevel;
    SInt Channels;
    SInt SubframeSize;
    SInt fs ;
    SInt CutOffFreq ;
}SSC_FILTER_INFO, *PFILTERINFO ;

typedef struct ssc_filterstate_tag
{
    SSCLOG*    pLog ;
    SSCPIPE*   pPipe ;
    SInt       Channels ;
    SSCFILTER* pFilter[SSC_MAX_CHANNELS] ;
}SSC_FILTER_STATE, *PFILTERSTATE ;

typedef struct filtersetup_tag
{
    SInt   fs ;
    SInt   CutOffFreq ;
    SInt   nCntNum ;
    Double Numerator[2] ;
    SInt   nCntDen ;
    Double Denominator[2] ;
}FILTERSETUP ;


/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void         IMP_FILTER_Initialise     ( void );
PFILTERSTATE IMP_FILTER_CreateInstance ( PFILTERINFO pFilterInfo, Char* szLogFile, SInt nTraceLevel );
SSCBUNCH*    IMP_FILTER_Process        ( PFILTERSTATE pFilterState, SSCBUNCH* pSscWorkBunch );
void         IMP_FILTER_DestroyInstance( PFILTERSTATE* ppFilterState );
void         IMP_FILTER_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_Filter_h */

#endif



