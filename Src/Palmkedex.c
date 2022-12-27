/*
 * Palmkedex.c
 *
 * main file for Palmkedex
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */

#include <PalmOS.h>
#include <PalmOSGlue.h>
#include <SonyCLIE.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"
#include "Rsc/pkmn_names.h"

/*********************************************************************
 * Entry Points
 *********************************************************************/

/*********************************************************************
 * Global variables
 *********************************************************************/



/*********************************************************************
 * Internal Constants
 *********************************************************************/

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)

/*********************************************************************
 * Internal Functions
 *********************************************************************/

/*
 * FUNCTION: GetObjectPtr
 *
 * DESCRIPTION:
 *
 * This routine returns a pointer to an object in the current form.
 *
 * PARAMETERS:
 *
 * formId
 *     id of the form to display
 *
 * RETURNED:
 *     address of object as a void pointer
 */

void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}


/*
 * FUNCTION: AppHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine loads form resources and set the event handler for
 * the form loaded.
 *
 * PARAMETERS:
 *
 * event
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed
 *     to a higher level handler.
 */

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType * frmP;

	if (eventP->eType == frmLoadEvent)
	{
		/* Load the form resource. */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/*
		 * Set the event handler for the form.  The handler of the
		 * currently active form is called by FrmHandleEvent each
		 * time is receives an event.
		 */
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;
			case PkmnMainForm:
				FrmSetEventHandler(frmP, PkmnMainFormHandleEvent);
				break;
			case PkmnTypeForm:
				FrmSetEventHandler(frmP, PkmnTypeFormHandleEvent);
				break;
		}
		return true;
	}

	return false;
}

/*
 * FUNCTION: AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.
 */

static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do
	{
		/* change timeout if you need periodic nilEvents */
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
		{
			if (! MenuHandleEvent(0, &event, &error))
			{
				if (! AppHandleEvent(&event))
				{
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}


static void MakeSharedVariables()
{
	SharedVariables *sharedVars;
	Err err = errNone;

	sharedVars = (SharedVariables *)MemPtrNew(sizeof(SharedVariables));
	ErrFatalDisplayIf ((!sharedVars), "Out of memory");
	MemSet(sharedVars, sizeof(SharedVariables), 0);

	sharedVars->sizeAfterFiltering = PKMN_QUANTITY;

	err = FtrSet(appFileCreator, ftrShrdVarsNum, (UInt32)sharedVars);
	ErrFatalDisplayIf (err != errNone, "Failed to set feature memory");
}

static void LoadSpecies()
{
	Species *species;
	UInt16 i;
	Err err = errNone;

	species = (Species *)MemPtrNew(sizeof(Species));
	ErrFatalDisplayIf ((!species), "Out of memory");
	MemSet(species, sizeof(Species), 0);

	for (i = 0; i < PKMN_QUANTITY; i++)
	{
		StrCopy(species->nameList[i].name, pkmnsNames[i].name);
	}

	err = FtrSet(appFileCreator, ftrPkmnNamesNum, (UInt32)species);
	ErrFatalDisplayIf (err != errNone, "Failed to set feature memory");
}

static Err SetColorDepth(void)
{
	UInt32 supportedDepths, desiredDepth = 8;
	Err err;

	err = WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &supportedDepths, NULL);
	if (err)
		return err;

	//set highest available color depth, but not more than 8
	while (desiredDepth) {

		UInt32 desiredDepthMask = (1UL << (desiredDepth - 1));

		if (supportedDepths & desiredDepthMask)
			return WinScreenMode(winScreenModeSet, NULL, NULL, &desiredDepth, NULL);

		desiredDepth >>= 1;
	}

	SysFatalAlert("As of now, Palmkedex does not support this device's screen.");

	return errNone;
}

/*
 * FUNCTION: AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * RETURNED:
 *     errNone - if nothing went wrong
 */

static Err AppStart(void)
{
	LoadSpecies();
	MakeSharedVariables();
	SetColorDepth();

	return errNone;
}

static void UnloadSpecies()
{
	FtrPtrFree(appFileCreator, ftrPkmnNamesNum);
}

static void FreeSharedVariables()
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	if ((UInt32)sharedVars->filteredList != 0)
	{
		MemPtrFree(sharedVars->filteredList);
	}

	if ((UInt32)sharedVars->filteredPkmnNumbers != 0)
	{
		MemPtrFree(sharedVars->filteredPkmnNumbers);
	}

	if ((UInt32)sharedVars->pkmnFormTitle != 0)
	{
		MemPtrFree(sharedVars->pkmnFormTitle);
	}

	FtrPtrFree(appFileCreator, ftrShrdVarsNum);
}

/*
 * FUNCTION: AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 */

static void AppStop(void)
{
    UnloadSpecies();
	FreeSharedVariables();
	/* Close all the open forms. */
	FrmCloseAllForms();

}

/*
 * FUNCTION: RomVersionCompatible
 *
 * DESCRIPTION:
 *
 * This routine checks that a ROM version is meet your minimum
 * requirement.
 *
 * PARAMETERS:
 *
 * requiredVersion
 *     minimum rom version required
 *     (see sysFtrNumROMVersion in SystemMgr.h for format)
 *
 * launchFlags
 *     flags that indicate if the application UI is initialized
 *     These flags are one of the parameters to your app's PilotMain
 *
 * RETURNED:
 *     error code or zero if ROM version is compatible
 */

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags &
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);

			/* Palm OS versions before 2.0 will continuously relaunch this
			 * app unless we switch to another safe one. */
			if (romVersion < kPalmOS20Version)
			{
				AppLaunchWithCommand(
					sysFileCDefaultApp,
					sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}

	return errNone;
}

static Err loadSonyHrLib(UInt16 *hrLibRefP)
{
	UInt32 val320 = 320;
	UInt16 hrLibRef;
	Err e;

	e = SysLibFind(sonySysLibNameHR, &hrLibRef);
	if (e == sysErrLibNotFound)
		e = SysLibLoad(sysFileTLibrary, sonySysFileCHRLib, &hrLibRef);

	if (e == errNone)
		e = HROpen(hrLibRef);

	if (e == errNone) {

		e = HRWinScreenMode(hrLibRef, winScreenModeSet, &val320, &val320, NULL, NULL);
		if (e != errNone) {

			if (errNone == HRClose(hrLibRef))
				SysLibRemove(hrLibRef);
		}
	}

	*hrLibRefP = hrLibRef;

	return e;
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	if (cmd == sysAppLaunchCmdNormalLaunch) {

		UInt16 sonyHrLibRef;

		error = AppStart();
		if (error)
			return error;

		error = loadSonyHrLib(&sonyHrLibRef);
		if (error)
			sonyHrLibRef = 0xffff;

		/*
		 * start application by opening the main form
		 * and then entering the main event loop
		 */
		FrmGotoForm(MainForm);
		AppEventLoop();

		if (sonyHrLibRef != 0xffff && errNone == HRClose(sonyHrLibRef))
			SysLibRemove(sonyHrLibRef);

		AppStop();
	}

	return errNone;
}

UInt32 __attribute__((section(".vectors"))) __Startup__(void)
{
	SysAppInfoPtr appInfoP;
	void *prevGlobalsP;
	void *globalsP;
	UInt32 ret;

	SysAppStartup(&appInfoP, &prevGlobalsP, &globalsP);
	ret = PilotMain(appInfoP->cmd, appInfoP->cmdPBP, appInfoP->launchFlags);
	SysAppExit(appInfoP, prevGlobalsP, globalsP);

	return ret;
}
