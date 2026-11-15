/***********************************************************************
MPEG-4 Audio RM Module
Parametric codec

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

Source file: SSC_fft_ff.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
10-Sep-2002 NT      SSC_FFT_FF_GetPRe function added
************************************************************************/

/******************************************************************************
 *
 *   Module              : SSC FFT implementation
 *
 *   Description         : 
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_FFT_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>

#include "SSC_fft_ff.h"

#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"

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

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

void SSC_FFT_FF_Initialise( void )
{
}

void SSC_FFT_FF_Free ( void )
{
}

SSC_FFT_FF_STATE* SSC_FFT_FF_CreateInstance ( SSCLOG* const pLog, const SInt sizeFFT, SSC_FFT_FF_TYPE type)
{
    SSC_FFT_FF_STATE* pms = NULL ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_FFT_FF_STATE), 1, "FFT Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for FFT instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_FFT_FF_Initialise();

        /* get the log object from the caller for logging */
        pms->pLog    = pLog;
        pms->sizeFFT = sizeFFT;
        pms->type    = type;

        switch(type)
        {
            case SSC_FFT_FF_DOUBLE:
            case SSC_FFT_FF_FLOAT:
                break;
            default:
                SSCLOG_TRACE5( pLog,"Error: Undefined FFT type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }
        pms->pFft = SSC_FFT_CreateInstance( sizeFFT );
        if( pms->pFft == NULL )
        {
            SSCLOG_TRACE5( pLog,"Error: Unable to alloc requested memory for fft\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pms->pRData = SSC_FFT_GetPr( pms->pFft );
        pms->pIData = SSC_FFT_GetPi( pms->pFft );
    }
    Catch(Error)
    {
        pms = SSC_FFT_FF_DestroyInstance( pms );
    }

    return pms;
}

SSC_FFT_FF_STATE* SSC_FFT_FF_DestroyInstance( SSC_FFT_FF_STATE* const pms )
{
    if( pms != NULL )
    {
        SSC_FFT_DestroyInstance( &pms->pFft );
        SSC_FFT_FF_Free();

        SSC_FREE( pms, "FFT Instance" );
    }

    /* return NULL to indicate that memory is freed */
    return NULL;
}

void SSC_FFT_FF_FftCalculate( SSC_FFT_FF_STATE* const pms, const void* const pData, void* const pResult)
{
    SInt idx = 0;

    if ( pms != NULL )
    {
        switch( pms->type )
        {
            case SSC_FFT_FF_FLOAT:
                for ( idx = 0; idx < pms->sizeFFT; idx++)
                {
                    ((Double*)pms->pRData)[idx] = ((FloatSSC*)pData)[idx];
                    ((Double*)pms->pIData)[idx] = (Double)0.0;

                }
                SSC_FFT_Process( pms->pFft, pms->sizeFFT );

                /* copy the separate real/imaginary arrays to in FF
                 * format return variable*/
                for ( idx = 0; idx < (pms->sizeFFT/2 + 1); idx++)
                {
                    ((FloatSSC*)pResult)[idx] = (FloatSSC)((Double*)pms->pRData)[idx];
                }
                for ( idx = pms->sizeFFT/2 + 1; idx < pms->sizeFFT; idx++)
                {
                    ((FloatSSC*)pResult)[idx] = -(FloatSSC)((Double*)pms->pIData)[idx];
                }

                break;
            case SSC_FFT_FF_DOUBLE:
                for ( idx = 0; idx < pms->sizeFFT; idx++)
                {
                    ((Double*)pms->pRData)[idx] = ((Double*)pData)[idx];
                    ((Double*)pms->pIData)[idx] = (Double)0.0;
                }
                SSC_FFT_Process( pms->pFft, pms->sizeFFT );

                for ( idx = 0; idx < (pms->sizeFFT/2 + 1); idx++)
                {
                    ((Double*)pResult)[idx] = (Double)((Double*)pms->pRData)[idx];
                }
                for ( idx = pms->sizeFFT/2 + 1; idx < pms->sizeFFT; idx++)
                {
                    ((Double*)pResult)[idx] = -((Double*)pms->pIData)[idx];
                }
                break;
        }
    }
}


void SSC_FFT_FF_IFftCalculate( SSC_FFT_FF_STATE* const pms, const void* const pData, void* const pResult)
{
    SInt idx = 0;

    if ( pms != NULL )
    {
        switch( pms->type )
        {
            case SSC_FFT_FF_FLOAT:
                /* extract real part */
                ((Double*)pms->pRData)[0             ] = ((FloatSSC*)pData)[0             ];
                ((Double*)pms->pRData)[pms->sizeFFT/2] = ((FloatSSC*)pData)[pms->sizeFFT/2];

                for ( idx = 1; idx < (pms->sizeFFT/2); idx++)
                {
                    ((Double*)pms->pRData)[idx             ] = ((FloatSSC*)pData)[idx];
                    ((Double*)pms->pRData)[pms->sizeFFT-idx] = ((Double*)pms->pRData)[idx];
                }
                /* extract imaginairy part */
                ((Double*)pms->pIData)[0             ] = (FloatSSC)0.0;
                ((Double*)pms->pIData)[pms->sizeFFT/2] = (FloatSSC)0.0;
                for ( idx = 1; idx < (pms->sizeFFT/2); idx++)
                {
                    ((Double*)pms->pIData)[idx             ] =  ((FloatSSC*)pData)[pms->sizeFFT - idx];
                    ((Double*)pms->pIData)[pms->sizeFFT-idx] = -((Double*)pms->pIData)[idx];
                }

                SSC_FFT_Process_Inverse( pms->pFft, pms->sizeFFT );

                for ( idx = 0; idx < pms->sizeFFT; idx++)
                {
                    ((FloatSSC*)pResult)[idx] = (FloatSSC)((Double*)pms->pRData)[idx];
                }

                break;
            case SSC_FFT_FF_DOUBLE:
                /* extract real part */
                ((Double*)pms->pRData)[0             ] = ((Double*)pData)[0             ];
                ((Double*)pms->pRData)[pms->sizeFFT/2] = ((Double*)pData)[pms->sizeFFT/2];

                for ( idx = 1; idx < (pms->sizeFFT/2); idx++)
                {
                    ((Double*)pms->pRData)[idx             ] = ((Double*)pData)[idx];
                    ((Double*)pms->pRData)[pms->sizeFFT-idx] = ((Double*)pms->pRData)[idx];
                }
                /* extract imaginairy part */
                ((Double*)pms->pIData)[0             ] = (Double)0.0;
                ((Double*)pms->pIData)[pms->sizeFFT/2] = (Double)0.0;
                for ( idx = 1; idx < (pms->sizeFFT/2); idx++)
                {
                    ((Double*)pms->pIData)[idx             ] =  ((Double*)pData)[pms->sizeFFT - idx];
                    ((Double*)pms->pIData)[pms->sizeFFT-idx] = -((Double*)pms->pIData)[idx];
                }

                SSC_FFT_Process_Inverse( pms->pFft, pms->sizeFFT );

                for ( idx = 0; idx < pms->sizeFFT; idx++)
                {
                    ((Double*)pResult)[idx] = (Double)((Double*)pms->pRData)[idx];
                }

                break;
        }
    }
}

/* returns a pointer to the real part */
Double *SSC_FFT_FF_GetPRe( const SSC_FFT_FF_STATE * const pFft )
{
    if( pFft != NULL )
    {
        return pFft->pRData ;
    }
    else
    {
        return NULL ;
    }
}
