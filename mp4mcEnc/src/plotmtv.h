/**********************************************************************
MPEG-4 Audio VM


SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Bodo Teichmann (Fraunhofer IIS-A)

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
Copyright (c) ISO/IEC 1997.



Header file: plotmtv.h

$Id: plotmtv.h,v 1.1.1.1 1998/06/09 01:05:56 lueck Exp $

Authors:
tmn  Bodo Teichmann  Fraunhofer IIS
**********************************************************************/


#ifndef _plotmtv_h
#define _plotmtv_h
/* plotmtv_interf.c is just a C interface to the freeware program plotmtv which  */
/* is a graphical data-display program for X-windows with a nice user interface */
/* with this C interface it can be use to display 1-dim arrays (eg. the mdct spectrum)  */
/* directly from the debugger while debugging with 'call plotDirect("",MTV_DOUBLE,npts,array1,array2,array3,array4) */
/* is is verry verry usefull for debugging; please do not remove the files from the VM frame work  */
/* i will sent an executable (sgi,linux,solaris,sunos) to everybody who wants to use it */

enum DATA_TYPE {MTV_DOUBLE,MTV_FLOAT,MTV_ABSFLOAT,MTV_LONG,MTV_INT,MTV_SHORT};
int plotInit(void);
int plotSend( char *legend, char *plotSet  , enum DATA_TYPE dtype,long npts             ,void *dataVector);
/*                          legend, subwindow name , data type           , number of data values, start adress of vector of data */
/* it is possible to plot different vectors with different legendnames into to the same graph(=subwindow)       */

void plotDisplay(void);

/* just for use in the debugger: type call plotDirect(...) in the GNUdebugger command window, 
   vector2,vector3 and vector4 might be the  NULL vector 
   and are ignored then .

   TO USE THIS FEATURE YOU NEED A PLOTMTV.RC FILE IN THE COURRENT WORK DIRECTORY,
   WHICH ENABLES THE PLOTSETS CALLED:   "direct1" and "direct2"

   in other words: plotmtv.rc should include  at least these lines :
   :direct1 
   %xlabel=""
   %ylabel=""
   %xlog=On
   %ylog=Off  
   %boundary=True
   EOF;
   :direct2 
   %xlabel=""
   %ylabel=""
   %xlog=On
   %ylog=Off
   %boundary=True
   EOF;
   # end of mplot.rc
   
   you can replace the words "True"(or "On")  with "False"(or "Off") and vice versa 
*/
extern void plotDirect(char *label,enum DATA_TYPE dtype,long npts,void *vector1,void *vector2,void *vector3,void *vector4  );

int plotChannel;

#endif
