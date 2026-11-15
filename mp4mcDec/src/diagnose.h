/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Fraunhofer IIS-A

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
Copyright (c) ISO/IEC 2001.
 *                                                                           *
 ****************************************************************************/

#ifndef	_diagnose_h_
#define	_diagnose_h_

#include "block.h"
#include "config.h"

/* diagnostic functions */

/* ---- bit buffer simulation ---- */
typedef struct tagBufferSimulation *BUFFER_SIMULATION;

/* - initialise buffer simulation */
BUFFER_SIMULATION create_bitBuffer_sim(void);
void		bitBuffer_sim_setFrameLen(BUFFER_SIMULATION sim, double frameLen, int bufferSize);
void		bitBuffer_sim_adjustFullness(BUFFER_SIMULATION sim, int fullness);
void		bitBuffer_sim_testPeakBitrate(BUFFER_SIMULATION sim, double peakRate);

/* - advance a frame and/or feed some extra bits */
double		update_bitBuffer_sim(BUFFER_SIMULATION sim, int bits, const ADTS_Header *adts_head);
void		feed_bitBuffer_sim(BUFFER_SIMULATION sim, int bits);

/* - query state of buffer simulation */
int		check_bitBuffer_sim(BUFFER_SIMULATION sim, const char *name, int show_buffer, int show_details, int show_rate);
double		bitBuffer_sim_getFullness(BUFFER_SIMULATION sim);
double		bitBuffer_sim_getCurrentBitrate(BUFFER_SIMULATION sim);
double		bitBuffer_sim_calculateBitrate(BUFFER_SIMULATION sim);
long		bitBuffer_sim_getTotalBits(BUFFER_SIMULATION sim);
void		bitBuffer_sim_getMinMax(BUFFER_SIMULATION sim, double *min, double *max);


/* ---- CRC calculator ---- */
typedef struct tagCRCcalculator *HANDLE_CRC_CALCULATOR;


HANDLE_CRC_CALCULATOR CRC_calculator_create(void);
void		CRC_calculator_free(HANDLE_CRC_CALCULATOR crc);
void		CRC_enable(HANDLE_CRC_CALCULATOR crc, int length, int second);
void		CRC_feed_calculator(HANDLE_CRC_CALCULATOR crc, int length, unsigned long value);
void		CRC_fillup(HANDLE_CRC_CALCULATOR crc);
unsigned long	CRC_reset(HANDLE_CRC_CALCULATOR crc);


/* ---- check tools used ---- */
void		show_tools(MC_Info* mip,Wnd_Shape* wnd_shape,byte *hasmask);

extern	int	is_present;    /* intensity */
extern	int	tns_present;   /* TNS */
extern	int	pulse_present; /* pulse data */
extern	int	gc_present;    /* gain control */
extern	int	dse_present;   /* data stream element */
#ifdef DRC
extern	int	drc_present;   /* DRC data */
#endif
#ifdef MPEG4V1
extern	int	noise_present; /* noise energy */
extern	int	ltp_present;   /* LTP data */
extern	int	stream_is_mpeg4; /* is one of PNS and LTP used? */
#endif


#define BITRATE2FRAMELEN(br)	(((double)(br)*aacFrameLength)/mc_info.sampling_rate)
#define FRAMELEN2BITRATE(fl)	(((double)(fl)*mc_info.sampling_rate)/aacFrameLength)

#ifdef FRAMELENGTH_FLAG
#define BITRATE2FRAMELEN960(br)	(((double)(br)*960)/mc_info.sampling_rate)
#define FRAMELEN2BITRATE960(fl)	(((double)(fl)*mc_info.sampling_rate)/960)
#endif


#endif	/* _diagnose_h_ */
