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

Source file: SSC_Filter.c

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
 *   Module              : Filter
 *
 *   Description         : Implementation of filter.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSCENC_FILTER_
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

#include "Filter.h"
#include "Imp_Filter.h"
#include "SSC_Filter.h"
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
 * calls the IMP_FILTER_CreateInstance function to get a module
 * info structure. */
SSC_INTF_EXPORT PSSC_MODULESTATE SSC_FILTER_InitInstance( PENCODERINFO pEncoderInfo )
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    PSSC_MODULESTATE pms = NULL ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "SSC_filter" ) ;
    Char             szLogFile[260] ;

    SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        SSC_FILTER_INFO FilterInfo;
        Char* cp = NULL;

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
        strcat( szLogFile, "_Filter.log");

        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "" );
        if ( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for filter instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }
        IMP_FILTER_Initialise();

        FilterInfo.DecoderLevel  = pEncoderInfo->decoder_level ;
        FilterInfo.Channels      = pEncoderInfo->nr_of_channels ;
        FilterInfo.SubframeSize  = pEncoderInfo->sf_size ;
        FilterInfo.fs            = pEncoderInfo->fs ;
        FilterInfo.CutOffFreq    = 5 ; /* currently only supported : 5 Hz at 44100 */

        pms->pFilterState = IMP_FILTER_CreateInstance( &FilterInfo, szLogFile, nTraceLevel );
        if ( pms->pFilterState == NULL )
        {
            printf( "\nError: SSCFilter_CreateInstance failed.\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
        pms = SSC_FILTER_DestroyInstance( pms );
    }
    return pms ;
}


/* Wrapper function which calls the IMP_FILTER_Process function */
SSC_INTF_EXPORT SSCBUNCH *SSC_FILTER_Process( PSSC_MODULESTATE pms, SSCBUNCH *pSscWorkBunch, SSC_ERROR *pnError )
{
    SSCBUNCH *pSscNextWorkBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
        pSscNextWorkBunch = IMP_FILTER_Process( pms->pFilterState, pSscWorkBunch);
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
 * SSC_FILTER_Init call and then calls the IMP_FILTER_DestroyInstance
 * to free the memory allocated on the IMP_FILTER_CreateInstance
 * call */
SSC_INTF_EXPORT PSSC_MODULESTATE SSC_FILTER_DestroyInstance( PSSC_MODULESTATE pms )
{
    if ( pms != NULL )
    {
        IMP_FILTER_DestroyInstance( &pms->pFilterState );
        SSC_FREE( pms, "" );
        pms = NULL ;
        IMP_FILTER_Free();
    }
    return pms ;
}

