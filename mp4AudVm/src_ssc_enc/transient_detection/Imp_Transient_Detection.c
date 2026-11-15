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

Source file: Imp_Transient_Detection.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : TransientDetection implementation
 *
 *   Description         : Implementation of the transientdetection process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_TRANSIENTDETECTION_
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
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"
#include "SSC_System.h"

#include "Imp_Transient_Detection.h"
#include "SSC_Transient_Detection.h"
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

static void      LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );
static void      LOCAL_HandleError( PTRANSIENTDETECTIONSTATE pTDS, SSCBUNCH *pSscBunch, SInt Error, Char *szFunction );
static Double    LOCAL_Max( Double dValue1, Double dValue2 );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_TRANSIENTDETECTION_Initialise(void)
{
}


/* Creates an instance of the transientdetection module */
PTRANSIENTDETECTIONSTATE IMP_TRANSIENTDETECTION_CreateInstance( PTRANSIENTDETECTIONINFO pTDI, Char *szLogFile, SInt nTraceLevel )
{
    PTRANSIENTDETECTIONSTATE pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        SInt i;

        if ( pTDI == NULL )
        {
            printf("\nError: Invalid pointer to transientdetection info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_TRANSIENTDETECTION_STATE), 1, "" );
        if ( pms == NULL )
        {
            printf("\nError: Unable to alloc requested memory for transientdetection instance\n");
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
            strncpy( li.szTitle, "SSC_filter", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;

            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf("\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCTRANSIENTDETECTION_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( pTDI->Channels,
                                           pTDI->SubframeSize,
                                           pTDI->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCTRANSIENTDETECTION_INIT_PIPE_FAILED ;
        }
        pms->nChannels           = pTDI->Channels ;
        pms->Tau                 = pTDI->Tau ;
        pms->Threshold           = pTDI->Threshold ;
        pms->attack_duration     = pTDI->attack_duration ;
        for( i=0; i<SSC_MAX_CHANNELS; i++ )
        {
            pms->yPrev[i]             = pTDI->yPrev[i] ;
            pms->uPrev[i]             = pTDI->uPrev[i] ;
            pms->attack_count_down[i] = pTDI->attack_count_down[i] ;
        }
        if( pms->attack_duration < pTDI->SubframeSize )
        {
            SSCLOG_TRACE5( pms->pLog, "Attach duration may not be less than subframe size !\n" );
            Throw( SSC_ERROR_INVALID_PARAMETER );
        }

    }
    Catch( Error )
    {
        IMP_TRANSIENTDETECTION_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH *IMP_TRANSIENTDETECTION_Process( PTRANSIENTDETECTIONSTATE pTDS, SSCBUNCH *pSscBunch )
{
    if ( pTDS != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSC_EXCEPTION  tec;
        SSC_EXCEPTION *the_exception_context = &tec;

        SSCPIPE_Add( pTDS->pPipe, pSscBunch );

        Try
        {
            SInt nSfSize = pSscBunch->nSubframeSize ;

            if ( True == SSCBUNCH_GetDataFromStream(pSscBunch))
            {
                SInt   n, nCh ;
                Double y, u ;

                for ( nCh = 0; nCh < pTDS->nChannels; nCh++ )
                {
                    /* input data for a process always in residual buffer */
                    Double *pSrc = SSCBUNCH_GetResidual ( pSscBunch, nCh );

                    /* check for transient position */
                    for ( n=0; n<nSfSize; n++ )
                    {
                        y = LOCAL_Max( fabs( pSrc[n] ), pTDS->Tau * pTDS->yPrev[nCh] );
                        u = (1-pTDS->Tau)*y + pTDS->Tau * pTDS->uPrev[nCh] ;

                        if( pTDS->attack_count_down[nCh] > 0 )
                        {
                            pTDS->attack_count_down[nCh]-- ;
                        }
                        else
                        {
                            if( y > (pTDS->Threshold * u) )
                            {
                                SSCLOG_TRACE3( pTDS->pLog, "Transient found in sf %d at pos: %d, channel %d\n",
                                                            SSCBUNCH_GetSfCounter( pSscBunch), n, nCh );

                                pTDS->attack_count_down[nCh] = pTDS->attack_duration ;

                                SSCBUNCH_SetTransientPresent( pSscBunch, nCh, True );
                                SSCBUNCH_SetTransientPos    ( pSscBunch, nCh, n    );
                            }
                        }
                        pTDS->yPrev[nCh] = y ;
                        pTDS->uPrev[nCh] = u ;
                    }
                }
            }
        }
        Catch(Error)
        {
            LOCAL_HandleError( pTDS, pSscBunch, Error, "process" );
        }
        LOCAL_SetError( pSscBunch, Error );
        pSscBunch = SSCPIPE_Shift( pTDS->pPipe );
    }
    else
    {
        printf( "\nError: IMP_TRANSIENTDETECTION_Process invalid state pointer\n" );
    }
    return pSscBunch ;
}


/* Destroys the transientdetection module instance pointed to by
 * *ppState and frees all the memory allocated on
 * IMP_TRANSIENTDETECTION_CreateInstance */
void IMP_TRANSIENTDETECTION_DestroyInstance( PTRANSIENTDETECTIONSTATE *ppTDS )
{
    PTRANSIENTDETECTIONSTATE pTDS = *ppTDS ;

    if ( pTDS != NULL )
    {
        SSCLOG_TRACE4( pTDS->pLog, "SSCTRANSIENTDETECTION_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( pTDS->pPipe );
        if( pTDS->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pTDS->pLog );
            SSCLOG_FREE();
        }

        SSC_FREE( pTDS, "" );
        *ppTDS = NULL;
    }
    else
    {
        printf( "\nError: IMP_TRANSIENTDETECTION_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_TRANSIENTDETECTION_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == 0 )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_TRAN_DETECTION_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}



static void LOCAL_HandleError( PTRANSIENTDETECTIONSTATE pTDS, SSCBUNCH *pSscBunch, SInt Error, Char *szFunction )
{
    SSCLOG *pLog = NULL ;
    SInt    nSf  = -1 ;

    UNREFERENCED_PARM(szFunction);
    LOCAL_SetError( pSscBunch, Error );
    if ( pTDS != NULL )
    {
        pLog = pTDS->pLog ;
    }
    if ( (pSscBunch != NULL)  && (pSscBunch->pBunch != NULL) )
    {
        nSf = SSCBUNCH_GetSfCounter( pSscBunch );
    }
    if ( pLog != NULL )
    {
        SSCLOG_TRACE5( pLog, "Transientdetection '%s': error %d occurred in sf %d\n", szFunction, Error, nSf );
    }
    else
    {
        assert( False );
    }
}


static Double LOCAL_Max( Double dValue1, Double dValue2 )
{
    if ( dValue1 < dValue2 )
    {
        return dValue2 ;
    }
    else
    {
        return dValue1 ;
    }
}

