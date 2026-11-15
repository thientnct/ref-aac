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
 *   Module              : LagFilter implementation
 *
 *   Description         : Implementation of the laguerre filter process.
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

#include "LagFilter.h"


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

SSCLAGFILTER *SSCLAGFILTER_CreateInstance( const Double *pDen, const SInt nDen, const Double Pole)
{
    SSCLAGFILTER *pFilter = NULL ;
    SInt       i;

    assert( MAX_FILTER_LENGTH >= nDen );

    pFilter = SSC_CALLOC( sizeof(SSCLAGFILTER), 1, "" );
    if ( pFilter != NULL )
    {
        pFilter->FilterLength = nDen ;
        pFilter->Pole         = Pole ;
        for ( i=0; i < MAX_FILTER_LENGTH+1; i++ )
        {
            pFilter->FilterState[i] = 0.0 ;
        }
        for ( i=0; i < nDen; i++ )
        {
            pFilter->Denominator[i] = pDen[i] ;
        }
    }
    return pFilter ;
}


void SSCLAGFILTER_Process( SSCLAGFILTER * const pFilter, const Double *pIn,
                        Double * const pOut,       const UInt nLength )
{
	Double a[2],b[2];
	Double b1;
	Double inprod;
    SInt N = nLength;
	Double *states = pFilter->states;
	Double *out = pFilter->out;
	Double lambda = pFilter->Pole;
    SInt Lp = pFilter->FilterLength;
    Double *mem = pFilter->FilterState;
    Double *alpha = pFilter->Denominator;
	const Double *x = pIn;
	Double * const r = pOut;
	
	SInt i,k;

	a[0]=1.0;
	a[1]=-1.0*lambda;

	b[0]=-1.0*lambda;
	b[1]=1.0;

	b1=sqrt(1-pow(fabs(lambda),2.0));


	for (k=0;k<Lp;k++)			/* Initialisations	*/
	{
		states[k]=0.0;
		out[k]=0.0;
	}

	for (i=0;i<N;i++)
	{
		r[i]=0.0;
	}

	for (k=0;k<Lp;k++)
	{
		states[k]=mem[k];	
	}


	for (i=1; i<N; i++)					/* Laguerre filtering */
	{
		out[0]=b1*x[i-1]+states[0];
   		states[0]=-1.0*a[1]*out[0];
		for (k=1; k<Lp; k++)
		{
			out[k]=b[0]*out[k-1]+states[k];		/* out's are the y's	*/
			states[k]=b[1]*out[k-1]-a[1]*out[k];
		}

		inprod=0.0;
		for (k=0;k<Lp;k++)
		{
			inprod=inprod+out[k]*alpha[k];		/* Compute residual entry*/
		}			
		r[i-1]=x[i]-inprod;
	}


	out[0]=b1*x[N-1]+states[0];				/* Sample N-1 (last)*/
	states[0]=-1.0*a[1]*out[0];
	for (k=1; k<Lp; k++)
	{
		out[k]=b[0]*out[k-1]+states[k];
		states[k]=b[1]*out[k-1]-a[1]*out[k];
	}

	inprod=0.0;
	for (k=0;k<Lp;k++)
	{
		inprod=inprod+out[k]*alpha[k];			/* Compute residual entry (last sample)*/
	}
	r[N-1]=x[N]-inprod;



	for (k=0;k<Lp;k++)					/* Copy states */
	{							
		mem[k]=states[k];
	}
	
}


void SSCLAGFILTER_SetDenominator( SSCLAGFILTER * const pFilter, 
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


void SSCLAGFILTER_DestroyInstance( SSCLAGFILTER *pFilter )
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
