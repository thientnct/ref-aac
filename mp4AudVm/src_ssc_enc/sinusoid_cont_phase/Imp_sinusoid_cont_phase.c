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

Source file: IMP_sinusoid_cont_phase.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
20-Sep-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : sinusoid_cont_phase implementation
 *
 *   Description         : Implementation of the sinusoid_cont_phase process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_CONT_PHASE_
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

#include "Imp_sinusoid_cont_phase.h"
#include "SSC_sinusoid_cont_phase.h"
#include "Imp_Error.h"
#include "Imp_quantise.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH  1
#define PIPE_AHEAD_LENGTH 1
#define PIPE_LENGTH       (PIPE_HIST_LENGTH + PIPE_AHEAD_LENGTH)
/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError           ( SSCBUNCH *pSscBunch, SSC_ERROR Error );

static void LOCAL_Sinusoid_Cont_Phase_Process
                                     ( PSINUSOID_CONT_PHASESTATE pSCPS );

static void LOCAL_Sf_Calc_Cont_Phase ( SSCBUNCH *pPrev, SSCBUNCH *pCurr, SInt nCh,
                                       PSINUSOID_CONT_PHASESTATE pSCPS );

static void LOCAL_Sf_Calc_Cont_PhaseQ( SSCBUNCH *pPrev, SSCBUNCH *pCurr, SInt nCh,
                                       PSINUSOID_CONT_PHASESTATE pSCPS );

static Double LOCAL_Calc_Cont_Phase( Double p0, Double f1, Double f2, SInt Separation );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_CONT_PHASE_Initialise(void)
{
}

/* Creates an instance of the sinusoid_cont_phase module */
PSINUSOID_CONT_PHASESTATE IMP_SINUSOID_CONT_PHASE_CreateInstance( PSINUSOID_CONT_PHASEINFO psinusoid_cont_phaseInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_CONT_PHASESTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( psinusoid_cont_phaseInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_cont_phase info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_CONT_PHASESTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_cont_phase instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_cont_phase", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_CONT_PHASE_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_cont_phaseInfo->Channels,
                                           psinusoid_cont_phaseInfo->SubframeSize,
                                           psinusoid_cont_phaseInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_CONT_PHASE_INIT_PIPE_FAILED ;
        }
        if ( ( psinusoid_cont_phaseInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_cont_phaseInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels  = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels  = psinusoid_cont_phaseInfo->Channels ;
        }
        pms->SubframeSize  = psinusoid_cont_phaseInfo->SubframeSize ;
        pms->fs            = psinusoid_cont_phaseInfo->fs ;
        pms->freq_rl_base  = psinusoid_cont_phaseInfo->freq_rl_base ;
        pms->phase_rl_base = psinusoid_cont_phaseInfo->phase_rl_base ;
        pms->pPrevFreqDeq  = SSC_CALLOC( SSC_ENCODER_MAX_SINS, sizeof( *pms->pPrevFreqDeq ), "" );
        pms->pCurrFreqDeq  = SSC_CALLOC( SSC_ENCODER_MAX_SINS, sizeof( *pms->pCurrFreqDeq ), "" );
        pms->pPcDeq        = SSC_CALLOC( SSC_ENCODER_MAX_SINS, sizeof( *pms->pPcDeq ),       "" );

    }
    Catch( Error )
    {
        IMP_SINUSOID_CONT_PHASE_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_CONT_PHASE_Process( PSINUSOID_CONT_PHASESTATE psinusoid_cont_phaseState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_cont_phaseState != NULL )
    {
        SSCPIPE_Add( psinusoid_cont_phaseState->pPipe, pSscWorkBunch );

        /* Use LOCAL_SetError to set the error in the sbunch */
        LOCAL_Sinusoid_Cont_Phase_Process( psinusoid_cont_phaseState );
        pSscWorkBunch = SSCPIPE_Shift( psinusoid_cont_phaseState->pPipe );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_CONT_PHASE_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_cont_phase module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_SINUSOID_CONT_PHASE_CreateInstance */
void IMP_SINUSOID_CONT_PHASE_DestroyInstance( PSINUSOID_CONT_PHASESTATE* ppsinusoid_cont_phaseState )
{
    PSINUSOID_CONT_PHASESTATE psinusoid_cont_phaseState = *ppsinusoid_cont_phaseState ;

    if( psinusoid_cont_phaseState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_cont_phaseState->pLog, "SSCSINUSOID_CONT_PHASE_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_cont_phaseState->pPipe );
        if( psinusoid_cont_phaseState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_cont_phaseState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_cont_phaseState->pPrevFreqDeq, "" );
        SSC_FREE( psinusoid_cont_phaseState->pCurrFreqDeq, "" );
        SSC_FREE( psinusoid_cont_phaseState->pPcDeq,       "" );
        SSC_FREE( psinusoid_cont_phaseState, "" );
        *ppsinusoid_cont_phaseState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_CONT_PHASE_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_CONT_PHASE_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_CONTINUOUS_PHASE_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


static void LOCAL_Sinusoid_Cont_Phase_Process( PSINUSOID_CONT_PHASESTATE pSCPS )
{
    SSC_ERROR      Error                 = SSC_ERROR_OK ;
    SSCBUNCH      *pPrevBunch            = SSCPIPE_GetAt( pSCPS->pPipe, PIPE_LENGTH-2 );
    SSCBUNCH      *pCurrBunch            = SSCPIPE_GetAt( pSCPS->pPipe, PIPE_LENGTH-1 );
    SSC_EXCEPTION  tec                   = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION *the_exception_context = &tec;

    Try
    {
        if( SSCBUNCH_GetDataFromStream( pCurrBunch ) == 1 )
        {
            SInt nCh ;

            for( nCh = 0; nCh<pSCPS->Channels; nCh++ )
            {
                /* unquantised continuous phase */
                LOCAL_Sf_Calc_Cont_Phase ( pPrevBunch, pCurrBunch, nCh, pSCPS );
                /* quantised continuous phase */
                LOCAL_Sf_Calc_Cont_PhaseQ( pPrevBunch, pCurrBunch, nCh, pSCPS );
            }
        }
    }
    Catch(Error)
    {
        if( pCurrBunch != NULL )
        {
            LOCAL_SetError( pCurrBunch, Error );
        }
    }
}

static void LOCAL_Sf_Calc_Cont_Phase(  SSCBUNCH *pPrev, SSCBUNCH *pCurr, SInt nCh,
                                       PSINUSOID_CONT_PHASESTATE pSCPS )
{
    SInt nSin ;

    Double *pTrackIdx  = SSCBUNCH_GetSinusoidTrackIdx ( pCurr, nCh );
    Double *pTrackPrev = SSCBUNCH_GetSinusoidTrackPrev( pCurr, nCh );
    Double *pPrevFreq  = SSCBUNCH_GetSinusoidFreq     ( pPrev, nCh );
    Double *pCurrFreq  = SSCBUNCH_GetSinusoidFreq     ( pCurr, nCh );
    Double *pPo        = SSCBUNCH_GetSinusoidPhaseOrg ( pCurr, nCh );
    Double *pPc        = SSCBUNCH_GetSinusoidPhaseCont( pPrev, nCh );
    Double *pPcont     = SSCBUNCH_GetSinusoidPhaseCont( pCurr, nCh );

    for( nSin = 0; nSin < SSC_ENCODER_MAX_SINS; nSin++ )
    {
        if( pTrackIdx[nSin] == 1 )
        {
            /* births */
            pPcont[nSin] = pPo[nSin] ;
        }
        else if( pTrackIdx[nSin] > 1 )
        {
            /* continuations */
            pPcont[nSin] = LOCAL_Calc_Cont_Phase( pPc[(SInt)(pTrackPrev[nSin])], pPrevFreq[(SInt)(pTrackPrev[nSin])],
                                                  pCurrFreq[nSin], pSCPS->SubframeSize );
        }
    }
}

static void LOCAL_Sf_Calc_Cont_PhaseQ( SSCBUNCH *pPrev, SSCBUNCH *pCurr, SInt nCh,
                                       PSINUSOID_CONT_PHASESTATE pSCPS )
{
    SInt nSin ;

    Double *pQTrackIdx  = SSCBUNCH_GetQSinusoidTrackIdx ( pCurr, nCh );
    Double *pQTrackPrev = SSCBUNCH_GetQSinusoidTrackPrev( pCurr, nCh );
    Double *pQPrevFreq  = SSCBUNCH_GetQSinusoidFreq     ( pPrev, nCh );
    Double *pQCurrFreq  = SSCBUNCH_GetQSinusoidFreq     ( pCurr, nCh );
    Double *pQPo        = SSCBUNCH_GetQSinusoidPhaseOrg ( pCurr, nCh );
    Double *pQPc        = SSCBUNCH_GetQSinusoidPhaseCont( pPrev, nCh );
    Double *pQPcont     = SSCBUNCH_GetQSinusoidPhaseCont( pCurr, nCh );

    for( nSin = 0; nSin < SSC_ENCODER_MAX_SINS; nSin++ )
    {
        /* dequantise parameters */
        pSCPS->pPrevFreqDeq[nSin] = dequantise_freq ( (SInt)(pQPrevFreq[nSin]), pSCPS->freq_rl_base, pSCPS->fs );
        pSCPS->pCurrFreqDeq[nSin] = dequantise_freq ( (SInt)(pQCurrFreq[nSin]), pSCPS->freq_rl_base, pSCPS->fs );
        pSCPS->pPcDeq      [nSin] = dequantise_phase( (SInt)(pQPc      [nSin]), pSCPS->phase_rl_base );
    }

    for( nSin = 0; nSin < SSC_ENCODER_MAX_SINS; nSin++ )
    {
        if( pQTrackIdx[nSin] == 1 )
        {
            /* births */
            pQPcont[nSin] = pQPo[nSin] ;
        }
        else if( pQTrackIdx[nSin] > 1 )
        {
            /* continuations */
            Double dPhase = LOCAL_Calc_Cont_Phase( pQPc[(SInt)(pQTrackPrev[nSin])], pQPrevFreq[(SInt)(pQTrackPrev[nSin])],
                                                   pQCurrFreq[nSin], pSCPS->SubframeSize );
            pQPcont[nSin]  = quantise_phase_float( dPhase, pSCPS->phase_rl_base );
        }
    }
}

static Double LOCAL_Calc_Cont_Phase( Double p0, Double f1, Double f2, SInt Separation )
{
    return fold_phase(p0 + Separation * (f1 + f2) / 2.0);
}
