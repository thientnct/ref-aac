/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporaion),
Mitsuyuki Hatanaka (Sony Corporation),
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

#ifndef _port_h_
#define _port_h_

#include "bitstreamHandle.h"     /* handler */

void            byteclr ( byte* ip1, 
                          int   cnt);

int             ch_index ( MC_Info* mip, 
                           int      cpe, 
                           int      tag );

void            check_mc_info ( MC_Info*          mip, 
                                int               new_config,
                                HANDLE_EP_INFO    hEpInfo,
                                HANDLE_RESILIENCE hResilience );

int             chn_config ( int      id, 
                             int      tag, 
                             int      common_window, 
                             MC_Info* mip);

int             endblock ( void );

int             enter_chn (int      cpe, 
                           int      tag, 
                           char     position, 
                           int      common_window, 
                           MC_Info* mip);

int             enter_mc_info ( MC_Info*          mip, 
                                ProgConfig*       pcp,
                                int               block_size_samples,
                                HANDLE_EP_INFO    hEpInfo,
                                HANDLE_RESILIENCE hResilience );

long            f2ir ( float x );

void            fltcpy ( Float* dp1,
                         Float* dp2,
                         int    cnt );

void            fltclr ( Float* dp1, 
                         int    cnt );

void            fltset ( Float* dp1, 
                         Float  dval, 
                         int cnt );

void            fmtchan ( char*  p, 
                          float* data, 
                          int    stride );

int             get_adif_header( int               block_size_samples,
                                 HANDLE_RESILIENCE hResilience,
                                 HANDLE_BUFFER     hVm, 
                                 HANDLE_EP_INFO    hEPInfo );

void            get_ics_info ( WINDOW_SEQUENCE*             win, 
                               WINDOW_SHAPE*            wshape, 
                               byte*                    group,
                               byte*                    max_sfb, 
                               PRED_TYPE                pred_type, 
                               int*                     lpflag, 
                               int*                     prstflag, 
                               enum AAC_BIT_STREAM_TYPE bitStreamType, 
                               HANDLE_RESILIENCE        hResilience,
                               HANDLE_BUFFER            hVm, 
                               HANDLE_EP_INFO           hEPInfo,
                               NOK_LT_PRED_STATUS*      nok_ltp_left,
			       NOK_LT_PRED_STATUS*      nok_ltp_right,
			       int                      stereoFlag,
                               QC_MOD_SELECT            qc_select);

int             get_prog_config ( ProgConfig*       p,
                                  int               block_size_samples,
                                  HANDLE_RESILIENCE hResilience,
                                  HANDLE_BUFFER     hVm, 
                                  HANDLE_EP_INFO    hEPInfo );


int             getbyte ( void) ;

#if (CChans > 0)
int             getcc ( MC_Info*      mip, 
                        byte*         cc_wnd, 
                        Wnd_Shape*    cc_wnd_shape, 
                        Float**       cc_coef,
                        Float*        cc_gain[CChans][Chans],
                        HANDLE_BUFFER hVm,
                        HANDLE_BUFFER hHcrSpecData,
                        HANDLE_HCR    hHcrInfo,
                        HANDLE_EP_INFO hEPInfo );
#endif

long            getshort ( void );

void            huffbookinit( int );

int             huffcmp( const void *va, 
                         const void *vb);

int             huffdecode ( int                      id, 
                             MC_Info*                 mip, 
                             WINDOW_SEQUENCE*             win, 
                             Wnd_Shape*               wshape, 
                             byte**                   cb_map, 
                             short**                  factors, 
                             byte**                   group, 
                             byte*                    hasmask, 
                             byte**                   mask, 
                             byte*                    max_sfb, 
                             PRED_TYPE                pred_type, 
                             int**                    lpflag, 
                             int**                    prstflag,
                             NOK_LT_PRED_STATUS**     nok_ltp_status, 
                             TNS_frame_info**         tns, 
                             BsBitStream*             gc_stream[], 
                             Float**                  coef, 
                             enum AAC_BIT_STREAM_TYPE bitStreamType,
                             int                      common_window,
                             Info*                    sfbInfo,
                             HANDLE_RESILIENCE        hResilience,
                             HANDLE_BUFFER            hVm,
                             HANDLE_BUFFER            hHcrSpecData,
                             HANDLE_HCR               hHcrInfo,
                             HANDLE_EP_INFO           hEPInfo,
                             HANDLE_CONCEALMENT       hConcealment,
                             QC_MOD_SELECT            qc_select);

int             hufffac ( Info*              info, 
                          byte*              group, 
                          int                nsect, 
                          byte*              sect,
                          short              global_gain, 
                          short*             factors,
                          HANDLE_BUFFER      hVm,
                          HANDLE_RESILIENCE  hResilience,
                          HANDLE_EP_INFO     hEpInfo );

void            infoinit (SR_Info* sip,
                          int );

void            init ( int,
                       Info*** );

void            init_cc ( int use_monopred );

void            init_pred_stat ( PRED_STATUS* psp, 
                                 int          grad, 
                                 float        alpha, 
                                 float        a, 
                                 float        b );

void            intclr ( int* ip1, 
                         int  cnt);

void            intcpy ( int* ip1, 
                         int* ip2, 
                         int  cnt);

void            intensity ( MC_Info* mip, 
                            Info*    info, 
                            int      ch, 
                            byte*    group, 
                            byte*    cb_map, 
                            short*   factors, 
                            int*     lpflag, 
                            Float*   coef[Chans] );

void            main ( int argc, 
                       char *argv[]);

void*           mal1 ( long size );

void*           mal2 ( long size );

void            map_mask ( Info* info, 
                           byte* group, 
                           byte* mask, 
                           byte* cb_map );

int             pred_max_bands ( void );

void            nok_init_pred_stat ( NOK_PRED_STATUS* psp );

void            nok_predict ( Info*            info, 
                              int              profile, 
                              int*             lpflag, 
                              NOK_PRED_STATUS* psp, 
                              Float*           prev_quant,
                              Float*           coef );

void            nok_predict_reset ( Info*             info, 
                                    int*              prstflag, 
                                    NOK_PRED_STATUS** psp,
                                    int               firstCh, 
                                    int               lastCh );

void            nok_reset_pred_state ( NOK_PRED_STATUS* psp );

void            predict( Info*              info,
                         int*               lpflag,
                         PRED_STATUS*       psp,
                         Float*             coef,
                         HANDLE_CONCEALMENT hConcealment);

void            predict_reset ( Info*         info, 
                                int*          prstflag, 
                                PRED_STATUS** psp,
                                int           firstCh, 
                                int           lastCh );

void            predinit ( void );

void            print_tns ( TNSinfo* tns_info );

void            reset_mc_info ( MC_Info *mip );

void            reset_pred_state ( PRED_STATUS *psp );

void            restarttio ( void );

void            shortclr ( short* ip1, 
                           int cnt );

int             startblock ( void );

void            synt ( Info*  info, 
                       byte*  group, 
                       byte*  mask, 
                       Float* right, 
                       Float* left );

void            tns_ar_filter ( Float* spec, 
                                int    size, 
                                int    inc, 
                                Float* lpc, 
                                int    order );

void            tns_decode_coef ( int    order, 
                                  int    coef_res, 
                                  short* coef, 
                                  Float* a );

void            tns_decode_subblock ( Float*   spec, 
                                      int      nbands, 
                                      short*   sfb_top, 
                                      int      islong, 
                                      TNSinfo* tns_info,
                                      QC_MOD_SELECT qc_select );

int             tns_max_bands ( int win, QC_MOD_SELECT qc_select );

int             tns_max_order ( int islong );

void            usage ( char* s );

void            pns ( MC_Info* mip, 
                      Info*    info, 
                      int      ch, 
                      byte*    group, 
                      byte*    cb_map, 
                      short*   factors, 
                      int*     lpflag, 
                      Float*   coef[Chans] );

void            predict_pns_reset ( Info*        info, 
                                    PRED_STATUS* psp, 
                                    byte*        cb_map );



#endif  /* _port_h_ */
