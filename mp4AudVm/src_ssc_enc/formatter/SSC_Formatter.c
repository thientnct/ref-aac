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

Source file: SSC_Formatter.c

Required libraries: <none>

Authors:
AP:    Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:    Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:    Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:    Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>
AG:    Andy Gerrits, Philips Research Eindhoven <andy.gerrits@philips.com>

Changes:
17-Oct-2001 AP/JD   Initial version
20-Sep-2002 NT      RM2 change
21-Nov-2003 AG      Changes for RM4
************************************************************************/

/******************************************************************************
 *
 *   Module              : Bitstream formatter
 *
 *   Description         : Implementation of bitstream formatter
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSC_BITSTREAM_
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

#include "SSC_Alloc.h"
#include "SSC_Error.h"

#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"
#include "SSC_Log.h"

#include "SSC_BitStrm_Types.h"
#include "bsc_buffer.h"
#include "bsc_wres.h"
#include "SSC_Streaming.h"
#include "SSC_Formatter.h"

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
 * calls the SSC_STRM_CreateInstance function to get a module
 * info structure. */
#ifdef SSC_MP4REF_BUILD
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_BITSTREAM_InitInstance( ENCODERINFO* pEncoderInfo, unsigned char ** ppHeaderData, int * pHeaderBytes )
#else
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_BITSTREAM_InitInstance( ENCODERINFO* pEncoderInfo )
#endif
{
    SSC_ERROR        Error = SSC_ERROR_OK ;
    SSC_MODULESTATE* pms = NULL ;
    SInt             nTraceLevel = SSCLOG_GETMODULETRACELEVELFROMFILE( "ssc_bitstream" ) ;
    Char             szLogFile[260];
    Char             szSSCFile[260];

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    Try
    {
        Char* cp = NULL;
        SSC_STRM_INFO StreamInfo;

        if( pEncoderInfo == NULL )
        {
            printf( "\nError: Invalid pointer to encoderinfo\n" );
            Throw SSC_ERROR_NOT_SUPPORTED ;
        }

        /* identify/compose the SSC filename */
        strcpy( szSSCFile, pEncoderInfo->out_dir_name );
        strcat( szSSCFile, pEncoderInfo->out_file_name);

        /* identify/compose the log filename */
        strcpy( szLogFile, szSSCFile );
        cp = strrchr( szLogFile, (int)'.' );
        if ( cp != NULL )
        {
            *cp = '\0' ; /* strip extension, if found */
        }
        strcat( szLogFile, "_Bitstream.log");

        pms = SSC_CALLOC( sizeof(SSC_MODULESTATE), 1, "" );
        if( pms == NULL )
        {
            printf( "\nError: Unable to alloc requested memory for bitstream instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_STRM_Initialise();

        StreamInfo.EncoderVersion        = pEncoderInfo->encoder_version;
        StreamInfo.DecoderLevel          = pEncoderInfo->decoder_level ;
        StreamInfo.SampleRateIndex       = pEncoderInfo->sampling_rate ;
        StreamInfo.UpdateRate            = pEncoderInfo->update_rate ;
        StreamInfo.SynthesisMethod       = pEncoderInfo->synthesis_method ;
        StreamInfo.Enhancement           = pEncoderInfo->enhancement ;
        StreamInfo.Mode                  = pEncoderInfo->mode ;
        StreamInfo.ModeExt               = pEncoderInfo->mode_ext ;
        StreamInfo.Channels              = pEncoderInfo->nr_of_channels ;
        StreamInfo.EnvNoiseOrder         = pEncoderInfo->max_env_order;
        StreamInfo.RefreshStrategySinus  = pEncoderInfo->sin_refresh ;
        StreamInfo.RefreshStrategyNoise  = pEncoderInfo->noise_refresh ;
        StreamInfo.RefreshStrategyStereo = pEncoderInfo->stereo_refresh ;
        StreamInfo.SubFrameSize          = pEncoderInfo->sf_size ;
        StreamInfo.idxBins               = pEncoderInfo->stereo_nr_of_bins_idx ;
        StreamInfo.idxBinsItd            = pEncoderInfo->stereo_nr_of_itd_bins_idx ;
        StreamInfo.stereoUpdateRate      = pEncoderInfo->stereo_upd_rate ;


#ifdef SSC_MP4REF_BUILD
        pms->pBitstreamState = SSC_STRM_CreateInstance( &StreamInfo, szLogFile, nTraceLevel, szSSCFile, ppHeaderData, pHeaderBytes );
#else
        pms->pBitstreamState = SSC_STRM_CreateInstance( &StreamInfo, szLogFile, nTraceLevel, szSSCFile );
#endif        
        if( pms->pBitstreamState == NULL )
        {
            printf( "\nError: SSC_BITSTREAM_CreateInstance failed errorcode\n" );
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
    }
    Catch(Error)
    {
#ifdef SSC_MP4REF_BUILD
        SInt i0, i1, i2, i3, i4, i5;
        UByte c0, c1, c2;
        pms = SSC_BITSTREAM_DestroyInstance(pms, &c0, &i0, &i1, &c1, &i2, &i3, &c2, &i4, &i5);
#else
        pms = SSC_BITSTREAM_DestroyInstance(pms);
#endif        
    }
    return pms ;

}


/* Wrapper function which calls the SSC_STRM_Process function */
#ifdef SSC_MP4REF_BUILD
SSC_INTF_EXPORT SSCBUNCH* SSC_BITSTREAM_Process( SSC_MODULESTATE* pms, SSCBUNCH *pSscWorkBunch, SSC_ERROR *pnError, unsigned char ** ppFrameData, int * pFrameBytes )
#else
SSC_INTF_EXPORT SSCBUNCH* SSC_BITSTREAM_Process( SSC_MODULESTATE* pms, SSCBUNCH *pSscWorkBunch, SSC_ERROR *pnError  )
#endif
{
    SSCBUNCH *pSscNextWorkBunch = NULL ;

    *pnError = SSC_ERROR_OK ;
    if ( pms != NULL )
    {
#ifdef SSC_MP4REF_BUILD
        pSscNextWorkBunch = SSC_STRM_Process( pms->pBitstreamState, pSscWorkBunch, ppFrameData, pFrameBytes);
#else
        pSscNextWorkBunch = SSC_STRM_Process( pms->pBitstreamState, pSscWorkBunch);
#endif
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
 * SSC_BITSTREAM_InitInstance call and then calls the
 * SSC_STRM_DestroyInstance to free the memory allocated on the
 * SSC_STRM_CreateInstance call */

#ifdef SSC_MP4REF_BUILD
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_BITSTREAM_DestroyInstance( SSC_MODULESTATE* pms,
                                                               UByte* pFileLengthValue,
                                                               SInt* pFileLengthPos,
                                                               SInt* pFileLengthBytes,
                                                               UByte* pAudioLengthValue,
                                                               SInt* pAudioLengthPos,
                                                               SInt* pAuidoLengthBytes,
                                                               UByte* pNrOfFramesValue,
                                                               SInt* pNrOfFramesPos,
                                                               SInt* pNrOfFramesBytes
                                                               )
#else
SSC_INTF_EXPORT SSC_MODULESTATE* SSC_BITSTREAM_DestroyInstance( SSC_MODULESTATE* pms )
#endif
{
    if ( pms != NULL )
    {
#ifdef SSC_MP4REF_BUILD
        SSC_STRM_DestroyInstance(   pms->pBitstreamState,
                                    pFileLengthValue,
                                    pFileLengthPos,
                                    pFileLengthBytes,
                                    pAudioLengthValue,
                                    pAudioLengthPos,
                                    pAuidoLengthBytes,
                                    pNrOfFramesValue,
                                    pNrOfFramesPos,
                                    pNrOfFramesBytes);
#else
        SSC_STRM_DestroyInstance( pms->pBitstreamState );
#endif
        pms->pBitstreamState = NULL;
        SSC_FREE( pms, "" );
        pms = NULL ;
        SSC_STRM_Free();
    }
    return pms ;
}

