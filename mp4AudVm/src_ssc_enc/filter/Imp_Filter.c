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

Source file: Imp_Filter.c

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
 *   Module              : Filter implementation
 *
 *   Description         : Implementation of the filter process.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with IMP_FILTER_
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

#include "Filter.h"
#include "Imp_Filter.h"
#include "SSC_Filter.h"
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
static FILTERSETUP FilterSetup[] =
{
    {
        44100,
        5,
        2,
        { 9.996439371675712e-001, -9.996439371675712e-001 },  /* numerators */
        2,
        { 1.000000000000000e+000, -9.992878743351422e-001 },  /* denominators */
    },
} ;

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/
static void      LOCAL_SetError( SSCBUNCH *pSscBunch, SSC_ERROR Error );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_FILTER_Initialise(void)
{
}


/* Creates an instance of the filter module */
PFILTERSTATE IMP_FILTER_CreateInstance( PFILTERINFO pFilterInfo, Char *szLogFile, SInt nTraceLevel )
{
    PFILTERSTATE   pms = NULL ;
    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           i, nSetupIdx, nSetups ;

    SSC_ERROR Error = SSC_ERROR_OK ;

    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( pFilterInfo == NULL )
        {
            printf( "\nError: Invalid pointer to filter info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_FILTER_STATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for filter instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        if ( (pFilterInfo->Channels == 0) || (pFilterInfo->Channels > SSC_MAX_CHANNELS))
        {
            printf( "\nError: Invalid number of channels for filter instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (pFilterInfo->SubframeSize == 0) || pFilterInfo->SubframeSize > SSC_MAX_SUBFRAME_SAMPLES )
        {
            printf( "\nError: Invalid subframesize for filter instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms->Channels = pFilterInfo->Channels ;

        pms->pLog = NULL ;
        pms->pPipe = NULL ;
        memset( pms->pFilter, 0, sizeof(SSCFILTER*) * pFilterInfo->Channels );


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
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCFILTER_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( pFilterInfo->Channels,
                                           pFilterInfo->SubframeSize,
                                           pFilterInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCFILTER_INIT_PIPE_FAILED ;
        }
        nSetups = sizeof(FILTERSETUP)/sizeof(FilterSetup[0]);
        for ( nSetupIdx=0; nSetupIdx<nSetups; nSetupIdx++ )
        {
            if ( (FilterSetup[nSetupIdx].fs == pFilterInfo->fs) &&
                 (FilterSetup[nSetupIdx].CutOffFreq == pFilterInfo->CutOffFreq) )
            {
                break ;
            }
        }
        if ( nSetupIdx == nSetups )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Filter not supported for current settings.\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        for ( i=0; i<pFilterInfo->Channels; i++ )
        {
            pms->pFilter[i] = SSCFILTER_CreateInstance( FilterSetup[nSetupIdx].Numerator, FilterSetup[nSetupIdx].nCntNum,
                                                        FilterSetup[nSetupIdx].Denominator, FilterSetup[nSetupIdx].nCntDen );
            if ( pms->pFilter[i] == NULL )
            {
                SSCLOG_TRACE5(pms->pLog,"\nError: Filter init of channel %d failed.\n", i);
                Throw SSC_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    Catch( Error )
    {
        IMP_FILTER_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH *IMP_FILTER_Process( PFILTERSTATE pFilterState, SSCBUNCH *pSscWorkBunch )
{
    if ( pFilterState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;

        if ( 0 != SSCBUNCH_GetDataFromStream(pSscWorkBunch))
        {
            SInt nCh, nSfSize = pSscWorkBunch->nSubframeSize ;

            for ( nCh = 0; nCh < pFilterState->Channels; nCh++ )
            {
                /* copy data from residual to signal for next processes */
                SSCBUNCH_SetSignal (pSscWorkBunch, nCh, SSCBUNCH_GetResidual ( pSscWorkBunch, nCh ) );
            }
            for ( nCh = 0; nCh < pFilterState->Channels; nCh++ )
            {
                Double *pSrc = SSCBUNCH_GetSignal   ( pSscWorkBunch, nCh );
                Double *pDst = SSCBUNCH_GetResidual ( pSscWorkBunch, nCh );

                SSCFILTER_Process( pFilterState->pFilter[nCh], pSrc, pDst, nSfSize );
            }
        }

        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_FILTER_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the filter module instance pointed to by *ppState and
 * frees all the memory allocated on IMP_FILTER_CreateInstance */
void IMP_FILTER_DestroyInstance( PFILTERSTATE *ppFilterState )
{
    SInt i ;
    PFILTERSTATE pFilterState = *ppFilterState ;

    if ( pFilterState != NULL )
    {
        SSCLOG_TRACE4( pFilterState->pLog, "SSCFILTER_DestroyInstance\n" );

        for ( i=0; i<pFilterState->Channels; i++ )
        {
            SSCFILTER_DestroyInstance( pFilterState->pFilter[i] );
        }
        SSCPIPE_DestroyInstance( pFilterState->pPipe );
        if( pFilterState->pLog != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pFilterState->pLog );
            SSCLOG_FREE();
        }

        SSC_FREE( pFilterState, "" );
        *ppFilterState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_FILTER_DestroyInstance invalid state pointer\n" );
    }
}

/* One time uninitialise function */
void IMP_FILTER_Free(void)
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
        /* select the correct module define from the file
         * SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_FILTER_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}
