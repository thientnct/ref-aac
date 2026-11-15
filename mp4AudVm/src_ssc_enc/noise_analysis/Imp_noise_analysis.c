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

Copyright  2001.

Source file: Imp_noise_analysis.c

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
 *   Module              : Noise_Analysis implementation
 *
 *   Description         : Implementation of the noise_analysis process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_NOISE_ANALYSIS_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "Imp_noise_analysis.h"

#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_Encoder_Modules.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"

#include "cexcept.h"

#include "SSC_Vector.h"
#include "win.h"

#include "Imp_Error.h"
#include "laguerre_analysis.h"
#include "LagFilter.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH    1
#define PIPE_AHEAD_LENGTH   3

#define THRESHOLD  500

/* define the filter parameters used for filtering the incoming noise signal */
#define filtDenLength  ( 2 )
static const Double filtDen[] = { 1.0   , -0.7    };

#define filtNumLength  ( 2 )
static const Double filtNum[] = { 0.8650, -0.8350 };

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error );
static SInt LOCAL_apar2lar( const Double* const A, const SInt N, Double* const lar );
static void LOCAL_lag2apar( const Double* const Lag, const SInt N, const Double Pole, Double* const A);

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* One time inialisation code to allocate/initialise tables */
void IMP_NOISE_ANALYSIS_Initialise(void)
{
}

/* Creates an instance of the noise_analysis module */

SSC_NOISE_ANALYSIS_STATE* IMP_NOISE_ANALYSIS_CreateInstance( const SSC_NOISE_ANALYSIS_INFO* const pNoise_AnalysisInfo, Char* szLogFile, SInt nTraceLevel )
{
    SSC_NOISE_ANALYSIS_STATE* pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;


    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try 
    {

        SInt        ch    = 0;

        if ( pNoise_AnalysisInfo == NULL )
        {
            fprintf(stdout,"\nError: Invalid pointer to noise_analysis info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_NOISE_ANALYSIS_STATE), 1, "Noise Analysis Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for noise_analysis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        /* all struct members are resetted*/
        pms->pLogState    = NULL;
        pms->pPipeState   = NULL;

        for ( ch = 0; ch < pNoise_AnalysisInfo->Channels; ch++)            
        {
            pms->pFilterState[ch]    = NULL;
            pms->pLagFilter[ch]      = NULL;

            pms->ppDataVector[ch]    = NULL;
            pms->prevDenVector[ch]   = NULL;

            pms->ppLagResidual[ch]  = NULL;
        }
        IMP_NOISE_ANALYSIS_Initialise();

        /* for debugging messages to be routed to file, here this log
         * objects is constructed*/
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
            if( cp != NULL)
            {
                *cp = '\0' ; /* strip extension, if found */
            }
            strncpy( li.szTitle, "SSC_noise_analysis", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLogState = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLogState == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
#endif
        SSCLOG_TRACE4( pms->pLogState, "SSCNOISE_ANALYSIS_CreateInstance\n" );

        /* + 1 because locally we use 1 bigger then in the sbunch
         * is stored( first position is always 1 so it is not
         * stored in sbunch */
        pms->denLen = pNoise_AnalysisInfo->DenOrder + 1;
        pms->pLarDenVector = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->denLen - 1 );
        if ( pms->pLarDenVector == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested LarDenVector vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pLarDenVector);
		
        pms->pAparDenVector = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->denLen);
        if ( pms->pAparDenVector == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested AparDenVector vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pAparDenVector);
		
        pms->pDenVector = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->denLen);
        if ( pms->pDenVector == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested DenVector vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pDenVector);
        pms->pDenVector[0] = (Double)1.0;

        /* + 1 because locally we use 1 bigger then in the sbunch
         * is stored( first position is always 1 so it is not
         * stored in sbunch */
        /* create the module pipe for a noise module */
        pms->pPipeState = SSCPIPE_InitInstance( pNoise_AnalysisInfo->Channels,
                                                pNoise_AnalysisInfo->SubframeSize,
                                                pNoise_AnalysisInfo->DecoderLevel,
                                                PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if( pms->pPipeState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCNOISE_ANALYSIS_INIT_PIPE_FAILED ;
        }

        pms->anaLength = 4 * pNoise_AnalysisInfo->SubframeSize;

        if ( ( pNoise_AnalysisInfo->Mode == SSC_MODE_LCSTEREO ) && ( pNoise_AnalysisInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels  = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels  = pNoise_AnalysisInfo->Channels ;
        }


        /* create / initialise variables that need to be stored
         * per channel and remembered over sf */
        for ( ch = 0; ch < pNoise_AnalysisInfo->Channels; ch++)
        {
            pms->pFilterState[ch] = SSCFILTER_CreateInstance( filtNum, filtNumLength, filtDen, filtDenLength );
            if ( pms->pFilterState[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested filterfor channel[%d]\n", ch);
                Throw SSCNOISE_ANALYSIS_INIT_FILTER_FAILED ;
            }

            pms->pLagFilter[ch] = SSCLAGFILTER_CreateInstance(pms->pDenVector, pms->denLen - 1, pNoise_AnalysisInfo->Pole);
            if ( pms->pLagFilter[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested filter for channel[%d]\n", ch);
                Throw SSCNOISE_ANALYSIS_INIT_INV_FILTER_FAILED ;
            }

            /* create a buffer for analysing : 
             * size is 4 times sfsize */
            pms->ppDataVector[ch] = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->anaLength);
            if ( pms->ppDataVector[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested DataVector vector for channel[%d]\n", ch);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* create a buffer for lag residual */ 
            pms->ppLagResidual[ch] = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->anaLength);
            if ( pms->ppLagResidual[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested LagResidual vector for channel[%d]\n", ch);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }

            /* create a buffer for saving den */
            pms->prevDenVector[ch] = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->denLen);
            if ( pms->prevDenVector[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested prevDenVector vector for channel[%d]\n", ch);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
            SSC_VECT_Clear(pms->prevDenVector[ch]);
            pms->prevDenVector[ch][0] = (Double)1.0;

        }

        /* create a Lag instance */
        pms->pLagState = SSC_LAG_CreateInstance(pNoise_AnalysisInfo->DenOrder, pNoise_AnalysisInfo->Pole,  pms->pLogState );
        if ( pms->pLagState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested lag instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE ;
        }


        memcpy(&pms->Info, pNoise_AnalysisInfo, sizeof(SSC_NOISE_ANALYSIS_INFO));

    } 
    Catch( Error ) 
    {
        pms = IMP_NOISE_ANALYSIS_DestroyInstance( pms );
    }

    return pms ;
}

/* Destroys the noise_analysis module instance pointed to by *ppState
 * and frees all the memory allocated on
 * IMP_NOISE_ANALYSIS_CreateInstance */
SSC_NOISE_ANALYSIS_STATE* IMP_NOISE_ANALYSIS_DestroyInstance( SSC_NOISE_ANALYSIS_STATE* const pms )
{
    SInt ch = 0;


    if( pms != NULL )
    {
        SSCLOG_TRACE4( pms->pLogState, "SSCNOISE_ANALYSIS_DestroyInstance\n" );

        if ( pms->pPipeState != NULL )
        {
            SSCPIPE_DestroyInstance( pms->pPipeState );
        }

        if ( pms->pLogState != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pms->pLogState );
            SSCLOG_FREE();
        }

        if ( pms->pLagState != NULL )
        {
            pms->pLagState = SSC_LAG_DestroyInstance( pms->pLagState );
        }

        /* destroy channel dependant structs */
        for ( ch = 0; ch < pms->Channels; ch++ )
        {
            if ( pms->pFilterState[ch] != NULL )
            {
                SSCFILTER_DestroyInstance ( pms->pFilterState[ch] );
            }

            if ( pms->pLagFilter[ch] != NULL )
            {
                SSCLAGFILTER_DestroyInstance ( pms->pLagFilter[ch] );
            }

            if ( pms->ppDataVector[ch] != NULL )
            {
                pms->ppDataVector[ch] = SSC_VECT_DestroyInstance( pms->ppDataVector[ch] );
            }

            if ( pms->prevDenVector[ch] != NULL )
            {
                pms->prevDenVector[ch] = SSC_VECT_DestroyInstance( pms->prevDenVector[ch] );
            }

            if ( pms->ppLagResidual[ch] != NULL )
            {
                pms->ppLagResidual[ch] = SSC_VECT_DestroyInstance( pms->ppLagResidual[ch] );
            }
        }

        if ( pms->pLarDenVector  != NULL )
        {
            pms->pLarDenVector = SSC_VECT_DestroyInstance( pms->pLarDenVector );
        }
        if ( pms->pAparDenVector  != NULL )
        {
            pms->pAparDenVector = SSC_VECT_DestroyInstance( pms->pAparDenVector );
        }
        if ( pms->pDenVector  != NULL )
        {
            pms->pDenVector = SSC_VECT_DestroyInstance( pms->pDenVector );
        }

        IMP_NOISE_ANALYSIS_Free();

        SSC_FREE( pms, "Noise Analysis Instance" );

    }
    else
    {
        fprintf( stdout, "\nError: IMP_NOISE_ANALYSIS_DestroyInstance invalid state pointer\n" );
    }

    return NULL;
}


/* Process current sbunch */
SSCBUNCH* IMP_NOISE_ANALYSIS_Process( SSC_NOISE_ANALYSIS_STATE* const pms, SSCBUNCH* const pSscWorkBunch )
{
    /* helper variable(s)*/
    SSCPIPE*   pPipe = NULL;
    SSCLOG*    pLog = NULL;


    SInt       SfSize   = 0;
    SInt       Channels = 0;
    SInt       ch       = 0;
    SInt       idx1     = 0;
    SInt       idx2     = 0;

    if ( pms != NULL )
    {
        SInt      SfCounter =  SSCBUNCH_GetSfCounter(SSCPIPE_GetAt(pms->pPipeState, PIPE_HIST_LENGTH));
        SSC_ERROR Error     = SSC_ERROR_OK ;

        pPipe    = pms->pPipeState;
        pLog     = pms->pLogState;
        Channels = pms->Channels;
        SfSize   = pms->Info.SubframeSize;

        /* put the new received sbunch in the pipe */
        SSCPIPE_Add(pPipe, pSscWorkBunch);

        /* first filter the incoming sbunches from residual and store
         * it in signal */
        if ( SSCBUNCH_GetDataFromStream( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH ) ) == True )
        {
            for ( ch = 0; ch < Channels; ch++)
            {
                SSCFILTER_Process( pms->pFilterState[ch],
                                   SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH ), ch),
                                   SSCBUNCH_GetSignal  ( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH ), ch),
                                   SfSize);
            }
        } 
        else 
        {
            for ( ch = 0; ch < Channels; ch++)
            {
                memset(SSCBUNCH_GetSignal( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH ), ch), 0, SfSize * sizeof(Double));
            }
        }


        /* shift the filtered signal 
         * into the 'work' buffer */
        for ( ch = 0; ch < Channels; ch++)
        {
            SSC_VECT_Move(pms->ppDataVector[ch], SfSize,
                          pms->ppDataVector[ch], 0,
                          pms->anaLength - SfSize);

            /* update the buffer with the new data */
            for ( idx1 = (pms->anaLength - SfSize), idx2 = 0; idx2 < SfSize; idx1++, idx2++)
            {
                pms->ppDataVector[ch][idx1] = (Double) SSCBUNCH_GetSignal( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH ), ch )[idx2];
            }
        }


        /* check first if subframe consists 
         * of valid data */
        if ( SSCBUNCH_GetDataFromStream( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH) ) == True )
        {
            SSCLOG_TRACE4( pLog, "*************sf%d\n", SSCBUNCH_GetSfCounter( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH)));

            /* Analysis is executed 
             * once every two subframes */
            if ( SfCounter % 2 == 1)
            {
                for ( ch = 0; ch < Channels; ch++)
                {
                    SInt i = 0;

                    Double absDataVal = 0;

                    SSC_EXCEPTION  tec;
                    SSC_EXCEPTION* the_exception_context = &tec;

                    Try {

                        /* check for enough signal(abs values) for the
                         * laguerre analysis; otherwise skip*/
                        SSC_VECT_SumAbs(pms->ppDataVector[ch], 0, 0, &absDataVal );

                        SSCLOG_TRACE4(pLog,"Sum absolute value: %.8e\n", absDataVal);
                        if ( (SInt)absDataVal <= THRESHOLD )
                        {
                            SSCLOG_TRACE3( pLog, "No abs signal present\n");
                            Throw SSCNOISE_ANALYSIS_NO_SIGNAL;
                        }
 
                        /* first clear */
                        SSC_VECT_Clear(pms->pDenVector);						

						/* Estimate the Laguerre coefficients */
                        SSC_LAG_Analysis(pms->pLagState,
                                         pms->ppDataVector[ch],
                                         pms->anaLength,
                                         pms->pDenVector );
						
						
                        /* first clear */
                        SSC_VECT_Clear(pms->ppLagResidual[ch]);
                        
                        SSCLAGFILTER_SetDenominator( pms->pLagFilter[ch], 
                                                     pms->pDenVector, 
                                                     pms->denLen-1 );
						
                        SSCLAGFILTER_Process( pms->pLagFilter[ch],
                                              pms->ppDataVector[ch],
                                              pms->ppLagResidual[ch],
                                              pms->anaLength);

                        /* LOG the found values for den*/
                        SSCLOG_TRACE4(pLog,"Den Coefs: " );
                        for ( i = 0; i < pms->denLen; i++)
                        {
                            SSCLOG_TRACE4(pLog,"%.8e ", pms->pDenVector[i]);
                        }
                        SSCLOG_TRACE4(pLog,"\n");

                        /* first clear */
                        SSC_VECT_Clear(pms->pLarDenVector);
                        SSC_VECT_Clear(pms->pAparDenVector);

                        /* Convert Laguerre parameters to Apar */
                        LOCAL_lag2apar(pms->pDenVector, pms->denLen-1, pms->Info.Pole, pms->pAparDenVector);

						
                        /* Convert APAR parameters to LAR */
                        i = LOCAL_apar2lar(pms->pAparDenVector, pms->denLen, pms->pLarDenVector);
                        if ( i == 1 )
                        {
                            SSCLOG_TRACE3( pLog, "Apar2Lar failed on Den\n");
                            Throw SSCNOISE_ANALYSIS_APAR2LAR_ERROR;
                        }
						
                        /* LOG the found values for den*/
                        SSCLOG_TRACE4(pLog,"Den LARS: " );
                        for ( i = 0; i < pms->denLen - 1; i++)
                        {
                            SSCLOG_TRACE4(pLog,"%.8e ", pms->pLarDenVector[i]);
                        }
                        SSCLOG_TRACE4(pLog,"\n");
                    } 
                    Catch( Error ) 
                    {
                        /* if no signal or reflection coefs > 1 reset */

                        SSC_VECT_Clear(pms->pDenVector);
                        pms->pDenVector[0] = (Double) 1.0;

                        SSC_VECT_Clear(pms->pLarDenVector);

                        /* at the moment all errors are no errors to
                         * stop encoding */
                        Error = SSC_ERROR_OK;
                    }

                    /* save the found Num/Den in prevNum/prevDen for
                     * calculation in the next subframe */
                    SSC_VECT_Copy( pms->pDenVector, 0, pms->prevDenVector[ch], 0, 0);   /* src , len, dst, len, count */

                    /* save the residual*/         
                    SSCBUNCH_SetResidual(SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), ch, &pms->ppLagResidual[ch][384]);
                    SSCBUNCH_SetResidual(SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH+1), ch, &pms->ppLagResidual[ch][2*384]);

                    /* save DENs */
                    SSCBUNCH_SetNoiseDen(SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), ch, pms->pLarDenVector);

                }
            }
        }

        /* Use LOCAL_SetError to set the error in the sbunch if an error occurs*/
        LOCAL_SetError( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), Error );
    }
    else
    {
        fprintf( stdout, "\nError: IMP_NOISE_ANALYSIS_Process invalid state pointer\n" );
    }

    /* Shift the processed subframe out
       note that this implies:
       - the sbunch pipe[PIPE_HIST_LENGTH] is shifted out + copied to HIST part of the pipe ( pipe[0] = pipe[1] )
       - pipe[1] = pipe[2]
       - pipe[2] = pipe[3]
       - pipe[3] = NULL */
    return SSCPIPE_Shift(pPipe);
}


/* One time uninitialise function */
void IMP_NOISE_ANALYSIS_Free(void)
{
}


/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* Sets the error code in the current sbunch */
static void LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error )
{
    if( SSCBUNCH_GetErrorOrigin( pSscBunch ) == False )
    {
        /* select the correct module define from the file SSC_Encoder_Modules.h */
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_NOISE_ANALYSIS_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* Convert polynomial to LAR coefficients if possible
 *
 * Parameters  : A     : Input: Pointer to polynomial
 *               Output: Pointer to LAR's, size N-1
 *               N     : Length of array
 *
 * Returns     : 0 if OK;
 *               1 if error */
static SInt LOCAL_apar2lar( const Double* const A, const SInt N, Double* const lar)
{
    Double   d[SSC_N_MAX_POL_ORDER];
    Double tmp[SSC_N_MAX_POL_ORDER];
    Double rfc[SSC_N_MAX_POL_ORDER];
    SInt m, k, i;

    if (N > 0)	
	{
    	assert (A[0] == 1 );

    	m = N - 1;
    	for( k = 0; k < m; k++ )
    	{
        	d[k] = -A[k + 1];
    	}
    	for( k = m - 1; k >= 0; k-- )
    	{
        	rfc[k] = d[k];
        	for( i = 0; i < k ; i++ )
        	{
            	tmp[i] = (d[i] + rfc[k] * d[k-i-1]) / (1 - rfc[k] * rfc[k]);
        	}
        	for( i = 0; i < k ; i++ )
        	{
            	d[i] = tmp[i];
        	}
        	rfc[k] = -rfc[k];
    	}

    	/* rc2lar */
    	for( k = 0; k < m; k++ )
    	{
        	if( ( rfc[k] >= 1) || ( rfc[k] <= -1))
        	{
            	/* Error situation; signal to reset everything and die */
            	return 1;
        	}
        	lar[k] = (Double) log ((1 + rfc[k]) / (1 - rfc[k]));
    	}
	}
    return 0;
}

/* Convert Laguerre parameters to polynomial coefficients 
 *
 * Parameters  : Lag   : Input: Pointer to polynomial
 *               A     : Pointer to Apar's, size N-1
 *               N     : Length of array
 *
 */
static void LOCAL_lag2apar( const Double* const Lag, const SInt N, const Double Pole, Double* const A)
{
    Double l;
    Double tmp;
	SInt k;

    l = (Double)sqrt((double)1.0-pow((double)Pole,(double)2.0));

    A[0]=(Double)1.0+ Pole*(-Lag[0]/l);

	for (k=1;k<N;k++)
	{
		A[k]=(-Lag[k-1]/l)+Pole*(-Lag[k]/l);
	}
	A[N]=(-Lag[N-1]/l);

	tmp=A[0];
	for (k=0;k<=N;k++)
	{
		A[k]=A[k]/tmp;
	}
}




