/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

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
Copyright (c) ISO/IEC 1996.
 *                                                                           *
 ****************************************************************************/

#include "huffinit.h"
#include "hufftables.h"

/* --- local data --- */

static short	sfbwidth128[(1<<LEN_MAX_SFBS)];
static Info	eight_short_info;
static Info	only_long_info;

/* --- global data --- */

Hcb		book[NSPECBOOKS+2];
Float		iq_exp_tbl[MAX_IQ_TBL];
Float		exptable[TEXP];
int		maxfac;
Info		*win_seq_info[NUM_WIN_SEQ];


/* --- private functions --- */

/* coverts integer to binary string */
static char *
bin_str(unsigned long v, int len)
{
    int i, j;
    static char s[32];

    for (i=len-1, j=0; i>=0; i--, j++) {
	s[j] = (v & (1<<i)) ? '1' : '0';
    }
    s[j] = 0;
    return s;
}

static void
print_cb(Hcb *hcb) {
    int i;
    Huffman *hcw = hcb->hcw;
   PRINT(SE,"Huffman Codebook\n");
   PRINT(SE,"size %d, dim %d, lav %d, mod %d, off %d, signed %d\n", 
	hcb->n, hcb->dim, hcb->lav, hcb->mod, hcb->off, hcb->signed_cb);
   PRINT(SE,"%6s %6s %8s %-32s\n", "index", "length", "cw_10", "cw_2");
    for (i=0; i<hcb->n; i++)
	PRINT(SE,"%6d %6d %8ld %-32s\n",
	    hcw[i].index, hcw[i].len, hcw[i].cw,
	    bin_str(hcw[i].cw, hcw[i].len));
}    

static int
huffcmp(const void *va, const void *vb)
{
    const Huffman *a, *b;

    a = (Huffman *)va;
    b = (Huffman *)vb;
    if (a->len < b->len)
	return -1;
    if ( (a->len == b->len) && (a->cw < b->cw) )
	return -1;
    return 1;
}

/*
 * initialize the Hcb structure and sort the Huffman
 * codewords by length, shortest (most probable) first
 */
static void
hufftab(Hcb *hcb, Huffman *hcw, int dim, int lav, int signed_cb)
{
    int i, n;
    
    if (!signed_cb) {
	hcb->mod = lav + 1;
        hcb->off = 0;
    }
    else {
	hcb->mod = 2*lav + 1;
        hcb->off = lav;
    }
    n=1;	    
    for (i=0; i<dim; i++)
	n *= hcb->mod;
    hcb->n = n;
    hcb->dim = dim;
    hcb->lav = lav;
    hcb->signed_cb = signed_cb;
    hcb->hcw = hcw;
    
    if (debug['H'] && !debug['S']) print_cb(hcb);
    
    qsort(hcw, n, sizeof(Huffman), huffcmp);
    
    if (debug['H'] && debug['S']) print_cb(hcb);
}


static void
infoinit(int samp_rate_idx)
{ 
    int i, j, k, n, ws;
    short *sfbands;
    Info *ip;
    SR_Info *sip = &samp_rate_info[samp_rate_idx];

    /* long block info */
    ip = &only_long_info;
    win_seq_info[ONLY_LONG_WINDOW] = ip;
    ip->islong = 1;
    ip->nsbk = 1;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
	ip->sfb_per_sbk[i] = sip->nsfb1024;
	ip->sectbits[i] = LONG_SECT_BITS;
	ip->sbk_sfb_top[i] = sip->SFbands1024;
    }
    ip->sfb_width_128 = NULL;
    ip->num_groups = 1;
    ip->group_len[0] = 1;
    ip->group_offs[0] = 0;
    
    /* short block info */
    ip = &eight_short_info;
    win_seq_info[EIGHT_SHORT_WINDOW] = ip;
    ip->islong = 0;
    ip->nsbk = NSHORT;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
	ip->sfb_per_sbk[i] = sip->nsfb128;
	ip->sectbits[i] = SHORT_SECT_BITS;
	ip->sbk_sfb_top[i] = sip->SFbands128;
    }
    /* construct sfb width table */
    ip->sfb_width_128 = sfbwidth128;
    for (i=0, j=0, n=sip->nsfb128; i<n; i++) {
	k = sip->SFbands128[i];
	sfbwidth128[i] = k - j;
	j = k;
    }
    
    /* common to long and short */
    for (ws=0; ws<NUM_WIN_SEQ; ws++) {
        if ((ip = win_seq_info[ws]) == NULL)
            continue;
	ip->sfb_per_bk = 0;   
	k = 0;
	n = 0;
        for (i=0; i<ip->nsbk; i++) {
            /* compute bins_per_sbk */
	    ip->bins_per_sbk[i] = ip->bins_per_bk / ip->nsbk;
	    
            /* compute sfb_per_bk */
            ip->sfb_per_bk += ip->sfb_per_sbk[i];

            /* construct default (non-interleaved) bk_sfb_top[] */
            sfbands = ip->sbk_sfb_top[i];
            for (j=0; j < ip->sfb_per_sbk[i]; j++)
                ip->bk_sfb_top[j+k] = sfbands[j] + n;

            n += ip->bins_per_sbk[i];
            k += ip->sfb_per_sbk[i];
	}	    
 
	if (debug['I']) {
	    PRINT(SE,"\nsampling rate %d\n", sip->samp_rate);
	    PRINT(SE,"win_info\t%d has %d windows\n", ws, ip->nsbk);
	    PRINT(SE,"\tbins_per_bk\t%d\n", ip->bins_per_bk);
	    PRINT(SE,"\tsfb_per_bk\t%d\n", ip->sfb_per_bk);
	    for (i=0; i<ip->nsbk; i++) {
		PRINT(SE,"window\t%d\n", i);
		PRINT(SE,"\tbins_per_sbk\t%d\n", ip->bins_per_sbk[i]);
		PRINT(SE,"\tsfb_per_sbk	%d\n", ip->sfb_per_sbk[i]);
	    }
	    if (ip->sfb_width_128 != NULL) {
		PRINT(SE,"sfb top and width\n");
		for (i=0; i<ip->sfb_per_sbk[0]; i++)
		    PRINT(SE,"%6d %6d %6d\n", i, 
			ip->sbk_sfb_top[0][i], 
			ip->sfb_width_128[i]);
	    }	   
	}	 
    }	    
}


/* --- public functions --- */
 
void
huffbookinit(int samp_rate_idx)
{
    int i;

    hufftab(&book[1], book1, 4, 1, HUF1SGN);
    hufftab(&book[2], book2, 4, 1, HUF2SGN);
    hufftab(&book[3], book3, 4, 2, HUF3SGN);
    hufftab(&book[4], book4, 4, 2, HUF4SGN);
    hufftab(&book[5], book5, 2, 4, HUF5SGN);
    hufftab(&book[6], book6, 2, 4, HUF6SGN);
    hufftab(&book[7], book7, 2, 7, HUF7SGN);
    hufftab(&book[8], book8, 2, 7, HUF8SGN);
    hufftab(&book[9], book9, 2, 12, HUF9SGN);
    hufftab(&book[10], book10, 2, 12, HUF10SGN);
    hufftab(&book[11], book11, 2, 16, HUF11SGN);

    hufftab(&book[BOOKSCL], bookscl, 1, 60, 1);

    for(i = 0; i < TEXP; i++){
	exptable[i]  = pow( 2.0,  0.25*i);
    }
    maxfac = TEXP;

    for(i = 0; i < MAX_IQ_TBL; i++){
	iq_exp_tbl[i] = pow((double)i, 4./3.);
    }

    infoinit(samp_rate_idx);
}

