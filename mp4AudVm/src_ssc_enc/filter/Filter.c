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

Source file: Filter.c

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

/******************************************************************************
 *
 *   Module              : Filter implementation
 *
 *   Description         : Implementation of the filter process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_Filter_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include <assert.h>

#include "PlatformTypes.h"
#include "SSC_Alloc.h"

#include "Filter.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

SSCFILTER *SSCFILTER_CreateInstance(const Double *pNum, const SInt nNum, const Double *pDen, const SInt nDen)
{
    SSCFILTER *pFilter = NULL ;
    SInt       i, maxorder ;

    if ( nNum > nDen )
    {
        maxorder = nNum ;   /* b */
    }
    else
    {
        maxorder = nDen ;   /* a */
    }

    assert( MAX_FILTER_LENGTH >= maxorder );

    pFilter = SSC_CALLOC( sizeof(SSCFILTER), 1, "" );
    if ( pFilter != NULL )
    {
        pFilter->FilterLength = maxorder ;
        for ( i=0; i<MAX_FILTER_LENGTH+1; i++ )
        {
            pFilter->FilterState[i] = 0.0 ;
        }
        for ( i=0; i<nNum; i++ )
        {
            pFilter->Numerator[i] = pNum[i] ;
        }
        for (    ; i<MAX_FILTER_LENGTH+1; i++ )
        {
            pFilter->Numerator[i] = 0.0 ;
        }
        for ( i=0; i<nDen; i++ )
        {
            pFilter->Denominator[i] = pDen[i] ;
        }
        for (    ; i<MAX_FILTER_LENGTH+1; i++ )
        {
            pFilter->Denominator[i] = 0.0 ;
        }
    }
    return pFilter ;
}


void SSCFILTER_Process( SSCFILTER * const pFilter, const Double *pIn,
                        Double * const pOut,       const UInt nLength )
{
	UInt   i, j;
    Double x, y;

    for (i = 0; i < nLength; i++)
    {
        x      = pIn[i] ;
        y      = x*pFilter->Numerator[0] + pFilter->FilterState[0];
        pOut[i] = y;

        for (j = 0; j < pFilter->FilterLength-1; j++)
        {
            pFilter->FilterState[j] = pFilter->FilterState[j+1] +
                                      pFilter->Numerator[j+1]*x - pFilter->Denominator[j+1]*y;
        }
    }
}

void SSCFILTER_SetNumerator( SSCFILTER * const pFilter, 
                             const Double *pNum, 
                             const UInt nNumLength )
{
    UInt i;
    assert(pFilter);
    for ( i=0; i<nNumLength; i++ )
    {
        pFilter->Numerator[i] = pNum[i] ;
    }
    for (    ; i<MAX_FILTER_LENGTH+1; i++ )
    {
        pFilter->Numerator[i] = 0.0 ;
    }
}


void SSCFILTER_SetDenominator( SSCFILTER * const pFilter, 
                               const Double *pDen, 
                               const UInt nDenLength )
{
    UInt i;
    assert(pFilter);
    for ( i=0; i<nDenLength; i++ )
    {
        pFilter->Denominator[i] = pDen[i] ;
    }
    for (    ; i<MAX_FILTER_LENGTH+1; i++ )
    {
        pFilter->Denominator[i] = 0.0 ;
    }
}


void SSCFILTER_DestroyInstance( SSCFILTER *pFilter )
{
    if ( pFilter != NULL )
    {
        SSC_FREE( pFilter, "" );
        pFilter = NULL;
    }
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/
