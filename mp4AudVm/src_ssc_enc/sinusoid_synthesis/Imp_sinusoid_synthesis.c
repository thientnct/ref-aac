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

Source file: IMP_sinusoid_synthesis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
09-Oct-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : sinusoid_synthesis implementation
 *
 *   Description         : Implementation of the sinusoid_synthesis process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_SYNTHESIS_
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

#include "Imp_sinusoid_synthesis.h"
#include "SSC_sinusoid_synthesis.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  0
#define PIPE_AHEAD_LENGTH 2

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError         ( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static void LOCAL_Synthesis_Process( PSINUSOID_SYNTHESISSTATE pSSS );
static void LOCAL_Synthesize_Sine  ( PSINUSOID_SYNTHESISSTATE pSSS );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_SYNTHESIS_Initialise(void)
{
}


/* Creates an instance of the sinusoid_synthesis module */
PSINUSOID_SYNTHESISSTATE IMP_SINUSOID_SYNTHESIS_CreateInstance( PSINUSOID_SYNTHESISINFO psinusoid_synthesisInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_SYNTHESISSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        SInt i;

        if ( psinusoid_synthesisInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_synthesis info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_SYNTHESISSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_synthesis instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_synthesis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_SYNTHESIS_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_synthesisInfo->Channels,
                                           psinusoid_synthesisInfo->SubframeSize,
                                           psinusoid_synthesisInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_SYNTHESIS_INIT_PIPE_FAILED ;
        }

        if ( ( psinusoid_synthesisInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_synthesisInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels       = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels       = psinusoid_synthesisInfo->Channels ;
        }

        pms->SubframeSize       = psinusoid_synthesisInfo->SubframeSize ;
        pms->SynthesizeLength   = 2*psinusoid_synthesisInfo->SubframeSize;

        pms->pWindow = SSC_CALLOC( pms->SynthesizeLength * sizeof( *pms->pWindow ), 1, "" );
        if( pms->pWindow == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_synthesis window\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        for( i=0; i<pms->SynthesizeLength; i++ )
        {
            pms->pWindow[i] = .5*(1 - cos(2*SSC_PI*(i+1) / (pms->SynthesizeLength+1)));
        }
    }
    Catch( Error )
    {
        IMP_SINUSOID_SYNTHESIS_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_SYNTHESIS_Process( PSINUSOID_SYNTHESISSTATE psinusoid_synthesisState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_synthesisState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( psinusoid_synthesisState->pPipe, pSscWorkBunch );

        LOCAL_Synthesis_Process( psinusoid_synthesisState );
        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscWorkBunch = SSCPIPE_Shift( psinusoid_synthesisState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_SYNTHESIS_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_synthesis module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_SINUSOID_SYNTHESIS_CreateInstance */
void IMP_SINUSOID_SYNTHESIS_DestroyInstance( PSINUSOID_SYNTHESISSTATE* ppsinusoid_synthesisState )
{
    PSINUSOID_SYNTHESISSTATE psinusoid_synthesisState = *ppsinusoid_synthesisState ;

    if( psinusoid_synthesisState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_synthesisState->pLog, "SSCSINUSOID_SYNTHESIS_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_synthesisState->pPipe );
        if( psinusoid_synthesisState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_synthesisState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_synthesisState->pWindow, "" );
        SSC_FREE( psinusoid_synthesisState, "" );
        *ppsinusoid_synthesisState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_SYNTHESIS_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_SYNTHESIS_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_SYNTHESIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}

static void LOCAL_Synthesis_Process( PSINUSOID_SYNTHESISSTATE pSSS )
{
    SInt i, nCh ;
    SSCBUNCH *pSscBunch = SSCPIPE_GetAt( pSSS->pPipe, 1 );

    for( nCh = 0; nCh < pSSS->Channels; nCh++ )
    {
        /* copy incoming residual to signal buffer */
        memcpy( SSCBUNCH_GetSignal     ( pSscBunch, nCh ),
                SSCBUNCH_GetResidual   ( pSscBunch, nCh ),
                pSSS->SubframeSize*sizeof(Double) );
        /* clear synthesize buffer */
        memset( SSCBUNCH_GetSynthesized( pSscBunch, nCh ), 0,
                pSSS->SubframeSize*sizeof(Double));
    }

    pSscBunch = SSCPIPE_GetAt( pSSS->pPipe, 0 );
    if( SSCBUNCH_GetDataFromStream( pSscBunch ) == 1 )
    {
        LOCAL_Synthesize_Sine( pSSS );
    }


    /* calculate new residual */
    for( nCh=0; nCh < pSSS->Channels; nCh++ )
    {
        Double *pSignal   = SSCBUNCH_GetSignal          ( pSscBunch, nCh );
        Double *pResidual = SSCBUNCH_GetResidual        ( pSscBunch, nCh );
        Double *pSynth    = SSCBUNCH_GetSynthesized     ( pSscBunch, nCh );

        for( i=0; i < pSSS->SubframeSize; i++ )
        {
            pResidual[i] = pSignal[i] - pSynth[i] ;
        }
    }
}

static void LOCAL_Synthesize_Sine( PSINUSOID_SYNTHESISSTATE pSSS )
{
    SInt i, nCh, nSin ;
    SSCBUNCH *pCurrBunch = SSCPIPE_GetAt( pSSS->pPipe, 0 );
    SSCBUNCH *pNextBunch = SSCPIPE_GetAt( pSSS->pPipe, 1 );

    for( nCh = 0; nCh < pSSS->Channels; nCh++ )
    {
        Double *pFreq     = SSCBUNCH_GetSinusoidFreq    ( pCurrBunch, nCh );
        Double *pAmp      = SSCBUNCH_GetSinusoidAmp     ( pCurrBunch, nCh );
        Double *pPhase    = SSCBUNCH_GetSinusoidPhaseOrg( pCurrBunch, nCh );
        Double *pTrackIdx = SSCBUNCH_GetSinusoidTrackIdx( pCurrBunch, nCh );
        Double *pSynth0   = SSCBUNCH_GetSynthesized     ( pCurrBunch, nCh );
        Double *pSynth1   = SSCBUNCH_GetSynthesized     ( pNextBunch, nCh );
        Double *pWindow0  = &pSSS->pWindow[0] ;
        Double *pWindow1  = &pSSS->pWindow[pSSS->SubframeSize] ;

        for( nSin = 0; nSin < SSC_ENCODER_MAX_SINS; nSin++ )
        {
            if( pTrackIdx[nSin] > 0 )
            {
                Double dDelta = 0.5 * pFreq[nSin] ;

                for(i = 0; i < pSSS->SubframeSize; i++)
                {
                    pSynth0[pSSS->SubframeSize-i-1] += pWindow0[pSSS->SubframeSize-i-1] * pAmp[nSin] * cos( pPhase[nSin] - dDelta);
                    pSynth1[i]                      += pWindow1[i]                      * pAmp[nSin] * cos( pPhase[nSin] + dDelta);
                    dDelta += pFreq[nSin] ;
                }

            }
        }
    }
}
