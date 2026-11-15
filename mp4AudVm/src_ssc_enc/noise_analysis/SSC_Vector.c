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

Source file: SSC_Vector.c

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
 *   Module              : SSC Vector Calculation implementation
 *
 *   Description         : Implementation of the vector process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with SSC_VECT_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <math.h>
#include <string.h>

#include "SSC_Vector.h"

#include "SSC_System.h"
#include "SSC_Alloc.h"
#include "SSC_Error.h"

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


void SSC_VECT_Initialise( void )
{
}


void SSC_VECT_Free( void )
{
}


void* SSC_VECT_CreateInstance ( SSCLOG* const pLog, const SSC_VECT_TYPE type, const SInt length )
{
    SSC_VECT_INFO* pmi = NULL ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        SInt numBytes = byteLengthInfoStruct;

        switch(type)
        {
            case SSC_VECT_DOUBLE:
                numBytes += length * sizeof(Double);
                break;
            case SSC_VECT_FLOAT:
                numBytes += length * sizeof(FloatSSC);
                break;
            default:
               /* CHECK IF VALID TYPE IS PASSED*/
                SSCLOG_TRACE5(pLog, "Error: Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

        /* an empty state struct is allocated */
        pmi = SSC_CALLOC( numBytes, 1, "VECTOR Instance" );
        if ( pmi == NULL )
        {
            SSCLOG_TRACE5(pLog, "\nError: Unable to alloc requested memory for VECTOR instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_VECT_Initialise();

        /* set at begin/end of struct some identification*/
        strcpy(pmi->indentB, SSC_VECT_BEGIN);
        strcpy(pmi->indentE, SSC_VECT_END  );
        /* get the log object from the caller for logging */
        pmi->pLog   = pLog;
        pmi->length = length;
        pmi->type   = type;

        /* the last line alters the pointer to a value to the actual data*/
        pmi++;

    }
    Catch(Error)
    {
        if ( pmi != NULL )
        {
            pmi = SSC_VECT_DestroyInstance( pmi++ );
        }
    }

    return pmi;
}


void* SSC_VECT_DestroyInstance( void* const pData )
{
    SSC_VECT_INFO*  const pmi = (((SSC_VECT_INFO*)pData) - 1 );

    if ( pData != NULL )
    {

        SSC_VECT_Free();

        SSC_FREE( pmi, "VECTOR Instance" );
    }

    /* return NULL to indicate that memory is freed */
    return NULL;
}


Bool SSC_VECT_Clear(void* const pData)
{
    SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Clear function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Clear function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch(pmi->type)
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((Double*)pData)[idx] = (Double)0.0;
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((FloatSSC*)pData)[idx] = (FloatSSC)0.0;
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Clear Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }
    }
    Catch(Error)
    {
        rvalue = False;
    }


    return rvalue;
}


Bool SSC_VECT_Set(void* const pData, const void* const pValue)
{
    SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Set function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Set function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((Double*)pData)[idx] = *((Double*)pValue);
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((FloatSSC*)pData)[idx] = *((FloatSSC*)pValue);
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Set Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }
    }
    Catch(Error)
    {
        rvalue = False;
    }


    return rvalue;
}



Bool SSC_VECT_Copy(const void* const pDataSrc, SInt startSrc, void* const pDataDest, SInt startDest, SInt count)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (startSrc == 0) && (startDest == 0) && (count == 0) )
        {
            count = pmiSrc->length;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (startSrc + count) > pmiSrc->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy function called with length [%d] with start[%d] count[%d]\n", pmiSrc->length, startSrc, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (startDest + count) > pmiDest->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Copy function called with length [%d] with start[%d] count[%d]\n", pmiDest->length, startDest, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                memcpy(&((Double*)pDataDest)[startDest], &((Double*)pDataSrc)[startSrc], sizeof(Double)*count);
                break;
            case SSC_VECT_FLOAT:
                memcpy(&((FloatSSC* )pDataDest)[startDest], &((FloatSSC* )pDataSrc)[startSrc], sizeof(FloatSSC )*count);
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Copy Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Move(const void* const pDataSrc, SInt startSrc, void* const pDataDest, SInt startDest, SInt count)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (startSrc == 0) && (startDest == 0) && (count == 0) )
        {
            count = pmiSrc->length;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (startSrc + count) > pmiSrc->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move function called with length [%d] with start[%d] count[%d]\n", pmiSrc->length, startSrc, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (startDest + count) > pmiDest->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Move function called with length [%d] with start[%d] count[%d]\n", pmiDest->length, startDest, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                memmove(&((Double*)pDataDest)[startDest], &((Double*)pDataSrc)[startSrc], sizeof(Double)*count);
                break;
            case SSC_VECT_FLOAT:
                memmove(&((FloatSSC* )pDataDest)[startDest], &((FloatSSC* )pDataSrc)[startSrc], sizeof(FloatSSC )*count);
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Move Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Acc(const void* const pDataSrc, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    SInt  count = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Acc function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Acc function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Acc function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Acc on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( pmiDest->length < pmiSrc->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Acc on unequal length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        count = pmiSrc->length;

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < count; idx++)
                {
                    ((Double*)pDataDest)[idx] += ((Double*)pDataSrc)[idx];
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < count; idx++)
                {
                    ((FloatSSC* )pDataDest)[idx] += ((FloatSSC* )pDataSrc)[idx];
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Acc Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Abs(const void* const pDataSrc, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Abs function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Abs function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Abs function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Abs on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( pmiDest->length != pmiSrc->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Abs on unequal length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmiDest->length; idx++)
                {
                    if ( ((Double*)pDataSrc)[idx] >= (Double)0.0 )
                    {
                        ((Double*)pDataDest)[idx] =  ((Double*)pDataSrc)[idx];
                    }
                    else
                    {
                        ((Double*)pDataDest)[idx] = -((Double*)pDataSrc)[idx];
                    }
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmiDest->length; idx++)
                {
                    if ( ((FloatSSC* )pDataSrc)[idx] >= (FloatSSC )0.0 )
                    {
                        ((FloatSSC* )pDataDest)[idx] =  ((FloatSSC* )pDataSrc)[idx];
                    }
                    else
                    {
                        ((FloatSSC* )pDataDest)[idx] = -((FloatSSC* )pDataSrc)[idx];
                    }
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Abs Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Mpc(const void* const pDataSrc, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt idx    = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpc function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpc function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpc function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpc on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( pmiDest->length != pmiSrc->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpc on unequal length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmiDest->length; idx++)
                {
                    ((Double*)pDataDest)[idx] *= ((Double*)pDataSrc)[idx];
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmiDest->length; idx++)
                {
                    ((FloatSSC* )pDataDest)[idx] *= ((FloatSSC* )pDataSrc)[idx];
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Mpc Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_FFAbs (const void* const pDataSrc, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmiSrc  = (((SSC_VECT_INFO*)pDataSrc ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt idx1   = 0;
    SInt idx2   = 0;
    SInt count  = 0;

    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pDataSrc == NULL) || (pDataDest == NULL) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmiSrc->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiSrc->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( pmiDest->type != pmiSrc->type)
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (pmiSrc->length % 2) != 0 )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs src not even length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( pmiDest->length < (pmiSrc->length/2 + 1) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector FFAbs dest length[%d] too small for src length[%d]\n", pmiDest->length, pmiSrc->length);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        count = pmiSrc->length/2;

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    /* first */
                    if ( ((Double*)pDataDest)[0] >= (Double)0.0 )
                    {
                        ((Double*)pDataDest)[0] = ((Double*)pDataSrc)[0];
                    }
                    else
                    {
                        ((Double*)pDataDest)[0] = -((Double*)pDataSrc)[0];
                    }

                    /* last */
                    if ( ((Double*)pDataDest)[count] >= (Double)0.0 )
                    {
                        ((Double*)pDataDest)[count] = ((Double*)pDataSrc)[count];
                    }
                    else
                    {
                        ((Double*)pDataDest)[0] = -((Double*)pDataSrc)[0];
                    }

                    /* the rest */
                    for ( idx1 = (count - 1), idx2 = (count + 1); idx1 > 0; idx1--, idx2++ )
                    {
                        ((Double*)pDataDest)[idx1] = (Double)sqrt(
                                                                  ((Double*)pDataSrc)[idx1] * ((Double*)pDataSrc)[idx1] +
                                                                  ((Double*)pDataSrc)[idx2] * ((Double*)pDataSrc)[idx2]
                                                                 );
                    }
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    /* first */
                    if ( ((FloatSSC*)pDataDest)[0] >= (FloatSSC)0.0 )
                    {
                        ((FloatSSC*)pDataDest)[0] = ((FloatSSC*)pDataSrc)[0];
                    }
                    else
                    {
                        ((FloatSSC*)pDataDest)[0] = -((FloatSSC*)pDataSrc)[0];
                    }

                    /* last */
                    if ( ((FloatSSC*)pDataDest)[count] >= (FloatSSC)0.0 )
                    {
                        ((FloatSSC*)pDataDest)[count] = ((FloatSSC*)pDataSrc)[count];
                    }
                    else
                    {
                        ((FloatSSC*)pDataDest)[0] = -((FloatSSC*)pDataSrc)[0];
                    }

                    /* the rest */
                    for ( idx1 = (count - 1), idx2 = (count + 1); idx1 > 0; idx1--, idx2++ )
                    {
                        ((FloatSSC*)pDataDest)[idx1] = (FloatSSC)sqrt(
                                                                ((FloatSSC*)pDataSrc)[idx1] * ((FloatSSC*)pDataSrc)[idx1] +
                                                                ((FloatSSC*)pDataSrc)[idx2] * ((FloatSSC*)pDataSrc)[idx2]
                                                               );
                    }
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector FFAbs Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Sub(const void* const pData1, const void* const pData2, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmi1    = (((SSC_VECT_INFO*)pData1   ) - 1);
    const SSC_VECT_INFO* const pmi2    = (((SSC_VECT_INFO*)pData2   ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pData1 == NULL) || (pData2 == NULL) || (pDataDest == NULL))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi1->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi1->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi2->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi2->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( (pmi1->type != pmi2->type) || (pmi2->type != pmiDest->type) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (pmi1->length != pmi2->length) || (pmiDest->length != pmi2->length) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sub on to much count for dest or src\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmi1->length; idx++)
                {
                    ((Double*)pDataDest)[idx] = ((Double*)pData1)[idx] - ((Double*)pData2)[idx];
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmi1->length; idx++)
                {
                    ((FloatSSC* )pDataDest)[idx] = ((FloatSSC* )pData1)[idx] - ((FloatSSC* )pData2)[idx];
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Sub Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Mpy(const void* const pData1, SInt startData1, const void* const pData2, SInt startData2, void* const pDataDest, SInt startDest, SInt count)
{
    const SSC_VECT_INFO* const pmi1    = (((SSC_VECT_INFO*)pData1   ) - 1);
    const SSC_VECT_INFO* const pmi2    = (((SSC_VECT_INFO*)pData2   ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt   idx1 = 0;
    SInt   idx2 = 0;
    SInt   idx3 = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pData1 == NULL) || (pData2 == NULL) || (pDataDest == NULL))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi1->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi1->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi2->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi2->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( (pmi1->type != pmi2->type) || (pmi2->type != pmiDest->type) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (count == 0) && (startData1 == 0) && (startData2 == 0) && (startDest == 0) )
        {
            count = pmi1->length;
        }

        /* check the length */
        if ( (startData1 + count) > pmi1->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for src1\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (startData2 + count) > pmi2->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for scr2\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (startDest + count) > pmiDest->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for dest\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx1 = startData1, idx2 = startData2, idx3 = startDest; idx1 < (startData1+count); idx1++, idx2++, idx3++)
                {
                    ((Double*)pDataDest)[idx3] = ((Double*)pData1)[idx1] * ((Double*)pData2)[idx2];
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx1 = startData1, idx2 = startData2, idx3 = startDest; idx1 < (startData1+count); idx1++, idx2++, idx3++)
                {
                    ((FloatSSC* )pDataDest)[idx3] = ((FloatSSC* )pData1)[idx1] * ((FloatSSC* )pData2)[idx2];
                }
                break;
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Mpy Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Div(const void* const pData1, SInt startData1, const void* const pData2, SInt startData2, void* const pDataDest, SInt startDest, SInt count)
{
    const SSC_VECT_INFO* const pmi1    = (((SSC_VECT_INFO*)pData1   ) - 1);
    const SSC_VECT_INFO* const pmi2    = (((SSC_VECT_INFO*)pData2   ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt   idx1 = 0;
    SInt   idx2 = 0;
    SInt   idx3 = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( (pData1 == NULL) || (pData2 == NULL) || (pDataDest == NULL))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Div called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Div function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi1->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi1->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Div function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi2->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi2->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Div function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( (pmi1->type != pmi2->type) || (pmi2->type != pmiDest->type) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Div on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (count == 0) && (startData1 == 0) && (startData2 == 0) && (startDest == 0) )
        {
            count = pmi1->length;
        }

        /* check the length */
        if ( (startData1 + count) > pmi1->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for src1\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (startData2 + count) > pmi2->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for scr2\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        if ( (startDest + count) > pmiDest->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Mpy on too much count for dest\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }


        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx1 = startData1, idx2 = startData2, idx3 = startDest; idx1 < (startData1+count); idx1++, idx2++, idx3++)
                {
                    ((Double*)pDataDest)[idx3] = ((Double*)pData1)[idx1] / ((Double*)pData2)[idx2];
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx1 = startData1, idx2 = startData2, idx3 = startDest; idx1 < (startData1+count); idx1++, idx2++, idx3++)
                {
                    ((FloatSSC* )pDataDest)[idx3] = ((FloatSSC* )pData1)[idx1] / ((FloatSSC* )pData2)[idx2];
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Div Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_FFDiv(const void* const pData1, const void* const pData2, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmi1    = (((SSC_VECT_INFO*)pData1   ) - 1);
    const SSC_VECT_INFO* const pmi2    = (((SSC_VECT_INFO*)pData2   ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt   idx1 = 0;
    SInt   idx2 = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    SInt count = 0;

    Try
    {
        /* check on valid instances */
        if ( (pData1 == NULL) || (pData2 == NULL) || (pDataDest == NULL))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi1->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi1->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi2->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi2->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( (pmi1->type != pmi2->type) || (pmi2->type != pmiDest->type) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (pmi1->length != pmi2->length) || (pmi1->length != pmiDest->length) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC on unequal length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check even length */
        if ( (pmi1->length % 2) != 0 )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC length not even\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        count = pmiDest->length;

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double         t1 = (Double)0.0;
                    Double         t3 = (Double)0.0;
                    Double         t2 = (Double)0.0;
                    Double         t4 = (Double)0.0;
                    Double oneOverDen = (Double)0.0;

                    ((Double*)pDataDest)[0      ] = ((Double*)pData1)[0      ] / ((Double*)pData2)[0      ];
                    ((Double*)pDataDest)[count/2] = ((Double*)pData1)[count/2] / ((Double*)pData2)[count/2];
                    for ( idx1 = 1, idx2 = count - 1; idx1 < count/2; idx1++, idx2--)
                    {
                        t3 = ((Double*)pData2)[idx1];
                        t4 = ((Double*)pData2)[idx2];
                        oneOverDen = (Double)1.0/(t3*t3+t4*t4);

                        t1 = ((Double*)pData1)[idx1];
                        t2 = ((Double*)pData1)[idx2];

                        ((Double*)pDataDest)[idx1] = (t1 * t3 + t2 * t4) * oneOverDen;
                        ((Double*)pDataDest)[idx2] = (t2 * t3 - t1 * t4) * oneOverDen;

                    }
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC         t1 = (FloatSSC)0.0;
                    FloatSSC         t3 = (FloatSSC)0.0;
                    FloatSSC         t2 = (FloatSSC)0.0;
                    FloatSSC         t4 = (FloatSSC)0.0;
                    FloatSSC oneOverDen = (FloatSSC)0.0;

                    ((FloatSSC*)pDataDest)[0      ] = ((FloatSSC*)pData1)[0      ] / ((FloatSSC*)pData2)[0      ];
                    ((FloatSSC*)pDataDest)[count/2] = ((FloatSSC*)pData1)[count/2] / ((FloatSSC*)pData2)[count/2];
                    for ( idx1 = 1, idx2 = count - 1; idx1 < count/2; idx1++, idx2--)
                    {
                        t3 = ((FloatSSC*)pData2)[idx1];
                        t4 = ((FloatSSC*)pData2)[idx2];
                        oneOverDen = (FloatSSC)1.0/(t3*t3+t4*t4);

                        t1 = ((FloatSSC*)pData1)[idx1];
                        t2 = ((FloatSSC*)pData1)[idx2];

                        ((FloatSSC*)pDataDest)[idx1] = (t1 * t3 + t2 * t4) * oneOverDen;
                        ((FloatSSC*)pDataDest)[idx2] = (t2 * t3 - t1 * t4) * oneOverDen;

                    }
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Div Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_MpyC(const void* const pData1, const void* const pData2, void* const pDataDest)
{
    const SSC_VECT_INFO* const pmi1    = (((SSC_VECT_INFO*)pData1   ) - 1);
    const SSC_VECT_INFO* const pmi2    = (((SSC_VECT_INFO*)pData2   ) - 1);
          SSC_VECT_INFO* const pmiDest = (((SSC_VECT_INFO*)pDataDest) - 1);

    SSCLOG* pLog = NULL;

    SInt   idx1 = 0;
    SInt   idx2 = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    SInt count = 0;

    Try
    {
        /* check on valid instances */
        if ( (pData1 == NULL) || (pData2 == NULL) || (pDataDest == NULL))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmiDest->pLog;

        if ( (strcmp(pmiDest->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmiDest->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi1->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi1->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (strcmp(pmi2->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi2->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check each other types */
        if ( (pmi1->type != pmi2->type) || (pmi2->type != pmiDest->type) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC on different type\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check the length */
        if ( (pmi1->length != pmi2->length) || (pmi1->length != pmiDest->length) )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC on unequal length\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        /* check even length */
        if ( (pmi1->length % 2) != 0 )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector MpyC length not even\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        count = pmiDest->length;

        switch ( pmiDest->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double t1 = (Double)0.0;
                    Double t3 = (Double)0.0;
                    Double t2 = (Double)0.0;
                    Double t4 = (Double)0.0;

                    ((Double*)pDataDest)[0      ] = ((Double*)pData1)[0      ] * ((Double*)pData2)[0      ];
                    ((Double*)pDataDest)[count/2] = ((Double*)pData1)[count/2] * ((Double*)pData2)[count/2];
                    for ( idx1 = 1, idx2 = count - 1; idx1 < count/2; idx1++, idx2--)
                    {
                        t1 = ((Double*)pData1)[idx1];
                        t3 = ((Double*)pData2)[idx1];
                        t2 = ((Double*)pData1)[idx2];
                        t4 = ((Double*)pData2)[idx2];

                        ((Double*)pDataDest)[idx1] = t1 * t3 + t2 * t4;
                        ((Double*)pDataDest)[idx2] = t1 * t4 - t2 * t3;

                    }
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC t1 = (FloatSSC)0.0;
                    FloatSSC t3 = (FloatSSC)0.0;
                    FloatSSC t2 = (FloatSSC)0.0;
                    FloatSSC t4 = (FloatSSC)0.0;

                    ((FloatSSC*)pDataDest)[0      ] = ((FloatSSC*)pData1)[0      ] * ((FloatSSC*)pData2)[0      ];
                    ((FloatSSC*)pDataDest)[count/2] = ((FloatSSC*)pData1)[count/2] * ((FloatSSC*)pData2)[count/2];
                    for ( idx1 = 1, idx2 = count - 1; idx1 < count/2; idx1++, idx2--)
                    {
                        t1 = ((FloatSSC*)pData1)[idx1];
                        t3 = ((FloatSSC*)pData2)[idx1];
                        t2 = ((FloatSSC*)pData1)[idx2];
                        t4 = ((FloatSSC*)pData2)[idx2];

                        ((FloatSSC*)pDataDest)[idx1] = t1 * t3 + t2 * t4;
                        ((FloatSSC*)pDataDest)[idx2] = t1 * t4 - t2 * t3;
                    }
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector MpyC Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Max(const void* const pData, void* const pMax)
{
    const SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Max function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Max function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( pmi->length <= 0 )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Max function called with length [%d]\n", pmi->length);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double tmpMax = ((Double*)pData)[0];

                    for ( idx = 1; idx < pmi->length; idx++)
                    {
                        if ( ((Double*)pData)[idx] > tmpMax )
                        {
                            tmpMax = ((Double*)pData)[idx];
                        }
                    }

                    ((Double*)pMax)[0] = tmpMax;
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC tmpMax = ((FloatSSC*)pData)[0];

                    for ( idx = 1; idx < pmi->length; idx++)
                    {
                        if ( ((FloatSSC*)pData)[idx] > tmpMax )
                        {
                            tmpMax = ((FloatSSC*)pData)[idx];
                        }
                    }

                    ((FloatSSC*)pMax)[0] = tmpMax;
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Max Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Sum(const void* const pData, SInt start, SInt count, void* const pSum)
{
    const SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sum function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Sum function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (start == 0) && (count == 0) )
        {
            count = pmi->length;
        }

        if ( (start + count) > pmi->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumAbs function called with length [%d] with start[%d] count[%d]\n", pmi->length, start, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double tmpSum = (Double)0.0;

                    for ( idx = start; idx < count; idx++)
                    {
                        tmpSum += ((Double*)pData)[idx];
                    }

                    ((Double*)pSum)[0] = tmpSum;
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC tmpSum = (FloatSSC)0.0;

                    for ( idx = start; idx < count; idx++)
                    {
                        tmpSum += ((FloatSSC*)pData)[idx];
                    }

                    ((FloatSSC*)pSum)[0] = tmpSum;
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Sum Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_SumAbs(const void* const pData, SInt start, SInt count, void* const pSumAbs)
{
    const SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumAbs function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumAbs function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (start == 0) && (count == 0) )
        {
            count = pmi->length;
        }

        if ( (start + count) > pmi->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumAbs function called with length [%d] with start[%d] count[%d]\n", pmi->length, start, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double tmpSumAbs = (Double)0.0;
                    for ( idx = start; idx < (start + count); idx++)
                    {
                        if ( ((Double*)pData)[idx] >= (Double)0.0 )
                        {
                            tmpSumAbs += ((Double*)pData)[idx];
                        }
                        else
                        {
                            tmpSumAbs -= ((Double*)pData)[idx];
                        }
                    }
                    *((Double*)pSumAbs) = tmpSumAbs;
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC tmpSumAbs = (FloatSSC)0.0;
                    for ( idx = start; idx < (start + count); idx++)
                    {
                        if ( ((FloatSSC*)pData)[idx] >= (FloatSSC)0.0 )
                        {
                            tmpSumAbs += ((FloatSSC*)pData)[idx];
                        }
                        else
                        {
                            tmpSumAbs -= ((FloatSSC*)pData)[idx];
                        }

                    }
                    *((FloatSSC*)pSumAbs) = tmpSumAbs;
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector SumAbs Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_SumSqr(const void* const pData, SInt start, SInt count, void* const pSumSqr)
{
    const SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumSqr function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumSqr function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( (start == 0) && (count == 0) )
        {
            count = pmi->length;
        }

        if ( (start + count) > pmi->length )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector SumSqr function called with length [%d] with start[%d] count[%d]\n", pmi->length, start, count);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }
        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                {
                    Double tmpEnergy = (Double)0.0;
                    for ( idx = start; idx < (start + count); idx++)
                    {
                        tmpEnergy += ((Double*)pData)[idx] * ((Double*)pData)[idx];
                    }
                    ((Double*)pSumSqr)[0] = tmpEnergy;
                }
                break;
            case SSC_VECT_FLOAT:
                {
                    FloatSSC tmpEnergy = (Double)0.0;
                    for ( idx = start; idx < (start + count); idx++)
                    {
                        tmpEnergy += ((FloatSSC*)pData)[idx] * ((FloatSSC*)pData)[idx];
                    }
                    ((FloatSSC*)pSumSqr)[0] = tmpEnergy;
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector SumSqr Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}


Bool SSC_VECT_Scale(void* const pData, const void* const pScale)
{
    const SSC_VECT_INFO* const pmi = (((SSC_VECT_INFO*)pData) - 1);

    SSCLOG* pLog = NULL;

    SInt    idx = 0;
    Bool rvalue = True;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* check on valid instances */
        if ( pData == NULL )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Scale function called with NULL instance\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        pLog = pmi->pLog;

        if ( (strcmp(pmi->indentB, SSC_VECT_BEGIN) != 0) || (strcmp(pmi->indentE, SSC_VECT_END) != 0))
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Scale function called with no valid vector pointer\n");
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        if ( pmi->length <= 0 )
        {
            SSCLOG_TRACE5(pLog, "Error: Vector Scale function called with length [%d]\n", pmi->length);
            Throw SSC_ERROR_NOT_AVAILABLE;
        }

        switch ( pmi->type )
        {
            case SSC_VECT_DOUBLE:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((Double*)pData)[idx] *= *((Double*)pScale);
                }
                break;
            case SSC_VECT_FLOAT:
                for ( idx = 0; idx < pmi->length; idx++)
                {
                    ((FloatSSC* )pData)[idx] *= *((FloatSSC* )pScale);
                }
                break;
            default:
                /* should be flagged in init*/
                SSCLOG_TRACE5(pLog, "Error: Vector Scale Undefined vector type passed\n");
                Throw SSC_ERROR_NOT_AVAILABLE;
                break;
        }

    }
    Catch(Error)
    {
        rvalue = False;
    }

    return rvalue;
}



