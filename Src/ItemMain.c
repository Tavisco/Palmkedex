//
// Created by tavisco on 07/07/2026.
//

#include "BUILD_TYPE.h"

#include <PalmOS.h>
#include <stdarg.h>

#include "Palmkedex.h"
#include "UiResourceIDs.h"
#include "pokeInfo.h"
#include "imgDraw.h"
#include "osExtra.h"
#include "qrcode.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define ITEM_ICON_SIZE						32
#define POKE_ICON_SIZE_HANDERA				60
#define POKE_IMAGE_AT_X					64
#define POKE_IMAGE_AT_X_HANDERA			1
#define POKE_IMAGE_AT_Y					16
#define POKE_IMAGE_AT_Y_HANDERA			24

#define DANA_POTRAIT					1
#define DANA_LANDSCAPE					2

static const char noDexEntryString[28] = "This item has no Dex Entry.";

static void clearItemImage(void)
{
	RectangleType rect;
	int itemImageSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : ITEM_ICON_SIZE;

	rect.topLeft.x = isHanderaHiRes() ? POKE_IMAGE_AT_X_HANDERA : POKE_IMAGE_AT_X;
	rect.topLeft.y = isHanderaHiRes() ? POKE_IMAGE_AT_Y_HANDERA : POKE_IMAGE_AT_Y;
	rect.extent.x = itemImageSize + 10;

	WinEraseRectangle(&rect, 0);
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

static void FreeDescriptionField(void)
{
	FieldType *fld = GetObjectPtr(ItemsFormDescriptionField);
	Char *ptr = FldGetTextPtr(fld);

	FldSetTextPtr(fld, (char*)noDexEntryString);

	if (ptr && ptr != (char*)noDexEntryString){
		MemPtrFree(ptr);
	}
}

static void FreeUsedVariables(void)
{
	unregisterCurrentAci();
	FreeDescriptionField();
}

void SetItemMainFormTitle(SharedVariables *sharedVars)
{
	char titleStr[POKEMON_NAME_LEN + 6]; // 6 = space + # + 4nums + null char

	itemNameGet(titleStr, sharedVars->selectedPkmnId);
	StrCat(titleStr, " #");
	StrIToA(titleStr + StrLen(titleStr), sharedVars->selectedPkmnId);

	FrmCopyTitle(FrmGetActiveForm(), titleStr);
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

static void DrawItemSprite(UInt16 selectedItemId)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;

	// Check if there is any image for current pkmn
	imgMemHandle = pokeImageGet(selectedItemId, ITEM_ICON);
	if (imgMemHandle) {
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), ITEM_ICON_SIZE, ITEM_ICON_SIZE, 0))
			redrawDecodedSprite(ds);
		else
			ds = NULL;
		MemHandleUnlock(imgMemHandle);
		pokeImageRelease(imgMemHandle, ITEM_ICON);
	}

	if (!ds)
		DrawPkmnPlaceholder();

}

static UInt8 getDanaMode(Coord width, Coord height)
{
	if (height == 160 && width > 320)
		return DANA_LANDSCAPE;

	if (width == 160 && height > 320)
		return DANA_POTRAIT;

	return 0;
}

static void SetDescriptionField(UInt16 selectedItemID)
{
	Coord width, height;
	FieldType *fld;
	RectangleType rect;
	UInt8 danaMode;
	MemHandle hndl;
	char *dexEntry = NULL;

	WinGetWindowExtent(&width, &height);

	fld = GetObjectPtr(ItemsFormDescriptionField);

	danaMode = getDanaMode(width, height);

	if (danaMode == DANA_LANDSCAPE)
	{
		rect.topLeft.x = POKE_IMAGE_AT_X + ITEM_ICON_SIZE + 10;
		rect.extent.x = width - rect.topLeft.x - 59;
		rect.topLeft.y = 22;
		rect.extent.y = 58;
		FldSetBounds(fld, &rect);
	}

	DmOpenRef dbRef = DmOpenDatabaseByTypeCreator('ITEM', appFileCreator, dmModeReadOnly);
	if (!dbRef)
	{
		ErrFatalDisplay("Failed to find item database!");
		return;
	}

	hndl = DmGet1Resource('DESC', 0);
	dexEntry = pokeDescrGet(hndl, selectedItemID);

	if (!dexEntry)
		dexEntry = (char*)noDexEntryString;

	FreeDescriptionField();
	FldSetTextPtr(fld, dexEntry);
	FldRecalculateField(fld, true);
	DmCloseDatabase(dbRef);
}

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	drawBackButton(ItemsFormBackButton);
	SetItemMainFormTitle(sharedVars);
	DrawItemSprite(sharedVars->selectedPkmnId);
	SetDescriptionField(sharedVars->selectedPkmnId);
}

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
		selected = TOTAL_ITEM_COUNT_ZERO_BASED;
	}
	else if (selected > TOTAL_ITEM_COUNT_ZERO_BASED)
	{
		selected = 1;
	}

	sharedVars->selectedPkmnId = selected;

	clearItemImage();
	FreeUsedVariables();
	drawFormCustomThings();
}

static Boolean PkmnMainFormDoCommand(UInt16 command, EventType *eventP)
{
	Boolean handled = false;

	switch (command)
	{
		case ItemsFormBackButton:
		{
			GoToPreferredMainForm();
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

static Boolean isSelectedPokemonInvalid(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	return sharedVars->selectedPkmnId == 0 || sharedVars->selectedPkmnId > TOTAL_POKE_COUNT_ZERO_BASED;
}

Boolean ItemMainFormHandleEvent(EventType *eventP)
{
	FormType *frmP = FrmGetActiveForm();
	Boolean handled = false;
	UInt32 pinsVersion;

	switch (eventP->eType)
	{
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
		// resizePkmnMainForm(frmP);
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

// #ifdef HANDERA_SUPPORT
// 	case displayExtentChangedEvent:
// #endif
	// case winDisplayChangedEvent:
	// case frmUpdateEvent:
		// if (resizePkmnMainForm(frmP)) {
		// 	WinEraseWindow();
		// 	FrmDrawForm(frmP);
		// 	drawFormCustomThings();
		// }
		// return true;

	default:
		break;
	}

	return handled;
}