/* This file has been created automatically. */
/* Any changes in this file will be lost.    */

#define D(a) {const void *dummyfilepointer; dummyfilepointer = &a;}
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdarg.h>          /* variable artgument list */
#include <stdlib.h>
#ifndef WIN32
#include <sys/wait.h>
#include <sys/stat.h>
#endif
#include <fcntl.h>
#include <math.h>
#include "plotmtv.h"
#include "common_m4a.h"
#define PRINTER_OPT "-Pml6" /* should be set to -Pprinter_name eg: -Pml6 */
#define PLOTSET_PRAEF0 "/tmp/" /* 1st praefix for plotset files , should be /tmp/ */
#define PLOTSET_PRAEF1 "/tmp/_"/* 2nd praefix for plotset files , should be /tmp/_ */
#define BG_COLOR "black" /* background color */
#define ERR_FNAME "plotmtv.err"
#define LOG_FNAME "plotmtv.log"
int plotChannel = 0; /* channel number that shall be displayed , is used only in the user programm */
int firstFrame = 9999;
int framePlot = 0; /* from user program , only used to display the frame and granule number */
#define WIN_GEOM_1 "750x600+1+1" 
#define WIN_GEOM_2 "750x600+1+1"
#define WIN_GEOM_3 "750x900+1+1"
#if 0 
#define WIN_GEOM_4 "1000x700+0+0"
#define WIN_GEOM_5 "1000x700+0+0"
#define WIN_GEOM_6 "1000x700+0+0"
#else
#define WIN_GEOM_4 "1100x600+1+1"
#define WIN_GEOM_5 "1200x980+1+1"
#define WIN_GEOM_6 "1200x980+1+1"
#endif
#define ACTIV      1
#define DELETED    0
#define TRUE       1
#define FALSE      0
#define STRING_SIZE 1024
#define DISAB_MESS                   0x00001
#define INIT_ERROR                   0x00002
#define INV_DATAFORMAT               0x00003
#define FILE_MISSING                 0x00004
#define WRITE_BIN                    0x00005
#define FATAL_ERROR                  0x00006
#define FOR_TESTING                  0x00012
#define MESSAGE1                     0x00013
#define FORK_FAILED                  0x00014
#define PLOT_RC                     0x00015
#define SQR(a) ((a)*(a))
int plotInit(int frameL)
{
  D(frameL)
  return(0);
}

int plotSend(char           legend[],
             char           plot_set[],
             enum DATA_TYPE dtype,
             long           npts,
             const void     *dataPtr,
             const void     *dataPtr2)
{
  D(legend)
  D(plot_set)
  D(dtype)
  D(npts)
  D(dataPtr)
  D(dataPtr2)
  return(0);
}

#if DEBUG_MTV
#endif 
void plotDisplay(int noWait)
{
  D(noWait)
}

void plotDirect(char           *label,
                enum DATA_TYPE dtype,
                long           npts,
                void           *vector1,
                void           *vector2,
                void           *vector3,
                void           *vector4)
{
  D(label)
  D(dtype)
  D(npts)
  D(vector1)
  D(vector2)
  D(vector3)
  D(vector4)
}

#undef D
