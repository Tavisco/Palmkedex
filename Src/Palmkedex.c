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

void GoToPreferredMainForm(void)
{
	Boolean foundPrefs;
	PalmkedexPrefs *prefs;
	UInt16 latestPrefSize, mainFormId;

	latestPrefSize = sizeof(PalmkedexPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
		return;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		SysFatalAlert("Failed to load preferences!");
		return;
	}

	switch ( prefs->mainFormFormat)
	{
		case 1:
			mainFormId = GridMainForm;
			break;
		default:
			mainFormId = MainForm;
			break;
	}

	FrmGotoForm(mainFormId);
	MemPtrFree(prefs);
}

void * GetObjectPtr(const UInt16 objectID)
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

void SetFieldText(const UInt16 objectID, const Char* text)
{
	FieldType *fldP;
	MemHandle newTextH, oldTextH;
	char *str;

	fldP = GetObjectPtr(objectID);
	// Get the current text handle for the field, if any
	oldTextH = FldGetTextHandle(fldP);
	// Have the field stop using that handle
	FldSetTextHandle(fldP, NULL);

	// If there is a handle, free it
	if (oldTextH != NULL)
	{
		MemHandleFree(oldTextH);
	}

	// Create a new memory chunk
	// the +1 on the length is for
	// the null terminator
	newTextH = MemHandleNew(StrLen(text) + 1);
	// Allocate it, and lock
	str = MemHandleLock(newTextH);

	// Copy our new text to the memory chunk
	StrCopy(str, text);
	// and unlock it
	MemPtrUnlock(str);

	// Have the field use that new handle
	FldSetTextHandle(fldP, newTextH);
	FldDrawField(fldP);
}

Boolean isAdventureModeEnabled(void)
{
	Boolean foundPrefs, adventureMode;
	PalmkedexPrefs *prefs;
	UInt16 latestPrefSize;

	latestPrefSize = sizeof(PalmkedexPrefs);
	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
		return false;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		SysFatalAlert("Failed to load preferences!");
		return false;
	}

	adventureMode = prefs->adventureMode;
	MemPtrFree(prefs);

	return adventureMode;
}

UInt8 getPokeAdventureStatus(const UInt16 selectedPkmnId)
{
	Boolean foundPrefs, caught, seen;
	PerPokemonPrefs *prefs;
	UInt16 latestPrefSize;


	latestPrefSize = sizeof(struct PerPokemonPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to load per-poke preferences!");
		return 0;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(prefsCaughtCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		SysFatalAlert("Failed to load per-poke preferences!");
		return 0;
	}

	caught = checkPerPokeBit(prefs->caught, selectedPkmnId - 1);
	seen = checkPerPokeBit(prefs->seen, selectedPkmnId - 1);

	MemPtrFree(prefs);

	if (caught)
		return POKE_ADVENTURE_CAUGHT;

	if (seen)
		return POKE_ADVENTURE_SEEN;

	return POKE_ADVENTURE_NOT_SEEN;
}

static void InitPreferences(void)
{
	Boolean foundPrefs, initializePrefs;
	PalmkedexPrefs *prefs;
	UInt16 latestPrefSize;
	DmOpenRef iconDBRef;

	latestPrefSize = sizeof(PalmkedexPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
		return;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);

	initializePrefs = !foundPrefs;

	if (foundPrefs && prefs->prefsVersion != latestPrefVersion)
	{
		FrmAlert(VersionUpdateAlert);
		initializePrefs = true;
	}

	if (initializePrefs) {
		// Set default values
		// in that case, we check if the user has the icon pack installed
		// by trying to open the icon database, if it succeed, then the user
		// has is installed, thus, make it default.
		iconDBRef = DmOpenDatabaseByTypeCreator(ICON_RESOURCE_DB, appFileCreator, dmModeReadOnly);
		prefs->mainFormFormat = iconDBRef? 1 : 0;

		prefs->mainUnderGraffitiType = 0;
		prefs->prefsVersion = latestPrefVersion;

		prefs->adventureMode = FrmAlert(AdventureModeAlert);

		if (iconDBRef)
			DmCloseDatabase(iconDBRef);

		PrefSetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	}

	MemPtrFree(prefs);
}

static void InitCaughtPreferences(void)
{
	Boolean foundPrefs, initializePrefs;
	PerPokemonPrefs *prefs;
	UInt16 latestPrefSize;
	UInt16 arraySize;

	latestPrefSize = sizeof(PerPokemonPrefs);
	arraySize = (TOTAL_POKE_COUNT_ZERO_BASED + 7) / 8;

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store cuaght preferences!");
		return;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(prefsCaughtCreator, appPrefVersionNum, prefs, latestPrefSize);

	initializePrefs = !foundPrefs;

	if (foundPrefs && prefs->prefsVersion != latestCaughtPrefVersion)
	{
		FrmAlert(VersionUpdateAlert);
		initializePrefs = true;
	}

	if (initializePrefs) {
		// Set default values
		prefs->prefsVersion = latestCaughtPrefVersion;

		// Initialize the arrays clearing all bits
		for (UInt16 i = 0; i < arraySize; i++)
		{
			prefs->caught[i] = 0;
			prefs->seen[i] = 0;
		}

		PrefSetAppPreferencesV10(prefsCaughtCreator, appPrefVersionNum, prefs, latestPrefSize);
	}

	MemPtrFree(prefs);
}

// Used to modify the bit in the PerPokemonPrefs arrays
void modifyPerPokeBit(unsigned char array[], int x, int value) {
    if (value == 1) {
        // Set the bit to 1
        array[x / 8] |= (1 << (x % 8));
    } else if (value == 0) {
        // Unset the bit to 0
        array[x / 8] &= ~(1 << (x % 8));
    } else {
		SysFatalAlert("Trying to set the per-poke bit to non-boolean!");
	}
}

// Used to check the bit in the PerPokemonPrefs arrays
int checkPerPokeBit(unsigned char array[], const unsigned int x) {
	return (array[x / 8] & (1 << (x % 8))) != 0;
}

static Boolean AppHandleEvent(const EventType * eventP)
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
			case PrefsForm:
				FrmSetEventHandler(frmP, PrefsFormHandleEvent);
				break;
			default:
				return false;
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
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const UInt16 *chains;
	UInt16 i;

	sharedVars->indexHandle = DmGet1Resource('INDX', 0);

	WinGetDisplayExtent(&sharedVars->prevDispW, &sharedVars->prevDispH);

	chains = MemHandleLock(sharedVars->indexHandle);

	//point each chain properly
	for (i = 0; i < 26; i++)
		sharedVars->pokeIdsPerEachStartingLetter[i] = chains + chains[i];
}

void drawBackButton(const UInt16 buttonID)
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
	UInt32 supportedDepths, desiredDepth = 16, romVersion;
	Err err;

	//WinScreenMode only appears in PalmOS 3.0
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
		return errNone;

	err = WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &supportedDepths, NULL);
	if (err)
		return err;

	//set highest available color depth, but not more than 8
	while (desiredDepth) {
		const UInt32 desiredDepthMask = 1UL << desiredDepth - 1;

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
	*globalsSlotPtr(GLOBALS_SLOT_PCE_CALL_FUNC) = NULL;
	pokeInfoInit();
	MakeSharedVariables();
	makePokeFirstLetterLists();
	SetColorDepth();
	InitPreferences();
	InitCaughtPreferences();

	return errNone;
}

static void FreeSharedVariables(void)
{
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	MemHandleUnlock(sharedVars->indexHandle);
	DmReleaseResource(sharedVars->indexHandle);

	MemPtrFree(sharedVars);
	*globalsSlotPtr(GLOBALS_SLOT_SHARED_VARS) = NULL;
}

static void CloseIconDatabase(void)
{
	DmOpenRef dbRef = globalsSlotVal(GLOBALS_SLOT_ICON_DB);
	if (dbRef){
		*globalsSlotPtr(GLOBALS_SLOT_ICON_DB) = NULL;
		DmCloseDatabase(dbRef);
	}
}

static void AppStop(void)
{
	FreeSharedVariables();
	CloseIconDatabase();
	/* Close all the open forms. */
	FrmCloseAllForms();
	pokeInfoDeinit();
}

static Boolean sysHasNotifMgr(void)
{
	UInt32 notifMgrVer;

	return errNone == FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &notifMgrVer) && notifMgrVer;
}

static Err myDisplayChangedNotifHandler(const SysNotifyParamType *notifyParamsP)
{
	SharedVariables *sharedVars = notifyParamsP->userDataP;
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
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
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

UInt32 __attribute__((noinline)) PilotMain(const UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
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
		GoToPreferredMainForm();
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

Boolean isHighDensitySupported(void)
{
#ifdef SUPPORT_PALM_HIGH_DENSITY
	UInt32 version;

	return errNone == FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version) && version >= 4;
#else
	return false;
#endif
}

Boolean isSonyHiResSupported(void)
{
#ifdef SONY_HIRES_SUPPORT
	UInt16 hrLibRef;

	return errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff;
#else
	return false;
#endif
}

UInt16 getScreenDensity(void)
{
	if (isHighDensitySupported()) {

		UInt32 tmp;

		if (errNone == WinScreenGetAttribute(winScreenDensity, &tmp))
			return tmp;
		else
			return kDensityLow;
	}
	else if (isSonyHiResSupported()) {

		return kDensityDouble;
	}
#ifdef HANDERA_SUPPORT
	else if (isHanderaHiRes()) {

		VgaRotateModeType curRot;
		VgaScreenModeType curMod;

		VgaGetScreenMode(&curMod, &curRot);

		if (curMod == screenMode1To1)	//1:1 mode  - 1.5 density
			return kDensityOneAndAHalf;
		else							//auto-magnified mode: basically a shitty low density screen with ugly shapes
			return kDensityLow;
	}
#endif
	else {

		return kDensityLow;
	}
}

Boolean isPalmOsAtLeast(const UInt32 ver) {
	UInt32 romVersion;

	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	return romVersion < ver;
}