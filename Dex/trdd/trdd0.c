/*
 * Trial Rex Data Dumper
 *
 * $Log: trdd0.c,v $
 * Revision 1.1.1.1  2004/07/27 18:08:04  jwm
 * Imported using TkCVS
 *
 * Revision 2.0  1996/03/17 19:22:06  lmo
 * support for split events.
 *
 * Revision 1.5  1993/03/12  14:34:55  lmo
 * add onlyCode flag
 *
 * Revision 1.4  1993/03/05  15:15:02  lmo
 * fix help message
 *
 * Revision 1.3  1993/03/05  14:27:44  lmo
 * fix header
 *
 * Revision 1.2  1993/03/04  16:47:52  lmo
 * change analog record print
 *
 * Revision 1.1  1993/03/03  18:49:59  lmo
 * Initial revision
 *
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#ifdef sgi
#include <sys/types.h>
#include <unistd.h>
#endif

#include "rex.h"
#include "trdd.h"

char version[] = "TRDD: Trial Rex Data Dumper V. 2.2 -- 11apr96 -- LM Optican";
char *helpMsg = "\
Usage:\n\
	trdd [-s start_code] [-p pre_time] [-a] [-u] [-o onlyCode] [-m min] [-M max] [-v] rex_file > temp_file\n\
		-a	absolute time\n\
		-u	no units\n\
		-o	only print trials containing \"onlyCode\" events\n\
		-m	minimum trial number\n\
		-M	maximum trial number\n\
		-v	verbose (e.g., symbolic codes)\n\
";


int numTrials = 0;			/* number of records */
SignalList *signals = 0;		/* pointer to list of signals */
int nsignals = 0;			/* number of signals */
int units = 1;				/* print units */
int npts = 0;				/* number of points in signals */
int autoScale = 0;
int plotDots = 0;
int interpolate = 0;
int reDraw = 0;
int verbose = 0;
long int absTime = -1;

int maxSampRate = 1000;
int firstTrial = 0;
int lastTrial = 1e8;

char fname[128] = { 0 };

long aStartTime = 0;		/* start of analog record */
long aEndTime = 0;		/* end of analog record */
long tStartTime = 0;		/* start of trial */
long tEndTime = 0;		/* end of trial */
long curTrial = 1;		/* current record */
int fileOK = 0;			/* set if valid file is open */
FILE *scanstrm = 0;
EVENT *evbuf = 0;		/* array of events */
int nevbuf = 0;		/* number in event array */

int onlyCode = 0;		/* restriction code if non-zero */

