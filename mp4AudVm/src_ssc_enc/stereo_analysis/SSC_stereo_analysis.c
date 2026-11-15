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

Source file: SSC_stereo_analysis.c

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
18-Sep-2002	NT	Initial version
************************************************************************/


/******************************************************************************
 *
 *   Module              : stereo_analysis DLL entry points
 *
 *   Description         : Implementation of stereo analysis.
 *
 *   Tools               :
 *
 *   Target Platform     : 
 *
 *   Naming Conventions  : All functions begin with SSCENC_STEREO_ANALYSIS_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <string.h>
#include "PlatformTypes.h"

#include "SSC_Alloc.h"
#include "SSC_Error.h"
#include "SSC_System.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "Imp_stereo_analysis.h"
#include "SSC_stereo_analysis.h"
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
 * calls the IMP_STEREO_ANALYSIS_CreateInstance function to get a
 * module info structure. */

SSC_INTF_EXPORT SSC_MODULESTATE* SSC_STEREO_ANALYSIS_InitInstance( ENCODERINFO* pEncoderInfo )
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    SSC_MODULESTATE* pms = NULL ;

    Char             szLogFile[120] ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_stereo_analysis" ) ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;

        SSC_STEREO_ANALYSISINFO Stereo_AnalysisInfo;

        if ( pEncoderInfo == NULL )
        {
            fprintf( stdout, "\nError: Invalid pointer to encoderinfo\n" );
            Throw SSC_ERROR_NOT_SUPPORTED ;
        }
        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "Stereo Analysis Module State" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for stereo_analysis instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        pms->pStereo_AnalysisState = NULL;
        pms->pSscWorkPipe        = NULL;

        IMP_STEREO_ANALYSIS_Initialise();

        /* set initial info parameters */
        Stereo_AnalysisInfo.Channels      = pEncoderInfo->nr_of_channels ;
        Stereo_AnalysisInfo.SubframeSize  = pEncoderInfo->sf_size ;
        Stereo_AnalysisInfo.DecoderLevel  = pEncoderInfo->decoder_level ;
        Stereo_AnalysisInfo.Mode          = pEncoderInfo->mode ;
        Stereo_AnalysisInfo.ModeExt       = pEncoderInfo->mode_ext ;
        Stereo_AnalysisInfo.iidBinsIndex  = pEncoderInfo->stereo_nr_of_bins_idx ;
        Stereo_AnalysisInfo.Fs            = pEncoderInfo->fs ;

        /* identify/compose the log filename */
        strcpy( szLogFile, pEncoderInfo->out_dir_name );
        strcat( szLogFile, pEncoderInfo->out_file_name);
        cp = strrchr( szLogFile, (int)'.' );
        if ( cp != NULL )
        {
            *cp = '\0' ; /* strip extension, if found */
        }
        strcat( szLogFile, "_StereoAna.log");
       
        /* create instance */
        pms->pStereo_AnalysisState = IMP_STEREO_ANALYSIS_CreateInstance( &Stereo_AnalysisInfo, szLogFile, nTraceLevel );
        if ( pms->pStereo_AnalysisState == NULL )
        {
            fprintf( stdout, "\nError: SSCStereo_Analysis_CreateInstance failed errorcode\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_STEREO_ANALYSIS_DestroyInstance(pms);
    }
    return pms ;
}


/* Wrapper function which calls the SSC_STEREO_ANALYSIS_Process function */
SSC_INTF_EXPORT SSCBUNCH *SSC_STEREO_ANALYSIS_Process( SSC_MODULESTATE* pms, SSCBUNCH *pSBunch, SSC_ERROR *pnError )
{
    SSCBUNCH *pRetBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
        pRetBunch = IMP_STEREO_ANALYSIS_Process( pms->pStereo_AnalysisState, pSBunch);
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
        fprintf(stdout,"\nError: Illegal Stereo Analysis state in process\n");
    }
    return pRetBunch ;
}


/* Wrapper function which frees all memory allocated by the
 * SSC_STEREO_ANALYSIS_Init call and then calls the
 * IMP_STEREO_ANALYSIS_DestroyInstance to free the memory allocated on
 * the IMP_STEREO_ANALYSIS_CreateInstance call */

SSC_INTF_EXPORT SSC_MODULESTATE* SSC_STEREO_ANALYSIS_DestroyInstance( PSSC_MODULESTATE pms )
{
    if ( pms != NULL )
    {
        if ( pms->pStereo_AnalysisState != NULL )
        {
            pms->pStereo_AnalysisState = IMP_STEREO_ANALYSIS_DestroyInstance( pms->pStereo_AnalysisState );
        }

        SSC_FREE( pms, "Stereo_Analysis Module State" );
        pms = NULL;

    }
    
    return pms ;
}

