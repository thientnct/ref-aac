/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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

#ifndef	_all_h_
#define _all_h_

#include "common.h"
#include "stdinc.h"
#include "interface.h"
/* #include "monopred.h" */ 
#include "tns.h"
#include "dec.h"  /* this will include bitstream.h */

/*typedef	float	Float;*/

/*
#ifndef HAS_ULONG
typedef unsigned long   ulong;
#endif
*/

typedef	unsigned char	byte;

#define	nil			((void*)0)
#define	Tnleaf			0x8000

enum
{
#if 1
    /*
     * channels for mono main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 1,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 0,	/* side channels: */
    BChans	= 0,	/* back channels: left surround, right surround */
    BCenter	= 0,	/* 1 if decoder has back center channel */
    LChans	= 0,	/* LFE channels */
    XChans	= 0,	/* scratch space for parsing unused channels */  
    
    ICChans	= 0,	/* independently switched coupling channels */
    DCChans	= 0,	/* dependently switched coupling channels */
    XCChans	= 0,	/* scratch space for parsing unused coupling channels */

#else
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 3,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 0,	/* side channels: */
    BChans	= 2,	/* back channels: left surround, right surround */
    BCenter	= 0,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 1,	/* scratch space for parsing unused channels */  
    
    ICChans	= 1,	/* independently switched coupling channels */
    DCChans	= 2,	/* dependently switched coupling channels */
    XCChans	= 1,	/* scratch space for parsing unused coupling channels */
#endif    

    Chans	= FChans + SChans + BChans + LChans + XChans,
    CChans	= ICChans + DCChans + XCChans,
    
    /* block switch windows for single channels or channel pairs */
    Winds	= Chans,
    
    /* average channel block length, bytes */
    Avjframe	= 341,	

    TEXP	= 128,		/* size of exp cache table */
    MAX_IQ_TBL	= 128,		/* size of inv quant table */
    MAXFFT	= LN4,

    XXXXX
};

typedef struct
{
    int	    islong;			/* true if long block */
    int	    nsbk;			/* sub-blocks (SB) per block */
    int	    bins_per_bk;		/* coef's per block */
    int	    sfb_per_bk;			/* sfb per block */
    int	    bins_per_sbk[MAX_SBK];	/* coef's per SB */
    int	    sfb_per_sbk[MAX_SBK];	/* sfb per SB */
    int	    sectbits[MAX_SBK];
    short   *sbk_sfb_top[MAX_SBK];	/* top coef per sfb per SB */
    short   *sfb_width_128;		/* sfb width for short blocks */
    short   bk_sfb_top[200];		/* cum version of above */
    int	    num_groups;
    short   group_len[8];
    short   group_offs[8];
} Info;

typedef struct {
    int	    samp_rate;
    int	    nsfb1024;
    short*  SFbands1024;
    int	    nsfb128;
    short*  SFbands128;
} SR_Info;

typedef struct
{
    byte    this_bk;
    byte    prev_bk;
} Wnd_Shape;

typedef struct
{
    int		   index;
    int		   len;
    unsigned long  cw;
} Huffman;

typedef	struct
{
    int		n;
    int		dim;
    int		lav;
    int		mod;
    int		off;
    int		signed_cb;
    Huffman	*hcw;
} Hcb;

typedef struct
{
    Float	sin;
    Float	cos;
}Stab;

typedef struct
{
    int is_present;	/* right channel uses intensiy stereo */
    int is_used[MAXBANDS];
    int sign[2*(MAXBANDS+1)];
    int fac[2*(MAXBANDS+1)];
    int bot[2*(MAXBANDS+1)];
    int top[2*(MAXBANDS+1)];
} IS_Info;

typedef struct
{
    int is_present;  
    int ms_used[MAXBANDS];
} MS_Info;

typedef struct
{
    int present;	/* channel present */
    int tag;		/* element tag */
    int cpe;		/* 0 if single channel or lfe, 1 if channel pair */ 
    int lfe;            /* 1 if lfe channel */             
    int	common_window;	/* 1 if common window for cpe */
    int	ch_is_left;	/* 1 if left channel of cpe */
    int	paired_ch;	/* index of paired channel in cpe */
    int widx;		/* window element index for this channel */
    IS_Info is_info;	/* IS information */
    MS_Info ms_info;    /* MS information */
    int ncch;		/* number of coupling channels for this ch */
#if (CChans > 0)
    int cch[CChans];	/* coupling channel idx */
    int cc_dom[CChans];	/* coupling channel domain */
    int cc_ind[CChans];	/* independently switched coupling channel flag */
#endif
    char *fext;		/* filename extension */
} Ch_Info;

typedef struct {
    int nch;		/* total number of audio channels */
    int nfsce;		/* number of front SCE's pror to first front CPE */
    int nfch;		/* number of front channels */
    int nsch;		/* number of side channels */
    int nbch;		/* number of back channels */
    int nlch;		/* number of lfe channels */
    int ncch;		/* number of valid coupling channels */
    int cch_tag[(1<<LEN_TAG)];	/* tags of valid CCE's */
    int profile;
    int sampling_rate_idx;
    Ch_Info ch_info[Chans];
} MC_Info;

typedef struct {
    int num_ele;
    int ele_is_cpe[(1<<LEN_TAG)];
    int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
    int profile;
    int sampling_rate_idx;
    EleList front;
    EleList side;
    EleList back;
    EleList lfe;
    EleList data;
    EleList coupling;
    EleList mono_mix;
    EleList stereo_mix;
    char comments[(1<<LEN_PC_COMM)+1];
    long    buffer_fullness;	/* put this transport level info here */
} ProgConfig;

typedef struct {
    char    adif_id[LEN_ADIF_ID+1];
    int	    copy_id_present;
    char    copy_id[LEN_COPYRT_ID+1];
    int	    original_copy;
    int	    home;
    int	    bitstream_type;
    long    bitrate;
    int	    num_pce;
    int	    prog_tags[(1<<LEN_TAG)];
} ADIF_Header;

extern	long		bno;
extern	Huffman		book1[];
extern	Huffman		book2[];
extern	Huffman		book3[];
extern	Huffman		book4[];
extern	Huffman		book5[];
extern	Huffman		book6[];
extern	Huffman		book7[];
extern	Huffman		book8[];
extern	Huffman		book9[];
extern	Huffman		book10[];
extern	Huffman		book11[];
extern	Huffman		bookscl[];
extern	Hcb		book[NSPECBOOKS+2];
extern	int		bufpool;
extern	int		chn_present;
extern	long		cword;
extern	int		debug[256];
extern	Info		eight_short_info;
extern	Float		exptable[TEXP];
extern	Float*		hvals[NSPECBOOKS];
extern	Float		iq_exp_tbl[MAX_IQ_TBL];
extern	int		inerror;
extern	int		istrans;
extern	int		*lpflag[Chans];
extern	int		maxfac;
extern	int		nbits;
extern  int             numUsedBits;
extern	Info		only_long_info;
extern	short		sfbwidth128[];
extern	SR_Info		samp_rate_info[];
extern	byte		*sect;
extern	Stab		table2[MAXFFT];
extern	Stab		table[MAXFFT/2];
extern	long		total1;
extern	long		total2;
extern	Info*		winmap[NUM_WIN_SEQ];
extern	Info		*win_seq_info[NUM_WIN_SEQ];

extern	int		default_config;
extern	int		adif_header_present;
extern	int		current_program;
extern	ADIF_Header	adif_header;
extern	ProgConfig	prog_config;
extern	MC_Info		mc_info;

#include "port.h"
#ifndef	DOLBY_MDCT
#define fltmul(a,b)		((a)*(b))
#define mkflt(f)		((float)(f))
#endif


#endif	/* _all_h_ */
