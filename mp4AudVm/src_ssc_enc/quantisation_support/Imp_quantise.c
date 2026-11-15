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

Source file: Imp_quantise.c

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
 *   Module              : quantisation implementation
 *
 *   Description         : Implementation of the quantisation functions.
 *
 *   Tools               :
 *
 *   Naming Conventions  : All functions begin with quantise_
 *
 ******************************************************************************/

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <string.h>
#include <math.h>
#include "PlatformTypes.h"
#include "SSC_Error.h"
#include "SSC_System.h"

#include "Imp_quantise.h"

SInt quantise_freq( Double dFreq, Double dFreq_rl_base, Double dFs, SInt grid_freq )
{
    SSC_ERROR      Error = SSC_ERROR_OK ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           nFreq ;
    Try
    {
        nFreq = (SInt)( floor( dFreq_rl_base * Freq2erb( dFreq * dFs / (2*SSC_PI)) / pow(2, grid_freq) + 0.5) * pow( 2, grid_freq ) );
    }
    Catch(Error)
    {
        nFreq = 0 ;
    }
    return nFreq ;
}


SInt quantise_amp( Double dAmplitude, Double dAmp_rl_base, SInt grid_amp )
{
    SSC_ERROR      Error = SSC_ERROR_OK ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           nAmp ;
    Try
    {
        if( dAmplitude <= 1.0 )
        {
            nAmp = 0 ;
        }
        else
        {
            nAmp = (SInt)( floor( log(dAmplitude)/(2*log(dAmp_rl_base)*pow(2, grid_amp)) + 0.5) * pow( 2, grid_amp) );
        }
    }
    Catch(Error)
    {
        nAmp =0 ;
    }
    return nAmp ;
}

SInt quantise_phase( Double dPhase, Double dPhase_rl_base )
{
    SSC_ERROR      Error = SSC_ERROR_OK ;
    SSC_EXCEPTION  tec;
    SSC_EXCEPTION* the_exception_context = &tec;
    SInt           nPhase ;

    Try
    {
        nPhase = (SInt)( floor( quantise_phase_float( dPhase, dPhase_rl_base ) + 0.5) );
        if( nPhase == (SInt)(dPhase_rl_base) )
        {
            nPhase = (SInt)(-dPhase_rl_base) ;
        }
    }
    Catch(Error)
    {
        nPhase = 0 ;
    }
    return nPhase ;
}


SInt cliprl(Double rl, SInt minrl, SInt maxrl )
{
    if( rl < (Double)minrl )
    {
        return minrl ;
    }
    else if( rl > (Double)maxrl )
    {
        return maxrl ;
    }
    else
    {
        return (SInt)rl ;
    }
}

SInt lin2db( Double G, SInt offset )
{
    if( G == 0.0 )
    {
        return 0;
    }
    else
    {
        return (SInt)(floor(20*log10(G) + offset + 0.5 ));
    }
}

Double Freq2erb( Double x )
{
    return 21.4*log10(0.00437*x+1);
}


Double dequantise_phase( SInt nPq, Double StepsPerPi )
{
    Double step_phase = SSC_PI / StepsPerPi ;

    return nPq * step_phase ;
}

Double dequantise_freq( SInt nFq, Double dRlPerERB, Double dFs )
{
    return inverb( nFq/dRlPerERB) * 2 * SSC_PI / dFs ;
}

Double inverb( Double x )
{
    return (pow( 10, x/21.4) -1 ) * 1000 / 4.37 ;
}

Double quantise_phase_float( Double dPhase, Double dStepsPerPi )
{
    Double dStep_phase = SSC_PI / dStepsPerPi ;
    return dPhase / dStep_phase ;
}

Double fold_phase( Double dPhi )
{
    if( dPhi < 0 )
    {
        return( fmod( (dPhi-SSC_PI), 2*SSC_PI ) + SSC_PI );
    }
    else
    {
        return( fmod( (dPhi+SSC_PI), 2*SSC_PI ) - SSC_PI );
    }
}
