/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T

and edited by
  Olaf Kähler (Fraunhofer IIS-A)

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
Copyright (c) ISO/IEC 1999.
 *                                                                           *
 ****************************************************************************/

#ifndef _BITINPUT_H_
#define _BITINPUT_H_

#include "config.h"

typedef struct tagBitInput *HANDLE_BITINPUT;


void callbackOff( void );

/* getbits, byte_align and set_log_level use the global static_input */

void writebits(int n, long value);

long getbits(int n);
long GetBits(HANDLE_BITINPUT input, long n);

int byte_align(void);
int byteAlign(HANDLE_BITINPUT input);

void set_log_level(int id, int add_bits_num, int add_bits);
void setLogLevel(HANDLE_BITINPUT input, int id, int add_bits_num, int add_bits);

HANDLE_BITINPUT	bitinput_openFile(char *filename,
                                  ProgConfig *pc,
                                  MC_Info *mip,
                                  int *channelConfig,
                                  long *maxBitrate,
                                  long *avgBitrate,
                                  int *bufferFullness,
                                  int verbose);

int bitinput_resetCounter(HANDLE_BITINPUT input);
int bitinput_getNextBlock(HANDLE_BITINPUT input);
const ADTS_Header* bitinput_getCurrentADTSHeader(HANDLE_BITINPUT input);

void bitinput_close(HANDLE_BITINPUT input);


/* callback functions invoked at each GetBits and setLogLevel of the input */
void bitinput_setCallbacks(HANDLE_BITINPUT input,
                           void (*getbits_callback)(int numBits, long bitValue),
                           void (*set_log_level_callback)(int id, int add_bits_num, int add_bits));



/* maybe better moved elsewhere... */
int getDecoderBufferSize(const ProgConfig *pc, int channelConfig, long *decoderBufferSize );


/* not exactly part of bit input, but for command line parsing */

#define	ARGBEGIN	for(argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				char _argc;\
				_args = &argv[0][1];\
				if((_args[0]=='-') && (_args[1]==0)){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while((_argc = *_args++))\
				switch(_argc)
#define	ARGEND		}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc



#endif
