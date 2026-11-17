#ifndef _nttNew_h
#define  _nttNew_h


typedef struct{
   double *lsp_code_scl;
   double *lsp_fgcode_scl;
   double *ntt_fwcodev_scl;
   double   *ntt_codev0_scl;
   double   *ntt_codev1_scl;
   double   *ntt_codev0s_scl;
   double   *ntt_codev1s_scl;
   double   ac_top[8][2][4];
   double   ac_btm[8][2][4];

   int      ntt_NBITS_FR_SCL[8];
   int      ntt_VQTOOL_BITS_SCL[8];
   int      ntt_VQTOOL_BITS_S_SCL[8];
   int      ntt_NSclLay;
   int      ntt_NSclLayDec;
   int      *prev_fw_code[8];
   int      *prev_lsp_code[8];
   int      prev_w_type;
   int      ma_np;
   }ntt_DATA_scl;

typedef struct{
   double *lsp_code_base;
   double *lsp_fgcode_base;
   int    *ntt_crb_tbl;
   double *ntt_fwcodev;
   double  *ntt_codev0;
   double *ntt_codev1;
   double  *ntt_codev0s;
   double *ntt_codev1s;
   double *ntt_codevp0;
   short  *pleave0;
   short  *pleave1;
   double  bandUpper;
   double  bandLower;
   int      ntt_NBITS_FR;
   int      ntt_VQTOOL_BITS;
   int      ntt_VQTOOL_BITS_S;
   double   *cos_TT;
   int      *prev_fw_code;
   int      *prev_lsp_code;
   int      prev_w_type;
   int      ma_np;
}ntt_DATA_base;

typedef struct{
   ntt_DATA_scl* nttDataScl;
   ntt_DATA_base* nttDataBase;
}ntt_DATA;

#endif /* _nttNew_h */
