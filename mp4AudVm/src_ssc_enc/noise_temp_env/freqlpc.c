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

Source file: freqlpc.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
18-Sep-2002	NT	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : FREQLPC implementation
 *
 *   Description         : Implementation of the frequency lpc process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_FREQLPC_
 *
 *
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "freqlpc.h"

#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"

#include "cexcept.h"
#include "SSC_Vector.h"

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
static void LOCAL_levinson( Double* RVect, Double* result_coef, SInt NSize );
static void LOCAL_acorr(Double* xVect, SInt length, Double* a);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


void SSC_FREQLPC_Initialise ( void )
{
}


void SSC_FREQLPC_Free ( void )
{
}


SSC_FREQLPC_STATE* SSC_FREQLPC_CreateInstance ( SSCLOG* pLog, const SInt envOrder, const SInt dataLength )
{
    SSC_FREQLPC_STATE* pms = NULL ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {

        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_FREQLPC_STATE), 1, "FREQLPC Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for FREQLPC instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_FREQLPC_Initialise();

        /* get the log object from the caller for logging */
        pms->pLog    = pLog;
        pms->pFft    = NULL;
        pms->pSignal = NULL;
        pms->pRSignal= NULL;
        pms->S1      = NULL;
        pms->ac      = NULL;
        pms->ac1     = NULL;

        pms->envOrder   = envOrder;
        pms->dataLength = dataLength;

        pms->pFft    = SSC_FFT_FF_CreateInstance(pLog, 2*dataLength, SSC_FFT_FF_DOUBLE);
        if ( pms->pFft == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested FFT instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* temp storage for storage of the FFT data */
        pms->pFftData = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2*dataLength);
        if ( pms->pFftData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested FftData vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pFftData);

        pms->pSignal = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2*dataLength);
        if ( pms->pSignal == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested Signal vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pSignal);

        pms->pRSignal = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2*dataLength);
        if ( pms->pRSignal == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested RSignal vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pRSignal);

	    pms->S1 = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 2*dataLength );
        if ( pms->S1 == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested S1 vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

	    pms->ac = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, 4*dataLength - 1 );
        if ( pms->ac == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested ac vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms->ac1 = SSC_VECT_CreateInstance(pLog, SSC_VECT_DOUBLE, envOrder + 1 );
        if ( pms->ac1 == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Unable to create requested ac1 vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_FREQLPC_DestroyInstance( pms );
    }

    return pms;

}


SSC_FREQLPC_STATE* SSC_FREQLPC_DestroyInstance( SSC_FREQLPC_STATE* const pms )
{
    if ( pms != NULL )
    {
        if ( pms->pFft != NULL )
        {
            pms->pFft = SSC_FFT_FF_DestroyInstance(pms->pFft);
        }
        if ( pms->pSignal != NULL )
        {
            pms->pSignal = SSC_VECT_DestroyInstance(pms->pSignal);
        }
        if ( pms->pRSignal != NULL )
        {
            pms->pRSignal = SSC_VECT_DestroyInstance(pms->pRSignal);
        }
        if ( pms->S1 != NULL )
        {
            pms->S1 = SSC_VECT_DestroyInstance(pms->S1);
        }
        if ( pms->ac != NULL )
        {
            pms->ac = SSC_VECT_DestroyInstance(pms->ac);
        }
        if ( pms->ac1 != NULL )
        {
            pms->ac1 = SSC_VECT_DestroyInstance(pms->ac1);
        }

        SSC_FREQLPC_Free();

        SSC_FREE( pms, "FREQLPC Instance" );
    }

    /* return NULL to indicate that memory is free */
    return NULL;
}


void SSC_FREQLPC_Calculate( SSC_FREQLPC_STATE* const pms, 
                            const Double* const pData,
                            Double* const pLpcPars )
{
    if ( pms != NULL )
    {
        SInt    c             = 0;    
        SInt    k             = 0;    
        SInt    n             = 0;    
        SInt    envOrder      = pms->envOrder;
        SInt    dataLength    = pms->dataLength;

        Double* pRSignal  = pms->pRSignal;
        Double* pSignal   = pms->pSignal;
        Double* S1        = pms->S1;

        Double* ac        = pms->ac;
        Double* ac1       = pms->ac1;

        SInt    signalLength  = 2*dataLength;
        for( c = 0 ; c < dataLength ; c++)
        {
		    pSignal[c] = pData[c];
            if (c != dataLength-1)
            {
                pSignal[signalLength - c - 1] = pData[c+1];
            }
            else
            {
                pSignal[signalLength - c - 1] = 0;
            }
        }
        for (k = 0; k < signalLength; k++)
        {
            Double s = 0;
            for (n = 0; n < signalLength; n++)
            {
                s += pSignal[n] * cos(-2 * SSC_PI * k * n / signalLength);

            }
            pRSignal[k] = s;
        }


        /* Perform LPC on frequency parameters */
        for ( c = 0; c < dataLength; c++)
	    {
		    S1[c]                    = pRSignal[dataLength - 1 - c];
		    S1[signalLength - c - 1] = pRSignal[dataLength - c];
	    }
         

        LOCAL_acorr(S1, signalLength, ac);

        for ( c = 0; c < envOrder + 1; c++) 
        {
            ac1[c] = ac[signalLength-1+c]; 
        }

        LOCAL_levinson( ac1, pLpcPars, envOrder+1 ); 

        return;
    }
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Implementation of Levinson algorithm.
 *
 * NSize should not exceed maximum possible polynomial order plus 1 */
static void LOCAL_levinson( Double * RVect,
                            Double * result_coef,
                            SInt     NSize /* order+1 */ )
{
    Double ptrR[SSC_N_MAX_POL_ORDER+1];
    Double out_coef[SSC_N_MAX_POL_ORDER+1];
    Double coef[SSC_N_MAX_POL_ORDER+1];

    SInt   n            = 0;  /* general counter */
    SInt   c1           = 0;
    SInt   nmax         = 0;
    Double sigmaSqr     = 0;
    Double k            = 0;
    Double inprod       = 0;

    assert( NSize <= SSC_N_MAX_POL_ORDER+1);
    for( c1 = 0 ; c1 < NSize ; c1++ )
    {
        coef[c1] = 0;
        /* Convert RVect to double */
        ptrR[c1] = RVect[c1];
    }

    nmax        = (NSize) - 2;
    coef[0]     = 1;out_coef[0]=1;
    coef[1]     = - (ptrR[1]/ptrR[0]);
    out_coef[1] = coef[1];
    sigmaSqr    = ptrR[0] - (ptrR[1]*ptrR[1]) / ptrR[0];

    /* recursion  loop */
    for ( n = 1 ; n <= nmax ; n++)
    {
        inprod = 0;

        for ( c1 = n ; c1 > 0 ; c1--)
        {
            inprod = inprod + ptrR[c1] * coef[n - c1 +1 ];
        }

        k = -((ptrR[n+1] + inprod)/sigmaSqr);

        sigmaSqr =  sigmaSqr * (1 - (k*k));

        for ( c1 = 1 ; c1 <= n ; c1++ )
        {
            out_coef[c1] = coef[c1] + k * coef[n - c1 +1];
        }

        out_coef[n+1] = k;

        for ( c1 = 1 ; c1 <= n+1 ; c1++ )
        {
            coef[c1] = out_coef[c1];
        }
    }

    for( c1 = 0 ; c1 < NSize ; c1++ )
    {
        /* Convert out_coef to float */
        result_coef[c1] = (Double) out_coef[c1];
    }


    return;
}

/* auto-correlation*/
static void LOCAL_acorr(Double* xVect, SInt length, Double* a)
{
    SInt   d   = 0;
    
    for (d=-(length-1);d<=(length-1);d++) 
    {
        SInt   i   = 0;
        SInt   j   = 0;
        Double sx1 = 0;
        if (d >= 0)
        {       
            for (i=0;i<=length-1-d;i++)
            {
         
                j = i + d;
                sx1 += xVect[i] * xVect[j];
            }
        }
        else
        {
            for (i=0;i<=length-1+d;i++)
            {
                j = i - d;
                sx1 += xVect[i] * xVect[j];
            }
        }
       a[d+length-1] = sx1;
    }
}
