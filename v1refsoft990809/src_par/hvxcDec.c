
/*

This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

    and edited by

    Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.),
    Akira Inoue (Sony Corporation) and
    Yuji Maeda (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

#include "pan_par_const.h"


static void IPC_makeCoefDec(
float *coef)
{
    int i;
    float energy;
    float c4;

    energy = 0.;
    for(i=0; i < SAMPLE ; i++){
        coef[i]=(0.54  - 0.46 * cos( 2*M_PI*i/(SAMPLE-1)));
        energy=energy + coef[i] * coef[i] ;
    }
    for(i=0; i < SAMPLE ; i++)
        coef[i]=coef[i]/sqrt(energy);

    c4 = 0.;
    for(i=0; i < SAMPLE ; i++)
        c4 = c4 + pow(coef[i], 4.);
}

static void IPC_makeCoefDecLD(
float *coef)
{
      int i;
      float energy;

      for(i=0; i < SAMPLE ; i++)
            coef[i]=(0.54  - 0.46 * cos( 2*M_PI*i/(SAMPLE-1)));
      for(i=0;i<8;i++)
            coef[i]=0.;
      for(i=SAMPLE-8;i<SAMPLE;i++)
            coef[i]=0.;

      energy = 0.;
      for(i=0; i < SAMPLE ; i++)
            energy=energy + coef[i] * coef[i] ;

      for(i=0; i < SAMPLE ; i++)
            coef[i]=coef[i]/sqrt(energy);

}

/* Init HVXC decoder */
/* return a pointer to decoder status handler(AI 990129) */

HvxcDecStatus *hvxc_decode_init(
int varMode,		/* in: HVXC variable rate mode */
int rateMode,		/* in: HVXC bit rate mode */
int extensionFlag,	/* in: the presense of verion 2 data */
int delayMode,		/* in: decoder delay mode */
int testMode		/* in: test_mode for decoder conformance testing */
)
{
  int i, j;
  HvxcDecStatus *HDS;

  if ((HDS = (HvxcDecStatus*)malloc(sizeof(HvxcDecStatus)))==NULL)
    printf("hvxc_decode_init: memory allocation error");

  /* HVXC decoder configuration */
  HDS->varMode = varMode;
  HDS->rateMode = rateMode;
  HDS->extensionFlag = extensionFlag;

  if (delayMode)
    HDS->decDelayMode = DM_LONG;
  else
    HDS->decDelayMode = DM_SHORT;

  HDS->testMode = testMode;    
  
  IPC_makeCoefDec(HDS->ipc_coef);
  IPC_makeCoefDecLD(HDS->ipc_coefLD);
  IPC_set_const_lpcVM_dec();
  IPC_make_f_coef_dec();
  IPC_make_c_dis();
  IPC_make_w_celp();

  /* initialization of decoder status */
  /* pan_lspdec() in pan_lspdec.c(for decMode == DEC0K) */
  HDS->flagLspPred = 1;

  /* DecParFrameHvx() in dec_par.c */
  HDS->nbs = 0;
  HDS->fr0 = 0;
  HDS->targetP = 0.0f;

  HDS->idVUV2[0] = HDS->idVUV2[1] = 0;
  HDS->bgnFlag2[0] = HDS->bgnFlag2[1] = 0;
  HDS->pch2[0] = HDS->pch2[1] = 0.0f;
	
  for (i = 0; i < 4; i++) {
    for (j = 0; j < P; j++) {
      HDS->qLspQueue[i][j] = 0.5f/11.0f*(j+1);
    }
  }

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      HDS->idCelp2[i].idSL0[j] = 0;
      HDS->idCelp2[i].idGL0[j] = 0;
    }
    for (j = 0; j < 4; j++) {
      HDS->idCelp2[i].idSL1[j] = 0;
      HDS->idCelp2[i].idGL1[j] = 0;
    }
  }
    
  /* Bit2PrmVR() in hvxcDec.c */
  HDS->prevVUV = 0;
    
  /* IPC_DecParams1st(), IPC_DecParams1stVR() in hvxcDec.c */
  HDS->dec_frm = 0;
  for (i = 0; i < LPCORDER; i++) {
    HDS->prevQLsp[i] = (float)(i+1)/(float)(LPCORDER+1);
    HDS->prevQLsp2[i] = (float)(i+1)/(float)(LPCORDER+1);
  }
  HDS->bgnCnt = 0;
  HDS->prevGainId = 0;
    
  /* IPC_SynthSC() in hvxcDec.c */
  HDS->syn_frm = 0;
  HDS->muteflag = 0;
    
  /* harm_sew_dec() in hvxcQAmDec.c */
  HDS->feneqold = 0.0f;
    
  /* IPC_vExt_fft() in hvxcVExtGenDec.c */
  HDS->old_old_vuv = 0;
  for (i = 0; i < SAMPLE; i++) {
    HDS->wave2[i] = 0.0f;
    HDS->pha2[i] = 0.0f;
  }
    
  /* IPC_UvAdd() in hvxcVExtGenDec.c */
  for (i = 0; i < 2*FRM; i++) {
    HDS->wnsozp[i] = 0.0f;
  }
    
  /* IPC_uvExt() in hvxcUVExtGenDec.c */
  for (i = 0; i < FRM/2; i++) {
    HDS->old_qRes[i] = 0.0f;
  }
    
  /* calc_syncont2v() in hvxcSynVDec.c */
  for (i = 0; i < P+1; i++) {
    HDS->lpv_mem2[i] = 0.0f;
  }
  /* calc_syncont2u(), calc_syncont2u_LD() in hvxcSynUVDec.c */
  for (i = 0; i < P+1; i++) {
    HDS->lpu_mem2[i] = 0.0f;
  }
    
  /* IPC_posfil_v() in hvxcFltDec.c */
  for (i = 0; i < P+1; i++) {
    HDS->pfv_mem1[i] = 0.0f;
    HDS->pfv_mem2[i] = 0.0f;
    HDS->pfv_alphaipFIRold[i] = 0.0f;
    HDS->pfv_alphaipIIRold[i] = 0.0f;
  }
  HDS->pfv_hpm = 0.0f;
  HDS->pfv_oldgain = 0.0f;

  /* IPC_posfil_u(), IPC_posfil_u_LD() in hvxcFltDec.c */
  for (i = 0; i < P+1; i++) {
    HDS->pfu_mem1[i] = 0.0f;
    HDS->pfu_mem2[i] = 0.0f;
    HDS->pfu_alphaipFIRold[i] = 0.0f;
    HDS->pfu_alphaipIIRold[i] = 0.0f;
  }
  HDS->pfu_hpm = 0.0f;
  HDS->pfu_oldgain = 0.0f;
    
  for (i = 0; i < 4; i++) {
    HDS->hpf_posfil_uv_state[i] = 0.0f;
  }

  /* IPC_bpf() in hvxcFltDec.c */
  for (i = 0; i < 4; i++) {
    HDS->hpf_dec_state[i] = 0.0f;
    HDS->lpf3400_state[i] = 0.0f;
  }
  for (i = 0; i < 8; i++) {
    HDS->hpf4ji_state[i] = 0.0;
  }

  /* initialize lpf3400() coefficients */
  HDS->lpf3400_coef[0] = -2. * 1. * cos((4.0/4.0)*M_PI);
  HDS->lpf3400_coef[1] = 1.;
  HDS->lpf3400_coef[2] = -2. * 0.78 * cos((3.55/4.0)*M_PI);
  HDS->lpf3400_coef[3] = 0.78*0.78;
  HDS->lpf3400_coef[4] = 0.768;		/* gain */

  return HDS;
}


static void UnpackBitsChar(char *encBitChar, int *ptr, int *param, int nbit)
{
    int     i;
    
    *param = 0;

    for(i = 0; i < nbit; i++)
    {
	if(encBitChar[*ptr] == '1')
	{
	    *param |= 0x01 << (nbit - i - 1);
	}
	else
	{
	    *param |= 0x00;
	}
	(*ptr)++;
    }
}

static void Bit2Prm(
unsigned char	*encBit,
EncPrm		*encPrm,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    char	encBitChar[80];
    int		i, j;
    int		ptr = 0;

    for(i = 0; i < 5; i++)
    {
	for(j = 0; j < 8; j++)
	{
	    if(encBit[i] & (0x80 >> j))
	    {
		encBitChar[i * 8 + j] = '1';
	    }
	    else
	    {
		encBitChar[i * 8 + j] = '0';
	    }
	}
    }

    UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[0], 
        PAN_BIT_LSP18_0);
    UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[1], 
        PAN_BIT_LSP18_1);
    UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[2], 
        PAN_BIT_LSP18_2);
    UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[3], 
        PAN_BIT_LSP18_3);
    UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[4], 
        PAN_BIT_LSP18_4);

    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvFlag, 2);

    if(encPrm->vuvFlag == 0)
    {
	for(i = 0; i < N_SFRM_L0; i++)
	{
	    UnpackBitsChar(encBitChar, &ptr,
			   &encPrm->vuvPrm.idUV.idSL0[i], 6);
	    UnpackBitsChar(encBitChar, &ptr,
			   &encPrm->vuvPrm.idUV.idGL0[i], 4);
	}
    }
    else
    {
	UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.pchcode, 7);
	UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idS0, 4);
	UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idS1, 4);
	UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idG, 5);
    }
    

    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	for(i = 5; i < 10; i++)
	{
	    for(j = 0; j < 8; j++)
	    {
		if(encBit[i] & (0x80 >> j))
		{
		    encBitChar[i * 8 + j] = '1';
		}
		else
		{
		    encBitChar[i * 8 + j] = '0';
		}
	    }
	}
	
	UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[5], 8);
	
	if(encPrm->vuvFlag == 0)
	{
	    for(i = 0; i < N_SFRM_L1; i++)
	    {
		UnpackBitsChar(encBitChar, &ptr,
			       &encPrm->vuvPrm.idUV.idSL1[i], 5);
		UnpackBitsChar(encBitChar, &ptr,
			       &encPrm->vuvPrm.idUV.idGL1[i], 3);
	    }
	}
	else
	{
	    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS0, 7);
	    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS1, 10);
	    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS2, 9);
	    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS3, 6);
	}
    }
}

static void Bit2PrmVR(
unsigned char   *encBit,
EncPrm          *encPrm,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    char        encBitChar[80]; /* for 4kbps one frame */
    int         i, j;
    int         ptr = 0;

    for(i = 0; i < 5; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(encBit[i] & (0x80 >> j))
            {
                encBitChar[i * 8 + j] = '1';
            }
            else
            {
                encBitChar[i * 8 + j] = '0';
            }
        }
    }

    UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvFlag, 2);

    if(encPrm->vuvFlag != 1)
    {
        UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[0],
                       PAN_BIT_LSP18_0);
        UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[1],
                       PAN_BIT_LSP18_1);
        UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[2],
                       PAN_BIT_LSP18_2);
        UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[3],
                       PAN_BIT_LSP18_3);
        UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[4],
                       PAN_BIT_LSP18_4);

        if(encPrm->vuvFlag == 0)
        {
            for(i = 0; i < N_SFRM_L0; i++)
            {
                UnpackBitsChar(encBitChar, &ptr,
                               &encPrm->vuvPrm.idUV.idGL0[i], 4);
            }
        }
        else
        {
            UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.pchcode, 7);
            UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idS0, 4);
            UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idS1, 4);
            UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.idG, 5);
        }

        if(HDS->decMode == DEC4K)
        {
            for(i = 5; i < 10; i++)
            {
                for(j = 0; j < 8; j++)
                {
                    if(encBit[i] & (0x80 >> j))
                    {
                        encBitChar[i * 8 + j] = '1';
                    }
                    else
                    {
                        encBitChar[i * 8 + j] = '0';
                    }
                }
            }

            UnpackBitsChar(encBitChar, &ptr, &encPrm->idLsp.nVq[5], 8);

            if(encPrm->vuvFlag == 0)
            {
                for(i = 0; i < N_SFRM_L1; i++)
                {
                    UnpackBitsChar(encBitChar, &ptr,
                                   &encPrm->vuvPrm.idUV.idSL1[i], 5);
                    UnpackBitsChar(encBitChar, &ptr,
                                   &encPrm->vuvPrm.idUV.idGL1[i], 3);
                }
            }
            else
            {
                UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS0, 7);
                UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS1, 10);
                UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS2, 9);
                UnpackBitsChar(encBitChar, &ptr, &encPrm->vuvPrm.idV.id4kS3, 6);
            }
        }
    }

    if(encPrm->vuvFlag == 1)
    {

        encPrm->vuvFlag = HDS->prevVUV;

            encPrm->bgnFlag = 1;
    }
    else
    {
        HDS->prevVUV = encPrm->vuvFlag;
        encPrm->bgnFlag = 0;
    }
}





static void IPC_UnPackBit2Prm(
unsigned char	*encBit,
IdLsp		*idLsp,
int		*idVUV,
IdCelp		*idCelp,
float		*pitch,
IdAm		*idAm,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int		i, pchcode;

        Bit2Prm(encBit, &(HDS->encPrm), HDS);
    
    for(i = 0; i < L_VQ - 1; i++)
    {
	idLsp->nVq[i] = HDS->encPrm.idLsp.nVq[i];
    }
    
    *idVUV = HDS->encPrm.vuvFlag;
    
    if(HDS->encPrm.vuvFlag == 0)
    {
	*pitch = 148.0;

	for(i = 0; i < N_SFRM_L0; i++)
	{
	    {
	        idCelp->idSL0[i] = HDS->encPrm.vuvPrm.idUV.idSL0[i];
	        idCelp->idGL0[i] = HDS->encPrm.vuvPrm.idUV.idGL0[i];
            }
	}
    }
    else
    {
	pchcode = HDS->encPrm.vuvPrm.idV.pchcode;
	*pitch = pchcode + 20.0;
	idAm->idS0 = HDS->encPrm.vuvPrm.idV.idS0;
	idAm->idS1 = HDS->encPrm.vuvPrm.idV.idS1;
	idAm->idG = HDS->encPrm.vuvPrm.idV.idG;
    }
    
    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	idLsp->nVq[L_VQ - 1] = HDS->encPrm.idLsp.nVq[L_VQ - 1];
	    
	if(HDS->encPrm.vuvFlag == 0)
	{
	    for(i = 0; i < N_SFRM_L1; i++)
	    {
		{
		    idCelp->idSL1[i] = HDS->encPrm.vuvPrm.idUV.idSL1[i];
		    idCelp->idGL1[i] = HDS->encPrm.vuvPrm.idUV.idGL1[i];
                }
	    }
	}
	else
	{
	    idAm->id4kS0 = HDS->encPrm.vuvPrm.idV.id4kS0;
	    idAm->id4kS1 = HDS->encPrm.vuvPrm.idV.id4kS1;
	    idAm->id4kS2 = HDS->encPrm.vuvPrm.idV.id4kS2;
	    idAm->id4kS3 = HDS->encPrm.vuvPrm.idV.id4kS3;
	}
    }

}

static void IPC_UnPackBit2PrmVR(
unsigned char   *encBit,
IdLsp           *idLsp,
int             *idVUV,
int             *bgnFlag,
IdCelp          *idCelp,
float           *pitch,
IdAm            *idAm,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int                 i, pchcode;

    Bit2PrmVR(encBit, &(HDS->encPrm), HDS);

    *bgnFlag = HDS->encPrm.bgnFlag;
    *idVUV = HDS->encPrm.vuvFlag;

    for(i = 0; i < L_VQ - 1; i++)
    {
        idLsp->nVq[i] = HDS->encPrm.idLsp.nVq[i];
    }

    if(HDS->encPrm.vuvFlag == 0)
    {
        *pitch = 148.0;

        for(i = 0; i < N_SFRM_L0; i++)
        {
            idCelp->idSL0[i] = HDS->encPrm.vuvPrm.idUV.idSL0[i];
            idCelp->idGL0[i] = HDS->encPrm.vuvPrm.idUV.idGL0[i];
        }
    }
    else
    {
        pchcode = HDS->encPrm.vuvPrm.idV.pchcode;
        *pitch = pchcode + 20.0;
        idAm->idS0 = HDS->encPrm.vuvPrm.idV.idS0;
        idAm->idS1 = HDS->encPrm.vuvPrm.idV.idS1;
        idAm->idG = HDS->encPrm.vuvPrm.idV.idG;
    }

    if(HDS->decMode == DEC4K)
    {
        idLsp->nVq[L_VQ - 1] = HDS->encPrm.idLsp.nVq[L_VQ - 1];

        if(HDS->encPrm.vuvFlag == 0)
        {
            for(i = 0; i < N_SFRM_L1; i++)
            {
                idCelp->idSL1[i] = HDS->encPrm.vuvPrm.idUV.idSL1[i];
                idCelp->idGL1[i] = HDS->encPrm.vuvPrm.idUV.idGL1[i];
            }
        }
        else
        {
            idAm->id4kS0 = HDS->encPrm.vuvPrm.idV.id4kS0;
            idAm->id4kS1 = HDS->encPrm.vuvPrm.idV.id4kS1;
            idAm->id4kS2 = HDS->encPrm.vuvPrm.idV.id4kS2;
            idAm->id4kS3 = HDS->encPrm.vuvPrm.idV.id4kS3;
        }
    }
}

#define LONG64	/* for 64bit platform like IRIX64@R10000(AI 990219) */

int IPC_DecParams1st(
unsigned char	*encBit,
float		pchmod,
int		*idVUV2,
float		(*qLspQueue)[LPCORDER],
float		*pch2,
float		(*am2)[SAMPLE/2][3],
IdCelp		*idCelp2,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    IdAm 	idAm;
    float	mfdpch;
    float	tmpQLsp[LPCORDER];
    int		i, j, k;
    int		idAmS[2], idAmG;
    int		idAm4k[4];
    float	tmpAm[SAMPLE/2][3];
    float       *dumLSF = NULL;

    IdLsp	idLsp;
    int		idVUV;
    int		normMode, bitNum;


#include "inc_lsp_575.tbl"
    float min_gap=PAN_MINGAP_PAR;
    float p_factor=PAN_LSP_AR_R_PAR;

#ifdef LONG64
    long idx[L_VQ];
#endif
    if(0==HDS->dec_frm) {
        for(j=0;j<LPCORDER;j++) {
            HDS->prevQLsp[j] = (float)(j+1)/(float)(LPCORDER+1);
        }
    }

    IPC_UnPackBit2Prm(encBit, &idLsp, &idVUV, &idCelp2[HDS->dec_frm % 2], &mfdpch,
		      &idAm, HDS);


#ifdef LONG64
    for (i = 0; i < L_VQ; i++) idx[i] = (long)idLsp.nVq[i];
#endif

    pan_lspdec(HDS->prevQLsp, tmpQLsp, 
               p_factor, min_gap, LPCORDER, 
#ifdef LONG64
	       idx,
#else
               (long *)idLsp.nVq, 
#endif
	       lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 1, HDS->flagLspPred);


    if (HDS->decMode == DEC0K)
      HDS->flagLspPred = 0;
    else
      HDS->flagLspPred = 1;
    
/* 98.1.16 */
    for(j=0;j<LPCORDER;j++) HDS->prevQLsp[j] = tmpQLsp[j];

    if(DEC4K==HDS->decMode || DEC3K==HDS->decMode)
    {
	IPC_DecLspEnh(&idLsp, tmpQLsp);
    }

    for(j=0;j<LPCORDER;j++) tmpQLsp[j] *= .5;
    
    for(j = 0; j < P; j++)
    {
	qLspQueue[(HDS->dec_frm + 2) % 4][j] = tmpQLsp[j];
    }

    idVUV2[0] = idVUV2[1];
    idVUV2[1] = idVUV;
    
    pch2[0] = pch2[1];
    pch2[1] = mfdpch;
    
    normMode = 0;
    bitNum = 10;
    
    idAmS[0]=idAm.idS0;
    idAmS[1]=idAm.idS1;
    idAmG=idAm.idG;
    
    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	idAm4k[0]=idAm.id4kS0;
	idAm4k[1]=idAm.id4kS1;
	idAm4k[2]=idAm.id4kS2;
	idAm4k[3]=idAm.id4kS3;
    }

    harm_sew_dec(tmpAm, &pch2[1], dumLSF, normMode, idVUV2[1], idAmS, bitNum,
		 idAmG, 0, pchmod, idAm4k, HDS);
    
    for(j = 0; j < SAMPLE / 2; j++)
    {
	for(k = 0; k < 3; k++)
	{
	    am2[0][j][k] = am2[1][j][k];
	}
    }

    for(j = 0; j < SAMPLE / 2; j++)
    {
	for(k = 0; k < 3; k++)
	{
	    am2[1][j][k] = tmpAm[j][k];
	}
    }
    
    HDS->dec_frm++;

    return(0);
}


int IPC_DecParams1stVR(
unsigned char   *encBit,
float           pchmod,
int             *idVUV2,
int             *bgnFlag2,
float           (*qLspQueue)[LPCORDER],
float           *pch2,
float           (*am2)[SAMPLE/2][3],
IdCelp          *idCelp2,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    IdAm        idAm;

    float       mfdpch;
    float       tmpQLsp[LPCORDER];
    int         i, j, k;
    int         idAmS[2], idAmG;
    int         idAm4k[4];
    float       tmpAm[SAMPLE/2][3];
    float       *dumLSF = NULL;

    IdLsp       idLsp;
    int         idVUV;
    int         bgnFlag;
    int         normMode, bitNum;

#include "inc_lsp_575.tbl"
    float min_gap=PAN_MINGAP_PAR;
    float p_factor=PAN_LSP_AR_R_PAR;

#ifdef LONG64
    long idx[L_VQ];
#endif

    IPC_UnPackBit2PrmVR(encBit, &idLsp, &idVUV, &bgnFlag, &idCelp2[HDS->dec_frm % 2],
                      &mfdpch, &idAm, HDS);
#ifdef LONG64
    for (i = 0; i < L_VQ; i++) idx[i] = (long)idLsp.nVq[i];
#endif
    pan_lspdec(HDS->prevQLsp, tmpQLsp, 
               p_factor, min_gap, LPCORDER, 
#ifdef LONG64
	       idx,
#else
               (long *)idLsp.nVq, 
#endif
               lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 1, HDS->flagLspPred);

    if (HDS->decMode == DEC0K)
      HDS->flagLspPred = 0;
    else
      HDS->flagLspPred = 1;

    if(DEC4K==HDS->decMode)
    {
        IPC_DecLspEnh(&idLsp, tmpQLsp);
    }

    {
        if(bgnFlag)
        {
            for(i = 0; i < LPCORDER; i++)
            {
                tmpQLsp[i] = (HDS->prevQLsp2[i] * (2.0 * BGN_INTVL - 1.0 - 2.0 * HDS->bgnCnt)
                    + HDS->prevQLsp[i] * (1.0 + 2.0 * HDS->bgnCnt)) /
                        (BGN_INTVL * 2.0);
            }
            HDS->bgnCnt++;
        }
        else
        {
            for(i = 0; i < LPCORDER; i++)
            {
                HDS->prevQLsp2[i] = HDS->prevQLsp[i];
            }

            for(j = 0; j < LPCORDER; j++)
            {
                HDS->prevQLsp[j] = tmpQLsp[j];
            }

            /* check gain of excitations */

            if(HDS->bgnCnt == BGN_INTVL)
            {
/*
            printf("gain %d\n", idCelp2[HDS->dec_frm % 2].idGL0[0]);
            printf("     %d\n", idCelp2[HDS->dec_frm % 2].idGL0[1]);
*/
                if(idCelp2[HDS->dec_frm % 2].idGL0[0] <= HDS->prevGainId + 2)
                {
                    for(j = 0; j < LPCORDER; j++)
                    {
                        tmpQLsp[j] = HDS->prevQLsp2[j];
                    }
                }
            }

            HDS->bgnCnt = 0;
        }

        HDS->prevGainId = idCelp2[HDS->dec_frm % 2].idGL0[1];
    }


    for(j=0;j<LPCORDER;j++) tmpQLsp[j] *= .5;

    for(j = 0; j < P; j++)
    {
        qLspQueue[(HDS->dec_frm + 2) % 4][j] = tmpQLsp[j];
    }

    idVUV2[0] = idVUV2[1];
    idVUV2[1] = idVUV;

    bgnFlag2[0] = bgnFlag2[1];
    bgnFlag2[1] = bgnFlag;

    pch2[0] = pch2[1];
    pch2[1] = mfdpch;

    normMode = 0;
    bitNum = 10;

    idAmS[0]=idAm.idS0;
    idAmS[1]=idAm.idS1;
    idAmG=idAm.idG;

    if(HDS->decMode == DEC4K)
    {
        idAm4k[0]=idAm.id4kS0;
        idAm4k[1]=idAm.id4kS1;
        idAm4k[2]=idAm.id4kS2;
        idAm4k[3]=idAm.id4kS3;
    }

    harm_sew_dec(tmpAm, &pch2[1], dumLSF, normMode, idVUV2[1], idAmS, bitNum,
                 idAmG, 0, pchmod, idAm4k, HDS);
    for(j = 0; j < SAMPLE / 2; j++)
    {
        for(k = 0; k < 3; k++)
        {
            am2[0][j][k] = am2[1][j][k];
        }
    }

    for(j = 0; j < SAMPLE / 2; j++)
    {
        for(k = 0; k < 3; k++)
        {
            am2[1][j][k] = tmpAm[j][k];
        }
    }

    HDS->dec_frm++;

    return(0);
}


void IPC_DecParams2nd(
int	fr0,
int	*idVUV2,
float	(*qLspQueue)[LPCORDER],
IdCelp	*idCelp2,
float	(*qLsp2)[LPCORDER],
float	(*qRes2)[FRM],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int		j;
    int		idCelpSL0[N_SFRM_L0], idCelpGL0[N_SFRM_L0];
    int		idCelpSL1[N_SFRM_L1], idCelpGL1[N_SFRM_L1];

    fr0 += 2;

    for(j = 0; j < P; j++)
    {
	qLsp2[0][j] = qLspQueue[(fr0 + 0) % 4][j];
	qLsp2[1][j] = qLspQueue[(fr0 + 1) % 4][j];
    }
    
    for(j = 0; j < N_SFRM_L0; j++)
    {
	idCelpSL0[j] = idCelp2[fr0 % 2].idSL0[j];
	idCelpGL0[j] = idCelp2[fr0 % 2].idGL0[j];
    }
    
    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	for(j = 0; j < N_SFRM_L1; j++)
	{
	    idCelpSL1[j] = idCelp2[fr0 % 2].idSL1[j];
	    idCelpGL1[j] = idCelp2[fr0 % 2].idGL1[j];
	}
    }
    
    td_decoder(idVUV2[0], idCelpSL0, idCelpGL0, idCelpSL1, idCelpGL1,
	       qRes2[0], HDS);
    
    for(j = 0; j < N_SFRM_L0; j++)
    {
	idCelpSL0[j] = idCelp2[(fr0 + 1) % 2].idSL0[j];
	idCelpGL0[j] = idCelp2[(fr0 + 1) % 2].idGL0[j];
    }
    
    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	for(j = 0; j < N_SFRM_L1; j++)
	{
	    idCelpSL1[j] = idCelp2[(fr0 + 1) % 2].idSL1[j];
	    idCelpGL1[j] = idCelp2[(fr0 + 1) % 2].idGL1[j];
	}
    }
    
    td_decoder(idVUV2[1], idCelpSL0, idCelpGL0, idCelpSL1, idCelpGL1,
	       qRes2[1], HDS);

}

void IPC_DecParams2ndVR(
int     fr0,
int     *idVUV2,
int     *bgnFlag2,
float   (*qLspQueue)[LPCORDER],
IdCelp  *idCelp2,
float   (*qLsp2)[LPCORDER],
float   (*qRes2)[FRM],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int         j;
    int         idCelpSL0[N_SFRM_L0], idCelpGL0[N_SFRM_L0];
    int         idCelpSL1[N_SFRM_L1], idCelpGL1[N_SFRM_L1];

    fr0 += 2;

    for(j = 0; j < P; j++)
    {
        qLsp2[0][j] = qLspQueue[(fr0 + 0) % 4][j];
        qLsp2[1][j] = qLspQueue[(fr0 + 1) % 4][j];
    }

    for(j = 0; j < N_SFRM_L0; j++)
    {
        idCelpSL0[j] = idCelp2[fr0 % 2].idSL0[j];
        idCelpGL0[j] = idCelp2[fr0 % 2].idGL0[j];
    }

    if(HDS->decMode == DEC4K)
    {
        for(j = 0; j < N_SFRM_L1; j++)
        {
            idCelpSL1[j] = idCelp2[fr0 % 2].idSL1[j];
            idCelpGL1[j] = idCelp2[fr0 % 2].idGL1[j];
        }
    }

    td_decoderVR(idVUV2[0], bgnFlag2[0], idCelpSL0, idCelpGL0, idCelpSL1, idCelpGL1,
                 qRes2[0], HDS);

    for(j = 0; j < N_SFRM_L0; j++)
    {
        idCelpSL0[j] = idCelp2[(fr0 + 1) % 2].idSL0[j];
        idCelpGL0[j] = idCelp2[(fr0 + 1) % 2].idGL0[j];
    }

    if(HDS->decMode == DEC4K)
    {
        for(j = 0; j < N_SFRM_L1; j++)
        {
            idCelpSL1[j] = idCelp2[(fr0 + 1) % 2].idSL1[j];
            idCelpGL1[j] = idCelp2[(fr0 + 1) % 2].idGL1[j];
        }
    }

    td_decoderVR(idVUV2[1], bgnFlag2[1], idCelpSL0, idCelpGL0, idCelpSL1, idCelpGL1,
                 qRes2[1], HDS);

}

void IPC_SynthSC(
int	idVUV,
float	*qLsp,
float	mfdpch,
float	(*am)[3],
float	*qRes,
short int	*frmBuf,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int		i;
    int		lsUn;
    float	synoutu[SAMPLE-OVERLAP]; 
    float	synoutv[SAMPLE-OVERLAP]; 
    float	bufsf[FRM];
    int tmpIdVUVs[2];

    if(HDS->syn_frm == 0)
    {
	for(i = 0; i < P; i++)
	{
	    HDS->qLsps[0][i] = HDS->qLsps[1][i] = qLsp[i];
	}

	HDS->idVUVs[0] = HDS->idVUVs[1] = idVUV;
	HDS->pchs[0]= HDS->pchs[1]= mfdpch;

	for(i = 0; i < FRM; i++)
	{
	    bufsf[i] = 0.0;
	}
    }
    else
    {
        for(i = 0; i < P; i++)
	{
	  HDS->qLsps[0][i] = HDS->qLsps[1][i];
	  HDS->qLsps[1][i] = qLsp[i];
        }
    
	HDS->idVUVs[0] = HDS->idVUVs[1];
	HDS->idVUVs[1] = idVUV;  
		
	HDS->pchs[0] = HDS->pchs[1];
	HDS->pchs[1] = mfdpch;  

	lsUn=8;

        if(HDS->varMode == BM_VARIABLE)
        {
            tmpIdVUVs[0] = HDS->idVUVs[0];
            tmpIdVUVs[1] = HDS->idVUVs[1];

            if(tmpIdVUVs[0] == 1)
            {
                tmpIdVUVs[0] = 0;
            }
            if(tmpIdVUVs[1] == 1)
            {
                tmpIdVUVs[1] = 0;
            }

            harm_srew_synt(am,HDS->pchs,tmpIdVUVs,HDS->qLsps,lsUn,synoutv,HDS);

	    td_synt(qRes,HDS->idVUVs,HDS->qLsps,synoutu,HDS);
        }
        else
        {
	    harm_srew_synt(am,HDS->pchs,HDS->idVUVs,HDS->qLsps,lsUn,synoutv,HDS);

	    td_synt(qRes,HDS->idVUVs,HDS->qLsps,synoutu,HDS);
        }

	for(i=0;i<FRM;i++)
	  bufsf[i]= synoutu[i] + synoutv[i];
		
	if (!(HDS->testMode & TM_POSFIL_DISABLE)) IPC_bpf(bufsf, HDS);
    }
    if (HDS->decMode == DEC0K) {
	if (HDS->muteflag == 0)
	    for (i=0;i<FRM;i++)
		bufsf[i] *= ((float)(FRM-i)/(float)FRM);
	else
	    for (i=0;i<FRM;i++) bufsf[i] = 0;
        HDS->muteflag = 1;
    } else {
	if (HDS->muteflag == 1)
	    for (i=0;i<FRM;i++)
	        bufsf[i] *= ((float)i/(float)FRM);
	HDS->muteflag = 0;
    }

    for(i=0;i<FRM;i++)
    {
	if(bufsf[i] > 32767.) bufsf[i]=32767.;
	if(bufsf[i] < -32767.) bufsf[i]= -32767.;
	frmBuf[i] = (short) bufsf[i];
    }

    HDS->syn_frm++;
}


void hvxc_decode(
  HvxcDecStatus *HDS,		/* in: pointer to decoder status */
  unsigned char *encBit,	/* in: bit stream */
  float *sampleBuf,		/* out: frameNumSample audio samples */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
  float speedFact,		/* in: speed change factor */
  float pitchFact,		/* in: pitch change factor */
  int decMode			/* in: decode rate mode */
)
{
  int i;
  int dcfrm;
  float ratio;

  int idVUV;
  float mfdpch;
  float qLsp[LPCORDER];
  float am[SAMPLE/2][3];
  float qRes[FRM];

  short int frmBuf[FRM];

  
  HDS->speedFact = speedFact;
  HDS->pitchFact = pitchFact;
  HDS->decMode = decMode;
  
  dcfrm = 0;

  
  if(HDS->varMode == BM_VARIABLE)
  {
    IPC_DecParams1stVR(encBit, 1.0/HDS->pitchFact, HDS->idVUV2, HDS->bgnFlag2,
			 HDS->qLspQueue, HDS->pch2, HDS->am2, HDS->idCelp2, HDS);
  }
  else
  {
    IPC_DecParams1st(encBit, 1.0/HDS->pitchFact, HDS->idVUV2, HDS->qLspQueue,
                         HDS->pch2, HDS->am2, HDS->idCelp2, HDS);
  }

  while(HDS->nbs == HDS->fr0)
  {
    if(HDS->varMode == BM_VARIABLE)
    {
      IPC_DecParams2ndVR(HDS->fr0-1, HDS->idVUV2, HDS->bgnFlag2, HDS->qLspQueue, HDS->idCelp2,
			 HDS->qLsp2, HDS->qRes2, HDS);
    }
    else
    {
      IPC_DecParams2nd(HDS->fr0-1, HDS->idVUV2, HDS->qLspQueue, HDS->idCelp2,
		       HDS->qLsp2, HDS->qRes2, HDS);
    }

    ratio = HDS->targetP - (float)HDS->fr0 + 1;

    IPC_InterpolateParams(ratio,
			  HDS->idVUV2,
			  HDS->qLsp2,
			  HDS->pch2,
			  HDS->am2,
			  HDS->qRes2,

			  &idVUV,
			  qLsp,
			  &mfdpch,
			  am,
			  qRes,

			  HDS->testMode);
	    
    IPC_SynthSC(idVUV, qLsp, mfdpch, am, qRes, frmBuf, HDS);
	    
    for(i = 0; i < FRM; i++)
    {
      sampleBuf[i + FRM * dcfrm] = (float) frmBuf[i];
    }
    /*
      frm++;
      HDS->targetP = (float)frm*HDS->speedFact;
      */
    HDS->targetP += HDS->speedFact;
    HDS->fr0 = (int)ceil(HDS->targetP);
    /* ratio = HDS->targetP - (float)HDS->fr0 + 1; */
    dcfrm++;
  }
  HDS->nbs++;

  *frameBufNumSample = dcfrm * FRM;
  
}

void hvxc_decode_free(
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  free(HDS);
}
