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

Source file: SSC_Transient_Synthesis.c

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
 *   Module              : Transient synthesis
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <string.h>
#include <stdlib.h>
#include "PlatformTypes.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_Transient_Synthesis.h"
#include "SSC_Transient_Synthesis.h"
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
 * calls the IMP_TRANSIENT_SYNTHESIS_CreateInstance function to get a
 * module info structure. */
SSC_INTF_EXPORT PSSC_MODULESTATE SSC_TRANSIENT_SYNTHESIS_InitInstance( PENCODERINFO pEncoderInfo )
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    PSSC_MODULESTATE pms = NULL ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_transient_synthesis" ) ;
    Char             szLogFile[260] ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;
        SSC_TRANSIENT_SYNTHESIS_INFO Transient_SynthesisInfo;

        if( pEncoderInfo == NULL )
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
        strcat( szLogFile, "_TranSynth.log");

        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "" );
        if( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for transient_synthesis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        IMP_TRANSIENT_SYNTHESIS_Initialise();

        Transient_SynthesisInfo.DecoderLevel      = pEncoderInfo->decoder_level ;
        Transient_SynthesisInfo.Mode              = pEncoderInfo->mode ;
        Transient_SynthesisInfo.Mode              = pEncoderInfo->mode_ext ;
        Transient_SynthesisInfo.Channels          = pEncoderInfo->nr_of_channels ;
        Transient_SynthesisInfo.SubframeSize      = pEncoderInfo->sf_size ;
        Transient_SynthesisInfo.pSSC_Chi          = (Double *)SSC_Chi ;
        Transient_SynthesisInfo.pSSC_a_max        = (Double *)SSC_a_max ;
        Transient_SynthesisInfo.nCntChi           = SSC_T_MEIXNER_NROF_CHIS ;
        Transient_SynthesisInfo.nCntB             = SSC_T_MEIXNER_NROF_BS ;
        Transient_SynthesisInfo.nSynthesizeLength = 2* pEncoderInfo->sf_size ;
        /* TODO : fill rest of pms->Transient_SynthesisInfo structure */

        pms->pTransient_SynthesisState = IMP_TRANSIENT_SYNTHESIS_CreateInstance( &Transient_SynthesisInfo, szLogFile, nTraceLevel );
        if( pms->pTransient_SynthesisState == NULL )
        {
            printf( "\nError: SSCTransient_Synthesis_CreateInstance failed errorcode\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_TRANSIENT_SYNTHESIS_DestroyInstance(pms);
    }
    return pms ;
}


/* Wrapper function which calls the IMP_TRANSIENT_SYNTHESIS_Process
 * function */
SSC_INTF_EXPORT SSCBUNCH *SSC_TRANSIENT_SYNTHESIS_Process( PSSC_MODULESTATE pms, SSCBUNCH *pSscWorkBunch, SSC_ERROR *pnError )
{
    SSCBUNCH *pSscNextWorkBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
        pSscNextWorkBunch = IMP_TRANSIENT_SYNTHESIS_Process( pms->pTransient_SynthesisState, pSscWorkBunch);
        if( pSscNextWorkBunch && pSscNextWorkBunch->pBunch )
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
 * SSC_TRANSIENT_SYNTHESIS_Init call and then calls the
 * IMP_TRANSIENT_SYNTHESIS_DestroyInstance to free the memory
 * allocated on the IMP_TRANSIENT_SYNTHESIS_CreateInstance call */
SSC_INTF_EXPORT PSSC_MODULESTATE SSC_TRANSIENT_SYNTHESIS_DestroyInstance( PSSC_MODULESTATE pms )
{
    if ( pms != NULL )
    {
        IMP_TRANSIENT_SYNTHESIS_DestroyInstance( &pms->pTransient_SynthesisState );
        SSC_FREE( pms, "" );
        pms = NULL ;
        IMP_TRANSIENT_SYNTHESIS_Free();
    }
    return pms ;
}

