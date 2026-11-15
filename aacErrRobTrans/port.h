/*
  "This software module was originally developed by 

  AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS
  
  and edited by

  Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
  
  in the course of 
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
  Copyright(c)1996, 2000.
*/

#ifndef	_port_h_
#define	_port_h_

#include "all.h"

int		byte_align(void);
void	byteclr(byte *ip1, int cnt);
void    longclr ( unsigned long *ip1, int cnt );
int		ch_index(MC_Info *mip, int cpe, int tag);
void	check_mc_info(MC_Info *mip, int new_config);
int		chn_config(int id, int tag, int common_window, MC_Info *mip);
void	closefiles( MC_Info *mip );
void	clr_tns( Info *info, TNS_frame_info *tns_frame_info );
void	coupling(MC_Info *mip, Float **coef, Float **cc_coef, Float **cc_gain, int ch, int cc_dom, int cc_dep);
int		decode_huff_cw(Huffman *h, unsigned short spectralDataFlag, unsigned short *len);
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
void		get_ics_info(byte *win, byte *wshape, byte *group,byte *max_sfb, int *lpflag, int *prstflag, int common_window);
int		get_prog_config(ProgConfig *p);
void		get_sign_bits(int *q, int n);
int		get_tns( Info *info, TNS_frame_info *tns_frame_info );
long		getbits(int n);
int		getbyte(void);
#if (CChans > 0)
int		getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef,Float *cc_gain[CChans][Chans]);
#endif
int		getdata(int *tag, int *dt_cnt, byte *data_bytes);
int		getescape(int q);
void		getfill(void);
void		getgroup(Info *info, byte *group);
int		getics(Info *info, int common_window, byte *win, byte *wshape, byte *group, byte max_sfb, int *lpflag, int *prstflag,     int *pns, byte *sect, Float *coef, unsigned short *global_gain     , short *factors    , TNS_frame_info *tns    );
int		getmask(Info *info, byte *group, byte max_sfb, byte *mask);
long		getshort(void);
void		huffbookinit(void);
void		huffdecinit(void);
int		huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, byte **group, byte *hasmask, byte **mask, byte *max_sfb,     int **lpflag, int **prstflag,    TNS_frame_info **tns,    Float **coef);
int		hufffac(Info *info, byte *group, int nsect, byte *sect,unsigned short global_gain, short *factors);
void		hufftab ( Hcb*           hcb, 
                          Huffman*       hcw, 
                          int            dim, 
                          int            lav,
                          int            lavInclEsc,
                          int            signed_cb,
                          unsigned short maxCWLen );
void		infoinit(SR_Info *sip);
void		init(void);
void		init_cc(void);
void		init_pred_stat(PRED_STATUS *psp, int grad, float alpha, float a, float b);
void		initio(int argc, char *argv[]);
void		ind_coupling(MC_Info *mip, byte *wnd, Wnd_Shape *wnd_shape, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float *cc_coef);
void		intclr(int *ip1, int cnt);
void		intcpy(int *ip1, int *ip2, int cnt);
void		intensity(MC_Info *mip, Info *info, int *lpflag, int ch, Float *coef[Chans]);
int		main(int argc, char *argv[]);
void*		mal1(long size);
void*		mal2(long size);
void		myexit(const char *s);
void		predict(Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *coef);
void		predict_reset(Info* info, int *prstflag, PRED_STATUS **psp, int firstCh, int lastCh);
void		predinit(void);
void		print_tns( TNSinfo *tns_info);
void		reset_mc_info(MC_Info *mip);
void		reset_pred_state(PRED_STATUS *psp);
void		restarttio(void);
int		startblock(void);
void		synt(Info *info, byte *group, byte *mask, Float *right, Float *left);
void		tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order );
void		tns_decode_coef( int order, int coef_res, short *coef, Float *a );
void		tns_decode_subblock(Float *spec, short *sfb_top, int islong, TNSinfo *tns_info);
int		tns_max_bands(int win);
int		tns_max_order(int islong);
int		pred_max_bands(void);
void		unpack_idx(int *qp, int idx, Hcb *hcb);
void		usage(char *s);
#ifdef PCM_OUTPUT
void		writeout(Float **data, MC_Info *mip);
#endif /*PCM_OUTPUT*/
void            ResetBitCount ( void );
unsigned long   GetBitCnt( void );
unsigned long   getbitstreamword( unsigned long *wordext );
void            clearbitstreamword( void );
void            ReadParameterUEP(void);

#endif	/* _port_h_ */
