/*
 * N.Meine 1999
 * meine@tnt.uni-hannover.de
 *
 * au_eof(a)		becomes true when there are no more samples to read
 * au_samcount(a)	returns the number of samples that have already been read/written
 * au_name(a)		returns the pointer to the name used to open the stream
 * au_channels(a)	returns the number of channels
 * au_samplerate(a)	returns the sample rate in Hz
 *
 */

#include <libtsp.h>             /* AFsp audio file library */
#include <libtsp/AFpar.h>       /* AFsp audio file library - definitions */

#define	AUBUFSIZE	1024

typedef struct {
	const char	*name;
	AFILE		*fd;
	long		sbcount;
	int		bp;
	int		channels;
	int		samrate;
	int		status;
	long		minuf,maxof;

#ifdef USE_AFSP
	float		buffer[AUBUFSIZE];
#else
	unsigned char	buffer[AUBUFSIZE];
#endif

} Taufi;

#ifdef USE_AFSP
#define au_samcount(af)		(af->sbcount+af->bp)
#else
#define au_samcount(af)		((af->sbcount+af->bp)/2)
#endif


#define au_eof(af)		(af->bp<0)
#define au_samplerate(af)	(af->samrate)
#define au_channels(af)		(af->channels)
#define au_name(af)		(af->name)

void au_putdbl(double xd,Taufi *af);
double au_getdbl(Taufi *af);
Taufi *au_open_read(char *name);
Taufi *au_open_write(char *name,long samrate,long channels);
void au_close(Taufi *af);

