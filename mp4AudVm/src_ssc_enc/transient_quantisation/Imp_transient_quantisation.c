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

Source file: Imp_transient_quantisation.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/

/******************************************************************************
 *
 *   Module              : transient_quantisation implementation
 *
 *   Description         : Implementation of the transient_quantisation process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_TRANSIENT_QUANTISATION_
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

#include "Imp_transient_quantisation.h"
#include "SSC_transient_quantisation.h"
#include "Imp_Error.h"

#include "cexcept.h"

#include "Imp_quantise.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 1

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
void IMP_TRANSIENT_QUANTISATION_Initialise(void)
{
}


/* Creates an instance of the transient_quantisation module */
PTRANSIENT_QUANTISATIONSTATE IMP_TRANSIENT_QUANTISATION_CreateInstance( PTRANSIENT_QUANTISATIONINFO ptransient_quantisationInfo, Char *szLogFile, SInt nTraceLevel )
{
    PTRANSIENT_QUANTISATIONSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( ptransient_quantisationInfo == NULL )
        {
            printf( "\nError: Invalid pointer to transient_quantisation info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_TRANSIENT_QUANTISATIONSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for transient_quantisation instance\n");
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
            strncpy( li.szTitle, "SSC_transient_quantisation", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCTRANSIENT_QUANTISATION_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( ptransient_quantisationInfo->Channels,
                                           ptransient_quantisationInfo->SubframeSize,
                                           ptransient_quantisationInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCTRANSIENT_QUANTISATION_INIT_PIPE_FAILED ;
        } 

        if ( ( ptransient_quantisationInfo->Mode == SSC_MODE_LCSTEREO ) && ( ptransient_quantisationInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->nChannels   = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->nChannels   = ptransient_quantisationInfo->Channels ;
        }

        pms->dFreq_rl_base  = ptransient_quantisationInfo->dFreq_rl_base ;
        pms->dAmp_rl_base   = ptransient_quantisationInfo->dAmp_rl_base ;
        pms->dPhase_rl_base = ptransient_quantisationInfo->dPhase_rl_base ;
        pms->dFs            = ptransient_quantisationInfo->dFs ;
        pms->grid_freq      = ptransient_quantisationInfo->grid_freq ;
        pms->grid_amp       = ptransient_quantisationInfo->grid_amp ;
    }
    Catch( Error )
    {
        IMP_TRANSIENT_QUANTISATION_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_TRANSIENT_QUANTISATION_Process( PTRANSIENT_QUANTISATIONSTATE pTQS, SSCBUNCH* pSscBunch )
{
    if ( pTQS != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;

        SSCPIPE_Add( pTQS->pPipe, pSscBunch );

        /* actually no pipe is needed, to be removed in the future */
        if( SSCBUNCH_GetDataFromStream( pSscBunch ) == 1 )
        {
            SInt      nCh, nSines ;
            Double    q_Frequency[SSC_T_MAX_MEIXNER_SINUSOIDS] ;
            Double    q_Amplitude[SSC_T_MAX_MEIXNER_SINUSOIDS] ;
            Double    q_Phase    [SSC_T_MAX_MEIXNER_SINUSOIDS] ;
            Double    *pFrequency = NULL ;
            Double    *pAmplitude = NULL ;
            Double    *pPhase     = NULL ;

            for( nCh = 0; nCh < pTQS->nChannels; nCh++ )
            {
                Bool bPresent  = SSCBUNCH_GetTransientPresent( pSscBunch, nCh );
                SInt nPos      = SSCBUNCH_GetTransientPos    ( pSscBunch, nCh );
                SInt nTranType = SSCBUNCH_GetTransientType   ( pSscBunch, nCh );

                if( bPresent )
                {
                    if( nTranType == SSC_TRANSIENT_TYPE_MEIXNER )
                    {
                        SSCLOG_TRACE2(pTQS->pLog,"Meixner transient in sf:%d\n", SSCBUNCH_GetSfCounter(pSscBunch) );
                        /* b and chi are already in representation levels */
                        SSCBUNCH_SetQTransientBpar( pSscBunch, nCh, SSCBUNCH_GetTransientBpar( pSscBunch, nCh) );
                        SSCBUNCH_SetQTransientChi ( pSscBunch, nCh, SSCBUNCH_GetTransientChi ( pSscBunch, nCh) );
                        SSCBUNCH_SetQTransientMeixnerSins( pSscBunch, nCh, SSCBUNCH_GetTransientMeixnerSins( pSscBunch, nCh) );

                        SSCLOG_TRACE2(pTQS->pLog," B:   %f\n", SSCBUNCH_GetQTransientBpar        ( pSscBunch, nCh) );
                        SSCLOG_TRACE2(pTQS->pLog," Chi: %f\n", SSCBUNCH_GetQTransientChi         ( pSscBunch, nCh) );
                        SSCLOG_TRACE2(pTQS->pLog," Sins:%d\n", SSCBUNCH_GetQTransientMeixnerSins ( pSscBunch, nCh) );
                        SSCLOG_TRACE2(pTQS->pLog,"        freq  amplitude  phase\n" );

                        pFrequency = SSCBUNCH_GetTransientMeixnerFreq ( pSscBunch, nCh );
                        pAmplitude = SSCBUNCH_GetTransientMeixnerAmp  ( pSscBunch, nCh );
                        pPhase     = SSCBUNCH_GetTransientMeixnerPhase( pSscBunch, nCh );

                        for( nSines = 0; nSines < SSCBUNCH_GetTransientMeixnerSins( pSscBunch, nCh); nSines++ )
                        {
                            q_Frequency[nSines] = quantise_freq ( pFrequency[nSines], pTQS->dFreq_rl_base, pTQS->dFs, pTQS->grid_freq );
                            q_Amplitude[nSines] = quantise_amp  ( pAmplitude[nSines], pTQS->dAmp_rl_base,  pTQS->grid_amp );
                            q_Phase    [nSines] = quantise_phase( pPhase    [nSines], pTQS->dPhase_rl_base );

                            SSCLOG_TRACE2( pTQS->pLog, "        %f %f %f\n", q_Frequency[nSines], q_Amplitude[nSines], q_Phase[nSines] );
                        }
                        for( ; nSines < SSC_T_MAX_MEIXNER_SINUSOIDS; nSines++ )
                        {
                            q_Frequency[nSines] = 0 ;
                            q_Amplitude[nSines] = 0 ;
                            q_Phase    [nSines] = 0 ;
                        }
                        SSCBUNCH_SetQTransientMeixnerFreq ( pSscBunch, nCh, q_Frequency );
                        SSCBUNCH_SetQTransientMeixnerAmp  ( pSscBunch, nCh, q_Amplitude );
                        SSCBUNCH_SetQTransientMeixnerPhase( pSscBunch, nCh, q_Phase );
                    }
                }
                SSCBUNCH_SetQTransientPresent( pSscBunch, nCh, bPresent );
                SSCBUNCH_SetQTransientPos    ( pSscBunch, nCh, nPos );
                SSCBUNCH_SetQTransientType   ( pSscBunch, nCh, nTranType );
            }
        }
        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscBunch = SSCPIPE_Shift( pTQS->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_QUANTISATION_Process invalid state pointer\n" );
    }
    return pSscBunch ;
}


/* Destroys the transient_quantisation module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_TRANSIENT_QUANTISATION_CreateInstance */
void IMP_TRANSIENT_QUANTISATION_DestroyInstance( PTRANSIENT_QUANTISATIONSTATE* pptransient_quantisationState )
{
    PTRANSIENT_QUANTISATIONSTATE ptransient_quantisationState = *pptransient_quantisationState ;

    if( ptransient_quantisationState != NULL )
    {
        SSCLOG_TRACE4( ptransient_quantisationState->pLog, "SSCTRANSIENT_QUANTISATION_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( ptransient_quantisationState->pPipe );
        if( ptransient_quantisationState->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( ptransient_quantisationState->pLog );
            SSCLOG_FREE();
        }
        SSC_FREE( ptransient_quantisationState, "" );
        *pptransient_quantisationState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_TRANSIENT_QUANTISATION_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_TRANSIENT_QUANTISATION_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_TRAN_QUANTISATION_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}
