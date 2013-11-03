/*
 * Trial Rex Data Dumper
 *
 * $Log: trdd2.c,v $
 * Revision 1.1.1.1  2004/07/27 18:08:04  jwm
 * Imported using TkCVS
 *
 * Revision 2.0  1996/03/17 19:22:06  lmo
 * support for split events.
 *
 * Revision 1.4  1993/03/12  14:34:55  lmo
 * add onlyCode flag
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

#ifdef sgi
#include <sys/types.h>
#include <unistd.h>
#endif

#include "rex.h"
#include "trdd.h"

static int lastUnit = 0, repeats = 0, nlOut = 1;
static ANALOGHDR ahdr;
static ANALOGHDR *a = &ahdr;

int codeInTrial(int code, EVENT *ev, int n)
{
	while(n--) if (ev++->e_code == code) return(1);
	return(0);
}

char * codeName(int code)
{
	static char buf[15];

	if (verbose) switch(code) {
	case STDHDRCD:
		sprintf(buf, "%10s", "STDHDR");
		break;
	case SAMPHDRCD:
		sprintf(buf, "%10s", "SAMPHDR");
		break;
	case EYEHCD:
		sprintf(buf, "%10s", "EYEHBUF");
		break;
	case EYEVCD:
		sprintf(buf, "%10s", "EYEVBUF");
		break;
	case ADATACD:
		sprintf(buf, "%10s", "ADATABUF");
		break;
	case WOPENCD:
		sprintf(buf, "%10s", "WOPENCD");
		break;
	case WCLOSECD:
		sprintf(buf, "%10s", "WCLOSECD");
		break;
	case WCANCELCD:
		sprintf(buf, "%10s", "WCANCELCD");
		break;
	case WERRCD:
		sprintf(buf, "%10s", "WERRCD");
		break;
	case UWOPENCD:
		sprintf(buf, "%10s", "UWOPENCD");
		break;
	case UWCLOSECD:
		sprintf(buf, "%10s", "UWCLOSECD");
		break;
	case ENABLECD:
		sprintf(buf, "%10s", "ENABLECD");
		break;
	case PAUSECD:
		sprintf(buf, "%10s", "PAUSECD");
		break;
	case STARTCD:
		sprintf(buf, "%10s", "STARTCD");
		break;
	default:
		sprintf(buf, "%10d", code);
		break;
	}
	else sprintf(buf, "%10d", code);
	return(buf);
}

void printUnits(RexInfo *ri)
{
	int i, j, n, nrep, code, rTime, time, elapsedTime;
	long *t;
	char buf[26];
	RexUnits *ru = ri->rexunits;

	if (ru->nTimes == NULL) return;

	for (i = 0; i < ri->nUnits; i++) {
		if ((n = ru->nTimes[i]) == 0) continue;

		code = ri->unitCodes[i];
		t = ru->unitTimes[i];

		time = *t++;
		elapsedTime =  time - tStartTime;
		rTime = time - absTime;
		sprintf(buf, "%d", rTime);
		if (strlen(buf) >= 8) nrep = 4;
		else nrep = 5;

		if (!nlOut) printf("\n");
		printf("%10d\t%d\t%d", code, elapsedTime, rTime);
		nlOut = 0;
		repeats = 1;

		for (j = 1; j < n; j++) {
			rTime = *t++ - absTime;

			if (repeats < 1) printf("\t\t\t%d", rTime);
			else printf("  %d", rTime);

			if (repeats++ == nrep) {
				printf("\n");
				nlOut = 1;
				repeats = 0;
			}
			else nlOut = 0;
		}
	}	
}

void printEvent(EVENT *ev)
{
	int code, nrep;
	static int rTime, time, elapsedTime, lastTime;
	char buf[32];

	code = ev->e_code;
	time = ev->e_key;
	rTime = time - absTime;

	sprintf(buf, "%d", rTime);
	if (strlen(buf) >= 8) nrep = 4;
	else nrep = 5;

	if (code >= 0) {
		elapsedTime = time - tStartTime;

#ifndef USE_SPLIT
		if (UNIT1CD <= code &&
			code <= (UNIT1CD + rexMaxUnits)) {

			if (!units) return;

			if (code != lastUnit) {
				printf("%10d\t%d\t%d", code, elapsedTime, rTime);
				lastUnit = code;
				nlOut = 0;
				repeats = 1;
			}
			else {
				if (repeats < 1) printf("\t\t\t%d", rTime);
				else printf("  %d", rTime);

				if (repeats++ == nrep) {
					printf("\n");
					nlOut = 1;
					repeats = 0;
				}
				else nlOut = 0;
			}
		}
		else {
			if (!nlOut) printf("\n");
			printf("%s\t%d\t%d\n", codeName(code), elapsedTime, rTime);

			nlOut = 1;
			lastUnit = 0;
		}
#else
		if (!nlOut) printf("\n");
		printf("%s\t%d\t%d\n", codeName(code), elapsedTime, rTime);

		nlOut = 1;
		lastUnit = 0;
#endif
	}
	else {				/* analog record */
		if (rexGetAnalogHeader(ev, a)) return;

		time = a->atime;
		rTime = time - absTime;
		if (a->acontinue && time == 0) {
			time = lastTime;
		}
		else lastTime = time;

		elapsedTime = time - tStartTime;

		if (!nlOut) printf("\n");

		printf("%s\t%d\t%d\t%d\n",
			codeName(code), elapsedTime, rTime, a->acontinue);

		nlOut = 1;
		lastUnit = 0;
	}
}

/*
 * Print rex data
 */
static RexInfo *ri = NULL;

void printTrial(int curTrial)
{
	int i, n;
	EVENT *ev;

	ri = rexGetTrial(curTrial, 0);
	ev = ri->events;
	n = ri->nEvents;
	aStartTime = ri->aStartTime;
	aEndTime = ri->aEndTime;
	tStartTime = ri->tStartTime;

	if (absTime == -1) absTime = tStartTime;

	if (onlyCode && !codeInTrial(onlyCode, ev, n)) return;

	if (!nlOut) {
		printf("\n");
		nlOut = 1;
	}
	printf("\nTrial %d:  %d events\n", curTrial, n);
	lastUnit = 0;

	for (i = 0; i < n; i++) printEvent(ev++);
	if (units) printUnits(ri);
}
