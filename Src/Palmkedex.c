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
#include "Src/pokeInfo.h"
#include "Rsc/Palmkedex_Rsc.h"

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
#define ourMinVersion    sysMakeROMVersion(1,0,0,sysROMStageDevelopment,0)
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
	UInt16 idx;

	frmP = FrmGetActiveForm();
	if (!frmP)
		return NULL;

	idx = FrmGetObjectIndex(frmP, objectID);
	if (idx == frmInvalidObjectId)
		return NULL;

	return FrmGetObjectPtr(frmP, idx);
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

	sharedVars->sizeAfterFiltering = pokeGetNumber();

	err = FtrSet(appFileCreator, ftrShrdVarsNum, (UInt32)sharedVars);
	ErrFatalDisplayIf (err != errNone, "Failed to set feature memory");
}

static void LoadSpecies()
{
	SpeciesName *species;
	UInt16 i;
	Err err = errNone;

	species = (SpeciesName *)MemPtrNew(sizeof(SpeciesName) * pokeGetNumber());
	ErrFatalDisplayIf ((!species), "Out of memory");

	for (i = 0; i < pokeGetNumber(); i++)
	{
		pokeNameGet(species[i].name, i + 1 /* as pokes are 1-based */);
	}

	err = FtrSet(appFileCreator, ftrPkmnNamesNum, (UInt32)species);
	ErrFatalDisplayIf (err != errNone, "Failed to set feature memory");
}

static Err SetColorDepth(void)
{
	UInt32 supportedDepths, desiredDepth = 8, romVersion;
	Err err;

	//WinScreenMode only appears in PalmOS 3.0
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
		return errNone;

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
	void *ptr;

	if (errNone == FtrGet(appFileCreator, ftrPkmnNamesNum, (UInt32*)&ptr))
		MemPtrFree(ptr);

	FtrUnregister(appFileCreator, ftrPkmnNamesNum);
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
	MemPtrFree(sharedVars);
	FtrUnregister(appFileCreator, ftrShrdVarsNum);
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
	UInt32 val320 = 320, romVersion;
	UInt16 hrLibRef;
	Err e;

	//Library loading is broken on OS1, and no sony devices shipped with less than 3, so give up early
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
		return sysErrLibNotFound;

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
