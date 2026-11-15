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

Source file: win.c

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
 *   Module              : WINDOW implementation
 *
 *   Description         : Implementation of the window process.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with SSC_WIN_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <math.h>

#include "win.h"

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


void SSC_WIN_Initialise( void )
{
}


void SSC_WIN_Free( void )
{
}


SSC_WIN_STATE* SSC_WIN_CreateInstance( SSCLOG* const pLog, const SInt winSize, const SSC_WIN_TYPE type )
{
    SSC_WIN_STATE* pms = NULL ;

    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;

    SSC_ERROR Error = SSC_ERROR_OK ;

    Try
    {
        /* an empty state struct is allocated */
        pms = SSC_CALLOC( sizeof(SSC_WIN_STATE), 1, "WIN Instance" );
        if ( pms == NULL )
        {
            fprintf(stdout,"\nError: Unable to alloc requested memory for WIN instance\n");
            Throw SSC_ERROR_OUT_OF_MEMORY;
        }

        SSC_WIN_Initialise();

        /* get the log object from the caller for logging */
        pms->pLog    = pLog;
        pms->type    = type;
        pms->winSize = winSize;
    }
    Catch(Error)
    {
        pms = SSC_WIN_DestroyInstance( pms );
    }

    return pms;
}


SSC_WIN_STATE* SSC_WIN_DestroyInstance( SSC_WIN_STATE* const pms )
{
    if ( pms != NULL )
    {

        SSC_WIN_Free();

        SSC_FREE( pms, "WIN Instance" );
    }

    /* return NULL to indicate that memory is freed */
    return NULL;
}



SInt SSC_WIN_Create( SSC_WIN_STATE* const pms, const SSC_WIN_FORM winForm, void* const pWindow)
{
    SInt rvalue = 0;

    if ( pms != NULL )
    {
        SInt   idx;
        SInt   size = pms->winSize;

        switch( pms->type )
        {
            case SSC_WIN_DOUBLE:
                {
                    Double value = (Double)0.0;

                    switch ( winForm )
                    {
                        case SSC_WIN_RECTANGUAL:
                            value = (Double)1.0;
                            for ( idx = 0; idx < size; idx++)
                            {
                                ((Double*)pWindow)[idx] = value;
                            }
                            break;
                        case SSC_WIN_HANNING:
                            for( idx = 0; idx < size/2; idx++ )
                            {
                                value = (Double) (0.5 - 0.5 * cos( 2*SSC_PI * (idx + 0.5)/ size ));
                                ((Double*)pWindow)[       idx    ] = value;
                                ((Double*)pWindow)[size - idx - 1] = value;
                            }
                            break;
                        case SSC_WIN_SQRT_HANNING:
                            for( idx = 0; idx < size; idx++ )
                            {
                                value = (Double)sin( SSC_PI * (0.5 + idx)/size );
                                ((Double*)pWindow)[idx] = value;
                            }
                            break;
                        case SSC_WIN_HANNING_BURAK:
                            for( idx = 0; idx < size/2; idx++ )
                            {
                                value = (Double) (0.5 - 0.5 * cos(2*SSC_PI * (idx+1) /(size+1)));
                                ((Double*)pWindow)[       idx    ] = value;
                                ((Double*)pWindow)[size - idx - 1] = value;
                            }
                            break;
                        default:
                            SSCLOG_TRACE5(pms->pLog, "Error: illegal window form[%d] specified\n", winForm);
                            rvalue = 1; /* report an error */
                            break;
                    }
                }
                break;
            case SSC_WIN_FLOAT:
                {
                    FloatSSC value = (FloatSSC)0.0;

                    switch ( winForm )
                    {
                        case SSC_WIN_RECTANGUAL:
                            value = (FloatSSC)1.0;
                            for ( idx = 0; idx < size; idx++)
                            {
                                ((FloatSSC*)pWindow)[idx] = value;
                            }
                            break;
                        case SSC_WIN_HANNING:
                            for( idx = 0; idx < size/2; idx++ )
                            {
                                value = (FloatSSC) (0.5 - 0.5 * cos( 2*SSC_PI * (idx + 0.5)/ size ));
                                ((FloatSSC*)pWindow)[       idx    ] = value;
                                ((FloatSSC*)pWindow)[size - idx - 1] = value;
                            }
                            break;
                        case SSC_WIN_SQRT_HANNING:
                            for( idx = 0; idx < size; idx++ )
                            {
                                value = (FloatSSC)sin( SSC_PI * (0.5 + idx)/size );
                                ((FloatSSC*)pWindow)[idx] = value;
                            }
                            break;
                        case SSC_WIN_HANNING_BURAK:
                            for( idx = 0; idx < size/2; idx++ )
                            {
                                value = (FloatSSC) (0.5 - 0.5 * cos(2*SSC_PI * (idx+1) /(size+1)));
                                ((FloatSSC*)pWindow)[       idx    ] = value;
                                ((FloatSSC*)pWindow)[size - idx - 1] = value;
                            }
                            break;
                        default:
                            SSCLOG_TRACE5(pms->pLog, "Error: illegal window form[%d] specified\n", winForm);
                            rvalue = 1; /* report an error */
                            break;
                    }
                }
                break;
            default:
                SSCLOG_TRACE5(pms->pLog, "Error: illegal window type[%d] specified\n", pms->type);
                rvalue = 1; /* report an error */
                break;
        }

    }
    return rvalue;
}

/*============================================================================*/
/*        LOCAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


