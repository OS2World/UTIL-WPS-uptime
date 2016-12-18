/*
 * File: client.c
 *
 * Class for Workplace Shell uptime monitor.
 *
 * Client dialog procedure and support
 *
 * Bob Eager   October 2004
 *
 */

#pragma	strings(readonly)

#pragma	hdrfile "main.pch"
#define	INCL_DOS
#define	INCL_DOSERRORS
#define	INCL_DOSFILEMGR
#define	INCL_DOSPROCESS
#define	INCL_DOSSEMAPHORES
#define	INCL_DOSPROFILE
#define	INCL_DOSMISC
#define	INCL_DOSDATETIME
#define	INCL_WIN
#include <os2.h>
#pragma	hdrstop

#pragma	alloc_text(a_dlg_seg, AboutDlgProc)
#pragma	alloc_text(a_init_seg, Client)
#pragma	alloc_text(a_dlg_seg, ClientAbout)
#pragma	alloc_text(a_init_seg, ClientCloseView)
#pragma	alloc_text(a_dlg_seg, ClientDisplayUptime)
#pragma	alloc_text(a_dlg_seg, ClientRefresh)
#pragma	alloc_text(a_dlg_seg, ClientSetRefreshInterval)
#pragma	alloc_text(a_dlg_seg, ClientSetMethod)
#pragma	alloc_text(a_init_seg, ClientStartState)
#pragma	alloc_text(a_init_seg, ClientStopState)
#pragma	alloc_text(a_dlg_seg, UptimeSeconds)
#pragma	alloc_text(a_dlg_seg, UptimeSecondsTimer)
#pragma	alloc_text(a_dlg_seg, UptimeSecondsSwapfile)
#pragma	alloc_text(a_dlg_seg, UptimeSecondsDriver)
#pragma	alloc_text(a_dlg_seg, GetLine)

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "uptime.ih"

#include "resource.h"
#include "dialog.h"
#include "common.h"
#ifdef	DEBUG
#include "logd.h"
#endif

#define	ABOUTSTACK	16384		/* Stack size for About thread */
#define	CLIENTSTACK	16384		/* Stack size for client thread */

#define	UPTIME_DEVICE	"UPTIME$"	/* Name of 'uptime' device */

#define	BUFSIZE		1024		/* For CONFIG.SYS */
#define	LINESIZE	1024		/* For CONFIG.SYS */

/* Structure definitions */

typedef struct _IO {			/* Used for calling 'dbldiv' */
ULONG	dendhi;				/* Dividend high */
ULONG	dendlo;				/* Dividend low */
ULONG	divisor;			/* Divisor */
ULONG	quot;				/* Quotient */
ULONG	remain;				/* Remainder */
} IO, *PIO;

/* External references */

extern	VOID _dbldiv(PIO);

/* Forward references */

static	MRESULT EXPENTRY ClientDlgProc(HWND hwnd, ULONG msg,
						MPARAM mp1, MPARAM mp2);
static	MRESULT	EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg,
						MPARAM mp1, MPARAM mp2);
static	VOID	ClientDisplayUptime(PCLIENTDATA);
static	VOID	ClientStartState(PCLIENTDATA);
static	VOID	ClientStopState(PCLIENTDATA);
static	INT	GetLine(HFILE, PUCHAR, PULONG, PUCHAR, PUCHAR *);
static	VOID	MonitorThread(PVOID);
static	ULONG	UptimeSeconds(PCLIENTDATA);
static	ULONG	UptimeSecondsTimer(VOID);
static	ULONG	UptimeSecondsSwapfile(PCLIENTDATA);
static	ULONG	UptimeSecondsDriver(PCLIENTDATA);


/*
 * This is the main client procedure. It is called when the Watch view
 * is opened. It loads the view dialog and runs it, performing the necessary
 * WPS bookkeeping tasks along the way.
 *
 */

HWND Client(PCLIENTDATA cd)
{	HWND hwndDlg;

	/* Load the dialog for the view */

	hwndDlg = WinLoadDlg(
			HWND_DESKTOP,
			HWND_DESKTOP,
			ClientDlgProc,
			cd->hmod,
			DLG_WATCH,
			(PVOID) cd);
	cd->hwnd = hwndDlg;

	/* Register the new open view */

	if(_wpRegisterView(cd->so, hwndDlg, "Watch View") == FALSE)
		return(NULLHANDLE);

	/* Add the new view to the object use list */

	cd->UseItem.type = USAGE_OPENVIEW;
	cd->ViewItem.view = OPEN_WATCH;
	cd->ViewItem.handle = (LHANDLE) hwndDlg;
	if(_wpAddToObjUseList(cd->so, &cd->UseItem) == FALSE)
		return(NULLHANDLE);

	return(hwndDlg);		/* WPS will now run this window */
}


/*
 * Client dialog procedure. Run by the WPS when the view has been opened.
 *
 */

static	MRESULT EXPENTRY ClientDlgProc(HWND hwnd, ULONG msg,
						MPARAM mp1, MPARAM mp2)
{	PCLIENTDATA cd;

	switch(msg) {
		case WM_INITDLG:	/* Dialog window is about to open */
			cd = (PCLIENTDATA) mp2;	/* All data are here */
			cd->hwnd = hwnd;	/* Apart from this... */
			WinSetWindowPtr(hwnd, QWL_USER, cd);
			ClientStartState(cd);
			return((MRESULT) FALSE);

		case WM_CLOSE:		/* User closed dialog window */
			cd = (PCLIENTDATA) WinQueryWindowPtr(hwnd, QWL_USER);
			WinDismissDlg(hwnd, 0);
			WinDestroyWindow(hwnd);	/* Because we used WinLoadDlg */
			return((MRESULT) 0);

		case WM_DESTROY:	/* Window being destroyed */
			cd = (PCLIENTDATA) WinQueryWindowPtr(hwnd, QWL_USER);
			ClientStopState(cd);
			break;

		case UM_RESULTS:	/* Server results */
			cd = (PCLIENTDATA) WinQueryWindowPtr(hwnd, QWL_USER);
			ClientDisplayUptime(cd);
			return((MRESULT) 0);
	}
	return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}


/*
/*
 * Client dialog initial state.
 *
 */

static VOID ClientStartState(PCLIENTDATA cd)
{	PUCHAR s;
	UCHAR temp[MAXTEMP];
	UCHAR LoadError[CCHMAXPATH];
	RESULTCODES ChildRes;
	ULONG SaveMethod;
	APIRET rc;

	cd->Stopping = FALSE;		/* TRUE only when closing view */
	cd->HDev = (HFILE) NULL;	/* Handle to device, none yet */
	cd->SwapBase = 0;		/* Swapfile base date/time, none yet */

	/* Do some juggling so that we can use the 'change method' code to
	   set the initial method. This is done by setting an invalid
	   'current method' and then calling 'ClientSetMethod' to change
	   it to what we actually want. */

	SaveMethod = cd->method;
	cd->method = -1;		/* Invalid value */

	ClientSetMethod(cd->so, cd->env, SaveMethod);

	/* Retrieve the object title text and save copy for later use */

	s = _wpQueryTitle(cd->so);
	cd->TitleText = _wpAllocMem(cd->so, strlen(s)+1, NULL);
	strcpy(cd->TitleText, s);

	/* Create an event semaphore for timing and thread control */

	rc = DosCreateEventSem(
			NULL,		/* Unnamed semaphore */
			&cd->Sem,
			0L,		/* Private */
			FALSE);		/* Start in 'reset' state */

	/* Start the thread that monitors the uptime; use an event
	   semaphore for the thread to tell us that it is terminating. */

	rc = DosCreateEventSem(
			NULL,
			&cd->ThreadSem,
			0L,
			FALSE);		/* Starts off reset */

	cd->MonitorTID = _beginthread(
				MonitorThread,
				NULL,
				CLIENTSTACK,
				(PVOID) cd);
}


/*
 * Client dialog cleanup and termination.
 *
 */

static VOID ClientStopState(PCLIENTDATA cd)
{	uptimeData *somThis = uptimeGetData(cd->so);
	APIRET rc;

	/* Tell the monitor thread to terminate, and wait for it */

	cd->Stopping = TRUE;
	rc = DosPostEventSem(cd->Sem);
	rc = DosWaitEventSem(cd->ThreadSem, SEM_INDEFINITE_WAIT);
	rc = DosWaitThread((PTID) &cd->MonitorTID, DCWW_WAIT);

	/* Close semaphores */

	if(cd->Sem != (HEV) NULLHANDLE)
		DosCloseEventSem(cd->Sem);
	if(cd->ThreadSem != (HEV) NULLHANDLE)
		DosCloseEventSem(cd->ThreadSem);

	/* Close device driver, if open */

	if(cd->HDev != (HFILE) NULL) {
		(VOID) DosClose(cd->HDev);
	}

	/* Reset title to the one for the closed state */

	_wpSetTitle(cd->so, cd->TitleText);

	/* Remove use item from list */

	_wpDeleteFromObjUseList(cd->so, &cd->UseItem);

	/* Release memory */

	_wpFreeMem(cd->so, cd->TitleText);
	_wpFreeMem(cd->so, (PUCHAR) cd);
	_dlginfo = (ULONG) NULL;
}


/*
 * Client monitor thread. This displays the current uptime, then waits
 * on the control semaphore.
 * When the semaphore clears, either:
 *
 *	a) the timer has expired; display uptime and loop
 * or
 *	b) the monitor is being requested to stop; just exit
 *
 * The uptime is displayed by posting to the view dialog procedure
 * using a UM_RESULTS message.
 *
 */

static VOID MonitorThread(PVOID param)
{	PCLIENTDATA cd = (PCLIENTDATA) param;
	ULONG PostCount;
	APIRET rc;

	for(;;) {
		WinPostMsg(			/* Display uptime */
			cd->hwnd,
			UM_RESULTS,
			(MPARAM) 0,
			(MPARAM) 0);

		rc = DosWaitEventSem(
			cd->Sem,
			cd->refreshinterval*1000);
		if(cd->Stopping == TRUE) break;	/* No display - just stopping */

		rc = DosResetEventSem(
			cd->Sem,
			&PostCount);

	}

	rc = DosPostEventSem(cd->ThreadSem);
}


/*
 * Client request to close the watch view. This is called when the "Close view"
 * item is selected on the pop-up menu.
 *
 */

BOOL ClientCloseView(uptimeData *somThis, Environment *env)
{	WinSendMsg(
		((PCLIENTDATA) _dlginfo)->hwnd,
		WM_CLOSE,
		(MPARAM) 0,
		(MPARAM) 0);

	return(TRUE);
}


/*
 * Client request to display an About box.
 *
 */

BOOL ClientAbout(HMODULE hmod)
{	static CAINFO cainfo;	/* Must be static; used after we return */

	cainfo.cb = sizeof(CAINFO);
	cainfo.hmod = hmod;
	sprintf(cainfo.text, ABOUT_TEXT, VERSION, EDIT);

	WinDlgBox(
		HWND_DESKTOP,
		HWND_DESKTOP,
		AboutDlgProc,
		cainfo.hmod,
		DLG_ABOUT,
		&cainfo);

	return(TRUE);
}


/*
 * Dialog procedure for About dialog box.
 *
 */

static MRESULT EXPENTRY AboutDlgProc(
				HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{	PCAINFO cainfo;

	switch(msg) {
		case WM_INITDLG:	/* Dialog window is about to open */
			cainfo = (PCAINFO) mp2;
			WinSetWindowPtr(hwnd, QWL_USER, cainfo);
			cainfo->hwnd = hwnd;
			(VOID) WinSetDlgItemText(
				hwnd,
				DLG_ABOUT_TEXT,
				cainfo->text);
			return((MRESULT) 0);

		case WM_DESTROY:	/* Window being destroyed */
			WinDismissDlg(hwnd, 0);
			return((MRESULT) 0);
	}
	return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}


/*
 * Client request to change the refresh interval. This takes effect
 * after the next tick.
 *
 */

VOID ClientSetRefreshInterval(uptime *somSelf, Environment *env,
					ULONG value)
{	uptimeData *somThis = uptimeGetData(somSelf);
	PCLIENTDATA cd = (PCLIENTDATA) _dlginfo;
	APIRET rc;

	if(cd != (PCLIENTDATA) NULL) {
		cd->refreshinterval = value;
	}
}


/*
 * Client request to change the method. Any failures here are 'soft',
 * and are reported indirectly by the failure of the timer code to get
 * a valid value for the uptime. The request takes effect on the next tick.
 *
 */

VOID ClientSetMethod(uptime *somSelf, Environment *env,
					ULONG value)
{	uptimeData *somThis = uptimeGetData(somSelf);
	PCLIENTDATA cd = (PCLIENTDATA) _dlginfo;
	ULONG action, res, nbytes;
	UCHAR s[CCHMAXPATH+1];
	UCHAR line[LINESIZE];
	UCHAR buf[BUFSIZE];
	PUCHAR p, q;
	BOOL found;
	HFILE fh;
	FILESTATUS3 fs;
	struct tm ts;
	APIRET rc;
	static UCHAR key[] = "SWAPPATH";

	if(cd == (PCLIENTDATA) NULL) return;	/* Can do nothing */

#ifdef	DEBUG
	dolog("ClientSetMethod value=%d, cd->method=%d", value, cd->method);
#endif

	if(cd->method == value) return;		/* No change */

	/* Tidy up after old method */

	switch(cd->method) {
		case DLG_METHOD_TIMER:
			/* Nothing to do */
			break;

		case DLG_METHOD_SWAPFILE:
			cd->SwapBase = 0;	/* So that we re-read on return */
			break;

		case DLG_METHOD_DRIVER:
			if(cd->HDev != (HFILE) NULL) {
				(VOID) DosClose(cd->HDev);		
				cd->HDev = (HFILE) NULL;
			}
		/* Note that invalid values must be ignored here */
	}

	/* Attempt to switch to new method */

	cd->method = value;

	/* For the timer method, no initialisation is necessary. */

	if(value == DLG_METHOD_TIMER) return;	/* Nothing to do */

	/* For the driver method, we need to get and store a handle
	   to the driver. */

	if(value == DLG_METHOD_DRIVER) {	/* Use device driver */
		rc = DosOpen(
			UPTIME_DEVICE,
			&cd->HDev,
			&action,
			0L,
			FILE_NORMAL,
			OPEN_ACTION_FAIL_IF_NEW |
			OPEN_ACTION_OPEN_IF_EXISTS,
			OPEN_FLAGS_SEQUENTIAL |
			OPEN_SHARE_DENYWRITE |
			OPEN_FLAGS_FAIL_ON_ERROR |
			OPEN_ACCESS_READONLY,
			(PEAOP2) NULL);
		if(rc != 0) {
			cd->HDev = (HFILE) NULL;
		}
		return;
	}

	/* For the swapfile method, we need to get the date and time of
	   the swapfile. Note that we need the date/time of modification,
	   not the date/time of creation, as the latter is not reset at
	   boot time. */

	if(value == DLG_METHOD_SWAPFILE) {	/* Use swapfile date/time */
		(VOID) DosQuerySysInfo(		/* Get boot drive */
			QSV_BOOT_DRIVE,
			QSV_BOOT_DRIVE,
			&res,
			sizeof(res));

		/* Generate full name for CONFIG.SYS, and open it. */

		sprintf(s, "%c:\\CONFIG.SYS", res + 'A' - 1);

		rc = DosOpen(
			s,			/* Name */
			&fh,			/* Handle returned here */
			&action,		/* Action returned here */
			0L,			/* Size */
			FILE_NORMAL,		/* Attributes */
			OPEN_ACTION_FAIL_IF_NEW |
			OPEN_ACTION_OPEN_IF_EXISTS,
			OPEN_FLAGS_SEQUENTIAL |
			OPEN_SHARE_DENYWRITE |
			OPEN_ACCESS_READONLY,
			(PEAOP2) NULL);		/* Extended attributes area */
		if(rc != 0) {
#ifdef	DEBUG
			dolog("Open %s failed, rc = %d", s, rc);
#endif
			return;
		}
#ifdef	DEBUG
		dolog("Opened %s OK", s);
#endif
		/* Scan CONFIG.SYS, and get the swap path. */

		nbytes = 0;
		found = FALSE;
		for(;;) {
			if(GetLine(fh, buf, &nbytes, line, &p) == 0) break;
			q = &line[0];
			while((*q == ' ') || (*q == '\t')) q++;/* Skip BWSP */
			if(strnicmp(key, q, strlen(key)) != 0) continue;
			/* Found it! */
			q += strlen(key);	/* Move past keyword */
			while((*q == ' ') || (*q == '\t')) q++;/* Skip BWSP */
			if(*q != '=') continue;	/* Badly formatted */
			q++;			/* Move past '=' */
			while((*q == ' ') || (*q == '\t')) q++;/* Skip BWSP */
			p = q;
			while((*p != '\0') && (*p != ' ') && (*p != '\t')) p++;
			found = TRUE;
			break;
		}
		(VOID) DosClose(fh);		/* Close CONFIG.SYS */

		if(found == FALSE) {
#ifdef	DEBUG
			dolog("Swap path not found");
#endif
			return;
		}

		*p = '\0';	/* 'q' points to the string required */
		strcpy(s, q);	/* Build full filename */
		strcat(s, "SWAPPER.DAT");
#ifdef	DEBUG
		dolog("Swap file is %s\n", s);
#endif

		/* Now get the date/time information for the swapfile */

		rc = DosQueryPathInfo(
			s,
			FIL_STANDARD,
			&fs,
			sizeof(fs));

		/* Convert the date/time value to seconds. */

		ts.tm_sec    = fs.ftimeLastAccess.twosecs*2;
		ts.tm_min    = fs.ftimeLastAccess.minutes;
		ts.tm_hour   = fs.ftimeLastAccess.hours;
		ts.tm_mday   = fs.fdateLastAccess.day;
		ts.tm_mon    = fs.fdateLastAccess.month;
		ts.tm_year   = fs.fdateLastAccess.year+80;
		ts.tm_isdst  = 0;
		cd->SwapBase = mktime(&ts);
#ifdef	DEBUG
		dolog("SwapBase = %#08x", cd->SwapBase);
#endif
		return;			/* All done */
	}
}


/*
 * Display the uptime in the title text.
 *
 */

static VOID ClientDisplayUptime(PCLIENTDATA cd)
{	DATETIME dt;
	ULONG up;			/* Uptime in seconds */
	UCHAR s1[15];
	
	up = UptimeSeconds(cd);

	if(up == 0) {
		strcpy(cd->Info, "<unknown>");
	} else {
		dt.seconds = up%60; up /= 60;
		dt.minutes = up%60; up /= 60;
		dt.hours   = up%24; up /= 24;
		dt.day     = up;

		if(dt.day == 0)
			strcpy(s1, "Up");
		else
			sprintf(s1, "Up %d day%s", dt.day, dt.day == 1 ? "" : "s");

		sprintf(
			cd->Info,
			"%s\n%02d:%02d:%02d",
			s1,
			dt.hours,
			dt.minutes,
			dt.seconds);

#ifdef	DEBUG
		switch(cd->method) {
			case DLG_METHOD_TIMER:
				strcat(cd->Info, " [T]");
				break;
			case DLG_METHOD_SWAPFILE:
				strcat(cd->Info, " [S]");
				break;
			case DLG_METHOD_DRIVER:
				strcat(cd->Info, " [D]");
				break;
			default:
				strcat(cd->Info, " [?]");
				break;
		}
#endif
	}

	_wpSetTitle(cd->so, cd->Info);
}


/*
 * Return the system uptime in seconds, using the specified method.
 *
 * Returns zero if there is an error.
 *
 */

static ULONG UptimeSeconds(PCLIENTDATA cd)
{	switch(cd->method) {
		case DLG_METHOD_SWAPFILE:
			return(UptimeSecondsSwapfile(cd));

		case DLG_METHOD_DRIVER:
			return(UptimeSecondsDriver(cd));

		case DLG_METHOD_TIMER:
		default:
			return(UptimeSecondsTimer());
	}
}


/*
 * Return the system uptime in seconds, using the high resolution timer.
 *
 * This method is accurate, but does not work past approximately 49 days,
 * and is inaccurate over suspend/resumes.
 *
 * Returns zero if there is an error.
 *
 */

static ULONG UptimeSecondsTimer(VOID)
{	APIRET rc;
	IO io;
	ULONG freq;
	QWORD time;

	rc = DosTmrQueryFreq(&freq);	/* Get timer ticks/second */
	if(rc != 0) return(0);

	rc = DosTmrQueryTime(&time);	/* Get ticks since boot */
	if(rc != 0) return(0);

	/* Set up for long division, and do it */

	io.dendhi = time.ulHi;
	io.dendlo = time.ulLo;
	io.divisor = freq;
	_dbldiv(&io);

	return(io.quot);		/* Uptime in seconds */
}


/*
 * Return the system uptime in seconds, using the date and time of
 * creation of the swapfile.
 *
 * This method fails if SWAPPATH in CONFIG.SYS has been edited since boot,
 * or if there is no SWAPPATH statement at all.
 *
 * Returns zero if there is an error.
 *
 */

static ULONG UptimeSecondsSwapfile(PCLIENTDATA cd)
{	struct tm ts;
	ULONG now;
	DATETIME dt;

	if(cd->SwapBase == 0) return(0);	/* Not working */

	/* Get current date and time, and convert to seconds. */

	(VOID) DosGetDateTime(&dt);
	ts.tm_sec   = dt.seconds;
	ts.tm_min   = dt.minutes;
	ts.tm_hour  = dt.hours;
	ts.tm_mday  = dt.day;
	ts.tm_mon   = dt.month;
	ts.tm_year  = dt.year-1900;
	ts.tm_isdst = 0;
	now = mktime(&ts);

#ifdef	DEBUG
	dolog("Time now = %#08x", now);
	dolog("Uptime = %#08x", now - cd->SwapBase);
#endif
	return(now - cd->SwapBase);
}


/*
 * Return the system uptime in seconds, using the UPTIME$ device.
 *
 * This method fails if the device driver is not loaded.
 *
 * Returns zero if there is an error.
 *
 */

static ULONG UptimeSecondsDriver(PCLIENTDATA cd)
{	APIRET rc;
	ULONG res, actual;

	if(cd->HDev == (HFILE) NULL) return(0);	/* No open device */

	rc = DosRead(cd->HDev, &res, 4, &actual);

	if(rc != 0) res = 0;

	return(res);
}


/*
 * Read a line into 'line' from file handle 'fh', using 'buf' as an
 * intermediate buffer, and updating buffer pointer 'pp' and remaining
 * byte count 'nbytes'.
 *
 * Returns:
 *	0	no more to read
 *	1	line has been read
 *
 */

static INT GetLine(HFILE fh, PUCHAR buf, PULONG nbytes, PUCHAR line, PUCHAR *pp)
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
		if((c == '\n') || (n >= LINESIZE-1)) {
			line[n] = '\0';
			break;
		}
		line[n++] = c;
	}
	*pp = p;

	return(1);
}

/*
 * End of file: client.c
 *
 */
