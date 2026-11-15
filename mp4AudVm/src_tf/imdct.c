/**********************************************************************
  This software module was originally developed by
  Bernhard Grill (University of Erlangen)
  and edited by
  Huseyin Kemal Cakmak (Fraunhofer Gesellschaft IIS)
  and edited by
  Bernhard Grill (University of Erlangen),
  Takashi Koike (Sony Corporation)
  Naoki Iwakami (Nippon Telegraph and Telephone)

  FN3 LN3 (CN3),
  in the course of development of the
  MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
  This software module is an implementation of a part of one or more
  MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
  standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
  free license to this software module or modifications thereof for use in
  hardware or software products claiming conformance to the MPEG-2 AAC/
  MPEG-4 Audio  standards. Those intending to use this software module in
  hardware or software products are advised that this use may infringe
  existing patents. The original developer of this software module and
  his/her company, the subsequent editors and their companies, and ISO/IEC
  have no liability for use of this software module or modifications
  thereof in an implementation. Copyright is not released for non
  MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
  retains full right to use the code for his/her  own purpose, assign or
  donate the code to a third party and to inhibit third party from using
  the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
  This copyright notice must be included in all copies or derivative works.
  Copyright (c)1996. 
  **********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#include "common_m4a.h"
#include "tf_main.h"
#include "transfo.h"
#include "dolby_win.h"
#include "dolby_win_ssr.h"

#define DPI	3.14159265358979323846264338327950288
#define USE_FAST_IMDCT 1

static double zero = 0;

/* This is actually not an fft but implements just the dft formula,
   and should therefore be replaced */

void fft( double in[], double out[], int len )
{
  double tmp1, tmp2;
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

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
  if (vlen) /* just for bounds-checkers sake */
    *dest = *src;

}

static void vmult( double src1[], double src2[], double dest[], 
            int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src1 * *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
  if (i<vlen)
    *dest = *src1 * *src2;

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
 
/*****************************************************************************

    functionname: Izero
    description:  calculates the modified Bessel function of the first kind
    returns:      value of Bessel function
    input:        argument
    output:       

*****************************************************************************/
static double Izero(double x)
{
  const double IzeroEPSILON = 1E-41;  /* Max error acceptable in Izero */
  double sum, u, halfx, temp;
  int n;
  
  sum = u = n = 1;
  halfx = x/2.0;
  do {
    temp = halfx/(double)n;
    n += 1;
    temp *= temp;
    u *= temp;
    sum += u;
  } while (u >= IzeroEPSILON*sum);

  return(sum);
}



/*****************************************************************************

    functionname: CalculateKBDWindow
    description:  calculates the window coefficients for the Kaiser-Bessel
                  derived window
    returns:        
    input:        window length, alpha
    output:       window coefficients

*****************************************************************************/
static void CalculateKBDWindow(double* win, double alpha, int length)
{
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
  int i;
  double IBeta;
  double tmp;
  double sum = 0.0;

  alpha *= M_PI;
  IBeta = 1.0/Izero(alpha);
 
  /* calculate lower half of Kaiser Bessel window */
  for(i=0; i<(length>>1); i++) {
    tmp = 4.0*(double)i/(double)length - 1.0;
    win[i] = Izero(alpha*sqrt(1.0-tmp*tmp))*IBeta;
    sum += win[i];
  }

  sum = 1.0/sum;
  tmp = 0.0;

  /* calculate lower half of window */
  for(i=0; i<(length>>1); i++) {
    tmp += win[i];
    win[i] = sqrt(tmp*sum);
  }
}

/* Calculate window */
static void calc_window( double window[], int len, WINDOW_SHAPE wfun_select)

{
  int i;
  extern double dolby_win_1024[1024];
  extern double dolby_win_960 [ 960];
  extern double dolby_win_128 [ 128];
  extern double dolby_win_120 [ 120];

  switch(wfun_select) {
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
        memcpy(window, dolby_win_128, len*sizeof(double));
        break;
      case BLOCK_LEN_SHORT_SSR:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_32[i]; 
        break;
      case BLOCK_LEN_MEDIUM_S:
      case BLOCK_LEN_MEDIUM:
        for( i=0; i<len; i++ ) 
          window[i] = sin( ((double)(i+1)-0.5) * DPI / (double)(2*len) ); /* Using WS_FHG */
        break;
      case BLOCK_LEN_LONG_S:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_960[i]; 
        break;
      case BLOCK_LEN_LONG:
        memcpy(window, dolby_win_1024, len*sizeof(double));
        break;
      case BLOCK_LEN_LONG_SSR:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_256[i]; 
        break;
      case 4:
        memcpy(window, dolby_win_4, len*sizeof(double));
        break;
      case 16:
        memcpy(window, dolby_win_16, len*sizeof(double));
        break;
      default:
        CalculateKBDWindow(window, 6.0, 2*len);
        /* 	  CommonExit(1,"Unsupported window size: %d", len); */
        break;
      }
    break;

#ifdef VERSION2
    /* Zero padded window for low delay mode */
  case WS_ZPW:
    for( i=0; i<3*(len>>3); i++ )
      window[i] = 0.0;
    for(; i<5*(len>>3); i++)
      window[i] = sin((i-3*(len>>3)+0.5) * DPI / (len>>1));
    for(; i<len; i++)
      window[i] = 1.0;
    break; 
#endif
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
  WINDOW_SEQUENCE  windowSequence,
  int              nlong,            /* shift length for long windows   */
  int              nmed,             /* shift length for medium windows */
  int              nshort,           /* shift length for short windows  */
  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     wfun_select_prev, /* YB : 971113 */
  Imdct_out	   overlap_select,   /* select imdct output *TK*	*/
				     /* switch (overlap_select) {	*/
				     /* case OVERLAPPED_MODE:		*/
				     /*   p_out_data[]			*/
				     /*   = overlapped and added signal */
				     /*		(bufferlength: nlong)	*/
				     /* case NON_OVERLAPPED_MODE:		*/
				     /*   p_out_data[]			*/
				     /*   = non overlapped signal	*/
				     /*		(bufferlength: 2*nlong)	*/
  int              num_short_win     /* number of short windows to      */
                                     /* transform                       */
  )
{
  double           *o_buf, transf_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double           overlap_buf[ 2*MAX_SHIFT_LEN_LONG ]; 
 
  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
#if 0 /* not used anymore */
  double           window_med[MAX_SHIFT_LEN_LONG]; 
  double           window_med_prev[MAX_SHIFT_LEN_LONG]; 
#endif
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG]; 
  double           *window_short_prev_ptr;
 
  double  *fp; 
  int      k; 
 
  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 
  int transfak_lm =  nlong/nmed; 
#if 0 /* not used anymore */
  int nflat_lm    = (nlong-nmed)  / 2; 
  int nflat_ms    = (nmed-nshort) / 2; 
#endif
 
  window_short_prev_ptr=window_short_prev ; 


  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_synthesis: Problem with window length" ); 
  } 
  if( (nlong%nmed) || (nmed%nshort) || (nmed > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_synthesis: Problem with window length" ); 
  } 
  if( transfak_lm%2 ) { 
    CommonExit( 1, "mdct_synthesis: Problem with window length" ); 
  } 
  if( windowSequence==EIGHT_SHORT_SEQUENCE
      && ( (num_short_win <= 0) || (num_short_win > transfak_ls) ) ) {
    CommonExit( 1, "mdct_synthesis: Problem with number of short windows" ); 
  } 

  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
#if 0 /* not used anymore */
  calc_window( window_med, nmed, wfun_select ); 
  calc_window( window_med_prev, nmed, wfun_select_prev );
#endif
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev_ptr, nshort, wfun_select_prev ); 

  /* Assemble overlap buffer */ 
  vcopy( p_overlap, overlap_buf, 1, 1, nlong ); 
  o_buf = overlap_buf; 
 
#if 0
  printf("\n input imdct: \n%d:",0);
  for (i=0;i<1024;i++){
#if 0
    p_in_data[i] = 0;
    if (i==50)
      p_in_data[i] = 500;    
#endif
    printf(" %d",(int)p_in_data[i]);
    if (i%50 == 0)
      printf("\n%d:",i);
  }
#endif

  /* Separate action for each Block Type */ 
   switch( windowSequence ) { 
   case ONLY_LONG_SEQUENCE : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    if (overlap_select != NON_OVERLAPPED_MODE) {
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
      vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong );
    }

    break; 
 
  case LONG_START_SEQUENCE : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    if (overlap_select != NON_OVERLAPPED_MODE) {
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
      vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nflat_ls ); 
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, o_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, transf_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, transf_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    break; 
    
  case LONG_STOP_SEQUENCE : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf+nflat_ls, window_short_prev_ptr, transf_buf+nflat_ls, 1, 1, 1, nshort ); 
    if (overlap_select != NON_OVERLAPPED_MODE) {
      vadd( transf_buf+nflat_ls, o_buf+nflat_ls, o_buf+nflat_ls, 1, 1, 1, nshort+nflat_ls ); 
      vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      vcopy( &zero, transf_buf, 0, 1, nflat_ls);
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong);
    }
    break; 
 
  case EIGHT_SHORT_SEQUENCE : 
    {int ii; for(ii=0; ii<2*nlong; ii++) transf_buf[ii]=0;}
    if (overlap_select != NON_OVERLAPPED_MODE) {
      fp = o_buf + nflat_ls; 
      vcopy(&zero,o_buf+nlong,0,1,nlong);
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      fp = transf_buf;
    }
    for( k = num_short_win-1; k-->= 0; ) { 
      if (overlap_select != NON_OVERLAPPED_MODE) {
        imdct( p_in_data, transf_buf, 2*nshort ); 
        vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort ); 
        vadd( transf_buf, fp, fp, 1, 1, 1, nshort ); 
        vmult( transf_buf+nshort, window_short+nshort-1, transf_buf+nshort, 1, -1, 1, nshort ); 
        vadd( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort ); 
        p_in_data += nshort; 
        fp        += nshort; 
        window_short_prev_ptr = window_short; 
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
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
    
#if 0 /* not used anymore */
  case LONG_MEDIUM_WINDOW : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
    vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nflat_lm ); 
    vmult( transf_buf+nlong+nflat_lm, window_med+nmed-1, o_buf+nlong+nflat_lm, 1, -1, 1, nmed ); 
    vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_lm ); 
    break; 
 
   case MEDIUM_LONG_WINDOW : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf+nflat_lm, window_med, transf_buf+nflat_lm, 1, 1, 1, nmed ); 
    vadd( transf_buf+nflat_lm, o_buf+nflat_lm, o_buf+nflat_lm, 1, 1, 1, nmed ); 
    vcopy( transf_buf+nflat_lm+nmed, o_buf+nflat_lm+nmed, 1, 1, nflat_lm ); 
    vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    break; 
 
   case ONLY_MEDIUM_WINDOW : 
    fp = o_buf + nflat_lm; 
    for( k = transfak_lm-1; k-->=0; ) { 
      imdct( p_in_data, transf_buf, 2*nmed ); 
      vmult( transf_buf, window_med, transf_buf, 1, 1, 1, nmed ); 
      vadd( transf_buf, fp, fp, 1, 1, 1, nmed ); 
      vmult( transf_buf+nmed, window_med+nmed-1, fp+nmed, 1, -1, 1, nmed ); 
      p_in_data += nmed; 
      fp        += nmed; 
    } 
    vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_lm ); 
    break; 
 
   case MEDIUM_SHORT_WINDOW : 
    fp = o_buf + nflat_lm; 
    for( k = transfak_lm-2; k-->=0; ) {  /* NI */
      imdct( p_in_data, transf_buf, 2*nmed ); 
      vmult( transf_buf, window_med, transf_buf, 1, 1, 1, nmed ); 
      vadd( transf_buf, fp, fp, 1, 1, 1, nmed ); 
      vmult( transf_buf+nmed, window_med+nmed-1, fp+nmed, 1, -1, 1, nmed ); 
      p_in_data += nmed; 
      fp        += nmed; 
    } 
    imdct( p_in_data, transf_buf, 2*nmed ); 
    vmult( transf_buf, window_med, transf_buf, 1, 1, 1, nmed ); 
    vadd( transf_buf, fp, fp, 1, 1, 1, nmed ); 
    vcopy( transf_buf+nmed, fp+nmed, 1, 1, nflat_ms ); 
    vmult( transf_buf+nmed+nflat_ms, window_short+nshort-1, fp+nmed+nflat_ms, 1, -1, 1, nshort ); 
    break; 
 
   case SHORT_MEDIUM_WINDOW : 
    fp = o_buf + nflat_lm; 
    imdct( p_in_data, transf_buf, 2*nmed ); 
    vmult( transf_buf+nflat_ms, window_short_prev_ptr, transf_buf+nflat_ms, 1, 1, 1, nshort ); 
    vadd( transf_buf+nflat_ms, fp+nflat_ms, fp+nflat_ms, 1, 1, 1, nshort ); 
    vcopy( transf_buf+nflat_ms+nshort, fp+nflat_ms+nshort, 1, 1, nflat_ms ); 
    vmult( transf_buf+nmed, window_med+nmed-1, fp+nmed, 1, -1, 1, nmed ); 
    fp        += nmed; 
    p_in_data += nmed; 
    for( k = transfak_lm-2; k-->=0; ) { /* NI */
      imdct( p_in_data, transf_buf, 2*nmed ); 
      vmult( transf_buf, window_med, transf_buf, 1, 1, 1, nmed ); 
      vadd( transf_buf, fp, fp, 1, 1, 1, nmed ); 
      vmult( transf_buf+nmed, window_med+nmed-1, fp+nmed, 1, -1, 1, nmed ); 
      p_in_data += nmed; 
      fp        += nmed; 
    } 
    break; 
#endif
 
   default : 
    CommonExit( 1, "mdct_synthesis: Unknown window type(1)" ); 
  } 
 
  if (overlap_select != NON_OVERLAPPED_MODE) {
    vcopy( o_buf, p_out_data, 1, 1, nlong ); 
  }
  else { /* overlap_select == NON_OVERLAPPED_MODE */
    vcopy( transf_buf, p_out_data, 1, 1, 2*nlong ); 
  }
  
  /* save unused output data */ 
  if (overlap_select != NON_OVERLAPPED_MODE)
  vcopy( o_buf+nlong, p_overlap, 1, 1, nlong ); 
} 



/* %%%%%%%%%%%%%%%%% MDCT - STUFF %%%%%%%%%%%%%%%% */

void mdct( double in_data[], double out_data[], int len )
{
  double *tmp_data;

  tmp_data = (double*)malloc(len*sizeof(double));
  vcopy(in_data, tmp_data, 1, 1, len); 
  MDCT(tmp_data, len, len/2, 1);
  vcopy(tmp_data, out_data, 1, 1, len/2); 
  free(tmp_data);
}


void buffer2freq(
  double           p_in_data[], 
  double           p_out_mdct[],
  double           p_overlap[],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     wfun_select_prev,
  int              nlong,            /* shift length for long windows   */
  int              nmed,             /* shift length for medium windows */
  int              nshort,           /* shift length for short windows  */
  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                                     /* switch (overlap_select) {       */
                                     /* case OVERLAPPED_MODE:                */
                                     /*   p_in_data[]                   */
                                     /*   = overlapped signal           */
                                     /*         (bufferlength: nlong)   */
                                     /* case NON_OVERLAPPED_MODE:            */
                                     /*   p_in_data[]                   */
                                     /*   = non overlapped signal       */
                                     /*         (bufferlength: 2*nlong) */
  int              num_short_win     /* number of short windows to      */
                                     /* transform                       */
  /*int              save_window*/       /* save window information         */
)
{
  double         transf_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double         windowed_buf[ 2*MAX_SHIFT_LEN_LONG ];
  double           *p_o_buf;
  int             k;

  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
#if 0 /* not used anymore */
  double           window_med[MAX_SHIFT_LEN_LONG]; 
  double           window_med_prev[MAX_SHIFT_LEN_LONG]; 
#endif
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG]; 
  double           *window_short_prev_ptr;


  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 
  int transfak_lm =  nlong/nmed; 
#if 0 /* not used anymore */
  int nflat_lm    = (nlong-nmed)  / 2; 
  int nflat_ms    = (nmed-nshort) / 2; 
#endif
  static int firstTime=1;
  window_short_prev_ptr = window_short_prev ; 


  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_analysis: Problem with window length" ); } 
  if( (nlong%nmed) || (nmed%nshort) || (nmed > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_analysis: Problem with window length" );  } 
  if( transfak_lm%2 ) { 
    CommonExit( 1, "mdct_analysis: Problem with window length" );  } 
  if( windowSequence==EIGHT_SHORT_SEQUENCE
      && ( (num_short_win <= 0) || (num_short_win > transfak_ls) ) ) {
    CommonExit( 1, "mdct_analysis: Problem with number of short windows" );  } 
  
  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
#if 0 /* not used anymore */
  calc_window( window_med, nmed, wfun_select ); 
  calc_window( window_med_prev, nmed, wfun_select_prev ); 
#endif
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev, nshort, wfun_select_prev ); 
  
  if (overlap_select != NON_OVERLAPPED_MODE) {
    /* create / shift old values */
    /* We use p_overlap here as buffer holding the last frame time signal*/
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
  else { /* overlap_select == NON_OVERLAPPED_MODE */
    vcopy( p_in_data, transf_buf, 1, 1, 2*nlong);
  }

  /* Set ptr to transf-Buffer */
  p_o_buf = transf_buf;
  
  
  /* Separate action for each Block Type */
  switch( windowSequence ) {
   case ONLY_LONG_SEQUENCE :
     vmult( p_o_buf, window_long_prev, windowed_buf,       1, 1,  1, nlong );
     vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
     mdct( windowed_buf, p_out_mdct, 2*nlong );    
     break;
   
   case LONG_START_SEQUENCE :
    vmult( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
    vcopy( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort );
    vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case LONG_STOP_SEQUENCE :
    vcopy( &zero, windowed_buf, 0, 1, nflat_ls );
    vmult( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
    vcopy( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case EIGHT_SHORT_SEQUENCE :
    if (overlap_select != NON_OVERLAPPED_MODE) {
      p_o_buf += nflat_ls;
    }
    for (k=num_short_win-1; k-->=0; ) {
      vmult( p_o_buf,        window_short_prev_ptr,          windowed_buf,        1, 1,  1, nshort );
      vmult( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
      mdct( windowed_buf, p_out_mdct, 2*nshort );

      p_out_mdct += nshort;
      p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
      window_short_prev_ptr = window_short; 
    }
    break;
#if 0 /* not used anymore */
   case LONG_MEDIUM_WINDOW :
    vmult( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
    vcopy( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_lm );
    vmult( p_o_buf+nlong+nflat_lm, window_med+nmed-1, windowed_buf+nlong+nflat_lm, 1, -1, 1, nmed );
    vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_lm );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case MEDIUM_LONG_WINDOW :
    vcopy( &zero, windowed_buf, 0, 1, nflat_lm );
    vmult( p_o_buf+nflat_lm, window_med, windowed_buf+nflat_lm, 1, 1, 1, nmed );
    vcopy( p_o_buf+nflat_lm+nmed, windowed_buf+nflat_lm+nmed, 1, 1, nflat_lm );
    vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;

   case ONLY_MEDIUM_WINDOW :
    p_o_buf += nflat_lm;
    for( k=transfak_lm-1; k-->=0; ) {
      vmult( p_o_buf,      window_med,        windowed_buf,      1, 1,  1, nmed );
      vmult( p_o_buf+nmed, window_med+nmed-1, windowed_buf+nmed, 1, -1, 1, nmed );
      mdct( windowed_buf, p_out_mdct, 2*nmed );

      p_o_buf    += nmed;
      p_out_mdct += nmed;
    }
    break;

   case MEDIUM_SHORT_WINDOW :
    p_o_buf += nflat_lm;
    for( k=transfak_lm-2; k-->=0; ) { /* NI */
      vmult( p_o_buf,      window_med,        windowed_buf,      1, 1,  1, nmed );
      vmult( p_o_buf+nmed, window_med+nmed-1, windowed_buf+nmed, 1, -1, 1, nmed );
      mdct( windowed_buf, p_out_mdct, 2*nmed );

      p_o_buf    += nmed;
      p_out_mdct += nmed;
    }
    vmult( p_o_buf, window_med, windowed_buf, 1, 1, 1, nmed );
    vcopy( p_o_buf+nmed, windowed_buf+nmed, 1, 1, nflat_ms );
    vmult( p_o_buf+nmed+nflat_ms, window_short+nshort-1, windowed_buf+nmed+nflat_ms, 1, -1, 1, nshort );
    vcopy( &zero, windowed_buf+2*nmed-1, 0, -1, nflat_ms );
    mdct( windowed_buf, p_out_mdct, 2*nmed );
    break;

   case SHORT_MEDIUM_WINDOW :
    p_o_buf += nflat_lm;
    vcopy( &zero, windowed_buf, 0, 1, nflat_ms );
    vmult( p_o_buf+nflat_ms, window_short_prev_ptr, windowed_buf+nflat_ms, 1, 1, 1, nshort );
    vcopy( p_o_buf+nflat_ms+nshort, windowed_buf+nflat_ms+nshort, 1, 1, nflat_ms );
    vmult( p_o_buf+nmed, window_med+nmed-1, windowed_buf+nmed, 1, -1, 1, nmed );
    mdct( windowed_buf, p_out_mdct, 2*nmed );

    p_o_buf    += nmed;
    p_out_mdct += nmed;
    for (k=transfak_lm-2; k-->=0; ) { /* NI */
      vmult( p_o_buf,      window_med,        windowed_buf,      1, 1,  1, nmed );
      vmult( p_o_buf+nmed, window_med+nmed-1, windowed_buf+nmed, 1, -1, 1, nmed );
      mdct( windowed_buf, p_out_mdct, 2*nmed );

      p_o_buf    += nmed;
      p_out_mdct += nmed;
    }
    break;
#endif
   default :
      CommonExit( 1, "mdct_synthesis: Unknown window type (2)" ); 
  }

  /* Set output data 
  vcopy(transf_buf, p_out_mdct,1, 1, nlong); */
 
}

/***********************************************************************************************/ 
 
