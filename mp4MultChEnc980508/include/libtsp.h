/*------------ Telecommunications & Signal Processing Lab --------------
                         McGill University

Routine:
  libtsp.h

Description:
  Function prototypes and declarations for the TSP library routines

Author / revision:
  P. Kabal  Copyright (C) 1996
  $Revision: 1.1.1.2 $  $Date: 1998-05-07 17:19:31+03 $

----------------------------------------------------------------------*/

#ifndef libtsp_h_
#define libtsp_h_

#include <stdio.h>	/* typedef for FILE */
			/* defines FILENAME_MAX */
			/* stdio.h also defines NULL, size_t */

/* For non-ANSI headers */
#ifndef FILENAME_MAX	/* normally in stdio.h */
#  include <sys/param.h>
#  define FILENAME_MAX	MAXPATHLEN
#endif

/* For HPUX */
#ifdef hpux
#  undef FILENAME_MAX	/* defined in stdio.h as 14 */
#  include <sys/param.h>
#  define FILENAME_MAX	MAXPATHLEN
#endif

/* Audio file parameter structure */
#ifndef	AFILE_t_
#  define AFILE_t_
typedef struct AF_filepar AFILE;	/* Audio file parameters */
#endif

#ifdef __STDC__
#  define p_(params)	params
#else
#  define p_(params)	()
#  define const				/* Define const for K&R C */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ----- AF Prototypes ----- */
void
AFclose p_((AFILE *AFp));
char *
AFgetHinfo p_((void));
AFILE *
AFopenRead p_((const char Fname[], long int *Nsamp, long int *Nchan,
	       float *Sfreq, FILE *fpout));
AFILE *
AFopenWrite p_((const char Fname[], int Fformat, long int Nchan,
		double Sfreq, FILE *fpout));
int
AFreadData p_((AFILE *AFp, long int offs, float Fbuff[], int Nreq));
char *
AFreadHinfo p_((AFILE *AFp, int *Ninfo));
void
AFsetHinfo p_((const char String[]));
void
AFsetNH p_((const char String[]));
void
AFwriteData p_((AFILE *AFp, float Fbuff[], int Nval));

/* ----- FI Prototypes ----- */
void
FIbiquad p_((const float x[], float y[], int Nout, const float h[]));
void
FIconvSI p_((const float x[], float y[], int Nout, const float h[], int Ncof,
	     int mr, int Nsub, int Ir));
void
FIconvol p_((const float x[], float y[], int Nout, const float h[], int Ncof));
void
FIdeem p_((double a, float *Fmem, const float x[], float y[], int Nout));
void
FIfiltAP p_((const float x[], float y[], int Nout, const float p[], int Np));
void
FIfiltIIR p_((const float x[], float y[], int Nout, const float h[][5],
	      int Nsec));
void
FIpreem p_((double a, float *Fmem, const float x[], float y[], int Nout));
int
FIreadFilt p_((const char Fname[], int MaxNcof, float h[], int *Ncof,
	       FILE *fpout));
void
FIwinHamm p_((float win[], int N, double a));

/* ----- FL Prototypes ----- */
void
FLbackup p_((const char Fname[]));
int
FLbaseName p_((const char Fname[], char Bname[]));
int
FLdefName p_((const char Fi[], const char Fd[], char Fo[]));
int
FLdirName p_((const char Fname[], char Bname[]));
int
FLexpHome p_((const char Fname[], char Ename[]));
int
FLextName p_((const char Fname[], char Ext[]));
char *
FLfileDate p_((FILE *fp, int format));
int
FLfullName p_((const char Fname[], char Fullname[]));
char *
FLgetLine p_((FILE *fp));
char *
FLgetRec p_((FILE *fp, const char cc[], int echo));
int
FLhomeDir p_((const char User[], char Home[]));
int
FLpathList p_((const char Fi[], const char Dlist[], char Fo[]));
int
FLpreName p_((const char Fname[], char Pname[]));

/* ----- FN Prototypes ----- */
double
FNbessI0 p_((double x));
long int
FNgcd p_((long int u, long int v));
double
FNsinc p_((double x));


/* ----- MA Prototypes ----- */
float **
MAfAllocMat p_((int Nrow, int Ncol));

/* ----- MS Prototypes ----- */
double
MSdNint p_((double x));
double
MSfGaussRand p_((double rms));
double
MSfUnifRand p_((void));
int
MSiCeil p_((int n, int m));
int
MSiFloor p_((int n, int m));
void
MSrandSeed p_((int seed));
void
MSratio p_((double Val, long int *N, long int *D, double tol, long int MaxN,
	    long int MaxD));

/* ----- SP Prototypes ----- */
void
SPautoc p_((const float x[], int Nx, float cor[], int Nt));
double
SPcorXpc p_((const float rxx[], float pc[], int Np));
void
SPpcBWexp p_((double bwexp, const float pc[], float pcb[], int Np));
void
SPpcXec p_((const float pc[], float ec[], int Np));
int
SPquant p_((double x, const float Xq[], int Nreg));

/* ----- ST Prototypes ----- */
int
STcatMax p_((const char Si[], char So[], int Maxchar));
int
STcopyMax p_((const char Si[], char So[], int Maxchar));
int
STcopyNMax p_((const char Si[], char So[], int N, int Maxchar));
int
STdec1double p_((const char String[], double *Dval));
int
STdec1float p_((const char String[], float *Fval));
int
STdec1int p_((const char String[], int *Ival));
int
STdec1long p_((const char String[], long int *Lval));
int
STdecDfrac p_((const char String[], double *Dval1, double *Dval2));
int
STdecIfrac p_((const char String[], int *Ival1, int *Ival2));
int
STdecIrange p_((const char String[], int *Ival1, int *Ival2));
int
STdecLrange p_((const char String[], long int *Lval1, long int *Lval2));
int
STdecNfloat p_((const char String[], int Nmin, int Nmax, float Fval[],
		int *N));
char *
STfindToken p_((const char String[], const char Delims[],
		const char Quotes[], char Token[], int WSFlag,
		int Maxchar));
int
STkeyMatch p_((const char String[], const char *KeyTable[]));
int
STkeyXpar p_((const char Line[], const char Delims[], const char Quotes[],
	      const char *Keytable[], char Par[]));
int
STunQuote p_((const char Si[], const char Quotes[], char So[]));

/* ----- UT Prototypes ----- */
char *
UTdate p_((int format));
void
UTerror p_((const char Errmsg[], ...));
void
UTfree p_((void *ptr));
int
UTgetOption p_((int *Index, const int argc, const char *argv[],
		const char *OptTable[], const char **OptArg));
char *
UTgetProg p_((void));
void
UThalt p_((const char Errmsg[], ...));
void *
UTmalloc p_((int size));
void *
UTrealloc p_((void *ptr, int size));
void
UTsetProg p_((const char Program[]));
char *
UTuserName p_((void));
void
UTwarn p_((const char Warnmsg[], ...));

/* ----- VR Prototypes ----- */
int
VRfCheckSym p_((const float x[], int N));
double
VRfDotProd p_((const float x1[], const float x2[], int N));
void
VRfMult p_((const float x1[], const float x2[], float y[], int N));
void
VRfScale p_((double a, const float x[], float y[], int N));
void
VRfShift p_((float x[], int Nkeep, int Nshift));
void
VRfZero p_((float y[], int N));

#ifdef __cplusplus
}
#endif

#endif /* libtsp_h_ */
