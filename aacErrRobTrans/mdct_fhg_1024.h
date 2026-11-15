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

/* General definitions: */

enum CHANNEL_NO {
    LEFT_CHAN               = 0,
        RIGHT_CHAN,
        CENTER_CHAN,
        LFE_CHAN,
        LEFT_SURR_CHAN,
        RIGHT_SURR_CHAN,
        MAX_CHANNELS            
};

enum WINDOW_TYPE {
    LONG_BLOCK              = 0,
        START_BLOCK,
        SHORT_BLOCK,
        STOP_BLOCK,
        AUTO_SWITCH     /* AT&T: filter bank detects switching inside */
};


/* Required definitions for time to frequency mapping module: */

/* definitions of the spectral resolutions of the windows */

#define SHORT_BLOCKS_IN_LONG_BLOCK      8       /* no of short blocks replacing one long block */
#define BLOCK_LEN_LONG                  1024    /* #spectral values in long blocks */
#define BLOCK_LEN_SHORT                 128     /* #spectral values in short blocks */
#define BLOCK_LEN_START                 1024    /* #spectral values in start blocks */
#define BLOCK_LEN_STOP                  1024    /* #spectral values in stop blocks */
#define N_SHORT_IN_START                4       /* GAD 10/30/95 */
#define N_SHORT_IN_STOP                 3


/* definitions of the frequency output buffer sizes */

#define OUTPUT_LEN_LONG   BLOCK_LEN_LONG
#define OUTPUT_LEN_SHORT  BLOCK_LEN_LONG
#define OUTPUT_LEN_START  BLOCK_LEN_LONG
#define OUTPUT_LEN_STOP   BLOCK_LEN_LONG
#define MAX_OUTPUT_LEN    BLOCK_LEN_LONG




/* definition of event map requested by AT&T and Dolby for internal use */

typedef struct {
    int n;          /* number of independent outputs */
    int *b_len;     /* array of lengths (shift) of outputs */
    int *event;     /* pointer to identified event */
} EVENT_MAP;

/* definition of gain info used by Sony. The gain info must be transmitted. 
   The maximum number of bits used for gain_info is 66*4+2 bits per block 
   per audio channel */

typedef struct {
    int lev;
    int loc;
} GDATA;

typedef struct {
    int      num_gain_data;
    GDATA gData[7];
} GINFO;

typedef struct {
    int     max_band;
    GINFO   gInfo[4];
} GAIN_INFO;

/* definition of filterbank control & info structure */

typedef struct {
    double          *p_time_signal;
    long            window_type;
    double          *p_spectrum;
    EVENT_MAP       **pp_event_map;
    GAIN_INFO       *gain_info;
} T2M_CHANNEL_INFO;


/* Interface for the time to frequency mapping module */
/* The delay added in the time2freq module is 3072 - BLOCK_LEN_LONG/2 */
/* No delay has been added in the synthesis filterbank */
/* The overall analysis/synthesis delay is 3072 + BLOCK_LEN_LONG/2 */

void time2freq(      /* Input */
long                    channel_enable_mask,    /*      bit #0 turns on processing of 
                                                                        channel #0 etc.         */
T2M_CHANNEL_INFO        ch_info[MAX_CHANNELS]
/*      input via       *p_time_signal, 
                        other input:    block_type, event_map
                        output spectrum is delivered to the location specified by *p_spectrum
                        optional output:        *gain_info      */
);


/* Interface for the frequency to time mapping module */

void freq2time(      /* Input */
long                    channel_enable_mask,    /*      bit #0 turns on processing of 
                                                                        channel #0 etc.         */
T2M_CHANNEL_INFO        decoder_ch_info[MAX_CHANNELS]
/*      input via       *p_spectrum, 
                        other input:    block_type,
                        optional input:         *gain_info
                        output time signal is delivered to the location specified by *p_time_sig        */
);
