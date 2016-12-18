/*
 * File: pagedlg.c
 *
 * Class for Workplace Shell uptime monitor.
 *
 * Page dialog procedures
 *
 * Bob Eager   October 2004
 *
 */

#pragma	strings(readonly)

#pragma	hdrfile "main.pch"
#define	INCL_DOS
#define	INCL_DOSERRORS
#define	INCL_WIN
#include <os2.h>
#pragma	hdrstop

#pragma	alloc_text(a_dlg_seg, OptionsPageDlgProc)
#pragma	alloc_text(a_dlg_seg, OptionsPageStartState)
#pragma	alloc_text(a_dlg_seg, OptionsPageSetData)
#pragma	alloc_text(a_dlg_seg, SetRadioButtons)
#pragma	alloc_text(a_dlg_seg, SetRadioButton)

#include <time.h>

#include "uptime.ih"

#include "dialog.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <types.h>

/* Structure definitions */

typedef	struct _OPTIONSDLGDATA	{
short		cb;		/* Size of structure */
HWND		hwnd;		/* Page window handle */
uptime		*so;		/* Instance pointer (somSelf) */
Environment	*env;		/* Environment pointer */
ULONG		refreshinterval;/* Refresh interval in seconds */
ULONG		method;		/* Uptime determination method */
} OPTIONSDLGDATA, *POPTIONSDLGDATA;

/* Forward references */

static	VOID	OptionsPageSetData(POPTIONSDLGDATA);
static	VOID	OptionsPageStartState(POPTIONSDLGDATA);
static	VOID	SetRadioButtons(HWND, ULONG);
static	VOID	SetRadioButton(HWND, ULONG, INT);

/***********************************************************************
*                                                                      *
*                  Options settings page                               *
*                                                                      *
***********************************************************************/

/*
 * Options settings page dialog procedure
 *
 */

MRESULT EXPENTRY OptionsPageDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{	POPTIONSDLGDATA od;
	MRESULT rc;

	switch(msg) {
		case WM_INITDLG:	/* Dialog window is about to open */
			/* Set up window state structure */
			od = (POPTIONSDLGDATA) _wpAllocMem((uptime *) mp2,
						sizeof(OPTIONSDLGDATA), NULL);
			WinSetWindowPtr(hwnd, QWL_USER, od);
			od->cb = sizeof(OPTIONSDLGDATA);/* Structure size */
			od->so = (uptime *) mp2;	/* Instance pointer (somSelf) */
			od->env = somGetGlobalEnvironment();/* Env. pointer */
			od->hwnd = hwnd;		/* Window handle */
			OptionsPageStartState(od);	/* Set control states */
			return((MRESULT) FALSE);

		case WM_COMMAND:	/* Command from control */
			od = (POPTIONSDLGDATA) WinQueryWindowPtr(hwnd,
								QWL_USER);
			switch(SHORT1FROMMP(mp1)) {
				/* Restore to values set when dialog opened */
				case DLG_OPTIONS_UNDO:
					WinSendMsg(
						WinWindowFromID(hwnd, DLG_INTERVAL),
						SPBM_SETCURRENTVALUE,
						(MPARAM) od->refreshinterval,
						(MPARAM) 0);
					SetRadioButtons(od->hwnd, od->method);
					OptionsPageSetData(od);
					break;

				case DLG_OPTIONS_DEFAULT:
					/* Restore to default values */
					WinSendMsg(
						WinWindowFromID(hwnd, DLG_INTERVAL),
						SPBM_SETCURRENTVALUE,
						(MPARAM) DEFAULT_REFRESH_INTERVAL,
						(MPARAM) 0);
					SetRadioButtons(od->hwnd, DEFAULT_METHOD);
					OptionsPageSetData(od);
					break;

				case DLG_OPTIONS_HELP:
					break;
			}
			return((MRESULT) 0);

		case WM_DESTROY:	/* Window being destroyed */
			od = (POPTIONSDLGDATA) WinQueryWindowPtr(hwnd,
								QWL_USER);
			OptionsPageSetData(od);
			_wpFreeMem(od->so, (PBYTE) od);
			break;

		case WM_CONTROL:	/* Control window messages */
			od = (POPTIONSDLGDATA) WinQueryWindowPtr(hwnd,
								QWL_USER);
			/* If any option has been altered, update the
			   data straight away. */

			switch(SHORT1FROMMP(mp1)) {
				case DLG_INTERVAL:
					if(SHORT2FROMMP(mp1) == SPBN_KILLFOCUS) {
						rc = WinDefDlgProc(hwnd, msg, mp1, mp2);
						OptionsPageSetData(od);
						return(rc);
					}

				case DLG_METHOD_SWAPFILE:
				case DLG_METHOD_TIMER:
				case DLG_METHOD_DRIVER:
					if(SHORT2FROMMP(mp1) == BN_CLICKED) {
						OptionsPageSetData(od);
						return((MRESULT) 0);
					}
			}
			break;
	}

	return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}


/*
 * Options page; set values from dialog window into instance data.
 *
 */

static void OptionsPageSetData(POPTIONSDLGDATA od)
{	ULONG lTemp;

	WinSendDlgItemMsg(
		od->hwnd,
		DLG_INTERVAL,
		SPBM_QUERYVALUE,
		&lTemp,
		MPFROM2SHORT(0, SPBQ_ALWAYSUPDATE));
	if(lTemp != __get_refreshinterval(od->so, od->env)) {
		ClientSetRefreshInterval(od->so, od->env, lTemp);
		__set_refreshinterval(od->so, od->env, lTemp);
	}

	lTemp = LONGFROMMR(			/* Retrieve uptime method (0 based) */
			WinSendDlgItemMsg(
				od->hwnd,
				DLG_METHOD_SWAPFILE,
				BM_QUERYCHECKINDEX,
				MPFROMLONG(0),
				MPVOID));
	lTemp += DLG_METHOD_SWAPFILE;		/* Convert to button ID */
	if(lTemp != __get_method(od->so, od->env)) {
		ClientSetMethod(od->so, od->env, lTemp);
		__set_method(od->so, od->env, lTemp);
	}

	_wpSaveDeferred(od->so);	/* Ensure data are saved */
}


/*
 * Options page initial state.
 * Retrieve object instance data, using it to initialise the states
 * of the controls.
 *
 */

static void OptionsPageStartState(POPTIONSDLGDATA od)
{	HWND hwndOptionsInterval = WinWindowFromID(od->hwnd, DLG_INTERVAL);
	HWND hwndOptionsMethod = WinWindowFromID(od->hwnd, DLG_METHOD);

	/* Set limits on refresh interval spin button. */

	WinSendMsg(
		hwndOptionsInterval,
		SPBM_SETLIMITS,
		(MPARAM) MAX_REFRESH_INTERVAL,
		(MPARAM) MIN_REFRESH_INTERVAL);

	/* Retrieve stored interval setting, and save a copy in dialog data,
	   as well as initialising the spin-button field. */

	od->refreshinterval = __get_refreshinterval(od->so, od->env);
	WinSendMsg(
		hwndOptionsInterval,
		SPBM_SETCURRENTVALUE,
		(MPARAM) od->refreshinterval,
		(MPARAM) 0);
	WinEnableWindow(hwndOptionsInterval, TRUE);

	/* Retrieve stored method setting, and save a copy in dialog data,
	   as well as initialising the radio buttons. */

	od->method = __get_method(od->so, od->env);
	SetRadioButtons(od->hwnd, od->method);
}


/*           
 * Set the state of all radio buttons based on the current settings
 *
 */

static VOID SetRadioButtons(HWND hwnd, ULONG method)
{	SetRadioButton(hwnd, method, DLG_METHOD_SWAPFILE);
	SetRadioButton(hwnd, method, DLG_METHOD_TIMER);
	SetRadioButton(hwnd, method, DLG_METHOD_DRIVER);
}


/*
 * Set the state of a radio button based on the current settings
 *
 */

static VOID SetRadioButton(HWND hwnd, ULONG method, INT item)
{	(VOID) WinSendDlgItemMsg(
		hwnd,
		item,
		BM_SETCHECK,
		MPFROM2SHORT((method == item), 0),
		MPVOID);
}

/*
 * End of file: pagedlg.c
 *
 */
