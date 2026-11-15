#include <stdlib.h>
#include <stdio.h>

#include "afsp_io.h"

void au_putdbl(double xd,Taufi *af)
{
	fprintf(stderr,"au_putdbl not implemented\n");
	exit(0);
}

static void au_read_buffer(Taufi *af)
{
	register long	i,j;

	if ( af->status )
	{
		af->sbcount+=AUBUFSIZE+1;
		af->bp=-1; 
		return;
	}

	i=AFreadData (af->fd,au_samcount(af),af->buffer,AUBUFSIZE);

	if ( i<AUBUFSIZE )
	{
		if ( i<1 ) { af->sbcount+=AUBUFSIZE+1; af->bp=-1; return; }
		af->status=1;
		for (j=i-1; j>=0; j--) af->buffer[AUBUFSIZE-i+j]=af->buffer[j];
	}
	af->bp=AUBUFSIZE-i;
	af->sbcount+=i;
}

double au_getdbl(Taufi *af)
{
	register long	i=af->bp;
	register double	j=0.0;

	if ( i>=0 )
	{
		j = (double) af->buffer[i];
		i++;
		af->bp=i;
		if ( i==AUBUFSIZE ) au_read_buffer(af);
	}
	return (double) j;
}

/*
static long be_getint(int *x)
{
	register long	r,h;

	h=((unsigned char *) x)[0];	r =h<<24;
	h=((unsigned char *) x)[1];	r|=h<<16;
	h=((unsigned char *) x)[2];	r|=h<< 8;
	h=((unsigned char *) x)[3];

	return r|h;
}
*/

Taufi *au_open_read(char *name)
{
	long		nsamp,nchan;
	float		srate;
	register AFILE	*fd;
	register Taufi	*af;

	fd = AFopenRead (name, &nsamp, &nchan, &srate, NULL );

	if ( fd==NULL )
	{
		fprintf(stderr,"Could not open audio-file <%s>\n",name);
		exit(1);
	}

	af=(Taufi *) malloc(sizeof(Taufi));

	af->name     = name;
	af->fd       = fd;
	af->bp       = AUBUFSIZE;
	af->sbcount  = -AUBUFSIZE;
	af->channels = nchan;
	af->samrate  = (int) srate;
	af->status   = 0;
	af->minuf    = 0;
	af->maxof    = 0;

	au_read_buffer(af);

	return af;
}

Taufi *au_open_write(char *name,long samrate,long channels)
{
	fprintf(stderr,"au_open_write not implemented\n");
	exit(0);
	return (Taufi*)NULL;   /* to avoid compiler warnings */
}

void au_close(Taufi *af)
{
	if ( af->maxof ) fprintf(stderr,"Highest overflow: %li\n",af->maxof);
	if ( af->minuf ) fprintf(stderr,"Lowest underflow: %li\n",af->minuf);

	AFclose(af->fd);

	free(af);
}
