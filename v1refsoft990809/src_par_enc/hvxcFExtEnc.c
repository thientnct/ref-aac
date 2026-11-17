
/*


This software module was originally developed by

    Kazuyuki Iijima, Masayuki Nishiguchi, Jun Matsumoto (Sony Corporation)

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

#include "hvxc.h"
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#define	VUVBAND	12

#define THRESHVUV	0.2
#define THRESHVUVC	0.2

#define HVXC_MAXFLOAT ((float)1e+35)	/* HP 971104 */

extern float	ipc_coef[SAMPLE];

extern float	ipc_bsamp[SAMPLE * R + 1];


static void vuv_reduce(
float	*pch,
float	(*am)[3],
float	*rms,
int	*vuvflg,
int	*vuvcode,
float	fb)
{
    int i,j,send,ub_r,lb_r,id;
    float s,dif,sdif,w0,w0_r,nsr,pitch;
    float oamp[SAMPLE/2];
    float sums,sumsdif;
    static int cnt = 0;
    float vuvbw;
    int bandnum;
    float bunshi,bunbo;
    
    float	reducedNSR[VUVBAND];

    float	dynamic_thre;
    
    pitch = *pch;
    w0 = (float) SAMPLE * (float) R / (pitch+EPS);   
    w0_r = (float) SAMPLE / (pitch+EPS);   
    send= (int) (pitch / 2.);
    vuvbw = (float) (SAMPLE / 2) / (float) VUVBAND;
    bandnum = 1;
    bunshi = 0.0;
    bunbo = 0.0;
    sums = 0.0;
    sumsdif = 0.0;
    ub_r = 0;


    for(i=0; i<=send; i++)
    {
	if(i==0)
	    lb_r=0; 
	else 
	    lb_r=ub_r+1; 
	
	if (i==send) 
	    ub_r=SAMPLE / 2 - 1;
	else 
	    ub_r=IPC_inint((float)i * w0_r + w0_r/2.);
	
	
	if(ub_r >= SAMPLE/2 ) 
	    ub_r=SAMPLE/2 -1;
	
	s = 0.0;
	sdif = 0.0;
	for(j=lb_r; j<=ub_r; j++)
	{
	    id=IPC_inint((float)(SAMPLE*R/2) + (float)R*(float)j - (float)i * w0);
	    s += rms[j] * rms[j];
	    dif = rms[j] -ipc_bsamp[id] * am[i][2];
	    oamp[j] = ipc_bsamp[id] * am[i][2];
	    sdif += dif * dif;
	}
	
	if(s==0.0) s=0.0000001;
	nsr=sdif/s;
	dynamic_thre = THRESHVUV;	
	
	if(nsr > dynamic_thre || (pitch==(float)(20+NUMPITCH/2)) || fb==0.)
	{
	    vuvflg[i]=0;
	}
	else
	{
	    vuvflg[i]=1;
	}
	
	if(((float)i * w0_r) <= ((float)bandnum * vuvbw ))
	{
	    bunshi += nsr*am[i][2];
	    bunbo += am[i][2];
	    if(i == send)
	    {
		reducedNSR[VUVBAND - 1] = bunshi / (bunbo+EPS);

		if((bunshi/(bunbo+EPS))>dynamic_thre ||
		   (pitch==(float)(20+NUMPITCH/2)) ||
		   fb==0.0)
		{
		    vuvcode[VUVBAND-1]=0;
		}
		else
		{
		    vuvcode[VUVBAND-1]=1;
		}
	    }
	}
	else
	{
	    reducedNSR[bandnum - 1] = bunshi / (bunbo+EPS);
	    if((bunshi/(bunbo+EPS))>dynamic_thre || (pitch==(float)(20+NUMPITCH/2))
	       || fb==0.0)
		vuvcode[bandnum-1]=0;
	    else
		vuvcode[bandnum-1]=1;
	    
	    bunshi=nsr*am[i][2];
	    bunbo=am[i][2];
	    bandnum++;
	}
	
	sumsdif += sdif;
	sums += s;
    }
    if(sumsdif==0.) sumsdif=0.0000001;
    if(sums==0.) sums=0.0000001;
    
    cnt++;
}

static void restore_vuv_e(
short int	*vuvPos,
float		vh,
int		*vuv,
float		*mfdpch,
int		numZeroXP,
float		tranRatio,
float		*r0r,
float		fb,
float		lev)
{
    int		i;

    IPC_VUVDecisionRule(fb, lev, vuvPos, vh, numZeroXP, tranRatio, r0r, *mfdpch);

    if( *mfdpch > 145)
	*vuvPos = 0;
    
    if( *vuvPos == 0 )
	*mfdpch=(float)(20+NUMPITCH/2);
    
    for(i=0; i< *vuvPos; i++)
	vuv[i] = 1;
    
    for(i= *vuvPos; i<VUVBAND; i++)
	vuv[i] = 0;
}


static int edge_n(int e, int *s)
{
    while( s[ e ]^s[ e+1 ]==0 ){
	e++;
	if ( e==VUVBAND-1 )
	    break;
    }
    return( ++e );
}


static int transient_n(int *s)
{
    int		tr,i;
    
    tr = 0;
    for(i=1; i<VUVBAND; i++){
	if (s[i]^s[i-1])
	    tr++;
    }
    
    return( tr );
}

static int search_vuv_edge(int *orig_vuv)
{
#define     VRATE_THRE 	0.8
#define     V_BALANCE   4
    
    int		i;
    int		last_1,cnt_of_1; 
    int		local_vuv[ VUVBAND ];
    
    if(transient_n(orig_vuv)>1)
    {
	i=VUVBAND-1;
	
	while(i>0 && orig_vuv[i]==0)
	    i--;
	
	last_1=i;
	
	for(i = cnt_of_1 = 0; i <= last_1; i++)
	{
	    if (orig_vuv[i])
		cnt_of_1++;
	}
	if ((float) cnt_of_1 / (float)last_1 > VRATE_THRE)
	{
	    for(i=0; i<=last_1; i++)
		local_vuv[i] = 1;
	    for(i=last_1+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	else if (last_1<=3 && (float)cnt_of_1/(float)last_1>0.5)
	{
	    for(i=0; i<=last_1; i++)
		local_vuv[i] = 1;
	    for(i=last_1+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	else
	{
	    for(i=0; i<=last_1/V_BALANCE; i++)
		local_vuv[i] = 1;
	    
	    for(i=last_1/V_BALANCE+1; i<VUVBAND; i++)
		local_vuv[i] = 0;
	}
	
	if (!orig_vuv[0] && !orig_vuv[1] && local_vuv[0] && local_vuv[1])
	    return( 0 );
	else
	    return( edge_n( 0,local_vuv ) );
    }
    else if ( transient_n( orig_vuv )==1 )
    {
	if ( orig_vuv[0]==1 )
	{
	    return(edge_n( 0, orig_vuv ));
	}
	else
	{ 
	    if(edge_n(0,orig_vuv)<4)
		return(VUVBAND);
	    else
		return( 0 );
	}
    }
    else if( orig_vuv[0]==1 )
    {
	return( VUVBAND );
    }
    else
    {
	return( 0 );
    }
}




static int CntZeroXP(float *sbuf)
{
    int		i;
    int		numZeroXP;

    numZeroXP = 0;
    for(i = 48; i < FRM + 48; i++)
    {
	if((sbuf[i] < 0.0) && (sbuf[i - 1] > 0.0) ||
	   (sbuf[i] > 0.0) && (sbuf[i - 1] < 0.0))
	{
	    numZeroXP++;
	}
    }
    
    return(numZeroXP);
}    

static float transient(
float	*sbuf,
int	n)
{
    int         i;
    float       po1,po2,po2rms,ratio;

    po1 = 0.;
    for(i = 0; i < 90; i++)
	po1 += sbuf[i] * sbuf[i];

    po2 = 0.;
    for(i = (n-90); i < n; i++)
	po2 += sbuf[i] * sbuf[i];

    if(po1 >= 1 )
    	ratio = po2/po1;
    else
	ratio = 0.;
    
    po2rms = sqrt(po2/90.);

    if(po2rms <= 1000.)
	ratio=0.;

    return(ratio);
}    


#define BLNUM	32

static void FreqBalance(
float	*rms,
float	*arys,
float	*fb,
float	*lev)
{
    int i,j;
    float lbl,hbl,level,th;
    float ene[BLNUM],ave,avem,std;
    float pw,peak,av,rmsf;
    short rmss;
    int grn;
    static int stdm=0;
    static int  co=0;
    static float pl=0.;
    float devi,l1devi;

    lbl=0.;
    for(i=0; i<SAMPLE/4; i++)
	lbl=lbl+rms[i]*rms[i];
    hbl=0.;
    for(i=SAMPLE/4;i<SAMPLE/2; i++)
	hbl=hbl+rms[i]*rms[i];

    for(i=0;i<BLNUM;i++)
	ene[i]=0.;
    for(i=0;i<BLNUM;i++){
	for(j=0;j<SAMPLE/BLNUM;j++)
	    ene[i] += arys[i*SAMPLE/BLNUM+j]*arys[i*SAMPLE/BLNUM+j];
    }

    for(i=0;i<BLNUM;i++)
	ene[i]=sqrt(ene[i]/(float)(SAMPLE/BLNUM));
    for(i=0, ave=0.0;i<BLNUM;i++)
	ave += ene[i];
    ave=ave/BLNUM;

    avem=1.;
    for(i=0;i<BLNUM;i++){
	if(ene[i] != 0)
	    avem = avem * pow(ene[i], 1./(float)BLNUM);
	else
	    avem = 0.;
    }

    for(i=0, devi=l1devi=0;i<BLNUM;i++){
	devi += (ene[i]-ave)*(ene[i]-ave);
	l1devi += fabs(ene[i]-ave);
    }
    devi=sqrt(devi/(float)BLNUM);
    l1devi=(l1devi/(float)BLNUM);
    if(ave>0.001){
	devi=devi/ave;
	l1devi=l1devi/ave;
    }
    else{
	devi=0.1;
	l1devi=0.1;
    }

    if(avem > 0.01)
	std = ave/avem;
    else
	std = 1.;

    for(i=0, av=0.0;i<SAMPLE;i++)
	av += arys[i];
    av=av/SAMPLE;

    for(i=0,pw=peak=0.0;i<SAMPLE;i++){
	pw += (arys[i]-av)*(arys[i]-av);
	if(fabs(arys[i]-av)>peak) 
	    peak=fabs(arys[i]-av);
    }
    rmsf=sqrt(pw/(float)SAMPLE);

    rmss=(short)rmsf & 0xffff;
    if(rmss <= 7.5)
	grn=1;
    else
	grn=0;

    if(hbl != 0. )
	*fb= lbl/hbl ;
    else
	*fb= 20.;
    if(hbl ==0. && lbl==0.) *fb=0.;

    *lev = level = sqrt((lbl+hbl)/(SAMPLE/2));

    if(level > pl)
	pl = level;

    if(pl >0. && level>0.)
	th=20. * log10(level/pl);
    else
	th= -60.;

    if (th <= -60.)
	th= -60.;

    if(devi < 0.4)  
	stdm++;
    else
	stdm=0;

    if(stdm >=5 )
	stdm =5;

    if(grn==1)
    {
	*fb=0;
    }
    else if((*fb < 5.) && (level < 550.) && (stdm >=1))
    {
	*fb=0.;
    }
    else if( (level < 550. && devi < 0.45) || (level < 600. && devi < 0.4))
    {
	*fb=2.;
    }
    else
    {
	*fb=1.;
    }

    co++;
}

static void ModifyFB(
float	*fb,
float	*lev)
{
    
    float  fbb;
    float  levv;
    static float    threshold=10.;
    static float    fb0=0.;
    static float    fb1=0.;
    static float    fb2=0.;
    static float    fb3=0.;
    static float    fb4=0.;
    static float    fb5=0.;
    
    fbb = *fb;
    levv = *lev;
    
    fb5 = fb4;
    fb4 = fb3;
    fb3 = fb2;
    fb2 = fb1;
    fb1 = fb0;
    fb0 = *fb;  
    
    if( ( fb5+fb4+fb3+fb2+fb1+fb0)==0. )
	threshold = 10.;
    
    if( levv> 500. && threshold !=200.)
	threshold =  90.;

    if( levv>  2000.)
	threshold = 200.;

    if( levv<threshold && fbb!=0 )
	*fb = 0.;
}


static void PostVFineModeDecisionVR(
int     *vuvFlg,
float   *r0r,
float   r0h,
float   fb,
float   lev,
float   refLev,
float   gml,
int     cont)
{
    static int oldvuvFlg[2] = {0, 0};
    int onset;

    static int bgnCnt = 0;
    static int bgnIntvl = 0;

    if(oldvuvFlg[0] == 0 && oldvuvFlg[1] == 0 && *vuvFlg == 1)
    {
        onset = 1;
    }
    else
    {
        onset = 0;
    }


    if(*vuvFlg != 0)
    {
        if(r0h < 0.20)
        {
            /* mixed voiced UV */
            *vuvFlg = 2;
        }
        else if(r0r[1] < 0.7)
        {
            /* mixed voiced V */
            *vuvFlg = 2;
        }
        else
        {
            /* perfect V */
            *vuvFlg = 3;
        }

            if(refLev < gml * 2.0 && cont < 2)
            {
                *vuvFlg = 1;
            }
    }

    oldvuvFlg[1] = oldvuvFlg[0];
    oldvuvFlg[0] = *vuvFlg;

}

#define max(x, y)       (x > y ? x : y)
#define min(x, y)       (x < y ? x : y)

static void PostUVFineModeDecisionVR(
int     *idVUV,
float   lev,
float   fb,
float   tranRatio,
float   refLev,
float   gml)
{
    static int bgnCnt = 0;
    static int bgnIntvl = 0;

        if(*idVUV == 0)
        {
            if(refLev < gml * 2.0)
            {
                if(bgnCnt < 3)
                {
                    bgnCnt++;
                }
                else
                {
                    if(bgnIntvl < BGN_INTVL)
                    {
                        *idVUV = 1;
                        bgnIntvl++;
                    }
                    else
                    {
                        bgnIntvl = 0;
                    }
                }
            }
            else
            {
                bgnCnt = 0;
            }
        }
}

void VFineModeDecision(
int	*vuvFlg,
float	*r0r,
float	r0h)
{
    static int oldvuvFlg[2] = {0, 0};
    int onset;

    if(oldvuvFlg[0] == 0 && oldvuvFlg[1] == 0 && *vuvFlg == 1)
    {
	onset = 1;
    }
    else
    {
	onset = 0;
    }
    

    if(*vuvFlg != 0)
    {
        if(r0h < 0.20)
        {
            *vuvFlg = 1;
        }
        else if(r0r[1] < 0.7)
        {
            *vuvFlg = 2;
        }
        else
        {
            *vuvFlg = 3;
        }
    }

    oldvuvFlg[1] = oldvuvFlg[0];
    oldvuvFlg[0] = *vuvFlg;
}    

void vuv_decision(
float	*arys,
float	*mfdpch,
float	(*am)[3],
float	*rms,
float	*r0r,
int	*vuvFlg,
float   r0h)
{
    int		i;
    int		vuvflg[SAMPLE/2];
    int		vuvcode[VUVBAND];
    float	tranRatio;
    int		numZeroXP;

    float	aryd[SAMPLE];
    float	ang2[SAMPLE], re2[SAMPLE], im2[SAMPLE], rms2[SAMPLE];

    short	vuvPos;

    static float	fb[2], lev[2];

    float	arysTmp[SAMPLE];

    static int	frm = 0;

    float	w;

    float	vh, sum;

    for(i = 0; i < SAMPLE; i++)
    {
	arysTmp[i] = arys[i];
    }

    IPC_hp_filterp2(arysTmp, frm);

    IPC_window(ipc_coef, arysTmp, aryd, SAMPLE);
    IPC_fcall_fft(aryd, rms2, ang2, re2, im2);
    
    FreqBalance(rms2, arys, &fb[0], &lev[0]);

    if(fb[0] == 2.0)
    {
	if (r0r[1] < 0.4)
	{
	    fb[0] = 0.0;
	}
	else
	{
	    fb[0] = 1.0;
	}
    }
    
    if(r0r[1] > 0.5)
    {
	fb[0] = 1.0;
    }

    ModifyFB(&fb[0],&lev[0]);

    numZeroXP = CntZeroXP(arysTmp);

    tranRatio = transient(arysTmp, SAMPLE);

    vuv_reduce(mfdpch, am, rms, vuvflg, vuvcode, fb[0]);
    
    vh = 0.0;
    sum = 0.0;
    for(i = 1; i < (int) (*mfdpch / 2.0); i++)
    {
	w = 1.0;
	vh += vuvflg[i] * w;
	sum += w;
    }

    vh /= sum;

    vuvPos = (short) search_vuv_edge( vuvcode );

    restore_vuv_e(&vuvPos, vh, vuvcode , mfdpch, numZeroXP, tranRatio, r0r,
		  fb[0], lev[0]);

    if(vuvPos == 0)
    {
	*vuvFlg = 0;
    }
    else
    {
	*vuvFlg = 1;
    }

    VFineModeDecision(vuvFlg, r0r, r0h);

    frm++;

}

void HvxcVUVDecisionVR(

float   *arys,
float   pitch,
float   *r0r,
float   r0h,
int     *idVUV)
{
    int         i;
    float       arysTmp[SAMPLE];
    float       aryd[SAMPLE];
    int         numZeroXP;
    float       tranRatio;
    float       rms2[SAMPLE], ang2[SAMPLE], re2[SAMPLE], im2[SAMPLE];


    static float        fb = 0.0, lev = 0.0;


    static float refLev = 1000.0;

    static int  frm = 0;

    static int  prevVUV, cont = 0;

    static float        gml = HVXC_MAXFLOAT;         /* Global Minimum Level */

    for(i = 0; i < SAMPLE; i++)
    {
        arysTmp[i] = arys[i];
    }

    IPC_hp_filterp2(arysTmp, frm);

    IPC_window(ipc_coef, arysTmp, aryd, SAMPLE);
    IPC_fcall_fft(aryd, rms2, ang2, re2, im2);

    /* fb 0:unvoiced, 1:voiced, 2:unknown */

    FreqBalance(rms2, arys, &fb, &lev);


    if(fb == 2.0)
    {
        if (r0r[1] < 0.4)
        {
            fb = 0.0;
        }
        else
        {
            fb = 1.0;
        }
    }

    if(r0r[1] > 0.5)
    {
        fb = 1.0;
    }

    ModifyFB(&fb,&lev);

    numZeroXP = CntZeroXP(arysTmp);

    tranRatio = transient(arysTmp, SAMPLE);

    HvxcVUVDecisionRuleVR(fb, lev, numZeroXP, tranRatio, r0r, pitch, &gml, idVUV);


    if(prevVUV == *idVUV)
    {
        cont++;
    }
    else
    {
        cont = 0;
    }

    prevVUV = *idVUV;

    refLev = 0.75 * max(lev, refLev) + 0.25 * min(lev, refLev);

    PostVFineModeDecisionVR(idVUV, r0r, r0h, fb, lev, refLev, gml, cont);
    PostUVFineModeDecisionVR(idVUV, lev, fb, tranRatio, refLev, gml);

    frm++;

}

