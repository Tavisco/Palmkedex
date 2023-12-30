#include "BUILD_TYPE.h"

#include <PalmOS.h>

#include "Palmkedex.h"
#include "UiResourceIDs.h"
#include "pokeInfo.h"
#include "imgDraw.h"
#include "osExtra.h"
#include "qrcode/qrcode.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_IMAGE_AT_X					1
#define POKE_IMAGE_AT_X_HANDERA			1
#define POKE_IMAGE_AT_Y					16
#define POKE_IMAGE_AT_Y_HANDERA			24

#define POKE_IMAGE_SIZE					96
#define POKE_IMAGE_SIZE_HANDERA			144

#define POKE_TYPE_1_X					1
#define POKE_TYPE_1_X_HANDERA			1
#define POKE_TYPE_2_X					34
#define POKE_TYPE_2_X_HRES				52
#define POKE_TYPE_2_X_HANDERA			51

#define POKE_TYPE_Y						113
#define POKE_TYPE_Y_HANDERA				175
#define POKE_TYPE_HEIGHT				20
#define POKE_TYPE_HEIGHT_HANDERA		30

#define QR_OFFSET_X						3
#define QR_OFFSET_X_HANDERA				12
#define QR_OFFSET_Y						6
#define QR_OFFSET_Y_HANDERA				17
#define QR_MODULE_SIZE					3
#define QR_MODULE_SIZE_HANDERA			4

#define TYPE_EFF_X_TXT_OFFSET			17
#define TYPE_EFF_X_TXT_OFFSET_HANDERA	25
#define TYPE_EFF_X_OFFSET				41
#define TYPE_EFF_X_OFFSET_HANDERA		48
#define TYPE_EFF_Y_OFFSET				17
#define TYPE_EFF_Y_OFFSET_HANDERA		20

#define DANA_POTRAIT					1
#define DANA_LANDSCAPE					2

static const char noDexEntryString[31] = "This pokemon has no Dex Entry.";
static const char noStats [4] = "???";

static void DrawTypes(const struct PokeInfo *info);

static Int16 GetTypeEffXTxtOffset(const UInt8 danaMode)
{

	if (danaMode != 0)
		return POKE_TYPE_2_X - POKE_TYPE_1_X + 2;

	switch (getScreenDensity())
	{
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPE_EFF_X_TXT_OFFSET_HANDERA : TYPE_EFF_X_TXT_OFFSET;
	
	default:
		return TYPE_EFF_X_TXT_OFFSET;
	}
}

static Int16 GetTypeEffXOffset(const UInt8 danaMode)
{
	if (danaMode == DANA_LANDSCAPE)
		return TYPE_EFF_X_OFFSET + 38;

	if (danaMode == DANA_POTRAIT)
		return TYPE_EFF_X_OFFSET + 38;

	switch (getScreenDensity())
	{
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPE_EFF_X_OFFSET_HANDERA : TYPE_EFF_X_OFFSET;
	
	default:
		return TYPE_EFF_X_OFFSET;
	}
}

static Int16 GetTypeEffYOffset(void)
{
	switch (getScreenDensity())
	{
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPE_EFF_Y_OFFSET_HANDERA : TYPE_EFF_Y_OFFSET;
	
	default:
		return TYPE_EFF_Y_OFFSET;
	}
}

static UInt16 getType2X(void)
{
	switch (getScreenDensity())
	{
	case kDensityDouble:
		return POKE_TYPE_2_X_HRES;
	
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? POKE_TYPE_2_X_HANDERA : POKE_TYPE_2_X;

	default:
		return POKE_TYPE_2_X;
	}
}

static void showDexEntryPopup(void)
{
	const SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	char *dexEntry = NULL;

	dexEntry = pokeDescrGet(sharedVars->selectedPkmnId);

	if (dexEntry == NULL)
	{
		FrmCustomAlert(DexEntryAlert, noDexEntryString, "", "");
		return;
	}

	FrmCustomAlert(DexEntryAlert, dexEntry, " ", "");
	MemPtrFree(dexEntry);
}

static void DrawPkmnPlaceholder(void)
{
	MemHandle h;
	BitmapPtr bitmapP;
	h = DmGetResource(bitmapRsc, BmpMissingPokemon);

	bitmapP = (BitmapPtr)MemHandleLock(h);

	if (isHanderaHiRes())
		WinDrawBitmap(bitmapP, POKE_IMAGE_AT_X_HANDERA, POKE_IMAGE_AT_Y_HANDERA);
	else
		WinDrawBitmap(bitmapP, POKE_IMAGE_AT_X, POKE_IMAGE_AT_Y);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);
}

static void redrawDecodedSprite(struct DrawState *ds)
{
	if (isHanderaHiRes())
		imgDrawRedraw(ds, POKE_IMAGE_AT_X_HANDERA, POKE_IMAGE_AT_Y_HANDERA);
	else
		imgDrawRedraw(ds, POKE_IMAGE_AT_X, POKE_IMAGE_AT_Y);
}

static void clearPkmnImage(const Boolean includeTypes)
{
	RectangleType rect;
	const int pokeImageSize = isHanderaHiRes() ? POKE_IMAGE_SIZE_HANDERA : POKE_IMAGE_SIZE;
	const int pokeTypeHeight = isHanderaHiRes() ? POKE_TYPE_HEIGHT_HANDERA : POKE_TYPE_HEIGHT;

	rect.topLeft.x = isHanderaHiRes() ? POKE_IMAGE_AT_X_HANDERA : POKE_IMAGE_AT_X;
	rect.topLeft.y = isHanderaHiRes() ? POKE_IMAGE_AT_Y_HANDERA : POKE_IMAGE_AT_Y;
	rect.extent.x = pokeImageSize + 10;

	rect.extent.y = includeTypes ?  pokeImageSize + pokeTypeHeight : pokeImageSize; 
	WinEraseRectangle(&rect, 0);
}

// ReSharper disable CppLocalVariableMightNotBeInitialized
static void drawQr(const UInt16 selectedPkmnId, const bool useBitmap)
{
	char url[43];
	char pokeName[POKEMON_NAME_LEN + 1];

	QRCode *qrcode;
	uint8_t* qrcodeData;
	int moduleSize;
	int qrModifierX, qrModifierY, qrOffsetX, qrOffsetY;
	BitmapType *qrBmpP;
	WinHandle bmpWin;
	Err error;
	RectangleType rect;
	Coord x, y;

	pokeNameGet(pokeName, selectedPkmnId);
	StrCopy(url, "https://pokemondb.net/pokedex/");
	StrCat(url, pokeName);

	qrcode = MemPtrNew(sizeof(QRCode));
	qrcodeData = MemPtrNew(qrcode_getBufferSize(3) * sizeof(uint8_t));

	if (qrcode == NULL || qrcodeData == NULL)
	{
		ErrFatalDisplay("No memory for QR Code");
	}

	uint8_t ret = qrcode_initText(qrcode, qrcodeData, 3, ECC_MEDIUM, url);
	ErrFatalDisplayIf(ret != 0, "Error encoding QR Code");

 	moduleSize = isHanderaHiRes() ? QR_MODULE_SIZE_HANDERA : QR_MODULE_SIZE;
	x = isHanderaHiRes() ? POKE_IMAGE_AT_X_HANDERA : POKE_IMAGE_AT_X;
	y = isHanderaHiRes() ? POKE_IMAGE_AT_Y_HANDERA : POKE_IMAGE_AT_Y;

	// An offset is needed to center the QR code, as it is smaller than the pokemon sprite
	qrOffsetX = isHanderaHiRes() ? QR_OFFSET_X_HANDERA : QR_OFFSET_X;
	qrOffsetY = isHanderaHiRes() ? QR_OFFSET_Y_HANDERA : QR_OFFSET_Y;

	if (useBitmap) {
		qrBmpP = BmpCreate(qrcode->size * moduleSize, qrcode->size * moduleSize, 1, NULL, &error);
		ErrFatalDisplayIf(qrBmpP == NULL, "Error creating QR Code bitmap");

		bmpWin = WinCreateBitmapWindow(qrBmpP, &error);
		ErrFatalDisplayIf(bmpWin == NULL, "Error creating QR Code bitmap window");

		WinSetDrawWindow(bmpWin);
		qrModifierX = 0;
		qrModifierY = 0;
	} else {
		WinSetDrawWindow(WinGetDrawWindow());
		clearPkmnImage(false);

		qrModifierX = x + qrOffsetX;  // X coordinate of the QR Code
		qrModifierY = y + qrOffsetY;  // Y coordinate of the QR Code
	}

	for (int y1 = 0; y < qrcode->size; y1++) {
		for (int x2 = 0; x < qrcode->size; x2++) {
			if (qrcode_getModule(qrcode, x2, y1)) {
				rect.topLeft.x = x2 * moduleSize + qrModifierX;
				rect.topLeft.y = y1 * moduleSize + qrModifierY;
				rect.extent.x = moduleSize;
				rect.extent.y = moduleSize;
				WinDrawRectangle(&rect, 0);
			}
		}
	}

	if (useBitmap) {
		WinSetDrawWindow(WinGetDisplayWindow());
		clearPkmnImage(false);
		WinPaintBitmap(qrBmpP, x + qrOffsetX, y + qrOffsetY);
		WinDeleteWindow(bmpWin, false);
		BmpDelete(qrBmpP);
	}

	MemPtrFree(qrcode);
	MemPtrFree(qrcodeData);
}

static void DrawPkmnSprite(const UInt16 selectedPkmnId)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;

	// Check if the PNG for the current pkmn
	// is already decoded in memory
	ds = (struct DrawState*)globalsSlotVal(GLOBALS_SLOT_POKE_IMAGE);
	if (ds)
	{
		// If it is, draw it and return
		redrawDecodedSprite(ds);
		return;
	}

	// Check if there is any image for current pkmn
	imgMemHandle = pokeImageGet(selectedPkmnId, POKE_SPRITE);
	if (imgMemHandle) {
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), POKE_IMAGE_SIZE, POKE_IMAGE_SIZE, 0))
			redrawDecodedSprite(ds);
		else
			ds = NULL;
		MemHandleUnlock(imgMemHandle);
		pokeImageRelease(imgMemHandle, POKE_SPRITE);
	}
	// And store its pointer to quickly redraw it
	*globalsSlotPtr(GLOBALS_SLOT_POKE_IMAGE) = ds;

	if (!ds)
		DrawPkmnPlaceholder();

}

#ifdef SCREEN_RESIZE_SUPPORT
static void FreeDescriptionField(void)
{
	FieldType *fld = GetObjectPtr(PkmnMainDescriptionField);
	Char *ptr = FldGetTextPtr(fld);

	FldSetTextPtr(fld, noDexEntryString);

	if (ptr && ptr != (char*)noDexEntryString){
		MemPtrFree(ptr);
	}
		
}

static Boolean isHanderaCollapsed(const Coord width, const Coord height)
{
	return (height == width) && (height == 240);
}

static Boolean isLowResCollapsed(const Coord width, const Coord height)
{
	return height <= 160 && width <= 160;
}

static Boolean isHighResLandscape(const Coord width, const Coord height)
{
	// 560 is the width of Dana
	return height == 160 && (width > 160 && width != 560);
}

static UInt8 getDanaMode(const Coord width, const Coord height)
{
	if (height == 160 && width > 320)
		return DANA_LANDSCAPE;
	
	if (width == 160 && height > 320)
		return DANA_POTRAIT;

	return 0;
}

static void SetDescriptionField(const UInt16 selectedPkmnId)
{
	Coord width, height;
	FieldType *fld;
	RectangleType rect;
	FormType *frm;
	UInt8 danaMode;

	// Don't try to set description on devices that can't show them...
	frm = FrmGetActiveForm();
	WinGetWindowExtent(&width, &height);

	if (isHanderaCollapsed(width, height) || isLowResCollapsed(width, height) || isHighResLandscape(width, height))
	{
		FrmShowObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
		FreeDescriptionField();
		return;
	}

	fld = GetObjectPtr(PkmnMainDescriptionField);

	danaMode = getDanaMode(width, height);

	if (danaMode == DANA_LANDSCAPE)
	{
		rect.topLeft.x = POKE_IMAGE_AT_X + POKE_IMAGE_SIZE + 10;
		rect.extent.x = width - rect.topLeft.x - 59;
		rect.topLeft.y = 22;
		rect.extent.y = 58;
		FldSetBounds(fld, &rect);
	}

	char *text = pokeDescrGet(selectedPkmnId);

	if (!text)
		text = (char*)noDexEntryString;

	FreeDescriptionField();
	FldSetTextPtr(fld, text);
	FldRecalculateField(fld, true);

	FrmHideObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
	CtlSetUsable(GetObjectPtr(PkmnMainDexEntryButton), false);
}

static Int16 getInitialXForTypesMatchup(const UInt8 danaMode)
{
	if (danaMode == DANA_LANDSCAPE)
	{
		return POKE_IMAGE_AT_X + POKE_IMAGE_SIZE + 22;
	}

	if (danaMode == DANA_POTRAIT)
	{
		return 15;
	}
	
	return 0;
}

static Int16 getInitialYForTypesMatchup(const Int16 initialY, const UInt8 danaMode)
{
	if (danaMode == DANA_LANDSCAPE)
	{
		return 61;
	}

	if (danaMode == DANA_POTRAIT)
	{
		return initialY + 75;
	}

	return initialY;
}


static void DrawTypeEff(const UInt16 selectedPkmnId)
{
	UInt8 danaMode;
	Coord height, width;
	Int16 x, y;
	UInt8 i;
	FormType *frm;

	// Don't try to set description on devices that can't show them...
	WinGetWindowExtent(&width, &height);
	if (isHanderaCollapsed(width, height) || isLowResCollapsed(width, height))
	{
		return;
	}

	frm = FrmGetActiveForm();
	FrmGetObjectPosition(frm, FrmGetObjectIndex(frm, PkmnMainDescriptionField), &x, &y);

	danaMode = getDanaMode(width, height);

	x = getInitialXForTypesMatchup(danaMode);
	y = getInitialYForTypesMatchup(y, danaMode);

	for (i = PokeTypeFirst; i <= PokeTypeFairy; i++)
	{
		if (!DrawEffectiveness(selectedPkmnId, x + GetTypeEffXTxtOffset(danaMode), y, (enum PokeType)i, true))
			continue;

		drawBmpForType(i, x, y, getDanaMode(width, height)? false : true);

		x += GetTypeEffXOffset(danaMode);

		// The x >= 500 is the start of the buttons on the right of the screen in the Dana
		if ((danaMode == DANA_LANDSCAPE && x >= 500) || x >= width)
		{
			x = getInitialXForTypesMatchup(danaMode);
			y += GetTypeEffYOffset();
		}
	}
}
#endif

static void SetAdventureCheckboxes(const UInt8 adventureStatus)
{
	ControlType *chkBoxCaught, *chkBoxSeen;

	chkBoxCaught = GetObjectPtr(PkmnMainCaughtCheckbox);
	chkBoxSeen = GetObjectPtr(PkmnMainSeenCheckbox);

	switch (adventureStatus)
	{
	case POKE_ADVENTURE_CAUGHT:
		CtlSetValue(chkBoxCaught, true);
		CtlSetValue(chkBoxSeen, true);
		break;

	case POKE_ADVENTURE_SEEN:
		CtlSetValue(chkBoxCaught, false);
		CtlSetValue(chkBoxSeen, true);
		break;
	
	default:
		CtlSetValue(chkBoxCaught, false);
		CtlSetValue(chkBoxSeen, false);
		break;
	}
}

static void LoadPkmnStats(const struct PokeInfo info, const Boolean adventureModeEnabled, const UInt8 adventureStatus)
{
	FormType *frm;
	frm = FrmGetActiveForm();

	if (adventureModeEnabled && adventureStatus != POKE_ADVENTURE_CAUGHT)
	{
		FrmCopyLabel(frm, PkmnMainHPValueLabel, noStats);
		FrmCopyLabel(frm, PkmnMainAtkValueLabel, noStats);
		FrmCopyLabel(frm, PkmnMainDefValueLabel, noStats);
		FrmCopyLabel(frm, PkmnMainSPAtkValueLabel, noStats);
		FrmCopyLabel(frm, PkmnMainSPDefValueLabel, noStats);
		FrmCopyLabel(frm, PkmnMainSpeedValueLabel, noStats);
	} else {
		SetLabelInfo(PkmnMainHPValueLabel, info.stats.hp, frm);
		SetLabelInfo(PkmnMainAtkValueLabel, info.stats.atk, frm);
		SetLabelInfo(PkmnMainDefValueLabel, info.stats.def, frm);
		SetLabelInfo(PkmnMainSPAtkValueLabel, info.stats.spAtk, frm);
		SetLabelInfo(PkmnMainSPDefValueLabel, info.stats.spDef, frm);
		SetLabelInfo(PkmnMainSpeedValueLabel, info.stats.speed, frm);
	}
}

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	struct PokeInfo info;
	Boolean foundPrefs, adventureModeEnabled;
	PalmkedexPrefs *prefs;
	UInt16 latestPrefSize;
	UInt8 adventureStatus;
	FormType *frm;

	frm = FrmGetActiveForm();

	pokeInfoGet(&info, sharedVars->selectedPkmnId);

	adventureModeEnabled = isAdventureModeEnabled();
	adventureStatus = getPokeAdventureStatus(sharedVars->selectedPkmnId);

	SetAdventureCheckboxes(adventureStatus);
	drawBackButton(PkmnMainBackButton);

	SetFormTitle(sharedVars);
	LoadPkmnStats(info, adventureModeEnabled, adventureStatus);

	if (!adventureModeEnabled || adventureStatus == POKE_ADVENTURE_CAUGHT) {
		DrawTypes(&info);
		FrmShowObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
		FrmShowObject(frm,  FrmGetObjectIndex(frm, PkmnMainTypeButton));
	} else {
		FrmHideObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
		FrmHideObject(frm,  FrmGetObjectIndex(frm, PkmnMainTypeButton));
	}

	if (!adventureModeEnabled ||  adventureStatus != POKE_ADVENTURE_NOT_SEEN) {
		DrawPkmnSprite(sharedVars->selectedPkmnId);
	} else {
		DrawPkmnPlaceholder();
	}

	#ifdef SCREEN_RESIZE_SUPPORT

	if (adventureModeEnabled && adventureStatus != POKE_ADVENTURE_CAUGHT)
		return;

	Coord height, width;
	WinGetWindowExtent(&width, &height);

	if (getDanaMode(width, height) != 0)
	{
		SetDescriptionField(sharedVars->selectedPkmnId);
		DrawTypeEff(sharedVars->selectedPkmnId);
	} else {
		latestPrefSize = sizeof(PalmkedexPrefs);
		prefs = MemPtrNew(latestPrefSize);
		if (!prefs)
		{
			SysFatalAlert("Failed to allocate memory to store preferences!");
		}
		MemSet(prefs, latestPrefSize, 0);

		foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
		if (!foundPrefs)
		{
			SysFatalAlert("Failed to load preferences!");
		}

		if (prefs->mainUnderGraffitiType == 0)
		{
			SetDescriptionField(sharedVars->selectedPkmnId);
		} 
		else if (prefs->mainUnderGraffitiType == 1)
		{
			DrawTypeEff(sharedVars->selectedPkmnId);
		}
		else
		{
			SysFatalAlert("Invalid details type!");
		}
		MemPtrFree(prefs);
	}
	#endif
}

void drawBmpForType(const enum PokeType type, const Coord x, const Coord y, const Boolean icon)
{
	BitmapPtr bitmapP;
	MemHandle h;
	const UInt16 base = icon? POKEMON_TYPE_ICON_IMAGES_BASE : POKEMON_TYPE_IMAGES_BASE;

	h = DmGetResource(bitmapRsc, base + (UInt8)type);
	bitmapP = (BitmapPtr)MemHandleLock(h);

	WinDrawBitmap(bitmapP, x, y);
	MemHandleUnlock(h);
	DmReleaseResource(h);
}

static void DrawTypes(const struct PokeInfo *info)
{
	const Boolean isSingleType = info->type[1] == PokeTypeNone;

	const UInt16 x1 = isHanderaHiRes() ?  POKE_TYPE_1_X_HANDERA : POKE_TYPE_1_X;
	const UInt16 x2 = getType2X();
	const UInt16 y = isHanderaHiRes() ? POKE_TYPE_Y_HANDERA : POKE_TYPE_Y;

	drawBmpForType(info->type[0], x1, y, false);

	if (!isSingleType)
		drawBmpForType(info->type[1], x2, y, false);
}

void SetLabelInfo(const UInt16 labelId, const UInt8 stat, FormType *frm)
{
	Char *str;

	str = (Char *)MemPtrNew(sizeof(Char[4]));
	if ((UInt32)str == 0)
		return;
	MemSet(str, sizeof(Char[4]), 0);

	StrIToA(str, stat);
	FrmCopyLabel(frm, labelId, str);

	MemPtrFree(str);
}

void SetFormTitle(const SharedVariables *sharedVars)
{
	char titleStr[POKEMON_NAME_LEN + 6]; // 6 = space + # + 4nums + null char

	pokeNameGet(titleStr, sharedVars->selectedPkmnId);
	StrCat(titleStr, " #");
	StrIToA(titleStr + StrLen(titleStr), sharedVars->selectedPkmnId);

	FrmCopyTitle(FrmGetActiveForm(), titleStr);
}

static void unregisterCurrentAci(void)
{
	struct DrawState *ds;

	ds = (struct DrawState*)globalsSlotVal(GLOBALS_SLOT_POKE_IMAGE);

	if (ds)
	{
		imgDrawStateFree(ds);
		*globalsSlotPtr(GLOBALS_SLOT_POKE_IMAGE) = NULL;
	}
}

static Boolean isBmpCreateSupported(void)
{
	UInt32 romVersion;
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	return (romVersion >= sysMakeROMVersion(3, 5, 0, sysROMStageRelease, 0));
}

static void toggleQr(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	if (sharedVars->isQrDisplayed)
	{
		DrawPkmnSprite(sharedVars->selectedPkmnId);
		CtlSetLabel(GetObjectPtr(PkmnMainQrCodeButton), "QR Code");
		sharedVars->isQrDisplayed = false;
	}
	else
	{
		CtlSetLabel(GetObjectPtr(PkmnMainQrCodeButton), "Sprite");
		drawQr(sharedVars->selectedPkmnId, isBmpCreateSupported());
		sharedVars->isQrDisplayed = true;
	}
}

static void updatePerPokePrefs(const EventType *eventP)
{
	const SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Boolean foundPrefs;
	PerPokemonPrefs *prefs;
	UInt16 latestPrefSize, pokeID;

	latestPrefSize = sizeof(PerPokemonPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to load per-poke preferences!");
		return;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(prefsCaughtCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		SysFatalAlert("Failed to load per-poke preferences!");
		return;
	}

	pokeID = sharedVars->selectedPkmnId-1;

	if (eventP->data.ctlSelect.controlID == PkmnMainSeenCheckbox)
	{
		modifyPerPokeBit(prefs->seen, pokeID, eventP->data.ctlSelect.on);
		// If caught and marking as unseen, mark as uncaught
		if (checkPerPokeBit(prefs->caught, pokeID) && !eventP->data.ctlSelect.on)
		{
			modifyPerPokeBit(prefs->caught, pokeID, eventP->data.ctlSelect.on);
		}
	} else if (eventP->data.ctlSelect.controlID == PkmnMainCaughtCheckbox) {
		modifyPerPokeBit(prefs->caught, pokeID, eventP->data.ctlSelect.on);
		// If not seen and marking as caught, mark as seen
		if (!checkPerPokeBit(prefs->seen, pokeID) && eventP->data.ctlSelect.on)
		{
			modifyPerPokeBit(prefs->seen, pokeID, eventP->data.ctlSelect.on);
		}

	} else {
		SysFatalAlert("Invalid per-poke checkbox!");
	}

	PrefSetAppPreferencesV10(prefsCaughtCreator, appPrefVersionNum, prefs, latestPrefSize);

	MemPtrFree(prefs);

	if(isAdventureModeEnabled()){
		clearPkmnImage(true);
		drawFormCustomThings();
	} else {
		SetAdventureCheckboxes(getPokeAdventureStatus(sharedVars->selectedPkmnId));
	}
}

static Boolean PkmnMainFormDoCommand(const UInt16 command, const EventType *eventP)
{
	Boolean handled = false;

	switch (command)
	{
	case PkmnMainBackButton:
	{
		GoToPreferredMainForm();
		handled = true;
		break;
	}
	case PkmnMainQrCodeButton:
	{
		toggleQr();
		handled = true;
		break;
	}
	case PkmnMainTypeButton:
	{
		FrmGotoForm(PkmnTypeForm);
		handled = true;
		break;
	}
	case PkmnMainDexEntryButton:
	{
		showDexEntryPopup();
		handled = true;
		break;
	}
	case PkmnMainCaughtCheckbox:
	case PkmnMainSeenCheckbox:
	{
		updatePerPokePrefs(eventP);
		handled = true;
		break;
	}

	default:
		break;
	}

	return handled;
}

#ifdef MAGIC_FOR_EARLY_MONO_DEVICES
	struct m68kLCDC {	//@0xfffffa00
		volatile UInt32 LSSA;
		UInt8 rfu0[1];
		volatile UInt8 LVPW;
		UInt8 rfu1[2];
		volatile UInt16 LXMAX;
		volatile UInt16 LYMAX;
		UInt8 rfu2[12];
		volatile UInt16 LCXP;
		volatile UInt16 LCYP;
		volatile UInt16 LCWCH;
		UInt8 rfu3[1];
		volatile UInt8 LBLKC;
		volatile UInt8 LPICF;
		volatile UInt8 LPOLCF;
		UInt8 rfu4[1];
		volatile UInt8 LACDRC;
		UInt8 rfu5[1];
		volatile UInt8 LPXCD;
		UInt8 rfu6[1];
		volatile UInt8 LCKCON;
		UInt8 rfu7[1];
		volatile UInt8 LLBAR;
		UInt8 rfu8[1];
		volatile UInt8 LOTCR;
		UInt8 rfu9[1];
		volatile UInt8 LPOSR;
		UInt8 rfuA[3];
		volatile UInt8 LFRCM;
		volatile UInt16 LGPMR;
	};

	static const RectangleType imageRect = {.topLeft = {.x = 1, .y = 16}, .extent = {.x = 96, .y = 96, }};
	/*
		There is black magic in play here, tread with caution...
	*/
	static void drawMagicandTrackPenRelease(Int16 x, Int16 y)
	{
		const SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
		struct m68kLCDC *LCDC = (struct m68kLCDC*)0xfffffa00;
		UInt8 prevVPW, prevLBAR, prevLPICF;
		UInt8 *restrict newSSA = NULL;
		Boolean down, success = false;
		UInt32 prevSSA, romVersion;
		MemHandle imgMemHandle;
		DmOpenRef dbRef;
		UInt16 r, c;

		//this hackery is only for OS 1 & 2, which lack greyscale mode!
		if (errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
			return;

		dbRef = DmOpenDatabaseByTypeCreator('pSPR', appFileCreator, dmModeReadOnly);
		if (dbRef) {

			imgMemHandle = DmGet1Resource('pSPT', sharedVars->selectedPkmnId);
			if (imgMemHandle) {
				const MemHandle mh = DmNewHandle(SysCurAppInfoPV20()->dmAccessP, 160u * 160u * 2u / 8u);
				if (mh)
					newSSA = MemHandleLock(mh);
				if (newSSA) {

					struct DrawState *ds = NULL;

					MemSemaphoreReserve(true);
					if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 96, 96, 2)) {
						const volatile UInt8 *src = imgGetBits(ds);
						UInt8 *dst = newSSA;
						#define MAGIC_IMG_W		96
						#define MAGIC_IMG_H		96
						#define MAGIC_SCR_W		160
						#define MAGIC_SCR_H		160
						#define MAGIC_TOP		((MAGIC_SCR_H - MAGIC_IMG_H) / 2)
						#define MAGIC_FRONT		((MAGIC_SCR_W - MAGIC_IMG_W) / 2)
						#define MAGIC_BACK		(MAGIC_SCR_W - MAGIC_IMG_W - MAGIC_FRONT)
						

						MemSet(newSSA, 160u * 160u * 2u / 8u, 0);

						dst += 160 * MAGIC_TOP * 2 / 8;
						for (r = 0; r < MAGIC_IMG_H; r++) {

							dst += MAGIC_FRONT * 2 / 8;
							for (c = 0; c < MAGIC_IMG_W * 2 / 8; c++)
								*dst++ = *src++;
							dst += MAGIC_BACK * 2 / 8;
						}

						prevSSA = LCDC->LSSA;
						prevVPW = LCDC->LVPW;
						prevLBAR = LCDC->LLBAR;
						prevLPICF = LCDC->LPICF;
						LCDC->LCKCON &=~ 0x80;
						SysTaskDelay(2);
						LCDC->LSSA = (UInt32)newSSA;
						LCDC->LLBAR = 20;
						LCDC->LVPW = 20;
						LCDC->LPICF |= 1;
						LCDC->LCKCON |= 0x80;
						SysTaskDelay(2);

						success = true;
						imgDrawStateFree(ds);
					}
					MemSemaphoreRelease(true);
					MemHandleUnlock(imgMemHandle);
					DmReleaseResource(imgMemHandle);
				}
			}
			DmCloseDatabase(dbRef);
		}

		if (success) {

			do {
				EvtGetPen(&x, &y, &down);
			} while (down);

			LCDC->LCKCON &=~ 0x80;
			SysTaskDelay(2);
			LCDC->LPICF = prevLPICF;
			LCDC->LLBAR = prevLBAR;
			LCDC->LVPW = prevVPW;
			LCDC->LSSA = prevSSA;
			LCDC->LCKCON |= 0x80;

			FrmDrawForm(FrmGetActiveForm());
			DrawPkmnSprite(sharedVars->selectedPkmnId);
		}
		if (newSSA)
			MemPtrFree(newSSA);
	}
#endif

static Boolean resizePkmnMainForm(const FormPtr fp)
{
#ifdef SCREEN_RESIZE_SUPPORT
	WinHandle wh = FrmGetWindowHandle(fp);
	Coord newW, newH, oldW, oldH;
	RectangleType rect;
	UInt16 idx, num;

	WinGetDisplayExtent(&newW, &newH);
	wh = WinSetDrawWindow(wh);
	WinGetDrawWindowBounds(&rect);
	wh = WinSetDrawWindow(wh);

	if (rect.extent.x == newW && rect.extent.y == newH)
		return false;

	oldW = rect.extent.x;
	oldH = rect.extent.y;
	rect.extent.x = newW;
	rect.extent.y = newH;
	WinSetBounds(wh, &rect);
	(void)oldH;
	(void)oldW;

	for (idx = 0, num = FrmGetNumberOfObjects(fp); idx < num; idx++) {

		FrmGetObjectBounds(fp, idx, &rect);

		switch (FrmGetObjectId(fp, idx)) {
			case PkmnMainBackButton:
			case PkmnMainTypeButton:
			case PkmnMainDexEntryButton:
			case PkmnMainQrCodeButton:
			case PkmnMainCaughtCheckbox:
			case PkmnMainSeenCheckbox:
			case SeenIconBitmap:
			case CaughtIconBitmap:
				rect.topLeft.x += newW - oldW;
				break;

			default:
				continue;
		}

		FrmSetObjectBounds(fp, idx, &rect);
	}

	return true;
#else
	return false;
#endif
}

static void FreeUsedVariables(void)
{
	unregisterCurrentAci();
	#ifdef SCREEN_RESIZE_SUPPORT
	FreeDescriptionField();
	#endif
	
}


#ifdef SCREEN_RESIZE_SUPPORT
static void clearTypeEffs(void)
{
	RectangleType rect;
	Coord height, width;
	UInt8 danaMode;
	Int16 x, y;
	FormType *frm;

	WinGetWindowExtent(&width, &height);
	if (height <= 160 && width <= 160){
		return;
	}

	danaMode = getDanaMode(width, height);
	frm = FrmGetActiveForm();
	FrmGetObjectPosition(frm, FrmGetObjectIndex(frm, PkmnMainDescriptionField), &x, &y);

	if (danaMode == DANA_LANDSCAPE)
	{
		rect.topLeft.x = getInitialXForTypesMatchup(danaMode);
		rect.topLeft.y = getInitialYForTypesMatchup(y, danaMode);
		rect.extent.x = danaMode == DANA_LANDSCAPE? 385 : width;
		rect.extent.y = GetTypeEffYOffset() * 2;
	} else {
		rect.topLeft.x = 0;
		rect.topLeft.y = y;
		rect.extent.x = width;
		rect.extent.y = height; 
	}

	WinEraseRectangle(&rect, 0);
}
#endif



static void IteratePkmn(const WChar c)
{
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	UInt16 selected = sharedVars->selectedPkmnId;

	if (c == vchrPageUp)
	{
		selected--;
	}
	else if (c == vchrPageDown)
	{
		selected++;
	}

	if (selected == 0)
	{
		selected = TOTAL_POKE_COUNT_ZERO_BASED;
	} 
	else if (selected > TOTAL_POKE_COUNT_ZERO_BASED)
	{
		selected = 1;
	}

	sharedVars->selectedPkmnId = selected;

	if (sharedVars->isQrDisplayed)
		toggleQr();

	clearPkmnImage(true);
	#ifdef SCREEN_RESIZE_SUPPORT
	clearTypeEffs();
	#endif

	FreeUsedVariables();
	drawFormCustomThings();
}

static Boolean isSelectedPokemonInvalid(void)
{
	const SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	return sharedVars->selectedPkmnId == 0 || sharedVars->selectedPkmnId > TOTAL_POKE_COUNT_ZERO_BASED;
}

Boolean PkmnMainFormHandleEvent(const EventType *eventP)
{
	FormType *frmP = FrmGetActiveForm();
	Boolean handled = false;
	UInt32 pinsVersion;

	switch (eventP->eType)
	{
#ifdef MAGIC_FOR_EARLY_MONO_DEVICES
	case penDownEvent:
		if (RctPtInRectangle(eventP->screenX, eventP->screenY, &imageRect)) {
			drawMagicandTrackPenRelease(eventP->screenX, eventP->screenY);
			return true;
		}
#endif
		break;

	case ctlSelectEvent:
		return PkmnMainFormDoCommand(eventP->data.ctlSelect.controlID, eventP);

	case frmOpenEvent:
		if (isSelectedPokemonInvalid()) {
			ErrAlertCustom(0, "You tried opening an invalid pokemon!", NULL, NULL);
			GoToPreferredMainForm();
			return true;
		}
			
		if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
			FrmSetDIAPolicyAttr(frmP, frmDIAPolicyCustom);
			WinSetConstraintsSize(FrmGetWindowHandle(frmP), 160, 240, 640, 160, 240, 640);
			PINSetInputTriggerState(pinInputTriggerEnabled);
		}
#ifdef HANDERA_SUPPORT
		if (isHanderaHiRes())
			VgaFormModify(frmP, vgaFormModify160To240);
#endif
		resizePkmnMainForm(frmP);
		FrmDrawForm(FrmGetActiveForm());
		drawFormCustomThings();
		handled = true;
		break;

	case keyDownEvent:
	 	if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
		{
			IteratePkmn(eventP->data.keyDown.chr); // TODO: ADD HANDERA JOG SUPPORT AS WELL!
			handled = true;
		}

		break;

	case frmCloseEvent:
		//no matter why we're closing, free things we allocated
		FreeUsedVariables();
		break;

	case winEnterEvent:
		if (isHanderaHiRes())	//fallthrough except for handera
			break;
		//fallthrough

#ifdef HANDERA_SUPPORT
	case displayExtentChangedEvent:
#endif
	case winDisplayChangedEvent:
	case frmUpdateEvent:
		if (resizePkmnMainForm(frmP)) {
			WinEraseWindow();
			FrmDrawForm(frmP);
			drawFormCustomThings();
		}
		return true;

	default:
		break;
	}

	return handled;
}
