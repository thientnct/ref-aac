/**********************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Bernhard Grill (University of Erlangen)

and edited by
  Huseyin Kemal Cakmak (Fraunhofer IIS-A)
  Bernhard Grill (University of Erlangen),
  Takashi Koike (Sony Corporation),
  Yasuhiro Toguri (Sony Corporation)

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
  **********************************************************************/

#include <math.h>
#include <stdlib.h>

#include "common.h"
#include "tf_main.h"
#include "transfo.h"
#include "block.h"
#include "dolby_win.h"
#include "dolby_win_ssr.h"  /* 980129 added for SSR */

#define DPI	3.14159265358979323846264338327950288
#define USE_FAST_IMDCT 1

static double zero = 0;

/* This is actually not an fft but implements just the dft formula,
   and should therefore be replaced */

void fft( double in[], double out[], int len )
{
  float tmp1, tmp2;
  int n, m;
  tmp1 = 2*DPI/len;
  for( m=0; m<len; m++ ) {
    tmp2 = tmp1*m;
    out[2*m] = out[2*m+1] = 0;
    for( n=0; n<len; n++ ) {
      out[2*m]   += in[2*n] * cos( tmp2*n ) - in[2*n+1] * sin( tmp2*n );
      out[2*m+1] -= in[2*n] * sin( tmp2*n ) + in[2*n+1] * cos( tmp2*n );
    }
  }
}

static void vcopy( double src[], double dest[], int inc_src, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
}

static void vmult( double src1[], double src2[], double dest[], 
            int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 * *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}

static void vadd( double src1[], double src2[], double dest[], 
            int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 + *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}
 

/* Calculate window */
static void calc_window( double window[], int len, Window_shape wfun_select )
{
  int i;
  extern double dolby_win_1024[1024];
  extern double dolby_win_960[960];
  extern double dolby_win_128[128];
  extern double dolby_win_120[120];
  extern double dolby_win_256[256]; /* added 980128 for SSR*/
  extern double dolby_win_32[32];  /* added 980128 for SSR */


  switch(wfun_select)
    {
    case WS_FHG: 
      for( i=0; i<len; i++ ) 
	window[i] = sin( ((i+1)-0.5) * DPI / (2*len) );
      break;
      
    case WS_DOLBY: 
      switch(len)
	{
	case BLOCK_LEN_SHORT_S:
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_120[i]; 
	  break;
	case BLOCK_LEN_SHORT:
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_128[i]; 
	  break;
	case BLOCK_LEN_SHORT_SSR:  /* 980129 added for SSR */
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_32[i]; 
	  break;
	case BLOCK_LEN_LONG_S:
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_960[i]; 
	  break;
	case BLOCK_LEN_LONG:
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_1024[i]; 
	  break;
    case BLOCK_LEN_LONG_SSR:   /* added 980129 for SSR */
	  for( i=0; i<len; i++ ) 
	    window[i] = dolby_win_256[i]; 
	  break;

	default:
	  CommonExit(1,"Unsupported window size: %d", len);
	  break;
	}
      break;
    default:
      CommonExit(1,"Unsupported window shape: %d", wfun_select);
      break;
    }
}

/* %%%%%%%%%%%%%%%%%% IMDCT - STUFF %%%%%%%%%%%%%%%%*/

#define MAX_SHIFT_LEN_LONG 4096

void imdct(double in_data[], double out_data[], int len)
{
  vcopy(in_data, out_data, 1, 1, len/2);
  MDCT(out_data, len, len/2, -1);
}

void freq2buffer(
  double           p_in_data[], 
  double           p_out_data[],
  double           p_overlap[],
  enum WINDOW_TYPE block_type,
  int              nlong,            /* shift length for long windows   */
  int              nshort,           /* shift length for short windows  */
  Window_shape     wfun_select,      /* offers the possibility to select different window functions */
  Mdct_in	   overlap_select    /* select imdct output *TK*	*/
				     /* switch (overlap_select) {	*/
				     /* case OVERLAPPED:		*/
				     /*   p_out_data[]			*/
				     /*   = overlapped and added signal */
				     /*		(bufferlength: nlong)	*/
				     /* case NON_OVERLAPPED:		*/
				     /*   p_out_data[]			*/
				     /*   = non overlapped signal	*/
				     /*		(bufferlength: 2*nlong)	*/
  )
{
  double           *o_buf, transf_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double           overlap_buf[ 2*MAX_SHIFT_LEN_LONG ]; 
 
  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG]; 
  double           *window_short_prev_ptr;
 
  double  *fp; 
  int     i, k; 
 
  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 
 
  static Window_shape wfun_select_prev=WS_FHG;
  window_short_prev_ptr=window_short_prev ; 


  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_synthesis: Problem with window length" ); 
  } 

  /* --- PATCH: Use WS_FHG window shape if we start 
                with SHORT windows            --- */
  if  (block_type==LONG_SHORT_WINDOW ||
       block_type==ONLY_SHORT_WINDOW ){
    wfun_select=WS_FHG;
  } 
  /* --- PATCH  End  --- */
  
  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev_ptr, nshort, wfun_select_prev ); 
  
 
  /* Assemble overlap buffer */ 
  vcopy( p_overlap, overlap_buf, 1, 1, nlong ); 
  o_buf = overlap_buf; 
 
#if 0
  fprintf(stderr,"\n input imdct: \n%d:",0);
  for (i=0;i<1024;i++){
#if 0
    p_in_data[i] = 0;
    if (i==50)
      p_in_data[i] = 500;    
#endif
    fprintf(stderr," %d",(int)p_in_data[i]);
    if (i%50 == 0)
      fprintf(stderr,"\n%d:",i);
  }
#endif

  /* Separate action for each Block Type */ 
   switch( block_type ) { 
   case ONLY_LONG_WINDOW : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    if (overlap_select != NON_OVERLAPPED) {
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
      vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    }
    else { /* overlap_select == NON_OVERLAPPED */
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong );
    }

    break; 
 
  case LONG_SHORT_WINDOW : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    if (overlap_select != NON_OVERLAPPED) {
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
      vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nflat_ls ); 
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, o_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    else { /* overlap_select == NON_OVERLAPPED */
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, transf_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, transf_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    break; 
    
  case SHORT_LONG_WINDOW : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf+nflat_ls, window_short_prev_ptr, transf_buf+nflat_ls, 1, 1, 1, nshort ); 
    if (overlap_select != NON_OVERLAPPED) {
      vadd( transf_buf+nflat_ls, o_buf+nflat_ls, o_buf+nflat_ls, 1, 1, 1, nshort ); 
      vcopy( transf_buf+nflat_ls+nshort, o_buf+nflat_ls+nshort, 1, 1, nflat_ls ); 
      vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    }
    else { /* overlap_select == NON_OVERLAPPED */
      vcopy( &zero, transf_buf, 0, 1, nflat_ls);
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong);
    }
    break; 
 
  case ONLY_SHORT_WINDOW : 
    if (overlap_select != NON_OVERLAPPED) {
      fp = o_buf + nflat_ls; 
    }
    else { /* overlap_select == NON_OVERLAPPED */
      fp = transf_buf;
    }
    for( k = transfak_ls-1; k-->= 0; ) { 
      if (overlap_select != NON_OVERLAPPED) {
        imdct( p_in_data, transf_buf, 2*nshort ); 
        vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort ); 
        vadd( transf_buf, fp, fp, 1, 1, 1, nshort ); 
        vmult( transf_buf+nshort, window_short+nshort-1, fp+nshort, 1, -1, 1, nshort ); 
        p_in_data += nshort; 
        fp        += nshort; 
        window_short_prev_ptr = window_short; 
      }
      else { /* overlap_select == NON_OVERLAPPED */
        imdct( p_in_data, fp, 2*nshort );
        vmult( fp, window_short_prev_ptr, fp, 1, 1, 1, nshort ); 
        vmult( fp+nshort, window_short+nshort-1, fp+nshort, 1, -1, 1, nshort ); 
        p_in_data += nshort; 
        fp        += 2*nshort;
        window_short_prev_ptr = window_short; 
      }
    } 
    vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls ); 
    break;     
    
   default : 
    CommonExit( 1, "mdct_synthesis: Unknown window type" ); 
  } 
 
  if (overlap_select != NON_OVERLAPPED) {
    vcopy( o_buf, p_out_data, 1, 1, nlong ); 
  }
  else { /* overlap_select == NON_OVERLAPPED */
    vcopy( transf_buf, p_out_data, 1, 1, 2*nlong ); 
  }
  
  /* save unused output data */ 
  vcopy( o_buf+nlong, p_overlap, 1, 1, nlong ); 

  /* --- Save old window shape --- */
  wfun_select_prev = wfun_select;
  
} 



/* %%%%%%%%%%%%%%%%% MDCT - STUFF %%%%%%%%%%%%%%%% */

static void mdct( double in_data[], double out_data[], int len )
{
  vcopy(in_data, out_data, 1, 1, len); 
  MDCT(out_data, len, len/2, 1);
}


void buffer2freq(
  double           p_in_data[], 
  double           p_out_mdct[],
  double           p_overlap[],
  enum WINDOW_TYPE block_type,
  Window_shape     wfun_select,      /* offers the possibility to select different window functions */
  int              nlong,            /* shift length for long windows   */
  int              nshort,           /* shift length for short windows  */
  Mdct_in      overlap_select     /*  YT 970615 for son_PP */
)
{
  double         transf_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double         windowed_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double         a, *fp, *p_o_buf;
  int            i, k;

  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG]; 
  double           *window_short_prev_ptr;


  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 
 
  static Window_shape wfun_select_prev=WS_FHG;
  static int firstTime=1;
  window_short_prev_ptr = window_short_prev ; 


  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_analysis: Problem with window length" ); } 
  
  /* --- PATCH: Use WS_FHG window shape if we start with SHORT windows --- */
  if  (block_type==LONG_SHORT_WINDOW || block_type==ONLY_SHORT_WINDOW ){
    wfun_select=WS_FHG; } 
  /* --- PATCH  End  --- */
  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev, nshort, wfun_select_prev ); 
  
  /* create / shift old values */
  /* We use p_overlap here as buffer holding the last frame time signal*/
/* YT 970615 for son_pp */
if(overlap_select != NON_OVERLAPPED){
  if (firstTime){
    firstTime=0;
    vcopy( &zero, transf_buf, 0, 1, nlong );
  }
  else
    vcopy( p_overlap, transf_buf, 1, 1, nlong );

  /* Append new data */
  vcopy( p_in_data, transf_buf+nlong, 1, 1, nlong );
  vcopy( p_in_data, p_overlap,        1, 1, nlong );
}
else{
	vcopy( p_in_data, transf_buf, 1, 1, 2*nlong );
}

  /* Set ptr to transf-Buffer */
  p_o_buf = transf_buf;
  
  
  /* Separate action for each Block Type */
  switch( block_type ) {
   case ONLY_LONG_WINDOW :
     vmult( p_o_buf, window_long_prev, windowed_buf,       1, 1,  1, nlong );
     vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
     mdct( windowed_buf, p_out_mdct, 2*nlong );    
     break;
   
   case LONG_SHORT_WINDOW :
    vmult( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
    vcopy( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort );
    vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case SHORT_LONG_WINDOW :
    vcopy( &zero, windowed_buf, 0, 1, nflat_ls );
    vmult( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
    vcopy( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case ONLY_SHORT_WINDOW :
	if(overlap_select != NON_OVERLAPPED){ /* YT 970615 for sonPP */
    	p_o_buf += nflat_ls;
	}
    for (k=transfak_ls-1; k-->=0; ) {
      vmult( p_o_buf,        window_short_prev_ptr,          windowed_buf,        1, 1,  1, nshort );
      vmult( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
      mdct( windowed_buf, p_out_mdct, 2*nshort );

      p_out_mdct += nshort;
	/* YT 970615 for sonPP*/
	  if(overlap_select != NON_OVERLAPPED) p_o_buf += nshort;
      else 	p_o_buf += 2*nshort;
      window_short_prev_ptr = window_short; 
    }
    break;
   default :
      CommonExit( 1, "mdct_synthesis: Unknown window type" ); 
  }

  /* Set output data 
  vcopy(transf_buf, p_out_mdct,1, 1, nlong); */
  
  /* --- Save old window shape --- */
  wfun_select_prev = wfun_select;
}



/***********************************************************************************************/ 
 
