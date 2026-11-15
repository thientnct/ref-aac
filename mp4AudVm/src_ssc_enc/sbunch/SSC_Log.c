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

Source file: SSC_Log.c

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
 *   Module              : Logging ssc tool
 *
 *   Description         : This module implements a file logging facility.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with SSCLOG_
 *
 ******************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "SSC_Log.h"
#include "SSC_Alloc.h"
#include "PlatformTypes.h"
#include "SSC_Error.h"
#include "SSC_System.h"
#include "cexcept.h"

#ifdef LOG_ENABLE

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

#define CPUID __asm __emit 0fh __asm __emit 0a2h
#define RDTSC __asm __emit 0fh __asm __emit 031h

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

static SInt64 LOCAL_ReadTiming(void);
static void   LOCAL_InitClockFreq( SSCLOG *pLog );
static void   LOCAL_PrintTime( SSCLOG *pLog, FILE *pOutput );
static void   LOCAL_PrintTitle( SSCLOG *pLog, FILE *pOutput );
static void   LOCAL_Flush( SSCLOG *pLog, FILE *pOutput );
static void   LOCAL_Print(SSCLOG* pLog, char* format, va_list argp, SInt nTraceLevel );

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


SSC_INTF_EXPORT void SSCLOG_Initialise()
{
    return;
}


SSC_INTF_EXPORT void SSCLOG_Free()
{
    return;
}


SSC_INTF_EXPORT SSCLOG* SSCLOG_CreateInstance(SSCLOGINFO *pli)
{
    SSCLOG* pLog = NULL;

    if( pli != NULL )
    {
        pLog = SSC_MALLOC(sizeof(SSCLOG), "SSC Logging Instance");

        if ( pLog != NULL)
        {
            pLog->LogFile = NULL;
            if ( pli->szFileName[0] != '\0' )
            {
                strncpy( pLog->li.szTitle, pli->szTitle, (sizeof( pLog->li.szTitle ) /sizeof(Char)) - 1 );
                pLog->li.szTitle[sizeof(pLog->li.szTitle)/sizeof(Char) - 1] = '\0' ;
                pLog->LogFile = fopen( pli->szFileName,"w");
                if ( pLog->LogFile == NULL )
                {
                    fprintf(stdout, "\nError: Unable to open log file: %s\n", pli->szFileName);
                }
            }
            strncpy( pLog->li.szFileName, pli->szFileName, (sizeof(pLog->li.szFileName)/sizeof(Char)) -1 );
            pLog->li.szFileName[sizeof(pLog->li.szFileName)/sizeof(Char) - 1] = '\0' ;
            pLog->li.nTraceLevel = pli->nTraceLevel;
            pLog->li.bPrintTime = pli->bPrintTime ;
            pLog->li.bPrintTitle = pli->bPrintTitle ;
            pLog->li.bFlushAfterWrite = pli->bFlushAfterWrite ;
            pLog->li.bStdOut = pli->bStdOut ;
            pLog->clockfreq = 0 ;
            pLog->startcycles = LOCAL_ReadTiming();
            pLog->starttime = clock();
        }
        else
        {
            fprintf(stdout, "\nError: Unable to create logging struct\n");
        }
    }
    return pLog;
}


SSC_INTF_EXPORT void SSCLOG_DestroyInstance(SSCLOG* pLog)
{
    if( pLog != NULL )
    {
        if ( pLog->LogFile != NULL )
        {
            Bool bEmpty = (0 == ftell( pLog->LogFile ));

            fclose(pLog->LogFile);
            pLog->LogFile = NULL ;
            if( bEmpty )
            {
                remove( pLog->li.szFileName );
            }
        }
        SSC_FREE(pLog, "SSC Logging Instance");
        pLog = NULL;
    }
}



SSC_INTF_EXPORT void SSCLOG_SetPrintTime( SSCLOG *pLog, Bool bPrintTime )
{
    if( pLog != NULL )
    {
        pLog->li.bPrintTime = bPrintTime ;
    }
}


SSC_INTF_EXPORT void SSCLOG_SetPrintTitle( SSCLOG *pLog, Bool bPrintTitle )
{
    if( pLog != NULL )
    {
        pLog->li.bPrintTitle = bPrintTitle ;
    }
}


SSC_INTF_EXPORT void SSCLOG_SetFlushAfterWrite( SSCLOG *pLog, Bool bFlushAfterWrite )
{
    if( pLog != NULL )
    {
        pLog->li.bFlushAfterWrite = bFlushAfterWrite ;
    }
}


SSC_INTF_EXPORT void SSCLOG_SetStdOut( SSCLOG *pLog, Bool bStdOut )
{
    if( pLog != NULL )
    {
        pLog->li.bStdOut = bStdOut ;
    }
}


SSC_INTF_EXPORT void SSCLOG_SetTraceLevel( SSCLOG *pLog, SInt nTraceLevel )
{
    if( pLog != NULL )
    {
        pLog->li.nTraceLevel = nTraceLevel ;
    }
}


SSC_INTF_EXPORT SInt SSCLOG_GetTraceLevel( SSCLOG *pLog )
{
    if( pLog != NULL )
    {
        return pLog->li.nTraceLevel ;
    }
    else
    {
        return 0 ;
    }
}


/* Reads the log trace level from the file "SSC_TraceLevel.ini" Tries
 * to read the entry 'modname=<tracenumber>' where tracenumber 0 =
 * detailed, 5 toplevel debug only.  If the file is not found it is
 * created in the current directory.  If the entry is not found it is
 * appended to the end of file with a value of 5.  If no file is found
 * or no entry is found a tracelevel of 5 will be returned */
SSC_INTF_EXPORT SInt SSCLOG_GetModuleTraceLevelFromFile( Char *pszModName )
{
    Char  szIniFile[] = "SSC_TraceLevel.ini" ;
    FILE* IniFile;
    /* default minumum trace output */
    SInt  nMinTraceOutput = 5 ;
    SInt  nTraceLevel = nMinTraceOutput ;

    if( pszModName )
    {
        IniFile = fopen( szIniFile, "a+" );
        if( IniFile != NULL )
        {
            Char szLine[100] ;
            Char szFormat[80] ;
            SInt nDescription ;
            SInt nConverted ;

            fseek(IniFile, 0, SEEK_SET);
            /* Search for description, if not found write to file */
            do
            {
                nConverted = fscanf( IniFile, "* Usage=%d\n", &nDescription );
                fgets( szLine, (sizeof(szLine)/sizeof(Char))-1, IniFile );
            }
            while( nConverted == 0 );
            if( nConverted <= 0 )
            {
                /* file open for append, data will always be written to end of file */
                fprintf( IniFile, "*******************************************************************\n" );
                fprintf( IniFile, "* Usage=1                                                         *\n" );
                fprintf( IniFile, "*                                                                 *\n" );
                fprintf( IniFile, "* Define the tracelevel for each ssc_module like this             *\n" );
                fprintf( IniFile, "* modulename=tracelevel                                           *\n" );
                fprintf( IniFile, "*                                                                 *\n" );
                fprintf( IniFile, "* A tracelevel of 0 gives maximum output, 5 gives minimum output. *\n" );
                fprintf( IniFile, "* Any tracelevel > 5 will give no output.                         *\n" );
                fprintf( IniFile, "*                                                                 *\n" );
                fprintf( IniFile, "* If the entry string for a module is not recognized it will be   *\n" );
                fprintf( IniFile, "* appended to the end of the file. So if two uppercase/lowercase  *\n" );
                fprintf( IniFile, "* entries are present remove the first one(s).                    *\n" );
                fprintf( IniFile, "*                                                                 *\n" );
                fprintf( IniFile, "*******************************************************************\n\n" );
                fflush( IniFile );
            }
            if( strlen( pszModName ) < 70 )
            {
                Char *cp = pszModName ;

                while( *cp )
                {
                    *cp = (Char)tolower( *cp );
                    cp++ ;
                }
                sprintf( szFormat, "%s=%%d\n", pszModName );
                do
                {
                    nConverted = fscanf( IniFile, szFormat, &nTraceLevel );
                    fgets( szLine, (sizeof(szLine)/sizeof(Char))-1, IniFile );
                }
                while( nConverted == 0 );

                if( nConverted <= 0 ) /* EOF or no conversion done */
                {
                    /* entry was not found, write tracelevel nMinTraceOutput to end of file */
                    fflush( IniFile );
                    fprintf( IniFile, szFormat, nMinTraceOutput );
                }
            }
            fclose( IniFile );
        }
    }
    return nTraceLevel ;
}


SSC_INTF_EXPORT void SSCLOG_StartTimingCounter( SSCLOG *pLog )
{
    if ( pLog != NULL )
    {
        pLog->startcycles = LOCAL_ReadTiming();
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace0( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 0 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 0 );
            va_end(argp);
        }
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace1( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 1 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 1 );
            va_end(argp);
        }
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace2( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 2 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 2 );
            va_end(argp);
        }
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace3( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 3 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 3 );
            va_end(argp);
        }
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace4( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 4 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 4 );
            va_end(argp);
        }
    }
}


SSC_INTF_EXPORT void SSCLOG_Trace5( SSCLOG *pLog, char* format, ...)
{
    if( pLog )
    {
        if( pLog->li.nTraceLevel <= 5 )
        {
            va_list argp ;
            va_start(argp, format);
            LOCAL_Print(pLog, format, argp, 5 );
            va_end(argp);
        }
    }
}


/*============================================================================*/
/*       STATIC FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


#ifdef PLATFORM_WIN32_MSVC

static SInt64 LOCAL_ReadTiming(void)
{
    UInt32 cycles_high, cycles_low;
    SInt64 total_cycles ;

    __asm {
            pushad
            CPUID
            RDTSC
            mov cycles_high, edx
            mov cycles_low, eax
            popad
    }
    /*  Move the cycle counts into 64-bit integers */
    /* store signed in SInt64 so the value can be converted to a double */
    /* E.g. seconds = double(total_cycles)/double(mhz*1000000); */

    total_cycles = (SInt64)( ((UInt64)cycles_high << 32) | cycles_low);

    return total_cycles ;
}

#else

static SInt64 LOCAL_ReadTiming(void)
{
    return clock();
}
#endif /* PLATFORM_WIN32_MSVC */


static void LOCAL_InitClockFreq( SSCLOG *pLog )
{
    if( pLog != NULL)
    {
        long difftime = (clock() - pLog->starttime)/CLOCKS_PER_SEC ;
        if( difftime > 2.5 )
        {
            SInt64 cyclecount = LOCAL_ReadTiming() - pLog->startcycles ;
            pLog->clockfreq = (long)(cyclecount/ difftime) ;
        }
    }
}


static void LOCAL_PrintTime( SSCLOG *pLog, FILE *pOutput )
{
    if( pLog != NULL )
    {
        if( pLog->li.bPrintTime == True )
        {
            if( pLog->clockfreq == 0 )
            {
                fprintf( pOutput, "*****.*** s " );
            }
            else
            {
                fprintf( pOutput, "% 9.03f s ", (double)(LOCAL_ReadTiming() - pLog->startcycles) / pLog->clockfreq ) ;
            }
        }
    }
}


static void LOCAL_PrintTitle( SSCLOG *pLog, FILE *pOutput )
{
    if( pLog != NULL )
    {
        if( pLog->li.bPrintTitle == True )
        {
            fprintf( pOutput, "%s ", pLog->li.szTitle );
        }
    }
}


static void LOCAL_Flush( SSCLOG *pLog, FILE *pOutput )
{
    if( pLog != NULL )
    {
        if( pLog->li.bFlushAfterWrite == True)
        {
            fflush(pOutput);
        }
    }
}


static void LOCAL_Print(SSCLOG* pLog, char* format, va_list argp, SInt nTraceLevel )
{
    if( pLog )
    {
        if (pLog->LogFile != NULL)
        {
            if( pLog->clockfreq == 0 )
            {
                LOCAL_InitClockFreq(pLog);
            }

            LOCAL_PrintTime( pLog, pLog->LogFile );
            LOCAL_PrintTitle( pLog, pLog->LogFile );
            vfprintf(pLog->LogFile, format, argp);
            LOCAL_Flush( pLog, pLog->LogFile );
        }
        if( pLog->li.bStdOut && (nTraceLevel == 5) )
        {
            SSC_EXCEPTION  tec = SSC_INIT_EXCEPTION ;
            SSC_EXCEPTION* the_exception_context = &tec;
            SSC_ERROR      Error = SSC_ERROR_OK ;
            char           szMessage[512] ;

            Try
            {
                vsprintf( szMessage, format, argp );
            }
            Catch(Error)
            {
                strcpy ( szMessage, "Log message too long: " );
                strncat( szMessage, format, 100 );
                strcat ( szMessage, "...\n" );
            }
            printf( szMessage );
        }
    }
}


#endif /* LOG_ENABLE */


SSC_INTF_EXPORT void     SSCLOG_NoLog(SSCLOG* pLog, char* format, ...)
{
	UNREFERENCED_PARM(pLog);
	UNREFERENCED_PARM(format);
}
