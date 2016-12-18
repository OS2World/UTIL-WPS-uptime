/*
 * File: common.h
 *
 * Class for Workplace Shell uptime monitor.
 *
 * Common header file
 *
 * Bob Eager   October 2004
 *
 */

#include "miscdefs.h"

/* Debug box */

#define DebugBox(title, text) \
			WinMessageBox(  \
			HWND_DESKTOP,   \
			HWND_DESKTOP,   \
			(PSZ) text,     \
			(PSZ) title,    \
			20,             \
			MB_OK | MB_INFORMATION | MB_MOVEABLE)

/* Structure definitions */

typedef struct _CLIENTDATA {
USHORT		cb;			/* Size of structure */
uptime		*so;			/* somSelf */
Environment	*env;			/* Environment */
HMODULE		hmod;			/* Class module handle */
HOBJECT		ObjHandle;		/* Persistent object handle */
PUCHAR		TitleText;		/* Original object title text */
ULONG		refreshinterval;	/* Refresh interval (secs) */
ULONG		method;			/* Method used to get uptime */
BOOL		Stopping;		/* Tells monitor thread to stop */
HWND		hwnd;			/* Dialog window handle */
HEV		Sem;			/* Semaphore for timing and thread control */
HEV		ThreadSem;		/* Semaphore to sync with dying thread */
ULONG		MonitorTID;		/* Monitor thread ID */
USEITEM		UseItem;		/* For object use list; keep with next */
VIEWITEM	ViewItem;		/* For object use list; keep with previous*/
HFILE		HDev;			/* Handle to UPTIME$ device */
ULONG		SwapBase;		/* Swapfile base (creation date/time) */
UCHAR		Info[50];		/* Static place to hold date/time info */
} CLIENTDATA, *PCLIENTDATA;

typedef struct _CAINFO {
USHORT		cb;			/* Size of structure */
HMODULE		hmod;			/* DLL module handle */
HWND		hwnd;			/* About dialog box handle */
UCHAR		text[MAXABOUT];		/* About text */
} CAINFO, *PCAINFO;

/* External references */

extern	HWND	Client(PCLIENTDATA);
extern	BOOL	ClientAbout(HMODULE);
extern	BOOL	ClientCloseView(uptimeData *, Environment *);
extern	void	ClientInfo(PCLIENTDATA);
extern	void	ClientSetMethod(uptime *, Environment *, ULONG);
extern	void	ClientSetRefreshInterval(uptime *, Environment *, ULONG);

extern	MRESULT EXPENTRY OptionsPageDlgProc(HWND hwnd, ULONG msg,
						MPARAM mp1, MPARAM mp2);

/* User-defined window messages */

#define	UM_RESULTS	(WM_USER)	/* Reporting results */

/*
 * End of file: common.h
 *
 */
