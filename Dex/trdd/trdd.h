/*
 * Trial Rex Data Dumper
 *
 * $Log: trdd.h,v $
 * Revision 1.1.1.1  2004/07/27 18:08:04  jwm
 * Imported using TkCVS
 *
 * Revision 2.0  1996/03/17 19:21:47  lmo
 * support for split events
 *
 * Revision 1.3  1993/03/12  14:34:55  lmo
 * add onlyCode flag
 *
 * Revision 1.2  1993/03/04  16:47:52  lmo
 * change analog record print
 *
 * Revision 1.1  1993/03/03  18:49:59  lmo
 * Initial revision
 *
 *
 *
 */
#define USE_SPLIT

#define N_CHAN		rexNchannels	/* -110, -111, -112 */
#define MAX_SIGNALS	rexMaxSignals

extern int units;
extern int npts;
extern int verbose;
extern long int absTime;		/* if set, print absolute times */
extern int maxSampRate;
extern int firstTrial;
extern int lastTrial;
extern char version[];
extern char fname[128];
extern FILE *scanstrm;
extern char *no_file;

extern int fileOK;			/* set if valid file is open */
extern int nsignals;
extern int numTrials;
extern int awindow;
extern long curTrial;
extern long aStartTime;		/* start of analog record */
extern long aEndTime;		/* end of analog record */
extern long tStartTime;		/* start of trial */
extern long tEndTime;		/* end of trial */
extern EVENT *evbuf;		/* array of events */
extern int nevbuf;		/* number in event array */

extern int onlyCode;		/* restriction code if non-zero */

/* function prototypes */
void done(void);
int parse_com(int argc, char **argv);
void printTrial(int curTrial);


