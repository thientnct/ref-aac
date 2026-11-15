/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

#ifndef _H_chandefs
#define _H_chandefs


#ifdef BIGDECODER
enum
{
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 15,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 18,	/* side channels: */
    BChans	= 15,	/* back channels: left surround, right surround */
    BCenter	= 1,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 2,	/* scratch space for parsing unused channels */  
    
    Chans	= FChans + SChans + BChans + LChans + XChans
};
#else
enum
{
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 3,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 2,	/* side channels: */
    BChans	= 2,	/* back channels: left surround, right surround */
    BCenter	= 0,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 2,	/* scratch space for parsing unused channels */  
    
    Chans	= FChans + SChans + BChans + LChans + XChans
};
#endif

/* #define is required in order to use these args in #if () directive */
#define ICChans	3	/* independently switched coupling channels */
#define DCChans	3	/* dependently switched coupling channels */
#define XCChans	2	/* scratch space index for parsing unused coupling channels */
#define CChans  (ICChans + DCChans + XCChans)

#endif

