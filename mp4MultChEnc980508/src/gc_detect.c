
/*****************************************************************************
  detect_gc_short

  Gain changes in the short windows are detected.

*****************************************************************************/
#define GD_THRESH_1 0.0
#define GD_THRESH_2 1.0

void 
detect_gc_short(double prev_peak, 
					 double *a_peak,
					 GAINC *p_gainc1)
{
	int i, j;

	int s = 1; /* for starting the argument of array from -1 */
	int N;
	float r[5], v[6];
	int g[5]; 

	for( i = 0; i < 8; i++ ) {

		N = 4*i; /* constant value to specify the short window */

		/* peak values are copied to region values */
		if(i==0) v[-1+s] = prev_peak;
		else     v[-1+s] = a_peak[N-1];

		for(j=0; j<4; j++) {
			v[j+s] = a_peak[j+N];
		}
		v[4+s] = a_peak[4+N];

		/* calculate Gain Ratio */
		for(j=-1; j<3; j++) {
			if(v[3+s] > GD_THRESH_1 && v[j+s] > 0.0) {
				if(v[j+s] > GD_THRESH_2) r[j+s] = v[3+s] / v[j+s];
				else                     r[j+s] = v[3+s] / GD_THRESH_2;
			}
			else
			   r[j+s] = 1.0;
		}
		if(v[4+s] > GD_THRESH_1 && v[3+s] > 0.0) {
			if(v[3+s] > GD_THRESH_2) r[3+s] = v[4+s] / v[3+s];
			else                     r[3+s] = v[4+s] / GD_THRESH_2;
		}
		else
		   r[3+s] = 1.0;

		/* calculate log */
		for(j=-1; j<4; j++) {
			if(r[j+s] >= 1.0) g[j+s] = floor(log10(r[j+s])/log10(2));
			else             g[j+s] = ceil(log10(r[j+s])/log10(2));
		}

		/* */
      for(j=-1; j<4; j++) {
			if(j == 3) g[j+s] = g[3+s];
			else       g[j+s] = g[j+s] + g[3+s];
		}

		/* limiting */
		for(j=-1; j<4; j++) {
			if(g[j+s] > 11) g[j+s] = 11;
			if(g[j+s] < -4) g[j+s] = -4;
		}

		/* detect gain change */
		p_gainc1[i].natks = 0;
		for(j=0; j<3; j++) {

			/* detect attack */
			if(g[j-1+s] >= g[j+s] && g[j+s] > g[j+1+s]) {
				p_gainc1[i].a_loc[p_gainc1[i].natks] = j;
				p_gainc1[i].a_idgain[p_gainc1[i].natks] 
				= idof_lngain_detect(g[j+s]);
				p_gainc1[i].natks++;
			}
			if(p_gainc1[i].natks > 7) break;

			/* detect release */
			if(g[j-1+s] < g[j+s] && g[j+s] <= g[j+1+s]) {
				if(j != 0 || g[j-1+s] != 0) { 
					p_gainc1[i].a_loc[p_gainc1[i].natks] = j;
					p_gainc1[i].a_idgain[p_gainc1[i].natks] 
					= idof_lngain_detect(g[j-1+s]);
					p_gainc1[i].natks++;
				}
			}
			if(p_gainc1[i].natks > 7) break;
		}

		/* detect attack for the end of the region */
		if(p_gainc1[i].natks < 7) {
			if(g[3+s] > 1) {
				p_gainc1[i].a_loc[p_gainc1[i].natks] = 3;
				p_gainc1[i].a_idgain[p_gainc1[i].natks] 
				= idof_lngain_detect(g[3+s]);
				p_gainc1[i].natks++;
			}
		}

		if(p_gainc1[i].natks > 0 
		&& p_gainc1[i].a_idgain[p_gainc1[i].natks-1] == idof_lngain_detect(0)) {
			p_gainc1[i].natks--;
		}
	}

	p_gainc1[j].peak = v[3+s]; /* set the peak value for next frame */

} /* detect_gc_short */


/*******************************************************************************
	Locate PreEcho Position 

	I: *p_buf	: Data Buffer to be Scanned
	I: nsamples	: Num of Samples in Data Buffer
	I: *p_gainc0	: Gain Control Info for 1st Half
	O: *p_gainc1	: Gain Control Info for 2nd Half

	"nsamples*(NPEPARTS+1)/NPEPARTS" samples are assumed to be
	in the p_buf[].
*******************************************************************************/

void 
set_gainc(double *p_buf, 
			 int nsamples, 
			 GAINC *p_gainc0, 
			 GAINC *p_gainc1)
{
	int	isamp0, nsamps_inpart, idgain;
	int	idgain_total;
	double	absd, peak0, gain, peakmax;
	double	a_peak[NPEPARTS];
	int	i, j;
	int minid;
	int attacks;
	GAINC a_attack;

	a_attack.natks = 0;
	idgain_total = 0;
	idgain = 0;

	/* search the max value of each sub_block  */
	nsamps_inpart = nsamples / NPEPARTS;
	isamp0 = 144;
	for (i = 0; i < NPEPARTS; ++i) {
		a_peak[i] = 0.0;
		for (j = 0; j < nsamps_inpart; ++j) {
			absd = fabs ((double) p_buf[j + isamp0]);
			if (absd > a_peak[i]) {
				a_peak[i] = absd;
			}
		}
		isamp0 += nsamps_inpart;
	}

	/**************************/
	/** Detect in Short Area **/
	/**************************/
	detect_gc_short(p_gainc0->peak, a_peak, p_gainc1);

} /* set_gainc */

