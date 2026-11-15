/*
  "This software module was originally developed by 
  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
  development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
  14496-1,2 and 3. This software module is an implementation of a part of one or more 
  MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
  Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
  standards free license to this software module or modifications thereof for use in 
  hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
  Audio  standards. Those intending to use this software module in hardware or 
  software products are advised that this use may infringe existing patents. 
  The original developer of this software module and his/her company, the subsequent 
  editors and their companies, and ISO/IEC have no liability for use of this software 
  module or modifications thereof in an implementation. Copyright is not released for 
  non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
  retains full right to use the code for his/her  own purpose, assign or donate the 
  code to a third party and to inhibit third party from using the code for non 
  MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
  be included in all copies or derivative works." 
  Copyright(c)1996.
*/

#include <math.h>
#include "dolby_def.h"
#include "dolby_win.h"

#ifndef PI
#define PI                   3.14159265359
#endif


static Float	fhg_long [NWINLONG];
static Float	fhg_short [NWINSHORT];
static Float	fhg_edler [NWINLONG];
static Float	dol_edler [NWINLONG];
static Float	fhg_adv [NWINADV];
static Float	dol_adv [NWINADV];

#ifdef DOUBLE_WIN	/* block switching.  Switch long blocks, use FhG short blocks */
static Float *windowPtr [N_WINDOW_TYPES] [N_WINDOW_SHAPES] = {
    {fhg_long,	dol_long},
    {fhg_short,	fhg_short},
    {fhg_edler,	fhg_edler},
    {fhg_adv,	fhg_adv}
};	

#else	/* no block switching.  Use Dolby long blocks, FhG short blocks */
static Float*	windowPtr [N_WINDOW_TYPES] [N_WINDOW_SHAPES] = {
    {dol_long,	dol_long},
    {fhg_short,	fhg_short},
    {fhg_edler,	fhg_edler},
    {fhg_adv,	fhg_adv}
};	

#endif	/*	(!)DOUBLE_WIN	*/

static int		windowLeng [N_WINDOW_TYPES] = {
    sizeof(fhg_long)/sizeof(Float),	
    sizeof(fhg_short)/sizeof(Float),	
    sizeof(fhg_edler)/sizeof(Float),	
    sizeof(fhg_adv)/sizeof(Float)
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
void InitBlock ( ) {

/* calc half-window data */

    static int             i, j;
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

/* Advanced Edler windows */
    for (i = 0, j = 0; i < NADV0; i++, j++) {
		fhg_adv [j] = 0.0;
		dol_adv [j] = 0.0;
		}
    for (i = 0; i < NWINSHORT; i++, j++) {
		fhg_adv[j] = fhg_short[i];
		dol_adv[j] = dol_short[i];
		}
    for ( ; j < NWINADV; j++) {
		fhg_adv[j] = 1.0;
		dol_adv[j] = 1.0;
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

void ITransformBlock (Float* dataPtr,				/* vector to be windowed in place	*/
                      BLOCK_TYPE bT,				/* input: window type			*/
                      Wnd_Shape *wnd_shape
                      )
{
    static int	firstTime = 1, leng0, leng1;
    int			i,leng;
    Float		*windPtr;
    WINDOW_TYPE	beginWT, endWT;

#ifdef MFDBG
#define DISPLAY_NT 1
#else
    /*#define DISPLAY_NT 0	*/
#endif

#ifdef DISPLAY_NT
    Float		dispWindow[2048];
    Float		*dispWindPtr = dispWindow;
    OUT_DATATYPE* destPtr2 = dataPtr;		/* output: windowed vector */
#endif

    if (firstTime)  {
		InitBlock();			/*	calculate windows	*/
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

    if((bT==LONG_BLOCK) || (bT==START_BLOCK) || (bT==START_FLAT_BLOCK)
        || (bT==START_ADV_BLOCK))  {
		beginWT = WT_LONG;
		}
    else if(bT==STOP_FLAT_BLOCK) {
		beginWT = WT_FLAT;
		}
    else if(bT==STOP_ADV_BLOCK) {
		beginWT = WT_ADV;
		}
    else	
		beginWT = WT_SHORT;

    if ((bT == LONG_BLOCK) || (bT == STOP_BLOCK) || (bT == STOP_FLAT_BLOCK)
        || (bT == STOP_ADV_BLOCK)) {
		endWT = WT_LONG;
		}
    else if (bT == START_FLAT_BLOCK)  {
		endWT = WT_FLAT;
		}
    else if (bT == START_ADV_BLOCK)  {
		endWT = WT_ADV;
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
/*     windPtr = windowPtr [0] [0];  */
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

