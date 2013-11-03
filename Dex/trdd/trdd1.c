/*
 *-----------------------------------------------------------------------*
 * NOTICE:  This code was developed by the US Government.  The original
 * versions, REX 1.0-3.12, were developed for the pdp11 architecture and
 * distributed without restrictions.  This version, REX 4.0, is a port of
 * the original version to the Intel 80x86 architecture.  This version is
 * distributed only under license agreement from the National Institutes 
 * of Health, Laboratory of Sensorimotor Research, Bldg 10 Rm 10C101, 
 * 9000 Rockville Pike, Bethesda, MD, 20892, (301) 496-9375.
 *-----------------------------------------------------------------------*
 */
/*
 * Trial Rex Data Dumper
 *
 * $Log: trdd1.c,v $
 * Revision 1.1.1.1  2004/07/27 18:08:04  jwm
 * Imported using TkCVS
 *
 * Revision 2.0  1996/03/17 19:22:06  lmo
 * support for split events.
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
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#ifdef sgi
#include <sys/types.h>
#include <unistd.h>
#endif

#include <rex.h>
#include "trdd.h"

int startCode = ENABLECD;
int preTime = 0;

int main(int argc, char *argv[])
{
	int i;

#ifdef USE_LMALLOC
#ifdef sgi
	mallopt(M_DEBUG, 0);
#endif
#endif
	parse_com(argc, argv);

	if (maxSampRate < 10) {
		fprintf(stderr, "maxSampRate too low!\n");
		exit(0);
	}

#ifdef USE_SPLIT
	rexSplitEvents();
#endif
	numTrials = rexFileOpen(fname, maxSampRate, startCode, preTime);

	rexHeaderPrint(stdout);
	printf("\n");

	if (numTrials == 0) {
		fprintf(stderr, "Rextools can't find any data in file %s with start code %d\n!", fname, startCode);
		exit(0);
	}
	else fileOK = 1;


	if (firstTrial < 1) firstTrial = 1;
	if (lastTrial > numTrials) lastTrial = numTrials;


	if (verbose) {
		printf("%s\n", rexToolsVersion());
		printf("\nRex Sampling Header:\n");
		rexSampHdrPrint(stdout);
		printf("\n------------------------------------------\n");
	}

	/*
	 * print info for each trial
	 */
	if (absTime == 0) printf("    Code    Trial Time\tAbsolute Time\n");
	else printf("    Code    Trial Time\tRelative Time\n");
	for (i = firstTrial; i <= lastTrial; i++) printTrial(i);

	printf("\n------------------------------------------\n");
	rexTotalsPrint(stdout);
	exit(0);
}


/* PARSE_COM
 * parses command line
 */
int parse_com(int argc, char **argv)
{
	register char * str;
	extern char *helpMsg;

	/* are there any parameters? */
	if (argc < 2) {
		fprintf(stderr, "%s\n", version);
		fprintf(stderr, "%s\n", helpMsg);
		fprintf(stderr, "(%s)\n", rexToolsVersion());
		exit(1);
	}

	/* process parameters */
	for (argv++; *argv; argv++) {
		str = *argv;
		if (*str == '-') {
			while(*++str) switch(*str) {
			case 'a':		/* print absolute times */
				absTime = 0;
				break;
			case 'f':	/* max sample rate */
				if (sscanf(*(++argv), "%d", &maxSampRate) < 1) {
					fprintf(stdout, "No max_sample_rate!\n");
					exit(1);
				}
				break;
			case 'm':	/* minimum trial */
				if (sscanf(*(++argv), "%d", &firstTrial) < 1) {
					fprintf(stdout, "No minimum trial number!\n");
					exit(1);
				}
				break;
			case 'M':	/* maximum trial */
				if (sscanf(*(++argv), "%d", &lastTrial) < 1) {
					fprintf(stdout, "No maximum trial number!\n");
					exit(1);
				}
				break;
			case 'o':	/* onlyCode */
				if (sscanf(*(++argv), "%d", &onlyCode) < 1) {
					fprintf(stdout, "No onlyCode!\n");
					exit(1);
				}
				break;
			case 'p':	/* pre time */
				if (sscanf(*(++argv), "%d", &preTime) < 1) {
					fprintf(stdout, "No pre_time!\n");
					exit(1);
				}
				break;
			case 's':	/* start code */
				if (sscanf(*(++argv), "%d", &startCode) < 1) {
					fprintf(stdout, "No start code!\n");
					exit(1);
				}
				break;
			case 'u':		/* suppress units */
				units = 0;
				break;
			case 'v':		/* verbose output */
				verbose = 1;
				break;
			default:
				fprintf(stdout, "BAD FLAG:  %c\n",*str);
				exit(1);
				break;
			} /* end while switch */
		}
		else {
			strncpy(fname,str,sizeof(fname));
			fname[sizeof(fname) - 1] = '\0';	/* force null */
		}
	}
	return(0);
}
