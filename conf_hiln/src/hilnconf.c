/*
 * Statistic conformance tests for the HILN decoder
 *
 * Nikolaus Meine, Uni Hannover
 * meine@stud.uni-hannover.de
 * 16-Mar-2000
 * 01-Aug-2000  Heiko Purnhagen <purnhage@tnt.uni-hannover.de>
 * HP 20010115
 * HP 20010227 OUTSTREAM stdout, not stderr
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef USE_AFSP
#include "afsp_io.h"
#else
#include "au_io.h"
#endif

/* #define OUTSTREAM stderr */
#define OUTSTREAM stdout

#define pi	3.1415926535897932
#define zpi	6.2831853071795865

#define STEPSPERBARK 32
#define STEPSPERDB (1.0/1.5)
#define LOG10 (2.302585093)

#define sqa(c)	((c).re*(c).re+(c).im*(c).im)
#define abs(c)	sqrt(sqa(c))
#define arc(c)	atan2(c.im,c.re)

typedef struct { 
	double	re,im;
} complex;

typedef struct {
	long	group;
	char	*name;
	double	val;
} Tparam;


static double	*ewt = NULL;
static long	maxfx = -1;
static long	ok=1;
static char	*parafile;
static Tparam	*parameter;
static int	numpara,numgroups;

static void fail(char *text)
{
	fprintf(OUTSTREAM,"*** Conformance test failed: %s\n\n",text);
	ok=0;
}

static long param_cmp(Tparam *a,Tparam *b)
{
	register char	*pa,*pb;
	register long	x,y;

	if ( a->group>b->group ) return 1;
	if ( a->group<b->group ) return -1;

	
	pa=a->name;
	pb=b->name;

	for (;;)
	{
		x=*(pa++);
		y=*(pb++);

		if ( (!x) && (!y) ) return 0;

		if ( (x>y) || !y ) return  1;
		if ( (x<y) || !x ) return -1;
	}
}

static void sort(Tparam *x,long n)
{
	register long	d,r,i,j;
	Tparam		h;

	d=(n>>1)+1;
	r=n-1;

	while ( r>0 )
	{
		if ( d<2 )
		{
			h=x[0]; x[0]=x[r]; x[r]=h;
			i=0; r--;
		}
		else
		{
			d--;
			i=d-1;
		}

		h=x[i];
		j=i+i+1;
		while ( r>=j )
		{
			if ( j<r )
			{
				if ( param_cmp(x+j,x+j+1)<=0 ) j++;
			}

			if ( param_cmp(&h,x+j)>0 ) break;

			x[i]=x[j];
			i=j; j=i+i+1;
		}
		x[i]=h;
	}
}

static char *skip_ws(char *p)
{
	register long	h,c=0;

	for (;;)
	{
		h=*p;
		if ( h== 0  ) return NULL;
		if ( h=='#' ) c=1;
		if ( (h==10) || (h==13) ) c=0;
		if ( (h>32) && (!c) ) return p;
		p++;
	}
}

static long nws_len(char *p)
{
	register long	i;

	for (i=0; p[i]>32; i++);
	return i;
}

static void parse_parafile(char *name)
{
	register FILE	*f;
	register long	i,j,l;
	register char	*p;
	char		*hp;

	if ( (f=fopen(name,"r"))==NULL ) { fprintf(OUTSTREAM,"could not open parameter-file <%s>\n",name); exit(1); }

	l =8192;	parafile=(char *) malloc(l);

	for (i=0; (j=getc(f))!=EOF; i++)
	{
		parafile[i]=j;
		if ( i==l-1 )
		{
			l+=l;
			parafile=(char *) realloc(parafile,l);
		}
	}
	parafile[i]=0;
	fclose(f);

	l=512;
	parameter=(Tparam *) malloc(l*sizeof(Tparam));
	numgroups=-1;

	i=0;
	p=parafile;

	while ( (p=skip_ws(p)) )
	{
		if ( i==l )
		{
			l+=l;
			parameter=(Tparam *) realloc(parameter,l*sizeof(Tparam));
		}
		j=nws_len(p);
		if  ( (j==8) && !memcmp(p,"startpos",8) ) numgroups++;
		parameter[i].group=numgroups;
		parameter[i].name=p;
		p[j]=0;
		p=skip_ws(p+j+1);
		if ( !p ) break;
		if (p[0]=='0' && (p[1]=='x' || p[1]=='X'))
		  hp = p; /* catch base16 prefix  HP20010219 */
		else
		  parameter[i].val=strtod(p,&hp);
		if ( hp==p ) parameter[i].val=(double) strtol(p,&hp,0);
		if ( hp==p )
		{
			fprintf(OUTSTREAM,"error: could not convert <%s> to double\n",p);
			exit(1);
		}
		p=hp;
		i++;
	}
	numpara=i;
	numgroups++;
	sort(parameter,i);

        /*	for (i=0; i<numpara; i++) fprintf(OUTSTREAM,"%2i: %s=%f\n",parameter[i].group,parameter[i].name,parameter[i].val); exit(0); */
}

static double getdparam(char *name,long group)
{
	Tparam		t;
	register long	n;
	register Tparam	*l,*r,*m;

	t.group=group;
	t.name=name;

	n=numpara;
	l=parameter;
	r=l+n;

	while (n>1)
	{
		m=l+n/2;
		if ( param_cmp(m,&t)>0 ) r=m; else l=m;
		n=r-l;
	}
	if ( param_cmp(l,&t) ) { fprintf(OUTSTREAM,"error: could not find parameter <%s>\n",name); exit(1); }
	return l->val;
}

static long getiparam(char *name,long group)
{
	return (long) (getdparam(name,group)+0.5);
}

static double getindexparam(char *name,long group,long index)
{
	char	t[128];

	sprintf(t,"%s[%li]",name,index);

	return getdparam(t,group);
}

static void mkeiwutab(double *st,long fn)
{
	register int	i,k;
	register double	phi;

	for (k=fn; k; k>>=1)
	{
		phi = zpi/((double) k);
		for (i=0; i<k; i++) st[i]=(double) sin(phi*((double) i));
		st+=k;
	}
}

static void r4fftstep(register double *x0,register long n,register double *t)
{
	register long		n4 = n/4;
	register double		*x1 = x0+2*n4;
	register double		*x2 = x1+2*n4;
	register double		*x3 = x2+2*n4;
	register double		*y;
	register double		wr,wi,ar,ai,br,bi,cr,ci,dr,di,hr,hi;
	register long		i;


	if ( n>16 )
	{
		y=t+6*n4;
		r4fftstep(x0,n4,y);
		r4fftstep(x1,n4,y);
		r4fftstep(x2,n4,y);
		r4fftstep(x3,n4,y);
	}
	else
	{
		if ( n==8 )
		{
			for (y=x0; y<x0+16; y+=4)
			{
				ar=y[0]; ai=y[1];
				br=y[2]; bi=y[3];
				y[0]=ar+br;
				y[1]=ai+bi;
				y[2]=ar-br;
				y[3]=ai-bi;
			}
		}
		else
		{
			for (y=x0; y<x0+32; y+=8)
			{
				ar=y[0]; ai=y[1];
				br=y[2]; bi=y[3];
				cr=y[4]; ci=y[5];
				dr=y[6]; di=y[7];
				hr=ar+cr; wr=ar-cr; hi=ai+ci; wi=ai-ci;
				ar=br+dr; cr=br-dr; ai=bi+di; ci=bi-di;
				y[0]=hr+ar; y[1]=hi+ai;
				y[2]=wr+ci; y[3]=wi-cr;
				y[4]=hr-ar; y[5]=hi-ai;
				y[6]=wr-ci; y[7]=wi+cr;
			}
		}
	}

	y=t+n4;
	for (i=0; i<n4; i++)
	{
		ar=x0[0]; ai=x0[1];
		wr=y[  i]; wi=t[  i]; hr=x1[0]; hi=x1[1]; br=wr*hr+wi*hi; bi=wr*hi-wi*hr;
		wr=y[2*i]; wi=t[2*i]; hr=x2[0]; hi=x2[1]; cr=wr*hr+wi*hi; ci=wr*hi-wi*hr;
		wr=y[3*i]; wi=t[3*i]; hr=x3[0]; hi=x3[1]; dr=wr*hr+wi*hi; di=wr*hi-wi*hr;
		hr=ar+cr; wr=ar-cr; hi=ai+ci; wi=ai-ci;
		ar=br+dr; cr=br-dr; ai=bi+di; ci=bi-di;
		x0[0]=hr+ar; x0[1]=hi+ai;
		x1[0]=wr+ci; x1[1]=wi-cr;
		x2[0]=hr-ar; x2[1]=hi-ai;
		x3[0]=wr-ci; x3[1]=wi+cr;
		x0+=2; x1+=2; x2+=2; x3+=2;
	}
}


static void perm4(double *x,long p)
{
	register double	a,b,c,d;
	register long	i,j,k,l;

	l=1<<(p-1);
	for (i=j=0; i<(2<<p); i+=2)
	{
		if ( j>i )
		{
			a=x[i]; b=x[i+1]; c=x[j]; d=x[j+1]; x[j]=a; x[j+1]=b; x[i]=c; x[i+1]=d;	
		}
		j+=l;
		for (k=l<<2; j&k; k>>=2) j=(j^k)+(k>>4);
	}
}

static void perm42(double *x,long p)
{
	register long	i,j,n;
	register double	a,b,c,d;

	n=1<<p;
	perm4(x  ,p-1);
	perm4(x+n,p-1);

	for (i=2; i<n; i+=4)
	{
		j=i;
		do
		{
			if ( j<n ) j<<=1; else j=2*(j-n)+2;
		}
		while (j>i);
		if ( j<i ) continue;

		j=i;
		a=x[i  ];
		b=x[i+1];

		for (;;)
		{
			if ( j<n ) j<<=1; else j=2*(j-n)+2;
			if ( i==j )
			{
				x[i  ]=a;
				x[i+1]=b; break;
			}
			c=x[j  ];
			d=x[j+1];
			x[j  ]=a;
			x[j+1]=b;

			if ( j<n ) j<<=1; else j=2*(j-n)+2;
			if ( i==j )
			{
				x[i  ]=c;
				x[i+1]=d; break;
			}
			a=x[j  ];
			b=x[j+1];
			x[j  ]=c;
			x[j+1]=d;
		}
		
	}
}

static void recalc_ewt(long p)
{
	ewt=(double *) realloc(ewt,sizeof(double)<<(1+p));
	maxfx=p;
	mkeiwutab(ewt,1<<p);
}

static void r4fft(double *x,long p)
{
	register long	n;
	register double	*t;

	if ( p>maxfx ) recalc_ewt(p);
	t=ewt;
	for (n=1<<maxfx; n>(1<<p); n>>=1) t+=n;

	if ( p&1 ) perm42(x,p); else perm4(x,p);
	r4fftstep(x,n,t);
}

static void r4fft_r2c(double *x,long p)
{
	register double	*stn2;
	register long	i,n,n2;
	register double ar,ai,br,bi,wr,wi,h1r,h1i,h2r,h2i,cc,cs;

	r4fft(x,p-1);

	n =1<<p;
	n2=n/2;

	stn2=ewt;
	for (i=1<<maxfx; i>(1<<(p-1)); i>>=1) stn2+=i;

	ar=x[0];
	ai=x[1];
	x[   0]= 2.0*(ar+ai);
	x[   1]= 2.0*(ar-ai);
	x[n2  ]= 2.0*x[n2  ];
	x[n2+1]=-2.0*x[n2+1];

	cc=cos(pi/((double) n2));
	cs=sin(pi/((double) n2));
	wr=1.0;
	wi=0.0;

	for (i=2; i<n2; i+=2)
	{
		ar=x[  i  ];
		ai=x[  i+1];
		br=x[n-i  ];
		bi=x[n-i+1];

		if ( i&2 )
		{
			h1r = cc*wr;
			h1i = cc*wi;
			h2r = cs*wi;
			h2i = cs*wr;
			wr = h1r - h2r;
			wi = h1i + h2i;
		}
		else
		{
			wr=stn2[1+n2/4];
			wi=stn2[1];
			stn2++;
		}

		h1r=ar+br;
		h1i=ai-bi;

		h2r=ai+bi;
		h2i=ar-br;

		x[  i  ] = h1r + wr*h2r - wi*h2i;
		x[  i+1] = h1i - wr*h2i - wi*h2r;

		x[n-i  ] = h1r - wr*h2r + wi*h2i;
		x[n-i+1] =-h1i - wr*h2i - wi*h2r;
	}	
}

static double in_place_acf(double *x,long lxp)
{
	register long	i,n,n2;
	register double	h,re,im;

	n=1<<lxp;
	n2=n/2;

	r4fft_r2c(x,lxp);

	h=x[1];
	for (i=1; i<n2; i++)
	{
		re=x[2*i  ];
		im=x[2*i+1];
		x[i]=re*re+im*im;
	}
	x[0]*=x[0];
	x[n2]=h*h;
	for (i=1; i<n2; i++) x[n-i]=x[i];

	r4fft_r2c(x,lxp);
	re=x[0];
	h=1.0/re;
	x[0]=1.0;
	x[n2]*=h;
	for (i=1; i<n2; i++) x[  i]=x[2*i]*h;
	for (i=1; i<n2; i++) x[n-i]=x[i];

	return re/((double) (8*n));
}

/*
static void in_place_acf(double *x,long lxp)
{
	register double	*tmp;
	register long	i,j,n,n2;
	register double	h;

	n=1<<lxp;
	n2=n/2;

	tmp=(double *) malloc(n2*sizeof(double));

	for (i=0; i<n2; i++)
	{
		h=0.0;
		for (j=0; j<n2; j++) h+=x[j]*x[i+j];
		tmp[i]=h;
	}

	for (i=0; i<n2; i++) x[i]=tmp[i]/tmp[0];

	free(tmp);
}
*/

/*
static void syn_il(complex *s,double omega0,double omega1,double omega2)
{
	register long	i;
	register double	t,swp0,swp1;

	swp0 = (omega1-omega0) * (0.5/((double) framelen));
	swp1 = (omega2-omega1) * (0.5/((double) framelen));

	for (i=0; i<framelen; i++)
	{
		t=((double) i)-(((double) (framelen/2))-0.5);
		if ( i==framelen/2 ) swp0=swp1;
		s[i].re=cos(t*(omega1+t*swp0));
		s[i].im=sin(t*(omega1+t*swp0));
	}
}
*/

static void cs_corr(complex *c,double *x,complex *s,long n)
{
	register long	i;
	register double	t,a,b,re,im;
	register double	cc,ss,cs;

	re=im=cc=ss=cs=0.0;

	for (i=0; i<n; i++)
	{
		a=s[i].re;
		b=s[i].im;
		re+=a*x[i];
		im+=b*x[i];
		cc+=a*a;
		ss+=b*b;
		cs+=a*b;
	}

	t=cc*ss-cs*cs;
	c->re=(re*ss-im*cs)/t;
	c->im=(im*cc-re*cs)/t;
}

static double ScaleFToBark(double f)
{
	if ( f<500.0 ) return f*(1.0/100.0);
	
	return 5.0+5.0*log(f*(1.0/500.0));
}

static double ScaleBarkToF(double x)
{
	if ( x<5.0 ) return 100.0*x;

	return 500.0*exp(0.2*(x-5.0));
}

static double ADeQuant(long ai)
{
	register double	h = (double) ai;

	h = -0.5-h;
	return 32768.0*exp((LOG10/(STEPSPERDB*20.0))*h);
}

static long findabsmax(double *x,long n)
{
	register long	i,j;
	register double	h,max;

	max=fabs(x[0]);
	j=0;

	for (i=1; i<n; i++)
	{
		h=fabs(x[i]);
		if (  h>max ) { j=i; max= h; }
	}
	return j;
}

/*
static void k2h(double *h,double *k,long n)
{
	register long	i,j;
	register double	a,b,c;

	if ( k!=h ) for (i=0; i<n; i++) h[i]=k[i];

	for (i=1; i<n; i++)
	{
		c=h[i];

		for (j=0; j<i-j-1; j++)
		{
			a=h[j];
			b=h[i-j-1];
			h[  j  ]=a-c*b;
			h[i-j-1]=b-c*a;
		}
		if ( j==i-j-1 ) h[j]-=c*h[j];
	}
}

static double lar2k(double k)
{
	k=exp(k);
	return (k-1.0)/(k+1.0);
}
*/

static double env_diff(double *x,long env,long framelen)
{
	register double	t0,ra,rd,t,y,d,dmax;
	register long	i;

	t0=((((double)  (env>>8)    )+0.5)/16.0);	t0+=1.125;
	ra=((((double) ((env>>4)&15))-0.5)/15.0);	if ( ra<0.0 ) ra=0.0; else ra=5.0*tan(0.5*pi*ra);
	rd=((((double) ( env    &15))-0.5)/15.0);	if ( rd<0.0 ) rd=0.0; else rd=5.0*tan(0.5*pi*rd);

	dmax=0.0;
	for (i=0;  i<2*framelen; i++)
	{
		t=(((double) i)+0.5)*(1.0/((double) framelen));

		if ( t<0.25 ) y=sin(zpi*t); else
		if ( t<1.00 ) y=1.0; else
		if ( t<1.25 ) y=sin(zpi*(1.25-t)); else
		{
			if ( t<t0 ) y=1.0-ra*(t0-t); else y=1.0-rd*(t-t0);
			if ( y<0.0 ) y=0.0;
		}

/* printf("%f %f %f\n",x[i],y,x[i]-y);	*/

		d=x[i]-y;
		if (  d>dmax ) dmax= d;
		if ( -d>dmax ) dmax=-d;
	}

	return dmax;
}

static void check_startops(Taufi *iaf,long group)
{
	if ( getiparam("startpos",group)!=au_samcount(iaf) )
	{
		fprintf(OUTSTREAM,"error: wrong start position\n");
		exit(1);
	}
}

static void test_noise(Taufi *iaf,long group)
{
	register double	*x,*h;
	register long	fx,fn,i,j,framelen,nframes,order;
	register double	s,pwr,aref,amp,adiff;

	fprintf(OUTSTREAM,"Testing noise spectrum with parameter set %li\n",group);

	check_startops(iaf,group);

	framelen=getiparam("framelen"  ,group);
	nframes =getiparam("nframes"   ,group);
	order   =getiparam("order"     ,group);
	aref    =getdparam("aref"      ,group);

	h=(double *) malloc(order*sizeof(double));

	for (i=0; i<order; i++) h[i]=getindexparam("h",group,i);


	fn=nframes*framelen;
	for (fx=5; (1<<fx)<2*fn; fx++);
	fn=1<<fx;

	if(fn<16000) {
	  fprintf(OUTSTREAM,"error in parameter file: fn=%ld < 16000!\n",fn);
	  exit(1);
	}

	x=(double *) malloc(fn*sizeof(double));

	pwr=0.0;
	for (i=0; i<order; i++) x[i]=0.0;
	for (i=0; i<nframes*framelen; i++)
	{
		s=au_getdbl(iaf);
		pwr+=s*s;
		x[i+order]=s;
		for (j=0; j<order; j++) s-=h[j]*x[order-1+i-j];
		x[i]=s;
	}
	for (; i<fn; i++) x[i]=0.0;

	amp=sqrt(pwr/((double) (framelen*nframes)));

	in_place_acf(x,fx);
	i=findabsmax(x+1,15999)+1;

	adiff=20.0*log10(amp/aref);
	fprintf(OUTSTREAM,"noise ampl. diff: %10.5f dB\n",adiff);
	fprintf(OUTSTREAM,"max. noise acf  : %10.5f at %li\n\n",x[i],i);

	if ( fabs(x[i]) > getdparam("max_noiseacf",group) ) fail("noise acf too big");

	if ( fabs(adiff) > getdparam("max_noiseadiff",group) ) fail("invalid noise amplitude");


	free(x);
	free(h);
}

static void test_noise_env(Taufi *iaf,long group)
{
	double		*pwrhist;
	register long	i,k,nframes,framelen;
	register double	s,mediff,aref;

	framelen=getiparam("framelen"  ,group);
	nframes =getiparam("nframes"   ,group);
	aref    =getdparam("aref"      ,group);

	fprintf(OUTSTREAM,"Testing noise temporal envelope with parameter set %li\n",group);

	check_startops(iaf,group);

	pwrhist=(double *) malloc(2*framelen*sizeof(double));

	for (i=0; i<2*framelen; i++) pwrhist[i]=0.0;
	k=framelen/8;

	for (i=0; i<framelen*nframes; i++)
	{
		s=au_getdbl(iaf);
		pwrhist[k++]+=s*s;
		if ( k==2*framelen ) k=0;
	}

	for (i=0; i<2*framelen; i++)
	{
		s=2.0*pwrhist[i]/((double) nframes);
		pwrhist[i]=sqrt(s)/aref;
	}

	mediff=100.0*env_diff(pwrhist,0x676,framelen);

	fprintf(OUTSTREAM,"max. ampl diff  : %10.5f %%\n\n",mediff);

	if ( mediff>getdparam("max_noise_envdiff",group) ) fail("noise amplitude difference too big");

	free(pwrhist);
}

static void test_startphases(Taufi *iaf,long group)
{
	complex		*camp;
	double		*ibuf;
	complex		*sbuf;
	double		*pacf,*pbuf,*aref,*fref;
	int		phist[16];
	complex		cr;
	register double	f,a,p,h,ps,pn,mda,minsnr,samrate;
	register long	i,j,k,ph0,ph1,frnum=0,idx=0,numline,acflen,nframes,framelen;

	framelen=getiparam("framelen"  ,group);
	nframes =getiparam("nframes"   ,group);
	numline =getiparam("numline"   ,group);
	acflen  =getiparam("acflen"    ,group);
	samrate =getdparam("samrate"   ,group);

	fprintf(OUTSTREAM,"Testing startphases (%li frames) with parameter set %li\n",nframes,group);

	check_startops(iaf,group);

	aref=(double  *) malloc(numline*sizeof(double));
	fref=(double  *) malloc(numline*sizeof(double));

	for (i=0; i<numline; i++)
	{
		aref[i]=getindexparam("aref",group,i);
		fref[i]=getindexparam("fref",group,i);
	}

	camp=(complex *) malloc(numline*sizeof(complex));
	sbuf=(complex *) malloc(numline*2*framelen*sizeof(complex));
	pacf=(double  *) malloc(acflen*sizeof(double));
	pbuf=(double  *) malloc(acflen*sizeof(double));
	ibuf=(double  *) malloc(2*framelen*sizeof(double));

	for (j=0; j<numline; j++)
	{
		f=fref[j]*zpi/samrate;
		for (i=0; i<2*framelen; i++)
		{
			p=((double) (i-framelen))+0.5;
			h=0.5+0.5*cos(p*(pi/((double) framelen)));
			sbuf[2*framelen*j+i].re=h*cos(f*p);
			sbuf[2*framelen*j+i].im=h*sin(f*p);
		}
	}

	mda=0.0;
	minsnr=1000.0;

	for (i=0; i<16; i++) phist[i]=0;

	for (i=0; i<acflen; i++) pacf[i]=pbuf[i]=0.0;

	for (i=0; i<framelen/2; i++) au_getdbl(iaf);

	for (frnum=0; frnum<nframes-2; frnum+=2)
	{
		for (i=0; i<2*framelen; i++) ibuf[i]=au_getdbl(iaf);

		for (i=0,ps=0.0; i<2*framelen; i++) ps+=ibuf[i]*ibuf[i];

		for (j=0; j<numline; j++)
		{
			cs_corr(camp+j,ibuf,sbuf+2*framelen*j,2*framelen);
			cr=camp[j];
			for (i=0; i<2*framelen; i++)
			{
				ibuf[i] -= cr.re*sbuf[2*framelen*j+i].re + cr.im*sbuf[2*framelen*j+i].im;
			}
		}

		for (j=0; j<numline; j++)
		{
			cr=camp[j];
			cs_corr(camp+j,ibuf,sbuf+2*framelen*j,2*framelen);
			cr.re+=camp[j].re;
			cr.im+=camp[j].im;

			a=abs(cr);	h=fabs(a-aref[j]); if ( h>mda ) mda=h;
			p=arc(cr);	phist[((long) ((p+pi)*(16.0/zpi)))&15]++;

			pbuf[idx]=p;
			for (i=0,k=idx; i<acflen; i++)
			{
				pacf[i]+=p*pbuf[k];
				if ( k==0 ) k=acflen;
				k--;
			}

			cr=camp[j];
			for (i=0; i<2*framelen; i++)
			{
				ibuf[i] -= cr.re*sbuf[2*framelen*j+i].re + cr.im*sbuf[2*framelen*j+i].im;
			}
			idx++;
			if ( idx==acflen ) idx=0;
		}

		for (i=0,pn=0.0; i<2*framelen; i++) pn+=ibuf[i]*ibuf[i];

		if ( (pn>0.0) && (ps>0.0) )
		{
			h=10.0*log10(ps/pn);
			if ( h<minsnr ) minsnr=h;
		}
	}

	for (i=frnum*framelen+framelen/2; i<nframes*framelen; i++) au_getdbl(iaf);

	free(sbuf);
	free(camp);

	i=findabsmax(pacf+1,acflen-1)+1;
	a=pacf[i]/pacf[0];
	fprintf(OUTSTREAM,"max. ampl. diff.: %10.5f lsb\n",mda);
	fprintf(OUTSTREAM,"min. SNR        : %10.5f dB\n",minsnr);
	fprintf(OUTSTREAM,"max. startp. acf: %10.5f at %li\n\n",a,i);

	ph0=ph1=0;
	j=phist[0];
	for (i=1; i<16; i++)
	{
		k=phist[i];
		if ( k<phist[ph0] ) ph0=i;
		if ( k>phist[ph1] ) ph1=i;
		j+=k;
	}

	fprintf(OUTSTREAM,"min. norm. phist: %f at %li\n",16.0*((double) phist[ph0])/((double) j),ph0);
	fprintf(OUTSTREAM,"max. norm. phist: %f at %li\n\n",16.0*((double) phist[ph1])/((double) j),ph1);

	if ( phist[ph0] < getdparam("histmin"       ,group) ) fail("start phase distribution (min)");
	if ( phist[ph1] > getdparam("histmax"       ,group) ) fail("start phase distribution (max)");
	if ( mda        > getdparam("max_il_amplerr",group) ) fail("incorrect individual line amplitude");
	if ( minsnr     < getdparam("min_il_snr"    ,group) ) fail("too much noise in individual line synthesis");
	if ( fabs(a)    > getdparam("max_phaseacf"  ,group) ) fail("start phase periodicity detected");

	free(pacf);
	free(pbuf);
	free(ibuf);
	free(aref);
	free(fref);
}

int main(int argc,char **argv)
{
	register Taufi	*iaf;
	register long	i;

	if ( argc<3 )
	{
		fprintf(OUTSTREAM,"usage: %s parameter-file audio-file test_1 ...\n"
			"  1 : random start phases\n"
			"  2 : noise spectrum\n"
			"  3 : noise temporal envelope\n"
			,argv[0]);
		exit(0);
	}

	parse_parafile(argv[1]);
	iaf=au_open_read(argv[2]);

	fprintf(OUTSTREAM,"%s: ctp=%s wav=%s\n",argv[0],argv[1],argv[2]);

	for (i=3; i<argc; i++)
	{
		switch ( atoi(argv[i]) )
		{
			case  1 : test_startphases(iaf,i-3);	break;
			case  2 : test_noise(iaf,i-3);		break;
			case  3 : test_noise_env(iaf,i-3);	break;
			default : fprintf(OUTSTREAM,"Unknown test code\n"); exit(0);
		}
	}

	au_close(iaf);

	if ( ewt ) free(ewt);

	if ( !ok ) fprintf(OUTSTREAM,"*** TEST FAILED!\n\n"); else fprintf(OUTSTREAM,"all tests passed!\n\n");

	return 0;
}
