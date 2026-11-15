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

Source file: IMP_sinusoid_remove.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
06-Sep-2001	JD	Initial version
20-Sep-2002 NT  RM2 change
************************************************************************/



/******************************************************************************
 *
 *   Module              : sinusoid_remove implementation
 *
 *   Description         : Implementation of the sinusoid_remove process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_SINUSOID_REMOVE_
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

#include "Imp_sinusoid_remove.h"
#include "SSC_sinusoid_remove.h"
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
static void LOCAL_Sinusoid_remove( PSINUSOID_REMOVESTATE pSRS,
                                          SSCBUNCH *pSscBunch );


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* One time inialisation code to allocate/initialise tables */
void IMP_SINUSOID_REMOVE_Initialise(void)
{
}


/* Creates an instance of the sinusoid_remove module */
PSINUSOID_REMOVESTATE IMP_SINUSOID_REMOVE_CreateInstance( PSINUSOID_REMOVEINFO psinusoid_removeInfo, Char *szLogFile, SInt nTraceLevel )
{
    PSINUSOID_REMOVESTATE   pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try
    {
        if ( psinusoid_removeInfo == NULL )
        {
            printf( "\nError: Invalid pointer to sinusoid_remove info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pms = SSC_CALLOC( sizeof(SSC_SINUSOID_REMOVESTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for sinusoid_remove instance\n");
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
            strncpy( li.szTitle, "SSC_sinusoid_remove", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLog = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLog == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
        #endif
        SSCLOG_TRACE4( pms->pLog, "SSCSINUSOID_REMOVE_CreateInstance\n" );

        /* create the module pipe (default 0 hist, 1 ahead length) */
        pms->pPipe = SSCPIPE_InitInstance( psinusoid_removeInfo->Channels,
                                           psinusoid_removeInfo->SubframeSize,
                                           psinusoid_removeInfo->DecoderLevel,
                                           PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if ( pms->pPipe == NULL )
        {
            SSCLOG_TRACE5(pms->pLog,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCSINUSOID_REMOVE_INIT_PIPE_FAILED ;
        }

        if ( ( psinusoid_removeInfo->Mode == SSC_MODE_LCSTEREO ) && ( psinusoid_removeInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels    = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels    = psinusoid_removeInfo->Channels ;
        }
        pms->Rel_Threshold = psinusoid_removeInfo->Rel_Threshold ;
    }
    Catch( Error )
    {
        IMP_SINUSOID_REMOVE_DestroyInstance( &pms );
    }
    return pms ;
}


/* Process current sbunch */
SSCBUNCH* IMP_SINUSOID_REMOVE_Process( PSINUSOID_REMOVESTATE psinusoid_removeState, SSCBUNCH* pSscWorkBunch )
{
    if ( psinusoid_removeState != NULL )
    {
        SSC_ERROR Error = SSC_ERROR_OK ;
        SSCPIPE_Add( psinusoid_removeState->pPipe, pSscWorkBunch );

        /* Use LOCAL_SetError to set the error in the sbunch */
        LOCAL_Sinusoid_remove( psinusoid_removeState, pSscWorkBunch );

        pSscWorkBunch = SSCPIPE_Shift( psinusoid_removeState->pPipe );
        /* report error immidiate */
        LOCAL_SetError( pSscWorkBunch, Error );
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_REMOVE_Process invalid state pointer\n" );
    }
    return pSscWorkBunch ;
}


/* Destroys the sinusoid_remove module instance pointed to by *ppState
 * and frees all the memory allocated on
 * IMP_SINUSOID_REMOVE_CreateInstance */
void IMP_SINUSOID_REMOVE_DestroyInstance( PSINUSOID_REMOVESTATE* ppsinusoid_removeState )
{
    PSINUSOID_REMOVESTATE psinusoid_removeState = *ppsinusoid_removeState ;

    if( psinusoid_removeState != NULL )
    {
        SSCLOG_TRACE4( psinusoid_removeState->pLog, "SSCSINUSOID_REMOVE_DestroyInstance\n" );


        SSCPIPE_DestroyInstance( psinusoid_removeState->pPipe );
        if( psinusoid_removeState->pLog != NULL )
        {
          SSCLOG_DESTROYINSTANCE ( psinusoid_removeState->pLog );
          SSCLOG_FREE();
        }
        SSC_FREE( psinusoid_removeState, "" );
        *ppsinusoid_removeState = NULL ;
    }
    else
    {
        printf( "\nError: IMP_SINUSOID_REMOVE_DestroyInstance invalid state pointer\n" );
    }
}


/* One time uninitialise function */
void IMP_SINUSOID_REMOVE_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_SIN_REMOVE_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* Implementation of remove sinusoids */
static void LOCAL_Sinusoid_remove( PSINUSOID_REMOVESTATE pSRS, SSCBUNCH *pSscBunch )
{
    if( SSCBUNCH_GetDataFromStream( pSscBunch ) == 1 )
    {
        SInt nCh, nSin ;

        for( nCh = 0; nCh < pSRS->Channels; nCh++ )
        {
            Double *pFreq   = SSCBUNCH_GetSinusoidFreq     ( pSscBunch, nCh );
            Double *pAmp    = SSCBUNCH_GetSinusoidAmp      ( pSscBunch, nCh );
            Double *pPhOrg  = SSCBUNCH_GetSinusoidPhaseOrg ( pSscBunch, nCh );
            Double *pPhCont = SSCBUNCH_GetSinusoidPhaseCont( pSscBunch, nCh );
            Double *pRel    = SSCBUNCH_GetSinusoidRelevance( pSscBunch, nCh );
            Double *pTidx   = SSCBUNCH_GetSinusoidTrackIdx ( pSscBunch, nCh );
            Double *pTprev  = SSCBUNCH_GetSinusoidTrackPrev( pSscBunch, nCh );
            Double *pTnext  = SSCBUNCH_GetSinusoidTrackNext( pSscBunch, nCh );

            for( nSin = 0; nSin < SSC_ENCODER_MAX_SINS; nSin++ )
            {
                if ((pRel[nSin] < pSRS->Rel_Threshold) && (pTidx[nSin] > 0))
                {
                    pFreq  [nSin] = 0.0 ;
                    pAmp   [nSin] = 0.0 ;
                    pPhOrg [nSin] = 0.0 ;
                    pPhCont[nSin] = 0.0 ;
                    pRel   [nSin] = 0.0 ;
                    pTidx  [nSin] = 0.0 ;
                    pTprev [nSin] = -1.0;
                    pTnext [nSin] = -1.0;
                }
            }
        }
    }
}
