/*------------ Telecommunications & Signal Processing Lab --------------
                         McGill University

Routine:
  AFpar.h

Description:
  Declarations for the TSP audio file routines.

Author / revision:
  P. Kabal  Copyright (C) 1996
  $Revision: 1.1.1.2 $  $Date: 1998-05-07 17:19:32+03 $

----------------------------------------------------------------------*/

#ifndef AFpar_h_
#define AFpar_h_

#ifndef	AFILE_t_
#  define	AFILE_t_
typedef struct AF_filepar AFILE;	/* Audio file parameters */
#endif

/* Audio file parameter structure */
struct AF_filepar {
  FILE *fp;		/* File pointer */
  int Op;		/* Operation (read, write) */
  int Ftype;		/* File type */
  int Format;		/* Data format */
  int Swapb;		/* Swap flag */
  float ScaleF;		/* Scale factor */
  long int Nchan;	/* Number of channels */
  long int Start;	/* Start byte */
  long int End;		/* End-of-file byte */
  long int Isamp;	/* Sample offset */
  long int Nsamp;	/* Number of samples */
  long int Novld;	/* Number of points clipped */
};

/* Data format types - must be sequential */
enum {
  FD_UNDEF	= 0,	/* undefined file data format */
  FD_MULAW8	= 1,	/* mu-law 8-bit data */
  FD_ALAW8	= 2,	/* A-law 8-bit data */
  FD_UINT8	= 3,	/* offset binary integer 8-bit data */
  FD_INT8	= 4,	/* twos complement integer 8-bit data */
  FD_INT16	= 5,	/* twos complement integer 16-bit data */
  FD_FLOAT32	= 6,	/* 32-bit float data */
  FD_TEXT	= 7	/* text data */
};
#define NFD		(FD_TEXT+1)
#define FD_DEFAULT	FD_INT16	/* Default data format */

#define	FD_NH	256	/* headerless file flag */
#define	FD_BO	512	/* byte order offset */

/* Data format sizes */
#define FDL_MULAW8	1
#define FDL_ALAW8	1
#define FDL_UINT8	1
#define FDL_INT8	1
#define FDL_INT16	2
#define	FDL_FLOAT32	4
#define FDL_TEXT	0	/* Variable size */

/* Machine byte order types */
#define	SF_EB	0	/* big-endian */
#define	SF_EL	1	/* little-endian */

/* Byte order values */
enum {
  DS_UNDEF	= -1,		/* undefined */
  DS_EB		= SF_EB,	/* big-endian format data */
  DS_EL		= SF_EL,	/* little-endian format data*/
  DS_NATIVE	= 2,		/* native (no swap) format data */
  DS_SWAP	= 3		/* byte-swapped format data */
};

/* Internal codes for audio file types - must be sequential */
enum {
  FT_UNKNOWN	= 0,	/* unknown audio file format */
  FT_NH		= 1,	/* headerless (non-standard or no header) audio file */
  FT_AFSP	= 2,	/* AFsp audio file */
  FT_SUN	= 3,	/* Sun audio file */
  FT_WAVE	= 4,	/* RIFF WAVE file */
  FT_AIFF_C	= 5,	/* AIFF-C audio file */
  FT_SPHERE	= 6,	/* NIST SPHERE audio file */
  FT_ESPS	= 7,	/* ESPS sampled data feature file */
  FT_SF		= 8,	/* IRCAM soundfile */
  FT_SPPACK	= 9, 	/* SPPACK file */
  FT_INRS	= 10,	/* INRS-Telecom audio file */
  FT_AIFF	= 11,	/* AIFF audio file */
  FT_TXAUD	= 12	/* Text audio file */
};
#define NFT	(FT_TXAUD+1)

/* External file type codes for AFopenWrite */
#define FW_FTYPE_MOD	256
#define FW_code(ftype)	(ftype * FW_FTYPE_MOD)
#define FW_AFSP		FW_code (0)
#  define FW_SUN	FW_AFSP
#define FW_WAVE		FW_code (1)
#define FW_AIFF_C	FW_code (2)
#define FW_NH_EB	FW_code (3)
#define FW_NH_EL	FW_code (4)
#define FW_NH_NATIVE	FW_code (5)
#define FW_NH_SWAP	FW_code (6)
#define FW_UNDEF	FW_code (7)	/* Undefined file type */
#define FW_DEFAULT	FW_AFSP		/* Default file type */

/* File operation types */
enum {
  FO_NONE	= 0,	/* closed */
  FO_RO 	= 1,	/* read */
  FO_WO 	= 2	/* write */
};

/* Structure for headerless audio file parameters */
struct AF_NHpar {
  int Accept;		/* Headerless flag */
  int Format;		/* Data format */
  long int Start;	/* Offset in bytes to the start of data */
  float Sfreq;		/* Sampling frequency */
  int Swapb;		/* Byte swap flag */
  long int Nchan;	/* Number of channels */
  float ScaleF;		/* Scale factor */
};

/* Headerless flag values */
enum {
  NH_REJECT 	= 0,	/* reject headerless files */
  NH_ACCEPT 	= 1,	/* accept headerless files, check other types first */
  NH_FORCE	= 2	/* force interpretation as a headerless file */
};

#endif	/* AFpar_h_ */
