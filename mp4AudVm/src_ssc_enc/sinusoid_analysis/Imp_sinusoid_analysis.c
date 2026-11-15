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

Source file: IMP_sinusoid_analysis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
28-Sep-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : sinusoid_analysis implementation
 *
 *   Description         : Implementation of the sinusoid_analysis process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_ANALYSIS_
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

#include "Imp_sinusoid_analysis.h"
#include "SSC_sinusoid_analysis.h"
#include "Imp_Error.h"

#include "cexcept.h"

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
static void LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static void LOCAL_SinusAnalysis( PSINUSOID_ANALYSISSTATE pSAS );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */

void IMP_SINUSOID_ANALYSIS_Initialise(void)
{
}


/* Creates an instance of the sinusoid_analysis module */
PSINUSOID_ANALYSISSTATE IMP_SINUSOID_ANALYSIS_CreateInstance( PSINUSOID_ANALYSISINFO psinusoid_analysisInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_ANALYSISSTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        SInt i, nPipeLength ;

        if ( psinusoid_analysisInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_analysis info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_ANALYSISSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_analysis instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_analysis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_ANALYSIS_CreateInstance\n" );

        nPipeLength = 2 * (SInt)ceil((Double)psinusoid_analysisInfo->analysis_length / (2.0 * psinusoid_analysisInfo->SubframeSize));
        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_analysisInfo->Channels,
                                           psinusoid_analysisInfo->SubframeSize,
                                           psinusoid_analysisInfo->DecoderLevel,
                                           nPipeLength/2-1, nPipeLength/2+1, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", nPipeLength);
            Throw SSCSINUSOID_ANALYSIS_INIT_PIPE_FAILED ;
        }

        if ( ( psinusoid_analysisInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_analysisInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels    = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels    = psinusoid_analysisInfo->Channels ;
        }

        pms->nSinusoids      = psinusoid_analysisInfo->nSinusoids ;
        pms->Fs              = psinusoid_analysisInfo->Fs ;
        pms->freq_limit      = psinusoid_analysisInfo->freq_limit ;
        pms->sf_size         = psinusoid_analysisInfo->SubframeSize ;
        pms->analysis_length = psinusoid_analysisInfo->analysis_length ;
        pms->analysis_offset = (SInt)floor((nPipeLength * pms->sf_size - pms->analysis_length) / 2);
        pms->pData           = SSC_CALLOC( nPipeLength * pms->sf_size * sizeof(*pms->pData), 1, "" );
        pms->pWindow         = SSC_CALLOC( pms->analysis_length/2 * sizeof(Double), 1, "" );
        for( i=0; i<pms->analysis_length/2; i++ )
        {
            pms->pWindow[i] = .5*(1 - cos(2*SSC_PI*(i+1) / (pms->analysis_length/2+1)));
        }
        pms->pHiresFft = SSC_HIRES_FFT_CreateInstance( pms->analysis_length, pms->nSinusoids, pms->Fs, pms->freq_limit );
    }
    Catch( Error )
    {
        IMP_SINUSOID_ANALYSIS_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_ANALYSIS_Process( PSINUSOID_ANALYSISSTATE psinusoid_analysisState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_analysisState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( psinusoid_analysisState->pPipe, pSscWorkBunch );

        LOCAL_SinusAnalysis( psinusoid_analysisState );

        pSscWorkBunch = SSCPIPE_Shift( psinusoid_analysisState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_ANALYSIS_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_analysis module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_SINUSOID_ANALYSIS_CreateInstance */
void IMP_SINUSOID_ANALYSIS_DestroyInstance( PSINUSOID_ANALYSISSTATE* ppsinusoid_analysisState )
{
    PSINUSOID_ANALYSISSTATE psinusoid_analysisState = *ppsinusoid_analysisState ;

    if( psinusoid_analysisState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_analysisState->pLog, "SSCSINUSOID_ANALYSIS_DestroyInstance\n" );

        SSC_FREE( psinusoid_analysisState->pData, "" );
        SSC_FREE( psinusoid_analysisState->pWindow, "" );
        SSC_HIRES_FFT_DestroyInstance( &psinusoid_analysisState->pHiresFft );

        SSCPIPE_DestroyInstance( psinusoid_analysisState->pPipe );
        if( psinusoid_analysisState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_analysisState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_analysisState, "" );
        *ppsinusoid_analysisState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_ANALYSIS_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function*/
void IMP_SINUSOID_ANALYSIS_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_ANALYSIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


static void LOCAL_SinusAnalysis( PSINUSOID_ANALYSISSTATE pSAS )
{
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    Try
    {
        SInt      nCh ;
        SSCPIPE  *pPipe = pSAS->pPipe ;
        SSCBUNCH *pCurr = SSCPIPE_GetAt( pPipe, SSCPIPE_GetHistItemCount( pPipe ));

        if( SSCBUNCH_GetDataFromStream( pCurr ) == 1 )
        {
            for( nCh = 0; nCh < pSAS->Channels; nCh++ )
            {
                SInt i, nIdx ;
                Double *pData     = pSAS->pData ;
                Double *pFreq     = SSCBUNCH_GetSinusoidFreq( pCurr, nCh );
                Double *pAmp      = SSCBUNCH_GetSinusoidAmp ( pCurr, nCh );
                Double *pPhase    = SSCBUNCH_GetSinusoidPhaseOrg( pCurr, nCh );
                Double *pTrackIdx = SSCBUNCH_GetSinusoidTrackIdx( pCurr, nCh );

                for( nIdx = 0; nIdx < SSCPIPE_GetItemCount( pPipe ); nIdx++ )
                {
                    memcpy( &pData[nIdx*pSAS->sf_size], SSCBUNCH_GetResidual( SSCPIPE_GetAt( pPipe, nIdx), nCh ), pSAS->sf_size * sizeof(Double));
                }
                SSC_HIRES_FFT_Process( pSAS->pHiresFft, &pData[pSAS->analysis_offset], pFreq, pAmp, pPhase );
                for( i=0; i<pSAS->nSinusoids; i++ )
                {
                    if( pAmp[i] > 0 )
                    {
                        pTrackIdx[i] = 1 ;
                        if( pAmp[i] > 32767 )
                        {
                            pAmp[i] = 32767 ;
                        }
                    }
                    else
                    {
                        pTrackIdx[i] = 0 ;
                    }
                }
            }
        }
    }
    Catch(Error)
    {
        SSCLOG_TRACE5( pSAS->pLog, "SSCSINUSOID_ANALYSIS error %d\n", Error );
    }
}
