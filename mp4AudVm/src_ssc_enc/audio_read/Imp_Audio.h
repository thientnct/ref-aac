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

Source file: Imp_Audio.h

Required libraries: <none>

Authors:
AP:	Arno Peters,       Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar,     Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,      Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
NT:	Natasa Topalovic,  Philips PDSLabs Eindhoven <natasa.topalovic@philips.com>

Changes:
17-Oct-2001	AP/JD	Initial version
01-Oct-2002 NT      RM2 change 
************************************************************************/

/**************************************************************************
 *
 * Audio Read implementation
 *
 * Imp_Audio.h
 *
 * contains all structures and prototypes of the implementation
 *
 **************************************************************************/

#ifndef __Imp_Audio_h
#define __Imp_Audio_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/


/* this typedef must be define if we are not in microsoft environment */
/*   because off_t is not ANSI C                                      */
/* if defined it must be before "include sndfile.h" */
#if !defined(_OFF_T_DEFINED) && !defined(__off_t_defined)
typedef long off_t;     /*file offset value */
#define _OFF_T_DEFINED
#define __off_t_defined
#endif

#include "libtsp.h"				/* AFsp audio file library */
#include "libtsp/AFpar.h"       /* AFsp audio file library - definitions */
#include "audio.h"


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/
typedef struct audioinfo_tag
{
    /* variables which are needed, come in */
    Double   startSample ;
    Char*    dirName;
    Char*    fileName;

    /* initially needed( come in ); can be adapted*/
    Double   numberSamples ;
    SInt     channelDepend ;

    /* variables produced, send out */
    SInt     numberChannels ;
    SInt     sf_size ;

    SInt     nMode;
    SInt     nModeExt;

    SInt     fs;
    SInt     samplingRate;
    SInt     updateRate;
    SInt     numberSubFrames;


}SSC_AUDIO_INFO ,*PSSC_AUDIO_INFO ;

typedef struct ssc_modulestate_tag
{
    /* variables used for used objects */
    SSCLOG*          pLog ;
    Double*          pdBuf ;
	FloatSSC*		 pfBuf;
    /* variables used for internal functionallity */
	void *   af_file ;

	AFILE *  af_info;
    SInt     nCurPos ;
    SInt     sf_counter ;
    SInt     nStopPos;
    SInt     sf_size ;
    SInt     numberChannels ;

}SSC_AUDIO_STATE, *PSSC_AUDIO_STATE ;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/


/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
void             IMP_AUDIO_Initialise     ( void );
SSC_AUDIO_STATE* IMP_AUDIO_CreateInstance( SSC_AUDIO_INFO* pAudioInfo, Char* szLogFile, SInt nTraceLevel, int numChannel, float fSample, long fileNumSample);
SSCBUNCH*        IMP_AUDIO_Process_NoAudioRead( SSC_AUDIO_STATE* pas, SSCBUNCH* pSscWorkBunch, float ** p_time_signal );
SSC_AUDIO_STATE* IMP_AUDIO_DestroyInstance( SSC_AUDIO_STATE* pas );
void             IMP_AUDIO_Free           ( void );

#ifdef __cplusplus
}
#endif /* __Imp_Audio_h */

#endif



