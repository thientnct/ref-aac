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

Source file: SSC_noise_quantisation.c

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
 *   Module              : Noise Quantisation
 *
 *   Description         : Implementation of Noise quantisation
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with SSCENC_NOISE_QUANTISATION_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_noise_quantisation.h"
#include "SSC_noise_quantisation.h"
#include "Imp_Error.h"

#include "cexcept.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/* Wrapper function to create the module state structure and then
 * calls the IMP_NOISE_QUANTISATION_CreateInstance function to get a module
 * info structure. */
SSC_INTF_EXPORT PSSC_MODULESTATE SSC_NOISE_QUANTISATION_InitInstance( PENCODERINFO pEncoderInfo )
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    PSSC_MODULESTATE pms = NULL ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_noise_quantisation" ) ;
    Char             szLogFile[_MAX_PATH] ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;
        SSC_NOISE_QUANTISATIONINFO noise_quantisationInfo;

        if ( pEncoderInfo == NULL )
        {
            printf( "\nError: Invalid pointer to encoderinfo\n" );
            Throw SSC_ERROR_NOT_SUPPORTED ;
        }

        /* identify/compose the log filename */
        strcpy( szLogFile, pEncoderInfo->out_dir_name );
        strcat( szLogFile, pEncoderInfo->out_file_name);
        cp = strrchr( szLogFile, (int)'.' );
        if ( cp != NULL )
        {
            *cp = '\0' ; /* strip extension, if found */
        }
        strcat( szLogFile, "_noise_quantisation.log");

        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for noise_quantisation instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        IMP_NOISE_QUANTISATION_Initialise();

        noise_quantisationInfo.DecoderLevel  = pEncoderInfo->decoder_level ;
        noise_quantisationInfo.Mode          = pEncoderInfo->mode ;
        noise_quantisationInfo.ModeExt       = pEncoderInfo->mode_ext ;
        noise_quantisationInfo.Channels      = pEncoderInfo->nr_of_channels ;
        noise_quantisationInfo.SubframeSize  = pEncoderInfo->sf_size ;
        noise_quantisationInfo.EnvOrder      = pEncoderInfo->max_env_order ;

        /* TODO : fill rest of pms->noise_quantisationInfo structure */
        {
            noise_quantisationInfo.dLar_Stepsize  = SSC_N_LAR_DYN_RANGE / (SSC_N_LAR_LEVELS) ;
            noise_quantisationInfo.nAbs_max_lar   = (SInt)floor( ( (2^(SSC_N_LAR_BITS)) - 1 ) / 2 ) * 4 ;
            noise_quantisationInfo.nRlGain_min    = 0 ;
            noise_quantisationInfo.nRlGain_max    = 101 ;
            noise_quantisationInfo.nRlGain_offset = 21 ;

            noise_quantisationInfo.dLsf_Stepsize  = SSC_PI / (SSC_N_LSF_LEVELS) ;
            noise_quantisationInfo.nLsf_min_val   = 0 ;
            noise_quantisationInfo.nLsf_max_val   = SSC_N_LSF_LEVELS - 1 ;
        }

        pms->pnoise_quantisationState = IMP_NOISE_QUANTISATION_CreateInstance( &noise_quantisationInfo, szLogFile, nTraceLevel );
        if ( pms->pnoise_quantisationState == NULL )
        {
            printf( "\nError: SSCnoise_quantisation_CreateInstance failed errorcode\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_NOISE_QUANTISATION_DestroyInstance(pms);
    }
    return pms ;
}



/* Wrapper function which calls the IMP_NOISE_QUANTISATION_Process function */
SSC_INTF_EXPORT SSCBUNCH *SSC_NOISE_QUANTISATION_Process( PSSC_MODULESTATE pms, SSCBUNCH *pSscWorkBunch, SSC_ERROR *pnError )
{
    SSCBUNCH *pSscNextWorkBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
        pSscNextWorkBunch = IMP_NOISE_QUANTISATION_Process( pms->pnoise_quantisationState, pSscWorkBunch);
        if ( pSscNextWorkBunch && pSscNextWorkBunch->pBunch )
        {
            *pnError = pSscNextWorkBunch->pBunch->error_code ;
        }
        else
        {
            *pnError = SSC_ERROR_OUT_OF_MEMORY ;
        }
    }
    return pSscNextWorkBunch ;
}


/* Wrapper function which frees all memory allocated by the
 * SSC_NOISE_QUANTISATION_Init call and then calls the
 * IMP_NOISE_QUANTISATION_DestroyInstance to free the memory allocated
 * on the IMP_NOISE_QUANTISATION_CreateInstance call */

SSC_INTF_EXPORT PSSC_MODULESTATE SSC_NOISE_QUANTISATION_DestroyInstance( PSSC_MODULESTATE pms )
{
    if ( pms != NULL )
    {
        IMP_NOISE_QUANTISATION_DestroyInstance( &pms->pnoise_quantisationState );
        SSC_FREE( pms, "" );
        pms = NULL ;
        IMP_NOISE_QUANTISATION_Free();
    }
    return pms ;
}

