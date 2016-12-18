/*
 * File: logd.c
 *
 * General logging and tracing routines suitable for DLL use
 *
 * Bob Eager   October 2004
 *
 */

#pragma	strings(readonly)

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <os2.h>

#include "logd.h"

#define	MAXLINE		1000		/* Maximum length of log line */


/*
 * Write a string to the logfile. The string is timestamped, and a newline
 * appended to the end unless there is one there already.
 *
 */

static void logit(char *s)
{	time_t tod;
	char timeinfo[35];
	char *logname = getenv("DLL_LOG");
	FILE *logfp;

	if(logname == NULL) return;

	logfp = fopen(logname, "a");
	if(logfp == (FILE *) NULL) return;
	setbuf(logfp, NULL);

	(void) time(&tod);
	(void) strftime(timeinfo, sizeof(timeinfo),
		"%d/%m/%y %X>", localtime(&tod));
	fprintf(logfp, "%s %s", timeinfo, s);
	if(s[strlen(s)-1] != '\n') fputc('\n', logfp);
	fflush(logfp);
	fclose(logfp);
}


/*
 * Output message, in printf style, to the logfile.
 *
 */

void dolog(char *mes, ...)
{	va_list ap;
	char buf[MAXLINE+1];

	va_start(ap, mes);
	vsprintf(buf, mes, ap);
	va_end(ap);

	logit(buf);
}

/*
 * End of file: logd.c
 *
 */
