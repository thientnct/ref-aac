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

Source file: IMP_sinusoid_tracking.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
20-Sep-2002 NT      RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : sinusoid_tracking implementation
 *
 *   Description         : Implementation of the sinusoid_tracking process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_TRACKING_
 *
 *
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

#include "Imp_sinusoid_tracking.h"
#include "SSC_sinusoid_tracking.h"
#include "Imp_Error.h"
#include "Imp_quantise.h"

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

static void      LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static void      LOCAL_Sinusoid_tracking_process( PSINUSOID_TRACKINGSTATE pSTS );
static Double    LOCAL_CalcQ( Double dCurFreq, Double dPrevFreq,
                              Double dCurAmp,  Double dPrevAmp,
                              PSINUSOID_TRACKINGSTATE pSTS );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_TRACKING_Initialise(void)
{
}


/* Creates an instance of the sinusoid_tracking module */
PSINUSOID_TRACKINGSTATE IMP_SINUSOID_TRACKING_CreateInstance( PSINUSOID_TRACKINGINFO psinusoid_trackingInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_TRACKINGSTATE pms = NULL ;
    SSC_EXCEPTION           tec = SSC_INIT_EXCEPTION ;
    SSC_EXCEPTION*          the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( psinusoid_trackingInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_tracking info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_TRACKINGSTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_tracking instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_tracking", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_TRACKING_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_trackingInfo->Channels,
                                           psinusoid_trackingInfo->SubframeSize,
                                           psinusoid_trackingInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_TRACKING_INIT_PIPE_FAILED ;
        }

        if ( ( psinusoid_trackingInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_trackingInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->nChannels    = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->nChannels    = psinusoid_trackingInfo->Channels ;
        }
        pms->fs                = psinusoid_trackingInfo->fs ;
        pms->dDeltaFreqMax     = psinusoid_trackingInfo->dDeltaFreqMax ;
        pms->dAmpRatioMin      = psinusoid_trackingInfo->dAmpRatioMin ;
        pms->dAmpRatioMax      = psinusoid_trackingInfo->dAmpRatioMax ;
        pms->dQuanDeltaFreqMax = psinusoid_trackingInfo->dQuanDeltaFreqMax ;
        pms->dQuanAmpRatioMin  = psinusoid_trackingInfo->dQuanAmpRatioMin ;
        pms->dQuanAmpRatioMax  = psinusoid_trackingInfo->dQuanAmpRatioMax ;
    }
    Catch( Error )
    {
        IMP_SINUSOID_TRACKING_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_TRACKING_Process( PSINUSOID_TRACKINGSTATE psinusoid_trackingState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_trackingState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( psinusoid_trackingState->pPipe, pSscWorkBunch );

        LOCAL_Sinusoid_tracking_process( psinusoid_trackingState );

        /* Use LOCAL_SetError to set the error in the sbunch */
        pSscWorkBunch = SSCPIPE_Shift( psinusoid_trackingState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_TRACKING_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_tracking module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_SINUSOID_TRACKING_CreateInstance */
void IMP_SINUSOID_TRACKING_DestroyInstance( PSINUSOID_TRACKINGSTATE* ppsinusoid_trackingState )
{
    PSINUSOID_TRACKINGSTATE psinusoid_trackingState = *ppsinusoid_trackingState ;

    if( psinusoid_trackingState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_trackingState->pLog, "SSCSINUSOID_TRACKING_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_trackingState->pPipe );
        if( psinusoid_trackingState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_trackingState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_trackingState, "" );
        *ppsinusoid_trackingState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_TRACKING_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_TRACKING_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_TRACKING_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* Local implementation function of sinusoidal tracking */
static void LOCAL_Sinusoid_tracking_process( PSINUSOID_TRACKINGSTATE pSTS )
{
    SSCBUNCH *pPrevBunch = SSCPIPE_GetAt( pSTS->pPipe, PIPE_AHEAD_LENGTH-2 );
    SSCBUNCH *pCurBunch  = SSCPIPE_GetAt( pSTS->pPipe, PIPE_AHEAD_LENGTH-1 );

    if( (SSCBUNCH_GetDataFromStream( pPrevBunch ) == True) &&
        (SSCBUNCH_GetDataFromStream( pCurBunch  ) == True) )
    {
        SInt nCh ;

        for( nCh = 0; nCh < pSTS->nChannels; nCh++ )
        {
            Double *pdPrevAmp       = SSCBUNCH_GetSinusoidAmp      ( pPrevBunch, nCh );
            Double *pdPrevFreq      = SSCBUNCH_GetSinusoidFreq     ( pPrevBunch, nCh );
            Double *pdPrevTrackIdx  = SSCBUNCH_GetSinusoidTrackIdx ( pPrevBunch, nCh );
            Double *pdPrevTrackNext = SSCBUNCH_GetSinusoidTrackNext( pPrevBunch, nCh );
            Double *pdCurAmp        = SSCBUNCH_GetSinusoidAmp      ( pCurBunch,  nCh );
            Double *pdCurFreq       = SSCBUNCH_GetSinusoidFreq     ( pCurBunch,  nCh );
            Double *pdCurTrackIdx   = SSCBUNCH_GetSinusoidTrackIdx ( pCurBunch,  nCh );
            Double *pdCurTrackPrev  = SSCBUNCH_GetSinusoidTrackPrev( pCurBunch,  nCh );

            SInt nCurTrack ;

            for( nCurTrack = 0; nCurTrack < SSC_ENCODER_MAX_SINS; nCurTrack++ )
            {
                if( pdCurTrackIdx[nCurTrack] > 0 ) /* trackidx contains tracklength */
                {
                    /* nCurTrack points to a sinusoid, try to find the best */
                    /* match from the previous subframe */
                    Double dCurAmp    = pdCurAmp [nCurTrack];
                    Double dCurFreq   = pdCurFreq[nCurTrack] ;
                    Double dBestQ     = 0.0 ;
                    SInt   nBestPred  = -1 ;
                    SInt   nPrevTrack ;

                    for( nPrevTrack = 0; nPrevTrack < SSC_ENCODER_MAX_SINS; nPrevTrack++ )
                    {
                        if( (pdPrevTrackIdx[nPrevTrack] > 0) && (pdPrevTrackNext[nPrevTrack] < 0) )
                        {
                            /* nPrevTrack points to a sinusoid and is not linked yet, */
                            /* check if it is a better match */
                            Double q = LOCAL_CalcQ( dCurFreq, pdPrevFreq[nPrevTrack],
                                                    dCurAmp,  pdPrevAmp [nPrevTrack], pSTS );

                            if( q > dBestQ )
                            {
                                dBestQ    = q ;
                                nBestPred = nPrevTrack ;
                            }
                        }
                    }
                    if( nBestPred >= 0 )
                    {
                        /* update track info */
                        pdCurTrackPrev [nCurTrack] = nBestPred ;
                        pdPrevTrackNext[nBestPred] = nCurTrack ;
                        pdCurTrackIdx  [nCurTrack] = pdPrevTrackIdx[nBestPred] + 1 ;
                    }
                }
            }
        }
    }
}


/* Local implementation of track match function */
static Double LOCAL_CalcQ( Double dCurFreq, Double dPrevFreq,
                           Double dCurAmp,  Double dPrevAmp,
                           PSINUSOID_TRACKINGSTATE pSTS )
{
    Double dQresult     = -1.0 ;
    Double dCurFreqErb  = Freq2erb( dCurFreq  * pSTS->fs / (2 * SSC_PI));
    Double dPrevFreqErb = Freq2erb( dPrevFreq * pSTS->fs / (2 * SSC_PI));
    Double dAmpRatio    = dCurAmp / dPrevAmp ;

    if( (fabs( dCurFreqErb - dPrevFreqErb ) < pSTS->dQuanDeltaFreqMax) &&
        (dAmpRatio > pSTS->dQuanAmpRatioMin) && (dAmpRatio < pSTS->dQuanAmpRatioMax ) )
    {
        /* within quantisation limits */
        Double dFreqRatio = fabs(dCurFreq-dPrevFreq) / dPrevFreq ;

        if( (dFreqRatio < pSTS->dDeltaFreqMax) && (dAmpRatio > pSTS->dAmpRatioMin) && (dAmpRatio < pSTS->dAmpRatioMax) )
        {
            Double q2 ;
            Double q1 = 1.0 - dFreqRatio/pSTS->dDeltaFreqMax ;

            if( dAmpRatio < 1 )
            {
                q2 = (     dAmpRatio  - pSTS->dAmpRatioMin)/(1 - pSTS->dAmpRatioMin);
            }
            else
            {
                q2 = ((1.0/dAmpRatio) - pSTS->dAmpRatioMin)/(1 - pSTS->dAmpRatioMin);
            }
            dQresult = q1 * q2 ;
        }
    }
    return dQresult ;
}


