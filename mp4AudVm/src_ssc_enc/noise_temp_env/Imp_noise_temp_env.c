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

Source file: Imp_noise_temp_env.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
18-Sep-2002	NT	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : Noise_Temp_Env implementation
 *
 *   Description         : Implementation of the noise_temp_env process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with IMP_NOISE_TEMP_ENV_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "Imp_noise_temp_env.h"

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

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define PIPE_HIST_LENGTH    0
#define PIPE_AHEAD_LENGTH   6

#define MAXORDER  20
#define	NBIS	   4		        /* number of bisections          */
#define	DW		   (0.01 * SSC_PI)	/* resolution for initial search */

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static void   LOCAL_SetError( SSCBUNCH* pSscBunch, SSC_ERROR Error );
static void   LOCAL_Freqz(Double* a, SInt aLen, Double* pResult, SInt Length);
static Double LOCAL_standard_dev( Double* const X,SInt N);
static SInt   LOCAL_pc2lsf(const Double pc[], Double lsf[], SInt np);
static Double LOCAL_FNevChebP(Double x, const Double c[], SInt n);



/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/* One time inialisation code to allocate/initialise tables */
void IMP_NOISE_TEMP_ENV_Initialise(void)
{
}

/* Creates an instance of the noise_temp_env module */

SSC_NOISE_TEMP_ENV_STATE* IMP_NOISE_TEMP_ENV_CreateInstance( const SSC_NOISE_TEMP_ENV_INFO* const pNoise_Temp_EnvInfo, Char* szLogFile, SInt nTraceLevel )
{
    SSC_NOISE_TEMP_ENV_STATE* pms = NULL ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;


    SSC_ERROR Error = SSC_ERROR_OK ;
    UNREFERENCED_PARM(szLogFile);
    UNREFERENCED_PARM(nTraceLevel);
    Try {

        SInt        ch    = 0;
        SSC_WIN_STATE* pWinState = NULL;

        if ( pNoise_Temp_EnvInfo == NULL )
        {
            fprintf(stdout,"\nError: Invalid pointer to noise_temp_env info structure\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_NOISE_TEMP_ENV_STATE), 1, "Noise Temp_Env Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for noise_temp_env instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        /* all struct members are resetted*/
        pms->pLogState     = NULL;
        pms->pPipeState    = NULL;
        pms->pWindow       = NULL;
        pms->pHelpBuffer   = NULL;
        pms->pFreqLpcState = NULL;
        pms->pEnvCoefs     = NULL;
        pms->pLsfEnvCoefs  = NULL;
        pms->pWinDataVector= NULL;
        pms->pEnvVector    = NULL;
        for ( ch = 0; ch < pNoise_Temp_EnvInfo->Channels; ch++)
        {
            pms->ppDataVector[ch] = NULL;
        }

        IMP_NOISE_TEMP_ENV_Initialise();

        /* for debugging messages can be routed to file, here this log
         * objects is contructed*/
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
            strncpy( li.szTitle, "SSC_noise_temp_env", sizeof(li.szTitle)/sizeof(Char) - 1 );
            li.szTitle[sizeof(li.szTitle)/sizeof(Char) - 1] = '\0' ;
            pms->pLogState = SSCLOG_CREATEINSTANCE( &li );
            if ( pms->pLogState == NULL )
            {
                printf( "\nWarning: Unable to create requested(%s) log file\n", szLogFile);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }
#endif
        SSCLOG_TRACE4( pms->pLogState, "SSCNOISE_TEMP_ENV_CreateInstance\n" );

        /* create the module pipe for the noise_temp_env module*/
        pms->pPipeState = SSCPIPE_InitInstance( pNoise_Temp_EnvInfo->Channels,
                                                pNoise_Temp_EnvInfo->SubframeSize,
                                                pNoise_Temp_EnvInfo->DecoderLevel,
                                                PIPE_HIST_LENGTH, PIPE_AHEAD_LENGTH, NULL );

        if( pms->pPipeState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested pipe with lenght:%d\n", PIPE_AHEAD_LENGTH);
            Throw SSCNOISE_TEMP_ENV_INIT_PIPE_FAILED ;
        }

        pms->anaLength    = 6 * pNoise_Temp_EnvInfo->SubframeSize; 
        pms->envLength    = (4*pNoise_Temp_EnvInfo->SubframeSize)*(4/3);
        pms->hanWinLength = pms->envLength/2;
        pms->envOrder     = pNoise_Temp_EnvInfo->EnvOrder;

        if ( ( pNoise_Temp_EnvInfo->Mode == SSC_MODE_LCSTEREO ) && ( pNoise_Temp_EnvInfo->ModeExt == SSC_MODE_EXT_PAR_STEREO ) )
        {
            pms->Channels  = 1 ; /* we then only need to process a single channel */
        }
        else
        {
            pms->Channels  = pNoise_Temp_EnvInfo->Channels ;
        }

        pms->pWindow = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->hanWinLength);
        if ( pms->pWindow == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested window vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pWindow);

        /* Create/destroy window object to fill the window with data */
        pWinState = SSC_WIN_CreateInstance( pms->pLogState, pms->hanWinLength, SSC_WIN_DOUBLE);
        if ( pWinState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested window instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( SSC_WIN_Create( pWinState, SSC_WIN_SQRT_HANNING, pms->pWindow) != 0 )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: To be used window[%d] not created\n", SSC_WIN_SQRT_HANNING);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        pWinState = SSC_WIN_DestroyInstance( pWinState );


        pms->pHelpBuffer = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->envLength);
        if ( pms->pHelpBuffer == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested WorkBuffer vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pHelpBuffer);

        pms->pEnvCoefs = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->envOrder + 1);
        if ( pms->pEnvCoefs == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested LpcPars vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pEnvCoefs);
        pms->pEnvCoefs[0] = (Double)1.0;

        pms->pLsfEnvCoefs = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->envOrder );
        if ( pms->pLsfEnvCoefs == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested LsfEnvPars vector\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        SSC_VECT_Clear(pms->pLsfEnvCoefs);

        pms->pWinDataVector = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->envLength);
        if ( pms->pWinDataVector == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested WinDataVector vector for channel[%d]\n", ch);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pms->pEnvVector = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->envLength);
        if ( pms->pEnvVector == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested EnvVector vector for channel[%d]\n", ch);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* create / initialise variables that need to be stored
         * per channel and remembered over sf */
        for ( ch = 0; ch < pNoise_Temp_EnvInfo->Channels; ch++)
        {
            /* create a buffer for analysing : size is 6 times
             * sfsize */
            pms->ppDataVector[ch] = SSC_VECT_CreateInstance(pms->pLogState, SSC_VECT_DOUBLE, pms->anaLength);
            if ( pms->ppDataVector[ch] == NULL )
            {
                SSCLOG_TRACE5(pms->pLogState, "Error: Unable to create requested DataVector vector for channel[%d]\n", ch);
                Throw SSC_ERROR_NOT_AVAILABLE;
            }
        }

        /* create a FREQLPC instance */
        pms->pFreqLpcState = SSC_FREQLPC_CreateInstance( pms->pLogState, pms->envOrder, pms->envLength );
        if ( pms->pFreqLpcState == NULL )
        {
            SSCLOG_TRACE5(pms->pLogState,"\nError: Unable to create requested freqlpc instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE ;
        }


        memcpy(&pms->Info, pNoise_Temp_EnvInfo, sizeof(SSC_NOISE_TEMP_ENV_INFO));

    } 
    Catch( Error ) 
    {
        pms = IMP_NOISE_TEMP_ENV_DestroyInstance( pms );
    }

    return pms ;
}

/* Destroys the noise_temp_env module instance pointed to by *ppState
 * and frees all the memory allocated on
 * IMP_NOISE_TEMP_ENV_CreateInstance */
SSC_NOISE_TEMP_ENV_STATE* IMP_NOISE_TEMP_ENV_DestroyInstance( SSC_NOISE_TEMP_ENV_STATE* const pms )
{


    if( pms != NULL )
    {
        SInt ch = 0;

        SSCLOG_TRACE4( pms->pLogState, "SSCNOISE_TEMP_ENV_DestroyInstance\n" );

        if ( pms->pPipeState != NULL )
        {
            SSCPIPE_DestroyInstance( pms->pPipeState );
        }

        if ( pms->pLogState != NULL )
        {
            SSCLOG_DESTROYINSTANCE ( pms->pLogState );
            SSCLOG_FREE();
        }

        if ( pms->pFreqLpcState != NULL )
        {
            pms->pFreqLpcState = SSC_FREQLPC_DestroyInstance( pms->pFreqLpcState );
        }

        /* destroy channel dependant structs */
        for ( ch = 0; ch < pms->Channels; ch++ )
        {
            if ( pms->ppDataVector[ch] != NULL )
            {
                pms->ppDataVector[ch] = SSC_VECT_DestroyInstance( pms->ppDataVector[ch] );
            }
        }

        if ( pms->pWindow  != NULL )
        {
            pms->pWindow = SSC_VECT_DestroyInstance( pms->pWindow );
        }

        if ( pms->pWinDataVector  != NULL )
        {
            pms->pWinDataVector = SSC_VECT_DestroyInstance( pms->pWinDataVector );
        }

        if ( pms->pHelpBuffer  != NULL )
        {
            pms->pHelpBuffer = SSC_VECT_DestroyInstance( pms->pHelpBuffer );
        }

        if ( pms->pEnvCoefs  != NULL )
        {
            pms->pEnvCoefs = SSC_VECT_DestroyInstance( pms->pEnvCoefs );
        }
        if ( pms->pLsfEnvCoefs  != NULL )
        {
            pms->pLsfEnvCoefs = SSC_VECT_DestroyInstance( pms->pLsfEnvCoefs );
        }
        if ( pms->pEnvVector  != NULL )
        {
            pms->pEnvVector = SSC_VECT_DestroyInstance( pms->pEnvVector );
        }

        IMP_NOISE_TEMP_ENV_Free();

        SSC_FREE( pms, "Noise Temp_Env Instance" );

    }
    else
    {
        fprintf( stdout, "\nError: IMP_NOISE_TEMP_ENV_DestroyInstance invalid state pointer\n" );
    }

    return NULL;
}


/* Process current sbunch */
SSCBUNCH* IMP_NOISE_TEMP_ENV_Process( SSC_NOISE_TEMP_ENV_STATE* const pms, SSCBUNCH* const pSscWorkBunch )
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
        SInt      SfCounter = SSCBUNCH_GetSfCounter(SSCPIPE_GetAt(pms->pPipeState, PIPE_HIST_LENGTH));
        SSC_ERROR Error     = SSC_ERROR_OK ;

        pPipe    = pms->pPipeState;
        pLog     = pms->pLogState;
        Channels = pms->Channels;
        SfSize   = pms->Info.SubframeSize;

        /* put the new received sbunch in the pipe */
        SSCPIPE_Add(pPipe, pSscWorkBunch);

        /* shift the signal from position aheadlength - 1
         * into the ';work' buffer */
        for ( ch = 0; ch < Channels; ch++)
        {
            SSC_VECT_Move(pms->ppDataVector[ch], SfSize,
                          pms->ppDataVector[ch], 0,
                          pms->anaLength - SfSize);

            /* update the buffer with the new data */ 
            for ( idx1 = (pms->anaLength - SfSize), idx2 = 0; idx2 < SfSize; idx1++, idx2++)
            {
                pms->ppDataVector[ch][idx1] = (Double) SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH - 1), ch )[idx2];
            }
        }

        /* check first if subframe consists of valid data for dealing
         * with start situation*/
        if ( SSCBUNCH_GetDataFromStream( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH) ) == True )
        {
            SSCLOG_TRACE4( pLog, "*************sf%d\n", SSCBUNCH_GetSfCounter( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH)));

            /* Analysis is executed 
             * once every four subframes */
            if ( SfCounter % 4 == 1)
            {
                for ( ch = 0; ch < Channels; ch++ )
                {
                    SInt i = 0;
                    SInt k = 0;
                    Double absEnvVal = 0;

                    SSC_EXCEPTION  tec;
                    SSC_EXCEPTION* the_exception_context = &tec;

                    /* shift the signal into the ';work' buffer */
                    SSC_VECT_Move(pms->ppDataVector[ch], 5*SfSize,
                                  pms->ppDataVector[ch], 0,
                                  SfSize);

                    /*update the buffer with the new data */ 
                    for (k = 5; k > 0; k--)
                    {
                        for ( idx1 = (pms->anaLength - k*SfSize), idx2 = 0; idx2 < SfSize; idx1++, idx2++)
                        {
                            pms->ppDataVector[ch][idx1] = (Double) SSCBUNCH_GetResidual( SSCPIPE_GetAt(pPipe, PIPE_AHEAD_LENGTH - k), ch )[idx2];
                        }
                    }

                    Try 
                    {
                        SSC_VECT_Clear(pms->pWinDataVector);

                        SSC_VECT_Copy(pms->ppDataVector[ch], pms->envLength/4, pms->pWinDataVector, pms->envLength/4, pms->envLength/2);

                        SSC_VECT_Mpy( pms->ppDataVector[ch], 0, pms->pWindow, 0, pms->pWinDataVector, 0, pms->envLength/4);
                        SSC_VECT_Mpy( pms->ppDataVector[ch], 3*pms->envLength/4, pms->pWindow, pms->envLength/4, pms->pWinDataVector, 3*pms->envLength/4, pms->envLength/4);
                    
                        SSC_VECT_SumAbs(pms->pWinDataVector, 0, 0, &absEnvVal );

                        SSCLOG_TRACE4(pLog,"Sum absolute value: %.8e\n", absEnvVal);
                        if ( (SInt)absEnvVal == 0 )
                        {
                            SSCLOG_TRACE3( pLog, "No abs signal present\n");
                            Throw SSCNOISE_TEMP_ENV_NO_SIGNAL;
                        }

                        SSC_VECT_Clear(pms->pEnvCoefs);
                        SSC_FREQLPC_Calculate( pms->pFreqLpcState, pms->ppDataVector[ch], pms->pEnvCoefs );

                        /* LOG the found values for env coefficients */
                        SSCLOG_TRACE4(pLog,"Env coefs: " );
                        for ( i = 0; i < pms->envOrder; i++)
                        {
                            SSCLOG_TRACE4(pLog,"%.8e ", pms->pEnvCoefs[i]);
                        }
                        SSCLOG_TRACE4(pLog,"\n");
                  
                        /* Compute the time envelope (first clear the vector) */
                        SSC_VECT_Clear(pms->pEnvVector);
                        LOCAL_Freqz(pms->pEnvCoefs, pms->envOrder + 1, pms->pEnvVector, pms->envLength);

                        if ( absEnvVal != 0 )
                        {
                            SSC_VECT_Clear(pms->pHelpBuffer);
                            SSC_VECT_Div(pms->pWinDataVector, 0, pms->pEnvVector, 0, pms->pHelpBuffer, 0, 0);
                            pms->gain = LOCAL_standard_dev(pms->pHelpBuffer, pms->envLength);                        
                        }

                        /*log the found gain */
                        SSCLOG_TRACE4(pLog, "Gain: %.8e\n ", pms->gain);
                    
                        /* first clear */
                        SSC_VECT_Clear(pms->pLsfEnvCoefs);

                        /* Convert APAR parameters to LSFs */
                        i = LOCAL_pc2lsf(pms->pEnvCoefs, pms->pLsfEnvCoefs, pms->envOrder);
                        if ( i == 1 )
                        {
                            SSCLOG_TRACE3( pLog, "Apar2Lar failed on Den\n");
                            Throw SSCNOISE_TEMP_ENV_APAR2LSF_ERROR;
                        }

                        /* LOG the found values for lsf envelope parameters*/
                        SSCLOG_TRACE4(pLog,"LSFs: " );
                        for ( i = 0; i < pms->envOrder; i++)
                        {
                            SSCLOG_TRACE4(pLog,"%.8e ", pms->pLsfEnvCoefs[i]);
                        }
                        SSCLOG_TRACE4(pLog,"\n");
                    } 
                    Catch( Error ) 
                    {
                        /* if no signal => reset */
                        SSC_VECT_Clear(pms->pEnvCoefs);
                        pms->pEnvCoefs[0] = (Double) 1.0;

                        SSC_VECT_Clear(pms->pLsfEnvCoefs);

                        pms->gain = (Double) 0.0;
 
                        /* at the moment all errors are no errors to
                         * stop encoding */
                        Error = SSC_ERROR_OK;
                    }

                    /* save the found gain */
                    SSCBUNCH_SetNoiseEnvGain( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), ch, pms->gain);

                    /* save envelope parameters */
                    SSCBUNCH_SetNoiseEnvCoefs(SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), ch, pms->pLsfEnvCoefs);
                }
            }
        }

        /* Use LOCAL_SetError to set the error in the sbunch if an error occurs*/
        LOCAL_SetError( SSCPIPE_GetAt(pPipe, PIPE_HIST_LENGTH), Error );
    }
    else
    {
        fprintf( stdout, "\nError: IMP_NOISE_TEMP_ENV_Process invalid state pointer\n" );
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
void IMP_NOISE_TEMP_ENV_Free(void)
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
        SSCBUNCH_SetErrorOrigin( pSscBunch, SSCENC_NOISE_TEMP_ENV_MODULE );
        SSCBUNCH_SetErrorCode( pSscBunch, (SInt)Error );
    }
}


/* standard deviation */
static Double LOCAL_standard_dev(  Double* const X,
                                   SInt          N)
{
    SInt    c1  = 0;
    Double  mean = 0;
    Double  tmp = 0;
    Double  std = 0;


    SSC_VECT_Sum(X, 0, N, &mean);

    mean = mean/ N;

    for (c1 = 0; c1 < N; c1++)
    {
        X[c1] = X[c1] - mean; /* remove mean */
    }

    SSC_VECT_SumSqr(X, 0, N, &tmp);
    tmp = tmp / N;
    std = (Double)sqrt(tmp);

    return std;
}

/* frequency response |H(z)| = 1 / |A(z)| */
static void LOCAL_Freqz(Double* a, SInt aLen, Double* pResult, SInt Length)
{
    SInt k,i;

    for ( k = 0; k < Length; k++)
    {
        Double Re = 0.0;
        Double Im = 0.0;

        for ( i = 0; i < aLen; i++)
        {
            Re += a[i] * cos(SSC_PI/Length*k*i);
            Im -= a[i] * sin(SSC_PI/Length*k*i);
        }

        pResult[k] = 1/(sqrt(Re*Re+Im*Im));
    }

}

/* Convert polynomial to LSF coefficients
 * Parameters  : pc    : Polynomial, size (np+1)
 *               Output: LSFs, size np
 *               np    : order = # of freq to calculate
 *
 * Returns     : 0 if OK;
 *               1 if error 
 */
static SInt LOCAL_pc2lsf(const Double pc[], Double lsf[], SInt np)
{
    Double fa[(MAXORDER+1)/2+1], fb[(MAXORDER+1)/2+1];
    Double ta[(MAXORDER+1)/2+1], tb[(MAXORDER+1)/2+1];
    Double *t;
    Double xlow, xmid, xhigh;
    Double ylow, ymid, yhigh;
    Double xroot;
    Double dx;
    SInt   i, j, nf;
    SInt   odd;
    SInt   na, nb, n;
    Double ss, aa;

    assert(np <= MAXORDER);

/* Determine the number of coefficients in ta(.) and tb(.) */
    odd = np % 2 != 0;
    if (odd) 
    {
        nb = (np + 1) / 2;
        na = nb + 1;
    }
    else 
    {
        nb = np / 2 + 1;
        na = nb;
    }


    fa[0] = (Double)1.0;
    for (i = 1, j = np; i < na; ++i, --j)
        fa[i] = pc[i] + pc[j];

    fb[0] = (Double)1.0;
    for (i = 1, j = np; i < nb; ++i, --j)
        fb[i] = pc[i] - pc[j];


    if (odd) 
    {
        for (i = 2; i < nb; ++i)
            fb[i] = fb[i] + fb[i-2];
    }
    else 
    {
        for (i = 1; i < na; ++i) 
        {
            fa[i] = fa[i] - fa[i-1];
            fb[i] = fb[i] + fb[i-1];
        }
    }


    ta[0] = fa[na-1];
    for (i = 1, j = na - 2; i < na; ++i, --j)
        ta[i] = (Double)2.0 * fa[j];

    tb[0] = fb[nb-1];
    for (i = 1, j = nb - 2; i < nb; ++i, --j)
        tb[i] = (Double)2.0 * fb[j];

    nf = 0;
    t = ta;
    n = na;
    xroot = (Double)2.0;
    xlow = (Double)1.0;
    ylow = LOCAL_FNevChebP(xlow, t, n);

    ss = (Double)sin (DW);
    aa = (Double)(4.0 - 4.0 * cos (DW)  - ss);

    /* Root search loop */
    while (xlow > (Double)-1.0 && nf < np) 
    {

        /* New trial point */
        xhigh = xlow;
        yhigh = ylow;
        dx = aa * xhigh * xhigh + ss;
        xlow = xhigh - dx;
        if (xlow < (Double)-1.0)
            xlow = (Double)-1.0;
        ylow = LOCAL_FNevChebP(xlow, t, n);
        if (ylow * yhigh <= (Double)0.0) 
        {

          /* Bisections of the interval containing a sign change */
            dx = xhigh - xlow;
            for (i = 1; i <= NBIS; ++i) 
            {
	            dx = (Double)0.5 * dx;
	            xmid = xlow + dx;
	            ymid = LOCAL_FNevChebP(xmid, t, n);
	            if (ylow * ymid <= 0.0) 
                {
	                yhigh = ymid;
	                xhigh = xmid;
                }
	            else 
                {
	                ylow = ymid;
	                xlow = xmid;
                }
            }

            /*
             * Linear interpolation in the subinterval with a sign change
             * (take care if yhigh=ylow=0)
             */
            if (yhigh != ylow)
	            xmid = xlow + dx * ylow / (ylow - yhigh);
            else
	            xmid = xlow + dx;

            /* New root position */
            lsf[nf] = (Double)acos( xmid);
            ++nf;

            /* Start the search for the roots of the next polynomial at the estimated
             * location of the root just found.  We have to catch the case that the
             * two polynomials have roots at the same place to avoid getting stuck at
             * that root.
             */
            if (xmid >= xroot) 
            {
	            xmid = xlow - dx;
            }
            xroot = xmid;
            if (t == ta) 
            {
	            t = tb;
	            n = nb;
            }
            else 
            {
	            t = ta;
	            n = na;
            }
            xlow = xmid;
            ylow = LOCAL_FNevChebP(xlow, t, n);
        }
    }

    /* Error if np roots have not been found */
    if (nf != np) 
    {
        printf("\nWARNING: pc2lsf failed to find all lsf nf=%d np=%d\n", nf, np);
        return(1);
    }
    return(0);
}

/*  Evaluate a series expansion in Chebyshev polynomials 
 x   : value 
 c[] : Array of coefficient values 
 n   : Number of coefficients 
*/
static Double LOCAL_FNevChebP( Double x, const Double c[], SInt n )
{
    SInt   i;
    Double b0, b1;
    Double b2 = 0;


    b1 = (Double)0.0;
    b0 = (Double)0.0;
    for (i = n - 1; i >= 0; --i) 
    {
        b2 = b1;
        b1 = b0;
        b0 = (Double)2.0 * x * b1 - b2 + c[i];
    }

    return ((Double)0.5 * (b0 - b2 + c[0]));
}

