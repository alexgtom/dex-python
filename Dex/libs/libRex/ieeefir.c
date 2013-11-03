/* ieeefir.c
 *	subroutines to perform IEEE Finite-Impulse-Response filtering
 * HISTORY:
 *	24feb93	LMO	Created for use in rex standard library
 * $Log: ieeefir.c,v $
 * Revision 1.3  1994/03/21  20:21:16  lmo
 * fix_edge bug at right edge was kludged.
 *
 * Revision 1.2  1993/02/25  22:53:50  lmo
 * *** empty log message ***
 *
 * Revision 1.1  1993/02/24  16:07:19  lmo
 * Initial revision
 *
 */

#define SHURE		/* use Shure's reflection algorithm for fixing edges */

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "ieeefir.h"

/*
 * math stuff
 */
#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
#define twopi	(2 * M_PI)

extern int *nullPtr;
void diff(FirData *x, FirData *xl, FirData *xdot, FirData **pxdotl, IeeeFir *filter);
void filter(FirData *x, FirData *xl, FirData *xout, FirData **pxoutl, IeeeFir *filt);

/* DO_IEEE_FIR
 *	perform IEEE FIR filtering.
 * INPUT:
 *	raw	-- start of input signal, with real start of buffer at raw[-ieeeFirOff]
 *	out	-- start of ouput signal, with real start of buffer at filt[-ieeeFirOff]
 *	nraw	-- number of data points
 *	flt	-- pointer to filter array
 */
void do_ieee_fir(FirData * raw, FirData * out, int nraw, IeeeFir *flt)
{
	float *pl;
	
/*
fprintf(stderr, "do_ieee_fir:  raw = 0x%x, out = 0x%x, n = %d, flt = 0x%x\n",
raw, out, nraw, flt);
*/
	pl = &out[nraw];
	if (flt->dneg) {
		diff(raw, &raw[nraw], out, &pl, flt);
	}
	else {
		filter(raw, &raw[nraw], out, &pl, flt);
	}
}

/* FIX_EDGES
 * To avoid edge effects, use Shure's method of reflecting
 * data back at edges before fir filtering
 */
void fix_edges(FirData *x, FirData *xl, int num)
{
	int i;
	double z;

	if (num > ieeeFirOff) num = ieeeFirOff;
#ifdef SHURE
	for (i = 1, z = 2 * x[0]; i < num; i++) x[-i] = z - x[i];
	--xl;	/* back up one for bug in offsets */
	for (i = 0, z = 2 * xl[-1]; i < num; i++) xl[i] = z - xl[-i-2];

#else
	for (i = 1; i < num; i++) x[-i] = x[0];		/* fix beginning */
	for (i = 0; i < num; i++) xl[i] = xl[-1];	/* fix ending */
#endif

}

/*
 * FILTER
 *	N point Finite Impulse Filter
 *
 * INPUT:
 *	x = pointer to buffer of type FirData
 *	xl = pointer to one past data
 *	xout = pointer to buffer to hold output
 *	pxoutl = pointer to pointer to one beyond output.
 *		*PXOUTL MUST BE SET BY USER TO POINT TO MAX AVAILABLE SPACE,
 *		i.e., *pxoutl = &xout[MAXSIZE]
 *	filt = pointer to filter structure
 *		This filter is in the order put out by the FIR program,
 *		so that f[0] = the most distant negative time,
 *		and f[n] = time zero
 * OUTPUT:
 *	xout[] is filled with the calculated data
 *		at the beginning and end, where the filter output
 *		is undefined, the raw data is used.
 *	pxoutl is set to point to last calculated data
 *
 * REV
 *	30jul87	LMO	correct count for even low-pass filters
 *	29jul92	LMO	fix buffer ends
 */
void filter(FirData *x, FirData *xl, FirData *xout, FirData **pxoutl, IeeeFir *filt)
{
	register int i;
	register double *h;
	static double z;
	FirData w;
	int num, odd;

	num = filt->dnum;
	odd = (filt->dlen & 01 ? 1 : 0);

	if ((xl - x) > (*pxoutl - xout)) xl = x + (*pxoutl - xout); /* get last x */

	fix_edges(x, xl, num);

	/*
	 * compute by convolving with filter
	 */
	for (; x < xl ; x++, xout++) {
		/* at each point compute sum */
		h = &filt->dptr[num-1];
		if (odd) {
			z = *h-- * x[0];
			for (i = 1; i < num; i++, h--)
				z += *h * (x[i] + x[-i]);
		}
		else {
			z = 0;
			for (i = 0; i < num; h--) {
				w = x[-i];
				w += x[++i];
				z += *h * w;
			}
		}
		*xout = z;
	}

	*pxoutl = xout;
}

/*
 * DIFF
 *	2N + 1 point differentiator
 *
 * INPUT:
 *	x = pointer to buffer of type FirData
 *	xl = pointer to one past data
 *	xdot = pointer to buffer to hold output
 *	pxdotl = pointer to pointer to one beyond output.
 *		*PXDOTL MUST BE SET BY USER TO POINT TO MAX AVAILABLE SPACE,
 *		i.e., *pxdotl = &xdot[MAXSIZE]
 *	filter = pointer to filter structure
 *		This filter is in the order put out by the FIR program,
 *		so that f[0] = the most distant negative time,
 *		and f[n] = time zero
 * OUTPUT:
 *	xdot[] is filled with the calculated derivative
 *	pxdotl is set to point to last calculated derivative
 *
 * REV:
 *	30jul87	LMO	eliminate 0 pass in differentiator loop
 */
void diff(FirData *x, FirData *xl, FirData *xdot, FirData **pxdotl, IeeeFir *filter)
{
	register int i;
	register double *h;
	static double z;
	int num;

	num = filter->dnum;
/*
fprintf(stderr, "diff:  x = 0x%x, xdot = 0x%x, filter = 0x%x, filter->dptr = 0x%x num = %d\n",
x, xdot, filter, filter->dptr, num);
*/

	if ((xl - x) > (*pxdotl - xdot)) xl = x + (*pxdotl - xdot); /* get last x */


	fix_edges(x, xl, num);

	/*
	 * compute derivative by convolving with filter
	 */
	for (; x < xl ; x++, xdot++) {
		/* at each point compute sum */
		for (z = 0, i = 1, h = &filter->dptr[num-1];
			i <= num; i++, h--) {
/*
				fprintf(stderr, "i = %d\n", i);
				fprintf(stderr, "x = 0x%x\n", x);
				fprintf(stderr, "x[%d] = %f\n", i, x[i]);
				fprintf(stderr, "x[-%d] = %f\n", i, x[-i]);
				fprintf(stderr, "h = 0x%x\n", h);
				fprintf(stderr, "*h = %f\n", *h);
*/

				z += *h * (x[i] - x[-i]);
		}
		*xdot = z;
	}

	*pxdotl = xdot;	/* set up pointer to last */
}

/* GET_FIR
 *	read in a standard format FIR filter.
 * 	Scale it.  Fill in IeeeFir table with num and pointer
 *	If scale is zero, a scale of 1 is used.
 *	Multiply by 2pi to convert units
 */
#define ieeeN 68
void get_fir(IeeeFir *pf)
{
	int i;
	double *h;
	size_t nf, nbytes;
	size_t j;
	int nfilt;
	int neg;
	register double *f;
	FILE *fd;
	double scale;

	nbytes = ieeeN * sizeof(double);
	h = (double *) malloc(nbytes);
	if (h == NULL)
		fprintf(stderr, "get_fir() can not malloc space for filter %s!\n", pf->dname);

	if ((fd = fopen(pf->dname, "rb")) == (FILE *)NULL) {
		fprintf(stderr,"Can't open filter file %s\n", pf->dname);
		i = *nullPtr;
	}
	if ((nf = fread(h, nbytes, 1, fd) < nbytes)) {
		fprintf(stderr,"Read on filter file %s returns %d\n",
			pf->dname, nf);
		i = *nullPtr;
	}

	nf = h[66];	/* size is stored in array */
	nfilt = h[67];	/* number of points in filter */
	if (nfilt < 0) {
		nfilt = -nfilt;
		neg = 1;
	}
	else neg = 0;
	f = h;

	if (neg) scale = pf->dscale * twopi;
	else scale = 1.0;

	for (j = 0; j < nf; j++) f[j] = scale * h[j];

	pf->dnum = nf;
	pf->dlen = nfilt;
	pf->dneg = neg;
	pf->dptr = f;
}
