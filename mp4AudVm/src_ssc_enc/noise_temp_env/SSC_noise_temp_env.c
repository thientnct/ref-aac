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

Source file: SSC_noise_temp_env.c

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
 *   Module              : noise temporal envelope
 *
 *   Description         : Implementation of noise temporal envelope
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with SSCENC_NOISE_TEMP_ENV_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <string.h>
#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_noise_temp_env.h"
#include "SSC_noise_temp_env.h"
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
 * calls the IMP_NOISE_TEMP_ENV_CreateInstance function to get a
 * module info structure. */
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_NOISE_TEMP_ENV_InitInstance( ENCODERINFO* pEncoderInfo )
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    SSC_MODULESTATE* pms = NULL ;

    Char             szLogFile[120] ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_noise_temp_env" ) ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;

        SSC_NOISE_TEMP_ENV_INFO Noise_Temp_EnvInfo;

        if ( pEncoderInfo == NULL )
        {
            fprintf( stdout, "\nError: Invalid pointer to encoderinfo\n" );
            Throw SSC_ERROR_NOT_SUPPORTED ;
        }
        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "Noise Temp Env Module State" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for noise_temp_env instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        pms->pNoise_Temp_EnvState = NULL;
        pms->pSscWorkPipe        = NULL;

        IMP_NOISE_TEMP_ENV_Initialise();

        Noise_Temp_EnvInfo.DecoderLevel  = pEncoderInfo->decoder_level ;
        Noise_Temp_EnvInfo.Mode          = pEncoderInfo->mode ;
        Noise_Temp_EnvInfo.ModeExt       = pEncoderInfo->mode_ext ;
        Noise_Temp_EnvInfo.Channels      = pEncoderInfo->nr_of_channels ;
        Noise_Temp_EnvInfo.SubframeSize  = pEncoderInfo->sf_size ;
        /* TODO : fill rest of pms->Noise_Temp_EnvInfo structure */
        Noise_Temp_EnvInfo.EnvOrder      = pEncoderInfo->max_env_order;

        strcpy( szLogFile, pEncoderInfo->out_dir_name );
        strcat( szLogFile, pEncoderInfo->out_file_name);
        cp = strrchr( szLogFile, (int)'.' );
        if ( cp != NULL )
        {
            *cp = '\0' ; /* strip extension, if found */
        }
        strcat( szLogFile, "_NoiseAna.log");
        pms->pNoise_Temp_EnvState = IMP_NOISE_TEMP_ENV_CreateInstance( &Noise_Temp_EnvInfo, szLogFile, nTraceLevel );
        if( pms->pNoise_Temp_EnvState == NULL )
        {
            fprintf( stdout,"\nError: SSCNoise_Temp_Env_CreateInstance failed errorcode\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_NOISE_TEMP_ENV_DestroyInstance( pms );
    }

    return pms;
}


/* Wrapper function which calls the IMP_NOISE_TEMP_ENV_Process function */
SSC_INTF_EXPORT SSCBUNCH* SSC_NOISE_TEMP_ENV_Process( SSC_MODULESTATE* pms, SSCBUNCH* pSBunch, SSC_ERROR *pnError )
{
    SSCBUNCH *pRetBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
        pRetBunch = IMP_NOISE_TEMP_ENV_Process( pms->pNoise_Temp_EnvState, pSBunch );
        if ( pRetBunch && pRetBunch->pBunch )
        {
            *pnError = pRetBunch->pBunch->error_code ;
        }
        else
        {
            *pnError = SSC_ERROR_OUT_OF_MEMORY ;
        }
    }
    else
    {
        fprintf(stdout,"\nError: Illegal Noise Module state in process\n");
    }
    return pRetBunch;
}


/* Wrapper function which frees all memory allocated by the
 * SSC_NOISE_TEMP_ENV_Init call and then calls the
 * IMP_NOISE_TEMP_ENV_DestroyInstance to free the memory allocated on
 * the IMP_NOISE_TEMP_ENV_CreateInstance call */
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_NOISE_TEMP_ENV_DestroyInstance( SSC_MODULESTATE* pms )
{
    if ( pms != NULL )
    {
        if ( pms->pNoise_Temp_EnvState != NULL )
        {
            pms->pNoise_Temp_EnvState = IMP_NOISE_TEMP_ENV_DestroyInstance( pms->pNoise_Temp_EnvState );
        }

        SSC_FREE( pms, "Noise Temp_Env Module State" );
        pms = NULL;
    }

    return pms;
}

