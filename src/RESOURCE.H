/*
 * File: resource.h
 *
 * Class for Workplace Shell uptime monitor.
 *
 * Common resource header file
 *
 * Bob Eager   October 2004
 *
 */

/* Icon resources */

#define	ICN_MAIN	1

/* Menu resources */

#define	ID_POPUPMENU1	(WPMENUID_USER+1)
#define	IDM_CLOSEVIEW	(WPMENUID_USER+2)
#define	IDM_ABOUT	(WPMENUID_USER+3)

#define	ID_POPUPMENU2	(WPMENUID_USER+4)
#define	IDM_WATCH	(WPMENUID_USER+5)

/* Views */

/* Note that the ID for a new view MUST have the same value as the
   corresponding menu item that invokes that view. */

#define	OPEN_WATCH	IDM_WATCH

/*
 * End of file: resource.h
 *
 */
