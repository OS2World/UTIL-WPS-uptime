/*
 * File: miscdefs.h
 *
 * Class for Workplace Shell uptime monitor.
 *
 * Miscellaneous definitions used by programs and by IPF.
 *
 * Bob Eager   October 2004
 *
 */

/*
 * History:
 *	1.0	Initial version.
 *	1.1	Changed to use IRQ0 timer instead of swapfile date.
 *	1.2	Changed to use choice of timer, swapfile date or result
 *		from device driver.
 *	1.3	Revised device driver, and added BLDLEVEL information.
 */

#define	ABOUT_TEXT	"uptime class\r" \
			"(system uptime monitor)\r" \
			"Version %d.%d\r" \
			"Copyright (C)\r" \
			"Bob Eager 1999,2004"

#define	VERSION			1	/* Major version number */
#define	EDIT			3	/* Edit number within major version */

#define	MAXABOUT			100	/* Max length of ABOUT_TEXT */
#define	MAXTEMP				200

#define	DEFAULT_REFRESH_INTERVAL	10
#define	MIN_REFRESH_INTERVAL		1
#define	MAX_REFRESH_INTERVAL		7200
#define	DEFAULT_METHOD			DLG_METHOD_TIMER

/*
 * End of file: miscdefs.h
 *
 */

