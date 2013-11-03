#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include "ieeefir.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

/*
 * Global variables for fast differentiators
 *
 * History:
 * $Log: fdiff.c,v $
 * Revision 1.1  1994/03/21  20:21:16  lmo
 * Initial revision
 *
 * Revision 2.0  1992/07/22  18:52:51  lmo
 * SunView Final Version
 *
 * Revision 1.2  1991/06/03  14:47:22  lmo
 * fix fdiff end points
 *
 * Revision 1.1  1991/02/26  15:52:20  lmo
 * Initial revision
 *
 */
PRIVATE int pd_N, pd_L, pd_NpL, pd_2Lp1;
//PRIVATE int pd_N2, pd_L2;

/* dfdiff
 *	returns reciprocal of normalization factor for
 * fast differentiator rexFdiff()
 * NOTE:  set_fdiff() must be called first.
 *
 * INPUT:
 *	T = sample interval in msec.
 * OUTPUT:
 *	factor to convert from central difference to degrees/sec
 */
PRIVATE FirData dfdiff(int T)
{
	double z;

	z = (2.0 * pd_N * pd_2Lp1 * T);

	if (z == 0) return(0.0);
	else return(1000.0/z);
}

/* rexSetFdiff
 *	Set up constants for fast differentiator
 * INPUT:
 *	N	= half-width of central difference
 *	L 	= number of central differences in average
 */
PRIVATE void rexSetFdiff(int N, int L)
{
	pd_N = N;
	pd_L = L;
	pd_NpL = N + L;
	pd_2Lp1 = 2 * L + 1;
}

/* rexPfdiff
 *	Point low-pass differentiator.
 * Based on almost-optimal filters of S. Usui & I. Amidror,
 * IEEE T-BME October, 1982, pgs. 686-693
 *	The algorithm is based on the following
 * definition of the filter:
 *
 *	 2L+1
 *	f	=	d * sum from n = -L to +L of (X      - X     )
 *     N					       k+n+N    k+n-N
 *
 *	where 1/d = 2N(2L+1)T,
 *	where T is the sample interval
 *
 * To keep things efficient, d is calculated seperately by dfdiff(),
 * and just the sum is calculated by pfdiff().  This makes it very
 * fast.
 *
 * INPUT:
 *	din	= pointer to data X(k)
 * The following globals must be set with rexSetFdiff():
 *	pd_N	= width of central difference
 *	pd_L	= number of pairs in average
 *	pd_NpL	= N + L
 *	pd_2Lp1	= 2L + 1
 *
 * OUTPUT:
 *	the unnormalized value of the derivative of X at k
 */
FirData rexPfdiff(FirData *din)
{
	register FirData *plo, *p;
	double sum;

	p = &din[pd_NpL];
	plo = &p[-pd_2Lp1];
	for (sum = 0; p > plo; ) sum += *p--;

	p = &din[-pd_NpL];
	plo = &p[pd_2Lp1];
	for ( ; p < plo ; ) sum -= *p++;

	return(sum );
}

/* rexFdiff
 *	Fast differentiator
 * computes the normalized derivative of X at k
 * Args:
 *	din	= input data signal pointer
 *	dout	= output data signal pointer
 *	npts	= number of points in signal (excluding tails)
 *	N,L	= filter specification
 *	step	= sample interval in msec
 */
void rexFdiff(FirData *din, FirData *dout, int npts, int N, int L, int step)
{
	register FirData *p, *plo, *x, *xl, *xout;
	static double z, norm;

	/*
	 * compute with fast differentiator economy
	 */
	rexSetFdiff(N, L);
	norm = dfdiff(step);

	x  = din;
	xl = &din[npts];
	xout = dout;

	fix_edges(x, xl, 2 * (pd_NpL + pd_2Lp1));

	for ( ; x < xl; x++) {
		p = &x[pd_NpL];
		plo = &p[-pd_2Lp1];
		for (z = 0; p > plo; ) z += *p--;

		p  = &x[-pd_NpL];
		plo = &p[pd_2Lp1];
		for ( ; p < plo; ) z -= *p++;

		*xout++ = z * norm;
	}
}


/* DDIFF0
 *	fast double differentiator from Table II of Usui & Amidror
 *
 *	 5		 5
 *	f	+	f
 *   1/2	     1/2
 *
 * This has bandwidth 0.175 * PI or 0.175 Fs/2
 */
PRIVATE void ddiff0(FirData *din, FirData *dout, int npts, int step)
{
	register FirData *x, *xl, *xout;
	static double z, norm;

	z = step / 1000.0;
	norm = 1.0 / (25 * z * z);

	xout = dout;
	x  = din;
	xl = &din[npts];

	fix_edges(x, xl, 10);
	for ( ; x < xl; x++) {
		z = x[5] + x[-5] - (*x * 2);
		*xout++ = z * norm;
	}
}

/* DDIFF1
 *	fast double differentiator from Table II of Usui & Amidror
 *
 *	f	+	f
 *     3               1  
 *
 * This has bandwidth 0.193 * PI or 0.193 Fs/2
 */
PRIVATE void ddiff1(FirData *din, FirData *dout, int npts, int step)
{
	register FirData *x, *xl, *xout;
	static double z, norm;

	z = step / 1000.0;
	norm = 1.0 / (12 * z * z);

	xout = dout;
	x  = din;
	xl = &din[npts];

	fix_edges(x, xl, 10);
	for ( ; x < xl; x++) {
		z = ((x[4] + x[-4]) - (x[2] + x[-2]));
		*xout++ = z * norm;
	}
}

/* DDIFF2
 *	fast double differentiator from Table II of Usui & Amidror
 *
 *	 3
 *	f	+	f
 *     2	       1
 *
 * This has bandwidth 0.238 * PI or 0.238 Fs/2
 */
PRIVATE void ddiff2(FirData *din, FirData *dout, int npts, int step)
{
	register FirData *x, *xl, *xout;
	static double z, norm;

	z = step / 1000.0;
	norm = 1.0 / (24 * z * z);

	xout = dout;
	x  = din;
	xl = &din[npts];

	fix_edges(x, xl, 10);
	for ( ; x < xl; x++) {
		z = (x[3] + x[-3]) + (x[4] + x[-4])
		    - ((*x * 2) + (x[1] + x[-1]));
		*xout++ = z * norm;
	}
}

/* rexFddiff
 *	Fast double differentiator
 */
void rexFddiff(FirData *din, FirData *dout, int npts, int M, int step)
{
	/*
	 * compute with fast double differentiator economy
	 */
	switch(M) {
	case 0:		/* 1/2F^5+1/2F^5 */
		ddiff0(din, dout, npts, step);
		break;
	case 1:		/* 3f + 1f */
		ddiff1(din, dout, npts, step);
		break;
	case 2:		/* 2f^3 + 1f */
		ddiff2(din, dout, npts, step);
		break;
	default:
		fprintf(stderr,"no fast double diff # %d\n", M);
		break;
	}
}

/* FRR
 * 	return frequency response ratio of fast differentiator
 * at freq omega.  See eq. 8, pg 690 of Usui & Amidror
 *
 * INPUT:
 *	w = frequency in radians/sec
 *	N = half-spacing of central difference
 *	L = # of central differences in average
 *	T = time interval in sec
 * OUTPUT:
 *	ratio of frequency response to ideal differentiator freq response (jw).
 */
PRIVATE double frr(double w, int N, int L, int T)
{
	register int n;
	double fac, z, wt;

	if (w <= 0) return(1.0);	/* all differentiators are zero at zero ! */
	wt = w * T * 1000.0;
	fac = wt * N * (2 * L + 1);

	for (z = 0, n = -L; n <= L; n++) z += sin((N + n) * wt);
	return( z / fac );
}

/* GET_BW
 *	determine band_width of filter.
 * INPUT:
 *	dB = #decibels down to define bandwidth (e.g., -3)
 *	f  = function to calculate freq response ratio
 *	T  = time sample step size in sec, i.e., sampling rate = 1/T
 * OUTPUT:
 *	first frequency where response ratio is <= dB
 */
PRIVATE double get_bw(double dB, int N, int L, int T)
{	
	double w, dw, wmax, z, thresh;

	wmax = 0.5 / (T * 1000); /* Nyquist folding frequency */
	dw = wmax / 10000;
	thresh = pow(10.0, dB / 20);

	for (w = 0; w < wmax; w += dw) {
		z = frr(w, N, L, T);
		if (z <= thresh) break;
	}
	return(w);
}

/* RexFdiffBw
 * 	function to calculate the band-width of the
 * fast differentiators from:
 * S. Usui & I. Amidror, IEEE T-BME 29, pg 686-693, 1982
 *
 * these filters are central difference averages, characterized as:
 *	 2L+1
 *	f
 *     N
 *
 * N = half-width of central difference
 * L = # of central differences in avg
 * T = sample interval in milliseconds
 * printstrm, if not NULL, is output stream for printed message
 */
double rexFdiffBw(int N, int L, int T, FILE *printstrm)
{
	double bw, get_bw();
	
	bw = get_bw(-3.0, N, L, T);

	if (printstrm) {
		fprintf(printstrm,
"Fast Differentiator of Usui & Amidror:  N = %d, L = %d, Fs = %6.2f Hz\n",
		 N, L, (1000.0 / T));
		fprintf(printstrm, "\t\twhere N = half-width of central difference\n");
		fprintf(printstrm, "\t\twhere L = # of central differences in avg\n");
		fprintf(printstrm, "\t\twhere T = sample interval in milliseconds\n");
		printf("Band Width (-3 dB) = %f Hz\n", bw / (2 * M_PI));
	}
	return(bw);
}
