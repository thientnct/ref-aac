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

Source file: Env_Pipe.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
11-Oct-2001	AP/JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : Envelope pipe implementation
 *
 *   Description         : Implementation of the envelope process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with ENVPIPE_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"
#include "SSC_System.h"
#include "Env_Pipe.h"

#include "cexcept.h"

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
static __inline Double LOCAL_Max( Double d1, Double d2);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

PENVPIPE ENVPIPE_CreateInstance( SInt nChannels, SInt sf_size, Double dMin_Envelope, Double dDecay, SInt pipe_len )
{
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION *the_exception_context = &tec;
    PENVPIPE       pEnvPipe = NULL ;
    SSC_ERROR      Error    = SSC_ERROR_OK ;

    Try
    {
        SInt i;

        pEnvPipe                  = SSC_CALLOC( sizeof(ENVPIPE), 1, "" );
        pEnvPipe->pipe_len        = pipe_len ;
        pEnvPipe->nr_of_channels  = nChannels ;
        pEnvPipe->sf_size         = sf_size ;
        pEnvPipe->dMin_Envelope   = dMin_Envelope ;
        pEnvPipe->Envelope_Decay  = dDecay ;
        pEnvPipe->pdPrevEnvValue  = SSC_CALLOC( nChannels * sizeof(Double), 1, "" );
        for (i=0; i < nChannels; i++ )
        {
            pEnvPipe->pdPrevEnvValue[i] = dMin_Envelope ;
        }
        pEnvPipe->ppEnvBunch      = SSC_CALLOC( pipe_len * sizeof(ENVBUNCH *), 1, "" );
        for ( i=0; i < pipe_len; i++ )
        {
            pEnvPipe->ppEnvBunch[i]                = SSC_CALLOC( sizeof( ENVBUNCH ), 1, "" );
            pEnvPipe->ppEnvBunch[i]->pdEnvelope    = SSC_CALLOC( sf_size * nChannels * sizeof(Double), 1, "" );
        }
    }
    Catch(Error)
    {
        ENVPIPE_DestroyInstance( pEnvPipe );
        pEnvPipe = NULL ;
    }
    return pEnvPipe ;
}


void ENVPIPE_Shift( PENVPIPE pEnvPipe )
{
    if ( pEnvPipe != NULL )
    {
        SInt nIdx ;

        ENVBUNCH *pEnvBunchTemp = pEnvPipe->ppEnvBunch[0] ;
        for ( nIdx = 1; nIdx < pEnvPipe->pipe_len; nIdx++ )
        {
            pEnvPipe->ppEnvBunch[nIdx-1] = pEnvPipe->ppEnvBunch[nIdx] ;
        }
        pEnvPipe->ppEnvBunch[pEnvPipe->pipe_len-1] = pEnvBunchTemp ;
    }
}


void ENVPIPE_DestroyInstance( PENVPIPE pEnvPipe )
{
    if ( pEnvPipe != NULL )
    {
        SInt i;

        for ( i=0; i < pEnvPipe->pipe_len; i++ )
        {
            if ( pEnvPipe->ppEnvBunch[i] != NULL )
            {
                SSC_FREE( pEnvPipe->ppEnvBunch[i]->pdEnvelope, "" );
                SSC_FREE( pEnvPipe->ppEnvBunch[i], "" );
            }
        }
        SSC_FREE( pEnvPipe->pdPrevEnvValue, "" );
        SSC_FREE( pEnvPipe->ppEnvBunch, "" );
        SSC_FREE( pEnvPipe, "" );
    }
}


void ENVPIPE_CalcEnvelope ( PENVPIPE pEnvPipe, SInt nCh, Double *pSignal )
{
    SSC_EXCEPTION  tec;
    SSC_ERROR      Error = SSC_ERROR_OK ;
    SSC_EXCEPTION *the_exception_context = &tec;

    /* get last EnvBunch */

    Try
    {
        SInt      n ;
        ENVBUNCH *pEnvBunch      = pEnvPipe->ppEnvBunch[pEnvPipe->pipe_len-1];
        Double   *pdPrevEnvValue = pEnvPipe->pdPrevEnvValue ;
        SInt      nSfSize        = pEnvPipe->sf_size ;
        Double   *pDst           = &pEnvBunch->pdEnvelope[nCh*nSfSize];

        /* copy signal to envelope buffer */
        memcpy( pDst, pSignal, sizeof(*pSignal) * nSfSize );

        /* calculate envelope */
        for ( n=0; n<nSfSize; n++ )
        {
            pDst[n] = fabs( pDst[n] );
            if( pDst[n] < pEnvPipe->dMin_Envelope )
            {
                pDst[n] = pEnvPipe->dMin_Envelope;
            }
        }
        /* apply decay over all samples */
        pDst[0] = LOCAL_Max( pDst[0], pEnvPipe->Envelope_Decay * pdPrevEnvValue[nCh] );
        for ( n=1; n<nSfSize; n++ )
        {
            pDst[n] = LOCAL_Max( pDst[n], pEnvPipe->Envelope_Decay * pDst[n-1] );
        }
        pdPrevEnvValue[nCh] = pDst[nSfSize-1] ;
    }
    Catch(Error)
    {
        fprintf( stdout, "Transient analysis error\n" );
    }
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

static __inline Double LOCAL_Max( Double d1, Double d2)
{
    if( d1 > d2 )
    {
        return d1 ;
    }
    else
    {
        return d2 ;
    }
}
