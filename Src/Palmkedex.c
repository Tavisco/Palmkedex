#include "BUILD_TYPE.h"

#include <PalmOS.h>
#include <SonyCLIE.h>

#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "osPatches.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define palmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)

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
			case GridMainForm:
				FrmSetEventHandler(frmP, GridMainFormHandleEvent);
				break;
		}
		return true;
	}

	return false;
}

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
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const UInt16 *chains;
	UInt16 i;

	sharedVars->indexHandle = DmGet1Resource('INDX', 0);

	WinGetDisplayExtent(&sharedVars->prevDispW, &sharedVars->prevDispH);

	chains = MemHandleLock(sharedVars->indexHandle);

	//point each chain properly
	for (i = 0; i < 26; i++)
		sharedVars->pokeIdsPerEachStartingLetter[i] = chains + chains[i];
}

void drawBackButton(UInt16 buttonID)
{
	FormType *frm;
	Coord x, y;
	UInt16 buttonId;
	BitmapPtr bitmapP;
	MemHandle h;

	frm = FrmGetActiveForm();
	buttonId = FrmGetObjectIndex(frm, buttonID);
	FrmGetObjectPosition(frm, buttonId, &x, &y);

	h = DmGetResource(bitmapRsc, ReturnIconBitmap);
	bitmapP = (BitmapPtr)MemHandleLock(h);

	WinDrawBitmap(bitmapP, x, y);

	MemHandleUnlock(h);
	DmReleaseResource(h);
}

static void MakeSharedVariables(void)
{
	SharedVariables *sharedVars;
	Err err = errNone;

	sharedVars = (SharedVariables *)MemPtrNew(sizeof(SharedVariables));
	MemSet(sharedVars, sizeof(SharedVariables), 0);

	sharedVars->sizeAfterFiltering = TOTAL_POKE_COUNT_ZERO_BASED;
	sharedVars->selectedPkmnLstIndex = noListSelection;
	sharedVars->isQrDisplayed = false;

	*globalsSlotPtr(GLOBALS_SLOT_SHARED_VARS) = sharedVars;
}

static Err SetColorDepth(void)
{
#ifdef MORE_THAN_1BPP_SUPPORT
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
#endif
	return errNone;
}

static Err AppStart(void)
{
	pokeInfoInit();
	MakeSharedVariables();
	makePokeFirstLetterLists();
	SetColorDepth();

	return errNone;
}

static void FreeSharedVariables(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	UInt16 i;

	MemHandleUnlock(sharedVars->indexHandle);
	DmReleaseResource(sharedVars->indexHandle);

	MemPtrFree(sharedVars);
	*globalsSlotPtr(GLOBALS_SLOT_SHARED_VARS) = NULL;
}

static void AppStop(void)
{
	FreeSharedVariables();
	/* Close all the open forms. */
	FrmCloseAllForms();
	pokeInfoDeinit();
}

static Boolean sysHasNotifMgr(void)
{
	UInt32 notifMgrVer;

	return errNone == FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &notifMgrVer) && notifMgrVer;
}

static Err myDisplayChangedNotifHandler(SysNotifyParamType *notifyParamsP)
{
	SharedVariables *sharedVars = (SharedVariables*)notifyParamsP->userDataP;
	Coord newW, newH;
	EventType e;

	WinGetDisplayExtent(&newW, &newH);
	if (newW != sharedVars->prevDispW || newH != sharedVars->prevDispH) {

		e.eType = winDisplayChangedEvent;
		sharedVars->prevDispW = newW;
		sharedVars->prevDispH = newH;

		EvtAddEventToQueue(&e);
	}

	return errNone;
}

static Err subscribeToNotifs(void)
{
	Err e = errNone;
#ifdef SCREEN_RESIZE_SUPPORT
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	UInt16 myCard;
	LocalID myLID;

	if (!sysHasNotifMgr())
		return errNone;

	e = SysCurAppDatabase(&myCard, &myLID);

	if (e == errNone)
		e = SysNotifyRegister(myCard, myLID, sysNotifyDisplayChangeEvent, myDisplayChangedNotifHandler, sysNotifyNormalPriority, sharedVars);
	if (e == errNone)
		e = SysNotifyRegister(myCard, myLID, sysNotifyDisplayResizedEvent, myDisplayChangedNotifHandler, sysNotifyNormalPriority, sharedVars);
#endif
	return e;
}

static Err unsubFromNotifs(void)
{
	Err e = errNone;
#ifdef SCREEN_RESIZE_SUPPORT
	UInt16 myCard;
	LocalID myLID;

	if (!sysHasNotifMgr())
		return errNone;

	e = SysCurAppDatabase(&myCard, &myLID);

	if (e == errNone)
		e = SysNotifyUnregister(myCard, myLID, sysNotifyDisplayChangeEvent, sysNotifyNormalPriority);
	if (e == errNone)
		e = SysNotifyUnregister(myCard, myLID, sysNotifyDisplayResizedEvent, sysNotifyNormalPriority);
#endif
	return e;
}

static void setupHandera(void)
{
#ifdef HANDERA_SUPPORT
	if (isHanderaHiRes())
		VgaSetScreenMode(screenMode1To1, rotateModeNone);
#endif
}

static Err loadSonySilkLib(UInt16 *silkLibRefP, Boolean *useV2closeCallP)
{
#ifdef SONY_SILK_SUPPORT
	UInt32 romVersion, vskVersion;
	Err e;

	*useV2closeCallP = false;

	//Library loading is broken on OS1, and no sony devices shipped with less than 3, so give up early
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
		return sysErrLibNotFound;

	e = SysLibFind(sonySysLibNameSilk, silkLibRefP);
	if (e == sysErrLibNotFound)
		e = SysLibLoad(sysFileTLibrary, sonySysFileCSilkLib, silkLibRefP);

	if (e != errNone)
		return e;

	if (errNone != FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &vskVersion) || vskVersion == vskVersionNum1) {

		e = SilkLibOpen(*silkLibRefP);
		if (e == errNone)
			e = SilkLibEnableResize(*silkLibRefP);
	}
	else if (vskVersion == vskVersionNum2 || vskVersion == vskVersionNum3) {

		e = VskOpen(*silkLibRefP);
		if (e == errNone) {
			(void)VskSetState(*silkLibRefP, vskStateEnable, vskResizeVertically);
			(void)VskSetState(*silkLibRefP, vskStateEnable, vskResizeHorizontally);
		}
		*useV2closeCallP = true;
	}

	return e;
#else
	*silkLibRefP = 0xffff;
	return errNone;
#endif
}


static Err loadSonyHrLib(UInt16 *hrLibRefP)
{
#ifdef SONY_HIRES_SUPPORT
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
#else
	*hrLibRefP = 0xffff;
	return errNone;
#endif
}

UInt32 __attribute__((noinline)) PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	//RomVersionCompatible not needed since app supports PalmOS 1.0 :)

	if (cmd == sysAppLaunchCmdNormalLaunch) {

		UInt16 sonyHrLibRef, sonySilkLib;
		Boolean closeSilkLibUsingV2api;


		error = AppStart();
		if (error)
			return error;

		error = loadSonyHrLib(&sonyHrLibRef);
		if (error)
			sonyHrLibRef = 0xffff;

		error = loadSonySilkLib(&sonySilkLib, &closeSilkLibUsingV2api);
		if (error)
			sonySilkLib = 0xffff;

		(void)subscribeToNotifs();

		setupHandera();

		osPatchesInstall();

		/*
		 * start application by opening the main form
		 * and then entering the main event loop
		 */
		FrmGotoForm(GridMainForm);
		AppEventLoop();

		osPatchesRemove();

		(void)unsubFromNotifs();

		if (sonyHrLibRef != 0xffff)
			HRClose(sonyHrLibRef);

		if (sonySilkLib != 0xffff) {
			if (closeSilkLibUsingV2api)
				VskClose(sonySilkLib);
			else
				SilkLibClose(sonySilkLib);
		}

		AppStop();
	}

	return errNone;
}

UInt32 __attribute__((section(".vectors"), used)) __Startup__(void);
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

Boolean isHanderaHiRes(void)
{
#ifdef HANDERA_SUPPORT
	UInt32 handeraVersion;

	return errNone == FtrGet(TRGSysFtrID, TRGVgaFtrNum, &handeraVersion);
#else
	return false;
#endif
}

Boolean isPalmOS1(void)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	return romVersion < palmOS20Version;
}