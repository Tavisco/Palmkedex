#include "BUILD_TYPE.h"

#include <PalmOS.h>
#include <stdarg.h>

#include "Palmkedex.h"
#include "UiResourceIDs.h"
#include "pokeInfo.h"
#include "imgDraw.h"
#include "osExtra.h"
#include "qrcode/qrcode.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_IMAGE_AT_X				1
#define POKE_IMAGE_AT_X_HANDERA		1
#define POKE_IMAGE_AT_Y				16
#define POKE_IMAGE_AT_Y_HANDERA		24

#define POKE_IMAGE_SIZE				96
#define POKE_IMAGE_SIZE_HANDERA		144

#define POKE_TYPE_1_X				1
#define POKE_TYPE_1_X_HANDERA		1
#define POKE_TYPE_2_X				34
#define POKE_TYPE_2_X_HRES			52
#define POKE_TYPE_2_X_HANDERA		51

#define POKE_TYPE_Y					113
#define POKE_TYPE_Y_HANDERA			175
#define POKE_TYPE_HEIGHT			20
#define POKE_TYPE_HEIGHT_HANDERA	30

#define QR_OFFSET_X					3
#define QR_OFFSET_X_HANDERA			12
#define QR_OFFSET_Y					6
#define QR_OFFSET_Y_HANDERA			17
#define QR_MODULE_SIZE				3
#define QR_MODULE_SIZE_HANDERA		4

static const char emptyString[1] = {0};	//needed for PalmOS under 4.0 as we cannot pass NULL to FldSetTextPtr

static void DrawTypes(const struct PokeInfo *info);

static UInt16 getType2X(void)
{
	switch (getScreenDensity())
	{
	case kDensityDouble:
		return POKE_TYPE_2_X_HRES;
		break;
	
	case kDensityOneAndAHalf:
		return POKE_TYPE_2_X_HANDERA;
		break;
	default:
		return POKE_TYPE_2_X;
		break;
	}
}

static void showDexEntryPopup(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	char *dexEntry = NULL;

	dexEntry = pokeDescrGet(sharedVars->selectedPkmnId);

	if (dexEntry == NULL)
	{
		FrmCustomAlert(DexEntryAlert, "This pokemon has no Dex Entry.", "", "");
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

static void clearPkmnImage(Boolean includeTypes)
{
	RectangleType rect;
	int pokeImageSize = isHanderaHiRes() ? POKE_IMAGE_SIZE_HANDERA : POKE_IMAGE_SIZE;
	int pokeTypeHeight = isHanderaHiRes() ? POKE_TYPE_HEIGHT_HANDERA : POKE_TYPE_HEIGHT;

	rect.topLeft.x = isHanderaHiRes() ? POKE_IMAGE_AT_X_HANDERA : POKE_IMAGE_AT_X;
	rect.topLeft.y = isHanderaHiRes() ? POKE_IMAGE_AT_Y_HANDERA : POKE_IMAGE_AT_Y;
	rect.extent.x = pokeImageSize + 10;

	rect.extent.y = includeTypes ?  pokeImageSize + pokeTypeHeight : pokeImageSize; 
	WinEraseRectangle(&rect, 0);
}

static void drawQr(UInt16 selectedPkmnId, bool useBitmap)
{
	char url[43];
	char pokeName[POKEMON_NAME_LEN + 1];

	QRCode *qrcode;
	uint8_t* qrcodeData;
	RectangleType bounds;
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

	for (int y = 0; y < qrcode->size; y++) {
		for (int x = 0; x < qrcode->size; x++) {
			if (qrcode_getModule(qrcode, x, y)) {
				rect.topLeft.x = x * moduleSize + qrModifierX;
				rect.topLeft.y = y * moduleSize + qrModifierY;
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

static void DrawPkmnSprite(UInt16 selectedPkmnId)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;
	BitmapType *bmpP;
	MemPtr pngData;
	WinHandle win;
	UInt32 size;
	Err error;
	int ret;

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

	FldSetTextPtr(fld, (char*)emptyString);

	if (ptr && ptr != (char*)emptyString){
		MemPtrFree(ptr);
	}
		
}

static void SetDescriptionField(UInt16 selectedPkmnId)
{
	Coord width, height;
	FieldType *fld;
	RectangleType rect;
	FormType *frm;

	// Don't try to set description on devices that can't show them...
	frm = FrmGetActiveForm();
	WinGetWindowExtent(&width, &height);

	if (height <= 160 && width <= 160){
		FrmShowObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
		FreeDescriptionField();
		return;
	}

	fld = GetObjectPtr(PkmnMainDescriptionField);
	// Special case for Dana, where the width is is larger than height
	if (height == 160 && width > 320)
	{
		rect.topLeft.x = POKE_IMAGE_AT_X + POKE_IMAGE_SIZE + 10;
		rect.extent.x = width - rect.topLeft.x - 59;
		rect.topLeft.y = 22;
		rect.extent.y = 58;
		FldSetBounds(fld, &rect);
	}

	char *text = pokeDescrGet(selectedPkmnId);

	if (!text)
		text = (char*)emptyString;

	FreeDescriptionField();
	FldSetTextPtr(fld, text);
	FldRecalculateField(fld, true);

	FrmHideObject(frm,  FrmGetObjectIndex(frm, PkmnMainDexEntryButton));
	CtlSetUsable(GetObjectPtr(PkmnMainDexEntryButton), false);
}

static void DrawTypeEff(UInt16 selectedPkmnId)
{
	Boolean keepDrawing = true;
	Coord height, width;
	Int16 x, y;
	UInt8 i;

	// Don't try to set description on devices that can't show them...
	WinGetWindowExtent(&width, &height);

	if (height <= 160 && width <= 160){
		return;
	}

	x = 0;
	y = 160;

	for (i = PokeTypeFirst; i <= PokeTypeFairy; i++)
	{
		if (!DrawEffectiveness(selectedPkmnId, x + 17, y, (enum PokeType)i, true))
			continue;

		drawBmpForType(i, x, y, true);

		x += 41;

		if (x >= width)
		{
			x = 0;
			y += 17;
		}
	}
}
#endif

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	struct PokeInfo info;
	Boolean foundPrefs;
	struct PalmkedexPrefs *prefs;
	UInt16 latestPrefSize;

	drawBackButton(PkmnMainBackButton);

	pokeInfoGet(&info, sharedVars->selectedPkmnId);

	DrawTypes(&info);
	DrawPkmnSprite(sharedVars->selectedPkmnId);

	#ifdef SCREEN_RESIZE_SUPPORT
	Coord height, width;

	latestPrefSize = sizeof(struct PalmkedexPrefs);
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
	#endif
}

void LoadPkmnStats(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	struct PokeInfo info;
	FormType *frm;

	pokeInfoGet(&info, sharedVars->selectedPkmnId);

	frm = FrmGetActiveForm();

	SetFormTitle(sharedVars);

	SetLabelInfo(PkmnMainHPValueLabel, info.stats.hp, frm);
	SetLabelInfo(PkmnMainAtkValueLabel, info.stats.atk, frm);
	SetLabelInfo(PkmnMainDefValueLabel, info.stats.def, frm);
	SetLabelInfo(PkmnMainSPAtkValueLabel, info.stats.spAtk, frm);
	SetLabelInfo(PkmnMainSPDefValueLabel, info.stats.spDef, frm);
	SetLabelInfo(PkmnMainSpeedValueLabel, info.stats.speed, frm);
}

void drawBmpForType(enum PokeType type, Coord x, Coord y, Boolean icon)
{
	BitmapPtr bitmapP;
	MemHandle h;
	UInt16 base = icon? POKEMON_TYPE_ICON_IMAGES_BASE : POKEMON_TYPE_IMAGES_BASE;

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

void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm)
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

void SetFormTitle(SharedVariables *sharedVars)
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


static Boolean PkmnMainFormDoCommand(UInt16 command)
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
		SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
		struct m68kLCDC *LCDC = (struct m68kLCDC*)0xfffffa00;
		UInt8 prevVPW, prevLBAR, prevLPICF;
		UInt8 *restrict newSSA = NULL;
		Boolean down, success = false;
		UInt32 prevSSA, romVersion;
		MemHandle imgMemHandle;
		DmOpenRef dbRef;
		UInt16 i, r, c;

		//this hackery is only for OS 1 & 2, which lack greyscale mode!
		if (errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
			return;

		dbRef = DmOpenDatabaseByTypeCreator('pSPR', appFileCreator, dmModeReadOnly);
		if (dbRef) {

			imgMemHandle = DmGet1Resource('pSPT', sharedVars->selectedPkmnId);
			if (imgMemHandle) {

				MemHandle mh = DmNewHandle((DmOpenRef)(SysCurAppInfoPV20()->dmAccessP), 160u * 160u * 2u / 8u);
				if (mh)
					newSSA = MemHandleLock(mh);
				if (newSSA) {

					struct DrawState *ds = NULL;

					MemSemaphoreReserve(true);
					if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 96, 96, 2)) {

						static const UInt16 expandTab[] = {0x0000, 0x0005, 0x000a, 0x000f, 0x0050, 0x0055, 0x005a, 0x005f, 0x00a0, 0x00a5, 0x00aa, 0x00af, 0x00f0, 0x00f5, 0x00fa, 0x00ff, 0x0500, 0x0505, 0x050a, 0x050f, 0x0550, 0x0555, 0x055a, 0x055f, 0x05a0, 0x05a5, 0x05aa, 0x05af, 0x05f0, 0x05f5, 0x05fa, 0x05ff, 0x0a00, 0x0a05, 0x0a0a, 0x0a0f, 0x0a50, 0x0a55, 0x0a5a, 0x0a5f, 0x0aa0, 0x0aa5, 0x0aaa, 0x0aaf, 0x0af0, 0x0af5, 0x0afa, 0x0aff, 0x0f00, 0x0f05, 0x0f0a, 0x0f0f, 0x0f50, 0x0f55, 0x0f5a, 0x0f5f, 0x0fa0, 0x0fa5, 0x0faa, 0x0faf, 0x0ff0, 0x0ff5, 0x0ffa, 0x0fff, 0x5000, 0x5005, 0x500a, 0x500f, 0x5050, 0x5055, 0x505a, 0x505f, 0x50a0, 0x50a5, 0x50aa, 0x50af, 0x50f0, 0x50f5, 0x50fa, 0x50ff, 0x5500, 0x5505, 0x550a, 0x550f, 0x5550, 0x5555, 0x555a, 0x555f, 0x55a0, 0x55a5, 0x55aa, 0x55af, 0x55f0, 0x55f5, 0x55fa, 0x55ff, 0x5a00, 0x5a05, 0x5a0a, 0x5a0f, 0x5a50, 0x5a55, 0x5a5a, 0x5a5f, 0x5aa0, 0x5aa5, 0x5aaa, 0x5aaf, 0x5af0, 0x5af5, 0x5afa, 0x5aff, 0x5f00, 0x5f05, 0x5f0a, 0x5f0f, 0x5f50, 0x5f55, 0x5f5a, 0x5f5f, 0x5fa0, 0x5fa5, 0x5faa, 0x5faf, 0x5ff0, 0x5ff5, 0x5ffa, 0x5fff, 0xa000, 0xa005, 0xa00a, 0xa00f, 0xa050, 0xa055, 0xa05a, 0xa05f, 0xa0a0, 0xa0a5, 0xa0aa, 0xa0af, 0xa0f0, 0xa0f5, 0xa0fa, 0xa0ff, 0xa500, 0xa505, 0xa50a, 0xa50f, 0xa550, 0xa555, 0xa55a, 0xa55f, 0xa5a0, 0xa5a5, 0xa5aa, 0xa5af, 0xa5f0, 0xa5f5, 0xa5fa, 0xa5ff, 0xaa00, 0xaa05, 0xaa0a, 0xaa0f, 0xaa50, 0xaa55, 0xaa5a, 0xaa5f, 0xaaa0, 0xaaa5, 0xaaaa, 0xaaaf, 0xaaf0, 0xaaf5, 0xaafa, 0xaaff, 0xaf00, 0xaf05, 0xaf0a, 0xaf0f, 0xaf50, 0xaf55, 0xaf5a, 0xaf5f, 0xafa0, 0xafa5, 0xafaa, 0xafaf, 0xaff0, 0xaff5, 0xaffa, 0xafff, 0xf000, 0xf005, 0xf00a, 0xf00f, 0xf050, 0xf055, 0xf05a, 0xf05f, 0xf0a0, 0xf0a5, 0xf0aa, 0xf0af, 0xf0f0, 0xf0f5, 0xf0fa, 0xf0ff, 0xf500, 0xf505, 0xf50a, 0xf50f, 0xf550, 0xf555, 0xf55a, 0xf55f, 0xf5a0, 0xf5a5, 0xf5aa, 0xf5af, 0xf5f0, 0xf5f5, 0xf5fa, 0xf5ff, 0xfa00, 0xfa05, 0xfa0a, 0xfa0f, 0xfa50, 0xfa55, 0xfa5a, 0xfa5f, 0xfaa0, 0xfaa5, 0xfaaa, 0xfaaf, 0xfaf0, 0xfaf5, 0xfafa, 0xfaff, 0xff00, 0xff05, 0xff0a, 0xff0f, 0xff50, 0xff55, 0xff5a, 0xff5f, 0xffa0, 0xffa5, 0xffaa, 0xffaf, 0xfff0, 0xfff5, 0xfffa, 0xffff, };
						const volatile UInt8 *src = imgGetBits(ds);
						UInt8 *dst = (UInt8*)newSSA;
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

static Boolean resizePkmnMainForm(FormPtr fp)
{
#ifdef SCREEN_RESIZE_SUPPORT
	WinHandle wh = FrmGetWindowHandle(fp);
	Coord newW, newH, oldW, oldH;
	FieldPtr field = NULL;
	RectangleType rect;
	UInt32 romVersion;
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
				rect.topLeft.x += newW - oldW;
				break;

			default:
				continue;
		}

		FrmSetObjectBounds(fp, idx, &rect);
	}
	if (field)
		FldRecalculateField(field, true /* we do not need the redraw but before PalmOs 4.0, without it, no recalculation takes place */);
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

	WinGetWindowExtent(&width, &height);
	if (height <= 160 && width <= 160){
		return;
	}

	rect.topLeft.x = 0;
	rect.topLeft.y = 160;
	rect.extent.x = width;
	rect.extent.y = height; 
	WinEraseRectangle(&rect, 0);
}
#endif

static void IteratePkmn(WChar c)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

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
	LoadPkmnStats();
	drawFormCustomThings();
}

static Boolean isSelectedPokemonInvalid(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	return sharedVars->selectedPkmnId == 0 || sharedVars->selectedPkmnId > TOTAL_POKE_COUNT_ZERO_BASED;
}

Boolean PkmnMainFormHandleEvent(EventType *eventP)
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
		return PkmnMainFormDoCommand(eventP->data.ctlSelect.controlID);

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
		LoadPkmnStats();
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
