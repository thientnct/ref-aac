/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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
Copyright(c)1996, 1997, 1998.
 *                                                                           *
 ****************************************************************************/

#ifndef _all_h_
#define _all_h_

#include <sys/types.h>

#include "interface.h"
#include "block.h"

#ifndef HAS_Float
#define HAS_Float
typedef float   Float;
#endif /*HAS_FLOAT*/

#ifndef HAS_ULONG
#define HAS_ULONG
typedef unsigned long   ulong;
#endif /*HAS_ULONG*/

typedef unsigned char   byte;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define nil                     ((void*)0)
#define Tnleaf                  0x8000

#define    FChans        3      /* front channels: left, center, right */
#define    FCenter       1      /* 1 if decoder has front center channel */
#define    SChans        2      /* side channels: */
#define    BChans        2      /* back channels: left surround, right surround */
#define    BCenter       0      /* 1 if decoder has back center channel */
#define    LChans        0      /* 1 * LFE channels */
#define    XChans        1      /* scratch space for parsing unused channels */  
#define    ICChans       0      /* 1 * independently switched coupling channels */
#define    DCChans       0      /* 2 * dependently switched coupling channels */
#define    XCChans       0      /* 1 * scratch space for parsing unused coupling channels */

#define    Chans         (FChans + SChans + BChans + LChans + XChans)

#if 0    
#define    CChans        (ICChans + DCChans + XCChans)
#endif

enum
{
  /* block switch windows for single channels or channel pairs */
  Winds       = Chans,
    
  /* average channel block length, bytes */
  Avjframe    = 341,  

  TEXP        = 128,          /* size of exp cache table */
  MAX_IQ_TBL  = 128,          /* size of inv quant table */
  MAXFFT      = LN4,

  XXXXX
};


typedef struct {
  int     samp_rate;
  int     nsfb1024;
  short*  SFbands1024;
  int     nsfb128;
  short*  SFbands128;
  int     nsfb960;
  short*  SFbands960;
  int     nsfb120;
  short*  SFbands120;
  int    shortFssWidth;
  int    longFssGroups;
} SR_Info;

typedef struct
{
  WINDOW_SHAPE    this_bk;
  WINDOW_SHAPE    prev_bk;
} Wnd_Shape;

typedef struct
{
  int           index;
  int           len;
  unsigned long cw;
} Huffman;

typedef struct
{
  int           n;
  int           dim;
  int           lav;
  int           lavInclEsc;
  int           mod;
  int           off;
  int           signed_cb;
  unsigned char maxCWLen;
  Huffman*      hcw;
} Hcb;

typedef struct
{
  int present;        /* channel present */
  int tag;            /* element tag */
  int cpe;            /* 0 if single channel, 1 if channel pair */
  int common_window;  /* 1 if common window for cpe */
  int ch_is_left;     /* 1 if left channel of cpe */
  int paired_ch;      /* index of paired channel in cpe */
  int widx;           /* window element index for this channel */
  int is_present;     /* intensity stereo is used */
  int ncch;           /* number of coupling channels for this ch */
#if (CChans > 0)
  int cch[CChans];    /* coupling channel idx */
  int cc_dom[CChans]; /* coupling channel domain */
  int cc_ind[CChans]; /* independently switched coupling channel flag */
#endif
  char *fext;         /* filename extension */
} Ch_Info;

typedef struct {
  int           nch;            /* total number of audio channels */
  int           nfsce;          /* number of front SCE's pror to first front CPE */
  int           nfch;           /* number of front channels */
  int           nsch;           /* number of side channels */
  int           nbch;           /* number of back channels */
  int           nlch;           /* number of lfe channels */
  int           ncch;           /* number of valid coupling channels */
  int           cch_tag[(1<<LEN_TAG)];  /* tags of valid CCE's */
  int           profile;
  int           sampling_rate_idx;
  Ch_Info       ch_info[Chans];
  unsigned char mcInfoCorrectFlag;
} MC_Info;

typedef struct {
  int num_ele;
  int ele_is_cpe[(1<<LEN_TAG)];
  int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
  int present;
  int ele_tag;
  int pseudo_enab;
} MIXdown;

typedef struct {
  int profile;
  int sampling_rate_idx;
  EleList front;
  EleList side;
  EleList back;
  EleList lfe;
  EleList data;
  EleList coupling;
  MIXdown mono_mix;
  MIXdown stereo_mix;
  MIXdown matrix_mix;
  char comments[(1<<LEN_PC_COMM)+1];
  long    buffer_fullness;    /* put this transport level info here */
} ProgConfig;

typedef struct {
  char    adif_id[LEN_ADIF_ID+1];
  int     copy_id_present;
  char    copy_id[LEN_COPYRT_ID+1];
  int     original_copy;
  int     home;
  int     bitstream_type;
  long    bitrate;
  int     num_pce;
  int     prog_tags[(1<<LEN_TAG)];
} ADIF_Header;

#ifndef DOLBY_MDCT
#define fltmul(a,b)             ((a)*(b))
#define mkflt(f)                ((float)(f))
#endif


#endif  /* _all_h_ */
