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

Source file: SSC_Pipe.c

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
************************************************************************/


/*===========================================================================
 *
 *  Module      : SSC_Pipe
 *
 *  File        : SSC_Pipe.c
 *
 *  Description : This module implements the sbunch pipe used during encoding.
 *
 ============================================================================*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "PlatformTypes.h"
#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Bunch_Types.h"
#include "SSC_Bunch.h"
#include "SSC_Pipe.h"


/*********************************************************/
/*                                                       */
/*                                                       */
/* global functions                                      */
/*                                                       */
/*                                                       */
/*********************************************************/



SSC_INTF_EXPORT SSCPIPE *SSCPIPE_InitInstance( SInt nChannels, SInt nSubframeSize, SInt nDecoder_level, SInt nHistPipeLength, SInt nAheadPipeLength, SSCLOG *pLog )
{
    SInt     i;
    SSCPIPE *pSscPipe = SSC_CALLOC( sizeof(SSCPIPE), 1, "");
    Bool     bSuccess = True ;

    if( pSscPipe && nAheadPipeLength > 0)
    {
        pSscPipe->pLog = pLog ;
        SSCLOG_TRACE1( pSscPipe->pLog, "SSCPIPE_CreateInstance: %s\n", (pSscPipe->pLog) ? (pSscPipe->pLog->li.szTitle) : ("") );
        pSscPipe->nChannels = nChannels ;
        if( nHistPipeLength > 0 )
            pSscPipe->ppHistSignalPipe = SSC_CALLOC( nHistPipeLength * sizeof(SSCBUNCH*), 1, "");
        if( pSscPipe->ppHistSignalPipe )
        {
            pSscPipe->nHistPipeLength = nHistPipeLength ;
            for(i=0; i<pSscPipe->nHistPipeLength; i++ )
            {
                pSscPipe->ppHistSignalPipe[i] = SSCBUNCH_InitInstance( nChannels, nSubframeSize, nDecoder_level, pLog );
                if( pSscPipe->ppHistSignalPipe[i] == NULL )
                    break ;
            }
            bSuccess = (i==pSscPipe->nHistPipeLength)  ;
        }
        if( bSuccess )
        {
            if( nAheadPipeLength > 0 )
                pSscPipe->ppAheadSignalPipe = SSC_CALLOC( (nAheadPipeLength) * sizeof(SSCBUNCH*), 1, "" );

            if( pSscPipe->ppAheadSignalPipe )
            {
                pSscPipe->nAheadPipeLength = nAheadPipeLength ;
                for(i=0; i<pSscPipe->nAheadPipeLength-1; i++ )
                {
                    pSscPipe->ppAheadSignalPipe[i] = SSCBUNCH_InitInstance( nChannels, nSubframeSize, nDecoder_level, pLog );
                    if( pSscPipe->ppAheadSignalPipe[i] == NULL )
                        break ;
                }
                bSuccess = (i==pSscPipe->nAheadPipeLength-1)  ;
            }
        }
    }
    if( !bSuccess || (nAheadPipeLength <= 0))
    {
        if( pSscPipe )
        {
            SSCPIPE_DestroyInstance( pSscPipe );
        }
        pSscPipe = NULL ;
    }
    return pSscPipe ;
}

SSC_INTF_EXPORT SSCBUNCH *SSCPIPE_GetAt( SSCPIPE *pSscPipe, SInt nIndex )
{
    SSCBUNCH *pSscRet = NULL ;

    if( pSscPipe )
    {
        int nHistLength  = SSCPIPE_GetHistItemCount(pSscPipe);
        int nAheadLength = SSCPIPE_GetAheadItemCount(pSscPipe);
        int nIndexCopy = nIndex ;

        if( nHistLength > 0 )
        {
            nIndexCopy = nIndex ; /* done this way to prevent compiler warning (non logging) */
            if( (nIndex >= 0) && (nIndex < nHistLength) )
            {
                pSscRet = pSscPipe->ppHistSignalPipe[nIndex] ;
            }
            nIndex -= nHistLength ;
        }
        if( (pSscRet == NULL) && (nAheadLength > 0) )
        {
            nIndexCopy = nIndex ; /* done this way to prevent compiler warning (non logging) */
            if( (nIndex >= 0) && (nIndex < nAheadLength) )
            {
                pSscRet = pSscPipe->ppAheadSignalPipe[nIndex] ;
            }
        }
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_GetAt(%d) sf_counter %d\n", nIndexCopy,
                      pSscRet ? pSscRet->pBunch->sf_counter : -1 );
    }
    return pSscRet ;
}

SSC_INTF_EXPORT SInt SSCPIPE_GetHistItemCount( SSCPIPE *pSscPipe )
{
    if( pSscPipe )
    {
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_GetHistItemCount returned %d\n", pSscPipe->nHistPipeLength );
        return pSscPipe->nHistPipeLength ;
    }
    else
        return 0 ;
}

SSC_INTF_EXPORT SInt SSCPIPE_GetAheadItemCount( SSCPIPE *pSscPipe )
{
    if( pSscPipe )
    {
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_GetAheadItemCount returned %d\n", pSscPipe->nAheadPipeLength );
        return pSscPipe->nAheadPipeLength ;
    }
    else
        return 0 ;
}

SSC_INTF_EXPORT SInt SSCPIPE_GetItemCount( SSCPIPE *pSscPipe )
{
    SInt nItems = 0 ;
    if( pSscPipe )
    {
        nItems = pSscPipe->nHistPipeLength + pSscPipe->nAheadPipeLength;
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_GetItemCount returned %d\n", nItems );
    }
    return nItems ;
}

SSC_INTF_EXPORT void SSCPIPE_Add( SSCPIPE *pSscPipe, SSCBUNCH *pSscBunch )
{
    if( pSscPipe)
    {
        if( pSscBunch )
        {
            pSscBunch->pLog = pSscPipe->pLog ;
        }
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_Add SBunch, sf counter %d\n", pSscBunch ? pSscBunch->pBunch->sf_counter : -1 );
        pSscPipe->ppAheadSignalPipe[pSscPipe->nAheadPipeLength-1] = pSscBunch ;
    }
}

SSC_INTF_EXPORT SSCBUNCH *SSCPIPE_Shift( SSCPIPE *pSscPipe )
{
    SInt    i ;
    SSCBUNCH *pSscRet = NULL ;

    if( pSscPipe)
    {
        pSscRet = pSscPipe->ppAheadSignalPipe[0] ;
        for( i=0; i<pSscPipe->nAheadPipeLength-1; i++ )
        {
            pSscPipe->ppAheadSignalPipe[i] = pSscPipe->ppAheadSignalPipe[i+1];
        }
        pSscPipe->ppAheadSignalPipe[pSscPipe->nAheadPipeLength-1] = NULL ;

        /* shift history pipe and add a copy of the normal pipe to the end of it */
        {
            if( pSscPipe->nHistPipeLength > 0 )
            {
                /* get a pointer to the bunch which falls out of the history pipe */
                SSCBUNCH *pSscHistOut = pSscPipe->ppHistSignalPipe[0] ;

                /* shift the history pipe */
                for( i=0; i<pSscPipe->nHistPipeLength-1; i++ )
                {
                    pSscPipe->ppHistSignalPipe[i] = pSscPipe->ppHistSignalPipe[i+1];
                }
                /* make a copy of the bunch to be returned to the free history bunch */
                SSCBUNCH_Copy( pSscHistOut, pSscRet );
                /* add the copied item to the end of the history pipe */
                pSscPipe->ppHistSignalPipe[pSscPipe->nHistPipeLength-1] = pSscHistOut ;
            }
        }
        SSCLOG_TRACE0( pSscPipe->pLog, "SSCPIPE_Shift returned SBunch, sf counter %d\n", pSscRet ? pSscRet->pBunch->sf_counter : -1 );
    }
    if( pSscRet )
    {
        pSscRet->pLog = NULL ;
    }
    return pSscRet ;
}

SSC_INTF_EXPORT void SSCPIPE_DestroyInstance( SSCPIPE *pSscPipe )
{
    if( pSscPipe )
    {
        if( pSscPipe->ppHistSignalPipe )
        {
            SInt i;

            for(i=0; i<pSscPipe->nHistPipeLength; i++ )
                SSCBUNCH_DestroyInstance( pSscPipe->ppHistSignalPipe[i] );

            SSC_FREE( pSscPipe->ppHistSignalPipe, "");
        }
        if( pSscPipe->ppAheadSignalPipe )
        {
            SInt i;

            for(i=0; i<pSscPipe->nAheadPipeLength; i++ )
                SSCBUNCH_DestroyInstance( pSscPipe->ppAheadSignalPipe[i] );

            SSC_FREE( pSscPipe->ppAheadSignalPipe, "");
        }
        SSCLOG_TRACE1( pSscPipe->pLog, "SSCPIPE_DestroyInstance: %s\n", (pSscPipe->pLog) ? (pSscPipe->pLog->li.szTitle) : ("") );
        SSC_FREE( pSscPipe, "" );
    }
}
