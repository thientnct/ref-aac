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

Source file: SSC_Log.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
************************************************************************/



/******************************************************************************
 *
 *   Module              :  ssc logging
 *
 *   Description         : Prototypes and defines for logging facility.
 *
 *   Tools               :
 *
 *   Target Platform     :
 *
 *   Naming Conventions  : All functions begin with SSCLOG_
 *
 ******************************************************************************/

#ifndef __ssc_log_h
#define __ssc_log_h

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>

#include "PlatformTypes.h"

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

typedef struct _SSCLOGINFO
{
    char szFileName[256] ;
    char szTitle[60] ;
    Bool bPrintTime ;
    Bool bPrintTitle ;
    Bool bFlushAfterWrite ;
    Bool bStdOut ;
    SInt nTraceLevel;
}SSCLOGINFO;

typedef struct _SSCLOG
{
    SSCLOGINFO li ;

    SInt64     startcycles ;
    long       starttime ;
    long       clockfreq ;
    FILE*      LogFile;
} SSCLOG;

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/

#define MAXTRACELEVELS 6

#ifdef LOG_ENABLE

#define SSCLOG_INITIALISE                   SSCLOG_Initialise
#define SSCLOG_FREE                         SSCLOG_Free

#define SSCLOG_CREATEINSTANCE               SSCLOG_CreateInstance
#define SSCLOG_DESTROYINSTANCE              SSCLOG_DestroyInstance
#define SSCLOG_DESTROYINSTANCE              SSCLOG_DestroyInstance
#define SSCLOG_SETPRINTTIME                 SSCLOG_SetPrintTime
#define SSCLOG_SETPRINTTITLE                SSCLOG_SetPrintTitle
#define SSCLOG_SETFLUSHAFTERWRITE           SSCLOG_SetFlushAfterWrite
#define SSCLOG_SETSTDOUT                    SSCLOG_SetStdOut
#define SSCLOG_TRACE0                       SSCLOG_Trace0
#define SSCLOG_TRACE1                       SSCLOG_Trace1
#define SSCLOG_TRACE2                       SSCLOG_Trace2
#define SSCLOG_TRACE3                       SSCLOG_Trace3
#define SSCLOG_TRACE4                       SSCLOG_Trace4
#define SSCLOG_TRACE5                       SSCLOG_Trace5
#define SSCLOG_STARTTIMINGCOUNTER           SSCLOG_StartTimingCounter
#define SSCLOG_SETTRACELEVEL                SSCLOG_SetTraceLevel
#define SSCLOG_GETTRACELEVEL                SSCLOG_GetTraceLevel
#define SSCLOG_GETMODULETRACELEVELFROMFILE  SSCLOG_GetModuleTraceLevelFromFile

#else

#define SSCLOG_INITIALISE()
#define SSCLOG_FREE()

#define SSCLOG_CREATEINSTANCE(x)                (0)
#define SSCLOG_DESTROYINSTANCE(x)
#define SSCLOG_SETPRINTTIME(x, y)
#define SSCLOG_SETPRINTTITLE(x, y)
#define SSCLOG_SETFLUSHAFTERWRITE(x, y)
#define SSCLOG_SETSTDOUT(x, y)
#define SSCLOG_TRACE0    SSCLOG_NoLog
#define SSCLOG_TRACE1    SSCLOG_NoLog
#define SSCLOG_TRACE2    SSCLOG_NoLog
#define SSCLOG_TRACE3    SSCLOG_NoLog
#define SSCLOG_TRACE4    SSCLOG_NoLog
#define SSCLOG_TRACE5    SSCLOG_NoLog
#define SSCLOG_STARTTIMINGCOUNTER(x)
#define SSCLOG_SETTRACELEVEL(x,y)
#define SSCLOG_GETTRACELEVEL(x)                 (0)
#define SSCLOG_GETMODULETRACELEVELFROMFILE(x)   (5)

#endif

/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

SSC_INTF_EXPORT void     SSCLOG_Initialise();
SSC_INTF_EXPORT void     SSCLOG_Free();

SSC_INTF_EXPORT SSCLOG*  SSCLOG_CreateInstance(SSCLOGINFO *pli);
SSC_INTF_EXPORT void     SSCLOG_DestroyInstance(SSCLOG* pLog);
SSC_INTF_EXPORT void     SSCLOG_SetPrintTime( SSCLOG *pLog, Bool bPrintTime );
SSC_INTF_EXPORT void     SSCLOG_SetPrintTitle( SSCLOG *pLog, Bool bPrintTitle );
SSC_INTF_EXPORT void     SSCLOG_SetFlushAfterWrite( SSCLOG *pLog, Bool bFlushAfterWrite );
SSC_INTF_EXPORT void     SSCLOG_SetStdOut( SSCLOG *pLog, Bool bStdOut );
SSC_INTF_EXPORT void     SSCLOG_SetTraceLevel( SSCLOG *pLog, SInt nTraceLevel );
SSC_INTF_EXPORT SInt     SSCLOG_GetTraceLevel( SSCLOG *pLog );
SSC_INTF_EXPORT void     SSCLOG_Trace0(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_Trace1(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_Trace2(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_Trace3(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_Trace4(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_Trace5(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_NoLog(SSCLOG* pLog, char* format, ...);
SSC_INTF_EXPORT void     SSCLOG_StartTimingCounter( SSCLOG *pLog );
SSC_INTF_EXPORT SInt     SSCLOG_GetModuleTraceLevelFromFile( Char *pszModName );

#ifdef __cplusplus
}
#endif

#endif /* __ssc_log_h_ */
