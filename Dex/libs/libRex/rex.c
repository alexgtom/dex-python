/*
 *-----------------------------------------------------------------------*
 * notice:  This code was developed by the US Government.  The original
 * versions, REX 1.0-3.12, were developed for the pdp11 architecture and
 * distributed without restrictions.  This version, REX 4.0, is a port of
 * the original version to the Intel 80x86 architecture.  This version is
 * distributed only under license agreement from the National Institutes 
 * of Health, Laboratory of Sensorimotor Research, Bldg 10 Rm 10C101, 
 * 9000 Rockville Pike, Bethesda, MD, 20892, (301) 496-9375.
 *-----------------------------------------------------------------------*
 */

/* second of these two lines determines debug flag */
#define DEBUG
#undef DEBUG

#define min_(a,b)	((a) < (b) ? (a) : (b))

#include <stdio.h>
#include <stdlib.h>

#if defined(_LINUX) || defined(_MACOSX)
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#endif

#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "ieeefir.h"
#include "rex.h"

/*
 * private macros
 */
#define recNum2Index(z)	((z) - 1)
#define index2RecNum(z)	((z) + 1)
#define loword_(a)	(unsigned short)(a & 0xFFFF)

/* version message
 */
#ifdef MRDDMOD
PRIVATE char *rexToolsVerNum =
"RexTools Version 4.6.8 (MOD for KELVIN) -- 29mar04 -- LM Optican (LSR/NEI/NIH)";
#else
PRIVATE char *rexToolsVerNum =
"RexTools Version 4.6.9 -- 24may04 -- LM Optican (LSR/NEI/NIH)";
#endif

/*
 * private variables
 */
int *nullPtr = 0;

PRIVATE char aHeader[512] = { 0 };	/* A-file headers */
PRIVATE char eHeader[512] = { 0 };	/* E-file headers */
PRIVATE FILE *fa = NULL, *fe = NULL;	/* analog and event streams */
PRIVATE int startCode = ENABLECD;	/* trial start code */
PRIVATE int preTime = 0;		/* trial pre-time */
PRIVATE int wopencd = WOPENCD;		/* analog window open code */
PRIVATE int wclosecd = WCLOSECD;		/* analog window close code */
PRIVATE int wcancelcd = WCANCELCD;		/* analog window cancel code */
PRIVATE int numRec = { 0 };		/* number of records */
PRIVATE int rexSplit = 0;		/* flag for splitting event buffer */

PRIVATE AIX *aindx[rexNchannels] = { 0 };	/* hold analog record keys */
PRIVATE int laindx[rexNchannels] = { 0 };	/* length of aindx */
PRIVATE int naindx[rexNchannels] = { 0 };	/* number of A-file records, including continues */
PRIVATE int nrindx[rexNchannels] = { 0 };	/* number of A-file records, after joining continues */

PRIVATE RexInfo rexInfo = {{ 0 }};			/* record information structure */
PRIVATE RexSignal *signals[rexMaxSignals] = { 0 };	/* array of signals */
PRIVATE SignalList signalList[rexMaxSignals] = {{ 0 }};	/* list of signal arrays */
PRIVATE int lsignal[rexMaxSignals] = { 0 };		/* length of signal arrays */
PRIVATE int nSignals = { 0 };				/* number of signals */

PRIVATE EVENT *evbuf = { 0 };		/* the Event Buffer */
PRIVATE int levbuf = { 0 };
PRIVATE int nevbuf = { 0 };

PRIVATE Trial *trials = { 0 };
PRIVATE Trial *getTr = { 0 };		/* current trial data pointer */
PRIVATE int ltrials = { 0 };
PRIVATE int numTrials = { 0 };		/* number of trials */

PRIVATE short int *adata = { 0 };
PRIVATE int ladata = { 0 };
PRIVATE int nadata = { 0 };

PRIVATE int mpts[rexMaxSignals] = { 0 };
PRIVATE int nPoints = 0;
PRIVATE int rexMaxSampRate = 1000;	/* maximum sampling rate for any signal */
PRIVATE float abufTfac = 0;		/* samples/sec in A-buffer */

PRIVATE ANALOGHDR ahd = { 0 };
PRIVATE int32_t aStartTime = { 0 };
PRIVATE int32_t aEndTime = { 0 };
PRIVATE int32_t tStartTime = { 0 };
PRIVATE int32_t evnum = { 0 };			/* event counter */
PRIVATE int32_t anum = { 0 };			/* analog file counter */
PRIVATE int arsize = { 0 };			/* size of analog header */
PRIVATE int edone = { 0 };			/* flag set if event info was printed */

PRIVATE int32_t *ubuf[rexMaxUnits] = { 0 };	/* unit pointers for sorting */
PRIVATE int lubuf[rexMaxUnits] = { 0 };		/* length of unit buffers */
PRIVATE int nubuf[rexMaxUnits] = { 0 };		/* unit count */
PRIVATE int nUnits = 0;				/* number of non-zero units */ 
PRIVATE int unitCodes[rexMaxUnits];		/* codes for non-zero units */
PRIVATE int unitsU[rexMaxUnits];		/* index for non-zero units */
PRIVATE int unitsL[rexMaxUnits];		/* last index checked */

PRIVATE int a110, ac110 = { 0 };		/* 110 a records and continues */
PRIVATE int a111, ac111 = { 0 };		/* ditto for 111 */
PRIVATE int a112, ac112 = { 0 };		/* ditto for 112 */

PRIVATE int32_t first_pass = 1;			/* local flag */

PRIVATE char *efilen = (char *)NULL;
PRIVATE char *afilen = (char *)NULL;
PRIVATE int masks[MASK_N] = { MASK_0, MASK_1, MASK_2, MASK_3 };

/*
 * sample header variables
 */
PRIVATE SAMP sampHdr = { 0 };
PRIVATE short *sa_ad_rate = 0;
PRIVATE short *sa_store_rate = 0;
PRIVATE short *sa_ad_calib = 0;
PRIVATE short *sa_shift = 0;
PRIVATE short *sa_gain = 0;
PRIVATE short *sa_ad_delay = 0;
PRIVATE short *sa_ad_chan = 0;
PRIVATE short *sa_frame = 0;
PRIVATE char **sa_gvname = 0;
PRIVATE char **sa_title = 0;
PRIVATE char **sa_calibst = 0;

/*
 * gains for each signal channel
 */
PRIVATE double sigGain[rexMaxSignals] = { 0 };

/*
 * PRIVATE function prototypes
 */

/*
 * ----------------------  PRIVATE FUNCTIONS --------------------------------
 */
/* maintain a list of allocated memory
 */
PRIVATE void **raPtrs = NULL;
PRIVATE int nPtrs = 0;
PRIVATE int lPtrs = 0;
#ifdef DEBUG
PRIVATE char **ptrNames = NULL;
#endif

PRIVATE void svPtrs(void * p, char * name)
{
	name = name;

	if (nPtrs >= lPtrs) {
		lPtrs += 256;
		if (raPtrs)
			raPtrs = (void **) realloc(raPtrs, lPtrs * sizeof(void *));
		else
			raPtrs = (void **) malloc(lPtrs * sizeof(void *));

#ifdef DEBUG
fprintf(stderr, "in: raPtrs = %x, %d, %d, %d\n", raPtrs, lPtrs, nPtrs, sizeof(void *));
		if (ptrNames)
			ptrNames = (char **) realloc(ptrNames, lPtrs * sizeof(char*));
		else
			ptrNames = (char **) malloc(lPtrs * sizeof(char*));
#endif
	}
#ifdef DEBUG
	ptrNames[nPtrs] = (char * ) malloc(strlen(name) * sizeof(char));
	strcpy(ptrNames[nPtrs], name);
fprintf(stderr, "out: raPtrs = %x, %d, %d\n", raPtrs, lPtrs, nPtrs);
#endif
	raPtrs[nPtrs++] = p;
}

PRIVATE void sv2Ptrs(void * old, void * p, char * name)
{
	int i;

	if (old == p) return;

	for (i = 0; i < nPtrs; i++) {
		if (raPtrs[i] == old) {
			raPtrs[i] = p;
			return;
		}
	}
	fprintf(stderr, "Error in sv2Ptrs for: %s\n", name);
}

PRIVATE void clrPtrs(void) {
	while (nPtrs) {
#ifdef DEBUG
fprintf(stderr, "%d:  0x%x %s\n", nPtrs, raPtrs[nPtrs-1], ptrNames[nPtrs-1]);
#endif
		free(raPtrs[--nPtrs]);
	}
	if (lPtrs) free(raPtrs);
	lPtrs = 0;
	raPtrs = NULL;
}

PRIVATE void * rexAlloc(void * ptr, int32_t size, int32_t number, char * err_msg)
{
	int i;
	void *oldp;
	size_t nbytes;
	char * type;
#ifdef DEBUG
	static int32_t total = 0;
#endif
	nbytes = size * number;
#ifdef DEBUG
	total += nbytes;
	fprintf(stderr, "\trexAlloc(%s): ptr = 0x%x, size = %d, total = %d\n", err_msg, ptr, nbytes, total);
#endif
	if (ptr == NULL) {
		ptr = malloc(nbytes);
		type = "m";
		svPtrs(ptr, err_msg);
	}
	else {
		oldp = ptr;
		ptr = realloc(ptr, nbytes);
		type = "re";
		sv2Ptrs(oldp, ptr, err_msg);
	}
	if (ptr == NULL) {
		fprintf(stderr, "Can't %salloc %ld items of size %ld:  %s\n",
			type, number, size, err_msg);
		i = *nullPtr;
	}

	return(ptr);
}

PRIVATE void errorMsg(char *msg)
{
	char buf[256];

	sprintf(buf, "Error: %s", msg);
	fprintf(stderr, "%s\n", buf);
}

PRIVATE char ** mk_dblchar(int r, int c, char * msg)
{
	char ** p;
	char buf[256];
	int i;

	p = (char **) rexAlloc(NULL, sizeof(char *), r, "mk_dblchar1");
	for (i = 0; i < r; i++) {
		sprintf(buf, "mk_dblchar: %s[%d][%d]", msg, i, c);
		p[i] = rexAlloc(NULL, sizeof(char), c, buf);
	}
	return(p);
}

PRIVATE short * mk_short(int n, char * msg)
{
	short * p;

	p = (short *) rexAlloc(NULL, sizeof(short int), n, msg);
	return(p);
}


/*
 * magic sizes determining version of sample header
 */
static int length41 = 1580;
static int length43 = 1614;

/*
 * load sample header
 * use version number to decide on interpretation
 */
PRIVATE void loadSampHdr(ANALOGHDR *a)
{
	int i, j, maxsig, frsize, maxcal, lname, g;
	short *buf, *p;
	char *b = (char *)NULL;
	char *c = (char *)NULL;
	static float z;
	SAMP *ps;
	SAMP *s = &sampHdr;

	if ((buf = (short *) malloc(a->alength)) == NULL) {
		fprintf(stderr, "can't malloc space for sample header\n");
		i = *nullPtr;
	}

	if ((i = fread(buf, 1, a->alength, fa)) != a->alength) {
		fprintf(stderr, "Error reading sampling header. Aborting!\n");
		i = *nullPtr;
	}

	/*
	 * copy into sample header
	 * if this is old header, then fake it
	 */
	if (a->alength == length41) {	/* original version of header */
		SAMP4_1 *p;
#ifdef DEBUG
fprintf(stderr, "REX v == 4.1!\n");
#endif
		p = ((SAMP4_1 *)buf);

		s->sa_maxsig = p->sa_maxsig;
		s->sa_fr_array_sz = p->sa_fr_array_sz;
		s->sa_maxcal = p->sa_maxcal;
		s->sa_lname = p->sa_lname;
		s->sa_signum = p->sa_signum;
		s->sa_maxrate = p->sa_maxrate;
		s->sa_minrate = p->sa_minrate;
		s->sa_subfr_num = p->sa_subfr_num;
		s->sa_mfr_num = p->sa_mfr_num;
		s->sa_mfr_num = p->sa_mfr_num;
		s->sa_mfr_dur = p->sa_mfr_dur;
		s->sa_fr_sa_cnt = p->sa_fr_sa_cnt;
		s->sa_mfr_sa_cnt = p->sa_mfr_sa_cnt;
		s->sa_ad_channels = p->sa_ad_channels;
		s->sa_ad_res = p->sa_ad_res;
		s->sa_ad_rcomp = p->sa_ad_rcomp;
		s->sa_ad_ov_gain = p->sa_ad_ov_gain;
		s->sa_datumsz = p->sa_datumsz;

		s->sa_ad_rate_bo = p->sa_ad_rate_bo;
		s->sa_store_rate_bo = p->sa_store_rate_bo;
		s->sa_ad_calib_bo = p->sa_ad_calib_bo;
		s->sa_shift_bo = p->sa_shift_bo;
		s->sa_gain_bo = p->sa_gain_bo;
		s->sa_ad_delay_bo = (int)NULL;
		s->sa_ad_chan_bo = p->sa_ad_chan_bo;
		s->sa_frame_bo = p->sa_frame_bo;
		s->sa_gvname_bo = p->sa_gvname_bo;
		s->sa_title_bo = p->sa_title_bo;
		s->sa_calibst_bo = p->sa_calibst_bo;
		s->sa_var_data_begin = p->sa_var_data_begin;

		b = (char *) &p->sa_var_data_begin;
	}
	else if (a->alength == length43) { /* second version, >= 4.3 */
#ifdef DEBUG
fprintf(stderr, "REX v >= 4.3!\n");
#endif
		ps = ((SAMP *)buf);
		*s = *ps;

		b = (char *) &ps->sa_var_data_begin;
	}
	else {
		fprintf(stderr, "loadSampHdr():  Can't determine Sample Header version:  length = %d\n",
				a->alength);
		i = *nullPtr;
	}

	maxsig = s->sa_maxsig;
	frsize = s->sa_fr_array_sz;
	maxcal = s->sa_maxcal;
	lname = s->sa_lname;

	/*
 	 * make room for short int buffers
	 */
	sa_ad_rate = mk_short(maxsig, "sa_ad_rate");
	sa_store_rate = mk_short(maxsig, "sa_store_rate");
	sa_ad_calib = mk_short(maxsig, "sa_ad_calib");
	sa_shift = mk_short(maxsig, "sa_shift");
	sa_gain = mk_short(maxsig, "sa_gain");
	sa_ad_delay = mk_short(maxsig, "sa_ad_delay");
	sa_ad_chan = mk_short(maxsig, "sa_ad_chan");
	sa_frame = mk_short(frsize, "sa_frame");

	/*
	 * load short int buffers
	 */
	for (p = (short *)&b[s->sa_ad_rate_bo], i = 0; i < maxsig; i++) sa_ad_rate[i] = *p++;
	for (p = (short *)&b[s->sa_store_rate_bo], i = 0; i < maxsig; i++) sa_store_rate[i] = *p++;
	for (p = (short *)&b[s->sa_ad_calib_bo], i = 0; i < maxsig; i++) sa_ad_calib[i] = *p++;
	for (p = (short *)&b[s->sa_shift_bo], i = 0; i < maxsig; i++) sa_shift[i] = *p++;
	for (p = (short *)&b[s->sa_gain_bo], i = 0; i < maxsig; i++) {
			sa_gain[i] = *p++;
			if (sa_gain[i] < 0) sa_gain[i] = s->sa_ad_ov_gain;
	}
	if(s->sa_ad_delay_bo) {
		for (p = (short *)&b[s->sa_ad_delay_bo], i = 0; i < maxsig; i++) sa_ad_delay[i] = *p++;
	}
	else {
		for (i = 0; i < maxsig; i++) sa_ad_delay[i] = 0;
	}
	for (p = (short *)&b[s->sa_ad_chan_bo], i = 0; i < maxsig; i++) sa_ad_chan[i] = *p++;
	for (p = (short *)&b[s->sa_frame_bo], i = 0; i < frsize; i++) sa_frame[i] = *p++;

	/*
	 * make room for character arrays
	 */
	sa_gvname = mk_dblchar(maxsig, lname, "sa_gvname");
	sa_title = mk_dblchar(maxsig, lname, "sa_title");
	sa_calibst = mk_dblchar(maxcal, lname, "sa_calibst");

	/*
	 * load character arrays
	 */
	for (c = &b[s->sa_gvname_bo], i = 0; i < maxsig; i++) {
		for (j = 0; j < lname; j++) {
			sa_gvname[i][j] = *c++;
		}
	}
	for (c = &b[s->sa_title_bo], i = 0; i < maxsig; i++) {
		for (j = 0; j < lname; j++) {
			sa_title[i][j] = *c++;
		}
	}
	for (c = &b[s->sa_calibst_bo], i = 0; i < maxcal; i++) {
		for (j = 0; j < lname; j++) {
			sa_calibst[i][j] = *c++;
		}
	}

	/*
	 * load gain array
	 */
	for (i = 0; i < s->sa_signum; i++) {
		g = sa_ad_calib[i];
		if (g < 0) continue;

		sscanf(sa_calibst[g], "%d: %f", &j, &z);
		sigGain[i] = z / (1 << s->sa_ad_res);
	}

	/*
	 * load max sample rate
	 */
	rexMaxSampRate = s->sa_maxrate;

	/*
	 * release memory
	 */
	free(buf);


#ifdef MRDDMOD
	/* fake an aSampHeader for Kelvin's data */
		sampHdr.sa_signum = 4;
		sampHdr.sa_mfr_sa_cnt = 16;
		sampHdr.sa_fr_sa_cnt = 16;
		
		sa_ad_rate[2] = sa_ad_rate[0];
		sa_ad_rate[3] = sa_ad_rate[1];
		sa_store_rate[2] = sa_store_rate[0];
		sa_store_rate[3] = sa_store_rate[1];
		sa_calibst[2] = sa_calibst[0];
		sa_calibst[3] = sa_calibst[1];
		sa_shift[2] = sa_shift[0];
		sa_shift[3] = sa_shift[1];
		sa_gain[2] = sa_gain[0];
		sa_gain[3] = sa_gain[1];		

		pf = sa_frame;
		u = 0;
		pf[u++] = 0x4;
		pf[u++] = 0x0;
		pf[u++] = 0x1;
		pf[u++] = 0x2;
		pf[u++] = 0x3;
		pf[u++] = 0xffff;
		pf[u++] = 0x0;
		pf[u++] = 0x4000;
		pf[u++] = 0x4001;
		pf[u++] = 0x4002;
		pf[u++] = 0x4003;
		pf[u++] = 0xffff;

		pf[u++] = 0x4;
		pf[u++] = 0x0;
		pf[u++] = 0x1;
		pf[u++] = 0x2;
		pf[u++] = 0x3;
		pf[u++] = 0xffff;
		pf[u++] = 0x0;
		pf[u++] = 0x4000;
		pf[u++] = 0x4001;
		pf[u++] = 0x4002;
		pf[u++] = 0x4003;
		pf[u++] = 0xffff;

		pf[u++] = 0x4;
		pf[u++] = 0x0;
		pf[u++] = 0x1;
		pf[u++] = 0x2;
		pf[u++] = 0x3;
		pf[u++] = 0xffff;
		pf[u++] = 0x0;
		pf[u++] = 0x4000;
		pf[u++] = 0x4001;
		pf[u++] = 0x4002;
		pf[u++] = 0x4003;
		pf[u++] = 0xffff;

		pf[u++] = 0x4;
		pf[u++] = 0x0;
		pf[u++] = 0x1;
		pf[u++] = 0x2;
		pf[u++] = 0x3;
		pf[u++] = 0xffff;
		pf[u++] = 0x0;
		pf[u++] = 0x4000;
		pf[u++] = 0x4001;
		pf[u++] = 0x4002;
		pf[u++] = 0x4003;
		pf[u++] = 0xffff;

		pf[u++] = 0xfffe;


		strcpy(sa_gvname[2], "oeyeh");
		strcpy(sa_gvname[3], "oeyev");
		strcpy(sa_title[2], "l_eye_h");
		strcpy(sa_title[3], "l_eye_v");
	}
#endif
}

PRIVATE int getChan(short int code)
{
	switch(code) {
	case -110:
		return(0);
	case -111:
		return(1);
	case -112:
		return(2);
	default:
		return(-1);
	}
}

PRIVATE int getBufChan(short int t)
{
	int m, mask;

	for (m = 1; m < MASK_N; m++) {
		mask = masks[m];
		if (((t > 0) && (t & mask)) ||
			((t < 0) && !(t & mask))) {
			return(m);
		}
	}
	return(0);
}

/*
 * save key for later use
 */
PRIVATE void sv_key(int k_ev, ANALOGHDR *a)
{
	int chan;
	int na;
	int dt;
	EVENT *ev = &evbuf[k_ev];
	AIX *p;

	chan = getChan(a->acode);
	if (chan < 0 || chan >= rexNchannels) return;

	na = naindx[chan];

	if (a->acontinue && !na) return; /* don't start on continuation record */
	while (na >= laindx[chan]) {
		char buf[128];

		laindx[chan] += 500;
		sprintf(buf, "aindx[%d]", chan);
		aindx[chan] = (AIX *) rexAlloc((void *) aindx[chan],
				sizeof(AIX), laindx[chan], buf);
	}
	p = aindx[chan];
	p[na].key = ev->e_key;
	p[na].cont = a->acontinue;
	p[na].loEv = k_ev;
	p[na].nEv = -1;
	p[na].more = 0;

	dt = a->alength*abufTfac;	/* just an estimate */
#ifdef DEBUG
fprintf(stderr, "dt = %d, alength = %d, abufTfac = %d\n", dt, a->alength, abufTfac);
#endif
	if (a->acontinue) {
		if (na) {
			p[na-1].more = 1;
			p[na-1].endTime += dt;
#ifdef DEBUG
fprintf(stderr, "continue: p[%d].endTime = %d\n", na-1, p[na-1].endTime);
#endif
		}
		p[na].strt = 0;
	}
	else {		
		if (na) p[na-1].more = 0;
		p[na].strtTime = a->atime;
		p[na].endTime = a->atime + dt;
#ifdef DEBUG
fprintf(stderr, "first: p[%d].endTime = %d\n", na, p[na].endTime);
#endif

		/*
		 * set up index to start for nrindx
		 */
		p[nrindx[chan]++].strt = na;
	}
	naindx[chan]++;
#ifdef DEBUG
fprintf(stderr, "nrindx[0] = %d, nrindx[1] = %d, nrindx[2] = %d, nrindx[3] = %d\n",
nrindx[0], nrindx[1], nrindx[2], nrindx[3]);
#endif
}

/*
 * load index of trial start events
 * k_ev is the index of the trial start code
 * b_ev is the index of the first code in the trial
 * b_ev can be lower that k_ev when a pretime is specified
 */
PRIVATE void loadTrial(int k_ev, int b_ev)
{
	int ix;
	int jx;
	Trial * t;

	if (numTrials >= ltrials) {
		ltrials += 1024;
		trials = (Trial *) rexAlloc((void *) trials,
			sizeof(Trial), ltrials, "trials");
	}

	ix = (numTrials ? k_ev : 0);	/* get index of trial start code to use */

	jx = (numTrials ? b_ev : 0);	/* get index of first event code to use */

	t = &trials[numTrials++];

	t->events = ix;
	t->firstEvent = jx;
	t->tStartTime = evbuf[k_ev].e_key - preTime;
	t->recNum = 0;
}

/*
 * back up the record pointer for preTime > 0
 */
PRIVATE int backup(int indx, EVENT *p)
{
	int i, lotime;

	lotime = p->e_key - preTime;

	for (i = indx; i > 0; --i, --p) {
		if ((p->e_key < lotime) && (p->e_code > 0)) break;
	}
	i++;
	p++;
			
	/* if the ecode of this event is negative, move forward */
	while (p->e_code < 0) {
		i++;
		p++;
	}
	return(i);
}

/*
 *	Scan events
 */
PRIVATE void escan(int indx, EVENT *p, FILE * outstrm)
{
	int code, u;

	outstrm = outstrm;
	code = p->e_code;

	u = code - UNIT1CD;
	if ((0 <= u) && (u < rexMaxUnits)) {
		nubuf[u]++;		
	}
	else if (code == startCode) {
		if (preTime) loadTrial(evnum, backup(indx, p));
		else loadTrial(evnum, evnum);
	}
	edone = 0;
}

/*
 *	Scan A file entries.
 */
PRIVATE void ascan(int k_ev, FILE *outstrm)
{
	ANALOGHDR *a = &ahd;
	EVENT *ev = &evbuf[k_ev];
	int err= 0;

	if (fa == NULL) return;

	if (rexGetAnalogHeader(ev, a)) {
		error_M;
		fprintf(outstrm, "Analog file stops short in header.\n");
	}

	switch(a->acode) {
	case SAMPHDRCD:			/* sample header */
		loadSampHdr(a);
		if (rexMaxSampRate > 1000) {
			abufTfac = (2000/rexMaxSampRate)/(2.0 * sampHdr.sa_signum);
		}
		else {
			abufTfac = (1000/rexMaxSampRate)/(2.0 * sampHdr.sa_signum);
		}
#ifdef DEBUG
fprintf(stderr, "ascan(): rexMaxSampRate = %d, sampHdr.sa_signum = %d, abufTfac = %d\n",
rexMaxSampRate, sampHdr.sa_signum, abufTfac);
#endif
		return;
		break;
	case EYEHCD:			/* h-buffer */
		if(a->acontinue) ac110++;
		else a110++;
		break;
	case EYEVCD:			/* v-buffer */
		if(a->acontinue) ac111++;
		else a111++;
		break;
	case ADATACD:			/* single buffer */
		if (a->acontinue) ac112++;
		else a112++;
		break;
	}

	/*
	 *	Print A file entry info?
	 */
	if(err) {
		if(!edone) rexEprint(ev, outstrm);
		else fprintf(outstrm, "\n");
		rexAprint(ev, outstrm, 1);
	}
	else {
		sv_key(k_ev, a);
	}
}

/*
 * Assign analog records to trials.
 * algorithm:
 * 	Scan the a-codes for the analog record that starts before the open code,
 *	and ends after the close code.
 * 	
 */
PRIVATE void findAnalog(int chan, Trial * t)
{
	int j;
	int n;
	int ix;
	int pre, post;
	AIX *a, *b;
	int32_t tLo;
	int32_t tHi;
	int32_t aEnd;
	int32_t aStart;
	static int lo[rexNchannels];

	if (t == trials) {
		for (j = 0; j < rexNchannels; j++) lo[j] = 0;
	}

	tLo = t->tStartTime;
	tHi = t->tEndTime;

	b = aindx[chan];
	n = nrindx[chan];

#ifdef DEBUG
fprintf(stderr, "findAnalog(): lo[%d] = %d, n = %d\n", chan, lo[chan], n);
#endif
	for (j = lo[chan]; j < n; j++) {
		ix = b[j].strt;
		a = &b[ix];
		aStart = a->strtTime;
		aEnd = a->endTime;

#ifdef DEBUG
fprintf(stderr, "\t%d: tLo = %d, tHi = %d, aEnd = %d, aStart = %d\n",
j, tLo, tHi, aEnd, aStart);
#endif

		if (aEnd <= tLo) continue;	/* record too early */
		if (aStart > tHi) break;	/* record too late */
		pre = abs(tHi - aStart);
		post = abs(aEnd - tHi);
		/*
fprintf(stderr, "%d: tHi = %d, aStart = %d, aEnd = %d, pre = %d, post = %d\n", index2RecNum(j),
tHi, aStart, aEnd, pre, post );
		*/
		/*		if (post > pre) break;	*/	/* too little in window */


		t->recNum = index2RecNum(j);
#ifdef DEBUG
fprintf(stderr, "\t\trecNum[%d] = %d\n", j, t->recNum);
#endif
		t->aStartTime = aStart;
		lo[chan] = j + 1;
		break;
	}
#ifdef DEBUG
fprintf(stderr, "\t recNum = %d, aStart = %ld, aEnd = %ld\n", t->recNum, aStart, aEnd);
#endif
}

/* reorder an event by moving it back in the list */
PRIVATE void percolate(register EVENT *buf, register EVENT *a, register EVENT *b) {
	register EVENT t;
	register EVENT *c;

	/* find the place in line one below the new place for event b */
	for (c = a-1; c > buf; c--) {
		if (c->e_code < 0) continue;
		if (b->e_key >= c->e_key) break;
	}

	t = *b;	/* hold */

	/* move all events up one */
	while (a > c) {
		*b = *a;
		b = a--;
	}

	a[1] = t;	/* restore */
}
	 
PRIVATE void esort(EVENT *buf, int n)
{
	register EVENT *a, *b, *last;

	last = &buf[n];

	/* get first non-Analog event */
	for (a = buf; a < last; a++) {
		if (a->e_code > 0) break;
	}

	/* swap out-of-order non-Analog events */
	for (b = a+1; b < last; b++) {
		if (b->e_code > 0) {
			if (b->e_key < a->e_key) percolate(buf, a, b);
			a = b;
		}
	}
}

/*
 * match trial start code and window open codes
 */
PRIVATE void assignRecords(void)
{
	int i, chan;

	for (chan = 0; chan < rexNchannels; chan++) if (nrindx[chan]) break;
#ifdef DEBUG
fprintf(stderr, "assignRecords(): chan = %d, rexNchannels = %d\n", chan, rexNchannels);
#endif
	for (i = 0; i < numTrials; i++) {
		findAnalog(chan, &trials[i]);
	}
}

/*
 * match trial times and unit times
 */
PRIVATE void assignUnits(void)
{
	register int loT, hiT;
	register int32_t *p;
	Trial *t;
	RexUnits *r;
	register int u, i, j, k, n, m;

	/* get list of existing units
	 */
	for (nUnits = u = 0; u < rexMaxUnits; u++) if (nubuf[u]) {
		unitsU[nUnits] = u;
		unitCodes[nUnits] = u + UNIT1CD;
		unitsL[nUnits] = 0;
		nUnits++;
	}
	if (nUnits == 0) return;

	/* save list
	 */
	rexInfo.nUnits = nUnits;
	rexInfo.unitCodes = unitCodes;

	/* assign by times
	 */
	for (i = 0; i < numTrials; i++) {
		t = &trials[i];
		r = &t->rexunits;

		/* allocate space to hold unit pointers
		 */
		r->nTimes = (int32_t *) rexAlloc((void *) NULL,
			sizeof(int32_t), nUnits, "r->nTimes");
		r->unitTimes = (int32_t **) rexAlloc((void *) NULL,
			sizeof(int32_t *), nUnits, "r->unitTimes");

		loT = t->tStartTime;

		if (i < (numTrials - 1)) {
			hiT = trials[i+1].tStartTime + preTime;
		}
		else {
			hiT = t->tEndTime + 1000;	/* be sure to get last spikes */
		}

		for (m = 0; m < nUnits; m++) {
			u = unitsU[m];
			p = ubuf[u];
			n = nubuf[u] - 1;

			k = unitsL[m];

			/* find first unit with time >= trial start time
			 */
			while ((p[k] > loT) && (k > 0)) --k;	/* back up */
			while ((p[k] < loT) && (k < n)) k++;	/* search */
			r->unitTimes[m] = &p[k]; /* save pointer to start */
			if (p[k] >= hiT) {
				r->nTimes[m] = 0;
				continue;
			}

			/* count to end
			 */
			for (j = k + 1; ((p[j] < hiT) && (j < n)); j++);
			j--;	/* correct to last one in trial */

			if (p[j] > t->tEndTime) t->tEndTime = p[j];

			r->nTimes[m] = j - k + 1; /* save number */

			/* save last index
			 */
			unitsL[m] = j;			
		}
	}
}

/*
 * match e-codes and a-records
 */
PRIVATE void procTrials(void)
{
	int n;
	register Trial *t, *tn;
	register EVENT *p;

	if (numTrials == 0) return;	/* bad news file */

	/*
	 * get end times and number of events for trials
	 */
	t =  &trials[0];
	tn = &trials[1];
	for (n = 1; n < numTrials; n++) {
		t->nEvents = tn->events - t->firstEvent;

		p = &evbuf[tn->events - 1];
		while((p > evbuf) && (p->e_code < 0)) {
			--p; /* be sure is not an analog event */
		}

		t->tEndTime = p->e_key;
		t = tn++;
	}


	/* do last trial
	 */
	t->nEvents = nevbuf - t->firstEvent;

	p = &evbuf[nevbuf - 1];
	while((p > evbuf) && (p->e_code < 0)) {
		--p; /* be sure is not an analog event */
	}

	t->tEndTime = p->e_key;
		
	/* first, assign analog records to trials
	 */
	assignRecords();

	/* second, assign units to trials with full times
	 */
	if (rexSplit) {
		assignUnits();
	}
}

PRIVATE int getNumRecs(void)
{
	int i, num_rec;

	num_rec = nrindx[0];
	for (i = 1; i < rexNchannels; i++) {
		if (num_rec < nrindx[i]) num_rec = nrindx[i];
	}
#ifdef DEBUG
fprintf(stderr, "num_rec = %d\n", num_rec);
#endif
	return (num_rec);
}

/*
 * load event into split event buffer
 */
PRIVATE int loadEbuf(EVENT *ev)
{
	evnum++;

	if (nevbuf >= levbuf) {
		levbuf += 10240;
		evbuf = (EVENT *) rexAlloc((void *) evbuf,
			sizeof(EVENT), levbuf, "evbuf");
	}

	evbuf[nevbuf] = *ev;
	return(nevbuf++);
}

/*
 * load unit time into buffer
 */
PRIVATE void loadUbuf(int code, int time)
{
	register int u, n;

	u = code - UNIT1CD;		/* get index */
	n = nubuf[u]++;
	
	if (n >= lubuf[u]) {
		lubuf[u] += 512;
		ubuf[u] = (int32_t *) rexAlloc((void *) ubuf[u],
			sizeof(int32_t), lubuf[u], "ubuf");
	}
	ubuf[u][n] = time;
}

/*
 * Scan E and A file.
 * load events into in-core buffer
 * if no E-file, in-core buffer from A-file headers
 */
PRIVATE void scanFile(FILE *outstrm)
{
	EVENT *ev;
	EVENT *ebuf = NULL;
	ANALOGHDR ahb;
	int32_t code;
	int32_t first;
	int32_t last;
	int umin;
	int umax;
	int numEvent;
	int ebufSize;
	int nRead;
	int i;
	int j;
	int k;

	if (fa) fseek(fa, 512L, SEEK_SET);

	sampHdr.sa_signum = 2;	/* use two signals/channel for -110/-111 codes */
	abufTfac = (1000/rexMaxSampRate)/(2.0 * sampHdr.sa_signum);
	a110 = ac110 = a111 = ac111 = a112 = ac112 = 0;
	evnum = anum = -1;
	for (i = 0; i < rexMaxUnits; i++) {
		nubuf[i] = 0;
	}
	numTrials = 0;

	/* load e-file into memory.
	 * determine number of events
	 */
	if (fe){
		fseek(fe, 0L, SEEK_END);
		last = ftell(fe);	/* pointer to last event + 1 */
		fseek(fe, 512L, SEEK_SET);
		first = ftell(fe);	/* pointer to first event */
		numEvent = ((last - first) / sizeof(EVENT)) - 1;
	}
	else {
		numEvent = 0;
		while (fread(&ahb, sizeof(ANALOGHDR), 1, fa) == 1) {
			++numEvent;
			fseek(fa, ahb.alength, SEEK_CUR);
		}
	}

	/*
	 * allocate space
	 */
	ebufSize = numEvent * sizeof(EVENT);
	ebuf = (EVENT *)malloc(ebufSize);
	if(ebuf == (EVENT *)NULL) {
		fprintf(stderr, "can't malloc %d bytes for ebuf\n", ebufSize);
		i = *nullPtr;
	}

	/*
	 * read in all the events from the E-file
	 */
	if (fe) {
		first = ftell(fe);
		for(i = 0; i < numEvent; ++i) {
			nRead = fread(&ebuf[i], sizeof(EVENT), 1, fe);
			if (nRead != 1) {
				fprintf(stderr, "can't read %d events from E-file\n", numEvent);
				i = *nullPtr;
			}
		}

		/*
		 * sort into ascending times, as REX is not always sequential
		 */
		esort(ebuf, numEvent);
	}
	else { 
		fseek(fa, 512L, SEEK_SET);
		numEvent = 0;
		first = ftell(fa);	
		while (fread(&ahb, sizeof(ANALOGHDR), 1, fa) == 1) {
			ebuf[numEvent].e_code = -112;
			ebuf[numEvent].e_key = first;
			ebuf[numEvent].e_seqnum = numEvent++;
	    
			fseek(fa, ahb.alength, SEEK_CUR);
			first = ftell(fa);	
		}
	}

	/*
	 * build data tables
	 */
	if (rexSplit) {	/* construct split control & unit buffers */
		umin = UNIT1CD;
		umax = UNIT1CD + rexMaxUnits - 1;

		/* Code Sieve to split out units
		 */
		for (i = 0, ev = ebuf; i < numEvent; i++, ev++) {
			code = ev->e_code;

			if ((umin <= code) && (code <= umax)) {	/* is a unit */
				loadUbuf(code, ev->e_key);
			}
			else if((code == STDHDRCD) ||
					(code == SAMPHDRCD) ||
					(code == EYEHCD) ||
					(code == EYEVCD) ||
					(code == ADATACD)) { /* is an analog event */
				anum++;
				ascan(loadEbuf(ev), outstrm);
				if(fe == NULL) {
					loadTrial(i, i);
				}
			}
			else if(code < 0) {
				fprintf(stderr, "Warning: invalid ecode %d\n", code);
			}
			else if (code == startCode) {
				j = loadEbuf(ev);

				if (preTime) k = backup(j, &evbuf[j]);
				else k = j;

				loadTrial(j, k);
			}
			else {
				loadEbuf(ev);
			}
		}
	}
	else {			/* use single event buffer */
		/*
		 * Sort events into buffers and tables.
		 * Step down E buffer.  When
		 * an event references an A file entry, then check that A
		 * file header.  Also maintain an independent check of the
		 * A file to make sure its record sizes are correct and that
		 * it has no entries not indexed by the E file.
		 */
		levbuf = numEvent;
		evbuf = ebuf;
		for (nevbuf = 0, ev = evbuf; nevbuf < numEvent; nevbuf++, ev++) {
			/*
			 *	Check event
			 */
			evnum++;
			escan(nevbuf, ev, outstrm);

			/*
			 *	check type of event
			 */
			if((code == STDHDRCD) ||
			   (code == SAMPHDRCD) ||
			   (code == EYEHCD) ||
			   (code == EYEVCD) ||
			   (code == ADATACD)) { /* is an analog event */
				anum++;
				ascan(evnum, outstrm);
				if(fe == NULL) {
					loadTrial(nevbuf, nevbuf);
				}
			}
		}

	}
	numRec = getNumRecs();

	/*
	 * process trial table
	 */
	procTrials();
}

/*
 * find current record
 */
PRIVATE void getRecord(int chan, int recNum)
{
	int i;
	int indx, fadata;
	int npts;

	if (fa == NULL) return;

	indx = aindx[chan][recNum2Index(recNum)].strt;	/* convert to index */

	nadata = 0;
	fadata = ladata;
	aStartTime = 0;
	do {
		/*
		 *	Read A file header.
		 */
		fseek(fa, aindx[chan][indx].key, 0);
                fread(&ahd, sizeof(ANALOGHDR), 1, fa);
		if (aStartTime == 0) {
			aStartTime = ahd.atime;
		}

		if (ahd.alength < 0) fprintf(stderr, "UH-OH:  negative length! (%d) (%d) \n", ahd.alength, (unsigned short) ahd.alength);
		if (ahd.alength & 01) fprintf(stderr, "uh-oh, odd number of bytes (%d)!\n", ahd.alength);

		npts = ((unsigned short)ahd.alength) / 2;
		if (npts < 0) fprintf(stderr, "UH-OH:  negative npts! (%d) \n", npts);

		/*
		 *	Read analog data
		 */
		if (npts >= fadata) {
			char buf[64];

			ladata += npts + 1000;
			sprintf(buf, "adata[%d]", ladata);
			adata = (short int *)
				rexAlloc((void *) adata, sizeof(*adata), ladata, buf);
		}
		if (fread(&adata[nadata], sizeof(*adata), npts, fa) != npts) {
			fprintf(stderr, "A-file record too short!\n");
			i = *nullPtr;
		}
		nadata += npts;
		fadata = ladata - nadata;
#ifdef DEBUG
	fprintf(stderr, "getRecord(): nadata = %d, npts = %d, fadata = %d, ladata = %d\n", nadata, npts, fadata, ladata);
#endif
	} while(aindx[chan][indx++].more);
}

/*
 * check that times in the aindx list match for -110 and -111
 * codes.  delete any unmatched records
 */
PRIVATE void chk_times()
{
	int i, j, delta;
	int time;
	AIX *a, *b;

	if (nrindx[0] == 0 || nrindx[1] == 0) return;

	a = aindx[0];
	b = aindx[1];
	for (i = 0; i < nrindx[0]; i++) {
		if (a[i].strtTime == b[i].strtTime) continue;
		time = a[i].strtTime;

		/*
		 * make times match
		 */
		for (delta = 1; (delta+i) < nrindx[1]; delta++) {
			if (time == b[i+delta].strtTime) {
				/*
				 * delete unmatched chan 1 record
				 */
				for (j = i; (delta+j) < nrindx[1]; j++) {
					b[j] = b[j+delta];
				}
				nrindx[1] -= delta;
				break;
			}
			if (a[i+delta].strtTime == b[i].strtTime) {
				/*
				 * delete unmatched chan 0 record
				 */
				for (j = i; j < (nrindx[0]-delta); j++) {
					a[j] = a[j+delta];
				}
				nrindx[0] -= delta;
				break;
			}
		}
	}
	if (nrindx[0] != nrindx[1]) {
		fprintf(stderr, "number of -110 (%d) and -111 (%d) codes do not match!\n",
				nrindx[0], nrindx[1]);
	}
}

PRIVATE void loadSignal(int sig, int t)
{
	int m;

	m = mpts[sig]++;

	if (m >= lsignal[sig]) {
		char buf[64];

		lsignal[sig] += 1000;
		sprintf(buf, "signals[%d][%d/%d]", sig, m, lsignal[sig]);
		signals[sig] = (RexSignal *)
			rexAlloc((void *) signals[sig], sizeof(RexSignal), lsignal[sig], buf);
	}
	signals[sig][m] = t;
}

/*
 * do power of 2 interpolations
 */

/*
 * halfband-width filter, rescaled to have DC gain = 1
 */
static double halfbandfir[] = {
/*
 * 31 point halfband-width filter, rescaled to have DC gain = 1
	-2.1070728e-03,
	 1.9369840e-06,
	 4.6527505e-03,
	-7.3819809e-07,
	-9.4344543e-03,
	 2.8782184e-06,
	 1.7227786e-02,
	-2.0418261e-06,
	-2.9818008e-02,
	 5.1758984e-06,
	 5.1590234e-02,
	-4.1410686e-06,
	-9.8547111e-02,
	 4.7913256e-06,
	 3.1609809e-01,
	 5.0065984e-01
*/

/* 11 point halfband-width filter, rescaled to have DC gain = 1 */
	9.7241428e-03,
	3.9088155e-03,
	-5.8393549e-02,
	-1.0232324e-02,
	2.9840673e-01,
	5.1317237e-01
};

#define HLFPTS 	(sizeof(halfbandfir)/sizeof(*halfbandfir))
static IeeeFir filt = {
	"halfband",
	1.0,
	HLFPTS,
	(2 * HLFPTS - 1),
	0,
	halfbandfir
};

PRIVATE float *rawBuf = 0;
PRIVATE float *filtBuf = 0;
PRIVATE int lbuf = 0;

/* Perform Interpolation
 * on signals in sl, by a factor of Lfac (e.g., 2, 4, 8)
 */
PRIVATE void doInterp(SignalList *sl, int Lfac)
{
	int i, j, m, n, L;
	float *p, *f, *startRaw, *startFilt;
	RexSignal *s;

	m = sl->npts;

	L = Lfac;
	n = L * m + ieeeFirMax;

	if (n >= lbuf) {
		lbuf = n + 1024;

		rawBuf = (float *) rexAlloc((void *) rawBuf,
			sizeof(float), lbuf, "rawBuf");

		filtBuf = (float *) rexAlloc((void *) filtBuf,
			sizeof(float), lbuf, "filtBuf");
	}

	startRaw = &rawBuf[ieeeFirOff];
	startFilt = &filtBuf[ieeeFirOff];

	/*
	 * load
	 */
	s = sl->signal;
	p = startFilt;
	for (j = 0; j < m; j++) {
		*p++ = *s++;
	}

	/*
	 * interpolate up by 2
	 */
	for (; L > 1; L /= 2) {
		f = startFilt;	/* holds data to be interpolated in loop */
		p = startRaw;
		for (j = 0; ; ) {
			*p++ = *f++;
			if (++j == m) break;		/* don't address s[m] */
			*p++ = (*f + f[-1]) / 2;	/* should be zero, but smoother this way! */
		}
		*p = p[-1] + (p[-1] - p[-2]);	/* repeat slope for last point */
		m *= 2;
		do_ieee_fir(startRaw, startFilt, m, &filt);	
	}

	/*
	 * restore
	 */
	for (i = 0; i < rexMaxSignals; i++) if (sl->signal == signals[i]) break;
	mpts[i] = 0;
	p = startFilt;
	for (j = 0; j < m; j++) {
		loadSignal(i, rexSignalRound(*p++));
	}
	sl->npts = m;
#ifdef DEBUG
fprintf(stderr, "doInterp(): storeRate = %d, adRate = %d, Lfac = %d\n", sl->storeRate, sl->adRate, Lfac);
#endif

/*	sl->adRate *= Lfac;		*/	/* correct sampling rate */
	sl->adRate = Lfac * sl->storeRate;	/* correct sampling rate */
}

/*
 * unpacking for -110 and -111 codes.  Uses high 4 bits of
 * each data word for channel
 */
PRIVATE void unpackDbl(int chan)
{
	short int t;
	short int *p;
	int i, sig;

	p = adata;
	for (i = 0; i < nadata; i++) {
		t = *p++;
		sig = chan * MASK_N + getBufChan(t);

		/*
		 * clear mask bits
		 */
		if (t > 0) t &= MASK_12;
		else if (t < 0) t |= ~MASK_12;

		loadSignal(sig, t);
	}
}

/*
 * upacking for -112 codes.  Uses frame buffer array to
 * keep track of channels, so 16-bit data can be in buffer
 */
PRIVATE void unpackSngl(void)
{
	short int *p, *lp;
	short int *frame;
	int sig;
	int rcomp, z;

	p = adata;
	lp = &adata[nadata];
	rcomp = sampHdr.sa_ad_rcomp;	/* radix compensation for 2's complement */
	do {
		for (frame = sa_frame; *frame != SA_ARRAY_TERM; frame++) {
			for (frame++; *frame != SA_SUBFR_TERM; frame++) {
				if (*frame & SA_ACQUIRE_ONLY) continue;
				sig = *frame & SA_SIGMASK;	/* low byte is signal number */

				if (*frame & SA_MEMSIG) z = *p++;
				else z = (*p++ - rcomp);

				loadSignal(sig, z);
			}
		}
	} while(p < lp);
}

void
fake_names(int i, int k)
{
	char *name = (char *)NULL;
	char *label = (char *)NULL;

	if (k < MASK_N) switch(k) {
	case 0:
 		name = "h0";
		label = "h0";
		break;
	case 1:
		name = "h1";
		label = "h1";
		break;
	case 2:
		name = "h2";
		label = "h2";
		break;
	case 3:
		name = "h3";
		label = "h3";
		break;
	}
	else if (k < 2 * MASK_N) switch(k - MASK_N) {
	case 0:
		name = "v0";
		label = "v0";
		break;
	case 1:
		name = "v1";
		label = "v1";
		break;
	case 2:
		name = "v2";
		label = "v2";
		break;
	case 3:
		name = "v3";
		label = "v3";
		break;
	}
	else {
		name = "";
		label = "";
	}
	signalList[i].sigName = name;
	signalList[i].sigLabel = label;
}


/*
 * check for extra points
 */
PRIVATE void chknPoints()
{
	SignalList *sl;
	int n;

	n = nPoints;
	rexSignalLoop(sl) {
		if (sl->npts == (n - 1)) n--;
	}
	if (n < nPoints) {
		rexSignalLoop(sl) {
			if (sl->npts > n) sl->npts = n;
		}
		nPoints = n;
	}
}

/*
 * process signals for interpolation,
 * and set up public signal structure
 */
PRIVATE void procSigs(int interpolate)
{
	int m, n;
	int mxRate = 0;		/* max adRate */
	SignalList *sl, *s;
	extern int32_t first_pass;

	/*
	 * get signal indices
	 */
	nSignals = 0;
	for (m = 0; m < rexMaxSignals; m++) {
		if (mpts[m] == 0) continue;

		if (nSignals) signalList[nSignals - 1].next = 
						&signalList[nSignals];

		signalList[nSignals].signal = signals[m];
		signalList[nSignals].npts = mpts[m];
		signalList[nSignals].index = m;
		signalList[nSignals++].next = NULL;
#ifdef DEBUG
fprintf(stderr, "signalList[%d].npts = %d\n", nSignals, signalList[nSignals].npts);
#endif

	}

	/*
	 * set max number of data points
	 */
	nPoints = 0;
	rexSignalLoop(sl) {
		if (nPoints < sl->npts) nPoints = sl->npts;
	}
	chknPoints();

	/*
	 * INTERPOLATION
	 */
	if (interpolate) {
		rexSignalLoop(sl) {
			int L;

			if (sl->npts == nPoints) {
				sl->adRate = sa_store_rate[sl->index];
				continue;
			}

			L = (nPoints + (sl->npts - 1)) / sl->npts;
			doInterp(sl, L);

			/* reset pointer in case realloc moved it */
			sl->signal = signals[sl->index];
		}

		/*
		 * reset max number of data points
		 */
		rexSignalLoop(sl) {
			if (nPoints < sl->npts) sl->npts = nPoints;
		}
		chknPoints();
	}

	/*
	 * load count information
	 */
	n = 0;
	rexSignalLoop(sl) {
		sl->count = n++;
	}

	/*
	 * get channel names, numbers, rates and scales
	 */
	if (first_pass && nPoints) {
		int i, j, k;
		float q, z;

		first_pass = 0;

		q = (1 << sampHdr.sa_ad_res);
		for (i = 0; i < rexMaxSignals; i++) {
			s = &signalList[i];
			k = s->index;
			if (sampHdr.sa_maxsig) {
				if (i >= sampHdr.sa_signum) break;
				s->sigName = sa_gvname[k];
				s->sigLabel = sa_title[k];

				if (!interpolate || s->adRate <= 0) { /* ignore if interpolated */
					s->adRate = sa_store_rate[k];
				}

				s->storeRate = sa_store_rate[k];
				s->sigChan = sa_ad_chan[k];

				k = sa_ad_calib[i];
				s->sigGain = k;

				if (k < 0) {
					s->fscale = 102.4;
					s->scale = s->fscale / (1 << 12);	/* default to 12-bit */
				}
				else {
					if (sa_calibst[k][0] == ':') {
						int n;
						char tmp;
						char *str;

						fprintf(stderr, "Warning:  byte-swap error in calibration string: %s\n", sa_calibst[k]);
						str = sa_calibst[k];
						n = strlen(str) - 1;
						for (j = 0; j < n; j += 2, str += 2) {
							tmp = str[0];
							str[0] = str[1];
							str[1] = tmp;
						}
						fprintf(stderr, "Warning:  swapped to be: %s\n", sa_calibst[k]);
					}
					sscanf(sa_calibst[k], "%d: %f", &j, &z);
					s->fscale = z;
					s->scale = s->fscale / q;
				}
			}
			else {
				fake_names(i, k);
				s->fscale = 102.4;
				s->scale = s->fscale / (1 << 12);
				s->adRate = rexMaxSampRate;
				s->storeRate = rexMaxSampRate;
				s->sigChan = k;
			}
			if (s->adRate > mxRate) mxRate = s->adRate;
		}
		rexMaxSampRate = mxRate;
	}

	/*
	 * determine last time in this record
	 */
#ifdef DEBUG
fprintf(stderr, "procSigs(): rexMaxSampRate = %d\n", rexMaxSampRate);
#endif
	aEndTime = (aStartTime + ((nPoints * 1000.0) / rexMaxSampRate)) - 1;
	if (aEndTime < aStartTime) aEndTime = aStartTime;
}

/*
 * ----------------------  PUBLIC FUNCTIONS --------------------------------
 */

/*
 * this function sets up the use of separate buffers for
 * control and unit E-codes.  It must be called before rexFileOpen().
 */
void rexSplitEvents(void)
{
	rexSplit = 1;
}

/*
 * returns 0 if file name is bad,
 * otherwise returns numTrials, the number of Trials begun by startCode
 *
 */
int rexFileOpen(const char *f, int maxRate, int startCd, int preTm)
{
	char buf[256];
	short int *psi;
	int fileLen;

	startCode = startCd;
	preTime = preTm;
	fileLen = strlen(f);

	// allocate space for the E and A file names
	// add extra space to make sure there are no overflows
	efilen = (char *)malloc(fileLen + 16);
	memset(efilen, (int)'\0', fileLen + 16);
	afilen = (char *)malloc(fileLen + 16);
	memset(afilen, (int)'\0', fileLen + 16);

	// build the event file name
	strncpy(efilen, f, fileLen);
	strcat(efilen, "E");
	// build the analog file name
	strncpy(afilen, f, fileLen);
	strcat(afilen, "A");

	if (fe) fclose(fe);
	if (fa) fclose(fa);

	fe = fopen(efilen, "rb");
	fa = fopen(afilen, "rb"); /* NOTE: will be NULL if no A-file */

	if ((fa == NULL) && (fe == NULL)) {
		return(0);
	}

	/*
	 * read in headers
	 */
	if (fe) {
		if (fread(eHeader, 1, 512, fe) != 512)  {
			sprintf(buf, "%s is too short!\n", efilen);
			errorMsg(buf);
			return(0);
		}
	}
	if (fa) {
		if (fread(aHeader, 1, 512, fa) != 512)  {
			sprintf(buf, "%s is too short!\n", afilen);
			errorMsg(buf);
			return(0);
		}
		/* a hdr rec size is in the first byte only if not swapped */
		psi = (short int *) aHeader;
		arsize = *psi;
		if (arsize != sizeof(ANALOGHDR)) {
			sprintf(buf, "analog header size is %d, but should be %d!\n",
					arsize, sizeof(ANALOGHDR));
			errorMsg(buf);
			return(0);
		}
	}

	/*
	 * check file for integrity, and load buffers
	 */
	scanFile(stderr);

	chk_times();	/* be sure times line up */

	rexInfo.numRec = numRec;	/* save */
	rexInfo.numTrials = numTrials;
	rexInfo.ahdr = &aHeader[2];
	rexInfo.ehdr = &eHeader[2];
	rexInfo.maxSampRate = rexMaxSampRate;
	rexInfo.startCode = startCode;

	rexGetTrial(min_(2, numRec), 0);	/* reload the max a2d Rate */

	return(numTrials);
}

/*
 * close any open rex files and clear malloc() memory & pointers
 */
void rexFileClose(void)
{
	int i;

	if (fa) fclose(fa);
	if (fe) fclose(fe);
	fe = fa = NULL;

	rexInfo.startCode = 0;

	/* first, free all the allocated memory */
	clrPtrs();

	/* clear the file name arrays */
	if(efilen != (char *)NULL) {
		free(efilen);
		efilen = (char *)NULL;
	}
	if(afilen != (char *)NULL) {
		free(afilen);
		afilen = (char *)NULL;
	}

	/* second, NULL all the pointers to that memory */
	for (i = 0; i < rexNchannels; i++) {
		aindx[i] = NULL;
		laindx[i] = 0;
		naindx[i] = 0;
		nrindx[i] = 0;
	}

	evbuf = NULL;
	levbuf = 0;
	nevbuf = 0;

	trials = NULL;
	ltrials = 0;
	numTrials = 0;

	adata = NULL;
	ladata = 0;
	nadata = 0;

	for (i = 0; i < rexMaxUnits; i++) {
		ubuf[i] = NULL;
		lubuf[i] = 0;
		nubuf[i] = 0;
		unitCodes[i] = 0;
		unitsU[i] = 0;
		unitsL[i] = 0;
	}
	nUnits = 0;

	for (i = 0; i < rexMaxSignals; i++) {
		mpts[i] = 0;
		signals[i] = NULL;
		lsignal[i] = 0;
		signalList[i].next = NULL;

	}
	nPoints = 0;
	nSignals = 0;

	rawBuf = NULL;
	filtBuf = NULL;
	lbuf = 0;

	sampHdr.sa_maxsig = 0;

	first_pass = 1;	/* restore local flag */
}

/*
 * load signals into data buffers
 */
RexInfo *rexGetSignals(int recNum, int interpolate)
{
	int i, c;

#ifdef DEBUG
	fprintf(stderr, "rexGetSignals(%d)", recNum);
#endif

	if ((1 <= recNum) && (recNum <= numRec)) {
#ifdef DEBUG
		fprintf(stderr, "rexGetSignals(): recNum = %d, numRec = %d\n", recNum, numRec);
#endif
		/*
		 * clear counters
		 */
		for (i = 0; i < rexMaxSignals; i++) mpts[i] = 0;

		for (c = 0; c < rexNchannels; c++) {
#ifdef DEBUG
			fprintf(stderr, "rexGetSignals():  nrindx[%d] = %d\n", c, nrindx[c]);
#endif
			if (nrindx[c] == 0) continue;
			getRecord(c, recNum);
			if (c < 2) unpackDbl(c);
			else unpackSngl();
		}
		/*
		 * process signals
		 */
		procSigs(interpolate);

		/*
		 * pick up relevant events
		 */
	}
	else nSignals = 0;

	/*
	 * reset values if no analog records
	 */
	if (nSignals == 0) {
		nPoints = 0;
		aStartTime = 0;
		aEndTime = 0;
	}

	/*
	 * load record info structure
	 */
	rexInfo.signalList = signalList;
	rexInfo.nSignals = nSignals;
	rexInfo.nPoints = nPoints;
	rexInfo.aStartTime = aStartTime;
	rexInfo.aEndTime  = aEndTime;
	rexInfo.ad_res = (sampHdr.sa_ad_res ? sampHdr.sa_ad_res : 12);

	rexInfo.maxSampRate = rexMaxSampRate;

	return(&rexInfo);
}

/*
 * load events rexinfo buffer
 */
RexInfo *rexGetEvents(int trialNum)
{

#ifdef DEBUG
fprintf(stderr, "rexGetEvents(%d)\n", trialNum);
#endif

	if (trialNum < 1 || trialNum > numTrials) return(NULL);

	getTr = &trials[trialNum - 1];

	rexInfo.signalList = NULL;
	rexInfo.nSignals = 0;

	rexInfo.events = &evbuf[getTr->events];
	rexInfo.nEvents = getTr->nEvents;
	rexInfo.rexunits = &getTr->rexunits;

 	rexInfo.tStartTime = getTr->tStartTime;
	rexInfo.tEndTime = getTr->tEndTime;

	return(&rexInfo);
}

/*
 * load analog buffer
 * must be called after a call to rexGetEvents,
 * which sets "getTr"
 */
RexInfo *rexGetAnalog(int interpflag)
{
#ifdef DEBUG
fprintf(stderr, "rexGetAnalog(): getTr->recNum = %d\n", getTr->recNum);
#endif
	if (getTr->recNum) {
		rexGetSignals(getTr->recNum, interpflag);

		/* check times for window width
		 */
		if (rexInfo.aStartTime < getTr->tStartTime) {
			rexInfo.tStartTime = getTr->tStartTime = rexInfo.aStartTime;

		}

		if (rexInfo.aEndTime > getTr->tEndTime) {
			rexInfo.tEndTime = getTr->tEndTime = rexInfo.aEndTime;
		}
	}

	return(&rexInfo);
}

/*
 * load signals into data buffers
 */
RexInfo *rexGetTrial(int trialNum, int interpolate)
{
	if (rexGetEvents(trialNum) == NULL) return(NULL);
#ifdef DEBUG
fprintf(stderr, "rexGetTrial(%d) has events; getTr->recNum = %d\n", trialNum, getTr->recNum);
#endif
	rexGetAnalog(interpolate);


#ifdef DEBUG
	rexInfoPrint(&rexInfo, stderr);
#endif

	return(&rexInfo);
}

int rexGetUnitCount(int unitCode)
{
	int u;

	u = unitCode - UNIT1CD;
	if (u < 0 || u >= rexMaxUnits) return(0);
	return(nubuf[u]);
}

int rexGetAnalogHeader(EVENT *ev, ANALOGHDR *a)
{
	if (fa == NULL) return (1);

	/*
	 *	Read A file header.
	 */
	fseek(fa, ev->e_key, SEEK_SET);
	if(fread(a, sizeof(ANALOGHDR), 1, fa) != 1) return(1);
	else return(0);
}

/*
 * printing routines
 */
char *rexToolsVersion(void)
{
	return(rexToolsVerNum);
}

void rexTotalsPrint(FILE *outstrm)
{
	int i;

	fprintf(outstrm,"Totals:\tEvents: %ld\tAnalog (+ continues) %ld\n",
			evnum+1, anum+1);
	if (a110) fprintf(outstrm, "-110 records %d, conts %d\n", a110, ac110);
	if (a111) fprintf(outstrm, "-111 records %d, conts %d\n", a111, ac111);
	if (a112) fprintf(outstrm, "-112 records %d, conts %d\n", a112, ac112);

	for (i = 0; i < rexMaxUnits; i++) {
		if (nubuf[i]) fprintf(outstrm, "unit %d = %d\n",
							  (UNIT1CD + i), nubuf[i]);
	}
}

/*
 *	Print event.
 */
void rexEprint(EVENT *ev, FILE *outstrm)
{
	int code;
	static int32_t ltime1;

	code= ev->e_code;
	fprintf(outstrm, "\nEVENT: %u, expected seq num (unsign) %ld, (int32_t) %ld\n",
			ev->e_seqnum, evnum, evnum);
	fprintf(outstrm,"Type: ");
	if(code < 0) fprintf(outstrm,"analog, ");
	else if(code & INIT_MASK) {
		fprintf(outstrm,"init, ");
		code &= ~INIT_MASK;
	}
	else if(code & CANCEL_MASK) {
		fprintf(outstrm,"cancel, ");
		code &= ~CANCEL_MASK;
	}
	else if(code == UNIT1CD) fprintf(outstrm,"unit, ");
	else fprintf(outstrm,"event, ");
	fprintf(outstrm,"ecode value: (decoded decimal) %d, (actual octal) 0%o.\n",
			code, ev->e_code);
	if(code < 0) {
		fprintf(outstrm,"A file offset: %ld, in block %ld.\n",
				ev->e_key, (ev->e_key) / 512L);
	} else {
		rexTimePrint(ev->e_key, outstrm);
		fprintf(outstrm,"Diff: %ld\n", ev->e_key - ltime1);
		ltime1 = ev->e_key;
	}
}

/*
 *	Print A file info.
 */
void rexAprint(EVENT *ev, FILE *outstrm, int color)
{
	register ANALOGHDR *p = &ahd;

	if (color) fprintf(outstrm, "[1m");
	fprintf(outstrm,"A_HEADER:");
	if (color) fprintf(outstrm, "[0m");

	fprintf(outstrm,
		" %u, expected seq num (unsign) %u, (int32_t) %ld\n",
		p->aseqnum, loword_(anum), anum);
	fprintf(outstrm,
		"Corresponding E file sequence number: (unsign) %u, (int32_t) %ld\n",
		ev->e_seqnum, evnum);

	fprintf(outstrm,"Ecode %d, cont_flag %d, acount %ld, length %d\n",
		p->acode, p->acontinue, p->acount, p->alength);
	rexTimePrint(p->atime, outstrm);
}

/*
 * parse times
 */
char *rexTimeConv(int32_t time)
{
	int day, hour, mins, sec, msec;
	static char buf[64];

	msec= time % 1000;
	time /= 1000;
	sec= time % 60;
	time /= 60;
	mins= time % 60;
	time /= 60;
	hour= time % 24;
	day= time / 24;
	sprintf(buf, "%2d : %2d : %2d : %2d.%3d", day, hour, mins, sec, msec);
	return(buf);
}

/*
 *	Process long times.
 */
void rexTimePrint(int32_t time, FILE *outstrm)
{
	fprintf(outstrm, "Time %ld = %s\n", time, rexTimeConv(time));
}

/*
 * Print header info.
 */
PRIVATE void prHdr(FILE *outstrm, char lbl, char *hdr)
{
	int i, sz;

	sz = *((short int *) hdr);
	fprintf(outstrm,"%c-FILE HEADER-->  Record size: %d\n", lbl, sz);

	for (i = 511; i >= 2; i--) {
		if ((hdr[i] != '\0') && (hdr[i] != ' ')) break;
		hdr[i] = '\0';
	}
	fprintf(outstrm,"%s\n", &hdr[2]);
	fprintf(outstrm, "--------------------------------------\n");
}

void rexHeaderPrint(FILE *outstrm)
{
	prHdr(outstrm, 'E', eHeader);
	prHdr(outstrm, 'A', aHeader);
}

/*
 * print rex info header
 */
void rexInfoPrint(RexInfo *ri, FILE * strm)
{
	fprintf(strm, "\n\tri = 0x%p\n", ri);
	fprintf(strm, "\tehdr = 0x%p\n", ri->ehdr);
	fprintf(strm, "\tahdr = 0x%p\n", ri->ahdr);
	fprintf(strm, "\tnumTrials = %d\n", numTrials);
	fprintf(strm, "\tnumRec = %d\n", ri->numRec);
	fprintf(strm, "\tnSignals = %d\n", ri->nSignals);
	fprintf(strm, "\tsignalList = 0x%p\n", ri->signalList);
	fprintf(strm, "\tnPoints = %d\n", ri->nPoints);
	fprintf(strm, "\tevents = 0x%p\n", ri->events);
	fprintf(strm, "\tnEvents = %d\n", ri->nEvents);
	fprintf(strm, "\ttStartTime = %ld\n", ri->tStartTime);
	fprintf(strm, "\ttEndTime = %ld\n", ri->tEndTime);
	fprintf(strm, "\taStartTime = %ld\n", ri->aStartTime);
	fprintf(strm, "\taEndTime = %ld\n", ri->aEndTime);
	fprintf(strm, "\tmaxSampRate = %d\n", ri->maxSampRate);
	fprintf(strm, "\tstartCode = %d\n", ri->startCode);
}

/*
 * print sample header
 */
void rexSampHdrPrint(FILE *strm)
{
	int i;
	SAMP *s;

	s = &sampHdr;

	fprintf(strm, "maxsig = %d\n", s->sa_maxsig);
	fprintf(strm, "size of frame array = %d\n", s->sa_fr_array_sz);
	fprintf(strm, "maxcal = %d\n", s->sa_maxcal);
	fprintf(strm, "lname = %d\n", s->sa_lname);
	fprintf(strm, "number of signals = %d\n", s->sa_signum);
	fprintf(strm, "max sample rate = %d\n", s->sa_maxrate);
	fprintf(strm, "min sample rate = %d\n", s->sa_minrate);
	fprintf(strm, "number sub frames = %d\n", s->sa_subfr_num);
	fprintf(strm, "number frames in a master frame = %d\n", s->sa_mfr_num);
	fprintf(strm, "duration of master frame in msec = %d\n", s->sa_mfr_dur);
	fprintf(strm, "number of stored signals = %d\n", s->sa_fr_sa_cnt);
	fprintf(strm, "number of stored signals in a master frame = %d\n", s->sa_mfr_sa_cnt);
	fprintf(strm, "number of a/d channels = %d\n", s->sa_ad_channels);
	fprintf(strm, "a/d resolution = %d\n", s->sa_ad_res);
	fprintf(strm, "radix compensation --> 2's complement = %d\n", s->sa_ad_rcomp);
	fprintf(strm, "overall gain = %d\n", s->sa_ad_ov_gain);
	fprintf(strm, "size of sample datum in bytes = %d\n", s->sa_datumsz);
	for (i = 0; i < s->sa_maxsig; i++) {
		fprintf(strm, "a/d channel %d\n", i);
		fprintf(strm, "\ta/d rate = %d\n", sa_ad_rate[i]);
		fprintf(strm, "\tstore rate = %d\n", sa_store_rate[i]);
		fprintf(strm, "\ta/d calib = %d\n", sa_ad_calib[i]);
		fprintf(strm, "\ta/d shift = %d\n", sa_shift[i]);
		fprintf(strm, "\ta/d gain = %d\n", sa_gain[i]);
	}
	for (i = 0; i < s->sa_fr_array_sz; i++) {
		fprintf(strm, "frame[%d] = 0x%x\n", i, sa_frame[i]);
	}

	for (i = 0; i < s->sa_maxsig; i++) {
		fprintf(strm, "sa_gvname[%d] = %s\n", i, sa_gvname[i]);
	}

	for (i = 0; i < s->sa_maxsig; i++) {
		fprintf(strm, "sa_title[%d] = %s\n", i, sa_title[i]);
	}

	for (i = 0; i < s->sa_maxcal; i++) {
		fprintf(strm, "sa_calibst[%d] = %s\n", i, sa_calibst[i]);
	}
}


/*
 * set the e-codes used to determine analog window events
 */
void rexSetAWinCodes(int openCd, int closeCd, int cancelCd)
{
	wopencd = openCd;
	wclosecd = closeCd;
	wcancelcd = cancelCd;
}

/*
 * REX.C -- functions for REX A- and E-file data scanning and unpacking
 *
 * HISTORY
 *	24feb93	LMO	Create
 * $Log: rex.c,v $
 * Revision 3.10  1999/11/05 23:11:59  lmo
 * fix ad_store_rate problem in procSigs()
 *
 * Revision 3.9  1999/09/28 18:38:24  lmo
 * *** empty log message ***
 *
 * Revision 3.8  1999/09/16 18:17:05  lmo
 * *** empty log message ***
 *
 * Revision 3.7  1999/09/16 18:06:47  lmo
 * fix aEndTime being off by one
 *
 * Revision 3.6  1999/06/18 14:47:07  lmo
 * *** empty log message ***
 *
 * Revision 3.5  1998/09/24 20:25:34  lmo
 * change to faster sort algorithm (percolate instead of bubble)
 *
 * Revision 3.4  1997/03/17 22:00:15  lmo
 * assign all events prior to first enable code to first trial.
 *
 * Revision 3.3  1996/05/30 17:48:53  lmo
 * fix adata overrun
 *
 * Revision 3.2  1996/05/17  17:49:52  lmo
 * fix runaway index on chk_times()
 *
 * Revision 3.1  1996/05/03 21:21:50  lmo
 * beta-test version
 *
 * Revision 3.0  1996/04/11 09:46:05  lmo
 * alpha version of new algorithm for assigning records.
 *
 * Revision 2.5  1996/04/09 15:27:23  lmo
 * last version with implicit sorting.
 *
 * Revision 2.4  1996/03/17 19:21:09  lmo
 * split events into control and unit events.
 *
 * Revision 2.3  1996/03/13  21:02:39  lmo
 * fix e_code < 0 on backup
 *
 * Revision 2.2  1996/03/13  17:35:37  lmo
 * ok without rexSplit
 *
 * Revision 2.1  1996/03/12  21:13:03  lmo
 * last version with old escan()
 *
 * Revision 2.0  1996/03/07 19:20:03  lmo
 * begin addition of functions that split out unit events.
 *
 * Revision 1.26  1996/03/07 19:01:21  lmo
 * add rexSetAWinCodes with default 800, 801, 802.
 *
 * Revision 1.25  1996/02/29 15:08:55  lmo
 * remove OLD areas
 *
 * Revision 1.24  1996/02/29 14:56:11  lmo
 * use length of sample header to determine which version.
 *
 * Revision 1.23  1996/02/21  22:42:28  lmo
 * *** empty log message ***
 *
 * Revision 1.22  1995/08/28  16:10:25  lmo
 * fix test of window cancelcode, so ignore cancels before opens
 *
 * Revision 1.21  1995/05/30  20:42:42  lmo
 * fix version number break to be at 4.2
 *
 * Revision 1.20  1994/10/05  19:21:44  lmo
 * remove printf ver
 *
 * Revision 1.19  1994/10/05  19:18:31  lmo
 * add exception list for processing rex sample headers
 *
 * Revision 1.18  1994/09/08  19:38:21  lmo
 * fix test on REX version number
 *
 * Revision 1.17  1994/09/08  19:01:36  lmo
 * ok for REX 5.0
 *
 * Revision 1.16  1994/03/21  20:21:16  lmo
 * fix_edge bug at right edge was kludged.
 *
 * Revision 1.15  1994/02/25  16:28:22  lmo
 * sort event list by time for each trial
 *
 * Revision 1.14  1993/04/09  22:31:33  lmo
 * full key
 *
 * Revision 1.13  1993/04/07  22:12:17  lmo
 * fake sample rate
 *
 * Revision 1.12  1993/04/07  21:41:04  lmo
 * add fullscale
 *
 * Revision 1.11  1993/04/07  21:08:15  lmo
 * add rate to structure
 *
 * Revision 1.10  1993/04/07  18:27:02  lmo
 * add scaling for -112 codes
 *
 * Revision 1.9  1993/04/06  21:17:49  lmo
 * add maxSampRate from sampling header
 *
 * Revision 1.8  1993/04/05  20:16:15  lmo
 * add pre-time for bad start codes
 *
 * Revision 1.7  1993/03/05  14:25:33  lmo
 * fix assignRecord function
 *
 * Revision 1.6  1993/03/03  23:40:38  lmo
 * fix loword_() bug.
 *
 * Revision 1.5  1993/03/03  15:28:17  lmo
 * fix so don't need to read A-files
 *
 * Revision 1.4  1993/03/03  03:58:52  lmo
 * fix error messages
 *
 * Revision 1.3  1993/03/02  23:25:28  lmo
 * working version
 *
 * Revision 1.2  1993/02/25  22:53:50  lmo
 * *** empty log message ***
 *
 * RevVision 1.1  1993/02/25  22:28:51  lmo
 * Initial revision
 *
 */
