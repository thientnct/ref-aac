/*****************************************************************************
 *                                                                           *
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  AT&T, Dolby Laboratories, Fraunhofer IIS-A

and edited by
  Takashi Koike (Sony Corporation)

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

#ifndef	_port_h_
#define	_port_h_

#ifdef SRS
#include "bitstream.h"
#endif
#include "all.h"

int		byte_align(void);
void		byteclr(byte *ip1, int cnt);
int		ch_index(MC_Info *mip, int cpe, int tag);
void		check_mc_info(MC_Info *mip, int new_config);
int		chn_config(int id, int tag, int common_window, MC_Info *mip);
/* void		clr_tns( Info *info, TNS_frame_info *tns_frame_info ); */
void		coupling(MC_Info *mip, Float **coef, Float **cc_coef, Float **cc_gain, int ch, int cc_dom, int cc_dep);
int		decode_huff_cw(Huffman *h);
int		endblock(void);
int		enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);
int		enter_mc_info(MC_Info *mip, ProgConfig *pcp);
Float		esc_iquant(int q);
long		f2ir(float x);
void		fltclr(Float *dp1, int cnt);
void		fltcpy(Float *dp1, Float *dp2, int cnt);
void		fltset(Float *dp1, Float dval, int cnt);
void		fmtchan(char *p, float *data, int stride);
int		get_adif_header(void);
void		get_ics_info(byte *win, byte *wshape, byte *group,byte *max_sfb, int *lpflag, int *prstflag);
int		get_prog_config(ProgConfig *p);
void		get_sign_bits(int *q, int n);
/*int		get_tns( Info *info, TNS_frame_info *tns_frame_info );*/
long		getbits(int n);
int		getbyte(void);
#if (CChans > 0)
int		getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef,Float *cc_gain[CChans][Chans]);
#endif
int		getdata(int *tag, int *dt_cnt, byte *data_bytes);
int		getescape(int q);
void		getfill(void);
void		getgroup(Info *info, byte *group);
/* int		getics(Info *info, int common_window, byte *win, byte *wshape, byte *group, byte max_sfb, int *lpflag, int *prstflag, int *pns, byte *sect, Float *coef, short *global_gain, short *factors, TNS_frame_info *tns 
#ifdef	SRS
		, BsBitStream *
#endif
		       ); */
int		getmask(Info *info, byte *group, byte max_sfb, byte *mask);
long		getshort(void);
void		huffbookinit(void);
int		huffcb(byte *sect, int *sectbits, int tot_sfb, int sfb_per_sbk, byte max_sfb);
void		huffdecinit(void);
/* int		huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, byte **group, byte *hasmask, byte **mask, byte *max_sfb, int **lpflag, int **prstflag, TNS_frame_info **tns,
#ifdef SRS
			   BsBitStream *gc_stream[],
#endif
			   Float **coef); */
int		hufffac(Info *info, byte *group, int nsect, byte *sect,short global_gain, short *factors);
int		huffspec(Info *info, int nsect, byte *sect, short *factors, Float *coef);
void		hufftab(Hcb *hcb, Huffman *hcw, int dim, int lav, int signed_cb);
void		infoinit(SR_Info *sip);
void		init(void);
void		init_cc(void);
/*void		init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b);*/
void		ind_coupling(MC_Info *mip, byte *wnd, Wnd_Shape *wnd_shape, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float *cc_coef);
void		intclr(int *ip1, int cnt);
void		intcpy(int *ip1, int *ip2, int cnt);
void		intensity(MC_Info *mip, Info *info, int widx, int *lpflag, int ch, Float *coef[Chans]);
/* void		main(int argc, char *argv[]); */
void*		mal1(long size);
void*		mal2(long size);
int		max_dep_cc(int nch);
int		max_indep_cc(int nch);
int		max_lfe_chn(int nch);
/*Float		monopred(Float lx, PRED_STATUS *psp);
void		predict(Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *prev_quant,Float *coef);
void		predict_reset(Info* info, int *prstflag, PRED_STATUS **psp, Float **prev_quant,int firstCh, int lastCh);
void		predinit(void); */
/* void		print_tns( TNSinfo *tns_info);*/
void		reset_mc_info(MC_Info *mip);
/*void		reset_pred_state(PRED_STATUS *psp);*/
void		restarttio(void);
int		startblock(void);
void		synt(Info *info, byte *group, byte *mask, Float *right, Float *left);
void		tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order );
void		tns_decode_coef( int order, int coef_res, short *coef, Float *a );
/* void		tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong, TNSinfo *tns_info);*/
int		tns_max_bands(int win);
int		tns_max_order(void);
void		unpack_idx(int *qp, int idx, Hcb *hcb);
void		usage(char *s);

#endif	/* _port_h_ */
