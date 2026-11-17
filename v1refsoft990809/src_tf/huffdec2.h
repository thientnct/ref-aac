/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

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
#ifndef _HUFFDEC2_H_
#define _HUFFDEC2_H_

#include "resilienceHandle.h"    /* handler */
#include "buffersHandle.h"       /* handler */
#include "tf_mainHandle.h"       /* defines, enums */
#include "tf_mainStruct.h"       /* typedefs */

void deinterleave ( void              *inptr,  /* formerly pointer to type int */
                    void              *outptr, /* formerly pointer to type int */
                    short             length,  /* sizeof base type of inptr and outptr in chars */
                    short             ngroups,
                    short             nsubgroups[], 
                    int               ncells[], 
                    short             cellsize[],
                    int               islong,
                    HANDLE_RESILIENCE hResilience );

void          clr_tns ( Info*           info, 
                        TNS_frame_info* tns_frame_info );

void          getgroup ( Info*             info, 
                         unsigned char*    group,
                         HANDLE_RESILIENCE hResilience,
                         HANDLE_BUFFER     hVm, 
                         HANDLE_EP_INFO    hEPInfo  );

int getics ( Info*                    info, 
             int                      common_window, 
             WINDOW_SEQUENCE*         win, 
             WINDOW_SHAPE*            wshape, 
             unsigned char*           group, 
             unsigned char*           max_sfb, 
             PRED_TYPE                pred_type, 
             int*                     lpflag, 
             int*                     prstflag, 
             unsigned char*           cb_map, 
             Float*                   coef, 
             short*                   global_gain, 
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp, 
             TNS_frame_info*          tns,
             BsBitStream*             gc_streamCh, 
             enum AAC_BIT_STREAM_TYPE bitStreamType,
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hVm,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_EP_INFO           hEPInfo,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT qc_select);


int           getmask ( Info*             info, 
                        unsigned char*    group, 
                        unsigned char     max_sfb, 
                        unsigned char*    mask,
                        HANDLE_RESILIENCE hResilience,
                        HANDLE_BUFFER     hVm, 
                        HANDLE_EP_INFO    hEPInfo  );

unsigned char  HuffSpecKernelPure ( int*               qp, 
                                    Hcb*               hcb, 
                                    Huffman*           hcw, 
                                    int                step,
                                    HANDLE_BUFFER      hSpecData,
                                    HANDLE_RESILIENCE  hResilience,
                                    HANDLE_HCR         hHcrInfo,
                                    HANDLE_EP_INFO     hEpInfo,
                                    HANDLE_CONCEALMENT hConcealment );


#endif  /* #ifndef _HUFFDEC2_H_ */
