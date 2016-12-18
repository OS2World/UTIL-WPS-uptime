/*
 * File: test.c
 *
 * Test program for system uptime
 *
 * Bob Eager   October 2004
 *
 */

#define	INCL_DOSMISC
#define	INCL_DOSFILEMGR
#define	INCL_DOSDATETIME
#define	INCL_DOSPROFILE
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define	BUFSIZE		1024
#define	LINESIZ		1024

/* Forward references */

static	INT	getline(HFILE, PUCHAR, PULONG, PUCHAR, PUCHAR *);
static	ULONG	uptime_secs(VOID);


main(INT argc, PUCHAR argv[])
{	ULONG ut;
	INT days, hours, mins, secs;

	ut = uptime_secs();

	days = ut/86400;
	ut = ut%86400;

	hours = ut/3600;
	ut = ut%3600;

	mins = ut/60;
	secs = ut%60;

	fprintf(
		stdout,
		"Uptime = %d day%s, %02lu:%02lu:%02lu\n",
		days,
		days == 1 ? "" : "s", hours, mins, secs);
	
	return(EXIT_SUCCESS);
}

#if 0
static ULONG uptime_secs(VOID)
{	APIRET rc;
	ULONG freq;
	QWORD time;
typedef struct _IO {
ULONG	dendhi;			/* Dividend high */
ULONG	dendlo;			/* Dividend low */
ULONG	divisor;		/* Divisor */
ULONG	quot;			/* Quotient */
ULONG	remain;			/* Remainder */
} IO, *PIO;
	IO io;
extern	VOID _dbldiv(PIO);

	rc = DosTmrQueryFreq(
		&freq);
fprintf(stderr, "DosTmrQueryFreq => %d\n", rc);
fprintf(stderr, "Freq=%08x\n", freq);

	rc = DosTmrQueryTime(&time);
fprintf(stderr, "DosTmrQueryTime => %d\n", rc);
fprintf(stderr, "Time=%08x %08x\n", time.ulHi, time.ulLo);

io.dendhi = time.ulHi;
io.dendlo = time.ulLo;
io.divisor = freq;
	_dbldiv(&io);

fprintf(stderr, "Quotient=%d, remainder=%d\n", io.quot, io.remain);

	return(io.quot);
}
#endif

#if 0
static ULONG uptime_secs(VOID)
{	APIRET rc;
	ULONG res;

	rc = DosQuerySysInfo(
		QSV_MS_COUNT,
		QSV_MS_COUNT,
		&res,
		sizeof(res));

	return(res/1000);
}
#endif


#if 0
static ULONG uptime_secs(VOID)
{	APIRET rc;
	HFILE fh;
	ULONG action, actual;
	ULONG res;

	rc = DosOpen(
		"UPTIME$",		/* Name */
		&fh,			/* Handle returned here */
		&action,		/* Action returned here */
		0L,			/* Size */
		0L,			/* Attributes */
		OPEN_ACTION_FAIL_IF_NEW |
		OPEN_ACTION_OPEN_IF_EXISTS,
		OPEN_FLAGS_SEQUENTIAL |
		OPEN_SHARE_DENYWRITE |
		OPEN_FLAGS_FAIL_ON_ERROR |
		OPEN_ACCESS_READONLY,
		NULL);			/* Extended attributes area */
	if(rc != 0) {
		fprintf(stderr, "Open device failed, rc = %d\n", rc);
		exit(EXIT_FAILURE);
	}

	rc = DosRead(fh, &res, 4, &actual);

	fprintf(stderr, "Read result = %d, bytes read = %d\n", rc, actual);

	DosClose(fh);

	return(res);
}
#endif


static ULONG uptime_secs(VOID)
{	APIRET rc;
	ULONG res;
	UCHAR s[15];
	HFILE fh;
	ULONG action, n;
	INT i;
	ULONG nbytes;
	PUCHAR p, q;
	UCHAR line[LINESIZ];
	UCHAR buf[BUFSIZE];
	static UCHAR key[] = "SWAPPATH";
	FILESTATUS3 fs;
	DATETIME dt;
	time_t boot, now, diff;
	struct tm ts;
	struct tm *tsp;

	rc = DosQuerySysInfo(
		QSV_BOOT_DRIVE,
		QSV_BOOT_DRIVE,
		&res,
		sizeof(res));
	sprintf(s, "%c:\\CONFIG.SYS", res + 'A' - 1);
	fprintf(stdout, "Selected %s\n", s);

	rc = DosOpen(
		s,			/* Name */
		&fh,			/* Handle returned here */
		&action,		/* Action returned here */
		0L,			/* Size */
		0L,			/* Attributes */
		OPEN_ACTION_FAIL_IF_NEW |
		OPEN_ACTION_OPEN_IF_EXISTS,
		OPEN_FLAGS_SEQUENTIAL |
		OPEN_SHARE_DENYWRITE |
		OPEN_ACCESS_READONLY,
		NULL);			/* Extended attributes area */
	if(rc != 0) {
		fprintf(stderr, "Open %s failed, rc = %d\n", s, rc);
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "Opened CONFIG.SYS OK\n");

	nbytes = 0;
	for(;;) {
		if(getline(fh, buf, &nbytes, line, &p) == 0) break;
		q = &line[0];
		while((*q == ' ') || (*q == '\t')) p++;
		if(strnicmp(key, q, strlen(key)) != 0) continue;
		q += strlen(key);
		while((*q == ' ') || (*q == '\t')) p++;
		if(*q != '=') continue;
		q++;
		while((*q == ' ') || (*q == '\t')) p++;
		p = q;
		while((*p != '\0') && (*p != ' ') && (*p != '\t')) p++;
		break;
	}

	*p = '\0';
	strcpy(buf, q);
	strcat(buf, "SWAPPER.DAT");
	fprintf(stderr, "OK...swap file is %s\n", buf);

	rc = DosQueryPathInfo(
		buf,
		FIL_STANDARD,
		&fs,
		sizeof(fs));
	printf("rc from DosQueryPathInfo = %d\n", rc);
	printf(
		"Date=%02d/%02d/%04d; Time=%02d:%02d:%02d\n",
		fs.fdateLastAccess.day,
		fs.fdateLastAccess.month,
		fs.fdateLastAccess.year+1980,
		fs.ftimeLastAccess.hours,
		fs.ftimeLastAccess.minutes,
		fs.ftimeLastAccess.twosecs*2);
	ts.tm_sec   = fs.ftimeLastAccess.twosecs*2;
	ts.tm_min   = fs.ftimeLastAccess.minutes;
	ts.tm_hour  = fs.ftimeLastAccess.hours;
	ts.tm_mday  = fs.fdateLastAccess.day;
	ts.tm_mon   = fs.fdateLastAccess.month;
	ts.tm_year  = fs.fdateLastAccess.year+80;
	ts.tm_isdst = 0;
	boot = mktime(&ts);

	(VOID) DosGetDateTime(&dt);
	printf(
		"Date=%02d/%02d/%04d; Time=%02d:%02d:%02d\n",
		dt.day,
		dt.month,
		dt.year,
		dt.hours,
		dt.minutes,
		dt.seconds);
	ts.tm_sec   = dt.seconds;
	ts.tm_min   = dt.minutes;
	ts.tm_hour  = dt.hours;
	ts.tm_mday  = dt.day;
	ts.tm_mon   = dt.month;
	ts.tm_year  = dt.year-1900;
	ts.tm_isdst = 0;
	now = mktime(&ts);

	printf("boot=%08x; now=%08x; diff=%08x\n",
		boot, now, now-boot);
	diff = now - boot;
	dt.seconds = diff%60; diff /= 60;
	dt.minutes = diff%60; diff /= 60;
	dt.hours   = diff%24; diff /= 24;
	dt.day     = diff;

	printf("uptime=%d days, %02d:%02d:%02d\n",
		dt.day,
		dt.hours,
		dt.minutes,
		dt.seconds);

	(VOID) DosClose(fh);

	return(EXIT_SUCCESS);
}


static INT getline(HFILE fh, PUCHAR buf, PULONG nbytes, PUCHAR line, PUCHAR *pp)
{	INT i, c;
	INT n = 0;
	APIRET rc;
	PUCHAR p = *pp;

	for( ; ; ) {
		if(*nbytes == 0) {		/* Refill buffer */
			rc = DosRead(
				fh,
				buf,
				BUFSIZE,
				nbytes);
			if((rc != 0) || ((*nbytes == 0) && (n == 0)))
				return(0);
			p = buf;
		}
		c = *p++; (*nbytes)--;
		if(c == '\r') continue;
		if((c == '\n') || (n >= BUFSIZE-1)) {
			line[n] = '\0';
			break;
		}
		line[n++] = c;
	}
	*pp = p;

	return(1);
}

/*
 * End of file: test.c
 *
 */

