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

Source file: Imp_noise_quantisation.c

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

/******************************************************************************
 *
 *   Module              : noise_quantisation implementation
 *
 *   Description         : Implementation of the noise_quantisation process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_NOISE_QUANTISATION_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_noise_quantisation.h"
#include "SSC_noise_quantisation.h"
#include "Imp_Error.h"

#include "Imp_quantise.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 5

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void      LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* One time inialisation code to allocate/initialise tables */
void IMP_NOISE_QUANTISATION_Initialise(void)
{
}


/* Creates an instance of the noise_quantisation module */
PNOISE_QUANTISATIONSTATE IMP_NOISE_QUANTISATION_CreateInstance( PNOISE_QUANTISATIONINFO pnoise_quantisationInfo, Char *szLogFile, SInt nTraceLevel )
{
    PNOISE_QUANTISATIONSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( pnoise_quantisationInfo == NULL )
        {
            printf( "\nError: Invalid pointer to noise_quantisation info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_NOISE_QUANTISATIONSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for noise_quantisation instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        pms->pLog = NULL ;
        pms->pPipe = NULL ;

        #ifdef LOG_ENABLE
        {
            SSCLOGINFO li ;
            Char *cp ;

            SSCLOG_INITIALISE();
            li.bFlushAfterWrite = False ;
            li.bPrintTime = False ;
            li.bPrintTitle = False ;
            li.bStdOut = True ;
            li.nTraceLevel = nTraceLevel ;
            strncpy( li.szFileName , szLogFile, sizeof(li.szFileName)/sizeof(Char) - 1 );
            li.szFileName[sizeof(li.szFileName)/sizeof(Char) - 1] = '\0' ;
            cp = strrchr( szLogFile, (int)'.' );
            if( cp )
            {
                *cp = '\0' ; /* strip extension, if found */
            }
            strncpy( li.szTitle, "SSC_noise_quantisation", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCNOISE_QUANTISATION_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( pnoise_quantisationInfo->Channels,
                                           pnoise_quantisationInfo->SubframeSize,
                                           pnoise_quantisationInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCNOISE_QUANTISATION_INIT_PIPE_FAILED ;
        }

        if ( ( pnoise_quantisationInfo->Mode == SSC_MODE_LCSTEREO ) && ( pnoise_quantisationInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->nChannels  = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->nChannels  = pnoise_quantisationInfo->Channels ;
        }

        pms->dLar_Stepsize  = pnoise_quantisationInfo->dLar_Stepsize ;
        pms->nAbs_max_lar   = pnoise_quantisationInfo->nAbs_max_lar ;
        pms->nRlGain_min    = pnoise_quantisationInfo->nRlGain_min ;
        pms->nRlGain_max    = pnoise_quantisationInfo->nRlGain_max ;
        pms->nRlGain_offset = pnoise_quantisationInfo->nRlGain_offset ;

        pms->dLsf_Stepsize  = pnoise_quantisationInfo->dLsf_Stepsize ;
        pms->nLsf_min_val   = pnoise_quantisationInfo->nLsf_min_val ;
        pms->nLsf_max_val   = pnoise_quantisationInfo->nLsf_max_val ;
    }
    Catch( Error )
    {
        IMP_NOISE_QUANTISATION_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */

SSCBUNCH* IMP_NOISE_QUANTISATION_Process( PNOISE_QUANTISATIONSTATE pNQS, SSCBUNCH* pSscBunch )
{
    if ( pNQS != NULL )
    {
        SInt      SfCounter =  SSCBUNCH_GetSfCounter(SSCPIPE_GetAt(pNQS->pPipe, PIPE_HIST_LENGTH));
        SSC_ERROR Error = SSC_ERROR_OK ;

        SSCBUNCH* pCurrSscBunch = SSCPIPE_GetAt( pNQS->pPipe, PIPE_HIST_LENGTH );
        SSCBUNCH* pNextSscBunch = SSCPIPE_GetAt( pNQS->pPipe, PIPE_HIST_LENGTH+1 );


        SSCPIPE_Add( pNQS->pPipe, pSscBunch );

        /* Use LOCAL_SetError to set the error in the sbunch */
        if( SSCBUNCH_GetDataFromStream( pCurrSscBunch ) == 1 )
        {
            SInt i, nCh ;


            for( nCh = 0; nCh < pNQS->nChannels; nCh++ )
            {
                /* Noise analysis parameters are quantised 
                 * once every two subframes */
                if ( SfCounter % 2 == 1)
                {
                     Double Qden [SSC_ENCODER_MAX_DEN] ;

                     Double *pDen   = SSCBUNCH_GetNoiseDen ( pCurrSscBunch, nCh );
                     
                     for( i=0; i<SSC_ENCODER_MAX_DEN; i++ )
                     {
                        Qden[i]    = cliprl(4 * floor(pDen[i] / pNQS->dLar_Stepsize / 4 + 0.5), -pNQS->nAbs_max_lar, pNQS->nAbs_max_lar );
                     }
                     SSCBUNCH_SetQNoiseDen ( pCurrSscBunch, nCh, Qden );
                }
                /* Noise temporal envelope parameters are quantised 
                 * once every four subframes */
                if ( SfCounter % 4 == 1)
                {
                     SInt i = 0;

                     Double Qgain ;
                     Double Qlsf [SSC_N_MAX_LSF_ORDER] ;

                     SInt   nFlagCurr[SSC_N_MAX_LSF_ORDER] ;
                     SInt   nFlagNext[SSC_N_MAX_LSF_ORDER] ;

                     Double dMeanLsf[SSC_N_MAX_LSF_ORDER] ;
                     Double dWeightNext[SSC_N_MAX_LSF_ORDER] ;
                     Double dWeightCurr[SSC_N_MAX_LSF_ORDER] ;
                     Double dWeightedLsfCurr[SSC_N_MAX_LSF_ORDER] ;
                     Double dWeightedLsfNext[SSC_N_MAX_LSF_ORDER] ;

                     Double dEnvGain   = SSCBUNCH_GetNoiseEnvGain ( pCurrSscBunch, nCh );
                     Double *pdCurrLsf = SSCBUNCH_GetNoiseEnvCoefs( pCurrSscBunch, nCh );
                     Double *pdNextLsf = SSCBUNCH_GetNoiseEnvCoefs( pNextSscBunch, nCh );

                     SInt   nNrOfCurrLsf = 0 ;
                     SInt   nNrOfNextLsf = 0 ;


                     for ( i = 0; i < SSC_N_MAX_LSF_ORDER; i++)
                     {
                         dWeightedLsfCurr[i] = pdCurrLsf[i];
                         dWeightedLsfNext[i] = pdNextLsf[i];

                         if ( pdCurrLsf[i] >= SSC_PI*3/4 )
                         {
                             nNrOfCurrLsf++;
                             nFlagCurr[i] = 1;
                         }
                         else
                         {  
                             nFlagCurr[i] = 0;
                         }
                         if ( pdNextLsf[i] >= SSC_PI*3/4 )
                         {
                             nNrOfNextLsf++;
                             nFlagNext[i] = 1;
                         }
                         else
                         {
                             nFlagNext[i] = 0;
                         }
                     }

                     if ( nNrOfCurrLsf == nNrOfNextLsf )
                     {
                         SSCBUNCH_SetNoiseEnvOverlap ( pNextSscBunch, nCh, nNrOfNextLsf );
                         SSCBUNCH_SetQNoiseEnvOverlap( pNextSscBunch, nCh, nNrOfNextLsf );

                         for ( i = 0; i < SSC_N_MAX_LSF_ORDER; i++)
                         {
                             if (nFlagNext[i] == 1)
                             {
                                 dMeanLsf[i] = (pdCurrLsf[i] - SSC_PI*3/4 + pdNextLsf[i]) / 2;
                                 dWeightNext[i] = dMeanLsf[i]/(SSC_PI/4);
                                 dWeightNext[i] = 1 - dWeightNext[i];

                                 dWeightedLsfCurr[i] = dWeightCurr[i] * (pdCurrLsf[i] - SSC_PI*3/4) +
                                                       dWeightNext[i] * pdNextLsf[i];

                                 dWeightedLsfNext[i] = dWeightedLsfCurr[i] + SSC_PI*3/4;
                             }
                         }

                         SSCBUNCH_SetNoiseEnvCoefs( pNextSscBunch, nCh, dWeightedLsfCurr);
                         SSCBUNCH_SetNoiseEnvCoefs( pCurrSscBunch, nCh, dWeightedLsfNext);
                     }

                     for( i = 0; i < SSC_N_MAX_LSF_ORDER; i++ )
                     {
                        Qlsf[i]    = cliprl(floor((pdCurrLsf[i] - pNQS->dLsf_Stepsize/2) / pNQS->dLsf_Stepsize + 0.5), 
                                            pNQS->nLsf_min_val, pNQS->nLsf_max_val );
                     }

                     if ( dEnvGain > 0 )
                     {
                        Qgain = cliprl(lin2db(dEnvGain, pNQS->nRlGain_offset), pNQS->nRlGain_min, pNQS->nRlGain_max);
                     }
                     else
                     {
                         Qgain = 0;
                     }

                     SSCBUNCH_SetQNoiseEnvGain ( pCurrSscBunch, nCh, Qgain );
                     SSCBUNCH_SetQNoiseEnvCoefs( pCurrSscBunch, nCh, Qlsf);
                }
            }
        }

        pSscBunch = SSCPIPE_Shift( pNQS->pPipe );

        /* report error */ 
        LOCAL_SetError( pSscBunch, Error ); 
    }
    else
    {
        printf( "\nError: IMP_NOISE_QUANTISATION_Process invalid state pointer\n" );
    }
    return pSscBunch ;
}


/* Destroys the noise_quantisation module instance pointed to by *ppState and
 * frees all the memory allocated on IMP_NOISE_QUANTISATION_CreateInstance */

void IMP_NOISE_QUANTISATION_DestroyInstance( PNOISE_QUANTISATIONSTATE* ppnoise_quantisationState )
{
    PNOISE_QUANTISATIONSTATE pnoise_quantisationState = *ppnoise_quantisationState ;

    if( pnoise_quantisationState != NULL )
    {
        SSCLOG_TRACE4( pnoise_quantisationState->pLog, "SSCNOISE_QUANTISATION_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( pnoise_quantisationState->pPipe );
        if( pnoise_quantisationState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( pnoise_quantisationState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( pnoise_quantisationState, "" );
        *ppnoise_quantisationState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_NOISE_QUANTISATION_DestroyInstance invalid state pointer\n" );
    }
}


void IMP_NOISE_QUANTISATION_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_NOISE_QUANTISATION_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}
