#include <PalmOS.h>
#include <SonyCLIE.h>

#include "Palmkedex.h"
#include "Src/pokeInfo.h"
#include "Rsc/Palmkedex_Rsc.h"
#include "Src/osPatches.h"


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

static void makePokeFirstLetterLists(void)
{
	SharedVariables *sharedVars;
	UInt16 i, counts[26] = {0};

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

	//count how many pokes start from each letter
	for (i = 1; i <= pokeGetNumber(); i++) {

		char startingLetter = pokeNameGet(i)[0];
		UInt8 arrIdx;

		//upper-case it
		if (startingLetter >= 'a' && startingLetter <= 'z')
			startingLetter += 'A' - 'a';
		arrIdx = startingLetter - 'A';

		if (arrIdx >= 26)
			continue;

		counts[arrIdx]++;
	}

	//alloc memory and make the lists
	for (i = 0; i < 26; i++)
		sharedVars->pokeIdsPerEachStartingLetter[i] = MemPtrNew(sizeof(UInt16) * (counts[i] + 1));

	//populate pokemon into lists
	MemSet(counts, sizeof(counts), 0);
	for (i = 1; i <= pokeGetNumber(); i++) {

		char startingLetter = pokeNameGet(i)[0];
		UInt8 arrIdx;

		//upper-case it
		if (startingLetter >= 'a' && startingLetter <= 'z')
			startingLetter += 'A' - 'a';
		arrIdx = startingLetter - 'A';

		if (arrIdx >= 26)
			continue;
		sharedVars->pokeIdsPerEachStartingLetter[arrIdx][counts[arrIdx]++] = i;
	}

	//terminate the lists
	for (i = 0; i < 26; i++)
		sharedVars->pokeIdsPerEachStartingLetter[i][counts[i]] = 0;
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
	pokeInfoInit();
	MakeSharedVariables();
	makePokeFirstLetterLists();
	SetColorDepth();

	return errNone;
}

static void FreeSharedVariables()
{
	SharedVariables *sharedVars;
	Err err = errNone;
	UInt16 i;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");

	for (i = 0; i < 26; i++) {

		if (sharedVars->pokeIdsPerEachStartingLetter[i])
			MemPtrFree(sharedVars->pokeIdsPerEachStartingLetter[i]);
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
	FreeSharedVariables();
	/* Close all the open forms. */
	FrmCloseAllForms();
	pokeInfoDeinit();
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

UInt32 __attribute__((noinline)) PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	//RomVersionCompatible not needed since app supports PalmOS 1.0 :)

	if (cmd == sysAppLaunchCmdNormalLaunch) {

		UInt16 sonyHrLibRef;


		error = AppStart();
		if (error)
			return error;

		error = loadSonyHrLib(&sonyHrLibRef);
		if (error)
			sonyHrLibRef = 0xffff;

		osPatchesInstall();

		/*
		 * start application by opening the main form
		 * and then entering the main event loop
		 */
		FrmGotoForm(MainForm);
		AppEventLoop();

		osPatchesRemove();

		if (sonyHrLibRef != 0xffff && errNone == HRClose(sonyHrLibRef))
			SysLibRemove(sonyHrLibRef);

		AppStop();
	}

	return errNone;
}

UInt32 __attribute__((section(".vectors"), used)) __Startup__(void)
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
