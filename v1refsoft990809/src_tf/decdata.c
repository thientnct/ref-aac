/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporation)
,Mitsuyuki Hatanaka (Sony Corporation),
in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include "block.h"               /* handler, defines, enums */
#include "interface.h"           /* handler, defines, enums */
#include "mod_bufHandle.h"       /* handler, defines, enums */
#include "tf_mainHandle.h"       /* handler, defines, enums */

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */


int		inerror;
int		bufpool;
long		bno;
Float		iq_exp_tbl[MAX_IQ_TBL];
Float		exptable[TEXP];
Hcb		book[NSPECBOOKS+2+32]; /*TM 990527 */
/* Stab		table[MAXFFT/2]; */
/* Stab		table2[MAXFFT]; */
/* int		adif_header_present; */
int		current_program;
ADIF_Header	adif_header;
ProgConfig	prog_config;
/* MC_Info		mc_info; */
/* int		debug[256]; */
Info		*win_seq_info[NUM_WIN_SEQ];
Info		only_long_info;
Info		eight_short_info;
Info*		winmap[NUM_WIN_SEQ];
short		sfbwidthShort[(1<<LEN_MAX_SFBS)];


short            
sfb_96_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 96, 108, 120, 
  132, 144, 156, 172, 188, 212, 240, 
  276, 320, 384, 448, 512, 576, 640, 
  704, 768, 832, 896, 960, 1024
};   /* 41 scfbands */

short
sfb_96_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */

short
sfb_64_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  140, 156, 172, 192, 216, 240, 268, 
  304, 344, 384, 424, 464, 504, 544, 
  584, 624, 664, 704, 744, 784, 824, 
  864, 904, 944, 984, 1024
};   /* 41 scfbands 47 */ 

short
sfb_64_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */

#ifdef SRS
int     ssr_decoder_band = 4;
#endif


short sfb_48_1024[] = {
        4,	8,	12,	16,	20,	24,	28,	
        32,	36,	40,	48,	56,	64,	72,	
        80,	88,	96,	108,	120,	132,	144,	
        160,	176,	196,	216,	240,	264,	292,	
        320,	352,	384,	416,	448,	480,	512,	
        544,	576,	608,	640,	672,	704,	736,	
        768,	800,	832,	864,	896,	928,	1024
};

short sfb_48_960[] = { 
        4,      8,      12,     16,     20,     24,     28,
        32,     36,     40,     48,     56,     64,     72,
        80,     88,     96,     108,    120,    132,    144,
        160,    176,    196,    216,    240,    264,    292,
        320,    352,    384,    416,    448,    480,    512,
        544,    576,    608,    640,    672,    704,    736,
        768,    800,    832,    864,    896,    928,    960, 0 };


short sfb_48_128[] =
{
          4,	8,	12,	16,	20,	28,	36,	
          44,	56,	68,	80,	96,	112,	128
};

short sfb_48_120[] =
{         4,    8,      12,     16,     20,     28,     36,
          44,   56,     68,     80,     96,     112,    120};

short
sfb_32_1024[] =
{
        4,	8,	12,	16,	20,	24,	28,	
        32,	36,	40,	48,	56,	64,	72,	
        80,	88,	96,	108,	120,	132,	144,	
        160,	176,	196,	216,	240,	264,	292,	
        320,	352,	384,	416,	448,	480,	512,	
        544,	576,	608,	640,	672,	704,	736,	
        768,	800,	832,	864,	896,	928,	960,
        992,	1024
};


short sfb_24_1024[] =
{
        4,   8,   12,  16,  20,  24,  28, 
        32,  36,  40,  44,  52,  60,  68, 
        76,  84,  92,  100, 108, 116, 124, 
        136, 148, 160, 172, 188, 204, 220, 
        240, 260, 284, 308, 336, 364, 396, 
        432, 468, 508, 552, 600, 652, 704, 
        768, 832, 896, 960, 1024
};   /* 47 scfbands */

short sfb_24_960[] =
{
        4,   8,   12,  16,  20,  24,  28,
        32,  36,  40,  44,  52,  60,  68,
        76,  84,  92,  100, 108, 116, 124,
        136, 148, 160, 172, 188, 204, 220,
        240, 260, 284, 308, 336, 364, 396,
        432, 468, 508, 552, 600, 652, 704,
        768, 832, 896, 960,0
};   /* 47 scfbands */

short sfb_24_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};   /* 15 scfbands */

short sfb_24_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 120, 0
};   /* 15 scfbands */


short sfb_16_1024[] =
{
  8,   16,  24,  32,  40,  48,  56, 
  64,  72,  80,  88,  100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 772, 832, 896, 960, 
  1024
};   /* 43 scfbands */

short sfb_16_960[] =
{
  8,   16,  24,  32,  40,  48,  56, 
  64,  72,  80,  88,  100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 772, 832, 896, 960
};   /* 42 scfbands */

short sfb_16_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};   /* 15 scfbands */

short sfb_16_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 120
};   /* 15 scfbands */


short
sfb_8_1024[] =
{
  12, 24, 36, 48, 60, 72, 84, 
  96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 820, 880, 944, 1024
};   /* 40 scfbands */

short
sfb_8_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 108, 
  128
};   /* 15 scfbands */
 



short
sfb_8_960[] =
{
  12, 24, 36, 48, 60, 72, 84, 
  96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 820, 880, 944, 960
};   /* 40 scfbands */

short
sfb_8_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 108, 
  120
};   /* 15 scfbands */


short
sfb_32_960[] =
{ 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 
    352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960,0 };

short
sfb_32_120[] =
{ 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 120, 0 };
SR_Info
samp_rate_info[(1<<LEN_SAMP_IDX)] = {
    /* sampling_frequency, #long sfb, long sfb, #short sfb, short sfb */
    /* samp_rate, nsfb1024, SFbands1024, nsfb128, SFbands128 */
    {96000, 41, sfb_96_1024, 12, sfb_96_128},	    /* 96000 */
    {88200, 41, sfb_96_1024, 12, sfb_96_128},	    /* 88200 */
    {64000, 47, sfb_64_1024, 12, sfb_64_128},	    /* 64000 */
    {48000, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120,18,5
},	    /* 48000 */
    {44100, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120,18,5
},	    /* 44100 */
    {32000, 51, sfb_32_1024, 14, sfb_48_128, 49, sfb_32_960, 14, sfb_48_120,26,6
},	    /* 32000 */
    {24000, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_960, 15, sfb_24_120,36,8
},	    /* 24000 */
    {22050, 47, sfb_24_1024, 15, sfb_24_128
},	    /* 22050 */
    {16000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120,54,8},	    /* 16000 */
    {12000, 43, sfb_16_1024, 15, sfb_16_128},	    /* 12000 */
    {11025, 43, sfb_16_1024, 15, sfb_16_128},	    /* 11025 */
    {8000,  40, sfb_8_1024,  15, sfb_8_128 },	    /* 8000  */
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0}
};

int
tns_max_bands_tbl[(1<<LEN_SAMP_IDX)][4] = {
    /* entry for each sampling rate	
     * 1    Main/LC long window
     * 2    Main/LC short window
     * 3    SSR long window
     * 4    SSR short window
     */
    {31,  9, 28,  7},	    /* 96000 */
    {31,  9, 28,  7},	    /* 88200 */
    {34, 10, 27,  7},	    /* 64000 */
    {40, 14, 26,  6},	    /* 48000 */
    {42, 14, 26,  6},	    /* 44100 */
    {51, 14, 26,  6},	    /* 32000 */
    {46, 14, 29,  7},	    /* 24000 */
    {46, 14, 29,  7},	    /* 22050 */
    {42, 14, 23,  8},	    /* 16000 */
    {42, 14, 23,  8},	    /* 12000 */
    {42, 14, 23,  8},	    /* 11025 */
    {39, 14, 19,  7},	    /* 8000  */
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};


int
pred_max_bands_tbl[(1<<LEN_SAMP_IDX)] = {
  33,     /* 96000 */
  33,     /* 88200 */
  38,     /* 64000 */
  40,     /* 48000 */
  40,     /* 44100 */
  40,     /* 32000 */
  41,     /* 24000 */
  41,     /* 22050 */
  37,     /* 16000 */
  37,     /* 12000 */
  37,     /* 11025 */
  34,     /* 8000  */
  0,
  0,
  0,
  0
};
