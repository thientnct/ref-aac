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

#include "block.h"
#include "dolby_win.h"

#ifndef PI
#define PI                   3.14159265359
#endif

#ifdef MFDBG
#define DISPLAY_NT 1
#endif

static Float	fhg_long [NWINLONG];
static Float	fhg_short [NWINSHORT];
static Float	fhg_edler [NWINLONG];
static Float	dol_edler [NWINLONG];

/* block switching.  Switch long blocks, use FhG short blocks */
static Float *windowPtr [N_WINDOW_TYPES] [N_WINDOW_SHAPES] = {
    {fhg_long,	dol_long},
    {fhg_short,	dol_short},
    {fhg_edler,	dol_edler},
};	

static int		windowLeng [N_WINDOW_TYPES] = {
    sizeof(fhg_long)/sizeof(Float),	
    sizeof(fhg_short)/sizeof(Float),	
    sizeof(fhg_edler)/sizeof(Float),	
};



/*****************************************************************************
*
*	InitBlock
*	calculate windows for use by Window()
*	input: none
*	output: none
*	local static: none
*	globals: shortWindow[], longWindow[]
*
*****************************************************************************/
void InitBlock (void) {

/* calc half-window data */

  int             i, j;
  register float	phaseInc;

/* init half-windows */

  /* FhG long window */
  phaseInc = PI / (2.0 * NWINLONG);
  for (i = 0; i < NWINLONG; i++) {
    fhg_long [i]  = sin (phaseInc * ((Float) i + 0.5));
  }

  /* FhG short window */
  phaseInc = PI / (2.0 * NWINSHORT);
  for (i = 0; i < NWINSHORT; i++) {
    fhg_short [i] = sin (phaseInc * ((Float) i + 0.5));
  }

  /* Edler windows */
  for (i = 0, j = 0; i < NFLAT; i++, j++) {
    fhg_edler[j] = 0.0;
    dol_edler[j] = 0.0;
  }
  for (i = 0; i < NWINSHORT; i++, j++) {
    fhg_edler [j] = fhg_short [i];
    dol_edler [j] = dol_short [i];
  }
  for ( ; j < NWINFLAT; j++) {
    fhg_edler [j] = 1.0;
    dol_edler [j] = 1.0;
  }
}


/*****************************************************************************
*
*	Window
*	window input sequence based on window type
*	input: see below
*	output: see below
*	local static:
*	  firstTime				flag = need to initialize data structures
*	globals: shortWindow[], longWindow[]
*
*****************************************************************************/

void ITransformBlock (
	Float* dataPtr,		/* vector to be windowed in place */
	int bT,			/* input: window type */
	Wnd_Shape *wnd_shape
	)
{
  static int	firstTime = 1;
  int		i,leng, leng0, leng1;
  Float		*windPtr;
  WINDOW_TYPE	beginWT, endWT;

#ifdef DISPLAY_NT
  Float		dispWindow[2048];
  Float		*dispWindPtr = dispWindow;
  OUT_DATATYPE* destPtr2 = dataPtr;	/* output: windowed vector */
#endif

  if (firstTime)  {
    InitBlock();			/* calculate windows */
    firstTime = 0;
  }

#ifdef BTOB
    printf ("shape %d  prev %d\n", 
        window_shape [ch], decode_previous_window_shape [ch]);
#endif
#ifdef MFDBGx
    printf("\n%s", ((decode_previous_window_shape [ch] == WS_DOLBY) ? "WS_DOLBY" : "WS_FHG"));
    printf("\n%s, wT=%d\n", ((window_shape [ch] == WS_DOLBY) ? "WS_DOLBY" : "WS_FHG"), bT);
#endif
#if 0
    if(window_shape[ch] == WS_DOLBY)
	printf("\n%s, wT=%d\n", ((window_shape [ch] == WS_DOLBY) ? "WS_DOLBY" : "WS_FHG"), bT);
#endif

    if((bT==ONLY_LONG_WINDOW) || (bT==LONG_START_WINDOW))  {
      beginWT = WT_LONG;
    }
    else if(bT==LONG_STOP_WINDOW) {
      beginWT = WT_FLAT;
    }
    else	
      beginWT = WT_SHORT;

    if ((bT == ONLY_LONG_WINDOW) || (bT == LONG_STOP_WINDOW)) {
      endWT = WT_LONG;
    }
    else if (bT == LONG_START_WINDOW)  {
      endWT = WT_FLAT;
    }
    else	
      endWT = WT_SHORT;

    leng0 = windowLeng [beginWT];
    leng1 = windowLeng [endWT];

#ifdef DISPLAY_NT
    {
      static char str [512];
      sprintf (str, "Prexfrm  beginWT=%d endWT=%d leng0=%d leng1=%d",
               beginWT, endWT, leng0, leng1);
      plot_ary (dataPtr, (leng0+leng1)/2, 0, 0., 0., str);
    }
#endif

    ITransform (dataPtr, leng0 + leng1, leng1);

#ifdef DISPLAY_NT
    plot_ary (dataPtr, leng0 + leng1, 0, 0., 0., "Prewin");
#endif

    /*	first half of window */
    windPtr = windowPtr [beginWT] [wnd_shape->prev_bk];  

#ifdef BTOB
    /*	special debug option	*/
    decode_previous_window_shape [ch] = window_shape [ch];
#endif

    for (i = 0; i < windowLeng [beginWT]; i++)  {
#ifdef DISPLAY_NT
      *dispWindPtr++ = *windPtr;
#endif
      *dataPtr++ *= *windPtr++;
    }

    /*	second half of window */
    leng = windowLeng [endWT];
    windPtr = windowPtr [endWT] [wnd_shape->this_bk] + leng - 1;
    for (i = 0; i < leng; i++) {
#ifdef DISPLAY_NT
      *dispWindPtr++ = *windPtr;
#endif
      *dataPtr++ *= *windPtr--;
    }

#ifdef DISPLAY_NT
    printf ("beginWT = %d  len %d  endWT = %d  len %d\n",
            beginWT, windowLeng [beginWT], endWT, windowLeng [endWT]);

    plot_ary (dispWindow, windowLeng [beginWT] + leng, 0, 0., 0., "block.c: DOL Window");
    plot_ary (destPtr2, windowLeng [beginWT] + leng, 0, 0., 0., "block.c: DOL Windowed");
#endif

    wnd_shape->prev_bk = wnd_shape->this_bk;
}


/*****************************************************************************
*
*	MDCT
*	window input sequence based on window type and perform MDCT
*	This is adapted from ITransformBlock().
*	(long term prediction needs this routine) 
*	input: see below
*	output: see below
*	local static:
*	  firstTime				flag = need to initialize data structures
*	globals: -
*
*****************************************************************************/
void TransformBlock (
	Float* dataPtr,		/* time domain signal */
	int bT,			/* input: window type */
	Wnd_Shape *wnd_shape
	)
{
  int		i, leng, leng0, leng1;
  Float		*windPtr;
  WINDOW_TYPE	beginWT, endWT;

  if((bT==ONLY_LONG_WINDOW) || (bT==LONG_START_WINDOW))  {
    beginWT = WT_LONG;
  }
  else if(bT==LONG_STOP_WINDOW) {
    beginWT = WT_FLAT;
  }
  else	
    beginWT = WT_SHORT;

  if ((bT == ONLY_LONG_WINDOW) || (bT == LONG_STOP_WINDOW)) {
    endWT = WT_LONG;
  }
  else if (bT == LONG_START_WINDOW)  {
    endWT = WT_FLAT;
  }
  else	
    endWT = WT_SHORT;

  leng0 = windowLeng [beginWT];
  leng1 = windowLeng [endWT];

  /*	first half of window */
  windPtr = windowPtr [beginWT] [wnd_shape->prev_bk];  

  for (i = 0; i < windowLeng [beginWT]; i++)  {
    *dataPtr++ *= *windPtr++;
  }

  /*	second half of window */
  leng = windowLeng [endWT];
  windPtr = windowPtr [endWT] [wnd_shape->this_bk] + leng - 1;
  for (i = 0; i < leng; i++) {
    *dataPtr++ *= *windPtr--;
  }
  dataPtr -= (windowLeng [beginWT] + leng);

  Transform (dataPtr, (leng0 + leng1), leng1);
  wnd_shape->prev_bk = wnd_shape->this_bk;
}

