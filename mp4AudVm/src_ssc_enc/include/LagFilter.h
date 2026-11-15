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

Source file: Filter.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
05-Oct-2002	NT	    SetNumerator, SetDenominator added
************************************************************************/

/**************************************************************************
 *
 * SSC_Filter.h structure type definition file
 *
 * contains all structures of the filter implementation
 *
 **************************************************************************/
#ifndef __LagFilter_h
#define __LagFilter_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define MAX_FILTER_LENGTH   24

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct ssclagfilter_tag
{
    UInt   FilterLength;
    Double Pole;
    Double FilterState[MAX_FILTER_LENGTH+1];
    Double Denominator[MAX_FILTER_LENGTH+1];
	Double states[MAX_FILTER_LENGTH];
	Double out[MAX_FILTER_LENGTH];
}SSCLAGFILTER;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/

/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

SSCLAGFILTER *SSCLAGFILTER_CreateInstance( const Double *pDen, const SInt nDen, const Double Pole);
void          SSCLAGFILTER_Process( SSCLAGFILTER * const pFilter, const Double *pIn, Double * const pOut,  const UInt nLength );
void          SSCLAGFILTER_SetDenominator( SSCLAGFILTER * const pFilter, const Double *pDen, const UInt nDenLength );
void          SSCLAGFILTER_DestroyInstance( SSCLAGFILTER *pFilter );

#ifdef __cplusplus
}
#endif

#endif /* ifdef __LagFilter_H */
