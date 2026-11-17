/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands)
and edited by Naoya Tanaka (Matsushita Communication Ind. Co., Ltd.)
in the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/

/*====================================================================*/
/*                                                                    */
/*      SOURCE_FILE:    CELP_BITSTREAM_MUX.C                          */
/*                                                                    */
/*====================================================================*/
    
/*====================================================================*/
/*      I N C L U D E S                                               */
/*====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "lpc_common.h"  /* Common LPC core Defined Constants           */
#include "bitstream.h"     /* bit stream module                         */
#include "phi_cons.h"
#include "phi_priv.h"
#include "celp_bitstream_mux.h"    /* Parameter to bitstream conversion routines  */
#include "phi_freq.h"   
#include "pan_celp_const.h"

/*======================================================================*/
/*      G L O B A L    D A T A   D E F I N I T I O N S                  */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/* Arrays for index counting                                            */
/*----------------------------------------------------------------------*/
static short freq[ORDER_LPC_16 + 1][36];
static int counter[ORDER_LPC_16 + 1];
static long frame_count = 1;

/*======================================================================*/
/* Function Definition: write_celp_bitstream_header                     */
/*======================================================================*/
void write_celp_bitstream_header(
    BsBitStream *hdrStream,          /* Out: Bitstream                  */
    const long ExcitationMode,       /* In: Excitation Mode             */
    const long SampleRateMode,       /* In: SampleRate Mode             */
    const long QuantizationMode,     /* In: Type of Quantization        */
    const long FineRateControl,      /* In: Fine Rate Control switch    */
    const long LosslessCodingMode,   /* In: Lossless Coding Mode        */   
    const long RPE_configuration,    /* In: Wideband configuration      */
    const long Wideband_VQ,          /* In: Wideband VQ mode            */
    const long MPE_Configuration,    /* In: Narrowband configuration    */
    const long NumEnhLayers,         /* In: Number of Enhancement Layers*/
    const long BandwidthScalabilityMode, /* In: bandwidth switch        */
    const long BWS_Configuration     /* In: BWS_configuration           */
)
{

    BsPutBit(hdrStream, ExcitationMode, 1);
    BsPutBit(hdrStream, SampleRateMode, 1);
    BsPutBit(hdrStream, FineRateControl, 1);

    if (ExcitationMode == RegularPulseExc )
    {
	BsPutBit(hdrStream, RPE_configuration, 3);
    }
	
    if (ExcitationMode == MultiPulseExc )
    {
	BsPutBit(hdrStream, MPE_Configuration, 5);
	BsPutBit(hdrStream, NumEnhLayers, 2);
	BsPutBit(hdrStream, BandwidthScalabilityMode, 1);

	if (BandwidthScalabilityMode == 1)
	{
	    BsPutBit(hdrStream, BWS_Configuration, 2);
	}
    }
}

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_NarrowBand_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
    const long indices[]               /* In: indices                   */
)
{
    /*==================================================================*/
    /*     Read the Parameters from the Bitstream                       */
    /*           These represent the packed LPC Indices                 */           
    /*==================================================================*/
    BsPutBit(bitStream, indices[0], PAN_BIT_LSP22_0);
    BsPutBit(bitStream, indices[1], PAN_BIT_LSP22_1);
    BsPutBit(bitStream, indices[2], PAN_BIT_LSP22_2);
    BsPutBit(bitStream, indices[3], PAN_BIT_LSP22_3);
    BsPutBit(bitStream, indices[4], PAN_BIT_LSP22_4);
}

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_BandScalable_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
    const long indices[]            /* In: indices                   */
)
{
    /*==================================================================*/
    /*     Read the Parameters from the Bitstream                       */
    /*           These represent the packed LPC Indices                 */           
    /*==================================================================*/
    BsPutBit(bitStream, indices[0], 4);
    BsPutBit(bitStream, indices[1], 7);
    BsPutBit(bitStream, indices[2], 4);
    BsPutBit(bitStream, indices[3], 6);
    BsPutBit(bitStream, indices[4], 7);
    BsPutBit(bitStream, indices[5], 4);
}

/*======================================================================*/
/* Function Definition: Write_Wideband_LSP                              */
/*======================================================================*/
void Write_Wideband_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
    const long indices[]               /* In: indices                   */
)
{
    BsPutBit(bitStream, indices[0], PAN_BIT_LSP_WL_0);
    BsPutBit(bitStream, indices[1], PAN_BIT_LSP_WL_1);
    BsPutBit(bitStream, indices[2], PAN_BIT_LSP_WL_2);
    BsPutBit(bitStream, indices[3], PAN_BIT_LSP_WL_3);
    BsPutBit(bitStream, indices[4], PAN_BIT_LSP_WL_4);
    BsPutBit(bitStream, indices[5], PAN_BIT_LSP_WU_0);
    BsPutBit(bitStream, indices[6], PAN_BIT_LSP_WU_1);
    BsPutBit(bitStream, indices[7], PAN_BIT_LSP_WU_2);
    BsPutBit(bitStream, indices[8], PAN_BIT_LSP_WU_3);
    BsPutBit(bitStream, indices[9], PAN_BIT_LSP_WU_4);
}
 
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 07-06-96  R. Taori & A.Gerrits    Initial Version                    */
/* 18-09-96  R. Taori & A.Gerrits    MPEG bitstream used                */
